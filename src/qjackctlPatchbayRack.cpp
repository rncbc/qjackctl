// qjackctlPatchbayRack.cpp
//
/****************************************************************************
   Copyright  ( C) 2003, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or  ( at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*****************************************************************************/

#include "qjackctlPatchbayRack.h"

#include <qregexp.h>


//----------------------------------------------------------------------
// class qjackctlPatchbaySocket -- Patchbay socket implementation.
//

// Constructor.
qjackctlPatchbaySocket::qjackctlPatchbaySocket ( const QString& sSocketName, const QString& sClientName )
{
    m_sSocketName = sSocketName;
    m_sClientName = sClientName;
}

// Default destructor.
qjackctlPatchbaySocket::~qjackctlPatchbaySocket (void)
{
    m_pluglist.clear();
}


// Slot property accessors.
const QString& qjackctlPatchbaySocket::name (void)
{
    return m_sSocketName;
}

const QString& qjackctlPatchbaySocket::clientName (void)
{
    return m_sClientName;
}


// Slot property methods.
void qjackctlPatchbaySocket::setName ( const QString& sSocketName )
{
    m_sSocketName = sSocketName;
}

void qjackctlPatchbaySocket::setClientName ( const QString& sClientName )
{
    m_sClientName = sClientName;
}


// Plug list primitive methods.
void qjackctlPatchbaySocket::addPlug ( const QString& sPlugName )
{
    m_pluglist.append(sPlugName);
}

void qjackctlPatchbaySocket::removePlug ( const QString& sPlugName )
{
    QStringList::Iterator iter = m_pluglist.find(sPlugName);
    if (iter != m_pluglist.end())
        m_pluglist.remove(iter);
}


// Plug list accessor.
QStringList& qjackctlPatchbaySocket::pluglist (void)
{
    return m_pluglist;
}


//----------------------------------------------------------------------
// class qjackctlPatchbaySlot -- Patchbay socket slot implementation.
//

// Constructor.
qjackctlPatchbaySlot::qjackctlPatchbaySlot ( const QString& sSlotName, int iSlotMode )
{
    m_pOutputSocket = NULL;
    m_pInputSocket = NULL;

    m_sSlotName = sSlotName;
    m_iSlotMode = iSlotMode;
}

// Default destructor.
qjackctlPatchbaySlot::~qjackctlPatchbaySlot (void)
{
    setOutputSocket(NULL);
    setInputSocket(NULL);
}


// Slot property accessors.
const QString& qjackctlPatchbaySlot::name (void)
{
    return m_sSlotName;
}

int qjackctlPatchbaySlot::mode (void)
{
    return m_iSlotMode;
}


// Slot property methods.
void qjackctlPatchbaySlot::setName ( const QString& sSlotName )
{
    m_sSlotName = sSlotName;
}

void qjackctlPatchbaySlot::setMode ( int iSlotMode )
{
    m_iSlotMode = iSlotMode;
}


// Socket methods.
void qjackctlPatchbaySlot::setOutputSocket ( qjackctlPatchbaySocket *pSocket )
{
    m_pOutputSocket = pSocket;
}

void qjackctlPatchbaySlot::setInputSocket ( qjackctlPatchbaySocket *pSocket )
{
    m_pInputSocket = pSocket;
}


// Socket accessors.
qjackctlPatchbaySocket *qjackctlPatchbaySlot::outputSocket (void)
{
    return m_pOutputSocket;
}

qjackctlPatchbaySocket *qjackctlPatchbaySlot::inputSocket (void)
{
    return m_pInputSocket;
}


//----------------------------------------------------------------------
// class qjackctlPatchbayCable -- Patchbay cable connection implementation.
//

// Constructor.
qjackctlPatchbayCable::qjackctlPatchbayCable ( qjackctlPatchbaySocket *pOutputSocket, qjackctlPatchbaySocket *pInputSocket )
{
    m_pOutputSocket = pOutputSocket;
    m_pInputSocket = pInputSocket;
}

// Default destructor.
qjackctlPatchbayCable::~qjackctlPatchbayCable (void)
{
    setOutputSocket(NULL);
    setInputSocket(NULL);
}


// Socket methods.
void qjackctlPatchbayCable::setOutputSocket ( qjackctlPatchbaySocket *pSocket )
{
    m_pOutputSocket = pSocket;
}

void qjackctlPatchbayCable::setInputSocket ( qjackctlPatchbaySocket *pSocket )
{
    m_pInputSocket = pSocket;
}


// Socket accessors.
qjackctlPatchbaySocket *qjackctlPatchbayCable::outputSocket (void)
{
    return m_pOutputSocket;
}

qjackctlPatchbaySocket *qjackctlPatchbayCable::inputSocket (void)
{
    return m_pInputSocket;
}


