// qjackctlMainForm.ui.h
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

#include <qapplication.h>
#include <qeventloop.h>
#include <qmessagebox.h>
#include <qregexp.h>
#include <qtimer.h>

#include "config.h"

#include "qjackctlStatus.h"
#include "qjackctlAbout.h"

#include "qjackctlPatchbayFile.h"

#include <poll.h>


// Timer constant stuff.
#define QJACKCTL_TIMER_MSECS    200

// Notification pipes descriptors
#define QJACKCTL_FDNIL     -1
#define QJACKCTL_FDREAD     0
#define QJACKCTL_FDWRITE    1

static int g_fdStdout[2] = { QJACKCTL_FDNIL, QJACKCTL_FDNIL };

static int g_fdPort[2] = { QJACKCTL_FDNIL, QJACKCTL_FDNIL };
static int g_fdXrun[2] = { QJACKCTL_FDNIL, QJACKCTL_FDNIL };
static int g_fdBuff[2] = { QJACKCTL_FDNIL, QJACKCTL_FDNIL };
static int g_fdShut[2] = { QJACKCTL_FDNIL, QJACKCTL_FDNIL };

// To have clue about current buffer size (in frames).
static jack_nframes_t g_nframes = 0;

// Kind of constructor.
void qjackctlMainForm::init (void)
{
    m_pSetup = NULL;

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

    m_pStdoutNotifier = NULL;

    m_pAlsaNotifier = NULL;
    m_pPortNotifier = NULL;
    m_pXrunNotifier = NULL;
    m_pBuffNotifier = NULL;
    m_pShutNotifier = NULL;

    m_iAlsaNotify = 0;
    m_iPortNotify = 0;
    m_iXrunNotify = 0;
    m_iBuffNotify = 0;
    m_iShutNotify = 0;

    // All forms are to be created right now.
    m_pMessagesForm    = new qjackctlMessagesForm(this);
    m_pStatusForm      = new qjackctlStatusForm(this);
    m_pConnectionsForm = new qjackctlConnectionsForm(this);
    m_pPatchbayForm    = new qjackctlPatchbayForm(this);

    // Set the patchbay cable connection notification signal/slot.
    QObject::connect(&m_patchbayRack, SIGNAL(cableConnected(const QString&,const QString&,unsigned int)),
        this, SLOT(cableConnectSlot(const QString&,const QString&,unsigned int)));
}


// Kind of destructor.
void qjackctlMainForm::destroy (void)
{
    // Stop server, if not already...
    stopJack();

    // Terminate local ALSA sequencer interface.
    if (m_pAlsaNotifier)
        delete m_pAlsaNotifier;

    if (m_pAlsaSeq)
        snd_seq_close(m_pAlsaSeq);

    m_pAlsaNotifier = NULL;
    m_pAlsaSeq = NULL;
}


