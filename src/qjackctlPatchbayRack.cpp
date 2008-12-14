// qjackctlPatchbayRack.cpp
//
/****************************************************************************
   Copyright (C) 2003-2008, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qjackctlPatchbayRack.h"

// Aliases accessors.
#include "qjackctlConnectAlias.h"

#include <stdlib.h>


//----------------------------------------------------------------------
// class qjackctlPatchbaySnapshot -- Patchbay snapshot infrastructure.
//

#include <QHash>

class qjackctlPatchbaySnapshot
{
public:

	// Constructor.
	qjackctlPatchbaySnapshot() {};

	// Cleanup.
	void clear() { m_hash.clear(); }

	// Prepare/add snapshot connection item.
	void append (
		const QString& sOClientName, const QString& sOPortName,
		const QString& sIClientName, const QString& sIPortName )
	{
		Ports& ports = m_hash[sOClientName + ':' + sIClientName];
		ports.outs.append(sOPortName);
		ports.ins.append(sIPortName);
	}

	// Commit snapshot into patchbay rack.
	void commit ( qjackctlPatchbayRack *pRack, int iSocketType )
	{
		QHash<QString, Ports>::Iterator iter = m_hash.begin();
		for ( ; iter != m_hash.end(); ++iter) {
			const QString& sKey = iter.key();
			const QString& sOClient = sKey.section(':', 0, 0);
			const QString& sIClient = sKey.section(':', 1, 1);
			Ports& ports = iter.value();
			qjackctlPatchbaySocket *pOSocket = get_socket(
				pRack->osocketlist(), sOClient, ports.outs, iSocketType);
			qjackctlPatchbaySocket *pISocket = get_socket(
				pRack->isocketlist(), sIClient, ports.ins, iSocketType);
			pRack->addCable(new qjackctlPatchbayCable(pOSocket, pISocket));
		}

		m_hash.clear();
	}

	// Find first existing socket.
	static qjackctlPatchbaySocket *find_socket (
		QList<qjackctlPatchbaySocket *>& socketlist,
		const QString& sClientName, int iSocketType )
	{
		QListIterator<qjackctlPatchbaySocket *> iter(socketlist);
		while (iter.hasNext()) {
			qjackctlPatchbaySocket *pSocket = iter.next();
			if (QRegExp(pSocket->clientName()).exactMatch(sClientName)
				&& pSocket->type() == iSocketType) {
				return pSocket;
			}
		}
		return NULL;
	}

	// Add socket plug.
	static void add_socket ( QList<qjackctlPatchbaySocket *>& socketlist,
		const QString& sClientName, const QString& sPortName, int iSocketType )
	{
		qjackctlPatchbaySocket *pSocket
			= find_socket(socketlist, sClientName, iSocketType);
		if (pSocket == NULL) {
			pSocket = new qjackctlPatchbaySocket(sClientName,
				qjackctlClientAlias::escapeRegExpDigits(sClientName),
				iSocketType);
			socketlist.append(pSocket);
		}
		pSocket->addPlug(
			qjackctlClientAlias::escapeRegExpDigits(sPortName));
	}

	// Get client socket into rack (socket list).
	static qjackctlPatchbaySocket *get_socket (
		QList<qjackctlPatchbaySocket *>& socketlist,
		const QString& sClientName, const QStringList& ports,
		int iSocketType )
	{
		int iSocket = 0;
		qjackctlPatchbaySocket *pSocket = NULL;
		QListIterator<qjackctlPatchbaySocket *> iter(socketlist);
		while (iter.hasNext()) {
			pSocket = iter.next();
			if (QRegExp(pSocket->clientName()).exactMatch(sClientName)
				&& pSocket->type() == iSocketType) {
				QStringListIterator plug_iter(pSocket->pluglist());
				QStringListIterator port_iter(ports);
				bool bMatch = true;
				while (bMatch
					&& plug_iter.hasNext() && port_iter.hasNext()) {
					const QString& sPlug = plug_iter.next();
					const QString& sPort = port_iter.next();
					bMatch = (QRegExp(sPlug).exactMatch(sPort));
				}
				if (bMatch)
					return pSocket;
				iSocket++;
			}
		}

		QString sSocketName = sClientName;
		if (iSocket > 0)
			sSocketName += ' ' + QString::number(iSocket + 1);

		pSocket = new qjackctlPatchbaySocket(sSocketName,
			qjackctlClientAlias::escapeRegExpDigits(sClientName),
			iSocketType);
		QStringListIterator port_iter(ports);
		while (port_iter.hasNext()) {
			pSocket->addPlug(
				qjackctlClientAlias::escapeRegExpDigits(port_iter.next()));
		}

		socketlist.append(pSocket);

		return pSocket;
	}

private:

	// Snapshot instance variables.
	struct Ports
	{
		QStringList outs;
		QStringList ins;
	};

	QHash<QString, Ports> m_hash;
};


//----------------------------------------------------------------------
// class qjackctlPatchbaySocket -- Patchbay socket implementation.
//

// Constructor.
qjackctlPatchbaySocket::qjackctlPatchbaySocket ( const QString& sSocketName,
	const QString& sClientName, int iSocketType )
{
	m_sSocketName = sSocketName;
	m_sClientName = sClientName;
	m_iSocketType = iSocketType;
	m_bExclusive  = false;
	m_sSocketForward.clear();
}

// Default destructor.
qjackctlPatchbaySocket::~qjackctlPatchbaySocket (void)
{
	m_pluglist.clear();
}


// Socket property accessors.
const QString& qjackctlPatchbaySocket::name (void) const
{
	return m_sSocketName;
}

const QString& qjackctlPatchbaySocket::clientName (void) const
{
	return m_sClientName;
}

int qjackctlPatchbaySocket::type (void) const
{
	return m_iSocketType;
}

bool qjackctlPatchbaySocket::isExclusive (void) const
{
	return m_bExclusive;
}

const QString& qjackctlPatchbaySocket::forward (void) const
{
	return m_sSocketForward;
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

void qjackctlPatchbaySocket::setType ( int iSocketType )
{
	m_iSocketType = iSocketType;
}

void qjackctlPatchbaySocket::setExclusive ( bool bExclusive )
{
	m_bExclusive = bExclusive;
}

void qjackctlPatchbaySocket::setForward ( const QString& sSocketForward )
{
	m_sSocketForward = sSocketForward;
}


// Plug list primitive methods.
void qjackctlPatchbaySocket::addPlug ( const QString& sPlugName )
{
	m_pluglist.append(sPlugName);
}

void qjackctlPatchbaySocket::removePlug ( const QString& sPlugName )
{
	int iPlug = m_pluglist.indexOf(sPlugName);
	if (iPlug >= 0)
		m_pluglist.removeAt(iPlug);
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
qjackctlPatchbaySlot::qjackctlPatchbaySlot (
	const QString& sSlotName, int iSlotMode )
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
const QString& qjackctlPatchbaySlot::name (void) const
{
	return m_sSlotName;
}

int qjackctlPatchbaySlot::mode (void) const
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
qjackctlPatchbaySocket *qjackctlPatchbaySlot::outputSocket (void) const
{
	return m_pOutputSocket;
}

qjackctlPatchbaySocket *qjackctlPatchbaySlot::inputSocket (void) const
{
	return m_pInputSocket;
}


//----------------------------------------------------------------------
// class qjackctlPatchbayCable -- Patchbay cable connection implementation.
//

// Constructor.
qjackctlPatchbayCable::qjackctlPatchbayCable (
	qjackctlPatchbaySocket *pOutputSocket,
	qjackctlPatchbaySocket *pInputSocket )
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
qjackctlPatchbaySocket *qjackctlPatchbayCable::outputSocket (void) const
{
	return m_pOutputSocket;
}

qjackctlPatchbaySocket *qjackctlPatchbayCable::inputSocket (void) const
{
	return m_pInputSocket;
}


//----------------------------------------------------------------------
// class qjackctlPatchbayRack -- Patchbay rack profile implementation.
//

// Constructor.
qjackctlPatchbayRack::qjackctlPatchbayRack (void)
{
	m_pJackClient = NULL;
	m_ppszOAudioPorts = NULL;
	m_ppszIAudioPorts = NULL;
	m_ppszOMidiPorts = NULL;
	m_ppszIMidiPorts = NULL;

	// MIDI connection persistence cache variables.
	m_pAlsaSeq = NULL;
}

// Default destructor.
qjackctlPatchbayRack::~qjackctlPatchbayRack (void)
{
	clear();
}


// Common socket list primitive methods.
void qjackctlPatchbayRack::addSocket (
	QList<qjackctlPatchbaySocket *>& socketlist,
	qjackctlPatchbaySocket *pSocket )
{
	qjackctlPatchbaySocket *pSocketPtr
		= findSocket(socketlist, pSocket->name());
	if (pSocketPtr)
		removeSocket(socketlist, pSocketPtr);

	socketlist.append(pSocket);
}


void qjackctlPatchbayRack::removeSocket (
	QList<qjackctlPatchbaySocket *>& socketlist,
	qjackctlPatchbaySocket *pSocket )
{
	int iSocket = socketlist.indexOf(pSocket);
	if (iSocket >= 0) {
		socketlist.removeAt(iSocket);
		delete pSocket;
	}
}


// Slot list primitive methods.
void qjackctlPatchbayRack::addSlot ( qjackctlPatchbaySlot *pSlot )
{
	qjackctlPatchbaySlot *pSlotPtr = findSlot(pSlot->name());
	if (pSlotPtr)
		removeSlot(pSlotPtr);

	m_slotlist.append(pSlot);
}

void qjackctlPatchbayRack::removeSlot ( qjackctlPatchbaySlot *pSlot )
{
	int iSlot = m_slotlist.indexOf(pSlot);
	if (iSlot >= 0) {
		m_slotlist.removeAt(iSlot);
		delete pSlot;
	}
}


// Cable list primitive methods.
void qjackctlPatchbayRack::addCable ( qjackctlPatchbayCable *pCable )
{
	qjackctlPatchbayCable *pCablePtr = findCable(pCable);
	if (pCablePtr)
		removeCable(pCablePtr);

	m_cablelist.append(pCable);
}

void qjackctlPatchbayRack::removeCable ( qjackctlPatchbayCable *pCable )
{
	int iCable = m_cablelist.indexOf(pCable);
	if (iCable >= 0) {
		m_cablelist.removeAt(iCable);
		delete pCable;
	}
}


// Common socket finders.
qjackctlPatchbaySocket *qjackctlPatchbayRack::findSocket (
	QList<qjackctlPatchbaySocket *>& socketlist, const QString& sSocketName )
{
	QListIterator<qjackctlPatchbaySocket *> iter(socketlist);
	while (iter.hasNext()) {
		qjackctlPatchbaySocket *pSocket = iter.next();
		if (sSocketName == pSocket->name())
			return pSocket;
	}

	return NULL;
}


// Patchbay socket slot finders.
qjackctlPatchbaySlot *qjackctlPatchbayRack::findSlot ( const QString& sSlotName )
{
	QListIterator<qjackctlPatchbaySlot *> iter(m_slotlist);
	while (iter.hasNext()) {
		qjackctlPatchbaySlot *pSlot = iter.next();
		if (sSlotName == pSlot->name())
			return pSlot;
	}

	return NULL;
}


// Cable finders.
qjackctlPatchbayCable *qjackctlPatchbayRack::findCable (
	const QString& sOutputSocket, const QString& sInputSocket )
{
	qjackctlPatchbaySocket *pSocket;

	QListIterator<qjackctlPatchbayCable *> iter(m_cablelist);
	while (iter.hasNext()) {
		qjackctlPatchbayCable *pCable = iter.next();
		pSocket = pCable->outputSocket();
		if (pSocket && sOutputSocket == pSocket->name()) {
			pSocket = pCable->inputSocket();
			if (pSocket && sInputSocket == pSocket->name())
				return pCable;
		}
	}

	return NULL;
}

qjackctlPatchbayCable *qjackctlPatchbayRack::findCable (
	qjackctlPatchbayCable *pCablePtr )
{
	QString sOutputSocket;
	if (pCablePtr->outputSocket())
		sOutputSocket = pCablePtr->outputSocket()->name();

	QString sInputSocket;
	if (pCablePtr->inputSocket())
		sInputSocket = pCablePtr->inputSocket()->name();

	return findCable(sOutputSocket, sInputSocket);
}


// Cable finder (logical matching by client/port names).
qjackctlPatchbayCable *qjackctlPatchbayRack::findCable (
	const QString& sOClientName, const QString& sOPortName,
	const QString& sIClientName, const QString& sIPortName,
	int iSocketType )
{
	// This is a regex prefix needed for ALSA MDII
	// as client and port names include id-number+colon...
	QString sPrefix;
	if (iSocketType == QJACKCTL_SOCKETTYPE_ALSA_MIDI)
		sPrefix = "[0-9]+:";
	
	// Scan from output-socket list...
	QListIterator<qjackctlPatchbaySocket *> osocket(m_osocketlist);
	while (osocket.hasNext()) {
		qjackctlPatchbaySocket *pOSocket = osocket.next();
		if (pOSocket->type() != iSocketType)
			continue;
		// Output socket client name match?
		if (!QRegExp(sPrefix + pOSocket->clientName())
			.exactMatch(sOClientName))
			continue;
		// Output plug port names match?
		QStringListIterator oplug(pOSocket->pluglist());
		while (oplug.hasNext()) {
			if (!QRegExp(sPrefix + oplug.next())
				.exactMatch(sOPortName))
				continue;
			// Scan for output-socket cable...
			QListIterator<qjackctlPatchbayCable *> cable(m_cablelist);
			while (cable.hasNext()) {
				qjackctlPatchbayCable *pCable = cable.next();
				if (pCable->outputSocket() != pOSocket)
					continue;
				qjackctlPatchbaySocket *pISocket = pCable->inputSocket();
				// Input socket client name match?
				if (!QRegExp(sPrefix + pISocket->clientName())
					.exactMatch(sIClientName))
					continue;
				// Input plug port names match?
				QStringListIterator iplug(pISocket->pluglist());
				while (iplug.hasNext()) {
					// Found it?
					if (QRegExp(sPrefix + iplug.next())
						.exactMatch(sIPortName))
						return pCable;
				}
			}
		}
	}

	// No matching cable was found.
	return NULL;
}


// Patckbay cleaners.
void qjackctlPatchbayRack::clear (void)
{
	qDeleteAll(m_cablelist);
	m_cablelist.clear();

	qDeleteAll(m_slotlist);
	m_slotlist.clear();

	qDeleteAll(m_isocketlist);
	m_isocketlist.clear();

	qDeleteAll(m_osocketlist);
	m_osocketlist.clear();
}


// Patchbay rack output sockets list accessor.
QList<qjackctlPatchbaySocket *>& qjackctlPatchbayRack::osocketlist (void)
{
	return m_osocketlist;
}


// Patchbay rack input sockets list accessor.
QList<qjackctlPatchbaySocket *>& qjackctlPatchbayRack::isocketlist (void)
{
	return m_isocketlist;
}


// Patchbay rack slots list accessor.
QList<qjackctlPatchbaySlot *>& qjackctlPatchbayRack::slotlist (void)
{
	return m_slotlist;
}


// Patchbay cable connections list accessor.
QList<qjackctlPatchbayCable *>& qjackctlPatchbayRack::cablelist (void)
{
	return m_cablelist;
}


// Lookup for the n-th JACK client port
// that matches the given regular expression...
const char *qjackctlPatchbayRack::findJackPort ( const char **ppszJackPorts,
	const QString& sClientName, const QString& sPortName, int n )
{
	QRegExp rxClientName(sClientName);
	QRegExp rxPortName(sPortName);

	int i = 0;
	int iClientPort = 0;
	while (ppszJackPorts[iClientPort]) {
		QString sClientPort = ppszJackPorts[iClientPort];
		int iColon = sClientPort.indexOf(':');
		if (iColon >= 0) {
			if (rxClientName.exactMatch(sClientPort.left(iColon)) &&
				rxPortName.exactMatch(sClientPort.right(
					sClientPort.length() - iColon - 1))) {
				if (++i > n)
					return ppszJackPorts[iClientPort];
			}
		}
		iClientPort++;
	}

	return NULL;
}

// JACK port-pair connection executive IS DEPRECATED!
void qjackctlPatchbayRack::connectJackPorts (
	const char *pszOutputPort, const char *pszInputPort )
{
	unsigned int uiCableFlags = QJACKCTL_CABLE_FAILED;

	if (jack_connect(m_pJackClient, pszOutputPort, pszInputPort) == 0)
		uiCableFlags = QJACKCTL_CABLE_CONNECTED;

	emit cableConnected(
		pszOutputPort, pszInputPort, uiCableFlags);
}


// JACK port-pair disconnection executive.
void qjackctlPatchbayRack::disconnectJackPorts (
	const char *pszOutputPort, const char *pszInputPort )
{
	unsigned int uiCableFlags = QJACKCTL_CABLE_FAILED;

	if (jack_disconnect(m_pJackClient, pszOutputPort, pszInputPort) == 0)
		uiCableFlags = QJACKCTL_CABLE_DISCONNECTED;

	emit cableConnected(
		pszOutputPort, pszInputPort, uiCableFlags);
}


// JACK port-pair connection notifier.
void qjackctlPatchbayRack::checkJackPorts (
	const char *pszOutputPort, const char *pszInputPort )
{
	emit cableConnected(
		pszOutputPort, pszInputPort, QJACKCTL_CABLE_CHECKED);
}


// Check and enforce if an audio output client:port is connected to one input.
void qjackctlPatchbayRack::connectJackSocketPorts (
	qjackctlPatchbaySocket *pOutputSocket, const char *pszOutputPort,
	qjackctlPatchbaySocket *pInputSocket, const char *pszInputPort )
{
	bool bConnected = false;

	// Check for inputs from output...
	const char **ppszInputPorts = jack_port_get_all_connections(
		m_pJackClient, jack_port_by_name(m_pJackClient, pszOutputPort));
	if (ppszInputPorts) {
		for (int i = 0 ; ppszInputPorts[i]; i++) {
			if (strcmp(ppszInputPorts[i], pszInputPort) == 0)
				bConnected = true;
			else if (pOutputSocket->isExclusive())
				disconnectJackPorts(pszOutputPort, ppszInputPorts[i]);
		}
		::free(ppszInputPorts);
	}

	// Check for outputs from input, if the input socket is on exclusive mode...
	if (pInputSocket->isExclusive()) {
		const char **ppszOutputPorts = jack_port_get_all_connections(
			m_pJackClient, jack_port_by_name(m_pJackClient, pszInputPort));
		if (ppszOutputPorts) {
			for (int i = 0 ; ppszOutputPorts[i]; i++) {
				if (strcmp(ppszOutputPorts[i], pszOutputPort) == 0)
					bConnected = true;
				else
					disconnectJackPorts(ppszOutputPorts[i], pszInputPort);
			}
			::free(ppszOutputPorts);
		}
	}

	// Finally do the connection?...
	if (!bConnected) {
		connectJackPorts(pszOutputPort, pszInputPort);
	} else {
		emit cableConnected(
			pszOutputPort, 
			pszInputPort,
			QJACKCTL_CABLE_CHECKED);
	}
}


// Check and maint whether an JACK socket pair is fully connected.
void qjackctlPatchbayRack::connectJackCable (
	qjackctlPatchbaySocket *pOutputSocket, qjackctlPatchbaySocket *pInputSocket )
{
	if (pOutputSocket == NULL || pInputSocket == NULL)
		return;
	if (pOutputSocket->type() != pInputSocket->type())
		return;

	const char **ppszOutputPorts = NULL;
	const char **ppszInputPorts  = NULL;
	if (pOutputSocket->type() == QJACKCTL_SOCKETTYPE_JACK_AUDIO) {
		ppszOutputPorts = m_ppszOAudioPorts;
		ppszInputPorts  = m_ppszIAudioPorts;
	}
	else
	if (pOutputSocket->type() == QJACKCTL_SOCKETTYPE_JACK_MIDI) {
		ppszOutputPorts = m_ppszOMidiPorts;
		ppszInputPorts  = m_ppszIMidiPorts;
	}

	if (ppszOutputPorts == NULL || ppszInputPorts == NULL)
		return;

	// Iterate on each corresponding plug...
	QStringListIterator iterOutputPlug(pOutputSocket->pluglist());
	QStringListIterator iterInputPlug(pInputSocket->pluglist());
	while (iterOutputPlug.hasNext() && iterInputPlug.hasNext()) {
		const QString& sOutputPlug = iterOutputPlug.next();
		const QString& sInputPlug  = iterInputPlug.next();
		// Check audio port connection sequentially...
		int iOPort = 0;
		const char *pszOutputPort;
		while ((pszOutputPort = findJackPort(ppszOutputPorts,
				pOutputSocket->clientName(), sOutputPlug, iOPort)) != NULL) {
			int iIPort = 0;
			const char *pszInputPort;
			while ((pszInputPort = findJackPort(ppszInputPorts,
					pInputSocket->clientName(), sInputPlug, iIPort)) != NULL) {
				connectJackSocketPorts(
					 pOutputSocket, pszOutputPort,
					 pInputSocket, pszInputPort);
				iIPort++;
			}
			iOPort++;
		}
	}
}


// Main JACK cable connect persistance scan cycle.
void qjackctlPatchbayRack::connectJackScan ( jack_client_t *pJackClient )
{
	if (pJackClient == NULL || m_pJackClient)
		return;

	// Cache client descriptor.
	m_pJackClient = pJackClient;
	// Cache all current audio client-ports...
	m_ppszOAudioPorts = jack_get_ports(m_pJackClient,
		0, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
	m_ppszIAudioPorts = jack_get_ports(m_pJackClient,
		0, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
#ifdef CONFIG_JACK_MIDI
	// Cache all current MIDI client-ports...
	m_ppszOMidiPorts = jack_get_ports(m_pJackClient,
		0, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
	m_ppszIMidiPorts = jack_get_ports(m_pJackClient,
		0, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
#else
	m_ppszOMidiPorts = NULL;
	m_ppszIMidiPorts = NULL;
#endif

	// Start looking for connections...
	QListIterator<qjackctlPatchbayCable *> iter(m_cablelist);
	while (iter.hasNext()) {
		qjackctlPatchbayCable *pCable = iter.next();
		connectJackCable(
			pCable->outputSocket(),
			pCable->inputSocket());
	}

	// Forward sockets...
	connectForwardScan(QJACKCTL_SOCKETTYPE_JACK_AUDIO);
#ifdef CONFIG_JACK_MIDI
	connectForwardScan(QJACKCTL_SOCKETTYPE_JACK_MIDI);
#endif

	// Free client-ports caches...
	if (m_ppszOAudioPorts)
		::free(m_ppszOAudioPorts);
	if (m_ppszIAudioPorts)
		::free(m_ppszIAudioPorts);
#ifdef CONFIG_JACK_MIDI
	if (m_ppszOMidiPorts)
		::free(m_ppszOMidiPorts);
	if (m_ppszIMidiPorts)
		::free(m_ppszIMidiPorts);
#endif
	// Reset cached pointers.
	m_ppszOAudioPorts = NULL;
	m_ppszIAudioPorts = NULL;
	m_ppszOMidiPorts = NULL;
	m_ppszIMidiPorts = NULL;
	m_pJackClient = NULL;
}


// Load all midi available midi ports of a given type.
void qjackctlPatchbayRack::loadAlsaPorts (
	QList<qjackctlAlsaMidiPort *>& midiports, bool bReadable )
{
	if (m_pAlsaSeq == NULL)
		return;

	qDeleteAll(midiports);
	midiports.clear();

#ifdef CONFIG_ALSA_SEQ

	unsigned int uiAlsaFlags;
	if (bReadable)
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
			QString sClientName = snd_seq_client_info_get_name(pClientInfo);
			snd_seq_port_info_set_client(pPortInfo, iAlsaClient);
			snd_seq_port_info_set_port(pPortInfo, -1);
			while (snd_seq_query_next_port(m_pAlsaSeq, pPortInfo) >= 0) {
				unsigned int uiPortCapability = snd_seq_port_info_get_capability(pPortInfo);
				if (((uiPortCapability & uiAlsaFlags) == uiAlsaFlags) &&
					((uiPortCapability & SND_SEQ_PORT_CAP_NO_EXPORT) == 0)) {
					qjackctlAlsaMidiPort *pMidiPort = new qjackctlAlsaMidiPort;
					pMidiPort->sClientName = sClientName;
					pMidiPort->iAlsaClient = iAlsaClient;
					pMidiPort->sPortName   = snd_seq_port_info_get_name(pPortInfo);
					pMidiPort->iAlsaPort   = snd_seq_port_info_get_port(pPortInfo);
					midiports.append(pMidiPort);
				}
			}
		}
	}

#endif	// CONFIG_ALSA_SEQ
}


// Get current connections from given MIDI port.
void qjackctlPatchbayRack::loadAlsaConnections (
	QList<qjackctlAlsaMidiPort *>& midiports,
	qjackctlAlsaMidiPort *pMidiPort, bool bReadable )
{
	qDeleteAll(midiports);
	midiports.clear();

#ifdef CONFIG_ALSA_SEQ

	snd_seq_query_subs_type_t snd_subs_type;
	if (bReadable)
		snd_subs_type = SND_SEQ_QUERY_SUBS_READ;
	else
		snd_subs_type = SND_SEQ_QUERY_SUBS_WRITE;

	snd_seq_query_subscribe_t *pAlsaSubs;
	snd_seq_addr_t seq_addr;

	snd_seq_query_subscribe_alloca(&pAlsaSubs);

	snd_seq_query_subscribe_set_type(pAlsaSubs, snd_subs_type);
	snd_seq_query_subscribe_set_index(pAlsaSubs, 0);
	seq_addr.client = pMidiPort->iAlsaClient;
	seq_addr.port   = pMidiPort->iAlsaPort;
	snd_seq_query_subscribe_set_root(pAlsaSubs, &seq_addr);
	while (snd_seq_query_port_subscribers(m_pAlsaSeq, pAlsaSubs) >= 0) {
		seq_addr = *snd_seq_query_subscribe_get_addr(pAlsaSubs);
		qjackctlAlsaMidiPort *pPort = new qjackctlAlsaMidiPort;
		setAlsaPort(pPort, seq_addr.client, seq_addr.port);
		midiports.append(pPort);
		snd_seq_query_subscribe_set_index(pAlsaSubs,
			snd_seq_query_subscribe_get_index(pAlsaSubs) + 1);
	}

#endif	// CONFIG_ALSA_SEQ
}


// Lookup for the n-th MIDI client port that matches the given regular expression...
qjackctlAlsaMidiPort *qjackctlPatchbayRack::findAlsaPort (
	QList<qjackctlAlsaMidiPort *>& midiports,
	const QString& sClientName, const QString& sPortName, int n )
{
	QRegExp rxClientName(sClientName);
	QRegExp rxPortName(sPortName);

	int i = 0;
	// For each port...
	QListIterator<qjackctlAlsaMidiPort *> iter(midiports);
	while (iter.hasNext()) {
		qjackctlAlsaMidiPort *pMidiPort = iter.next();
		if (rxClientName.exactMatch(pMidiPort->sClientName) &&
			rxPortName.exactMatch(pMidiPort->sPortName)) {
			if (++i > n)
				return pMidiPort;
		}
	}

	return NULL;
}


// MIDI port-pair name string.
QString qjackctlPatchbayRack::getAlsaPortName ( qjackctlAlsaMidiPort *pMidiPort )
{
	return QString::number(pMidiPort->iAlsaClient)
		+ ':' + QString::number(pMidiPort->iAlsaPort)
		+ ' ' + pMidiPort->sClientName;
}


// MIDI port-pair name string.
void qjackctlPatchbayRack::setAlsaPort ( qjackctlAlsaMidiPort *pMidiPort,
	int iAlsaClient, int iAlsaPort )
{
#ifdef CONFIG_ALSA_SEQ

	snd_seq_client_info_t *pClientInfo;
	snd_seq_port_info_t   *pPortInfo;

	snd_seq_client_info_alloca(&pClientInfo);
	snd_seq_port_info_alloca(&pPortInfo);

	pMidiPort->iAlsaClient = iAlsaClient;
	pMidiPort->iAlsaPort   = iAlsaPort;

	if (snd_seq_get_any_client_info(m_pAlsaSeq, iAlsaClient, pClientInfo) == 0)
		pMidiPort->sClientName = snd_seq_client_info_get_name(pClientInfo);

	if (snd_seq_get_any_port_info(m_pAlsaSeq, iAlsaClient, iAlsaPort, pPortInfo) == 0)
		pMidiPort->sPortName = snd_seq_port_info_get_name(pPortInfo);

#endif	// CONFIG_ALSA_SEQ
}


// MIDI port-pair connection executive.
void qjackctlPatchbayRack::connectAlsaPorts (
	qjackctlAlsaMidiPort *pOutputPort, qjackctlAlsaMidiPort *pInputPort )
{
#ifdef CONFIG_ALSA_SEQ

	unsigned int uiCableFlags = QJACKCTL_CABLE_FAILED;

	snd_seq_port_subscribe_t *pAlsaSubs;
	snd_seq_addr_t seq_addr;

	snd_seq_port_subscribe_alloca(&pAlsaSubs);

	seq_addr.client = pOutputPort->iAlsaClient;
	seq_addr.port   = pOutputPort->iAlsaPort;
	snd_seq_port_subscribe_set_sender(pAlsaSubs, &seq_addr);

	seq_addr.client = pInputPort->iAlsaClient;
	seq_addr.port   = pInputPort->iAlsaPort;
	snd_seq_port_subscribe_set_dest(pAlsaSubs, &seq_addr);

	if (snd_seq_subscribe_port(m_pAlsaSeq, pAlsaSubs) == 0)
		uiCableFlags = QJACKCTL_CABLE_CONNECTED;

	emit cableConnected(
		getAlsaPortName(pOutputPort),
		getAlsaPortName(pInputPort),
		uiCableFlags);

#endif	// CONFIG_ALSA_SEQ
}


// MIDI port-pair disconnection executive.
void qjackctlPatchbayRack::disconnectAlsaPorts (
	qjackctlAlsaMidiPort *pOutputPort, qjackctlAlsaMidiPort *pInputPort )
{
#ifdef CONFIG_ALSA_SEQ

	unsigned int uiCableFlags = QJACKCTL_CABLE_FAILED;

	snd_seq_port_subscribe_t *pAlsaSubs;
	snd_seq_addr_t seq_addr;

	snd_seq_port_subscribe_alloca(&pAlsaSubs);

	seq_addr.client = pOutputPort->iAlsaClient;
	seq_addr.port   = pOutputPort->iAlsaPort;
	snd_seq_port_subscribe_set_sender(pAlsaSubs, &seq_addr);

	seq_addr.client = pInputPort->iAlsaClient;
	seq_addr.port   = pInputPort->iAlsaPort;
	snd_seq_port_subscribe_set_dest(pAlsaSubs, &seq_addr);

	if (snd_seq_unsubscribe_port(m_pAlsaSeq, pAlsaSubs) == 0)
		uiCableFlags = QJACKCTL_CABLE_DISCONNECTED;

	emit cableConnected(
		getAlsaPortName(pOutputPort),
		getAlsaPortName(pInputPort),
		uiCableFlags);

#endif	// CONFIG_ALSA_SEQ
}


// MIDI port-pair disconnection notifier.
void qjackctlPatchbayRack::checkAlsaPorts (
	qjackctlAlsaMidiPort *pOutputPort, qjackctlAlsaMidiPort *pInputPort )
{
	emit cableConnected(
		getAlsaPortName(pOutputPort),
		getAlsaPortName(pInputPort),
		QJACKCTL_CABLE_CHECKED);
}


// Check and enforce if a midi output client:port is connected to one input.
void qjackctlPatchbayRack::connectAlsaSocketPorts (
	qjackctlPatchbaySocket *pOutputSocket, qjackctlAlsaMidiPort *pOutputPort,
	qjackctlPatchbaySocket *pInputSocket, qjackctlAlsaMidiPort *pInputPort )
{
#ifdef CONFIG_ALSA_SEQ

	bool bConnected = false;

	snd_seq_query_subscribe_t *pAlsaSubs;
	snd_seq_addr_t seq_addr;

	snd_seq_query_subscribe_alloca(&pAlsaSubs);

	// Check for inputs from output...
	snd_seq_query_subscribe_set_type(pAlsaSubs, SND_SEQ_QUERY_SUBS_READ);
	snd_seq_query_subscribe_set_index(pAlsaSubs, 0);
	seq_addr.client = pOutputPort->iAlsaClient;
	seq_addr.port   = pOutputPort->iAlsaPort;
	snd_seq_query_subscribe_set_root(pAlsaSubs, &seq_addr);
	while (snd_seq_query_port_subscribers(m_pAlsaSeq, pAlsaSubs) >= 0) {
		seq_addr = *snd_seq_query_subscribe_get_addr(pAlsaSubs);
		if (seq_addr.client == pInputPort->iAlsaClient
			&& seq_addr.port == pInputPort->iAlsaPort)
			bConnected = true;
		else if (pOutputSocket->isExclusive()) {
			qjackctlAlsaMidiPort iport;
			setAlsaPort(&iport, seq_addr.client, seq_addr.port);
			disconnectAlsaPorts(pOutputPort, &iport);
		}
		snd_seq_query_subscribe_set_index(pAlsaSubs,
			snd_seq_query_subscribe_get_index(pAlsaSubs) + 1);
	}

	// Check for outputs from input, if the input socket is on exclusive mode...
	if (pInputSocket->isExclusive()) {
		snd_seq_query_subscribe_set_type(pAlsaSubs, SND_SEQ_QUERY_SUBS_WRITE);
		snd_seq_query_subscribe_set_index(pAlsaSubs, 0);
		seq_addr.client = pInputPort->iAlsaClient;
		seq_addr.port   = pInputPort->iAlsaPort;
		snd_seq_query_subscribe_set_root(pAlsaSubs, &seq_addr);
		while (snd_seq_query_port_subscribers(m_pAlsaSeq, pAlsaSubs) >= 0) {
			seq_addr = *snd_seq_query_subscribe_get_addr(pAlsaSubs);
			if (seq_addr.client == pOutputPort->iAlsaClient
				&& seq_addr.port == pOutputPort->iAlsaPort)
				bConnected = true;
			else if (pInputSocket->isExclusive()) {
				qjackctlAlsaMidiPort oport;
				setAlsaPort(&oport, seq_addr.client, seq_addr.port);
				disconnectAlsaPorts(&oport, pInputPort);
			}
			snd_seq_query_subscribe_set_index(pAlsaSubs,
				snd_seq_query_subscribe_get_index(pAlsaSubs) + 1);
		}
	}

	// Finally do the connection?...
	if (!bConnected) {
		connectAlsaPorts(pOutputPort, pInputPort);
	} else {
		emit cableConnected(
			getAlsaPortName(pOutputPort),
			getAlsaPortName(pInputPort),
			QJACKCTL_CABLE_CHECKED);
	}

#endif	// CONFIG_ALSA_SEQ
}


// Check and maint whether a MIDI socket pair is fully connected.
void qjackctlPatchbayRack::connectAlsaCable (
	qjackctlPatchbaySocket *pOutputSocket, qjackctlPatchbaySocket *pInputSocket )
{
	if (pOutputSocket == NULL || pInputSocket == NULL)
		return;
	if (pOutputSocket->type() != pInputSocket->type() ||
		pOutputSocket->type() != QJACKCTL_SOCKETTYPE_ALSA_MIDI)
		return;

	// Iterate on each corresponding plug...
	QStringListIterator iterOutputPlug(pOutputSocket->pluglist());
	QStringListIterator iterInputPlug(pInputSocket->pluglist());
	while (iterOutputPlug.hasNext() && iterInputPlug.hasNext()) {
		const QString& sOutputPlug = iterOutputPlug.next();
		const QString& sInputPlug  = iterInputPlug.next();
		// Check MIDI port connection sequentially...
		int iOPort = 0;
		qjackctlAlsaMidiPort *pOutputPort;
		while ((pOutputPort = findAlsaPort(m_omidiports,
				pOutputSocket->clientName(), sOutputPlug, iOPort)) != NULL) {
			int iIPort = 0;
			qjackctlAlsaMidiPort *pInputPort;
			while ((pInputPort = findAlsaPort(m_imidiports,
					pInputSocket->clientName(), sInputPlug, iIPort)) != NULL) {
				connectAlsaSocketPorts(
					 pOutputSocket, pOutputPort,
					 pInputSocket, pInputPort);
				iIPort++;
			}
			iOPort++;
		}
	}
}


// Overloaded MIDI cable connect persistance scan cycle.
void qjackctlPatchbayRack::connectAlsaScan ( snd_seq_t *pAlsaSeq )
{
	if (pAlsaSeq == NULL || m_pAlsaSeq)
		return;

	// Cache sequencer descriptor.
	m_pAlsaSeq = pAlsaSeq;

	// Cache all current output client-ports...
	loadAlsaPorts(m_omidiports, true);
	loadAlsaPorts(m_imidiports, false);

	// Run the MIDI cable scan...
	QListIterator<qjackctlPatchbayCable *> iter(m_cablelist);
	while (iter.hasNext()) {
		qjackctlPatchbayCable *pCable = iter.next();
		connectAlsaCable(
			pCable->outputSocket(),
			pCable->inputSocket());
	}

	// Forward MIDI sockets...
	connectForwardScan(QJACKCTL_SOCKETTYPE_ALSA_MIDI);

	// Free client-ports caches...
	qDeleteAll(m_omidiports);
	m_omidiports.clear();

	qDeleteAll(m_imidiports);
	m_imidiports.clear();

	m_pAlsaSeq = NULL;
}


// JACK socket/ports forwarding scan...
void qjackctlPatchbayRack::connectJackForwardPorts (
	const char *pszPort, const char *pszPortForward )
{
	// Check for outputs from forwarded input...
	const char **ppszOutputPorts = jack_port_get_all_connections(
		m_pJackClient, jack_port_by_name(m_pJackClient, pszPortForward));
	if (ppszOutputPorts) {
		// Grab current connections of target port...
		const char **ppszPorts = jack_port_get_all_connections(
			m_pJackClient, jack_port_by_name(m_pJackClient, pszPort));
		for (int i = 0 ; ppszOutputPorts[i]; i++) {
			// Need to lookup if already connected...
			bool bConnected = false;
			for (int j = 0; ppszPorts && ppszPorts[j]; j++) {
				if (strcmp(ppszOutputPorts[i], ppszPorts[j]) == 0) {
					bConnected = true;
					break;
				}
			}
			// Make or just report the connection...
			if (bConnected) {
				checkJackPorts(ppszOutputPorts[i], pszPort);
			} else {
				connectJackPorts(ppszOutputPorts[i], pszPort);
			}
		}
		// Free provided arrays...
		if (ppszPorts)
			::free(ppszPorts);
		::free(ppszOutputPorts);
	}
}

void qjackctlPatchbayRack::connectJackForward (
	qjackctlPatchbaySocket *pSocket, qjackctlPatchbaySocket *pSocketForward )
{
	if (pSocket == NULL || pSocketForward == NULL)
		return;
	if (pSocket->type() != pSocketForward->type())
		return;

	const char **ppszOutputPorts = NULL;
	const char **ppszInputPorts  = NULL;
	if (pSocket->type() == QJACKCTL_SOCKETTYPE_JACK_AUDIO) {
		ppszOutputPorts = m_ppszOAudioPorts;
		ppszInputPorts  = m_ppszIAudioPorts;
	}
	else
	if (pSocket->type() == QJACKCTL_SOCKETTYPE_JACK_MIDI) {
		ppszOutputPorts = m_ppszOMidiPorts;
		ppszInputPorts  = m_ppszIMidiPorts;
	}

	if (ppszOutputPorts == NULL || ppszInputPorts == NULL)
		return;

	QStringListIterator iterPlug(pSocket->pluglist());
	QStringListIterator iterPlugForward(pSocketForward->pluglist());
	while (iterPlug.hasNext() && iterPlugForward.hasNext()) {
		// Check audio port connection sequentially...
		const QString& sPlug = iterPlug.next();
		const QString& sPlugForward = iterPlugForward.next();
		const char *pszPortForward = findJackPort(ppszInputPorts,
			pSocketForward->clientName(), sPlugForward, 0);
		if (pszPortForward) {
			const char *pszPort = findJackPort(ppszInputPorts,
				pSocket->clientName(), sPlug, 0);
			if (pszPort)
				connectJackForwardPorts(pszPort, pszPortForward);
		}
	}
}


// ALSA socket/ports forwarding scan...
void qjackctlPatchbayRack::connectAlsaForwardPorts (
	qjackctlAlsaMidiPort *pPort, qjackctlAlsaMidiPort *pPortForward )
{
#ifdef CONFIG_ALSA_SEQ

	// Grab current connections of target port...
	QList<qjackctlAlsaMidiPort *> midiports;
	loadAlsaConnections(midiports, pPort, false);

	snd_seq_query_subscribe_t *pAlsaSubs;
	snd_seq_addr_t seq_addr;
	
	snd_seq_query_subscribe_alloca(&pAlsaSubs);
	
	// Check for inputs from output...
	snd_seq_query_subscribe_set_type(pAlsaSubs, SND_SEQ_QUERY_SUBS_WRITE);
	snd_seq_query_subscribe_set_index(pAlsaSubs, 0);
	seq_addr.client = pPortForward->iAlsaClient;
	seq_addr.port   = pPortForward->iAlsaPort;
	snd_seq_query_subscribe_set_root(pAlsaSubs, &seq_addr);
	while (snd_seq_query_port_subscribers(m_pAlsaSeq, pAlsaSubs) >= 0) {
		seq_addr = *snd_seq_query_subscribe_get_addr(pAlsaSubs);
		// Need to lookup if already connected...
		bool bConnected = false;
		QListIterator<qjackctlAlsaMidiPort *> iter(midiports);
		while (iter.hasNext()) {
			qjackctlAlsaMidiPort *pMidiPort = iter.next();
			if (pMidiPort->iAlsaClient == seq_addr.client &&
				pMidiPort->iAlsaPort == seq_addr.port) {
				bConnected = true;
				break;
			}
		}
		// Make and/or just report the connection...
		qjackctlAlsaMidiPort oport;
		setAlsaPort(&oport, seq_addr.client, seq_addr.port);
		if (bConnected) {
			checkAlsaPorts(&oport, pPort);
		} else {
			connectAlsaPorts(&oport, pPort);
		}
		snd_seq_query_subscribe_set_index(pAlsaSubs,
			snd_seq_query_subscribe_get_index(pAlsaSubs) + 1);
	}

	qDeleteAll(midiports);
	midiports.clear();

#endif	// CONFIG_ALSA_SEQ
}

void qjackctlPatchbayRack::connectAlsaForward (
	qjackctlPatchbaySocket *pSocket, qjackctlPatchbaySocket *pSocketForward )
{
	if (pSocket == NULL || pSocketForward == NULL)
		return;
	if (pSocket->type() != pSocketForward->type() ||
		pSocket->type() != QJACKCTL_SOCKETTYPE_ALSA_MIDI)
		return;

	QStringListIterator iterPlug(pSocket->pluglist());
	QStringListIterator iterPlugForward(pSocketForward->pluglist());
	while (iterPlug.hasNext() && iterPlugForward.hasNext()) {
		// Check MIDI port connection sequentially...
		const QString& sPlug = iterPlug.next();
		const QString& sPlugForward = iterPlugForward.next();
		qjackctlAlsaMidiPort *pPortForward = findAlsaPort(m_imidiports,
			pSocketForward->clientName(), sPlugForward, 0);
		if (pPortForward) {
			qjackctlAlsaMidiPort *pPort = findAlsaPort(m_imidiports,
				pSocket->clientName(), sPlug, 0);
			if (pPort)
				connectAlsaForwardPorts(pPort, pPortForward);
		}
	}
}


// Common socket forwrading scan...
void qjackctlPatchbayRack::connectForwardScan ( int iSocketType )
{
	// First, make a copy of a seriously forwarded socket list...
	QList<qjackctlPatchbaySocket *> socketlist;

	QListIterator<qjackctlPatchbaySocket *> isocket(m_isocketlist);
	while (isocket.hasNext()) {
		qjackctlPatchbaySocket *pSocket = isocket.next();
		if (pSocket->type() != iSocketType)
			continue;
		if (pSocket->forward().isEmpty())
			continue;
		socketlist.append(pSocket);
	}

	// Second, scan for input forwarded sockets...
	QListIterator<qjackctlPatchbaySocket *> iter(socketlist);
	while (iter.hasNext()) {
		qjackctlPatchbaySocket *pSocket = iter.next();
		qjackctlPatchbaySocket *pSocketForward
			= findSocket(m_isocketlist, pSocket->forward());
		if (pSocketForward == NULL)
			continue;
		switch (iSocketType) {
		case QJACKCTL_SOCKETTYPE_JACK_AUDIO:
		case QJACKCTL_SOCKETTYPE_JACK_MIDI:
			connectJackForward(pSocket, pSocketForward);
			break;
		case QJACKCTL_SOCKETTYPE_ALSA_MIDI:
			connectAlsaForward(pSocket, pSocketForward);
			break;
		}
	}
}


//----------------------------------------------------------------------
// JACK snapshot.

void qjackctlPatchbayRack::connectJackSnapshotEx ( int iSocketType )
{
	if (m_pJackClient == NULL)
		return;

	const char *pszJackPortType = JACK_DEFAULT_AUDIO_TYPE;
#ifdef CONFIG_JACK_MIDI
	if (iSocketType == QJACKCTL_SOCKETTYPE_JACK_MIDI)
		pszJackPortType = JACK_DEFAULT_MIDI_TYPE;
#endif

	const char **ppszInputPorts = jack_get_ports(m_pJackClient,
		0, pszJackPortType, JackPortIsInput);
	if (ppszInputPorts) {
		for (int i = 0; ppszInputPorts[i]; ++i) {
			const char *pszInputPort = ppszInputPorts[i];
			const QString sInputPort = pszInputPort;
			const QString& sIClient = sInputPort.section(':', 0, 0);
			const QString& sIPort   = sInputPort.section(':', 1, 1);
			qjackctlPatchbaySnapshot::add_socket(
				isocketlist(), sIClient, sIPort, iSocketType);
		}
		::free(ppszInputPorts);
	}

	const char **ppszOutputPorts = jack_get_ports(m_pJackClient,
		0, pszJackPortType, JackPortIsOutput);
	if (ppszOutputPorts == NULL)
		return;

	qjackctlPatchbaySnapshot snapshot;

	for (int i = 0; ppszOutputPorts[i]; ++i) {
		const char *pszOutputPort = ppszOutputPorts[i];
		const QString sOutputPort = pszOutputPort;
		const QString& sOClient = sOutputPort.section(':', 0, 0);
		const QString& sOPort   = sOutputPort.section(':', 1, 1);
		qjackctlPatchbaySnapshot::add_socket(
			osocketlist(), sOClient, sOPort, iSocketType);
		// Check for inputs from output...
		ppszInputPorts = jack_port_get_all_connections(
			m_pJackClient, jack_port_by_name(m_pJackClient, pszOutputPort));
		if (ppszInputPorts == NULL)
			continue;
		for (int j = 0 ; ppszInputPorts[j]; ++j) {
			const char *pszInputPort = ppszInputPorts[j];
			const QString sInputPort = pszInputPort;
			const QString& sIClient = sInputPort.section(':', 0, 0);
			const QString& sIPort   = sInputPort.section(':', 1, 1);
			snapshot.append(sOClient, sOPort, sIClient, sIPort);
		}
		::free(ppszInputPorts);
	}
	::free(ppszOutputPorts);

	snapshot.commit(this, iSocketType);
}


void qjackctlPatchbayRack::connectJackSnapshot ( jack_client_t *pJackClient )
{
	if (pJackClient == NULL || m_pJackClient)
		return;

	// Cache JACK client descriptor.
	m_pJackClient = pJackClient;

	connectJackSnapshotEx(QJACKCTL_SOCKETTYPE_JACK_AUDIO);
	connectJackSnapshotEx(QJACKCTL_SOCKETTYPE_JACK_MIDI);

	// Done.
	m_pJackClient = NULL;
}


//----------------------------------------------------------------------
// ALSA snapshot.

void qjackctlPatchbayRack::connectAlsaSnapshot ( snd_seq_t *pAlsaSeq )
{
	if (pAlsaSeq == NULL || m_pAlsaSeq)
		return;

	// Cache sequencer descriptor.
	m_pAlsaSeq = pAlsaSeq;

#ifdef CONFIG_ALSA_SEQ

	QList<qjackctlAlsaMidiPort *> imidiports;
	loadAlsaPorts(imidiports, false);

	QListIterator<qjackctlAlsaMidiPort *> iport(imidiports);
	while (iport.hasNext()) {
		qjackctlAlsaMidiPort *pIPort = iport.next();
		qjackctlPatchbaySnapshot::add_socket(
			isocketlist(), pIPort->sClientName, pIPort->sPortName,
			QJACKCTL_SOCKETTYPE_ALSA_MIDI);
	}

	qDeleteAll(imidiports);
	imidiports.clear();

	qjackctlPatchbaySnapshot snapshot;

	QList<qjackctlAlsaMidiPort *> omidiports;
	loadAlsaPorts(omidiports, true);

	QListIterator<qjackctlAlsaMidiPort *> oport(omidiports);
	while (oport.hasNext()) {
		qjackctlAlsaMidiPort *pOPort = oport.next();
		qjackctlPatchbaySnapshot::add_socket(
			osocketlist(), pOPort->sClientName, pOPort->sPortName,
			QJACKCTL_SOCKETTYPE_ALSA_MIDI);
		loadAlsaConnections(imidiports, pOPort, true);
		QListIterator<qjackctlAlsaMidiPort *> iport(imidiports);
		while (iport.hasNext()) {
			qjackctlAlsaMidiPort *pIPort = iport.next();
			snapshot.append(
				pOPort->sClientName, pOPort->sPortName,
				pIPort->sClientName, pIPort->sPortName);
		}
		qDeleteAll(imidiports);
		imidiports.clear();
	}

	qDeleteAll(omidiports);
	omidiports.clear();

	snapshot.commit(this, QJACKCTL_SOCKETTYPE_ALSA_MIDI);

#endif

	// Done.
	m_pAlsaSeq = NULL;
}


// qjackctlPatchbayRack.cpp
