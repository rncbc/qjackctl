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

    // Set dialog validators...
    PresetComboBox->setValidator(new QRegExpValidator(QRegExp("[\\w]+"), PresetComboBox));
    ChanComboBox->setValidator(new QIntValidator(ChanComboBox));
    PriorityComboBox->setValidator(new QIntValidator(PriorityComboBox));
    FramesComboBox->setValidator(new QIntValidator(FramesComboBox));
    SampleRateComboBox->setValidator(new QIntValidator(SampleRateComboBox));
    PeriodsComboBox->setValidator(new QIntValidator(PeriodsComboBox));
    WaitComboBox->setValidator(new QIntValidator(WaitComboBox));
    TimeoutComboBox->setValidator(new QIntValidator(TimeoutComboBox));
    TimeRefreshComboBox->setValidator(new QIntValidator(TimeRefreshComboBox));
    StartDelayComboBox->setValidator(new QIntValidator(StartDelayComboBox));

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

    // Load combo box history...
    m_pSetup->loadComboBoxHistory(ServerComboBox);
    m_pSetup->loadComboBoxHistory(InterfaceComboBox);
    m_pSetup->loadComboBoxHistory(StartupScriptShellComboBox);
    m_pSetup->loadComboBoxHistory(PostStartupScriptShellComboBox);
    m_pSetup->loadComboBoxHistory(ShutdownScriptShellComboBox);
    m_pSetup->loadComboBoxHistory(XrunRegexComboBox);
    m_pSetup->loadComboBoxHistory(ActivePatchbayPathComboBox);

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

    // Other misc options...
    StartJackCheckBox->setChecked(m_pSetup->bStartJack);
    QueryCloseCheckBox->setChecked(m_pSetup->bQueryClose);
    KeepOnTopCheckBox->setChecked(m_pSetup->bKeepOnTop);

    // Finally, load preset list...
    m_iDirtySetup++;
    resetPresets();
    PresetComboBox->setCurrentText(m_pSetup->sDefPreset);
    m_iDirtySetup--;

    // Finally, load default settings...
    changePreset(PresetComboBox->currentText());
}


void qjackctlSetupForm::changePreset( const QString& sPreset )
{
    if (m_iDirtySetup > 0)
        return;

    // Load Settings...
    qjackctlPreset preset;
    if (m_pSetup->loadPreset(preset, sPreset)) {
        ServerComboBox->setCurrentText(preset.sServer);
        RealtimeCheckBox->setChecked(preset.bRealtime);
        SoftModeCheckBox->setChecked(preset.bSoftMode);
        MonitorCheckBox->setChecked(preset.bMonitor);
        ShortsCheckBox->setChecked(preset.bShorts);
        ChanComboBox->setCurrentText(QString::number(preset.iChan));
        PriorityComboBox->setCurrentText(QString::number(preset.iPriority));
        FramesComboBox->setCurrentText(QString::number(preset.iFrames));
        SampleRateComboBox->setCurrentText(QString::number(preset.iSampleRate));
        PeriodsComboBox->setCurrentText(QString::number(preset.iPeriods));
        WaitComboBox->setCurrentText(QString::number(preset.iWait));
        DriverComboBox->setCurrentText(preset.sDriver);
        InterfaceComboBox->setCurrentText(preset.sInterface);
        AudioComboBox->setCurrentItem(preset.iAudio);
        DitherComboBox->setCurrentItem(preset.iDither);
        TimeoutComboBox->setCurrentText(QString::number(preset.iTimeout));
        HWMonCheckBox->setChecked(preset.bHWMon);
        HWMeterCheckBox->setChecked(preset.bHWMeter);
        InChannelsSpinBox->setValue(preset.iInChannels);
        OutChannelsSpinBox->setValue(preset.iOutChannels);
        StartDelayComboBox->setCurrentText(QString::number(preset.iStartDelay));
        VerboseCheckBox->setChecked(preset.bVerbose);
    }
    stabilizeForm();
}


void qjackctlSetupForm::savePreset (void)
{
    QString sPreset = PresetComboBox->currentText();
    if (sPreset.isEmpty())
        return;

    // Unload settings.
    qjackctlPreset preset;
    preset.sServer      = ServerComboBox->currentText();
    preset.bRealtime    = RealtimeCheckBox->isChecked();
    preset.bSoftMode    = SoftModeCheckBox->isChecked();
    preset.bMonitor     = MonitorCheckBox->isChecked();
    preset.bShorts      = ShortsCheckBox->isChecked();
    preset.iChan        = ChanComboBox->currentText().toInt();
    preset.iPriority    = PriorityComboBox->currentText().toInt();
    preset.iFrames      = FramesComboBox->currentText().toInt();
    preset.iSampleRate  = SampleRateComboBox->currentText().toInt();
    preset.iPeriods     = PeriodsComboBox->currentText().toInt();
    preset.iWait        = WaitComboBox->currentText().toInt();
    preset.sDriver      = DriverComboBox->currentText();
    preset.sInterface   = InterfaceComboBox->currentText();
    preset.iAudio       = AudioComboBox->currentItem();
    preset.iDither      = DitherComboBox->currentItem();
    preset.iTimeout     = TimeoutComboBox->currentText().toInt();
    preset.bHWMon       = HWMonCheckBox->isChecked();
    preset.bHWMeter     = HWMeterCheckBox->isChecked();
    preset.iInChannels  = InChannelsSpinBox->value();
    preset.iOutChannels = OutChannelsSpinBox->value();
    preset.iStartDelay  = StartDelayComboBox->currentText().toInt();
    preset.bVerbose     = VerboseCheckBox->isChecked();
    m_pSetup->savePreset(preset, sPreset);

    m_iDirtySetup++;
    resetPresets();
    PresetComboBox->setCurrentText(sPreset);
    m_iDirtySetup--;

    stabilizeForm();
}


