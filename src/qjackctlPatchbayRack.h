// qjackctlPatchbayRack.h
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

#ifndef __qjackctlPatchbayRack_h
#define __qjackctlPatchbayRack_h

#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qptrlist.h>

#include <jack/jack.h>

// Patchbay slot normalization modes.
#define QJACKCTL_SLOTMODE_OPEN      0
#define QJACKCTL_SLOTMODE_HALF      1
#define QJACKCTL_SLOTMODE_FULL      2

// Patchbay change signal flags.
#define QJACKCTL_CABLE_FAILED       0
#define QJACKCTL_CABLE_OK           1
#define QJACKCTL_CABLE_CONNECTED    2


// Patchbay socket definition.
class qjackctlPatchbaySocket
{
public:

    // Constructor.
    qjackctlPatchbaySocket(const QString& sSocketName, const QString& sClientName);
    // Default destructor.
    ~qjackctlPatchbaySocket();

    // Slot property accessors.
    const QString& name();
    const QString& clientName();

    // Slot property methods.
    void setName(const QString& sSocketName);
    void setClientName(const QString& sClientName);

    // Plug list primitive methods.
    void addPlug(const QString& sPlugName);
    void removePlug(const QString& sPlugName);

    // Plug list accessor.
    QStringList& pluglist();

private:

    // Properties.
    QString m_sSocketName;
    QString m_sClientName;

    // Patchbay socket plug list.
    QStringList m_pluglist;
};



// Patchbay socket slot definition.
class qjackctlPatchbaySlot
{
public:

    // Constructor.
    qjackctlPatchbaySlot(const QString& sSlotName, int iSlotMode = QJACKCTL_SLOTMODE_OPEN);
    // Default destructor.
    ~qjackctlPatchbaySlot();

    // Slot property accessors.
    const QString& name();
    int mode();

    // Slot property methods.
    void setName(const QString& sSlotName);
    void setMode(int iSlotMode);

    // Socket methods.
    void setOutputSocket(qjackctlPatchbaySocket *pSocket);
    void setInputSocket(qjackctlPatchbaySocket *pSocket);

    // Socket accessors.
    qjackctlPatchbaySocket *outputSocket();
    qjackctlPatchbaySocket *inputSocket();

private:

    // Slot properties.
    QString m_sSlotName;
    int m_iSlotMode;

    // Socket references.
    qjackctlPatchbaySocket *m_pOutputSocket;
    qjackctlPatchbaySocket *m_pInputSocket;
};


// Patchbay cable connection definition.
class qjackctlPatchbayCable
{
public:

    // Constructor.
    qjackctlPatchbayCable(qjackctlPatchbaySocket *pOutputSocket, qjackctlPatchbaySocket *pInputSocket);
    // Default destructor.
    ~qjackctlPatchbayCable();

    // Socket methods.
    void setOutputSocket(qjackctlPatchbaySocket *pSocket);
    void setInputSocket(qjackctlPatchbaySocket *pSocket);

    // Socket accessors.
    qjackctlPatchbaySocket *outputSocket();
    qjackctlPatchbaySocket *inputSocket();

private:

    // Socket references.
    qjackctlPatchbaySocket *m_pOutputSocket;
    qjackctlPatchbaySocket *m_pInputSocket;
};


// Patchbay rack profile definition.
class qjackctlPatchbayRack : public QObject
{
    Q_OBJECT

public:

    // Constructor.
    qjackctlPatchbayRack();
    // Default destructor.
    ~qjackctlPatchbayRack();

    // Common socket list primitive methods.
    void addSocket(QPtrList<qjackctlPatchbaySocket>& socketlist, qjackctlPatchbaySocket *pSocket);
    void removeSocket(QPtrList<qjackctlPatchbaySocket>& socketlist, qjackctlPatchbaySocket *pSocket);

    // Slot list primitive methods.
    void addSlot(qjackctlPatchbaySlot *pSlot);
    void removeSlot(qjackctlPatchbaySlot *pSlot);

    // Cable list primitive methods.
    void addCable(qjackctlPatchbayCable *pCable);
    void removeCable(qjackctlPatchbayCable *pCable);

    // Common socket finder.
    qjackctlPatchbaySocket *findSocket(QPtrList<qjackctlPatchbaySocket>& socketlist, const QString& sSocketName);
    // Slot finders.
    qjackctlPatchbaySlot *findSlot(const QString& sSlotName);
    // Cable finder.
    qjackctlPatchbayCable *findCable(const QString& sOutputSocket, const QString& sInputSocket);
    qjackctlPatchbayCable *findCable(qjackctlPatchbayCable *pCablePtr);

    // Patchbay cleaner.
    void clear();

    // Patchbay rack socket list accessors.
    QPtrList<qjackctlPatchbaySocket>& osocketlist();
    QPtrList<qjackctlPatchbaySocket>& isocketlist();
    // Patchbay rack slots list accessor.
    QPtrList<qjackctlPatchbaySlot>& slotlist();
    // Patchbay cable connections list accessor.
    QPtrList<qjackctlPatchbayCable>& cablelist();

    // Connect persistence scan cycle method.
    void connectScan(jack_client_t *pJackClient);

signals:

    // Cable connection change signal.
    void cableConnected(const char *pszOutputPort, const char *pszInputPort, unsigned int uiCableFlags);

private:

    // Lookup for the n-th client port that matches the given regular expressions...
    const char *findClientPort(const char **ppszClientPorts, const QString& sClientName, const QString& sPortName, int n = 0);
    // Check if an output client:port is already connected to yet another one.
    bool isConnected(const char *pszOutputPort, const char *pszInputPort);
    // Check whether a socket pair is fully connected, and e
    void connectCable(qjackctlPatchbaySocket *pOutputSocket, qjackctlPatchbaySocket *pInputSocket);

    // Patchbay sockets lists.
    QPtrList<qjackctlPatchbaySocket> m_osocketlist;
    QPtrList<qjackctlPatchbaySocket> m_isocketlist;
    // Patchbay rack slots list.
    QPtrList<qjackctlPatchbaySlot> m_slotlist;
    // Patchbay cable connections list.
    QPtrList<qjackctlPatchbayCable> m_cablelist;

    // Connection peristence cache variables.
    jack_client_t *m_pJackClient;
    const char **m_ppszOutputPorts;
    const char **m_ppszInputPorts;
};


#endif  // __qjackctlPatchbayRack_h

// qjackctlPatchbayRack.h
