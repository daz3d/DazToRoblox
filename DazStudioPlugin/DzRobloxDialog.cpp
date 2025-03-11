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
#include "DzRobloxAction.h"
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
	this->setObjectName("DzBridge_Roblox_Dialog");

	 intermediateFolderEdit = nullptr;
	 intermediateFolderButton = nullptr;

	 settings = new QSettings("Daz 3D", "DazToRoblox");

	 // Declarations
	 int margin = style()->pixelMetric(DZ_PM_GeneralMargin);
	 int wgtHeight = style()->pixelMetric(DZ_PM_ButtonHeight);
	 int btnMinWidth = style()->pixelMetric(DZ_PM_ButtonMinWidth);

	 // Original window title, for potential interim hotfix releases before GUI refresh
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
	 m_wConvertBumpToNormalCheckBox->hide();
	 m_wNormalMapsRowLabelWidget->hide();
	 exportMaterialPropertyCSVCheckBox->hide();
	 m_wExportCsvRowLabelWidget->hide();
	 m_enableExperimentalOptionsCheckBox->hide();
	 m_wEnableExperimentalRowLabelWidget->hide();

	 // set Roblox specific defaults
	 m_wResizeTexturesGroupBox->setDisabled(false);
	 m_wResizeTexturesGroupBox->setChecked(true);
	 //
	 m_wMaxTextureFileSizeCombo->addItem("Roblox Studio Maximum (19 MB)", ROBLOX_MAX_TEXTURE_SIZE);
	 int roblox_default_index = m_wMaxTextureFileSizeCombo->findData(ROBLOX_MAX_TEXTURE_SIZE);
	 if (roblox_default_index != -1) m_wMaxTextureFileSizeCombo->setCurrentIndex(roblox_default_index);
	 //
	 roblox_default_index = m_wMaxTextureResolutionCombo->findData(1024);
	 if (roblox_default_index != -1) m_wMaxTextureResolutionCombo->setCurrentIndex(roblox_default_index);
	 //
	 roblox_default_index = m_wExportTextureFileFormatCombo->findData("png+jpg");
	 if (roblox_default_index != -1) m_wExportTextureFileFormatCombo->setCurrentIndex(roblox_default_index);
	 //
	 roblox_default_index = m_wBakeInstancesComboBox->findData("always");
	 if (roblox_default_index != -1) m_wBakeInstancesComboBox->setCurrentIndex(roblox_default_index);
	 roblox_default_index = m_wBakeCustomPivotsComboBox->findData("always");
	 if (roblox_default_index != -1) m_wBakeCustomPivotsComboBox->setCurrentIndex(roblox_default_index);
	 roblox_default_index = m_wBakeRigidFollowNodesComboBox->findData("always");
	 if (roblox_default_index != -1) m_wBakeRigidFollowNodesComboBox->setCurrentIndex(roblox_default_index);
	 m_wObjectBakingGroupBox->setChecked(false);
	 m_wObjectBakingGroupBox->setDisabled(true);
	 m_wObjectBakingGroupBox->setVisible(false);

	 // Modesty Overlay Options
	 m_wModestyOverlayCombo = new QComboBox();
	 m_wModestyOverlayCombo->addItem(tr("Strapless bra and bikini"), eModestyOverlay::StraplessBra_Bikini);
	 m_wModestyOverlayCombo->addItem(tr("Sports bra and shorts"), eModestyOverlay::SportsBra_Shorts);
	 m_wModestyOverlayCombo->addItem(tr("Tank top and shorts"), eModestyOverlay::TankTop_Shorts);
	 m_wModestyOverlayCombo->addItem(tr("Use current textures"), eModestyOverlay::UseCurrentTextures);
	 m_wModestyOverlayCombo->addItem(tr("Custom modesty overlay..."), eModestyOverlay::CustomModestyOverlay);
	 m_wModestyOverlayCombo->setCurrentIndex(0);
	 connect(m_wModestyOverlayCombo, SIGNAL(activated(int)), this, SLOT(HandleCustomModestyOverlayActivated(int)));
	 m_wModestyOverlayRowLabel = new QLabel(tr("Modesty Overlay"));
	 mainLayout->addRow(m_wModestyOverlayRowLabel, m_wModestyOverlayCombo);
	 m_aRowLabels.append(m_wModestyOverlayRowLabel);

	 // Add Project Folder
	 QHBoxLayout* robloxOutputFolderLayout = new QHBoxLayout();
	 robloxOutputFolderLayout->setSpacing(0);
	 m_wRobloxOutputFolderEdit = new QLineEdit(this);
	 m_wRobloxOutputFolderEdit->setValidator(&m_dzValidatorFileExists);
	 //m_wRobloxOutputFolderButton = new QPushButton("...", this);
	 m_wRobloxOutputFolderButton = new DzBridgeBrowseButton(this);
	 robloxOutputFolderLayout->addWidget(m_wRobloxOutputFolderEdit);
	 robloxOutputFolderLayout->addWidget(m_wRobloxOutputFolderButton);
	 connect(m_wRobloxOutputFolderButton, SIGNAL(released()), this, SLOT(HandleSelectRobloxOutputFolderButton()));
	 connect(m_wRobloxOutputFolderEdit, SIGNAL(textChanged(const QString&)), this, SLOT(HandleTextChanged(const QString&)));

	 // Add No Breast Option
	 m_wBreastsGoneCheckbox = new QCheckBox("Breasts Gone Morph");
	 QString sBreastsGone = tr("Optional Remove Breasts. REQUIRES Genesis 9 Body Shapes Add-On");
	 m_wBreastsGoneCheckbox->setToolTip(sBreastsGone);
	 m_wBreastsGoneCheckbox->setWhatsThis(sBreastsGone);
	 m_wBreastsGoneCheckbox->setChecked(true);

	 // TODO: complepte custom eyelash/eyebrow pathway
	 // Eyebrow replacer Option
	 QHBoxLayout* wReplacementPartsLayout = new QHBoxLayout();
	 m_wEyebrowReplacement = new QComboBox();
	 m_wEyebrowReplacement->addItem(tr("Replace Eyebrows"), eReplacementAsset::DefaultReplacement);
	 m_wEyebrowReplacement->addItem(tr("Use Current Eyebrows"), eReplacementAsset::CurrentlyUsed);
