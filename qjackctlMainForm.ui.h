/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/
#define QJACKCTL_TITLE		"JACK Audio Connection Kit"
#define QJACKCTL_SUBTITLE	"Qt GUI Interface"
#define QJACKCTL_VERSION	"0.0.1"
#define QJACKCTL_WEBSITE	"http://qjackctl.sourceforge.net"

#include <qvalidator.h>
#include <qmessagebox.h>
#include <qlistview.h>
#include <qapplication.h>

void qjackctlMainForm::init()
{
    m_pJack = NULL;
    
    resetXrunStats();

    // Set dialog validators...
    PriorityComboBox->setValidator(new QIntValidator(PriorityComboBox));
    FramesComboBox->setValidator(new QIntValidator(FramesComboBox));
    SampleRateComboBox->setValidator(new QIntValidator(SampleRateComboBox));
    PeriodsComboBox->setValidator(new QIntValidator(PeriodsComboBox));
    TimeoutComboBox->setValidator(new QIntValidator(TimeoutComboBox));
    
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
    m_Settings.endGroup();
    
    QPoint pt = pos();
    m_Settings.beginGroup("/Position");
    m_Settings.writeEntry("/x", pt.x());
    m_Settings.writeEntry("/y", pt.y());
    m_Settings.endGroup();

    m_Settings.endGroup();
}


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
    connect(m_pJack, SIGNAL(readyReadStdout()), this, SLOT(readJackStdout()));
    connect(m_pJack, SIGNAL(readyReadStderr()), this, SLOT(readJackStderr()));
    connect(m_pJack, SIGNAL(processExited()),   this, SLOT(processJackExit()));
    
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
}


void qjackctlMainForm::stopJack()
{    
    if (m_pJack) {
        appendMessages(tr("JACK is stopping..."));
        if (m_pJack->isRunning())
            m_pJack->tryTerminate();
        else
            processJackExit();
    }
}


void qjackctlMainForm::readJackStdout()
{
    QString s = m_pJack->readStdout();
    appendMessages(detectXrun(s));
}


void qjackctlMainForm::readJackStderr()
{
    QString s = m_pJack->readStderr();
    appendMessages(detectXrun(s));
}


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
            refreshXrunStats();
        }
        m_iXrunStats++;
    }
    return s;
}


void qjackctlMainForm::appendMessages( const QString & s )
{
    while (MessagesTextView->paragraphs() > 100) {
        MessagesTextView->removeParagraph(0);
        MessagesTextView->scrollToBottom();		
    }
    MessagesTextView->append(s);
}



void qjackctlMainForm::stabilizeForm()
{
    bool bShowDetails = DetailsCheckBox->isChecked();
    
    if (bShowDetails) {
        bool bEnabled = RealtimeCheckBox->isChecked();
        PriorityTextLabel->setEnabled(bEnabled);
        PriorityComboBox->setEnabled(bEnabled);
        ForceArtsShellComboBox->setEnabled(ForceArtsCheckBox->isChecked());
        ForceJackShellComboBox->setEnabled(ForceJackCheckBox->isChecked());
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
    } 
    else if (!m_posSave.isNull())
        move(m_posSave);
}


void qjackctlMainForm::resetXrunStats()
{
    m_iXrunStats = 0;
    m_iXrunCount = 0;
    m_fXrunTotal = 0.0;
    m_fXrunMin   = 0.0;
    m_fXrunMax   = 0.0;
    m_fXrunLast  = 0.0;    
    
    refreshXrunStats();
}


void qjackctlMainForm::refreshXrunStats()
{
    StatsListView->clear();
    float fXrunAverage = 0.0;
    if (m_iXrunCount > 0)
        fXrunAverage = (m_fXrunTotal / m_iXrunCount);
    QString s = " ";
    QString c = ":" + s;
    new QListViewItem(StatsListView, s + tr("XRUN count since last server startup") + c, QString::number(m_iXrunCount));
    new QListViewItem(StatsListView, s + tr("XRUN total (msecs)") + c, QString::number(m_fXrunTotal));
    new QListViewItem(StatsListView, s + tr("XRUN minimum (msecs)") + c, QString::number(m_fXrunMin));
    new QListViewItem(StatsListView, s + tr("XRUN maximum (msecs)") + c, QString::number(m_fXrunMax));
    new QListViewItem(StatsListView, s + tr("XRUN average (msecs)") + c, QString::number(fXrunAverage));
    new QListViewItem(StatsListView, s + tr("XRUN last (msecs)") + c, QString::number(m_fXrunLast));
    new QListViewItem(StatsListView, s + tr("XRUN last time detected") + c, m_tXrunLast.toString());
    StatsListView->triggerUpdate();
}
