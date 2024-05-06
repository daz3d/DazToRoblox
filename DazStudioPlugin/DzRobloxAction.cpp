#define COMBINED_UVSET_STRING "Combined Head And Body"
#define R15_POSTFIX_STRING "_R15_reminder_adjust_cage_and_attachments"
#define S1_POSTFIX_STRING "_S1_ready_for_avatar_autosetup"

#include <QtGui/qcheckbox.h>
#include <QtGui/QMessageBox>
#include <QtNetwork/qudpsocket.h>
#include <QtNetwork/qabstractsocket.h>
#include <QCryptographicHash>
#include <QtCore/qdir.h>

#include <dzapp.h>
#include <dzscene.h>
#include <dzmainwindow.h>
#include <dzshape.h>
#include <dzproperty.h>
#include <dzobject.h>
#include <dzpresentation.h>
#include <dznumericproperty.h>
#include <dzimageproperty.h>
#include <dzcolorproperty.h>
#include <dpcimages.h>

#include "QtCore/qmetaobject.h"
#include "dzmodifier.h"
#include "dzgeometry.h"
#include "dzweightmap.h"
#include "dzfacetshape.h"
#include "dzfacetmesh.h"
#include "dzfacegroup.h"
#include "dzprogress.h"
#include "dzscript.h"
#include "dzenumproperty.h"
#include "dzcontentmgr.h"
#include "dzfigure.h"

#include "DzRobloxAction.h"
#include "DzRobloxDialog.h"
#include "DzBridgeMorphSelectionDialog.h"
#include "DzBridgeSubdivisionDialog.h"

#ifdef WIN32
#include <shellapi.h>
#endif

#include "zip.h"

#include "dzbridge.h"
#include "OpenFBXInterface.h"
#include "FbxTools.h"
#include "MvcTools.h"

DzRobloxAction::DzRobloxAction() :
	DzBridgeAction(tr("&Roblox Avatar Exporter"), tr("Export the selected character for Roblox Studio."))
{
	m_nNonInteractiveMode = 0;
//	m_sAssetType = QString("__");

	//Setup Icon
	QString iconName = "Daz to Roblox";
	QPixmap basePixmap = QPixmap::fromImage(getEmbeddedImage(iconName.toLatin1()));
	QIcon icon;
	icon.addPixmap(basePixmap, QIcon::Normal, QIcon::Off);
	QAction::setIcon(icon);

	m_bConvertToPng = true;
	m_bConvertToJpg = false;
	m_bExportAllTextures = true;
	m_bCombineDiffuseAndAlphaMaps = false;
	m_bResizeTextures = true;
	m_qTargetTextureSize = QSize(1024, 1024);
	m_bMultiplyTextureValues = false;
	m_bRecompressIfFileSizeTooBig = false;
	m_nFileSizeThresholdToInitiateRecompression = 1024 * 1024 * 1;

}

bool DzRobloxAction::createUI()
{
	// Check if the main window has been created yet.
	// If it hasn't, alert the user and exit early.
	DzMainWindow* mw = dzApp->getInterface();
	if (!mw)
	{
		if (m_nNonInteractiveMode == 0) QMessageBox::warning(0, tr("Error"),
			tr("The main window has not been created yet."), QMessageBox::Ok);

		return false;
	}

	// m_subdivisionDialog creation REQUIRES valid Character or Prop selected
	if (dzScene->getNumSelectedNodes() != 1)
	{
		if (m_nNonInteractiveMode == 0) QMessageBox::warning(0, tr("Error"),
			tr("Please select one Character to send."), QMessageBox::Ok);

		return false;
	}

	 // Create the dialog
	if (!m_bridgeDialog)
	{
		m_bridgeDialog = new DzRobloxDialog(mw);
	}
	else
	{
		DzRobloxDialog* robloxDialog = qobject_cast<DzRobloxDialog*>(m_bridgeDialog);
		if (robloxDialog)
		{
			robloxDialog->resetToDefaults();
			robloxDialog->loadSavedSettings();
		}
	}

	if (!m_subdivisionDialog) m_subdivisionDialog = DZ_BRIDGE_NAMESPACE::DzBridgeSubdivisionDialog::Get(m_bridgeDialog);
	if (!m_morphSelectionDialog) m_morphSelectionDialog = DZ_BRIDGE_NAMESPACE::DzBridgeMorphSelectionDialog::Get(m_bridgeDialog);

	return true;
}

#include <vector>

bool DzRobloxAction::mergeScenes(FbxScene* pDestinationScene, FbxScene* pSourceScene)
{

	deepCopyNode(pDestinationScene->GetRootNode(), pSourceScene->GetRootNode());

	pSourceScene->GetRootNode()->DisconnectAllSrcObject();

	int lNumSceneObjects = pSourceScene->GetSrcObjectCount();
	for (int i = 0; i < lNumSceneObjects; i++) {
		FbxObject* lObj = pSourceScene->GetSrcObject(i);
		if (lObj == pSourceScene->GetRootNode() || *lObj == pSourceScene->GetGlobalSettings()) {
			// Don't move the root node or the scene's global settings; these
			// objects are created for every scene.
			continue;
		}
		/*************************/
		// DEBUG
		FbxObject* pObjGlobalSettings = &pSourceScene->GetGlobalSettings();
		QString globalSettingsName = QString(pObjGlobalSettings->GetName());
		QString objName = QString(lObj->GetName());
		if (objName == "GlobalSettings")
			continue;
		if (lObj->GetClassId() == FbxAnimCurveNode::ClassId ||
			lObj->GetClassId() == FbxAnimCurve::ClassId ||
			lObj->GetClassId() == FbxAnimLayer::ClassId ||
			lObj->GetClassId() == FbxAnimStack::ClassId ||
			lObj->GetClassId() == FbxAnimEvalClassic::ClassId ||
			lObj->GetClassId() == FbxSkeleton::ClassId)
		{
			printf("DEBUG: skipping FbxAnimCurve, FbxAnimCurveNode");
			continue;
		}
		FbxClassId classID = lObj->GetClassId();
		QString className = classID.GetName();
		dzApp->log("DzRoblox DEBUG: adding object=" + objName + " [" + className + "]  to destination scene.");
		/*************************/

		// Attach the object to the reference scene.
		lObj->ConnectDstObject(pDestinationScene);

	}

	pSourceScene->DisconnectAllSrcObject();

	return true;
}

