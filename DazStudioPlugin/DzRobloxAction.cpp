#define WS_SCALE_FACTOR 0.01f

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
#include "dzfloatproperty.h"
#include "dzbone.h"
#include "dzjsonreader.h"
#include "dzjsondom.h"
#include "dzviewportmgr.h"
#include "dzviewtool.h"

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


void DzRobloxUtils::FACSexportSkeleton(DzNode* Node, DzNode* Parent, FbxNode* FbxParent, FbxScene* Scene, QMap<DzNode*, FbxNode*>& BoneMap)
{

	FbxNode* BoneNode;

	// null parent is the root bone
	if (FbxParent == nullptr)
	{
		if ( (BoneNode = Scene->FindNodeByName("root")) == NULL) {
			// Create a root bone.  Always named root so we don't have to fix it in Unreal
			FbxSkeleton* SkeletonAttribute = FbxSkeleton::Create(Scene, "root");
			SkeletonAttribute->SetSkeletonType(FbxSkeleton::eRoot);
			BoneNode = FbxNode::Create(Scene, "root");
			BoneNode->SetNodeAttribute(SkeletonAttribute);

			//BoneNode->PreRotation.Set(-90, 0, 0);

			FbxNode* RootNode = Scene->GetRootNode();
			RootNode->AddChild(BoneNode);
		}
		else
		{
			int nop = 0;
		}

		// Looks through the child nodes for more bones
		for (int ChildIndex = 0; ChildIndex < Node->getNumNodeChildren(); ChildIndex++)
		{
			DzNode* ChildNode = Node->getNodeChild(ChildIndex);
			FACSexportSkeleton(ChildNode, Node, BoneNode, Scene, BoneMap);
		}
	}
	else
	{
		// Child nodes need to be bones
		if (DzBone* Bone = qobject_cast<DzBone*>(Node))
		{
			FbxString fbxName(Node->getName().toLocal8Bit().data());
			if ( (BoneNode = Scene->FindNodeByName(fbxName)) == NULL) {
				// create the bone
				FbxSkeleton* SkeletonAttribute = FbxSkeleton::Create(Scene, Node->getName().toLocal8Bit().data());
				SkeletonAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
				BoneNode = FbxNode::Create(Scene, Node->getName().toLocal8Bit().data());
				BoneNode->SetNodeAttribute(SkeletonAttribute);

				// find the bones position
				DzVec3 Position = Node->getWSPos(DzTime(0), true) * WS_SCALE_FACTOR;
				DzVec3 ParentPosition = Parent->getWSPos(DzTime(0), true) * WS_SCALE_FACTOR;
				DzVec3 LocalPosition = Position - ParentPosition;

				// find the bone's rotation
				DzQuat Rotation = Node->getWSRot(DzTime(0), true);
				DzQuat ParentRotation = Parent->getWSRot(DzTime(0), true);
				DzQuat LocalRotation = Node->getOrientation(true);//Rotation * ParentRotation.inverse();
				DzVec3 VectorRotation;
				LocalRotation.getValue(VectorRotation);

				// set the position and rotation properties
				BoneNode->LclTranslation.Set(FbxVector4(LocalPosition.m_x, LocalPosition.m_y, LocalPosition.m_z));
				//			BoneNode->PreRotation.Set(FbxVector4(VectorRotation.m_x, VectorRotation.m_y, VectorRotation.m_z));
				VectorRotation = DzVec3(0, 0, 0);
				//			BoneNode->LclRotation.Set(FbxVector4(VectorRotation.m_x, VectorRotation.m_y, VectorRotation.m_z));

				FbxParent->AddChild(BoneNode);
			}
			else
			{
				int nop = 0;
			}

			// Looks through the child nodes for more bones
			for (int ChildIndex = 0; ChildIndex < Node->getNumNodeChildren(); ChildIndex++)
			{
				DzNode* ChildNode = Node->getNodeChild(ChildIndex);
				FACSexportSkeleton(ChildNode, Node, BoneNode, Scene, BoneMap);
			}
		}
	}

	if (BoneMap.contains(Node) == false) {
		// Add the bone to the map
		BoneMap.insert(Node, BoneNode);
	}

}

void DzRobloxUtils::setKey(int& KeyIndex, FbxTime Time, FbxAnimLayer* AnimLayer, FbxPropertyT<FbxDouble3>& Property, const char* pChannel, float Value) {
	FbxAnimCurve* animCurve = Property.GetCurve(AnimLayer, pChannel, true);
	animCurve->KeyModifyBegin();
	KeyIndex = animCurve->KeyAdd(Time);
	animCurve->KeySet(KeyIndex, Time, Value);
	animCurve->KeyModifyEnd();
}

bool DzRobloxUtils::hasAncestorName(DzNode* pNode, QString sAncestorName, bool bCaseSensitive) {
	if (DzNode* pParentNode = pNode->getNodeParent()) {
		if (bCaseSensitive) {
			if (pParentNode->getName() == sAncestorName)
				return true;
		}
		else {
			if (pParentNode->getName().toLower() == sAncestorName.toLower())
				return true;
		}
		if (pParentNode->getNodeParent())
			return hasAncestorName(pParentNode, sAncestorName, bCaseSensitive);
	}
	return false;
}

void DzRobloxUtils::FACSexportNodeAnimation(DzNode* Bone, QMap<DzNode*, FbxNode*>& BoneMap, FbxAnimLayer* AnimBaseLayer, float FigureScale)
{
	DzTimeRange PlayRange = dzScene->getPlayRange();

	QString Name = Bone->getName();

	if (hasAncestorName(Bone, "head", false) == false) {
		return;
	}

	FbxNode* Node = BoneMap.value(Bone);
	if (Node == nullptr) return;

	// Create a curve node for this bone
	FbxAnimCurveNode* AnimCurveNodeR = Node->LclRotation.GetCurveNode(AnimBaseLayer, true);
	FbxAnimCurveNode* AnimCurveNodeT = Node->LclTranslation.GetCurveNode(AnimBaseLayer, true);
	FbxAnimCurveNode* AnimCurveNodeS = Node->LclScaling.GetCurveNode(AnimBaseLayer, true);

	// For each frame, write a key (equivalent of bake)
	for (DzTime CurrentTime = PlayRange.getStart(); CurrentTime <= PlayRange.getEnd(); CurrentTime += dzScene->getTimeStep())
	{
		DzTime Frame = CurrentTime / dzScene->getTimeStep();

		dzScene->setTime(CurrentTime);
		Bone->finalize();
		DzVec3 Position = DzVec3(0, 0, 0);
		DzVec3 VectorRotation = DzVec3(0, 0, 0);
		DzVec3 VectorScale = DzVec3(1, 1, 1);

		Position = Bone->getWSPos(CurrentTime, false) * WS_SCALE_FACTOR;
		DzQuat Orientation = Bone->getWSRot(CurrentTime, false);
		DzMatrix3 ScaleMatrix = Bone->getWSScale(CurrentTime, false);
		if (DzNode* ParentBone = Bone->getNodeParent()) {
			ParentBone->finalize();
			Position = (Bone->getWSPos(CurrentTime, false) * WS_SCALE_FACTOR) - (ParentBone->getWSPos(CurrentTime, false) * WS_SCALE_FACTOR);
			Orientation = Bone->getWSRot(CurrentTime, false) * ParentBone->getWSRot(CurrentTime, false).inverse();
			ScaleMatrix = Bone->getWSScale(CurrentTime, false) * ParentBone->getWSScale(CurrentTime, false).inverse();
		}
		Orientation.getValue(Bone->getRotationOrder(), VectorRotation);
		VectorRotation.m_x = VectorRotation.m_x * FBXSDK_180_DIV_PI;
		VectorRotation.m_y = VectorRotation.m_y * FBXSDK_180_DIV_PI;
		VectorRotation.m_z = VectorRotation.m_z * FBXSDK_180_DIV_PI;
		VectorScale.m_x = ScaleMatrix.row(0)[0];
		VectorScale.m_y = ScaleMatrix.row(1)[1];
		VectorScale.m_z = ScaleMatrix.row(2)[2];

		// Set the frame
		FbxTime Time;
		Time.SetFrame(Frame);
		int KeyIndex = 0;

		// Position
		setKey(KeyIndex, Time, AnimBaseLayer, Node->LclTranslation, "X", Position.m_x);
		setKey(KeyIndex, Time, AnimBaseLayer, Node->LclTranslation, "Y", Position.m_y);
		setKey(KeyIndex, Time, AnimBaseLayer, Node->LclTranslation, "Z", Position.m_z);

		// Rotation
		setKey(KeyIndex, Time, AnimBaseLayer, Node->LclRotation, "X", VectorRotation.m_x);
		setKey(KeyIndex, Time, AnimBaseLayer, Node->LclRotation, "Y", VectorRotation.m_y);
		setKey(KeyIndex, Time, AnimBaseLayer, Node->LclRotation, "Z", VectorRotation.m_z);

		// Scale
		setKey(KeyIndex, Time, AnimBaseLayer, Node->LclScaling, "X", VectorScale.m_x);
		setKey(KeyIndex, Time, AnimBaseLayer, Node->LclScaling, "Y", VectorScale.m_y);
		setKey(KeyIndex, Time, AnimBaseLayer, Node->LclScaling, "Z", VectorScale.m_z);
	}
}

