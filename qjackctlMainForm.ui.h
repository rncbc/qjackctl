/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/
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
#define QJACKCTL_TITLE		"JACK Audio Connection Kit"
#define QJACKCTL_SUBTITLE	"Qt GUI Interface"
#define QJACKCTL_VERSION	"0.0.3.4"
#define QJACKCTL_WEBSITE	"http://qjackctl.sourceforge.net"

#include <qapplication.h>
#include <qmessagebox.h>
#include <qvalidator.h>

// List view statistics item indexes
#define STATS_CPU_LOAD      0
#define STATS_SAMPLE_RATE   1
#define STATS_XRUN_AVG      2
#define STATS_XRUN_COUNT    3
#define STATS_XRUN_LAST     4
#define STATS_XRUN_MAX      5
#define STATS_XRUN_MIN      6
#define STATS_XRUN_TIME     7
#define STATS_XRUN_TOTAL    8


// Kind of constructor.
void qjackctlMainForm::init()
{
    m_pJack       = NULL;
    m_pJackClient = NULL;
    m_pTimer      = NULL;
    m_iTimerSlot  = 0;

    m_pPortNotifier = NULL;
    m_pXrunNotifier = NULL;
    m_pShutNotifier = NULL;

    m_iPortNotify = 0;
    m_iXrunNotify = 0;
    m_iShutNotify = 0;

    m_pJackPatchbay = NULL;

    // Create the list view items 'a priori'...
    QString s = " ";
    QString c = ":" + s;
    QString z = "0";
    QString n = tr("n/a");
    m_apStats[STATS_CPU_LOAD]    = new QListViewItem(StatsListView, s + tr("CPU Load") + c, n);
    m_apStats[STATS_SAMPLE_RATE] = new QListViewItem(StatsListView, s + tr("Sample Rate") + c, n);
    m_apStats[STATS_XRUN_AVG]    = new QListViewItem(StatsListView, s + tr("XRUN average") + c, z);
    m_apStats[STATS_XRUN_COUNT]  = new QListViewItem(StatsListView, s + tr("XRUN count since last server startup") + c, z);
    m_apStats[STATS_XRUN_LAST]   = new QListViewItem(StatsListView, s + tr("XRUN last") + c, z);
    m_apStats[STATS_XRUN_MAX]    = new QListViewItem(StatsListView, s + tr("XRUN maximum") + c, z);
    m_apStats[STATS_XRUN_MIN]    = new QListViewItem(StatsListView, s + tr("XRUN minimum") + c, z);
    m_apStats[STATS_XRUN_TIME]   = new QListViewItem(StatsListView, s + tr("XRUN last time detected") + c, n);
    m_apStats[STATS_XRUN_TOTAL]  = new QListViewItem(StatsListView, s + tr("XRUN total") + c, z);

    resetXrunStats();

    // Set dialog validators...
    PriorityComboBox->setValidator(new QIntValidator(PriorityComboBox));
    FramesComboBox->setValidator(new QIntValidator(FramesComboBox));
    SampleRateComboBox->setValidator(new QIntValidator(SampleRateComboBox));
    PeriodsComboBox->setValidator(new QIntValidator(PeriodsComboBox));
    TimeoutComboBox->setValidator(new QIntValidator(TimeoutComboBox));
    TimeRefreshComboBox->setValidator(new QIntValidator(TimeRefreshComboBox));

    // Stuff the about box...
    QString sText = "<p align=\"center\"><br />\n";
    sText += "<b>" QJACKCTL_TITLE " - " QJACKCTL_SUBTITLE "</b><br />\n";
    sText += "<br />\n";
    sText += tr("Version") + ": <b>" QJACKCTL_VERSION "</b><br />\n";
    sText += tr("Build") + ": " __DATE__ " " __TIME__ "<br />\n";
    sText += "<br />\n";
    sText += tr("Website") + ": <a href=\"" QJACKCTL_WEBSITE "\">" QJACKCTL_WEBSITE "</a><br />\n";
    sText += "</p>\n";
    AboutTextView->setText(sText);

    // Read saved settings and options...
    m_Settings.beginGroup("/qjackctl");

    m_Settings.beginGroup("/Settings");
    ServerComboBox->setCurrentText(m_Settings.readEntry("/Server", "jackstart"));
    RealtimeCheckBox->setChecked(m_Settings.readBoolEntry("/Realtime", true));
    SoftModeCheckBox->setChecked(m_Settings.readBoolEntry("/SoftMode", false));
    AsioCheckBox->setChecked(m_Settings.readBoolEntry("/Asio", false));
    MonitorCheckBox->setChecked(m_Settings.readBoolEntry("/Monitor", false));
    PriorityComboBox->setCurrentText(QString::number(m_Settings.readNumEntry("/Priority", 0)));
    FramesComboBox->setCurrentText(QString::number(m_Settings.readNumEntry("/Frames", 1024)));
    SampleRateComboBox->setCurrentText(QString::number(m_Settings.readNumEntry("/SampleRate", 48000)));
    PeriodsComboBox->setCurrentText(QString::number(m_Settings.readNumEntry("/Periods", 2)));
    InterfaceComboBox->setCurrentText(m_Settings.readEntry("/Interface", "default"));
    AudioComboBox->setCurrentItem(m_Settings.readNumEntry("/Audio", 0));
    DitherComboBox->setCurrentItem(m_Settings.readNumEntry("/Dither", 0));
    TimeoutComboBox->setCurrentText(QString::number(m_Settings.readNumEntry("/Timeout", 500)));
    HWMonCheckBox->setChecked(m_Settings.readBoolEntry("/HWMon", false));
    HWMeterCheckBox->setChecked(m_Settings.readBoolEntry("/HWMeter", false));
    TempDirComboBox->setCurrentText(m_Settings.readEntry("/TempDir", "(default)"));
    m_Settings.endGroup();

    m_Settings.beginGroup("/Options");
    DetailsCheckBox->setChecked(m_Settings.readBoolEntry("/Details", false));
    ForceArtsCheckBox->setChecked(m_Settings.readBoolEntry("/ForceArts", true));
    ForceArtsShellComboBox->setCurrentText(m_Settings.readEntry("/ForceArtsShell", "artsshell -q terminate"));
    ForceJackCheckBox->setChecked(m_Settings.readBoolEntry("/ForceJack", true));
    ForceJackShellComboBox->setCurrentText(m_Settings.readEntry("/ForceJackShell", "killall -9 jackd"));
    XrunRegexComboBox->setCurrentText(m_Settings.readEntry("/XrunRegex", "xrun of at least ([0-9|\\.]+) msecs"));
    XrunIgnoreFirstCheckBox->setChecked(m_Settings.readBoolEntry("/XrunIgnoreFirst", true));
    AutoRefreshCheckBox->setChecked(m_Settings.readBoolEntry("/AutoRefresh", true));
    TimeRefreshComboBox->setCurrentText(QString::number(m_Settings.readNumEntry("/TimeRefresh", 10)));
    m_Settings.endGroup();

    QPoint pt;
    m_Settings.beginGroup("/Position");
    pt.setX(m_Settings.readNumEntry("/x", -1));
    pt.setY(m_Settings.readNumEntry("/y", -1));
    m_Settings.endGroup();

    m_Settings.endGroup();

    // Try to restore old window position.
    if (pt.x() > 0 && pt.y() > 0)
        move(pt);

    // Final startup stabilization...
    stabilizeForm();
    processJackExit();
}