//	 m_wEyebrowReplacement->addItem(tr("Use Custom Replacement..."), eReplacementAsset::CustomReplacement);
	 m_wEyebrowReplacement->setToolTip(tr("Replace eyebrows with something else."));
	 m_wEyebrowReplacement->setCurrentIndex(0);
	 m_wEyelashReplacement = new QComboBox();
	 m_wEyelashReplacement->addItem(tr("Replace Eyelashes"), eReplacementAsset::DefaultReplacement);
	 m_wEyelashReplacement->addItem(tr("Use Current Eyelashes"), eReplacementAsset::CurrentlyUsed);
//	 m_wEyelashReplacement->addItem(tr("Use Custom Replacement..."), eReplacementAsset::CustomReplacement);
	 m_wEyelashReplacement->setCurrentIndex(0);
	 m_wEyelashReplacement->setToolTip(tr("Replace eyelashes with something else."));
	 wReplacementPartsLayout->addWidget(m_wEyebrowReplacement);
	 wReplacementPartsLayout->addWidget(m_wEyelashReplacement);
	 m_wReplacementPartsRowLabel = new QLabel(tr("Replacement Assets"));
	 m_aRowLabels.append(m_wReplacementPartsRowLabel);

	 // Accessory Export Options
	 QVBoxLayout* wClothingOptionsLayout = new QVBoxLayout();
	 m_wBakeSingleOutfitCheckbox = new QCheckBox(tr("Merge all clothing together"));
	 QString sBakeSingleOutfit = tr("Merge all clothing and hair items togother so that they use a single Roblox clothing slot");
	 m_wBakeSingleOutfitCheckbox->setToolTip(sBakeSingleOutfit);
	 m_wBakeSingleOutfitCheckbox->setWhatsThis(sBakeSingleOutfit);
	 m_wBakeSingleOutfitCheckbox->setChecked(false);
	 m_wHiddenSurfaceRemovalCheckbox = new QCheckBox(tr("Remove Hidden Geometry"));
	 QString sHiddenSurfaceRemoval = tr("Remove unnecessary triangles which are hidden underneath other triangles and visible (Experimental)");
	 m_wHiddenSurfaceRemovalCheckbox->setToolTip(sHiddenSurfaceRemoval);
	 m_wHiddenSurfaceRemovalCheckbox->setWhatsThis(sHiddenSurfaceRemoval);
	 m_wHiddenSurfaceRemovalCheckbox->setChecked(false);
	 QString sRemoveScalp = tr("Remove scalp geometry from hair assets. May fix opaque scalp issues.");
	 m_wRemoveScalpMaterialCheckbox = new QCheckBox(tr("Remove Hair Cap"));
	 m_wRemoveScalpMaterialCheckbox->setToolTip(sRemoveScalp);
	 m_wRemoveScalpMaterialCheckbox->setWhatsThis(sRemoveScalp);
	 m_wRemoveScalpMaterialCheckbox->setChecked(true);
	 wClothingOptionsLayout->addWidget(m_wBakeSingleOutfitCheckbox);
	 wClothingOptionsLayout->addWidget(m_wHiddenSurfaceRemovalCheckbox);
	 wClothingOptionsLayout->addWidget(m_wRemoveScalpMaterialCheckbox);
	 m_wLayeredClothingRowLabel = new QLabel(tr("Accessories"));
	 m_aRowLabels.append(m_wLayeredClothingRowLabel);

	 // Add GUI to layout
	 m_wRobloxOutputFolderRowLabel = new QLabel(tr("Roblox Output Folder"));
	 m_aRowLabels.append(m_wRobloxOutputFolderRowLabel);
	 mainLayout->insertRow(1, m_wRobloxOutputFolderRowLabel, robloxOutputFolderLayout);
	 showRobloxOptions(true);
	 this->showLodRow(false);
	 m_wContentModerationRowLabel = new QLabel(tr("Content Moderation"));
	 m_aRowLabels.append(m_wContentModerationRowLabel);
	 mainLayout->addRow(m_wContentModerationRowLabel, m_wBreastsGoneCheckbox);
	 mainLayout->addRow(m_wReplacementPartsRowLabel, wReplacementPartsLayout);
	 mainLayout->addRow(m_wLayeredClothingRowLabel, wClothingOptionsLayout);

	 // Select Blender Executable Path GUI
	 m_wBlenderExecutablePathRowLabel = new QLabel("Blender Executable");
	 QHBoxLayout* blenderExecutablePathLayout = new QHBoxLayout();
	 blenderExecutablePathLayout->setSpacing(0);
	 m_wBlenderExecutablePathEdit = new QLineEdit(this);
	 m_wBlenderExecutablePathEdit->setValidator(&m_dzValidatorFileExists);
	 //m_wBlenderExecutablePathButton = new QPushButton("...", this);
	 m_wBlenderExecutablePathButton = new DzBridgeBrowseButton(this);
	 blenderExecutablePathLayout->addWidget(m_wBlenderExecutablePathEdit);
	 blenderExecutablePathLayout->addWidget(m_wBlenderExecutablePathButton);
	 connect(m_wBlenderExecutablePathButton, SIGNAL(released()), this, SLOT(HandleSelectBlenderExecutablePathButton()));
	 connect(m_wBlenderExecutablePathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(HandleTextChanged(const QString&)));

	 // Intermediate Folder
	 m_wIntermediateFolderRowLabel = new QLabel("Intermediate Folder");
	 QHBoxLayout* intermediateFolderLayout = new QHBoxLayout();
	 intermediateFolderLayout->setSpacing(0);
	 intermediateFolderEdit = new QLineEdit(this);
	 intermediateFolderEdit->setValidator(&m_dzValidatorFileExists);
	 //intermediateFolderButton = new QPushButton("...", this);
	 intermediateFolderButton = new DzBridgeBrowseButton(this);
	 intermediateFolderLayout->addWidget(intermediateFolderEdit);
	 intermediateFolderLayout->addWidget(intermediateFolderButton);
	 connect(intermediateFolderButton, SIGNAL(released()), this, SLOT(HandleSelectIntermediateFolderButton()));
	 connect(intermediateFolderEdit, SIGNAL(textChanged(const QString&)), this, SLOT(HandleTextChanged(const QString&)));

	 // Force GPU Pathway
	 m_wForceGpuRowLabel = new QLabel(tr("GPU Acceleration"));
	 QHBoxLayout* useGpuLayout = new QHBoxLayout();
	 useGpuLayout->setSpacing(0);
	 m_wForceGpuCheckbox = new QCheckBox(tr("Force GPU pathway"));
	 m_wForceGpuCheckbox->setChecked(false);
	 QString sUseGpuHelpString = tr("Force GPU pathway will disable compatibility tasks to speed up converison times, but may cause black textures or other artifacts.");
	 m_wForceGpuCheckbox->setToolTip(sUseGpuHelpString);
	 m_wForceGpuCheckbox->setWhatsThis(sUseGpuHelpString);
	 useGpuLayout->addWidget(m_wForceGpuCheckbox);

	 //  Add Intermediate Folder to Advanced Settings container as a new row with specific headers
	 if (advancedLayout)
	 {
		 advancedLayout->insertRow(1, m_wForceGpuRowLabel, useGpuLayout);
		 m_aRowLabels.append(m_wForceGpuRowLabel);

		 advancedLayout->insertRow(1, m_wBlenderExecutablePathRowLabel, blenderExecutablePathLayout);
		 m_aRowLabels.append(m_wBlenderExecutablePathRowLabel);
		 
		 advancedLayout->addRow(m_wIntermediateFolderRowLabel, intermediateFolderLayout);
		 m_aRowLabels.append(m_wIntermediateFolderRowLabel);

		 // reposition the Open Intermediate Folder button so it aligns with the center section
		 advancedLayout->removeWidget(m_OpenIntermediateFolderButton);
		 m_wOpenIntermediateFolderButtonRowLabel = new QLabel("");
		 advancedLayout->addRow(m_wOpenIntermediateFolderButtonRowLabel, m_OpenIntermediateFolderButton);

	 }
	 else {
		 // if not available, add to mainLayout
		 mainLayout->insertRow(2, m_wBlenderExecutablePathRowLabel, blenderExecutablePathLayout);
		 mainLayout->addRow(m_wForceGpuRowLabel, useGpuLayout);
		 mainLayout->addRow(m_wIntermediateFolderRowLabel, intermediateFolderLayout);
		 m_aRowLabels.append(m_wForceGpuRowLabel);
		 m_aRowLabels.append(m_wBlenderExecutablePathRowLabel);
		 m_aRowLabels.append(m_wIntermediateFolderRowLabel);
	 }

	 // Disable Experimental Options Checkbox
	 m_enableExperimentalOptionsCheckBox->setEnabled(false);
	 m_enableExperimentalOptionsCheckBox->setToolTip(tr("No experimental options in this version."));
	 m_enableExperimentalOptionsCheckBox->setWhatsThis(tr("No experimental options in this version."));

	 QString sVersionString = tr("Roblox Studio Exporter %1 v%2.%3.%4").arg(PLUGIN_MAJOR).arg(PLUGIN_MINOR).arg(PLUGIN_REV).arg(PLUGIN_BUILD);
	 setBridgeVersionStringAndLabel(sVersionString);

	 //// Configure Target Plugin Installer
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

	 m_wModestyOverlayCombo->setToolTip(m_sModestyOverlayHelp);
	 m_wModestyOverlayCombo->setWhatsThis(m_sModestyOverlayHelp);
	 m_wModestyOverlayRowLabel->setToolTip(m_sModestyOverlayHelp);
	 m_wModestyOverlayRowLabel->setWhatsThis(m_sModestyOverlayHelp);

	 // Set Defaults
	 resetToDefaults();

	 // Load Settings
	 loadSavedSettings();

	 // called in base, but needs to be called in derived class for overriden method to fire
	 if (m_bSetupMode)
	 {
		 setDisabled(true);
	 }

	 disableAcceptUntilAllRequirementsValid();

	 // GUI Refresh
	 m_WelcomeLabel->hide();
	 setWindowTitle(tr("Roblox Export Options"));
	 wHelpMenuButton->insertItem(tr("Roblox Guidelines..."), ROBLOX_HELP_ID_GUIDELINES);
	 wHelpMenuButton->insertItem(tr("Character Specification..."), ROBLOX_HELP_ID_SPECIFICATION);
	 wHelpMenuButton->show();

	 fixRowLabelStyle();
	 fixRowLabelWidths();

}

