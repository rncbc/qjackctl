// qjackctlInterfaceComboBox.h
//
/****************************************************************************
   Copyright (C) 2013, Arnout Engelen. All rights reserved.
   Copyright (C) 2003-2013, rncbc aka Rui Nuno Capela. All rights reserved.

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


#ifndef __qjackctlInterfaceComboBox_h
#define __qjackctlInterfaceComboBox_h

#include <QComboBox>


// Forward decls.
class qjackctlSetup;
class QStandardItemModel;


//----------------------------------------------------------------------------
// Combobox for device interfaces

class qjackctlInterfaceComboBox : public QComboBox
{
public:

	// Constructor.
	qjackctlInterfaceComboBox(QWidget *pPrent = 0);

	void setup(QComboBox *pDriverComboBox, int iAudio, const QString& sDefName);

protected:
	
	void clearCards();
	void addCard(const QString& sName, const QString& sDescription);
	void populateModel();
	void showPopup();

	QStandardItemModel *model() const;

private:

	QComboBox *m_pDriverComboBox;
	int        m_iAudio;
	QString    m_sDefName;
};


#endif 	// __qjackctlInterfaceComboBox_h


// end of qjackctlInterfaceComboBox.h
