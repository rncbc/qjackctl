// qjackctlMainForm.cpp
//
/****************************************************************************
   Copyright (C) 2003-2020, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include "qjackctlGraphForm.h"
#include "qjackctlSetupForm.h"
#include "qjackctlAboutForm.h"

#include "qjackctlJackGraph.h"
#ifdef CONFIG_ALSA_SEQ
#include "qjackctlAlsaGraph.h"
#endif

#ifdef CONFIG_SYSTEM_TRAY
#include "qjackctlSystemTray.h"
#endif

#include <QApplication>
#include <QSocketNotifier>
#include <QRegularExpression>
#include <QMessageBox>
#include <QTextStream>
#include <QMenu>
#include <QTimer>
#include <QLabel>
#include <QPixmap>
#include <QFileInfo>
#include <QDir>

#include <QContextMenuEvent>
#include <QCloseEvent>

#include <QElapsedTimer>

#if QT_VERSION < QT_VERSION_CHECK(4, 5, 0)
namespace Qt {
const WindowFlags WindowCloseButtonHint = WindowFlags(0x08000000);
}
#endif

// Deprecated QTextStreamFunctions/Qt namespaces workaround.
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#define endl	Qt::endl
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

#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
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


// Custom event types.
#define QJACKCTL_PORT_EVENT     QEvent::Type(QEvent::User + 1)
#define QJACKCTL_XRUN_EVENT     QEvent::Type(QEvent::User + 2)
#define QJACKCTL_BUFF_EVENT     QEvent::Type(QEvent::User + 3)
#define QJACKCTL_FREE_EVENT     QEvent::Type(QEvent::User + 4)
#define QJACKCTL_SHUT_EVENT     QEvent::Type(QEvent::User + 5)
#define QJACKCTL_EXIT_EVENT     QEvent::Type(QEvent::User + 6)

#ifdef CONFIG_DBUS
#define QJACKCTL_LINE_EVENT     QEvent::Type(QEvent::User + 7)
#endif

#ifdef CONFIG_JACK_METADATA
#define QJACKCTL_PROP_EVENT     QEvent::Type(QEvent::User + 8)
#endif


// Time dashes format helper.^
static const char *c_szTimeDashes = "--:--:--.---";


//-------------------------------------------------------------------------
// UNIX Signal handling support stuff.

#ifdef HAVE_SIGNAL_H

#include <sys/types.h>
#include <sys/socket.h>

#include <signal.h>

// File descriptor for SIGTERM notifier.
static int g_fdSigterm[2] = { QJACKCTL_FDNIL, QJACKCTL_FDNIL };

// Unix SIGTERM signal handler.
static void qjackctl_sigterm_handler ( int /* signo */ )
{
	char c = 1;

	(::write(g_fdSigterm[0], &c, sizeof(c)) > 0);
}

#endif	// HAVE_SIGNAL_H


//----------------------------------------------------------------------------
// qjackctl -- Static callback posters.

// To have clue about current buffer size (in frames).
static jack_nframes_t g_buffsize = 0;

// Current freewheel running state.
static int g_freewheel = 0;

// Current server error state.
static QProcess::ProcessError g_error = QProcess::UnknownError;


// Jack client registration callback funtion, called
// whenever a jack client is registered or unregistered.
static void qjackctl_client_registration_callback (
	const char *, int, void * )
{
	QApplication::postEvent(
		qjackctlMainForm::getInstance(),
		new QEvent(QJACKCTL_PORT_EVENT));
}



// Jack port registration callback funtion, called
// whenever a jack port is registered or unregistered.
static void qjackctl_port_registration_callback (
	jack_port_id_t, int, void * )
{
	QApplication::postEvent(
		qjackctlMainForm::getInstance(),
		new QEvent(QJACKCTL_PORT_EVENT));
}


// Jack port (dis)connection callback funtion, called
// whenever a jack port is connected or disconnected.
static void qjackctl_port_connect_callback (
	jack_port_id_t, jack_port_id_t, int, void * )
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


#ifdef CONFIG_JACK_PORT_RENAME

// Jack port rename callback funtion, called
// whenever a jack port is renamed.
static void qjackctl_port_rename_callback (
	jack_port_id_t, const char *, const char *, void * )
{
	QApplication::postEvent(
		qjackctlMainForm::getInstance(),
		new QEvent(QJACKCTL_PORT_EVENT));
}

#endif


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
	g_buffsize = nframes;

	QApplication::postEvent(
		qjackctlMainForm::getInstance(),
		new QEvent(QJACKCTL_BUFF_EVENT));

	return 0;
}


