// qjackctlAlsaConnect.h
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

#ifndef __qjackctlAlsaConnect_h
#define __qjackctlAlsaConnect_h

#include "qjackctlAbout.h"
#include "qjackctlConnect.h"

#ifdef CONFIG_ALSA_SEQ
#include <alsa/asoundlib.h>
#else
typedef void snd_seq_t;
#endif

// Forward declarations.
class qjackctlAlsaPort;
class qjackctlAlsaClient;
class qjackctlAlsaClientList;
class qjackctlAlsaConnect;

// Pixmap-set array indexes.
#define QJACKCTL_ALSA_CLIENTO	0	// Output client item pixmap.
#define QJACKCTL_ALSA_CLIENTI	1	// Input client item pixmap.
#define QJACKCTL_ALSA_PORTO		2	// Output port pixmap.
#define QJACKCTL_ALSA_PORTI		3	// Input port pixmap.
#define QJACKCTL_ALSA_PIXMAPS	4	// Number of pixmaps in local array.


// Jack port list item.
class qjackctlAlsaPort : public qjackctlPortItem
{
public:

	// Constructor.
	qjackctlAlsaPort(qjackctlAlsaClient *pClient,
		const QString& sPortName, int iAlsaPort);
	// Default destructor.
	~qjackctlAlsaPort();

	// Jack handles accessors.
	int alsaClient() const;
	int alsaPort() const;

private:

	// Instance variables.
	int m_iAlsaPort;
};


// Jack client list item.
class qjackctlAlsaClient : public qjackctlClientItem
{
public:

	// Constructor.
	qjackctlAlsaClient(qjackctlAlsaClientList *pClientList,
		const QString& sClientName, int iAlsaClient);
	// Default destructor.
	~qjackctlAlsaClient();

	// Jack client accessors.
	int alsaClient() const;

	// Port finder by id.
	qjackctlAlsaPort *findPort(int iAlsaPort);

private:

	// Instance variables.
	int m_iAlsaClient;
};


// Jack client list.
class qjackctlAlsaClientList : public qjackctlClientList
{
public:

	// Constructor.
	qjackctlAlsaClientList(qjackctlClientListView *pListView,
		snd_seq_t *pAlsaSeq, bool bReadable);
	// Default destructor.
	~qjackctlAlsaClientList();

	// Alsa sequencer accessor.
	snd_seq_t *alsaSeq() const;

	// Client finder by id.
	qjackctlAlsaClient *findClient(int iAlsaClient);
	// Client port finder by id.
	qjackctlAlsaPort *findClientPort(int iAlsaClient, int iAlsaPort);

	// Client:port refreshner (return newest item count).
	int updateClientPorts();

private:

	// Instance variables.
	snd_seq_t *m_pAlsaSeq;
};


//----------------------------------------------------------------------------
// qjackctlAlsaConnect -- Connections model integrated object.

class qjackctlAlsaConnect : public qjackctlConnect
{
public:

	// Constructor.
	qjackctlAlsaConnect(qjackctlConnectView *pConnectView,
		snd_seq_t *pAlsaSeq);
	// Default destructor.
	~qjackctlAlsaConnect();

	// Alsa sequencer accessor.
	snd_seq_t *alsaSeq() const;

	// Common pixmap accessor.
	QPixmap& pixmap(int iPixmap) const;

protected:

	// Virtual Connect/Disconnection primitives.
	void connectPorts    (qjackctlPortItem *pOPort, qjackctlPortItem *pIPort);
	void disconnectPorts (qjackctlPortItem *pOPort, qjackctlPortItem *pIPort);

	// Update port connection references.
	void updateConnections();

	// Update icon size implementation.
	void updateIconPixmaps();

private:

	// Local pixmap-set janitor methods.
	void createIconPixmaps();
	void deleteIconPixmaps();

	// Instance variables.
	snd_seq_t *m_pAlsaSeq;

	// Local pixmap-set array.
	QPixmap *m_apPixmaps[QJACKCTL_ALSA_PIXMAPS];
};


#endif  // __qjackctlAlsaConnect_h

// end of qjackctlAlsaConnect.h