void DzRobloxUtils::FACSexportAnimation(DzNode* pNode, QString sFacsAnimOutputFilename, bool bAsciiMode )
{
	if (!pNode) return;

	DzSkeleton* Skeleton = pNode->getSkeleton();
	DzFigure* Figure = Skeleton ? qobject_cast<DzFigure*>(Skeleton) : NULL;

	if (!Figure) return;

	// Setup FBX Exporter
	FbxManager* SdkManager = FbxManager::Create();

	FbxIOSettings* ios = FbxIOSettings::Create(SdkManager, IOSROOT);
	SdkManager->SetIOSettings(ios);



	int FileFormat = -1;
	FileFormat = SdkManager->GetIOPluginRegistry()->GetNativeWriterFormat();
	if (bAsciiMode) {
		FileFormat = SdkManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)");
	}

	FbxExporter* Exporter = FbxExporter::Create(SdkManager, "");
	if (!Exporter->Initialize(sFacsAnimOutputFilename.toLocal8Bit().data(), FileFormat, SdkManager->GetIOSettings()))
	{
		return;
	}

	// Get the Figure Scale
	float FigureScale = pNode->getScaleControl()->getValue();

	// Create the Scene
	FbxScene* Scene = FbxScene::Create(SdkManager, "");

	FbxGlobalSettings* pGlobalSettings = &Scene->GetGlobalSettings();
	pGlobalSettings->SetOriginalUpAxis(FbxAxisSystem::DirectX);
	pGlobalSettings->SetSystemUnit(FbxSystemUnit::cm);

	FbxAnimStack* AnimStack = FbxAnimStack::Create(Scene, "AnimStack");
	FbxAnimLayer* AnimBaseLayer = FbxAnimLayer::Create(Scene, "Layer0");
	AnimStack->AddMember(AnimBaseLayer);

	// Add the skeleton to the scene
	QMap<DzNode*, FbxNode*> BoneMap;
	FACSexportSkeleton(pNode, nullptr, nullptr, Scene, BoneMap);
	// add additional bones from followers
	for (int i = 0; i < pNode->getNumNodeChildren(); i++) {
		DzNode* pChild = pNode->getNodeChild(i);
		DzFigure* pFigChild = qobject_cast<DzFigure*>(pChild);
		if (pFigChild && pFigChild->getFollowTarget() == Skeleton) {
			FACSexportSkeleton(pChild, nullptr, nullptr, Scene, BoneMap);
		}
	}

	FbxTime oFbxStartTime;
	FbxTime oFbxEndTime;
	FbxTimeSpan oFbxTimeSpan;
	DzTimeRange oAnimRange = dzScene->getAnimRange();
	DzTime tTimeStep = dzScene->getTimeStep();
	oFbxStartTime.SetFrame(oAnimRange.getStart() / tTimeStep);
	oFbxEndTime.SetFrame(oAnimRange.getEnd() / tTimeStep);
	oFbxTimeSpan.Set(oFbxStartTime, oFbxEndTime);
	
	AnimStack->SetLocalTimeSpan(oFbxTimeSpan);
	AnimStack->SetReferenceTimeSpan(oFbxTimeSpan);

	FACSexportNodeAnimation(Figure, BoneMap, AnimBaseLayer, FigureScale);

	// Iterate the bones
	DzBoneList Bones;
	Skeleton->getAllBones(Bones);
	for (auto Bone : Bones)
	{
		FACSexportNodeAnimation(Bone, BoneMap, AnimBaseLayer, FigureScale);
	}
	for (int i = 0; i < pNode->getNumNodeChildren(); i++) {
		DzNode* pChild = pNode->getNodeChild(i);
		DzFigure* pFigChild = qobject_cast<DzFigure*>(pChild);
		if (pFigChild && pFigChild->getFollowTarget() == Skeleton) {
			DzSkeleton* pSkeletonChild = pFigChild->getSkeleton();
			DzBoneList ChildBones;
			pSkeletonChild->getAllBones(ChildBones);
			for (auto ChildBone : ChildBones)
			{
				if (Bones.contains(ChildBone) == false)
					FACSexportNodeAnimation(ChildBone, BoneMap, AnimBaseLayer, FigureScale);
			}
		}
	}


	//// Get a list of animated properties
	//if (m_bAnimationExportActiveCurves)
	//{
	//	QList<DzNumericProperty*> animatedProperties = getAnimatedProperties(m_pSelectedNode);
	//	exportAnimatedProperties(animatedProperties, Scene, AnimBaseLayer);
	//}

	// Write the FBX
	Exporter->Export(Scene);
	Exporter->Destroy();
}

bool DzRobloxUtils::setMorphKeyFrame(DzProperty* prop, double fStrength, int nFrame) {
	DzTime timeStep = dzScene->getTimeStep();
	DzTime tmPrevFrameTime = (nFrame - 1) * timeStep;
	DzTime tmCurrentFrameTime = nFrame * timeStep;
	DzTime tmNextFrameTime = (nFrame + 1) * timeStep;
	DzFloatProperty* floatProp = qobject_cast<DzFloatProperty*>(prop);
	if (floatProp)
	{
		int key;
		if (floatProp->isKey(tmPrevFrameTime, key) == false) floatProp->setValue(tmPrevFrameTime, 0.0);
		floatProp->setValue(tmCurrentFrameTime, fStrength);
		floatProp->setValue(tmNextFrameTime, 0.0);
	}

	return true;
}

DzProperty* DzRobloxUtils::loadMorph(QMap<QString, MorphInfo>* morphInfoTable, QString sMorphName) {
	DzProperty* prop = NULL;
	auto result = morphInfoTable->find(sMorphName);
	if (result != morphInfoTable->end())
	{
		MorphInfo morphInfo = result.value();
		prop = morphInfo.Property;
	}

	return prop;
}

bool DzRobloxUtils::processElementRecursively(QMap<QString, MorphInfo>* morphInfoTable, int nFrame, DzJsonElement el, double& current_fStrength, const double default_fStrength) {

	if (el.isNumberValue()) {
		current_fStrength = el.numberValue(default_fStrength);
	}
	else if (el.isStringValue()) {
		QString morphName = el.stringValue();
		DzProperty* prop = loadMorph(morphInfoTable, morphName);
		setMorphKeyFrame(prop, current_fStrength, nFrame);
		// reset fStrength after use
		current_fStrength = default_fStrength;
	}
	else if (el.isArray()) {
		DzJsonArray array = el.toArray();
		double sub_fStrength = default_fStrength;
		foreach(DzJsonElement sub_el, array.getItems()) {
			processElementRecursively(morphInfoTable, nFrame, sub_el, sub_fStrength, default_fStrength);
		}
	}

	return true;
}

bool DzRobloxUtils::generateFacs50(DzRobloxAction* that)
{
	if (dzScene->getPrimarySelection() == NULL) return false;

	//MvcTools::testMvc(dzScene->getPrimarySelection());

	dzScene->setFrame(1);
	DzTimeRange range = dzScene->getAnimRange();
	DzTime endTime = range.getEnd();
	DzTime timeStep = dzScene->getTimeStep();
	float frame = endTime / timeStep;
	dzScene->setTime(endTime);

	DzTime frame01_time = 1 * timeStep;

	QString jsonFilename = "C:/GitHub/DazToRoblox-daz3d/PluginData/Daz_FACS_List.json";
	QFile fileFacs(jsonFilename);
	if (fileFacs.open(QIODevice::ReadOnly) == false)
	{
		return false;
	}
	DzJsonReader jsonReader(&fileFacs);
	DzJsonDomParser jsonParser;
	jsonReader.read(&jsonParser);
	fileFacs.close();
	DzJsonElement el = jsonParser.getRoot();
	DzJsonObject obj;
	DzJsonArray array;
	if (el.isObject()) {
		obj = el.toObject();
		DzJsonElement el2 = obj.getMember("FACS");
		array = el2.toArray();
	}
	else {
		array = el.toArray();
	}
	int numMorphs = array.itemCount() + 1;
	range.setEnd(numMorphs * timeStep);
	dzScene->setAnimRange(range);
	dzScene->setPlayRange(range);

	int nNextFrame = 1;
	double global_fStrength = 1.0;
	double local_fStrength = global_fStrength;
	DzNode* pMainNode = dzScene->getPrimarySelection();
	QMap<QString, MorphInfo>* morphInfoTable = MorphTools::getAvailableMorphs(pMainNode);
	foreach(DzJsonElement el, array.getItems()) {
		processElementRecursively(morphInfoTable, nNextFrame, el, local_fStrength, global_fStrength);
		nNextFrame++;
	}
	MorphTools::safeDeleteMorphInfoTable(morphInfoTable);
#define FACS50_EXPORT_TONGUE
#ifdef FACS50_EXPORT_TONGUE
	// DB 2024-06-13: configure morph in children (for tongue? but no bones? depending on tongue bone solution, may not want to keep)
	for (int i = 0; i < pMainNode->getNumNodeChildren(); i++) {
		DzNode* pChildNode = pMainNode->getNodeChild(i);
		DzFigure* pChildFig = qobject_cast<DzFigure*>(pChildNode);
		if (pChildFig) {
			int nNextFrame = 1;
			double global_fStrength = 1.0;
			double local_fStrength = global_fStrength;
			QMap<QString, MorphInfo>* morphInfoTable = MorphTools::getAvailableMorphs(pChildFig);
			foreach(DzJsonElement el, array.getItems()) {
				processElementRecursively(morphInfoTable, nNextFrame, el, local_fStrength, global_fStrength);
				nNextFrame++;
			}
			MorphTools::safeDeleteMorphInfoTable(morphInfoTable);
		}
	}
#endif

	QString sFacsAnimOutputFilename = that->m_sDestinationPath + "/facs50.fbx";
	FACSexportAnimation(that->m_pSelectedNode, QString(sFacsAnimOutputFilename).replace(".fbx", "-ascii.fbx"), true);
	FACSexportAnimation(that->m_pSelectedNode, sFacsAnimOutputFilename, false);

	// load facs50.fbx scene???..... won't have mesh....
	// create new class to use with Daz... base it on MvcTest....

	return true;

}

