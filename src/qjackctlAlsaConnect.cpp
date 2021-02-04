// qjackctlAlsaConnect.cpp
//
/****************************************************************************
   Copyright (C) 2003-2021, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qjackctlMainForm.h"

#include <QPixmap>


//----------------------------------------------------------------------
// class qjackctlAlsaPort -- Alsa port list item.
//

// Constructor.
qjackctlAlsaPort::qjackctlAlsaPort ( qjackctlAlsaClient *pClient )
	: qjackctlPortItem(pClient)
{
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
	return portName().section(':', 0, 0).toInt();
}


//----------------------------------------------------------------------
// class qjackctlAlsaClient -- Alsa client list item.
//

// Constructor.
qjackctlAlsaClient::qjackctlAlsaClient ( qjackctlAlsaClientList *pClientList )
	: qjackctlClientItem(pClientList)
{
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
	return clientName().section(':', 0, 0).toInt();
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

	return nullptr;
}


//----------------------------------------------------------------------
// qjackctlAlsaClientList -- Jack client list.
//

// Constructor.
qjackctlAlsaClientList::qjackctlAlsaClientList (
	qjackctlClientListView *pListView, bool bReadable )
	: qjackctlClientList(pListView, bReadable)
{
}

// Default destructor.
qjackctlAlsaClientList::~qjackctlAlsaClientList (void)
{
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

	return nullptr;
}


// Client port finder by id.
qjackctlAlsaPort *qjackctlAlsaClientList::findClientPort ( int iAlsaClient,
	int iAlsaPort )
{
	qjackctlAlsaClient *pClient = findClient(iAlsaClient);
	if (pClient == nullptr)
		return nullptr;

	return pClient->findPort(iAlsaPort);
}


// Client:port refreshner.
int qjackctlAlsaClientList::updateClientPorts (void)
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == nullptr)
		return 0;

	snd_seq_t *pAlsaSeq = pMainForm->alsaSeq();
	if (pAlsaSeq == nullptr)
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

	while (snd_seq_query_next_client(pAlsaSeq, pClientInfo) >= 0) {

		const int iAlsaClient = snd_seq_client_info_get_client(pClientInfo);
		if (iAlsaClient > 0) {
			qjackctlAlsaClient *pClient = findClient(iAlsaClient);
			snd_seq_port_info_set_client(pPortInfo, iAlsaClient);
			snd_seq_port_info_set_port(pPortInfo, -1);
			while (snd_seq_query_next_port(pAlsaSeq, pPortInfo) >= 0) {
				const unsigned int uiPortCapability
					= snd_seq_port_info_get_capability(pPortInfo);
				if (((uiPortCapability & uiAlsaFlags) == uiAlsaFlags) &&
					((uiPortCapability & SND_SEQ_PORT_CAP_NO_EXPORT) == 0)) {
					QString sClientName = QString::number(iAlsaClient) + ':';
					sClientName += QString::fromUtf8(
						snd_seq_client_info_get_name(pClientInfo));
					qjackctlAlsaPort *pPort = 0;
					const int iAlsaPort = snd_seq_port_info_get_port(pPortInfo);
					if (pClient == 0) {
						pClient = new qjackctlAlsaClient(this);
						pClient->setClientName(sClientName);
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
						sPortName += QString::fromUtf8(
							snd_seq_port_info_get_name(pPortInfo));
						if (pPort == 0) {
							pPort = new qjackctlAlsaPort(pClient);
							pPort->setPortName(sPortName);
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

	iDirtyCount += cleanClientPorts(0);

	return iDirtyCount;
}


//----------------------------------------------------------------------
// qjackctlAlsaConnect -- Output-to-Input client/ports connection object.
//

// Constructor.
qjackctlAlsaConnect::qjackctlAlsaConnect ( qjackctlConnectView *pConnectView )
	: qjackctlConnect(pConnectView)
{
	createIconPixmaps();

	setOClientList(new qjackctlAlsaClientList(
		connectView()->OListView(), true));
	setIClientList(new qjackctlAlsaClientList(
		connectView()->IListView(), false));
}

// Default destructor.
qjackctlAlsaConnect::~qjackctlAlsaConnect (void)
{
	deleteIconPixmaps();
}


// Common pixmap accessor (static).
const QPixmap& qjackctlAlsaConnect::pixmap ( int iPixmap ) const
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
		m_apPixmaps[i] = nullptr;
	}
}


#ifndef CONFIG_ALSA_SEQ
#if defined(Q_CC_GNU) || defined(Q_CC_MINGW)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#endif

// Connection primitive.
bool qjackctlAlsaConnect::connectPorts (
	qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
#ifdef CONFIG_ALSA_SEQ

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == nullptr)
		return false;

	snd_seq_t *pAlsaSeq = pMainForm->alsaSeq();
	if (pAlsaSeq == nullptr)
		return false;

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

	return (snd_seq_subscribe_port(pAlsaSeq, pAlsaSubs) >= 0);

#else

	return false;

#endif	// CONFIG_ALSA_SEQ
}


// Disconnection primitive.
bool qjackctlAlsaConnect::disconnectPorts (
	qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
#ifdef CONFIG_ALSA_SEQ

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == nullptr)
		return false;

	snd_seq_t *pAlsaSeq = pMainForm->alsaSeq();
	if (pAlsaSeq == nullptr)
		return false;

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

	return (snd_seq_unsubscribe_port(pAlsaSeq, pAlsaSubs) >= 0);

#else

	return false;

#endif	// CONFIG_ALSA_SEQ
}


// Update port connection references.
void qjackctlAlsaConnect::updateConnections (void)
{
#ifdef CONFIG_ALSA_SEQ

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == nullptr)
		return;

	snd_seq_t *pAlsaSeq = pMainForm->alsaSeq();
	if (pAlsaSeq == nullptr)
		return;

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
			while (snd_seq_query_port_subscribers(pAlsaSeq, pAlsaSubs) >= 0) {
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

#ifndef CONFIG_ALSA_SEQ
#if defined(Q_CC_GNU) || defined(Q_CC_MINGW)
#pragma GCC diagnostic pop
#endif
#endif


// Update icon size implementation.
void qjackctlAlsaConnect::updateIconPixmaps (void)
{
	deleteIconPixmaps();
	createIconPixmaps();
}


// end of qjackctlAlsaConnect.cpp
