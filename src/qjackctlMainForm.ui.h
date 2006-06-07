// qjackctlMainForm.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
/****************************************************************************
   Copyright (C) 2003-2006, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qjackctlAbout.h"
#include "qjackctlStatus.h"

#include "qjackctlPatchbayFile.h"

#include <qapplication.h>
#include <qobjectlist.h>
#include <qeventloop.h>
#include <qmessagebox.h>
#include <qregexp.h>
#include <qtimer.h>

#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef CONFIG_JACK_STATISTICS
#include <jack/statistics.h>
#endif

// Timer constant stuff.
#define QJACKCTL_TIMER_MSECS    200

// Status refresh cycle (~2 secs)
#define QJACKCTL_STATUS_CYCLE   10

// Server display enumerated states.
#define QJACKCTL_INACTIVE       0
#define QJACKCTL_ACTIVATING     1
#define QJACKCTL_ACTIVE         2
#define QJACKCTL_STARTING       3
#define QJACKCTL_STARTED        4
#define QJACKCTL_STOPPING       5
#define QJACKCTL_STOPPED        6

// Notification pipes descriptors
#define QJACKCTL_FDNIL         -1
#define QJACKCTL_FDREAD         0
#define QJACKCTL_FDWRITE        1

static int g_fdStdout[2] = { QJACKCTL_FDNIL, QJACKCTL_FDNIL };

// Custom event types.
#define QJACKCTL_PORT_EVENT     1000
#define QJACKCTL_XRUN_EVENT     1001
#define QJACKCTL_BUFF_EVENT     1002
#define QJACKCTL_SHUT_EVENT     1003

// To have clue about current buffer size (in frames).
static jack_nframes_t g_nframes = 0;

// Kind of constructor.
void qjackctlMainForm::init (void)
{
    m_pSetup = NULL;

    m_iServerState = QJACKCTL_INACTIVE;

    m_pJack         = NULL;
    m_pJackClient   = NULL;
    m_bJackDetach   = false;
    m_bJackSurvive  = false;
    m_pAlsaSeq      = NULL;
    m_iStartDelay   = 0;
    m_iTimerDelay   = 0;
    m_iTimerRefresh = 0;
    m_iJackRefresh  = 0;
    m_iAlsaRefresh  = 0;
    m_iJackDirty    = 0;
    m_iAlsaDirty    = 0;

    m_iStatusRefresh = 0;

    m_iPatchbayRefresh = 0;

    m_pStdoutNotifier = NULL;

    m_pAlsaNotifier = NULL;

    // All forms are to be created later on setup.
    m_pMessagesForm    = NULL;
    m_pStatusForm      = NULL;
    m_pConnectionsForm = NULL;
    m_pPatchbayForm    = NULL;

    // The eventual system tray widget.
    m_pSystemTray  = NULL;

    // Working presets popup menu.
    m_pPresetsMenu = NULL;

    // We're not quitting so early :)
    m_bQuitForce = false;

	// Transport skip accelerate factor.
	m_fSkipAccel = 1.0;

#ifdef HAVE_SIGNAL_H
	// Set to ignore any fatal "Broken pipe" signals.
	signal(SIGPIPE, SIG_IGN);
#endif
}


// Kind of destructor.
void qjackctlMainForm::destroy (void)
{
	if (m_bJackDetach)
		m_bJackSurvive = true;

    // Stop server, if not already...
    stopJackServer();

    // Terminate local ALSA sequencer interface.
    if (m_pAlsaNotifier)
        delete m_pAlsaNotifier;
#ifdef CONFIG_ALSA_SEQ
    if (m_pAlsaSeq)
        snd_seq_close(m_pAlsaSeq);
#endif
    m_pAlsaNotifier = NULL;
    m_pAlsaSeq = NULL;

    // Finally drop any popup widgets around...
    if (m_pMessagesForm)
        delete m_pMessagesForm;
    if (m_pStatusForm)
        delete m_pStatusForm;
    if (m_pConnectionsForm)
        delete m_pConnectionsForm;
    if (m_pPatchbayForm)
        delete m_pPatchbayForm;

    // Quit off system tray widget.
    if (m_pSystemTray)
        delete m_pSystemTray;
}


// Make and set a proper setup step.
bool qjackctlMainForm::setup ( qjackctlSetup *pSetup )
{
    // Finally, fix settings descriptor
    // and stabilize the form.
    m_pSetup = pSetup;

    // To avoid any background flickering,
    // we'll hide the main display.
    StatusDisplayFrame->hide();
	updateButtons();

    // What style do we create these forms?
    WFlags wflags = Qt::WType_TopLevel;
    if (m_pSetup->bKeepOnTop)
        wflags |= Qt::WStyle_Tool;
    // All forms are to be created right now.
    m_pMessagesForm    = new qjackctlMessagesForm    (this, 0, wflags);
    m_pStatusForm      = new qjackctlStatusForm      (this, 0, wflags);
    m_pConnectionsForm = new qjackctlConnectionsForm (this, 0, wflags);
    m_pPatchbayForm    = new qjackctlPatchbayForm    (this, 0, wflags);
	// Setup appropriately...
	m_pConnectionsForm->setup(m_pSetup);
	m_pPatchbayForm->setup(m_pSetup);

    // Set the patchbay cable connection notification signal/slot.
    QObject::connect(&m_patchbayRack, SIGNAL(cableConnected(const QString&, const QString&, unsigned int)),
        this, SLOT(cableConnectSlot(const QString&, const QString&, unsigned int)));

    // Try to restore old window positioning and appearence.
    m_pSetup->loadWidgetGeometry(this);

    // Make it final show...
    StatusDisplayFrame->show();

    // Set other defaults...
    updateDisplayEffect();
    updateTimeDisplayFonts();
    updateTimeDisplayToolTips();
    updateTimeFormat();
    updateMessagesFont();
    updateMessagesLimit();
    updateConnectionsFont();
    updateConnectionsIconSize();
    updateBezierLines();
    updateActivePatchbay();
    updateSystemTray();

    // And for the whole widget gallore...
    m_pSetup->loadWidgetGeometry(m_pMessagesForm);
    m_pSetup->loadWidgetGeometry(m_pStatusForm);
    m_pSetup->loadWidgetGeometry(m_pConnectionsForm);
    m_pSetup->loadWidgetGeometry(m_pPatchbayForm);

    // Initial XRUN statistics reset.
    resetXrunStats();

    // Check if we can redirect our own stdout/stderr...
    if (m_pSetup->bStdoutCapture && ::pipe(g_fdStdout) == 0) {
        ::dup2(g_fdStdout[QJACKCTL_FDWRITE], STDOUT_FILENO);
        ::dup2(g_fdStdout[QJACKCTL_FDWRITE], STDERR_FILENO);
        m_pStdoutNotifier = new QSocketNotifier(g_fdStdout[QJACKCTL_FDREAD], QSocketNotifier::Read, this);
        QObject::connect(m_pStdoutNotifier, SIGNAL(activated(int)), this, SLOT(stdoutNotifySlot(int)));
    }

#ifdef CONFIG_ALSA_SEQ
    // Start our ALSA sequencer interface.
    if (snd_seq_open(&m_pAlsaSeq, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0)
        m_pAlsaSeq = NULL;
    if (m_pAlsaSeq) {
        snd_seq_port_subscribe_t *pAlsaSubs;
        snd_seq_addr_t seq_addr;
        int iPort;
        struct pollfd pfd[1];
        iPort = snd_seq_create_simple_port(
            m_pAlsaSeq,
            "qjackctl",
            SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_NO_EXPORT,
            SND_SEQ_PORT_TYPE_APPLICATION
        );
        if (iPort >= 0) {
            snd_seq_port_subscribe_alloca(&pAlsaSubs);
            seq_addr.client = SND_SEQ_CLIENT_SYSTEM;
            seq_addr.port   = SND_SEQ_PORT_SYSTEM_ANNOUNCE;
            snd_seq_port_subscribe_set_sender(pAlsaSubs, &seq_addr);
            seq_addr.client = snd_seq_client_id(m_pAlsaSeq);
            seq_addr.port   = iPort;
            snd_seq_port_subscribe_set_dest(pAlsaSubs, &seq_addr);
            snd_seq_subscribe_port(m_pAlsaSeq, pAlsaSubs);
            snd_seq_poll_descriptors(m_pAlsaSeq, pfd, 1, POLLIN);
            m_pAlsaNotifier = new QSocketNotifier(pfd[0].fd, QSocketNotifier::Read);
            QObject::connect(m_pAlsaNotifier, SIGNAL(activated(int)), this, SLOT(alsaNotifySlot(int)));
        }
    }
    // Could we start without it?
    if (m_pAlsaSeq == NULL) {
        appendMessagesError(tr("Could not open ALSA sequencer as a client.\n\nMIDI patchbay will be not available."));
    } else {
        // Rather obvious setup.
        if (m_pConnectionsForm)
            m_pConnectionsForm->setAlsaSeq(m_pAlsaSeq);
        if (m_pPatchbayForm)
            m_pPatchbayForm->setAlsaSeq(m_pAlsaSeq);
    }
#endif

    // Load patchbay form recent paths...
    if (m_pPatchbayForm) {
		m_pPatchbayForm->setRecentPatchbays(m_pSetup->patchbays);
	 	if (!m_pSetup->sPatchbayPath.isEmpty())
			m_pPatchbayForm->loadPatchbayFile(m_pSetup->sPatchbayPath);
	}
	
    // Try to find if we can start in detached mode (client-only)
    // just in case there's a JACK server already running.
    startJackClient(true);
    // Final startup stabilization...
    stabilizeForm();
    processJackExit();

    // Look for immediate server startup?...
    if (m_pSetup->bStartJack || !m_pSetup->sCmdLine.isEmpty())
        startJack();

    // Register the first timer slot.
    QTimer::singleShot(QJACKCTL_TIMER_MSECS, this, SLOT(timerSlot()));

    // We're ready to go...
    return true;
}


// Window close event handlers.
bool qjackctlMainForm::queryClose (void)
{
    bool bQueryClose = true;

#ifdef CONFIG_SYSTEM_TRAY
    // If we're not quitting explicitly and there's an
    // active system tray icon, then just hide ourselves.
	if (!m_bQuitForce && isVisible()
		&& m_pSetup->bSystemTray && m_pSystemTray && m_pJackClient) {
        m_pSetup->saveWidgetGeometry(this);
        hide();
        bQueryClose = false;
    }
#endif

    // Check if JACK daemon is currently running...
	if (bQueryClose && m_pJack && m_pJack->isRunning()
		&& (m_pSetup->bQueryClose || m_pSetup->bQueryShutdown)) {
        switch (QMessageBox::warning(this,
			tr("Warning") + " - " QJACKCTL_SUBTITLE1,
            tr("JACK is currently running.") + "\n\n" +
            tr("Do you want to terminate the JACK audio server?"),
            tr("Terminate"), tr("Leave"), tr("Cancel"))) {
          case 0:   // Terminate...
            m_bJackSurvive = false;
            break;
          case 1:   // Leave...
            m_bJackSurvive = true;
            break;
          default:  // Cancel.
            bQueryClose = false;
            break;
        }
    }

    // Try to save current aliases default settings.
    if (bQueryClose && m_pConnectionsForm)
        bQueryClose = m_pConnectionsForm->queryClose();

    // Try to save current patchbay default settings.
    if (bQueryClose && m_pPatchbayForm) {
        bQueryClose = m_pPatchbayForm->queryClose();
        if (bQueryClose && !m_pPatchbayForm->patchbayPath().isEmpty())
            m_pSetup->sPatchbayPath = m_pPatchbayForm->patchbayPath();
    }

    // Some windows default fonts are here on demand too.
    if (bQueryClose && m_pMessagesForm)
        m_pSetup->sMessagesFont = m_pMessagesForm->messagesFont().toString();

    // Try to save current positioning.
    if (bQueryClose) {
        m_pSetup->saveWidgetGeometry(m_pMessagesForm);
        m_pSetup->saveWidgetGeometry(m_pStatusForm);
        m_pSetup->saveWidgetGeometry(m_pConnectionsForm);
        m_pSetup->saveWidgetGeometry(m_pPatchbayForm);
        m_pSetup->saveWidgetGeometry(this);
        // Close popup widgets.
        if (m_pMessagesForm)
            m_pMessagesForm->close();
        if (m_pStatusForm)
            m_pStatusForm->close();
        if (m_pConnectionsForm)
            m_pConnectionsForm->close();
        if (m_pPatchbayForm)
            m_pPatchbayForm->close();
        // And the system tray icon too.
        if (m_pSystemTray)
            m_pSystemTray->close();
    }

#ifdef CONFIG_SYSTEM_TRAY
    // Whether we're really quitting.
    m_bQuitForce = bQueryClose;
#endif

    return bQueryClose;
}


void qjackctlMainForm::closeEvent ( QCloseEvent *pCloseEvent )
{
    if (queryClose())
        pCloseEvent->accept();
    else
        pCloseEvent->ignore();
}


void qjackctlMainForm::customEvent ( QCustomEvent *pCustomEvent )
{
    switch (pCustomEvent->type()) {
      case QJACKCTL_PORT_EVENT:
        portNotifyEvent();
        break;
      case QJACKCTL_XRUN_EVENT:
        xrunNotifyEvent();
        break;
      case QJACKCTL_BUFF_EVENT:
        buffNotifyEvent();
        break;
      case QJACKCTL_SHUT_EVENT:
        shutNotifyEvent();
        break;
      default:
        QWidget::customEvent(pCustomEvent);
        break;
    }
}


// Common exit status text formatter...
QString qjackctlMainForm::formatExitStatus ( int iExitStatus )
{
    QString sTemp = " ";

    if (iExitStatus == 0)
        sTemp += tr("successfully");
    else
        sTemp += tr("with exit status=%1").arg(iExitStatus);

    return sTemp + ".";
}


// Common shell script executive, with placeholder substitution...
void qjackctlMainForm::shellExecute ( const QString& sShellCommand, const QString& sStartMessage, const QString& sStopMessage )
{
    QString sTemp = sShellCommand;

    sTemp.replace("%P", m_pSetup->sDefPreset);

    sTemp.replace("%s", m_preset.sServer);
    sTemp.replace("%d", m_preset.sDriver);
    sTemp.replace("%i", m_preset.sInterface);
    sTemp.replace("%r", QString::number(m_preset.iSampleRate));
    sTemp.replace("%p", QString::number(m_preset.iFrames));
    sTemp.replace("%n", QString::number(m_preset.iPeriods));

    appendMessages(sStartMessage);
    appendMessagesColor(sTemp.stripWhiteSpace(), "#990099");
    QApplication::eventLoop()->processEvents(QEventLoop::ExcludeUserInput);
    // Execute and set exit status message...
    sTemp = sStopMessage + formatExitStatus(::system(sTemp));
   // Wait a litle bit before continue...
    QTime t;
    t.start();
    while (t.elapsed() < QJACKCTL_TIMER_MSECS)
        QApplication::eventLoop()->processEvents(QEventLoop::ExcludeUserInput);
    // Final log message...
    appendMessages(sTemp);
}


// Start jack audio server...
void qjackctlMainForm::startJack (void)
{
    // If can't be already a client, are we?
    if (m_pJackClient)
        return;

    // Is the server process instance still here?
    if (m_pJack) {
        switch (QMessageBox::warning(this,
			tr("Warning") + " - " QJACKCTL_SUBTITLE1,
            tr("Could not start JACK.") + "\n\n" +
            tr("Maybe JACK audio server is already started."),
            tr("Stop"), tr("Kill"), tr("Cancel"))) {
          case 0:
            m_pJack->tryTerminate();
            break;
          case 1:
            m_pJack->kill();
            break;
        }
        return;
    }

    // Stabilize emerging server state...
    updateServerState(QJACKCTL_ACTIVATING);
    ServerStateTextLabel->setPaletteForegroundColor(Qt::yellow);
    StartPushButton->setEnabled(false);

    // Reset our timer counters...
    m_iStartDelay  = 0;
    m_iTimerDelay  = 0;
    m_iJackRefresh = 0;

	// If we ain't to be the server master, maybe we'll start
	// detached as client only (jackd server already running?)
    if (startJackClient(true)) {
        StopPushButton->setEnabled(true);
        return;
    }

    // Now we're sure it ain't detached.
    updateServerState(QJACKCTL_STARTING);
	m_bJackDetach = false;

    // Load primary/default server preset...
    if (!m_pSetup->loadPreset(m_preset, m_pSetup->sDefPreset)) {
        appendMessagesError(tr("Could not load preset \"%1\".\n\nRetrying with default.").arg(m_pSetup->sDefPreset));
        m_pSetup->sDefPreset = m_pSetup->sDefPresetName;
        if (!m_pSetup->loadPreset(m_preset, m_pSetup->sDefPreset)) {
            appendMessagesError(tr("Could not load default preset.\n\nSorry."));
            processJackExit();
            return;
        }
    }

	// Do we have any startup script?...
	if (m_pSetup->bStartupScript
		&& !m_pSetup->sStartupScriptShell.isEmpty()) {
		shellExecute(m_pSetup->sStartupScriptShell,
			tr("Startup script..."),
			tr("Startup script terminated"));
	}

    // OK. Let's build the startup process...
    m_pJack = new QProcess(this);

    // Setup stdout/stderr capture...
    if (m_pSetup->bStdoutCapture) {
        m_pJack->setCommunication(QProcess::Stdout | QProcess::Stderr | QProcess::DupStderr);
        QObject::connect(m_pJack, SIGNAL(readyReadStdout()), this, SLOT(readJackStdout()));
        QObject::connect(m_pJack, SIGNAL(readyReadStderr()), this, SLOT(readJackStdout()));
    }
    // The unforgiveable signal communication...
    QObject::connect(m_pJack, SIGNAL(processExited()), this, SLOT(processJackExit()));

	// Split the server path into arguments...
	QStringList args = QStringList::split(' ', m_preset.sServer);
    // Look for the executable in the search path;
    // this enforces the server command to be an 
    // executable absolute path whenever possible.
    QString sCommand = args[0];
    if (!sCommand.contains('/')) {
        QStringList list = QStringList::split(':', ::getenv("PATH"));
        for (QStringList::Iterator iter = list.begin(); iter != list.end(); ++iter) {
            QString sDirectory = *iter;
            QFileInfo fileinfo(sDirectory + "/" + sCommand);
            if (fileinfo.isExecutable())
                sCommand = fileinfo.filePath();
       }
    }

    // Build process arguments...
	bool bDummy     = (m_preset.sDriver == "dummy");
	bool bOss       = (m_preset.sDriver == "oss");
	bool bAlsa      = (m_preset.sDriver == "alsa");
	bool bPortaudio = (m_preset.sDriver == "portaudio");
	bool bCoreaudio = (m_preset.sDriver == "coreaudio");
	bool bFreebob   = (m_preset.sDriver == "freebob");
	for (QStringList::Iterator iter = args.begin(); iter != args.end(); ++iter)
		m_pJack->addArgument(*iter);
    if (m_preset.bVerbose)
        m_pJack->addArgument("-v");
    if (m_preset.bRealtime) {
        m_pJack->addArgument("-R");
        if (m_preset.iPriority > 0 && !bCoreaudio)
            m_pJack->addArgument("-P" + QString::number(m_preset.iPriority));
    }
    if (m_preset.iPortMax > 0 && m_preset.iPortMax != 256)
        m_pJack->addArgument("-p" + QString::number(m_preset.iPortMax));
    if (m_preset.iTimeout > 0 && m_preset.iTimeout != 500)
        m_pJack->addArgument("-t" + QString::number(m_preset.iTimeout));
    if (m_preset.bNoMemLock)
        m_pJack->addArgument("-m");
    else if (m_preset.bUnlockMem)
        m_pJack->addArgument("-u");
    m_pJack->addArgument("-d" + m_preset.sDriver);
    if (bAlsa && (m_preset.iAudio != QJACKCTL_DUPLEX ||
		m_preset.sInDevice.isEmpty() || m_preset.sOutDevice.isEmpty())) {
		QString sInterface = m_preset.sInterface;
		if (sInterface.isEmpty())
			sInterface = "hw:0";
        m_pJack->addArgument("-d" + sInterface);
	}
    if (bPortaudio && m_preset.iChan > 0)
        m_pJack->addArgument("-c" + QString::number(m_preset.iChan));
    if ((bCoreaudio || bFreebob) && !m_preset.sInterface.isEmpty())
	    m_pJack->addArgument("-d" + m_preset.sInterface);
    if (m_preset.iSampleRate > 0)
        m_pJack->addArgument("-r" + QString::number(m_preset.iSampleRate));
    if (m_preset.iFrames > 0)
        m_pJack->addArgument("-p" + QString::number(m_preset.iFrames));
    if (bAlsa || bOss || bFreebob) {
        if (m_preset.iPeriods > 0)
            m_pJack->addArgument("-n" + QString::number(m_preset.iPeriods));
    }
    if (bAlsa) {
        switch (m_preset.iAudio) {
          case QJACKCTL_DUPLEX:
            if (!m_preset.sInDevice.isEmpty() || !m_preset.sOutDevice.isEmpty())
                m_pJack->addArgument("-D");
            if (!m_preset.sInDevice.isEmpty())
                m_pJack->addArgument("-C" + m_preset.sInDevice);
            if (!m_preset.sOutDevice.isEmpty())
                m_pJack->addArgument("-P" + m_preset.sOutDevice);
            break;
          case QJACKCTL_CAPTURE:
            m_pJack->addArgument("-C" + m_preset.sInDevice);
            break;
          case QJACKCTL_PLAYBACK:
            m_pJack->addArgument("-P" + m_preset.sOutDevice);
            break;
        }
        if (m_preset.bSoftMode)
            m_pJack->addArgument("-s");
        if (m_preset.bMonitor)
            m_pJack->addArgument("-m");
        if (m_preset.bShorts)
            m_pJack->addArgument("-S");
		if (m_preset.iInChannels > 0  && m_preset.iAudio != QJACKCTL_PLAYBACK)
			m_pJack->addArgument("-i" + QString::number(m_preset.iInChannels));
		if (m_preset.iOutChannels > 0 && m_preset.iAudio != QJACKCTL_CAPTURE)
			m_pJack->addArgument("-o" + QString::number(m_preset.iOutChannels));
    }
    else if (bOss) {
        if (m_preset.bIgnoreHW)
            m_pJack->addArgument("-b");
        if (m_preset.iWordLength > 0)
            m_pJack->addArgument("-w" + QString::number(m_preset.iWordLength));
        if (!m_preset.sInDevice.isEmpty()  && m_preset.iAudio != QJACKCTL_PLAYBACK)
            m_pJack->addArgument("-C" + m_preset.sInDevice);
        if (!m_preset.sOutDevice.isEmpty() && m_preset.iAudio != QJACKCTL_CAPTURE)
            m_pJack->addArgument("-P" + m_preset.sOutDevice);
        if (m_preset.iAudio == QJACKCTL_PLAYBACK)
            m_pJack->addArgument("-i0");
        else if (m_preset.iInChannels > 0)
            m_pJack->addArgument("-i" + QString::number(m_preset.iInChannels));
        if (m_preset.iAudio == QJACKCTL_CAPTURE)
            m_pJack->addArgument("-o0");
        else if (m_preset.iOutChannels > 0)
            m_pJack->addArgument("-o" + QString::number(m_preset.iOutChannels));
    }
	else if (bCoreaudio) {
		if (m_preset.iInChannels >= 0  && m_preset.iAudio != QJACKCTL_PLAYBACK)
			m_pJack->addArgument("-i" + QString::number(m_preset.iInChannels));
		if (m_preset.iOutChannels >= 0 && m_preset.iAudio != QJACKCTL_CAPTURE)
			m_pJack->addArgument("-o" + QString::number(m_preset.iOutChannels));
	}
	else if (bFreebob) {
		if (m_preset.iInChannels >= 0  && m_preset.iAudio != QJACKCTL_PLAYBACK)
			m_pJack->addArgument("-i" + QString::number(m_preset.iInChannels));
		else
			m_pJack->addArgument("-i0");
		if (m_preset.iOutChannels >= 0 && m_preset.iAudio != QJACKCTL_CAPTURE)
			m_pJack->addArgument("-o" + QString::number(m_preset.iOutChannels));
		else
			m_pJack->addArgument("-o0");
	}
    if (bDummy && m_preset.iWait > 0 && m_preset.iWait != 21333)
        m_pJack->addArgument("-w" + QString::number(m_preset.iWait));
    if (bAlsa || bPortaudio) {
        switch (m_preset.iDither) {
          case 0:
          //m_pJack->addArgument("-z-");
            break;
          case 1:
            m_pJack->addArgument("-zr");
            break;
          case 2:
            m_pJack->addArgument("-zs");
            break;
          case 3:
            m_pJack->addArgument("-zt");
            break;
        }
    }
    if (bAlsa) {
        if (m_preset.bHWMon)
            m_pJack->addArgument("-H");
        if (m_preset.bHWMeter)
            m_pJack->addArgument("-M");
    }
	if (bAlsa || bOss || bCoreaudio) {
		if (m_preset.iInLatency > 0)
			m_pJack->addArgument("-I" + QString::number(m_preset.iInLatency));
		if (m_preset.iOutLatency > 0)
			m_pJack->addArgument("-O" + QString::number(m_preset.iOutLatency));
	}

    appendMessages(tr("JACK is starting..."));
    m_sJackCmdLine = m_pJack->arguments().join(" ").stripWhiteSpace();
    appendMessagesColor(m_sJackCmdLine, "#990099");

    // Go jack, go...
    if (!m_pJack->start()) {
        appendMessagesError(tr("Could not start JACK.\n\nSorry."));
        processJackExit();
        return;
    }

    // Show startup results...
    appendMessages(tr("JACK was started with PID=%1 (0x%2).")
		.arg((long) m_pJack->processIdentifier())
		.arg(QString::number((long) m_pJack->processIdentifier(), 16)));

    // Sloppy boy fix: may the serve be stopped, just in case
    // the client will nerver make it...
    StopPushButton->setEnabled(true);

    // Make sure all status(es) will be updated ASAP...
    m_iStatusRefresh += QJACKCTL_STATUS_CYCLE;

    // Reset (yet again) the timer counters...
    m_iStartDelay  = 1 + (m_preset.iStartDelay * 1000);
    m_iTimerDelay  = 0;
    m_iJackRefresh = 0;

}


// Stop jack audio server...
void qjackctlMainForm::stopJack (void)
{
	// Check if we're allowed to stop (shutdown)...
	if (m_pSetup->bQueryShutdown
		&& m_pJack && m_pJack->isRunning() && !m_bJackSurvive
		&& m_pConnectionsForm && m_pConnectionsForm->isJackConnected()
		&& QMessageBox::warning(this,
			tr("Warning") + " - " QJACKCTL_SUBTITLE1,
			tr("Some client audio applications") + "\n" +
			tr("are still active and connected.") + "\n\n" +
			tr("Do you want to stop the JACK audio server?"),
			tr("Stop"), tr("Cancel")) > 0) {
		return;
	}
	
	// Stop the server unconditionally.
	stopJackServer();
}


// Stop jack audio server...
void qjackctlMainForm::stopJackServer (void)
{
    // Clear timer counters...
    m_iStartDelay  = 0;
    m_iTimerDelay  = 0;
    m_iJackRefresh = 0;

    // Stop client code.
    stopJackClient();

    // And try to stop server.
    if (m_pJack && m_pJack->isRunning() && !m_bJackSurvive) {
        appendMessages(tr("JACK is stopping..."));
        updateServerState(QJACKCTL_STOPPING);
		// Do we have any pre-shutdown script?...
		if (m_pSetup->bShutdownScript
			&& !m_pSetup->sShutdownScriptShell.isEmpty()) {
			shellExecute(m_pSetup->sShutdownScriptShell,
				tr("Shutdown script..."),
				tr("Shutdown script terminated"));
		}
        // Now it's the time to real try stopping the server daemon...
        m_pJack->tryTerminate();
        // Give it some time to terminate gracefully and stabilize...
        QTime t;
        t.start();
        while (t.elapsed() < QJACKCTL_TIMER_MSECS)
            QApplication::eventLoop()->processEvents(QEventLoop::ExcludeUserInput);
        // Keep on, if not exiting for good.
		return;
     }

	// If we have a post-shutdown script enabled it must be called,
	// despite we've not started the server before...
	if (m_bJackDetach && !m_bJackSurvive
		&& m_pSetup->bPostShutdownScript
		&& !m_pSetup->sPostShutdownScriptShell.isEmpty()) {
		shellExecute(m_pSetup->sPostShutdownScriptShell,
			tr("Post-shutdown script..."),
			tr("Post-shutdown script terminated"));
	}

     // Do final processing anyway.
     processJackExit();
}


// Stdout handler...
void qjackctlMainForm::readJackStdout (void)
{
    appendStdoutBuffer(m_pJack->readStdout());
}


// Stdout buffer handler -- now splitted by complete new-lines...
void qjackctlMainForm::appendStdoutBuffer ( const QString& s )
{
    m_sStdoutBuffer.append(s);

    int iLength = m_sStdoutBuffer.findRev('\n') + 1;
    if (iLength > 0) {
        QString sTemp = m_sStdoutBuffer.left(iLength);
        m_sStdoutBuffer.remove(0, iLength);
        QStringList list = QStringList::split('\n', sTemp, true);
        for (QStringList::Iterator iter = list.begin(); iter != list.end(); iter++)
            appendMessagesText(detectXrun(*iter));
    }
}


// Stdout flusher -- show up any unfinished line...
void qjackctlMainForm::flushStdoutBuffer (void)
{
    if (!m_sStdoutBuffer.isEmpty()) {
        appendMessagesText(detectXrun(m_sStdoutBuffer));
        m_sStdoutBuffer.truncate(0);
    }
}


// Jack audio server cleanup.
void qjackctlMainForm::processJackExit (void)
{
    // Force client code cleanup.
    if (!m_bJackDetach)
        stopJackClient();

    // Flush anything that maybe pending...
    flushStdoutBuffer();

    if (m_pJack) {
        // Force final server shutdown...
        if (!m_bJackSurvive) {
            appendMessages(tr("JACK was stopped") + formatExitStatus(m_pJack->exitStatus()));
            if (!m_pJack->normalExit())
                m_pJack->kill();
        }
        // Destroy it.
        delete m_pJack;
        m_pJack = NULL;
		// Do we have any post-shutdown script?...
		// (this will be always called, despite we've started the server or not)
		if (!m_bJackSurvive && m_pSetup->bPostShutdownScript
			&& !m_pSetup->sPostShutdownScriptShell.isEmpty()) {
			shellExecute(m_pSetup->sPostShutdownScriptShell,
				tr("Post-shutdown script..."),
				tr("Post-shutdown script terminated"));
		}
    }

    // Stabilize final server state...
    int iServerState;
    if (m_bJackDetach)
        iServerState = (m_pJackClient == NULL ? QJACKCTL_INACTIVE : QJACKCTL_ACTIVE);
    else
        iServerState = QJACKCTL_STOPPED;
    updateServerState(iServerState);
    ServerStateTextLabel->setPaletteForegroundColor(m_pJackClient == NULL ? Qt::darkYellow : Qt::yellow);
    StartPushButton->setEnabled(m_pJackClient == NULL);
    StopPushButton->setEnabled(m_pJackClient != NULL);
    RewindPushButton->setEnabled(false);
    BackwardPushButton->setEnabled(false);
    PlayPushButton->setEnabled(false);
    PausePushButton->setEnabled(false);
    ForwardPushButton->setEnabled(false);
}


// XRUN detection routine.
QString& qjackctlMainForm::detectXrun ( QString & s )
{
	if (m_iXrunSkips > 1)
		return s;
    QRegExp rx(m_pSetup->sXrunRegex);
    int iPos = rx.search(s);
    if (iPos >= 0) {
        s.insert(iPos + rx.matchedLength(), "</font>");
        s.insert(iPos, "<font color=\"#cc0000\">");
#ifndef CONFIG_JACK_XRUN_DELAY
        m_tXrunLast.restart();
        updateXrunStats(rx.cap(1).toFloat());
#endif
    }
    return s;
}


// Update the XRUN last delay and immediate statistical values (in msecs).
void qjackctlMainForm::updateXrunStats ( float fXrunLast )
{
    if (m_iXrunStats > 0 || !m_pSetup->bXrunIgnoreFirst) {
        m_fXrunLast   = fXrunLast;
        m_fXrunTotal += m_fXrunLast;
        if (m_fXrunLast < m_fXrunMin || m_iXrunCount == 0)
            m_fXrunMin = m_fXrunLast;
        if (m_fXrunLast > m_fXrunMax || m_iXrunCount == 0)
            m_fXrunMax = m_fXrunLast;
        m_iXrunCount++;
	//	refreshXrunStats();
    }
    m_iXrunStats++;
}


// Own stdout/stderr socket notifier slot.
void qjackctlMainForm::stdoutNotifySlot ( int fd )
{
    char achBuffer[1024];
    int  cchBuffer = ::read(fd, achBuffer, sizeof(achBuffer) - 1);
    if (cchBuffer > 0) {
        achBuffer[cchBuffer] = (char) 0;
        appendStdoutBuffer(achBuffer);
    }
}


// Messages output methods.
void qjackctlMainForm::appendMessages( const QString& s )
{
    if (m_pMessagesForm)
        m_pMessagesForm->appendMessages(s);
}

void qjackctlMainForm::appendMessagesColor( const QString& s, const QString& c )
{
    if (m_pMessagesForm)
        m_pMessagesForm->appendMessagesColor(s, c);
}

void qjackctlMainForm::appendMessagesText( const QString& s )
{
    if (m_pMessagesForm)
        m_pMessagesForm->appendMessagesText(s);
}

void qjackctlMainForm::appendMessagesError( const QString& s )
{
    if (m_pMessagesForm)
        m_pMessagesForm->show();

    appendMessagesColor(s.simplifyWhiteSpace(), "#ff0000");

    QMessageBox::critical(this,
		tr("Error") + " - " QJACKCTL_SUBTITLE1, s, tr("Cancel"));
}


// Force update of the messages font.
void qjackctlMainForm::updateMessagesFont (void)
{
    if (m_pSetup == NULL)
        return;

    if (m_pMessagesForm && !m_pSetup->sMessagesFont.isEmpty()) {
        QFont font;
        if (font.fromString(m_pSetup->sMessagesFont))
            m_pMessagesForm->setMessagesFont(font);
    }
}


// Update messages window line limit.
void qjackctlMainForm::updateMessagesLimit (void)
{
    if (m_pSetup == NULL)
        return;

    if (m_pMessagesForm) {
        if (m_pSetup->bMessagesLimit)
            m_pMessagesForm->setMessagesLimit(m_pSetup->iMessagesLimitLines);
        else
            m_pMessagesForm->setMessagesLimit(-1);
    }
}


// Force update of the connections font.
void qjackctlMainForm::updateConnectionsFont (void)
{
    if (m_pSetup == NULL)
        return;

    if (m_pConnectionsForm && !m_pSetup->sConnectionsFont.isEmpty()) {
        QFont font;
        if (font.fromString(m_pSetup->sConnectionsFont))
            m_pConnectionsForm->setConnectionsFont(font);
    }
}


// Update of the connections view icon size.
void qjackctlMainForm::updateConnectionsIconSize (void)
{
    if (m_pSetup == NULL)
        return;

    if (m_pConnectionsForm)
        m_pConnectionsForm->setConnectionsIconSize(m_pSetup->iConnectionsIconSize);
}


// Update the connection and patchbay line style.
void qjackctlMainForm::updateBezierLines (void)
{
    if (m_pSetup == NULL)
        return;

    if (m_pConnectionsForm) {
        m_pConnectionsForm->JackConnectView->setBezierLines(m_pSetup->bBezierLines);
        m_pConnectionsForm->AlsaConnectView->setBezierLines(m_pSetup->bBezierLines);
        m_pConnectionsForm->JackConnectView->ConnectorView()->update();
        m_pConnectionsForm->AlsaConnectView->ConnectorView()->update();
    }

    if (m_pPatchbayForm) {
        m_pPatchbayForm->PatchbayView->setBezierLines(m_pSetup->bBezierLines);
        m_pPatchbayForm->PatchbayView->PatchworkView()->update();
    }
}


// Update main display background effect.
void qjackctlMainForm::updateDisplayEffect (void)
{
    if (m_pSetup == NULL)
        return;

    QPixmap pm;
    if (m_pSetup->bDisplayEffect)
        pm = QPixmap::fromMimeSource("displaybg1.png");

    // Set the main origin...
    StatusDisplayFrame->setPaletteBackgroundPixmap(pm);

    // Iterate for every child text label...
    QObjectList *pList = StatusDisplayFrame->queryList("QLabel");
    if (pList) {
        for (QLabel *pLabel = (QLabel *) pList->first(); pLabel; pLabel = (QLabel *) pList->next())
            pLabel->setPaletteBackgroundPixmap(pm);
        delete pList;
    }
}


// Force update of big time display related fonts.
void qjackctlMainForm::updateTimeDisplayFonts (void)
{
    QFont font;
    if (!m_pSetup->sDisplayFont1.isEmpty() && font.fromString(m_pSetup->sDisplayFont1))
        TimeDisplayTextLabel->setFont(font);
    if (!m_pSetup->sDisplayFont2.isEmpty() && font.fromString(m_pSetup->sDisplayFont2)) {
        ServerStateTextLabel->setFont(font);
        ServerModeTextLabel->setFont(font);
        CpuLoadTextLabel->setFont(font);
        SampleRateTextLabel->setFont(font);
        XrunCountTextLabel->setFont(font);
        TransportStateTextLabel->setFont(font);
        TransportBPMTextLabel->setFont(font);
        font.setBold(true);
        TransportTimeTextLabel->setFont(font);
    }
}


// Force update of big time display related tooltips.
void qjackctlMainForm::updateTimeDisplayToolTips (void)
{
    QToolTip::remove(TimeDisplayTextLabel);
    QToolTip::remove(TransportTimeTextLabel);

    QString sTimeDisplay   = tr("Transport BBT (bar:beat.ticks)");
    QString sTransportTime = tr("Transport time code");

    switch (m_pSetup->iTimeDisplay) {
      case DISPLAY_TRANSPORT_TIME:
      {
        QString sTemp  = sTimeDisplay;
        sTimeDisplay   = sTransportTime;
        sTransportTime = sTemp;
        break;
      }
      case DISPLAY_RESET_TIME:
        sTimeDisplay = tr("Elapsed time since last reset");
        break;
      case DISPLAY_XRUN_TIME:
        sTimeDisplay = tr("Elapsed time since last XRUN");
        break;
    }

    QToolTip::add(TimeDisplayTextLabel, sTimeDisplay);
    QToolTip::add(TransportTimeTextLabel, sTransportTime);
}


// Force update of time format dependant stuff.
void qjackctlMainForm::updateTimeFormat (void)
{
    // Time dashes format helper.
    m_sTimeDashes = "--:--:--";
    switch (m_pSetup->iTimeFormat) {
      case 1:   // Tenths of second.
        m_sTimeDashes += ".-";
        break;
      case 2:   // Hundredths of second.
        m_sTimeDashes += ".--";
        break;
      case 3:   // Raw milliseconds
        m_sTimeDashes += ".---";
        break;
    }
}


// Update the connections client/port aliases.
void qjackctlMainForm::updateAliases (void)
{
	if (m_pConnectionsForm)
		m_pConnectionsForm->updateAliases();
}


// Update the main form buttons display.
void qjackctlMainForm::updateButtons (void)
{
	updateTitleStatus();

	if (m_pSetup->bLeftButtons) {
		StartPushButton->show();
		StopPushButton->show();
		MessagesPushButton->show();
		StatusPushButton->show();
		ConnectionsPushButton->show();
		PatchbayPushButton->show();
	} else {
		StartPushButton->hide();
		StopPushButton->hide();
		MessagesPushButton->hide();
		StatusPushButton->hide();
		ConnectionsPushButton->hide();
		PatchbayPushButton->hide();
	}

	if (m_pSetup->bRightButtons) {
		QuitPushButton->show();
		SetupPushButton->show();
	} else {
		QuitPushButton->hide();
		SetupPushButton->hide();
	}

	if (m_pSetup->bRightButtons &&
		(m_pSetup->bLeftButtons || m_pSetup->bTransportButtons)) {
		AboutPushButton->show();
	} else {
		AboutPushButton->hide();
	}

	if (m_pSetup->bLeftButtons || m_pSetup->bTransportButtons) {
		RewindPushButton->show();
		BackwardPushButton->show();
		PlayPushButton->show();
		PausePushButton->show();
		ForwardPushButton->show();
	} else {
		RewindPushButton->hide();
		BackwardPushButton->hide();
		PlayPushButton->hide();
		PausePushButton->hide();
		ForwardPushButton->hide();
	}

	adjustSize();
}


// Force update of active patchbay definition profile, if applicable.
bool qjackctlMainForm::isActivePatchbay ( const QString& sPatchbayPath )
{
    bool bActive = false;

    if (m_pSetup && m_pSetup->bActivePatchbay && !m_pSetup->sActivePatchbayPath.isEmpty())
        bActive = (m_pSetup->sActivePatchbayPath == sPatchbayPath);

    return bActive;
}


// Force update of active patchbay definition profile, if applicable.
void qjackctlMainForm::updateActivePatchbay (void)
{
    if (m_pSetup == NULL)
        return;

    // Time to load the active patchbay rack profiler?
    if (m_pSetup->bActivePatchbay && !m_pSetup->sActivePatchbayPath.isEmpty()) {
        if (!qjackctlPatchbayFile::load(&m_patchbayRack, m_pSetup->sActivePatchbayPath)) {
            appendMessagesError(tr("Could not load active patchbay definition.\n\nDisabled."));
            m_pSetup->bActivePatchbay = false;
        } else {
            appendMessages(tr("Patchbay activated."));
            // If we're up and running, make it dirty :)
            if (m_pJackClient)
                m_iJackDirty++;
            if (m_pAlsaSeq)
                m_iAlsaDirty++;
        }
    }   // We're sure there's no active patchbay...
    else appendMessages(tr("Patchbay deactivated."));

    // Should refresh anyway.
    m_iPatchbayRefresh++;
}

// Toggle active patchbay setting.
void qjackctlMainForm::setActivePatchbay ( const QString& sPatchbayPath )
{
    if (m_pSetup == NULL)
        return;
        
    if (sPatchbayPath.isEmpty()) {
        m_pSetup->bActivePatchbay = false;
	} else {
		m_pSetup->bActivePatchbay = true;
		m_pSetup->sActivePatchbayPath = sPatchbayPath;
	}

    updateActivePatchbay();
}


// Reset the MRU patchbay list.
void qjackctlMainForm::setRecentPatchbays ( const QStringList& patchbays )
{
	m_pSetup->patchbays = patchbays;
}


// Stabilize current form toggle buttons that may be astray.
void qjackctlMainForm::stabilizeForm (void)
{
    MessagesPushButton->setOn(m_pMessagesForm && m_pMessagesForm->isVisible());
    StatusPushButton->setOn(m_pStatusForm && m_pStatusForm->isVisible());
    ConnectionsPushButton->setOn(m_pConnectionsForm && m_pConnectionsForm->isVisible());
    PatchbayPushButton->setOn(m_pPatchbayForm && m_pPatchbayForm->isVisible());
}


// Reset XRUN cache items.
void qjackctlMainForm::resetXrunStats (void)
{
    m_tResetLast = QTime::currentTime();

    m_iXrunStats = 0;
    m_iXrunCount = 0;
    m_fXrunTotal = 0.0;
    m_fXrunMin   = 0.0;
    m_fXrunMax   = 0.0;
    m_fXrunLast  = 0.0;

    m_tXrunLast.setHMS(0, 0, 0);

    m_iXrunCallbacks = 0;
    m_iXrunSkips     = 0;

#ifdef CONFIG_JACK_MAX_DELAY
    if (m_pJackClient)
        jack_reset_max_delayed_usecs(m_pJackClient);
#endif

    refreshXrunStats();

    appendMessages(tr("Statistics reset."));

    // Make sure all status(es) will be updated ASAP.
    m_iStatusRefresh += QJACKCTL_STATUS_CYCLE;
}


// Update the XRUN count/callbacks item.
void qjackctlMainForm::updateXrunCount (void)
{
    // We'll change XRUN status colors here!
    QColor color = (m_pJackClient ? Qt::green : Qt::darkGreen);
    if ((m_iXrunCount + m_iXrunCallbacks) > 0) {
        if (m_iXrunCallbacks > 0)
            color = (m_pJackClient ? Qt::red : Qt::darkRed);
        else
            color = (m_pJackClient ? Qt::yellow : Qt::darkYellow);
        // Change the system tray icon background color!
        if (m_pSystemTray) {
            m_pSystemTray->setBackgroundMode(Qt::PaletteBackground);
            m_pSystemTray->setPaletteBackgroundColor(color);
            m_pSystemTray->repaint(true);
        }
    }   // Reset the system tray icon background!
    else if (m_pSystemTray) {
        m_pSystemTray->setBackgroundMode(Qt::X11ParentRelative);
        m_pSystemTray->repaint(true);
    }

    XrunCountTextLabel->setPaletteForegroundColor(color);

    QString sText = QString::number(m_iXrunCount);
    sText += " (";
    sText += QString::number(m_iXrunCallbacks);
    sText += ")";
    updateStatusItem(STATUS_XRUN_COUNT, sText);
}

// Convert whole elapsed seconds to hh:mm:ss time format.
QString qjackctlMainForm::formatTime ( double secs )
{
    unsigned int hh, mm, ss;

    hh = mm = ss = 0;
    if (secs >= 3600.0) {
        hh    = (unsigned int) (secs / 3600.0);
        secs -= (double) hh * 3600.0;
    }
    if (secs >= 60.0) {
        mm    = (unsigned int) (secs / 60.0);
        secs -= (double) mm * 60.0;
    }
    if (secs >= 0.0) {
        ss    = (unsigned int) secs;
        secs -= (double) ss;
    }

    QString sTemp;
    switch (m_pSetup->iTimeFormat) {
      case 1:   // Tenths of second.
        sTemp.sprintf("%02u:%02u:%02u.%u", hh, mm, ss, (unsigned int) (secs * 10.0));
        break;
      case 2:   // Hundredths of second.
        sTemp.sprintf("%02u:%02u:%02u.%02u", hh, mm, ss, (unsigned int) (secs * 100.0));
        break;
      case 3:   // Raw milliseconds
        sTemp.sprintf("%02u:%02u:%02u.%03u", hh, mm, ss, (unsigned int) (secs * 1000.0));
        break;
      default:  // No second decimation.
        sTemp.sprintf("%02u:%02u:%02u", hh, mm, ss);
        break;
    }
    return sTemp;
}


// Update the XRUN last/elapsed time item.
QString qjackctlMainForm::formatElapsedTime ( int iStatusItem, const QTime& t, bool bElapsed )
{
    QString sTemp = m_sTimeDashes;
    QString sText;

    // Compute and format elapsed time.
    if (t.isNull()) {
        sText = sTemp;
    } else {
        sText = t.toString();
        if (m_pJackClient) {
            double secs = (double) t.elapsed() / 1000.0;
            if (bElapsed && secs > 0) {
                sTemp = formatTime(secs);
                sText += " (" + sTemp + ")";
            }
        }
    }

    // Display elapsed time as big time?
    if ((iStatusItem == STATUS_RESET_TIME && m_pSetup->iTimeDisplay == DISPLAY_RESET_TIME) ||
        (iStatusItem == STATUS_XRUN_TIME  && m_pSetup->iTimeDisplay == DISPLAY_XRUN_TIME)) {
        TimeDisplayTextLabel->setText(sTemp);
    }

    return sText;
}


// Update the XRUN last/elapsed time item.
void qjackctlMainForm::updateElapsedTimes (void)
{
    // Display time remaining on start delay...
    if (m_iTimerDelay < m_iStartDelay)
        TimeDisplayTextLabel->setText(formatTime((double) (m_iStartDelay - m_iTimerDelay) / 1000.0));
    else {
        updateStatusItem(STATUS_RESET_TIME, formatElapsedTime(STATUS_RESET_TIME, m_tResetLast, true));
        updateStatusItem(STATUS_XRUN_TIME, formatElapsedTime(STATUS_XRUN_TIME, m_tXrunLast, ((m_iXrunCount + m_iXrunCallbacks) > 0)));
    }
}


// Update the XRUN list view items.
void qjackctlMainForm::refreshXrunStats (void)
{
    updateXrunCount();

    if (m_fXrunTotal < 0.001) {
        QString n = "--";
        updateStatusItem(STATUS_XRUN_TOTAL, n);
        updateStatusItem(STATUS_XRUN_MIN, n);
        updateStatusItem(STATUS_XRUN_MAX, n);
        updateStatusItem(STATUS_XRUN_AVG, n);
        updateStatusItem(STATUS_XRUN_LAST, n);
    } else {
        float fXrunAverage = 0.0;
        if (m_iXrunCount > 0)
            fXrunAverage = (m_fXrunTotal / m_iXrunCount);
        QString s = " " + tr("msec");
        updateStatusItem(STATUS_XRUN_TOTAL, QString::number(m_fXrunTotal) + s);
        updateStatusItem(STATUS_XRUN_MIN, QString::number(m_fXrunMin) + s);
        updateStatusItem(STATUS_XRUN_MAX, QString::number(m_fXrunMax) + s);
        updateStatusItem(STATUS_XRUN_AVG, QString::number(fXrunAverage) + s);
        updateStatusItem(STATUS_XRUN_LAST, QString::number(m_fXrunLast) + s);
    }

    updateElapsedTimes();
}


// Jack port registration callback funtion, called
// whenever a jack port is registered or unregistered.
static void qjackctl_portRegistrationCallback ( jack_port_id_t, int, void *pvData )
{
    QApplication::postEvent((qjackctlMainForm *) pvData, new QCustomEvent(QJACKCTL_PORT_EVENT));
}


// Jack graph order callback function, called
// whenever the processing graph is reordered.
static int qjackctl_graphOrderCallback ( void *pvData )
{
    QApplication::postEvent((qjackctlMainForm *) pvData, new QCustomEvent(QJACKCTL_PORT_EVENT));

    return 0;
}


// Jack XRUN callback function, called
// whenever there is a xrun.
static int qjackctl_xrunCallback ( void *pvData )
{
    QApplication::postEvent((qjackctlMainForm *) pvData, new QCustomEvent(QJACKCTL_XRUN_EVENT));

    return 0;
}

// Jack buffer size function, called
// whenever the server changes buffer size.
static int qjackctl_bufferSizeCallback ( jack_nframes_t nframes, void *pvData )
{
    // Update our global static variable.
    g_nframes = nframes;

    QApplication::postEvent((qjackctlMainForm *) pvData, new QCustomEvent(QJACKCTL_BUFF_EVENT));

    return 0;
}


// Jack shutdown function, called
// whenever the server terminates this client.
static void qjackctl_shutdown ( void *pvData )
{
    QApplication::postEvent((qjackctlMainForm *) pvData, new QCustomEvent(QJACKCTL_SHUT_EVENT));
}


// Jack socket notifier port/graph callback funtion.
void qjackctlMainForm::portNotifyEvent (void)
{
    // Log some message here, if new.
    if (m_iJackRefresh == 0)
        appendMessagesColor(tr("Audio connection graph change."), "#cc9966");
    // Do what has to be done.
    refreshJackConnections();
    // We'll be dirty too...
    m_iJackDirty++;
}


// Jack socket notifier XRUN callback funtion.
void qjackctlMainForm::xrunNotifyEvent (void)
{
    // Just increment callback counter.
    m_iXrunCallbacks++;

	// Skip this one? Maybe we're under some kind of storm...
	m_iXrunSkips++;

	// Report rate must be under one second...
	if (m_tXrunLast.restart() < 1000)
		return;

#ifdef CONFIG_JACK_XRUN_DELAY
    // We have an official XRUN delay value (convert usecs to msecs)...
    updateXrunStats(0.001 * jack_get_xrun_delayed_usecs(m_pJackClient));
#endif

    // Just log this single event...
    appendMessagesColor(tr("XRUN callback (%1).").arg(m_iXrunCallbacks), "#cc66cc");
}


// Jack buffer size notifier callback funtion.
void qjackctlMainForm::buffNotifyEvent (void)
{
    // Don't need to nothing, it was handled on qjackctl_bufferSizeCallback;
    // just log this event as routine.
    appendMessagesColor(tr("Buffer size change (%1).").arg((int) g_nframes), "#996633");
}


// Jack socket notifier callback funtion.
void qjackctlMainForm::shutNotifyEvent (void)
{
    // Log this event.
    appendMessagesColor(tr("Shutdown notification."), "#cc6666");
    // Do what has to be done.
    stopJackServer();
    // We're not detached anymore, anyway.
    m_bJackDetach = false;
}


// ALSA announce slot.
void qjackctlMainForm::alsaNotifySlot ( int /*fd*/ )
{
#ifdef CONFIG_ALSA_SEQ
    snd_seq_event_t *pAlsaEvent;
    snd_seq_event_input(m_pAlsaSeq, &pAlsaEvent);
    snd_seq_free_event(pAlsaEvent);
#endif
    // Log some message here, if new.
    if (m_iAlsaRefresh == 0)
        appendMessagesColor(tr("MIDI connection graph change."), "#66cc99");
    // Do what has to be done.
    refreshAlsaConnections();
    // We'll be dirty too...
    m_iAlsaDirty++;
}


