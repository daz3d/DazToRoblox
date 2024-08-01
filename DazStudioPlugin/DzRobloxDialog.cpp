#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>
#include <QtGui/QToolTip>
#include <QtGui/QWhatsThis>
#include <QtGui/qlineedit.h>
#include <QtGui/qboxlayout.h>
#include <QtGui/qfiledialog.h>
#include <QtCore/qsettings.h>
#include <QtGui/qformlayout.h>
#include <QtGui/qcombobox.h>
#include <QtGui/qdesktopservices.h>
#include <QtGui/qcheckbox.h>
#include <QtGui/qlistwidget.h>
#include <QtGui/qgroupbox.h>
#include <QtGui/qvalidator.h>
#include <qtextbrowser.h>

#include "dzapp.h"
#include "dzscene.h"
#include "dzstyle.h"
#include "dzmainwindow.h"
#include "dzactionmgr.h"
#include "dzaction.h"
#include "dzskeleton.h"
#include "qstandarditemmodel.h"

#include "DzRobloxDialog.h"
#include "DzBridgeMorphSelectionDialog.h"
#include "DzBridgeSubdivisionDialog.h"

#include "version.h"

/*****************************
Local definitions
*****************************/
#define DAZ_BRIDGE_PLUGIN_NAME "Daz To Roblox Studio Exporter"

#include "dzbridge.h"

QValidator::State DzFileValidator::validate(QString& input, int& pos) const {
	QFileInfo fi(input);
	if (fi.exists() == false) {
		dzApp->log("DzBridge: DzFileValidator: DEBUG: file does not exist: " + input);
		return QValidator::Intermediate;
	}

	return QValidator::Acceptable;
};