//----------------------------------------------------------------------
// class qjackctlPatchbayRack -- Patchbay rack profile implementation.
//

// Constructor.
qjackctlPatchbayRack::qjackctlPatchbayRack (void)
{
    m_osocketlist.setAutoDelete(true);
    m_isocketlist.setAutoDelete(true);
    m_slotlist.setAutoDelete(true);
    m_cablelist.setAutoDelete(true);

    m_pJackClient = NULL;
    m_ppszOutputPorts = NULL;
    m_ppszInputPorts = NULL;
}

// Default destructor.
qjackctlPatchbayRack::~qjackctlPatchbayRack (void)
{
    clear();
}


// Common socket list primitive methods.
void qjackctlPatchbayRack::addSocket ( QPtrList<qjackctlPatchbaySocket>& socketlist, qjackctlPatchbaySocket *pSocket )
{
    qjackctlPatchbaySocket *pSocketPtr = findSocket(socketlist, pSocket->name());
    if (pSocketPtr)
        socketlist.remove(pSocketPtr);

    socketlist.append(pSocket);
}


void qjackctlPatchbayRack::removeSocket ( QPtrList<qjackctlPatchbaySocket>& socketlist, qjackctlPatchbaySocket *pSocket )
{
    socketlist.remove(pSocket);
}


// Slot list primitive methods.
void qjackctlPatchbayRack::addSlot ( qjackctlPatchbaySlot *pSlot )
{
    qjackctlPatchbaySlot *pSlotPtr = findSlot(pSlot->name());
    if (pSlotPtr)
        m_slotlist.remove(pSlotPtr);

    m_slotlist.append(pSlot);
}

void qjackctlPatchbayRack::removeSlot ( qjackctlPatchbaySlot *pSlot )
{
    m_slotlist.remove(pSlot);
}


// Cable list primitive methods.
void qjackctlPatchbayRack::addCable ( qjackctlPatchbayCable *pCable )
{
    qjackctlPatchbayCable *pCablePtr = findCable(pCable);
    if (pCablePtr)
        m_cablelist.remove(pCablePtr);

    m_cablelist.append(pCable);
}

void qjackctlPatchbayRack::removeCable ( qjackctlPatchbayCable *pCable )
{
    m_cablelist.remove(pCable);
}


// Common socket finders.
qjackctlPatchbaySocket *qjackctlPatchbayRack::findSocket ( QPtrList<qjackctlPatchbaySocket>& socketlist, const QString& sSocketName )
{
    for (qjackctlPatchbaySocket *pSocket = socketlist.first(); pSocket; pSocket = socketlist.next()) {
        if (sSocketName == pSocket->name())
            return pSocket;
    }

    return NULL;
}


// Patchbay socket slot finders.
qjackctlPatchbaySlot *qjackctlPatchbayRack::findSlot ( const QString& sSlotName )
{
    for (qjackctlPatchbaySlot *pSlot = m_slotlist.first(); pSlot; pSlot = m_slotlist.next()) {
        if (sSlotName == pSlot->name())
            return pSlot;
    }

    return NULL;
}


// Cable finder.
qjackctlPatchbayCable *qjackctlPatchbayRack::findCable ( const QString& sOutputSocket, const QString& sInputSocket )
{
    qjackctlPatchbaySocket *pSocket;

    for (qjackctlPatchbayCable *pCable = m_cablelist.first(); pCable; pCable = m_cablelist.next()) {
        pSocket = pCable->outputSocket();
        if (pSocket && sOutputSocket == pSocket->name()) {
            pSocket = pCable->inputSocket();
            if (pSocket && sInputSocket == pSocket->name())
                return pCable;
        }
    }

    return NULL;
}

qjackctlPatchbayCable *qjackctlPatchbayRack::findCable ( qjackctlPatchbayCable *pCablePtr )
{
    QString sOutputSocket;
    if (pCablePtr->outputSocket())
        sOutputSocket = pCablePtr->outputSocket()->name();

    QString sInputSocket;
    if (pCablePtr->inputSocket())
        sInputSocket = pCablePtr->inputSocket()->name();

    return findCable(sOutputSocket, sInputSocket);
}


// Patckbay cleaners.
void qjackctlPatchbayRack::clear (void)
{
    m_cablelist.clear();
    m_slotlist.clear();
    m_isocketlist.clear();
    m_osocketlist.clear();
}


// Patchbay rack output sockets list accessor.
QPtrList<qjackctlPatchbaySocket>& qjackctlPatchbayRack::osocketlist (void)
{
    return m_osocketlist;
}


// Patchbay rack input sockets list accessor.
QPtrList<qjackctlPatchbaySocket>& qjackctlPatchbayRack::isocketlist (void)
{
    return m_isocketlist;
}


