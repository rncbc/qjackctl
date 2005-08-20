// qjackctlSetupForm.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
/****************************************************************************
   Copyright (C) 2003-2005, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*****************************************************************************/

#include <qvalidator.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qfontdialog.h>
#include <qpopupmenu.h>
#include <qlineedit.h>

#include "config.h"

#ifdef CONFIG_COREAUDIO
#include <CoreAudio/CoreAudio.h>
#endif

#ifdef CONFIG_ALSA_SEQ
#include <alsa/asoundlib.h>
#endif

// Kind of constructor.
void qjackctlSetupForm::init (void)
{
    // No settings descriptor initially (the caller will set it).
    m_pSetup = NULL;
    // Initialize dirty control state.
    m_iDirtySetup = 0;
    m_iDirtySettings = 0;
    m_iDirtyOptions = 0;

    // Set dialog validators...
    PresetComboBox->setValidator(new QRegExpValidator(QRegExp("[\\w-]+"), PresetComboBox));
    FramesComboBox->setValidator(new QIntValidator(FramesComboBox));
    SampleRateComboBox->setValidator(new QIntValidator(SampleRateComboBox));
    WaitComboBox->setValidator(new QIntValidator(WaitComboBox));
    WordLengthComboBox->setValidator(new QIntValidator(WordLengthComboBox));
    TimeoutComboBox->setValidator(new QIntValidator(TimeoutComboBox));
    TimeRefreshComboBox->setValidator(new QIntValidator(TimeRefreshComboBox));
    PortMaxComboBox->setValidator(new QIntValidator(PortMaxComboBox));
    MessagesLimitLinesComboBox->setValidator(new QIntValidator(MessagesLimitLinesComboBox));

    // Try to restore old window positioning.
    adjustSize();
}


// Kind of destructor.
void qjackctlSetupForm::destroy (void)
{
}


