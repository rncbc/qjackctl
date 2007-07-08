// qjackctlAlsaConnect.cpp
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

#include "qjackctlAlsaConnect.h"

#include <QPixmap>


//----------------------------------------------------------------------
// class qjackctlAlsaPort -- Alsa port list item.
//

// Constructor.
qjackctlAlsaPort::qjackctlAlsaPort ( qjackctlAlsaClient *pClient,
	const QString& sPortName, int iAlsaPort )
	: qjackctlPortItem(pClient, /* QString::number(iAlsaPort) + ":" + */ sPortName)
{
	m_iAlsaPort = iAlsaPort;

	qjackctlAlsaConnect *pAlsaConnect
		= static_cast<qjackctlAlsaConnect *> (
			((pClient->clientList())->listView())->binding());

	if (pAlsaConnect) {
		if (pClient->isReadable()) {
			QTreeWidgetItem::setIcon(0,
				QIcon(pAlsaConnect->pixmap(QJACKCTL_ALSA_PORTO)));
		} else {
			QTreeWidgetItem::setIcon(0,
				QIcon(pAlsaConnect->pixmap(QJACKCTL_ALSA_PORTI)));
		}
	}
}

// Default destructor.
qjackctlAlsaPort::~qjackctlAlsaPort (void)
{
}


// Alsa handles accessors.
int qjackctlAlsaPort::alsaClient (void) const
{
	return (static_cast<qjackctlAlsaClient *> (client()))->alsaClient();
}

int qjackctlAlsaPort::alsaPort (void) const
{
	return m_iAlsaPort;
}


//----------------------------------------------------------------------
// class qjackctlAlsaClient -- Alsa client list item.
//

// Constructor.
qjackctlAlsaClient::qjackctlAlsaClient ( qjackctlAlsaClientList *pClientList,
	const QString& sClientName, int iAlsaClient )
	: qjackctlClientItem(pClientList, /* QString::number(iAlsaClient) + ":" + */ sClientName)
{
	m_iAlsaClient = iAlsaClient;

	qjackctlAlsaConnect *pAlsaConnect
		= static_cast<qjackctlAlsaConnect *> (
			(pClientList->listView())->binding());

	if (pAlsaConnect) {
		if (pClientList->isReadable()) {
			QTreeWidgetItem::setIcon(0,
				QIcon(pAlsaConnect->pixmap(QJACKCTL_ALSA_CLIENTO)));
		} else {
			QTreeWidgetItem::setIcon(0,
				QIcon(pAlsaConnect->pixmap(QJACKCTL_ALSA_CLIENTI)));
		}
	}
}

// Default destructor.
qjackctlAlsaClient::~qjackctlAlsaClient (void)
{
}


// Jack client accessor.
int qjackctlAlsaClient::alsaClient (void) const
{
	return m_iAlsaClient;
}


// Derived port finder.
qjackctlAlsaPort *qjackctlAlsaClient::findPort ( int iAlsaPort )
{
	QListIterator<qjackctlPortItem *> iter(ports());
	while (iter.hasNext()) {
		qjackctlAlsaPort *pPort
			= static_cast<qjackctlAlsaPort *> (iter.next());
		if (iAlsaPort == pPort->alsaPort())
			return pPort;
	}

	return NULL;
}


//----------------------------------------------------------------------
// qjackctlAlsaClientList -- Jack client list.
//

// Constructor.
qjackctlAlsaClientList::qjackctlAlsaClientList (
	qjackctlClientListView *pListView,
	snd_seq_t *pAlsaSeq, bool bReadable )
	: qjackctlClientList(pListView, bReadable)
{
	m_pAlsaSeq = pAlsaSeq;
}

// Default destructor.
qjackctlAlsaClientList::~qjackctlAlsaClientList (void)
{
}


// Alsa sequencer accessor.
snd_seq_t *qjackctlAlsaClientList::alsaSeq (void) const
{
	return m_pAlsaSeq;
}


