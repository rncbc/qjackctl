// qjackctlJackConnect.cpp
//
/****************************************************************************
   Copyright (C) 2003, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qjackctlJackConnect.h"

// Local pixmaps.
#include "icons/clienti.xpm"
#include "icons/cliento.xpm"
#include "icons/portpti.xpm"
#include "icons/portpto.xpm"
#include "icons/portpni.xpm"
#include "icons/portpno.xpm"
#include "icons/portlti.xpm"
#include "icons/portlto.xpm"
#include "icons/portlni.xpm"
#include "icons/portlno.xpm"

static int g_iXpmRefCount = 0;

static QPixmap *g_pXpmClientI = 0;  // Input client item pixmap.
static QPixmap *g_pXpmClientO = 0;  // Output client item pixmap.
static QPixmap *g_pXpmPortPTI = 0;  // Physcal Terminal Input port pixmap.
static QPixmap *g_pXpmPortPTO = 0;  // Physical Terminal Output port pixmap.
static QPixmap *g_pXpmPortPNI = 0;  // Physical Non-terminal Input port pixmap.
static QPixmap *g_pXpmPortPNO = 0;  // Physical Non-terminal Output port pixmap.
static QPixmap *g_pXpmPortLTI = 0;  // Logical Terminal Input port pixmap.
static QPixmap *g_pXpmPortLTO = 0;  // Logical Terminal Output port pixmap.
static QPixmap *g_pXpmPortLNI = 0;  // Logical Non-terminal Input port pixmap.
static QPixmap *g_pXpmPortLNO = 0;  // Logical Non-terminal Output port pixmap.


//----------------------------------------------------------------------
// class qjackctlJackPort -- Jack port list item.
//

// Constructor.
qjackctlJackPort::qjackctlJackPort ( qjackctlJackClient *pClient, const QString& sPortName, jack_port_t *pJackPort )
    : qjackctlPortItem(pClient, sPortName)
{
    m_pJackPort = pJackPort;

    unsigned long ulPortFlags = jack_port_flags(m_pJackPort);
    if (ulPortFlags & JackPortIsInput) {
        if (ulPortFlags & JackPortIsTerminal) {
            QListViewItem::setPixmap(0, (ulPortFlags & JackPortIsPhysical ? *g_pXpmPortPTI : *g_pXpmPortLTI));
        } else {
            QListViewItem::setPixmap(0, (ulPortFlags & JackPortIsPhysical ? *g_pXpmPortPNI : *g_pXpmPortLNI));
        }
    } else if (ulPortFlags & JackPortIsOutput) {
        if (ulPortFlags & JackPortIsTerminal) {
            QListViewItem::setPixmap(0, (ulPortFlags & JackPortIsPhysical ? *g_pXpmPortPTO : *g_pXpmPortLTO));
        } else {
            QListViewItem::setPixmap(0, (ulPortFlags & JackPortIsPhysical ? *g_pXpmPortPNO : *g_pXpmPortLNO));
        }
    }
}

// Default destructor.
qjackctlJackPort::~qjackctlJackPort (void)
{
}


// Jack handles accessors.
jack_client_t *qjackctlJackPort::jackClient (void)
{
    return ((qjackctlJackClient *) client())->jackClient();
}

jack_port_t *qjackctlJackPort::jackPort (void)
{
    return m_pJackPort;
}


//----------------------------------------------------------------------
// class qjackctlJackClient -- Jack client list item.
//

// Constructor.
qjackctlJackClient::qjackctlJackClient ( qjackctlJackClientList *pClientList, const QString& sClientName )
    : qjackctlClientItem(pClientList, sClientName)
{
    if (pClientList->isReadable()) {
        QListViewItem::setPixmap(0, *g_pXpmClientO);
    } else {
        QListViewItem::setPixmap(0, *g_pXpmClientI);
    }
}

// Default destructor.
qjackctlJackClient::~qjackctlJackClient (void)
{
}


// Jack client accessor.
jack_client_t *qjackctlJackClient::jackClient (void)
{
    return ((qjackctlJackClientList *) clientlist())->jackClient();
}


//----------------------------------------------------------------------
// qjackctlJackClientList -- Jack client list.
//

// Constructor.
qjackctlJackClientList::qjackctlJackClientList( qjackctlClientListView *pListView, jack_client_t *pJackClient, bool bReadable )
    : qjackctlClientList(pListView, bReadable)
{
    if (g_iXpmRefCount == 0) {
        g_pXpmClientI = new QPixmap((const char **) clienti_xpm);
        g_pXpmClientO = new QPixmap((const char **) cliento_xpm);
        g_pXpmPortPTI = new QPixmap((const char **) portpti_xpm);
        g_pXpmPortPTO = new QPixmap((const char **) portpto_xpm);
        g_pXpmPortPNI = new QPixmap((const char **) portpni_xpm);
        g_pXpmPortPNO = new QPixmap((const char **) portpno_xpm);
        g_pXpmPortLTI = new QPixmap((const char **) portlti_xpm);
        g_pXpmPortLTO = new QPixmap((const char **) portlto_xpm);
        g_pXpmPortLNI = new QPixmap((const char **) portlni_xpm);
        g_pXpmPortLNO = new QPixmap((const char **) portlno_xpm);
    }
    g_iXpmRefCount++;

    m_pJackClient = pJackClient;
}

// Default destructor.
qjackctlJackClientList::~qjackctlJackClientList (void)
{
    if (--g_iXpmRefCount == 0) {
        delete g_pXpmClientI;
        delete g_pXpmClientO;
        delete g_pXpmPortPTI;
        delete g_pXpmPortPTO;
        delete g_pXpmPortPNI;
        delete g_pXpmPortPNO;
        delete g_pXpmPortLTI;
        delete g_pXpmPortLTO;
        delete g_pXpmPortLNI;
        delete g_pXpmPortLNO;
    }
}


// Jack client accessor.
jack_client_t *qjackctlJackClientList::jackClient (void)
{
    return m_pJackClient;
}


// Client:port refreshner.
int qjackctlJackClientList::updateClientPorts (void)
{
    if (m_pJackClient == 0)
        return 0;

    int iDirtyCount = 0;
    
    markClientPorts(0);

    const char **ppszClientPorts = jack_get_ports(m_pJackClient, 0, 0, isReadable() ? JackPortIsOutput : JackPortIsInput);
    if (ppszClientPorts) {
        int iClientPort = 0;
        while (ppszClientPorts[iClientPort]) {
            jack_port_t *pJackPort = jack_port_by_name(m_pJackClient, ppszClientPorts[iClientPort]);
            QString sClientPort = ppszClientPorts[iClientPort];
            qjackctlJackClient *pClient = 0;
            qjackctlJackPort   *pPort   = 0;
            int iColon = sClientPort.find(":");
            if (iColon >= 0) {
                QString sClientName = sClientPort.left(iColon);
                QString sPortName   = sClientPort.right(sClientPort.length() - iColon - 1);
                pClient = (qjackctlJackClient *) findClient(sClientName);
                if (pClient)
                    pPort = (qjackctlJackPort *) pClient->findPort(sPortName);
                if (pClient == 0) {
                    pClient = new qjackctlJackClient(this, sClientName);
                    iDirtyCount++;
                }
                if (pClient && pPort == 0) {
                    pPort = new qjackctlJackPort(pClient, sPortName, pJackPort);
                    iDirtyCount++;
                }
                if (pPort)
                    pPort->markClientPort(1);
            }
            iClientPort++;
        }
        ::free(ppszClientPorts);
    }

    cleanClientPorts(0);
    
    return iDirtyCount;
}


//----------------------------------------------------------------------
// qjackctlJackConnect -- Output-to-Input client/ports connection object.
//

// Constructor.
qjackctlJackConnect::qjackctlJackConnect ( qjackctlConnectView *pConnectView, jack_client_t *pJackClient )
    : qjackctlConnect(pConnectView)
{
    setOClientList(new qjackctlJackClientList(connectView()->OListView(), pJackClient, true));
    setIClientList(new qjackctlJackClientList(connectView()->IListView(), pJackClient, false));
}

// Default destructor.
qjackctlJackConnect::~qjackctlJackConnect (void)
{
}


// Connection primitive.
void qjackctlJackConnect::connectPorts ( qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
    qjackctlJackPort *pOJack = (qjackctlJackPort *) pOPort;
    qjackctlJackPort *pIJack = (qjackctlJackPort *) pIPort;

    jack_connect(pOJack->jackClient(), pOJack->clientPortName().latin1(), pIJack->clientPortName().latin1());
}


// Disconnection primitive.
void qjackctlJackConnect::disconnectPorts ( qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
    qjackctlJackPort *pOJack = (qjackctlJackPort *) pOPort;
    qjackctlJackPort *pIJack = (qjackctlJackPort *) pIPort;

    jack_disconnect(pOJack->jackClient(), pOJack->clientPortName().latin1(), pIJack->clientPortName().latin1());
}


// Update port connection references.
void qjackctlJackConnect::updateConnections (void)
{
    // For each output client item...
    for (qjackctlClientItem *pOClient = OClientList()->clients().first();
            pOClient;
                pOClient = OClientList()->clients().next()) {
        // For each output port item...
        for (qjackctlPortItem *pOPort = pOClient->ports().first();
                pOPort;
                    pOPort = pOClient->ports().next()) {
            // Are there already any connections?
            if (pOPort->connects().count() > 0)
                continue;
            // Hava a proper type cast.
            qjackctlJackPort *pOJack = (qjackctlJackPort *) pOPort;
            // Get port connections...
            const char **ppszClientPorts = jack_port_get_all_connections(pOJack->jackClient(), pOJack->jackPort());
            if (ppszClientPorts) {
                // Now, for each input client port...
                int iClientPort = 0;
                while (ppszClientPorts[iClientPort]) {
                    qjackctlPortItem *pIPort = IClientList()->findClientPort(ppszClientPorts[iClientPort]);
                    if (pIPort) {
                        pOPort->addConnect(pIPort);
                        pIPort->addConnect(pOPort);
                    }
                    iClientPort++;
                }
                ::free(ppszClientPorts);
            }
        }
    }
}


// end of qjackctlJackConnect.cpp