// Populate (setup) dialog controls from settings descriptors.
void qjackctlSetupForm::setup ( qjackctlSetup *pSetup )
{
    // Set reference descriptor.
    m_pSetup = pSetup;

    // Avoid dirty this all up.
    m_iDirtySetup++;

    // Load combo box history...
    m_pSetup->loadComboBoxHistory(ServerComboBox);
    m_pSetup->loadComboBoxHistory(InterfaceComboBox);
    m_pSetup->loadComboBoxHistory(InDeviceComboBox);
    m_pSetup->loadComboBoxHistory(OutDeviceComboBox);
    m_pSetup->loadComboBoxHistory(StartupScriptShellComboBox);
    m_pSetup->loadComboBoxHistory(PostStartupScriptShellComboBox);
    m_pSetup->loadComboBoxHistory(ShutdownScriptShellComboBox);
    m_pSetup->loadComboBoxHistory(PostShutdownScriptShellComboBox);
    m_pSetup->loadComboBoxHistory(XrunRegexComboBox);
    m_pSetup->loadComboBoxHistory(ActivePatchbayPathComboBox);
    m_pSetup->loadComboBoxHistory(ServerConfigNameComboBox);

    // Load Options...
    StartupScriptCheckBox->setChecked(m_pSetup->bStartupScript);
    StartupScriptShellComboBox->setCurrentText(m_pSetup->sStartupScriptShell);
    PostStartupScriptCheckBox->setChecked(m_pSetup->bPostStartupScript);
    PostStartupScriptShellComboBox->setCurrentText(m_pSetup->sPostStartupScriptShell);
    ShutdownScriptCheckBox->setChecked(m_pSetup->bShutdownScript);
    ShutdownScriptShellComboBox->setCurrentText(m_pSetup->sShutdownScriptShell);
    PostShutdownScriptCheckBox->setChecked(m_pSetup->bPostShutdownScript);
    PostShutdownScriptShellComboBox->setCurrentText(m_pSetup->sPostShutdownScriptShell);
    StdoutCaptureCheckBox->setChecked(m_pSetup->bStdoutCapture);
    XrunRegexComboBox->setCurrentText(m_pSetup->sXrunRegex);
    XrunIgnoreFirstCheckBox->setChecked(m_pSetup->bXrunIgnoreFirst);
    ActivePatchbayCheckBox->setChecked(m_pSetup->bActivePatchbay);
    ActivePatchbayPathComboBox->setCurrentText(m_pSetup->sActivePatchbayPath);
    AutoRefreshCheckBox->setChecked(m_pSetup->bAutoRefresh);
    TimeRefreshComboBox->setCurrentText(QString::number(m_pSetup->iTimeRefresh));
    BezierLinesCheckBox->setChecked(m_pSetup->bBezierLines);

    // Load some other defaults...
    TimeDisplayButtonGroup->setButton(m_pSetup->iTimeDisplay);
    TimeFormatComboBox->setCurrentItem(m_pSetup->iTimeFormat);

    // Load font chooser samples...
    const QString sHelvetica = "Helvetica";
    QFont font;

    if (m_pSetup->sMessagesFont.isEmpty() || !font.fromString(m_pSetup->sMessagesFont))
        font = QFont("Terminal", 8);
    MessagesFontTextLabel->setFont(font);
    MessagesFontTextLabel->setText(font.family() + " " + QString::number(font.pointSize()));

    if (m_pSetup->sDisplayFont1.isEmpty() || !font.fromString(m_pSetup->sDisplayFont1))
        font = QFont(sHelvetica, 12, QFont::Bold);
    DisplayFont1TextLabel->setFont(font);
    DisplayFont1TextLabel->setText(font.family() + " " + QString::number(font.pointSize()));

    if (m_pSetup->sDisplayFont2.isEmpty() || !font.fromString(m_pSetup->sDisplayFont2))
        font = QFont(sHelvetica, 8, QFont::Bold);
    DisplayFont2TextLabel->setFont(font);
    DisplayFont2TextLabel->setText(font.family() + " " + QString::number(font.pointSize()));

    if (m_pSetup->sConnectionsFont.isEmpty() || !font.fromString(m_pSetup->sConnectionsFont))
        font = QFont(sHelvetica, 10);
    ConnectionsFontTextLabel->setFont(font);
    ConnectionsFontTextLabel->setText(font.family() + " " + QString::number(font.pointSize()));

    // The main display shiny effect option.
    DisplayEffectCheckBox->setChecked(m_pSetup->bDisplayEffect);
    toggleDisplayEffect(m_pSetup->bDisplayEffect);

    // Connections view icon size.
    ConnectionsIconSizeComboBox->setCurrentItem(m_pSetup->iConnectionsIconSize);

    // Messages limit option.
    MessagesLimitCheckBox->setChecked(m_pSetup->bMessagesLimit);
    MessagesLimitLinesComboBox->setCurrentText(QString::number(m_pSetup->iMessagesLimitLines));

    // Other misc options...
    StartJackCheckBox->setChecked(m_pSetup->bStartJack);
    QueryCloseCheckBox->setChecked(m_pSetup->bQueryClose);
    KeepOnTopCheckBox->setChecked(m_pSetup->bKeepOnTop);
    SystemTrayCheckBox->setChecked(m_pSetup->bSystemTray);
    DelayedSetupCheckBox->setChecked(m_pSetup->bDelayedSetup);
    ServerConfigCheckBox->setChecked(m_pSetup->bServerConfig);
    ServerConfigNameComboBox->setCurrentText(m_pSetup->sServerConfigName);
    ServerConfigTempCheckBox->setChecked(m_pSetup->bServerConfigTemp);
    QueryShutdownCheckBox->setChecked(m_pSetup->bQueryShutdown);
    AliasesEnabledCheckBox->setChecked(m_pSetup->bAliasesEnabled);
    AliasesEditingCheckBox->setChecked(m_pSetup->bAliasesEditing);

#ifndef CONFIG_SYSTEM_TRAY
    SystemTrayCheckBox->setChecked(false);
    SystemTrayCheckBox->setEnabled(false);
#endif

    // Load preset list...
    resetPresets();
    PresetComboBox->setCurrentText(m_pSetup->sDefPreset);
    // Finally, load default settings...
    changePreset(PresetComboBox->currentText());

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
        ServerComboBox->setCurrentText(preset.sServer);
        RealtimeCheckBox->setChecked(preset.bRealtime);
        SoftModeCheckBox->setChecked(preset.bSoftMode);
        MonitorCheckBox->setChecked(preset.bMonitor);
        ShortsCheckBox->setChecked(preset.bShorts);
        NoMemLockCheckBox->setChecked(preset.bNoMemLock);
        UnlockMemCheckBox->setChecked(preset.bUnlockMem);
        HWMonCheckBox->setChecked(preset.bHWMon);
        HWMeterCheckBox->setChecked(preset.bHWMeter);
        IgnoreHWCheckBox->setChecked(preset.bIgnoreHW);
        PrioritySpinBox->setValue(preset.iPriority);
        FramesComboBox->setCurrentText(QString::number(preset.iFrames));
        SampleRateComboBox->setCurrentText(QString::number(preset.iSampleRate));
        PeriodsSpinBox->setValue(preset.iPeriods);
        WordLengthComboBox->setCurrentText(QString::number(preset.iWordLength));
        WaitComboBox->setCurrentText(QString::number(preset.iWait));
        ChanSpinBox->setValue(preset.iChan);
        DriverComboBox->setCurrentText(preset.sDriver);
        if (preset.sInterface.isEmpty())
            InterfaceComboBox->setCurrentText(m_pSetup->sDefPresetName);
        else
            InterfaceComboBox->setCurrentText(preset.sInterface);
        AudioComboBox->setCurrentItem(preset.iAudio);
        DitherComboBox->setCurrentItem(preset.iDither);
        TimeoutComboBox->setCurrentText(QString::number(preset.iTimeout));
        if (preset.sInDevice.isEmpty())
            InDeviceComboBox->setCurrentText(m_pSetup->sDefPresetName);
        else
            InDeviceComboBox->setCurrentText(preset.sInDevice);
        if (preset.sOutDevice.isEmpty())
            OutDeviceComboBox->setCurrentText(m_pSetup->sDefPresetName);
        else
            OutDeviceComboBox->setCurrentText(preset.sOutDevice);
        InChannelsSpinBox->setValue(preset.iInChannels);
        OutChannelsSpinBox->setValue(preset.iOutChannels);
        InLatencySpinBox->setValue(preset.iInLatency);
        OutLatencySpinBox->setValue(preset.iOutLatency);
        StartDelaySpinBox->setValue(preset.iStartDelay);
        VerboseCheckBox->setChecked(preset.bVerbose);
        PortMaxComboBox->setCurrentText(QString::number(preset.iPortMax));
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
    preset.sServer      = ServerComboBox->currentText();
    preset.bRealtime    = RealtimeCheckBox->isChecked();
    preset.bSoftMode    = SoftModeCheckBox->isChecked();
    preset.bMonitor     = MonitorCheckBox->isChecked();
    preset.bShorts      = ShortsCheckBox->isChecked();
    preset.bNoMemLock   = NoMemLockCheckBox->isChecked();
    preset.bUnlockMem   = UnlockMemCheckBox->isChecked();
    preset.bHWMon       = HWMonCheckBox->isChecked();
    preset.bHWMeter     = HWMeterCheckBox->isChecked();
    preset.bIgnoreHW    = IgnoreHWCheckBox->isChecked();
    preset.iPriority    = PrioritySpinBox->value();
    preset.iFrames      = FramesComboBox->currentText().toInt();
    preset.iSampleRate  = SampleRateComboBox->currentText().toInt();
    preset.iPeriods     = PeriodsSpinBox->value();
    preset.iWordLength  = WordLengthComboBox->currentText().toInt();
    preset.iWait        = WaitComboBox->currentText().toInt();
    preset.iChan        = ChanSpinBox->value();
    preset.sDriver      = DriverComboBox->currentText();
    preset.sInterface   = InterfaceComboBox->currentText();
    preset.iAudio       = AudioComboBox->currentItem();
    preset.iDither      = DitherComboBox->currentItem();
    preset.iTimeout     = TimeoutComboBox->currentText().toInt();
    preset.sInDevice    = InDeviceComboBox->currentText();
    preset.sOutDevice   = OutDeviceComboBox->currentText();
    preset.iInChannels  = InChannelsSpinBox->value();
    preset.iOutChannels = OutChannelsSpinBox->value();
    preset.iInLatency   = InLatencySpinBox->value();
    preset.iOutLatency  = OutLatencySpinBox->value();
    preset.iStartDelay  = StartDelaySpinBox->value();
    preset.bVerbose     = VerboseCheckBox->isChecked();
    preset.iPortMax     = PortMaxComboBox->currentText().toInt();
    if (preset.sInterface == m_pSetup->sDefPresetName || preset.sInterface.isEmpty())
        preset.sInterface = "hw:0";
    if (preset.sInDevice == m_pSetup->sDefPresetName)
        preset.sInDevice = QString::null;
    if (preset.sOutDevice == m_pSetup->sDefPresetName)
        preset.sOutDevice = QString::null;
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
    PresetComboBox->clear();
    PresetComboBox->insertStringList(m_pSetup->presets);
    PresetComboBox->insertItem(m_pSetup->sDefPresetName);
}