// Client finder by id.
qjackctlAlsaClient *qjackctlAlsaClientList::findClient ( int iAlsaClient )
{
	QListIterator<qjackctlClientItem *> iter(clients());
	while (iter.hasNext()) {
		qjackctlAlsaClient *pClient
			= static_cast<qjackctlAlsaClient *> (iter.next());
		if (iAlsaClient == pClient->alsaClient())
			return pClient;
	}

	return NULL;
}


// Client port finder by id.
qjackctlAlsaPort *qjackctlAlsaClientList::findClientPort ( int iAlsaClient,
	int iAlsaPort )
{
	qjackctlAlsaClient *pClient = findClient(iAlsaClient);
	if (pClient == NULL)
		return NULL;

	return pClient->findPort(iAlsaPort);
}


// Client:port refreshner.
int qjackctlAlsaClientList::updateClientPorts (void)
{
	if (m_pAlsaSeq == NULL)
		return 0;

	int iDirtyCount = 0;

	markClientPorts(0);

#ifdef CONFIG_ALSA_SEQ

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
				unsigned int uiPortCapability
					= snd_seq_port_info_get_capability(pPortInfo);
				if (((uiPortCapability & uiAlsaFlags) == uiAlsaFlags) &&
					((uiPortCapability & SND_SEQ_PORT_CAP_NO_EXPORT) == 0)) {
					QString sClientName = QString::number(iAlsaClient) + ':';
					sClientName += snd_seq_client_info_get_name(pClientInfo);
					qjackctlAlsaPort *pPort = 0;
					int iAlsaPort = snd_seq_port_info_get_port(pPortInfo);
					if (pClient == 0) {
						pClient = new qjackctlAlsaClient(this,
							sClientName, iAlsaClient);
						iDirtyCount++;
					} else {
						pPort = pClient->findPort(iAlsaPort);
						if (sClientName != pClient->clientName()) {
							pClient->setClientName(sClientName);
							iDirtyCount++;
						}
					}
					if (pClient) {
						QString sPortName = QString::number(iAlsaPort) + ':';
						sPortName += snd_seq_port_info_get_name(pPortInfo);
						if (pPort == 0) {
							pPort = new qjackctlAlsaPort(pClient,
								sPortName, iAlsaPort);
							iDirtyCount++;
						} else if (sPortName != pPort->portName()) {
							pPort->setPortName(sPortName);
							iDirtyCount++;
						}
					}
					if (pPort)
						pPort->markClientPort(1);
				}
			}
		}
	}

#endif	// CONFIG_ALSA_SEQ

	cleanClientPorts(0);

	return iDirtyCount;
}


//----------------------------------------------------------------------
// qjackctlAlsaConnect -- Output-to-Input client/ports connection object.
//

// Constructor.
qjackctlAlsaConnect::qjackctlAlsaConnect (
	qjackctlConnectView *pConnectView, snd_seq_t *pAlsaSeq )
	: qjackctlConnect(pConnectView)
{
	createIconPixmaps();

	setOClientList(new qjackctlAlsaClientList(
		connectView()->OListView(), pAlsaSeq, true));
	setIClientList(new qjackctlAlsaClientList(
		connectView()->IListView(), pAlsaSeq, false));

	m_pAlsaSeq = pAlsaSeq;
}

// Default destructor.
qjackctlAlsaConnect::~qjackctlAlsaConnect (void)
{
	deleteIconPixmaps();
}


// Common pixmap accessor (static).
QPixmap& qjackctlAlsaConnect::pixmap ( int iPixmap ) const
{
	return *m_apPixmaps[iPixmap];
}


// Local pixmap-set janitor methods.
void qjackctlAlsaConnect::createIconPixmaps (void)
{
	m_apPixmaps[QJACKCTL_ALSA_CLIENTI] = createIconPixmap("mclienti");
	m_apPixmaps[QJACKCTL_ALSA_CLIENTO] = createIconPixmap("mcliento");
	m_apPixmaps[QJACKCTL_ALSA_PORTI]   = createIconPixmap("mporti");
	m_apPixmaps[QJACKCTL_ALSA_PORTO]   = createIconPixmap("mporto");
}

