// qjackctlStatusForm.h
//
/****************************************************************************
   Copyright (C) 2003-2007, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctlStatusForm_h
#define __qjackctlStatusForm_h

#include "ui_qjackctlStatusForm.h"

// Forward declarations.
class QTreeWidgetItem;


//----------------------------------------------------------------------------
// qjackctlStatusForm -- UI wrapper form.

class qjackctlStatusForm : public QWidget
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlStatusForm(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);
	// Destructor.
	~qjackctlStatusForm();

	void updateStatusItem(int iStatusItem, const QString& sText);

public slots:

	void resetXrunStats();
	void refreshXrunStats();

protected:

	void showEvent(QShowEvent *);
	void hideEvent(QHideEvent *);

private:

	// The Qt-designer UI struct...
	Ui::qjackctlStatusForm m_ui;

	// Instance variables.
	QTreeWidgetItem *m_apStatus[18];
};


#endif	// __qjackctlStatusForm_h


// end of qjackctlStatusForm.h