// Kind of destructor.
void qjackctlMainForm::destroy()
{
    // Stop server, if not already...
    stopJack();

    // Save all settings and options...
    m_Settings.beginGroup("/qjackctl");

    m_Settings.beginGroup("/Program");
    m_Settings.writeEntry("/Version", QJACKCTL_VERSION);
    m_Settings.endGroup();

    m_Settings.beginGroup("/Settings");
    m_Settings.writeEntry("/Server", ServerComboBox->currentText());
    m_Settings.writeEntry("/Realtime", RealtimeCheckBox->isChecked());
    m_Settings.writeEntry("/SoftMode", SoftModeCheckBox->isChecked());
    m_Settings.writeEntry("/Asio", AsioCheckBox->isChecked());
    m_Settings.writeEntry("/Monitor", MonitorCheckBox->isChecked());
    m_Settings.writeEntry("/Priority", PriorityComboBox->currentText().toInt());
    m_Settings.writeEntry("/Frames", FramesComboBox->currentText().toInt());
    m_Settings.writeEntry("/SampleRate", SampleRateComboBox->currentText().toInt());
    m_Settings.writeEntry("/Periods", PeriodsComboBox->currentText().toInt());
    m_Settings.writeEntry("/Interface", InterfaceComboBox->currentText());
    m_Settings.writeEntry("/Audio", AudioComboBox->currentItem());
    m_Settings.writeEntry("/Dither", DitherComboBox->currentItem());
    m_Settings.writeEntry("/Timeout", TimeoutComboBox->currentText().toInt());
    m_Settings.writeEntry("/HWMon", HWMonCheckBox->isChecked());
    m_Settings.writeEntry("/HWMeter", HWMeterCheckBox->isChecked());
    m_Settings.writeEntry("/TempDir", TempDirComboBox->currentText());
    m_Settings.endGroup();

    m_Settings.beginGroup("/Options");
    m_Settings.writeEntry("/Details", DetailsCheckBox->isChecked());
    m_Settings.writeEntry("/ForceArts", ForceArtsCheckBox->isChecked());
    m_Settings.writeEntry("/ForceArtsShell", ForceArtsShellComboBox->currentText());
    m_Settings.writeEntry("/ForceJack", ForceJackCheckBox->isChecked());
    m_Settings.writeEntry("/ForceJackShell", ForceJackShellComboBox->currentText());
    m_Settings.writeEntry("/XrunRegex", XrunRegexComboBox->currentText());
    m_Settings.writeEntry("/XrunIgnoreFirst", XrunIgnoreFirstCheckBox->isChecked());
    m_Settings.writeEntry("/AutoRefresh", AutoRefreshCheckBox->isChecked());
    m_Settings.writeEntry("/TimeRefresh", TimeRefreshComboBox->currentText().toInt());
    m_Settings.endGroup();

    QPoint pt = pos();
    m_Settings.beginGroup("/Position");
    m_Settings.writeEntry("/x", pt.x());
    m_Settings.writeEntry("/y", pt.y());
    m_Settings.endGroup();

    m_Settings.endGroup();
}