bool DzRobloxAction::deepCopyNode(FbxNode* pDestinationRoot, FbxNode* pSourceNode)
{
	// get children
	if (!pSourceNode)
	{
		dzApp->log("DzRoblox-MvcTest: deepCopyNode: pSourceNode is null.");
		return false;
	}
	if (!pDestinationRoot)
	{
		dzApp->log("DzRoblox-MvcTest: deepCopyNode: pDestinationRoot is null.");
		return false;
	}

	std::vector<FbxNode*> lChildren;
	int lNumChildren = pSourceNode->GetChildCount();
	int debug_pdestinationroot_numchildren = pDestinationRoot->GetChildCount();
	for (int i = 0; i < lNumChildren; i++) {

		// Obtain a child node from the currently loaded scene.
		lChildren.push_back(pSourceNode->GetChild(i));

		// Attach the child node to the reference scene's root node.
//		int debug_lchildren_size = lChildren.size();
//		for (int c = 0; c < lChildren.size(); c++)
//			pDestinationRoot->AddChild(lChildren[c]);
	}
	int debug_lchildren_size = lChildren.size();
	for (int c = 0; c < lChildren.size(); c++)
		pDestinationRoot->AddChild(lChildren[c]);

	int debug_pdestinationroot_numchildren_2 = pDestinationRoot->GetChildCount();

	/*
	{
		FbxGeometry* pCageGeo = openFbx->FindGeometry(pTemplateScene, sNodeName);
		if (!pCageGeo)
		{
			bFailed = true;
			dzApp->log("DzRoblox-MvcTest: Unable to extract cage mesh from template scene.");
		}
		else
		{
			// copy geo
			bResult = pDestinationRoot->AddGeometry(pCageGeo);
			if (!bResult)
			{
				bFailed = true;
				dzApp->log("DzRoblox-MvcTest: Unable to copy cage mesh to template scene.");
			}
			// copy node
			bResult = pDestinationRoot->AddNode(pCageNode);
			if (!bResult)
			{
				bFailed = true;
				dzApp->log("DzRoblox-MvcTest: Unable to copy cage node to template scene.");
			}
		}
	}
	*/

	return true;
}