bool DzRobloxDialog::loadSavedSettings()
{
#define LOAD_CHECKED(name,widget) if (!settings->value(name).isNull() && widget) widget->setChecked(settings->value(name).toBool());
#define LOAD_ITEMDATA(name,widget) if (!settings->value(name).isNull() && widget) widget->setCurrentIndex(widget->findData(settings->value(name)));

	DzBridgeDialog::loadSavedSettings();

	// intermediate path
	QString directoryName = "";
	if (!settings->value("IntermediatePath").isNull())
	{
		directoryName = settings->value("IntermediatePath").toString();
		intermediateFolderEdit->setText(directoryName);
	}
	if (directoryName == "")
	{
		QString DefaultPath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DazToRoblox";
		intermediateFolderEdit->setText(DefaultPath);
	}
	// roblox output path
	if (!settings->value("RobloxOutputPath").isNull())
	{
		m_wRobloxOutputFolderEdit->setText(settings->value("RobloxOutputPath").toString());
	}
	// blender executable path
	if (!settings->value("BlenderExecutablePath").isNull())
	{
		m_wBlenderExecutablePathEdit->setText(settings->value("BlenderExecutablePath").toString());
	}

	// gpu acceleration
	LOAD_CHECKED("ForceGpu", m_wForceGpuCheckbox);
	// asset type
	LOAD_ITEMDATA("AssetType", assetTypeCombo);
	// modesty overlay
	LOAD_ITEMDATA("ModestyOverlay", m_wModestyOverlayCombo);
	QString sFilePath = settings->value("ModestyOverlay").toString();
	if (!sFilePath.isEmpty() && sFilePath != "" && sFilePath.toLower().endsWith(".png")) {
		if (QFileInfo(sFilePath).exists()) {
			// check if filepath itemData is already in dropdown
			if (int nMatchingIndex = m_wModestyOverlayCombo->findData(sFilePath) != -1) {
				m_wModestyOverlayCombo->setCurrentIndex(nMatchingIndex);
			}
			else {
				QString sShortFilename = ".../" + QFileInfo(sFilePath).fileName();
				m_wModestyOverlayCombo->addItem(sShortFilename, QVariant(sFilePath));
				m_wModestyOverlayCombo->setCurrentIndex(m_wModestyOverlayCombo->count() - 1);
			}
		}
	}
	// breasts gone
	LOAD_CHECKED("BreastsGone", m_wBreastsGoneCheckbox);
	// replace eyebrows
	LOAD_ITEMDATA("Eyebrows", m_wEyebrowReplacement);
	// replace eyelashes
	LOAD_ITEMDATA("Eyelashes", m_wEyelashReplacement);
	// merge all clothing
	LOAD_CHECKED("MergeAllClothing", m_wBakeSingleOutfitCheckbox);
	// remove hidden geometry
	LOAD_CHECKED("RemoveHiddenGeometry", m_wHiddenSurfaceRemovalCheckbox);
	// remove hair cap
	LOAD_CHECKED("RemoveHairCap", m_wRemoveScalpMaterialCheckbox);

	// ROBLOX specific overrides
	int roblox_default_index = m_wBakeInstancesComboBox->findData("always");
	if (roblox_default_index != -1) m_wBakeInstancesComboBox->setCurrentIndex(roblox_default_index);
	roblox_default_index = m_wBakeCustomPivotsComboBox->findData("always");
	if (roblox_default_index != -1) m_wBakeCustomPivotsComboBox->setCurrentIndex(roblox_default_index);
	roblox_default_index = m_wBakeRigidFollowNodesComboBox->findData("always");
	if (roblox_default_index != -1) m_wBakeRigidFollowNodesComboBox->setCurrentIndex(roblox_default_index);

	return true;
}

