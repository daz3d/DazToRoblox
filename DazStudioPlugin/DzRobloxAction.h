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
	static bool generateBlenderBatchFile(QString batchFilePath, QString sBlenderExecutablePath, QString sCommandArgs);

};

#define DAZTOROBLOX_EULA tr("\
<div><font size=\"4\"><p>By using Daz to Roblox Studio, you agree to these Daz to Roblox Studio Terms \
(\"Supplemental Terms\") which you agree supplement your obligations under the \
<a href=\"https://www.daz3d.com/eula/\">Daz End User License Agreement</a> (\"EULA\").</p>\
<p><ol type=\"A\">\
<li>Supplement to Interactive License Restrictions:<br> \
Importing Daz Characters into Roblox Studio requires this supplement to the standard \
<a href=\"https://www.daz3d.com/interactive-license-info\">Daz Interactive License</a>, \
as described in Section 3.0 of the EULA, because the assets are uploaded to the Roblox servers. \
As a limited exception to the EULA and Interactive License, these Supplemental Terms grant you the right to upload \
assets that you have purchased from the Daz store using your own account into a single Roblox account which is also \
your personal account. You hereby agree that:\
(1) you must have an Interactive License (as modified by these Supplemental Terms) for all Daz Studio assets \
including characters, textures and morphs which are used in the process of exporting and \
uploading characters to the Roblox servers; \
(2) these Supplemental Terms are specific to Roblox and do not permit you to upload Daz Studio Assets to any other \
third-party platform; \
(3) you are only permitted to upload the Daz Studio assets to a single Roblox account for your own personal use; and \
(4) you still must purchase all applicable assets from Daz before uploading them to Roblox in accordance with these \
Supplemental Terms.</li><br>\
<li>Monitoring of Content; Liability and Disclaimers:<br> \
You understand and agree that Roblox uses both automated and human moderation to review assets uploaded to its servers. \
Uploaded assets which are rejected by Roblox moderation may result in actions by the Roblox moderation team including \
removal of assets from the Roblox servers and/or banning of your Roblox account. It is the your responsibility to ensure \
that assets uploaded to the Roblox servers comply with \
<a href=\"https://en.help.roblox.com/hc/en-us/articles/203313410-Roblox-Community-Standards\">Roblox Community Standards</a>, \
especially regarding Sexual Content and \
<a href=\"https://create.roblox.com/docs/art/marketplace/marketplace-policy#age-appropriate\">Age Appropriate Avatar bodies</a> \
(\"Roblox Policies\"). Notwithstanding anything to the contrary herin or in the EULA, \
Daz will not be liable for any damages arising from your use of Roblox, breach of the Roblox Policies, \
or any dispute between you and any other user on Roblox. In addition to your indemnification obligations in the EULA, \
you will indemnify, defend and hold Daz harmless from and against any third-party claim, including, without limitation, \
claims brought by Roblox or users of Roblox, relating to your breach of the Roblox Policies or otherwise arising from any \
dispute between you and another Roblox User.</li><br>\
<li>General: The EULA will remain in full force in effect and is modified by these Supplemental Terms solely with respect \
to your use of Daz to Roblox Studio.</li></ol></div>")


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
	Q_INVOKABLE bool resetEula();

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

	Q_INVOKABLE virtual bool addAccessory(DzNode* pBaseNode, QString accessoryFilename, QString accessoryNodeName = "");

	bool m_bEnableBreastsGone = false;
	int m_nModestyOverlay = 0;
	QString m_sModestyOverlayCustomFilePath = "";
	bool m_bBakeSingleOutfit = false;
	int m_nRobloxTextureSize = 1024;
	int m_nBlenderTextureBakeQuality = 1; // number of samples per texel

	int m_nReplaceEyebrows = 0;
	int m_nReplaceEyelashes = 0;

	friend class DzRobloxUtils;

#ifdef UNITTEST_DZBRIDGE
	friend class UnitTest_DzRobloxAction;
#endif

};
