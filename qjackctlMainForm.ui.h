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
#define QJACKCTL_VERSION	"0.0.5.3"
#define QJACKCTL_WEBSITE	"http://qjackctl.sourceforge.net"

#include <qapplication.h>
#include <qmessagebox.h>
#include <qvalidator.h>

// List view statistics item indexes
#define STATS_CPU_LOAD      0
#define STATS_SAMPLE_RATE   1
#define STATS_RESET_TIME    2
#define STATS_XRUN_AVG      3
#define STATS_XRUN_COUNT    4
#define STATS_XRUN_LAST     5
#define STATS_XRUN_MAX      6
#define STATS_XRUN_MIN      7
#define STATS_XRUN_TIME     8
#define STATS_XRUN_TOTAL    9


// Kind of constructor.
void qjackctlMainForm::init (void)
{
    m_pJack       = NULL;
    m_pJackClient = NULL;
    m_pTimer      = NULL;
    m_iTimerSlot  = 0;
    m_iRefresh    = 0;

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
    m_apStats[STATS_RESET_TIME]  = new QListViewItem(StatsListView, s + tr("Time of last reset") + c, z);
    m_apStats[STATS_XRUN_AVG]    = new QListViewItem(StatsListView, s + tr("XRUN average") + c, z);
    m_apStats[STATS_XRUN_COUNT]  = new QListViewItem(StatsListView, s + tr("XRUN count since last server startup") + c, z);
    m_apStats[STATS_XRUN_LAST]   = new QListViewItem(StatsListView, s + tr("XRUN last") + c, z);
    m_apStats[STATS_XRUN_MAX]    = new QListViewItem(StatsListView, s + tr("XRUN maximum") + c, z);
    m_apStats[STATS_XRUN_MIN]    = new QListViewItem(StatsListView, s + tr("XRUN minimum") + c, z);
    m_apStats[STATS_XRUN_TIME]   = new QListViewItem(StatsListView, s + tr("XRUN last time detected") + c, n);
    m_apStats[STATS_XRUN_TOTAL]  = new QListViewItem(StatsListView, s + tr("XRUN total") + c, z);

    resetXrunStats();

    // Set dialog validators...
    ChanComboBox->setValidator(new QIntValidator(ChanComboBox));
    PriorityComboBox->setValidator(new QIntValidator(PriorityComboBox));
    FramesComboBox->setValidator(new QIntValidator(FramesComboBox));
    SampleRateComboBox->setValidator(new QIntValidator(SampleRateComboBox));
    PeriodsComboBox->setValidator(new QIntValidator(PeriodsComboBox));
    WaitComboBox->setValidator(new QIntValidator(WaitComboBox));
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
    ChanComboBox->setCurrentText(QString::number(m_Settings.readNumEntry("/Chan", 0)));
    PriorityComboBox->setCurrentText(QString::number(m_Settings.readNumEntry("/Priority", 0)));
    FramesComboBox->setCurrentText(QString::number(m_Settings.readNumEntry("/Frames", 1024)));
    SampleRateComboBox->setCurrentText(QString::number(m_Settings.readNumEntry("/SampleRate", 48000)));
    PeriodsComboBox->setCurrentText(QString::number(m_Settings.readNumEntry("/Periods", 2)));
    WaitComboBox->setCurrentText(QString::number(m_Settings.readNumEntry("/Wait", 21333)));
    DriverComboBox->setCurrentText(m_Settings.readEntry("/Driver", "alsa"));
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

    m_Settings.beginGroup("/Position");
    m_pos.setX(m_Settings.readNumEntry("/x", -1));
    m_pos.setY(m_Settings.readNumEntry("/y", -1));
    m_pos1.setX(m_Settings.readNumEntry("/x1", -1));
    m_pos1.setY(m_Settings.readNumEntry("/y1", -1));
    m_size1.setWidth(m_Settings.readNumEntry("/width1", -1));
    m_size1.setHeight(m_Settings.readNumEntry("/height1", -1));
    m_Settings.endGroup();

    m_Settings.endGroup();

    // Final startup stabilization...
    // (try to restore old window positioning)
    stabilizeForm(false);
    processJackExit();
}


// Kind of destructor.
void qjackctlMainForm::destroy (void)
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
    m_Settings.writeEntry("/Chan", ChanComboBox->currentText().toInt());
    m_Settings.writeEntry("/Priority", PriorityComboBox->currentText().toInt());
    m_Settings.writeEntry("/Frames", FramesComboBox->currentText().toInt());
    m_Settings.writeEntry("/SampleRate", SampleRateComboBox->currentText().toInt());
    m_Settings.writeEntry("/Periods", PeriodsComboBox->currentText().toInt());
    m_Settings.writeEntry("/Wait", WaitComboBox->currentText().toInt());
    m_Settings.writeEntry("/Driver", DriverComboBox->currentText());
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

    m_Settings.beginGroup("/Position");
    m_Settings.writeEntry("/x", m_pos.x());
    m_Settings.writeEntry("/y", m_pos.y());
    m_Settings.writeEntry("/x1", m_pos1.x());
    m_Settings.writeEntry("/y1", m_pos1.y());
    m_Settings.writeEntry("/width1", m_size1.width());
    m_Settings.writeEntry("/height1", m_size1.height());
    m_Settings.endGroup();

    m_Settings.endGroup();
}