void DzRobloxDialog::saveSettings()
{
#define SAVE_CHECKED(name, widget) if (widget) settings->setValue(name, widget->isChecked());
#define SAVE_ITEMDATA(name, widget) if (widget) settings->setValue(name, widget->itemData(widget->currentIndex()));

	if (settings == nullptr || m_bDontSaveSettings) return;

	DzBridgeDialog::saveSettings();

	///// Preserve manuallly entered paths ///////// 
	// Intermediate Path
	settings->setValue("IntermediatePath", intermediateFolderEdit->text());
	// Blender Executable Path
	settings->setValue("BlenderExecutablePath", m_wBlenderExecutablePathEdit->text());
	// Roblox Output Path
	settings->setValue("RobloxOutputPath", m_wRobloxOutputFolderEdit->text());

	// gpu acceleration
	SAVE_CHECKED("ForceGpu", m_wForceGpuCheckbox);
	// asset type
	SAVE_ITEMDATA("AssetType", assetTypeCombo);
	// modesty overlay
	SAVE_ITEMDATA("ModestyOverlay", m_wModestyOverlayCombo);
	// breasts gone
	SAVE_CHECKED("BreastsGone", m_wBreastsGoneCheckbox);
	// replace eyebrows
	SAVE_ITEMDATA("Eyebrows", m_wEyebrowReplacement);
	// replace eyelashes
	SAVE_ITEMDATA("Eyelashes", m_wEyelashReplacement);
	// merge all clothing
	SAVE_CHECKED("MergeAllClothing", m_wBakeSingleOutfitCheckbox);
	// remove hidden geometry
	SAVE_CHECKED("RemoveHiddenGeometry", m_wHiddenSurfaceRemovalCheckbox);
	// remove hair cap
	SAVE_CHECKED("RemoveHairCap", m_wRemoveScalpMaterialCheckbox);

}

