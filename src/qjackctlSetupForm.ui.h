// qjackctlSetupForm.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
/****************************************************************************
   Copyright (C) 2003-2004, rncbc aka Rui Nuno Capela. All rights reserved.

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
    ChanComboBox->setValidator(new QIntValidator(ChanComboBox));
    PriorityComboBox->setValidator(new QIntValidator(PriorityComboBox));
    FramesComboBox->setValidator(new QIntValidator(FramesComboBox));
    SampleRateComboBox->setValidator(new QIntValidator(SampleRateComboBox));
    PeriodsComboBox->setValidator(new QIntValidator(PeriodsComboBox));
    WaitComboBox->setValidator(new QIntValidator(WaitComboBox));
    WordLengthComboBox->setValidator(new QIntValidator(WordLengthComboBox));
    TimeoutComboBox->setValidator(new QIntValidator(TimeoutComboBox));
    TimeRefreshComboBox->setValidator(new QIntValidator(TimeRefreshComboBox));
    StartDelayComboBox->setValidator(new QIntValidator(StartDelayComboBox));
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
    StdoutCaptureCheckBox->setChecked(m_pSetup->bStdoutCapture);
    XrunRegexComboBox->setCurrentText(m_pSetup->sXrunRegex);
    XrunIgnoreFirstCheckBox->setChecked(m_pSetup->bXrunIgnoreFirst);
    ActivePatchbayCheckBox->setChecked(m_pSetup->bActivePatchbay);
    ActivePatchbayPathComboBox->setCurrentText(m_pSetup->sActivePatchbayPath);
    AutoRefreshCheckBox->setChecked(m_pSetup->bAutoRefresh);
    TimeRefreshComboBox->setCurrentText(QString::number(m_pSetup->iTimeRefresh));

    // Load some other defaults...
    TimeDisplayButtonGroup->setButton(m_pSetup->iTimeDisplay);
    TimeFormatComboBox->setCurrentItem(m_pSetup->iTimeFormat);

    QFont font;
    if (m_pSetup->sMessagesFont.isEmpty() || !font.fromString(m_pSetup->sMessagesFont))
        font = QFont("Terminal", 8);
    MessagesFontTextLabel->setFont(font);
    MessagesFontTextLabel->setText(font.family() + " " + QString::number(font.pointSize()));

    if (m_pSetup->sDisplayFont1.isEmpty() || !font.fromString(m_pSetup->sDisplayFont1))
        font = QFont("Helvetica", 12, QFont::Bold);
    DisplayFont1TextLabel->setFont(font);
    DisplayFont1TextLabel->setText(font.family() + " " + QString::number(font.pointSize()));

    if (m_pSetup->sDisplayFont2.isEmpty() || !font.fromString(m_pSetup->sDisplayFont2))
        font = QFont("Helvetica", 8, QFont::Bold);
    DisplayFont2TextLabel->setFont(font);
    DisplayFont2TextLabel->setText(font.family() + " " + QString::number(font.pointSize()));

    // Messages limit option.
    MessagesLimitCheckBox->setChecked(m_pSetup->bMessagesLimit);
    MessagesLimitLinesComboBox->setCurrentText(QString::number(m_pSetup->iMessagesLimitLines));

    // Other misc options...
    StartJackCheckBox->setChecked(m_pSetup->bStartJack);
    QueryCloseCheckBox->setChecked(m_pSetup->bQueryClose);
    KeepOnTopCheckBox->setChecked(m_pSetup->bKeepOnTop);
    SystemTrayCheckBox->setChecked(m_pSetup->bSystemTray);
    ServerConfigCheckBox->setChecked(m_pSetup->bServerConfig);
    ServerConfigNameComboBox->setCurrentText(m_pSetup->sServerConfigName);
    ServerConfigTempCheckBox->setChecked(m_pSetup->bServerConfigTemp);

#ifndef CONFIG_KDE
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
        HWMonCheckBox->setChecked(preset.bHWMon);
        HWMeterCheckBox->setChecked(preset.bHWMeter);
        IgnoreHWCheckBox->setChecked(preset.bIgnoreHW);
        PriorityComboBox->setCurrentText(QString::number(preset.iPriority));
        FramesComboBox->setCurrentText(QString::number(preset.iFrames));
        SampleRateComboBox->setCurrentText(QString::number(preset.iSampleRate));
        PeriodsComboBox->setCurrentText(QString::number(preset.iPeriods));
        WordLengthComboBox->setCurrentText(QString::number(preset.iWordLength));
        WaitComboBox->setCurrentText(QString::number(preset.iWait));
        ChanComboBox->setCurrentText(QString::number(preset.iChan));
        DriverComboBox->setCurrentText(preset.sDriver);
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
        StartDelayComboBox->setCurrentText(QString::number(preset.iStartDelay));
        VerboseCheckBox->setChecked(preset.bVerbose);
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
    preset.bHWMon       = HWMonCheckBox->isChecked();
    preset.bHWMeter     = HWMeterCheckBox->isChecked();
    preset.bIgnoreHW    = IgnoreHWCheckBox->isChecked();
    preset.iPriority    = PriorityComboBox->currentText().toInt();
    preset.iFrames      = FramesComboBox->currentText().toInt();
    preset.iSampleRate  = SampleRateComboBox->currentText().toInt();
    preset.iPeriods     = PeriodsComboBox->currentText().toInt();
    preset.iWordLength  = WordLengthComboBox->currentText().toInt();
    preset.iWait        = WaitComboBox->currentText().toInt();
    preset.iChan        = ChanComboBox->currentText().toInt();
    preset.sDriver      = DriverComboBox->currentText();
    preset.sInterface   = InterfaceComboBox->currentText();
    preset.iAudio       = AudioComboBox->currentItem();
    preset.iDither      = DitherComboBox->currentItem();
    preset.iTimeout     = TimeoutComboBox->currentText().toInt();
    preset.sInDevice    = InDeviceComboBox->currentText();
    preset.sOutDevice   = OutDeviceComboBox->currentText();
    preset.iInChannels  = InChannelsSpinBox->value();
    preset.iOutChannels = OutChannelsSpinBox->value();
    preset.iStartDelay  = StartDelayComboBox->currentText().toInt();
    preset.bVerbose     = VerboseCheckBox->isChecked();
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
    int n = PeriodsComboBox->currentText().toInt();
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
//  bool bDummy     = (sDriver == "dummy");
    bool bOss       = (sDriver == "oss");
    bool bAlsa      = (sDriver == "alsa");
//  bool bPortaudio = (sDriver == "portaudio");

    bool bInEnabled  = false;
    bool bOutEnabled = false;
    switch (iAudio) {
    case QJACKCTL_DUPLEX:
        bInEnabled  = true;
        bOutEnabled = true;
        break;
    case QJACKCTL_CAPTURE:
        bInEnabled  = true;
        bOutEnabled = false;
        break;
    case QJACKCTL_PLAYBACK:
        bInEnabled  = false;
        bOutEnabled = true;
        break;
    }

    InDeviceTextLabel->setEnabled(bInEnabled && bOss);
    InDeviceComboBox->setEnabled(bInEnabled && bOss);
    OutDeviceTextLabel->setEnabled(bOutEnabled && bOss);
    OutDeviceComboBox->setEnabled(bOutEnabled && bOss);

    InChannelsTextLabel->setEnabled(bInEnabled && (bOss || bAlsa));
    InChannelsSpinBox->setEnabled(bInEnabled && (bOss || bAlsa));
    OutChannelsTextLabel->setEnabled(bOutEnabled && (bOss || bAlsa));
    OutChannelsSpinBox->setEnabled(bOutEnabled && (bOss || bAlsa));

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

    SoftModeCheckBox->setEnabled(bAlsa);
    MonitorCheckBox->setEnabled(bAlsa);
    ShortsCheckBox->setEnabled(bAlsa);
    HWMonCheckBox->setEnabled(bAlsa);
    HWMeterCheckBox->setEnabled(bAlsa);
    
    IgnoreHWCheckBox->setEnabled(bOss);

    PeriodsTextLabel->setEnabled(bAlsa || bOss);
    PeriodsComboBox->setEnabled(bAlsa || bOss);

    WordLengthTextLabel->setEnabled(bOss);
    WordLengthComboBox->setEnabled(bOss);

    WaitTextLabel->setEnabled(bDummy);
    WaitComboBox->setEnabled(bDummy);

    ChanTextLabel->setEnabled(bPortaudio);
    ChanComboBox->setEnabled(bPortaudio);

    InterfaceTextLabel->setEnabled(bAlsa);
    InterfaceComboBox->setEnabled(bAlsa);

    DitherTextLabel->setEnabled(bAlsa || bPortaudio);
    DitherComboBox->setEnabled(bAlsa || bPortaudio);

    changeDriverAudio(sDriver, AudioComboBox->currentItem());
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
    PriorityComboBox->setEnabled(bEnabled);

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

    changeDriver(DriverComboBox->currentText());
}


// Meta-symbol menu executive.
void qjackctlSetupForm::symbolMenu( QLineEdit *pLineEdit, QPushButton *pPushButton )
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
        m_pSetup->bStartupScript          = StartupScriptCheckBox->isChecked();
        m_pSetup->sStartupScriptShell     = StartupScriptShellComboBox->currentText();
        m_pSetup->bPostStartupScript      = PostStartupScriptCheckBox->isChecked();
        m_pSetup->sPostStartupScriptShell = PostStartupScriptShellComboBox->currentText();
        m_pSetup->bShutdownScript         = ShutdownScriptCheckBox->isChecked();
        m_pSetup->sShutdownScriptShell    = ShutdownScriptShellComboBox->currentText();
        m_pSetup->bStdoutCapture          = StdoutCaptureCheckBox->isChecked();
        m_pSetup->sXrunRegex              = XrunRegexComboBox->currentText();
        m_pSetup->bXrunIgnoreFirst        = XrunIgnoreFirstCheckBox->isChecked();
        m_pSetup->bActivePatchbay         = ActivePatchbayCheckBox->isChecked();
        m_pSetup->sActivePatchbayPath     = ActivePatchbayPathComboBox->currentText();
        m_pSetup->bAutoRefresh            = AutoRefreshCheckBox->isChecked();
        m_pSetup->iTimeRefresh            = TimeRefreshComboBox->currentText().toInt();
        // Save Defaults...
        m_pSetup->iTimeDisplay            = TimeDisplayButtonGroup->id(TimeDisplayButtonGroup->selected());
        m_pSetup->iTimeFormat             = TimeFormatComboBox->currentItem();
        m_pSetup->sMessagesFont           = MessagesFontTextLabel->font().toString();
        m_pSetup->bMessagesLimit          = MessagesLimitCheckBox->isChecked();
        m_pSetup->iMessagesLimitLines     = MessagesLimitLinesComboBox->currentText().toInt();
        m_pSetup->sDisplayFont1           = DisplayFont1TextLabel->font().toString();
        m_pSetup->sDisplayFont2           = DisplayFont2TextLabel->font().toString();
        m_pSetup->bStartJack              = StartJackCheckBox->isChecked();
        m_pSetup->bQueryClose             = QueryCloseCheckBox->isChecked();
        m_pSetup->bKeepOnTop              = KeepOnTopCheckBox->isChecked();
        m_pSetup->bSystemTray             = SystemTrayCheckBox->isChecked();
        m_pSetup->bServerConfig           = ServerConfigCheckBox->isChecked();
        m_pSetup->sServerConfigName       = ServerConfigNameComboBox->currentText();
        m_pSetup->bServerConfigTemp       = ServerConfigTempCheckBox->isChecked();
    }

    // Save combobox history...
    m_pSetup->saveComboBoxHistory(ServerComboBox);
    m_pSetup->saveComboBoxHistory(PriorityComboBox);
    m_pSetup->saveComboBoxHistory(InterfaceComboBox);
    m_pSetup->saveComboBoxHistory(InDeviceComboBox);
    m_pSetup->saveComboBoxHistory(OutDeviceComboBox);
    m_pSetup->saveComboBoxHistory(StartupScriptShellComboBox);
    m_pSetup->saveComboBoxHistory(PostStartupScriptShellComboBox);
    m_pSetup->saveComboBoxHistory(ShutdownScriptShellComboBox);
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


