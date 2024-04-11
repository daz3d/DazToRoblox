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

class DzRobloxAction : public DZ_BRIDGE_NAMESPACE::DzBridgeAction {
	Q_OBJECT
	Q_PROPERTY(QString sRobloxOutputFolderPath READ getRobloxOutputFolderPath WRITE setRobloxOutputFolderPath)
	Q_PROPERTY(QString sBlenderExecutablePath READ getBlenderExecutablePath WRITE setBlenderExecutablePath)
public:
	DzRobloxAction();

	Q_INVOKABLE virtual DZ_BRIDGE_NAMESPACE::DzBridgeDialog* getBridgeDialog() override;

	virtual bool preProcessScene(DzNode* parentNode = nullptr) override;

	Q_INVOKABLE QString getRobloxOutputFolderPath() { return this->m_sRobloxOutputFolderPath; };
	Q_INVOKABLE void setRobloxOutputFolderPath(QString arg_Filename) { this->m_sRobloxOutputFolderPath = arg_Filename; };
	Q_INVOKABLE QString getBlenderExecutablePath() { return this->m_sBlenderExecutablePath; };
	Q_INVOKABLE void setBlenderExecutablePath(QString arg_Filename) { this->m_sBlenderExecutablePath = arg_Filename; };

	Q_INVOKABLE bool executeBlenderScripts(QString sFilePath, QString sCommandlineArguments);

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

#ifdef UNITTEST_DZBRIDGE
	friend class UnitTest_DzRobloxAction;
#endif

};