// Patchbay rack slots list accessor.
QPtrList<qjackctlPatchbaySlot>& qjackctlPatchbayRack::slotlist (void)
{
    return m_slotlist;
}


// Patchbay cable connections list accessor.
QPtrList<qjackctlPatchbayCable>& qjackctlPatchbayRack::cablelist (void)
{
    return m_cablelist;
}


// Lookup for the n-th client port that matches the given regular expression...
const char *qjackctlPatchbayRack::findClientPort ( const char **ppszClientPorts, const QString& sClientName, const QString& sPortName, int n )
{
    QRegExp rxClientName(sClientName);
    QRegExp rxPortName(sPortName);

    int i = 0;
    int iClientPort = 0;
    while (ppszClientPorts[iClientPort]) {
        QString sClientPort = ppszClientPorts[iClientPort];
        int iColon = sClientPort.find(":");
        if (iColon >= 0) {
            if (rxClientName.exactMatch(sClientPort.left(iColon)) &&
                rxPortName.exactMatch(sClientPort.right(sClientPort.length() - iColon - 1))) {
                if (++i > n)
                    return ppszClientPorts[iClientPort];
            }
        }
        iClientPort++;
    }

    return NULL;
}


// Check if an output client:port is already connected to yet another one.
bool qjackctlPatchbayRack::isConnected ( const char *pszOutputPort, const char *pszInputPort )
{
    bool bConnected = false;

    const char **ppszInputPorts = jack_port_get_all_connections(m_pJackClient, jack_port_by_name(m_pJackClient, pszOutputPort));
    if (ppszInputPorts) {
        for (int i = 0 ; ppszInputPorts[i] && !bConnected; i++)
            bConnected = (strcmp(ppszInputPorts[i], pszInputPort) == 0);
    }
    ::free(ppszInputPorts);

    return bConnected;
}


// Check whether a socket pair is fully connected, and e
void qjackctlPatchbayRack::connectCable ( qjackctlPatchbaySocket *pOutputSocket, qjackctlPatchbaySocket *pInputSocket )
{
    if (pOutputSocket == NULL || pInputSocket == NULL)
        return;

    QStringList::Iterator iterOutputPlug = pOutputSocket->pluglist().begin();
    QStringList::Iterator iterInputPlug  = pInputSocket->pluglist().begin();
    while (iterOutputPlug != pOutputSocket->pluglist().end() &&
           iterInputPlug  != pInputSocket->pluglist().end()) {
        // Check port connection sequentially...
        int iPort = 0;
        const char *pszOutputPort;
        while ((pszOutputPort = findClientPort(m_ppszOutputPorts, pOutputSocket->clientName(), *iterOutputPlug, iPort)) != NULL) {
            const char *pszInputPort = findClientPort(m_ppszInputPorts, pInputSocket->clientName(), *iterInputPlug, iPort);
            if (pszInputPort) {
                unsigned int uiCableFlags = QJACKCTL_CABLE_FAILED;
                if (isConnected(pszOutputPort, pszInputPort))
                    uiCableFlags = QJACKCTL_CABLE_OK;
                else if (jack_connect(m_pJackClient, pszOutputPort, pszInputPort) == 0)
                    uiCableFlags = QJACKCTL_CABLE_CONNECTED;
                emit cableConnected(pszOutputPort, pszInputPort, uiCableFlags);
                iPort++;
            }
        }
        // Get on next plug pair...
        iterOutputPlug++;
        iterInputPlug++;
    }
}


// Main connect perisistance scan cycle.
void qjackctlPatchbayRack::connectScan ( jack_client_t *pJackClient )
{
    if (pJackClient == NULL || m_pJackClient)
        return;

    // Cache client descriptor.
    m_pJackClient = pJackClient;
    // Cache all current output client-ports...
    m_ppszOutputPorts = jack_get_ports(m_pJackClient, 0, 0, JackPortIsOutput);
    if (m_ppszOutputPorts == NULL)
        return;

    // Cache all current input client-ports...
    m_ppszInputPorts = jack_get_ports(m_pJackClient, 0, 0, JackPortIsInput);
    if (m_ppszInputPorts) {
        for (qjackctlPatchbayCable *pCable = m_cablelist.first(); pCable; pCable = m_cablelist.next())
            connectCable(pCable->outputSocket(), pCable->inputSocket());
    }

    // Free client-ports caches...
    if (m_ppszOutputPorts)
        ::free(m_ppszOutputPorts);
    if (m_ppszInputPorts)
        ::free(m_ppszInputPorts);

    // Reset cached pointers.
    m_ppszOutputPorts = NULL;
    m_ppszInputPorts  = NULL;
    m_pJackClient = NULL;
}


// qjackctlPatchbayRack.cpp
