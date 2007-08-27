// qjackctlJackConnect.cpp
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

#include "qjackctlJackConnect.h"

#include <QPixmap>


//----------------------------------------------------------------------
// class qjackctlJackPort -- Jack port list item.
//

// Constructor.
qjackctlJackPort::qjackctlJackPort ( qjackctlJackClient *pClient,
	const QString& sPortName, jack_port_t *pJackPort )
	: qjackctlPortItem(pClient, sPortName)
{
	m_pJackPort = pJackPort;

	qjackctlJackConnect *pJackConnect
		= static_cast<qjackctlJackConnect *> (
			((pClient->clientList())->listView())->binding());

	if (pJackConnect) {
		unsigned long ulPortFlags = jack_port_flags(m_pJackPort);
		if (ulPortFlags & JackPortIsInput) {
			if (ulPortFlags & JackPortIsTerminal) {
				QTreeWidgetItem::setIcon(0, QIcon(pJackConnect->pixmap(
					ulPortFlags & JackPortIsPhysical
						? QJACKCTL_JACK_PORTPTI : QJACKCTL_JACK_PORTLTI)));
			} else {
				QTreeWidgetItem::setIcon(0, QIcon(pJackConnect->pixmap(
					ulPortFlags & JackPortIsPhysical
						? QJACKCTL_JACK_PORTPNI : QJACKCTL_JACK_PORTLNI)));
			}
		} else if (ulPortFlags & JackPortIsOutput) {
			if (ulPortFlags & JackPortIsTerminal) {
				QTreeWidgetItem::setIcon(0, QIcon(pJackConnect->pixmap(
					ulPortFlags & JackPortIsPhysical
						? QJACKCTL_JACK_PORTPTO : QJACKCTL_JACK_PORTLTO)));
			} else {
				QTreeWidgetItem::setIcon(0, QIcon(pJackConnect->pixmap(
					ulPortFlags & JackPortIsPhysical
						? QJACKCTL_JACK_PORTPNO : QJACKCTL_JACK_PORTLNO)));
			}
		}
	}
}

// Default destructor.
qjackctlJackPort::~qjackctlJackPort (void)
{
}


// Jack handles accessors.
jack_client_t *qjackctlJackPort::jackClient (void) const
{
	return ((qjackctlJackClient *) client())->jackClient();
}

jack_port_t *qjackctlJackPort::jackPort (void) const
{
	return m_pJackPort;
}


//----------------------------------------------------------------------
// class qjackctlJackClient -- Jack client list item.
//

// Constructor.
qjackctlJackClient::qjackctlJackClient ( qjackctlJackClientList *pClientList,
	const QString& sClientName )
	: qjackctlClientItem(pClientList, sClientName)
{
	qjackctlJackConnect *pJackConnect
		= static_cast<qjackctlJackConnect *> (
			(pClientList->listView())->binding());

	if (pJackConnect) {
		if (pClientList->isReadable()) {
			QTreeWidgetItem::setIcon(0,
				QIcon(pJackConnect->pixmap(QJACKCTL_JACK_CLIENTO)));
		} else {
			QTreeWidgetItem::setIcon(0,
				QIcon(pJackConnect->pixmap(QJACKCTL_JACK_CLIENTI)));
		}
	}
}

// Default destructor.
qjackctlJackClient::~qjackctlJackClient (void)
{
}


// Jack client accessor.
jack_client_t *qjackctlJackClient::jackClient (void) const
{
	return ((qjackctlJackClientList *) clientList())->jackClient();
}


//----------------------------------------------------------------------
// qjackctlJackClientList -- Jack client list.
//

// Constructor.
qjackctlJackClientList::qjackctlJackClientList (
	qjackctlClientListView *pListView,
	jack_client_t *pJackClient, bool bReadable )
	: qjackctlClientList(pListView, bReadable)
{
	m_pJackClient = pJackClient;
}

// Default destructor.
qjackctlJackClientList::~qjackctlJackClientList (void)
{
}


// Jack client accessor.
jack_client_t *qjackctlJackClientList::jackClient (void) const
{
	return m_pJackClient;
}


