// qjackctlMainForm.ui.h
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

#include <qapplication.h>
#include <qeventloop.h>
#include <qmessagebox.h>
#include <qregexp.h>

#include "config.h"

#include "qjackctlStatus.h"
#include "qjackctlAbout.h"


// Time in seconds of client start code delay.
#define QJACKCTL_START_DELAY    3

// To have clue about current buffer size (in frames).
static jack_nframes_t g_nframes = 0;

// Kind of constructor.
void qjackctlMainForm::init (void)
{
    m_pJack       = NULL;
    m_pJackClient = NULL;
    m_bJackDetach = false;
    m_pTimer      = new QTimer(this);
    m_iTimerSlot  = 0;
    m_iRefresh    = 0;

    m_pPortNotifier = NULL;
    m_pXrunNotifier = NULL;
    m_pBuffNotifier = NULL;
    m_pShutNotifier = NULL;

    m_iPortNotify = 0;
    m_iXrunNotify = 0;
    m_iBuffNotify = 0;
    m_iShutNotify = 0;

    // Messages and Status forms are the only ones
    // who will be created here and now;
    // other modeless forms get done on demand.
    m_pMessagesForm    = new qjackctlMessagesForm(this);
    m_pStatusForm      = new qjackctlStatusForm(this);
    m_pConnectionsForm = NULL;
    m_pPatchbayForm    = NULL;

    // Register the timer slot.
    QObject::connect(m_pTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
    
    // Load any saved profile settings and options.
    m_setup.load();
    // Try to restore old window positioning.
    m_setup.loadWidgetGeometry(this);
    // And for the whole widget gallore...
    m_setup.loadWidgetGeometry(m_pMessagesForm);
    m_setup.loadWidgetGeometry(m_pStatusForm);
    m_setup.loadWidgetGeometry(m_pConnectionsForm);
    m_setup.loadWidgetGeometry(m_pPatchbayForm);

    // Initial XRUN statistics reset.
    resetXrunStats();
    // Try to find if we can start in detached mode (client-only)
    // just in case there's a JACK server already running.
    m_bJackDetach = startJackClient(true);
    // Final startup stabilization...
    stabilizeForm();
    processJackExit();
    
    // Our timer is one second standard.
    m_pTimer->start(1000, false);
}


// Kind of destructor.
void qjackctlMainForm::destroy (void)
{
    // Stop server, if not already...
    stopJack();

    // Save profile settings and options.
    m_setup.save();

    // Kill timer.
    delete m_pTimer;
}


// Window close event handlers.
bool qjackctlMainForm::queryClose (void)
{
    bool bQueryClose = true;

    if (m_pJack && m_pJack->isRunning()) {
        bQueryClose = (QMessageBox::warning(this, tr("Warning"),
            tr("JACK is currently running.") + "\n\n" +
            tr("Closing this application will also terminate the JACK audio server."),
            tr("OK"), tr("Cancel")) == 0);
    }
    
    // Try to save current patchbay default settings.
    if (m_pPatchbayForm) {
        bQueryClose = m_pPatchbayForm->queryClose();
        if (bQueryClose && !m_pPatchbayForm->patchbayPath().isEmpty())
            m_setup.sPatchbayPath = m_pPatchbayForm->patchbayPath();
    }

    // Try to save current window positioning.
    if (bQueryClose) {
        m_setup.saveWidgetGeometry(m_pMessagesForm);
        m_setup.saveWidgetGeometry(m_pStatusForm);
        m_setup.saveWidgetGeometry(m_pConnectionsForm);
        m_setup.saveWidgetGeometry(m_pPatchbayForm);
        m_setup.saveWidgetGeometry(this);
    }

    return bQueryClose;
}

void qjackctlMainForm::reject (void)
{
    if (queryClose())
        QDialog::reject();
}

void qjackctlMainForm::closeEvent ( QCloseEvent *pCloseEvent )
{
    if (queryClose())
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


// Common shell executive...
void qjackctlMainForm::shellExecute ( const QString& sShellCommand, const QString& sStartMessage, const QString& sStopMessage )
{
    appendMessages(sStartMessage);
    QString sTemp = "[" + sShellCommand;
    appendMessages(sTemp.stripWhiteSpace() + "]");
    QApplication::eventLoop()->processEvents(QEventLoop::ExcludeUserInput);
    appendMessages(sStopMessage + formatExitStatus(::system(sShellCommand)));
   // Wait a litle bit (~1 second) before continue...
    QTime t;
    t.start();
    while (t.elapsed() < 1000)
        QApplication::eventLoop()->processEvents(QEventLoop::ExcludeUserInput);
}


// Start jack audio server...
void qjackctlMainForm::startJack (void)
{
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
    setCaption(QJACKCTL_TITLE " - " + sTemp + "...");
    updateStatus(STATUS_SERVER_STATE, sTemp);
    StartPushButton->setEnabled(false);

    // Reset our timer counters...
    m_iTimerSlot = 0;
    m_iRefresh = 0;

    // If we're not to be the server master...
    if (m_bJackDetach)
        return;

    // Do we force aRts sound server?...
//  if (m_setup.bForceArts && !m_setup.sForceArtsShell.isEmpty())
//      shellExecute(m_setup.sForceArtsShell, tr("ARTS is being forced..."), tr("ARTS has been forced"));

    // Do we force stray JACK daemon threads?...
//  if (m_setup.bForceJack && !m_setup.sForceJackShell.isEmpty())
//      shellExecute(m_setup.sForceArtsShell, tr("JACK is being forced..."), tr("JACK has been forced"));

    // Do we have any startup script?...
    if (m_setup.bStartupScript && !m_setup.sStartupScriptShell.isEmpty())
        shellExecute(m_setup.sStartupScriptShell, tr("Startup script..."), tr("Startup script terminated"));

    // OK. Let's build the startup process...
    m_pJack = new QProcess(this);

    // Setup communications...
    m_pJack->setCommunication(QProcess::Stdout | QProcess::Stderr | QProcess::DupStderr);

    QObject::connect(m_pJack, SIGNAL(readyReadStdout()), this, SLOT(readJackStdout()));
    QObject::connect(m_pJack, SIGNAL(readyReadStderr()), this, SLOT(readJackStderr()));
    QObject::connect(m_pJack, SIGNAL(processExited()),   this, SLOT(processJackExit()));

    // Build process arguments...
    m_pJack->addArgument(m_setup.sServer);
    if (m_setup.bVerbose)
        m_pJack->addArgument("-v");
    if (m_setup.bRealtime)
        m_pJack->addArgument("-R");
    if (m_setup.iPriority > 0) {
        m_pJack->addArgument("-P");
        m_pJack->addArgument(QString::number(m_setup.iPriority));
    }
    if (m_setup.bAsio)
        m_pJack->addArgument("-a");
    if (m_setup.iTimeout > 0) {
        m_pJack->addArgument("-t");
        m_pJack->addArgument(QString::number(m_setup.iTimeout));
    }
    sTemp = m_setup.sTempDir;
    if (sTemp == "(default)")
        sTemp = "";
    if (!sTemp.isEmpty()) {
        m_pJack->addArgument("-D");
        m_pJack->addArgument(sTemp);
    }
    sTemp = m_setup.sDriver;
    m_pJack->addArgument("-d");
    m_pJack->addArgument(sTemp);
    bool bDummy     = (sTemp == "dummy");
    bool bAlsa      = (sTemp == "alsa");
    bool bPortaudio = (sTemp == "portaudio");
    if (bAlsa) {
        m_pJack->addArgument("-d");
        m_pJack->addArgument(m_setup.sInterface);
    }
    if (bPortaudio && m_setup.iChan > 0) {
        m_pJack->addArgument("-c");
        m_pJack->addArgument(QString::number(m_setup.iChan));
    }
    if (m_setup.iSampleRate > 0) {
        m_pJack->addArgument("-r");
        m_pJack->addArgument(QString::number(m_setup.iSampleRate));
    }
    if (m_setup.iFrames > 0) {
        m_pJack->addArgument("-p");
        m_pJack->addArgument(QString::number(m_setup.iFrames));
    }
    if (bAlsa) {
        if (!m_setup.bAsio && m_setup.iPeriods > 0) {
            m_pJack->addArgument("-n");
            m_pJack->addArgument(QString::number(m_setup.iPeriods));
        }
        if (m_setup.bSoftMode)
            m_pJack->addArgument("-s");
        if (m_setup.bMonitor)
            m_pJack->addArgument("-m");
    }
    switch (m_setup.iAudio) {
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
    if (bDummy && m_setup.iWait > 0) {
        m_pJack->addArgument("-w");
        m_pJack->addArgument(QString::number(m_setup.iWait));
    }
    if (!bDummy) {
        switch (m_setup.iDither) {
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
        if (m_setup.bHWMon)
            m_pJack->addArgument("-H");
        if (m_setup.bHWMeter)
            m_pJack->addArgument("-M");
    }

    appendMessages(tr("JACK is starting..."));
    QStringList list = m_pJack->arguments();
    QStringList::Iterator iter = list.begin();
    sTemp = "[";
    while( iter != list.end() ) {
	    sTemp += *iter++;
        sTemp += " ";
    }
    appendMessages(sTemp.stripWhiteSpace() + "]");

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
    setCaption(QJACKCTL_TITLE " - " + sTemp + ".");
    updateStatus(STATUS_SERVER_STATE, sTemp);
    StopPushButton->setEnabled(true);
}


// Stop jack audio server...
void qjackctlMainForm::stopJack (void)
{
    // Stop client code.
    stopJackClient();

    // And try to stop server.
    if (m_pJack == NULL) {
        processJackExit();
    } else {
        appendMessages(tr("JACK is stopping..."));
        QString sTemp = tr("Stopping");
        setCaption(QJACKCTL_TITLE " - " + sTemp + "...");
        updateStatus(STATUS_SERVER_STATE, sTemp);
        if (m_pJack->isRunning())
            m_pJack->tryTerminate();
        else
            processJackExit();
    }
}


// Stdout handler...
void qjackctlMainForm::readJackStdout (void)
{
    QString s = m_pJack->readStdout();
    appendMessagesText(detectXrun(s));
}


// Stderr handler...
void qjackctlMainForm::readJackStderr (void)
{
    QString s = m_pJack->readStderr();
    appendMessagesText(detectXrun(s));
}


// Jack audio server cleanup.
void qjackctlMainForm::processJackExit (void)
{
    // Force client code cleanup.
    if (!m_bJackDetach)
        stopJackClient();

    if (m_pJack) {
        // Force final server shutdown...
        appendMessages(tr("JACK was stopped") + formatExitStatus(m_pJack->exitStatus()));
        if (!m_pJack->normalExit())
            m_pJack->kill();
        delete m_pJack;
        // Do we have any shutdown script?...
        if (m_setup.bShutdownScript && !m_setup.sShutdownScriptShell.isEmpty())
            shellExecute(m_setup.sShutdownScriptShell, tr("Shutdown script..."), tr("Shutdown script terminated"));
    }
    m_pJack = NULL;

    QString sTemp;
    if (m_bJackDetach)
        sTemp = (m_pJackClient == NULL ? tr("Inactive") : tr("Active"));
    else
        sTemp = tr("Stopped");
    setCaption(QJACKCTL_TITLE " - " + sTemp + ".");
    updateStatus(STATUS_SERVER_STATE, sTemp);
    StartPushButton->setEnabled(m_pJackClient == NULL);
    StopPushButton->setEnabled(m_pJackClient != NULL);
    PlayPushButton->setEnabled(false);
    PausePushButton->setEnabled(false);
}


// XRUN detection routine.
QString& qjackctlMainForm::detectXrun( QString & s )
{
    QRegExp rx(m_setup.sXrunRegex);
    int iPos = rx.search(s);
    if (iPos >= 0) {
        s.insert(iPos + rx.matchedLength(), "</font>");
        s.insert(iPos, "<font color=\"#cc0000\">");
        if (m_iXrunStats > 0 || !m_setup.bXrunIgnoreFirst) {
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

    QMessageBox::critical(this, tr("Error"), s, QMessageBox::Cancel, QMessageBox::NoButton, QMessageBox::NoButton);
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


// Update the buffer size (nframes) item.
void qjackctlMainForm::updateBufferSize (void)
{
    updateStatus(STATUS_BUFFER_SIZE, QString::number(g_nframes) + " " + tr("frames"));
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


// Update the XRUN last/elapsed time item.
QString qjackctlMainForm::formatElapsedTime ( const QTime& t, bool bElapsed )
{
    QString sText;
    if (t.isNull()) {
        sText = "--:--:--";
    } else {
        sText = t.toString();
        if (m_pJackClient) {
            int iSeconds = (t.elapsed() / 1000);
            if (bElapsed && iSeconds > 0) {
                int iHours   = 0;
                int iMinutes = 0;
                sText += " (";
                if (iSeconds >= 3600) {
                    iHours = (iSeconds / 3600);
                    iSeconds -= (iHours * 3600);
                }
                if (iSeconds >= 60) {
                    iMinutes = (iSeconds / 60);
                    iSeconds -= (iMinutes * 60);
                }
                sText += QTime(iHours, iMinutes, iSeconds).toString() + ")";
            }
        }
    }
    return sText;
}


// Update the XRUN last/elapsed time item.
void qjackctlMainForm::updateElapsedTimes (void)
{
    updateStatus(STATUS_RESET_TIME, formatElapsedTime(m_tResetLast, true));
    updateStatus(STATUS_XRUN_TIME, formatElapsedTime(m_tXrunLast, ((m_iXrunCount + m_iXrunCallbacks) > 0)));
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


// Notification pipes descriptors
#define QJACKCTL_FDNIL     -1
#define QJACKCTL_FDREAD     0
#define QJACKCTL_FDWRITE    1

static int g_fdPort[2] = { QJACKCTL_FDNIL, QJACKCTL_FDNIL };
static int g_fdXrun[2] = { QJACKCTL_FDNIL, QJACKCTL_FDNIL };
static int g_fdBuff[2] = { QJACKCTL_FDNIL, QJACKCTL_FDNIL };
static int g_fdShut[2] = { QJACKCTL_FDNIL, QJACKCTL_FDNIL };

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

    // Log some message here.
    appendMessagesColor(tr("Connection graph change."), "#cc9966");
    // Do what has to be done.
    refreshConnections();

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

    // Log highlight this event.
    appendMessagesColor(tr("XRUN callback."), "#cc99cc");
    // Just increment callback counter.
    m_iXrunCallbacks++;
    m_tXrunLast.restart();
    // Update the status item directly.
    updateXrunCount();

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

    // Log this event.
    appendMessagesColor(tr("Buffer size change."), "#cc9966");
    // Update the status item directly.
    updateBufferSize();

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


// Timer callback funtion.
void qjackctlMainForm::timerSlot (void)
{
    // Is it the first shot on server start after a few delay?
    if (++m_iTimerSlot == QJACKCTL_START_DELAY) {
        startJackClient(false);
        return;
    }

    // Shall we refresh connections now and then?
    if (m_setup.bAutoRefresh && (m_iTimerSlot % m_setup.iTimeRefresh) == 0)
        refreshConnections();

    // Are we about to refresh it, really?
    if (m_iRefresh > 0) {
        m_iRefresh = 0;
        if (m_pConnectionsForm)
            m_pConnectionsForm->refresh(true);
    }

    // Update some statistical fields, directly.
    refreshStatus();
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
    if (bDetach)
        m_iTimerSlot += QJACKCTL_START_DELAY;
        
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

    // Create the jack client handle
    m_pJackClient = jack_client_new("qjackctl");
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
    
    // If we're started detached, just change active status.
    if (m_bJackDetach) {
        QString sTemp = tr("Active");
        setCaption(QJACKCTL_TITLE " - " + sTemp + ".");
        updateStatus(STATUS_SERVER_STATE, sTemp);
        StopPushButton->setEnabled(true);
    }

    // Log success here.
    appendMessages(tr("Client activated."));

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

    // Refresh jack client statistics explicitly.
    refreshXrunStats();
}


// Rebuild all patchbay items.
void qjackctlMainForm::refreshConnections (void)
{
    // Hack this as for a while.
    if (m_pConnectionsForm)
        m_pConnectionsForm->stabilize(false);

    // Just increment our intentions; it will be deferred
    // to be executed just on timer slot processing...
    m_iRefresh++;
}


// Message log form requester slot.
void qjackctlMainForm::toggleMessagesForm (void)
{
    if (m_pMessagesForm == NULL) {
        m_pMessagesForm = new qjackctlMessagesForm(this);
        m_setup.loadWidgetGeometry(m_pMessagesForm);
    }

    if (m_pMessagesForm) {
        if (m_pMessagesForm->isVisible())
            m_pMessagesForm->hide();
        else
            m_pMessagesForm->show();
    }
}


// Setup dialog requester slot.
void qjackctlMainForm::showSetupForm (void)
{
    qjackctlSetupForm *pSetupForm = new qjackctlSetupForm(this);
    if (pSetupForm) {
        pSetupForm->load(&m_setup);
        if (pSetupForm->exec())
            pSetupForm->save(&m_setup);
        delete pSetupForm;
    }
}


// Status form requester slot.
void qjackctlMainForm::toggleStatusForm (void)
{
    if (m_pStatusForm == NULL) {
        m_pStatusForm = new qjackctlStatusForm(this);
        m_setup.loadWidgetGeometry(m_pStatusForm);
    }
    
    if (m_pStatusForm) {
        if (m_pStatusForm->isVisible())
            m_pStatusForm->hide();
        else
            m_pStatusForm->show();
    }
}


// Connections form requester slot.
void qjackctlMainForm::toggleConnectionsForm (void)
{
    if (m_pConnectionsForm == NULL) {
        m_pConnectionsForm = new qjackctlConnectionsForm(this);
        m_setup.loadWidgetGeometry(m_pConnectionsForm);
    }
    
    if (m_pConnectionsForm) {
        m_pConnectionsForm->setJackClient(m_pJackClient);
        if (m_pConnectionsForm->isVisible())
            m_pConnectionsForm->hide();
        else
            m_pConnectionsForm->show();
    }
}


// Patchbay form requester slot.
void qjackctlMainForm::togglePatchbayForm (void)
{
    if (m_pPatchbayForm == NULL) {
        m_pPatchbayForm = new qjackctlPatchbayForm(this);
        m_setup.loadWidgetGeometry(m_pPatchbayForm);
        // Load patchbay from default path.
        if (m_pPatchbayForm && !m_setup.sPatchbayPath.isEmpty())
            m_pPatchbayForm->loadPatchbayFile(m_setup.sPatchbayPath);
    }
    
    if (m_pPatchbayForm) {
        m_pPatchbayForm->setJackClient(m_pJackClient);
        if (m_pPatchbayForm->isVisible())
            m_pPatchbayForm->hide();
        else
            m_pPatchbayForm->show();
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
    QString n = "--";
    QString t = "--:--:--.---";
    QString b = "--:--.--";
    QString sStopped = tr("Stopped");
    
    if (m_pJackClient) {
        QString s = " ";
        updateStatus(STATUS_CPU_LOAD, QString::number(jack_cpu_load(m_pJackClient), 'g', 3) + s + "%");
        updateStatus(STATUS_SAMPLE_RATE, QString::number(jack_get_sample_rate(m_pJackClient)) + s + tr("Hz"));
        updateBufferSize();
#ifdef CONFIG_JACK_TRANSPORT
        char    szText[32];
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
        // Transport timecode position (hh:mm:ss.ddd).
    //  if (tpos.valid & JackPositionTimecode) {
        if (bPlaying) {
            unsigned int hh, mm, ss, dd;
            double tt = (double) (tpos.frame / tpos.frame_rate);
            hh  = (unsigned int) (tt / 3600.0);
            tt -= (double) (hh * 3600.0);
            mm  = (unsigned int) (tt / 60.0);
            tt -= (double) (mm * 60.0);
            ss  = (unsigned int) tt;
            tt -= (double) ss;
            dd  = (unsigned int) (tt * 100.0);
            snprintf(szText, sizeof(szText), "%02u:%02u:%02u.%03u", hh, mm, ss, dd);
            updateStatus(STATUS_TRANSPORT_TIME, szText);
    //  } else {
    //      updateStatus(STATUS_TRANSPORT_TIME, t);
        }
        // Transport barcode position (bar:beat.tick)
        if (tpos.valid & JackPositionBBT) {
            snprintf(szText, sizeof(szText), "%u:%u.%u", tpos.bar, tpos.beat, tpos.tick);
            updateStatus(STATUS_TRANSPORT_BBT, szText);
            updateStatus(STATUS_TRANSPORT_BPM, QString::number(tpos.beats_per_minute));
        } else {
            updateStatus(STATUS_TRANSPORT_BBT, b);
            updateStatus(STATUS_TRANSPORT_BPM, n);
        }
        PlayPushButton->setEnabled(tstate == JackTransportStopped);
        PausePushButton->setEnabled(bPlaying);
#else   // !CONFIG_JACK_TRANSPORT
        updateStatus(STATUS_TRANSPORT_STATE, n);
        updateStatus(STATUS_TRANSPORT_TIME, t);
        updateStatus(STATUS_TRANSPORT_BBT, b);
        updateStatus(STATUS_TRANSPORT_BPM, n);
        PlayPushButton->setEnabled(false);
        PausePushButton->setEnabled(false);
#endif
    } else {
        updateStatus(STATUS_CPU_LOAD, n);
        updateStatus(STATUS_SAMPLE_RATE, n);
        updateStatus(STATUS_TRANSPORT_STATE, n);
        updateStatus(STATUS_TRANSPORT_TIME, t);
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
        QColor fgcolor = Qt::green;
        if (m_iXrunCount > 0 || m_iXrunCallbacks > 0)
            fgcolor = (m_iXrunCallbacks > 0 ? Qt::red : Qt::yellow);
        XrunCountTextLabel->setPaletteForegroundColor(fgcolor);
        XrunCountTextLabel->setText(sText);
        break;
    }
    case STATUS_TRANSPORT_STATE:
        TransportStateTextLabel->setText(sText);
        break;
    case STATUS_TRANSPORT_TIME:
        TransportTimeTextLabel->setText(sText);
        break;
    case STATUS_TRANSPORT_BBT:
        TransportBBTTextLabel->setText(sText);
        break;
    case STATUS_TRANSPORT_BPM:
        TransportBPMTextLabel->setText(sText);
        break;
    }

    if (m_pStatusForm)
        m_pStatusForm->updateStatus(iStatusItem, sText);
}


// end of qjackctlMainForm.ui.h