bool DzRobloxUtils::generateBlenderBatchFile(QString batchFilePath, QString sBlenderExecutablePath, QString sCommandArgs)
{
	// 4. Generate manual batch file to launch blender scripts
	QString sBatchString = QString("\"%1\"").arg(sBlenderExecutablePath);
	foreach(QString arg, sCommandArgs.split(";"))
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
	QFile batchFileOut(batchFilePath);
	bool bResult = batchFileOut.open(QIODevice::WriteOnly | QIODevice::OpenModeFlag::Truncate);
	if (bResult) {
		batchFileOut.write(sBatchString.toAscii().constData());
		batchFileOut.close();
	}
	else {
		dzApp->log("ERROR: DazToRoblox: generateBlenderBatchFile(): Unable to open batch filr for writing: " + batchFilePath);
	}

	return true;
}

DzRobloxAction::DzRobloxAction() :
	DzBridgeAction(tr("&Roblox Studio Exporter"), tr("Export the selected character for Roblox Studio."))
{
	m_nNonInteractiveMode = 0;
	m_sAssetType = QString("__");

	//Setup Icon
	QString iconName = "Daz to Roblox";
	QPixmap basePixmap = QPixmap::fromImage(getEmbeddedImage(iconName.toLatin1()));
	QIcon icon;
	icon.addPixmap(basePixmap, QIcon::Normal, QIcon::Off);
	QAction::setIcon(icon);

	m_bConvertToPng = true;
	m_bConvertToJpg = true;
	m_bExportAllTextures = true;
	m_bCombineDiffuseAndAlphaMaps = true;
	m_bGenerateNormalMaps = true;
	m_bResizeTextures = true;
	m_qTargetTextureSize = QSize(m_nRobloxTextureSize, m_nRobloxTextureSize);
	m_bMultiplyTextureValues = true;
	m_bRecompressIfFileSizeTooBig = true;
	m_nFileSizeThresholdToInitiateRecompression = 1024 * 1024 * 19; // 2024-08-01, DB: current roblox image size upload limit of < 20MB

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
	int debug_lchildren_size = (int) lChildren.size();
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

	// Make sure geometry edit tool is not active
	DzViewportMgr* pViewportMgr = mw->getViewportMgr();
	DzViewTool* pNodeTool = pViewportMgr->findTool("Node Selection");
	if (!pNodeTool) {
		pNodeTool = pViewportMgr->getTool(0);
	}

	DzViewTool* pCurrentViewTool = pViewportMgr->getActiveTool();
	QString sToolName = pCurrentViewTool->getName();
	if ( m_nNonInteractiveMode == 0 &&
			(sToolName != "Node Selection" && 
			sToolName != "Scene Navigator" &&
			sToolName != "Universal" &&
			sToolName != "Rotate" && 
			sToolName != "Translate" &&
			sToolName != "Scale"  &&
			sToolName != "ActivePose" &&
			sToolName != "aniMate2" &&
			sToolName != "Region Navigator" &&
			sToolName != "Surface Selection" &&
			sToolName != "Spot Render")
		)
	{
		// issue error message
		int nResult = QMessageBox::warning(0, tr("WARNING"),
			tr("DazToRoblox may not function correctly in the current Tool mode. \n\
Do you want to switch to a compatible Tool mode now?"), QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
		if (nResult == QMessageBox::Cancel)
		{
			return;
		}
		if (nResult == QMessageBox::Yes)
		{
			// select node tool
			pViewportMgr->setActiveTool(pNodeTool);
			DzViewTool* pNewTool = pViewportMgr->getActiveTool();
			QString sNewToolName = pNewTool->getName();
		}
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
							if (m_nNonInteractiveMode == 0)
							{
								QMessageBox::warning(0, tr("Warning"),
									tr("A non-standard UV Set was detected, overlay will NOT be applied."), QMessageBox::Ok);
							}
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
		if (m_aMorphListOverride.isEmpty() == false)
		{
			m_bEnableMorphs = true;
			m_sMorphSelectionRule = m_aMorphListOverride.join("\n1\n");
			m_sMorphSelectionRule += "\n1\n.CTRLVS\n2\nAnything\n0";
			if (m_morphSelectionDialog == nullptr)
			{
				m_morphSelectionDialog = DZ_BRIDGE_NAMESPACE::DzBridgeMorphSelectionDialog::Get(m_bridgeDialog);
			}
			m_MorphNamesToExport.clear();
			foreach(QString morphName, m_aMorphListOverride)
			{
				QString label = m_morphSelectionDialog->GetMorphLabelFromName(morphName);
				m_MorphNamesToExport.append(morphName);
			}
		}
		else
		{
			m_bEnableMorphs = false;
			m_sMorphSelectionRule = "";
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
		DzProgress* exportProgress = new DzProgress("Sending to Roblox Studio...", 50, false, true);
        exportProgress->setCloseOnFinish(false);
        exportProgress->enable(true);
		DzProgress::setCurrentInfo("Sending to DazToRobloxStudio...");

		//Create Daz3D folder if it doesn't exist
		QDir dir;
		dir.mkpath(m_sRootFolder);
		exportProgress->setCurrentInfo("Starting up conversion pipeline...");
		exportProgress->step();

		// export FBX
		bool bExportResult = exportHD(exportProgress);

		if (!bExportResult)
		{
			if (m_nNonInteractiveMode == 0)
			{
				QMessageBox::information(0, "Roblox Studio Exporter",
					tr("Export cancelled."), QMessageBox::Ok);
			}
			exportProgress->finish();
			return;
		}

        exportProgress->setCurrentInfo("Preparing FBX PostProcessing Pipeline...");
		exportProgress->step();
        
		// run blender scripts
		QString sBlenderLogPath = QString("%1/blender.log").arg(m_sDestinationPath);

		// Hardcoded to use FallbackScriptFolder
		bool bUseFallbackScriptFolder = true;
		QStringList aOverrideFilenameList = (QStringList() << "blender_tools.py" << "NodeArrange.py" << 
			"blender_dtu_to_roblox_blend.py" << "blender_dtu_to_avatar_autosetup.py" <<
			"blender_dtu_to_r15_accessories.py" <<
			"roblox_tools.py" << "Daz_Cage_Att_Template.blend" <<
			"game_readiness_tools.py" << "game_readiness_roblox_data.py" <<
			"Genesis9facs50.blend"
			);
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
						dzApp->log("Roblox Studio Exporter: ERROR: Unable to create fallback folder: " + sFallbackFolder + ", will fallback to using temp folder to store script files...");
						sScriptFolderPath = dzApp->getTempPath();
						break;
					}
				}
				QString sOverrideFilePath = sFallbackFolder + "/" + filename;
				QString sTempFilePath = dzApp->getTempPath() + "/" + filename;
				// delete file if it already exists
				if (QFileInfo(sOverrideFilePath).exists() == true)
				{
					dzApp->log(QString("Roblox Studio Exporter: Removing existing override file (%1)...").arg(sOverrideFilePath));
					bool result = QFile(sOverrideFilePath).remove();
					if (!result) {
						dzApp->debug("Roblox Studio Exporter: ERROR: unable to remove existing script from intermediate folder: " + sOverrideFilePath);
					}
				}
				dzApp->log(QString("Roblox Studio Exporter: Found override file (%1), copying from temp folder.").arg(sOverrideFilePath));
				bool result = QFile(sTempFilePath).copy(sOverrideFilePath);
				if (result)
				{
					// if successful, use overridepath as scriptpath
					sScriptFolderPath = QFileInfo(sOverrideFilePath).path();
				}
				else
				{
					dzApp->log("Roblox Studio Exporter: ERROR: Unable to copy script files to scriptfolder: " + sOverrideFilePath);
				}
			}
		}

		/*****************************************************************************************************/
		DzProgress::setCurrentInfo("Automatic Cage and Attachment Retargeting...");
		exportProgress->setCurrentInfo("Automatic Cage and Attachment Retargeting...");
		exportProgress->step();

		if (m_sAssetType.contains("R15") || m_sAssetType.contains("layered") || m_sAssetType.contains("ALL") )
		{
			OpenFBXInterface* openFbx = OpenFBXInterface::GetInterface();
			bool bFailed = false;
			bool bResult = false;
			// 1a. read template fbx
			QString templateFbxFile = "G9_Cage_Att_MVC_Template.fbx";
			QString tempFilePath = dzApp->getTempPath() + "/" + templateFbxFile;
			FbxScene* pTemplateScene = openFbx->CreateScene("My Scene");
			if (openFbx->LoadScene(pTemplateScene, tempFilePath.toLocal8Bit().data()) == false)
			{
				bFailed = true;
				dzApp->log("DzRoblox Cage Retargeting: Load Fbx Scene failed.");
			}
			// 1b. read morphed fbx
			FbxScene* pMorphedSourceScene = openFbx->CreateScene("My Scene");
			if (openFbx->LoadScene(pMorphedSourceScene, m_sDestinationFBX.toLocal8Bit().data()) == false)
			{
				bFailed = true;
				dzApp->log("DzRoblox Cage Retargeting: Load Fbx Scene failed.");
			}
			// 2a. extract template mesh
			FbxNode* pTemplateMesh = pTemplateScene->FindNodeByName("OriginalGenesis9");
			if (!pTemplateMesh)
			{
				pTemplateMesh = pTemplateScene->FindNodeByName("Genesis9");
				if (!pTemplateMesh)
				{
					bFailed = true;
					dzApp->log("DzRoblox Cage Retargeting: Unable to extract template mesh.");
				}
			}
			// 2b. extract morphed mesh
			FbxNode* pSourceNode = pMorphedSourceScene->FindNodeByName("Genesis9.Shape");
			if (!pSourceNode)
			{
				bFailed = true;
				dzApp->log("DzRoblox Cage Retargeting: Unable to extract source mesh.");
			}
			FbxGeometry* pSourceGeo = openFbx->FindGeometry(pMorphedSourceScene, "Genesis9.Shape");

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
			if (!characterRoot)
			{
				bFailed = true;
				QString sCriticalException = "DzRoblox Cage Retargeting: Critical Exception: Unable to find Genesis9 node in: " + m_sDestinationFBX + ", aborting.";
				dzApp->log(sCriticalException);
				exportProgress->setCurrentInfo(sCriticalException);
				exportProgress->cancel();
				return;
			}
			// local transform
//			FbxDouble3 scale = characterRoot->LclScaling.Get();
//			FbxDouble3 newScale = FbxDouble3(scale[0] / 30, scale[1] / 30, scale[2] / 30);
//			characterRoot->LclScaling.Set(newScale);

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
			QStringList aAttachmentNames;
			aAttachmentNames << "Hair_Att" << "Hat_Att" << "FaceCenter_Att" << "FaceFront_Att" <<
				"Neck_Att" << "LeftCollar_Att" << "RightCollar_Att" << "BodyBack_Att" << "BodyFront_Att" <<
				"Root_Att" << "WaistFront_Att" << "WaistBack_Att" << "WaistCenter_Att" <<
				"LeftShoulder_Att" << "RightShoulder_Att" << "LeftGrip_Att" << "RightGrip_Att" <<
				"LeftFoot_Att" << "RightFoot_Att";
			foreach(QString sNodeName, aCageNames)
			{
				// 3b. for each vertex of cage or att
				// 3c. calculate f(original mesh, vertex) == mvcweights
				// 4a. calculate deformation for each vertex:
				// 4b. mvcweights * morphed mesh ==> deformed vertex
				// 5. replace vertex in morphed fbx

				FbxNode* pCageNode = pMorphedSourceScene->FindNodeByName(sNodeName.toLocal8Bit().data());
				if (pCageNode)
				{
					// make weights with originalgenesis9 + cage
					// get cage vertexbuffer
					FbxMesh* pCageMesh = pCageNode->GetMesh();
					MvcCustomCageRetargeter* pCageRetargeter = new MvcCustomCageRetargeter();
					pCageRetargeter->createMvcWeights(pTemplateMesh->GetMesh(), pCageMesh, exportProgress);

					// make new cage with weights * newgenesis9
					int numCageVerts = pCageMesh->GetControlPointsCount();
					FbxVector4* pFinalCageBuffer = pCageMesh->GetControlPoints();
					FbxVector4* pWorkInProgressCageBuffer = new FbxVector4[numCageVerts];
					memcpy(pWorkInProgressCageBuffer, pFinalCageBuffer, sizeof(FbxVector4) * numCageVerts);

					bool bUseHardCodeWorkaround = false;
					if (sNodeName == "Head_OuterCage") {
//						bUseHardCodeWorkaround = true;
					}
					bool bResult = pCageRetargeter->deformCage(pSourceNode->GetMesh(), pCageMesh, pWorkInProgressCageBuffer, bUseHardCodeWorkaround);
					if (!bResult) {
						delete[] pWorkInProgressCageBuffer;
						if (m_nNonInteractiveMode == 0) 
						{
							QMessageBox::critical(0, "Roblox Studio Exporter",
								tr("Critical Error occured during Cage Retargeting.  Unable to continue export operation.  Aborting."), QMessageBox::Ok);
						}
						exportProgress->cancel();
						exportProgress->setCloseOnFinish(false);
						exportProgress->finish();
						return;
					}
					// update cage with verts from new cage
					memcpy(pFinalCageBuffer, pWorkInProgressCageBuffer, sizeof(FbxVector4) * numCageVerts);
					delete[] pWorkInProgressCageBuffer;
				}

			}

			// Attachments
			foreach(QString sNodeName, aAttachmentNames)
			{
				FbxNode* pAttNode = pMorphedSourceScene->FindNodeByName(sNodeName.toLocal8Bit().data());
				if (pAttNode)
				{
					// make weights with originalgenesis9 + cage
					// get cage vertexbuffer
					FbxMesh* pAttMesh = pAttNode->GetMesh();
					MvcCustomCageRetargeter* pCageRetargeter = new MvcCustomCageRetargeter();
					pCageRetargeter->createMvcWeights(pTemplateMesh->GetMesh(), pAttMesh, exportProgress);

					// make new cage with weights * newgenesis9
					int numAttVerts = pAttMesh->GetControlPointsCount();
					FbxVector4* pFinalAttBuffer = pAttMesh->GetControlPoints();
					FbxVector4 original_cloudCenter = FbxTools::CalculatePointCloudCenter(pFinalAttBuffer, numAttVerts);

					FbxVector4* pWorkInProgressAttBuffer = new FbxVector4[numAttVerts];
					memcpy(pWorkInProgressAttBuffer, pFinalAttBuffer, sizeof(FbxVector4) * numAttVerts);

					pCageRetargeter->deformCage(pSourceNode->GetMesh(), pAttMesh, pWorkInProgressAttBuffer);
					FbxVector4 new_cloudCenter = FbxTools::CalculatePointCloudCenter(pWorkInProgressAttBuffer, numAttVerts);

					FbxVector4 deformOffset = new_cloudCenter - original_cloudCenter;
					// create matrix transform and bake to final buffer
					FbxAMatrix transform;
					transform.SetT(deformOffset);
					FbxTools::BakePoseToVertexBuffer(pFinalAttBuffer, &transform, nullptr, pAttMesh);

					delete[] pWorkInProgressAttBuffer;
				}

			}

			// Remove Template Mesh ( OriginalGenesis9 )
			//pTemplateMesh
			int numChildren = pTemplateMesh->GetChildCount(true);
			for (int i = numChildren - 1; i >= 0; i--)
			{
				FbxNode* pChild = pTemplateMesh->GetChild(i);
				if (!pChild) continue;
				int numMats = pChild->GetMaterialCount();
				for (int matIndex = numMats - 1; matIndex >= 0; matIndex--)
				{
					FbxSurfaceMaterial* pMat = pChild->GetMaterial(matIndex);
					pChild->RemoveMaterial(pMat);
					pMat->Destroy();
				}
				pChild->RemoveAllMaterials();
				pMorphedSourceScene->RemoveGeometry(pChild->GetGeometry());
				pMorphedSourceScene->RemoveNode(pChild);
				pChild->GetGeometry()->Destroy();
				pChild->Destroy();
			}
			int numMats = pTemplateMesh->GetMaterialCount();
			for (int matIndex = numMats - 1; matIndex >= 0; matIndex--)
			{
				FbxSurfaceMaterial* pMat = pTemplateMesh->GetMaterial(matIndex);
				pTemplateMesh->RemoveMaterial(pMat);
				pMat->Destroy();
			}
			pTemplateMesh->RemoveAllMaterials();
			pMorphedSourceScene->RemoveGeometry(pTemplateMesh->GetGeometry());
			pMorphedSourceScene->RemoveNode(pTemplateMesh);
			pTemplateMesh->GetGeometry()->Destroy();
			pTemplateMesh->Destroy();

			// 6. save morphed fbx as morphed fbx with deformed cage/att
			//QString morphedOutputFilename = "morphedFile.fbx";
			//QString morphedOutputPath = m_sDestinationPath + "/" + morphedOutputFilename;
			QString morphedOutputPath = m_sDestinationFBX;
			// backup original Destination FBX
			QFile rawFBX(m_sDestinationFBX);
			QString rawFbxFilename = QString(m_sDestinationFBX).replace(".fbx", "_raw.fbx", Qt::CaseInsensitive);
			QFileInfo rawFi(rawFbxFilename);
			if (rawFi.exists()) {
				QFile existingFile(rawFbxFilename);
				existingFile.remove();
			}
			rawFBX.copy(rawFbxFilename);
			rawFBX.remove();

			// type 1 == ascii
			if (openFbx->SaveScene(pMorphedSourceScene, morphedOutputPath.toLocal8Bit().data()) == false)
			{
				bFailed = true;
				dzApp->log("DzRoblox Cage Retargeting: Load Source Fbx Scene failed.");
			}

		}

		/*****************************************************************************************************/