// Timer callback funtion.
void qjackctlMainForm::timerSlot (void)
{
    // Is it the first shot on server start after a few delay?
    if (m_iTimerDelay < m_iStartDelay) {
        m_iTimerDelay += QJACKCTL_TIMER_MSECS;
        if (m_iTimerDelay >= m_iStartDelay) {
            // If we cannot start it now, maybe a lil'mo'later ;)
            if (!startJackClient(false) && m_pJack && m_pJack->isRunning()) {
                m_iStartDelay += m_iTimerDelay;
                m_iTimerDelay  = 0;
            }
        }
    }

    // Is the connection patchbay dirty enough?
    if (m_pConnectionsForm) {
        const QString sEllipsis = "...";
        // Are we about to enforce an audio connections persistence profile?
        if (m_iJackDirty > 0) {
            m_iJackDirty = 0;
            if (m_pSetup->bActivePatchbay) {
                appendMessagesColor(tr("Audio active patchbay scan") + sEllipsis, "#6699cc");
                m_patchbayRack.connectAudioScan(m_pJackClient);
            }
            refreshJackConnections();
        }
        // Or is it from the MIDI field?
        if (m_iAlsaDirty > 0) {
            m_iAlsaDirty = 0;
            if (m_pSetup->bActivePatchbay) {
                appendMessagesColor(tr("MIDI active patchbay scan") + sEllipsis, "#99cc66");
                m_patchbayRack.connectMidiScan(m_pAlsaSeq);
            }
            refreshAlsaConnections();
        }
        // Shall we refresh connections now and then?
        if (m_pSetup->bAutoRefresh) {
            m_iTimerRefresh += QJACKCTL_TIMER_MSECS;
            if (m_iTimerRefresh >= (m_pSetup->iTimeRefresh * 1000)) {
                m_iTimerRefresh = 0;
                refreshConnections();
            }
        }
        // Are we about to refresh it, really?
        if (m_iJackRefresh > 0) {
            m_iJackRefresh = 0;
            m_pConnectionsForm->refreshJack(true);
        }
        if (m_iAlsaRefresh > 0) {
            m_iAlsaRefresh = 0;
            m_pConnectionsForm->refreshAlsa(true);
        }
    }

    // Is the patchbay dirty enough?
    if (m_pPatchbayForm && m_iPatchbayRefresh > 0) {
        m_iPatchbayRefresh = 0;
        m_pPatchbayForm->refreshForm();
    }

    // Update some statistical fields, directly.
    refreshStatus();

    // Register the next timer slot.
    QTimer::singleShot(QJACKCTL_TIMER_MSECS, this, SLOT(timerSlot()));
}


