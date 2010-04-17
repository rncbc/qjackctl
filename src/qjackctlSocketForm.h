// qjackctlSocketForm.h
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

#ifndef __qjackctlSocketForm_h
#define __qjackctlSocketForm_h

#include <QtGlobal>

#if QT_VERSION < 0x040200
#define setAllColumnsShowFocus(x) parent()
#endif

#include "ui_qjackctlSocketForm.h"

#include "qjackctlJackConnect.h"
#include "qjackctlAlsaConnect.h"

// Forward declarations.
class qjackctlPatchbay;
class qjackctlPatchbaySocket;
class qjackctlSocketList;

class QButtonGroup;
class QPixmap;


//----------------------------------------------------------------------------
// qjackctlSocketForm -- UI wrapper form.

class qjackctlSocketForm : public QDialog
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlSocketForm(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);
	// Destructor.
	~qjackctlSocketForm();

	void setSocketCaption(const QString& sSocketCaption);
	void setSocketList(qjackctlSocketList *pSocketList);

	void setPixmaps(QPixmap **ppPixmaps);

	void setConnectCount(int iConnectCount);

	void load(qjackctlPatchbaySocket *pSocket);
	void save(qjackctlPatchbaySocket *pSocket);

public slots:

	void changed();

	void addPlug();
	void editPlug();
	void removePlug();
	void moveUpPlug();
	void moveDownPlug();
	void selectedPlug();

	void activateAddPlugMenu(QAction *);

	void customContextMenu(const QPoint&);

	void socketTypeChanged();
	void clientNameChanged();

	void stabilizeForm();

protected slots:

	void accept();
	void reject();

protected:

	void updateJackClients(int iSocketType);
	void updateAlsaClients(int iSocketType);

	void updateJackPlugs(int iSocketType);
	void updateAlsaPlugs(int iSocketType);

	bool validateForm();

private:

	// The Qt-designer UI struct...
	Ui::qjackctlSocketForm m_ui;

	// Instance variables.
	qjackctlSocketList *m_pSocketList;
	QPixmap           **m_ppPixmaps;
	int                 m_iDirtyCount;

	QButtonGroup       *m_pSocketTypeButtonGroup;
};


#endif	// __qjackctlSocketForm_h


// end of qjackctlSocketForm.h
