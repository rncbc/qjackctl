// qjackctlSetupForm.cpp
//
/****************************************************************************
   Copyright (C) 2003-2016, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "qjackctlAbout.h"
#include "qjackctlSetupForm.h"

#include "qjackctlMainForm.h"

#include "qjackctlSetup.h"

#include <QValidator>
#include <QMessageBox>
#include <QFileDialog>
#include <QFontDialog>
#include <QTextStream>
#include <QLineEdit>
#include <QPixmap>
#include <QMenu>

#include <QButtonGroup>

#ifdef CONFIG_COREAUDIO
#include <iostream>
#include <cstring>
#include <map>
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CFString.h>
#endif

#ifdef CONFIG_PORTAUDIO
#include <iostream>
#include <cstring>
#include <portaudio.h>
#endif

#ifdef CONFIG_ALSA_SEQ
#include <alsa/asoundlib.h>
#endif


//----------------------------------------------------------------------------
// qjackctlSetupForm -- UI wrapper form.

// Constructor.
qjackctlSetupForm::qjackctlSetupForm (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QDialog(pParent, wflags)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	// No settings descriptor initially (the caller will set it).
	m_pSetup = NULL;

	// Setup time-display radio-button group.
	m_pTimeDisplayButtonGroup = new QButtonGroup(this);
	m_pTimeDisplayButtonGroup->addButton(m_ui.TransportTimeRadioButton, 0);
	m_pTimeDisplayButtonGroup->addButton(m_ui.TransportBBTRadioButton,  1);
	m_pTimeDisplayButtonGroup->addButton(m_ui.ElapsedResetRadioButton,  2);
	m_pTimeDisplayButtonGroup->addButton(m_ui.ElapsedXrunRadioButton,   3);
	m_pTimeDisplayButtonGroup->setExclusive(true);

	// Initialize dirty control state.
	m_iDirtySetup = 0;
	m_iDirtySettings = 0;
	m_iDirtyOptions = 0;

	// Set dialog validators...
	m_ui.PresetComboBox->setValidator(
		new QRegExpValidator(QRegExp("[\\w-]+"), m_ui.PresetComboBox));
	m_ui.FramesComboBox->setValidator(
		new QIntValidator(m_ui.FramesComboBox));
	m_ui.SampleRateComboBox->setValidator(
		new QIntValidator(m_ui.SampleRateComboBox));
	m_ui.WaitComboBox->setValidator(
		new QIntValidator(m_ui.WaitComboBox));
	m_ui.WordLengthComboBox->setValidator(
		new QIntValidator(m_ui.WordLengthComboBox));
	m_ui.TimeoutComboBox->setValidator(
		new QIntValidator(m_ui.TimeoutComboBox));
	m_ui.PortMaxComboBox->setValidator(
		new QIntValidator(m_ui.PortMaxComboBox));
	m_ui.MessagesLimitLinesComboBox->setValidator(
		new QIntValidator(m_ui.MessagesLimitLinesComboBox));

	m_ui.PresetComboBox->setCompleter(NULL);

	m_ui.ServerNameComboBox->setCompleter(NULL);
	m_ui.ServerPrefixComboBox->setCompleter(NULL);
	m_ui.ServerSuffixComboBox->setCompleter(NULL);

	// UI connections...

	QObject::connect(m_ui.PresetComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(changeCurrentPreset(const QString&)));
	QObject::connect(m_ui.PresetSavePushButton,
		SIGNAL(clicked()),
		SLOT(saveCurrentPreset()));
	QObject::connect(m_ui.PresetDeletePushButton,
		SIGNAL(clicked()),
		SLOT(deleteCurrentPreset()));

	QObject::connect(m_ui.DriverComboBox,
		SIGNAL(activated(int)),
		SLOT(changeDriver(int)));
	QObject::connect(m_ui.AudioComboBox,
		SIGNAL(activated(int)),
		SLOT(changeAudio(int)));
	QObject::connect(m_ui.ServerPrefixComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.ServerNameComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.DriverComboBox,
		SIGNAL(activated(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.RealtimeCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.NoMemLockCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.SoftModeCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.MonitorCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.ShortsCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.HWMonCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.HWMeterCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.IgnoreHWCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.UnlockMemCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.VerboseCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.PrioritySpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.FramesComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.SampleRateComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.PeriodsSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.WordLengthComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.WaitComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.ChanSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.InterfaceComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.AudioComboBox,
		SIGNAL(activated(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.DitherComboBox,
		SIGNAL(activated(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.TimeoutComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.InDeviceComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.OutDeviceComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.InChannelsSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.OutChannelsSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.InLatencySpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.OutLatencySpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.MidiDriverComboBox,
		SIGNAL(activated(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.StartDelaySpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.PortMaxComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.ServerSuffixComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(settingsChanged()));

	QObject::connect(m_ui.StartupScriptCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.StartupScriptShellComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.PostStartupScriptCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.PostStartupScriptShellComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ShutdownScriptCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ShutdownScriptShellComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.PostShutdownScriptCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.PostShutdownScriptShellComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.StartupScriptBrowseToolButton,
		SIGNAL(clicked()),
		SLOT(browseStartupScript()));
	QObject::connect(m_ui.PostStartupScriptBrowseToolButton,
		SIGNAL(clicked()),
		SLOT(browsePostStartupScript()));
	QObject::connect(m_ui.ShutdownScriptBrowseToolButton,
		SIGNAL(clicked()),
		SLOT(browseShutdownScript()));
	QObject::connect(m_ui.PostShutdownScriptBrowseToolButton,
		SIGNAL(clicked()),
		SLOT(browsePostShutdownScript()));
	QObject::connect(m_ui.StartupScriptSymbolToolButton,
		SIGNAL(clicked()),
		SLOT(symbolStartupScript()));
	QObject::connect(m_ui.PostStartupScriptSymbolToolButton,
		SIGNAL(clicked()),
		SLOT(symbolPostStartupScript()));
	QObject::connect(m_ui.ShutdownScriptSymbolToolButton,
		SIGNAL(clicked()),
		SLOT(symbolShutdownScript()));
	QObject::connect(m_ui.PostShutdownScriptSymbolToolButton,
		SIGNAL(clicked()),
		SLOT(symbolPostShutdownScript()));

	QObject::connect(m_ui.StdoutCaptureCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.XrunRegexComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ActivePatchbayCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ActivePatchbayPathComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ActivePatchbayPathToolButton,
		SIGNAL(clicked()),
		SLOT(browseActivePatchbayPath()));
	QObject::connect(m_ui.ActivePatchbayResetCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.QueryDisconnectCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.MessagesLogCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.MessagesLogPathComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.MessagesLogPathToolButton,
		SIGNAL(clicked()),
		SLOT(browseMessagesLogPath()));
	QObject::connect(m_ui.TransportTimeRadioButton,
		SIGNAL(toggled(bool)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.TransportBBTRadioButton,
		SIGNAL(toggled(bool)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ElapsedResetRadioButton,
		SIGNAL(toggled(bool)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ElapsedXrunRadioButton,
		SIGNAL(toggled(bool)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.TimeFormatComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.DisplayEffectCheckBox,
		SIGNAL(toggled(bool)),
		SLOT(toggleDisplayEffect(bool)));
	QObject::connect(m_ui.DisplayBlinkCheckBox,
		SIGNAL(toggled(bool)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.DisplayFont1PushButton,
		SIGNAL(clicked()),
		SLOT(chooseDisplayFont1()));
	QObject::connect(m_ui.DisplayFont2PushButton,
		SIGNAL(clicked()),
		SLOT(chooseDisplayFont2()));
	QObject::connect(m_ui.MessagesFontPushButton,
		SIGNAL(clicked()),
		SLOT(chooseMessagesFont()));
	QObject::connect(m_ui.ConnectionsFontPushButton,
		SIGNAL(clicked()),
		SLOT(chooseConnectionsFont()));
	QObject::connect(m_ui.MessagesLimitCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.MessagesLimitLinesComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ConnectionsIconSizeComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.BezierLinesCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.AliasesEnabledCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.AliasesEditingCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.JackClientPortAliasComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.JackClientPortMetadataCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));

	QObject::connect(m_ui.StartJackCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.StopJackCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.QueryCloseCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.QueryShutdownCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.KeepOnTopCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QString sHideMinimize = tr("Mi&nimize");
	QString sShowRestore  = tr("Rest&ore");
#ifdef CONFIG_SYSTEM_TRAY
	QObject::connect(m_ui.SystemTrayCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.SystemTrayQueryCloseCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
#endif
	QObject::connect(m_ui.SingletonCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ServerConfigCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ServerConfigNameComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ServerConfigTempCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.AlsaSeqEnabledCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.DBusEnabledCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.LeftButtonsCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.RightButtonsCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.TransportButtonsCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.TextLabelsCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.BaseFontSizeComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(optionsChanged()));

	QObject::connect(m_ui.DialogButtonBox,
		SIGNAL(accepted()),
		SLOT(accept()));
	QObject::connect(m_ui.DialogButtonBox,
		SIGNAL(rejected()),
		SLOT(reject()));

	// Try to restore old window positioning.
	adjustSize();
}


// Destructor.
qjackctlSetupForm::~qjackctlSetupForm (void)
{
	delete m_pTimeDisplayButtonGroup;
}


// A combo-box text item setter helper.
void qjackctlSetupForm::setComboBoxCurrentText (
	QComboBox *pComboBox, const QString& sText ) const
{
	if (pComboBox->isEditable()) {
		pComboBox->setEditText(sText);
	} else {
		int iIndex = pComboBox->findText(sText);
		if (iIndex < 0) {
			iIndex = 0;
			if (!sText.isEmpty())
				pComboBox->insertItem(0, sText);
		}
		pComboBox->setCurrentIndex(iIndex);
	}
}


// Populate (setup) dialog controls from settings descriptors.
void qjackctlSetupForm::setup ( qjackctlSetup *pSetup )
{
	// Set reference descriptor.
	m_pSetup = pSetup;

	// Avoid dirty this all up.
	m_iDirtySetup++;

	// Load combo box history...
	m_pSetup->loadComboBoxHistory(m_ui.ServerPrefixComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.ServerNameComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.ServerSuffixComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.StartupScriptShellComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.PostStartupScriptShellComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.ShutdownScriptShellComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.PostShutdownScriptShellComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.XrunRegexComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.ActivePatchbayPathComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.MessagesLogPathComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.ServerConfigNameComboBox);

	m_ui.InterfaceComboBox->setup(
		m_ui.DriverComboBox, QJACKCTL_DUPLEX, m_pSetup->sDefPresetName);
	m_ui.InDeviceComboBox->setup(
		m_ui.DriverComboBox, QJACKCTL_CAPTURE, m_pSetup->sDefPresetName);
	m_ui.OutDeviceComboBox->setup(
		m_ui.DriverComboBox, QJACKCTL_PLAYBACK, m_pSetup->sDefPresetName);

	// Load Options...
	m_ui.StartupScriptCheckBox->setChecked(m_pSetup->bStartupScript);
	setComboBoxCurrentText(m_ui.StartupScriptShellComboBox,
		m_pSetup->sStartupScriptShell);
	m_ui.PostStartupScriptCheckBox->setChecked(m_pSetup->bPostStartupScript);
	setComboBoxCurrentText(m_ui.PostStartupScriptShellComboBox,
		m_pSetup->sPostStartupScriptShell);
	m_ui.ShutdownScriptCheckBox->setChecked(m_pSetup->bShutdownScript);
	setComboBoxCurrentText(m_ui.ShutdownScriptShellComboBox,
		m_pSetup->sShutdownScriptShell);
	m_ui.PostShutdownScriptCheckBox->setChecked(m_pSetup->bPostShutdownScript);
	setComboBoxCurrentText(m_ui.PostShutdownScriptShellComboBox,
		m_pSetup->sPostShutdownScriptShell);
	m_ui.StdoutCaptureCheckBox->setChecked(m_pSetup->bStdoutCapture);
	setComboBoxCurrentText(m_ui.XrunRegexComboBox,
		m_pSetup->sXrunRegex);
	m_ui.ActivePatchbayCheckBox->setChecked(m_pSetup->bActivePatchbay);
	setComboBoxCurrentText(m_ui.ActivePatchbayPathComboBox,
		m_pSetup->sActivePatchbayPath);
	m_ui.ActivePatchbayResetCheckBox->setChecked(m_pSetup->bActivePatchbayReset);
	m_ui.QueryDisconnectCheckBox->setChecked(m_pSetup->bQueryDisconnect);
	m_ui.MessagesLogCheckBox->setChecked(m_pSetup->bMessagesLog);
	setComboBoxCurrentText(m_ui.MessagesLogPathComboBox,
		m_pSetup->sMessagesLogPath);
	m_ui.BezierLinesCheckBox->setChecked(m_pSetup->bBezierLines);

	// Load some other defaults...
	QRadioButton *pRadioButton
		= static_cast<QRadioButton *> (
			m_pTimeDisplayButtonGroup->button(m_pSetup->iTimeDisplay));
	if (pRadioButton)
		pRadioButton->setChecked(true);

	m_ui.TimeFormatComboBox->setCurrentIndex(m_pSetup->iTimeFormat);

	// Load font chooser samples...
	const QString sSansSerif = "Sans Serif";
	QFont font;
	QPalette pal;

	if (m_pSetup->sDisplayFont1.isEmpty()
		|| !font.fromString(m_pSetup->sDisplayFont1))
		font = QFont(sSansSerif, 12, QFont::Bold);
	m_ui.DisplayFont1TextLabel->setFont(font);
	m_ui.DisplayFont1TextLabel->setText(
		font.family() + ' ' + QString::number(font.pointSize()));

	if (m_pSetup->sDisplayFont2.isEmpty()
		|| !font.fromString(m_pSetup->sDisplayFont2))
		font = QFont(sSansSerif, 6, QFont::Bold);
	m_ui.DisplayFont2TextLabel->setFont(font);
	m_ui.DisplayFont2TextLabel->setText(
		font.family() + ' ' + QString::number(font.pointSize()));

	if (m_pSetup->sMessagesFont.isEmpty()
		|| !font.fromString(m_pSetup->sMessagesFont))
		font = QFont("Monospace", 8);
	pal = m_ui.MessagesFontTextLabel->palette();
	pal.setColor(QPalette::Background, pal.base().color());
	m_ui.MessagesFontTextLabel->setPalette(pal);
	m_ui.MessagesFontTextLabel->setFont(font);
	m_ui.MessagesFontTextLabel->setText(
		font.family() + ' ' + QString::number(font.pointSize()));

	if (m_pSetup->sConnectionsFont.isEmpty()
		|| !font.fromString(m_pSetup->sConnectionsFont))
		font = QFont(sSansSerif, 10);
	pal = m_ui.ConnectionsFontTextLabel->palette();
	pal.setColor(QPalette::Background, pal.base().color());
	m_ui.ConnectionsFontTextLabel->setPalette(pal);
	m_ui.ConnectionsFontTextLabel->setFont(font);
	m_ui.ConnectionsFontTextLabel->setText(
		font.family() + ' ' + QString::number(font.pointSize()));

	// The main display shiny effect option.
	m_ui.DisplayEffectCheckBox->setChecked(m_pSetup->bDisplayEffect);
	m_ui.DisplayBlinkCheckBox->setChecked(m_pSetup->bDisplayBlink);
	toggleDisplayEffect(m_pSetup->bDisplayEffect);

	// Connections view icon size.
	m_ui.ConnectionsIconSizeComboBox->setCurrentIndex(
		m_pSetup->iConnectionsIconSize);
	// and this JACK specialities...
	m_ui.JackClientPortAliasComboBox->setCurrentIndex(
		m_pSetup->iJackClientPortAlias);
	m_ui.JackClientPortMetadataCheckBox->setChecked(
		m_pSetup->bJackClientPortMetadata);

	// Messages limit option.
	m_ui.MessagesLimitCheckBox->setChecked(m_pSetup->bMessagesLimit);
	setComboBoxCurrentText(m_ui.MessagesLimitLinesComboBox,
		QString::number(m_pSetup->iMessagesLimitLines));

	// Other misc options...
	m_ui.StartJackCheckBox->setChecked(m_pSetup->bStartJack);
	m_ui.StopJackCheckBox->setChecked(m_pSetup->bStopJack);
	m_ui.QueryCloseCheckBox->setChecked(m_pSetup->bQueryClose);
	m_ui.QueryShutdownCheckBox->setChecked(m_pSetup->bQueryShutdown);
	m_ui.KeepOnTopCheckBox->setChecked(m_pSetup->bKeepOnTop);
#ifdef CONFIG_SYSTEM_TRAY
	m_ui.SystemTrayCheckBox->setChecked(m_pSetup->bSystemTray);
	m_ui.SystemTrayQueryCloseCheckBox->setChecked(m_pSetup->bSystemTrayQueryClose);
#endif
	m_ui.SingletonCheckBox->setChecked(m_pSetup->bSingleton);
	m_ui.ServerConfigCheckBox->setChecked(m_pSetup->bServerConfig);
	setComboBoxCurrentText(m_ui.ServerConfigNameComboBox,
		m_pSetup->sServerConfigName);
	m_ui.ServerConfigTempCheckBox->setChecked(m_pSetup->bServerConfigTemp);
	m_ui.AlsaSeqEnabledCheckBox->setChecked(m_pSetup->bAlsaSeqEnabled);
	m_ui.DBusEnabledCheckBox->setChecked(m_pSetup->bDBusEnabled);
	m_ui.AliasesEnabledCheckBox->setChecked(m_pSetup->bAliasesEnabled);
	m_ui.AliasesEditingCheckBox->setChecked(m_pSetup->bAliasesEditing);
	m_ui.LeftButtonsCheckBox->setChecked(!m_pSetup->bLeftButtons);
	m_ui.RightButtonsCheckBox->setChecked(!m_pSetup->bRightButtons);
	m_ui.TransportButtonsCheckBox->setChecked(!m_pSetup->bTransportButtons);
	m_ui.TextLabelsCheckBox->setChecked(!m_pSetup->bTextLabels);
	if (m_pSetup->iBaseFontSize > 0)
		m_ui.BaseFontSizeComboBox->setEditText(QString::number(m_pSetup->iBaseFontSize));
	else
		m_ui.BaseFontSizeComboBox->setCurrentIndex(0);

#ifndef CONFIG_SYSTEM_TRAY
	m_ui.SystemTrayCheckBox->setChecked(false);
	m_ui.SystemTrayCheckBox->setEnabled(false);
	m_ui.SystemTrayQueryCloseCheckBox->setChecked(false);
	m_ui.SystemTrayQueryCloseCheckBox->setEnabled(false);
	m_ui.StartMinimizedCheckBox->setChecked(false);
	m_ui.StartMinimizedCheckBox->setEnabled(false);
#endif
#ifndef CONFIG_JACK_MIDI
	m_ui.MidiDriverComboBox->setCurrentIndex(0);
	m_ui.MidiDriverTextLabel->setEnabled(false);
	m_ui.MidiDriverComboBox->setEnabled(false);
#endif
#ifndef CONFIG_JACK_PORT_ALIASES
	m_ui.JackClientPortAliasComboBox->setCurrentIndex(0);
	m_ui.JackClientPortAliasTextLabel->setEnabled(false);
	m_ui.JackClientPortAliasComboBox->setEnabled(false);
#endif
#ifndef CONFIG_JACK_METADATA
	m_ui.JackClientPortMetadataCheckBox->setChecked(false);
	m_ui.JackClientPortMetadataCheckBox->setEnabled(false);
#endif
#ifndef CONFIG_ALSA_SEQ
	m_ui.AlsaSeqEnabledCheckBox->setEnabled(false);
#endif
#ifndef CONFIG_DBUS
	m_ui.DBusEnabledCheckBox->setEnabled(false);
#endif

	// Load preset list...
	resetPresets();
	setComboBoxCurrentText(m_ui.PresetComboBox,
		m_pSetup->sDefPreset);
	// Finally, load default settings...
	changePreset(m_ui.PresetComboBox->currentText());

	// We're clean now.
	m_iDirtySetup--;
	stabilizeForm();
}


void qjackctlSetupForm::changePreset ( const QString& sPreset )
{
	if (sPreset.isEmpty())
		return;

	// Load Settings...
	qjackctlPreset preset;
	if (m_pSetup->loadPreset(preset, sPreset)) {
		setComboBoxCurrentText(m_ui.ServerPrefixComboBox, preset.sServerPrefix);
		setComboBoxCurrentText(m_ui.ServerNameComboBox,
			preset.sServerName.isEmpty()
			? m_pSetup->sDefPresetName
			: preset.sServerName);
		m_ui.RealtimeCheckBox->setChecked(preset.bRealtime);
		m_ui.SoftModeCheckBox->setChecked(preset.bSoftMode);
		m_ui.MonitorCheckBox->setChecked(preset.bMonitor);
		m_ui.ShortsCheckBox->setChecked(preset.bShorts);
		m_ui.NoMemLockCheckBox->setChecked(preset.bNoMemLock);
		m_ui.UnlockMemCheckBox->setChecked(preset.bUnlockMem);
		m_ui.HWMonCheckBox->setChecked(preset.bHWMon);
		m_ui.HWMeterCheckBox->setChecked(preset.bHWMeter);
		m_ui.IgnoreHWCheckBox->setChecked(preset.bIgnoreHW);
		m_ui.PrioritySpinBox->setValue(preset.iPriority);
		setComboBoxCurrentText(m_ui.FramesComboBox,
			QString::number(preset.iFrames));
		setComboBoxCurrentText(m_ui.SampleRateComboBox,
			QString::number(preset.iSampleRate));
		m_ui.PeriodsSpinBox->setValue(preset.iPeriods);
		setComboBoxCurrentText(m_ui.WordLengthComboBox,
			QString::number(preset.iWordLength));
		setComboBoxCurrentText(m_ui.WaitComboBox,
		QString::number(preset.iWait));
		m_ui.ChanSpinBox->setValue(preset.iChan);
		setComboBoxCurrentText(m_ui.DriverComboBox, preset.sDriver);
		setComboBoxCurrentText(m_ui.InterfaceComboBox,
			preset.sInterface.isEmpty()
				? m_pSetup->sDefPresetName
				: preset.sInterface);
		m_ui.AudioComboBox->setCurrentIndex(preset.iAudio);
		m_ui.DitherComboBox->setCurrentIndex(preset.iDither);
		setComboBoxCurrentText(m_ui.TimeoutComboBox,
			QString::number(preset.iTimeout));
		setComboBoxCurrentText(m_ui.InDeviceComboBox,
			preset.sInDevice.isEmpty()
				? m_pSetup->sDefPresetName
				: preset.sInDevice);
		setComboBoxCurrentText(m_ui.OutDeviceComboBox,
			preset.sOutDevice.isEmpty()
				? m_pSetup->sDefPresetName
				: preset.sOutDevice);
		m_ui.InChannelsSpinBox->setValue(preset.iInChannels);
		m_ui.OutChannelsSpinBox->setValue(preset.iOutChannels);
		m_ui.InLatencySpinBox->setValue(preset.iInLatency);
		m_ui.OutLatencySpinBox->setValue(preset.iOutLatency);
		m_ui.StartDelaySpinBox->setValue(preset.iStartDelay);
		m_ui.VerboseCheckBox->setChecked(preset.bVerbose);
		setComboBoxCurrentText(m_ui.PortMaxComboBox,
			QString::number(preset.iPortMax));
#ifdef CONFIG_JACK_MIDI
		setComboBoxCurrentText(m_ui.MidiDriverComboBox,
			preset.sMidiDriver);
#endif
		setComboBoxCurrentText(m_ui.ServerSuffixComboBox, preset.sServerSuffix);
		// Reset dirty flag.
		m_iDirtySettings = 0;
	}

	// Set current preset name..
	m_sPreset = sPreset;
}


bool qjackctlSetupForm::savePreset ( const QString& sPreset )
{
	if (sPreset.isEmpty())
		return false;

	// Unload settings.
	qjackctlPreset preset;
	preset.sServerPrefix = m_ui.ServerPrefixComboBox->currentText();
	preset.sServerName  = m_ui.ServerNameComboBox->currentText();
	preset.bRealtime    = m_ui.RealtimeCheckBox->isChecked();
	preset.bSoftMode    = m_ui.SoftModeCheckBox->isChecked();
	preset.bMonitor     = m_ui.MonitorCheckBox->isChecked();
	preset.bShorts      = m_ui.ShortsCheckBox->isChecked();
	preset.bNoMemLock   = m_ui.NoMemLockCheckBox->isChecked();
	preset.bUnlockMem   = m_ui.UnlockMemCheckBox->isChecked();
	preset.bHWMon       = m_ui.HWMonCheckBox->isChecked();
	preset.bHWMeter     = m_ui.HWMeterCheckBox->isChecked();
	preset.bIgnoreHW    = m_ui.IgnoreHWCheckBox->isChecked();
	preset.iPriority    = m_ui.PrioritySpinBox->value();
	preset.iFrames      = m_ui.FramesComboBox->currentText().toInt();
	preset.iSampleRate  = m_ui.SampleRateComboBox->currentText().toInt();
	preset.iPeriods     = m_ui.PeriodsSpinBox->value();
	preset.iWordLength  = m_ui.WordLengthComboBox->currentText().toInt();
	preset.iWait        = m_ui.WaitComboBox->currentText().toInt();
	preset.iChan        = m_ui.ChanSpinBox->value();
	preset.sDriver      = m_ui.DriverComboBox->currentText();
	preset.sInterface   = m_ui.InterfaceComboBox->currentText();
	preset.iAudio       = m_ui.AudioComboBox->currentIndex();
	preset.iDither      = m_ui.DitherComboBox->currentIndex();
	preset.iTimeout     = m_ui.TimeoutComboBox->currentText().toInt();
	preset.sInDevice    = m_ui.InDeviceComboBox->currentText();
	preset.sOutDevice   = m_ui.OutDeviceComboBox->currentText();
	preset.iInChannels  = m_ui.InChannelsSpinBox->value();
	preset.iOutChannels = m_ui.OutChannelsSpinBox->value();
	preset.iInLatency   = m_ui.InLatencySpinBox->value();
	preset.iOutLatency  = m_ui.OutLatencySpinBox->value();
	preset.iStartDelay  = m_ui.StartDelaySpinBox->value();
	preset.bVerbose     = m_ui.VerboseCheckBox->isChecked();
	preset.iPortMax     = m_ui.PortMaxComboBox->currentText().toInt();
#ifdef CONFIG_JACK_MIDI
	preset.sMidiDriver  = m_ui.MidiDriverComboBox->currentText();
#endif
	preset.sServerSuffix = m_ui.ServerSuffixComboBox->currentText();
	if (preset.sServerName == m_pSetup->sDefPresetName)
		preset.sServerName.clear();
	if (preset.sInterface == m_pSetup->sDefPresetName)
		preset.sInterface.clear();
	if (preset.sInDevice == m_pSetup->sDefPresetName)
		preset.sInDevice.clear();
	if (preset.sOutDevice == m_pSetup->sDefPresetName)
		preset.sOutDevice.clear();
	m_pSetup->savePreset(preset, sPreset);

	return true;
}


bool qjackctlSetupForm::deletePreset ( const QString& sPreset )
{
	if (sPreset.isEmpty())
		return false;

	// Just remove the preset item...
	m_pSetup->deletePreset(sPreset);

	return true;
}


void qjackctlSetupForm::resetPresets (void)
{
	m_ui.PresetComboBox->clear();
	m_ui.PresetComboBox->addItems(m_pSetup->presets);
	m_ui.PresetComboBox->addItem(m_pSetup->sDefPresetName);
}


void qjackctlSetupForm::changeCurrentPreset ( const QString& sPreset )
{
	if (m_iDirtySetup > 0)
		return;

	// Check if there's any pending changes...
	if (m_iDirtySettings > 0 && !m_sPreset.isEmpty()) {
		switch (QMessageBox::warning(this,
			tr("Warning") + " - " QJACKCTL_SUBTITLE1,
			tr("Some settings have been changed:\n\n"
			"\"%1\"\n\nDo you want to save the changes?")
			.arg(m_sPreset),
			QMessageBox::Save |
			QMessageBox::Discard |
			QMessageBox::Cancel)) {
		case QMessageBox::Save:
			savePreset(m_sPreset);
			m_iDirtySetup++;
			resetPresets();
			setComboBoxCurrentText(m_ui.PresetComboBox, sPreset);
			m_iDirtySetup--;
		case QMessageBox::Discard:
			m_iDirtySettings = 0;
			break;
		default: // Cancel...
			m_iDirtySetup++;
			resetPresets();
			setComboBoxCurrentText(m_ui.PresetComboBox, m_sPreset);
			m_iDirtySetup--;
			return;
		}
	}

	changePreset(sPreset);
	optionsChanged();
}

void qjackctlSetupForm::saveCurrentPreset (void)
{
	const QString sPreset = m_ui.PresetComboBox->currentText();

	if (savePreset(sPreset)) {
		// Reset preset combobox list.
		m_iDirtySetup++;
		resetPresets();
		setComboBoxCurrentText(m_ui.PresetComboBox, sPreset);
		m_iDirtySetup--;
		// Reset dirty flag.
		m_iDirtySettings = 0;
		stabilizeForm();
	}
}


void qjackctlSetupForm::deleteCurrentPreset (void)
{
	const QString sPreset = m_ui.PresetComboBox->currentText();

	// Try to prompt user if he/she really wants this...
	if (QMessageBox::warning(this,
		tr("Warning") + " - " QJACKCTL_SUBTITLE1,
		tr("Delete preset:\n\n"
		"\"%1\"\n\nAre you sure?")
		.arg(sPreset),
		QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
		return;

	if (deletePreset(sPreset)) {
		// Reset preset combobox list,
		// and load a new available preset..
		m_iDirtySetup++;
		int iItem = m_ui.PresetComboBox->currentIndex();
		resetPresets();
		m_ui.PresetComboBox->setCurrentIndex(iItem);
		changePreset(m_ui.PresetComboBox->currentText());
		m_iDirtySetup--;
		// Take care that maybe it was the default one...
		if (m_pSetup->sDefPreset == sPreset)
			m_pSetup->sDefPreset = m_sPreset;
		// Make this stable now.
		optionsChanged();
	}
}


void qjackctlSetupForm::computeLatency (void)
{
	float lat = 0.0;
	int p = m_ui.FramesComboBox->currentText().toInt();
	int r = m_ui.SampleRateComboBox->currentText().toInt();
	int n = m_ui.PeriodsSpinBox->value();
	if (r > 0)
		lat = (float) (1000.0 * p * n) / (float) r;
	if (lat > 0.0)
		m_ui.LatencyTextValue->setText(QString::number(lat, 'g', 3) + " " + tr("msec"));
	else
		m_ui.LatencyTextValue->setText(tr("n/a"));
}


void qjackctlSetupForm::changeDriverAudio ( const QString& sDriver, int iAudio )
{
	bool bSun        = (sDriver == "sun");
	bool bOss        = (sDriver == "oss");
	bool bAlsa       = (sDriver == "alsa");
	bool bCoreaudio  = (sDriver == "coreaudio");
	bool bPortaudio  = (sDriver == "portaudio");
	bool bFreebob    = (sDriver == "freebob");
	bool bFirewire   = (sDriver == "firewire");
	bool bNet        = (sDriver == "net" || sDriver == "netone");
	bool bInEnabled  = false;
	bool bOutEnabled = false;
	bool bEnabled;

	switch (iAudio) {
	case QJACKCTL_DUPLEX:
		bInEnabled  = (bSun || bOss || bAlsa || bCoreaudio || bPortaudio || bNet);
		bOutEnabled = (bSun || bOss || bAlsa || bCoreaudio || bPortaudio || bNet);
		break;
	case QJACKCTL_CAPTURE:
		bInEnabled  = (bSun || bOss || bCoreaudio || bPortaudio || bNet);
		break;
	case QJACKCTL_PLAYBACK:
		bOutEnabled = (bSun || bOss || bCoreaudio || bPortaudio || bNet);
		break;
	}

	bEnabled = (bInEnabled && (bAlsa || bSun || bOss || bPortaudio));
	m_ui.InDeviceTextLabel->setEnabled(bEnabled);
	m_ui.InDeviceComboBox->setEnabled(bEnabled);
	if (!bEnabled)
		setComboBoxCurrentText(m_ui.InDeviceComboBox, m_pSetup->sDefPresetName);

	bEnabled = (bOutEnabled && (bAlsa || bSun || bOss || bPortaudio));
	m_ui.OutDeviceTextLabel->setEnabled(bEnabled);
	m_ui.OutDeviceComboBox->setEnabled(bEnabled);
	if (!bEnabled)
		setComboBoxCurrentText(m_ui.OutDeviceComboBox, m_pSetup->sDefPresetName);

	m_ui.InOutChannelsTextLabel->setEnabled(bInEnabled || (bAlsa || bFirewire));
	m_ui.InChannelsSpinBox->setEnabled(bInEnabled
		|| ((bAlsa || bFirewire) && iAudio != QJACKCTL_PLAYBACK));
	m_ui.OutChannelsSpinBox->setEnabled(bOutEnabled
		|| ((bAlsa || bFirewire) && iAudio != QJACKCTL_CAPTURE));

	m_ui.InOutLatencyTextLabel->setEnabled((bInEnabled && !bNet)
		|| (bAlsa || bFreebob || bFirewire));
	m_ui.InLatencySpinBox->setEnabled((bInEnabled && !bNet)
		|| ((bAlsa || bFreebob || bFirewire) && iAudio != QJACKCTL_PLAYBACK));
	m_ui.OutLatencySpinBox->setEnabled((bOutEnabled && !bNet)
		|| ((bAlsa || bFreebob || bFirewire) && iAudio != QJACKCTL_CAPTURE));

	computeLatency();
}


void qjackctlSetupForm::changeAudio ( int iAudio )
{
	changeDriverAudio(m_ui.DriverComboBox->currentText(), iAudio);
}


void qjackctlSetupForm::changeDriver ( int iDriver )
{
	changeDriverUpdate(m_ui.DriverComboBox->itemText(iDriver), true);
}


void qjackctlSetupForm::changeDriverUpdate ( const QString& sDriver, bool bUpdate )
{
	bool bDummy     = (sDriver == "dummy");
	bool bSun       = (sDriver == "sun");
	bool bOss       = (sDriver == "oss");
	bool bAlsa      = (sDriver == "alsa");
	bool bPortaudio = (sDriver == "portaudio");
	bool bCoreaudio = (sDriver == "coreaudio");
	bool bFreebob   = (sDriver == "freebob");
	bool bFirewire  = (sDriver == "firewire");
	bool bNet       = (sDriver == "net" || sDriver == "netone");

	m_ui.NoMemLockCheckBox->setEnabled(!bCoreaudio);
	m_ui.UnlockMemCheckBox->setEnabled(!bCoreaudio
		&& !m_ui.NoMemLockCheckBox->isChecked());

	m_ui.SoftModeCheckBox->setEnabled(bAlsa);
	m_ui.MonitorCheckBox->setEnabled(bAlsa);
	m_ui.ShortsCheckBox->setEnabled(bAlsa);
	m_ui.HWMonCheckBox->setEnabled(bAlsa);
	m_ui.HWMeterCheckBox->setEnabled(bAlsa);

	m_ui.IgnoreHWCheckBox->setEnabled(bSun || bOss);

	if (bCoreaudio || bPortaudio) {
		m_ui.PriorityTextLabel->setEnabled(false);
		m_ui.PrioritySpinBox->setEnabled(false);
	} else {
		bool bPriorityEnabled = m_ui.RealtimeCheckBox->isChecked();
		m_ui.PriorityTextLabel->setEnabled(bPriorityEnabled);
		m_ui.PrioritySpinBox->setEnabled(bPriorityEnabled);
	}

	m_ui.SampleRateTextLabel->setEnabled(!bNet);
	m_ui.SampleRateComboBox->setEnabled(!bNet);

	m_ui.FramesTextLabel->setEnabled(!bNet);
	m_ui.FramesComboBox->setEnabled(!bNet);
	
	m_ui.PeriodsTextLabel->setEnabled(bAlsa || bSun || bOss || bFreebob || bFirewire);
	m_ui.PeriodsSpinBox->setEnabled(bAlsa || bSun || bOss || bFreebob || bFirewire);

	if (bUpdate && (bFreebob || bFirewire) && m_ui.PeriodsSpinBox->value() < 3)
		m_ui.PeriodsSpinBox->setValue(3);

	m_ui.WordLengthTextLabel->setEnabled(bSun || bOss);
	m_ui.WordLengthComboBox->setEnabled(bSun || bOss);

	m_ui.WaitTextLabel->setEnabled(bDummy);
	m_ui.WaitComboBox->setEnabled(bDummy);

	m_ui.ChanTextLabel->setEnabled(bPortaudio);
	m_ui.ChanSpinBox->setEnabled(bPortaudio);

	int  iAudio   = m_ui.AudioComboBox->currentIndex();
	bool bEnabled = (bAlsa || bPortaudio);
	if (bEnabled && iAudio == QJACKCTL_DUPLEX) {
		const QString& sInDevice  = m_ui.InDeviceComboBox->currentText();
		const QString& sOutDevice = m_ui.OutDeviceComboBox->currentText();
		bEnabled = (sInDevice.isEmpty()  || sInDevice  == m_pSetup->sDefPresetName ||
					sOutDevice.isEmpty() || sOutDevice == m_pSetup->sDefPresetName);
	}

	bool bInterface = (bEnabled || bCoreaudio || bFreebob || bFirewire);
	m_ui.InterfaceTextLabel->setEnabled(bInterface);
	m_ui.InterfaceComboBox->setEnabled(bInterface);
	if (!bInterface)
		setComboBoxCurrentText(m_ui.InterfaceComboBox, m_pSetup->sDefPresetName);

	m_ui.DitherTextLabel->setEnabled(bAlsa || bPortaudio);
	m_ui.DitherComboBox->setEnabled(bAlsa || bPortaudio);

#ifdef CONFIG_JACK_MIDI
	m_ui.MidiDriverTextLabel->setEnabled(bAlsa);
	m_ui.MidiDriverComboBox->setEnabled(bAlsa);
#endif

	changeDriverAudio(sDriver, iAudio);
}


// Stabilize current form state.
void qjackctlSetupForm::stabilizeForm (void)
{
	bool bValid = (m_iDirtySettings > 0 || m_iDirtyOptions > 0);

	QString sPreset = m_ui.PresetComboBox->currentText();
	if (!sPreset.isEmpty()) {
		const bool bPreset = (m_pSetup->presets.contains(sPreset));
		m_ui.PresetSavePushButton->setEnabled(m_iDirtySettings > 0
			|| (!bPreset && sPreset != m_pSetup->sDefPresetName));
		m_ui.PresetDeletePushButton->setEnabled(bPreset);
	} else {
		m_ui.PresetSavePushButton->setEnabled(false);
		m_ui.PresetDeletePushButton->setEnabled(false);
	}

	bool bEnabled = m_ui.StartupScriptCheckBox->isChecked();
	m_ui.StartupScriptShellComboBox->setEnabled(bEnabled);
	m_ui.StartupScriptSymbolToolButton->setEnabled(bEnabled);
	m_ui.StartupScriptBrowseToolButton->setEnabled(bEnabled);

	bEnabled = m_ui.PostStartupScriptCheckBox->isChecked();
	m_ui.PostStartupScriptShellComboBox->setEnabled(bEnabled);
	m_ui.PostStartupScriptSymbolToolButton->setEnabled(bEnabled);
	m_ui.PostStartupScriptBrowseToolButton->setEnabled(bEnabled);

	bEnabled = m_ui.ShutdownScriptCheckBox->isChecked();
	m_ui.ShutdownScriptShellComboBox->setEnabled(bEnabled);
	m_ui.ShutdownScriptSymbolToolButton->setEnabled(bEnabled);
	m_ui.ShutdownScriptBrowseToolButton->setEnabled(bEnabled);

	bEnabled = m_ui.PostShutdownScriptCheckBox->isChecked();
	m_ui.PostShutdownScriptShellComboBox->setEnabled(bEnabled);
	m_ui.PostShutdownScriptSymbolToolButton->setEnabled(bEnabled);
	m_ui.PostShutdownScriptBrowseToolButton->setEnabled(bEnabled);

	bEnabled = m_ui.StdoutCaptureCheckBox->isChecked();
	m_ui.XrunRegexTextLabel->setEnabled(bEnabled);
	m_ui.XrunRegexComboBox->setEnabled(bEnabled);

	bEnabled = m_ui.ActivePatchbayCheckBox->isChecked();
	m_ui.ActivePatchbayPathComboBox->setEnabled(bEnabled);
	m_ui.ActivePatchbayPathToolButton->setEnabled(bEnabled);
	m_ui.ActivePatchbayResetCheckBox->setEnabled(bEnabled);
	m_ui.QueryDisconnectCheckBox->setEnabled(bEnabled);
	if (bEnabled && bValid) {
		const QString& sPath = m_ui.ActivePatchbayPathComboBox->currentText();
		bValid = (!sPath.isEmpty() && QFileInfo(sPath).exists());
	}

	bEnabled = m_ui.MessagesLogCheckBox->isChecked();
	m_ui.MessagesLogPathComboBox->setEnabled(bEnabled);
	m_ui.MessagesLogPathToolButton->setEnabled(bEnabled);
	if (bEnabled && bValid) {
		const QString& sPath = m_ui.MessagesLogPathComboBox->currentText();
		bValid = !sPath.isEmpty();
	}

	m_ui.MessagesLimitLinesComboBox->setEnabled(
		m_ui.MessagesLimitCheckBox->isChecked());

#ifdef CONFIG_JACK_METADATA
#ifdef CONFIG_JACK_PORT_ALIASES
	bEnabled = !m_ui.JackClientPortMetadataCheckBox->isChecked();
	m_ui.JackClientPortAliasTextLabel->setEnabled(bEnabled);
	m_ui.JackClientPortAliasComboBox->setEnabled(bEnabled);
	if (bEnabled) {
		m_ui.JackClientPortMetadataCheckBox->setEnabled(
			m_ui.JackClientPortAliasComboBox->currentIndex() == 0);
	}
#endif
#endif

#ifdef CONFIG_SYSTEM_TRAY
	m_ui.SystemTrayQueryCloseCheckBox->setEnabled(
		m_ui.SystemTrayCheckBox->isChecked());
#endif

	m_ui.StopJackCheckBox->setEnabled(
		m_ui.DBusEnabledCheckBox->isChecked());
	if (!m_ui.DBusEnabledCheckBox->isChecked()) {
		m_ui.StopJackCheckBox->setChecked(true);
	}

	bEnabled = m_ui.ServerConfigCheckBox->isChecked();
	m_ui.ServerConfigNameComboBox->setEnabled(bEnabled);
	m_ui.ServerConfigTempCheckBox->setEnabled(bEnabled);

	m_ui.AliasesEditingCheckBox->setEnabled(
		m_ui.AliasesEnabledCheckBox->isChecked());

#ifndef CONFIG_XUNIQUE
	m_ui.SingletonCheckBox->setEnabled(false);
#endif

	m_ui.TransportButtonsCheckBox->setEnabled(
		m_ui.LeftButtonsCheckBox->isChecked());

	changeDriverUpdate(m_ui.DriverComboBox->currentText(), false);

	m_ui.DialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(bValid);
}


// Meta-symbol menu executive.
void qjackctlSetupForm::symbolMenu( QLineEdit *pLineEdit,
	QToolButton *pToolButton )
{
	const QString s = "  ";

	QMenu menu(this);

	menu.addAction("%P" + s + tr("&Preset Name"));
	menu.addSeparator();
	menu.addAction("%N" + s + tr("&Server Name"));
	menu.addAction("%s" + s + tr("&Server Path"));
	menu.addAction("%d" + s + tr("&Driver"));
	menu.addAction("%i" + s + tr("&Interface"));
	menu.addSeparator();
	menu.addAction("%r" + s + tr("Sample &Rate"));
	menu.addAction("%p" + s + tr("&Frames/Period"));
	menu.addAction("%n" + s + tr("Periods/&Buffer"));

	QAction *pAction = menu.exec(pToolButton->mapToGlobal(QPoint(0,0)));
	if (pAction) {
		const QString sText = pAction->text();
		int iMetaChar = sText.indexOf('%');
		if (iMetaChar >= 0) {
			pLineEdit->insert('%' + sText[iMetaChar + 1]);
		//  optionsChanged();
		}
	}
}


// Startup script meta-symbol button slot.
void qjackctlSetupForm::symbolStartupScript (void)
{
	symbolMenu(m_ui.StartupScriptShellComboBox->lineEdit(),
		m_ui.StartupScriptSymbolToolButton);
}

// Post-startup script meta-symbol button slot.
void qjackctlSetupForm::symbolPostStartupScript (void)
{
	symbolMenu(m_ui.PostStartupScriptShellComboBox->lineEdit(),
		m_ui.PostStartupScriptSymbolToolButton);
}

// Shutdown script meta-symbol button slot.
void qjackctlSetupForm::symbolShutdownScript (void)
{
	symbolMenu(m_ui.ShutdownScriptShellComboBox->lineEdit(),
		m_ui.ShutdownScriptSymbolToolButton);
}

// Post-shutdown script meta-symbol button slot.
void qjackctlSetupForm::symbolPostShutdownScript (void)
{
	symbolMenu(m_ui.PostShutdownScriptShellComboBox->lineEdit(),
		m_ui.PostShutdownScriptSymbolToolButton);
}


// Startup script browse slot.
void qjackctlSetupForm::browseStartupScript()
{
	QString sFileName = QFileDialog::getOpenFileName(
		this,												// Parent.
		tr("Startup Script"),								// Caption.
		m_ui.StartupScriptShellComboBox->currentText() 		// Start here.
	);

	if (!sFileName.isEmpty()) {
		setComboBoxCurrentText(m_ui.StartupScriptShellComboBox, sFileName);
		m_ui.StartupScriptShellComboBox->setFocus();
		optionsChanged();
	}
}


// Post-startup script browse slot.
void qjackctlSetupForm::browsePostStartupScript()
{
	QString sFileName = QFileDialog::getOpenFileName(
		this,												// Parent.
		tr("Post-Startup Script"),							// Caption.
		m_ui.PostStartupScriptShellComboBox->currentText() 	// Start here.
	);

	if (!sFileName.isEmpty()) {
		setComboBoxCurrentText(m_ui.PostStartupScriptShellComboBox, sFileName);
		m_ui.PostStartupScriptShellComboBox->setFocus();
		optionsChanged();
	}
}


// Shutdown script browse slot.
void qjackctlSetupForm::browseShutdownScript()
{
	QString sFileName = QFileDialog::getOpenFileName(
		this,												// Parent.
		tr("Shutdown Script"),								// Caption.
		m_ui.ShutdownScriptShellComboBox->currentText() 	// Start here.
	);

	if (!sFileName.isEmpty()) {
		setComboBoxCurrentText(m_ui.ShutdownScriptShellComboBox, sFileName);
		m_ui.ShutdownScriptShellComboBox->setFocus();
		optionsChanged();
	}
}


// Post-shutdown script browse slot.
void qjackctlSetupForm::browsePostShutdownScript()
{
	QString sFileName = QFileDialog::getOpenFileName(
		this,												// Parent.
		tr("Post-Shutdown Script"),							// Caption.
		m_ui.PostShutdownScriptShellComboBox->currentText() // Start here.
	);

	if (!sFileName.isEmpty()) {
		setComboBoxCurrentText(m_ui.PostShutdownScriptShellComboBox, sFileName);
		m_ui.PostShutdownScriptShellComboBox->setFocus();
		optionsChanged();
	}
}


// Active Patchbay path browse slot.
void qjackctlSetupForm::browseActivePatchbayPath()
{
	QString sFileName = QFileDialog::getOpenFileName(
		this,											// Parent.
		tr("Active Patchbay Definition"),				// Caption.
		m_ui.ActivePatchbayPathComboBox->currentText(),	// Start here.
		tr("Patchbay Definition files") + " (*.xml)"	// Filter (XML files)
	);

	if (!sFileName.isEmpty()) {
		setComboBoxCurrentText(m_ui.ActivePatchbayPathComboBox, sFileName);
		m_ui.ActivePatchbayPathComboBox->setFocus();
		optionsChanged();
	}
}


// Messages log path browse slot.
void qjackctlSetupForm::browseMessagesLogPath()
{
	QString sFileName = QFileDialog::getSaveFileName(
		this,											// Parent.
		tr("Messages Log"),				                // Caption.
		m_ui.MessagesLogPathComboBox->currentText(),	// Start here.
		tr("Log files") + " (*.log)"	                // Filter (log files)
	);

	if (!sFileName.isEmpty()) {
		setComboBoxCurrentText(m_ui.MessagesLogPathComboBox, sFileName);
		m_ui.MessagesLogPathComboBox->setFocus();
		optionsChanged();
	}
}


// The display font 1 (big time) selection dialog.
void qjackctlSetupForm::chooseDisplayFont1()
{
	bool  bOk  = false;
	QFont font = QFontDialog::getFont(&bOk,
		m_ui.DisplayFont1TextLabel->font(), this);
	if (bOk) {
		m_ui.DisplayFont1TextLabel->setFont(font);
		m_ui.DisplayFont1TextLabel->setText(font.family()
			+ ' ' + QString::number(font.pointSize()));
		optionsChanged();
	}
}


// The display font 2 (normal time et al.) selection dialog.
void qjackctlSetupForm::chooseDisplayFont2()
{
	bool  bOk  = false;
	QFont font = QFontDialog::getFont(&bOk,
		m_ui.DisplayFont2TextLabel->font(), this);
	if (bOk) {
		m_ui.DisplayFont2TextLabel->setFont(font);
		m_ui.DisplayFont2TextLabel->setText(font.family()
			+ ' ' + QString::number(font.pointSize()));
		optionsChanged();
	}
}


// The channel display effect demo changer.
void qjackctlSetupForm::toggleDisplayEffect ( bool bOn )
{
	QPalette pal;
	pal.setColor(QPalette::Foreground, Qt::green);
	if (bOn) {
		QPixmap pm(":/images/displaybg1.png");
		pal.setBrush(QPalette::Background, QBrush(pm));
	} else {
		pal.setColor(QPalette::Background, Qt::black);
	}
	m_ui.DisplayFont1TextLabel->setPalette(pal);
	m_ui.DisplayFont2TextLabel->setPalette(pal);
	optionsChanged();
}


// The messages font selection dialog.
void qjackctlSetupForm::chooseMessagesFont (void)
{
	bool  bOk  = false;
	QFont font = QFontDialog::getFont(&bOk,
		m_ui.MessagesFontTextLabel->font(), this);
	if (bOk) {
		m_ui.MessagesFontTextLabel->setFont(font);
		m_ui.MessagesFontTextLabel->setText(font.family()
			+ ' ' + QString::number(font.pointSize()));
		optionsChanged();
	}
}


// The connections font selection dialog.
void qjackctlSetupForm::chooseConnectionsFont (void)
{
	bool  bOk  = false;
	QFont font = QFontDialog::getFont(&bOk,
		m_ui.ConnectionsFontTextLabel->font(), this);
	if (bOk) {
		m_ui.ConnectionsFontTextLabel->setFont(font);
		m_ui.ConnectionsFontTextLabel->setText(font.family()
			+ ' ' + QString::number(font.pointSize()));
		optionsChanged();
	}
}


// Mark that some server preset settings have changed.
void qjackctlSetupForm::settingsChanged (void)
{
	if (m_iDirtySetup > 0)
		return;

	m_iDirtySettings++;
	stabilizeForm();
}

// Mark that some program options have changed.
void qjackctlSetupForm::optionsChanged (void)
{
	if (m_iDirtySetup > 0)
		return;

	m_iDirtyOptions++;
	stabilizeForm();
}


// Accept settings (OK button slot).
void qjackctlSetupForm::accept (void)
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	if (m_iDirtySettings > 0 || m_iDirtyOptions > 0) {
		// To track down deferred or immediate changes.
		const bool    bOldMessagesLog         = m_pSetup->bMessagesLog;
		const QString sOldMessagesLogPath     = m_pSetup->sMessagesLogPath;
		const QString sOldMessagesFont        = m_pSetup->sMessagesFont;
		const QString sOldDisplayFont1        = m_pSetup->sDisplayFont1;
		const QString sOldDisplayFont2        = m_pSetup->sDisplayFont2;
		const QString sOldConnectionsFont     = m_pSetup->sConnectionsFont;
		const int     iOldConnectionsIconSize = m_pSetup->iConnectionsIconSize;
		const int     iOldJackClientPortAlias = m_pSetup->iJackClientPortAlias;
		const bool    bOldJackClientPortMetadata = m_pSetup->bJackClientPortMetadata;
		const int     iOldTimeDisplay         = m_pSetup->iTimeDisplay;
		const int     iOldTimeFormat          = m_pSetup->iTimeFormat;
		const bool    bOldDisplayEffect       = m_pSetup->bDisplayEffect;
		const bool    bOldActivePatchbay      = m_pSetup->bActivePatchbay;
		const QString sOldActivePatchbayPath  = m_pSetup->sActivePatchbayPath;
		const bool    bOldStdoutCapture       = m_pSetup->bStdoutCapture;
		const bool    bOldKeepOnTop           = m_pSetup->bKeepOnTop;
	#ifdef CONFIG_SYSTEM_TRAY
		const bool    bOldSystemTray          = m_pSetup->bSystemTray;
	#endif
		const int     bOldMessagesLimit       = m_pSetup->bMessagesLimit;
		const int     iOldMessagesLimitLines  = m_pSetup->iMessagesLimitLines;
		const bool    bOldBezierLines         = m_pSetup->bBezierLines;
		const bool    bOldAlsaSeqEnabled      = m_pSetup->bAlsaSeqEnabled;
		const bool    bOldDBusEnabled         = m_pSetup->bDBusEnabled;
		const bool    bOldAliasesEnabled      = m_pSetup->bAliasesEnabled;
		const bool    bOldAliasesEditing      = m_pSetup->bAliasesEditing;
		const bool    bOldLeftButtons         = m_pSetup->bLeftButtons;
		const bool    bOldRightButtons        = m_pSetup->bRightButtons;
		const bool    bOldTransportButtons    = m_pSetup->bTransportButtons;
		const bool    bOldTextLabels          = m_pSetup->bTextLabels;
		const int     iOldBaseFontSize        = m_pSetup->iBaseFontSize;
		// Save current preset selection.
		m_pSetup->sDefPreset = m_ui.PresetComboBox->currentText();
		// Always save current settings...
		savePreset(m_pSetup->sDefPreset);
		// Save Options...
		m_pSetup->bStartupScript           = m_ui.StartupScriptCheckBox->isChecked();
		m_pSetup->sStartupScriptShell      = m_ui.StartupScriptShellComboBox->currentText();
		m_pSetup->bPostStartupScript       = m_ui.PostStartupScriptCheckBox->isChecked();
		m_pSetup->sPostStartupScriptShell  = m_ui.PostStartupScriptShellComboBox->currentText();
		m_pSetup->bShutdownScript          = m_ui.ShutdownScriptCheckBox->isChecked();
		m_pSetup->sShutdownScriptShell     = m_ui.ShutdownScriptShellComboBox->currentText();
		m_pSetup->bPostShutdownScript      = m_ui.PostShutdownScriptCheckBox->isChecked();
		m_pSetup->sPostShutdownScriptShell = m_ui.PostShutdownScriptShellComboBox->currentText();
		m_pSetup->bStdoutCapture           = m_ui.StdoutCaptureCheckBox->isChecked();
		m_pSetup->sXrunRegex               = m_ui.XrunRegexComboBox->currentText();
		m_pSetup->bActivePatchbay          = m_ui.ActivePatchbayCheckBox->isChecked();
		m_pSetup->sActivePatchbayPath      = m_ui.ActivePatchbayPathComboBox->currentText();
		m_pSetup->bActivePatchbayReset     = m_ui.ActivePatchbayResetCheckBox->isChecked();
		m_pSetup->bQueryDisconnect         = m_ui.QueryDisconnectCheckBox->isChecked();
		m_pSetup->bMessagesLog             = m_ui.MessagesLogCheckBox->isChecked();
		m_pSetup->sMessagesLogPath         = m_ui.MessagesLogPathComboBox->currentText();
		m_pSetup->bBezierLines             = m_ui.BezierLinesCheckBox->isChecked();
		// Save Defaults...
		m_pSetup->iTimeDisplay             = m_pTimeDisplayButtonGroup->checkedId();
		m_pSetup->iTimeFormat              = m_ui.TimeFormatComboBox->currentIndex();
		m_pSetup->sMessagesFont            = m_ui.MessagesFontTextLabel->font().toString();
		m_pSetup->bMessagesLimit           = m_ui.MessagesLimitCheckBox->isChecked();
		m_pSetup->iMessagesLimitLines      = m_ui.MessagesLimitLinesComboBox->currentText().toInt();
		m_pSetup->sDisplayFont1            = m_ui.DisplayFont1TextLabel->font().toString();
		m_pSetup->sDisplayFont2            = m_ui.DisplayFont2TextLabel->font().toString();
		m_pSetup->bDisplayEffect           = m_ui.DisplayEffectCheckBox->isChecked();
		m_pSetup->bDisplayBlink            = m_ui.DisplayBlinkCheckBox->isChecked();
		m_pSetup->iJackClientPortAlias     = m_ui.JackClientPortAliasComboBox->currentIndex();
		m_pSetup->bJackClientPortMetadata  = m_ui.JackClientPortMetadataCheckBox->isChecked();
		m_pSetup->iConnectionsIconSize     = m_ui.ConnectionsIconSizeComboBox->currentIndex();
		m_pSetup->sConnectionsFont         = m_ui.ConnectionsFontTextLabel->font().toString();
		m_pSetup->bStartJack               = m_ui.StartJackCheckBox->isChecked();
		m_pSetup->bStopJack                = m_ui.StopJackCheckBox->isChecked();
		m_pSetup->bQueryClose              = m_ui.QueryCloseCheckBox->isChecked();
		m_pSetup->bQueryShutdown           = m_ui.QueryShutdownCheckBox->isChecked();
		m_pSetup->bKeepOnTop               = m_ui.KeepOnTopCheckBox->isChecked();
	#ifdef CONFIG_SYSTEM_TRAY
		m_pSetup->bSystemTray              = m_ui.SystemTrayCheckBox->isChecked();
		m_pSetup->bSystemTrayQueryClose    = m_ui.SystemTrayQueryCloseCheckBox->isChecked();
	#endif
		m_pSetup->bSingleton               = m_ui.SingletonCheckBox->isChecked();
		m_pSetup->bServerConfig            = m_ui.ServerConfigCheckBox->isChecked();
		m_pSetup->sServerConfigName        = m_ui.ServerConfigNameComboBox->currentText();
		m_pSetup->bServerConfigTemp        = m_ui.ServerConfigTempCheckBox->isChecked();
		m_pSetup->bAlsaSeqEnabled          = m_ui.AlsaSeqEnabledCheckBox->isChecked();
		m_pSetup->bDBusEnabled             = m_ui.DBusEnabledCheckBox->isChecked();
		m_pSetup->bAliasesEnabled          = m_ui.AliasesEnabledCheckBox->isChecked();
		m_pSetup->bAliasesEditing          = m_ui.AliasesEditingCheckBox->isChecked();
		m_pSetup->bLeftButtons             = !m_ui.LeftButtonsCheckBox->isChecked();
		m_pSetup->bRightButtons            = !m_ui.RightButtonsCheckBox->isChecked();
		m_pSetup->bTransportButtons        = !m_ui.TransportButtonsCheckBox->isChecked();
		m_pSetup->bTextLabels              = !m_ui.TextLabelsCheckBox->isChecked();
		m_pSetup->iBaseFontSize            = m_ui.BaseFontSizeComboBox->currentText().toInt();
		// Check wheather something immediate has changed.
		if (( bOldMessagesLog && !m_pSetup->bMessagesLog) ||
			(!bOldMessagesLog &&  m_pSetup->bMessagesLog) ||
			(sOldMessagesLogPath != m_pSetup->sMessagesLogPath))
			pMainForm->updateMessagesLogging();
		if (( bOldBezierLines && !m_pSetup->bBezierLines) ||
			(!bOldBezierLines &&  m_pSetup->bBezierLines))
			pMainForm->updateBezierLines();
		if (( bOldDisplayEffect && !m_pSetup->bDisplayEffect) ||
			(!bOldDisplayEffect &&  m_pSetup->bDisplayEffect))
			pMainForm->updateDisplayEffect();
		if (iOldJackClientPortAlias != m_pSetup->iJackClientPortAlias)
			pMainForm->updateJackClientPortAlias();
		if (( bOldJackClientPortMetadata && !m_pSetup->bJackClientPortMetadata) ||
			(!bOldJackClientPortMetadata &&  m_pSetup->bJackClientPortMetadata))
			pMainForm->updateJackClientPortMetadata();
		if (iOldConnectionsIconSize != m_pSetup->iConnectionsIconSize)
			pMainForm->updateConnectionsIconSize();
		if (sOldConnectionsFont != m_pSetup->sConnectionsFont)
			pMainForm->updateConnectionsFont();
		if (sOldMessagesFont != m_pSetup->sMessagesFont)
			pMainForm->updateMessagesFont();
		if (( bOldMessagesLimit && !m_pSetup->bMessagesLimit) ||
			(!bOldMessagesLimit &&  m_pSetup->bMessagesLimit) ||
			(iOldMessagesLimitLines !=  m_pSetup->iMessagesLimitLines))
			pMainForm->updateMessagesLimit();
		if (sOldDisplayFont1 != m_pSetup->sDisplayFont1 ||
			sOldDisplayFont2 != m_pSetup->sDisplayFont2)
			pMainForm->updateTimeDisplayFonts();
		if (iOldTimeDisplay != m_pSetup->iTimeDisplay)
			pMainForm->updateTimeDisplayToolTips();
		if (iOldTimeFormat != m_pSetup->iTimeFormat)
			pMainForm->updateTimeFormat();
		if ((!bOldActivePatchbay && m_pSetup->bActivePatchbay) ||
			(sOldActivePatchbayPath != m_pSetup->sActivePatchbayPath))
			pMainForm->updateActivePatchbay();
	#ifdef CONFIG_SYSTEM_TRAY
		if (( bOldSystemTray && !m_pSetup->bSystemTray) ||
			(!bOldSystemTray &&  m_pSetup->bSystemTray))
			pMainForm->updateSystemTray();
	#endif
		if (( bOldAliasesEnabled && !m_pSetup->bAliasesEnabled) ||
			(!bOldAliasesEnabled &&  m_pSetup->bAliasesEnabled) ||
			( bOldAliasesEditing && !m_pSetup->bAliasesEditing) ||
			(!bOldAliasesEditing &&  m_pSetup->bAliasesEditing))
			pMainForm->updateAliases();
		if (( bOldLeftButtons  && !m_pSetup->bLeftButtons)  ||
			(!bOldLeftButtons  &&  m_pSetup->bLeftButtons)  ||
			( bOldRightButtons && !m_pSetup->bRightButtons) ||
			(!bOldRightButtons &&  m_pSetup->bRightButtons) ||
			( bOldTransportButtons && !m_pSetup->bTransportButtons) ||
			(!bOldTransportButtons &&  m_pSetup->bTransportButtons) ||
			( bOldTextLabels && !m_pSetup->bTextLabels) ||
			(!bOldTextLabels &&  m_pSetup->bTextLabels))
			pMainForm->updateButtons();
		// Warn if something will be only effective on next run.
		if (( bOldStdoutCapture  && !m_pSetup->bStdoutCapture)  ||
			(!bOldStdoutCapture  &&  m_pSetup->bStdoutCapture)  ||
			( bOldKeepOnTop      && !m_pSetup->bKeepOnTop)      ||
			(!bOldKeepOnTop      &&  m_pSetup->bKeepOnTop)      ||
			( bOldAlsaSeqEnabled && !m_pSetup->bAlsaSeqEnabled) ||
			(!bOldAlsaSeqEnabled &&  m_pSetup->bAlsaSeqEnabled) ||
			( bOldDBusEnabled    && !m_pSetup->bDBusEnabled)    ||
			(!bOldDBusEnabled    &&  m_pSetup->bDBusEnabled)    ||
			(iOldBaseFontSize    !=  m_pSetup->iBaseFontSize))
			pMainForm->showDirtySetupWarning();
		// If server is currently running, warn user...
		pMainForm->showDirtySettingsWarning();
	}

	// Save combobox history...
	m_pSetup->saveComboBoxHistory(m_ui.ServerPrefixComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.ServerNameComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.ServerSuffixComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.StartupScriptShellComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.PostStartupScriptShellComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.ShutdownScriptShellComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.PostShutdownScriptShellComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.XrunRegexComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.ActivePatchbayPathComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.MessagesLogPathComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.ServerConfigNameComboBox);

	// Save/commit to disk.
	m_pSetup->saveSetup();

	// Reset dirty flags.
	m_iDirtySettings = 0;
	m_iDirtyOptions = 0;

	// Just go with dialog acceptance.
	QDialog::accept();
}


// Reject settings (Cancel button slot).
void qjackctlSetupForm::reject (void)
{
	if (queryClose())
		QDialog::reject();
}


// Check whether we're clear to close.
bool qjackctlSetupForm::queryClose (void)
{
	bool bQueryClose = true;

	// Check if there's any pending changes...
	if (m_iDirtySettings > 0 || m_iDirtyOptions > 0) {
		switch (QMessageBox::warning(this,
			tr("Warning") + " - " QJACKCTL_SUBTITLE1,
			tr("Some settings have been changed.\n\n"
			"Do you want to apply the changes?"),
			QMessageBox::Apply |
			QMessageBox::Discard |
			QMessageBox::Cancel)) {
		case QMessageBox::Apply:
			accept();
			// Fall thru...
		case QMessageBox::Discard:
			// Reset dirty flags...
			m_iDirtySettings = 0;
			m_iDirtyOptions = 0;
			break;
		default:    // Cancel.
			bQueryClose = false;
			break;
		}
	}

	return bQueryClose;
}


// Notify our parent that we're emerging.
void qjackctlSetupForm::showEvent ( QShowEvent *pShowEvent )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();

	stabilizeForm();

	QDialog::showEvent(pShowEvent);
}

// Notify our parent that we're closing.
void qjackctlSetupForm::hideEvent ( QHideEvent *pHideEvent )
{
	QDialog::hideEvent(pHideEvent);

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}

// Just about to notify main-window that we're closing.
void qjackctlSetupForm::closeEvent ( QCloseEvent * /*pCloseEvent*/ )
{
	QDialog::hide();

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}


// end of qjackctlSetupForm.cpp
