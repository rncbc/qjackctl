// qjackctlJackConnect.h
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

#ifndef __qjackctlJackConnect_h
#define __qjackctlJackConnect_h

#include "qjackctlAbout.h"
#include "qjackctlConnect.h"

#include <jack/jack.h>

// Forward declarations.
class qjackctlJackPort;
class qjackctlJackClient;
class qjackctlJackClientList;
class qjackctlJackConnect;

// Connection port type.
#define QJACKCTL_JACK_AUDIO		0
#define QJACKCTL_JACK_MIDI		1

// Pixmap-set array indexes/types.
#define QJACKCTL_JACK_CLIENTI	0	// Input client item pixmap.
#define QJACKCTL_JACK_CLIENTO	1	// Output client item pixmap.
#define QJACKCTL_JACK_PORTPTI	2	// Physcal Terminal Input port pixmap.
#define QJACKCTL_JACK_PORTPTO	3	// Physical Terminal Output port pixmap.
#define QJACKCTL_JACK_PORTPNI	4	// Physical Non-terminal Input port pixmap.
#define QJACKCTL_JACK_PORTPNO	5	// Physical Non-terminal Output port pixmap.
#define QJACKCTL_JACK_PORTLTI	6	// Logical Terminal Input port pixmap.
#define QJACKCTL_JACK_PORTLTO	7	// Logical Terminal Output port pixmap.
#define QJACKCTL_JACK_PORTLNI	8	// Logical Non-terminal Input port pixmap.
#define QJACKCTL_JACK_PORTLNO	9	// Logical Non-terminal Output port pixmap.
#define QJACKCTL_JACK_PIXMAPS	10	// Number of pixmaps in array.


// Jack port list item.
class qjackctlJackPort : public qjackctlPortItem
{
public:

	// Constructor.
	qjackctlJackPort(qjackctlJackClient *pClient,
		const QString& sPortName, jack_port_t *pJackPort);
	// Default destructor.
	~qjackctlJackPort();

	// Jack handles accessors.
	jack_client_t *jackClient() const;
	jack_port_t   *jackPort() const;

private:

	// Instance variables.
	jack_port_t *m_pJackPort;
};


// Jack client list item.
class qjackctlJackClient : public qjackctlClientItem
{
public:

	// Constructor.
	qjackctlJackClient(qjackctlJackClientList *pClientList,
		const QString& sClientName);
	// Default destructor.
	~qjackctlJackClient();

	// Jack client accessors.
	jack_client_t *jackClient() const;
};


// Jack client list.
class qjackctlJackClientList : public qjackctlClientList
{
public:

	// Constructor.
	qjackctlJackClientList(qjackctlClientListView *pListView, jack_client_t *pJackClient, bool bReadable);
	// Default destructor.
	~qjackctlJackClientList();

	// Jack client accessors.
	jack_client_t *jackClient() const;

	// Client:port refreshner (return newest item count).
	int updateClientPorts();

private:

	// Instance variables.
	jack_client_t *m_pJackClient;
};


//----------------------------------------------------------------------------
// qjackctlJackConnect -- Connections model integrated object.

class qjackctlJackConnect : public qjackctlConnect
{
public:

	// Constructor.
	qjackctlJackConnect(qjackctlConnectView *pConnectView,
		jack_client_t *pJackClient, int iJackType);
	// Default destructor.
	~qjackctlJackConnect();

	// Connection type accessors.
	int jackType() const;

	// Common pixmap accessor.
	const QPixmap& pixmap (int iPixmap) const;
	
protected:

	// Virtual Connect/Disconnection primitives.
	bool connectPorts    (qjackctlPortItem *pOPort, qjackctlPortItem *pIPort);
	bool disconnectPorts (qjackctlPortItem *pOPort, qjackctlPortItem *pIPort);

	// Update port connection references.
	void updateConnections();

	// Update icon size implementation.
	void updateIconPixmaps();

private:

	// Local pixmap-set janitor methods.
	void createIconPixmaps();
	void deleteIconPixmaps();

	// Local variables.
	int m_iJackType;

	// Local pixmap-set array.
	QPixmap *m_apPixmaps[QJACKCTL_JACK_PIXMAPS];
};


#endif  // __qjackctlJackConnect_h

// end of qjackctlJackConnect.h