// Window close event handlers.
bool qjackctlMainForm::queryClose (void)
{
    bool bResult = true;
    
    if (m_pJack && m_pJack->isRunning()) {
        bResult = (QMessageBox::warning(this, tr("Warning"),
            tr("JACK is currently running.") + "\n\n" +
            tr("Closing this application will also terminate the JACK audio server."),
            tr("OK"), tr("Cancel")) == 0);
    }

    return bResult;
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

    setCaption(QJACKCTL_TITLE " - " + tr("Starting..."));
    StartPushButton->setEnabled(false);

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
    sTemp = DriverComboBox->currentText();
    m_pJack->addArgument("-d");
    m_pJack->addArgument(sTemp);
    bool bDummy     = (sTemp == "dummy");
    bool bAlsa      = (sTemp == "alsa");
    bool bPortaudio = (sTemp == "portaudio");
    if (bAlsa) {
        m_pJack->addArgument("-d");
        m_pJack->addArgument(InterfaceComboBox->currentText());
    }
    if (bPortaudio && ChanComboBox->currentText().toInt() > 0) {
        m_pJack->addArgument("-c");
        m_pJack->addArgument(ChanComboBox->currentText());
    }
    if (SampleRateComboBox->currentText().toInt() > 0) {
        m_pJack->addArgument("-r");
        m_pJack->addArgument(SampleRateComboBox->currentText());
    }
    if (FramesComboBox->currentText().toInt() > 0) {
        m_pJack->addArgument("-p");
        m_pJack->addArgument(FramesComboBox->currentText());
    }
    if (bAlsa) {
        if (!AsioCheckBox->isChecked() && PeriodsComboBox->currentText().toInt() > 0) {
            m_pJack->addArgument("-n");
            m_pJack->addArgument(PeriodsComboBox->currentText());
        }
        if (SoftModeCheckBox->isChecked())
            m_pJack->addArgument("-s");
        if (MonitorCheckBox->isChecked())
            m_pJack->addArgument("-m");
    }
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
    if (!bDummy) {
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
    }
    if (bAlsa) {
        if (HWMonCheckBox->isChecked())
            m_pJack->addArgument("-H");
        if (HWMeterCheckBox->isChecked())
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

    // Do not forget to reset XRUN stats variables.
    resetXrunStats();

    // Go jack, go...
    if (!m_pJack->start()) {
        QMessageBox::critical(this, tr("Fatal error"),
            tr("Could not start JACK.") + "\n\n" +
            tr("Sorry."),
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
    StopPushButton->setEnabled(true);

    // Create our timer...
    m_iTimerSlot = 0;
    m_iRefresh = 0;
    m_pTimer = new QTimer(this);
    QObject::connect(m_pTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
    // Let's wait three seconds for client startup?
    m_pTimer->start(3000, false);
}


// Stop jack audio server...
void qjackctlMainForm::stopJack (void)
{
    // Stop client code.
    stopJackClient();

    // Kill timer.
    if (m_pTimer)
        delete m_pTimer;
    m_pTimer = NULL;

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
void qjackctlMainForm::readJackStdout (void)
{
    QString s = m_pJack->readStdout();
    appendMessages(detectXrun(s));
}


// Stderr handler...
void qjackctlMainForm::readJackStderr (void)
{
    QString s = m_pJack->readStderr();
    appendMessages(detectXrun(s));
}


// Jack audio server cleanup.
void qjackctlMainForm::processJackExit (void)
{
    // Force client code cleanup.
    stopJackClient();

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
void qjackctlMainForm::toggleDetails (void)
{
    stabilizeForm(true);
}

void qjackctlMainForm::toggleAsio (void)
{
    changeDriver(DriverComboBox->currentText());
}

void qjackctlMainForm::changeDriver ( const QString& sDriver )
{
    bool bDummy     = (sDriver == "dummy");
    bool bAlsa      = (sDriver == "alsa");
    bool bPortaudio = (sDriver == "portaudio");
    bool bAsio      = AsioCheckBox->isChecked();
    
    SoftModeCheckBox->setEnabled(bAlsa);
    MonitorCheckBox->setEnabled(bAlsa);

    ChanTextLabel->setEnabled(bPortaudio);
    ChanComboBox->setEnabled(bPortaudio);

    PeriodsTextLabel->setEnabled(bAlsa && !bAsio);
    PeriodsComboBox->setEnabled(bAlsa && !bAsio);

    WaitTextLabel->setEnabled(bDummy);
    WaitComboBox->setEnabled(bDummy);    

    InterfaceTextLabel->setEnabled(bAlsa);
    InterfaceComboBox->setEnabled(bAlsa);

    DitherTextLabel->setEnabled(!bDummy);
    DitherComboBox->setEnabled(!bDummy);
    
    HWMonCheckBox->setEnabled(bAlsa);
    HWMeterCheckBox->setEnabled(bAlsa);
}

void qjackctlMainForm::stabilizeForm ( bool bSavePosition )
{
    bool bShowDetails = DetailsCheckBox->isChecked();

    if (bShowDetails) {
        bool bEnabled = RealtimeCheckBox->isChecked();
        PriorityTextLabel->setEnabled(bEnabled);
        PriorityComboBox->setEnabled(bEnabled);
        ForceArtsShellComboBox->setEnabled(ForceArtsCheckBox->isChecked());
        ForceJackShellComboBox->setEnabled(ForceJackCheckBox->isChecked());
        TimeRefreshComboBox->setEnabled(AutoRefreshCheckBox->isChecked());
        changeDriver(DriverComboBox->currentText());
        DetailsTabWidget->show();
    } else {
        DetailsTabWidget->hide();
    }

    if (bShowDetails) {
        if (bSavePosition)
            m_pos = pos();
        if (m_pos1.x() > 0 && m_pos1.y() > 0)
            move(m_pos1);
        if (m_size1.width() > 0 && m_size1.height() > 0)
            resize(m_size1);
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
        if (delta.x() < 0 || delta.y() < 0)
            move(curPos + delta);
        stabilizeConnections();
    } else {
        if (bSavePosition) {
            m_pos1 = pos();
            m_size1 = size();
        }
        if (m_pos.x() > 0 && m_pos.y() > 0)
            move(m_pos);
        adjustSize();
    }
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
QString qjackctlMainForm::formatElapsedTime ( QTime& t, bool bElapsed )
{
    QString sText = t.toString();
    int iSeconds = (t.elapsed() / 1000);
    if (bElapsed && iSeconds > 1) {
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
    return sText;
}


// Update the XRUN last/elapsed time item.
void qjackctlMainForm::updateElapsedTimes (void)
{
    m_apStats[STATS_RESET_TIME]->setText(1, formatElapsedTime(m_tResetLast, true));
    m_apStats[STATS_XRUN_TIME]->setText(1, formatElapsedTime(m_tXrunLast, (m_iXrunCount > 1)));
}


// Update the XRUN list view items.
void qjackctlMainForm::refreshXrunStats (void)
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
    updateElapsedTimes();
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

    // Are we about to refresh it, really?
    if (m_iRefresh > 0) {
        m_iRefresh = 0;
        if (m_pJackPatchbay)
            m_pJackPatchbay->refresh();
        stabilizeConnections();
    }

    // Update some statistical fields, directly.
    if (m_pJackClient) {
        QString s = " ";
        m_apStats[STATS_CPU_LOAD]->setText(1, QString::number(jack_cpu_load(m_pJackClient), 'g', 3) + s + "%");
        m_apStats[STATS_SAMPLE_RATE]->setText(1, QString::number(jack_get_sample_rate(m_pJackClient)) + s + tr("Hz"));
    }
    updateElapsedTimes();
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
void qjackctlMainForm::startJackClient (void)
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
            tr("Could not connect to JACK server."),
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

    // Create our patchbay...
    m_pJackPatchbay = new qjackctlPatchbay(ConnectorFrame, ReadListView, WriteListView, m_pJackClient);

    // Activate us as a client...
    jack_activate(m_pJackClient);

    // Our timer will now get one second standard.
    m_pTimer->start(1000, false);
    
    // Remember to schedule an initial connection refreshment.
    refreshConnections();
}


// Stop jack audio client...
void qjackctlMainForm::stopJackClient (void)
{
    // Stop timer.
    if (m_pTimer)
        m_pTimer->stop();

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
void qjackctlMainForm::connectSelected (void)
{
    if (m_pJackPatchbay)
        m_pJackPatchbay->connectSelected();

    stabilizeConnections();
}


// Disconnect current selected ports.
void qjackctlMainForm::disconnectSelected (void)
{
    if (m_pJackPatchbay)
        m_pJackPatchbay->disconnectSelected();

    stabilizeConnections();
}


// Disconnect all connected ports.
void qjackctlMainForm::disconnectAll()
{
    if (m_pJackPatchbay)
        m_pJackPatchbay->disconnectAll();

    stabilizeConnections();
}


// Rebuild all patchbay items.
void qjackctlMainForm::refreshConnections (void)
{
    // Just increment our intentions; it will be deferred
    // to be executed just on timer slot processing...
    m_iRefresh++;
}


// Proper enablement of patchbay command controls.
void qjackctlMainForm::stabilizeConnections (void)
{
    if (m_pJackPatchbay) {
        ConnectPushButton->setEnabled(m_pJackPatchbay->canConnectSelected());
        DisconnectPushButton->setEnabled(m_pJackPatchbay->canDisconnectSelected());
        DisconnectAllPushButton->setEnabled(m_pJackPatchbay->canDisconnectAll());
        RefreshPushButton->setEnabled(true);
    } else {
        ConnectPushButton->setEnabled(false);
        DisconnectPushButton->setEnabled(false);
        DisconnectAllPushButton->setEnabled(false);
        RefreshPushButton->setEnabled(false);
    }
}


// end of ui.h
