#pragma once
#include <dzaction.h>
#include <dznode.h>
#include <dzjsonwriter.h>
#include <QtCore/qfile.h>
#include <QtCore/qtextstream.h>
#include <DzBridgeAction.h>
#include "DzRobloxDialog.h"

class UnitTest_DzRobloxAction;

#include "dzbridge.h"

namespace DZ_BRIDGE_NAMESPACE
{
	class DzBridgeDialog;
}

#include "MvcTools.h"
class MvcCustomCageRetargeter : public MvcCageRetargeter
{
public:
	virtual bool deformCage(const FbxMesh* pMorphedMesh, const FbxMesh* pCage, FbxVector4* pVertexBuffer) override 
	{
		return deformCage(pMorphedMesh, pCage, pVertexBuffer, false);
	};
	virtual bool deformCage(const FbxMesh* pMorphedMesh, const FbxMesh* pCage, FbxVector4* pVertexBuffer, bool bUseHardCodeWorkaround );
};

class DzJsonElement;
class DzRobloxUtils
{
public:
	static void FACSexportAnimation(DzNode* pNode, QString sFacsAnimOutputFilename, bool bAsciiMode = false);
	static void FACSexportNodeAnimation(DzNode* Bone, QMap<DzNode*, FbxNode*>& BoneMap, FbxAnimLayer* AnimBaseLayer, float FigureScale);
	static void FACSexportSkeleton(DzNode* Node, DzNode* Parent, FbxNode* FbxParent, FbxScene* Scene, QMap<DzNode*, FbxNode*>& BoneMap);
	static bool hasAncestorName(DzNode* pNode, QString sAncestorName, bool bCaseSensitive = true);
	static void setKey(int& KeyIndex, FbxTime Time, FbxAnimLayer* AnimLayer, FbxPropertyT<FbxDouble3>& Property, const char* pChannel, float Value);
	static DzProperty* loadMorph(QMap<QString, MorphInfo>* morphInfoTable, QString sMorphName);
	static bool processElementRecursively(QMap<QString, MorphInfo>* morphInfoTable, int nFrame, DzJsonElement el, double& current_fStrength, const double default_fStrength);
	static bool setMorphKeyFrame(DzProperty* prop, double fStrength, int nFrame);
	static bool generateFacs50(DzRobloxAction* that);

};


class DzRobloxAction : public DZ_BRIDGE_NAMESPACE::DzBridgeAction {
	Q_OBJECT
	Q_PROPERTY(QString sRobloxOutputFolderPath READ getRobloxOutputFolderPath WRITE setRobloxOutputFolderPath)
	Q_PROPERTY(QString sBlenderExecutablePath READ getBlenderExecutablePath WRITE setBlenderExecutablePath)
public:
	DzRobloxAction();

	Q_INVOKABLE virtual DZ_BRIDGE_NAMESPACE::DzBridgeDialog* getBridgeDialog() override;

	virtual bool preProcessScene(DzNode* parentNode = nullptr) override;
	virtual bool undoPreProcessScene() override;

	Q_INVOKABLE QString getRobloxOutputFolderPath() { return this->m_sRobloxOutputFolderPath; };
	Q_INVOKABLE void setRobloxOutputFolderPath(QString arg_Filename) { this->m_sRobloxOutputFolderPath = arg_Filename; };
	Q_INVOKABLE QString getBlenderExecutablePath() { return this->m_sBlenderExecutablePath; };
	Q_INVOKABLE void setBlenderExecutablePath(QString arg_Filename) { this->m_sBlenderExecutablePath = arg_Filename; };

	Q_INVOKABLE bool executeBlenderScripts(QString sFilePath, QString sCommandlineArguments);
	
	Q_INVOKABLE bool deepCopyNode(FbxNode* pDestinationRoot, FbxNode* pSourceNode);
	Q_INVOKABLE bool mergeScenes(FbxScene* pDestinationScene, FbxScene* pSourceScene);

	QList<DzNode*> m_aGeograftConversionHelpers;

	Q_INVOKABLE bool showDisclaimer();

protected:
	unsigned char m_nPythonExceptionExitCode = 11; // arbitrary exit code to check for blener python exceptions

	void executeAction();
	Q_INVOKABLE bool createUI();
	Q_INVOKABLE void writeConfiguration();
	Q_INVOKABLE void setExportOptions(DzFileIOSettings& ExportOptions);
	virtual QString readGuiRootFolder() override;
	Q_INVOKABLE virtual bool readGui(DZ_BRIDGE_NAMESPACE::DzBridgeDialog*) override;

	QString m_sRobloxOutputFolderPath = "";
	QString m_sBlenderExecutablePath = "";
	int m_nBlenderExitCode = 0;

	Q_INVOKABLE virtual bool isAssetMorphCompatible(QString sAssetType) override;
	Q_INVOKABLE virtual bool isAssetMeshCompatible(QString sAsseType) override;
	Q_INVOKABLE virtual bool isAssetAnimationCompatible(QString sAssetType) override;

	Q_INVOKABLE virtual bool applyGeograft(DzNode* pBaseNode, QString geograftFilename, QString geograftNodeName = "");
	Q_INVOKABLE virtual bool copyMaterialsToGeograft(DzNode* pGeograftNode, DzNode* pBaseNode = NULL);

	bool m_bEnableBreastsGone = false;
	int m_nModestyOverlay = 0;

	friend class DzRobloxUtils;

#ifdef UNITTEST_DZBRIDGE
	friend class UnitTest_DzRobloxAction;
#endif

};
