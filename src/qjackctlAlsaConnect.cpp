// qjackctlAlsaConnect.cpp
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

#include "qjackctlAlsaConnect.h"

// Local pixmaps.
#include "icons/mcliento.xpm"
#include "icons/mclienti.xpm"
#include "icons/mporto.xpm"
#include "icons/mporti.xpm"

static int g_iXpmRefCount = 0;

static QPixmap *g_pXpmClientO = 0;  // Output client item pixmap.
static QPixmap *g_pXpmClientI = 0;  // Input client item pixmap.
static QPixmap *g_pXpmPortO   = 0;  // Output port pixmap.
static QPixmap *g_pXpmPortI   = 0;  // Input port pixmap.


//----------------------------------------------------------------------
// class qjackctlAlsaPort -- Alsa port list item.
//

// Constructor.
qjackctlAlsaPort::qjackctlAlsaPort ( qjackctlAlsaClient *pClient, const QString& sPortName, int iAlsaPort )
    : qjackctlPortItem(pClient, QString::number(iAlsaPort) + ":" + sPortName)
{
    m_iAlsaPort = iAlsaPort;

    if (pClient->isReadable()) {
        QListViewItem::setPixmap(0, *g_pXpmPortO);
    } else {
        QListViewItem::setPixmap(0, *g_pXpmPortI);
    }
}

// Default destructor.
qjackctlAlsaPort::~qjackctlAlsaPort (void)
{
}


// Alsa handles accessors.
int qjackctlAlsaPort::alsaClient (void)
{
    return ((qjackctlAlsaClient *) client())->alsaClient();
}

int qjackctlAlsaPort::alsaPort (void)
{
    return m_iAlsaPort;
}


//----------------------------------------------------------------------
// class qjackctlAlsaClient -- Alsa client list item.
//

// Constructor.
qjackctlAlsaClient::qjackctlAlsaClient ( qjackctlAlsaClientList *pClientList, const QString& sClientName, int iAlsaClient )
    : qjackctlClientItem(pClientList, QString::number(iAlsaClient) + ":" + sClientName)
{
    m_iAlsaClient = iAlsaClient;
    
    if (pClientList->isReadable()) {
        QListViewItem::setPixmap(0, *g_pXpmClientO);
    } else {
        QListViewItem::setPixmap(0, *g_pXpmClientI);
    }
}

// Default destructor.
qjackctlAlsaClient::~qjackctlAlsaClient (void)
{
}


// Jack client accessor.
int qjackctlAlsaClient::alsaClient (void)
{
    return m_iAlsaClient;
}


// Derived port finder.
qjackctlAlsaPort *qjackctlAlsaClient::findPort ( int iAlsaPort )
{
    for (qjackctlAlsaPort *pPort = (qjackctlAlsaPort *) ports().first();
            pPort;
                pPort = (qjackctlAlsaPort *) ports().next()) {
        if (iAlsaPort == pPort->alsaPort())
            return pPort;
    }
    
    return 0;
}


//----------------------------------------------------------------------
// qjackctlAlsaClientList -- Jack client list.
//

// Constructor.
qjackctlAlsaClientList::qjackctlAlsaClientList( qjackctlClientListView *pListView, snd_seq_t *pAlsaSeq, bool bReadable )
    : qjackctlClientList(pListView, bReadable)
{
    if (g_iXpmRefCount == 0) {
        g_pXpmClientO = new QPixmap((const char **) mcliento_xpm);
        g_pXpmClientI = new QPixmap((const char **) mclienti_xpm);
        g_pXpmPortO   = new QPixmap((const char **) mporto_xpm);
        g_pXpmPortI   = new QPixmap((const char **) mporti_xpm);
    }
    g_iXpmRefCount++;

    m_pAlsaSeq  = pAlsaSeq;
}

// Default destructor.
qjackctlAlsaClientList::~qjackctlAlsaClientList (void)
{
    if (--g_iXpmRefCount == 0) {
        delete g_pXpmClientO;
        delete g_pXpmClientI;
        delete g_pXpmPortO;
        delete g_pXpmPortI;
    }
}