//		DzRobloxUtils::generateFacs50(this);

		QString sScriptPath;
		QString sScriptPath_R15 = sScriptFolderPath + "/blender_dtu_to_roblox_blend.py";
		QString sScriptPath_S1 = sScriptFolderPath + "/blender_dtu_to_avatar_autosetup.py";
		QString sScriptPath_Accessories = sScriptFolderPath + "/blender_dtu_to_r15_accessories.py";

		if (m_sAssetType.contains("R15"))
		{
			sScriptPath = sScriptFolderPath + "/blender_dtu_to_roblox_blend.py";
		}
		else if (m_sAssetType.contains("S1"))
		{
			sScriptPath = sScriptFolderPath + "/blender_dtu_to_avatar_autosetup.py";
		}
		else if (m_sAssetType.contains("layered") || m_sAssetType.contains("rigid"))
		{
			sScriptPath = sScriptFolderPath + "/blender_dtu_to_r15_accessories.py";
		}
		QString sCommandArgs = QString("--background;--log-file;%1;--python-exit-code;%2;--python;%3;%4").arg(sBlenderLogPath).arg(m_nPythonExceptionExitCode).arg(sScriptPath).arg(m_sDestinationFBX);
#ifdef WIN32
		QString batchFilePath = m_sDestinationPath + "/manual_blender_script.bat";
#elif defined(__APPLE__)
		QString batchFilePath = m_sDestinationPath + "/manual_blender_script.sh";
