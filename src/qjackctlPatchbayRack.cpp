// qjackctlPatchbayRack.cpp
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

#include "qjackctlPatchbayRack.h"

#include <QRegExp>


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


// Cable finder.
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


// Lookup for the n-th client port that matches the given regular expression...
const char *qjackctlPatchbayRack::findAudioPort ( const char **ppszAudioPorts,
	const QString& sClientName, const QString& sPortName, int n )
{
	QRegExp rxClientName(sClientName);
	QRegExp rxPortName(sPortName);

	int i = 0;
	int iClientPort = 0;
	while (ppszAudioPorts[iClientPort]) {
		QString sClientPort = ppszAudioPorts[iClientPort];
		int iColon = sClientPort.indexOf(':');
		if (iColon >= 0) {
			if (rxClientName.exactMatch(sClientPort.left(iColon)) &&
				rxPortName.exactMatch(sClientPort.right(
					sClientPort.length() - iColon - 1))) {
				if (++i > n)
					return ppszAudioPorts[iClientPort];
			}
		}
		iClientPort++;
	}

	return NULL;
}

// Audio port-pair connection executive IS DEPRECATED!
void qjackctlPatchbayRack::connectAudioPorts (
	const char *pszOutputPort, const char *pszInputPort )
{
	unsigned int uiCableFlags = QJACKCTL_CABLE_FAILED;

	if (jack_connect(m_pJackClient, pszOutputPort, pszInputPort) == 0)
		uiCableFlags = QJACKCTL_CABLE_CONNECTED;

	emit cableConnected(
		pszOutputPort, pszInputPort, uiCableFlags);
}


// Audio port-pair disconnection executive.
void qjackctlPatchbayRack::disconnectAudioPorts (
	const char *pszOutputPort, const char *pszInputPort )
{
	unsigned int uiCableFlags = QJACKCTL_CABLE_FAILED;

	if (jack_disconnect(m_pJackClient, pszOutputPort, pszInputPort) == 0)
		uiCableFlags = QJACKCTL_CABLE_DISCONNECTED;

	emit cableConnected(
		pszOutputPort, pszInputPort, uiCableFlags);
}


// Audio port-pair connection notifier.
void qjackctlPatchbayRack::checkAudioPorts (
	const char *pszOutputPort, const char *pszInputPort )
{
	emit cableConnected(
		pszOutputPort, pszInputPort, QJACKCTL_CABLE_CHECKED);
}


// Check and enforce if an audio output client:port is connected to one input.
void qjackctlPatchbayRack::connectAudioSocketPorts (
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
				disconnectAudioPorts(pszOutputPort, ppszInputPorts[i]);
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
					disconnectAudioPorts(ppszOutputPorts[i], pszInputPort);
			}
			::free(ppszOutputPorts);
		}
	}

	// Finally do the connection?...
	if (!bConnected) {
		connectAudioPorts(pszOutputPort, pszInputPort);
	} else {
		emit cableConnected(
			pszOutputPort, 
			pszInputPort,
			QJACKCTL_CABLE_CHECKED);
	}
}


// Check and maint whether an audio socket pair is fully connected.
void qjackctlPatchbayRack::connectAudioCable (
	qjackctlPatchbaySocket *pOutputSocket, qjackctlPatchbaySocket *pInputSocket )
{
	if (pOutputSocket == NULL || pInputSocket == NULL)
		return;
	if (pOutputSocket->type() != pInputSocket->type() ||
		pOutputSocket->type() != QJACKCTL_SOCKETTYPE_AUDIO)
		return;

	// Iterate on each corresponding plug...
	QStringListIterator iterOutputPlug(pOutputSocket->pluglist());
	QStringListIterator iterInputPlug(pInputSocket->pluglist());
	while (iterOutputPlug.hasNext() && iterInputPlug.hasNext()) {
		const QString& sOutputPlug = iterOutputPlug.next();
		const QString& sInputPlug  = iterInputPlug.next();
		// Check audio port connection sequentially...
		int iPort = 0;
		const char *pszOutputPort;
		while ((pszOutputPort = findAudioPort(m_ppszOAudioPorts,
			pOutputSocket->clientName(), sOutputPlug, iPort)) != NULL) {
			const char *pszInputPort = findAudioPort(m_ppszIAudioPorts,
				pInputSocket->clientName(), sInputPlug, iPort);
			if (pszInputPort) {
				connectAudioSocketPorts(
					pOutputSocket, pszOutputPort,
					pInputSocket, pszInputPort);
			}
			iPort++;
		}
	}
}