// JACK connection notification slot.
void qjackctlMainForm::jackConnectChanged (void)
{
    // Just shake the audio connections status quo.
    if (++m_iJackDirty == 1)
        appendMessagesColor(tr("Audio connection change."), "#9999cc");
}


// ALSA connection notification slot.
void qjackctlMainForm::alsaConnectChanged (void)
{
    // Just shake the MIDI connections status quo.
    if (++m_iAlsaDirty == 1)
        appendMessagesColor(tr("MIDI connection change."), "#cccc99");
}


// Cable connection notification slot.
void qjackctlMainForm::cableConnectSlot ( const QString& sOutputPort, const QString& sInputPort, unsigned int ulCableFlags )
{
    QString sText = QFileInfo(m_pSetup->sActivePatchbayPath).baseName() + ": ";
    QString sColor;

    sText += sOutputPort;
    sText += " -> ";
    sText += sInputPort;
    sText += " ";

    switch (ulCableFlags) {
      case QJACKCTL_CABLE_CHECKED:
        sText += tr("checked");
        sColor = "#99cccc";
        break;
      case QJACKCTL_CABLE_CONNECTED:
        sText += tr("connected");
        sColor = "#669999";
        break;
      case QJACKCTL_CABLE_DISCONNECTED:
        sText += tr("disconnected");
        sColor = "#cc9999";
        break;
      case QJACKCTL_CABLE_FAILED:
      default:
        sText += tr("failed");
        sColor = "#cc6699";
        break;
    }

    appendMessagesColor(sText + ".", sColor);
}