void qjackctlSetupForm::changeCurrentPreset( const QString& sPreset )
{
    if (m_iDirtySetup > 0)
        return;

    // Check if there's any pending changes...
    if (m_iDirtySettings > 0 && !m_sPreset.isEmpty()) {
        switch (QMessageBox::warning(this, tr("Warning"),
            tr("Some settings have been changed:") + "\n\n" +
            m_sPreset + "\n\n" +
            tr("Do you want to save the changes?"),
            tr("Save"), tr("Discard"), tr("Cancel"))) {
          case 0: // Save...
            savePreset(m_sPreset);
            m_iDirtySetup++;
            resetPresets();
            PresetComboBox->setCurrentText(sPreset);
            m_iDirtySetup--;
          case 1: // Discard...
            m_iDirtySettings = 0;
            break;
          default:// Cancel...
            m_iDirtySetup++;
            resetPresets();
            PresetComboBox->setCurrentText(m_sPreset);
            m_iDirtySetup--;
            return;
        }
    }

    changePreset(sPreset);
    optionsChanged();
}

void qjackctlSetupForm::saveCurrentPreset (void)
{
    const QString sPreset = PresetComboBox->currentText();

    if (savePreset(sPreset)) {
        // Reset preset combobox list.
        m_iDirtySetup++;
        resetPresets();
        PresetComboBox->setCurrentText(sPreset);
        m_iDirtySetup--;
        // Reset dirty flag.
        m_iDirtySettings = 0;
        stabilizeForm();
    }
}