// Main audio cable connect persistance scan cycle.
void qjackctlPatchbayRack::connectAudioScan ( jack_client_t *pJackClient )
{
	if (pJackClient == NULL || m_pJackClient)
		return;

	// Cache client descriptor.
	m_pJackClient = pJackClient;
	// Cache all current client-ports...
	m_ppszOAudioPorts = jack_get_ports(m_pJackClient,
		0, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
	m_ppszIAudioPorts = jack_get_ports(m_pJackClient,
		0, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);

	// Start looking for connections...
	if (m_ppszOAudioPorts && m_ppszIAudioPorts) {
		QListIterator<qjackctlPatchbayCable *> iter(m_cablelist);
		while (iter.hasNext()) {
			qjackctlPatchbayCable *pCable = iter.next();
			connectAudioCable(
				pCable->outputSocket(),
				pCable->inputSocket());
		}
	}

	// Forward Audio sockets...
	connectForwardScan(QJACKCTL_SOCKETTYPE_AUDIO);

	// Free client-ports caches...
	if (m_ppszOAudioPorts)
		::free(m_ppszOAudioPorts);
	if (m_ppszIAudioPorts)
		::free(m_ppszIAudioPorts);
	// Reset cached pointers.
	m_ppszOAudioPorts = NULL;
	m_ppszIAudioPorts = NULL;
	m_pJackClient = NULL;
}


// Load all midi available midi ports of a given type.
void qjackctlPatchbayRack::loadMidiPorts (
	QList<qjackctlMidiPort *>& midiports, bool bReadable )
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
					qjackctlMidiPort *pMidiPort = new qjackctlMidiPort;
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
void qjackctlPatchbayRack::loadMidiConnections (
	QList<qjackctlMidiPort *>& midiports,
	qjackctlMidiPort *pMidiPort, bool bReadable )
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
		qjackctlMidiPort *pPort = new qjackctlMidiPort;
		setMidiPort(pPort, seq_addr.client, seq_addr.port);
		midiports.append(pPort);
		snd_seq_query_subscribe_set_index(pAlsaSubs,
			snd_seq_query_subscribe_get_index(pAlsaSubs) + 1);
	}

#endif	// CONFIG_ALSA_SEQ
}


// Lookup for the n-th MIDI client port that matches the given regular expression...
qjackctlMidiPort *qjackctlPatchbayRack::findMidiPort (
	QList<qjackctlMidiPort *>& midiports,
	const QString& sClientName, const QString& sPortName, int n )
{
	QRegExp rxClientName(sClientName);
	QRegExp rxPortName(sPortName);

	int i = 0;
	// For each port...
	QListIterator<qjackctlMidiPort *> iter(midiports);
	while (iter.hasNext()) {
		qjackctlMidiPort *pMidiPort = iter.next();
		if (rxClientName.exactMatch(pMidiPort->sClientName) &&
			rxPortName.exactMatch(pMidiPort->sPortName)) {
			if (++i > n)
				return pMidiPort;
		}
	}

	return NULL;
}


// MIDI port-pair name string.
QString qjackctlPatchbayRack::getMidiPortName ( qjackctlMidiPort *pMidiPort )
{
	return QString::number(pMidiPort->iAlsaClient)
		+ ':' + QString::number(pMidiPort->iAlsaPort)
		+ ' ' + pMidiPort->sClientName;
}


// MIDI port-pair name string.
void qjackctlPatchbayRack::setMidiPort ( qjackctlMidiPort *pMidiPort,
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
void qjackctlPatchbayRack::connectMidiPorts (
	qjackctlMidiPort *pOutputPort, qjackctlMidiPort *pInputPort )
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
		getMidiPortName(pOutputPort), 
		getMidiPortName(pInputPort),
		uiCableFlags);

#endif	// CONFIG_ALSA_SEQ
}


// MIDI port-pair disconnection executive.
void qjackctlPatchbayRack::disconnectMidiPorts (
	qjackctlMidiPort *pOutputPort, qjackctlMidiPort *pInputPort )
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
		getMidiPortName(pOutputPort),
		getMidiPortName(pInputPort),
		uiCableFlags);