DzRobloxDialog::DzRobloxDialog(QWidget* parent) :
	 DzBridgeDialog(parent, DAZ_BRIDGE_PLUGIN_NAME)
{
	 intermediateFolderEdit = nullptr;
	 intermediateFolderButton = nullptr;

	 settings = new QSettings("Daz 3D", "DazToRoblox");

	 // Declarations
	 int margin = style()->pixelMetric(DZ_PM_GeneralMargin);
	 int wgtHeight = style()->pixelMetric(DZ_PM_ButtonHeight);
	 int btnMinWidth = style()->pixelMetric(DZ_PM_ButtonMinWidth);

	 // Set the dialog title
	 setWindowTitle(tr("Daz To Roblox Studio Exporter %1 v%2.%3").arg(PLUGIN_MAJOR).arg(PLUGIN_MINOR).arg(PLUGIN_REV));
	 QString sDazAppDir = dzApp->getHomePath().replace("\\", "/");
	 QString sPdfPath = sDazAppDir + "/docs/Plugins" + "/Daz to Roblox/Daz to Roblox.pdf";
	 QString sSetupModeString = tr("\
<div style=\"background-color:#282f41;\" align=center>\
<img src=\":/DazBridgeRoblox/banner.png\" width=\"370\" height=\"95\" align=\"center\" hspace=\"0\" vspace=\"0\">\
<table width=100% cellpadding=8 cellspacing=2 style=\"vertical-align:middle; font-size:x-large; font-weight:bold; background-color:#FFAA00;foreground-color:#FFFFFF\" align=center>\
  <tr>\
    <td width=33% style=\"text-align:center; background-color:#282f41;\"><div align=center><a href=\"https://github.com/daz3d/DazToRoblox?tab=readme-ov-file#readme\">Read Me</a></td>\
    <td width=33% style=\"text-align:center; background-color:#282f41;\"><div align=center><a href=\"https://github.com/daz3d/DazToRoblox\">GitHub Repo</a></td>\
    <td width=33% style=\"text-align:center; background-color:#282f41;\"><div align=center><a href=\"https://create.roblox.com/docs/art/characters/specifications\">Roblox Guidelines</a></td>\
  </tr>\
</table>\
</div>\
");

	 m_WelcomeLabel->setText(sSetupModeString);

	 // populate Roblox Asset Types
	 assetTypeCombo->clear();
//	 assetTypeCombo->addItem("Please select an asset type...", "__");
	 assetTypeCombo->addItem("Complete Character and Outfit", "ALL");
	 assetTypeCombo->addItem("Roblox R15 Avatar", "R15");
	 assetTypeCombo->addItem("Roblox S1 for Avatar Auto Setup", "S1");
	 assetTypeCombo->addItem("Layered Clothing or Hair", "layered");
	 assetTypeCombo->addItem("Rigid Accessories", "rigid");
	 assetTypeCombo->setCurrentIndex(0);

	 // Remove options
	 morphsButton->hide();
	 morphsEnabledCheckBox->hide();
	 morphSettingsGroupBox->hide();
	 m_wMorphsRowLabelWidget->hide();
	 subdivisionButton->hide();
	 subdivisionEnabledCheckBox->hide();
	 m_wSubDRowLabelWidget->hide();
	 fbxVersionCombo->hide();
	 m_wFbxVersionRowLabelWidget->hide();
	 showFbxDialogCheckBox->hide();
	 m_wShowFbxRowLabelWidget->hide();
	 enableNormalMapGenerationCheckBox->hide();
	 m_wNormalMapsRowLabelWidget->hide();
	 exportMaterialPropertyCSVCheckBox->hide();
	 m_wExportCsvRowLabelWidget->hide();
	 m_enableExperimentalOptionsCheckBox->hide();
	 m_wEnableExperimentalRowLabelWidget->hide();

	 // Figure Export Options
	 m_wModestyOverlayCombo = new QComboBox();
	 m_wModestyOverlayCombo->addItem("Strapless bra and bikini", eModestyOverlay::StraplessBra_Bikini);
	 m_wModestyOverlayCombo->addItem("Sports bra and shorts", eModestyOverlay::SportsBra_Shorts);
	 m_wModestyOverlayCombo->addItem("Tank top and shorts", eModestyOverlay::TankTop_Shorts);
	 m_wModestyOverlayCombo->addItem("Custom modesty overlay...", eModestyOverlay::CustomModestyOverlay);
	 m_wModestyOverlayCombo->setCurrentIndex(0);
	 connect(m_wModestyOverlayCombo, SIGNAL(activated(int)), this, SLOT(HandleCustomModestyOverlayActivated(int)));
	 m_wModestyOverlayRowLabel = new QLabel(tr("Modesty Overlay"));
	 mainLayout->addRow(m_wModestyOverlayRowLabel, m_wModestyOverlayCombo);

	 // Add Project Folder
	 QHBoxLayout* robloxOutputFolderLayout = new QHBoxLayout();
	 m_wRobloxOutputFolderEdit = new QLineEdit(this);
	 m_wRobloxOutputFolderEdit->setValidator(&m_dzValidatorFileExists);
	 m_wRobloxOutputFolderButton = new QPushButton("...", this);
	 robloxOutputFolderLayout->addWidget(m_wRobloxOutputFolderEdit);
	 robloxOutputFolderLayout->addWidget(m_wRobloxOutputFolderButton);
	 connect(m_wRobloxOutputFolderButton, SIGNAL(released()), this, SLOT(HandleSelectRobloxOutputFolderButton()));
	 connect(m_wRobloxOutputFolderEdit, SIGNAL(textChanged(const QString&)), this, SLOT(HandleTextChanged(const QString&)));

	 // Add No Breast Option
	 m_wBreastsGoneCheckbox = new QCheckBox("Breasts Gone Morph");
	 m_wBreastsGoneCheckbox->setToolTip("Optional Remove Breasts. REQUIRES Genesis 9 Body Shapes Add-On");
	 m_wBreastsGoneCheckbox->setChecked(true);

	 // Eyebrow replacer Option
	 QHBoxLayout* wReplacementPartsLayout = new QHBoxLayout();
	 m_wEyebrowReplacement = new QComboBox();
	 m_wEyebrowReplacement->addItem(tr("Replace Eyebrows"), "replace_eyebrows");
	 m_wEyebrowReplacement->addItem(tr("Use Current Eyebrows"), "use_current_eyebrows");
	 m_wEyebrowReplacement->addItem(tr("Use Custom Replacement..."), "use_custom_eyebrows");
	 m_wEyebrowReplacement->setToolTip(tr("Replace eyebrows with something else."));
	 m_wEyebrowReplacement->setCurrentIndex(0);
	 m_wEyelashReplacement = new QComboBox();
	 m_wEyelashReplacement->addItem(tr("Replace Eyelashes"), "replace_eyelashes");
	 m_wEyelashReplacement->addItem(tr("Use Current Eyelashes"), "use_current_eyelashes");
	 m_wEyelashReplacement->addItem(tr("Use Custom Replacement..."), "use_custom_eyelashes");
	 m_wEyelashReplacement->setCurrentIndex(0);
	 m_wEyelashReplacement->setToolTip(tr("Replace eyelashes with something else."));
	 wReplacementPartsLayout->addWidget(m_wEyebrowReplacement);
	 wReplacementPartsLayout->addWidget(m_wEyelashReplacement);
	 QLabel* wReplacementPartsRowLabel = new QLabel(tr("Replacement Assets"));

	 // Accessory Export Options
	 m_wBakeSingleOutfitCheckbox = new QCheckBox(tr("Bake Single Outfit"));
	 m_wBakeSingleOutfitCheckbox->setToolTip(tr("Bake all items into a single layered clothing outfit"));
	 QLabel* wLayeredClothingRowLabel = new QLabel(tr("Layered Clothing"));

	 // Add GUI to layout
	 mainLayout->insertRow(1, "Roblox Output Folder", robloxOutputFolderLayout);
	 m_wGodotProjectFolderRowLabelWidget = mainLayout->itemAt(1, QFormLayout::LabelRole)->widget();
	 showRobloxOptions(true);
	 this->showLodRow(false);
	 QLabel* wContentModerationRowLabel = new QLabel(tr("Content Moderation"));
	 mainLayout->addRow(wContentModerationRowLabel, m_wBreastsGoneCheckbox);
	 mainLayout->addRow(wReplacementPartsRowLabel, wReplacementPartsLayout);
	 mainLayout->addRow(wLayeredClothingRowLabel, m_wBakeSingleOutfitCheckbox);

	 // Select Blender Executable Path GUI
	 QHBoxLayout* blenderExecutablePathLayout = new QHBoxLayout();
	 m_wBlenderExecutablePathEdit = new QLineEdit(this);
	 m_wBlenderExecutablePathEdit->setValidator(&m_dzValidatorFileExists);
	 m_wBlenderExecutablePathButton = new QPushButton("...", this);
	 blenderExecutablePathLayout->addWidget(m_wBlenderExecutablePathEdit);
	 blenderExecutablePathLayout->addWidget(m_wBlenderExecutablePathButton);
	 connect(m_wBlenderExecutablePathButton, SIGNAL(released()), this, SLOT(HandleSelectBlenderExecutablePathButton()));
	 connect(m_wBlenderExecutablePathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(HandleTextChanged(const QString&)));


	 // Intermediate Folder
	 QHBoxLayout* intermediateFolderLayout = new QHBoxLayout();
	 intermediateFolderEdit = new QLineEdit(this);
	 intermediateFolderEdit->setValidator(&m_dzValidatorFileExists);
	 intermediateFolderButton = new QPushButton("...", this);
	 intermediateFolderLayout->addWidget(intermediateFolderEdit);
	 intermediateFolderLayout->addWidget(intermediateFolderButton);
	 connect(intermediateFolderButton, SIGNAL(released()), this, SLOT(HandleSelectIntermediateFolderButton()));
	 connect(intermediateFolderEdit, SIGNAL(textChanged(const QString&)), this, SLOT(HandleTextChanged(const QString&)));

	 //  Add Intermediate Folder to Advanced Settings container as a new row with specific headers
	 QFormLayout* advancedLayout = qobject_cast<QFormLayout*>(advancedWidget->layout());
	 if (advancedLayout)
	 {
		 advancedLayout->insertRow(1, "Blender Executable", blenderExecutablePathLayout);

		 advancedLayout->addRow("Intermediate Folder", intermediateFolderLayout);
		 // reposition the Open Intermediate Folder button so it aligns with the center section
		 advancedLayout->removeWidget(m_OpenIntermediateFolderButton);
		 advancedLayout->addRow("", m_OpenIntermediateFolderButton);
	 }

	 // Disable Experimental Options Checkbox
	 m_enableExperimentalOptionsCheckBox->setEnabled(false);
	 m_enableExperimentalOptionsCheckBox->setToolTip(tr("No experimental options in this version."));
	 m_enableExperimentalOptionsCheckBox->setWhatsThis(tr("No experimental options in this version."));

	 QString sVersionString = tr("Roblox Studio Exporter %1 v%2.%3.%4").arg(PLUGIN_MAJOR).arg(PLUGIN_MINOR).arg(PLUGIN_REV).arg(PLUGIN_BUILD);
	 setBridgeVersionStringAndLabel(sVersionString);

	 //// Configure Target Plugin Installer
	 //renameTargetPluginInstaller("Godot Plugin Installer");
	 //m_TargetSoftwareVersionCombo->clear();
	 showTargetPluginInstaller(false);

	 // Make the dialog fit its contents, with a minimum width, and lock it down
	 resize(QSize(500, 0).expandedTo(minimumSizeHint()));
	 setFixedWidth(width());
	 setFixedHeight(height());

	 update();

	 // Help
	 assetNameEdit->setWhatsThis("This is the name the asset will use in Roblox Studio.");
	 assetTypeCombo->setWhatsThis("Skeletal Mesh for something with moving parts, like a character\nStatic Mesh for things like props\nAnimation for a character animation.");
	 intermediateFolderEdit->setWhatsThis("Roblox Studio Exporter will collect the assets in a subfolder under this folder.");
	 intermediateFolderButton->setWhatsThis("Roblox Studio Exporter will collect the assets in a subfolder under this folder.");
	 //m_wTargetPluginInstaller->setWhatsThis("You can install the Godot Plugin by selecting the desired Godot version and then clicking Install.");

	 // Set Defaults
	 resetToDefaults();

	 // Load Settings
	 loadSavedSettings();

	 disableAcceptUntilAllRequirementsValid();

}

