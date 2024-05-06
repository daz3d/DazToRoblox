#pragma once
#include "dzbasicdialog.h"
#include <QtGui/qcombobox.h>
#include <QtCore/qsettings.h>
#include <DzBridgeDialog.h>

class QPushButton;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QWidget;
class DzRobloxAction;

class UnitTest_DzRobloxDialog;

#include "dzbridge.h"

class DzFileValidator : public QValidator {
public:
	State validate(QString& input, int& pos) const;
};

class DzRobloxDialog : public DZ_BRIDGE_NAMESPACE::DzBridgeDialog{
	friend DzRobloxAction;
	Q_OBJECT
	Q_PROPERTY(QWidget* intermediateFolderEdit READ getIntermediateFolderEdit)
public:
	Q_INVOKABLE QLineEdit* getIntermediateFolderEdit() { return intermediateFolderEdit; }

	/** Constructor **/
	 DzRobloxDialog(QWidget *parent=nullptr);

	/** Destructor **/
	virtual ~DzRobloxDialog() {}

	Q_INVOKABLE void resetToDefaults() override;
	Q_INVOKABLE bool loadSavedSettings() override;
	Q_INVOKABLE void saveSettings() override;

	DzFileValidator m_dzValidatorFileExists;
	Q_INVOKABLE bool disableAcceptUntilBlenderValid(const QString& text="");
	Q_INVOKABLE bool disableAcceptUntilAssetTypeValid();
	Q_INVOKABLE bool disableAcceptUntilAllRequirementsValid();

public slots:
	void HandleTextChanged( const QString &text);

protected slots:
	void HandleSelectIntermediateFolderButton();
	void HandleAssetTypeComboChange(int state);

	virtual void HandleDisabledChooseSubdivisionsButton();
	virtual void HandleOpenIntermediateFolderButton(QString sFolderPath="");

	void HandleSelectRobloxOutputFolderButton();
	void showRobloxOptions(bool bVisible);

	void HandleSelectBlenderExecutablePathButton();

protected:
	QLineEdit* intermediateFolderEdit;
	QPushButton* intermediateFolderButton;

	QLineEdit* m_wRobloxOutputFolderEdit;
	QPushButton* m_wRobloxOutputFolderButton;
	QWidget* m_wGodotProjectFolderRowLabelWidget;

	QLineEdit* m_wBlenderExecutablePathEdit;
	QPushButton* m_wBlenderExecutablePathButton;
	QWidget* m_wBlenderExecutablePathRowLabelWdiget;

	virtual void refreshAsset() override;

#ifdef UNITTEST_DZBRIDGE
	friend class UnitTest_DzRobloxDialog;
#endif
};