// Start our jack audio control client...
bool qjackctlMainForm::startJackClient ( bool bDetach )
{
    // If can't be already started, are we?
    if (m_pJackClient)
        return true;

    // Have it a setup?
    if (m_pSetup == NULL)
        return false;

	// Time to (re)load current preset aliases?
	if (m_pConnectionsForm && !m_pConnectionsForm->loadAliases())
	    return false;

    // Make sure all status(es) will be updated ASAP.
    m_iStatusRefresh += QJACKCTL_STATUS_CYCLE;

    // Are we about to start detached?
    if (bDetach) {
        // To fool timed client initialization delay.
        m_iTimerDelay += (m_iStartDelay + 1);
        // Refresh status (with dashes?)
        refreshStatus();
    }

    // Create the jack client handle, using a distinct identifier (PID?)
    QString sClientName = "qjackctl-" + QString::number((int) ::getpid());
    m_pJackClient = jack_client_new(sClientName.latin1());
    if (m_pJackClient == NULL) {
		if (!bDetach) {
			appendMessagesError(
				tr("Could not connect to JACK server as client.\n\n"
				"Please check the messages window for more info."));
		}
        return false;
    }

    // Set notification callbacks.
    jack_set_graph_order_callback(m_pJackClient, qjackctl_graphOrderCallback, this);
    jack_set_port_registration_callback(m_pJackClient, qjackctl_portRegistrationCallback, this);
    jack_set_xrun_callback(m_pJackClient, qjackctl_xrunCallback, this);
    jack_set_buffer_size_callback(m_pJackClient, qjackctl_bufferSizeCallback, this);
    jack_on_shutdown(m_pJackClient, qjackctl_shutdown, this);

    // First knowledge about buffer size.
    g_nframes = jack_get_buffer_size(m_pJackClient);

    // Reconstruct our connections patchbay...
    if (m_pConnectionsForm)
        m_pConnectionsForm->setJackClient(m_pJackClient);
    if (m_pPatchbayForm)
        m_pPatchbayForm->setJackClient(m_pJackClient);

    // Save server configuration file.
    if (m_pSetup->bServerConfig && !m_sJackCmdLine.isEmpty()) {
        QString sJackCmdLine = m_sJackCmdLine;
        if (m_pSetup->bServerConfigTemp) {
            int iPos = sJackCmdLine.find(' ');
            if (iPos > 0)
                sJackCmdLine = sJackCmdLine.insert(iPos, " -T");
        }
        QString sFilename = ::getenv("HOME");
        sFilename += "/" + m_pSetup->sServerConfigName;
        QFile file(sFilename);
        if (file.open(IO_WriteOnly | IO_Truncate)) {
            QTextStream(&file) << sJackCmdLine << endl;
            file.close();
            appendMessagesColor(tr("Server configuration saved to \"%1\".").arg(sFilename), "#999933");
        }
    }

   	// Do not forget to reset XRUN stats variables.
    if (!bDetach)
        resetXrunStats();
	else // We'll flag that we've been detached!
		m_bJackDetach = true;

    // Activate us as a client...
    jack_activate(m_pJackClient);

    // Remember to schedule an initial connection refreshment.
    refreshConnections();

    // All displays are highlighted from now on.
    ServerStateTextLabel->setPaletteForegroundColor(Qt::yellow);
    ServerModeTextLabel->setPaletteForegroundColor(Qt::darkYellow);
    CpuLoadTextLabel->setPaletteForegroundColor(Qt::yellow);
    SampleRateTextLabel->setPaletteForegroundColor(Qt::darkYellow);
    TimeDisplayTextLabel->setPaletteForegroundColor(Qt::green);
    TransportStateTextLabel->setPaletteForegroundColor(Qt::green);
    TransportBPMTextLabel->setPaletteForegroundColor(Qt::green);
    TransportTimeTextLabel->setPaletteForegroundColor(Qt::green);

    // Whether we've started detached, just change active status.
    updateServerState(m_bJackDetach ? QJACKCTL_ACTIVE : QJACKCTL_STARTED);
    StopPushButton->setEnabled(true);

    // Log success here.
    appendMessages(tr("Client activated."));

	// Do we have any post-startup scripting?...
	// (only if we're not a detached client)
	if (!bDetach && !m_bJackDetach) {
		if (m_pSetup->bPostStartupScript
			&& !m_pSetup->sPostStartupScriptShell.isEmpty()) {
			shellExecute(m_pSetup->sPostStartupScriptShell,
				tr("Post-startup script..."),
				tr("Post-startup script terminated"));
		}
	}

	// Have we an initial command-line to start away?
	if (!m_pSetup->sCmdLine.isEmpty()) {
		// Run it dettached...
		shellExecute(m_pSetup->sCmdLine,
			tr("Command line argument..."),
			tr("Command line argument started"));
		// And reset it forever more...
		m_pSetup->sCmdLine = QString::null;
	}

    // OK, we're at it!
    return true;
}


