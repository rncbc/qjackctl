// qjackctlMessagesForm.cpp
//
/****************************************************************************
   Copyright (C) 2003-2009, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include "qjackctlMessagesForm.h"

#include "qjackctlMainForm.h"

#include <QFile>
#include <QDateTime>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextStream>

#include <QShowEvent>
#include <QHideEvent>


// The maximum number of message lines.
#define QJACKCTL_MESSAGES_MAXLINES  1000


//----------------------------------------------------------------------------
// qjackctlMessagesForm -- UI wrapper form.

// Constructor.
qjackctlMessagesForm::qjackctlMessagesForm (
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
}


// Destructor.
qjackctlMessagesForm::~qjackctlMessagesForm (void)
{
	setLogging(false);
}


// Notify our parent that we're emerging.
void qjackctlMessagesForm::showEvent ( QShowEvent *pShowEvent )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();

	QWidget::showEvent(pShowEvent);
}

// Notify our parent that we're closing.
void qjackctlMessagesForm::hideEvent ( QHideEvent *pHideEvent )
{
	QWidget::hideEvent(pHideEvent);

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}

// Just about to notify main-window that we're closing.
void qjackctlMessagesForm::closeEvent ( QCloseEvent * /*pCloseEvent*/ )
{
	QWidget::hide();

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}


// Messages view font accessors.
QFont qjackctlMessagesForm::messagesFont (void) const
{
	return m_ui.MessagesTextView->font();
}

void qjackctlMessagesForm::setMessagesFont ( const QFont & font )
{
	m_ui.MessagesTextView->setFont(font);
}


// Messages line limit accessors.
int qjackctlMessagesForm::messagesLimit (void) const
{
	return m_iMessagesLimit;
}

void qjackctlMessagesForm::setMessagesLimit ( int iMessagesLimit )
{
	m_iMessagesLimit = iMessagesLimit;
	m_iMessagesHigh  = iMessagesLimit + (iMessagesLimit / 3);

//	m_ui.MessagesTextView->setMaxLogLines(iMessagesLimit);
}


// Messages logging stuff.
bool qjackctlMessagesForm::isLogging (void) const
{
	return (m_pMessagesLog != NULL);
}

void qjackctlMessagesForm::setLogging ( bool bEnabled, const QString& sFilename )
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
void qjackctlMessagesForm::appendMessagesLog ( const QString& s )
{
	if (m_pMessagesLog) {
		QTextStream(m_pMessagesLog) << s << endl;
		m_pMessagesLog->flush();
	}
}

// Messages widget output method.
void qjackctlMessagesForm::appendMessagesLine ( const QString& s )
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


void qjackctlMessagesForm::appendMessages ( const QString& s )
{
	appendMessagesColor(s, "#999999");
}

void qjackctlMessagesForm::appendMessagesColor ( const QString& s, const QString& c )
{
	QString sText = QTime::currentTime().toString("hh:mm:ss.zzz") + ' ' + s;
	
	appendMessagesLine("<font color=\"" + c + "\">" + sText + "</font>");
	appendMessagesLog(sText);
}

void qjackctlMessagesForm::appendMessagesText ( const QString& s )
{
	appendMessagesLine(s);
	appendMessagesLog(s);
}


// Keyboard event handler.
void qjackctlMessagesForm::keyPressEvent ( QKeyEvent *pKeyEvent )
{
#ifdef CONFIG_DEBUG_0
	qDebug("qjackctlMessagesForm::keyPressEvent(%d)", pKeyEvent->key());
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

	// Make sure we've get focus back...
	QWidget::setFocus();
}


// end of qjackctlMessagesForm.cpp
