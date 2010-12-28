// qjackctlMessagesStatusForm.cpp
//
/****************************************************************************
   Copyright (C) 2003-2010, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include "qjackctlMessagesStatusForm.h"

#include "qjackctlStatus.h"
#include "qjackctlSetup.h"

#include "qjackctlMainForm.h"

#include <QFile>
#include <QDateTime>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextStream>
#include <QHeaderView>

#include <QShowEvent>
#include <QHideEvent>


// The maximum number of message lines.
#define QJACKCTL_MESSAGES_MAXLINES  1000


//----------------------------------------------------------------------------
// qjackctlMessagesStatusForm -- UI wrapper form.

// Constructor.
qjackctlMessagesStatusForm::qjackctlMessagesStatusForm (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QWidget(pParent, wflags)
{
	// Setup UI struct...
	m_ui.setupUi(this);

//  m_ui.MessagesTextView->setTextFormat(Qt::LogText);

	// Initialize default message limit.
	m_iMessagesLines = 0;
	setMessagesLimit(QJACKCTL_MESSAGES_MAXLINES);

	m_pMessagesLog = NULL;

	// Create the list view items 'a priori'...
	const QString s = " ";
	const QString c = ":" + s;
	const QString n = "--";
	QTreeWidgetItem *pViewItem;

	// Status list view...
	QHeaderView *pHeader = m_ui.StatsListView->header();
//	pHeader->setResizeMode(QHeaderView::Custom);
	pHeader->setDefaultAlignment(Qt::AlignLeft);
//	pHeader->setDefaultSectionSize(320);
	pHeader->setMovable(false);
	pHeader->setStretchLastSection(true);

	m_apStatus[STATUS_SERVER_NAME] = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("Server name") + c << n);
	m_apStatus[STATUS_SERVER_STATE] = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("Server state") + c << n);
	m_apStatus[STATUS_DSP_LOAD]     = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("DSP Load") + c << n);
	m_apStatus[STATUS_SAMPLE_RATE]  = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("Sample Rate") + c << n);
	m_apStatus[STATUS_BUFFER_SIZE]  = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("Buffer Size") + c << n);
	m_apStatus[STATUS_REALTIME]     = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("Realtime Mode") + c << n);

	pViewItem = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("Transport state") + c << n);
	m_apStatus[STATUS_TRANSPORT_STATE] = pViewItem;
	m_apStatus[STATUS_TRANSPORT_TIME] = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("Transport Timecode") + c << n);
	m_apStatus[STATUS_TRANSPORT_BBT]  = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("Transport BBT") + c << n);
	m_apStatus[STATUS_TRANSPORT_BPM]  = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("Transport BPM") + c << n);

	pViewItem = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("XRUN count since last server startup") + c << n);
	m_apStatus[STATUS_XRUN_COUNT] = pViewItem;
	m_apStatus[STATUS_XRUN_TIME]  = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("XRUN last time detected") + c << n);
	m_apStatus[STATUS_XRUN_LAST]  = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("XRUN last") + c << n);
	m_apStatus[STATUS_XRUN_MAX]   = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("XRUN maximum") + c << n);
	m_apStatus[STATUS_XRUN_MIN]   = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("XRUN minimum") + c << n);
	m_apStatus[STATUS_XRUN_AVG]   = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("XRUN average") + c << n);
	m_apStatus[STATUS_XRUN_TOTAL] = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("XRUN total") + c << n);

#ifdef CONFIG_JACK_MAX_DELAY
	m_apStatus[STATUS_MAX_DELAY]  = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("Maximum scheduling delay") + c << n);
#endif
	m_apStatus[STATUS_RESET_TIME] = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("Time of last reset") + c << n);

	m_ui.StatsListView->resizeColumnToContents(0);	// Description.
	m_ui.StatsListView->resizeColumnToContents(1);	// Value.

	// UI connections...

	QObject::connect(m_ui.ResetPushButton,
		SIGNAL(clicked()),
		SLOT(resetXrunStats()));
	QObject::connect(m_ui.RefreshPushButton,
		SIGNAL(clicked()),
		SLOT(refreshXrunStats()));
}


// Destructor.
qjackctlMessagesStatusForm::~qjackctlMessagesStatusForm (void)
{
	setLogging(false);
}


// Notify our parent that we're emerging.
void qjackctlMessagesStatusForm::showEvent ( QShowEvent *pShowEvent )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();

	QWidget::showEvent(pShowEvent);
}

// Notify our parent that we're closing.
void qjackctlMessagesStatusForm::hideEvent ( QHideEvent *pHideEvent )
{
	QWidget::hideEvent(pHideEvent);

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}

// Just about to notify main-window that we're closing.
void qjackctlMessagesStatusForm::closeEvent ( QCloseEvent * /*pCloseEvent*/ )
{
	QWidget::hide();

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}