void qjackctlSetupForm::deleteCurrentPreset (void)
{
    const QString sPreset = PresetComboBox->currentText();

    // Try to prompt user if he/she really wants this...
    if (QMessageBox::warning(this, tr("Warning"),
        tr("Delete preset:") + "\n\n" +
        sPreset + "\n\n" +
        tr("Are you sure?"),
        tr("OK"), tr("Cancel")) > 0)
        return;

    if (deletePreset(sPreset)) {
        // Reset preset combobox list,
        // and load a new available preset..
        m_iDirtySetup++;
        int iItem = PresetComboBox->currentItem();
        resetPresets();
        PresetComboBox->setCurrentItem(iItem);
        changePreset(PresetComboBox->currentText());
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
    int p = FramesComboBox->currentText().toInt();
    int r = SampleRateComboBox->currentText().toInt();
    int n = PeriodsSpinBox->value();
    if (r > 0)
        lat = (float) (1000.0 * p * n) / (float) r;
    if (lat > 0.0)
        LatencyTextValue->setText(QString::number(lat, 'g', 3) + " " + tr("msec"));
    else
        LatencyTextValue->setText(tr("n/a"));

    OkPushButton->setEnabled(m_iDirtySettings > 0 || m_iDirtyOptions > 0);
}


void qjackctlSetupForm::changeDriverAudio ( const QString& sDriver, int iAudio )
{
    bool bOss        = (sDriver == "oss");
    bool bAlsa       = (sDriver == "alsa");
    bool bCoreaudio  = (sDriver == "coreaudio");
    bool bInEnabled  = false;
    bool bOutEnabled = false;

    switch (iAudio) {
      case QJACKCTL_DUPLEX:
        bInEnabled  = (bOss || bAlsa || bCoreaudio);
        bOutEnabled = (bOss || bAlsa || bCoreaudio);
        break;
      case QJACKCTL_CAPTURE:
        bInEnabled  = bOss || bCoreaudio;
        break;
      case QJACKCTL_PLAYBACK:
        bOutEnabled = bOss || bCoreaudio;
        break;
    }

    InDeviceTextLabel->setEnabled(bInEnabled && (bAlsa || bOss));
    InDeviceComboBox->setEnabled(bInEnabled && (bAlsa || bOss));
    InDevicePushButton->setEnabled(bInEnabled && (bAlsa || bOss));
    OutDeviceTextLabel->setEnabled(bOutEnabled && (bAlsa || bOss));
    OutDeviceComboBox->setEnabled(bOutEnabled && (bAlsa || bOss));
    OutDevicePushButton->setEnabled(bOutEnabled && (bAlsa || bOss));

    InChannelsTextLabel->setEnabled(bInEnabled || (bAlsa && iAudio != QJACKCTL_PLAYBACK));
    InChannelsSpinBox->setEnabled(bInEnabled || (bAlsa && iAudio != QJACKCTL_PLAYBACK));
    OutChannelsTextLabel->setEnabled(bOutEnabled || (bAlsa && iAudio != QJACKCTL_CAPTURE));
    OutChannelsSpinBox->setEnabled(bOutEnabled || (bAlsa && iAudio != QJACKCTL_CAPTURE));

	InLatencyTextLabel->setEnabled(bInEnabled || (bAlsa && iAudio != QJACKCTL_PLAYBACK));
	InLatencySpinBox->setEnabled(bInEnabled || (bAlsa && iAudio != QJACKCTL_PLAYBACK));
	OutLatencyTextLabel->setEnabled(bOutEnabled || (bAlsa && iAudio != QJACKCTL_CAPTURE));
	OutLatencySpinBox->setEnabled(bOutEnabled || (bAlsa && iAudio != QJACKCTL_CAPTURE));

    computeLatency();
}


void qjackctlSetupForm::changeAudio ( int iAudio )
{
    changeDriverAudio(DriverComboBox->currentText(), iAudio);
}


void qjackctlSetupForm::changeDriver ( const QString& sDriver )
{
    bool bDummy     = (sDriver == "dummy");
    bool bOss       = (sDriver == "oss");
    bool bAlsa      = (sDriver == "alsa");
    bool bPortaudio = (sDriver == "portaudio");
	bool bCoreaudio = (sDriver == "coreaudio");

    NoMemLockCheckBox->setEnabled(!bCoreaudio);
    UnlockMemCheckBox->setEnabled(!bCoreaudio && !NoMemLockCheckBox->isChecked());

    SoftModeCheckBox->setEnabled(bAlsa);
    MonitorCheckBox->setEnabled(bAlsa);
    ShortsCheckBox->setEnabled(bAlsa);
    HWMonCheckBox->setEnabled(bAlsa);
    HWMeterCheckBox->setEnabled(bAlsa);

    IgnoreHWCheckBox->setEnabled(bOss);

    PeriodsTextLabel->setEnabled(bAlsa || bOss);
    PeriodsSpinBox->setEnabled(bAlsa || bOss);

    WordLengthTextLabel->setEnabled(bOss);
    WordLengthComboBox->setEnabled(bOss);

    WaitTextLabel->setEnabled(bDummy);
    WaitComboBox->setEnabled(bDummy);

	ChanTextLabel->setEnabled(bPortaudio);
	ChanSpinBox->setEnabled(bPortaudio);

	int  iAudio   = AudioComboBox->currentItem();
	bool bEnabled = bAlsa;
	if (bEnabled && iAudio == QJACKCTL_DUPLEX) {
		const QString& sInDevice  = InDeviceComboBox->currentText();
		const QString& sOutDevice = OutDeviceComboBox->currentText();
		bEnabled = (sInDevice.isEmpty()  || sInDevice  == m_pSetup->sDefPresetName ||
					sOutDevice.isEmpty() || sOutDevice == m_pSetup->sDefPresetName);
	}
	InterfaceTextLabel->setEnabled(bEnabled || bCoreaudio);
	InterfaceComboBox->setEnabled(bEnabled || bCoreaudio);
	InterfacePushButton->setEnabled(bEnabled || bCoreaudio);

    DitherTextLabel->setEnabled(bAlsa || bPortaudio);
    DitherComboBox->setEnabled(bAlsa || bPortaudio);

    changeDriverAudio(sDriver, iAudio);
}


// Stabilize current form state.
void qjackctlSetupForm::stabilizeForm (void)
{
    QString sPreset = PresetComboBox->currentText();
    if (!sPreset.isEmpty()) {
        bool bPreset = (m_pSetup->presets.find(sPreset) != m_pSetup->presets.end());
        PresetSavePushButton->setEnabled(m_iDirtySettings > 0 || (!bPreset && sPreset != m_pSetup->sDefPresetName));
        PresetDeletePushButton->setEnabled(bPreset);
    } else {
        PresetSavePushButton->setEnabled(false);
        PresetDeletePushButton->setEnabled(false);
    }

    bool bEnabled = RealtimeCheckBox->isChecked();
    PriorityTextLabel->setEnabled(bEnabled);
    PrioritySpinBox->setEnabled(bEnabled);

    bEnabled = StartupScriptCheckBox->isChecked();
    StartupScriptShellComboBox->setEnabled(bEnabled);
    StartupScriptSymbolPushButton->setEnabled(bEnabled);
    StartupScriptBrowsePushButton->setEnabled(bEnabled);

    bEnabled = PostStartupScriptCheckBox->isChecked();
    PostStartupScriptShellComboBox->setEnabled(bEnabled);
    PostStartupScriptSymbolPushButton->setEnabled(bEnabled);
    PostStartupScriptBrowsePushButton->setEnabled(bEnabled);

    bEnabled = ShutdownScriptCheckBox->isChecked();
    ShutdownScriptShellComboBox->setEnabled(bEnabled);
    ShutdownScriptSymbolPushButton->setEnabled(bEnabled);
    ShutdownScriptBrowsePushButton->setEnabled(bEnabled);

    bEnabled = PostShutdownScriptCheckBox->isChecked();
    PostShutdownScriptShellComboBox->setEnabled(bEnabled);
    PostShutdownScriptSymbolPushButton->setEnabled(bEnabled);
    PostShutdownScriptBrowsePushButton->setEnabled(bEnabled);

    bEnabled = StdoutCaptureCheckBox->isChecked();
    XrunRegexTextLabel->setEnabled(bEnabled);
    XrunRegexComboBox->setEnabled(bEnabled);
    XrunIgnoreFirstCheckBox->setEnabled(bEnabled);

    bEnabled = ActivePatchbayCheckBox->isChecked();
    ActivePatchbayPathComboBox->setEnabled(bEnabled);
    ActivePatchbayPathPushButton->setEnabled(bEnabled);

    TimeRefreshComboBox->setEnabled(AutoRefreshCheckBox->isChecked());
    MessagesLimitLinesComboBox->setEnabled(MessagesLimitCheckBox->isChecked());

    bEnabled = ServerConfigCheckBox->isChecked();
    ServerConfigNameComboBox->setEnabled(bEnabled);
    ServerConfigTempCheckBox->setEnabled(bEnabled);

    AliasesEditingCheckBox->setEnabled(AliasesEnabledCheckBox->isChecked());

    changeDriver(DriverComboBox->currentText());
}


// Device selection menu executive.
void qjackctlSetupForm::deviceMenu( QLineEdit *pLineEdit,
	QPushButton *pPushButton, int iAudio )
{
	// FIXME: Only valid for ALSA and OSS devices,
	// for the time being... and also CoreAudio ones too.
	const QString& sDriver = DriverComboBox->currentText();
	bool bAlsa      = (sDriver == "alsa");
	bool bOss       = (sDriver == "oss");
#ifdef CONFIG_COREAUDIO
	bool bCoreaudio = (sDriver == "coreaudio");
#endif
	QString sCurName = pLineEdit->text();
	QString sName, sSubName;
	QString sText;
	
	QPopupMenu* pContextMenu = new QPopupMenu(this);
    int iItemID;

	int iCards = 0;

#ifdef CONFIG_ALSA_SEQ
	if (bAlsa) {
		// Enumerate the ALSA cards and PCM harfware devices...
		snd_ctl_t *handle;
		snd_ctl_card_info_t *info;
		snd_pcm_info_t *pcminfo;
		snd_ctl_card_info_alloca(&info);
		snd_pcm_info_alloca(&pcminfo);
		int iCard = -1;
		while (snd_card_next(&iCard) >= 0 && iCard >= 0) {
			sName = "hw:" + QString::number(iCard);
			if (snd_ctl_open(&handle, sName.latin1(), 0) >= 0
				&& snd_ctl_card_info(handle, info) >= 0) {
				if (iCards > 0)
					pContextMenu->insertSeparator();
				sText  = sName + '\t';
				sText += snd_ctl_card_info_get_name(info);
				iItemID = pContextMenu->insertItem(sText);
				pContextMenu->setItemChecked(iItemID, sCurName == sName);
				int iDevice = -1;
				while (snd_ctl_pcm_next_device(handle, &iDevice) >= 0
					&& iDevice >= 0) {
					snd_pcm_info_set_device(pcminfo, iDevice);
					snd_pcm_info_set_subdevice(pcminfo, 0);
					snd_pcm_info_set_stream(pcminfo,
						iAudio == QJACKCTL_CAPTURE ?
							SND_PCM_STREAM_CAPTURE :
							SND_PCM_STREAM_PLAYBACK);
					if (snd_ctl_pcm_info(handle, pcminfo) >= 0) {
						sSubName = sName + ',' + QString::number(iDevice);
						sText  = sSubName + '\t';
						sText += snd_pcm_info_get_name(pcminfo);
						iItemID = pContextMenu->insertItem(sText);
						pContextMenu->setItemChecked(iItemID,
							sCurName == sSubName);
					}
				}
				snd_ctl_close(handle);
				++iCards;
			}
		}
	}	// Enumerate the OSS Audio devices...
	else
#endif 	// CONFIG_ALSA_SEQ
	if (bOss) {
		QFile file("/dev/sndstat");
		if (file.open(IO_ReadOnly)) {
			QTextStream stream(&file);
			QString sLine;
			bool bAudioDevices = false;
			QRegExp rxHeader("Audio devices.*", false);
			QRegExp rxDevice("([0-9]+):[ ]+(.*)");
			while (!stream.atEnd()) {
				sLine = stream.readLine();
				if (bAudioDevices) {
					if (rxDevice.exactMatch(sLine)) {
						sName = "/dev/dsp" + rxDevice.cap(1);
						sText = sName + '\t' + rxDevice.cap(2);
						iItemID = pContextMenu->insertItem(sText);
						pContextMenu->setItemChecked(iItemID,
							sCurName == sName);
						++iCards;
					}
					else break;
				}
				else if (rxHeader.exactMatch(sLine))
					bAudioDevices = true;
			}
			file.close();
		}
	}
#ifdef CONFIG_COREAUDIO
	else if (bCoreaudio) {
		// Find out how many Core Audio devices are there, if any...
		// (code snippet gently "borrowed" from St�hane Letz jackdmp;)
		OSStatus err;
		Boolean isWritable;
		UInt32 outSize = sizeof(isWritable);
		err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,
				&outSize, &isWritable);
		if (err == noErr) {
			// Calculate the number of device available...
			int numCoreDevices = outSize / sizeof(AudioDeviceID);
			// Make space for the devices we are about to get...
			AudioDeviceID *coreDeviceIDs = new AudioDeviceID [numCoreDevices];
			err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices,
					&outSize, (void *) coreDeviceIDs);
			if (err == noErr) {
				// Look for the CoreAudio device name...
				char coreDeviceName[256];
				for (int i = 0; i < numCoreDevices; i++) {
					err = AudioDeviceGetPropertyInfo(coreDeviceIDs[i],
							0, true, kAudioDevicePropertyDeviceName,
							&outSize, &isWritable);
					if (err == noErr) {
						err = AudioDeviceGetProperty(coreDeviceIDs[i],
								0, true, kAudioDevicePropertyDeviceName,
								&outSize, (void *) coreDeviceName);
						if (err == noErr) {
							sName = QString::number(coreDeviceIDs[i]);
							sText = sName + '\t' + coreDeviceName;
							iItemID = pContextMenu->insertItem(sText);
							pContextMenu->setItemChecked(iItemID,
								sCurName == sName);
							++iCards;
						}
					}
				}
			}
			delete [] coreDeviceIDs;
		}
	}