#endif

		QString sCommandArgs_R15 = QString("--background;--log-file;%1;--python-exit-code;%2;--python;%3;%4").arg(sBlenderLogPath).arg(m_nPythonExceptionExitCode).arg(sScriptPath_R15).arg(m_sDestinationFBX);
		QString batchFilePath_R15 = QString(batchFilePath).replace("_script.", "_script_R15_Avatar.");
		DzRobloxUtils::generateBlenderBatchFile(batchFilePath_R15, m_sBlenderExecutablePath, sCommandArgs_R15);

		QString sCommandArgs_S1 = QString("--background;--log-file;%1;--python-exit-code;%2;--python;%3;%4").arg(sBlenderLogPath).arg(m_nPythonExceptionExitCode).arg(sScriptPath_S1).arg(m_sDestinationFBX);
		QString batchFilePath_S1 = QString(batchFilePath).replace("_script.", "_script_S1_Avatar.");
		DzRobloxUtils::generateBlenderBatchFile(batchFilePath_S1, m_sBlenderExecutablePath, sCommandArgs_S1);

		QString sCommandArgs_Accessories = QString("--background;--log-file;%1;--python-exit-code;%2;--python;%3;%4").arg(sBlenderLogPath).arg(m_nPythonExceptionExitCode).arg(sScriptPath_Accessories).arg(m_sDestinationFBX);
		QString batchFilePath_Accessories = QString(batchFilePath).replace("_script.", "_script_Accessories.");
		DzRobloxUtils::generateBlenderBatchFile(batchFilePath_Accessories, m_sBlenderExecutablePath, sCommandArgs_Accessories);

		bool retCode = false;
		if (m_sAssetType == "ALL") {
			// R15
//			DzProgress::setCurrentInfo("Starting Blender Processing... R15 Avatar");
			exportProgress->setCurrentInfo("Starting Blender Processing. Generating R15 Avatar...");
			retCode = executeBlenderScripts(m_sBlenderExecutablePath, sCommandArgs_R15);

			// S1
//			DzProgress::setCurrentInfo("... S1 Avatar");
			exportProgress->setCurrentInfo("Generating S1 Avatar...");
			retCode = executeBlenderScripts(m_sBlenderExecutablePath, sCommandArgs_S1);

			// Accessories
//			DzProgress::setCurrentInfo("... Accessories");
			exportProgress->setCurrentInfo("Generating Avatar Accessories...");
			retCode = executeBlenderScripts(m_sBlenderExecutablePath, sCommandArgs_Accessories);
		}
		else
		{
			DzProgress::setCurrentInfo("Starting Blender Processing...");
			exportProgress->setCurrentInfo("Starting Blender Processing...");
			retCode = executeBlenderScripts(m_sBlenderExecutablePath, sCommandArgs);
		}

        exportProgress->setCurrentInfo("Roblox Studio Exporter: Export Phase Completed.");
		// DB 2021-10-11: Progress Bar
		exportProgress->finish();

		// DB 2021-09-02: messagebox "Export Complete"
		if (m_nNonInteractiveMode == 0)
		{
			if (retCode)
			{
				QMessageBox::information(0, "Roblox Studio Exporter",
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
				// custom message for code 11 (Python Error)
				if (m_nBlenderExitCode == m_nPythonExceptionExitCode) {
					QString sErrorString;
					sErrorString += QString("An error occured while running the Blender Python script (ExitCode=%1).\n").arg(m_nBlenderExitCode);
					sErrorString += QString("\nPlease check log files at : %1\n").arg(m_sDestinationPath);
					sErrorString += QString("\nYou can rerun the Blender command-line script manually using: %1").arg(batchFilePath);
					QMessageBox::critical(0, "Roblox Studio Exporter", tr(sErrorString.toLocal8Bit()), QMessageBox::Ok);
				}
				else {
					QString sErrorString;
					sErrorString += QString("An error occured during the export process (ExitCode=%1).\n").arg(m_nBlenderExitCode);
					sErrorString += QString("Please check log files at : %1\n").arg(m_sDestinationPath);
					QMessageBox::critical(0, "Roblox Studio Exporter", tr(sErrorString.toLocal8Bit()), QMessageBox::Ok);
				}
#ifdef WIN32
				ShellExecuteA(NULL, "open", m_sDestinationPath.toLocal8Bit().data(), NULL, NULL, SW_SHOWDEFAULT);
#elif defined(__APPLE__)
				QStringList args;
				args << "-e";
				args << "tell application \"Finder\"";
				args << "-e";
				args << "activate";
				args << "-e";
				args << "select POSIX file \"" + batchFilePath + "\"";
				args << "-e";
				args << "end tell";
				QProcess::startDetached("osascript", args);
#endif
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

	// Plugin-specific items
	writer.addMember("Output Folder", m_sRobloxOutputFolderPath);
	writer.addMember("Bake Single Outfit", m_bBakeSingleOutfit);
	writer.addMember("Texture Size", m_nRobloxTextureSize);
	writer.addMember("Texture Bake Quality", m_nBlenderTextureBakeQuality);

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
	ExportOptions.setBoolValue("doFps", true);
	ExportOptions.setBoolValue("doLocks", false);
	ExportOptions.setBoolValue("doLimits", false);
	ExportOptions.setBoolValue("doBaseFigurePoseOnly", false);
	ExportOptions.setBoolValue("doHelperScriptScripts", false);
	ExportOptions.setBoolValue("doMentalRayMaterials", false);

	// Unable to use this option, since generated files are referenced only in FBX and unknown to DTU
	ExportOptions.setBoolValue("doDiffuseOpacity", false);
	// disable these options since we use Blender to generate a new FBX with embedded files
	ExportOptions.setBoolValue("doEmbed", false);
	ExportOptions.setBoolValue("doCopyTextures", false);

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
		m_bEnableBreastsGone = pRobloxDialog->m_wBreastsGoneCheckbox->isChecked();
		m_bBakeSingleOutfit = pRobloxDialog->m_wBakeSingleOutfitCheckbox->isChecked();

		// modesty overlay
		QVariant vItemData = pRobloxDialog->m_wModestyOverlayCombo->itemData(pRobloxDialog->m_wModestyOverlayCombo->currentIndex());
		if (vItemData.type() == QVariant::String) {
			m_nModestyOverlay = eModestyOverlay::CustomModestyOverlay;
			m_sModestyOverlayCustomFilePath = vItemData.toString();
		}
		else {
			m_nModestyOverlay = vItemData.toInt();
			m_sModestyOverlayCustomFilePath = "";
		}
	}
	else
	{
		// TODO: issue error and fail gracefully
		dzApp->log("Roblox Studio Exporter: ERROR: Roblox Dialog was not initialized.  Cancelling operation...");

		return false;
	}

	// if BreastsGone enabled, check if morph exists
	if (m_bEnableBreastsGone)
	{
		auto result = m_AvailableMorphsTable.find("body_bs_BreastsGone");
		if (result == m_AvailableMorphsTable.end())
		{
			// NOTIFY USER THAT BreastsGone morph not found/notinstalled
			QString sErrorString;
			sErrorString += QString("The Breasts Gone Morph was not found.  ");
			sErrorString += QString("Make sure you have Genesis 9 Body Shapes Add-On installed to use the Breasts Gone option.");
			sErrorString += QString("\n\nDo you want to continue without the Breasts Gone option?  Please make sure that your character will pass Roblox Moderation before attempting to upload.");
			sErrorString += QString("\n\nPress Cancel to cancel the export now.");
			auto result = QMessageBox::warning(0, "Roblox Studio Exporter", tr(sErrorString.toLocal8Bit()), QMessageBox::Yes | QMessageBox::Cancel);
			if (result == QMessageBox::Cancel) {
				return false;
			}
		}
	}

	return true;
}

bool DzRobloxAction::executeBlenderScripts(QString sFilePath, QString sCommandlineArguments)
{
	// fork or spawn child process
	QString sWorkingPath = m_sDestinationPath;
	QStringList args = sCommandlineArguments.split(";");

	float fTimeoutInSeconds = 2 * 60;
	float fMilliSecondsPerTick = 200;
	int numTotalTicks = fTimeoutInSeconds * 1000 / fMilliSecondsPerTick;
	DzProgress* progress = new DzProgress("Running Blender Script", numTotalTicks, false, true);
	progress->enable(true);
	QProcess* pToolProcess = new QProcess(this);
	pToolProcess->setWorkingDirectory(sWorkingPath);
	pToolProcess->start(sFilePath, args);
	int currentTick = 0;
	int timeoutTicks = numTotalTicks;
	bool bUserInitiatedTermination = false;
	while (pToolProcess->waitForFinished(fMilliSecondsPerTick) == false) {
		// if timeout reached, then terminate process
		if (currentTick++ > timeoutTicks) {
			if (!bUserInitiatedTermination) 
			{
				QString sTimeoutText = tr("\
The current Blender operation is taking a long time.\n\
Do you want to Ignore this message and keep going. or\
Do you want to Abort the operation?");
				int result = QMessageBox::critical(0, 
					tr("Daz To Roblox: Blender Timout Error"), 
					sTimeoutText, 
					QMessageBox::Ignore, 
					QMessageBox::Abort);
				if (result == QMessageBox::Ignore) 
				{
					int snoozeTime = 60 * 1000 / fMilliSecondsPerTick;
					timeoutTicks += snoozeTime;
				}
				else 
				{
					bUserInitiatedTermination = true;
				}
			}
			else 
			{
				if (currentTick - timeoutTicks < 5)
				{
					pToolProcess->terminate();
				}
				else
				{
					pToolProcess->kill();
				}
			}
		}
		if (pToolProcess->state() == QProcess::Running)
		{
			progress->step();
		}
		else
		{
			break;
		}
	}
    progress->setCurrentInfo("Blender Script Completed.");
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
			dzApp->log(QString("Roblox Studio Exporter: ERROR: Python error:.... %1").arg(m_nBlenderExitCode));
		}
		else
		{
            dzApp->log(QString("Roblox Studio Exporter: ERROR: exit code = %1").arg(m_nBlenderExitCode));
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

// DB 2024-06-01 ASSUMPTIONS: 
// This method assumes that it is being run ONCE per export operation, on a single G9 character.
// It also assumes that only one G9 character is in the scene.
bool DzRobloxAction::preProcessScene(DzNode* parentNode)
{
	DzProgress::setCurrentInfo("DazToRobloxStudio Preprocessing...");
	DzProgress robloxPreProcessProgress = DzProgress("DazToRobloxStudio Preprocessing...", 50, false, true);

	DzBridgeAction::preProcessScene(parentNode);

	// Sanity Check
	if (!parentNode || !parentNode->inherits("DzFigure")) {
		// TODO: add gui error message
		dzApp->debug("ERROR: DzRobloxAction: preProcessScene(): invalid parentNode. Returning false.");
		return false;
	}
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
	QString sParentNodeName = parentNode->getName();
	dzApp->debug("DzRobloxAction::preProcessScene() processing node: " + sParentNodeName);
	robloxPreProcessProgress.setCurrentInfo("Processing node: " + sParentNodeName);
	robloxPreProcessProgress.step();

	QString tempPath = dzApp->getTempPath();

	//////////////// Remove incompatible nodes, replace with game-ready equivalents
	// Remove eyebrows
	bool bReplaceEyebrows = true;
	if (bReplaceEyebrows) {
		DzFigure* pFigureNode = qobject_cast<DzFigure*>(dzScene->getPrimarySelection());
		if (pFigureNode && pFigureNode->getName() == "Genesis9")
		{
			for (int i = 0; i < pFigureNode->getNumNodeChildren(); i++) {
				DzNode* pNode = pFigureNode->getNodeChild(i);
				DzFigure* pChild = qobject_cast<DzFigure*>(pNode);
				if (pChild) {
					if (pChild->getName().contains("eyebrow", Qt::CaseInsensitive) ||
						pChild->getLabel().contains("eyebrow", Qt::CaseInsensitive)) 
					{
						robloxPreProcessProgress.setCurrentInfo("Replacing eyebrows...");
						robloxPreProcessProgress.step();
						dzScene->removeNode(pChild);
						// add replacement eyebrows
						if (dzScene->findNodeByLabel("game_engine_eyebrows") == NULL &&
							dzScene->findNode("game_engine_eyebrows_0") == NULL) {
							addAccessory(pFigureNode, tempPath + "/game_engine_eyebrows.duf", "game_engine_eyebrows_0");
						}
					}
				}
			}
		}
	}
	// Remove eyelashes
	bool bReplaceEyelashes = true;
	if (bReplaceEyelashes) {
		robloxPreProcessProgress.setCurrentInfo("Replacing eyebrows...");
		robloxPreProcessProgress.step();
		DzFigure* pFigureNode = qobject_cast<DzFigure*>(dzScene->getPrimarySelection());
		if (pFigureNode && pFigureNode->getName() == "Genesis9")
		{
			for (int i = 0; i < pFigureNode->getNumNodeChildren(); i++) {
				DzNode* pNode = pFigureNode->getNodeChild(i);
				DzFigure* pChild = qobject_cast<DzFigure*>(pNode);
				if (pChild) {
					if (pChild->getName().contains("eyelash", Qt::CaseInsensitive) ||
						pChild->getLabel().contains("eyelash", Qt::CaseInsensitive))
					{
						dzScene->removeNode(pChild);
						// add replacement eyelashes
						if (dzScene->findNodeByLabel("game_engine_eyelashes") == NULL &&
							dzScene->findNode("game_engine_eyelashes_0") == NULL) {
							addAccessory(pFigureNode, tempPath + "/game_engine_eyelashes.duf", "game_engine_eyelashes_0");
						}
					}
				}
			}
		}
	}

	// apply optional morphs
	if (m_bEnableBreastsGone)
	{
		robloxPreProcessProgress.setCurrentInfo("Applying conversion morphs...");
		robloxPreProcessProgress.step();

		// obtain selection
		DzFigure* pFigureNode = qobject_cast<DzFigure*>( dzScene->getPrimarySelection() );
		if (pFigureNode && pFigureNode->getName() == "Genesis9")
		{
			// find Breasts Gone Morph, set to 100%
			auto result = m_AvailableMorphsTable.find("body_bs_BreastsGone");
			if (result != m_AvailableMorphsTable.end())
			{
				MorphInfo morphInfo = result.value();
				DzProperty *prop = morphInfo.Property;
				DzFloatProperty* floatProp = qobject_cast<DzFloatProperty*>(prop);
				DzNumericProperty* numProp = qobject_cast<DzNumericProperty*>(prop);
				if (floatProp)
				{
					floatProp->setValue(1.0);
				}
				else if (numProp)
				{
					numProp->setDoubleValue(1.0);
				}
			}
			else {
				dzApp->log("DazToRoblox: ERROR: preProcessScene(): BreastsGone morph not found.  Continue without it.");
			}
		}
	}

	////// apply geografts
	// check if geograft present
	robloxPreProcessProgress.setCurrentInfo("Applying conversion geografts...");
	robloxPreProcessProgress.step();
	if (dzScene->findNodeByLabel("game_engine_mouth_geograft") == NULL &&
		dzScene->findNode("game_engine_mouth_geograft_0") == NULL) {
		DzNode* mouthNode = dzScene->findNode("Genesis9Mouth");
		if (mouthNode) {
			applyGeograft(mouthNode, tempPath + "/game_engine_mouth_geograft.duf", "game_engine_mouth_geograft_0");
		}
	}
	if (dzScene->findNodeByLabel("game_engine_eye_geograft") == NULL &&
		dzScene->findNode("game_engine_eye_geograft_0") == NULL) {
		DzNode* eyesNode = dzScene->findNode("Genesis9Eyes");
		if (eyesNode) {
			applyGeograft(eyesNode, tempPath + "/game_engine_eye_geograft.duf", "game_engine_eye_geograft_0");
		}
	}
	// groin geograft (no butt cleavage mod)
	if (dzScene->findNodeByLabel("game_engine_groin_geograft") == NULL &&
		dzScene->findNode("game_engine_groin_geograft_0") == NULL) {
		DzNode* baseFigureNode = dzScene->findNode("Genesis9");
		if (baseFigureNode) {
			applyGeograft(baseFigureNode, tempPath + "/game_engine_groin_geograft.duf", "game_engine_groin_geograft_0");
		}
	}

	//QString sPluginFolder = dzApp->getPluginsPath() + "/DazToRoblox";
    QString sPluginFolder = tempPath;

	QString sRobloxBoneConverter = "bone_converter.dsa";
	QString sApplyModestyOverlay = "apply_modesty_overlay_aArgs.dsa";
	QString sGenerateCombinedTextures = "generate_texture_parts.dsa";
	QString sCombineTextureMaps = "combine_texture_parts.dsa";
	QString sAssignCombinedTextures = "assign_combined_textures.dsa";
	QString sBakeHandsAndFeetPose = "bake_hands_and_feet_nogui.dsa";
	QString sScriptFilename = "";
	QScopedPointer<DzScript> Script(new DzScript());

	// APPLY HAND POSE
	bool bApplyHandPose = true;
	if (bApplyHandPose) {
		robloxPreProcessProgress.setCurrentInfo("Applying Roblox hand grip pose...");
		robloxPreProcessProgress.step();
		dzApp->getContentMgr()->openFile(tempPath + "/roblox_hand_grip.duf", true);
	}

	// BAKE HANDS AND FEET
	// DO NOT BAKE HANDS AND FEET IF BONES ARE ALREADY CONVERTED
	// Check for existence of finger and toe bones
	bool bFingerAndToeBonesExist = false;
	DzSkeleton* skeleton = parentNode->getSkeleton();
	if (skeleton->findNodeChild("l_thumb1", true) || skeleton->findNodeChild("l_bigtoe1", true))
	{
		bFingerAndToeBonesExist = true;
	}
	bool bBakeHandsAndFeet = true;
	if (bBakeHandsAndFeet && bFingerAndToeBonesExist) {
		robloxPreProcessProgress.setCurrentInfo("Baking hands and feet pose...");
		robloxPreProcessProgress.step();
		sScriptFilename = dzApp->getTempPath() + "/" + sBakeHandsAndFeetPose;
		// run bone conversion on main figure
		dzScene->selectAllNodes(false);
		dzScene->setPrimarySelection(parentNode);
		Script.reset(new DzScript());
		Script->loadFromFile(sScriptFilename);
		Script->execute();
	}

	/// BONE CONVERSION OPERATION
	robloxPreProcessProgress.setCurrentInfo("Converting to roblox compatible skeleton...");
	robloxPreProcessProgress.step();
	sScriptFilename = sPluginFolder + "/" + sRobloxBoneConverter;
	if (QFileInfo(sScriptFilename).exists() == false) {
		sScriptFilename = dzApp->getTempPath() + "/" + sRobloxBoneConverter;
	}
	// run bone conversion on main figure
	dzScene->selectAllNodes(false);
	dzScene->setPrimarySelection(parentNode);
	Script.reset(new DzScript());
	Script->loadFromFile(sScriptFilename);
	Script->execute();
	// run bone conversion each geograft and attached body part (aka, ALL FOLLOWERS)
	QObjectList conversionList;
	foreach(DzNode *listNode, m_aGeograftConversionHelpers) {
		conversionList.append(listNode);
	}
	conversionList.append(parentNode->getNodeChildren(true));
	foreach(QObject* listNode, conversionList)
	{
		DzFigure *figChild = qobject_cast<DzFigure*>(listNode);
		if (figChild) {
			QString sChildName = figChild->getName();
			QString sChildLabel = figChild->getLabel();
			dzApp->debug(QString("DzRoblox: DEBUG: converting skeleton for: %1 [%2]").arg(sChildLabel).arg(sChildName));
			dzScene->selectAllNodes(false);
			dzScene->setPrimarySelection(figChild);
			Script.reset(new DzScript());
			Script->loadFromFile(sScriptFilename);
			Script->execute();
			DzSkeleton* pFollowTarget = figChild->getFollowTarget();
			figChild->setFollowTarget(NULL);
			figChild->setFollowTarget(pFollowTarget);
		}
	}

	dzScene->selectAllNodes(false);
	dzScene->setPrimarySelection(parentNode);

	/// CHECK FOR ALTERED UV SET, IF FOUND SKIP TEXTURE OPERATIONS
	// 2. get UV property
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
				robloxPreProcessProgress.finish();
				return false;
			}
		}
	}

	if (m_sAssetType.contains("layered") || m_sAssetType.contains("rigid")) {
		robloxPreProcessProgress.finish();
		return false;
	}

	/// TEXTURE OPERATIONS (MODESTY OVERLAY, UV CONVERSION, ETC)
	if (!sApplyModestyOverlay.isEmpty())
	{
		robloxPreProcessProgress.setCurrentInfo("Applying modesty layer...");
		robloxPreProcessProgress.step();
		sScriptFilename = sPluginFolder + "/" + sApplyModestyOverlay;
		if (QFileInfo(sScriptFilename).exists() == false) {
			sScriptFilename = dzApp->getTempPath() + "/" + sApplyModestyOverlay;
		}
		DzScript* ScriptWithArgs = new DzScript();
		ScriptWithArgs->loadFromFile(sScriptFilename);
		QVariantList aArgs;
		QStringList slDiffuseOverlays, slNormalOverlays, slRoughnessOverlays, slMetallicOverlays;

		QString sDiffuseOverlayTorso = "";
		QString sNormalOverlayTorso = "";
		QString sRoughnessOverlayTorso = "";
		QString sMetallicOverlayTorso = "";

		QString sDiffuseOverlayHead = "";
		QString sNormalOverlayHead = "";
		QString sRoughnessOverlayHead = "";
		QString sMetallicOverlayHead = "";

		QString sDiffuseOverlayLegs = "";
		QString sNormalOverlayLegs = "";
		QString sRoughnessOverlayLegs = "";
		QString sMetallicOverlayLegs = "";

		if (m_nModestyOverlay == eModestyOverlay::StraplessBra_Bikini)
		{
			sDiffuseOverlayTorso = sPluginFolder + "/g9_straplessbra_torso_modesty_overlay_d.png";
			sNormalOverlayTorso = sPluginFolder + "/g9_straplessbra_torso_modesty_overlay_nm.png";
			sRoughnessOverlayTorso = sPluginFolder + "/g9_straplessbra_torso_modesty_overlay_r.png";
			sMetallicOverlayTorso = sPluginFolder + "/g9_straplessbra_torso_modesty_overlay_m.png";
		}

		if (m_nModestyOverlay == eModestyOverlay::SportsBra_Shorts)
		{
			sDiffuseOverlayTorso = sPluginFolder + "/g9_sportsbra_torso_modesty_overlay_d.png";
			sNormalOverlayTorso = sPluginFolder + "/g9_sportsbra_torso_modesty_overlay_n.png";
			sRoughnessOverlayTorso = sPluginFolder + "/g9_sportsbra_torso_modesty_overlay_r.png";
			sMetallicOverlayTorso = sPluginFolder + "/g9_sportsbra_torso_modesty_overlay_m.png";

			sDiffuseOverlayLegs = sPluginFolder + "/g9_shorts_legs_modesty_overlay_d.png";
			sNormalOverlayLegs = sPluginFolder + "/g9_shorts_legs_modesty_overlay_n.png";
			sRoughnessOverlayLegs = sPluginFolder + "/g9_shorts_legs_modesty_overlay_r.png";
			sMetallicOverlayLegs = sPluginFolder + "/g9_shorts_legs_modesty_overlay_m.png";
		}
		
		if (m_nModestyOverlay == eModestyOverlay::TankTop_Shorts)
		{
			sDiffuseOverlayHead = sPluginFolder + "/g9_tanktop_head_modesty_overlay_d.png";
			sNormalOverlayHead = sPluginFolder + "/g9_tanktop_head_modesty_overlay_n.png";
			sRoughnessOverlayHead = sPluginFolder + "/g9_tanktop_head_modesty_overlay_r.png";
			sMetallicOverlayHead = sPluginFolder + "/g9_tanktop_head_modesty_overlay_m.png";

			sDiffuseOverlayTorso = sPluginFolder + "/g9_tanktop_torso_modesty_overlay_d.png";
			sNormalOverlayTorso = sPluginFolder + "/g9_tanktop_torso_modesty_overlay_n.png";
			sRoughnessOverlayTorso = sPluginFolder + "/g9_tanktop_torso_modesty_overlay_r.png";
			sMetallicOverlayTorso = sPluginFolder + "/g9_tanktop_torso_modesty_overlay_m.png";

			sDiffuseOverlayLegs = sPluginFolder + "/g9_shorts_legs_modesty_overlay_d.png";
			sNormalOverlayLegs = sPluginFolder + "/g9_shorts_legs_modesty_overlay_n.png";
			sRoughnessOverlayLegs = sPluginFolder + "/g9_shorts_legs_modesty_overlay_r.png";
			sMetallicOverlayLegs = sPluginFolder + "/g9_shorts_legs_modesty_overlay_m.png";
		}

		// if custom, generate template string from selected file, then generate all variants from template string 
		if (m_nModestyOverlay == eModestyOverlay::CustomModestyOverlay)
		{
			// clean and generate template string
			QString sTemplateString = m_sModestyOverlayCustomFilePath;
			sTemplateString = sTemplateString.replace("_d.png", "", Qt::CaseInsensitive);
			sTemplateString = sTemplateString.replace("_nm.png", "", Qt::CaseInsensitive);
			sTemplateString = sTemplateString.replace("_r.png", "", Qt::CaseInsensitive);
			sTemplateString = sTemplateString.replace("_m.png", "", Qt::CaseInsensitive);
			sTemplateString = sTemplateString.replace("_torso_modesty_overlay", "_*_modesty_overlay", Qt::CaseInsensitive);
			sTemplateString = sTemplateString.replace("_legs_modesty_overlay", "_*_modesty_overlay", Qt::CaseInsensitive);
			sTemplateString = sTemplateString.replace("_head_modesty_overlay", "_*_modesty_overlay", Qt::CaseInsensitive);
			// generate variants from template
			sDiffuseOverlayTorso = QString(sTemplateString).replace("_*_modesty_overlay", "_torso_modesty_overlay") + "_d.png";
			sDiffuseOverlayLegs = QString(sTemplateString).replace("_*_modesty_overlay", "_legs_modesty_overlay") + "_d.png";
			sDiffuseOverlayHead = QString(sTemplateString).replace("_*_modesty_overlay", "_head_modesty_overlay") + "_d.png";

			sNormalOverlayTorso = QString(sTemplateString).replace("_*_modesty_overlay", "_torso_modesty_overlay") + "_nm.png";
			sNormalOverlayLegs = QString(sTemplateString).replace("_*_modesty_overlay", "_legs_modesty_overlay") + "_nm.png";
			sNormalOverlayHead = QString(sTemplateString).replace("_*_modesty_overlay", "_head_modesty_overlay") + "_nm.png";

			sRoughnessOverlayTorso = QString(sTemplateString).replace("_*_modesty_overlay", "_torso_modesty_overlay") + "_r.png";
			sRoughnessOverlayLegs = QString(sTemplateString).replace("_*_modesty_overlay", "_legs_modesty_overlay") + "_r.png";
			sRoughnessOverlayHead = QString(sTemplateString).replace("_*_modesty_overlay", "_head_modesty_overlay") + "_r.png";

			sMetallicOverlayTorso = QString(sTemplateString).replace("_*_modesty_overlay", "_torso_modesty_overlay") + "_m.png";
			sMetallicOverlayLegs = QString(sTemplateString).replace("_*_modesty_overlay", "_legs_modesty_overlay") + "_m.png";
			sMetallicOverlayHead = QString(sTemplateString).replace("_*_modesty_overlay", "_head_modesty_overlay") + "_m.png";
		}

		slDiffuseOverlays.append(sDiffuseOverlayTorso); slDiffuseOverlays.append(sDiffuseOverlayLegs); slDiffuseOverlays.append(sDiffuseOverlayHead);
		slNormalOverlays.append(sNormalOverlayTorso); slNormalOverlays.append(sNormalOverlayLegs); slNormalOverlays.append(sNormalOverlayHead);
		slRoughnessOverlays.append(sRoughnessOverlayTorso); slRoughnessOverlays.append(sRoughnessOverlayLegs); slRoughnessOverlays.append(sRoughnessOverlayHead);
		slMetallicOverlays.append(sMetallicOverlayTorso); slMetallicOverlays.append(sMetallicOverlayLegs); slMetallicOverlays.append(sMetallicOverlayHead);

		aArgs.append(QVariant(slDiffuseOverlays));
		aArgs.append(QVariant(slNormalOverlays));
		aArgs.append(QVariant(slRoughnessOverlays));
		aArgs.append(QVariant(slMetallicOverlays));
		ScriptWithArgs->execute(aArgs);
	}
	// copy modesty overlaid materials to geografts
	foreach(auto geograft_helper, m_aGeograftConversionHelpers)
	{
		copyMaterialsToGeograft(geograft_helper);
	}
	dzScene->selectAllNodes(false);
	dzScene->setPrimarySelection(parentNode);

	robloxPreProcessProgress.setCurrentInfo("Generating combined texture parts...");
	robloxPreProcessProgress.step();
	sScriptFilename = sPluginFolder + "/" + sGenerateCombinedTextures;
	if (QFileInfo(sScriptFilename).exists() == false) {
		sScriptFilename = dzApp->getTempPath() + "/" + sGenerateCombinedTextures;
	}
    Script.reset(new DzScript());
	Script->loadFromFile(sScriptFilename);
	Script->execute();

	dzScene->selectAllNodes(false);
	dzScene->setPrimarySelection(parentNode);

	robloxPreProcessProgress.setCurrentInfo("Merging combined texture parts...");
	robloxPreProcessProgress.step();
	sScriptFilename = sPluginFolder + "/" + sCombineTextureMaps;
	if (QFileInfo(sScriptFilename).exists() == false) {
		sScriptFilename = dzApp->getTempPath() + "/" + sCombineTextureMaps;
	}
    Script.reset(new DzScript());
	Script->loadFromFile(sScriptFilename);
	Script->execute();

	dzScene->selectAllNodes(false);
	dzScene->setPrimarySelection(parentNode);

	robloxPreProcessProgress.setCurrentInfo("Applying combined texture...");
	robloxPreProcessProgress.step();
	sScriptFilename = sPluginFolder + "/" + sAssignCombinedTextures;
	if (QFileInfo(sScriptFilename).exists() == false) {
		sScriptFilename = dzApp->getTempPath() + "/" + sAssignCombinedTextures;
	}
    Script.reset(new DzScript());
	Script->loadFromFile(sScriptFilename);
	Script->execute();

	dzScene->selectAllNodes(false);
	dzScene->setPrimarySelection(parentNode);

	robloxPreProcessProgress.setCurrentInfo("DazToRobloxStudio Preprocessing complete.");
	robloxPreProcessProgress.finish();

	return true;
}