// Stop jack audio client...
void qjackctlMainForm::stopJackClient (void)
{
    // Deactivate us as a client...
    if (m_pJackClient) {
        jack_deactivate(m_pJackClient);
        // Log deactivation here.
        appendMessages(tr("Client deactivated."));
    }
    // Destroy our connections patchbay...
    if (m_pConnectionsForm)
        m_pConnectionsForm->setJackClient(NULL);
    if (m_pPatchbayForm)
        m_pPatchbayForm->setJackClient(NULL);

    // Reset command-line configuration info.
    m_sJackCmdLine = QString::null;

    // Close us as a client...
    if (m_pJackClient)
        jack_client_close(m_pJackClient);
    m_pJackClient = NULL;

    // Displays are dimmed again.
    ServerModeTextLabel->setPaletteForegroundColor(Qt::darkYellow);
    CpuLoadTextLabel->setPaletteForegroundColor(Qt::darkYellow);
    SampleRateTextLabel->setPaletteForegroundColor(Qt::darkYellow);
    TimeDisplayTextLabel->setPaletteForegroundColor(Qt::darkGreen);
    TransportStateTextLabel->setPaletteForegroundColor(Qt::darkGreen);
    TransportBPMTextLabel->setPaletteForegroundColor(Qt::darkGreen);
    TransportTimeTextLabel->setPaletteForegroundColor(Qt::darkGreen);

    // Refresh jack client statistics explicitly.
    refreshXrunStats();
}


