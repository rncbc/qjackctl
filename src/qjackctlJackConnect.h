// qjackctlJackConnect.h
//
/****************************************************************************
   Copyright (C) 2003-2019, rncbc aka Rui Nuno Capela. All rights reserved.

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
	qjackctlJackPort(qjackctlJackClient *pClient, unsigned long ulPortFlags);
	// Default destructor.
	~qjackctlJackPort();

	// Pretty/display name method (virtual override).
	void updatePortName(bool bRename = false);

	// Tooltip text builder (virtual override).
	QString tooltip() const;
};


// Jack client list item.
class qjackctlJackClient : public qjackctlClientItem
{
public:

	// Constructor.
	qjackctlJackClient(qjackctlJackClientList *pClientList);
	// Default destructor.
	~qjackctlJackClient();

	// Jack port lookup.
	qjackctlJackPort *findJackPort(const QString& sPortName);

	// Pretty/display name method (virtual override).
	void updateClientName(bool bRename = false);
};


// Jack client list.
class qjackctlJackClientList : public qjackctlClientList
{
public:

	// Constructor.
	qjackctlJackClientList(qjackctlClientListView *pListView, bool bReadable);
	// Default destructor.
	~qjackctlJackClientList();

	// Jack port lookup.
	qjackctlJackPort *findJackClientPort(const QString& sClientPort);

	// Client:port refreshner (return newest item count).
	int updateClientPorts();

	// Jack client port aliases mode.
	static void setJackClientPortAlias(int iJackClientPortAlias);
	static int jackClientPortAlias();

	// Jack client port pretty-name (metadata) mode.
	static void setJackClientPortMetadata(bool bJackClientPortMetadata);
	static bool isJackClientPortMetadata();

private:

	// Jack client port aliases mode.
	static int g_iJackClientPortAlias;

	// Jack client port pretty-name (metadata) mode.
	static bool g_bJackClientPortMetadata;
};


//----------------------------------------------------------------------------
// qjackctlJackConnect -- Connections model integrated object.

class qjackctlJackConnect : public qjackctlConnect
{
public:

	// Constructor.
	qjackctlJackConnect(qjackctlConnectView *pConnectView, int iJackType);
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
