// qjackctlMainForm.cpp
//
/****************************************************************************
   Copyright (C) 2003-2015, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include "qjackctlMainForm.h"

#include "qjackctlStatus.h"

#include "qjackctlPatchbay.h"
#include "qjackctlPatchbayFile.h"

#include "qjackctlMessagesStatusForm.h"
#include "qjackctlSessionForm.h"
#include "qjackctlConnectionsForm.h"
#include "qjackctlPatchbayForm.h"
#include "qjackctlSetupForm.h"
#include "qjackctlAboutForm.h"
#include "qjackctlSystemTray.h"

#include <QApplication>
#include <QSocketNotifier>
#include <QMessageBox>
#include <QTextStream>
#include <QRegExp>
#include <QMenu>
#include <QTimer>
#include <QLabel>
#include <QPixmap>
#include <QFileInfo>
#include <QDir>

#include <QContextMenuEvent>
#include <QCloseEvent>

#if QT_VERSION < 0x040500
namespace Qt {
const WindowFlags WindowCloseButtonHint = WindowFlags(0x08000000);
}
#endif


#ifdef CONFIG_DBUS
#include <QDBusInterface>
#include <QThread>
#endif

#ifdef CONFIG_JACK_STATISTICS
#include <jack/statistics.h>
#endif

#ifdef CONFIG_JACK_METADATA
#include <jack/metadata.h>
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

#if defined(WIN32)
#include <io.h>
#undef HAVE_POLL_H
#undef HAVE_SIGNAL_H
#else
#include <unistd.h>
#include <fcntl.h>
// Notification pipes descriptors
#define QJACKCTL_FDNIL         -1
#define QJACKCTL_FDREAD         0
#define QJACKCTL_FDWRITE        1
static int g_fdStdout[2] = { QJACKCTL_FDNIL, QJACKCTL_FDNIL };
#endif

#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif


// Custom event types.
#define QJACKCTL_PORT_EVENT     QEvent::Type(QEvent::User + 1)
#define QJACKCTL_XRUN_EVENT     QEvent::Type(QEvent::User + 2)
#define QJACKCTL_BUFF_EVENT     QEvent::Type(QEvent::User + 3)
#define QJACKCTL_SHUT_EVENT     QEvent::Type(QEvent::User + 4)
#define QJACKCTL_EXIT_EVENT     QEvent::Type(QEvent::User + 5)

#ifdef CONFIG_DBUS
#define QJACKCTL_LINE_EVENT     QEvent::Type(QEvent::User + 6)
#endif

#ifdef CONFIG_JACK_METADATA
#define QJACKCTL_PROP_EVENT     QEvent::Type(QEvent::User + 7)
#endif


//----------------------------------------------------------------------------
// qjackctl -- Static callback posters.

// To have clue about current buffer size (in frames).
static jack_nframes_t g_nframes = 0;

static QProcess::ProcessError g_error = QProcess::UnknownError;

// Jack port registration callback funtion, called
// whenever a jack port is registered or unregistered.
static void qjackctl_port_registration_callback ( jack_port_id_t, int, void * )
{
	QApplication::postEvent(
		qjackctlMainForm::getInstance(),
		new QEvent(QJACKCTL_PORT_EVENT));
}


// Jack graph order callback function, called
// whenever the processing graph is reordered.
static int qjackctl_graph_order_callback ( void * )
{
	QApplication::postEvent(
		qjackctlMainForm::getInstance(),
		new QEvent(QJACKCTL_PORT_EVENT));

	return 0;
}


// Jack XRUN callback function, called
// whenever there is a xrun.
static int qjackctl_xrun_callback ( void * )
{
	QApplication::postEvent(
		qjackctlMainForm::getInstance(),
		new QEvent(QJACKCTL_XRUN_EVENT));

	return 0;
}

// Jack buffer size function, called
// whenever the server changes buffer size.
static int qjackctl_buffer_size_callback ( jack_nframes_t nframes, void * )
{
	// Update our global static variable.
	g_nframes = nframes;

	QApplication::postEvent(
		qjackctlMainForm::getInstance(),
		new QEvent(QJACKCTL_BUFF_EVENT));

	return 0;
}


// Jack shutdown function, called
// whenever the server terminates this client.
static void qjackctl_on_shutdown ( void * )
{
	QApplication::postEvent(
		qjackctlMainForm::getInstance(),
		new QEvent(QJACKCTL_SHUT_EVENT));
}


// Jack process exit function, called
// whenever the server terminates abnormally.
static void qjackctl_on_error ( QProcess::ProcessError error )
{
	g_error = error;

	QApplication::postEvent(
		qjackctlMainForm::getInstance(),
		new QEvent(QJACKCTL_EXIT_EVENT));
}


#ifdef CONFIG_DBUS

//----------------------------------------------------------------------
// class qjackctlDBusLogWatcher -- Simple D-BUS log watcher thread.
//

class qjackctlDBusLogWatcher : public QThread
{
public:

	// Constructor.
	qjackctlDBusLogWatcher(const QString& sFilename) : QThread(),
		m_sFilename(sFilename), m_bRunState(false) {}

	// Destructor.
	~qjackctlDBusLogWatcher()
		{ if (isRunning()) do { m_bRunState = false; } while (!wait(1000)); }

	// Custom log event.
	class LineEvent : public QEvent
	{
	public:
		// Constructor.
		LineEvent(QEvent::Type eType, const QString& sLine)
			: QEvent(eType), m_sLine(sLine)
			{ m_sLine.remove(QRegExp("\x1B\[[0-9|;]+m")); }
		// Accessor.
		const QString& line() const
			{ return m_sLine; }
	private:
		// Custom event data.
		QString m_sLine;
	};

protected:

	// The main thread executive.
	void run()
	{
		QFile file(m_sFilename);

		m_bRunState = true;

		while (m_bRunState) {
			if (file.isOpen()) {
				char achBuffer[1024];
				while (file.readLine(achBuffer, sizeof(achBuffer)) > 0) {
					QApplication::postEvent(
						qjackctlMainForm::getInstance(),
						new LineEvent(QJACKCTL_LINE_EVENT, achBuffer));
				}
				if (file.size()  == file.pos() &&
					file.error() == QFile::NoError) {
					msleep(1000);
				} else {
					file.close();
				}
			}
			else
			if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
				file.seek(file.size());
			}
			else msleep(1000);
		}
	}

private:

	// The log filename to watch.
	QString m_sFilename;

	// Whether the thread is logically running.
	volatile bool m_bRunState;
};

#endif	// CONFIG_DBUS


#ifdef CONFIG_JACK_METADATA
// Jack property change function, called
// whenever metadata is changed
static void qjackctl_property_change_callback (
	jack_uuid_t, const char *key, jack_property_change_t, void * )
{
	// PRETTY_NAME is the only metadata we are currently interested in...
	if (qjackctlJackClientList::isJackClientPortMetadata() &&
		key && (strcmp(key, JACK_METADATA_PRETTY_NAME) == 0)) {
		QApplication::postEvent(
			qjackctlMainForm::getInstance(),
			new QEvent(QJACKCTL_PROP_EVENT));
	}
}
#endif


//----------------------------------------------------------------------------
// qjackctlMainForm -- UI wrapper form.

// Kind of singleton reference.
qjackctlMainForm *qjackctlMainForm::g_pMainForm = NULL;

// Constructor.
qjackctlMainForm::qjackctlMainForm (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QWidget(pParent, wflags)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	// Pseudo-singleton reference setup.
	g_pMainForm = this;

	m_pSetup = NULL;

	m_iServerState = QJACKCTL_INACTIVE;

	m_pJack         = NULL;
	m_pJackClient   = NULL;
	m_bJackDetach   = false;
	m_bJackShutdown = false;
	m_pAlsaSeq      = NULL;
#ifdef CONFIG_DBUS
	m_pDBusControl  = NULL;
	m_pDBusConfig   = NULL;
	m_pDBusLogWatcher = NULL;
	m_bDBusStarted  = false;
#endif
	m_iStartDelay   = 0;
	m_iTimerDelay   = 0;
	m_iTimerRefresh = 0;
	m_iJackRefresh  = 0;
	m_iAlsaRefresh  = 0;
	m_iJackDirty    = 0;
	m_iAlsaDirty    = 0;

	m_iStatusRefresh = 0;
	m_iStatusBlink   = 0;

	m_iPatchbayRefresh = 0;

#ifdef CONFIG_JACK_METADATA
	m_iJackPropertyChange = 0;
#endif

	m_pStdoutNotifier = NULL;
	m_pAlsaNotifier = NULL;

	// All forms are to be created later on setup.
	m_pMessagesStatusForm = NULL;
	m_pSessionForm     = NULL;
	m_pConnectionsForm = NULL;
	m_pPatchbayForm    = NULL;
	m_pSetupForm       = NULL;

	// Patchbay rack can be readily created.
	m_pPatchbayRack = new qjackctlPatchbayRack();

	// The eventual system tray widget.
	m_pSystemTray  = NULL;

	// We're not quitting so early :)
	m_bQuitForce = false;

	// Transport skip accelerate factor.
	m_fSkipAccel = 1.0;

	// Avoid extra transport toggles (play/stop)
	m_iTransportPlay = 0;

	// Whether we've Qt::Tool flag (from bKeepOnTop),
	// this is actually the main last application window...
	QWidget::setAttribute(Qt::WA_QuitOnClose);

#ifdef HAVE_SIGNAL_H
	// Set to ignore any fatal "Broken pipe" signals.
	signal(SIGPIPE, SIG_IGN);
#endif

#if 0
	// FIXME: Iterate for every child text label...
	m_ui.StatusDisplayFrame->setAutoFillBackground(true);
	QList<QLabel *> labels = m_ui.StatusDisplayFrame->findChildren<QLabel *> ();
	QListIterator<QLabel *> iter(labels);
	while (iter.hasNext())
		iter.next()->setAutoFillBackground(false);
#endif

	// UI connections...

	QObject::connect(m_ui.StartToolButton,
		SIGNAL(clicked()),
		SLOT(startJack()));
	QObject::connect(m_ui.StopToolButton,
		SIGNAL(clicked()),
		SLOT(stopJack()));
	QObject::connect(m_ui.MessagesStatusToolButton,
		SIGNAL(clicked()),
		SLOT(toggleMessagesStatusForm()));
	QObject::connect(m_ui.SessionToolButton,
		SIGNAL(clicked()),
		SLOT(toggleSessionForm()));
	QObject::connect(m_ui.ConnectionsToolButton,
		SIGNAL(clicked()),
		SLOT(toggleConnectionsForm()));
	QObject::connect(m_ui.PatchbayToolButton,
		SIGNAL(clicked()),
		SLOT(togglePatchbayForm()));

	QObject::connect(m_ui.QuitToolButton,
		SIGNAL(clicked()),
		SLOT(quitMainForm()));
	QObject::connect(m_ui.SetupToolButton,
		SIGNAL(clicked()),
		SLOT(showSetupForm()));
	QObject::connect(m_ui.AboutToolButton,
		SIGNAL(clicked()),
		SLOT(showAboutForm()));

	QObject::connect(m_ui.RewindToolButton,
		SIGNAL(clicked()),
		SLOT(transportRewind()));
	QObject::connect(m_ui.BackwardToolButton,
		SIGNAL(clicked()),
		SLOT(transportBackward()));
	QObject::connect(m_ui.PlayToolButton,
		SIGNAL(toggled(bool)),
		SLOT(transportPlay(bool)));
	QObject::connect(m_ui.PauseToolButton,
		SIGNAL(clicked()),
		SLOT(transportStop()));
	QObject::connect(m_ui.ForwardToolButton,
		SIGNAL(clicked()),
		SLOT(transportForward()));
}


// Destructor.
qjackctlMainForm::~qjackctlMainForm (void)
{
	// Stop server, if not already...

#ifdef CONFIG_DBUS
	if (m_pSetup->bStopJack || !m_pSetup->bDBusEnabled)
		stopJackServer();
	if (m_pDBusLogWatcher)
		delete m_pDBusLogWatcher;
	if (m_pDBusConfig)
		delete m_pDBusConfig;
	if (m_pDBusControl)
		delete m_pDBusControl;
	m_pDBusControl = NULL;
	m_pDBusConfig  = NULL;
	m_pDBusLogWatcher = NULL;
	m_bDBusStarted = false;
#else
	stopJackServer();
#endif

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
	if (m_pMessagesStatusForm)
		delete m_pMessagesStatusForm;
	if (m_pSessionForm)
		delete m_pSessionForm;
	if (m_pConnectionsForm)
		delete m_pConnectionsForm;
	if (m_pPatchbayForm)
		delete m_pPatchbayForm;
	if (m_pSetupForm)
		delete m_pSetupForm;

	// Quit off system tray widget.
	if (m_pSystemTray)
		delete m_pSystemTray;

	// Patchbay rack is also dead.
	if (m_pPatchbayRack)
		delete m_pPatchbayRack;

	// Pseudo-singleton reference shut-down.
	g_pMainForm = NULL;
}



// Kind of singleton reference.
qjackctlMainForm *qjackctlMainForm::getInstance (void)
{
	return g_pMainForm;
}


// Make and set a proper setup step.
bool qjackctlMainForm::setup ( qjackctlSetup *pSetup )
{
	// Finally, fix settings descriptor
	// and stabilize the form.
	m_pSetup = pSetup;

	// To avoid any background flickering,
	// we'll hide the main display.
	m_ui.StatusDisplayFrame->hide();
	updateButtons();

	// What style do we create these forms?
	QWidget *pParent = NULL;
	Qt::WindowFlags wflags = Qt::Window
		| Qt::CustomizeWindowHint
		| Qt::WindowTitleHint
		| Qt::WindowSystemMenuHint
		| Qt::WindowMinMaxButtonsHint
		| Qt::WindowCloseButtonHint;
	if (m_pSetup->bKeepOnTop) {
		pParent = this;
		wflags |= Qt::Tool;
	}
	// All forms are to be created right now.
	m_pMessagesStatusForm = new qjackctlMessagesStatusForm (pParent, wflags);
	m_pSessionForm        = new qjackctlSessionForm        (pParent, wflags);
	m_pConnectionsForm    = new qjackctlConnectionsForm    (pParent, wflags);
	m_pPatchbayForm       = new qjackctlPatchbayForm       (pParent, wflags);

	// Setup form is kind of special (modeless dialog).
	m_pSetupForm = new qjackctlSetupForm(this);

	// Setup appropriately...
	m_pMessagesStatusForm->setTabPage(m_pSetup->iMessagesStatusTabPage);
	m_pMessagesStatusForm->setLogging(
		m_pSetup->bMessagesLog, m_pSetup->sMessagesLogPath);
	m_pSessionForm->setup(m_pSetup);
	m_pConnectionsForm->setTabPage(m_pSetup->iConnectionsTabPage);
	m_pConnectionsForm->setup(m_pSetup);
	m_pPatchbayForm->setup(m_pSetup);
	m_pSetupForm->setup(m_pSetup);

	// Check out some initial nullities(tm)...
	if (m_pSetup->sMessagesFont.isEmpty() && m_pMessagesStatusForm)
		m_pSetup->sMessagesFont = m_pMessagesStatusForm->messagesFont().toString();
	if (m_pSetup->sDisplayFont1.isEmpty())
		m_pSetup->sDisplayFont1 = m_ui.TimeDisplayTextLabel->font().toString();
	if (m_pSetup->sDisplayFont2.isEmpty())
		m_pSetup->sDisplayFont2 = m_ui.ServerStateTextLabel->font().toString();
	if (m_pSetup->sConnectionsFont.isEmpty() && m_pConnectionsForm)
		m_pSetup->sConnectionsFont = m_pConnectionsForm->connectionsFont().toString();

	// Set the patchbay cable connection notification signal/slot.
	QObject::connect(m_pPatchbayRack, SIGNAL(cableConnected(const QString&, const QString&, unsigned int)),
		this, SLOT(cableConnectSlot(const QString&, const QString&, unsigned int)));

	// Try to restore old window positioning and appearence.
	m_pSetup->loadWidgetGeometry(this, true);

	// Make it final show...
	m_ui.StatusDisplayFrame->show();

	// Set other defaults...
	updateDisplayEffect();
	updateTimeDisplayFonts();
	updateTimeDisplayToolTips();
	updateTimeFormat();
	updateMessagesFont();
	updateMessagesLimit();
	updateConnectionsFont();
	updateConnectionsIconSize();
	updateJackClientPortAlias();
	updateJackClientPortMetadata();
	updateBezierLines();
//	updateActivePatchbay();
	updateSystemTray();

	// And for the whole widget gallore...
	m_pSetup->loadWidgetGeometry(m_pMessagesStatusForm);
	m_pSetup->loadWidgetGeometry(m_pSessionForm);
	m_pSetup->loadWidgetGeometry(m_pConnectionsForm);
	m_pSetup->loadWidgetGeometry(m_pPatchbayForm);
//	m_pSetup->loadWidgetGeometry(m_pSetupForm);

	// Initial XRUN statistics reset.
	resetXrunStats();

	// Check if we can redirect our own stdout/stderr...
#if !defined(WIN32)
	if (m_pSetup->bStdoutCapture && ::pipe(g_fdStdout) == 0) {
		::dup2(g_fdStdout[QJACKCTL_FDWRITE], STDOUT_FILENO);
		::dup2(g_fdStdout[QJACKCTL_FDWRITE], STDERR_FILENO);
		m_pStdoutNotifier = new QSocketNotifier(g_fdStdout[QJACKCTL_FDREAD], QSocketNotifier::Read, this);
		QObject::connect(m_pStdoutNotifier, SIGNAL(activated(int)), this, SLOT(stdoutNotifySlot(int)));
	}
#endif
#ifdef CONFIG_ALSA_SEQ
	if (m_pSetup->bAlsaSeqEnabled) {
		// Start our ALSA sequencer interface.
		if (snd_seq_open(&m_pAlsaSeq, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0)
			m_pAlsaSeq = NULL;
		if (m_pAlsaSeq) {
			snd_seq_port_subscribe_t *pAlsaSubs;
			snd_seq_addr_t seq_addr;
			struct pollfd pfd[1];
			int iPort = snd_seq_create_simple_port(
				m_pAlsaSeq,	"qjackctl",
				SND_SEQ_PORT_CAP_WRITE
				| SND_SEQ_PORT_CAP_SUBS_WRITE
				| SND_SEQ_PORT_CAP_NO_EXPORT,
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
				m_pAlsaNotifier
					= new QSocketNotifier(pfd[0].fd, QSocketNotifier::Read);
				QObject::connect(m_pAlsaNotifier,
					SIGNAL(activated(int)),
					SLOT(alsaNotifySlot(int)));
			}
		}
		// Could we start without it?
		if (m_pAlsaSeq) {
			// Rather obvious setup.
			if (m_pConnectionsForm)
				m_pConnectionsForm->stabilizeAlsa(true);
		} else {
			appendMessagesError(
				tr("Could not open ALSA sequencer as a client.\n\n"
				"ALSA MIDI patchbay will be not available."));
		}
	}
#endif
#ifdef CONFIG_DBUS
	// Register D-Bus service...
	if (m_pSetup->bDBusEnabled) {
		const QString s; // Just an empty string.
		const QString sDBusName("org.rncbc.qjackctl");
		QDBusConnection dbus = QDBusConnection::systemBus();
		dbus.connect(s, s, sDBusName, "start",
			this, SLOT(startJack()));
		dbus.connect(s, s, sDBusName, "stop",
			this, SLOT(stopJack()));
		dbus.connect(s, s, sDBusName, "main",
			this, SLOT(toggleMainForm()));
		dbus.connect(s, s, sDBusName, "messages",
			this, SLOT(toggleMessagesForm()));
		dbus.connect(s, s, sDBusName, "status",
			this, SLOT(toggleStatusForm()));
		dbus.connect(s, s, sDBusName, "session",
			this, SLOT(toggleSessionForm()));
		dbus.connect(s, s, sDBusName, "connections",
			this, SLOT(toggleConnectionsForm()));
		dbus.connect(s, s, sDBusName, "patchbay",
			this, SLOT(togglePatchbayForm()));
		dbus.connect(s, s, sDBusName, "rewind",
			this, SLOT(transportRewind()));
		dbus.connect(s, s, sDBusName, "backward",
			this, SLOT(transportBackward()));
		dbus.connect(s, s, sDBusName, "play",
			this, SLOT(transportStart()));
		dbus.connect(s, s, sDBusName, "pause",
			this, SLOT(transportStop()));
		dbus.connect(s, s, sDBusName, "forward",
			this, SLOT(transportForward()));
		dbus.connect(s, s, sDBusName, "setup",
			this, SLOT(showSetupForm()));
		dbus.connect(s, s, sDBusName, "about",
			this, SLOT(showAboutForm()));
		dbus.connect(s, s, sDBusName, "quit",
			this, SLOT(quitMainForm()));
		dbus.connect(s, s, sDBusName, "preset",
			this, SLOT(activatePreset(const QString&)));
		// Session related slots...
		if (m_pSessionForm) {
			dbus.connect(s, s, sDBusName, "load",
				m_pSessionForm, SLOT(loadSession()));
			dbus.connect(s, s, sDBusName, "save",
				m_pSessionForm, SLOT(saveSessionSave()));
		#ifdef CONFIG_JACK_SESSION
			dbus.connect(s, s, sDBusName, "savequit",
				m_pSessionForm, SLOT(saveSessionSaveAndQuit()));
			dbus.connect(s, s, sDBusName, "savetemplate",
				m_pSessionForm, SLOT(saveSessionSaveTemplate()));
		#endif
		}
		// Detect whether jackdbus is avaliable...
		QDBusConnection dbusc = QDBusConnection::sessionBus();
		m_pDBusControl = new QDBusInterface(
			"org.jackaudio.service",		// Service
			"/org/jackaudio/Controller",	// Path
			"org.jackaudio.JackControl",	// Interface
			dbusc);							// Connection
		QDBusMessage dbusm = m_pDBusControl->call("IsStarted");
		if (dbusm.type() == QDBusMessage::ReplyMessage) {
			// Yes, jackdbus is available and/or already started
			// -- use jackdbus control interface...
			appendMessages(tr("D-BUS: Service is available (%1 aka jackdbus).")
				.arg(m_pDBusControl->service()));
			// Parse reply (should be boolean)
			m_bDBusStarted = dbusm.arguments().first().toBool();
			// Register server start/stop notification slots...
			dbusc.connect(
				m_pDBusControl->service(),
				m_pDBusControl->path(),
				m_pDBusControl->interface(),
				"ServerStarted", this,
				SLOT(jackStarted()));
			dbusc.connect(
				m_pDBusControl->service(),
				m_pDBusControl->path(),
				m_pDBusControl->interface(),
				"ServerStopped", this,
				SLOT(jackFinished()));
			// -- use jackdbus configure interface...
			m_pDBusConfig = new QDBusInterface(
				m_pDBusControl->service(),	// Service
				m_pDBusControl->path(),		// Path
				"org.jackaudio.Configure",	// Interface
				m_pDBusControl->connection());	// Connection
			// Start our log watcher thread...
			m_pDBusLogWatcher = new qjackctlDBusLogWatcher(
				QDir::homePath() + "/.log/jack/jackdbus.log");
			m_pDBusLogWatcher->start();
			// Ready now.
		} else {
			// No, jackdbus is not available, not started
			// or not even installed -- use classic jackd, BAU...
			appendMessages(tr("D-BUS: Service not available (%1 aka jackdbus).")
				.arg(m_pDBusControl->service()));
			// Destroy tentative jackdbus interface.
			delete m_pDBusControl;
			m_pDBusControl = NULL;
		}
	}
#endif

	// Load patchbay form recent paths...
	if (m_pPatchbayForm) {
		m_pPatchbayForm->setRecentPatchbays(m_pSetup->patchbays);
		if (!m_pSetup->sPatchbayPath.isEmpty()
			&& QFileInfo(m_pSetup->sPatchbayPath).exists())
			m_pPatchbayForm->loadPatchbayFile(m_pSetup->sPatchbayPath);
		m_pPatchbayForm->updateRecentPatchbays();
		m_pPatchbayForm->stabilizeForm();
	}

	// Try to find if we can start in detached mode (client-only)
	// just in case there's a JACK server already running.
	startJackClient(true);
	// Final startup stabilization...
	stabilizeForm();
	jackStabilize();

	// Look for immediate server startup?...
	if (m_pSetup->bStartJack || !m_pSetup->sCmdLine.isEmpty())
		m_pSetup->bStartJackCmd = true;
	if (m_pSetup->bStartJackCmd)
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
		&& m_pSetup->bSystemTray && m_pSystemTray) {
		m_pSetup->saveWidgetGeometry(this, true);
		if (m_pSetup->bSystemTrayQueryClose) {
			const QString& sTitle
				= tr("Information") + " - " QJACKCTL_SUBTITLE1;
			const QString& sText
				= tr("The program will keep running in the system tray.\n\n"
					"To terminate the program, please choose \"Quit\"\n"
					"in the context menu of the system tray icon.");
		#if 0//QJACKCTL_SYSTEM_TRAY_QUERY_CLOSE
			if (QSystemTrayIcon::supportsMessages()) {
				m_pSystemTray->showMessage(
					sTitle, sText, QSystemTrayIcon::Information);
			}
			else
			QMessageBox::information(this, sTitle, sText);
		#else
			QMessageBox mbox(this);
			mbox.setIcon(QMessageBox::Information);
			mbox.setWindowTitle(sTitle);
			mbox.setText(sText);
			mbox.setStandardButtons(QMessageBox::Ok);
			QCheckBox cbox(tr("Don't show this message again"));
			cbox.setChecked(false);
			cbox.blockSignals(true);
			mbox.addButton(&cbox, QMessageBox::ActionRole);
			mbox.exec();
			if (cbox.isChecked())
				m_pSetup->bSystemTrayQueryClose = false;
		#endif
		}
		hide();
		bQueryClose = false;
	}
#endif

	// Check if JACK daemon is currently running...
	if (bQueryClose && m_pJack && m_pJack->state() == QProcess::Running
		&& (m_pSetup->bQueryClose || m_pSetup->bQueryShutdown)) {
		show();
		raise();
		activateWindow();
		if (m_pSetup->bQueryClose) {
			const QString& sTitle
				= tr("Warning") + " - " QJACKCTL_SUBTITLE1;
			const QString& sText
				= tr("JACK is currently running.\n\n"
					"Do you want to terminate the JACK audio server?");
		#if 0//QJACKCTL_QUERY_CLOSE
			bQueryClose = (QMessageBox::warning(this, sTitle, sText,
				QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok);
		#else
			QMessageBox mbox(this);
			mbox.setIcon(QMessageBox::Warning);
			mbox.setWindowTitle(sTitle);
			mbox.setText(sText);
			mbox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
			QCheckBox cbox(tr("Don't ask this again"));
			cbox.setChecked(false);
			cbox.blockSignals(true);
			mbox.addButton(&cbox, QMessageBox::ActionRole);
			bQueryClose = (mbox.exec() == QMessageBox::Ok);
			if (bQueryClose && cbox.isChecked())
				m_pSetup->bQueryClose = false;
		#endif
		}
	}

	// Try to save current setup settings.
	if (bQueryClose && m_pSetupForm)
		bQueryClose = m_pSetupForm->queryClose();

	// Try to save current aliases default settings.
	if (bQueryClose && m_pConnectionsForm) {
		bQueryClose = m_pConnectionsForm->queryClose();
		if (bQueryClose)
			m_pSetup->iConnectionsTabPage = m_pConnectionsForm->tabPage();
	}

	// Try to save current patchbay default settings.
	if (bQueryClose && m_pPatchbayForm) {
		bQueryClose = m_pPatchbayForm->queryClose();
		if (bQueryClose && !m_pPatchbayForm->patchbayPath().isEmpty())
			m_pSetup->sPatchbayPath = m_pPatchbayForm->patchbayPath();
	}

	// Try to save current session directories list...
	if (bQueryClose && m_pSessionForm) {
		bQueryClose = m_pSessionForm->queryClose();
		if (bQueryClose) {
			m_pSetup->sessionDirs = m_pSessionForm->sessionDirs();
			m_pSetup->bSessionSaveVersion = m_pSessionForm->isSaveSessionVersion();
		}
	}

	// Some windows default fonts are here on demand too.
	if (bQueryClose && m_pMessagesStatusForm) {
		m_pSetup->sMessagesFont = m_pMessagesStatusForm->messagesFont().toString();
		m_pSetup->iMessagesStatusTabPage = m_pMessagesStatusForm->tabPage();
	}

	// Whether we're really quitting.
	m_bQuitForce = bQueryClose;

	// Try to save current positioning.
	if (bQueryClose) {
		m_pSetup->saveWidgetGeometry(m_pMessagesStatusForm);
		m_pSetup->saveWidgetGeometry(m_pSessionForm);
		m_pSetup->saveWidgetGeometry(m_pConnectionsForm);
		m_pSetup->saveWidgetGeometry(m_pPatchbayForm);
	//	m_pSetup->saveWidgetGeometry(m_pSetupForm);
		m_pSetup->saveWidgetGeometry(this);
		// Close popup widgets.
		if (m_pMessagesStatusForm)
			m_pMessagesStatusForm->close();
		if (m_pSessionForm)
			m_pSessionForm->close();
		if (m_pConnectionsForm)
			m_pConnectionsForm->close();
		if (m_pPatchbayForm)
			m_pPatchbayForm->close();
		if (m_pSetupForm)
			m_pSetupForm->close();
		// And the system tray icon too.
		if (m_pSystemTray)
			m_pSystemTray->close();
		// Stop any service out there...
		if (m_pSetup->bStopJack)
			stopJackServer();
		// Finally, save settings.
		m_pSetup->saveSetup();
	}

	return bQueryClose;
}


void qjackctlMainForm::closeEvent ( QCloseEvent *pCloseEvent )
{
	// Let's be sure about that...
	if (queryClose()) {
		pCloseEvent->accept();
		QApplication::quit();
	} else {
		pCloseEvent->ignore();
	}
}


void qjackctlMainForm::customEvent ( QEvent *pEvent )
{
	switch (int(pEvent->type())) {
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
	case QJACKCTL_EXIT_EVENT:
		exitNotifyEvent();
		break;
#ifdef CONFIG_DBUS
	case QJACKCTL_LINE_EVENT:
		appendStdoutBuffer(
			static_cast<qjackctlDBusLogWatcher::LineEvent *> (pEvent)->line());
		break;
#endif
#ifdef CONFIG_JACK_METADATA
	case QJACKCTL_PROP_EVENT:
		propNotifyEvent();
		break;
#endif
	default:
		QWidget::customEvent(pEvent);
		break;
	}
}


// Common exit status text formatter...
QString qjackctlMainForm::formatExitStatus ( int iExitStatus ) const
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
	sTemp.replace("%N", m_pSetup->sServerName);
	sTemp.replace("%s", m_preset.sServerPrefix);
	sTemp.replace("%d", m_preset.sDriver);
	sTemp.replace("%i", m_preset.sInterface);
	sTemp.replace("%r", QString::number(m_preset.iSampleRate));
	sTemp.replace("%p", QString::number(m_preset.iFrames));
	sTemp.replace("%n", QString::number(m_preset.iPeriods));

	appendMessages(sStartMessage);
	appendMessagesColor(sTemp.trimmed(), "#990099");
	stabilize(QJACKCTL_TIMER_MSECS);

	// Execute and set exit status message...
	sTemp = sStopMessage + formatExitStatus(
		::system(sTemp.toUtf8().constData()));

	// Wait a litle bit before continue...
	stabilize(QJACKCTL_TIMER_MSECS);
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
		if (QMessageBox::warning(this,
			tr("Warning") + " - " QJACKCTL_SUBTITLE1,
			tr("Could not start JACK.\n\n"
			"Maybe JACK audio server is already started."),
			QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok) {
			m_pJack->terminate();
			m_pJack->kill();
		}
		return;
	}

	// Stabilize emerging server state...
	QPalette pal;
	pal.setColor(QPalette::Foreground, Qt::yellow);
	m_ui.ServerStateTextLabel->setPalette(pal);
	m_ui.StartToolButton->setEnabled(false);
	updateServerState(QJACKCTL_ACTIVATING);

	// Reset our timer counters...
	m_iStartDelay  = 0;
	m_iTimerDelay  = 0;
	m_iJackRefresh = 0;

	// Now we're sure it ain't detached.
	m_bJackShutdown = false;
	m_bJackDetach = false;

	// Load primary/default server preset...
	if (!m_pSetup->loadPreset(m_preset, m_pSetup->sDefPreset)) {
		appendMessagesError(tr("Could not load preset \"%1\".\n\nRetrying with default.").arg(m_pSetup->sDefPreset));
		m_pSetup->sDefPreset = m_pSetup->sDefPresetName;
		if (!m_pSetup->loadPreset(m_preset, m_pSetup->sDefPreset)) {
			appendMessagesError(tr("Could not load default preset.\n\nSorry."));
			jackCleanup();
			return;
		}
	}

	// Override server name now...
	if (m_pSetup->sServerName.isEmpty())
		m_pSetup->sServerName = m_preset.sServerName;

#if 0 // defined(__GNUC__) && defined(Q_OS_LINUX)
	// Take care for the environment as well...
	if (!m_pSetup->sServerName.isEmpty()) {
		setenv("JACK_DEFAULT_SERVER",
			m_pSetup->sServerName.toUtf8().constData(), 1);
	}
#endif

	// If we ain't to be the server master, maybe we'll start
	// detached as client only (jackd server already running?)
	if (startJackClient(true)) {
		m_ui.StopToolButton->setEnabled(true);
		return;
	}

	// Say that we're starting...
	updateServerState(QJACKCTL_STARTING);

	// Do we have any startup script?...
	if (m_pSetup->bStartupScript
		&& !m_pSetup->sStartupScriptShell.isEmpty()) {
		shellExecute(m_pSetup->sStartupScriptShell,
			tr("Startup script..."),
			tr("Startup script terminated"));
	}

	// Split the server path into arguments...
	QStringList args = m_preset.sServerPrefix.split(' ');

	// Look for the executable in the search path;
	// this enforces the server command to be an
	// executable absolute path whenever possible.
	QString sCommand = args[0];
	QFileInfo fi(sCommand);
	if (fi.isRelative()) {
	#if defined(WIN32)
		const char chPathSep = ';';
		if (fi.suffix().isEmpty())
			sCommand += ".exe";
	#else
		const char chPathSep = ':';
	#endif
		const QString sPath = ::getenv("PATH");
		QStringList paths = sPath.split(chPathSep);
		QStringListIterator iter(paths);
		while (iter.hasNext()) {
			const QString& sDirectory = iter.next();
			fi.setFile(QDir(sDirectory), sCommand);
			if (fi.isExecutable()) {
				sCommand = fi.filePath();
				break;
			}
		}
	}
	// Now that we got a command, remove it from args list...
	args.removeAt(0);

	// Build process arguments...
	const bool bDummy     = (m_preset.sDriver == "dummy");
	const bool bSun       = (m_preset.sDriver == "sun");
	const bool bOss       = (m_preset.sDriver == "oss");
	const bool bAlsa      = (m_preset.sDriver == "alsa");
	const bool bPortaudio = (m_preset.sDriver == "portaudio");
	const bool bCoreaudio = (m_preset.sDriver == "coreaudio");
	const bool bFreebob   = (m_preset.sDriver == "freebob");
	const bool bFirewire  = (m_preset.sDriver == "firewire");
	const bool bNet       = (m_preset.sDriver == "net" || m_preset.sDriver == "netone");

	if (!m_pSetup->sServerName.isEmpty())
		args.append("-n" + m_pSetup->sServerName);
	if (m_preset.bVerbose)
		args.append("-v");
	if (m_preset.bRealtime) {
	//	args.append("-R");
		if (m_preset.iPriority > 5 && !bCoreaudio)
			args.append("-P" + QString::number(m_preset.iPriority));
	}
	else args.append("-r");
	if (m_preset.iPortMax > 0 && m_preset.iPortMax != 256)
		args.append("-p" + QString::number(m_preset.iPortMax));
	if (m_preset.iTimeout > 0 && m_preset.iTimeout != 500)
		args.append("-t" + QString::number(m_preset.iTimeout));
	if (m_preset.bNoMemLock)
		args.append("-m");
	else if (m_preset.bUnlockMem)
		args.append("-u");
	args.append("-d" + m_preset.sDriver);
	if ((bAlsa || bPortaudio) && (m_preset.iAudio != QJACKCTL_DUPLEX ||
		m_preset.sInDevice.isEmpty() || m_preset.sOutDevice.isEmpty())) {
		QString sInterface = m_preset.sInterface;
		if (bAlsa && sInterface.isEmpty())
			sInterface = "hw:0";
		if (!sInterface.isEmpty())
			args.append("-d" + formatQuoted(sInterface));
	}
	if (bPortaudio && m_preset.iChan > 0)
		args.append("-c" + QString::number(m_preset.iChan));
	if ((bCoreaudio || bFreebob || bFirewire) && !m_preset.sInterface.isEmpty())
		args.append("-d" + formatQuoted(m_preset.sInterface));
	if (m_preset.iSampleRate > 0 && !bNet)
		args.append("-r" + QString::number(m_preset.iSampleRate));
	if (m_preset.iFrames > 0 && !bNet)
		args.append("-p" + QString::number(m_preset.iFrames));
	if (bAlsa || bSun || bOss || bFreebob || bFirewire) {
		if (m_preset.iPeriods > 0)
			args.append("-n" + QString::number(m_preset.iPeriods));
	}
	if (bAlsa) {
		if (m_preset.bSoftMode)
			args.append("-s");
		if (m_preset.bMonitor)
			args.append("-m");
		if (m_preset.bShorts)
			args.append("-S");
		if (m_preset.bHWMon)
			args.append("-H");
		if (m_preset.bHWMeter)
			args.append("-M");
	#ifdef CONFIG_JACK_MIDI
		if (!m_preset.sMidiDriver.isEmpty())
			args.append("-X" + formatQuoted(m_preset.sMidiDriver));
	#endif
	}
	if (bAlsa || bPortaudio) {
		switch (m_preset.iAudio) {
		case QJACKCTL_DUPLEX:
			if (!m_preset.sInDevice.isEmpty() || !m_preset.sOutDevice.isEmpty())
				args.append("-D");
			if (!m_preset.sInDevice.isEmpty())
				args.append("-C" + formatQuoted(m_preset.sInDevice));
			if (!m_preset.sOutDevice.isEmpty())
				args.append("-P" + formatQuoted(m_preset.sOutDevice));
			break;
		case QJACKCTL_CAPTURE:
			args.append("-C" + formatQuoted(m_preset.sInDevice));
			break;
		case QJACKCTL_PLAYBACK:
			args.append("-P" + formatQuoted(m_preset.sOutDevice));
			break;
		}
		if (m_preset.iInChannels > 0  && m_preset.iAudio != QJACKCTL_PLAYBACK)
			args.append("-i" + QString::number(m_preset.iInChannels));
		if (m_preset.iOutChannels > 0 && m_preset.iAudio != QJACKCTL_CAPTURE)
			args.append("-o" + QString::number(m_preset.iOutChannels));
		switch (m_preset.iDither) {
		case 0:
		//	args.append("-z-");
			break;
		case 1:
			args.append("-zr");
			break;
		case 2:
			args.append("-zs");
			break;
		case 3:
			args.append("-zt");
			break;
		}
	}
	else if (bOss || bSun) {
		if (m_preset.bIgnoreHW)
			args.append("-b");
		if (m_preset.iWordLength > 0)
			args.append("-w" + QString::number(m_preset.iWordLength));
		if (!m_preset.sInDevice.isEmpty()  && m_preset.iAudio != QJACKCTL_PLAYBACK)
			args.append("-C" + formatQuoted(m_preset.sInDevice));
		if (!m_preset.sOutDevice.isEmpty() && m_preset.iAudio != QJACKCTL_CAPTURE)
			args.append("-P" + formatQuoted(m_preset.sOutDevice));
		if (m_preset.iAudio == QJACKCTL_PLAYBACK)
			args.append("-i0");
		else if (m_preset.iInChannels > 0)
			args.append("-i" + QString::number(m_preset.iInChannels));
		if (m_preset.iAudio == QJACKCTL_CAPTURE)
			args.append("-o0");
		else if (m_preset.iOutChannels > 0)
			args.append("-o" + QString::number(m_preset.iOutChannels));
	}
	else if (bCoreaudio || bFirewire || bNet) {
		if (m_preset.iInChannels > 0  && m_preset.iAudio != QJACKCTL_PLAYBACK)
			args.append("-i" + QString::number(m_preset.iInChannels));
		if (m_preset.iOutChannels > 0 && m_preset.iAudio != QJACKCTL_CAPTURE)
			args.append("-o" + QString::number(m_preset.iOutChannels));
	}
	else if (bFreebob) {
		switch (m_preset.iAudio) {
		case QJACKCTL_DUPLEX:
			args.append("-D");
			break;
		case QJACKCTL_CAPTURE:
			args.append("-C");
			args.append("-o0");
			break;
		case QJACKCTL_PLAYBACK:
			args.append("-P");
			args.append("-i0");
			break;
		}
	}
	if (bDummy && m_preset.iWait > 0 && m_preset.iWait != 21333)
		args.append("-w" + QString::number(m_preset.iWait));
	if (bAlsa || bSun || bOss || bCoreaudio || bPortaudio || bFreebob || bFirewire) {
		if (m_preset.iInLatency > 0)
			args.append("-I" + QString::number(m_preset.iInLatency));
		if (m_preset.iOutLatency > 0)
			args.append("-O" + QString::number(m_preset.iOutLatency));
	}

	// Split the server path into arguments...
	if (!m_preset.sServerSuffix.isEmpty())
		args.append(m_preset.sServerSuffix.split(' '));
	
	// This is emulated jackd command line, for future reference purposes...
	m_sJackCmdLine = sCommand + ' ' + args.join(" ").trimmed();

#ifdef CONFIG_DBUS

	// Jack D-BUS server backend startup method...
	if (m_pDBusControl) {

		// Jack D-BUS server backend configuration...
		setDBusParameters();

		QDBusMessage dbusm = m_pDBusControl->call("StartServer");
		if (dbusm.type() == QDBusMessage::ReplyMessage) {
			appendMessages(
				tr("D-BUS: JACK server is starting..."));
		} else {
			appendMessagesError(
				tr("D-BUS: JACK server could not be started.\n\nSorry"));
		}

		// Delay our control client...
		startJackClientDelay();

	} else {

#endif	// !CONFIG_DBUS

		// Jack classic server backend startup process...
		m_pJack = new QProcess(this);

		// Setup stdout/stderr capture...
		if (m_pSetup->bStdoutCapture) {
			m_pJack->setProcessChannelMode(QProcess::ForwardedChannels);
			QObject::connect(m_pJack,
				SIGNAL(readyReadStandardOutput()),
				SLOT(readStdout()));
			QObject::connect(m_pJack,
				SIGNAL(readyReadStandardError()),
				SLOT(readStdout()));
		}

		// The unforgiveable signal communication...
		QObject::connect(m_pJack,
			SIGNAL(started()),
			SLOT(jackStarted()));
		QObject::connect(m_pJack,
			SIGNAL(error(QProcess::ProcessError)),
			SLOT(jackError(QProcess::ProcessError)));
		QObject::connect(m_pJack,
			SIGNAL(finished(int, QProcess::ExitStatus)),
			SLOT(jackFinished()));

		appendMessages(tr("JACK is starting..."));
		appendMessagesColor(m_sJackCmdLine, "#990099");

	#if defined(WIN32)
		const QString& sCurrentDir = QFileInfo(sCommand).dir().absolutePath();
		m_pJack->setWorkingDirectory(sCurrentDir);
	//	QDir::setCurrent(sCurrentDir);
	#endif

		// Unquote arguments as necessary...
		const QChar q = '"';
		QStringList cmd_args;
		QStringListIterator iter(args);
		while (iter.hasNext()) {
			const QString& arg = iter.next();
			if (arg.contains(q)) {
				cmd_args.append(arg.section(q, 0, 0));
				cmd_args.append(arg.section(q, 1, 1));
			} else {
				cmd_args.append(arg);
			}
		}

		// Go jack, go...
		m_pJack->start(sCommand, cmd_args);

#ifdef CONFIG_DBUS
	}
#endif
}


// Stop jack audio server...
void qjackctlMainForm::stopJack (void)
{
	bool bQueryShutdown = true;

	// Check if we're allowed to stop (shutdown)...
	if (m_pSetup->bQueryShutdown && m_pConnectionsForm
		&& (m_pConnectionsForm->isAudioConnected() ||
			m_pConnectionsForm->isMidiConnected())) {
		const QString& sTitle
			= tr("Warning") + " - " QJACKCTL_SUBTITLE1;
		const QString& sText
			= tr("Some client audio applications\n"
				"are still active and connected.\n\n"
				"Do you want to stop the JACK audio server?");
		#if 0//QJACKCTL_QUERY_SHUTDOWN
			bQueryShutdown = (QMessageBox::warning(this, sTitle, sText,
				QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok);
		#else
			QMessageBox mbox(this);
			mbox.setIcon(QMessageBox::Warning);
			mbox.setWindowTitle(sTitle);
			mbox.setText(sText);
			QCheckBox cbox(tr("Don't ask this again"));
			cbox.setChecked(false);
			cbox.blockSignals(true);
			mbox.addButton(&cbox, QMessageBox::ActionRole);
			mbox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
			bQueryShutdown = (mbox.exec() == QMessageBox::Ok);
			if (bQueryShutdown && cbox.isChecked())
				m_pSetup->bQueryShutdown = false;
		#endif
	}

	// Stop the server unconditionally.
	if (bQueryShutdown)
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
#ifdef CONFIG_DBUS
	if ((m_pJack && m_pJack->state() == QProcess::Running)
		|| (m_pDBusControl && m_bDBusStarted)) {
#else
	if (m_pJack && m_pJack->state() == QProcess::Running) {
#endif
		updateServerState(QJACKCTL_STOPPING);
		// Do we have any pre-shutdown script?...
		if (m_pSetup->bShutdownScript
			&& !m_pSetup->sShutdownScriptShell.isEmpty()) {
			shellExecute(m_pSetup->sShutdownScriptShell,
				tr("Shutdown script..."),
				tr("Shutdown script terminated"));
		}
		// Now it's the time to real try stopping the server daemon...
		if (!m_bJackShutdown) {
			// Jack classic server backend...
			if (m_pJack) {
				appendMessages(tr("JACK is stopping..."));
			#if defined(WIN32)
				// Try harder...
				m_pJack->kill();
			#else
				// Try softly...
				m_pJack->terminate();
			#endif
			}
		#ifdef CONFIG_DBUS
			// Jack D-BUS server backend...
			if (m_pDBusControl) {
				QDBusMessage dbusm = m_pDBusControl->call("StopServer");
				if (dbusm.type() == QDBusMessage::ReplyMessage) {
					appendMessages(
						tr("D-BUS: JACK server is stopping..."));
				} else {
					appendMessagesError(
						tr("D-BUS: JACK server could not be stopped.\n\nSorry"));
				}
			}
		#endif
			// Give it some time to terminate gracefully and stabilize...
			stabilize(QJACKCTL_TIMER_MSECS);
			// Keep on, if not exiting for good.
			if (!m_bQuitForce)
				return;
		}
	}

	// Do final processing anyway.
	jackCleanup();
}


// Stdout handler...
void qjackctlMainForm::readStdout (void)
{
	appendStdoutBuffer(m_pJack->readAllStandardOutput());
}


// Stdout buffer handler -- now splitted by complete new-lines...
void qjackctlMainForm::appendStdoutBuffer ( const QString& s )
{
	m_sStdoutBuffer.append(s);

	const int iLength = m_sStdoutBuffer.lastIndexOf('\n');
	if (iLength > 0) {
		QString sTemp = m_sStdoutBuffer.left(iLength);
		m_sStdoutBuffer.remove(0, iLength + 1);
		QStringList list = sTemp.split('\n');
		QStringListIterator iter(list);
		while (iter.hasNext()) {
			sTemp = iter.next();
			if (!sTemp.isEmpty())
				appendMessagesText(detectXrun(sTemp));
		}
	}
}


// Stdout flusher -- show up any unfinished line...
void qjackctlMainForm::flushStdoutBuffer (void)
{
	if (!m_sStdoutBuffer.isEmpty()) {
		appendMessagesText(detectXrun(m_sStdoutBuffer));
		m_sStdoutBuffer.clear();
	}
}


// Jack audio server startup.
void qjackctlMainForm::jackStarted (void)
{
	// Show startup results...
	if (m_pJack) {
		appendMessages(tr("JACK was started with PID=%1.")
			.arg(long(m_pJack->pid())));
	}

#ifdef CONFIG_DBUS
	// Special for D-BUS control....
	if (m_pDBusControl) {
		m_bDBusStarted = true;
		appendMessages(tr("D-BUS: JACK server was started (%1 aka jackdbus).")
			.arg(m_pDBusControl->service()));
	}
#endif

	// Delay our control client...
	startJackClientDelay();
}


// Jack audio server got an error.
void qjackctlMainForm::jackError ( QProcess::ProcessError error )
{
	qjackctl_on_error(error);
}


// Jack audio server finish.
void qjackctlMainForm::jackFinished (void)
{
	// Force client code cleanup.
	if (!m_bJackShutdown)
		jackCleanup();
}


// Jack audio server cleanup.
void qjackctlMainForm::jackCleanup (void)
{
	// Force client code cleanup.
	bool bPostShutdown = m_bJackDetach;
	if (!bPostShutdown)
		stopJackClient();

	// Flush anything that maybe pending...
	flushStdoutBuffer();

	// Classic server control...
	if (m_pJack) {
		if (m_pJack->state() != QProcess::NotRunning) {
			appendMessages(tr("JACK is being forced..."));
			// Force final server shutdown...
			m_pJack->kill();
			// Give it some time to terminate gracefully and stabilize...
			stabilize(QJACKCTL_TIMER_MSECS);
		}
		// Force final server shutdown...
		appendMessages(tr("JACK was stopped"));
		// Destroy it.
		delete m_pJack;
		m_pJack = NULL;
		// Flag we need a post-shutdown script...
		bPostShutdown = true;
	}

#ifdef CONFIG_DBUS
	// Special for D-BUS control...
	if (m_pDBusControl && m_bDBusStarted) {
		m_bDBusStarted = false;
		appendMessages(tr("D-BUS: JACK server was stopped (%1 aka jackdbus).")
			.arg(m_pDBusControl->service()));
		// Flag we need a post-shutdown script...
		bPostShutdown = true;
	}
#endif

	// Cannot be detached anymore.
	m_bJackDetach = false;

	// Do we have any post-shutdown script?...
	// (this will be always called, despite we've started the server or not)
	if (bPostShutdown && m_pSetup->bPostShutdownScript
		&& !m_pSetup->sPostShutdownScriptShell.isEmpty()) {
		shellExecute(m_pSetup->sPostShutdownScriptShell,
			tr("Post-shutdown script..."),
			tr("Post-shutdown script terminated"));
	}

	// Reset server name.
	m_pSetup->sServerName.clear();

	// Stabilize final server state...
	jackStabilize();
}


// Stabilize server state...
void qjackctlMainForm::jackStabilize (void)
{
	QPalette pal;
	pal.setColor(QPalette::Foreground,
		m_pJackClient == NULL ? Qt::darkYellow : Qt::yellow);
	m_ui.ServerStateTextLabel->setPalette(pal);
	m_ui.StartToolButton->setEnabled(m_pJackClient == NULL);
	m_ui.StopToolButton->setEnabled(m_pJackClient != NULL);
	m_ui.RewindToolButton->setEnabled(false);
	m_ui.BackwardToolButton->setEnabled(false);
	m_ui.PlayToolButton->setEnabled(false);
	m_ui.PauseToolButton->setEnabled(false);
	m_ui.ForwardToolButton->setEnabled(false);
	transportPlayStatus(false);
	int iServerState;
	if (m_bJackDetach)
		iServerState = (m_pJackClient ? QJACKCTL_ACTIVE : QJACKCTL_INACTIVE);
	else
		iServerState = QJACKCTL_STOPPED;
	updateServerState(iServerState);
}


// XRUN detection routine.
QString& qjackctlMainForm::detectXrun ( QString& s )
{
	if (m_iXrunSkips > 1)
		return s;
	QRegExp rx(m_pSetup->sXrunRegex);
	int iPos = rx.indexIn(s);
	if (iPos >= 0) {
		s.insert(iPos + rx.matchedLength(), "</font>");
		s.insert(iPos, "<font color=\"#cc0000\">");
	#ifndef CONFIG_JACK_XRUN_DELAY
		m_tXrunLast.restart();
		updateXrunStats(rx.cap(1).toFloat());
		refreshXrunStats();
	#endif
	}
	return s;
}


// Update the XRUN last delay and immediate statistical values (in msecs).
void qjackctlMainForm::updateXrunStats ( float fXrunLast )
{
	if (m_iXrunStats > 0) {
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
 #if !defined(WIN32)
	// Set non-blocking reads, if not already...
	const int iFlags = ::fcntl(fd, F_GETFL, 0);
	int iBlock = ((iFlags & O_NONBLOCK) == 0);
	if (iBlock)
		iBlock = ::fcntl(fd, F_SETFL, iFlags | O_NONBLOCK);
	// Read as much as is available...
	QString sTemp;
	char achBuffer[1024];
	const int cchBuffer = sizeof(achBuffer) - 1;
	int cchRead = ::read(fd, achBuffer, cchBuffer);
	while (cchRead > 0) {
		achBuffer[cchRead] = (char) 0;
		sTemp.append(achBuffer);
		cchRead = (iBlock ? 0 : ::read(fd, achBuffer, cchBuffer));
	}
	// Needs to be non-empty...
	if (!sTemp.isEmpty())
		appendStdoutBuffer(sTemp);
#endif	
}


// Messages output methods.
void qjackctlMainForm::appendMessages ( const QString& s )
{
	if (m_pMessagesStatusForm)
		m_pMessagesStatusForm->appendMessages(s);
}

void qjackctlMainForm::appendMessagesColor ( const QString& s, const QString& c )
{
	if (m_pMessagesStatusForm)
		m_pMessagesStatusForm->appendMessagesColor(s, c);
}

void qjackctlMainForm::appendMessagesText ( const QString& s )
{
	if (m_pMessagesStatusForm)
		m_pMessagesStatusForm->appendMessagesText(s);
}

void qjackctlMainForm::appendMessagesError ( const QString& s )
{
	if (m_pMessagesStatusForm) {
		m_pMessagesStatusForm->setTabPage(
			int(qjackctlMessagesStatusForm::MessagesTab));
		m_pMessagesStatusForm->show();
	}

	appendMessagesColor(s.simplified(), "#ff0000");

	const QString& sTitle = tr("Error") + " - " QJACKCTL_SUBTITLE1;
#ifdef CONFIG_SYSTEM_TRAY
	if (m_pSetup->bSystemTray && m_pSystemTray
		&& QSystemTrayIcon::supportsMessages())
		m_pSystemTray->showMessage(sTitle, s, QSystemTrayIcon::Critical);
	else
#endif
	QMessageBox::critical(this, sTitle, s, QMessageBox::Cancel);
}


// Force update of the messages font.
void qjackctlMainForm::updateMessagesFont (void)
{
	if (m_pSetup == NULL)
		return;

	if (m_pMessagesStatusForm && !m_pSetup->sMessagesFont.isEmpty()) {
		QFont font;
		if (font.fromString(m_pSetup->sMessagesFont))
			m_pMessagesStatusForm->setMessagesFont(font);
	}
}


// Update messages window line limit.
void qjackctlMainForm::updateMessagesLimit (void)
{
	if (m_pSetup == NULL)
		return;

	if (m_pMessagesStatusForm) {
		if (m_pSetup->bMessagesLimit)
			m_pMessagesStatusForm->setMessagesLimit(m_pSetup->iMessagesLimitLines);
		else
			m_pMessagesStatusForm->setMessagesLimit(-1);
	}
}


// Update messages logging state.
void qjackctlMainForm::updateMessagesLogging (void)
{
	if (m_pSetup == NULL)
		return;

	if (m_pMessagesStatusForm) {
		m_pMessagesStatusForm->setLogging(
			m_pSetup->bMessagesLog, m_pSetup->sMessagesLogPath);
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


// Update of JACK client/port alias display mode.
void qjackctlMainForm::updateJackClientPortAlias (void)
{
	if (m_pSetup == NULL)
		return;

	qjackctlJackClientList::setJackClientPortAlias(m_pSetup->iJackClientPortAlias);

	refreshJackConnections();
}


// Update of JACK client/port pretty-name (metadata) display mode.
void qjackctlMainForm::updateJackClientPortMetadata (void)
{
	if (m_pSetup == NULL)
		return;

	qjackctlJackClientList::setJackClientPortMetadata(m_pSetup->bJackClientPortMetadata);

	refreshJackConnections();
}


// Update the connection and patchbay line style.
void qjackctlMainForm::updateBezierLines (void)
{
	if (m_pSetup == NULL)
		return;

	if (m_pConnectionsForm) {
		m_pConnectionsForm->audioConnectView()->setBezierLines(m_pSetup->bBezierLines);
		m_pConnectionsForm->midiConnectView()->setBezierLines(m_pSetup->bBezierLines);
		m_pConnectionsForm->alsaConnectView()->setBezierLines(m_pSetup->bBezierLines);
		m_pConnectionsForm->audioConnectView()->connectorView()->update();
		m_pConnectionsForm->midiConnectView()->connectorView()->update();
		m_pConnectionsForm->alsaConnectView()->connectorView()->update();
	}

	if (m_pPatchbayForm) {
		m_pPatchbayForm->patchbayView()->setBezierLines(m_pSetup->bBezierLines);
		m_pPatchbayForm->patchbayView()->PatchworkView()->update();
	}
}


// Update main display background effect.
void qjackctlMainForm::updateDisplayEffect (void)
{
	if (m_pSetup == NULL)
		return;

	// Set the main background...
	QPalette pal;
	if (m_pSetup->bDisplayEffect) {
		QPixmap pm(":/images/displaybg1.png");
		pal.setBrush(QPalette::Background, QBrush(pm));
	} else {
		pal.setColor(QPalette::Background, Qt::black);
	}
	m_ui.StatusDisplayFrame->setPalette(pal);
}


// Force update of big time display related fonts.
void qjackctlMainForm::updateTimeDisplayFonts (void)
{
	QFont font;
	if (!m_pSetup->sDisplayFont1.isEmpty() && font.fromString(m_pSetup->sDisplayFont1))
		m_ui.TimeDisplayTextLabel->setFont(font);
	if (!m_pSetup->sDisplayFont2.isEmpty() && font.fromString(m_pSetup->sDisplayFont2)) {
		m_ui.ServerStateTextLabel->setFont(font);
		m_ui.ServerModeTextLabel->setFont(font);
		m_ui.DspLoadTextLabel->setFont(font);
		m_ui.SampleRateTextLabel->setFont(font);
		m_ui.XrunCountTextLabel->setFont(font);
		m_ui.TransportStateTextLabel->setFont(font);
		m_ui.TransportBpmTextLabel->setFont(font);
		font.setBold(true);
		m_ui.TransportTimeTextLabel->setFont(font);
	}
}


// Force update of big time display related tooltips.
void qjackctlMainForm::updateTimeDisplayToolTips (void)
{
	QString sTimeDisplay   = tr("Transport BBT (bar.beat.ticks)");
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

	m_ui.TimeDisplayTextLabel->setToolTip(sTimeDisplay);
	m_ui.TransportTimeTextLabel->setToolTip(sTransportTime);
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
		m_ui.StartToolButton->show();
		m_ui.StopToolButton->show();
		m_ui.MessagesStatusToolButton->show();
		m_ui.SessionToolButton->show();
		m_ui.ConnectionsToolButton->show();
		m_ui.PatchbayToolButton->show();
	} else {
		m_ui.StartToolButton->hide();
		m_ui.StopToolButton->hide();
		m_ui.MessagesStatusToolButton->hide();
		m_ui.SessionToolButton->hide();
		m_ui.ConnectionsToolButton->hide();
		m_ui.PatchbayToolButton->hide();
	}

	if (m_pSetup->bRightButtons) {
		m_ui.QuitToolButton->show();
		m_ui.SetupToolButton->show();
	} else {
		m_ui.QuitToolButton->hide();
		m_ui.SetupToolButton->hide();
	}

	if (m_pSetup->bRightButtons &&
		(m_pSetup->bLeftButtons || m_pSetup->bTransportButtons)) {
		m_ui.AboutToolButton->show();
	} else {
		m_ui.AboutToolButton->hide();
	}

	if (m_pSetup->bLeftButtons || m_pSetup->bTransportButtons) {
		m_ui.RewindToolButton->show();
		m_ui.BackwardToolButton->show();
		m_ui.PlayToolButton->show();
		m_ui.PauseToolButton->show();
		m_ui.ForwardToolButton->show();
	} else {
		m_ui.RewindToolButton->hide();
		m_ui.BackwardToolButton->hide();
		m_ui.PlayToolButton->hide();
		m_ui.PauseToolButton->hide();
		m_ui.ForwardToolButton->hide();
	}

	Qt::ToolButtonStyle toolButtonStyle = (m_pSetup->bTextLabels
		? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
	m_ui.StartToolButton->setToolButtonStyle(toolButtonStyle);
	m_ui.StopToolButton->setToolButtonStyle(toolButtonStyle);
	m_ui.MessagesStatusToolButton->setToolButtonStyle(toolButtonStyle);
	m_ui.SessionToolButton->setToolButtonStyle(toolButtonStyle);
	m_ui.ConnectionsToolButton->setToolButtonStyle(toolButtonStyle);
	m_ui.PatchbayToolButton->setToolButtonStyle(toolButtonStyle);
	m_ui.QuitToolButton->setToolButtonStyle(toolButtonStyle);
	m_ui.SetupToolButton->setToolButtonStyle(toolButtonStyle);
	m_ui.AboutToolButton->setToolButtonStyle(toolButtonStyle);

	adjustSize();
}


// Force update of active patchbay definition profile, if applicable.
bool qjackctlMainForm::isActivePatchbay ( const QString& sPatchbayPath ) const
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
		// Check whether to reset/disconect-all on patchbay activation...
		if (m_pSetup->bActivePatchbayReset) {
			if (m_pJackClient) {
				m_pPatchbayRack->disconnectAllJackPorts(m_pJackClient);
				m_iJackRefresh = 0;
			}
			if (m_pAlsaSeq) {
				m_pPatchbayRack->disconnectAllAlsaPorts(m_pAlsaSeq);
				m_iAlsaRefresh = 0;
			}
			appendMessages(tr("Patchbay reset."));
		}
		// Load/activate patchbay-rack...
		const QFileInfo fi(m_pSetup->sActivePatchbayPath);
		if (fi.isRelative())
			m_pSetup->sActivePatchbayPath = fi.absoluteFilePath();
		if (!qjackctlPatchbayFile::load(m_pPatchbayRack, m_pSetup->sActivePatchbayPath)) {
			appendMessagesError(
				tr("Could not load active patchbay definition.\n\n\"%1\"\n\nDisabled.")
				.arg(m_pSetup->sActivePatchbayPath));
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
	m_ui.MessagesStatusToolButton->setChecked(
		m_pMessagesStatusForm && m_pMessagesStatusForm->isVisible());
	m_ui.SessionToolButton->setChecked(
		m_pSessionForm && m_pSessionForm->isVisible());
	m_ui.ConnectionsToolButton->setChecked(
		m_pConnectionsForm && m_pConnectionsForm->isVisible());
	m_ui.PatchbayToolButton->setChecked(
		m_pPatchbayForm && m_pPatchbayForm->isVisible());
	m_ui.SetupToolButton->setChecked(
		m_pSetupForm && m_pSetupForm->isVisible());
}


// Stabilize current business over the application event loop.
void qjackctlMainForm::stabilize ( int msecs )
{
	QTime t;
	t.start();
	while (t.elapsed() < msecs)
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}


// Reset XRUN cache items.
void qjackctlMainForm::resetXrunStats (void)
{
	m_tResetLast = QTime::currentTime();

	m_iXrunStats = 0;
	m_iXrunCount = 0;
	m_fXrunTotal = 0.0f;
	m_fXrunMin   = 0.0f;
	m_fXrunMax   = 0.0f;
	m_fXrunLast  = 0.0f;

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
		if (m_pSystemTray)
			m_pSystemTray->setBackground(color);
	}   // Reset the system tray icon background!
	else if (m_pSystemTray)
		m_pSystemTray->setBackground(Qt::transparent);

	QPalette pal;
	pal.setColor(QPalette::Foreground, color);
	m_ui.XrunCountTextLabel->setPalette(pal);

	QString sText = QString::number(m_iXrunCount);
	sText += " (";
	sText += QString::number(m_iXrunCallbacks);
	sText += ")";
	updateStatusItem(STATUS_XRUN_COUNT, sText);
}

// Convert whole elapsed seconds to hh:mm:ss time format.
QString qjackctlMainForm::formatTime ( float secs ) const
{
	unsigned int hh, mm, ss;

	hh = mm = ss = 0;
	if (secs >= 3600.0f) {
		hh    = (unsigned int) (secs / 3600.0f);
		secs -= (float) hh * 3600.0f;
	}
	if (secs >= 60.0f) {
		mm    = (unsigned int) (secs / 60.0f);
		secs -= (float) mm * 60.0f;
	}
	if (secs >= 0.0) {
		ss    = (unsigned int) secs;
		secs -= (float) ss;
	}

	QString sTemp;
	switch (m_pSetup->iTimeFormat) {
	case 1:   // Tenths of second.
		sTemp.sprintf("%02u:%02u:%02u.%u", hh, mm, ss,
			(unsigned int) (secs * 10.0f));
		break;
	case 2:   // Hundredths of second.
		sTemp.sprintf("%02u:%02u:%02u.%02u", hh, mm, ss,
			(unsigned int) (secs * 100.0f));
		break;
	case 3:   // Raw milliseconds
		sTemp.sprintf("%02u:%02u:%02u.%03u", hh, mm, ss,
			(unsigned int) (secs * 1000.0f));
		break;
	default:  // No second decimation.
		sTemp.sprintf("%02u:%02u:%02u", hh, mm, ss);
		break;
	}

	return sTemp;
}


// Update the XRUN last/elapsed time item.
QString qjackctlMainForm::formatElapsedTime ( int iStatusItem,
	const QTime& t, bool bElapsed ) const
{
	QString sTemp = m_sTimeDashes;
	QString sText;

	// Compute and format elapsed time.
	if (t.isNull()) {
		sText = sTemp;
	} else {
		sText = t.toString();
		if (m_pJackClient) {
			float secs = float(t.elapsed()) / 1000.0f;
			if (bElapsed && secs > 0) {
				sTemp = formatTime(secs);
				sText += " (" + sTemp + ")";
			}
		}
	}

	// Display elapsed time as big time?
	if ((iStatusItem == STATUS_RESET_TIME
			&& m_pSetup->iTimeDisplay == DISPLAY_RESET_TIME) ||
		(iStatusItem == STATUS_XRUN_TIME
			&& m_pSetup->iTimeDisplay == DISPLAY_XRUN_TIME)) {
		m_ui.TimeDisplayTextLabel->setText(sTemp);
	}

	return sText;
}


// Update the XRUN last/elapsed time item.
void qjackctlMainForm::updateElapsedTimes (void)
{
	// Display time remaining on start delay...
	if (m_iTimerDelay < m_iStartDelay)
		m_ui.TimeDisplayTextLabel->setText(formatTime(
			float(m_iStartDelay - m_iTimerDelay) / 1000.0f));
	else {
		updateStatusItem(STATUS_RESET_TIME,
			formatElapsedTime(STATUS_RESET_TIME,
				m_tResetLast, true));
		updateStatusItem(STATUS_XRUN_TIME,
			formatElapsedTime(STATUS_XRUN_TIME,
				m_tXrunLast, ((m_iXrunCount + m_iXrunCallbacks) > 0)));
	}
}


// Update the XRUN list view items.
void qjackctlMainForm::refreshXrunStats (void)
{
	updateXrunCount();

	if (m_fXrunTotal < 0.001f) {
		QString n = "--";
		updateStatusItem(STATUS_XRUN_TOTAL, n);
		updateStatusItem(STATUS_XRUN_MIN, n);
		updateStatusItem(STATUS_XRUN_MAX, n);
		updateStatusItem(STATUS_XRUN_AVG, n);
		updateStatusItem(STATUS_XRUN_LAST, n);
	} else {
		float fXrunAverage = 0.0f;
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


// Jack port/graph change event notifier.
void qjackctlMainForm::portNotifyEvent (void)
{
	// Log some message here, if new.
	if (m_iJackRefresh == 0)
		appendMessagesColor(tr("JACK connection graph change."), "#cc9966");
	// Do what has to be done.
	refreshJackConnections();
	// We'll be dirty too...
	m_iJackDirty++;
}


// Jack XRUN event notifier.
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
	updateXrunStats(0.001f * jack_get_xrun_delayed_usecs(m_pJackClient));
#endif

	// Just log this single event...
	appendMessagesColor(tr("XRUN callback (%1).")
		.arg(m_iXrunCallbacks), "#cc66cc");
}


// Jack buffer size event notifier.
void qjackctlMainForm::buffNotifyEvent (void)
{
	// Don't need to nothing, it was handled on qjackctl_buffer_size_callback;
	// just log this event as routine.
	appendMessagesColor(tr("Buffer size change (%1).")
		.arg((int) g_nframes), "#996633");
}


// Jack shutdown event notifier.
void qjackctlMainForm::shutNotifyEvent (void)
{
	// Log this event.
	appendMessagesColor(tr("Shutdown notification."), "#cc6666");
	// SHUTDOWN: JACK client handle might not be valid anymore...
	m_bJackShutdown = true;
	// m_pJackClient = NULL;
	// Do what has to be done.
	stopJackServer();
}


// Process exit event notifier.
void qjackctlMainForm::exitNotifyEvent (void)
{
	// Poor-mans read, copy, update (RCU)
	QProcess::ProcessError error = g_error;
	g_error = QProcess::UnknownError;

	switch (error) {
	case QProcess::FailedToStart:
		appendMessagesError(tr("Could not start JACK.\n\nSorry."));
		jackFinished();
		break;
	case QProcess::Crashed:
		appendMessagesColor(tr("JACK has crashed."), "#cc3366");
		break;
	case QProcess::Timedout:
		appendMessagesColor(tr("JACK timed out."), "#cc3366");
		break;
	case QProcess::WriteError:
		appendMessagesColor(tr("JACK write error."), "#cc3366");
		break;
	case QProcess::ReadError:
		appendMessagesColor(tr("JACK read error."), "#cc3366");
		break;
	case QProcess::UnknownError:
	default:
		appendMessagesColor(tr("Unknown JACK error (%d).")
			.arg(int(error)), "#990099");
		break;
	}
}

#ifdef CONFIG_JACK_METADATA
// Jack property (metadata) event notifier.
void qjackctlMainForm::propNotifyEvent (void)
{
	// Log some message here, if new.
	if (m_iJackRefresh == 0)
		appendMessagesColor(tr("JACK property change."), "#993366");
	// Special refresh mode...
	m_iJackPropertyChange++;
	// Do what has to be done.
	refreshJackConnections();
	// We'll be dirty too...
	m_iJackDirty++;
}
#endif


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
		appendMessagesColor(tr("ALSA connection graph change."), "#66cc99");
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
			// If we cannot start it now,
			// maybe we ought to cease & desist...
			if (!startJackClient(false))
				stopJackServer();
		}
	}

	// Is the connection patchbay dirty enough?
	if (m_pConnectionsForm) {
		const QString sEllipsis = "...";
		// Are we about to enforce an audio connections persistence profile?
		if (m_iJackDirty > 0) {
			m_iJackDirty = 0;
			if (m_pSessionForm)
				m_pSessionForm->updateSession();
			if (m_pSetup->bActivePatchbay) {
				appendMessagesColor(
					tr("JACK active patchbay scan") + sEllipsis, "#6699cc");
				m_pPatchbayRack->connectJackScan(m_pJackClient);
			}
			refreshJackConnections();
		}
		// Or is it from the MIDI field?
		if (m_iAlsaDirty > 0) {
			m_iAlsaDirty = 0;
			if (m_pSetup->bActivePatchbay) {
				appendMessagesColor(
					tr("ALSA active patchbay scan") + sEllipsis, "#99cc66");
				m_pPatchbayRack->connectAlsaScan(m_pAlsaSeq);
			}
			refreshAlsaConnections();
		}
		// Are we about to refresh it, really?
		if (m_iJackRefresh > 0 && m_pJackClient != NULL) {
			m_iJackRefresh = 0;
		#ifdef CONFIG_JACK_METADATA
			const bool bClear = (m_iJackPropertyChange > 0);
			m_iJackPropertyChange = 0;
			m_pConnectionsForm->refreshAudio(true, bClear);
			m_pConnectionsForm->refreshMidi(true, bClear);
		#else
			m_pConnectionsForm->refreshAudio(true);
			m_pConnectionsForm->refreshMidi(true);
		#endif
		}
		if (m_iAlsaRefresh > 0 && m_pAlsaSeq != NULL) {
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
		appendMessagesColor(tr("JACK connection change."), "#9999cc");
}


// ALSA connection notification slot.
void qjackctlMainForm::alsaConnectChanged (void)
{
	// Just shake the MIDI connections status quo.
	if (++m_iAlsaDirty == 1)
		appendMessagesColor(tr("ALSA connection change."), "#cccc99");
}


// Cable connection notification slot.
void qjackctlMainForm::cableConnectSlot (
	const QString& sOutputPort, const QString& sInputPort,
	unsigned int ulCableFlags )
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

	appendMessagesColor(sText + '.', sColor);
}


// Patchbay (dis)connection slot.
void qjackctlMainForm::queryDisconnect (
	qjackctlPortItem *pOPort, qjackctlPortItem *pIPort, int iSocketType )
{
	if (m_pSetup->bActivePatchbay && m_pSetup->bQueryDisconnect) {
		qjackctlPatchbayCable *pCable = m_pPatchbayRack->findCable(
			pOPort->clientName(), pOPort->portName(),
			pIPort->clientName(), pIPort->portName(), iSocketType);
		if (pCable) {
			bool bQueryDisconnect = true;
			const QString& sTitle
				= tr("Warning") + " - " QJACKCTL_SUBTITLE1;
			const QString& sText
				= tr("A patchbay definition is currently active,\n"
					"which is probable to redo this connection:\n\n"
					"%1 -> %2\n\n"
					"Do you want to remove the patchbay connection?")
					.arg(pCable->outputSocket()->name())
					.arg(pCable->inputSocket()->name());
		#if 0//QJACKCTL_QUERY_DISCONNECT
			bQueryDisconnect = (QMessageBox::warning(this, sTitle, Text,
				QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok);
		#else
			QMessageBox mbox(this);
			mbox.setIcon(QMessageBox::Warning);
			mbox.setWindowTitle(sTitle);
			mbox.setText(sText);
			mbox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
			QCheckBox cbox(tr("Don't ask this again"));
			cbox.setChecked(false);
			cbox.blockSignals(true);
			mbox.addButton(&cbox, QMessageBox::ActionRole);
			bQueryDisconnect = (mbox.exec() == QMessageBox::Ok);
			if (bQueryDisconnect && cbox.isChecked())
				m_pSetup->bQueryDisconnect = false;
		#endif
			if (bQueryDisconnect)
				m_pPatchbayRack->removeCable(pCable);
		}
		// Refresh patchbay form anyway...
		if (m_pPatchbayForm
			&& isActivePatchbay(m_pPatchbayForm->patchbayPath()))
			m_pPatchbayForm->loadPatchbayRack(m_pPatchbayRack);
	}
}


// Delay jack control client start...
void qjackctlMainForm::startJackClientDelay (void)
{
	// Sloppy boy fix: may the serve be stopped, just in case
	// the client will nerver make it...
	m_ui.StopToolButton->setEnabled(true);

	// Make sure all status(es) will be updated ASAP...
	m_iStatusRefresh += QJACKCTL_STATUS_CYCLE;
	m_iStatusBlink = 0;

	// Reset (yet again) the timer counters...
	m_iStartDelay  = 1 + (m_preset.iStartDelay * 1000);
	m_iTimerDelay  = 0;
	m_iJackRefresh = 0;
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
	m_iStatusBlink = 0;

	// Are we about to start detached?
	if (bDetach) {
		// To fool timed client initialization delay.
		m_iTimerDelay += (m_iStartDelay + 1);
		// Refresh status (with dashes?)
		refreshStatus();
	}

	// Create the jack client handle, using a distinct identifier (PID?)
	const char *pszClientName = "qjackctl";
	jack_status_t status = JackFailure;
	if (m_pSetup->sServerName.isEmpty()) {
		m_pJackClient = jack_client_open(pszClientName,
			JackNoStartServer, &status);
	} else {
		m_pJackClient = jack_client_open(pszClientName,
			jack_options_t(JackNoStartServer | JackServerName), &status,
			m_pSetup->sServerName.toUtf8().constData());
	}

	if (m_pJackClient == NULL) {
		if (!bDetach) {
			QStringList errs;
			if (status & JackFailure)
				errs << tr("Overall operation failed.");
			if (status & JackInvalidOption)
				errs << tr("Invalid or unsupported option.");
			if (status & JackNameNotUnique)
				errs << tr("Client name not unique.");
			if (status & JackServerStarted)
				errs << tr("Server is started.");
			if (status & JackServerFailed)
				errs << tr("Unable to connect to server.");
			if (status & JackServerError)
				errs << tr("Server communication error.");
			if (status & JackNoSuchClient)
				errs << tr("Client does not exist.");
			if (status & JackLoadFailure)
				errs << tr("Unable to load internal client.");
			if (status & JackInitFailure)
				errs << tr("Unable to initialize client.");
			if (status & JackShmFailure)
				errs << tr("Unable to access shared memory.");
			if (status & JackVersionError)
				errs << tr("Client protocol version mismatch.");
			appendMessagesError(
				tr("Could not connect to JACK server as client.\n"
				"- %1\nPlease check the messages window for more info.")
				.arg(errs.join("\n- ")));
		}
		return false;
	}

	// Set notification callbacks.
	jack_set_graph_order_callback(m_pJackClient,
		qjackctl_graph_order_callback, this);
	jack_set_port_registration_callback(m_pJackClient,
		qjackctl_port_registration_callback, this);
	jack_set_xrun_callback(m_pJackClient,
		qjackctl_xrun_callback, this);
	jack_set_buffer_size_callback(m_pJackClient,
		qjackctl_buffer_size_callback, this);
	jack_on_shutdown(m_pJackClient,
		qjackctl_on_shutdown, this);
#ifdef CONFIG_JACK_METADATA
	jack_set_property_change_callback(m_pJackClient,
		qjackctl_property_change_callback, this);
#endif

	// First knowledge about buffer size.
	g_nframes = jack_get_buffer_size(m_pJackClient);

	// Reconstruct our connections and session...
	if (m_pConnectionsForm) {
		m_pConnectionsForm->stabilizeAudio(true);
		m_pConnectionsForm->stabilizeMidi(true);
	}
	if (m_pSessionForm)
		m_pSessionForm->stabilizeForm(true);

	// Save server configuration file.
	if (m_pSetup->bServerConfig && !m_sJackCmdLine.isEmpty()) {
		QString sJackCmdLine = m_sJackCmdLine;
		if (m_pSetup->bServerConfigTemp) {
			int iPos = sJackCmdLine.indexOf(' ');
			if (iPos > 0)
				sJackCmdLine = sJackCmdLine.insert(iPos, " -T");
		}
		QString sFilename = ::getenv("HOME");
		sFilename += '/' + m_pSetup->sServerConfigName;
		QFile file(sFilename);
		if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			QTextStream(&file) << sJackCmdLine << endl;
			file.close();
			appendMessagesColor(
				tr("Server configuration saved to \"%1\".")
				.arg(sFilename), "#999933");
		}
	}

	// Do not forget to reset XRUN stats variables.
	if (!bDetach)
		resetXrunStats();
	else // We'll flag that we've been detached!
		m_bJackDetach = true;

	// Activate us as a client...
	jack_activate(m_pJackClient);

	// All displays are highlighted from now on.
	QPalette pal;
	pal.setColor(QPalette::Foreground, Qt::yellow);
	m_ui.ServerStateTextLabel->setPalette(pal);
	m_ui.DspLoadTextLabel->setPalette(pal);
	m_ui.ServerModeTextLabel->setPalette(pal);
	pal.setColor(QPalette::Foreground, Qt::darkYellow);
	m_ui.SampleRateTextLabel->setPalette(pal);
	pal.setColor(QPalette::Foreground, Qt::green);
	m_ui.TimeDisplayTextLabel->setPalette(pal);
	m_ui.TransportStateTextLabel->setPalette(pal);
	m_ui.TransportBpmTextLabel->setPalette(pal);
	m_ui.TransportTimeTextLabel->setPalette(pal);

	// Whether we've started detached, just change active status.
	updateServerState(m_bJackDetach ? QJACKCTL_ACTIVE : QJACKCTL_STARTED);
	m_ui.StopToolButton->setEnabled(true);

	// Log success here.
	appendMessages(tr("Client activated."));

	// Formal patchbay activation, if any, is in order...
	updateActivePatchbay();

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
		m_pSetup->sCmdLine.clear();
	}

	// Remember to schedule an initial connection refreshment.
	refreshConnections();

	// OK, we're at it!
	return true;
}


// Stop jack audio client...
void qjackctlMainForm::stopJackClient (void)
{
	// Deactivate and close us as a client...
	if (m_pJackClient) {
		jack_deactivate(m_pJackClient);
		jack_client_close(m_pJackClient);
		m_pJackClient = NULL;
		// Log deactivation here.
		appendMessages(tr("Client deactivated."));
	}

	// Reset command-line configuration info.
	m_sJackCmdLine.clear();

	// Clear out the connections and session...
	if (m_pConnectionsForm) {
		m_pConnectionsForm->stabilizeAudio(false);
		m_pConnectionsForm->stabilizeMidi(false);
	}
	if (m_pSessionForm)
		m_pSessionForm->stabilizeForm(false);

	// Displays are dimmed again.
	QPalette pal;
	pal.setColor(QPalette::Foreground, Qt::darkYellow);
	m_ui.ServerModeTextLabel->setPalette(pal);
	m_ui.DspLoadTextLabel->setPalette(pal);
	m_ui.SampleRateTextLabel->setPalette(pal);
	pal.setColor(QPalette::Foreground, Qt::darkGreen);
	m_ui.TimeDisplayTextLabel->setPalette(pal);
	m_ui.TransportStateTextLabel->setPalette(pal);
	m_ui.TransportBpmTextLabel->setPalette(pal);
	m_ui.TransportTimeTextLabel->setPalette(pal);

	// Refresh jack client statistics explicitly.
	refreshXrunStats();
}


// JACK client accessor.
jack_client_t *qjackctlMainForm::jackClient (void) const
{
	return (m_bJackShutdown ? NULL : m_pJackClient);
}


// ALSA sequencer client accessor.
snd_seq_t *qjackctlMainForm::alsaSeq (void) const
{
	return m_pAlsaSeq;
}


// Rebuild all patchbay items.
void qjackctlMainForm::refreshConnections (void)
{
	refreshJackConnections();
	refreshAlsaConnections();
}

void qjackctlMainForm::refreshJackConnections (void)
{
#if 0
	// Hack this as for a while...
	if (m_pConnectionsForm && m_iJackRefresh == 0) {
		m_pConnectionsForm->stabilizeAudio(false);
		m_pConnectionsForm->stabilizeMidi(false);
	}
#endif
	// Just increment our intentions; it will be deferred
	// to be executed just on timer slot processing...
	m_iJackRefresh++;
}

void qjackctlMainForm::refreshAlsaConnections (void)
{
#if 0
	// Hack this as for a while...
	if (m_pConnectionsForm && m_iAlsaRefresh == 0)
		m_pConnectionsForm->stabilizeAlsa(false);
#endif
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
	m_pSetup->saveWidgetGeometry(this, true);

	if (isVisible()) {
		if (m_pSetup->bSystemTray && m_pSystemTray) {
			// Hide away from sight, if not active...
			if (isActiveWindow()) {
				hide();
			} else {
				raise();
				activateWindow();
			}
		} else {
			// Minimize (iconify) normally.
			showMinimized();
		}
	} else {
		show();
		raise();
		activateWindow();
	}
}


// Message log/status form requester slot.
void qjackctlMainForm::toggleMessagesStatusForm (void)
{
	if (m_pMessagesStatusForm) {
		m_pSetup->saveWidgetGeometry(m_pMessagesStatusForm);
		if (m_pMessagesStatusForm->isVisible()) {
			m_pMessagesStatusForm->hide();
		} else {
			m_pMessagesStatusForm->show();
			m_pMessagesStatusForm->raise();
			m_pMessagesStatusForm->activateWindow();
		}
	}
}

void qjackctlMainForm::toggleMessagesForm (void)
{
	if (m_pMessagesStatusForm) {
		int iTabPage = m_pMessagesStatusForm->tabPage();
		m_pMessagesStatusForm->setTabPage(
			int(qjackctlMessagesStatusForm::MessagesTab));
		if (m_pMessagesStatusForm->isVisible()
			&& iTabPage != m_pMessagesStatusForm->tabPage())
			return;
	}

	toggleMessagesStatusForm();
}


void qjackctlMainForm::toggleStatusForm (void)
{
	if (m_pMessagesStatusForm) {
		int iTabPage = m_pMessagesStatusForm->tabPage();
		m_pMessagesStatusForm->setTabPage(
			int(qjackctlMessagesStatusForm::StatusTab));
		if (m_pMessagesStatusForm->isVisible()
			&& iTabPage != m_pMessagesStatusForm->tabPage())
			return;
	}

	toggleMessagesStatusForm();
}


// Session form requester slot.
void qjackctlMainForm::toggleSessionForm (void)
{
	if (m_pSessionForm) {
		m_pSetup->saveWidgetGeometry(m_pSessionForm);
		m_pSessionForm->stabilizeForm(m_pJackClient != NULL);
		if (m_pSessionForm->isVisible()) {
			m_pSessionForm->hide();
		} else {
			m_pSessionForm->show();
			m_pSessionForm->raise();
			m_pSessionForm->activateWindow();
		}
	}
}


// Connections form requester slot.
void qjackctlMainForm::toggleConnectionsForm (void)
{
	if (m_pConnectionsForm) {
		m_pSetup->saveWidgetGeometry(m_pConnectionsForm);
		m_pConnectionsForm->stabilizeAudio(m_pJackClient != NULL);
		m_pConnectionsForm->stabilizeMidi(m_pJackClient != NULL);
		m_pConnectionsForm->stabilizeAlsa(m_pAlsaSeq != NULL);
		if (m_pConnectionsForm->isVisible()) {
			m_pConnectionsForm->hide();
		} else {
			m_pConnectionsForm->show();
			m_pConnectionsForm->raise();
			m_pConnectionsForm->activateWindow();
		}
	}
}


// Patchbay form requester slot.
void qjackctlMainForm::togglePatchbayForm (void)
{
	if (m_pPatchbayForm) {
		m_pSetup->saveWidgetGeometry(m_pPatchbayForm);
		if (m_pPatchbayForm->isVisible()) {
			m_pPatchbayForm->hide();
		} else {
			m_pPatchbayForm->show();
			m_pPatchbayForm->raise();
			m_pPatchbayForm->activateWindow();
		}
	}
}


// Setup dialog requester slot.
void qjackctlMainForm::showSetupForm (void)
{
	if (m_pSetupForm) {
	//	m_pSetup->saveWidgetGeometry(m_pSetupForm);
		if (m_pSetupForm->isVisible()) {
			m_pSetupForm->hide();
		} else {
			m_pSetupForm->show();
			m_pSetupForm->raise();
			m_pSetupForm->activateWindow();
		}
	}
}


// About dialog requester slot.
void qjackctlMainForm::showAboutForm (void)
{
	qjackctlAboutForm(this).exec();
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
		float rate = float(tpos.frame_rate);
		float tloc = ((float(tpos.frame) / rate) - m_fSkipAccel) * rate;
		if (tloc < 0.0f) tloc = 0.0f;
		jack_transport_locate(m_pJackClient, (jack_nframes_t) tloc);
		// Log this here (if on initial toggle).
		if (m_fSkipAccel < 1.1f)
			appendMessages(tr("Transport backward."));
		// Take care of backward acceleration...
		if (m_ui.BackwardToolButton->isDown() && m_fSkipAccel < 60.0)
			m_fSkipAccel *= 1.1f;
		// Make sure all status(es) will be updated ASAP...
		m_iStatusRefresh += QJACKCTL_STATUS_CYCLE;
	}
#endif
}

// Transport toggle (start/stop)
void qjackctlMainForm::transportPlay ( bool bOn )
{
	if (m_iTransportPlay > 0)
		return;

	if (bOn)
		transportStart();
	else
		transportStop();
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
		float rate = float(tpos.frame_rate);
		float tloc = ((float(tpos.frame) / rate) + m_fSkipAccel) * rate;
		if (tloc < 0.0f) tloc = 0.0f;
		jack_transport_locate(m_pJackClient, (jack_nframes_t) tloc);
		// Log this here.
		if (m_fSkipAccel < 1.1f)
			appendMessages(tr("Transport forward."));
		// Take care of forward acceleration...
		if (m_ui.ForwardToolButton->isDown() && m_fSkipAccel < 60.0f)
			m_fSkipAccel *= 1.1f;
		// Make sure all status(es) will be updated ASAP...
		m_iStatusRefresh += QJACKCTL_STATUS_CYCLE;
	}
#endif
}


// Almost-complete running status refresher.
void qjackctlMainForm::refreshStatus (void)
{
	const QString n = "--";
	const QString b = "-.-.---";
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
			updateStatusItem(STATUS_TRANSPORT_TIME,
				formatTime(float(tpos.frame) / float(tpos.frame_rate)));
	//  else
	//      updateStatusItem(STATUS_TRANSPORT_TIME, m_sTimeDashes);
		// Transport barcode position (bar:beat.tick)
		if (tpos.valid & JackPositionBBT) {
			updateStatusItem(STATUS_TRANSPORT_BBT,
				QString().sprintf("%u.%u.%03u", tpos.bar, tpos.beat, tpos.tick));
			updateStatusItem(STATUS_TRANSPORT_BPM,
				QString::number(tpos.beats_per_minute, 'g', 4));
		} else {
			updateStatusItem(STATUS_TRANSPORT_BBT, b);
			updateStatusItem(STATUS_TRANSPORT_BPM, n);
		}
	#endif  // !CONFIG_JACK_TRANSPORT
		// Less frequent status items update...
		if (m_iStatusRefresh >= QJACKCTL_STATUS_CYCLE) {
			m_iStatusRefresh = 0;
			float fDspLoad = jack_cpu_load(m_pJackClient);
			const char f = (fDspLoad > 0.1f ? 'f' : 'g'); // format
			const int  p = (fDspLoad > 1.0f ?  1  :  2 ); // precision
			if (m_pSystemTray) {
				if (m_iXrunCount > 0) {
					m_pSystemTray->setToolTip(tr("%1 (%2%)")
						.arg(windowTitle())
						.arg(fDspLoad, 0, f, p));
				} else {
					m_pSystemTray->setToolTip(tr("%1 (%2%, %3 xruns)")
						.arg(windowTitle())
						.arg(fDspLoad, 0, f, p)
						.arg(m_iXrunCount));
				}
			}
			updateStatusItem(STATUS_DSP_LOAD,
				tr("%1 %").arg(fDspLoad, 0, f, p));
			updateStatusItem(STATUS_SAMPLE_RATE,
				tr("%1 Hz").arg(jack_get_sample_rate(m_pJackClient)));
			updateStatusItem(STATUS_BUFFER_SIZE,
				tr("%1 frames").arg(g_nframes));
			// Blink server mode indicator?...
			if (m_pSetup && m_pSetup->bDisplayBlink) {
				QPalette pal;
				pal.setColor(QPalette::Foreground,
					(++m_iStatusBlink % 2) ? Qt::darkYellow: Qt::yellow);
				m_ui.ServerModeTextLabel->setPalette(pal);
			}
		#ifdef CONFIG_JACK_REALTIME
			bool bRealtime = jack_is_realtime(m_pJackClient);
			updateStatusItem(STATUS_REALTIME,
				(bRealtime ? tr("Yes") : tr("No")));
			m_ui.ServerModeTextLabel->setText(bRealtime ? tr("RT") : n);
		#else
			updateStatusItem(STATUS_REALTIME, n);
			m_ui.ServerModeTextLabel->setText(n);
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
			m_ui.RewindToolButton->setEnabled(tpos.frame > 0);
			m_ui.BackwardToolButton->setEnabled(tpos.frame > 0);
			m_ui.PlayToolButton->setEnabled(true);
			m_ui.PauseToolButton->setEnabled(bPlaying);
			m_ui.ForwardToolButton->setEnabled(true);
			transportPlayStatus(bPlaying);
			if (!m_ui.BackwardToolButton->isDown() &&
				!m_ui.ForwardToolButton->isDown())
				m_fSkipAccel = 1.0;
		#else
			updateStatusItem(STATUS_TRANSPORT_STATE, n);
			m_ui.RewindToolButton->setEnabled(false);
			m_ui.BackwardToolButton->setEnabled(false);
			m_ui.PlayToolButton->setEnabled(false);
			m_ui.PauseToolButton->setEnabled(false);
			m_ui.ForwardToolButton->setEnabled(false);
			transportPlayStatus(false);
			updateStatusItem(STATUS_TRANSPORT_TIME, m_sTimeDashes);
			updateStatusItem(STATUS_TRANSPORT_BBT, b);
			updateStatusItem(STATUS_TRANSPORT_BPM, n);
		#endif  // !CONFIG_JACK_TRANSPORT
		#ifdef CONFIG_JACK_MAX_DELAY
			updateStatusItem(STATUS_MAX_DELAY, tr("%1 msec")
				.arg(0.001f * jack_get_max_delayed_usecs(m_pJackClient)));
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
		updateStatusItem(STATUS_DSP_LOAD, n);
		updateStatusItem(STATUS_SAMPLE_RATE, n);
		updateStatusItem(STATUS_BUFFER_SIZE, n);
		updateStatusItem(STATUS_REALTIME, n);
		m_ui.ServerModeTextLabel->setText(n);
		updateStatusItem(STATUS_TRANSPORT_STATE, n);
		updateStatusItem(STATUS_TRANSPORT_TIME, m_sTimeDashes);
		updateStatusItem(STATUS_TRANSPORT_BBT, b);
		updateStatusItem(STATUS_TRANSPORT_BPM, n);
		m_ui.RewindToolButton->setEnabled(false);
		m_ui.BackwardToolButton->setEnabled(false);
		m_ui.PlayToolButton->setEnabled(false);
		m_ui.PauseToolButton->setEnabled(false);
		m_ui.ForwardToolButton->setEnabled(false);
		transportPlayStatus(false);
	}

	// Elapsed times should be rigorous...
	updateElapsedTimes();
}


// Status item updater.
void qjackctlMainForm::updateStatusItem( int iStatusItem, const QString& sText )
{
	switch (iStatusItem) {
	case STATUS_SERVER_STATE:
		m_ui.ServerStateTextLabel->setText(sText);
		break;
	case STATUS_DSP_LOAD:
		m_ui.DspLoadTextLabel->setText(sText);
		break;
	case STATUS_SAMPLE_RATE:
		m_ui.SampleRateTextLabel->setText(sText);
		break;
	case STATUS_XRUN_COUNT:
		m_ui.XrunCountTextLabel->setText(sText);
		break;
	case STATUS_TRANSPORT_STATE:
		m_ui.TransportStateTextLabel->setText(sText);
		break;
	case STATUS_TRANSPORT_TIME:
		if (m_pSetup->iTimeDisplay == DISPLAY_TRANSPORT_TIME)
			m_ui.TimeDisplayTextLabel->setText(sText);
		else
			m_ui.TransportTimeTextLabel->setText(sText);
		break;
	case STATUS_TRANSPORT_BBT:
		if (m_pSetup->iTimeDisplay == DISPLAY_TRANSPORT_BBT)
			m_ui.TimeDisplayTextLabel->setText(sText);
		else
		if (m_pSetup->iTimeDisplay == DISPLAY_TRANSPORT_TIME)
			m_ui.TransportTimeTextLabel->setText(sText);
		break;
	case STATUS_TRANSPORT_BPM:
		m_ui.TransportBpmTextLabel->setText(sText);
		break;
	}

	if (m_pMessagesStatusForm)
		m_pMessagesStatusForm->updateStatusItem(iStatusItem, sText);
}


// Main window caption title and system tray icon and tooltip update.
void qjackctlMainForm::updateTitleStatus (void)
{
	QString sTitle;

	if (!m_pSetup->bLeftButtons  ||
		!m_pSetup->bRightButtons ||
		!m_pSetup->bTextLabels) {
		sTitle = QJACKCTL_SUBTITLE0;
	} else {
		sTitle = QJACKCTL_SUBTITLE1;
	}

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
	setWindowTitle(sTitle);

	updateStatusItem(STATUS_SERVER_STATE, sState);

	if (m_pSystemTray) {
		switch (m_iServerState) {
		case QJACKCTL_STARTING:
			m_pSystemTray->setPixmapOverlay(QPixmap(":/images/xstarting1.png"));
			break;
		case QJACKCTL_STARTED:
			m_pSystemTray->setPixmapOverlay(QPixmap(":/images/xstarted1.png"));
			break;
		case QJACKCTL_STOPPING:
			m_pSystemTray->setPixmapOverlay(QPixmap(":/images/xstopping1.png"));
			break;
		case QJACKCTL_STOPPED:
			m_pSystemTray->setPixmapOverlay(QPixmap(":/images/xstopped1.png"));
			break;
		case QJACKCTL_ACTIVE:
			m_pSystemTray->setPixmapOverlay(QPixmap(":/images/xactive1.png"));
			break;
		case QJACKCTL_ACTIVATING:
			m_pSystemTray->setPixmapOverlay(QPixmap(":/images/xactivating1.png"));
			break;
		case QJACKCTL_INACTIVE:
		default:
			m_pSystemTray->setPixmapOverlay(QPixmap(":/images/xinactive1.png"));
			break;
		}
		m_pSystemTray->setToolTip(sTitle);
	}

	sTitle = m_pSetup->sServerName;
	if (sTitle.isEmpty())
		sTitle = ::getenv("JACK_DEFAULT_SERVER");
	if (sTitle.isEmpty())
		sTitle = m_pSetup->sDefPresetName;
	updateStatusItem(STATUS_SERVER_NAME, sTitle);
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
		QObject::connect(m_pSystemTray,
			SIGNAL(clicked()),
			SLOT(toggleMainForm()));
		QObject::connect(m_pSystemTray,
			SIGNAL(middleClicked()),
			SLOT(resetXrunStats()));
		QObject::connect(m_pSystemTray,
			SIGNAL(contextMenuRequested(const QPoint &)),
			SLOT(systemTrayContextMenu(const QPoint &)));
		m_pSystemTray->show();
	} else {
		// Make sure the main widget is visible.
		show();
		raise();
		activateWindow();
	}
#endif
}


// System tray context menu request slot.
void qjackctlMainForm::systemTrayContextMenu ( const QPoint& pos )
{
	QMenu menu(this);
	QAction *pAction;

	QString sHideMinimize = (m_pSetup->bSystemTray && m_pSystemTray
		? tr("&Hide") : tr("Mi&nimize"));
	QString sShowRestore  = (m_pSetup->bSystemTray && m_pSystemTray
		? tr("S&how") : tr("Rest&ore"));

	pAction = menu.addAction(isVisible()
		? sHideMinimize : sShowRestore, this, SLOT(toggleMainForm()));
	menu.addSeparator();

	if (m_pJackClient == NULL) {
		pAction = menu.addAction(QIcon(":/images/start1.png"),
			tr("&Start"), this, SLOT(startJack()));
	} else {
		pAction = menu.addAction(QIcon(":/images/stop1.png"),
			tr("&Stop"), this, SLOT(stopJack()));
	}
	pAction = menu.addAction(QIcon(":/images/reset1.png"),
		tr("&Reset"), this, SLOT(resetXrunStats()));
//  pAction->setEnabled(m_pJackClient != NULL);
	menu.addSeparator();

	// Construct the actual presets menu,
	// overriding the last one, if any...
	QMenu *pPresetsMenu = menu.addMenu(tr("&Presets"));
	// Assume QStringList iteration follows item index order (0,1,2...)
	int iPreset = 0;
	QStringListIterator iter(m_pSetup->presets);
	while (iter.hasNext()) {
		const QString& sPreset = iter.next();
		pAction = pPresetsMenu->addAction(sPreset);
		pAction->setCheckable(true);
		pAction->setChecked(sPreset == m_pSetup->sDefPreset);
		pAction->setData(iPreset);
		iPreset++;
	}
	// Default preset always present, and has invalid index parameter (-1)...
	if (iPreset > 0)
		pPresetsMenu->addSeparator();
	pAction = pPresetsMenu->addAction(m_pSetup->sDefPresetName);
	pAction->setCheckable(true);
	pAction->setChecked(m_pSetup->sDefPresetName == m_pSetup->sDefPreset);
	pAction->setData(-1);
	QObject::connect(pPresetsMenu,
		SIGNAL(triggered(QAction*)),
		SLOT(activatePresetsMenu(QAction*)));
	menu.addSeparator();

	if (m_pSessionForm) {
		bool bEnabled = (m_pJackClient != NULL);
		const QString sTitle = tr("S&ession");
		const QIcon iconSession(":/images/session1.png");
		QMenu *pSessionMenu = menu.addMenu(sTitle);
		pSessionMenu->setIcon(iconSession);
		pAction = pSessionMenu->addAction(m_pSessionForm->isVisible()
			? tr("&Hide") : tr("S&how"),
			this, SLOT(toggleSessionForm()));
		pSessionMenu->addSeparator();
		pAction = pSessionMenu->addAction(QIcon(":/images/open1.png"),
			tr("&Load..."),
			m_pSessionForm, SLOT(loadSession()));
		pAction->setEnabled(bEnabled);
		QMenu *pRecentMenu = m_pSessionForm->recentMenu();
		pAction = pSessionMenu->addMenu(pRecentMenu);
		pAction->setEnabled(m_pJackClient != NULL && !pRecentMenu->isEmpty());
		pSessionMenu->addSeparator();
		pAction = pSessionMenu->addAction(QIcon(":/images/save1.png"),
			tr("&Save..."),
			m_pSessionForm, SLOT(saveSessionSave()));
		pAction->setEnabled(bEnabled);
	#ifdef CONFIG_JACK_SESSION
		pAction = pSessionMenu->addAction(
			tr("Save and &Quit..."),
			m_pSessionForm, SLOT(saveSessionSaveAndQuit()));
		pAction->setEnabled(bEnabled);
		pAction = pSessionMenu->addAction(
			tr("Save &Template..."),
			m_pSessionForm, SLOT(saveSessionSaveTemplate()));
		pAction->setEnabled(bEnabled);
	#endif
		pSessionMenu->addSeparator();
		pAction = pSessionMenu->addAction(
			tr("&Versioning"),
			m_pSessionForm, SLOT(saveSessionVersion(bool)));
		pAction->setCheckable(true);
		pAction->setChecked(m_pSessionForm->isSaveSessionVersion());
		pAction->setEnabled(bEnabled);
		pSessionMenu->addSeparator();
		pAction = pSessionMenu->addAction(QIcon(":/images/refresh1.png"),
			tr("Re&fresh"),
			m_pSessionForm, SLOT(updateSession()));
		pAction->setEnabled(bEnabled);
	}

	pAction = menu.addAction(QIcon(":/images/messages1.png"),
		tr("&Messages"), this, SLOT(toggleMessagesForm()));
	pAction->setCheckable(true);
	pAction->setChecked(m_pMessagesStatusForm
		&& m_pMessagesStatusForm->isVisible()
		&& m_pMessagesStatusForm->tabPage() == qjackctlMessagesStatusForm::MessagesTab);
	pAction = menu.addAction(QIcon(":/images/status1.png"),
		tr("St&atus"), this, SLOT(toggleStatusForm()));
	pAction->setCheckable(true);
	pAction->setChecked(m_pMessagesStatusForm
		&& m_pMessagesStatusForm->isVisible()
		&& m_pMessagesStatusForm->tabPage() == qjackctlMessagesStatusForm::StatusTab);
	pAction = menu.addAction(QIcon(":/images/connections1.png"),
		tr("&Connections"), this, SLOT(toggleConnectionsForm()));
	pAction->setCheckable(true);
	pAction->setChecked(m_pConnectionsForm && m_pConnectionsForm->isVisible());
	pAction = menu.addAction(QIcon(":/images/patchbay1.png"),
		tr("Patch&bay"), this, SLOT(togglePatchbayForm()));
	pAction->setCheckable(true);
	pAction->setChecked(m_pPatchbayForm && m_pPatchbayForm->isVisible());
	menu.addSeparator();

	QMenu *pTransportMenu = menu.addMenu(tr("&Transport"));
	pAction = pTransportMenu->addAction(QIcon(":/images/rewind1.png"),
		tr("&Rewind"), this, SLOT(transportRewind()));
	pAction->setEnabled(m_ui.RewindToolButton->isEnabled());
//	pAction = pTransportMenu->addAction(QIcon(":/images/backward1.png"),
//		tr("&Backward"), this, SLOT(transportBackward()));
//	pAction->setEnabled(m_ui.BackwardToolButton->isEnabled());
	pAction = pTransportMenu->addAction(QIcon(":/images/play1.png"),
		tr("&Play"), this, SLOT(transportStart()));
	pAction->setEnabled(!m_ui.PlayToolButton->isChecked());
	pAction = pTransportMenu->addAction(QIcon(":/images/pause1.png"),
		tr("Pa&use"), this, SLOT(transportStop()));
	pAction->setEnabled(m_ui.PauseToolButton->isEnabled());
//	pAction = pTransportMenu->addAction(QIcon(":/images/forward1.png"),
//		tr("&Forward"), this, SLOT(transportForward()));
//	pAction->setEnabled(m_ui.ForwardToolButton->isEnabled());
	menu.addSeparator();

	pAction = menu.addAction(QIcon(":/images/setup1.png"),
		tr("Set&up..."), this, SLOT(showSetupForm()));
	pAction->setCheckable(true);
	pAction->setChecked(m_pSetupForm && m_pSetupForm->isVisible());

	if (!m_pSetup->bRightButtons || !m_pSetup->bTransportButtons) {
		pAction = menu.addAction(QIcon(":/images/about1.png"),
			tr("Ab&out..."), this, SLOT(showAboutForm()));
	}
	menu.addSeparator();

	pAction = menu.addAction(QIcon(":/images/quit1.png"),
		tr("&Quit"), this, SLOT(quitMainForm()));

	menu.exec(pos);
}


// Server settings change warning.
void qjackctlMainForm::showDirtySettingsWarning (void)
{
	// If client service is currently running,
	// prompt the effective warning...
	if (m_pJackClient) {
		const QString& sTitle
			= tr("Warning") + " - " QJACKCTL_SUBTITLE1;
		const QString& sText
			= tr("Server settings will be only effective after\n"
				"restarting the JACK audio server.");
	#ifdef CONFIG_SYSTEM_TRAY
		if (m_pSetup->bSystemTray && m_pSystemTray
			&& QSystemTrayIcon::supportsMessages()) {
			m_pSystemTray->showMessage(sTitle, sText, QSystemTrayIcon::Warning);
		}
		else
	#endif
		QMessageBox::warning(this, sTitle, sText);
	}   // Otherwise, it will be just as convenient to update status...
	else updateTitleStatus();
}


// Setup otions change warning.
void qjackctlMainForm::showDirtySetupWarning (void)
{
	const QString& sTitle
		= tr("Information") + " - " QJACKCTL_SUBTITLE1;
	const QString& sText
		= tr("Some settings will be only effective\n"
			"the next time you start this program.");
#ifdef CONFIG_SYSTEM_TRAY
	if (m_pSetup->bSystemTray && m_pSystemTray
		&& QSystemTrayIcon::supportsMessages()) {
		m_pSystemTray->showMessage(sTitle, sText, QSystemTrayIcon::Information);
	}
	else
#endif
	QMessageBox::information(this, sTitle, sText);
}


// Select the current default preset (by name from context menu).
void qjackctlMainForm::activatePresetsMenu ( QAction *pAction )
{
	activatePreset(pAction->data().toInt());
}


// Select the current default preset (by name).
void qjackctlMainForm::activatePreset ( const QString& sPreset )
{
	activatePreset(m_pSetup->presets.indexOf(sPreset));
}


// Select the current default preset (by index).
void qjackctlMainForm::activatePreset ( int iPreset )
{
	if (m_pConnectionsForm && !m_pConnectionsForm->queryClose())
		return;

	if (iPreset >= 0 && iPreset < m_pSetup->presets.count())
		m_pSetup->sDefPreset = m_pSetup->presets[iPreset];
	else
		m_pSetup->sDefPreset = m_pSetup->sDefPresetName;

	showDirtySettingsWarning();
}


// Close main form slot.
void qjackctlMainForm::quitMainForm (void)
{
	// Flag that we're quitting explicitly.
	m_bQuitForce = true;

	// And then, do the closing dance.
	close();
}


// Context menu event handler.
void qjackctlMainForm::contextMenuEvent ( QContextMenuEvent *pEvent )
{
	// We'll just show up the usual system tray menu.
	systemTrayContextMenu(pEvent->globalPos());
}


void qjackctlMainForm::mousePressEvent(QMouseEvent *pMouseEvent)
{
	if (pMouseEvent->button() == Qt::MidButton &&
		m_ui.StatusDisplayFrame->geometry().contains(pMouseEvent->pos())) {
		resetXrunStats();
	}
}


#ifdef CONFIG_DBUS

// D-BUS: Set/reset parameter values from current selected preset options.
void qjackctlMainForm::setDBusParameters (void)
{
	// Set configuration parameters...
	bool bDummy     = (m_preset.sDriver == "dummy");
	bool bSun       = (m_preset.sDriver == "sun");
	bool bOss       = (m_preset.sDriver == "oss");
	bool bAlsa      = (m_preset.sDriver == "alsa");
	bool bPortaudio = (m_preset.sDriver == "portaudio");
	bool bCoreaudio = (m_preset.sDriver == "coreaudio");
	bool bFreebob   = (m_preset.sDriver == "freebob");
	bool bFirewire  = (m_preset.sDriver == "firewire");
	bool bNet       = (m_preset.sDriver == "net" || m_preset.sDriver == "netone");
	setDBusEngineParameter("name",
		m_pSetup->sServerName,
		!m_pSetup->sServerName.isEmpty());
	setDBusEngineParameter("verbose", m_preset.bVerbose);
	setDBusEngineParameter("realtime", m_preset.bRealtime);
	setDBusEngineParameter("realtime-priority",
		m_preset.iPriority,
		m_preset.bRealtime && m_preset.iPriority > 5);
//	setDBusEngineParameter("port-max",
//		m_preset.iPortMax,
//		m_preset.iPortMax > 0);
	setDBusEngineParameter("client-timeout",
		m_preset.iTimeout,
		m_preset.iTimeout > 0);
//	setDBusEngineParameter("no-memlock", m_preset.bNoMemLock);
//	setDBusEngineParameter("unlock-mem",
//		m_preset.bUnlockMem,
//		!m_preset.bNoMemLock);
	setDBusEngineParameter("driver", m_preset.sDriver);
	if ((bAlsa || bPortaudio) && (m_preset.iAudio != QJACKCTL_DUPLEX ||
		m_preset.sInDevice.isEmpty() || m_preset.sOutDevice.isEmpty())) {
		QString sInterface = m_preset.sInterface;
		if (bAlsa && sInterface.isEmpty())
			sInterface = "hw:0";
		setDBusDriverParameter("device", sInterface);
	}
	if (bPortaudio) {
		setDBusDriverParameter("channel",
			(unsigned int) m_preset.iChan,
			m_preset.iChan > 0);
	}
	if (bCoreaudio || bFreebob || bFirewire) {
		setDBusDriverParameter("device",
			m_preset.sInterface,
			!m_preset.sInterface.isEmpty());
	}
	if (!bNet) {
		setDBusDriverParameter("rate",
			(unsigned int) m_preset.iSampleRate,
			m_preset.iSampleRate > 0);
		setDBusDriverParameter("period",
			(unsigned int) m_preset.iFrames,
			m_preset.iFrames > 0);
	}
	if (bAlsa || bSun || bOss || bFreebob || bFirewire) {
		setDBusDriverParameter("nperiods",
			(unsigned int) m_preset.iPeriods,
			m_preset.iPeriods > 0);
	}
	if (bAlsa) {
		setDBusDriverParameter("softmode", m_preset.bSoftMode);
		setDBusDriverParameter("monitor", m_preset.bMonitor);
		setDBusDriverParameter("shorts", m_preset.bShorts);
		setDBusDriverParameter("hwmon", m_preset.bHWMon);
		setDBusDriverParameter("hwmeter", m_preset.bHWMeter);
	#ifdef CONFIG_JACK_MIDI
		setDBusDriverParameter("midi-driver",
			m_preset.sMidiDriver,
			!m_preset.sMidiDriver.isEmpty());
	#endif
	}
	if (bAlsa || bPortaudio) {
		QString sInterface = m_preset.sInterface;
		if (bAlsa && sInterface.isEmpty())
			sInterface = "hw:0";
		QString sInDevice = m_preset.sInDevice;
		if (sInDevice.isEmpty())
			sInDevice = sInterface;
		QString sOutDevice = m_preset.sOutDevice;
		if (sOutDevice.isEmpty())
			sOutDevice = sInterface;
		switch (m_preset.iAudio) {
		case QJACKCTL_DUPLEX:
			setDBusDriverParameter("duplex", true);
			setDBusDriverParameter("capture", sInDevice);
			setDBusDriverParameter("playback", sOutDevice);
			break;
		case QJACKCTL_CAPTURE:
			resetDBusDriverParameter("duplex");
			setDBusDriverParameter("capture", sInDevice);
			resetDBusDriverParameter("playback");
			break;
		case QJACKCTL_PLAYBACK:
			resetDBusDriverParameter("duplex");
			setDBusDriverParameter("playback", sOutDevice);
			resetDBusDriverParameter("capture");
			break;
		}
		setDBusDriverParameter("inchannels",
			(unsigned int) m_preset.iInChannels,
			m_preset.iInChannels > 0 && m_preset.iAudio != QJACKCTL_PLAYBACK);
		setDBusDriverParameter("outchannels",
			(unsigned int) m_preset.iOutChannels,
			m_preset.iOutChannels > 0 && m_preset.iAudio != QJACKCTL_CAPTURE);
		unsigned char dither = 0;
		switch (m_preset.iDither) {
		case 0: dither = 'n'; break;
		case 1: dither = 'r'; break;
		case 2: dither = 's'; break;
		case 3: dither = 't'; break; }
		setDBusDriverParameter("dither",
			QVariant::fromValue(dither),
			dither > 0);
	}
	else if (bOss || bSun) {
		setDBusDriverParameter("ignorehw", m_preset.bIgnoreHW);
		setDBusDriverParameter("wordlength",
			(unsigned int) m_preset.iWordLength,
			m_preset.iWordLength > 0);
		QString sInDevice = m_preset.sInDevice;
		if (sInDevice.isEmpty() && m_preset.iAudio == QJACKCTL_CAPTURE)
			sInDevice = m_preset.sInterface;
		setDBusDriverParameter("capture",
			sInDevice,
			!sInDevice.isEmpty() && m_preset.iAudio != QJACKCTL_PLAYBACK);
		QString sOutDevice = m_preset.sOutDevice;
		if (sOutDevice.isEmpty() && m_preset.iAudio == QJACKCTL_PLAYBACK)
			sOutDevice = m_preset.sInterface;
		setDBusDriverParameter("playback",
			sOutDevice,
			!sOutDevice.isEmpty() && m_preset.iAudio != QJACKCTL_CAPTURE);
		setDBusDriverParameter("inchannels",
			(unsigned int) m_preset.iInChannels,
			m_preset.iInChannels > 0  && m_preset.iAudio != QJACKCTL_PLAYBACK);
		setDBusDriverParameter("outchannels",
			(unsigned int) m_preset.iOutChannels,
			m_preset.iOutChannels > 0 && m_preset.iAudio != QJACKCTL_CAPTURE);
	}
	else if (bCoreaudio || bFirewire || bNet) {
		setDBusDriverParameter("inchannels",
			(unsigned int) m_preset.iInChannels,
			m_preset.iInChannels > 0  && m_preset.iAudio != QJACKCTL_PLAYBACK);
		setDBusDriverParameter("outchannels",
			(unsigned int) m_preset.iOutChannels,
			m_preset.iOutChannels > 0 && m_preset.iAudio != QJACKCTL_CAPTURE);
	}
	else if (bFreebob) {
		setDBusDriverParameter("duplex",
			bool(m_preset.iAudio == QJACKCTL_DUPLEX));
		resetDBusDriverParameter("capture");
		resetDBusDriverParameter("playback");
		resetDBusDriverParameter("outchannels");
		resetDBusDriverParameter("inchannels");
	}
	if (bDummy) {
		setDBusDriverParameter("wait",
			(unsigned int) m_preset.iWait,
			m_preset.iWait > 0);
	}
	else
	if (!bNet) {
		setDBusDriverParameter("input-latency",
			(unsigned int) m_preset.iInLatency,
			m_preset.iInLatency > 0);
		setDBusDriverParameter("output-latency",
			(unsigned int) m_preset.iOutLatency,
			m_preset.iOutLatency > 0);
	}
}


// D-BUS: Set parameter values (with reset option).
bool qjackctlMainForm::setDBusEngineParameter (
	const QString& param, const QVariant& value, bool bSet )
{
	return setDBusParameter(QStringList() << "engine" << param, value, bSet);
}

bool qjackctlMainForm::setDBusDriverParameter (
	const QString& param, const QVariant& value, bool bSet )
{
	return setDBusParameter(QStringList() << "driver" << param, value, bSet);
}

bool qjackctlMainForm::setDBusParameter (
	const QStringList& path, const QVariant& value, bool bSet )
{
	if (m_pDBusConfig == NULL)
		return false;

	if (!bSet) return resetDBusParameter(path); // Reset option.

	QDBusMessage dbusm = m_pDBusConfig->call(
		"SetParameterValue", path, QVariant::fromValue(QDBusVariant(value)));

	if (dbusm.type() == QDBusMessage::ErrorMessage) {
		appendMessagesError(
			tr("D-BUS: SetParameterValue('%1', '%2'):\n\n"
			"%3.\n(%4)").arg(path.join(":")).arg(value.toString())
			.arg(dbusm.errorMessage())
			.arg(dbusm.errorName()));
		return false;
	}

	return true;
}


// D-BUS: Reset parameter (to default) values.
bool qjackctlMainForm::resetDBusEngineParameter ( const QString& param )
{
	return resetDBusParameter(QStringList() << "engine" << param);
}

bool qjackctlMainForm::resetDBusDriverParameter ( const QString& param )
{
	return resetDBusParameter(QStringList() << "driver" << param);
}

bool qjackctlMainForm::resetDBusParameter ( const QStringList& path )
{
	if (m_pDBusConfig == NULL)
		return false;

	QDBusMessage dbusm = m_pDBusConfig->call("ResetParameterValue", path);

	if (dbusm.type() == QDBusMessage::ErrorMessage) {
		appendMessagesError(
			tr("D-BUS: ResetParameterValue('%1'):\n\n"
			"%2.\n(%3)").arg(path.join(":"))
			.arg(dbusm.errorMessage())
			.arg(dbusm.errorName()));
		return false;
	}

	return true;
}


// D-BUS: Get parameter values.
QVariant qjackctlMainForm::getDBusEngineParameter ( const QString& param )
{
	return getDBusParameter(QStringList() << "engine" << param);
}

QVariant qjackctlMainForm::getDBusDriverParameter ( const QString& param )
{
	return getDBusParameter(QStringList() << "driver" << param);
}

QVariant qjackctlMainForm::getDBusParameter ( const QStringList& path )
{
	if (m_pDBusConfig == NULL)
		return QVariant();

	QDBusMessage dbusm = m_pDBusConfig->call("GetParameterValue", path);

	if (dbusm.type() == QDBusMessage::ErrorMessage) {
		appendMessagesError(
			tr("D-BUS: GetParameterValue('%1'):\n\n"
			"%2.\n(%3)").arg(path.join(":"))
			.arg(dbusm.errorMessage())
			.arg(dbusm.errorName()));
		return QVariant();
	}

	return dbusm.arguments().at(2);
}

#endif	// CONFIG_DBUS


// Quotes string with embedded whitespace.
QString qjackctlMainForm::formatQuoted ( const QString& s ) const
{
	const QChar b = ' ';
	const QChar q = '"';

	return (s.contains(b) && !s.contains(q) ? q + s + q : s);
}


// Guarded transport play/pause toggle.
void qjackctlMainForm::transportPlayStatus ( bool bOn )
{
	++m_iTransportPlay;

	m_ui.PlayToolButton->setChecked(bOn);

	--m_iTransportPlay;
}


// Session (desktop) shutdown signal handler.
void qjackctlMainForm::setQuitForce ( bool bQuitForce )
{
	m_bQuitForce = bQuitForce;
}

bool qjackctlMainForm::isQuitForce (void) const
{
	return m_bQuitForce;
}


// end of qjackctlMainForm.cpp