bool DzRobloxDialog::loadSavedSettings()
{
	DzBridgeDialog::loadSavedSettings();

	if (!settings->value("IntermediatePath").isNull())
	{
		QString directoryName = settings->value("IntermediatePath").toString();
		intermediateFolderEdit->setText(directoryName);
	}
	else
	{
		QString DefaultPath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DazToRoblox";
		intermediateFolderEdit->setText(DefaultPath);
	}
	if (!settings->value("RobloxOutputPath").isNull())
	{
		m_wRobloxOutputFolderEdit->setText(settings->value("RobloxOutputPath").toString());
	}
	if (!settings->value("BlenderExecutablePath").isNull())
	{
		m_wBlenderExecutablePathEdit->setText(settings->value("BlenderExecutablePath").toString());
	}

	return true;
}

void DzRobloxDialog::saveSettings()
{
	if (settings == nullptr || m_bDontSaveSettings) return;

	DzBridgeDialog::saveSettings();

	///// Preserve manuallly entered paths ///////// 
	// Intermediate Path
	settings->setValue("IntermediatePath", intermediateFolderEdit->text());
	// Blender Executable Path
	settings->setValue("BlenderExecutablePath", m_wBlenderExecutablePathEdit->text());
	// Godot Project Path
	settings->setValue("RobloxOutputPath", m_wRobloxOutputFolderEdit->text());

}

