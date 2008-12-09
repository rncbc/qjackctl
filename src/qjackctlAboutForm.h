// qjackctlAboutForm.h
//
/****************************************************************************
   Copyright (C) 2003-2008, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctlAboutForm_h
#define __qjackctlAboutForm_h

#include <QtGlobal>

#if QT_VERSION < 0x040200
#define setOpenExternalLinks(x) parent()
#endif

#include "ui_qjackctlAboutForm.h"


//----------------------------------------------------------------------------
// qjackctlAboutForm -- UI wrapper form.

class qjackctlAboutForm : public QDialog
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlAboutForm(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);
	// Destructor.
	~qjackctlAboutForm();

public slots:

	void aboutQt();

private:

	// The Qt-designer UI struct...
	Ui::qjackctlAboutForm m_ui;
};


#endif	// __qjackctlAboutForm_h


// end of qjackctlAboutForm.h
