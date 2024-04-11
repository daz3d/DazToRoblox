#define COMBINED_UVSET_STRING "Combined Head And Body"

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

DzRobloxAction::DzRobloxAction() :
	DzBridgeAction(tr("Daz to &Roblox"), tr("Export the selected node for Roblox Studio."))
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
			tr("Please select one Character or Prop to send."), QMessageBox::Ok);

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
					tr("Please select one Character or Prop to send."), QMessageBox::Ok);
			}
		}
	}

	// if UV is not default, then issue error and return
	// 1. get UV of selected node
	DzNode* testNode = dzScene->getPrimarySelection();
	if (testNode && testNode->inherits("DzFigure")) {
		// 2. get UV property
		DzObject* obj = testNode->getObject();
		if (obj) {
			DzShape* shape = obj->getCurrentShape();
			QObjectList rawList = shape->getAllMaterials();
			for (int i=0; i < rawList.count(); i++) {
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
			QMessageBox::information(0, "Daz To Roblox Exporter",
				tr("Export cancelled."), QMessageBox::Ok);
			exportProgress->finish();
			return;
		}

        exportProgress->setInfo("Preparing Blender Scripts...");
        
		// run blender scripts
		//QString sBlenderPath = QString("C:/Program Files/Blender Foundation/Blender 3.6/blender.exe");
		QString sBlenderLogPath = QString("%1/blender.log").arg(m_sDestinationPath);

		// search for override files in folder with DLL and copy over extracted files
		bool bUseFallbackScriptFolder = true;
		//QString sPluginFolder = dzApp->getPluginsPath() + "/DazToRoblox";
		//if (QDir(sPluginFolder).exists() == false)
		//{
		//	QDir dir;
		//	bool result = dir.mkpath(sPluginFolder);
		//	if (!result)
		//	{
		//		dzApp->log("ERROR: Unable to create script folder: " + sPluginFolder + ", will fallback to using temp folder to store script files...");
		//		sPluginFolder = "";
		//		bUseFallbackScriptFolder = true;
		//	}
		//}

		//// 1. extract to temp folder
		//// 2. attempt copy to plugindata folder
		//QString sScriptFolderPath;
		//bool replace = true;
		//QString sArchiveFilename = "/plugindata.zip";
		//QString sEmbeddedArchivePath = ":/DazBridgeRoblox" + sArchiveFilename;
		//QFile srcFile(sEmbeddedArchivePath);
		//QString tempPathArchive = dzApp->getTempPath() + sArchiveFilename;
		//DzBridgeAction::copyFile(&srcFile, &tempPathArchive, replace);
		//srcFile.close();
		//int result = ::zip_extract(tempPathArchive.toAscii().data(), dzApp->getTempPath().toAscii().data(), nullptr, nullptr);

		//// 2. attempt copy to plugindata folder, if already exist, use as override
  //      // search for override files in folder with DLL and copy over extracted files
		QStringList aOverrideFilenameList = (QStringList() << "blender_tools.py" << "NodeArrange.py" << 
			"blender_dtu_to_roblox_blend.py" << "blender_dtu_to_avatar_autosetup.py" <<
			"roblox_tools.py" << "Daz_Cage_Att_Template.blend" <<
			"game_readiness_tools.py" << "game_readiness_roblox_data.py");
		//if (sPluginFolder.isEmpty() == false)
		//{
		//	foreach(QString filename, aOverrideFilenameList)
		//	{
		//		QString sOverrideFilePath = sPluginFolder + "/" + filename;
		//		QString sTempFilePath = dzApp->getTempPath() + "/" + filename;
		//		// if doesn't exist in override folder, copy from temp
		//		if (QFileInfo(sOverrideFilePath).exists() == false)
		//		{
		//			dzApp->log(QString("Found override file (%1), copying to temp folder.").arg(sOverrideFilePath));
		//			bool result = QFile(sTempFilePath).copy(sOverrideFilePath);
		//			if (result)
		//			{
		//				// if successful, use overridepath as scriptpath
		//				sScriptFolderPath = QFileInfo(sOverrideFilePath).path();
		//			}
		//			else
		//			{
		//				// script does not exist in override path (plug folder) so fallback to using intermediate folder
		//				dzApp->log("ERROR: Unable to copy script files to scriptfolder: " + sOverrideFilePath + ", attempting to use intermediate folder...");
		//				bUseFallbackScriptFolder = true;
		//				break;
		//			}
		//		}
		//		else
		//		{
		//			// file exists, so use overridepath as scriptpath
		//			sScriptFolderPath = QFileInfo(sOverrideFilePath).path();
		//		}
		//	}
		//}

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
						dzApp->log("ERROR: Unable to create fallback folder: " + sFallbackFolder + ", will fallback to using temp folder to store script files...");
						sScriptFolderPath = dzApp->getTempPath();
						break;
					}
				}
				QString sOverrideFilePath = sFallbackFolder + "/" + filename;
				QString sTempFilePath = dzApp->getTempPath() + "/" + filename;
				// delete file if it already exists
				if (QFileInfo(sOverrideFilePath).exists() == true)
				{
					dzApp->log(QString("Removing existing override file (%1)...").arg(sOverrideFilePath));
					bool result = QFile(sOverrideFilePath).remove();
					if (!result) {
						dzApp->debug("DazToRoblox: ERROR: unable to remove existing script from intermediate folder: " + sOverrideFilePath);
					}
				}
				dzApp->log(QString("Found override file (%1), copying from temp folder.").arg(sOverrideFilePath));
				bool result = QFile(sTempFilePath).copy(sOverrideFilePath);
				if (result)
				{
					// if successful, use overridepath as scriptpath
					sScriptFolderPath = QFileInfo(sOverrideFilePath).path();
				}
				else
				{
					dzApp->log("ERROR: Unable to copy script files to scriptfolder: " + sOverrideFilePath);
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

		exportProgress->setInfo("Starting Blender Processing...");
		bool retCode = executeBlenderScripts(m_sBlenderExecutablePath, sCommandArgs);

        exportProgress->setInfo("Daz To Roblox: Export Phase Completed.");
		// DB 2021-10-11: Progress Bar
		exportProgress->finish();

		// DB 2021-09-02: messagebox "Export Complete"
		if (m_nNonInteractiveMode == 0)
		{
			if (retCode)
			{
				QMessageBox::information(0, "Daz To Roblox Exporter",
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
				args << "select POSIX file \"" + m_sRobloxOutputFolderPath + "/." + "\"";
				args << "-e";
				args << "end tell";
				QProcess::startDetached("osascript", args);
#endif
			}
			else
			{
				QMessageBox::critical(0, "Daz To Roblox Exporter",
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
		dzApp->log("ERROR: DazToRoblox: Roblox Dialog was not initialized.  Cancelling operation...");

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
			dzApp->log(QString("ERROR: DazToRoblox: Python error:.... %1").arg(m_nBlenderExitCode));
		}
		else
		{
            dzApp->log(QString("ERROR: DazToRoblox: exit code = %1").arg(m_nBlenderExitCode));
		}
		return false;
	}

	return true;
}

bool DzRobloxAction::isAssetMorphCompatible(QString sAssetType)
{
	return true;
}

bool DzRobloxAction::isAssetMeshCompatible(QString sAssetType)
{
	return true;
}

bool DzRobloxAction::isAssetAnimationCompatible(QString sAssetType)
{
	return true;
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
	if (dzScene->findNodeByLabel("game_engine_mouth_geograft") == NULL) {	
		DzNode* mouthNode = dzScene->findNode("Genesis9Mouth");
		//DzNode* mouthNode = dzScene->findNodeByLabel("Genesis 9 Mouth");
		QString mouthNodeName = mouthNode->getName();
		applyGeograft(mouthNode, tempPath + "/game_engine_mouth_geograft.duf");
	}
	if (dzScene->findNodeByLabel("game_engine_eye_geograft") == NULL) {
		DzNode* eyesNode = dzScene->findNode("Genesis9Eyes");
		//DzNode* eyesNode = dzScene->findNodeByLabel("Genesis 9 Eyes");
		QString eyesNodeName = eyesNode->getName();
		applyGeograft(eyesNode, tempPath + "/game_engine_eye_geograft.duf");
	}

	QString sPluginFolder = dzApp->getPluginsPath() + "/DazToRoblox";

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
			//DzNode* generic_geograft_node = dzScene->findNode(geograftNodeName);
			DzNode* generic_geograft_node = dzScene->findNodeByLabel(geograftNodeName);
			QString geograftNodeName = generic_geograft_node->getName();
			DzFigure* geograft_node = qobject_cast<DzFigure*>(generic_geograft_node);
			if (geograft_node && pBaseNode)
			{
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
				// DB: Do NOT parent node, since this will cause duplicate geo on export and other issues
//				pBaseNode->addNodeChild(geograft_node);
				node_name = geograft_node->getName();
			}
		}
	}
	dzApp->debug("Geografting done: geograft node = " + node_name);
	return true;
}

#include "moc_DzRobloxAction.cpp"
