// qjackctlSetupForm.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
/****************************************************************************
   Copyright (C) 2003, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "config.h"


// Kind of constructor.
void qjackctlSetupForm::init (void)
{
    // Set dialog validators...
    ChanComboBox->setValidator(new QIntValidator(ChanComboBox));
    PriorityComboBox->setValidator(new QIntValidator(PriorityComboBox));
    FramesComboBox->setValidator(new QIntValidator(FramesComboBox));
    SampleRateComboBox->setValidator(new QIntValidator(SampleRateComboBox));
    PeriodsComboBox->setValidator(new QIntValidator(PeriodsComboBox));
    WaitComboBox->setValidator(new QIntValidator(WaitComboBox));
    TimeoutComboBox->setValidator(new QIntValidator(TimeoutComboBox));
    TimeRefreshComboBox->setValidator(new QIntValidator(TimeRefreshComboBox));

    // Try to restore old window positioning.
    adjustSize();

    // Final startup stabilization...
    stabilizeForm();
}


// Kind of destructor.
void qjackctlSetupForm::destroy (void)
{
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

void qjackctlSetupForm::stabilizeForm (void)
{
    bool bEnabled = RealtimeCheckBox->isChecked();
    PriorityTextLabel->setEnabled(bEnabled);
    PriorityComboBox->setEnabled(bEnabled);
    
    bEnabled = StartupScriptCheckBox->isChecked();
    StartupScriptShellComboBox->setEnabled(bEnabled);
    StartupScriptPushButton->setEnabled(bEnabled);
    
    bEnabled = PostStartupScriptCheckBox->isChecked();
    PostStartupScriptShellComboBox->setEnabled(bEnabled);
    PostStartupScriptPushButton->setEnabled(bEnabled);
    
    bEnabled = ShutdownScriptCheckBox->isChecked();
    ShutdownScriptShellComboBox->setEnabled(bEnabled);
    ShutdownScriptPushButton->setEnabled(bEnabled);

    bEnabled = ActivePatchbayCheckBox->isChecked();
    ActivePatchbayPathComboBox->setEnabled(bEnabled);
    ActivePatchbayPathPushButton->setEnabled(bEnabled);

    TimeRefreshComboBox->setEnabled(AutoRefreshCheckBox->isChecked());
    
    changeDriver(DriverComboBox->currentText());
}


// Startup script browse slot.
void qjackctlSetupForm::browseStartupScript()
{
    QString sFileName = QFileDialog::getOpenFileName(
            StartupScriptShellComboBox->currentText(),	// Start here.
            QString::null, 								// Filter (all files?)
            this, 0,									// Parent and name (none)
            tr("Startup script")						// Caption.
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
            tr("Post-startup script")				        // Caption.
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
            ShutdownScriptShellComboBox->currentText(),	// Start here.
            QString::null, 								// Filter (all files?)
            this, 0,									// Parent and name (none)
            tr("Shutdown script")						// Caption.
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


// Populate (load) dialog controls from setup struct members.
void qjackctlSetupForm::load ( qjackctlSetup *pSetup )
{
    // Load combo box history...
    pSetup->settings.beginGroup("/History");
    pSetup->loadComboBoxHistory(ServerComboBox);
    pSetup->loadComboBoxHistory(InterfaceComboBox);
    pSetup->loadComboBoxHistory(StartupScriptShellComboBox);
    pSetup->loadComboBoxHistory(PostStartupScriptShellComboBox);
    pSetup->loadComboBoxHistory(ShutdownScriptShellComboBox);
    pSetup->loadComboBoxHistory(XrunRegexComboBox);
    pSetup->loadComboBoxHistory(ActivePatchbayPathComboBox);
    pSetup->settings.endGroup();

    // Load Settings...
    ServerComboBox->setCurrentText(pSetup->sServer);
    RealtimeCheckBox->setChecked(pSetup->bRealtime);
    SoftModeCheckBox->setChecked(pSetup->bSoftMode);
    MonitorCheckBox->setChecked(pSetup->bMonitor);
    ShortsCheckBox->setChecked(pSetup->bShorts);
    ChanComboBox->setCurrentText(QString::number(pSetup->iChan));
    PriorityComboBox->setCurrentText(QString::number(pSetup->iPriority));
    FramesComboBox->setCurrentText(QString::number(pSetup->iFrames));
    SampleRateComboBox->setCurrentText(QString::number(pSetup->iSampleRate));
    PeriodsComboBox->setCurrentText(QString::number(pSetup->iPeriods));
    WaitComboBox->setCurrentText(QString::number(pSetup->iWait));
    DriverComboBox->setCurrentText(pSetup->sDriver);
    InterfaceComboBox->setCurrentText(pSetup->sInterface);
    AudioComboBox->setCurrentItem(pSetup->iAudio);
    DitherComboBox->setCurrentItem(pSetup->iDither);
    TimeoutComboBox->setCurrentText(QString::number(pSetup->iTimeout));
    HWMonCheckBox->setChecked(pSetup->bHWMon);
    HWMeterCheckBox->setChecked(pSetup->bHWMeter);
    InChannelsSpinBox->setValue(pSetup->iInChannels);
    OutChannelsSpinBox->setValue(pSetup->iOutChannels);
    VerboseCheckBox->setChecked(pSetup->bVerbose);

    // Load Options...
    StartupScriptCheckBox->setChecked(pSetup->bStartupScript);
    StartupScriptShellComboBox->setCurrentText(pSetup->sStartupScriptShell);
    PostStartupScriptCheckBox->setChecked(pSetup->bPostStartupScript);
    PostStartupScriptShellComboBox->setCurrentText(pSetup->sPostStartupScriptShell);
    ShutdownScriptCheckBox->setChecked(pSetup->bShutdownScript);
    ShutdownScriptShellComboBox->setCurrentText(pSetup->sShutdownScriptShell);
    XrunRegexComboBox->setCurrentText(pSetup->sXrunRegex);
    XrunIgnoreFirstCheckBox->setChecked(pSetup->bXrunIgnoreFirst);
    ActivePatchbayCheckBox->setChecked(pSetup->bActivePatchbay);
    ActivePatchbayPathComboBox->setCurrentText(pSetup->sActivePatchbayPath);
    AutoRefreshCheckBox->setChecked(pSetup->bAutoRefresh);
    TimeRefreshComboBox->setCurrentText(QString::number(pSetup->iTimeRefresh));

    // Load Defaults...
    TimeDisplayButtonGroup->setButton(pSetup->iTimeDisplay);
    QFont font;
    if (pSetup->sMessagesFont.isEmpty() || !font.fromString(pSetup->sMessagesFont))
        font = QFont("Terminal", 8);
    MessagesFontTextLabel->setFont(font);
    MessagesFontTextLabel->setText(font.family() + " " + QString::number(font.pointSize()));

    // Othet options finally...
    QueryCloseCheckBox->setChecked(pSetup->bQueryClose);
}


// Populate (save) setup struct members from dialog controls.
void qjackctlSetupForm::save ( qjackctlSetup *pSetup )
{
    // Save Settings...
    pSetup->sServer      = ServerComboBox->currentText();
    pSetup->bRealtime    = RealtimeCheckBox->isChecked();
    pSetup->bSoftMode    = SoftModeCheckBox->isChecked();
    pSetup->bMonitor     = MonitorCheckBox->isChecked();
    pSetup->bShorts      = ShortsCheckBox->isChecked();
    pSetup->iChan        = ChanComboBox->currentText().toInt();
    pSetup->iPriority    = PriorityComboBox->currentText().toInt();
    pSetup->iFrames      = FramesComboBox->currentText().toInt();
    pSetup->iSampleRate  = SampleRateComboBox->currentText().toInt();
    pSetup->iPeriods     = PeriodsComboBox->currentText().toInt();
    pSetup->iWait        = WaitComboBox->currentText().toInt();
    pSetup->sDriver      = DriverComboBox->currentText();
    pSetup->sInterface   = InterfaceComboBox->currentText();
    pSetup->iAudio       = AudioComboBox->currentItem();
    pSetup->iDither      = DitherComboBox->currentItem();
    pSetup->iTimeout     = TimeoutComboBox->currentText().toInt();
    pSetup->bHWMon       = HWMonCheckBox->isChecked();
    pSetup->bHWMeter     = HWMeterCheckBox->isChecked();
    pSetup->iInChannels  = InChannelsSpinBox->value();
    pSetup->iOutChannels = OutChannelsSpinBox->value();
    pSetup->bVerbose     = VerboseCheckBox->isChecked();

    // Save Options...
    pSetup->bStartupScript          = StartupScriptCheckBox->isChecked();
    pSetup->sStartupScriptShell     = StartupScriptShellComboBox->currentText();
    pSetup->bPostStartupScript      = PostStartupScriptCheckBox->isChecked();
    pSetup->sPostStartupScriptShell = PostStartupScriptShellComboBox->currentText();
    pSetup->bShutdownScript         = ShutdownScriptCheckBox->isChecked();
    pSetup->sShutdownScriptShell    = ShutdownScriptShellComboBox->currentText();
    pSetup->sXrunRegex              = XrunRegexComboBox->currentText();
    pSetup->bXrunIgnoreFirst        = XrunIgnoreFirstCheckBox->isChecked();
    pSetup->bActivePatchbay         = ActivePatchbayCheckBox->isChecked();
    pSetup->sActivePatchbayPath     = ActivePatchbayPathComboBox->currentText();
    pSetup->bAutoRefresh            = AutoRefreshCheckBox->isChecked();
    pSetup->iTimeRefresh            = TimeRefreshComboBox->currentText().toInt();

    // Save Defaults...
    pSetup->iTimeDisplay  = TimeDisplayButtonGroup->id(TimeDisplayButtonGroup->selected());
    pSetup->sMessagesFont = MessagesFontTextLabel->font().toString();
    pSetup->bQueryClose   = QueryCloseCheckBox->isChecked();

    // Save combobox history...
    pSetup->settings.beginGroup("/History");
    pSetup->saveComboBoxHistory(ServerComboBox);
    pSetup->saveComboBoxHistory(PriorityComboBox);
    pSetup->saveComboBoxHistory(InterfaceComboBox);
    pSetup->saveComboBoxHistory(StartupScriptShellComboBox);
    pSetup->saveComboBoxHistory(PostStartupScriptShellComboBox);
    pSetup->saveComboBoxHistory(ShutdownScriptShellComboBox);
    pSetup->saveComboBoxHistory(XrunRegexComboBox);
    pSetup->saveComboBoxHistory(ActivePatchbayPathComboBox);
    pSetup->settings.endGroup();
}


// end of qjackctlSetupForm.ui.h