void qjackctlSetupForm::deletePreset (void)
{
    QString sPreset = PresetComboBox->currentText();
    if (sPreset.isEmpty())
        return;

    // Remove current preset item...
    m_iDirtySetup++;
    int iItem = PresetComboBox->currentItem();
    m_pSetup->deletePreset(sPreset);
    resetPresets();
    PresetComboBox->setCurrentItem(iItem);
    m_iDirtySetup--;

    // Load a new preset...
    changePreset(PresetComboBox->currentText());
    stabilizeForm();
}


void qjackctlSetupForm::resetPresets (void)
{
    PresetComboBox->clear();
    PresetComboBox->insertStringList(m_pSetup->presets);
    PresetComboBox->insertItem("(default)");
}


void qjackctlSetupForm::changeLatency ( const QString& )
{
    computeLatency();
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
}

void qjackctlSetupForm::changeDriver ( const QString& sDriver )
{
    bool bDummy     = (sDriver == "dummy");
    bool bAlsa      = (sDriver == "alsa");
    bool bPortaudio = (sDriver == "portaudio");

    SoftModeCheckBox->setEnabled(bAlsa);
    MonitorCheckBox->setEnabled(bAlsa);
    ShortsCheckBox->setEnabled(bAlsa);

    ChanTextLabel->setEnabled(bPortaudio);
    ChanComboBox->setEnabled(bPortaudio);

    PeriodsTextLabel->setEnabled(bAlsa);
    PeriodsComboBox->setEnabled(bAlsa);

    WaitTextLabel->setEnabled(bDummy);
    WaitComboBox->setEnabled(bDummy);

    InterfaceTextLabel->setEnabled(bAlsa);
    InterfaceComboBox->setEnabled(bAlsa);

    DitherTextLabel->setEnabled(!bDummy);
    DitherComboBox->setEnabled(!bDummy);

    HWMonCheckBox->setEnabled(bAlsa);
    HWMeterCheckBox->setEnabled(bAlsa);

    InChannelsTextLabel->setEnabled(bAlsa);
    InChannelsSpinBox->setEnabled(bAlsa);
    OutChannelsTextLabel->setEnabled(bAlsa);
    OutChannelsSpinBox->setEnabled(bAlsa);

    computeLatency();
}


// Stabilize current form state.
void qjackctlSetupForm::stabilizeForm (void)
{
    QString sPreset = PresetComboBox->currentText();
    if (!sPreset.isEmpty()) {
        PresetSavePushButton->setEnabled(true);
        PresetDeletePushButton->setEnabled(m_pSetup->presets.find(sPreset) != m_pSetup->presets.end());
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
    XrunIgnoreFirstCheckBox->setChecked(bEnabled);

    bEnabled = ActivePatchbayCheckBox->isChecked();
    ActivePatchbayPathComboBox->setEnabled(bEnabled);
    ActivePatchbayPathPushButton->setEnabled(bEnabled);

    TimeRefreshComboBox->setEnabled(AutoRefreshCheckBox->isChecked());
    
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
        if (iMetaChar >= 0)
            pLineEdit->insert("%" + sText[iMetaChar + 1]);
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
    }
}


// Accept settings (OK button slot).
void qjackctlSetupForm::accept (void)
{
    // Save preset list.
    m_pSetup->sDefPreset = PresetComboBox->currentText();

    // Save current settings...
    savePreset();

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
    m_pSetup->iTimeDisplay   = TimeDisplayButtonGroup->id(TimeDisplayButtonGroup->selected());
    m_pSetup->iTimeFormat    = TimeFormatComboBox->currentItem();
    m_pSetup->sMessagesFont  = MessagesFontTextLabel->font().toString();
    m_pSetup->sDisplayFont1  = DisplayFont1TextLabel->font().toString();
    m_pSetup->sDisplayFont2  = DisplayFont2TextLabel->font().toString();
    m_pSetup->bStartJack     = StartJackCheckBox->isChecked();
    m_pSetup->bQueryClose    = QueryCloseCheckBox->isChecked();
    m_pSetup->bKeepOnTop     = KeepOnTopCheckBox->isChecked();

    // Save combobox history...
    m_pSetup->saveComboBoxHistory(ServerComboBox);
    m_pSetup->saveComboBoxHistory(PriorityComboBox);
    m_pSetup->saveComboBoxHistory(InterfaceComboBox);
    m_pSetup->saveComboBoxHistory(StartupScriptShellComboBox);
    m_pSetup->saveComboBoxHistory(PostStartupScriptShellComboBox);
    m_pSetup->saveComboBoxHistory(ShutdownScriptShellComboBox);
    m_pSetup->saveComboBoxHistory(XrunRegexComboBox);
    m_pSetup->saveComboBoxHistory(ActivePatchbayPathComboBox);

    // Just go with dialog acceptance.
    QDialog::accept();
}

// Reject settings (Cancel button slot).
void qjackctlSetupForm::reject (void)
{
    QDialog::reject();
}


// end of qjackctlSetupForm.ui.h