void DzRobloxDialog::resetToDefaults()
{
	m_bDontSaveSettings = true;
	DzBridgeDialog::resetToDefaults();

	QString DefaultPath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DazToRoblox";
	intermediateFolderEdit->setText(DefaultPath);

	DzNode* Selection = dzScene->getPrimarySelection();
	if (dzScene->getFilename().length() > 0)
	{
		QFileInfo fileInfo = QFileInfo(dzScene->getFilename());
		assetNameEdit->setText(fileInfo.baseName().remove(QRegExp("[^A-Za-z0-9_]")));
	}
	else if (dzScene->getPrimarySelection())
	{
		assetNameEdit->setText(Selection->getLabel().remove(QRegExp("[^A-Za-z0-9_]")));
	}

	if (qobject_cast<DzSkeleton*>(Selection))
	{
		assetTypeCombo->setCurrentIndex(0);
	}
	else
	{
		assetTypeCombo->setCurrentIndex(1);
	}
	m_bDontSaveSettings = false;
}

void DzRobloxDialog::HandleSelectIntermediateFolderButton()
{
	 // DB (2021-05-15): prepopulate with existing folder string
	 QString directoryName = "/home";
	 if (settings != nullptr && settings->value("IntermediatePath").isNull() != true)
	 {
		 directoryName = settings->value("IntermediatePath").toString();
	 }
	 directoryName = QFileDialog::getExistingDirectory(this, tr("Choose Directory"),
		  directoryName,
		  QFileDialog::ShowDirsOnly
		  | QFileDialog::DontResolveSymlinks);

	 if (directoryName != NULL)
	 {
		 intermediateFolderEdit->setText(directoryName);
		 if (settings != nullptr)
		 {
			 settings->setValue("IntermediatePath", directoryName);
		 }
	 }
}

#include <QProcessEnvironment>


