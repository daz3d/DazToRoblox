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
	StraplessBra_Bikini = 3,
	CustomModestyOverlay = -1
};

enum eReplacementAsset {
	CurrentlyUsed = 1,
	DefaultReplacement = 2,
	CustomReplacement = -1
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

//	QString sNonBaseUvWarning = tr("\
//The character is already using the \"Combined Head and Body\" UV Set for Roblox export.  Modesty layer will NOT be re-applied.  \
//Please make sure that your character already has a Roblox compliant modesty layer applied.\
//\n\n\
//The character must be using the \"Base Multi UDIM\" UV Set in order for the modesty layer to be applied.  \
//If you want to re-apply the modesty layer, then please first apply another material by double-clicking a compatible \
//character material from the \"Smart Content\" or \"Content Library\" pane.\
//");
	QString m_sModestyOverlayDisabledNotice = tr("\
<p>Your character is already configured for exporting to Roblox, with the head and body materials combined together. \
Because of this, the modesty layer cannot be re-applied automatically.  If your character already has a modesty layer \
that meets Roblox guidelines, you can proceed with the transfer.</p>\
<p>To add or re-add a modesty layer, your character needs to be reset to the original Daz material settings:</p>\
<ol>\
<li>Select the root node of the character in the Scene pane.</li>\
<li>Then double-click a compatible character Material from Materials section of the \"Smart Content\" or \"Content Library\" pane.</li>\
<li>Once the Material is applied, the modesty layer options will become available again.</li>\
</ol>\
");
	QString m_sModestyOverlayHelp = tr("\
Select a modesty layer to be overlaid on top of the unclothed Genesis skin texture.\
");

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
	Q_INVOKABLE void enableModestyOptions(bool bEnable);

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

	QCheckBox* m_wHiddenSurfaceRemovalCheckbox;
	QCheckBox* m_wRemoveScalpMaterialCheckbox;

	virtual void refreshAsset() override;

#ifdef UNITTEST_DZBRIDGE
	friend class UnitTest_DzRobloxDialog;
#endif
};