#endif 	// CONFIG_COREAUDIO

	// There's always the default device...
	if (iCards > 0)
		pContextMenu->insertSeparator();
	iItemID = pContextMenu->insertItem(m_pSetup->sDefPresetName);
	pContextMenu->setItemChecked(iItemID,
		sCurName == m_pSetup->sDefPresetName);

	// Show the device menu and read selection...
	iItemID = pContextMenu->exec(pPushButton->mapToGlobal(QPoint(0,0)));
	if (iItemID != -1) {
		sText = pContextMenu->text(iItemID);
		int iTabPos = sText.find('\t');
		if (iTabPos >= 0) {
			pLineEdit->setText(sText.left(iTabPos));
		} else {
			pLineEdit->setText(sText);
		}
	//  settingsChanged();
	}

	delete pContextMenu;
}


// Interface device selection menu.
void qjackctlSetupForm::selectInterface (void)
{
	deviceMenu(InterfaceComboBox->lineEdit(), InterfacePushButton,
		AudioComboBox->currentItem());
}


// Input device selection menu.
void qjackctlSetupForm::selectInDevice (void)
{
	deviceMenu(InDeviceComboBox->lineEdit(), InDevicePushButton,
		QJACKCTL_CAPTURE);
}


// Output device selection menu.
void qjackctlSetupForm::selectOutDevice (void)
{
	deviceMenu(OutDeviceComboBox->lineEdit(), OutDevicePushButton,
		QJACKCTL_PLAYBACK);
}