#endif	// CONFIG_ALSA_SEQ
}


// MIDI port-pair disconnection notifier.
void qjackctlPatchbayRack::checkMidiPorts (
	qjackctlMidiPort *pOutputPort, qjackctlMidiPort *pInputPort )
{
	emit cableConnected(
		getMidiPortName(pOutputPort),
		getMidiPortName(pInputPort),
		QJACKCTL_CABLE_CHECKED);
}


// Check and enforce if a midi output client:port is connected to one input.
void qjackctlPatchbayRack::connectMidiSocketPorts (
	qjackctlPatchbaySocket *pOutputSocket, qjackctlMidiPort *pOutputPort,
	qjackctlPatchbaySocket *pInputSocket, qjackctlMidiPort *pInputPort )
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
			qjackctlMidiPort iport;
			setMidiPort(&iport, seq_addr.client, seq_addr.port);
			disconnectMidiPorts(pOutputPort, &iport);
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
				qjackctlMidiPort oport;
				setMidiPort(&oport, seq_addr.client, seq_addr.port);
				disconnectMidiPorts(&oport, pInputPort);
			}
			snd_seq_query_subscribe_set_index(pAlsaSubs,
				snd_seq_query_subscribe_get_index(pAlsaSubs) + 1);
		}
	}

	// Finally do the connection?...
	if (!bConnected) {
		connectMidiPorts(pOutputPort, pInputPort);
	} else {
		emit cableConnected(
			getMidiPortName(pOutputPort),
			getMidiPortName(pInputPort),
			QJACKCTL_CABLE_CHECKED);
	}

#endif	// CONFIG_ALSA_SEQ
}


// Check and maint whether a MIDI socket pair is fully connected.
void qjackctlPatchbayRack::connectMidiCable (
	qjackctlPatchbaySocket *pOutputSocket, qjackctlPatchbaySocket *pInputSocket )
{
	if (pOutputSocket == NULL || pInputSocket == NULL)
		return;
	if (pOutputSocket->type() != pInputSocket->type() ||
		pOutputSocket->type() != QJACKCTL_SOCKETTYPE_MIDI)
		return;

	// Iterate on each corresponding plug...
	QStringListIterator iterOutputPlug(pOutputSocket->pluglist());
	QStringListIterator iterInputPlug(pInputSocket->pluglist());
	while (iterOutputPlug.hasNext() && iterInputPlug.hasNext()) {
		const QString& sOutputPlug = iterOutputPlug.next();
		const QString& sInputPlug  = iterInputPlug.next();
		// Check MIDI port connection sequentially...
		int iPort = 0;
		qjackctlMidiPort *pOutputPort;
		while ((pOutputPort = findMidiPort(m_omidiports,
				pOutputSocket->clientName(), sOutputPlug, iPort)) != NULL) {
			qjackctlMidiPort *pInputPort = findMidiPort(m_imidiports,
				pInputSocket->clientName(), sInputPlug, iPort);
			if (pInputPort) {
				connectMidiSocketPorts(
					pOutputSocket, pOutputPort,
					pInputSocket, pInputPort);
			}
			iPort++;
		}
	}
}


// Overloaded MIDI cable connect persistance scan cycle.
void qjackctlPatchbayRack::connectMidiScan ( snd_seq_t *pAlsaSeq )
{
	if (pAlsaSeq == NULL || m_pAlsaSeq)
		return;

	// Cache sequencer descriptor.
	m_pAlsaSeq = pAlsaSeq;

	// Cache all current output client-ports...
	loadMidiPorts(m_omidiports, true);
	loadMidiPorts(m_imidiports, false);

	// Run the MIDI cable scan...
	QListIterator<qjackctlPatchbayCable *> iter(m_cablelist);
	while (iter.hasNext()) {
		qjackctlPatchbayCable *pCable = iter.next();
		connectMidiCable(
			pCable->outputSocket(),
			pCable->inputSocket());
	}

	// Forward MIDI sockets...
	connectForwardScan(QJACKCTL_SOCKETTYPE_MIDI);

	// Free client-ports caches...
	qDeleteAll(m_omidiports);
	m_omidiports.clear();

	qDeleteAll(m_imidiports);
	m_imidiports.clear();

	m_pAlsaSeq = NULL;
}