void DzRobloxAction::executeAction()
{
	// CreateUI() disabled for debugging -- 2022-Feb-25
	/*
		 // Create and show the dialog. If the user cancels, exit early,
		 // otherwise continue on and do the thing that required modal
		 // input from the user.
		 if (createUI() == false)
			 return;
	*/

	// Check if the main window has been created yet.
	// If it hasn't, alert the user and exit early.
	DzMainWindow* mw = dzApp->getInterface();
	if (!mw)
	{
		if (m_nNonInteractiveMode == 0)
		{
			QMessageBox::warning(0, tr("Error"),
				tr("The main window has not been created yet."), QMessageBox::Ok);
		}
		return;
	}

	// Create and show the dialog. If the user cancels, exit early,
	// otherwise continue on and do the thing that required modal
	// input from the user.
	if (dzScene->getNumSelectedNodes() != 1)
	{
		DzNodeList rootNodes = buildRootNodeList();
		if (rootNodes.length() == 1)
		{
			dzScene->setPrimarySelection(rootNodes[0]);
		}
		else if (rootNodes.length() > 1)
		{
			if (m_nNonInteractiveMode == 0)
			{
				QMessageBox::warning(0, tr("Error"),
					tr("Please select one Character to send."), QMessageBox::Ok);
			}
		}
	}

	// if UV is not default, then issue error and return
	// 1. get UV of selected node
	DzNode* testNode = dzScene->getPrimarySelection();
	if (testNode) {
		if (testNode->inherits("DzFigure")) {
			// hardcode check for Genesis 9, fail gracefully if Genesis 8
			if (testNode->getName() != "Genesis9") {
				if (m_nNonInteractiveMode == 0)
				{
					QMessageBox::warning(0, tr("Error"),
						tr("Please make sure you have selected the root node of a Genesis 9 character. ") +
						tr("Only Genesis 9 characters are currently supported."), QMessageBox::Ok);
				}
				return;
			}

			// 2. get UV property
			DzObject* obj = testNode->getObject();
			if (obj) {
				DzShape* shape = obj->getCurrentShape();
				QObjectList rawList = shape->getAllMaterials();
				for (int i = 0; i < rawList.count(); i++) {
					DzMaterial* mat = qobject_cast<DzMaterial*>(rawList[i]);
					if (!mat) continue;
					DzProperty* prop = mat->findProperty("UV Set");
					QString debugName = prop->getName();
					DzEnumProperty* uvset = qobject_cast<DzEnumProperty*>(prop);
					if (uvset) {
						int combinedUvVal = uvset->findItemString(COMBINED_UVSET_STRING);
						int currentVal = uvset->getValue();
						if (currentVal == combinedUvVal) {
							QMessageBox::warning(0, tr("Warning"),
								tr("A non-standard UV Set was detected, overlay will NOT be applied."), QMessageBox::Ok);
							break;
						}
					}
				}
			}
		}
		else {
			// gracefully exit, primary selection must be character (DzFigure)
			if (m_nNonInteractiveMode == 0)
			{
				QMessageBox::warning(0, tr("Error"),
					tr("Please make sure you have selected the root node of a Genesis 9 character. ") +
					tr("Only Genesis 9 characters are currently supported."), QMessageBox::Ok);
			}
			return;

		}
	} 

	// Create the dialog
	if (m_bridgeDialog == nullptr)
	{
		m_bridgeDialog = new DzRobloxDialog(mw);
	}
	else
	{
		if (m_nNonInteractiveMode == 0)
		{
			m_bridgeDialog->resetToDefaults();
			m_bridgeDialog->loadSavedSettings();
		}
	}

	// Prepare member variables when not using GUI
	if (m_nNonInteractiveMode == 1)
	{
//		if (m_sRootFolder != "") m_bridgeDialog->getIntermediateFolderEdit()->setText(m_sRootFolder);

		if (m_aMorphListOverride.isEmpty() == false)
		{
			m_bEnableMorphs = true;
			m_sMorphSelectionRule = m_aMorphListOverride.join("\n1\n");
			m_sMorphSelectionRule += "\n1\n.CTRLVS\n2\nAnything\n0";
			if (m_morphSelectionDialog == nullptr)
			{
				m_morphSelectionDialog = DZ_BRIDGE_NAMESPACE::DzBridgeMorphSelectionDialog::Get(m_bridgeDialog);
			}
//			m_mMorphNameToLabel.clear();
			m_MorphNamesToExport.clear();
			foreach(QString morphName, m_aMorphListOverride)
			{
				QString label = m_morphSelectionDialog->GetMorphLabelFromName(morphName);
//				m_mMorphNameToLabel.insert(morphName, label);
				m_MorphNamesToExport.append(morphName);
			}
		}
		else
		{
			m_bEnableMorphs = false;
			m_sMorphSelectionRule = "";
//			m_mMorphNameToLabel.clear();
			m_MorphNamesToExport.clear();
		}

	}

	// If the Accept button was pressed, start the export
	int dlgResult = -1;
	if (m_nNonInteractiveMode == 0)
	{
		qobject_cast<DzRobloxDialog*>(m_bridgeDialog)->disableAcceptUntilAllRequirementsValid();
		dlgResult = m_bridgeDialog->exec();
	}
	if (m_nNonInteractiveMode == 1 || dlgResult == QDialog::Accepted)
	{
		// Read Common GUI values
		if (readGui(m_bridgeDialog) == false)
		{
			return;
		}

		// Check if Roblox Output Folder and Blender Executable are valid, if not issue Error and fail gracefully
		bool bSettingsValid = false;
		do 
		{
			if (m_sRobloxOutputFolderPath != "" && QDir(m_sRobloxOutputFolderPath).exists() &&
				m_sBlenderExecutablePath != "" && QFileInfo(m_sBlenderExecutablePath).exists() &&
				m_sAssetType != "__")
			{
				bSettingsValid = true;
				break;
			}
			if (bSettingsValid == false && m_nNonInteractiveMode == 1)
			{
				return;
			}
			if (m_sRobloxOutputFolderPath == "" || QDir(m_sRobloxOutputFolderPath).exists() == false)
			{
				QMessageBox::warning(0, tr("Roblox Output Folder"), tr("Roblox Output Folder must be set."), QMessageBox::Ok);
			}
			else if (m_sBlenderExecutablePath == "" || QFileInfo(m_sBlenderExecutablePath).exists() == false)
			{
				QMessageBox::warning(0, tr("Blender Executable Path"), tr("Blender Executable Path must be set."), QMessageBox::Ok);
				// Enable Advanced Settings
				QGroupBox* advancedBox = m_bridgeDialog->getAdvancedSettingsGroupBox();
				if (advancedBox->isChecked() == false)
				{
					advancedBox->setChecked(true);
					foreach(QObject* child, advancedBox->children())
					{
						QWidget* widget = qobject_cast<QWidget*>(child);
						if (widget)
						{
							widget->setHidden(false);
							QString name = widget->objectName();
							dzApp->log("DEBUG: widget name = " + name);
						}
					}
				}
			}
			else if (m_sAssetType == "__")
			{
				QMessageBox::warning(0, tr("Select Asset Type"), tr("Please select an asset type from the dropdown menu."), QMessageBox::Ok);
			}

			qobject_cast<DzRobloxDialog*>(m_bridgeDialog)->disableAcceptUntilAllRequirementsValid();
			dlgResult = m_bridgeDialog->exec();
			if (dlgResult == QDialog::Rejected)
			{
				return;
			}
			if (readGui(m_bridgeDialog) == false)
			{
				return;
			}

		} while (bSettingsValid == false);


		// Extract PluginData
		// 1. extract to temp folder
		// 2. attempt copy to plugindata folder
		QString sScriptFolderPath;
		bool replace = true;
		QString sArchiveFilename = "/plugindata.zip";
		QString sEmbeddedArchivePath = ":/DazBridgeRoblox" + sArchiveFilename;
		QFile srcFile(sEmbeddedArchivePath);
		QString tempPathArchive = dzApp->getTempPath() + sArchiveFilename;
		DzBridgeAction::copyFile(&srcFile, &tempPathArchive, replace);
		srcFile.close();
		int result = ::zip_extract(tempPathArchive.toAscii().data(), dzApp->getTempPath().toAscii().data(), nullptr, nullptr);


		// DB 2021-10-11: Progress Bar
		DzProgress* exportProgress = new DzProgress("Sending to Roblox...", 10);
        exportProgress->setCloseOnFinish(false);
        exportProgress->enable(true);
        
		//Create Daz3D folder if it doesn't exist
		QDir dir;
		dir.mkpath(m_sRootFolder);
		exportProgress->step();

		// export FBX
		bool bExportResult = exportHD(exportProgress);

		if (!bExportResult)
		{
			QMessageBox::information(0, "Roblox Avatar Exporter",
				tr("Export cancelled."), QMessageBox::Ok);
			exportProgress->finish();
			return;
		}

        exportProgress->setInfo("Preparing Blender Scripts...");
        
		// run blender scripts
		//QString sBlenderPath = QString("C:/Program Files/Blender Foundation/Blender 3.6/blender.exe");
		QString sBlenderLogPath = QString("%1/blender.log").arg(m_sDestinationPath);

		// Hardcoded to use FallbackScriptFolder
		bool bUseFallbackScriptFolder = true;
		QStringList aOverrideFilenameList = (QStringList() << "blender_tools.py" << "NodeArrange.py" << 
			"blender_dtu_to_roblox_blend.py" << "blender_dtu_to_avatar_autosetup.py" <<
			"roblox_tools.py" << "Daz_Cage_Att_Template.blend" <<
			"game_readiness_tools.py" << "game_readiness_roblox_data.py");
		if (bUseFallbackScriptFolder)
		{
			foreach(QString filename, aOverrideFilenameList)
			{
				QString sFallbackFolder = m_sDestinationPath + "/scripts";
				if (QDir(sFallbackFolder).exists() == false)
				{
					QDir dir;
					bool result = dir.mkpath(sFallbackFolder);
					if (!result)
					{
						dzApp->log("Roblox Avatar Exporter: ERROR: Unable to create fallback folder: " + sFallbackFolder + ", will fallback to using temp folder to store script files...");
						sScriptFolderPath = dzApp->getTempPath();
						break;
					}
				}
				QString sOverrideFilePath = sFallbackFolder + "/" + filename;
				QString sTempFilePath = dzApp->getTempPath() + "/" + filename;
				// delete file if it already exists
				if (QFileInfo(sOverrideFilePath).exists() == true)
				{
					dzApp->log(QString("Roblox Avatar Exporter: Removing existing override file (%1)...").arg(sOverrideFilePath));
					bool result = QFile(sOverrideFilePath).remove();
					if (!result) {
						dzApp->debug("Roblox Avatar Exporter: ERROR: unable to remove existing script from intermediate folder: " + sOverrideFilePath);
					}
				}
				dzApp->log(QString("Roblox Avatar Exporter: Found override file (%1), copying from temp folder.").arg(sOverrideFilePath));
				bool result = QFile(sTempFilePath).copy(sOverrideFilePath);
				if (result)
				{
					// if successful, use overridepath as scriptpath
					sScriptFolderPath = QFileInfo(sOverrideFilePath).path();
				}
				else
				{
					dzApp->log("Roblox Avatar Exporter: ERROR: Unable to copy script files to scriptfolder: " + sOverrideFilePath);
				}
			}
		}

		QString sScriptPath;
		
		if (m_sAssetType.contains("R15"))
		{
			sScriptPath = sScriptFolderPath + "/blender_dtu_to_roblox_blend.py";
		}
		else if (m_sAssetType.contains("S1"))
		{
			sScriptPath = sScriptFolderPath + "/blender_dtu_to_avatar_autosetup.py";
		}
		QString sCommandArgs = QString("--background;--log-file;%1;--python-exit-code;%2;--python;%3;%4").arg(sBlenderLogPath).arg(m_nPythonExceptionExitCode).arg(sScriptPath).arg(m_sDestinationFBX);

		// 4. Generate manual batch file to launch blender scripts
		QString sBatchString = QString("\"%1\"").arg(m_sBlenderExecutablePath);
		foreach (QString arg, sCommandArgs.split(";"))
		{
			if (arg.contains(" "))
			{
				sBatchString += QString(" \"%1\"").arg(arg);
			}
			else
			{
				sBatchString += " " + arg;
			}
		}
		// write batch
		QString batchFilePath = m_sDestinationPath + "/manual_blender_script_1.bat";
		QFile batchFileOut(batchFilePath);
		batchFileOut.open(QIODevice::WriteOnly | QIODevice::OpenModeFlag::Truncate);
		batchFileOut.write(sBatchString.toAscii().constData());
		batchFileOut.close();

		/*****************************************************************************************************/

		dzApp->log("DzRoblox-MvcTest: Hello, World.");
		OpenFBXInterface* openFbx = OpenFBXInterface::GetInterface();
		bool bFailed = false;
		bool bResult = false;
		// 1a. read template fbx
		QString templateFbxFile = "V9_Cage_Att_MVC_Template.fbx";
		QString tempFilePath = dzApp->getTempPath() + "/" + templateFbxFile;
		FbxScene* pTemplateScene = openFbx->CreateScene("My Scene");
		if (openFbx->LoadScene(pTemplateScene, tempFilePath.toLocal8Bit().data() ) == false)
		{
			bFailed = true;
			dzApp->log("DzRoblox-MvcTest: Load Fbx Scene failed.");
		}
		// 1b. read morphed fbx
		FbxScene* pMorphedSourceScene = openFbx->CreateScene("My Scene");
		if (openFbx->LoadScene(pMorphedSourceScene, m_sDestinationFBX.toLocal8Bit().data()) == false)
		{
			bFailed = true;
			dzApp->log("DzRoblox-MvcTest: Load Fbx Scene failed.");
		}
		// 2a. extract template mesh
		FbxNode* pTemplateMesh = pTemplateScene->FindNodeByName("OriginalGenesis9");
		if (!pTemplateMesh) 
		{
			pTemplateMesh = pTemplateScene->FindNodeByName("Genesis9");
			if (!pTemplateMesh)
			{
				bFailed = true;
				dzApp->log("DzRoblox-MvcTest: Unable to extract template mesh.");
			}
		}
		// 2b. extract morphed mesh
		FbxNode* pSourceNode = pMorphedSourceScene->FindNodeByName("Genesis9.Shape");
		if (!pSourceNode)
		{
			bFailed = true;
			dzApp->log("DzRoblox-MvcTest: Unable to extract source mesh.");
		}
		FbxGeometry* pSourceGeo = openFbx->FindGeometry(pMorphedSourceScene, "Genesis9.Shape");
/*
		// add to templatescene
		bResult = pTemplateScene->AddGeometry(pSourceGeo);
		if (!bResult)
		{
			bFailed = true;
			dzApp->log("DzRoblox-MvcTest: Unable to copy source mesh to template scene.");
		}
		bResult = pTemplateScene->AddNode(pSourceNode);
		if (!bResult)
		{
			bFailed = true;
			dzApp->log("DzRoblox-MvcTest: Unable to copy source mesh to template scene.");
		}
*/

		/****************************************************************************************/

		// unparent mesh
		for (int i = 0; i < pMorphedSourceScene->GetNodeCount(); i++)
		{
			FbxNode* pChildNode = pMorphedSourceScene->GetNode(i);
			if (pChildNode->GetNodeAttribute() && 
				pChildNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
			{
				// reparent to root node
				if (pChildNode->GetParent() != pMorphedSourceScene->GetRootNode())
				{
					pMorphedSourceScene->GetRootNode()->AddChild(pChildNode);
				}
			}
		}
		FbxNode* characterRoot = pMorphedSourceScene->FindNodeByName("Genesis9");
		// local transform
		FbxDouble3 scale = characterRoot->LclScaling.Get();
		FbxDouble3 newScale = FbxDouble3(scale[0]/30, scale[1]/30, scale[2]/30);
		characterRoot->LclScaling.Set(newScale);

		// causes original skeleton to be piled up at origin
		mergeScenes(pMorphedSourceScene, pTemplateScene);

		/****************************************************************************************/

		// 3a. for each cage or att...
		QStringList aCageNames;
		aCageNames << "Head_OuterCage" << "LeftFoot_OuterCage" << "LeftHand_OuterCage" <<
			"LeftLowerArm_OuterCage" << "LeftLowerLeg_OuterCage" << "LeftUpperArm_OuterCage" <<
			"LeftUpperLeg_OuterCage" << "LowerTorso_OuterCage" << "RightFoot_OuterCage" <<
			"RightHand_OuterCage" << "RightLowerArm_OuterCage" << "RightLowerLeg_OuterCage" <<
			"RightUpperArm_OuterCage" << "RightUpperLeg_OuterCage" << "UpperTorso_OuterCage";
		foreach (QString sNodeName, aCageNames)
		{
//			FbxNode* pCageNode = pTemplateScene->FindNodeByName(sNodeName.toLocal8Bit().data());
//			deepCopyNode(pMorphedSourceScene->GetRootNode(), pCageNode);

			FbxNode* pCageNode = pMorphedSourceScene->FindNodeByName(sNodeName.toLocal8Bit().data());
			if (pCageNode)
			{
				// TODO: Remap cages using Mvc deform
				printf("TODO: Remap cages using Mvc deform");
				// make weights with originalgenesis9 + cage
				// get cage vertexbuffer
				FbxMesh* pCageMesh = pCageNode->GetMesh();
				MvcCageRetargeter* pCageRetargeter = new MvcCageRetargeter();
				pCageRetargeter->createMvcWeights(pTemplateMesh->GetMesh(), pCageMesh, exportProgress);
				// make new cage with weights * newgenesis9
				int numCageVerts = pCageMesh->GetControlPointsCount();
				FbxVector4* pTempCageBuffer = pCageMesh->GetControlPoints();
				FbxVector4* pMorphedCageBuffer = new FbxVector4[numCageVerts];
				memcpy(pMorphedCageBuffer, pTempCageBuffer, sizeof(FbxVector4) * numCageVerts);
				pCageRetargeter->deformCage(pSourceNode->GetMesh(), pCageMesh, pMorphedCageBuffer);
				// update cage with verts from new cage
				memcpy(pTempCageBuffer, pMorphedCageBuffer, sizeof(FbxVector4) * numCageVerts);
			}

		}
		// 3b. for each vertex of cage or att
		// 3c. calculate f(original mesh, vertex) == mvcweights
		// 4a. calculate deformation for each vertex:
		// 4b. mvcweights * morphed mesh ==> deformed vertex
		// 5. replace vertex in morphed fbx
		// 6. save morphed fbx as morphed fbx with deformed cage/att
		QString morphedOutputFilename = "morphedFile.fbx";
		QString morphedOutputPath = m_sDestinationPath + "/" + morphedOutputFilename;
		// type 1 == ascii
		if (openFbx->SaveScene(pMorphedSourceScene, morphedOutputPath.toLocal8Bit().data()) == false)
		{
			bFailed = true;
			dzApp->log("DzRoblox-MvcTest: Load Source Fbx Scene failed.");
		}

		exportProgress->finish();
		return;

		/*****************************************************************************************************/

		exportProgress->setInfo("Starting Blender Processing...");
		bool retCode = executeBlenderScripts(m_sBlenderExecutablePath, sCommandArgs);

        exportProgress->setInfo("Roblox Avatar Exporter: Export Phase Completed.");
		// DB 2021-10-11: Progress Bar
		exportProgress->finish();

		// DB 2021-09-02: messagebox "Export Complete"
		if (m_nNonInteractiveMode == 0)
		{
			if (retCode)
			{
				QMessageBox::information(0, "Roblox Avatar Exporter",
					tr("Export from Daz Studio complete. Ready to import into Roblox Studio."), QMessageBox::Ok);

#ifdef WIN32
				ShellExecuteA(NULL, "open", m_sRobloxOutputFolderPath.toLocal8Bit().data(), NULL, NULL, SW_SHOWDEFAULT);
				//// The above line does the equivalent as following lines, but has advantage of only opening 1 explorer window
				//// with multiple clicks.
				//
				//	QStringList args;
				//	args << "/select," << QDir::toNativeSeparators(sIntermediateFolderPath);
				//	QProcess::startDetached("explorer", args);
				//
#elif defined(__APPLE__)
				QStringList args;
				args << "-e";
				args << "tell application \"Finder\"";
				args << "-e";
				args << "activate";
				args << "-e";
                if (m_sAssetType.contains("R15")) {
                    args << "select POSIX file \"" + m_sRobloxOutputFolderPath + "/" + m_sExportFilename + R15_POSTFIX_STRING + ".fbx" + "\"";
                }
                else if (m_sAssetType.contains("S1")) {
                    args << "select POSIX file \"" + m_sRobloxOutputFolderPath + "/" + m_sExportFilename + S1_POSTFIX_STRING + ".fbx" + "\"";
                }
                else {
                    args << "select POSIX file \"" + m_sRobloxOutputFolderPath + "/." + "\"";
                }
				args << "-e";
				args << "end tell";
				QProcess::startDetached("osascript", args);
#endif
			}
			else
			{
				QMessageBox::critical(0, "Roblox Avatar Exporter",
					tr(QString("An error occured during the export process (ExitCode=%1).  Please check log files at: %2").arg(m_nBlenderExitCode).arg(m_sDestinationPath).toLocal8Bit()), QMessageBox::Ok);
			}

		}

	}
}

void DzRobloxAction::writeConfiguration()
{
	QString DTUfilename = m_sDestinationPath + m_sExportFilename + ".dtu";
	QFile DTUfile(DTUfilename);
	DTUfile.open(QIODevice::WriteOnly);
	DzJsonWriter writer(&DTUfile);
	writer.startObject(true);

	writeDTUHeader(writer);

	// Godot-specific items
	writer.addMember("Output Folder", m_sRobloxOutputFolderPath);

	if (true)
	{
		QTextStream *pCVSStream = nullptr;
		if (m_bExportMaterialPropertiesCSV)
		{
			QString filename = m_sDestinationPath + m_sExportFilename + "_Maps.csv";
			QFile file(filename);
			file.open(QIODevice::WriteOnly);
			pCVSStream = new QTextStream(&file);
			*pCVSStream << "Version, Object, Material, Type, Color, Opacity, File" << endl;
		}
		writeAllMaterials(m_pSelectedNode, writer, pCVSStream);
		writeAllMorphs(writer);

		writeMorphLinks(writer);
		writeMorphNames(writer);

		DzBoneList aBoneList = getAllBones(m_pSelectedNode);

		writeSkeletonData(m_pSelectedNode, writer);
		writeHeadTailData(m_pSelectedNode, writer);

		writeJointOrientation(aBoneList, writer);
		writeLimitData(aBoneList, writer);
		writePoseData(m_pSelectedNode, writer, true);
		writeAllSubdivisions(writer);
		writeAllDforceInfo(m_pSelectedNode, writer);
	}

	if (m_sAssetType == "Pose")
	{
	   writeAllPoses(writer);
	}

	if (m_sAssetType == "Environment")
	{
		writeEnvironment(writer);
	}

	writer.finishObject();
	DTUfile.close();

}

// Setup custom FBX export options
void DzRobloxAction::setExportOptions(DzFileIOSettings& ExportOptions)
{
	//ExportOptions.setBoolValue("doEmbed", false);
	//ExportOptions.setBoolValue("doDiffuseOpacity", false);
	//ExportOptions.setBoolValue("doCopyTextures", false);
	ExportOptions.setBoolValue("doFps", true);
	ExportOptions.setBoolValue("doLocks", false);
	ExportOptions.setBoolValue("doLimits", false);
	ExportOptions.setBoolValue("doBaseFigurePoseOnly", false);
	ExportOptions.setBoolValue("doHelperScriptScripts", false);
	ExportOptions.setBoolValue("doMentalRayMaterials", false);
}

QString DzRobloxAction::readGuiRootFolder()
{
	QString rootFolder = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DazToRoblox";

	if (m_bridgeDialog)
	{
		QLineEdit* intermediateFolderEdit = nullptr;
		DzRobloxDialog* robloxDialog = qobject_cast<DzRobloxDialog*>(m_bridgeDialog);

		if (robloxDialog)
			intermediateFolderEdit = robloxDialog->getIntermediateFolderEdit();

		if (intermediateFolderEdit)
			rootFolder = intermediateFolderEdit->text().replace("\\", "/");
	}

	return rootFolder;
}

bool DzRobloxAction::readGui(DZ_BRIDGE_NAMESPACE::DzBridgeDialog* BridgeDialog)
{
	bool bResult = DzBridgeAction::readGui(BridgeDialog);
	if (!bResult)
	{
		return false;
	}

	DzRobloxDialog* pRobloxDialog = qobject_cast<DzRobloxDialog*>(BridgeDialog);

	if (pRobloxDialog)
	{
		// Collect the values from the dialog fields
		if (m_sRobloxOutputFolderPath == "" || m_nNonInteractiveMode == 0) m_sRobloxOutputFolderPath = pRobloxDialog->m_wRobloxOutputFolderEdit->text().replace("\\", "/");
		if (m_sBlenderExecutablePath == "" || m_nNonInteractiveMode == 0) m_sBlenderExecutablePath = pRobloxDialog->m_wBlenderExecutablePathEdit->text().replace("\\", "/");

	}
	else
	{
		// TODO: issue error and fail gracefully
		dzApp->log("Roblox Avatar Exporter: ERROR: Roblox Dialog was not initialized.  Cancelling operation...");

		return false;
	}

	return true;
}

bool DzRobloxAction::executeBlenderScripts(QString sFilePath, QString sCommandlineArguments)
{
	// fork or spawn child process
	QString sWorkingPath = m_sDestinationPath;
	QStringList args = sCommandlineArguments.split(";");

	float fTimeoutInSeconds = 2.3 * 60;
	float fMilliSecondsPerTick = 200;
	int numTotalTicks = fTimeoutInSeconds * 1000 / fMilliSecondsPerTick;
	DzProgress* progress = new DzProgress("Running Blender Scripts", numTotalTicks, false, true);
	progress->enable(true);
	QProcess* pToolProcess = new QProcess(this);
	pToolProcess->setWorkingDirectory(sWorkingPath);
	pToolProcess->start(sFilePath, args);
	while (pToolProcess->waitForFinished(fMilliSecondsPerTick) == false) {
		if (pToolProcess->state() == QProcess::Running)
		{
			progress->step();
		}
		else
		{
			break;
		}
	}
    progress->setInfo("Blender Scripts Completed.");
	progress->finish();
	delete progress;
	m_nBlenderExitCode = pToolProcess->exitCode();
#ifdef __APPLE__
    if (m_nBlenderExitCode != 0 && m_nBlenderExitCode != 120)
#else
    if (m_nBlenderExitCode != 0)
#endif
    {
		if (m_nBlenderExitCode == m_nPythonExceptionExitCode)
		{
			dzApp->log(QString("Roblox Avatar Exporter: ERROR: Python error:.... %1").arg(m_nBlenderExitCode));
		}
		else
		{
            dzApp->log(QString("Roblox Avatar Exporter: ERROR: exit code = %1").arg(m_nBlenderExitCode));
		}
		return false;
	}

	return true;
}

bool DzRobloxAction::isAssetMorphCompatible(QString sAssetType)
{
	return false;
}

bool DzRobloxAction::isAssetMeshCompatible(QString sAssetType)
{
	return true;
}

bool DzRobloxAction::isAssetAnimationCompatible(QString sAssetType)
{
	return false;
}

DZ_BRIDGE_NAMESPACE::DzBridgeDialog* DzRobloxAction::getBridgeDialog()
{
	if (m_bridgeDialog == nullptr)
	{
		DzMainWindow* mw = dzApp->getInterface();
		if (!mw)
		{
			return nullptr;
		}
		m_bridgeDialog = new DzRobloxDialog(mw);
		m_bridgeDialog->setBridgeActionObject(this);
	}

	return m_bridgeDialog;
}

bool DzRobloxAction::preProcessScene(DzNode* parentNode)
{
	DzBridgeAction::preProcessScene(parentNode);

	// apply geografts
	QString tempPath = dzApp->getTempPath();

	// check if geograft present
	if (dzScene->findNodeByLabel("game_engine_mouth_geograft") == NULL &&
		dzScene->findNode("game_engine_mouth_geograft_0") == NULL) {
		DzNode* mouthNode = dzScene->findNode("Genesis9Mouth");
		//DzNode* mouthNode = dzScene->findNodeByLabel("Genesis 9 Mouth");
		QString mouthNodeName = mouthNode->getName();
		applyGeograft(mouthNode, tempPath + "/game_engine_mouth_geograft.duf", "game_engine_mouth_geograft_0");
	}
	if (dzScene->findNodeByLabel("game_engine_eye_geograft") == NULL &&
		dzScene->findNode("game_engine_eye_geograft_0") == NULL) {
		DzNode* eyesNode = dzScene->findNode("Genesis9Eyes");
		//DzNode* eyesNode = dzScene->findNodeByLabel("Genesis 9 Eyes");
		QString eyesNodeName = eyesNode->getName();
		applyGeograft(eyesNode, tempPath + "/game_engine_eye_geograft.duf", "game_engine_eye_geograft_0");
	}

	//QString sPluginFolder = dzApp->getPluginsPath() + "/DazToRoblox";
    QString sPluginFolder = tempPath;

	QString sRobloxBoneConverter = "bone_converter.dsa";
	QString sApplyModestyOverlay = "apply_modesty_overlay.dsa";
	QString sGenerateCombinedTextures = "generate_texture_parts.dsa";
	QString sCombineTextureMaps = "combine_texture_parts.dsa";
	QString sAssignCombinedTextures = "assign_combined_textures.dsa";


	if (m_sAssetType.contains("_M"))
	{
		sApplyModestyOverlay = "apply_modesty_overlay_M.dsa";
	}
	//else if (m_sAssetType == "R15Z")
	//{
	//	sApplyModestyOverlay = "";
	//}


	/// BONE CONVERSION OPERATION
	QString sScriptFilename = sPluginFolder + "/" + sRobloxBoneConverter;
	if (QFileInfo(sScriptFilename).exists() == false) {
		sScriptFilename = dzApp->getTempPath() + "/" + sRobloxBoneConverter;
	}
	QScopedPointer<DzScript> Script(new DzScript());
	Script->loadFromFile(sScriptFilename);
	Script->execute();

	dzScene->selectAllNodes(false);
	dzScene->setPrimarySelection(parentNode);

	/// CHECK FOR ALTERED UV SET, IF FOUND SKIP TEXTURE OPERATIONS
	if (!parentNode || !parentNode->inherits("DzFigure")) {
		// TODO: add gui error message
		dzApp->debug("ERROR: DzRobloxAction: preProcessScene(): invalid parentNode. Returning false.");
		return false;
	}

	// 2. get UV property
	DzObject* obj = parentNode->getObject();
	if (!obj) {
		// TODO: add gui error message
		dzApp->debug("ERROR: DzRobloxAction: preProcessScene(): invalid parentNode->getObject(). Returning false.");
		return false;
	}
	DzShape* shape = obj->getCurrentShape();
	if (!shape) {
		// TODO: add gui error message
		dzApp->debug("ERROR: DzRobloxAction: preProcessScene(): invalid obj->getCurrentShape(). Returning false.");
		return false;
	}
	QObjectList rawList = shape->getAllMaterials();
	for (int i = 0; i < rawList.count(); i++) {
		DzMaterial* mat = qobject_cast<DzMaterial*>(rawList[i]);
		if (!mat) continue;
		DzProperty* prop = mat->findProperty("UV Set");
		QString debugName = prop->getName();
		DzEnumProperty* uvset = qobject_cast<DzEnumProperty*>(prop);
		if (uvset) {
			int combinedUvVal = uvset->findItemString(COMBINED_UVSET_STRING);
			int currentVal = uvset->getValue();
			if (currentVal == combinedUvVal) {
				dzApp->debug("WARNING: DzRobloxAction: preProcessScene(): Combined UV already set, skipping texture operations. Returning false.");
				return false;
			}
		}
	}


	/// TEXTURE OPERATIONS (MODESTY OVERLAY, UV CONVERSION, ETC)
	if (!sApplyModestyOverlay.isEmpty())
	{
		sScriptFilename = sPluginFolder + "/" + sApplyModestyOverlay;
		if (QFileInfo(sScriptFilename).exists() == false) {
			sScriptFilename = dzApp->getTempPath() + "/" + sApplyModestyOverlay;
		}
		Script->loadFromFile(sScriptFilename);
		Script->execute();
	}

	dzScene->selectAllNodes(false);
	dzScene->setPrimarySelection(parentNode);

	sScriptFilename = sPluginFolder + "/" + sGenerateCombinedTextures;
	if (QFileInfo(sScriptFilename).exists() == false) {
		sScriptFilename = dzApp->getTempPath() + "/" + sGenerateCombinedTextures;
	}
    Script.reset(new DzScript());
	Script->loadFromFile(sScriptFilename);
	Script->execute();

	dzScene->selectAllNodes(false);
	dzScene->setPrimarySelection(parentNode);

	sScriptFilename = sPluginFolder + "/" + sCombineTextureMaps;
	if (QFileInfo(sScriptFilename).exists() == false) {
		sScriptFilename = dzApp->getTempPath() + "/" + sCombineTextureMaps;
	}
    Script.reset(new DzScript());
	Script->loadFromFile(sScriptFilename);
	Script->execute();

	dzScene->selectAllNodes(false);
	dzScene->setPrimarySelection(parentNode);

	sScriptFilename = sPluginFolder + "/" + sAssignCombinedTextures;
	if (QFileInfo(sScriptFilename).exists() == false) {
		sScriptFilename = dzApp->getTempPath() + "/" + sAssignCombinedTextures;
	}
    Script.reset(new DzScript());
	Script->loadFromFile(sScriptFilename);
	Script->execute();

	dzScene->selectAllNodes(false);
	dzScene->setPrimarySelection(parentNode);

	return true;
}


bool DzRobloxAction::applyGeograft(DzNode* pBaseNode, QString geograftFilename, QString geograftNodeName)
{
	DzContentMgr* contentMgr = dzApp->getContentMgr();
//	QString tempPath = dzApp->getTempPath() + "/" + geograftFilename;
	QFile srcFile(geograftFilename);
	if (geograftNodeName == "")
	{
		geograftNodeName = QFileInfo(geograftFilename).baseName();
	}
	QString node_name;
	// copy to temp folder and merge tpose into scene
	if (srcFile.exists())
	{
//		DzBridgeAction::copyFile(&srcFile, &tempPath, true);
	//	if (contentMgr->openFile(tempPath, true))

		// deselect all ndoes
		dzScene->setPrimarySelection(NULL);
		if (contentMgr->openFile(geograftFilename, true))
		{
			// parent geograft
			DzNode* generic_geograft_node = dzScene->findNode(geograftNodeName);
			if (!generic_geograft_node)
				generic_geograft_node = dzScene->findNodeByLabel(QString(geograftNodeName).replace("_0", ""));
			//QString debug_geograftNodeName = generic_geograft_node->getName();
			DzFigure* geograft_node = qobject_cast<DzFigure*>(generic_geograft_node);
			if (geograft_node && pBaseNode)
			{
				// track geograft_node for removal in undoPreprocessScene()
				m_aGeograftConversionHelpers.append(geograft_node);

				// copy base materials to geograft
				DzShape *geograftShape = geograft_node->getObject()->getCurrentShape();
				auto dstMatList = geograftShape->getAllMaterials();
				DzShape* baseShape = pBaseNode->getObject()->getCurrentShape();
				auto srcMatList = baseShape->getAllMaterials();
				foreach( QObject *dobj, dstMatList ) {
					DzMaterial* dstMat = qobject_cast<DzMaterial*>(dobj);
					foreach(QObject * sobj, srcMatList) {
						DzMaterial *srcMat = qobject_cast<DzMaterial*>(sobj);
						if (dstMat && srcMat) {
							QString cleanedDestMatName = QString(dstMat->getName()).replace(" ","").replace("_","").toLower();
							QString cleanedSrcMatName = QString(srcMat->getName()).replace(" ", "").replace("_", "").toLower();
							if (cleanedDestMatName == cleanedSrcMatName) {
								dstMat->copyFrom(srcMat);
								break;
							}
						}
					}
				}


				geograft_node->setFollowTarget(pBaseNode->getSkeleton());
				// DB: must parent with in-place=false, then unparent with in-place=true in order to position geograft correctly
				// while also working-around FBX exporter geograft mesh duplication
				pBaseNode->addNodeChild(geograft_node, false);
				pBaseNode->removeNodeChild(geograft_node, true);
				node_name = geograft_node->getName();
			}
		}
	}
	dzApp->debug("Geografting done: geograft node = " + node_name);
	return true;
}

bool DzRobloxAction::undoPreProcessScene()
{
	foreach(DzNode *pNode, m_aGeograftConversionHelpers)
	{
		DzFigure* geograft_node = qobject_cast<DzFigure*>(pNode);
		if (geograft_node)
		{
			geograft_node->setFollowTarget(NULL);
			dzScene->removeNode(geograft_node);
		}
	}
	m_aGeograftConversionHelpers.clear();

	bool result = DzBridgeAction::undoPreProcessScene();

	return result;
}

#include "moc_DzRobloxAction.cpp"
