// qjackctlJackConnect.h
//
/****************************************************************************
   Copyright (C) 2003-2005, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*****************************************************************************/

#ifndef __qjackctlJackConnect_h
#define __qjackctlJackConnect_h

#include "qjackctlConnect.h"

#include <jack/jack.h>

// Forward declarations.
class qjackctlJackPort;
class qjackctlJackClient;
class qjackctlJackClientList;
class qjackctlJackConnect;

// Pixmap-set array indexes.
#define QJACKCTL_XPM_ACLIENTI   0   // Input client item pixmap.
#define QJACKCTL_XPM_ACLIENTO   1   // Output client item pixmap.
#define QJACKCTL_XPM_APORTPTI   2   // Physcal Terminal Input port pixmap.
#define QJACKCTL_XPM_APORTPTO   3   // Physical Terminal Output port pixmap.
#define QJACKCTL_XPM_APORTPNI   4   // Physical Non-terminal Input port pixmap.
#define QJACKCTL_XPM_APORTPNO   5   // Physical Non-terminal Output port pixmap.
#define QJACKCTL_XPM_APORTLTI   6   // Logical Terminal Input port pixmap.
#define QJACKCTL_XPM_APORTLTO   7   // Logical Terminal Output port pixmap.
#define QJACKCTL_XPM_APORTLNI   8   // Logical Non-terminal Input port pixmap.
#define QJACKCTL_XPM_APORTLNO   9   // Logical Non-terminal Output port pixmap.
#define QJACKCTL_XPM_APIXMAPS  10   // Number of pixmaps in local array.

// Jack port list item.
class qjackctlJackPort : public qjackctlPortItem
{
public:

    // Constructor.
    qjackctlJackPort(qjackctlJackClient *pClient, const QString& sPortName, jack_port_t *pJackPort);
    // Default destructor.
    ~qjackctlJackPort();

    // Jack handles accessors.
    jack_client_t *jackClient();
    jack_port_t   *jackPort();

private:

    // Instance variables.
    jack_port_t *m_pJackPort;
};


// Jack client list item.
class qjackctlJackClient : public qjackctlClientItem
{
public:

    // Constructor.
    qjackctlJackClient(qjackctlJackClientList *pClientList, const QString& sClientName);
    // Default destructor.
    ~qjackctlJackClient();

    // Jack client accessors.
    jack_client_t *jackClient();
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
    jack_client_t *jackClient();

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
    qjackctlJackConnect(qjackctlConnectView *pConnectView, jack_client_t *pJackClient);
    // Default destructor.
    ~qjackctlJackConnect();

    // Common pixmap accessor.
    static QPixmap& pixmap (int iPixmap);
    
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

    // Local pixmap-set array.
    static QPixmap *g_apPixmaps[QJACKCTL_XPM_APIXMAPS];
};


#endif  // __qjackctlJackConnect_h

// end of qjackctlJackConnect.h
