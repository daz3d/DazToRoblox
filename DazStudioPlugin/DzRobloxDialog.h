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

enum eModestyOverlay {
	SportsBra_Shorts = 1,
	TankTop_Shorts = 2,
	StraplessBra_Bikini =3,
	CustomModestyOverlay = -1
};

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
	Q_INVOKABLE bool isBlenderTextBoxValid(const QString& text="");
	Q_INVOKABLE bool isAssetTypeComboBoxValid();
	Q_INVOKABLE bool disableAcceptUntilAllRequirementsValid();

	Q_INVOKABLE bool showDisclaimer();

	DzBasicDialog* m_wEulaAgreementDialog;

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
	bool HandleAcceptButtonValidationFeedback();
	void HandleCustomModestyOverlayActivated(int index);

	void HandleAgreeEulaCheckbox(bool checked);

	void accept() override;

protected:
	QLineEdit* intermediateFolderEdit;
	QPushButton* intermediateFolderButton;
	QComboBox* m_wModestyOverlayCombo;
	QLabel* m_wModestyOverlayRowLabel;

	QLineEdit* m_wRobloxOutputFolderEdit;
	QPushButton* m_wRobloxOutputFolderButton;
	QWidget* m_wGodotProjectFolderRowLabelWidget;

	QLineEdit* m_wBlenderExecutablePathEdit;
	QPushButton* m_wBlenderExecutablePathButton;
	QWidget* m_wBlenderExecutablePathRowLabelWdiget;

	QCheckBox* m_wBreastsGoneCheckbox;
	QCheckBox* m_wBakeSingleOutfitCheckbox;
	QCheckBox* m_wRunTextureAtlas;

	QComboBox* m_wEyebrowReplacement;
	QComboBox* m_wEyelashReplacement;

	virtual void refreshAsset() override;

#ifdef UNITTEST_DZBRIDGE
	friend class UnitTest_DzRobloxDialog;
#endif
};