// Rebuild all patchbay items.
void qjackctlMainForm::refreshConnections (void)
{
    refreshJackConnections();
    refreshAlsaConnections();
}

void qjackctlMainForm::refreshJackConnections (void)
{
    // Hack this as for a while.
    if (m_pConnectionsForm && m_iJackRefresh == 0)
        m_pConnectionsForm->stabilizeJack(false);

    // Just increment our intentions; it will be deferred
    // to be executed just on timer slot processing...
    m_iJackRefresh++;
}

void qjackctlMainForm::refreshAlsaConnections (void)
{
    // Hack this as for a while.
    if (m_pConnectionsForm && m_iAlsaRefresh == 0)
        m_pConnectionsForm->stabilizeAlsa(false);

    // Just increment our intentions; it will be deferred
    // to be executed just on timer slot processing...
    m_iAlsaRefresh++;
}


void qjackctlMainForm::refreshPatchbay (void)
{
    // Just increment our intentions; it will be deferred
    // to be executed just on timer slot processing...
    m_iPatchbayRefresh++;
}


// Main form visibility requester slot.
void qjackctlMainForm::toggleMainForm (void)
{
    m_pSetup->saveWidgetGeometry(this);
    if (isVisible()) {
        if (m_pSetup->bSystemTray && m_pSystemTray) {
            // Hide away from sight.
            hide();
        } else {
            // Minimize (iconify) normally.
            showMinimized();
        }
    } else {
        show();
        raise();
        setActiveWindow();
    }
}


// Message log form requester slot.
void qjackctlMainForm::toggleMessagesForm (void)
{
    if (m_pMessagesForm) {
        m_pSetup->saveWidgetGeometry(m_pMessagesForm);
        if (m_pMessagesForm->isVisible()) {
            m_pMessagesForm->hide();
        } else {
            m_pMessagesForm->show();
            m_pMessagesForm->raise();
            m_pMessagesForm->setActiveWindow();
        }
    }
}


// Status form requester slot.
void qjackctlMainForm::toggleStatusForm (void)
{
    if (m_pStatusForm) {
        m_pSetup->saveWidgetGeometry(m_pStatusForm);
        if (m_pStatusForm->isVisible()) {
            m_pStatusForm->hide();
        } else {
            m_pStatusForm->show();
            m_pStatusForm->raise();
            m_pStatusForm->setActiveWindow();
        }
    }
}


// Connections form requester slot.
void qjackctlMainForm::toggleConnectionsForm (void)
{
    if (m_pConnectionsForm) {
        m_pSetup->saveWidgetGeometry(m_pConnectionsForm);
        m_pConnectionsForm->setJackClient(m_pJackClient);
        m_pConnectionsForm->setAlsaSeq(m_pAlsaSeq);
        if (m_pConnectionsForm->isVisible()) {
            m_pConnectionsForm->hide();
        } else {
            m_pConnectionsForm->show();
            m_pConnectionsForm->raise();
            m_pConnectionsForm->setActiveWindow();
        }
    }
}


// Patchbay form requester slot.
void qjackctlMainForm::togglePatchbayForm (void)
{
    if (m_pPatchbayForm) {
        m_pSetup->saveWidgetGeometry(m_pPatchbayForm);
        m_pPatchbayForm->setJackClient(m_pJackClient);
        m_pPatchbayForm->setAlsaSeq(m_pAlsaSeq);
        if (m_pPatchbayForm->isVisible()) {
            m_pPatchbayForm->hide();
        } else {
            m_pPatchbayForm->show();
            m_pPatchbayForm->raise();
            m_pPatchbayForm->setActiveWindow();
        }
    }
}


// Setup dialog requester slot.
void qjackctlMainForm::showSetupForm (void)
{
    qjackctlSetupForm *pSetupForm = new qjackctlSetupForm(this);
    if (pSetupForm) {
        // Check out some initial nullities(tm)...
        if (m_pSetup->sMessagesFont.isEmpty() && m_pMessagesForm)
            m_pSetup->sMessagesFont = m_pMessagesForm->messagesFont().toString();
        if (m_pSetup->sDisplayFont1.isEmpty())
            m_pSetup->sDisplayFont1 = TimeDisplayTextLabel->font().toString();
        if (m_pSetup->sDisplayFont2.isEmpty())
            m_pSetup->sDisplayFont2 = ServerStateTextLabel->font().toString();
        if (m_pSetup->sConnectionsFont.isEmpty() && m_pConnectionsForm)
            m_pSetup->sConnectionsFont = m_pConnectionsForm->connectionsFont().toString();
        // To track down deferred or immediate changes.
        QString sOldMessagesFont        = m_pSetup->sMessagesFont;
        QString sOldDisplayFont1        = m_pSetup->sDisplayFont1;
        QString sOldDisplayFont2        = m_pSetup->sDisplayFont2;
        QString sOldConnectionsFont     = m_pSetup->sConnectionsFont;
        int     iOldConnectionsIconSize = m_pSetup->iConnectionsIconSize;
        int     iOldTimeDisplay         = m_pSetup->iTimeDisplay;
        int     iOldTimeFormat          = m_pSetup->iTimeFormat;
        bool    bOldDisplayEffect       = m_pSetup->bDisplayEffect;
        bool    bOldActivePatchbay      = m_pSetup->bActivePatchbay;
        QString sOldActivePatchbayPath  = m_pSetup->sActivePatchbayPath;
        bool    bOldStdoutCapture       = m_pSetup->bStdoutCapture;
        bool    bOldKeepOnTop           = m_pSetup->bKeepOnTop;
        bool    bOldSystemTray          = m_pSetup->bSystemTray;
        bool    bOldDelayedSetup        = m_pSetup->bDelayedSetup;
        int     bOldMessagesLimit       = m_pSetup->bMessagesLimit;
        int     iOldMessagesLimitLines  = m_pSetup->iMessagesLimitLines;
        bool    bOldBezierLines         = m_pSetup->bBezierLines;
        bool    bOldAliasesEnabled      = m_pSetup->bAliasesEnabled;
        bool    bOldAliasesEditing      = m_pSetup->bAliasesEditing;
        bool    bOldLeftButtons         = m_pSetup->bLeftButtons;
        bool    bOldRightButtons        = m_pSetup->bRightButtons;
        bool    bOldTransportButtons    = m_pSetup->bTransportButtons;
        // Load the current setup settings.
        pSetupForm->setup(m_pSetup);
        // Show the setup dialog...
        if (pSetupForm->exec()) {
            // Check wheather something immediate has changed.
            if (( bOldBezierLines && !m_pSetup->bBezierLines) ||
                (!bOldBezierLines &&  m_pSetup->bBezierLines))
                updateBezierLines();
            if (( bOldDisplayEffect && !m_pSetup->bDisplayEffect) ||
                (!bOldDisplayEffect &&  m_pSetup->bDisplayEffect))
                updateDisplayEffect();
            if (iOldConnectionsIconSize != m_pSetup->iConnectionsIconSize)
                updateConnectionsIconSize();
            if (sOldConnectionsFont != !m_pSetup->sConnectionsFont)
                updateConnectionsFont();
            if (sOldMessagesFont != m_pSetup->sMessagesFont)
                updateMessagesFont();
            if (( bOldMessagesLimit && !m_pSetup->bMessagesLimit) ||
                (!bOldMessagesLimit &&  m_pSetup->bMessagesLimit) ||
                (iOldMessagesLimitLines !=  m_pSetup->iMessagesLimitLines))
                updateMessagesLimit();
            if (sOldDisplayFont1 != m_pSetup->sDisplayFont1 ||
                sOldDisplayFont2 != m_pSetup->sDisplayFont2)
                updateTimeDisplayFonts();
            if (iOldTimeDisplay |= m_pSetup->iTimeDisplay)
                updateTimeDisplayToolTips();
            if (iOldTimeFormat |= m_pSetup->iTimeFormat)
                updateTimeFormat();
            if ((!bOldActivePatchbay && m_pSetup->bActivePatchbay) ||
                (sOldActivePatchbayPath != m_pSetup->sActivePatchbayPath))
                updateActivePatchbay();
            if (( bOldSystemTray && !m_pSetup->bSystemTray) ||
                (!bOldSystemTray &&  m_pSetup->bSystemTray))
                updateSystemTray();
			if (( bOldAliasesEnabled && !m_pSetup->bAliasesEnabled) ||
				(!bOldAliasesEnabled &&  m_pSetup->bAliasesEnabled) ||
				( bOldAliasesEditing && !m_pSetup->bAliasesEditing) ||
				(!bOldAliasesEditing &&  m_pSetup->bAliasesEditing))
				updateAliases();
            if (( bOldLeftButtons  && !m_pSetup->bLeftButtons)  ||
                (!bOldLeftButtons  &&  m_pSetup->bLeftButtons)  ||
                ( bOldRightButtons && !m_pSetup->bRightButtons) ||
                (!bOldRightButtons &&  m_pSetup->bRightButtons) ||
                ( bOldTransportButtons && !m_pSetup->bTransportButtons) ||
                (!bOldTransportButtons &&  m_pSetup->bTransportButtons))
                updateButtons();
            // Warn if something will be only effective on next run.
            if (( bOldStdoutCapture && !m_pSetup->bStdoutCapture) ||
                (!bOldStdoutCapture &&  m_pSetup->bStdoutCapture) ||
                ( bOldKeepOnTop     && !m_pSetup->bKeepOnTop)     ||
                (!bOldKeepOnTop     &&  m_pSetup->bKeepOnTop)     ||
                ( bOldDelayedSetup  && !m_pSetup->bDelayedSetup)  ||
                (!bOldDelayedSetup  &&  m_pSetup->bDelayedSetup))
                showDirtySetupWarning();
            // If server is currently running, warn user...
            showDirtySettingsWarning();
        }
        delete pSetupForm;
    }
}