// Tab page accessors.
void qjackctlMessagesStatusForm::setTabPage ( int iTabPage )
{
	m_ui.MessagesStatusTabWidget->setCurrentIndex(iTabPage);
}

int qjackctlMessagesStatusForm::tabPage (void) const
{
	return m_ui.MessagesStatusTabWidget->currentIndex();
}


// Messages view font accessors.
QFont qjackctlMessagesStatusForm::messagesFont (void) const
{
	return m_ui.MessagesTextView->font();
}

void qjackctlMessagesStatusForm::setMessagesFont ( const QFont & font )
{
	m_ui.MessagesTextView->setFont(font);
}


// Messages line limit accessors.
int qjackctlMessagesStatusForm::messagesLimit (void) const
{
	return m_iMessagesLimit;
}

void qjackctlMessagesStatusForm::setMessagesLimit ( int iMessagesLimit )
{
	m_iMessagesLimit = iMessagesLimit;
	m_iMessagesHigh  = iMessagesLimit + (iMessagesLimit / 3);

//	m_ui.MessagesTextView->setMaxLogLines(iMessagesLimit);
}


// Messages logging stuff.
bool qjackctlMessagesStatusForm::isLogging (void) const
{
	return (m_pMessagesLog != NULL);
}

void qjackctlMessagesStatusForm::setLogging ( bool bEnabled, const QString& sFilename )
{
	if (m_pMessagesLog) {
		appendMessages(tr("Logging stopped --- %1 ---")
			.arg(QDateTime::currentDateTime().toString()));
		m_pMessagesLog->close();
		delete m_pMessagesLog;
		m_pMessagesLog = NULL;
	}

	if (bEnabled) {
		m_pMessagesLog = new QFile(sFilename);
		if (m_pMessagesLog->open(QIODevice::Text | QIODevice::Append)) {
			appendMessages(tr("Logging started --- %1 ---")
				.arg(QDateTime::currentDateTime().toString()));
		} else {
			delete m_pMessagesLog;
			m_pMessagesLog = NULL;
		}
	}
}


// Messages log output method.
void qjackctlMessagesStatusForm::appendMessagesLog ( const QString& s )
{
	if (m_pMessagesLog) {
		QTextStream(m_pMessagesLog) << s << endl;
		m_pMessagesLog->flush();
	}
}

// Messages widget output method.
void qjackctlMessagesStatusForm::appendMessagesLine ( const QString& s )
{
	// Check for message line limit...
	if (m_iMessagesLines > m_iMessagesHigh) {
		m_ui.MessagesTextView->setUpdatesEnabled(false);
		QTextCursor textCursor(m_ui.MessagesTextView->document()->begin());
		while (m_iMessagesLines > m_iMessagesLimit) {
			// Move cursor extending selection
			// from start to next line-block...
			textCursor.movePosition(
				QTextCursor::NextBlock, QTextCursor::KeepAnchor);
			m_iMessagesLines--;
		}
		// Remove the excessive line-blocks...
		textCursor.removeSelectedText();
		m_ui.MessagesTextView->setUpdatesEnabled(true);
	}

	m_ui.MessagesTextView->append(s);
	m_iMessagesLines++;
}


void qjackctlMessagesStatusForm::appendMessages ( const QString& s )
{
	appendMessagesColor(s, "#999999");
}

void qjackctlMessagesStatusForm::appendMessagesColor ( const QString& s, const QString& c )
{
	QString sText = QTime::currentTime().toString("hh:mm:ss.zzz") + ' ' + s;
	
	appendMessagesLine("<font color=\"" + c + "\">" + sText + "</font>");
	appendMessagesLog(sText);
}

void qjackctlMessagesStatusForm::appendMessagesText ( const QString& s )
{
	appendMessagesLine(s);
	appendMessagesLog(s);
}


// Ask our parent to reset status.
void qjackctlMessagesStatusForm::resetXrunStats (void)
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->resetXrunStats();
}

// Ask our parent to refresh our status.
void qjackctlMessagesStatusForm::refreshXrunStats (void)
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->refreshXrunStats();
}


// Update one status item value.
void qjackctlMessagesStatusForm::updateStatusItem ( int iStatusItem,
	const QString& sText )
{
	m_apStatus[iStatusItem]->setText(1, sText);
}


// Keyboard event handler.
void qjackctlMessagesStatusForm::keyPressEvent ( QKeyEvent *pKeyEvent )
{
#ifdef CONFIG_DEBUG_0
	qDebug("qjackctlMessagesStatusForm::keyPressEvent(%d)", pKeyEvent->key());
#endif
	int iKey = pKeyEvent->key();
	switch (iKey) {
	case Qt::Key_Escape:
		close();
		break;
	default:
		QWidget::keyPressEvent(pKeyEvent);
		break;
	}
}


// end of qjackctlMessagesStatusForm.cpp