// Make and set a proper setup step.
bool qjackctlMainForm::setupInit ( qjackctlSetup *pSetup )
{
    // Finally, fix settings descriptor
    // and stabilize the form.
    m_pSetup = pSetup;

    // Set defaults...
    updateMessagesFont();
    updateTimeDisplayFonts();
    updateTimeDisplayToolTips();
    updateTimeFormat();
    updateActivePatchbay();

    // Initial XRUN statistics reset.
    resetXrunStats();

    // Check if we can redirect our own stdout/stderr...
    if (m_pSetup->bStdoutCapture && ::pipe(g_fdStdout) == 0) {
        ::dup2(g_fdStdout[QJACKCTL_FDWRITE], STDOUT_FILENO);
        ::dup2(g_fdStdout[QJACKCTL_FDWRITE], STDERR_FILENO);
        m_pStdoutNotifier = new QSocketNotifier(g_fdStdout[QJACKCTL_FDREAD], QSocketNotifier::Read, this);
        QObject::connect(m_pStdoutNotifier, SIGNAL(activated(int)), this, SLOT(stdoutNotifySlot(int)));
    }

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

    // Rather obvious setup.
    if (m_pConnectionsForm)
        m_pConnectionsForm->setAlsaSeq(m_pAlsaSeq);
    if (m_pPatchbayForm)
        m_pPatchbayForm->setAlsaSeq(m_pAlsaSeq);

    // Load patchbay from default path.
    if (m_pPatchbayForm && !m_pSetup->sPatchbayPath.isEmpty())
        m_pPatchbayForm->loadPatchbayFile(m_pSetup->sPatchbayPath);

    // Try to find if we can start in detached mode (client-only)
    // just in case there's a JACK server already running.
    m_bJackDetach = startJackClient(true);
    // Have we a command-line to start away?
    if (m_bJackDetach && !m_pSetup->sCmdLine.isEmpty()) {
        // Run it dettached...
        shellExecute(m_pSetup->sCmdLine, QString::null, QString::null);
        // And bail out...
        return false;
    }
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


// Show up initial widget geometries and visbility state.
void qjackctlMainForm::setupShow (void)
{
    // Try to restore old window positioning.
    m_pSetup->loadWidgetGeometry(this);
    // And for the whole widget gallore...
    m_pSetup->loadWidgetGeometry(m_pMessagesForm);
    m_pSetup->loadWidgetGeometry(m_pStatusForm);
    m_pSetup->loadWidgetGeometry(m_pConnectionsForm);
    m_pSetup->loadWidgetGeometry(m_pPatchbayForm);
    // Always make main form visible :)
    QWidget::show();
}


// Window close event handlers.
bool qjackctlMainForm::queryClose (void)
{
    bool bQueryClose = true;

    if (m_pJack && m_pJack->isRunning() && m_pSetup->bQueryClose) {
        switch (QMessageBox::warning(this, tr("Warning"),
            tr("JACK is currently running.") + "\n\n" +
            tr("Do you want to terminate the JACK audio server?"),
            tr("Terminate"), tr("Leave"), tr("Cancel"))) {
        case 0:     // Terminate...
            break;
        case 1:     // Leave...
            m_bJackSurvive = true;
            break;
        default:    // Cancel.
            bQueryClose = false;
            break;
        }
    }

    // Try to save current patchbay default settings.
    if (bQueryClose && m_pPatchbayForm) {
        bQueryClose = m_pPatchbayForm->queryClose();
        if (bQueryClose && !m_pPatchbayForm->patchbayPath().isEmpty())
            m_pSetup->sPatchbayPath = m_pPatchbayForm->patchbayPath();
    }

    // Some windows default fonts is here on demeand too.
    if (bQueryClose && m_pMessagesForm)
        m_pSetup->sMessagesFont = m_pMessagesForm->messagesFont().toString();

    // Try to save current positioning.
    if (bQueryClose) {
        m_pSetup->saveWidgetGeometry(m_pMessagesForm);
        m_pSetup->saveWidgetGeometry(m_pStatusForm);
        m_pSetup->saveWidgetGeometry(m_pConnectionsForm);
        m_pSetup->saveWidgetGeometry(m_pPatchbayForm);
        m_pSetup->saveWidgetGeometry(this);
    }

    return bQueryClose;
}


void qjackctlMainForm::closeEvent ( QCloseEvent *pCloseEvent )
{
    if (m_pSetup && m_pSetup->sCmdLine.isEmpty() && queryClose())
        pCloseEvent->accept();
    else
        pCloseEvent->ignore();
}


// Common exit status text formatter...
QString qjackctlMainForm::formatExitStatus ( int iExitStatus )
{
    QString sTemp = " ";

    if (iExitStatus == 0) {
        sTemp += tr("successfully");
    } else {
        sTemp += tr("with exit status");
        sTemp += "=";
        sTemp += QString::number(iExitStatus);
    }
    sTemp += ".";

    return sTemp;
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
   // Wait a litle bit (~half-second) before continue...
    QTime t;
    t.start();
    while (t.elapsed() < 500)
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
        switch (QMessageBox::warning(this, tr("Warning"),
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

    QString sTemp;

    sTemp = (m_bJackDetach ? tr("Activating") : tr("Starting"));
    setCaption(QJACKCTL_TITLE " [" + m_pSetup->sDefPreset + "] " + sTemp + "...");
    ServerStateTextLabel->setPaletteForegroundColor(Qt::yellow);
    updateStatus(STATUS_SERVER_STATE, sTemp);
    StartPushButton->setEnabled(false);

    // Reset our timer counters...
    m_iStartDelay  = 0;
    m_iTimerDelay  = 0;
    m_iJackRefresh = 0;

    // If we ain't to be the server master...
    if (m_bJackDetach) {
        StopPushButton->setEnabled(true);
        startJackClient(false);
        return;
    }

    // Load primary/default server preset...
    if (!m_pSetup->loadPreset(m_preset, m_pSetup->sDefPreset)) {
        appendMessagesError(tr("Could not load preset") + " \"" + m_pSetup->sDefPreset + "\". " + tr("Retrying with default."));
        m_pSetup->sDefPreset = "(default)";
        if (!m_pSetup->loadPreset(m_preset, m_pSetup->sDefPreset)) {
            appendMessagesError(tr("Could not load default preset. Sorry."));
            processJackExit();
            return;
        }
    }

    // Do we have any startup script?...
    if (m_pSetup->bStartupScript && !m_pSetup->sStartupScriptShell.isEmpty())
        shellExecute(m_pSetup->sStartupScriptShell, tr("Startup script..."), tr("Startup script terminated"));

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

    // Build process arguments...
    m_pJack->addArgument(m_preset.sServer);
    if (m_preset.bVerbose)
        m_pJack->addArgument("-v");
    if (m_preset.bRealtime)
        m_pJack->addArgument("-R");
    if (m_preset.iPriority > 0) {
        m_pJack->addArgument("-P" + QString::number(m_preset.iPriority));
    }
    if (m_preset.iTimeout > 0) {
        m_pJack->addArgument("-t" + QString::number(m_preset.iTimeout));
    }
    sTemp = m_preset.sDriver;
    m_pJack->addArgument("-d" + sTemp);
    bool bDummy     = (sTemp == "dummy");
    bool bAlsa      = (sTemp == "alsa");
    bool bPortaudio = (sTemp == "portaudio");
    if (bAlsa)
        m_pJack->addArgument("-d" + m_preset.sInterface);
    if (bPortaudio && m_preset.iChan > 0)
        m_pJack->addArgument("-c" + QString::number(m_preset.iChan));
    if (m_preset.iSampleRate > 0)
        m_pJack->addArgument("-r" + QString::number(m_preset.iSampleRate));
    if (m_preset.iFrames > 0)
        m_pJack->addArgument("-p" + QString::number(m_preset.iFrames));
    if (bAlsa) {
        if (m_preset.iPeriods > 0)
            m_pJack->addArgument("-n" + QString::number(m_preset.iPeriods));
        if (m_preset.bSoftMode)
            m_pJack->addArgument("-s");
        if (m_preset.bMonitor)
            m_pJack->addArgument("-m");
        if (m_preset.bShorts)
            m_pJack->addArgument("-S");
        if (m_preset.iInChannels > 0)
            m_pJack->addArgument("-i" + QString::number(m_preset.iInChannels));
        if (m_preset.iOutChannels > 0)
            m_pJack->addArgument("-o" + QString::number(m_preset.iOutChannels));
    }
    switch (m_preset.iAudio) {
    case 0:
    //  m_pJack->addArgument("-D");
        break;
    case 1:
        m_pJack->addArgument("-C");
        break;
    case 2:
        m_pJack->addArgument("-P");
        break;
    }
    if (bDummy && m_preset.iWait > 0)
        m_pJack->addArgument("-w" + QString::number(m_preset.iWait));
    if (!bDummy) {
        switch (m_preset.iDither) {
        case 0:
        //  m_pJack->addArgument("-z-");
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

    appendMessages(tr("JACK is starting..."));
    QStringList list = m_pJack->arguments();
    QStringList::Iterator iter = list.begin();
    sTemp = "";
    while (iter != list.end()) {
	    sTemp += *iter++;
        sTemp += " ";
    }
    appendMessagesColor(sTemp.stripWhiteSpace(), "#990099");

    // Go jack, go...
    if (!m_pJack->start()) {
        appendMessagesError(tr("Could not start JACK. Sorry."));
        processJackExit();
        return;
    }

    // Show startup results...
    sTemp = " " + tr("with") + " ";
    sTemp += tr("PID");
    sTemp += "=";
    sTemp += QString::number((long) m_pJack->processIdentifier());
    sTemp += " (0x";
    sTemp += QString::number((long) m_pJack->processIdentifier(), 16);
    sTemp += ").";
    appendMessages(tr("JACK was started") + sTemp);

    sTemp = tr("Started");
    setCaption(QJACKCTL_TITLE " [" + m_pSetup->sDefPreset + "] " + sTemp + ".");
    updateStatus(STATUS_SERVER_STATE, sTemp);
    StopPushButton->setEnabled(true);

    // Reset (yet again) the timer counters...
    m_iStartDelay  = 1 + (m_preset.iStartDelay * 1000);
    m_iTimerDelay  = 0;
    m_iJackRefresh = 0;
}


// Stop jack audio server...
void qjackctlMainForm::stopJack (void)
{
    // Clear timer counters...
    m_iStartDelay  = 0;
    m_iTimerDelay  = 0;
    m_iJackRefresh = 0;

    // Stop client code.
    stopJackClient();

    // And try to stop server.
    if (m_pJack && !m_bJackSurvive) {
        appendMessages(tr("JACK is stopping..."));
        QString sTemp = tr("Stopping");
        setCaption(QJACKCTL_TITLE " [" + m_pSetup->sDefPreset + "] " + sTemp + "...");
        updateStatus(STATUS_SERVER_STATE, sTemp);
        if (m_pJack->isRunning())
            m_pJack->tryTerminate();
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
        // Do we have any shutdown script?...
        if (!m_bJackSurvive && m_pSetup->bShutdownScript && !m_pSetup->sShutdownScriptShell.isEmpty())
            shellExecute(m_pSetup->sShutdownScriptShell, tr("Shutdown script..."), tr("Shutdown script terminated"));
    }

    QString sTemp;
    if (m_bJackDetach)
        sTemp = (m_pJackClient == NULL ? tr("Inactive") : tr("Active"));
    else
        sTemp = tr("Stopped");
    setCaption(QJACKCTL_TITLE " [" + m_pSetup->sDefPreset + "] " + sTemp + ".");
    ServerStateTextLabel->setPaletteForegroundColor(m_pJackClient == NULL ? Qt::darkYellow : Qt::yellow);
    updateStatus(STATUS_SERVER_STATE, sTemp);
    StartPushButton->setEnabled(m_pJackClient == NULL);
    StopPushButton->setEnabled(m_pJackClient != NULL);
    PlayPushButton->setEnabled(false);
    PausePushButton->setEnabled(false);
}


// XRUN detection routine.
QString& qjackctlMainForm::detectXrun( QString & s )
{
    QRegExp rx(m_pSetup->sXrunRegex);
    int iPos = rx.search(s);
    if (iPos >= 0) {
        s.insert(iPos + rx.matchedLength(), "</font>");
        s.insert(iPos, "<font color=\"#cc0000\">");
        if (m_iXrunStats > 0 || !m_pSetup->bXrunIgnoreFirst) {
            m_tXrunLast   = QTime::currentTime();
            m_fXrunLast   = rx.cap(1).toFloat();
            m_fXrunTotal += m_fXrunLast;
            if (m_fXrunLast < m_fXrunMin || m_iXrunCount == 0)
                m_fXrunMin = m_fXrunLast;
            if (m_fXrunLast > m_fXrunMax || m_iXrunCount == 0)
                m_fXrunMax = m_fXrunLast;
            m_iXrunCount++;
            m_tXrunLast.restart();
            refreshXrunStats();
        }
        m_iXrunStats++;
    }
    return s;
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
    appendMessagesColor(s, "#ff0000");

    QMessageBox::critical(this, tr("Error"), s, tr("Cancel"));
}


// Force update of the messages font.
void qjackctlMainForm::updateMessagesFont (void)
{
    if (m_pMessagesForm && !m_pSetup->sMessagesFont.isEmpty()) {
        QFont font;
        if (font.fromString(m_pSetup->sMessagesFont))
            m_pMessagesForm->setMessagesFont(font);
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
    // Time to load the active patchbay rack profiler?
    if (m_pSetup->bActivePatchbay && !m_pSetup->sActivePatchbayPath.isEmpty()) {
        if (!qjackctlPatchbayFile::load(&m_patchbayRack, m_pSetup->sActivePatchbayPath)) {
            appendMessagesError(tr("Could not load active patchbay definition. Disabled."));
            m_pSetup->bActivePatchbay = false;
        } else {
            // If we're up and running, make it dirty :)
            if (m_pJackClient)
                m_iJackDirty++;
            if (m_pAlsaSeq)
                m_iAlsaDirty++;
        }
    }
}

// Force active patchbay setting.
void qjackctlMainForm::activatePatchbay ( const QString& sPatchbayPath )
{
    if (!sPatchbayPath.isEmpty()) {
        m_pSetup->bActivePatchbay = true;
        m_pSetup->sActivePatchbayPath = sPatchbayPath;
    }
    updateActivePatchbay();
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

    refreshXrunStats();

    appendMessages(tr("Statistics reset."));
}


// Update the XRUN count/callbacks item.
void qjackctlMainForm::updateXrunCount (void)
{
    QString sText = QString::number(m_iXrunCount);
    sText += " (";
    sText += QString::number(m_iXrunCallbacks);
    sText += ")";
    updateStatus(STATUS_XRUN_COUNT, sText);
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
        updateStatus(STATUS_RESET_TIME, formatElapsedTime(STATUS_RESET_TIME, m_tResetLast, true));
        updateStatus(STATUS_XRUN_TIME, formatElapsedTime(STATUS_XRUN_TIME, m_tXrunLast, ((m_iXrunCount + m_iXrunCallbacks) > 0)));
    }
}


// Update the XRUN list view items.
void qjackctlMainForm::refreshXrunStats (void)
{
    updateXrunCount();

    if (m_bJackDetach) {
        QString n = "--";
        updateStatus(STATUS_XRUN_TOTAL, n);
        updateStatus(STATUS_XRUN_MIN, n);
        updateStatus(STATUS_XRUN_MAX, n);
        updateStatus(STATUS_XRUN_AVG, n);
        updateStatus(STATUS_XRUN_LAST, n);
    } else {
        float fXrunAverage = 0.0;
        if (m_iXrunCount > 0)
            fXrunAverage = (m_fXrunTotal / m_iXrunCount);
        QString s = " " + tr("msec");
        updateStatus(STATUS_XRUN_TOTAL, QString::number(m_fXrunTotal) + s);
        updateStatus(STATUS_XRUN_MIN, QString::number(m_fXrunMin) + s);
        updateStatus(STATUS_XRUN_MAX, QString::number(m_fXrunMax) + s);
        updateStatus(STATUS_XRUN_AVG, QString::number(fXrunAverage) + s);
        updateStatus(STATUS_XRUN_LAST, QString::number(m_fXrunLast) + s);
    }
    
    updateElapsedTimes();
}


// Jack port registration callback funtion, called
// whenever a jack port is registered or unregistered.
static void qjackctl_portRegistrationCallback ( jack_port_id_t, int, void * )
{
    char c = 0;

    ::write(g_fdPort[QJACKCTL_FDWRITE], &c, sizeof(c));
}


// Jack graph order callback function, called
// whenever the processing graph is reordered.
static int qjackctl_graphOrderCallback ( void * )
{
    char c = 0;

    ::write(g_fdPort[QJACKCTL_FDWRITE], &c, sizeof(c));

    return 0;
}


// Jack XRUN callback function, called
// whenever there is a xrun.
static int qjackctl_xrunCallback ( void * )
{
    char c = 0;

    ::write(g_fdXrun[QJACKCTL_FDWRITE], &c, sizeof(c));

    return 0;
}

// Jack buffer size function, called
// whenever the server changes buffer size.
static int qjackctl_bufferSizeCallback ( jack_nframes_t nframes, void * )
{
    char c = 0;

    // Update our global static variable.
    g_nframes = nframes;

    ::write(g_fdBuff[QJACKCTL_FDWRITE], &c, sizeof(c));

    return 0;
}


// Jack shutdown function, called
// whenever the server terminates this client.
static void qjackctl_shutdown ( void * )
{
    char c = 0;

    ::write(g_fdShut[QJACKCTL_FDWRITE], &c, sizeof(c));
}


// Jack socket notifier port/graph callback funtion.
void qjackctlMainForm::portNotifySlot ( int fd )
{
    char c = 0;

    if (m_iPortNotify > 0)
        return;
    m_iPortNotify++;

    // Read from our pipe.
    ::read(fd, &c, sizeof(c));

    // Log some message here, if new.
    if (m_iJackRefresh == 0)
        appendMessagesColor(tr("Audio connection graph change."), "#cc9966");
    // Do what has to be done.
    refreshJackConnections();
    // We'll be dirty too...
    m_iJackDirty++;

    m_iPortNotify--;
}


// Jack socket notifier XRUN callback funtion.
void qjackctlMainForm::xrunNotifySlot ( int fd )
{
    char c = 0;

    if (m_iXrunNotify > 0)
        return;
    m_iXrunNotify++;

    // Read from our pipe.
    ::read(fd, &c, sizeof(c));

    // Just increment callback counter.
    m_iXrunCallbacks++;
    m_tXrunLast.restart();
    // Update the status item directly.
    updateXrunCount();
    // Log highlight this event.
    appendMessagesColor(tr("XRUN callback.") + " (" + QString::number(m_iXrunCallbacks) + ")", "#cc99cc");

    m_iXrunNotify--;
}


// Jack buffer size notifier callback funtion.
void qjackctlMainForm::buffNotifySlot ( int fd )
{
    char c = 0;

    if (m_iBuffNotify > 0)
        return;
    m_iBuffNotify++;

    // Read from our pipe.
    ::read(fd, &c, sizeof(c));

    // Don't need to nothing, it was handled on qjackctl_bufferSizeCallback;
    // just log this event as routine.
    appendMessagesColor(tr("Buffer size change.") + " (" + QString::number((int) g_nframes) + ")", "#cc9966");

    m_iBuffNotify--;
}


// Jack socket notifier callback funtion.
void qjackctlMainForm::shutNotifySlot ( int fd )
{
    char c = 0;

    if (m_iShutNotify > 0)
        return;
    m_iShutNotify++;

    // Read from our pipe.
    ::read(fd, &c, sizeof(c));

    // Log this event.
    appendMessagesColor(tr("Shutdown notification."), "#cc9999");
    // Do what has to be done.
    stopJack();
    // We're not detached anymore, anyway.
    m_bJackDetach = false;

    m_iShutNotify--;
}


// ALSA announce slot.
void qjackctlMainForm::alsaNotifySlot ( int /*fd*/ )
{
    if (m_iAlsaNotify > 0)
        return;
    m_iAlsaNotify++;

    snd_seq_event_t *pAlsaEvent;

    snd_seq_event_input(m_pAlsaSeq, &pAlsaEvent);
    snd_seq_free_event(pAlsaEvent);

    // Log some message here, if new.
    if (m_iAlsaRefresh == 0)
        appendMessagesColor(tr("MIDI connection graph change."), "#66cc99");
    // Do what has to be done.
    refreshAlsaConnections();
    // We'll be dirty too...
    m_iAlsaDirty++;

    m_iAlsaNotify--;
}


// Timer callback funtion.
void qjackctlMainForm::timerSlot (void)
{
    // Is it the first shot on server start after a few delay?
    if (m_iTimerDelay < m_iStartDelay) {
        m_iTimerDelay += QJACKCTL_TIMER_MSECS;
        if (m_iTimerDelay >= m_iStartDelay) {
            // If we cannot start it now, maybe a lil'mo'later ;)
            if (!startJackClient(false)) {
                m_iStartDelay += m_iTimerDelay;
                m_iTimerDelay  = 0;
            }
        }
    }

    // Is the connection patchbay dirty enough?
    if (m_pConnectionsForm) {
        // Are we about to enforce an audio connections persistence profile?
        if (m_iJackDirty > 0) {
            m_iJackDirty = 0;
            if (m_pSetup->bActivePatchbay) {
                appendMessagesColor(tr("Audio active patchbay scan") + "...", "#6699cc");
                m_patchbayRack.connectAudioScan(m_pJackClient);
            }
            refreshJackConnections();
        }
        // Or is it from the MIDI field?
        if (m_iAlsaDirty > 0) {
            m_iAlsaDirty = 0;
            if (m_pSetup->bActivePatchbay) {
                appendMessagesColor(tr("MIDI active patchbay scan") + "...", "#99cc66");
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
        appendMessagesColor(tr("Audio connection change") + ".", "#9999cc");
}


// ALSA connection notification slot.
void qjackctlMainForm::alsaConnectChanged (void)
{
    // Just shake the MIDI connections status quo.
    if (++m_iAlsaDirty == 1)
        appendMessagesColor(tr("MIDI connection change") + ".", "#cccc99");
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


// Close notification pipes.
void qjackctlMainForm::closePipes (void)
{
    // Port/Graph notification pipe.
    if (g_fdPort[QJACKCTL_FDREAD] != QJACKCTL_FDNIL) {
        ::close(g_fdPort[QJACKCTL_FDREAD]);
        g_fdPort[QJACKCTL_FDREAD] = QJACKCTL_FDNIL;
    }
    if (g_fdPort[QJACKCTL_FDWRITE] != QJACKCTL_FDNIL) {
        ::close(g_fdPort[QJACKCTL_FDWRITE]);
        g_fdPort[QJACKCTL_FDWRITE] = QJACKCTL_FDNIL;
    }
    // XRUN notification pipe.
    if (g_fdXrun[QJACKCTL_FDREAD] != QJACKCTL_FDNIL) {
        ::close(g_fdXrun[QJACKCTL_FDREAD]);
        g_fdXrun[QJACKCTL_FDREAD] = QJACKCTL_FDNIL;
    }
    if (g_fdXrun[QJACKCTL_FDWRITE] != QJACKCTL_FDNIL) {
        ::close(g_fdXrun[QJACKCTL_FDWRITE]);
        g_fdXrun[QJACKCTL_FDWRITE] = QJACKCTL_FDNIL;
    }
    // Buffer size notification pipe.
    if (g_fdBuff[QJACKCTL_FDREAD] != QJACKCTL_FDNIL) {
        ::close(g_fdBuff[QJACKCTL_FDREAD]);
        g_fdBuff[QJACKCTL_FDREAD] = QJACKCTL_FDNIL;
    }
    if (g_fdBuff[QJACKCTL_FDWRITE] != QJACKCTL_FDNIL) {
        ::close(g_fdBuff[QJACKCTL_FDWRITE]);
        g_fdBuff[QJACKCTL_FDWRITE] = QJACKCTL_FDNIL;
    }
    // Shutdown notification pipe.
    if (g_fdShut[QJACKCTL_FDREAD] != QJACKCTL_FDNIL) {
        ::close(g_fdShut[QJACKCTL_FDREAD]);
        g_fdShut[QJACKCTL_FDREAD] = QJACKCTL_FDNIL;
    }
    if (g_fdShut[QJACKCTL_FDWRITE] != QJACKCTL_FDNIL) {
        ::close(g_fdShut[QJACKCTL_FDWRITE]);
        g_fdShut[QJACKCTL_FDWRITE] = QJACKCTL_FDNIL;
    }
}


// Start our jack audio control client...
bool qjackctlMainForm::startJackClient ( bool bDetach )
{
    // If can't be already started, are we?
    if (m_pJackClient)
        return true;

    // Are we about to start detached?
    if (bDetach) {
        // To fool timed client initialization delay.
        m_iTimerDelay += (m_iStartDelay + 1);
        // Refresh status (with dashes?)
        refreshStatus();
    }

    // Create port notification pipe.
    if (::pipe(g_fdPort) < 0) {
        g_fdPort[QJACKCTL_FDREAD]  = QJACKCTL_FDNIL;
        g_fdPort[QJACKCTL_FDWRITE] = QJACKCTL_FDNIL;
        closePipes();
        appendMessagesError(tr("Could not create port notification pipe."));
        return false;
    }

    // Create XRUN notification pipe.
    if (::pipe(g_fdXrun) < 0) {
        g_fdXrun[QJACKCTL_FDREAD]  = QJACKCTL_FDNIL;
        g_fdXrun[QJACKCTL_FDWRITE] = QJACKCTL_FDNIL;
        closePipes();
        appendMessagesError(tr("Could not create XRUN notification pipe."));
        return false;
    }

    // Create buffer size notification pipe.
    if (::pipe(g_fdBuff) < 0) {
        g_fdBuff[QJACKCTL_FDREAD]  = QJACKCTL_FDNIL;
        g_fdBuff[QJACKCTL_FDWRITE] = QJACKCTL_FDNIL;
        closePipes();
        appendMessagesError(tr("Could not create buffer size notification pipe."));
        return false;
    }

    // Create shutdown notification pipe.
    if (::pipe(g_fdShut) < 0) {
        g_fdShut[QJACKCTL_FDREAD]  = QJACKCTL_FDNIL;
        g_fdShut[QJACKCTL_FDWRITE] = QJACKCTL_FDNIL;
        closePipes();
        appendMessagesError(tr("Could not create shutdown notification pipe."));
        return false;
    }

    // Create the jack client handle, using a distinct identifier (PID?)
    // surely
    QString sClientName = "qjackctl-" + QString::number((int) ::getpid());
    m_pJackClient = jack_client_new(sClientName.latin1());
    if (m_pJackClient == NULL) {
        closePipes();
        if (!bDetach)
            appendMessagesError(tr("Could not connect to JACK server as client."));
        return false;
    }

    // Set notification callbacks.
    jack_set_graph_order_callback(m_pJackClient, qjackctl_graphOrderCallback, NULL);
    jack_set_port_registration_callback(m_pJackClient, qjackctl_portRegistrationCallback, NULL);
    jack_set_xrun_callback(m_pJackClient, qjackctl_xrunCallback, NULL);
    jack_set_buffer_size_callback(m_pJackClient, qjackctl_bufferSizeCallback, NULL);
    jack_on_shutdown(m_pJackClient, qjackctl_shutdown, NULL);

    // Create our notification managers.
    m_pPortNotifier = new QSocketNotifier(g_fdPort[QJACKCTL_FDREAD], QSocketNotifier::Read);
    m_pXrunNotifier = new QSocketNotifier(g_fdXrun[QJACKCTL_FDREAD], QSocketNotifier::Read);
    m_pBuffNotifier = new QSocketNotifier(g_fdBuff[QJACKCTL_FDREAD], QSocketNotifier::Read);
    m_pShutNotifier = new QSocketNotifier(g_fdShut[QJACKCTL_FDREAD], QSocketNotifier::Read);

    // And connect it to the proper slots.
    QObject::connect(m_pPortNotifier, SIGNAL(activated(int)), this, SLOT(portNotifySlot(int)));
    QObject::connect(m_pXrunNotifier, SIGNAL(activated(int)), this, SLOT(xrunNotifySlot(int)));
    QObject::connect(m_pBuffNotifier, SIGNAL(activated(int)), this, SLOT(buffNotifySlot(int)));
    QObject::connect(m_pShutNotifier, SIGNAL(activated(int)), this, SLOT(shutNotifySlot(int)));

    // First knowledge about buffer size.
    g_nframes = jack_get_buffer_size(m_pJackClient);

    // Reconstruct our connections patchbay...
    if (m_pConnectionsForm)
        m_pConnectionsForm->setJackClient(m_pJackClient);
    if (m_pPatchbayForm)
        m_pPatchbayForm->setJackClient(m_pJackClient);

    // Do not forget to reset XRUN stats variables.
    if (!bDetach)
        resetXrunStats();

    // Activate us as a client...
    jack_activate(m_pJackClient);

    // Remember to schedule an initial connection refreshment.
    refreshConnections();

    // All displays are highlighted from now on.
    ServerStateTextLabel->setPaletteForegroundColor(Qt::yellow);
    CpuLoadTextLabel->setPaletteForegroundColor(Qt::yellow);
    TimeDisplayTextLabel->setPaletteForegroundColor(Qt::green);
    TransportStateTextLabel->setPaletteForegroundColor(Qt::green);
    TransportBPMTextLabel->setPaletteForegroundColor(Qt::green);
    TransportTimeTextLabel->setPaletteForegroundColor(Qt::green);

    // If we've started detached, just change active status.
    if (m_bJackDetach) {
        QString sTemp = tr("Active");
        setCaption(QJACKCTL_TITLE " [" + m_pSetup->sDefPreset + "] " + sTemp + ".");
        updateStatus(STATUS_SERVER_STATE, sTemp);
        StopPushButton->setEnabled(true);
    }

    // Log success here.
    appendMessages(tr("Client activated."));

    // Do we have any post-startup scripting?...
    // (only if we're not a detached client)
    if (!bDetach && !m_bJackDetach) {
        if (m_pSetup->bPostStartupScript && !m_pSetup->sPostStartupScriptShell.isEmpty())
            shellExecute(m_pSetup->sPostStartupScriptShell, tr("Post-startup script..."), tr("Post-startup script terminated"));
        // Have we an initial command-line to start away?
        if (!m_pSetup->sCmdLine.isEmpty()) {
            // Run it dettached...
            shellExecute(m_pSetup->sCmdLine, tr("Command line argument..."), tr("Command line argument started"));
            // And reset it forever more...
            m_pSetup->sCmdLine = QString::null;
        }
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

    // Close us as a client...
    if (m_pJackClient)
        jack_client_close(m_pJackClient);
    m_pJackClient = NULL;

    // Close notification pipes.
    closePipes();

    // Destroy socket notifiers.
    if (m_pPortNotifier)
        delete m_pPortNotifier;
    m_pPortNotifier = NULL;
    m_iPortNotify = 0;

    if (m_pXrunNotifier)
        delete m_pXrunNotifier;
    m_pXrunNotifier = NULL;
    m_iXrunNotify = 0;

    if (m_pBuffNotifier)
        delete m_pBuffNotifier;
    m_pBuffNotifier = NULL;
    m_iBuffNotify = 0;

    if (m_pShutNotifier)
        delete m_pShutNotifier;
    m_pShutNotifier = NULL;
    m_iShutNotify = 0;

    // Displays are deemed again.
    CpuLoadTextLabel->setPaletteForegroundColor(Qt::darkYellow);
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



// Message log form requester slot.
void qjackctlMainForm::toggleMessagesForm (void)
{
    if (m_pMessagesForm) {
        m_pSetup->saveWidgetGeometry(m_pMessagesForm);
        if (m_pMessagesForm->isVisible())
            m_pMessagesForm->hide();
        else
            m_pMessagesForm->show();
    }
}


// Status form requester slot.
void qjackctlMainForm::toggleStatusForm (void)
{
    if (m_pStatusForm) {
        m_pSetup->saveWidgetGeometry(m_pStatusForm);
        if (m_pStatusForm->isVisible())
            m_pStatusForm->hide();
        else
            m_pStatusForm->show();
    }
}


// Connections form requester slot.
void qjackctlMainForm::toggleConnectionsForm (void)
{
    if (m_pConnectionsForm) {
        m_pSetup->saveWidgetGeometry(m_pConnectionsForm);
        m_pConnectionsForm->setJackClient(m_pJackClient);
        m_pConnectionsForm->setAlsaSeq(m_pAlsaSeq);
        if (m_pConnectionsForm->isVisible())
            m_pConnectionsForm->hide();
        else
            m_pConnectionsForm->show();
    }
}


// Patchbay form requester slot.
void qjackctlMainForm::togglePatchbayForm (void)
{
    if (m_pPatchbayForm) {
        m_pSetup->saveWidgetGeometry(m_pPatchbayForm);
        m_pPatchbayForm->setJackClient(m_pJackClient);
        m_pPatchbayForm->setAlsaSeq(m_pAlsaSeq);
        if (m_pPatchbayForm->isVisible())
            m_pPatchbayForm->hide();
        else
            m_pPatchbayForm->show();
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
        // To track down immediate changes.
        QString sOldMessagesFont       = m_pSetup->sMessagesFont;
        QString sOldDisplayFont1       = m_pSetup->sDisplayFont1;
        QString sOldDisplayFont2       = m_pSetup->sDisplayFont2;
        int     iOldTimeDisplay        = m_pSetup->iTimeDisplay;
        int     iOldTimeFormat         = m_pSetup->iTimeFormat;
        bool    bOldActivePatchbay     = m_pSetup->bActivePatchbay;
        QString sOldActivePatchbayPath = m_pSetup->sActivePatchbayPath;
        // Load the current setup settings.
        pSetupForm->setup(m_pSetup);
        // Show the setup dialog...
        if (pSetupForm->exec()) {
            // Check wheather something immediate has changed.
            if (sOldMessagesFont != m_pSetup->sMessagesFont)
                updateMessagesFont();
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


// Transport start (play)
void qjackctlMainForm::transportStart()
{
#ifdef CONFIG_JACK_TRANSPORT
    if (m_pJackClient) {
        jack_transport_start(m_pJackClient);
        updateStatus(STATUS_TRANSPORT_STATE, tr("Starting"));
        // Log this here.
        appendMessages(tr("Transport start."));
    }
#endif
}

// Transport stop (pause).
void qjackctlMainForm::transportStop()
{
#ifdef CONFIG_JACK_TRANSPORT
    if (m_pJackClient) {
        jack_transport_stop(m_pJackClient);
        updateStatus(STATUS_TRANSPORT_STATE, tr("Stopping"));
        // Log this here.
        appendMessages(tr("Transport stop."));
    }
#endif
}


// Almost-complete running status refresher.
void qjackctlMainForm::refreshStatus (void)
{
    const QString n = "--";
    const QString b = "--:--.----";
    const QString sStopped = tr("Stopped");

    if (m_pJackClient) {
        QString s = " ";
        updateStatus(STATUS_CPU_LOAD, QString::number(jack_cpu_load(m_pJackClient), 'g', 2) + s + "%");
        updateStatus(STATUS_SAMPLE_RATE, QString::number(jack_get_sample_rate(m_pJackClient)) + s + tr("Hz"));
        updateStatus(STATUS_BUFFER_SIZE, QString::number(g_nframes) + " " + tr("frames"));
#ifdef CONFIG_JACK_REALTIME
        updateStatus(STATUS_REALTIME, (jack_is_realtime(m_pJackClient) ? tr("Yes") : tr("No")));
#else
        updateStatus(STATUS_REALTIME, n);
#endif
#ifdef CONFIG_JACK_TRANSPORT
        QString sText = n;
        jack_position_t tpos;
        jack_transport_state_t tstate = jack_transport_query(m_pJackClient, &tpos);
        bool bPlaying = (tstate == JackTransportRolling || tstate == JackTransportLooping);
        switch (tstate) {
            case JackTransportStopped:
                sText = sStopped;
                break;
            case JackTransportRolling:
                sText = tr("Rolling");
                break;
            case JackTransportLooping:
                sText = tr("Looping");
                break;
            case JackTransportStarting:
                sText = tr("Starting");
                break;
        }
        updateStatus(STATUS_TRANSPORT_STATE, sText);
        // Transport timecode position.
    //  if (bPlaying)
            updateStatus(STATUS_TRANSPORT_TIME, formatTime((double) tpos.frame / (double) tpos.frame_rate));
    //  else
    //      updateStatus(STATUS_TRANSPORT_TIME, m_sTimeDashes);
        // Transport barcode position (bar:beat.tick)
        if (tpos.valid & JackPositionBBT) {
            updateStatus(STATUS_TRANSPORT_BBT, QString().sprintf("%u:%u.%04u", tpos.bar, tpos.beat, tpos.tick));
            updateStatus(STATUS_TRANSPORT_BPM, QString::number(tpos.beats_per_minute));
        } else {
            updateStatus(STATUS_TRANSPORT_BBT, b);
            updateStatus(STATUS_TRANSPORT_BPM, n);
        }
        PlayPushButton->setEnabled(tstate == JackTransportStopped);
        PausePushButton->setEnabled(bPlaying);
#else   // !CONFIG_JACK_TRANSPORT
        updateStatus(STATUS_TRANSPORT_STATE, n);
        updateStatus(STATUS_TRANSPORT_TIME, m_sTimeDashes);
        updateStatus(STATUS_TRANSPORT_BBT, b);
        updateStatus(STATUS_TRANSPORT_BPM, n);
        PlayPushButton->setEnabled(false);
        PausePushButton->setEnabled(false);
#endif
    } else {
        updateStatus(STATUS_CPU_LOAD, n);
        updateStatus(STATUS_SAMPLE_RATE, n);
        updateStatus(STATUS_BUFFER_SIZE, n);
        updateStatus(STATUS_REALTIME, n);
        updateStatus(STATUS_TRANSPORT_STATE, n);
        updateStatus(STATUS_TRANSPORT_TIME, m_sTimeDashes);
        updateStatus(STATUS_TRANSPORT_BBT, b);
        updateStatus(STATUS_TRANSPORT_BPM, n);
        PlayPushButton->setEnabled(false);
        PausePushButton->setEnabled(false);
    }

    updateElapsedTimes();
}


// Status item updater.
void qjackctlMainForm::updateStatus( int iStatusItem, const QString& sText )
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
    {
        QColor fgcolor = (m_pJackClient ? Qt::green : Qt::darkGreen);
        if ((m_iXrunCount + m_iXrunCallbacks) > 0) {
            if (m_iXrunCallbacks > 0)
                fgcolor = (m_pJackClient ? Qt::red : Qt::darkRed);
            else
                fgcolor = (m_pJackClient ? Qt::yellow : Qt::darkYellow);
        }
        XrunCountTextLabel->setPaletteForegroundColor(fgcolor);
        XrunCountTextLabel->setText(sText);
        break;
    }
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
        m_pStatusForm->updateStatus(iStatusItem, sText);
}


// end of qjackctlMainForm.ui.h
