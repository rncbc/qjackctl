// qjackctlGraphForm.h
//
/****************************************************************************
   Copyright (C) 2003-2018, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctlGraphForm_h
#define __qjackctlGraphForm_h

#include "ui_qjackctlGraphForm.h"


//----------------------------------------------------------------------------
// qjackctlGraphForm -- UI wrapper form.

class qjackctlGraphForm : public QMainWindow
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlGraphForm(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);
	// Destructor.
	~qjackctlGraphForm();

private:

	// The Qt-designer UI struct...
	Ui::qjackctlGraphForm m_ui;
};


#endif	// __qjackctlGraphForm_h


// end of qjackctlGraphForm.h