// Start jack audio server...
void qjackctlMainForm::startJack()
{
    // Is the server process instance still here?
    if (m_pJack) {
        switch (QMessageBox::warning(this, tr("Warning"),
            tr("Could not start JACK. Maybe already started."),
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

    setCaption(QJACKCTL_TITLE " - " + tr("Starting..."));
    
    QString sTemp;
    int iExitStatus;
    
    // Do we force aRts sound server?...
    if (ForceArtsCheckBox->isChecked()) {
        appendMessages(tr("ARTS is being forced..."));
        sTemp  = "[" + ForceArtsShellComboBox->currentText();
        appendMessages(sTemp.stripWhiteSpace() + "]");
        iExitStatus = system(ForceArtsShellComboBox->currentText());
        sTemp  = " " + tr("exit status");
        sTemp += "=";
        sTemp += QString::number(iExitStatus);
        sTemp += ".";
        appendMessages(tr("ARTS has been forced with") + sTemp);
        // Wait a litle bit...
        system("sleep 1");
    }

    // Do we force stray JACK daemon threads?...
    if (ForceJackCheckBox->isChecked()) {
        appendMessages(tr("JACK is being forced..."));
        sTemp  = "[" + ForceJackShellComboBox->currentText();
        appendMessages(sTemp.stripWhiteSpace() + "]");
        iExitStatus = system(ForceJackShellComboBox->currentText());
        sTemp  = " " + tr("exit status");
        sTemp += "=";
        sTemp += QString::number(iExitStatus);
        sTemp += ".";
        appendMessages(tr("JACK has been forced with") + sTemp);
        // Wait yet another bit...
        system("sleep 1");
    }       
    
    // OK. Let's build the startup process...
    m_pJack = new QProcess(this);

    // Setup communications...
    QObject::connect(m_pJack, SIGNAL(readyReadStdout()), this, SLOT(readJackStdout()));
    QObject::connect(m_pJack, SIGNAL(readyReadStderr()), this, SLOT(readJackStderr()));
    QObject::connect(m_pJack, SIGNAL(processExited()),   this, SLOT(processJackExit()));

    // Build process arguments...
    m_pJack->addArgument(ServerComboBox->currentText());
    if (RealtimeCheckBox->isChecked())
        m_pJack->addArgument("-R");
    if (PriorityComboBox->currentText().toInt()) {
        m_pJack->addArgument("-P");
        m_pJack->addArgument(PriorityComboBox->currentText());
    }
    if (AsioCheckBox->isChecked())
        m_pJack->addArgument("-a");
    if (TimeoutComboBox->currentText().toInt() > 0) {
        m_pJack->addArgument("-t");
        m_pJack->addArgument(TimeoutComboBox->currentText());
    }
    sTemp = TempDirComboBox->currentText();
    if (sTemp == "(default)")
        sTemp = "";
    if (!sTemp.isEmpty()) {
        m_pJack->addArgument("-D");
        m_pJack->addArgument(sTemp);
    }
    m_pJack->addArgument("-d");
    m_pJack->addArgument("alsa");
    m_pJack->addArgument("-d");
    m_pJack->addArgument(InterfaceComboBox->currentText());
    if (SampleRateComboBox->currentText().toInt() > 0) {
        m_pJack->addArgument("-r");
        m_pJack->addArgument(SampleRateComboBox->currentText());
    }
    if (FramesComboBox->currentText().toInt() > 0) {
        m_pJack->addArgument("-p");
        m_pJack->addArgument(FramesComboBox->currentText());
    }
    if (PeriodsComboBox->currentText().toInt() > 0) {
        m_pJack->addArgument("-n");
        m_pJack->addArgument(PeriodsComboBox->currentText());
    }
    if (SoftModeCheckBox->isChecked())
        m_pJack->addArgument("-s");
    if (MonitorCheckBox->isChecked())
        m_pJack->addArgument("-m");
    switch (AudioComboBox->currentItem()) {
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
    switch (DitherComboBox->currentItem()) {
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
    if (HWMonCheckBox->isChecked())
        m_pJack->addArgument("-H");
    if (HWMeterCheckBox->isChecked())
        m_pJack->addArgument("-M");
	
    appendMessages(tr("JACK is starting..."));
    QStringList list = m_pJack->arguments();
    QStringList::Iterator iter = list.begin();
    sTemp = "[";
    while( iter != list.end() ) {
	    sTemp += *iter++;
        sTemp += " ";
    }
    appendMessages(sTemp.stripWhiteSpace() + "]");

    // Do not forget to reset XRUN stats variables.
    resetXrunStats();

    // Go jack, go...
    if (!m_pJack->start()) {
        QMessageBox::critical(this, tr("Fatal error"),
            tr("Could not start JACK. Sorry."),
            QMessageBox::Cancel, QMessageBox::NoButton, QMessageBox::NoButton);
        processJackExit();
        return;
    }

    // Show startup results...
    sTemp = " ";
    sTemp += tr("PID");
    sTemp += "=";
    sTemp += QString::number((long) m_pJack->processIdentifier());
    sTemp += " (0x";
    sTemp += QString::number((long) m_pJack->processIdentifier(), 16);
    sTemp += ").";
    appendMessages(tr("JACK is started with") + sTemp);

    setCaption(QJACKCTL_TITLE " - " + tr("Started."));

    StartPushButton->setEnabled(false);
    StopPushButton->setEnabled(true);

    // Create our timer...
    m_iTimerSlot = 0;
    m_pTimer = new QTimer(this);
    QObject::connect(m_pTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
    // Let's wait three seconds for client startup?
    m_pTimer->start(3000, false);
}


// Stop jack audio server...
void qjackctlMainForm::stopJack()
{
    // Stop timer.
    if (m_pTimer)
        delete m_pTimer;
    m_pTimer = NULL;

    // Stop client code.
    stopJackClient();

    // And try to stop server.
    if (m_pJack) {
        appendMessages(tr("JACK is stopping..."));
        if (m_pJack->isRunning())
            m_pJack->tryTerminate();
        else
            processJackExit();
    }
}


// Stdout handler...
void qjackctlMainForm::readJackStdout()
{
    QString s = m_pJack->readStdout();
    appendMessages(detectXrun(s));
}


// Stderr handler...
void qjackctlMainForm::readJackStderr()
{
    QString s = m_pJack->readStderr();
    appendMessages(detectXrun(s));
}


// Jack audio server cleanup.
void qjackctlMainForm::processJackExit()
{
    if (m_pJack) {
        QString sTemp = " ";
        sTemp += tr("exit status");
        sTemp += "=";
        sTemp += QString::number(m_pJack->exitStatus());
        sTemp += ".";
        appendMessages(tr("JACK is stopped with") + sTemp);
        if (!m_pJack->normalExit())
            m_pJack->kill();
        delete m_pJack;
    }
    m_pJack = NULL;   

    setCaption(QJACKCTL_TITLE " - " + tr("Stopped."));

    StartPushButton->setEnabled(true);
    StopPushButton->setEnabled(false);    
}


// XRUN detection routine.
QString& qjackctlMainForm::detectXrun( QString & s )
{
    QRegExp rx(XrunRegexComboBox->currentText());
    int iPos = rx.search(s);
    if (iPos >= 0) {
        s.insert(iPos + rx.matchedLength(), "</font>");
        s.insert(iPos, "<font color=\"red\">");
        if (m_iXrunStats > 0 || !XrunIgnoreFirstCheckBox->isChecked()) {
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


// Messages widget output method.
void qjackctlMainForm::appendMessages( const QString & s )
{
    while (MessagesTextView->paragraphs() > 100) {
        MessagesTextView->removeParagraph(0);
        MessagesTextView->scrollToBottom();		
    }
    MessagesTextView->append(s);
}


// User interface stabilization.
void qjackctlMainForm::stabilizeForm()
{
    bool bShowDetails = DetailsCheckBox->isChecked();

    if (bShowDetails) {
        bool bEnabled = RealtimeCheckBox->isChecked();
        PriorityTextLabel->setEnabled(bEnabled);
        PriorityComboBox->setEnabled(bEnabled);
        ForceArtsShellComboBox->setEnabled(ForceArtsCheckBox->isChecked());
        ForceJackShellComboBox->setEnabled(ForceJackCheckBox->isChecked());
        TimeRefreshComboBox->setEnabled(AutoRefreshCheckBox->isChecked());
        DetailsTabWidget->show();
    } else {
        DetailsTabWidget->hide();
    }

    adjustSize();

    if (bShowDetails) {
        QDesktopWidget *pDesktop = QApplication::desktop();
        QPoint curPos = pos();
        QPoint delta(
            pDesktop->width()  - (curPos.x() + size().width()  + 16),
            pDesktop->height() - (curPos.y() + size().height() + 32)
        );
        if (delta.x() > 0)
            delta.setX(0);
        if (delta.y() > 0)
            delta.setY(0);
        if (delta.x() < 0 || delta.y() < 0) {
            m_posSave = curPos;
            move(curPos + delta);
        }
        stabilizeConnections();
    }
    else if (!m_posSave.isNull())
        move(m_posSave);
}


// Reset XRUN cache items.
void qjackctlMainForm::resetXrunStats()
{
    m_iXrunStats = 0;
    m_iXrunCount = 0;
    m_fXrunTotal = 0.0;
    m_fXrunMin   = 0.0;
    m_fXrunMax   = 0.0;
    m_fXrunLast  = 0.0;

    m_iXrunCallbacks = 0;

    refreshXrunStats();
}


// Update the XRUN count/callbacks item.
void qjackctlMainForm::updateXrunCount (void)
{
    QString sText = QString::number(m_iXrunCount);
    if (m_pJackClient) {
        sText += " (";
        sText += QString::number(m_iXrunCallbacks);
        sText += ")";
    }
    m_apStats[STATS_XRUN_COUNT]->setText(1, sText);
}


// Update the XRUN last/elapsed time item.
void qjackctlMainForm::updateXrunTime (void)
{
    QString sText = m_tXrunLast.toString();
    int iSeconds = (m_tXrunLast.elapsed() / 1000);
    if (m_iXrunCount > 0 && iSeconds > 1) {
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
    m_apStats[STATS_XRUN_TIME]->setText(1, sText);
}


// Update the XRUN list view items.
void qjackctlMainForm::refreshXrunStats()
{
    float fXrunAverage = 0.0;
    if (m_iXrunCount > 0)
        fXrunAverage = (m_fXrunTotal / m_iXrunCount);
    updateXrunCount();
    QString s = " " + tr("msec");
    m_apStats[STATS_XRUN_TOTAL]->setText(1, QString::number(m_fXrunTotal) + s);
    m_apStats[STATS_XRUN_MIN]->setText(1, QString::number(m_fXrunMin) + s);
    m_apStats[STATS_XRUN_MAX]->setText(1, QString::number(m_fXrunMax) + s);
    m_apStats[STATS_XRUN_AVG]->setText(1, QString::number(fXrunAverage) + s);
    m_apStats[STATS_XRUN_LAST]->setText(1, QString::number(m_fXrunLast) + s);
    updateXrunTime();
    StatsListView->triggerUpdate();
}


// Notification pipes descriptors
#define FD_NIL     -1
#define FD_READ     0
#define FD_WRITE    1

static int g_fdPort[2] = { FD_NIL, FD_NIL };
static int g_fdXrun[2] = { FD_NIL, FD_NIL };
static int g_fdShut[2] = { FD_NIL, FD_NIL };

// Jack port registration callback funtion, called
// whenever a jack port is registered or unregistered.
static void qjackctl_portRegistrationCallback ( jack_port_id_t, int, void * )
{
    char c = 0;

    ::write(g_fdPort[FD_WRITE], &c, sizeof(c));
}


// Jack graph order callback function, called
// whenever the processing graph is reordered.
static int qjackctl_graphOrderCallback ( void * )
{
    char c = 0;

    ::write(g_fdPort[FD_WRITE], &c, sizeof(c));

    return 0;
}


// Jack XRUN callback function, called
// whenever there is a xrun.
static int qjackctl_xrunCallback ( void * )
{
    char c = 0;

    ::write(g_fdXrun[FD_WRITE], &c, sizeof(c));

    return 0;
}

// Jack shutdown function, called
// whenever the server terminates this client.
static void qjackctl_shutdown ( void * )
{
    char c = 0;

    ::write(g_fdShut[FD_WRITE], &c, sizeof(c));
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

    // Just increment callback counter.
    m_iXrunCallbacks++;

    // Update the status item directly.
    updateXrunCount();
    StatsListView->triggerUpdate();

    m_iXrunNotify--;
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

    // Do what has to be done.
    stopJack();

    m_iShutNotify--;
}


// Timer callback funtion.
void qjackctlMainForm::timerSlot (void)
{
    // Is it the first shot on server start?
    m_iTimerSlot++;
    if (m_iTimerSlot == 1) {
        startJackClient();
        return;
    }
    // Shall we refresh connections now and then?
    if (AutoRefreshCheckBox->isChecked() && (m_iTimerSlot % TimeRefreshComboBox->currentText().toInt()) == 0)
        refreshConnections();

    // Update some statistical fields, directly.
    if (m_pJackClient) {
        QString s = " ";
        m_apStats[STATS_CPU_LOAD]->setText(1, QString::number(jack_cpu_load(m_pJackClient), 'g', 3) + s + "%");
        m_apStats[STATS_SAMPLE_RATE]->setText(1, QString::number(jack_get_sample_rate(m_pJackClient)) + s + tr("Hz"));
    }
    updateXrunTime();
    StatsListView->triggerUpdate();
}


// Close notification pipes.
void qjackctlMainForm::closePipes (void)
{
    // Port/Graph notification pipe.
    if (g_fdPort[FD_READ] != FD_NIL) {
        ::close(g_fdPort[FD_READ]);
        g_fdPort[FD_READ] = FD_NIL;
    }
    if (g_fdPort[FD_WRITE] != FD_NIL) {
        ::close(g_fdPort[FD_WRITE]);
        g_fdPort[FD_WRITE] = FD_NIL;
    }
    // XRUN notification pipe.
    if (g_fdXrun[FD_READ] != FD_NIL) {
        ::close(g_fdXrun[FD_READ]);
        g_fdXrun[FD_READ] = FD_NIL;
    }
    if (g_fdXrun[FD_WRITE] != FD_NIL) {
        ::close(g_fdXrun[FD_WRITE]);
        g_fdXrun[FD_WRITE] = FD_NIL;
    }
    // Shutdown notification pipe.
    if (g_fdShut[FD_READ] != FD_NIL) {
        ::close(g_fdShut[FD_READ]);
        g_fdShut[FD_READ] = FD_NIL;
    }
    if (g_fdShut[FD_WRITE] != FD_NIL) {
        ::close(g_fdShut[FD_WRITE]);
        g_fdShut[FD_WRITE] = FD_NIL;
    }
}


// Start our jack audio control client...
void qjackctlMainForm::startJackClient()
{
    // Create port notification pipe.
    if (::pipe(g_fdPort) < 0) {
        g_fdPort[FD_READ]  = FD_NIL;
        g_fdPort[FD_WRITE] = FD_NIL;
        closePipes();
        QMessageBox::critical(this, tr("Error"),
            tr("Could not create port notification pipe."),
            QMessageBox::Cancel, QMessageBox::NoButton, QMessageBox::NoButton);
        return;
    }

    // Create XRUN notification pipe.
    if (::pipe(g_fdXrun) < 0) {
        g_fdXrun[FD_READ]  = FD_NIL;
        g_fdXrun[FD_WRITE] = FD_NIL;
        closePipes();
        QMessageBox::critical(this, tr("Error"),
            tr("Could not create XRUN notification pipe."),
            QMessageBox::Cancel, QMessageBox::NoButton, QMessageBox::NoButton);
        return;
    }

    // Create shutdown notification pipe.
    if (::pipe(g_fdShut) < 0) {
        g_fdShut[FD_READ]  = FD_NIL;
        g_fdShut[FD_WRITE] = FD_NIL;
        closePipes();
        QMessageBox::critical(this, tr("Error"),
            tr("Could not create shutdown notification pipe."),
            QMessageBox::Cancel, QMessageBox::NoButton, QMessageBox::NoButton);
        return;
    }

    // Create the jack client handle
    m_pJackClient = jack_client_new("qjackctl");
    if (m_pJackClient == NULL) {
        closePipes();
        QMessageBox::critical(this, tr("Error"),
            tr("Could not connect to jack server."),
            QMessageBox::Cancel, QMessageBox::NoButton, QMessageBox::NoButton);
        return;
    }

    // Set notification callbacks.
    jack_set_graph_order_callback(m_pJackClient, qjackctl_graphOrderCallback, NULL);
    jack_set_port_registration_callback(m_pJackClient, qjackctl_portRegistrationCallback, NULL);
    jack_set_xrun_callback(m_pJackClient, qjackctl_xrunCallback, NULL);
    jack_on_shutdown(m_pJackClient, qjackctl_shutdown, NULL);

    // Create our notification managers.
    m_pPortNotifier = new QSocketNotifier(g_fdPort[FD_READ], QSocketNotifier::Read);
    m_pXrunNotifier = new QSocketNotifier(g_fdXrun[FD_READ], QSocketNotifier::Read);
    m_pShutNotifier = new QSocketNotifier(g_fdShut[FD_READ], QSocketNotifier::Read);

    // And connect it to the proper slots.
    QObject::connect(m_pPortNotifier, SIGNAL(activated(int)), this, SLOT(portNotifySlot(int)));
    QObject::connect(m_pXrunNotifier, SIGNAL(activated(int)), this, SLOT(xrunNotifySlot(int)));
    QObject::connect(m_pShutNotifier, SIGNAL(activated(int)), this, SLOT(shutNotifySlot(int)));

    // Activate us as a client...
    jack_activate(m_pJackClient);

    // Create our patchbay...
    m_pJackPatchbay = new qjackctlPatchbay(ConnectorFrame, ReadListView, WriteListView, m_pJackClient);

    // Our timer will now get one second standard.
    m_pTimer->start(1000, false);
}


// Stop jack audio client...
void qjackctlMainForm::stopJackClient()
{
    // Deactivate us as a client...
    if (m_pJackClient)
        jack_deactivate(m_pJackClient);

    // Destroy our patchbay...
    if (m_pJackPatchbay)
        delete m_pJackPatchbay;
    m_pJackPatchbay = NULL;

    stabilizeConnections();

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

    if (m_pShutNotifier)
        delete m_pShutNotifier;
    m_pShutNotifier = NULL;
    m_iShutNotify = 0;

    // Reset jack client statistics explicitly.
    QString n = tr("n/a");
    m_apStats[STATS_CPU_LOAD]->setText(1, n);
    m_apStats[STATS_SAMPLE_RATE]->setText(1, n);
    refreshXrunStats();
}


// Connect current selected ports.
void qjackctlMainForm::connectSelected()
{
    if (m_pJackPatchbay)
        m_pJackPatchbay->connectSelected();
}


// Disconnect current selected ports.
void qjackctlMainForm::disconnectSelected()
{
    if (m_pJackPatchbay)
        m_pJackPatchbay->disconnectSelected();
}


// Rebuild all patchbay items.
void qjackctlMainForm::refreshConnections()
{
    if (m_pJackPatchbay)
        m_pJackPatchbay->refresh();

    stabilizeConnections();
}


// Proper enablement of patchbay command controls.
void qjackctlMainForm::stabilizeConnections()
{
    if (m_pJackPatchbay) {
        ConnectPushButton->setEnabled(m_pJackPatchbay->canConnectSelected());
        DisconnectPushButton->setEnabled(m_pJackPatchbay->canDisconnectSelected());
        RefreshPushButton->setEnabled(true);
    } else {
        ConnectPushButton->setEnabled(false);
        DisconnectPushButton->setEnabled(false);
        RefreshPushButton->setEnabled(false);
    }
}


// end of ui.h