void DzRobloxDialog::setDisabled(bool bDisable)
{
	DzBridgeDialog::setDisabled(bDisable);

	m_wModestyOverlayCombo->setDisabled(bDisable);
	m_wBreastsGoneCheckbox->setDisabled(bDisable);
	m_wEyebrowReplacement->setDisabled(bDisable);
	m_wEyelashReplacement->setDisabled(bDisable);
	m_wBakeSingleOutfitCheckbox->setDisabled(bDisable);
	m_wHiddenSurfaceRemovalCheckbox->setDisabled(bDisable);
	m_wRemoveScalpMaterialCheckbox->setDisabled(bDisable);

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

	// set Roblox specific defaults
	m_wResizeTexturesGroupBox->setDisabled(false);
	m_wResizeTexturesGroupBox->setChecked(true);
	//
	int roblox_default_index = m_wMaxTextureFileSizeCombo->findData(ROBLOX_MAX_TEXTURE_SIZE);
	if (roblox_default_index != -1) m_wMaxTextureFileSizeCombo->setCurrentIndex(roblox_default_index);
	//
	roblox_default_index = m_wMaxTextureResolutionCombo->findData(1024);
	if (roblox_default_index != -1) m_wMaxTextureResolutionCombo->setCurrentIndex(roblox_default_index);
	//
	roblox_default_index = m_wExportTextureFileFormatCombo->findData("png+jpg");
	if (roblox_default_index != -1) m_wExportTextureFileFormatCombo->setCurrentIndex(roblox_default_index);
	//
	roblox_default_index = m_wBakeInstancesComboBox->findData("always");
	if (roblox_default_index != -1) m_wBakeInstancesComboBox->setCurrentIndex(roblox_default_index);
	//
	roblox_default_index = m_wBakeCustomPivotsComboBox->findData("always");
	if (roblox_default_index != -1) m_wBakeCustomPivotsComboBox->setCurrentIndex(roblox_default_index);
	//
	roblox_default_index = m_wBakeRigidFollowNodesComboBox->findData("always");
	if (roblox_default_index != -1) m_wBakeRigidFollowNodesComboBox->setCurrentIndex(roblox_default_index);

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

// override and replace base method, which modifies assetTypeCombo to incompatible state
void DzRobloxDialog::refreshAsset()
{
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
	m_wRobloxOutputFolderRowLabel->setVisible(bVisible);
}

void DzRobloxDialog::HandleSelectBlenderExecutablePathButton()
{
	// DB 2023-10-13: prepopulate with existing folder string
	QString directoryName = "";
	QString sBlenderExePath = m_wBlenderExecutablePathEdit->text();
	directoryName = QFileInfo(sBlenderExePath).dir().path();
	if (directoryName == "." || directoryName == "") {
		if (settings != nullptr && settings->value("BlenderExecutablePath").isNull() != true) {
			sBlenderExePath = settings->value("BlenderExecutablePath").toString();
			directoryName = QFileInfo(sBlenderExePath).dir().path();
		}
		if (directoryName == "." || directoryName == "") {
#ifdef WIN32
			directoryName = "C:/Program Files/";
#elif defined (__APPLE__)
			directoryName = "/Applications/";
#endif
		}
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
		updateBlenderExecutablePathEdit(isBlenderTextBoxValid());
	}
	if (senderWidget == m_wRobloxOutputFolderEdit) {
		QString text = m_wRobloxOutputFolderEdit->text();
		int pos = 0;
		bool bIsValid = (m_dzValidatorFileExists.validate(text, pos) == QValidator::Acceptable);
		updateRobloxOutputFolderEdit(bIsValid);
	}
	disableAcceptUntilAllRequirementsValid();
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
	if (!m_bSetupMode) {
		if (!isBlenderTextBoxValid() || !isAssetTypeComboBoxValid())
		{
			this->setAcceptButtonText("Unable to Proceed");
			return false;
		}
	}
	this->setAcceptButtonText("Accept");
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
		updateRobloxOutputFolderEdit(false);
	}
	else if (m_wBlenderExecutablePathEdit->text() == "" || QFileInfo(m_wBlenderExecutablePathEdit->text()).exists() == false)
	{
		QMessageBox::warning(0, tr("Blender Executable Path"), tr("Blender Executable Path must be set."), QMessageBox::Ok);
		updateBlenderExecutablePathEdit(false);
		// Enable Advanced Settings
		this->showOptions();
	}
	else if (assetTypeCombo->itemData(assetTypeCombo->currentIndex()).toString() == "__")
	{
		QMessageBox::warning(0, tr("Select Asset Type"), tr("Please select an asset type from the dropdown menu."), QMessageBox::Ok);
	}

	return bSettingsValid;

}