// Meta-symbol menu executive.
void qjackctlSetupForm::symbolMenu( QLineEdit *pLineEdit,
	QPushButton *pPushButton )
{
    const QString s = "  ";

    QPopupMenu* pContextMenu = new QPopupMenu(this);

    pContextMenu->insertItem("%P" + s + tr("&Preset Name"));
    pContextMenu->insertSeparator();
    pContextMenu->insertItem("%s" + s + tr("&Server Path"));
    pContextMenu->insertItem("%d" + s + tr("&Driver"));
    pContextMenu->insertItem("%i" + s + tr("&Interface"));
    pContextMenu->insertSeparator();
    pContextMenu->insertItem("%r" + s + tr("Sample &Rate"));
    pContextMenu->insertItem("%p" + s + tr("&Frames/Period"));
    pContextMenu->insertItem("%n" + s + tr("Periods/&Buffer"));

    int iItemID = pContextMenu->exec(pPushButton->mapToGlobal(QPoint(0,0)));
    if (iItemID != -1) {
        QString sText = pContextMenu->text(iItemID);
        int iMetaChar = sText.find('%');
        if (iMetaChar >= 0) {
            pLineEdit->insert("%" + sText[iMetaChar + 1]);
        //  optionsChanged();
        }
    }

    delete pContextMenu;
}


// Startup script meta-symbol button slot.
void qjackctlSetupForm::symbolStartupScript (void)
{
    symbolMenu(StartupScriptShellComboBox->lineEdit(), StartupScriptSymbolPushButton);
}

// Post-startup script meta-symbol button slot.
void qjackctlSetupForm::symbolPostStartupScript (void)
{
    symbolMenu(PostStartupScriptShellComboBox->lineEdit(), PostStartupScriptSymbolPushButton);
}

// Shutdown script meta-symbol button slot.
void qjackctlSetupForm::symbolShutdownScript (void)
{
    symbolMenu(ShutdownScriptShellComboBox->lineEdit(), ShutdownScriptSymbolPushButton);
}