void DzRobloxDialog::HandleDisabledChooseSubdivisionsButton()
{

}

void DzRobloxDialog::HandleOpenIntermediateFolderButton(QString sFolderPath)
{
	QString sIntermediateFolder = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DazToRoblox";
	if (intermediateFolderEdit != nullptr)
	{
		sIntermediateFolder = intermediateFolderEdit->text();
	}
	DzBridgeDialog::HandleOpenIntermediateFolderButton(sIntermediateFolder);
}

void DzRobloxDialog::refreshAsset()
{
	DzBridgeDialog::refreshAsset();
}

void DzRobloxDialog::HandleSelectRobloxOutputFolderButton()
{
	// DB (2021-05-15): prepopulate with existing folder string
	QString directoryName = "/home";
	if (settings != nullptr && settings->value("RobloxOutputPath").isNull() != true)
	{
		directoryName = settings->value("RobloxOutputPath").toString();
	}
	directoryName = QFileDialog::getExistingDirectory(this, tr("Select Roblox Output Folder"),
		directoryName,
		QFileDialog::ShowDirsOnly
		| QFileDialog::DontResolveSymlinks);

	if (directoryName != "")
	{
		m_wRobloxOutputFolderEdit->setText(directoryName);
		if (settings != nullptr)
		{
			settings->setValue("RobloxOutputPath", directoryName);
		}
	}
}

void DzRobloxDialog::showRobloxOptions(bool bVisible)
{
	m_wRobloxOutputFolderEdit->setVisible(bVisible);
	m_wRobloxOutputFolderButton->setVisible(bVisible);
	m_wGodotProjectFolderRowLabelWidget->setVisible(bVisible);
}

void DzRobloxDialog::HandleSelectBlenderExecutablePathButton()
{
	// DB 2023-10-13: prepopulate with existing folder string
	QString directoryName = "";
	if (settings != nullptr && settings->value("BlenderExecutablePath").isNull() != true)
	{
		directoryName = QFileInfo(settings->value("BlenderExecutablePath").toString()).dir().dirName();
	}
#ifdef WIN32
    QString sExeFilter = tr("Executable Files (*.exe)");
#elif defined(__APPLE__)
    QString sExeFilter = tr("Application Bundle (*.app)");
#endif
    QString fileName = QFileDialog::getOpenFileName(this,
		tr("Select Blender Executable"),
		directoryName,
		sExeFilter,
		&sExeFilter,
		QFileDialog::ReadOnly |
		QFileDialog::DontResolveSymlinks);

#if defined(__APPLE__)
    if (fileName != "")
    {
        fileName = fileName + "/Contents/MacOS/Blender";
    }
#endif
            
	if (fileName != "")
	{
		m_wBlenderExecutablePathEdit->setText(fileName);
		if (settings != nullptr)
		{
			settings->setValue("BlenderExecutablePath", fileName);
		}
	}
}

void DzRobloxDialog::HandleTextChanged(const QString& text)
{
	QObject* senderWidget = sender();

	if (senderWidget == m_wBlenderExecutablePathEdit) {
		// check if blender exe is valid
		printf("DEBUG: check stuff here...");
//		disableAcceptUntilBlenderValid(text);
		disableAcceptUntilAllRequirementsValid();
	}
	dzApp->log("DzRoblox: DEBUG: HandleTextChanged: text = " + text);
}

void DzRobloxDialog::HandleAssetTypeComboChange(int state)
{
	disableAcceptUntilAllRequirementsValid();
}

bool DzRobloxDialog::isBlenderTextBoxValid(const QString& arg_text)
{
	QString temp_text(arg_text);

	if (temp_text == "") {
		// check widget text
		temp_text = m_wBlenderExecutablePathEdit->text();
	}

	// validate blender executable
	QFileInfo fi(temp_text);
	if (fi.exists() == false) {
		dzApp->log("DzBridge: disableAcceptUntilBlenderValid: DEBUG: file does not exist: " + temp_text);
		return false;
	}

	return true;
}

bool DzRobloxDialog::isAssetTypeComboBoxValid()
{
	if (assetTypeCombo->itemData(assetTypeCombo->currentIndex()).toString() == "__")
	{
		return false;
	}

	return true;
}