void DzRobloxDialog::accept()
{
	if (m_bSetupMode) {
		saveSettings();
		return DzBasicDialog::reject();
	}

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

#include "qdatetime.h"
#include "dzauthor.h"

bool DzRobloxDialog::showDisclaimer()
{

	QString sSettingsUsername = settings->value("EulaAgreement_Username", "").toString();
	QString sSettingsPluginVersion = settings->value("EulaAgreement_PluginVersion", "").toString();
	QDateTime oSettingsCurrentDateTime = settings->value("EulaAgreement_Date", QDateTime::currentDateTime()).toDateTime();


	QString sUsername = QString::fromLocal8Bit(qgetenv("USER"));
	if (sUsername == "") {
		sUsername = QString::fromLocal8Bit(qgetenv("USERNAME"));
	}
	QString sPluginVersion = QString("%1.%2.%3.%4").arg(PLUGIN_MAJOR).arg(PLUGIN_MINOR).arg(PLUGIN_REV).arg(PLUGIN_BUILD);
	QDateTime oCurrentDateTime = QDateTime::currentDateTime();

	int nDaysPassed = oSettingsCurrentDateTime.daysTo(oCurrentDateTime);

	bool bAlreadyAgreed = false;
	if ( sUsername == sSettingsUsername && sSettingsPluginVersion == sPluginVersion )
	{
		bAlreadyAgreed = true;
		if (nDaysPassed > 365) {
			bAlreadyAgreed = false;
		}
	}

	if (bAlreadyAgreed) {
		return true;
	}

	QString content = DAZTOROBLOX_EULA;

	QTextBrowser* wContent = new QTextBrowser();
	wContent->setText(content);
	wContent->setOpenExternalLinks(true);

	QString sWindowTitle = tr("Daz To Roblox Studio Terms");
	m_wEulaAgreementDialog = new DzBasicDialog(NULL, sWindowTitle);
	QCheckBox* wAgreeCheckbox = new QCheckBox(tr("I agree to the Daz to Roblox Studio Terms."));
	wAgreeCheckbox->setChecked(false);
	connect(wAgreeCheckbox, SIGNAL(toggled(bool)), this, SLOT(HandleAgreeEulaCheckbox(bool)));

	m_wEulaAgreementDialog->setAcceptButtonText(tr("Accept Terms"));
	m_wEulaAgreementDialog->setAcceptButtonEnabled(false);

	m_wEulaAgreementDialog->setCancelButtonText(tr("Decline"));
	m_wEulaAgreementDialog->setWindowTitle(sWindowTitle);
	m_wEulaAgreementDialog->setMinimumWidth(500);
	m_wEulaAgreementDialog->setMinimumHeight(450);
	QGridLayout* layout = new QGridLayout(m_wEulaAgreementDialog);
	layout->addWidget(wContent);
	layout->addWidget(wAgreeCheckbox, 1, 0, Qt::AlignHCenter);
	m_wEulaAgreementDialog->addLayout(layout);
	int result = m_wEulaAgreementDialog->exec();

	if (result == QDialog::DialogCode::Rejected || result != QDialog::DialogCode::Accepted) {
		return false;
	}

	// record in Settings (registry)
	settings->setValue("EulaAgreement_Username", sUsername);
	settings->setValue("EulaAgreement_Date", oCurrentDateTime);
	settings->setValue("EulaAgreement_PluginVersion", sPluginVersion);

	return true;
}

void DzRobloxDialog::HandleAgreeEulaCheckbox(bool checked) {

	if (!m_wEulaAgreementDialog) return;

	if (checked) {
		m_wEulaAgreementDialog->setAcceptButtonEnabled(true);
	}
	else {
		m_wEulaAgreementDialog->setAcceptButtonEnabled(false);
	}

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
		else 
		{
			// gracefully fail
			dzApp->log("DzRoblox: ERROR: HandleCustomModestyOverlayActivated(): sFilePath does not exist: " + sFilePath);
		}
	}
}

