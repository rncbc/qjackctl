// qjackctlMessagesStatusForm.h
//
/****************************************************************************
   Copyright (C) 2003-2021, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctlMessagesStatusForm_h
#define __qjackctlMessagesStatusForm_h

#include "ui_qjackctlMessagesStatusForm.h"

// Forward declarations.
class QTreeWidgetItem;
class QFile;


//----------------------------------------------------------------------------
// qjackctlMessagesStatusForm -- UI wrapper form.

class qjackctlMessagesStatusForm : public QWidget
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlMessagesStatusForm(QWidget *pParent = nullptr,
		Qt::WindowFlags wflags = Qt::WindowFlags());
	// Destructor.
	~qjackctlMessagesStatusForm();

	enum TabPage { MessagesTab = 0, StatusTab = 1 };

	void setTabPage(int iTabPage);
	int tabPage() const;

	QFont messagesFont() const;
	void setMessagesFont(const QFont& font);

	int messagesLimit() const;
	void setMessagesLimit(int iLimit);

	bool isLogging() const;
	void setLogging(bool bEnabled, const QString& sFilename = QString());

	void appendMessages(const QString& s);
	void appendMessagesColor(const QString& s, const QColor& rgb);
	void appendMessagesText(const QString& s);

	void updateStatusItem(int iStatusItem, const QString& sText);

public slots:

	void resetXrunStats();
	void refreshXrunStats();

protected:

	void appendMessagesLine(const QString& s);
	void appendMessagesLog(const QString& s);

	void showEvent(QShowEvent *);
	void hideEvent(QHideEvent *);

	void keyPressEvent(QKeyEvent *);

private:

	// The Qt-designer UI struct...
	Ui::qjackctlMessagesStatusForm m_ui;

	// Instance variables.
	int m_iMessagesLines;
	int m_iMessagesLimit;
	int m_iMessagesHigh;

	// Logging stuff.
	QFile *m_pMessagesLog;

	// Status stuff.
	QTreeWidgetItem *m_apStatus[19];
};


#endif	// __qjackctlMessagesStatusForm_h


// end of qjackctlMessagesStatusForm.h