// About dialog requester slot.
void qjackctlMainForm::showAboutForm (void)
{
    qjackctlAboutForm *pAboutForm = new qjackctlAboutForm(this);
    if (pAboutForm) {
        pAboutForm->exec();
        delete pAboutForm;
    }
}


// Transport rewind.
void qjackctlMainForm::transportRewind (void)
{
#ifdef CONFIG_JACK_TRANSPORT
	if (m_pJackClient) {
		jack_transport_locate(m_pJackClient, 0);
		// Log this here.
		appendMessages(tr("Transport rewind."));
		// Make sure all status(es) will be updated ASAP...
		m_iStatusRefresh += QJACKCTL_STATUS_CYCLE;
	}
#endif
}

// Transport backward.
void qjackctlMainForm::transportBackward (void)
{
#ifdef CONFIG_JACK_TRANSPORT
	if (m_pJackClient) {
		jack_position_t tpos;
		jack_transport_query(m_pJackClient, &tpos);
		float rate = (float) tpos.frame_rate;
		float tloc = (((float) tpos.frame / rate) - m_fSkipAccel) * rate;
		if (tloc < 0.0)	tloc = 0.0;
		jack_transport_locate(m_pJackClient, (jack_nframes_t) tloc);
		// Log this here (if on initial toggle).
		if (m_fSkipAccel < 1.1) 
			appendMessages(tr("Transport backward."));
		// Take care of backward acceleration...
		if (BackwardPushButton->isDown() && m_fSkipAccel < 60.0)
			m_fSkipAccel *= 1.1;
		// Make sure all status(es) will be updated ASAP...
		m_iStatusRefresh += QJACKCTL_STATUS_CYCLE;
	}
#endif
}

// Transport start (play)
void qjackctlMainForm::transportStart (void)
{
#ifdef CONFIG_JACK_TRANSPORT
    if (m_pJackClient) {
        jack_transport_start(m_pJackClient);
        updateStatusItem(STATUS_TRANSPORT_STATE, tr("Starting"));
        // Log this here.
        appendMessages(tr("Transport start."));
        // Make sure all status(es) will be updated ASAP...
        m_iStatusRefresh += QJACKCTL_STATUS_CYCLE;
    }
#endif
}

// Transport stop (pause).
void qjackctlMainForm::transportStop (void)
{
#ifdef CONFIG_JACK_TRANSPORT
    if (m_pJackClient) {
        jack_transport_stop(m_pJackClient);
        updateStatusItem(STATUS_TRANSPORT_STATE, tr("Stopping"));
        // Log this here.
        appendMessages(tr("Transport stop."));
        // Make sure all status(es) will be updated ASAP...
        m_iStatusRefresh += QJACKCTL_STATUS_CYCLE;
    }
#endif
}

// Transport forward.
void qjackctlMainForm::transportForward (void)
{
#ifdef CONFIG_JACK_TRANSPORT
	if (m_pJackClient) {
		jack_position_t tpos;
		jack_transport_query(m_pJackClient, &tpos);
		float rate = (float) tpos.frame_rate;
		float tloc = (((float) tpos.frame / rate) + m_fSkipAccel) * rate;
		if (tloc < 0.0)	tloc = 0.0;
		jack_transport_locate(m_pJackClient, (jack_nframes_t) tloc);
		// Log this here.
		if (m_fSkipAccel < 1.1) 
			appendMessages(tr("Transport forward."));
		// Take care of forward acceleration...
		if (ForwardPushButton->isDown() && m_fSkipAccel < 60.0)
			m_fSkipAccel *= 1.1;
		// Make sure all status(es) will be updated ASAP...
		m_iStatusRefresh += QJACKCTL_STATUS_CYCLE;
	}
#endif
}


// Almost-complete running status refresher.
void qjackctlMainForm::refreshStatus (void)
{
    const QString n = "--";
    const QString b = "--:--.----";
    const QString sStopped = tr("Stopped");

    m_iStatusRefresh++;

    if (m_pJackClient) {
        const QString s = " ";
#ifdef CONFIG_JACK_TRANSPORT
        QString sText = n;
        jack_position_t tpos;
        jack_transport_state_t tstate = jack_transport_query(m_pJackClient, &tpos);
        bool bPlaying = (tstate == JackTransportRolling || tstate == JackTransportLooping);
        // Transport timecode position.
    //  if (bPlaying)
            updateStatusItem(STATUS_TRANSPORT_TIME, formatTime((double) tpos.frame / (double) tpos.frame_rate));
    //  else
    //      updateStatusItem(STATUS_TRANSPORT_TIME, m_sTimeDashes);
        // Transport barcode position (bar:beat.tick)
        if (tpos.valid & JackPositionBBT) {
            updateStatusItem(STATUS_TRANSPORT_BBT, QString().sprintf("%u:%u.%04u", tpos.bar, tpos.beat, tpos.tick));
            updateStatusItem(STATUS_TRANSPORT_BPM, QString::number(tpos.beats_per_minute));
        } else {
            updateStatusItem(STATUS_TRANSPORT_BBT, b);
            updateStatusItem(STATUS_TRANSPORT_BPM, n);
        }
#endif  // !CONFIG_JACK_TRANSPORT
        // Less frequent status items update...
        if (m_iStatusRefresh >= QJACKCTL_STATUS_CYCLE) {
            m_iStatusRefresh = 0;
            updateStatusItem(STATUS_CPU_LOAD, QString::number(jack_cpu_load(m_pJackClient), 'g', 2) + s + "%");
            updateStatusItem(STATUS_SAMPLE_RATE, QString::number(jack_get_sample_rate(m_pJackClient)) + s + tr("Hz"));
            updateStatusItem(STATUS_BUFFER_SIZE, QString::number(g_nframes) + " " + tr("frames"));
#ifdef CONFIG_JACK_REALTIME
            bool bRealtime = jack_is_realtime(m_pJackClient);
            updateStatusItem(STATUS_REALTIME, (bRealtime ? tr("Yes") : tr("No")));
            ServerModeTextLabel->setText(bRealtime ? tr("RT") : n);
#else
            updateStatusItem(STATUS_REALTIME, n);
            ServerModeTextLabel->setText(n);
#endif  // !CONFIG_JACK_REALTIME
#ifdef CONFIG_JACK_TRANSPORT
            switch (tstate) {
              case JackTransportStarting:
                sText = tr("Starting");
                break;
              case JackTransportRolling:
                sText = tr("Rolling");
                break;
              case JackTransportLooping:
                sText = tr("Looping");
                break;
              case JackTransportStopped:
              default:
                sText = sStopped;
                break;
            }
            updateStatusItem(STATUS_TRANSPORT_STATE, sText);
			RewindPushButton->setEnabled(tpos.frame > 0);
			BackwardPushButton->setEnabled(tpos.frame > 0);
			PlayPushButton->setEnabled(tstate == JackTransportStopped);
			PausePushButton->setEnabled(bPlaying);
			ForwardPushButton->setEnabled(true);
			if (!BackwardPushButton->isDown() && !ForwardPushButton->isDown())
				m_fSkipAccel = 1.0;
#else
            updateStatusItem(STATUS_TRANSPORT_STATE, n);
			RewindPushButton->setEnabled(false);
			BackwardPushButton->setEnabled(false);
			PlayPushButton->setEnabled(false);
			PausePushButton->setEnabled(false);
			ForwardPushButton->setEnabled(false);
            updateStatusItem(STATUS_TRANSPORT_TIME, m_sTimeDashes);
            updateStatusItem(STATUS_TRANSPORT_BBT, b);
            updateStatusItem(STATUS_TRANSPORT_BPM, n);
#endif  // !CONFIG_JACK_TRANSPORT
#ifdef CONFIG_JACK_MAX_DELAY
            updateStatusItem(STATUS_MAX_DELAY, QString::number(0.001 * jack_get_max_delayed_usecs(m_pJackClient)) + " " + tr("msec"));
#endif
			// Check if we're have some XRUNs to report...
			if (m_iXrunSkips > 0) {
				// Maybe we've skipped some...
				if (m_iXrunSkips > 1) {
					appendMessagesColor(tr("XRUN callback (%1 skipped).")
						.arg(m_iXrunSkips - 1), "#cc99cc");
				}
				// Reset skip count.
				m_iXrunSkips = 0;
				// Highlight the (new) status...
				refreshXrunStats();
			}
        }
    }   // No need to update often if we're just idle...
    else if (m_iStatusRefresh >= QJACKCTL_STATUS_CYCLE) {
        m_iStatusRefresh = 0;
        updateStatusItem(STATUS_CPU_LOAD, n);
        updateStatusItem(STATUS_SAMPLE_RATE, n);
        updateStatusItem(STATUS_BUFFER_SIZE, n);
        updateStatusItem(STATUS_REALTIME, n);
        ServerModeTextLabel->setText(n);
        updateStatusItem(STATUS_TRANSPORT_STATE, n);
        updateStatusItem(STATUS_TRANSPORT_TIME, m_sTimeDashes);
        updateStatusItem(STATUS_TRANSPORT_BBT, b);
        updateStatusItem(STATUS_TRANSPORT_BPM, n);
		RewindPushButton->setEnabled(false);
		BackwardPushButton->setEnabled(false);
		PlayPushButton->setEnabled(false);
		PausePushButton->setEnabled(false);
		ForwardPushButton->setEnabled(false);
    }

    // Elapsed times should be rigorous...
    updateElapsedTimes();
}


// Status item updater.
void qjackctlMainForm::updateStatusItem( int iStatusItem, const QString& sText )
{
    switch (iStatusItem) {
      case STATUS_SERVER_STATE:
        ServerStateTextLabel->setText(sText);
        break;
      case STATUS_CPU_LOAD:
        CpuLoadTextLabel->setText(sText);
        break;
      case STATUS_SAMPLE_RATE:
        SampleRateTextLabel->setText(sText);
        break;
      case STATUS_XRUN_COUNT:
        XrunCountTextLabel->setText(sText);
        break;
      case STATUS_TRANSPORT_STATE:
        TransportStateTextLabel->setText(sText);
        break;
      case STATUS_TRANSPORT_TIME:
        if (m_pSetup->iTimeDisplay == DISPLAY_TRANSPORT_TIME)
            TimeDisplayTextLabel->setText(sText);
        else
            TransportTimeTextLabel->setText(sText);
        break;
      case STATUS_TRANSPORT_BBT:
        if (m_pSetup->iTimeDisplay == DISPLAY_TRANSPORT_BBT)
            TimeDisplayTextLabel->setText(sText);
        else
        if (m_pSetup->iTimeDisplay == DISPLAY_TRANSPORT_TIME)
            TransportTimeTextLabel->setText(sText);
        break;
      case STATUS_TRANSPORT_BPM:
        TransportBPMTextLabel->setText(sText);
        break;
    }

    if (m_pStatusForm)
        m_pStatusForm->updateStatusItem(iStatusItem, sText);
}