// Alsa sequencer accessor.
snd_seq_t *qjackctlAlsaClientList::alsaSeq (void)
{
    return m_pAlsaSeq;
}


// Client finder by id.
qjackctlAlsaClient *qjackctlAlsaClientList::findClient ( int iAlsaClient )
{
    for (qjackctlAlsaClient *pClient = (qjackctlAlsaClient *) clients().first();
            pClient;
                pClient = (qjackctlAlsaClient *) clients().next()) {
        if (iAlsaClient == pClient->alsaClient())
            return pClient;
    }
    
    return 0;
}


// Client port finder by id.
qjackctlAlsaPort *qjackctlAlsaClientList::findClientPort ( int iAlsaClient, int iAlsaPort )
{
    qjackctlAlsaClient *pClient = findClient(iAlsaClient);
    if (pClient == 0)
        return 0;

    return pClient->findPort(iAlsaPort);
}


// Client:port refreshner.
int qjackctlAlsaClientList::updateClientPorts (void)
{
    if (m_pAlsaSeq == 0)
        return 0;

    int iDirtyCount = 0;

    markClientPorts(0);

    unsigned int uiAlsaFlags;
    if (isReadable())
        uiAlsaFlags = SND_SEQ_PORT_CAP_READ  | SND_SEQ_PORT_CAP_SUBS_READ;
    else
        uiAlsaFlags = SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;

    snd_seq_client_info_t *pClientInfo;
    snd_seq_port_info_t   *pPortInfo;

    snd_seq_client_info_alloca(&pClientInfo);
    snd_seq_port_info_alloca(&pPortInfo);
    snd_seq_client_info_set_client(pClientInfo, -1);

    while (snd_seq_query_next_client(m_pAlsaSeq, pClientInfo) >= 0) {

        int iAlsaClient = snd_seq_client_info_get_client(pClientInfo);
        if (iAlsaClient > 0) {
            qjackctlAlsaClient *pClient = findClient(iAlsaClient);
            snd_seq_port_info_set_client(pPortInfo, iAlsaClient);
            snd_seq_port_info_set_port(pPortInfo, -1);
            while (snd_seq_query_next_port(m_pAlsaSeq, pPortInfo) >= 0) {
                unsigned int uiPortCapability = snd_seq_port_info_get_capability(pPortInfo);
                if (((uiPortCapability & uiAlsaFlags) == uiAlsaFlags) &&
                    ((uiPortCapability & SND_SEQ_PORT_CAP_NO_EXPORT) == 0)) {
                    qjackctlAlsaPort *pPort = 0;
                    int iAlsaPort = snd_seq_port_info_get_port(pPortInfo);
                    if (pClient)
                        pPort = pClient->findPort(iAlsaPort);
                    if (pClient == 0) {
                        QString sClientName = snd_seq_client_info_get_name(pClientInfo);
                        pClient = new qjackctlAlsaClient(this, sClientName, iAlsaClient);
                        iDirtyCount++;
                    }
                    if (pClient && pPort == 0) {
                        QString sPortName = snd_seq_port_info_get_name(pPortInfo);
                        pPort = new qjackctlAlsaPort(pClient, sPortName, iAlsaPort);
                        iDirtyCount++;
                    }
                    if (pPort)
                        pPort->markClientPort(1);
                }
            }
        }
    }

    cleanClientPorts(0);

    return iDirtyCount;
}


//----------------------------------------------------------------------
// qjackctlAlsaConnect -- Output-to-Input client/ports connection object.
//

// Constructor.
qjackctlAlsaConnect::qjackctlAlsaConnect ( qjackctlConnectView *pConnectView, snd_seq_t *pAlsaSeq )
    : qjackctlConnect(pConnectView)
{
    setOClientList(new qjackctlAlsaClientList(connectView()->OListView(), pAlsaSeq, true));
    setIClientList(new qjackctlAlsaClientList(connectView()->IListView(), pAlsaSeq, false));

    m_pAlsaSeq = pAlsaSeq;
}

// Default destructor.
qjackctlAlsaConnect::~qjackctlAlsaConnect (void)
{
}


// Alsa sequencer accessor.
snd_seq_t *qjackctlAlsaConnect::alsaSeq (void)
{
    return m_pAlsaSeq;
}