bool DzRobloxAction::addAccessory(DzNode* pBaseNode, QString accessoryFilename, QString accessoryNodeName)
{
	DzContentMgr* contentMgr = dzApp->getContentMgr();
	//	QString tempPath = dzApp->getTempPath() + "/" + geograftFilename;
	QFile srcFile(accessoryFilename);
	if (accessoryNodeName == "")
	{
		accessoryNodeName = QFileInfo(accessoryFilename).baseName();
	}
	QString node_name;
	// copy to temp folder and merge tpose into scene
	if (srcFile.exists())
	{
		//		DzBridgeAction::copyFile(&srcFile, &tempPath, true);
			//	if (contentMgr->openFile(tempPath, true))

				// deselect all ndoes
		dzScene->setPrimarySelection(NULL);
		if (contentMgr->openFile(accessoryFilename, true))
		{
			// parent geograft
			DzNode* generic_node = dzScene->findNode(accessoryNodeName);
			if (!generic_node)
				generic_node = dzScene->findNodeByLabel(QString(accessoryNodeName).replace("_0", ""));
			//QString debug_geograftNodeName = generic_geograft_node->getName();
			DzFigure* accessory_node = qobject_cast<DzFigure*>(generic_node);
			if (accessory_node && pBaseNode)
			{
				pBaseNode->addNodeChild(accessory_node, false);
				accessory_node->setFollowTarget(pBaseNode->getSkeleton());
			}
		}
	}
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

				bool bResult = copyMaterialsToGeograft(geograft_node, pBaseNode);

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

bool MvcCustomCageRetargeter::deformCage(const FbxMesh* pMorphedMesh, const FbxMesh* pCage, FbxVector4* pVertexBuffer, bool bUseHardCodeWorkaround)
{
	if (pMorphedMesh == nullptr || pCage == nullptr)
		return false;

	int numCageVerts = pCage->GetControlPointsCount();
	for (int i = 0; i < numCageVerts; i++)
	{
#define HARDCODE_WORKAROUND_1
#ifdef HARDCODE_WORKAROUND_1
		if (bUseHardCodeWorkaround && i == 11)
//		if (i == 11 && QString(pCage->GetName()) == "Head_OuterCage")
		{
			int hard_code_index_lookup = 21;
			auto results = m_MvcWeightsTable.find(hard_code_index_lookup);
			if (results == m_MvcWeightsTable.end()) continue;
			QVector<double>* pMvcWeights = results.value();
			FbxVector4* pMorphedVertexBuffer = pMorphedMesh->GetControlPoints();
			FbxVector4 newVertexPosition = MvcTools::deform_using_mean_value_coordinates(pMorphedMesh, pMorphedVertexBuffer, pMvcWeights);
			newVertexPosition[0] *= -1.0;
			pVertexBuffer[i] = newVertexPosition;
			continue;
		}
#endif

		auto results = m_MvcWeightsTable.find(i);
		if (results == m_MvcWeightsTable.end())
		{
			pVertexBuffer[i] = FbxVector4(NAN, NAN, NAN);
			continue;
		}
		QVector<double>* pMvcWeights = results.value();
		FbxVector4* pMorphedVertexBuffer = pMorphedMesh->GetControlPoints();

		// Sanity Checks
		int numVerts = pMorphedMesh->GetControlPointsCount();
		int numWeights = pMvcWeights->count();
		if (numVerts != numWeights) {
			dzApp->log(QString("CRITICAL ERROR: DzRobloxAction.cpp, MvcCustomCageRetarger::deformCage(): (line1860): numVerts [%1] != numWeights [%2], aborting...").arg(numVerts).arg(numWeights));
			return false;
		}

		//QString name = QString(pCage->GetName());
		//if (i == 11 && name == "Head_OuterCage") {
		//	int break_here = 0;
		//}
		FbxVector4 newVertexPosition = MvcTools::deform_using_mean_value_coordinates(pMorphedMesh, pMorphedVertexBuffer, pMvcWeights);
		//if (newVertexPosition[1] > 215 && name == "Head_OuterCage") {
		//	int break_here = 0;
		//}
		//FbxVector4 findThisVertex(-1.81834, 152.583, 9.08465);
		//double distance = FbxTools::getDistance(newVertexPosition, findThisVertex);
		//if (distance < 1.0 && name == "Head_OuterCage") {
		//	int break_here = 0;
		//}

		pVertexBuffer[i] = newVertexPosition;
	}

	return true;
}

bool DzRobloxAction::copyMaterialsToGeograft(DzNode* pGeograftNode, DzNode* pBaseNode)
{
	DzFigure* pGeograftAsFigureNode = qobject_cast<DzFigure*>(pGeograftNode);
	if (pGeograftNode == NULL || pGeograftAsFigureNode == NULL)
	{
		dzApp->log("DzRobloxAction::copyMaterialsToGeograft(): ERROR: Geograft node is invalid. Returning false.");
		return false;
	}

	if (pBaseNode == NULL) {
		pBaseNode = pGeograftAsFigureNode->getFollowTarget();
		if (pBaseNode == NULL) {
			dzApp->log("DzRobloxAction::copyMaterialsToGeograft(): ERROR: Unable to find base node for Geograft. Returning false.");
			return false;
		}
	}

	// copy base materials to geograft
	DzShape* geograftShape = pGeograftNode->getObject()->getCurrentShape();
	auto dstMatList = geograftShape->getAllMaterials();
	DzShape* baseShape = pBaseNode->getObject()->getCurrentShape();
	auto srcMatList = baseShape->getAllMaterials();
	foreach(QObject * dobj, dstMatList) {
		DzMaterial* dstMat = qobject_cast<DzMaterial*>(dobj);
		foreach(QObject * sobj, srcMatList) {
			DzMaterial* srcMat = qobject_cast<DzMaterial*>(sobj);
			if (dstMat && srcMat) {
				QString cleanedDestMatName = QString(dstMat->getName()).replace(" ", "").replace("_", "").toLower();
				QString cleanedSrcMatName = QString(srcMat->getName()).replace(" ", "").replace("_", "").toLower();
				if (cleanedDestMatName == cleanedSrcMatName) {
					dstMat->copyFrom(srcMat);
					break;
				}
			}
		}
	}

	return true;
}

// DO NOT USE UNLESS DzRobloxDialog is not part of pipeline.  Otherwise, use the prefered DzRobloxDialog::showDisclaimer() instead.
bool DzRobloxAction::showDisclaimer()
{
	QString content = "\
<div><font size=\"4\"><p>By using Daz to Roblox Studio, the user agrees to the following:</p>\
<p><b>Interactive License Requirement:</b></p>\
<p>Importing Daz Characters into Roblox Studio requires an \
<a href=\"https://www.daz3d.com/interactive-license-info\">Interactive License</a> \
because the assets are uploaded to the Roblox servers. The user must have an Interactive License for all \
Daz Studio assets including characters, textures and morphs which are used in the process of exporting and \
uploading characters to the Roblox servers.</p>\
<p><b>Disclaimer:</b></p>\
<p>Roblox uses both automated and human moderation to review assets uploaded to its servers. \
Uploaded assets which are rejected by Roblox moderation may result actions by the Roblox moderation team including removal \
of assets from the Roblox servers and/or banning of the user's Roblox account. It is the user's responsibility to ensure \
assets uploaded to the Roblox servers comply with \
<a href=\"https://en.help.roblox.com/hc/en-us/articles/203313410-Roblox-Community-Standards\">Roblox Community Standards</a>, \
especially regarding Sexual Content and \
<a href=\"https://create.roblox.com/docs/art/marketplace/marketplace-policy#age-appropriate\">Age Appropriate Avatar bodies</a>. \
Daz 3D will not be liable for any damages arising from the use of this software.</p></div>";

	QTextBrowser* wContent = new QTextBrowser();
	wContent->setText(content);
	wContent->setOpenExternalLinks(true);

	QString sWindowTitle = "Daz To Roblox Studio EULA";
	DzBasicDialog* wDialog = new DzBasicDialog(NULL, sWindowTitle);
	wDialog->setWindowTitle(sWindowTitle);
	wDialog->setMinimumWidth(500);
	wDialog->setMinimumHeight(450);
	QGridLayout* layout = new QGridLayout(wDialog);
	layout->addWidget(wContent);
	wDialog->addLayout(layout);
	int result = wDialog->exec();

	if (result == QDialog::DialogCode::Rejected || result != QDialog::DialogCode::Accepted) {
		return false;
	}

	return true;
}

#include "moc_DzRobloxAction.cpp"