// Post-shutdown script meta-symbol button slot.
void qjackctlSetupForm::symbolPostShutdownScript (void)
{
    symbolMenu(PostShutdownScriptShellComboBox->lineEdit(), PostShutdownScriptSymbolPushButton);
}


// Startup script browse slot.
void qjackctlSetupForm::browseStartupScript()
{
    QString sFileName = QFileDialog::getOpenFileName(
            StartupScriptShellComboBox->currentText(),  // Start here.
            QString::null,                              // Filter (all files?)
            this, 0,                                    // Parent and name (none)
            tr("Startup script")                        // Caption.
    );

    if (!sFileName.isEmpty()) {
        StartupScriptShellComboBox->setCurrentText(sFileName);
        StartupScriptShellComboBox->setFocus();
        optionsChanged();
    }
}


// Post-startup script browse slot.
void qjackctlSetupForm::browsePostStartupScript()
{
    QString sFileName = QFileDialog::getOpenFileName(
            PostStartupScriptShellComboBox->currentText(),  // Start here.
            QString::null,                                  // Filter (all files?)
            this, 0,                                        // Parent and name (none)
            tr("Post-startup script")                       // Caption.
    );

    if (!sFileName.isEmpty()) {
        PostStartupScriptShellComboBox->setCurrentText(sFileName);
        PostStartupScriptShellComboBox->setFocus();
        optionsChanged();
    }
}


// Shutdown script browse slot.
void qjackctlSetupForm::browseShutdownScript()
{
    QString sFileName = QFileDialog::getOpenFileName(
            ShutdownScriptShellComboBox->currentText(), // Start here.
            QString::null,                              // Filter (all files?)
            this, 0,                                    // Parent and name (none)
            tr("Shutdown script")                       // Caption.
    );

    if (!sFileName.isEmpty()) {
        ShutdownScriptShellComboBox->setCurrentText(sFileName);
        ShutdownScriptShellComboBox->setFocus();
        optionsChanged();
    }
}


// Post-shutdown script browse slot.
void qjackctlSetupForm::browsePostShutdownScript()
{
    QString sFileName = QFileDialog::getOpenFileName(
            PostShutdownScriptShellComboBox->currentText(), // Start here.
            QString::null,                                  // Filter (all files?)
            this, 0,                                        // Parent and name (none)
            tr("Post-shutdown script")                      // Caption.
    );

    if (!sFileName.isEmpty()) {
        PostShutdownScriptShellComboBox->setCurrentText(sFileName);
        PostShutdownScriptShellComboBox->setFocus();
        optionsChanged();
    }
}


// Active Patchbay path browse slot.
void qjackctlSetupForm::browseActivePatchbayPath()
{
    QString sFileName = QFileDialog::getOpenFileName(
            ActivePatchbayPathComboBox->currentText(),	    // Start here.
            tr("Patchbay Definition files") + " (*.xml)",   // Filter (XML files)
            this, 0,                                        // Parent and name (none)
            tr("Active Patchbay definition")                // Caption.
    );

    if (!sFileName.isEmpty()) {
        ActivePatchbayPathComboBox->setCurrentText(sFileName);
        ActivePatchbayPathComboBox->setFocus();
        optionsChanged();
    }
}


// The display font 1 (big time) selection dialog.
void qjackctlSetupForm::chooseDisplayFont1()
{
    bool  bOk  = false;
    QFont font = QFontDialog::getFont(&bOk, DisplayFont1TextLabel->font(), this);
    if (bOk) {
        DisplayFont1TextLabel->setFont(font);
        DisplayFont1TextLabel->setText(font.family() + " " + QString::number(font.pointSize()));
        optionsChanged();
    }
}


// The display font 2 (normal time et al.) selection dialog.
void qjackctlSetupForm::chooseDisplayFont2()
{
    bool  bOk  = false;
    QFont font = QFontDialog::getFont(&bOk, DisplayFont2TextLabel->font(), this);
    if (bOk) {
        DisplayFont2TextLabel->setFont(font);
        DisplayFont2TextLabel->setText(font.family() + " " + QString::number(font.pointSize()));
        optionsChanged();
    }
}


// The channel display effect demo changer.
void qjackctlSetupForm::toggleDisplayEffect ( bool bOn )
{
    QPixmap pm;
    if (bOn)
        pm = QPixmap::fromMimeSource("displaybg1.png");

    DisplayFont1TextLabel->setPaletteBackgroundPixmap(pm);
    DisplayFont2TextLabel->setPaletteBackgroundPixmap(pm);
    optionsChanged();
}


// The messages font selection dialog.
void qjackctlSetupForm::chooseMessagesFont()
{
    bool  bOk  = false;
    QFont font = QFontDialog::getFont(&bOk, MessagesFontTextLabel->font(), this);
    if (bOk) {
        MessagesFontTextLabel->setFont(font);
        MessagesFontTextLabel->setText(font.family() + " " + QString::number(font.pointSize()));
        optionsChanged();
    }
}