// Audio socket/ports forwarding scan...
void qjackctlPatchbayRack::connectAudioForwardPorts (
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
				checkAudioPorts(ppszOutputPorts[i], pszPort);
			} else {
				connectAudioPorts(ppszOutputPorts[i], pszPort);
			}
		}
		// Free provided arrays...
		if (ppszPorts)
			::free(ppszPorts);
		::free(ppszOutputPorts);
	}
}

void qjackctlPatchbayRack::connectAudioForward (
	qjackctlPatchbaySocket *pSocket, qjackctlPatchbaySocket *pSocketForward )
{
	if (pSocket == NULL || pSocketForward == NULL)
		return;
	if (pSocket->type() != pSocketForward->type()
		|| pSocket->type() != QJACKCTL_SOCKETTYPE_AUDIO)
		return;

	QStringListIterator iterPlug(pSocket->pluglist());
	QStringListIterator iterPlugForward(pSocketForward->pluglist());
	while (iterPlug.hasNext() && iterPlugForward.hasNext()) {
		// Check audio port connection sequentially...
		const QString& sPlug = iterPlug.next();
		const QString& sPlugForward = iterPlugForward.next();
		const char *pszPortForward = findAudioPort(m_ppszIAudioPorts,
			pSocketForward->clientName(), sPlugForward, 0);
		if (pszPortForward) {
			const char *pszPort = findAudioPort(m_ppszIAudioPorts,
				pSocket->clientName(), sPlug, 0);
			if (pszPort)
				connectAudioForwardPorts(pszPort, pszPortForward);
		}
	}
}


// MIDI socket/ports forwarding scan...
void qjackctlPatchbayRack::connectMidiForwardPorts (
	qjackctlMidiPort *pPort, qjackctlMidiPort *pPortForward )
{
#ifdef CONFIG_ALSA_SEQ

	// Grab current connections of target port...
	QList<qjackctlMidiPort *> midiports;
	loadMidiConnections(midiports, pPort, false);

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
		QListIterator<qjackctlMidiPort *> iter(midiports);
		while (iter.hasNext()) {
			qjackctlMidiPort *pMidiPort = iter.next();
			if (pMidiPort->iAlsaClient == seq_addr.client
				&& pMidiPort->iAlsaPort == seq_addr.port) {
				bConnected = true;
				break;
			}
		}
		// Make and/or just report the connection...
		qjackctlMidiPort oport;
		setMidiPort(&oport, seq_addr.client, seq_addr.port);
		if (bConnected) {
			checkMidiPorts(&oport, pPort);
		} else {
			connectMidiPorts(&oport, pPort);
		}
		snd_seq_query_subscribe_set_index(pAlsaSubs,
			snd_seq_query_subscribe_get_index(pAlsaSubs) + 1);
	}

	qDeleteAll(midiports);
	midiports.clear();

#endif	// CONFIG_ALSA_SEQ
}

void qjackctlPatchbayRack::connectMidiForward (
	qjackctlPatchbaySocket *pSocket, qjackctlPatchbaySocket *pSocketForward )
{
	if (pSocket == NULL || pSocketForward == NULL)
		return;
	if (pSocket->type() != pSocketForward->type()
		|| pSocket->type() != QJACKCTL_SOCKETTYPE_MIDI)
		return;

	QStringListIterator iterPlug(pSocket->pluglist());
	QStringListIterator iterPlugForward(pSocketForward->pluglist());
	while (iterPlug.hasNext() && iterPlugForward.hasNext()) {
		// Check MIDI port connection sequentially...
		const QString& sPlug = iterPlug.next();
		const QString& sPlugForward = iterPlugForward.next();
		qjackctlMidiPort *pPortForward = findMidiPort(m_imidiports,
			pSocketForward->clientName(), sPlugForward, 0);
		if (pPortForward) {
			qjackctlMidiPort *pPort = findMidiPort(m_imidiports,
				pSocket->clientName(), sPlug, 0);
			if (pPort)
				connectMidiForwardPorts(pPort, pPortForward);
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
		case QJACKCTL_SOCKETTYPE_AUDIO:
			connectAudioForward(pSocket, pSocketForward);
			break;
		case QJACKCTL_SOCKETTYPE_MIDI:
			connectMidiForward(pSocket, pSocketForward);
			break;
		}
	}
}


// qjackctlPatchbayRack.cpp