void DzRobloxDialog::enableModestyOptions(bool bEnable)
{
	if (!m_bSetupMode) m_wModestyOverlayCombo->setDisabled(!bEnable);

	if (bEnable) {
		m_wModestyOverlayCombo->setToolTip(m_sModestyOverlayHelp);
		m_wModestyOverlayCombo->setWhatsThis(m_sModestyOverlayHelp);
		m_wModestyOverlayRowLabel->setToolTip(m_sModestyOverlayHelp);
		m_wModestyOverlayRowLabel->setWhatsThis(m_sModestyOverlayHelp);
	}
	else {
		// update help to disabled widget help
		m_wModestyOverlayCombo->setToolTip(m_sModestyOverlayDisabledNotice);
		m_wModestyOverlayCombo->setWhatsThis(m_sModestyOverlayDisabledNotice);
		m_wModestyOverlayRowLabel->setToolTip(m_sModestyOverlayDisabledNotice);
		m_wModestyOverlayRowLabel->setWhatsThis(m_sModestyOverlayDisabledNotice);
	}

	return;
}

// overrides base method
void DzRobloxDialog::HandleHelpMenuButton(int index)
{
	int id = wHelpMenuButton->getSelectionID();

	switch (id) {

	case ROBLOX_HELP_ID_GUIDELINES:
		HandleRobloxGuidelinesButton();
		break;

	case ROBLOX_HELP_ID_SPECIFICATION:
		HandleRobloxCharacterSpecification();
		break;

	default:
		return DzBridgeDialog::HandleHelpMenuButton(index);
	}

	wHelpMenuButton->setIndeterminate();
}