// Main window caption title and system tray icon and tooltip update.
void qjackctlMainForm::updateTitleStatus (void)
{
    QString sTitle;

	if (!m_pSetup->bLeftButtons || !m_pSetup->bRightButtons)
		sTitle = QJACKCTL_SUBTITLE0;
	else
		sTitle = QJACKCTL_SUBTITLE1;

    sTitle += " [" + m_pSetup->sDefPreset + "] ";

    QString sState;
    QString sDots = ".";
    const QString s = "..";
    switch (m_iServerState) {
      case QJACKCTL_STARTING:
        sState = tr("Starting");
        sDots += s;
        break;
      case QJACKCTL_STARTED:
        sState = tr("Started");
        break;
      case QJACKCTL_STOPPING:
        sState = tr("Stopping");
        sDots += s;
        break;
      case QJACKCTL_STOPPED:
        sState = tr("Stopped");
        break;
      case QJACKCTL_ACTIVE:
        sState = tr("Active");
        break;
      case QJACKCTL_ACTIVATING:
        sState = tr("Activating");
        sDots += s;
        break;
      case QJACKCTL_INACTIVE:
      default:
        sState = tr("Inactive");
        break;
    }
    sTitle += sState + sDots;
    setCaption(sTitle);

    updateStatusItem(STATUS_SERVER_STATE, sState);

    if (m_pSystemTray) {
        QToolTip::remove(m_pSystemTray);
        switch (m_iServerState) {
          case QJACKCTL_STARTING:
            m_pSystemTray->setPixmapOverlay(QPixmap::fromMimeSource("xstarting1.png"));
            break;
          case QJACKCTL_STARTED:
            m_pSystemTray->setPixmapOverlay(QPixmap::fromMimeSource("xstarted1.png"));
            break;
          case QJACKCTL_STOPPING:
            m_pSystemTray->setPixmapOverlay(QPixmap::fromMimeSource("xstopping1.png"));
            break;
          case QJACKCTL_STOPPED:
            m_pSystemTray->setPixmapOverlay(QPixmap::fromMimeSource("xstopped1.png"));
            break;
          case QJACKCTL_ACTIVE:
            m_pSystemTray->setPixmapOverlay(QPixmap::fromMimeSource("xactive1.png"));
            break;
          case QJACKCTL_ACTIVATING:
            m_pSystemTray->setPixmapOverlay(QPixmap::fromMimeSource("xactivating1.png"));
            break;
          case QJACKCTL_INACTIVE:
          default:
            m_pSystemTray->setPixmapOverlay(QPixmap::fromMimeSource("xinactive1.png"));
            break;
        }
        QToolTip::add(m_pSystemTray, sTitle);
    }
}


// Main server state status update helper.
void qjackctlMainForm::updateServerState ( int iServerState )
{
    // Just set the new server state.
    m_iServerState = iServerState;

    // Now's time to update main window
    // caption title and status immediately.
    updateTitleStatus();
}


// System tray master switcher.
void qjackctlMainForm::updateSystemTray (void)
{
#ifdef CONFIG_SYSTEM_TRAY
    if (!m_pSetup->bSystemTray && m_pSystemTray) {
	//  Strange enough, this would close the application too.
    //  m_pSystemTray->close();
        delete m_pSystemTray;
        m_pSystemTray = NULL;
    }
    if (m_pSetup->bSystemTray && m_pSystemTray == NULL) {
        m_pSystemTray = new qjackctlSystemTray(this);
        m_pSystemTray->show();
        QObject::connect(m_pSystemTray, SIGNAL(clicked()), this, SLOT(toggleMainForm()));
        QObject::connect(m_pSystemTray, SIGNAL(contextMenuRequested(const QPoint &)),
            this, SLOT(systemTrayContextMenu(const QPoint &)));
    } else {
        // Make sure the main widget is visible.
        show();
        raise();
        setActiveWindow();
    }
#endif
}


// System tray context menu request slot.
void qjackctlMainForm::systemTrayContextMenu ( const QPoint& pos )
{
    int iItemID;
    QPopupMenu* pContextMenu = new QPopupMenu(this);

    QString sHideMinimize = (m_pSetup->bSystemTray && m_pSystemTray ? tr("&Hide") : tr("Mi&nimize"));
    QString sShowRestore  = (m_pSetup->bSystemTray && m_pSystemTray ? tr("S&how") : tr("Rest&ore"));
    pContextMenu->insertItem(isVisible() ? sHideMinimize : sShowRestore, this, SLOT(toggleMainForm()));
    pContextMenu->insertSeparator();

    if (m_pJackClient == NULL) {
        pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("start1.png")),
            tr("&Start"), this, SLOT(startJack()));
    } else {
        pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("stop1.png")),
            tr("&Stop"), this, SLOT(stopJack()));
    }
    iItemID = pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("reset1.png")),
        tr("&Reset"), m_pStatusForm, SLOT(resetXrunStats()));
//  pContextMenu->setItemEnabled(iItemID, m_pJackClient != NULL);
    pContextMenu->insertSeparator();

    // Construct the actual presets menu,
    // overriding the last one, if any...
    if (m_pPresetsMenu)
        delete m_pPresetsMenu;
    m_pPresetsMenu = new QPopupMenu(this);
    QStringList presets = m_pSetup->presets;
    // Assume QStringList iteration follows item index order (0,1,2...)
    int iIndex = 0;
    for (QStringList::Iterator iter = presets.begin(); iter != presets.end(); ++iter) {
        iItemID = m_pPresetsMenu->insertItem(*iter);
        m_pPresetsMenu->setItemChecked(iItemID, *iter == m_pSetup->sDefPreset);
        m_pPresetsMenu->setItemParameter(iItemID, iIndex);
        iIndex++;
    }
    // Default preset always present, and has invalid index parameter (-1)...
    if (iIndex > 0)
        m_pPresetsMenu->insertSeparator();
    iItemID = m_pPresetsMenu->insertItem(m_pSetup->sDefPresetName);
    m_pPresetsMenu->setItemChecked(iItemID, m_pSetup->sDefPresetName == m_pSetup->sDefPreset);
    m_pPresetsMenu->setItemParameter(iItemID, -1);
    QObject::connect(m_pPresetsMenu, SIGNAL(activated(int)), this, SLOT(activatePresetsMenu(int)));
    // Add presets menu to the main context menu...
    pContextMenu->insertItem(tr("&Presets"), m_pPresetsMenu);
    pContextMenu->insertSeparator();

    iItemID = pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("messages1.png")),
        tr("&Messages"), this, SLOT(toggleMessagesForm()));
    pContextMenu->setItemChecked(iItemID, m_pMessagesForm && m_pMessagesForm->isVisible());
    iItemID = pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("status1.png")),
        tr("St&atus"), this, SLOT(toggleStatusForm()));
    pContextMenu->setItemChecked(iItemID, m_pStatusForm && m_pStatusForm->isVisible());
    iItemID = pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("connections1.png")),
        tr("&Connections"), this, SLOT(toggleConnectionsForm()));
    pContextMenu->setItemChecked(iItemID, m_pConnectionsForm && m_pConnectionsForm->isVisible());
    iItemID = pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("patchbay1.png")),
        tr("Patch&bay"), this, SLOT(togglePatchbayForm()));
    pContextMenu->setItemChecked(iItemID, m_pPatchbayForm && m_pPatchbayForm->isVisible());
    pContextMenu->insertSeparator();

	QPopupMenu *pTransportMenu = new QPopupMenu(this);
	iItemID = pTransportMenu->insertItem(
		QIconSet(QPixmap::fromMimeSource("rewind1.png")),
		tr("&Rewind"), this, SLOT(transportRewind()));
	pTransportMenu->setItemEnabled(iItemID, RewindPushButton->isEnabled());
//	iItemID = pTransportMenu->insertItem(
//		QIconSet(QPixmap::fromMimeSource("backward1.png")),
//		tr("&Backward"), this, SLOT(transportBackward()));
//	pTransportMenu->setItemEnabled(iItemID, BackwardPushButton->isEnabled());
	iItemID = pTransportMenu->insertItem(
		QIconSet(QPixmap::fromMimeSource("play1.png")),
		tr("&Play"), this, SLOT(transportStart()));
	pTransportMenu->setItemEnabled(iItemID, PlayPushButton->isEnabled());
	iItemID = pTransportMenu->insertItem(
		QIconSet(QPixmap::fromMimeSource("pause1.png")),
		tr("Pa&use"), this, SLOT(transportStop()));
	pTransportMenu->setItemEnabled(iItemID, PausePushButton->isEnabled());
//	iItemID = pTransportMenu->insertItem(
//		QIconSet(QPixmap::fromMimeSource("forward1.png")),
//		tr("&Forward"), this, SLOT(transportForward()));
//	pTransportMenu->setItemEnabled(iItemID, ForwardPushButton->isEnabled());
	pContextMenu->insertItem(tr("&Transport"), pTransportMenu);
	pContextMenu->insertSeparator();

    pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("setup1.png")),
        tr("S&etup..."), this, SLOT(showSetupForm()));

	if (!m_pSetup->bRightButtons || !m_pSetup->bTransportButtons) {
		pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("about1.png")),
			tr("Ab&out..."), this, SLOT(showAboutForm()));
	}
    pContextMenu->insertSeparator();

    pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("quit1.png")),
        tr("&Quit"), this, SLOT(quitMainForm()));

    pContextMenu->exec(pos);

    delete m_pPresetsMenu;
    m_pPresetsMenu = NULL;

	if (pTransportMenu)
		delete pTransportMenu;

    delete pContextMenu;

}


// Server settings change warning.
void qjackctlMainForm::showDirtySettingsWarning (void)
{
    // If client service is currently running,
    // prompt the effective warning...
    if (m_pJackClient) {
        QMessageBox::warning(this,
			tr("Warning") + " - " QJACKCTL_SUBTITLE1,
            tr("Server settings will be only effective after\n"
               "restarting the JACK audio server."), tr("OK"));
    }   // Otherwise, it will be just as convenient to update status...
    else updateTitleStatus();
}


// Setup otions change warning.
void qjackctlMainForm::showDirtySetupWarning (void)
{
    QMessageBox::information(this,
		tr("Information") + " - " QJACKCTL_SUBTITLE1,
        tr("Some settings will be only effective\n"
           "the next time you start this program."), tr("OK"));
}


// Select the current default preset name from context menu.
void qjackctlMainForm::activatePresetsMenu ( int iItemID )
{
    if (m_pPresetsMenu == NULL)
        return;
	if (m_pConnectionsForm && !m_pConnectionsForm->queryClose())
	    return;

    int iIndex = m_pPresetsMenu->itemParameter(iItemID);
    if (iIndex >= 0 && iIndex < (int) m_pSetup->presets.count())
        m_pSetup->sDefPreset = m_pSetup->presets[iIndex];
    else
        m_pSetup->sDefPreset = m_pSetup->sDefPresetName;

    showDirtySettingsWarning();
}


// Close main form slot.
void qjackctlMainForm::quitMainForm (void)
{
#ifdef CONFIG_SYSTEM_TRAY
    // Flag that we're quitting explicitly.
    m_bQuitForce = true;
#endif
    // And then, do the closing dance.
    close();
}


// Context menu event handler.
void qjackctlMainForm::contextMenuEvent( QContextMenuEvent *pEvent )
{
    // We'll just show up the usual system tray menu.
    systemTrayContextMenu(pEvent->globalPos());
}


// end of qjackctlMainForm.ui.h