// Connection primitive.
void qjackctlAlsaConnect::connectPorts ( qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
    qjackctlAlsaPort *pOAlsa = (qjackctlAlsaPort *) pOPort;
    qjackctlAlsaPort *pIAlsa = (qjackctlAlsaPort *) pIPort;

    snd_seq_port_subscribe_t *pAlsaSubs;
    snd_seq_addr_t seq_addr;

    snd_seq_port_subscribe_alloca(&pAlsaSubs);

    seq_addr.client = pOAlsa->alsaClient();
    seq_addr.port   = pOAlsa->alsaPort();
    snd_seq_port_subscribe_set_sender(pAlsaSubs, &seq_addr);

    seq_addr.client = pIAlsa->alsaClient();
    seq_addr.port   = pIAlsa->alsaPort();
    snd_seq_port_subscribe_set_dest(pAlsaSubs, &seq_addr);

    snd_seq_subscribe_port(m_pAlsaSeq, pAlsaSubs);
}


// Disconnection primitive.
void qjackctlAlsaConnect::disconnectPorts ( qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
    qjackctlAlsaPort *pOAlsa = (qjackctlAlsaPort *) pOPort;
    qjackctlAlsaPort *pIAlsa = (qjackctlAlsaPort *) pIPort;

    snd_seq_port_subscribe_t *pAlsaSubs;
    snd_seq_addr_t seq_addr;

    snd_seq_port_subscribe_alloca(&pAlsaSubs);

    seq_addr.client = pOAlsa->alsaClient();
    seq_addr.port   = pOAlsa->alsaPort();
    snd_seq_port_subscribe_set_sender(pAlsaSubs, &seq_addr);

    seq_addr.client = pIAlsa->alsaClient();
    seq_addr.port   = pIAlsa->alsaPort();
    snd_seq_port_subscribe_set_dest(pAlsaSubs, &seq_addr);

    snd_seq_unsubscribe_port(m_pAlsaSeq, pAlsaSubs);
}


// Update port connection references.
void qjackctlAlsaConnect::updateConnections (void)
{
    snd_seq_query_subscribe_t *pAlsaSubs;
    snd_seq_addr_t seq_addr;

    snd_seq_query_subscribe_alloca(&pAlsaSubs);

    // Proper type casts.
    qjackctlAlsaClientList *pOClientList = (qjackctlAlsaClientList *) OClientList();
    qjackctlAlsaClientList *pIClientList = (qjackctlAlsaClientList *) IClientList();
    
    // For each output client item...
    for (qjackctlClientItem *pOClient = pOClientList->clients().first();
            pOClient;
                pOClient = pOClientList->clients().next()) {
        // For each output port item...
        for (qjackctlPortItem *pOPort = pOClient->ports().first();
                pOPort;
                    pOPort = pOClient->ports().next()) {
            // Are there already any connections?
            if (pOPort->connects().count() > 0)
                continue;
            // Hava a proper type cast.
            qjackctlAlsaPort *pOAlsa = (qjackctlAlsaPort *) pOPort;
            // Get port connections...
            snd_seq_query_subscribe_set_type(pAlsaSubs, SND_SEQ_QUERY_SUBS_READ);
            snd_seq_query_subscribe_set_index(pAlsaSubs, 0);
            seq_addr.client = pOAlsa->alsaClient();
            seq_addr.port   = pOAlsa->alsaPort();
            snd_seq_query_subscribe_set_root(pAlsaSubs, &seq_addr);
            while (snd_seq_query_port_subscribers(m_pAlsaSeq, pAlsaSubs) >= 0) {
                seq_addr = *snd_seq_query_subscribe_get_addr(pAlsaSubs);
                qjackctlPortItem *pIPort = (qjackctlPortItem *) pIClientList->findClientPort(seq_addr.client, seq_addr.port);
                if (pIPort) {
                    pOPort->addConnect(pIPort);
                    pIPort->addConnect(pOPort);
                }
                snd_seq_query_subscribe_set_index(pAlsaSubs, snd_seq_query_subscribe_get_index(pAlsaSubs) + 1);
            }
        }
    }
}


// end of qjackctlAlsaConnect.cpp