// Client:port refreshner.
int qjackctlJackClientList::updateClientPorts (void)
{
	if (m_pJackClient == 0)
		return 0;

	qjackctlJackConnect *pJackConnect
		= static_cast<qjackctlJackConnect *> (listView()->binding());
	if (pJackConnect == 0)
		return 0;

	const char *pszJackType = JACK_DEFAULT_AUDIO_TYPE;
#ifdef CONFIG_JACK_MIDI
	if (pJackConnect->jackType() == QJACKCTL_JACK_MIDI)
		pszJackType = JACK_DEFAULT_MIDI_TYPE;
#endif

	int iDirtyCount = 0;

	markClientPorts(0);

	const char **ppszClientPorts = jack_get_ports(m_pJackClient, 0,
		pszJackType, isReadable() ? JackPortIsOutput : JackPortIsInput);
	if (ppszClientPorts) {
		int iClientPort = 0;
		while (ppszClientPorts[iClientPort]) {
			QString sClientPort = ppszClientPorts[iClientPort];
			qjackctlJackClient *pClient = 0;
			qjackctlJackPort   *pPort   = 0;
			int iColon = sClientPort.indexOf(':');
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
					jack_port_t *pJackPort = jack_port_by_name(m_pJackClient, ppszClientPorts[iClientPort]);
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
qjackctlJackConnect::qjackctlJackConnect ( qjackctlConnectView *pConnectView,
	jack_client_t *pJackClient, int iJackType  )
	: qjackctlConnect(pConnectView)
{
	m_iJackType = iJackType;

	createIconPixmaps();

	setOClientList(new qjackctlJackClientList(
		connectView()->OListView(), pJackClient, true));
	setIClientList(new qjackctlJackClientList(
		connectView()->IListView(), pJackClient, false));
}

// Default destructor.
qjackctlJackConnect::~qjackctlJackConnect (void)
{
	deleteIconPixmaps();
}


// Connection type accessors.
int qjackctlJackConnect::jackType (void) const
{
	return m_iJackType;
}


// Local pixmap-set janitor methods.
void qjackctlJackConnect::createIconPixmaps (void)
{
	switch (m_iJackType) {
	case QJACKCTL_JACK_MIDI:
		m_apPixmaps[QJACKCTL_JACK_CLIENTI] = createIconPixmap("mclienti");
		m_apPixmaps[QJACKCTL_JACK_CLIENTO] = createIconPixmap("mcliento");
		m_apPixmaps[QJACKCTL_JACK_PORTPTI] = createIconPixmap("mporti");
		m_apPixmaps[QJACKCTL_JACK_PORTPTO] = createIconPixmap("mporto");
		m_apPixmaps[QJACKCTL_JACK_PORTPNI] = createIconPixmap("mporti");
		m_apPixmaps[QJACKCTL_JACK_PORTPNO] = createIconPixmap("mporto");
		m_apPixmaps[QJACKCTL_JACK_PORTLTI] = createIconPixmap("mporti");
		m_apPixmaps[QJACKCTL_JACK_PORTLTO] = createIconPixmap("mporto");
		m_apPixmaps[QJACKCTL_JACK_PORTLNI] = createIconPixmap("mporti");
		m_apPixmaps[QJACKCTL_JACK_PORTLNO] = createIconPixmap("mporto");
		break;
	case QJACKCTL_JACK_AUDIO:
	default:
		m_apPixmaps[QJACKCTL_JACK_CLIENTI] = createIconPixmap("aclienti");
		m_apPixmaps[QJACKCTL_JACK_CLIENTO] = createIconPixmap("acliento");
		m_apPixmaps[QJACKCTL_JACK_PORTPTI] = createIconPixmap("aportpti");
		m_apPixmaps[QJACKCTL_JACK_PORTPTO] = createIconPixmap("aportpto");
		m_apPixmaps[QJACKCTL_JACK_PORTPNI] = createIconPixmap("aportpni");
		m_apPixmaps[QJACKCTL_JACK_PORTPNO] = createIconPixmap("aportpno");
		m_apPixmaps[QJACKCTL_JACK_PORTLTI] = createIconPixmap("aportlti");
		m_apPixmaps[QJACKCTL_JACK_PORTLTO] = createIconPixmap("aportlto");
		m_apPixmaps[QJACKCTL_JACK_PORTLNI] = createIconPixmap("aportlni");
		m_apPixmaps[QJACKCTL_JACK_PORTLNO] = createIconPixmap("aportlno");
		break;
	}
}

void qjackctlJackConnect::deleteIconPixmaps (void)
{
	for (int i = 0; i < QJACKCTL_JACK_PIXMAPS; i++) {
		if (m_apPixmaps[i])
			delete m_apPixmaps[i];
		m_apPixmaps[i] = NULL;
	}
}


// Common pixmap accessor (static).
const QPixmap& qjackctlJackConnect::pixmap ( int iPixmap ) const
{
	return *m_apPixmaps[iPixmap];
}


// Connection primitive.
bool qjackctlJackConnect::connectPorts ( qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
	qjackctlJackPort *pOJack = static_cast<qjackctlJackPort *> (pOPort);
	qjackctlJackPort *pIJack = static_cast<qjackctlJackPort *> (pIPort);

	return (jack_connect(pOJack->jackClient(),
		pOJack->clientPortName().toUtf8().constData(),
		pIJack->clientPortName().toUtf8().constData()) == 0);
}


// Disconnection primitive.
bool qjackctlJackConnect::disconnectPorts ( qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
	qjackctlJackPort *pOJack = static_cast<qjackctlJackPort *> (pOPort);
	qjackctlJackPort *pIJack = static_cast<qjackctlJackPort *> (pIPort);

	return (jack_disconnect(pOJack->jackClient(),
		pOJack->clientPortName().toUtf8().constData(),
		pIJack->clientPortName().toUtf8().constData()) == 0);
}


// Update port connection references.
void qjackctlJackConnect::updateConnections (void)
{
	// For each output client item...
	QListIterator<qjackctlClientItem *> oclient(OClientList()->clients());
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
			qjackctlJackPort *pOJack
				= static_cast<qjackctlJackPort *> (pOPort);
			// Get port connections...
			const char **ppszClientPorts
				= jack_port_get_all_connections(
					pOJack->jackClient(), pOJack->jackPort());
			if (ppszClientPorts) {
				// Now, for each input client port...
				int iClientPort = 0;
				while (ppszClientPorts[iClientPort]) {
					qjackctlPortItem *pIPort
						= IClientList()->findClientPort(
							ppszClientPorts[iClientPort]);
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


// Update icon size implementation.
void qjackctlJackConnect::updateIconPixmaps (void)
{
	deleteIconPixmaps();
	createIconPixmaps();
}


// end of qjackctlJackConnect.cpp