// Jack freewheel callback function, called
// whenever the server enters/exits freewheel mode.
static void qjackctl_freewheel_callback ( int starting, void * )
{
	// Update our global static variable.
	g_freewheel = starting;

	QApplication::postEvent(
		qjackctlMainForm::getInstance(),
		new QEvent(QJACKCTL_FREE_EVENT));
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
			{ m_sLine.remove(QRegularExpression("\\x1B\\[[0-9|;]+m")); }
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
qjackctlMainForm *qjackctlMainForm::g_pMainForm = nullptr;

// Constructor.
qjackctlMainForm::qjackctlMainForm (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QWidget(pParent, wflags), m_menu(this)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	// Pseudo-singleton reference setup.
	g_pMainForm = this;

	m_pSetup = nullptr;

	m_iServerState = QJACKCTL_INACTIVE;

	m_pJack         = nullptr;
	m_pJackClient   = nullptr;
	m_bJackDetach   = false;
	m_bJackShutdown = false;
	m_bJackRestart  = false;

	m_pAlsaSeq      = nullptr;

#ifdef CONFIG_DBUS
	m_pDBusControl  = nullptr;
	m_pDBusConfig   = nullptr;
	m_pDBusLogWatcher = nullptr;
	m_bDBusStarted  = false;
	m_bDBusDetach   = false;
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

	m_pStdoutNotifier = nullptr;
	m_pAlsaNotifier = nullptr;

	// All forms are to be created later on setup.
	m_pMessagesStatusForm = nullptr;
	m_pSessionForm     = nullptr;
	m_pConnectionsForm = nullptr;
	m_pPatchbayForm    = nullptr;
	m_pGraphForm       = nullptr;
	m_pSetupForm       = nullptr;

	// Patchbay rack can be readily created.
	m_pPatchbayRack = new qjackctlPatchbayRack();

#ifdef CONFIG_SYSTEM_TRAY
	// The eventual system tray widget.
	m_pSystemTray  = nullptr;
	m_bQuitClose = false;
#endif

	// We're not quitting so early :)
	m_bQuitForce = false;

	// Transport skip accelerate factor.
	m_fSkipAccel = 1.0;

	// Avoid extra transport toggles (play/stop)
	m_iTransportPlay = 0;

	// Whether to update context menu on next status refresh.
	m_iMenuRefresh = 0;

	// Whether we've Qt::Tool flag (from bKeepOnTop),
	// this is actually the main last application window...
	QWidget::setAttribute(Qt::WA_QuitOnClose);

#ifdef HAVE_SIGNAL_H

	// Set to ignore any fatal "Broken pipe" signals.
	::signal(SIGPIPE, SIG_IGN);

	// Initialize file descriptors for SIGTERM socket notifier.
	::socketpair(AF_UNIX, SOCK_STREAM, 0, g_fdSigterm);
	m_pSigtermNotifier
		= new QSocketNotifier(g_fdSigterm[1], QSocketNotifier::Read, this);

	QObject::connect(m_pSigtermNotifier,
		SIGNAL(activated(int)),
		SLOT(sigtermNotifySlot(int)));

	// Install SIGTERM signal handler.
	struct sigaction sigterm;
	sigterm.sa_handler = qjackctl_sigterm_handler;
	sigemptyset(&sigterm.sa_mask);
	sigterm.sa_flags = 0;
	sigterm.sa_flags |= SA_RESTART;
	::sigaction(SIGTERM, &sigterm, nullptr);
	::sigaction(SIGQUIT, &sigterm, nullptr);

	// Ignore SIGHUP/SIGINT signals.
	::signal(SIGHUP, SIG_IGN);
	::signal(SIGINT, SIG_IGN);

	// Also ignore SIGUSR1 (LADISH Level 1).
	::signal(SIGUSR1, SIG_IGN);

#else	// HAVE_SIGNAL_H

	m_pSigtermNotifier = nullptr;

#endif	// !HAVE_SIGNAL_H

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
	QObject::connect(m_ui.GraphToolButton,
		SIGNAL(clicked()),
		SLOT(toggleGraphForm()));
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
#ifdef HAVE_SIGNAL_H
	if (m_pSigtermNotifier)
		delete m_pSigtermNotifier;
#endif

	// Stop server, if not already...
#ifdef CONFIG_DBUS
	if (m_pSetup->bStopJack || !m_pSetup->bJackDBusEnabled)
		stopJackServer();
	if (m_pDBusLogWatcher)
		delete m_pDBusLogWatcher;
	if (m_pDBusConfig)
		delete m_pDBusConfig;
	if (m_pDBusControl)
		delete m_pDBusControl;
	m_pDBusControl = nullptr;
	m_pDBusConfig  = nullptr;
	m_pDBusLogWatcher = nullptr;
	m_bDBusStarted = false;
	m_bDBusDetach  = false;
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
	m_pAlsaNotifier = nullptr;
	m_pAlsaSeq = nullptr;

	// Finally drop any popup widgets around...
	if (m_pMessagesStatusForm)
		delete m_pMessagesStatusForm;
	if (m_pSessionForm)
		delete m_pSessionForm;
	if (m_pConnectionsForm)
		delete m_pConnectionsForm;
	if (m_pPatchbayForm)
		delete m_pPatchbayForm;
	if (m_pGraphForm)
		delete m_pGraphForm;
	if (m_pSetupForm)
		delete m_pSetupForm;

#ifdef CONFIG_SYSTEM_TRAY
	// Quit off system tray widget.
	if (m_pSystemTray)
		delete m_pSystemTray;
#endif

	// Patchbay rack is also dead.
	if (m_pPatchbayRack)
		delete m_pPatchbayRack;

	// Pseudo-singleton reference shut-down.
	g_pMainForm = nullptr;
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
	QWidget *pParent = nullptr;
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

	// Graph form should be a full-blown top-level window...
	m_pGraphForm = new qjackctlGraphForm(pParent, wflags);

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
	m_pGraphForm->setup(m_pSetup);
	m_pSetupForm->setup(m_pSetup);

	// Maybe time to load default preset aliases?
	m_pSetup->loadAliases();

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
	QObject::connect(
		m_pPatchbayRack, SIGNAL(cableConnected(const QString&, const QString&, unsigned int)),
		this, SLOT(cableConnectSlot(const QString&, const QString&, unsigned int)));

	// Try to restore old window positioning and appearence.
	m_pSetup->loadWidgetGeometry(this, true);

	// And for the whole widget gallore...
	m_pSetup->loadWidgetGeometry(m_pMessagesStatusForm);
	m_pSetup->loadWidgetGeometry(m_pSessionForm);
	m_pSetup->loadWidgetGeometry(m_pConnectionsForm);
	m_pSetup->loadWidgetGeometry(m_pPatchbayForm);
	m_pSetup->loadWidgetGeometry(m_pGraphForm);
//	m_pSetup->loadWidgetGeometry(m_pSetupForm);

	// Make it final show...
	m_ui.StatusDisplayFrame->show();

	// Set other defaults...
	updateDisplayEffect();
	updateTimeDisplayFonts();
	updateTimeDisplayToolTips();
	updateMessagesFont();
	updateMessagesLimit();
	updateConnectionsFont();
	updateConnectionsIconSize();
	updateJackClientPortAlias();
	updateJackClientPortMetadata();
//	updateActivePatchbay();
#ifdef CONFIG_SYSTEM_TRAY
	updateSystemTray();
#endif

	// Initial XRUN statistics reset.
	resetXrunStats();

	// Check if we can redirect our own stdout/stderr...
#if !defined(__WIN32__) && !defined(_WIN32) && !defined(WIN32)
	if (m_pSetup->bStdoutCapture && ::pipe(g_fdStdout) == 0) {
		::dup2(g_fdStdout[QJACKCTL_FDWRITE], STDOUT_FILENO);
		::dup2(g_fdStdout[QJACKCTL_FDWRITE], STDERR_FILENO);
		stdoutBlock(g_fdStdout[QJACKCTL_FDWRITE], false);
		m_pStdoutNotifier = new QSocketNotifier(
			g_fdStdout[QJACKCTL_FDREAD], QSocketNotifier::Read, this);
		QObject::connect(m_pStdoutNotifier,
			SIGNAL(activated(int)),
			SLOT(stdoutNotifySlot(int)));
	}
#endif
#ifdef CONFIG_ALSA_SEQ
	if (m_pSetup->bAlsaSeqEnabled) {
		// Start our ALSA sequencer interface.
		if (snd_seq_open(&m_pAlsaSeq, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0)
			m_pAlsaSeq = nullptr;
		if (m_pAlsaSeq) {
			snd_seq_port_subscribe_t *pAlsaSubs;
			snd_seq_addr_t seq_addr;
			struct pollfd pfd[1];
			const int iPort = snd_seq_create_simple_port(
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
			if (m_pGraphForm)
				m_pGraphForm->alsa_changed();
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
		dbus.connect(s, s, sDBusName, "graph",
			this, SLOT(toggleGraphForm()));
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
		dbus.connect(s, s, sDBusName, "active-patchbay",
			this, SLOT(activatePatchbay(const QString&)));
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
	}
	// Register JACK D-Bus service...
	updateJackDBus();
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


// Setup accessor.
qjackctlSetup *qjackctlMainForm::setup (void) const
{
	return m_pSetup;
}


// Window close event handlers.
bool qjackctlMainForm::queryClose (void)
{
	bool bQueryClose = true;

	if (m_pSetup == nullptr)
		return bQueryClose;

#ifdef CONFIG_SYSTEM_TRAY
	// If we're not quitting explicitly and there's an
	// active system tray icon, then just hide ourselves.
	if (!m_bQuitClose && !m_bQuitForce && isVisible()
		&& m_pSetup->bSystemTray && m_pSystemTray) {
		m_pSetup->saveWidgetGeometry(this, true);
		if (m_pSetup->bSystemTrayQueryClose) {
			const QString& sTitle
				= tr("Information");
			const QString& sText
				= tr("The program will keep running in the system tray.\n\n"
					"To terminate the program, please choose \"Quit\"\n"
					"in the context menu of the system tray icon.");
		#if 0//QJACKCTL_SYSTEM_TRAY_QUERY_CLOSE
			if (QSystemTrayIcon::supportsMessages()) {
				m_pSystemTray->showMessage(
					sTitle + " - " QJACKCTL_SUBTITLE1,
					sText, QSystemTrayIcon::Information);
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
		updateContextMenu();
		bQueryClose = false;
	}
#endif

	// Check if JACK daemon is currently running...
	if (bQueryClose && !m_bQuitForce)
		bQueryClose = queryCloseJack();

	// Try to save current aliases default settings.
	if (bQueryClose && !m_bQuitForce)
		bQueryClose = queryClosePreset();

	// Try to save current setup settings.
	if (bQueryClose  && !m_bQuitForce && m_pSetupForm)
		bQueryClose = m_pSetupForm->queryClose();

	// Try to save current patchbay default settings.
	if (bQueryClose && !m_bQuitForce && m_pPatchbayForm)
		bQueryClose = m_pPatchbayForm->queryClose();

	// Try to save current session directories list...
	if (bQueryClose && !m_bQuitForce && m_pSessionForm)
		bQueryClose = m_pSessionForm->queryClose();

	// Some windows default fonts are here on demand too.
	if (bQueryClose && m_pMessagesStatusForm) {
		m_pSetup->sMessagesFont = m_pMessagesStatusForm->messagesFont().toString();
		m_pSetup->iMessagesStatusTabPage = m_pMessagesStatusForm->tabPage();
	}

	// Whether we're really quitting.
#ifdef CONFIG_SYSTEM_TRAY
	m_bQuitClose = bQueryClose;
#endif
	m_bQuitForce = bQueryClose;

	// Try to save current positioning.
	if (bQueryClose) {
		m_pSetup->saveWidgetGeometry(m_pMessagesStatusForm);
		m_pSetup->saveWidgetGeometry(m_pSessionForm);
		m_pSetup->saveWidgetGeometry(m_pConnectionsForm);
		m_pSetup->saveWidgetGeometry(m_pPatchbayForm);
		m_pSetup->saveWidgetGeometry(m_pGraphForm);
	//	m_pSetup->saveWidgetGeometry(m_pSetupForm);
		m_pSetup->saveWidgetGeometry(this, true);
		// Close popup widgets.
		if (m_pMessagesStatusForm)
			m_pMessagesStatusForm->close();
		if (m_pSessionForm)
			m_pSessionForm->close();
		if (m_pConnectionsForm)
			m_pConnectionsForm->close();
		if (m_pPatchbayForm)
			m_pPatchbayForm->close();
		if (m_pGraphForm)
			m_pGraphForm->close();
		if (m_pSetupForm)
			m_pSetupForm->close();
	#if 0//CONFIG_SYSTEM_TRAY
		// And the system tray icon too.
		if (m_pSystemTray)
			m_pSystemTray->close();
	#endif
		// Stop any service out there...
		if (m_pSetup->bStopJack)
			stopJackServer();
		// Finally, save settings.
		m_pSetup->saveSetup();
	}

	return bQueryClose;
}


// Query whether to close/stop the JACK servervice.
bool qjackctlMainForm::queryCloseJack (void)
{
	bool bQueryClose = true;

	if (m_pJack && m_pJack->state() == QProcess::Running
		&& (m_pSetup->bQueryClose || m_pSetup->bQueryShutdown)) {
		show();
		raise();
		activateWindow();
		updateContextMenu();
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

	// Check if we're allowed to stop (shutdown)...
	if (bQueryClose && m_pSetup->bQueryShutdown
		&& m_pConnectionsForm
		&& (m_pConnectionsForm->isAudioConnected() ||
			m_pConnectionsForm->isMidiConnected())) {
		const QString& sTitle
			= tr("Warning");
		const QString& sText
			= tr("Some client audio applications\n"
				"are still active and connected.\n\n"
				"Do you want to stop the JACK audio server?");
		#if 0//QJACKCTL_QUERY_SHUTDOWN
			bQueryClose = (QMessageBox::warning(this, sTitle, sText,
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
			bQueryClose = (mbox.exec() == QMessageBox::Ok);
			if (bQueryClose && cbox.isChecked())
				m_pSetup->bQueryShutdown = false;
		#endif
	}

	return bQueryClose;
}


// Query whether current preset can be closed.
bool qjackctlMainForm::queryClosePreset (void)
{
	bool bQueryClose = true;

	if (m_pSetup->aliases.dirty) {
		switch (QMessageBox::warning(this,
			tr("Warning") + " - " QJACKCTL_SUBTITLE1,
			tr("The preset aliases have been changed:\n\n"
			"\"%1\"\n\nDo you want to save the changes?")
			.arg(m_pSetup->aliases.key),
			QMessageBox::Save |
			QMessageBox::Discard |
			QMessageBox::Cancel)) {
		case QMessageBox::Save:
			m_pSetup->saveAliases();
			// Fall thru....
		case QMessageBox::Discard:
			break;
		default:    // Cancel.
			bQueryClose = false;
			break;
		}
	}

	return bQueryClose;
}


void qjackctlMainForm::showEvent ( QShowEvent *pShowEvent )
{
	QWidget::showEvent(pShowEvent);
	updateContextMenu();
}


void qjackctlMainForm::hideEvent ( QHideEvent *pHideEvent )
{
	QWidget::hideEvent(pHideEvent);
	updateContextMenu();
}


void qjackctlMainForm::closeEvent ( QCloseEvent *pCloseEvent )
{
	// Let's be sure about that...
	if (queryClose()) {
		pCloseEvent->accept();
	#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		QApplication::exit(0);
	#else
		QApplication::quit();
	#endif
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
	case QJACKCTL_FREE_EVENT:
		freeNotifyEvent();
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
void qjackctlMainForm::shellExecute ( const QString& sShellCommand,
	const QString& sStartMessage, const QString& sStopMessage )
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
	appendMessagesColor(sTemp.trimmed(), Qt::darkMagenta);
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
	pal.setColor(QPalette::WindowText, Qt::yellow);
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

	// Take care for the environment as well...
	if (m_pSetup->sServerName.isEmpty()) {
		const char *pszServerName = ::getenv("JACK_DEFAULT_SERVER");
		if (pszServerName && strcmp("default", pszServerName))
			m_pSetup->sServerName = QString::fromUtf8(pszServerName);
	}

	// No JACK Classic command line to start with, yet...
	m_sJackCmdLine.clear();

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

#ifdef CONFIG_DBUS

	// Jack D-BUS server backend startup method...
	if (m_pDBusControl) {

		// Jack D-BUS server backend configuration...
		setDBusParameters(m_preset);

		QDBusMessage dbusm = m_pDBusControl->call("StartServer");
		if (dbusm.type() == QDBusMessage::ReplyMessage) {
			m_bDBusDetach = true;
			appendMessages(
				tr("D-BUS: JACK server is starting..."));
		} else {
			appendMessagesError(
				tr("D-BUS: JACK server could not be started.\n\nSorry"));
		}

		// Delay our control client...
		startJackClientDelay();

		// JACK D-BUS startup is done.
		return;
	}

#endif	// !CONFIG_DBUS

	// Split the server path into arguments...
	QStringList args = m_preset.sServerPrefix.split(' ');

	// Look for the executable in the search path;
	// this enforces the server command to be an
	// executable absolute path whenever possible.
	QString sCommand = args[0];
	QFileInfo fi(sCommand);
	if (fi.isRelative()) {
	#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
		const char chPathSep = ';';
		if (fi.suffix().isEmpty())
			sCommand += ".exe";
	#else
		const char chPathSep = ':';
	#endif
		const QString sPath = QString::fromUtf8(::getenv("PATH"));
		QStringList paths = sPath.split(chPathSep);
	#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
		paths = paths << "C:\\Program Files\\Jack" << "C:\\Program Files (x86)\\Jack";
	#endif
        #if defined(__APPLE__)
                paths = paths << "/usr/local/bin/";
        #endif
		QStringListIterator iter(paths);
		while (iter.hasNext()) {
			const QString& sDirectory = iter.next();
			fi.setFile(QDir(sDirectory), sCommand);
			if (fi.exists() && fi.isExecutable()) {
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
	if ((bCoreaudio || bFirewire) && !m_preset.sInterface.isEmpty())
		args.append("-d" + formatQuoted(m_preset.sInterface));
	if (m_preset.iSampleRate > 0 && !bNet)
		args.append("-r" + QString::number(m_preset.iSampleRate));
	if (m_preset.iFrames > 0 && !bNet)
		args.append("-p" + QString::number(m_preset.iFrames));
	if (bAlsa || bSun || bOss || bFirewire) {
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
	if (bDummy && m_preset.iWait > 0 && m_preset.iWait != 21333)
		args.append("-w" + QString::number(m_preset.iWait));
	if (bAlsa || bSun || bOss || bCoreaudio || bPortaudio || bFirewire) {
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

	// JACK Classic server backend startup process...
	m_pJack = new QProcess(this);

	// Setup stdout/stderr capture...
	if (m_pSetup->bStdoutCapture) {
	#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
		// QProcess::ForwardedChannels doesn't seem to work in windows.
		m_pJack->setProcessChannelMode(QProcess::MergedChannels);
	#else
		m_pJack->setProcessChannelMode(QProcess::ForwardedChannels);
	#endif
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
	appendMessagesColor(m_sJackCmdLine, Qt::darkMagenta);

#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
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

	// Go JACK, go...
	m_pJack->start(sCommand, cmd_args);
}


// Stop jack audio server...
void qjackctlMainForm::stopJack (void)
{
	// Stop the server conditionally...
	if (queryCloseJack())
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
			#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
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


void qjackctlMainForm::toggleJack (void)
{
	if (m_pJackClient)
		stopJack();
	else
		startJack();
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

	processStdoutBuffer();
}

void qjackctlMainForm::processStdoutBuffer (void)
{
	const int iLength = m_sStdoutBuffer.lastIndexOf('\n');
	if (iLength > 0) {
		QString sTemp = m_sStdoutBuffer.left(iLength);
		m_sStdoutBuffer.remove(0, iLength + 1);
		QStringList list = sTemp.split('\n');
		QStringListIterator iter(list);
		while (iter.hasNext()) {
			sTemp = iter.next();
			if (!sTemp.isEmpty())
			#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
				appendMessagesText(detectXrun(sTemp).trimmed());
			#else
				appendMessagesText(detectXrun(sTemp));
			#endif
		}
	}
}


// Stdout flusher -- show up any unfinished line...
void qjackctlMainForm::flushStdoutBuffer (void)
{
	if (!m_sStdoutBuffer.isEmpty()) {
		processStdoutBuffer();
		m_sStdoutBuffer.clear();
	}
}


// Jack audio server startup.
void qjackctlMainForm::jackStarted (void)
{
	// Sure we're over any previous shutdown...
	m_bJackShutdown = false;

	// We're still starting...
	updateServerState(QJACKCTL_STARTING);

	// Show startup results...
	if (m_pJack) {
		appendMessages(tr("JACK was started with PID=%1.")
			.arg(quint64(m_pJack->pid())));
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
		m_pJack = nullptr;
		// Flag we need a post-shutdown script...
		bPostShutdown = true;
	}

#ifdef CONFIG_DBUS
	// Special for D-BUS control...
	if (m_pDBusControl && m_bDBusStarted) {
		m_bDBusStarted = false;
		m_bDBusDetach  = false;
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
	pal.setColor(QPalette::WindowText,
		m_pJackClient == nullptr ? Qt::darkYellow : Qt::yellow);
	m_ui.ServerStateTextLabel->setPalette(pal);
	m_ui.StartToolButton->setEnabled(m_pJackClient == nullptr);
	m_ui.StopToolButton->setEnabled(m_pJackClient != nullptr);
	m_ui.RewindToolButton->setEnabled(false);
	m_ui.BackwardToolButton->setEnabled(false);
	m_ui.PlayToolButton->setEnabled(false);
	m_ui.PauseToolButton->setEnabled(false);
	m_ui.ForwardToolButton->setEnabled(false);
	transportPlayStatus(false);
#ifdef CONFIG_DBUS
	if (m_pDBusConfig && m_bDBusStarted)
		updateServerState(m_pJackClient ? QJACKCTL_STARTED : QJACKCTL_STOPPED);
	else
#endif
	if (m_bJackDetach)
		updateServerState(m_pJackClient ? QJACKCTL_ACTIVE : QJACKCTL_INACTIVE);
	else
		updateServerState(QJACKCTL_STOPPED);
}


// XRUN detection routine.
QString& qjackctlMainForm::detectXrun ( QString& s )
{
	if (m_iXrunSkips > 1)
		return s;
	QRegularExpression rx(m_pSetup->sXrunRegex);
	QRegularExpressionMatch match(rx.match(s));
	if (match.hasMatch()) {
		const int iPos = match.capturedStart(0);
		s.insert(iPos + match.capturedLength(0), "</font>");
		s.insert(iPos, "<font color=\"#cc0000\">");
	#ifndef CONFIG_JACK_XRUN_DELAY
		m_fXrunLast = 0.0f;
		updateXrunStats(match.captured(1).toFloat());
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


// SIGTERM signal handler...
void qjackctlMainForm::sigtermNotifySlot ( int /* fd */ )
{
#ifdef HAVE_SIGNAL_H

	char c;

	if (::read(g_fdSigterm[1], &c, sizeof(c)) > 0)
		quitMainForm();

#endif
}


// Set stdout/stderr blocking mode.
bool qjackctlMainForm::stdoutBlock ( int fd, bool bBlock ) const
{
#if !defined(__WIN32__) && !defined(_WIN32) && !defined(WIN32)
	const int iFlags = ::fcntl(fd, F_GETFL, 0);
	const bool bNonBlock = bool(iFlags & O_NONBLOCK);
	if (bBlock && bNonBlock)
		bBlock = (::fcntl(fd, F_SETFL, iFlags & ~O_NONBLOCK) == 0);
	else
	if (!bBlock && !bNonBlock)
		bBlock = (::fcntl(fd, F_SETFL, iFlags |  O_NONBLOCK) != 0);
#endif
	return bBlock;
}


// Own stdout/stderr socket notifier slot.
void qjackctlMainForm::stdoutNotifySlot ( int fd )
{
 #if !defined(__WIN32__) && !defined(_WIN32) && !defined(WIN32)
	// Set non-blocking reads, if not already...
	const bool bBlock = stdoutBlock(fd, false);
	// Read as much as is available...
	QString sTemp;
	char achBuffer[1024];
	const int cchBuffer = sizeof(achBuffer) - 1;
	int cchRead = ::read(fd, achBuffer, cchBuffer);
	while (cchRead > 0) {
		achBuffer[cchRead] = (char) 0;
		sTemp.append(achBuffer);
		cchRead = (bBlock ? 0 : ::read(fd, achBuffer, cchBuffer));
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

void qjackctlMainForm::appendMessagesColor (
	const QString& s, const QColor& rgb )
{
	if (m_pMessagesStatusForm)
		m_pMessagesStatusForm->appendMessagesColor(s, rgb);
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

	appendMessagesColor(s.simplified(), Qt::red);

	const QString& sTitle
		= tr("Error");
#ifdef CONFIG_SYSTEM_TRAY
	if (m_pSetup->bSystemTray && m_pSystemTray
		&& QSystemTrayIcon::supportsMessages())
		m_pSystemTray->showMessage(
			sTitle + " - " QJACKCTL_SUBTITLE1,
			s, QSystemTrayIcon::Critical);
	else
#endif
	QMessageBox::critical(this, sTitle, s, QMessageBox::Cancel);
}


// Force update of the messages font.
void qjackctlMainForm::updateMessagesFont (void)
{
	if (m_pSetup == nullptr)
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
	if (m_pSetup == nullptr)
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
	if (m_pSetup == nullptr)
		return;

	if (m_pMessagesStatusForm) {
		m_pMessagesStatusForm->setLogging(
			m_pSetup->bMessagesLog, m_pSetup->sMessagesLogPath);
	}
}


// Force update of the connections font.
void qjackctlMainForm::updateConnectionsFont (void)
{
	if (m_pSetup == nullptr)
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
	if (m_pSetup == nullptr)
		return;

	if (m_pConnectionsForm)
		m_pConnectionsForm->setConnectionsIconSize(m_pSetup->iConnectionsIconSize);
}


// Update of JACK client/port alias display mode.
void qjackctlMainForm::updateJackClientPortAlias (void)
{
	if (m_pSetup == nullptr)
		return;

	qjackctlJackClientList::setJackClientPortAlias(m_pSetup->iJackClientPortAlias);

	refreshJackConnections();
}


// Update of JACK client/port pretty-name (metadata) display mode.
void qjackctlMainForm::updateJackClientPortMetadata (void)
{
	if (m_pSetup == nullptr)
		return;

	qjackctlJackClientList::setJackClientPortMetadata(m_pSetup->bJackClientPortMetadata);

	refreshJackConnections();
}


// Update main display background effect.
void qjackctlMainForm::updateDisplayEffect (void)
{
	if (m_pSetup == nullptr)
		return;

	// Set the main background...
	QPalette pal;
	if (m_pSetup->bDisplayEffect) {
		QPixmap pm(":/images/displaybg1.png");
		pal.setBrush(QPalette::Window, QBrush(pm));
	} else {
		pal.setColor(QPalette::Window, Qt::black);
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
		m_ui.ConnectionsToolButton->setVisible(!m_pSetup->bGraphButton);
		m_ui.GraphToolButton->setVisible(m_pSetup->bGraphButton);
		m_ui.PatchbayToolButton->show();
	} else {
		m_ui.StartToolButton->hide();
		m_ui.StopToolButton->hide();
		m_ui.MessagesStatusToolButton->hide();
		m_ui.SessionToolButton->hide();
		m_ui.GraphToolButton->hide();
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
	m_ui.GraphToolButton->setToolButtonStyle(toolButtonStyle);
	m_ui.PatchbayToolButton->setToolButtonStyle(toolButtonStyle);
	m_ui.QuitToolButton->setToolButtonStyle(toolButtonStyle);
	m_ui.SetupToolButton->setToolButtonStyle(toolButtonStyle);
	m_ui.AboutToolButton->setToolButtonStyle(toolButtonStyle);

	adjustSize();
}


#ifdef CONFIG_DBUS

void qjackctlMainForm::updateJackDBus (void)
{
	// Unregister JACK D-Bus service controller...
	if (m_pDBusLogWatcher) {
		delete m_pDBusLogWatcher;
		m_pDBusLogWatcher = nullptr;
	}
	if (m_pDBusConfig) {
		delete m_pDBusConfig;
		m_pDBusConfig = nullptr;
	}
	if (m_pDBusControl) {
		delete m_pDBusControl;
		m_pDBusControl = nullptr;
	}

	// Register JACK D-Bus service...
	if (m_pSetup->bJackDBusEnabled) {
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
			m_pDBusControl = nullptr;
		}
	}
}

#endif


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
	if (m_pSetup == nullptr)
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
	}	// We're sure there's no active patchbay...
	else appendMessages(tr("Patchbay deactivated."));

	// Should refresh anyway.
	m_iPatchbayRefresh++;
}

// Toggle active patchbay setting.
void qjackctlMainForm::setActivePatchbay ( const QString& sPatchbayPath )
{
	if (m_pSetup == nullptr)
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
	m_ui.GraphToolButton->setChecked(
		m_pGraphForm && m_pGraphForm->isVisible());
	m_ui.ConnectionsToolButton->setChecked(
		m_pConnectionsForm && m_pConnectionsForm->isVisible());
	m_ui.PatchbayToolButton->setChecked(
		m_pPatchbayForm && m_pPatchbayForm->isVisible());
	m_ui.SetupToolButton->setChecked(
		m_pSetupForm && m_pSetupForm->isVisible());
}


void qjackctlMainForm::stabilizeFormEx (void)
{
	updateContextMenu();

	stabilizeForm();
}


// Stabilize current business over the application event loop.
void qjackctlMainForm::stabilize ( int msecs )
{
	QElapsedTimer timer;
	timer.start();
	while (timer.elapsed() < msecs)
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}


// Reset XRUN cache items.
void qjackctlMainForm::resetXrunStats (void)
{
	m_timeResetLast = QTime::currentTime();
	m_timerResetLast.start();

	m_iXrunStats = 0;
	m_iXrunCount = 0;
	m_fXrunTotal = 0.0f;
	m_fXrunMin   = 0.0f;
	m_fXrunMax   = 0.0f;
	m_fXrunLast  = 0.0f;

	m_timeXrunLast.setHMS(0, 0, 0);
	m_timerXrunLast.start();

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
	#ifdef CONFIG_SYSTEM_TRAY
		// Change the system tray icon background color!
		if (m_pSystemTray)
			m_pSystemTray->setBackground(color);
	}   // Reset the system tray icon background!
	else if (m_pSystemTray)
		m_pSystemTray->setBackground(Qt::transparent);
	#else
	}
	#endif

	QPalette pal;
	pal.setColor(QPalette::WindowText, color);
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

	// Raw milliseconds
#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
	QString sTemp;
	sTemp.sprintf("%02u:%02u:%02u.%03u",
		hh, mm, ss, (unsigned int) (secs * 1000.0f));
	return sTemp;
#else
	return QString::asprintf("%02u:%02u:%02u.%03u",
		hh, mm, ss, (unsigned int) (secs * 1000.0f));
#endif
}


// Update the XRUN last/elapsed time item.
QString qjackctlMainForm::formatElapsedTime (
	int iStatusItem, const QTime& time, const QElapsedTimer& timer ) const
{
	QString sTemp = c_szTimeDashes;
	QString sText;

	// Compute and format elapsed time.
	if (time.isNull() || time.msec() < 1) {
		sText = sTemp;
	} else {
		sText = time.toString();
		if (m_pJackClient) {
			const float secs = float(timer.elapsed()) / 1000.0f;
			if (secs > 0) {
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
	if (m_iTimerDelay < m_iStartDelay) {
		m_ui.TimeDisplayTextLabel->setText(formatTime(
			float(m_iStartDelay - m_iTimerDelay) / 1000.0f));
	} else {
		updateStatusItem(STATUS_RESET_TIME,
			formatElapsedTime(STATUS_RESET_TIME,
				m_timeResetLast, m_timerResetLast));
		updateStatusItem(STATUS_XRUN_TIME,
			formatElapsedTime(STATUS_XRUN_TIME,
				m_timeXrunLast, m_timerXrunLast));
	}
}


// Update the XRUN list view items.
void qjackctlMainForm::refreshXrunStats (void)
{
	updateXrunCount();

	if (m_fXrunTotal < 0.001f) {
		const QString n = "--";
		updateStatusItem(STATUS_XRUN_TOTAL, n);
		updateStatusItem(STATUS_XRUN_MIN, n);
		updateStatusItem(STATUS_XRUN_MAX, n);
		updateStatusItem(STATUS_XRUN_AVG, n);
		updateStatusItem(STATUS_XRUN_LAST, n);
	} else {
		float fXrunAverage = 0.0f;
		if (m_iXrunCount > 0)
			fXrunAverage = (m_fXrunTotal / m_iXrunCount);
		const QString s = " " + tr("msec");
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
	if (m_timerXrunLast.restart() < 1000)
		return;

	// Mark this one...
	m_timeXrunLast = QTime::currentTime();

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
		.arg((int) g_buffsize), "#996633");
}


// Jack shutdown event notifier.
void qjackctlMainForm::shutNotifyEvent (void)
{
	// Log this event.
	appendMessagesColor(tr("Shutdown notification."), "#cc6666");
	// SHUTDOWN: JACK client handle might not be valid anymore...
	m_bJackShutdown = true;
	// m_pJackClient = nullptr;
	// Do what has to be done.
	stopJackServer();
}


// Jack freewheel event notifier.
void qjackctlMainForm::freeNotifyEvent (void)
{
	// Log this event.
	appendMessagesColor(g_freewheel
		? tr("Freewheel started...")
		: tr("Freewheel exited."), "#996633");
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
	do {
		snd_seq_event_t *pAlsaEvent;
		snd_seq_event_input(m_pAlsaSeq, &pAlsaEvent);
		snd_seq_free_event(pAlsaEvent);
	}
	while (snd_seq_event_input_pending(m_pAlsaSeq, 0) > 0);
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
	// Is it about to restart?
	if (m_bJackRestart && m_pJack == nullptr) {
		m_bJackRestart = false;
		startJack();
	}

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
	if (m_pConnectionsForm && !g_freewheel) {
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
		if (m_iJackRefresh > 0 && m_pJackClient != nullptr) {
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
		if (m_iAlsaRefresh > 0 && m_pAlsaSeq != nullptr) {
			m_iAlsaRefresh = 0;
			m_pConnectionsForm->refreshAlsa(true);
		}
	}

	// Is the patchbay dirty enough?
	if (m_pPatchbayForm && m_iPatchbayRefresh > 0) {
		m_iPatchbayRefresh = 0;
		m_pPatchbayForm->refreshForm();
	}

	// Is the graph dirty enough?
	if (m_pGraphForm)
		m_pGraphForm->refresh();

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
	queryDisconnect(
		pOPort->clientName(), pOPort->portName(),
		pIPort->clientName(), pIPort->portName(), iSocketType);
}


void qjackctlMainForm::queryDisconnect (
	qjackctlGraphPort *port1, qjackctlGraphPort *port2 )
{
	qjackctlGraphNode *node1 = port1->portNode();
	qjackctlGraphNode *node2 = port2->portNode();

	if (node1 == nullptr || node2 == nullptr)
		return;

	int iSocketType = QJACKCTL_SOCKETTYPE_DEFAULT;
	if (qjackctlJackGraph::audioPortType() == port1->portType())
		iSocketType = QJACKCTL_SOCKETTYPE_JACK_AUDIO;
	else
	if (qjackctlJackGraph::midiPortType() == port1->portType())
		iSocketType = QJACKCTL_SOCKETTYPE_JACK_MIDI;
#ifdef CONFIG_ALSA_SEQ
	else
	if (qjackctlAlsaGraph::midiPortType() == port1->portType())
		iSocketType = QJACKCTL_SOCKETTYPE_ALSA_MIDI;
#endif

	queryDisconnect(
		node1->nodeName(), port1->portName(),
		node2->nodeName(), port2->portName(), iSocketType);
}


void qjackctlMainForm::queryDisconnect (
	const QString& sOClientName, const QString& sOPortName,
	const QString& sIClientName, const QString& sIPortName,
	int iSocketType )
{
	if (m_pSetup->bActivePatchbay && m_pSetup->bQueryDisconnect) {
		qjackctlPatchbayCable *pCable = m_pPatchbayRack->findCable(
			sOClientName, sOPortName, sIClientName, sIPortName, iSocketType);
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
	if (m_pSetup == nullptr)
		return false;

	// Time to (re)load current preset aliases?
	if (!m_pSetup->loadAliases())
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

	if (m_pJackClient == nullptr) {
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
	jack_set_client_registration_callback(m_pJackClient,
		qjackctl_client_registration_callback, this);
	jack_set_port_registration_callback(m_pJackClient,
		qjackctl_port_registration_callback, this);
	jack_set_port_connect_callback(m_pJackClient,
		qjackctl_port_connect_callback, this);
#ifdef CONFIG_JACK_PORT_RENAME
	jack_set_port_rename_callback(m_pJackClient,
		qjackctl_port_rename_callback, this);
#endif
	jack_set_xrun_callback(m_pJackClient,
		qjackctl_xrun_callback, this);
	jack_set_buffer_size_callback(m_pJackClient,
		qjackctl_buffer_size_callback, this);
	jack_set_freewheel_callback(m_pJackClient,
		qjackctl_freewheel_callback, this);
	jack_on_shutdown(m_pJackClient,
		qjackctl_on_shutdown, this);
#ifdef CONFIG_JACK_METADATA
	jack_set_property_change_callback(m_pJackClient,
		qjackctl_property_change_callback, this);
#endif

	// First knowledge about buffer size.
	g_buffsize = jack_get_buffer_size(m_pJackClient);

	// Sure we're not freewheeling at this point...
	g_freewheel = 0;

	// Reconstruct our connections and session...
	if (m_pConnectionsForm) {
		m_pConnectionsForm->stabilizeAudio(true);
		m_pConnectionsForm->stabilizeMidi(true);
	}
	if (m_pSessionForm)
		m_pSessionForm->stabilizeForm(true);

#ifdef CONFIG_DBUS
	// Current D-BUS configuration makes it the default preset always...
	if (m_pDBusConfig && !m_bDBusDetach) {
		const QString& sPreset = m_pSetup->sDefPresetName;
		if (m_pSetup->loadPreset(m_preset, sPreset)
			&& getDBusParameters(m_preset)) {
			m_pSetup->sDefPreset = sPreset;
			// Have current preset changed anyhow?
			if (m_pSetupForm)
				m_pSetupForm->updateCurrentPreset(m_preset);
		}
	}
#endif

	// Save server configuration file.
	if (m_pSetup->bServerConfig && !m_sJackCmdLine.isEmpty()) {
		const QString sFilename
			= QString::fromUtf8(::getenv("HOME"))
			+ '/' + m_pSetup->sServerConfigName;
		QFile file(sFilename);
		if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			QTextStream(&file) << m_sJackCmdLine << endl;
			file.close();
			appendMessagesColor(
				tr("Server configuration saved to \"%1\".")
				.arg(sFilename), "#999933");
		}
	}

	 // We'll flag that we've been detached!
	if (bDetach)
		m_bJackDetach = true;
	else // Do not forget to reset XRUN stats variables...
		resetXrunStats();

	// Activate us as a client...
	jack_activate(m_pJackClient);

	// All displays are highlighted from now on.
	QPalette pal;
	pal.setColor(QPalette::WindowText, Qt::yellow);
	m_ui.ServerStateTextLabel->setPalette(pal);
	m_ui.DspLoadTextLabel->setPalette(pal);
	m_ui.ServerModeTextLabel->setPalette(pal);
	pal.setColor(QPalette::WindowText, Qt::darkYellow);
	m_ui.SampleRateTextLabel->setPalette(pal);
	pal.setColor(QPalette::WindowText, Qt::green);
	m_ui.TimeDisplayTextLabel->setPalette(pal);
	m_ui.TransportStateTextLabel->setPalette(pal);
	m_ui.TransportBpmTextLabel->setPalette(pal);
	m_ui.TransportTimeTextLabel->setPalette(pal);

	// Whether we've started detached, just change active status.
	updateServerState(m_bJackDetach
	#ifdef CONFIG_DBUS
		&& !(m_pDBusConfig && m_bDBusStarted)
	#endif
		? QJACKCTL_ACTIVE : QJACKCTL_STARTED);
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
		m_pJackClient = nullptr;
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
	if (m_pGraphForm)
		m_pGraphForm->jack_shutdown();

	// Displays are dimmed again.
	QPalette pal;
	pal.setColor(QPalette::WindowText, Qt::darkYellow);
	m_ui.ServerModeTextLabel->setPalette(pal);
	m_ui.DspLoadTextLabel->setPalette(pal);
	m_ui.SampleRateTextLabel->setPalette(pal);
	pal.setColor(QPalette::WindowText, Qt::darkGreen);
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
	return (m_bJackShutdown ? nullptr : m_pJackClient);
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
	// Hack this as for a while...
	if (m_pGraphForm)
		m_pGraphForm->jack_changed();
#if 0
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
	// Hack this as for a while...
	if (m_pGraphForm)
		m_pGraphForm->alsa_changed();
#if 0
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
	if (m_pSetup == nullptr)
		return;

	m_pSetup->saveWidgetGeometry(this, true);

	if (isVisible() && !isMinimized()) {
	#ifdef CONFIG_SYSTEM_TRAY
		// Hide away from sight, totally...
		if (m_pSetup->bSystemTray && m_pSystemTray)
			hide();
		else
	#endif
		// Minimize (iconify) normally.
		showMinimized();
	} else {
		// Show normally.
		showNormal();
		raise();
		activateWindow();
	}

//	updateContextMenu();
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

	updateContextMenu();
}

void qjackctlMainForm::toggleMessagesForm (void)
{
	if (m_pMessagesStatusForm) {
		const int iTabPage = m_pMessagesStatusForm->tabPage();
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
		const int iTabPage = m_pMessagesStatusForm->tabPage();
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
		m_pSessionForm->stabilizeForm(m_pJackClient != nullptr);
		if (m_pSessionForm->isVisible()) {
			m_pSessionForm->hide();
		} else {
			m_pSessionForm->show();
			m_pSessionForm->raise();
			m_pSessionForm->activateWindow();
		}
	}

	updateContextMenu();
}


// Connections form requester slot.
void qjackctlMainForm::toggleConnectionsForm (void)
{
	if (m_pConnectionsForm) {
		m_pSetup->saveWidgetGeometry(m_pConnectionsForm);
		m_pConnectionsForm->stabilizeAudio(m_pJackClient != nullptr);
		m_pConnectionsForm->stabilizeMidi(m_pJackClient != nullptr);
		m_pConnectionsForm->stabilizeAlsa(m_pAlsaSeq != nullptr);
		if (m_pConnectionsForm->isVisible()) {
			m_pConnectionsForm->hide();
		} else {
			m_pConnectionsForm->show();
			m_pConnectionsForm->raise();
			m_pConnectionsForm->activateWindow();
		}
	}

	updateContextMenu();
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

	updateContextMenu();
}


// Graph form requester slot.
void qjackctlMainForm::toggleGraphForm (void)
{
	if (m_pGraphForm) {
		m_pSetup->saveWidgetGeometry(m_pGraphForm);
		if (m_pGraphForm->isVisible()) {
			m_pGraphForm->hide();
		} else {
			m_pGraphForm->show();
			m_pGraphForm->raise();
			m_pGraphForm->activateWindow();
		}
	}

	updateContextMenu();
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

	updateContextMenu();
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
		++m_iMenuRefresh;
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
		++m_iMenuRefresh;
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
		++m_iMenuRefresh;
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
		++m_iMenuRefresh;
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
		++m_iMenuRefresh;
	}
#endif
}


// Almost-complete running status refresher.
void qjackctlMainForm::refreshStatus (void)
{
	const QString n = "--";
	const QString b = "-.-.---";
	const QString sStopped = tr("Stopped");

	++m_iStatusRefresh;

	if (m_pJackClient) {
		const QString s = " ";
	#ifdef CONFIG_JACK_TRANSPORT
		QString sText = n;
		jack_position_t tpos;
		jack_transport_state_t tstate = jack_transport_query(m_pJackClient, &tpos);
		const bool bPlaying
			= (tstate == JackTransportRolling || tstate == JackTransportLooping);
		// Transport timecode position.
	//	if (bPlaying)
			updateStatusItem(STATUS_TRANSPORT_TIME,
				formatTime(float(tpos.frame) / float(tpos.frame_rate)));
	//	else
	//		updateStatusItem(STATUS_TRANSPORT_TIME, c_szTimeDashes);
		// Transport barcode position (bar:beat.tick)
		if (tpos.valid & JackPositionBBT) {
		#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
			updateStatusItem(STATUS_TRANSPORT_BBT,
				QString().sprintf("%u.%u.%03u", tpos.bar, tpos.beat, tpos.tick));
		#else
			updateStatusItem(STATUS_TRANSPORT_BBT,
				QString::asprintf("%u.%u.%03u", tpos.bar, tpos.beat, tpos.tick));
		#endif
			updateStatusItem(STATUS_TRANSPORT_BPM,
				QString::number(tpos.beats_per_minute, 'g', 4));
		} else {
			updateStatusItem(STATUS_TRANSPORT_BBT, b);
			updateStatusItem(STATUS_TRANSPORT_BPM, n);
		}
	#endif // !CONFIG_JACK_TRANSPORT
		// Less frequent status items update...
		if (m_iStatusRefresh >= QJACKCTL_STATUS_CYCLE) {
			m_iStatusRefresh = 0;
			const float fDspLoad = jack_cpu_load(m_pJackClient);
			const char f = (fDspLoad > 0.1f ? 'f' : 'g'); // format
			const int  p = (fDspLoad > 1.0f ?  1  :  2 ); // precision
		#ifdef CONFIG_SYSTEM_TRAY
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
		#endif // !CONFIG_SYSTEM_TRAY
			updateStatusItem(STATUS_DSP_LOAD,
				tr("%1 %").arg(fDspLoad, 0, f, p));
			updateStatusItem(STATUS_SAMPLE_RATE,
				tr("%1 Hz").arg(jack_get_sample_rate(m_pJackClient)));
			updateStatusItem(STATUS_BUFFER_SIZE,
				tr("%1 frames").arg(g_buffsize));
			// Blink server mode indicator?...
			if (m_pSetup && m_pSetup->bDisplayBlink) {
				QPalette pal;
				pal.setColor(QPalette::WindowText,
					(++m_iStatusBlink % 2) ? Qt::darkYellow: Qt::yellow);
				m_ui.ServerModeTextLabel->setPalette(pal);
			}
		#ifdef CONFIG_JACK_REALTIME
			const bool bRealtime = jack_is_realtime(m_pJackClient);
			updateStatusItem(STATUS_REALTIME,
				(bRealtime ? tr("Yes") : tr("No")));
		#else
			updateStatusItem(STATUS_REALTIME, n);
		#endif // !CONFIG_JACK_REALTIME
			if (g_freewheel)
				m_ui.ServerModeTextLabel->setText(tr("FW"));
			else
		#ifdef CONFIG_JACK_REALTIME
			m_ui.ServerModeTextLabel->setText(bRealtime ? tr("RT") : n);
		#else
			m_ui.ServerModeTextLabel->setText(n);
		#endif // !CONFIG_JACK_REALTIME
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
			updateStatusItem(STATUS_TRANSPORT_TIME, c_szTimeDashes);
			updateStatusItem(STATUS_TRANSPORT_BBT, b);
			updateStatusItem(STATUS_TRANSPORT_BPM, n);
		#endif // !CONFIG_JACK_TRANSPORT
		#ifdef CONFIG_JACK_MAX_DELAY
			updateStatusItem(STATUS_MAX_DELAY, tr("%1 msec")
				.arg(0.001f * jack_get_max_delayed_usecs(m_pJackClient)));
		#endif // !CONFIG_JACK_MAX_DELAY
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
	#ifdef CONFIG_SYSTEM_TRAY
		// XRUN: blink the system-tray icon backgroung...
		if (m_pSystemTray && m_iXrunCallbacks > 0
			&& m_pSetup && m_pSetup->bDisplayBlink) {
			const int iElapsed = m_timerXrunLast.elapsed();
			if (iElapsed > 0x7ff) { // T=2048ms.
				QColor color(m_pSystemTray->background());
				color.setAlpha(0x0ff - ((iElapsed >> 3) & 0x0ff));
				m_pSystemTray->setBackground(color);
			}
		}
	#endif // !CONFIG_SYSTEM_TRAY
	} // No need to update often if we're just idle...
	else if (m_iStatusRefresh >= QJACKCTL_STATUS_CYCLE) {
		m_iStatusRefresh = 0;
		updateStatusItem(STATUS_DSP_LOAD, n);
		updateStatusItem(STATUS_SAMPLE_RATE, n);
		updateStatusItem(STATUS_BUFFER_SIZE, n);
		updateStatusItem(STATUS_REALTIME, n);
		m_ui.ServerModeTextLabel->setText(n);
		updateStatusItem(STATUS_TRANSPORT_STATE, n);
		updateStatusItem(STATUS_TRANSPORT_TIME, c_szTimeDashes);
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

	if (m_iMenuRefresh > 0) {
		m_iMenuRefresh = 0;
		updateContextMenu();
	}
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

#ifdef CONFIG_SYSTEM_TRAY
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
#endif

	sTitle = m_pSetup->sServerName;
	if (sTitle.isEmpty())
		sTitle = QString::fromUtf8(::getenv("JACK_DEFAULT_SERVER"));
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

	// Update context-menu for sure...
	updateContextMenu();
}


#ifdef CONFIG_SYSTEM_TRAY

// System tray master switcher.
void qjackctlMainForm::updateSystemTray (void)
{
	if (!QSystemTrayIcon::isSystemTrayAvailable())
		return;

	if (!m_pSetup->bSystemTray && m_pSystemTray) {
	//  Strange enough, this would close the application too.
	//  m_pSystemTray->close();
		delete m_pSystemTray;
		m_pSystemTray = nullptr;
	}

	if (m_pSetup->bSystemTray && m_pSystemTray == nullptr) {
		m_pSystemTray = new qjackctlSystemTray(this);
		m_pSystemTray->setContextMenu(&m_menu);
		QObject::connect(m_pSystemTray,
			SIGNAL(clicked()),
			SLOT(toggleMainForm()));
		QObject::connect(m_pSystemTray,
			SIGNAL(middleClicked()),
			SLOT(resetXrunStats()));
		QObject::connect(m_pSystemTray,
			SIGNAL(doubleClicked()),
			SLOT(toggleJack()));
		m_pSystemTray->show();
	} else {
		// Make sure the main widget is visible.
		show();
		raise();
		activateWindow();
	}

	updateContextMenu();
}

#endif


// Common context menu request slots.
void qjackctlMainForm::updateContextMenu (void)
{
	m_menu.clear();

	QAction *pAction;

	QString sHideMinimize = tr("Mi&nimize");
	QString sShowRestore  = tr("Rest&ore");
#ifdef CONFIG_SYSTEM_TRAY
	if (m_pSetup->bSystemTray && m_pSystemTray) {
		sHideMinimize = tr("&Hide");
		sShowRestore  = tr("S&how");
	}
#endif
	pAction = m_menu.addAction(isVisible() && !isMinimized()
		? sHideMinimize : sShowRestore, this, SLOT(toggleMainForm()));
	m_menu.addSeparator();

	if (m_pJackClient == nullptr) {
		pAction = m_menu.addAction(QIcon(":/images/start1.png"),
			tr("&Start"), this, SLOT(startJack()));
	} else {
		pAction = m_menu.addAction(QIcon(":/images/stop1.png"),
			tr("&Stop"), this, SLOT(stopJack()));
	}
	pAction = m_menu.addAction(QIcon(":/images/reset1.png"),
		tr("&Reset"), this, SLOT(resetXrunStats()));
//  pAction->setEnabled(m_pJackClient != nullptr);
	m_menu.addSeparator();

	// Construct the actual presets menu,
	// overriding the last one, if any...
	QMenu *pPresetsMenu = m_menu.addMenu(tr("&Presets"));
	// Assume QStringList iteration follows item index order (0,1,2...)
	int iPreset = 0;
	QStringListIterator iter(m_pSetup->presets);
	while (iter.hasNext()) {
		const QString& sPreset = iter.next();
		pAction = pPresetsMenu->addAction(sPreset);
		pAction->setCheckable(true);
		pAction->setChecked(sPreset == m_pSetup->sDefPreset);
		pAction->setData(iPreset);
		++iPreset;
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
	m_menu.addSeparator();

	if (m_pSessionForm) {
		const bool bEnabled = (m_pJackClient != nullptr);
		const QString sTitle = tr("S&ession");
		const QIcon iconSession(":/images/session1.png");
		QMenu *pSessionMenu = m_menu.addMenu(sTitle);
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
		pAction->setEnabled(m_pJackClient != nullptr && !pRecentMenu->isEmpty());
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

	pAction = m_menu.addAction(QIcon(":/images/messages1.png"),
		tr("&Messages"), this, SLOT(toggleMessagesForm()));
	pAction->setCheckable(true);
	pAction->setChecked(m_pMessagesStatusForm
		&& m_pMessagesStatusForm->isVisible()
		&& m_pMessagesStatusForm->tabPage() == qjackctlMessagesStatusForm::MessagesTab);
	pAction = m_menu.addAction(QIcon(":/images/status1.png"),
		tr("St&atus"), this, SLOT(toggleStatusForm()));
	pAction->setCheckable(true);
	pAction->setChecked(m_pMessagesStatusForm
		&& m_pMessagesStatusForm->isVisible()
		&& m_pMessagesStatusForm->tabPage() == qjackctlMessagesStatusForm::StatusTab);
	pAction = m_menu.addAction(QIcon(":/images/connections1.png"),
		tr("&Connections"), this, SLOT(toggleConnectionsForm()));
	pAction->setCheckable(true);
	pAction->setChecked(m_pConnectionsForm && m_pConnectionsForm->isVisible());
	pAction = m_menu.addAction(QIcon(":/images/patchbay1.png"),
		tr("Patch&bay"), this, SLOT(togglePatchbayForm()));
	pAction->setCheckable(true);
	pAction->setChecked(m_pPatchbayForm && m_pPatchbayForm->isVisible());
	pAction = m_menu.addAction(QIcon(":/images/graph1.png"),
		tr("&Graph"), this, SLOT(toggleGraphForm()));
	pAction->setCheckable(true);
	pAction->setChecked(m_pGraphForm && m_pGraphForm->isVisible());
	m_menu.addSeparator();

	QMenu *pTransportMenu = m_menu.addMenu(tr("&Transport"));
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
	m_menu.addSeparator();

	pAction = m_menu.addAction(QIcon(":/images/setup1.png"),
		tr("Set&up..."), this, SLOT(showSetupForm()));
	pAction->setCheckable(true);
	pAction->setChecked(m_pSetupForm && m_pSetupForm->isVisible());

	if (!m_pSetup->bRightButtons || !m_pSetup->bTransportButtons) {
		pAction = m_menu.addAction(QIcon(":/images/about1.png"),
			tr("Ab&out..."), this, SLOT(showAboutForm()));
	}
	m_menu.addSeparator();

	pAction = m_menu.addAction(QIcon(":/images/quit1.png"),
		tr("&Quit"), this, SLOT(quitMainForm()));
}


// Server settings change warning.
void qjackctlMainForm::showDirtySettingsWarning (void)
{
	// If client service is currently running,
	// prompt the effective warning...
	if (m_pJackClient) {
		bool bQueryRestart = m_pSetup->bQueryRestart;
		const QString& sTitle
			= tr("Warning");
		const QString& sText
			= tr("Server settings will be only effective after\n"
				"restarting the JACK audio server.");
		// Should ask the user whether to
		// restart the JACK audio server...
		if (m_pSetup->bQueryShutdown) {
			const QString& sQueryText = sText + "\n\n"
				+ tr("Do you want to restart the JACK audio server?");
		#if 0//QJACKCTL_QUERY_RESTART
			bQueryRestart = (QMessageBox::warning(this, sTitle, sQueryText,
				QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok);
		#else
			QMessageBox mbox(this);
			mbox.setIcon(QMessageBox::Warning);
			mbox.setWindowTitle(sTitle);
			mbox.setText(sQueryText);
			mbox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
			QCheckBox cbox(tr("Don't ask this again"));
			cbox.setChecked(false);
			cbox.blockSignals(true);
			mbox.addButton(&cbox, QMessageBox::ActionRole);
			bQueryRestart = (mbox.exec() == QMessageBox::Ok);
			if (cbox.isChecked()) {
				m_pSetup->bQueryRestart = bQueryRestart;
				m_pSetup->bQueryShutdown = false;
			}
		#endif
		}
		else
		// Show the old warning message...
		if (!bQueryRestart) {
		#ifdef CONFIG_SYSTEM_TRAY
			if (m_pSetup->bSystemTray && m_pSystemTray
				&& QSystemTrayIcon::supportsMessages()) {
				m_pSystemTray->showMessage(
					sTitle + " - " QJACKCTL_SUBTITLE1,
					sText, QSystemTrayIcon::Warning);
			}
			else
		#endif
			QMessageBox::warning(this, sTitle, sText);
		}
		// Or restart immediately!...
		if (bQueryRestart) {
			stopJackServer();
			m_bJackRestart = true;
		}
	}	// Otherwise, it will be just as convenient to update status...
	else updateTitleStatus();

	updateContextMenu();
}


// Setup otions change warning.
void qjackctlMainForm::showDirtySetupWarning (void)
{
	const QString& sTitle
		= tr("Information");
	const QString& sText
		= tr("Some settings will be only effective\n"
			"the next time you start this program.");
#ifdef CONFIG_SYSTEM_TRAY
	if (m_pSetup->bSystemTray && m_pSystemTray
		&& QSystemTrayIcon::supportsMessages()) {
		m_pSystemTray->showMessage(
			sTitle + " - " QJACKCTL_SUBTITLE1,
			sText, QSystemTrayIcon::Information);
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
	if (!queryClosePreset())
		return;

	if (iPreset >= 0 && iPreset < m_pSetup->presets.count())
		m_pSetup->sDefPreset = m_pSetup->presets.at(iPreset);
	else
		m_pSetup->sDefPreset = m_pSetup->sDefPresetName;

	// Have current preset changed anyhow?
	if (m_pSetupForm)
		m_pSetupForm->updateCurrentPreset();

	showDirtySettingsWarning();
}


// Select the current active patchbay profile (by path).
void qjackctlMainForm::activatePatchbay ( const QString& sPatchbayPath )
{
	if (QFileInfo(sPatchbayPath).exists() && !isActivePatchbay(sPatchbayPath)) {
		setActivePatchbay(sPatchbayPath);
		if (m_pPatchbayForm) {
			m_pPatchbayForm->loadPatchbayFile(sPatchbayPath);
			m_pPatchbayForm->updateRecentPatchbays();
			m_pPatchbayForm->stabilizeForm();
		}
	}
}


// Close main form slot.
void qjackctlMainForm::quitMainForm (void)
{
#ifdef CONFIG_SYSTEM_TRAY
	// Flag that we're quitting explicitly.
	m_bQuitClose = true;
#endif
	// And then, do the closing dance.
	close();
}


// Context menu event handler.
void qjackctlMainForm::contextMenuEvent ( QContextMenuEvent *pEvent )
{
	m_menu.exec(pEvent->globalPos());
}


void qjackctlMainForm::mousePressEvent(QMouseEvent *pMouseEvent)
{
	if (pMouseEvent->button() == Qt::MiddleButton &&
		m_ui.StatusDisplayFrame->geometry().contains(pMouseEvent->pos())) {
		resetXrunStats();
	}
}


#ifdef CONFIG_DBUS

// D-BUS: Set/reset parameter values from current selected preset options.
void qjackctlMainForm::setDBusParameters ( const qjackctlPreset& preset )
{
	if (m_pDBusConfig == nullptr)
		return;

	// Set configuration parameters...
	const bool bDummy     = (preset.sDriver == "dummy");
	const bool bSun       = (preset.sDriver == "sun");
	const bool bOss       = (preset.sDriver == "oss");
	const bool bAlsa      = (preset.sDriver == "alsa");
	const bool bPortaudio = (preset.sDriver == "portaudio");
	const bool bCoreaudio = (preset.sDriver == "coreaudio");
	const bool bFirewire  = (preset.sDriver == "firewire");
	const bool bNet       = (preset.sDriver == "net" || m_preset.sDriver == "netone");

//	setDBusEngineParameter("name",
//		m_pSetup->sServerName,
//		!m_pSetup->sServerName.isEmpty());
	setDBusEngineParameter("sync", preset.bSync);
	setDBusEngineParameter("verbose", preset.bVerbose);
	setDBusEngineParameter("realtime", preset.bRealtime);
	setDBusEngineParameter("realtime-priority",
		preset.iPriority,
		preset.bRealtime && preset.iPriority > 5);
	setDBusEngineParameter("port-max",
		(unsigned int) preset.iPortMax,
		preset.iPortMax > 0 && preset.iPortMax != 256);
	setDBusEngineParameter("client-timeout",
		preset.iTimeout,
		preset.iTimeout > 0 && preset.iTimeout != 500);
	setDBusEngineParameter("clock-source",
		uint(preset.uClockSource));
	setDBusEngineParameter("self-connect-mode",
		QVariant::fromValue<uchar> (preset.ucSelfConnectMode));
//	setDBusEngineParameter("no-mem-lock", preset.bNoMemLock);
//	setDBusEngineParameter("libs-unlock",
//		preset.bUnlockMem,
//		!preset.bNoMemLock);
	setDBusEngineParameter("driver", preset.sDriver);
	if ((bAlsa || bPortaudio) && (preset.iAudio != QJACKCTL_DUPLEX ||
		preset.sInDevice.isEmpty() || preset.sOutDevice.isEmpty())) {
		QString sInterface = preset.sInterface;
		if (bAlsa && sInterface.isEmpty())
			sInterface = "hw:0";
		setDBusDriverParameter("device", sInterface);
	}
	if (bPortaudio) {
		setDBusDriverParameter("channel",
			(unsigned int) preset.iChan,
			preset.iChan > 0);
	}
	if (bCoreaudio || bFirewire) {
		setDBusDriverParameter("device",
			preset.sInterface,
			!preset.sInterface.isEmpty());
	}
	if (!bNet) {
		setDBusDriverParameter("rate",
			(unsigned int) preset.iSampleRate,
			preset.iSampleRate > 0);
		setDBusDriverParameter("period",
			(unsigned int) preset.iFrames,
			preset.iFrames > 0);
	}
	if (bAlsa || bSun || bOss || bFirewire) {
		setDBusDriverParameter("nperiods",
			(unsigned int) preset.iPeriods,
			preset.iPeriods > 0);
	}
	if (bAlsa) {
		setDBusDriverParameter("softmode", preset.bSoftMode);
		setDBusDriverParameter("monitor", preset.bMonitor);
		setDBusDriverParameter("shorts", preset.bShorts);
		setDBusDriverParameter("hwmeter", preset.bHWMeter);
	#ifdef CONFIG_JACK_MIDI
		setDBusDriverParameter("midi-driver",
			preset.sMidiDriver,
			!preset.sMidiDriver.isEmpty());
	#endif
	}
	if (bAlsa || bPortaudio) {
		QString sInterface = preset.sInterface;
		if (bAlsa && sInterface.isEmpty())
			sInterface = "hw:0";
		QString sInDevice = preset.sInDevice;
		if (sInDevice.isEmpty())
			sInDevice = sInterface;
		QString sOutDevice = preset.sOutDevice;
		if (sOutDevice.isEmpty())
			sOutDevice = sInterface;
		switch (preset.iAudio) {
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
			(unsigned int) preset.iInChannels,
			preset.iInChannels > 0 && preset.iAudio != QJACKCTL_PLAYBACK);
		setDBusDriverParameter("outchannels",
			(unsigned int) preset.iOutChannels,
			preset.iOutChannels > 0 && preset.iAudio != QJACKCTL_CAPTURE);
		unsigned char dither = 0;
		switch (preset.iDither) {
		case 0: dither = 'n'; break;
		case 1: dither = 'r'; break;
		case 2: dither = 's'; break;
		case 3: dither = 't'; break; }
		setDBusDriverParameter("dither",
			QVariant::fromValue(dither),
			dither > 0);
	}
	else if (bOss || bSun) {
		QString sInDevice = preset.sInDevice;
		if (sInDevice.isEmpty() && preset.iAudio == QJACKCTL_CAPTURE)
			sInDevice = preset.sInterface;
		setDBusDriverParameter("capture",
			sInDevice,
			!sInDevice.isEmpty() && preset.iAudio != QJACKCTL_PLAYBACK);
		QString sOutDevice = preset.sOutDevice;
		if (sOutDevice.isEmpty() && preset.iAudio == QJACKCTL_PLAYBACK)
			sOutDevice = preset.sInterface;
		setDBusDriverParameter("playback",
			sOutDevice,
			!sOutDevice.isEmpty() && preset.iAudio != QJACKCTL_CAPTURE);
		setDBusDriverParameter("inchannels",
			(unsigned int) preset.iInChannels,
			preset.iInChannels > 0  && preset.iAudio != QJACKCTL_PLAYBACK);
		setDBusDriverParameter("outchannels",
			(unsigned int) preset.iOutChannels,
			preset.iOutChannels > 0 && preset.iAudio != QJACKCTL_CAPTURE);
	}
	else if (bCoreaudio || bFirewire || bNet) {
		setDBusDriverParameter("inchannels",
			(unsigned int) preset.iInChannels,
			preset.iInChannels > 0  && preset.iAudio != QJACKCTL_PLAYBACK);
		setDBusDriverParameter("outchannels",
			(unsigned int) preset.iOutChannels,
			preset.iOutChannels > 0 && preset.iAudio != QJACKCTL_CAPTURE);
	}
	if (bDummy) {
		setDBusDriverParameter("wait",
			(unsigned int) preset.iWait,
			preset.iWait > 0);
	}
	else
	if (!bNet) {
		setDBusDriverParameter("input-latency",
			(unsigned int) preset.iInLatency,
			preset.iInLatency > 0);
		setDBusDriverParameter("output-latency",
			(unsigned int) preset.iOutLatency,
			preset.iOutLatency > 0);
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
	if (m_pDBusConfig == nullptr)
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
	if (m_pDBusConfig == nullptr)
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


// D-BUS: Get preset options from current parameter values.
bool qjackctlMainForm::getDBusParameters ( qjackctlPreset& preset )
{
	if (m_pSetup == nullptr)
		return false;

	if (m_pDBusConfig == nullptr)
		return false;

	// Get configuration parameters...
	QVariant var;

//	m_pSetup->sServerName.clear();
//	var = getDBusEngineParameter("name");
//  if (var.isValid())
//		m_pSetup->sServerName = var.toString();

	preset.bSync = false;
	var = getDBusEngineParameter("sync");
	if (var.isValid())
		preset.bSync = var.toBool();

	preset.bVerbose = false;
	var = getDBusEngineParameter("verbose");
	if (var.isValid())
		preset.bVerbose = var.toBool();

	preset.bRealtime = true;
	var = getDBusEngineParameter("realtime");
	if (var.isValid())
		preset.bRealtime = var.toBool();

	preset.iPriority = 0;
	var = getDBusEngineParameter("realtime-priority");
	if (var.isValid())
		preset.iPriority = var.toInt();

	preset.iPortMax = 0;
	var = getDBusEngineParameter("port-max");
	if (var.isValid())
		preset.iPortMax = var.toInt();

	preset.iTimeout = 0;
	var = getDBusEngineParameter("client-timeout");
	if (var.isValid())
		preset.iTimeout = var.toInt();

	preset.bNoMemLock = false;
//	var = getDBusEngineParameter("no-mem-lock");
//	if (var.isValid())
//		preset.bNoMemLock = var.ToBool();

	preset.bUnlockMem = false;
//	var = getDBusEngineParameter("libs-unlock",
//		preset.bUnlockMem = var.toBool();

	preset.ucSelfConnectMode = ' ';
	var = getDBusEngineParameter("self-connect-mode");
	if (var.isValid())
		preset.uClockSource = var.toUInt();

	preset.ucSelfConnectMode = ' ';
	var = getDBusEngineParameter("self-connect-mode");
	if (var.isValid())
		preset.ucSelfConnectMode = var.value<uchar> ();

	preset.sDriver.clear();
	var = getDBusEngineParameter("driver");
	if (var.isValid())
		preset.sDriver = var.toString();

	const bool bDummy     = (preset.sDriver == "dummy");
	const bool bSun       = (preset.sDriver == "sun");
	const bool bOss       = (preset.sDriver == "oss");
	const bool bAlsa      = (preset.sDriver == "alsa");
	const bool bPortaudio = (preset.sDriver == "portaudio");
	const bool bCoreaudio = (preset.sDriver == "coreaudio");
	const bool bFirewire  = (preset.sDriver == "firewire");
	const bool bNet       = (preset.sDriver == "net" || preset.sDriver == "netone");

	preset.sInterface.clear();
	if (bAlsa || bPortaudio || bCoreaudio || bFirewire) {
		var = getDBusDriverParameter("device");
		if (var.isValid())
			preset.sInterface = var.toString();
	}

	preset.iChan = 0;
	if (bPortaudio) {
		var = getDBusDriverParameter("channel");
		if (var.isValid())
			preset.iChan = var.toInt();
	}

	preset.iSampleRate = 0;
	preset.iFrames = 0;
	if (!bNet) {
		var = getDBusDriverParameter("rate");
		if (var.isValid())
			preset.iSampleRate = var.toInt();
		var = getDBusDriverParameter("period");
		if (var.isValid())
			preset.iFrames = var.toInt();
	}

	preset.iPeriods = 0;
	if (bAlsa || bSun || bOss || bFirewire) {
		var = getDBusDriverParameter("nperiods");
		if (var.isValid())
			preset.iPeriods = var.toInt();
	}

	preset.bSoftMode = false;
	preset.bMonitor = false;
	preset.bShorts = false;
	preset.bHWMeter = false;
	preset.sMidiDriver.clear();
	if (bAlsa) {
		var = getDBusDriverParameter("softmode");
		if (var.isValid())
			preset.bSoftMode = var.toBool();
		var = getDBusDriverParameter("monitor");
		if (var.isValid())
			preset.bMonitor = var.toBool();
		var = getDBusDriverParameter("shorts");
		if (var.isValid())
			preset.bShorts = var.toBool();
		var = getDBusDriverParameter("hwmeter");
		if (var.isValid())
			preset.bHWMeter = var.toBool();
	#ifdef CONFIG_JACK_MIDI
		var = getDBusDriverParameter("midi-driver");
		if (var.isValid())
			preset.sMidiDriver = var.toString();
	#endif
	}

	preset.iAudio = QJACKCTL_DUPLEX;
	preset.sInDevice.clear();
	preset.sOutDevice.clear();
	if (bAlsa || bPortaudio || bOss || bSun) {
		bool bDuplex = false;
		var = getDBusDriverParameter("duplex");
		if (var.isValid())
			bDuplex = var.toBool();
		if (!bDuplex) {
			var = getDBusDriverParameter("capture");
			if (var.isValid())
				preset.sInDevice = var.toString();
			var = getDBusDriverParameter("playback");
			if (var.isValid())
				preset.sOutDevice = var.toString();
			if (!preset.sInDevice.isEmpty())
				preset.iAudio = QJACKCTL_CAPTURE;
			if (!preset.sOutDevice.isEmpty())
				preset.iAudio = QJACKCTL_PLAYBACK;
		}
	}

	preset.iInChannels = 0;
	preset.iOutChannels = 0;
	if (bAlsa || bPortaudio || bOss || bSun || bCoreaudio || bFirewire || bNet) {
		if (preset.iAudio != QJACKCTL_PLAYBACK) {
			var = getDBusDriverParameter("inchannels");
			if (var.isValid())
				preset.iInChannels = var.toInt();
		}
		if (preset.iAudio != QJACKCTL_CAPTURE) {
			var = getDBusDriverParameter("outchannels");
			if (var.isValid())
				preset.iOutChannels = var.toInt();
		}
	}

	preset.iDither = 0;
	if (bAlsa || bPortaudio) {
		unsigned char dither = 'n';
		var = getDBusDriverParameter("dither");
		if (var.isValid())
			dither = var.toChar().cell();
		switch (dither) {
		case 'n': preset.iDither = 0; break;
		case 'r': preset.iDither = 1; break;
		case 's': preset.iDither = 2; break;
		case 't': preset.iDither = 3; break; }
	}

	preset.iWait = 0;
	preset.iInLatency = 0;
	preset.iOutLatency = 0;
	if (bDummy) {
		var = getDBusDriverParameter("wait");
		if (var.isValid())
			preset.iWait = var.toInt();
	}
	else
	if (!bNet) {
		var = getDBusDriverParameter("input-latency");
		if (var.isValid())
			preset.iInLatency = var.toInt();
		var = getDBusDriverParameter("output-latency");
		if (var.isValid())
			preset.iOutLatency = var.toInt();
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
	if (m_pDBusConfig == nullptr)
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

	const QDBusVariant& dbusv
		= qvariant_cast<QDBusVariant> (dbusm.arguments().at(2));
	return dbusv.variant();
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


void qjackctlMainForm::commitData ( QSessionManager& sm )
{
	sm.release();

#ifdef CONFIG_SYSTEM_TRAY
	m_bQuitClose = true;
#endif
	m_bQuitForce = true;
}


// Some settings that are special someway...
bool qjackctlMainForm::resetBuffSize ( jack_nframes_t nframes )
{
	if (m_pJackClient == nullptr)
		return false;

	if (g_buffsize == nframes)
		return true;

	if (jack_set_buffer_size(m_pJackClient, nframes) != 0)
		return false;

	// May reset some stats...
	resetXrunStats();
	return true;
}


// end of qjackctlMainForm.cpp
