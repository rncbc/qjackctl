// qjackctlMessagesForm.ui.h
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

#include "config.h"

#include "qjackctlSetup.h"

// The maximum number of message lines.
#define QJACKCTL_MESSAGES_MAXLINES  1000


// Kind of constructor.
void qjackctlMessagesForm::init (void)
{
}


// Kind of destructor.
void qjackctlMessagesForm::destroy (void)
{
}


// Notify our parent that we're emerging.
void qjackctlMessagesForm::showEvent ( QShowEvent *pShowEvent )
{
    qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
    if (pMainForm)
        pMainForm->stabilizeForm();

    QWidget::showEvent(pShowEvent);
}

// Notify our parent that we're closing.
void qjackctlMessagesForm::hideEvent ( QHideEvent *pHideEvent )
{
    QWidget::hideEvent(pHideEvent);

    qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
    if (pMainForm)
        pMainForm->stabilizeForm();
}


// Messages view font accessors.
QFont qjackctlMessagesForm::messagesFont (void)
{
    return MessagesTextView->font();
}

void qjackctlMessagesForm::setMessagesFont ( const QFont & font )
{
    MessagesTextView->setFont(font);
}


// Messages widget output method.
void qjackctlMessagesForm::appendMessages( const QString& s )
{
    appendMessagesColor(s, "#999999");
}

void qjackctlMessagesForm::appendMessagesColor( const QString& s, const QString& c )
{
    appendMessagesText("<font color=\"" + c + "\">" + QTime::currentTime().toString("hh:mm:ss.zzz") + " " + s + "</font>");
}

void qjackctlMessagesForm::appendMessagesText( const QString& s )
{
    int iParagraphs = MessagesTextView->paragraphs();
    if (iParagraphs > QJACKCTL_MESSAGES_MAXLINES) {
        MessagesTextView->setUpdatesEnabled(false);
        while (iParagraphs > QJACKCTL_MESSAGES_MAXLINES) {
            MessagesTextView->removeParagraph(0);
            iParagraphs--;
        }
        MessagesTextView->scrollToBottom();
        MessagesTextView->setUpdatesEnabled(true);
    }
    MessagesTextView->append(s);
}


// end of qjackctlMessagesForm.ui.h