void qjackctlAlsaConnect::deleteIconPixmaps (void)
{
	for (int i = 0; i < QJACKCTL_ALSA_PIXMAPS; i++) {
		if (m_apPixmaps[i])
			delete m_apPixmaps[i];
		m_apPixmaps[i] = NULL;
	}
}


// Alsa sequencer accessor.
snd_seq_t *qjackctlAlsaConnect::alsaSeq (void) const
{
	return m_pAlsaSeq;
}


// Connection primitive.
void qjackctlAlsaConnect::connectPorts (
	qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
#ifdef CONFIG_ALSA_SEQ

	qjackctlAlsaPort *pOAlsa = static_cast<qjackctlAlsaPort *> (pOPort);
	qjackctlAlsaPort *pIAlsa = static_cast<qjackctlAlsaPort *> (pIPort);

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

#endif	// CONFIG_ALSA_SEQ
}


// Disconnection primitive.
void qjackctlAlsaConnect::disconnectPorts (
	qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
#ifdef CONFIG_ALSA_SEQ

	qjackctlAlsaPort *pOAlsa = static_cast<qjackctlAlsaPort *> (pOPort);
	qjackctlAlsaPort *pIAlsa = static_cast<qjackctlAlsaPort *> (pIPort);

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

#endif	// CONFIG_ALSA_SEQ
}


// Update port connection references.
void qjackctlAlsaConnect::updateConnections (void)
{
#ifdef CONFIG_ALSA_SEQ

	snd_seq_query_subscribe_t *pAlsaSubs;
	snd_seq_addr_t seq_addr;

	snd_seq_query_subscribe_alloca(&pAlsaSubs);

	// Proper type casts.
	qjackctlAlsaClientList *pOClientList
		= static_cast<qjackctlAlsaClientList *> (OClientList());
	qjackctlAlsaClientList *pIClientList
		= static_cast<qjackctlAlsaClientList *> (IClientList());

	// For each output client item...
	QListIterator<qjackctlClientItem *> oclient(pOClientList->clients());
	while (oclient.hasNext()) {
		qjackctlClientItem *pOClient = oclient.next();
		// For each output port item...
		QListIterator<qjackctlPortItem *> oport(pOClient->ports());
		while (oport.hasNext()) {
			qjackctlPortItem *pOPort = oport.next();
			// Are there already any connections?
			if (pOPort->connects().count() > 0)
				continue;
			// Hava a proper type cast.
			qjackctlAlsaPort *pOAlsa
				= static_cast<qjackctlAlsaPort *> (pOPort);
			// Get port connections...
			snd_seq_query_subscribe_set_type(pAlsaSubs, SND_SEQ_QUERY_SUBS_READ);
			snd_seq_query_subscribe_set_index(pAlsaSubs, 0);
			seq_addr.client = pOAlsa->alsaClient();
			seq_addr.port   = pOAlsa->alsaPort();
			snd_seq_query_subscribe_set_root(pAlsaSubs, &seq_addr);
			while (snd_seq_query_port_subscribers(m_pAlsaSeq, pAlsaSubs) >= 0) {
				seq_addr = *snd_seq_query_subscribe_get_addr(pAlsaSubs);
				qjackctlPortItem *pIPort
					= pIClientList->findClientPort(
						seq_addr.client, seq_addr.port);
				if (pIPort) {
					pOPort->addConnect(pIPort);
					pIPort->addConnect(pOPort);
				}
				snd_seq_query_subscribe_set_index(pAlsaSubs,
					snd_seq_query_subscribe_get_index(pAlsaSubs) + 1);
			}
		}
	}

#endif	// CONFIG_ALSA_SEQ
}


// Update icon size implementation.
void qjackctlAlsaConnect::updateIconPixmaps (void)
{
	deleteIconPixmaps();
	createIconPixmaps();
}


// end of qjackctlAlsaConnect.cpp
