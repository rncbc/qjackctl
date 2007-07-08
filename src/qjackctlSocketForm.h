// qjackctlSocketForm.h
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

#ifndef __qjackctlSocketForm_h
#define __qjackctlSocketForm_h

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

	void setJackClient(jack_client_t *pJackClient);
	void setAlsaSeq(snd_seq_t *pAlsaSeq);

	void setConnectCount(int iConnectCount);

	void load(qjackctlPatchbaySocket *pSocket);
	void save(qjackctlPatchbaySocket *pSocket);

public slots:

	void addPlug();
	void editPlug();
	void removePlug();
	void moveUpPlug();
	void moveDownPlug();
	void selectedPlug();

	void activateAddPlugMenu(QAction *);

	void contextMenu(const QPoint&);

	void socketTypeChanged();
	void clientNameChanged();

	void stabilizeForm();

protected slots:

	void accept();
	void reject();

protected:

	bool validateForm();

private:

	// The Qt-designer UI struct...
	Ui::qjackctlSocketForm m_ui;

	// Instance variables.
	qjackctlSocketList *m_pSocketList;
	jack_client_t      *m_pJackClient;
	snd_seq_t          *m_pAlsaSeq;
	QPixmap           **m_ppPixmaps;

	QButtonGroup       *m_pSocketTypeButtonGroup;
};


#endif	// __qjackctlSocketForm_h


// end of qjackctlSocketForm.h