void DzRobloxDialog::HandlePdfButton()
{
	QString sPdfPath = "https://github.com/daz3d/DazToRoblox?tab=readme-ov-file#readme";
	QDesktopServices::openUrl(QUrl(sPdfPath));
}

void DzRobloxDialog::HandleYoutubeButton()
{
	// Roblox Avatar Help
//	QString url = "https://youtu.be/2My8jE47clI?si=Nhczl-xIPUlprp7D"; // What are Custom Characters? Roblox Sketch Series 
	QString url = "https://www.youtube.com/playlist?list=PLF3LSR7D48MeY3tzVo419FX3kBifsSTdD"; // Daz to Roblox Exporter Playlist
	QDesktopServices::openUrl(QUrl(url));
}

void DzRobloxDialog::HandleSupportButton()
{
	QString url = "https://bugs.daz3d.com/hc/en-us/requests/new";
	QDesktopServices::openUrl(QUrl(url));
}

void DzRobloxDialog::HandleRobloxGuidelinesButton()
{
	QString url = "https://create.roblox.com/docs/marketplace/marketplace-policy#age-appropriate";
	QDesktopServices::openUrl(QUrl(url));
}

void DzRobloxDialog::HandleRobloxCharacterSpecification()
{
	QString url = "https://create.roblox.com/docs/art/characters/specifications";
	QDesktopServices::openUrl(QUrl(url));
}

#include "dzstyle.h"
#include "dzstyledbutton.h"
void DzRobloxDialog::updateBlenderExecutablePathEdit(bool isValid) {
	if (!isValid) {
		m_wBlenderExecutablePathButton->setHighlightStyle(true);
	}
	else {
		m_wBlenderExecutablePathButton->setHighlightStyle(false);
	}
}

void DzRobloxDialog::updateRobloxOutputFolderEdit(bool isValid)
{
	if (!isValid) {
		m_wRobloxOutputFolderButton->setHighlightStyle(true);
	}
	else {
		m_wRobloxOutputFolderButton->setHighlightStyle(false);
	}
}


#include "moc_DzRobloxDialog.cpp"
