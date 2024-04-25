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
#define DAZ_BRIDGE_PLUGIN_NAME "Roblox Avatar Exporter"

#include "dzbridge.h"

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
	 setWindowTitle(tr("Roblox Avatar Exporter %1 v%2.%3").arg(PLUGIN_MAJOR).arg(PLUGIN_MINOR).arg(PLUGIN_REV));
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
	 assetTypeCombo->addItem("Please select an asset type...", "__");
	 assetTypeCombo->addItem("Roblox R15 Avatar", "R15");
	 assetTypeCombo->addItem("Roblox R15 Avatar (tanktop)", "R15_M");
	 assetTypeCombo->addItem("Roblox S1 for Avatar Auto Setup", "S1");
	 assetTypeCombo->addItem("Roblox S1 for Avatar Auto Setup (tanktop)", "S1_M");
	 //	 assetTypeCombo->addItem("Roblox R15 Avatar (No modesty covers)", "R15Z");
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

	 // Add Project Folder
	 QHBoxLayout* robloxOutputFolderLayout = new QHBoxLayout();
	 m_wRobloxOutputFolderEdit = new QLineEdit(this);
	 m_wRobloxOutputFolderButton = new QPushButton("...", this);
	 robloxOutputFolderLayout->addWidget(m_wRobloxOutputFolderEdit);
	 robloxOutputFolderLayout->addWidget(m_wRobloxOutputFolderButton);
	 connect(m_wRobloxOutputFolderButton, SIGNAL(released()), this, SLOT(HandleSelectRobloxOutputFolderButton()));

	 // Add GUI
	 mainLayout->insertRow(1, "Roblox Output Folder", robloxOutputFolderLayout);
	 m_wGodotProjectFolderRowLabelWidget = mainLayout->itemAt(1, QFormLayout::LabelRole)->widget();
	 showRobloxOptions(true);
	 this->showLodRow(false);

	 // Select Blender Executable Path GUI
	 QHBoxLayout* blenderExecutablePathLayout = new QHBoxLayout();
	 m_wBlenderExecutablePathEdit = new QLineEdit(this);
	 m_wBlenderExecutablePathButton = new QPushButton("...", this);
	 blenderExecutablePathLayout->addWidget(m_wBlenderExecutablePathEdit);
	 blenderExecutablePathLayout->addWidget(m_wBlenderExecutablePathButton);
	 connect(m_wBlenderExecutablePathButton, SIGNAL(released()), this, SLOT(HandleSelectBlenderExecutablePathButton()));


	 // Intermediate Folder
	 QHBoxLayout* intermediateFolderLayout = new QHBoxLayout();
	 intermediateFolderEdit = new QLineEdit(this);
	 intermediateFolderButton = new QPushButton("...", this);
	 intermediateFolderLayout->addWidget(intermediateFolderEdit);
	 intermediateFolderLayout->addWidget(intermediateFolderButton);
	 connect(intermediateFolderButton, SIGNAL(released()), this, SLOT(HandleSelectIntermediateFolderButton()));

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

	 QString sVersionString = tr("Roblox Avatar Exporter %1 v%2.%3.%4").arg(PLUGIN_MAJOR).arg(PLUGIN_MINOR).arg(PLUGIN_REV).arg(PLUGIN_BUILD);
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
	 assetNameEdit->setWhatsThis("This is the name the asset will use in Godot.");
	 assetTypeCombo->setWhatsThis("Skeletal Mesh for something with moving parts, like a character\nStatic Mesh for things like props\nAnimation for a character animation.");
	 intermediateFolderEdit->setWhatsThis("Roblox Avatar Exporter will collect the assets in a subfolder under this folder.");
	 intermediateFolderButton->setWhatsThis("Roblox Avatar Exporter will collect the assets in a subfolder under this folder.");
	 //m_wTargetPluginInstaller->setWhatsThis("You can install the Godot Plugin by selecting the desired Godot version and then clicking Install.");

	 // Set Defaults
	 resetToDefaults();

	 // Load Settings
	 loadSavedSettings();

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

void DzRobloxDialog::HandleAssetTypeComboChange(int state)
{

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

#include "moc_DzRobloxDialog.cpp"