// The connections font selection dialog.
void qjackctlSetupForm::chooseConnectionsFont()
{
    bool  bOk  = false;
    QFont font = QFontDialog::getFont(&bOk, ConnectionsFontTextLabel->font(), this);
    if (bOk) {
        ConnectionsFontTextLabel->setFont(font);
        ConnectionsFontTextLabel->setText(font.family() + " " + QString::number(font.pointSize()));
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
    if (m_iDirtySettings > 0 || m_iDirtyOptions > 0) {
        // Save current preset selection.
        m_pSetup->sDefPreset = PresetComboBox->currentText();
        // Always save current settings...
        savePreset(m_pSetup->sDefPreset);
        // Save Options...
        m_pSetup->bStartupScript           = StartupScriptCheckBox->isChecked();
        m_pSetup->sStartupScriptShell      = StartupScriptShellComboBox->currentText();
        m_pSetup->bPostStartupScript       = PostStartupScriptCheckBox->isChecked();
        m_pSetup->sPostStartupScriptShell  = PostStartupScriptShellComboBox->currentText();
        m_pSetup->bShutdownScript          = ShutdownScriptCheckBox->isChecked();
        m_pSetup->sShutdownScriptShell     = ShutdownScriptShellComboBox->currentText();
        m_pSetup->bPostShutdownScript      = PostShutdownScriptCheckBox->isChecked();
        m_pSetup->sPostShutdownScriptShell = PostShutdownScriptShellComboBox->currentText();
        m_pSetup->bStdoutCapture           = StdoutCaptureCheckBox->isChecked();
        m_pSetup->sXrunRegex               = XrunRegexComboBox->currentText();
        m_pSetup->bXrunIgnoreFirst         = XrunIgnoreFirstCheckBox->isChecked();
        m_pSetup->bActivePatchbay          = ActivePatchbayCheckBox->isChecked();
        m_pSetup->sActivePatchbayPath      = ActivePatchbayPathComboBox->currentText();
        m_pSetup->bAutoRefresh             = AutoRefreshCheckBox->isChecked();
        m_pSetup->iTimeRefresh             = TimeRefreshComboBox->currentText().toInt();
        m_pSetup->bBezierLines             = BezierLinesCheckBox->isChecked();
        // Save Defaults...
        m_pSetup->iTimeDisplay             = TimeDisplayButtonGroup->id(TimeDisplayButtonGroup->selected());
        m_pSetup->iTimeFormat              = TimeFormatComboBox->currentItem();
        m_pSetup->sMessagesFont            = MessagesFontTextLabel->font().toString();
        m_pSetup->bMessagesLimit           = MessagesLimitCheckBox->isChecked();
        m_pSetup->iMessagesLimitLines      = MessagesLimitLinesComboBox->currentText().toInt();
        m_pSetup->sDisplayFont1            = DisplayFont1TextLabel->font().toString();
        m_pSetup->sDisplayFont2            = DisplayFont2TextLabel->font().toString();
        m_pSetup->bDisplayEffect           = DisplayEffectCheckBox->isChecked();
        m_pSetup->iConnectionsIconSize     = ConnectionsIconSizeComboBox->currentItem();
        m_pSetup->sConnectionsFont         = ConnectionsFontTextLabel->font().toString();
        m_pSetup->bStartJack               = StartJackCheckBox->isChecked();
        m_pSetup->bQueryClose              = QueryCloseCheckBox->isChecked();
        m_pSetup->bKeepOnTop               = KeepOnTopCheckBox->isChecked();
        m_pSetup->bSystemTray              = SystemTrayCheckBox->isChecked();
        m_pSetup->bDelayedSetup            = DelayedSetupCheckBox->isChecked();
        m_pSetup->bServerConfig            = ServerConfigCheckBox->isChecked();
        m_pSetup->sServerConfigName        = ServerConfigNameComboBox->currentText();
        m_pSetup->bServerConfigTemp        = ServerConfigTempCheckBox->isChecked();
        m_pSetup->bQueryShutdown           = QueryShutdownCheckBox->isChecked();
        m_pSetup->bAliasesEnabled          = AliasesEnabledCheckBox->isChecked();
        m_pSetup->bAliasesEditing          = AliasesEditingCheckBox->isChecked();
    }

    // Save combobox history...
    m_pSetup->saveComboBoxHistory(ServerComboBox);
    m_pSetup->saveComboBoxHistory(InterfaceComboBox);
    m_pSetup->saveComboBoxHistory(InDeviceComboBox);
    m_pSetup->saveComboBoxHistory(OutDeviceComboBox);
    m_pSetup->saveComboBoxHistory(StartupScriptShellComboBox);
    m_pSetup->saveComboBoxHistory(PostStartupScriptShellComboBox);
    m_pSetup->saveComboBoxHistory(ShutdownScriptShellComboBox);
    m_pSetup->saveComboBoxHistory(PostShutdownScriptShellComboBox);
    m_pSetup->saveComboBoxHistory(XrunRegexComboBox);
    m_pSetup->saveComboBoxHistory(ActivePatchbayPathComboBox);
    m_pSetup->saveComboBoxHistory(ServerConfigNameComboBox);

    // Just go with dialog acceptance.
    QDialog::accept();
}


// Reject settings (Cancel button slot).
void qjackctlSetupForm::reject (void)
{
    bool bReject = true;

    // Check if there's any pending changes...
    if (m_iDirtySettings > 0 || m_iDirtyOptions > 0) {
        switch (QMessageBox::warning(this, tr("Warning"),
            tr("Some settings have been changed.") + "\n\n" +
            tr("Do you want to apply the changes?"),
            tr("Apply"), tr("Discard"), tr("Cancel"))) {
          case 0:     // Apply...
            accept();
            return;
          case 1:     // Discard
            break;
          default:    // Cancel.
            bReject = false;
        }
    }

    if (bReject)
        QDialog::reject();
}


// end of qjackctlSetupForm.ui.h