bool DzRobloxDialog::disableAcceptUntilAllRequirementsValid()
{
	if (dzScene->getPrimarySelection() == NULL)
	{
		this->setAcceptButtonEnabled(false);
		return true;
	}
	// otherwise, enable accept button so we can show feedback dialog to help user
	this->setAcceptButtonEnabled(true);

	if (!isBlenderTextBoxValid() || !isAssetTypeComboBoxValid())
	{
//		this->setAcceptButtonEnabled(false);
		this->setAcceptButtonText("Unable to Proceed");
		return false;
	}
	this->setAcceptButtonText("Accept");
//	this->setAcceptButtonEnabled(true);
	return true;

}


bool DzRobloxDialog::HandleAcceptButtonValidationFeedback() {

	// Check if Roblox Output Folder and Blender Executable are valid, if not issue Error and fail gracefully
	bool bSettingsValid = false;

	if (m_wRobloxOutputFolderEdit->text() != "" && QDir(m_wRobloxOutputFolderEdit->text()).exists() &&
		m_wBlenderExecutablePathEdit->text() != "" && QFileInfo(m_wBlenderExecutablePathEdit->text()).exists() &&
		assetTypeCombo->itemData(assetTypeCombo->currentIndex()).toString() != "__")
	{
		bSettingsValid = true;

		return bSettingsValid;

	}

	if (m_wRobloxOutputFolderEdit->text() == "" || QDir(m_wRobloxOutputFolderEdit->text()).exists() == false)
	{
		QMessageBox::warning(0, tr("Roblox Output Folder"), tr("Roblox Output Folder must be set."), QMessageBox::Ok);
	}
	else if (m_wBlenderExecutablePathEdit->text() == "" || QFileInfo(m_wBlenderExecutablePathEdit->text()).exists() == false)
	{
		QMessageBox::warning(0, tr("Blender Executable Path"), tr("Blender Executable Path must be set."), QMessageBox::Ok);
		// Enable Advanced Settings
		if (advancedSettingsGroupBox->isChecked() == false)
		{
			advancedSettingsGroupBox->setChecked(true);

			foreach(QObject * child, advancedSettingsGroupBox->children())
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
	else if (assetTypeCombo->itemData(assetTypeCombo->currentIndex()).toString() == "__")
	{
		QMessageBox::warning(0, tr("Select Asset Type"), tr("Please select an asset type from the dropdown menu."), QMessageBox::Ok);
	}

	return bSettingsValid;

}

void DzRobloxDialog::accept()
{
	bool bResult = HandleAcceptButtonValidationFeedback();

	if (bResult == true)
	{
		bResult = showDisclaimer();
	}

	if (bResult == true) 
	{
		DzBridgeDialog::accept();
	}

}

bool DzRobloxDialog::showDisclaimer()
{
	QString content = tr("\
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
Daz 3D will not be liable for any damages arising from the use of this software.</p></div>");

	QTextBrowser* wContent = new QTextBrowser();
	wContent->setText(content);
	wContent->setOpenExternalLinks(true);

	QString sWindowTitle = tr("Daz To Roblox Studio EULA");
	DzBasicDialog* wDialog = new DzBasicDialog(NULL, sWindowTitle);
	wDialog->setAcceptButtonText(tr("Accept EULA"));
	wDialog->setCancelButtonText(tr("Decline"));
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

void DzRobloxDialog::HandleCustomModestyOverlayActivated(int index)
{
	// get itemData
	QVariant vItemData = m_wModestyOverlayCombo->itemData(index);

	if (vItemData.type() != QVariant::String && vItemData.toInt() == eModestyOverlay::CustomModestyOverlay) {
		// roblox dev kit folder
		QString sStartingFolder = "";
		QString sFileFilter = tr("Images (*.png)");
		// pop file selection window
		QString sFilePath = QFileDialog::getOpenFileName(this,
			tr("Select Custom Modesty Overlay Image"),
			sStartingFolder, sFileFilter);

		if (QFileInfo(sFilePath).exists()) {
			// check if filepath itemData is already in dropdown
			if (int nMatchingIndex=m_wModestyOverlayCombo->findData(sFilePath) != -1) {
				m_wModestyOverlayCombo->setCurrentIndex(nMatchingIndex);
			}
			else {
				QString sShortFilename = ".../" + QFileInfo(sFilePath).fileName();
				m_wModestyOverlayCombo->addItem(sShortFilename, QVariant(sFilePath));
				m_wModestyOverlayCombo->setCurrentIndex(m_wModestyOverlayCombo->count()-1);
			}
		}
	}
}

#include "moc_DzRobloxDialog.cpp"
