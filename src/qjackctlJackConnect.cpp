// qjackctlJackConnect.cpp
//
/****************************************************************************
   Copyright (C) 2003-2023, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qjackctlMainForm.h"

#include <QPixmap>

#ifdef CONFIG_JACK_METADATA

#include <jack/metadata.h>
#include <jack/uuid.h>

static QString prettyName (	jack_uuid_t uuid )
{
	QString sPrettyName;

	char *pszValue = nullptr;
	char *pszType  = nullptr;

	if (::jack_get_property(uuid,
			JACK_METADATA_PRETTY_NAME, &pszValue, &pszType) == 0) {
		if (pszValue) {
			sPrettyName = QString::fromUtf8(pszValue);
			::jack_free(pszValue);
		}
		if (pszType)
			::jack_free(pszType);
	}

	return sPrettyName;
}

static void setPrettyName (
	jack_client_t *pJackClient, jack_uuid_t uuid, const QString& sPrettyName )
{
	const QByteArray aPrettyName = sPrettyName.toUtf8();

	::jack_set_property(pJackClient, uuid,
		JACK_METADATA_PRETTY_NAME, aPrettyName.constData(), nullptr);
}

static void removePrettyName (
	jack_client_t *pJackClient, jack_uuid_t uuid )
{
	::jack_remove_property(pJackClient, uuid, JACK_METADATA_PRETTY_NAME);
}

#endif


//----------------------------------------------------------------------
// class qjackctlJackPort -- Jack port list item.
//

// Constructor.
qjackctlJackPort::qjackctlJackPort (
	qjackctlJackClient *pClient, unsigned long ulPortFlags )
	: qjackctlPortItem(pClient)
{
	qjackctlJackConnect *pJackConnect
		= static_cast<qjackctlJackConnect *> (
			((pClient->clientList())->listView())->binding());

	if (pJackConnect) {
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


// Pretty/display name method (virtual override).
void qjackctlJackPort::updatePortName ( bool bRename )
{
	jack_client_t *pJackClient = nullptr;
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pJackClient = pMainForm->jackClient();
	if (pJackClient == nullptr)
		return;

#ifdef CONFIG_JACK_PORT_ALIASES
	const int iJackClientPortAlias
		= qjackctlJackClientList::jackClientPortAlias();
	if (iJackClientPortAlias > 0) {
		char *aliases[2];
		const unsigned short alias_size = jack_port_name_size() + 1;
		aliases[0] = new char [alias_size];
		aliases[1] = new char [alias_size];
		QString sPortName = portName();
		QString sClientPort = clientName() + ':' + sPortName;
		const QByteArray aClientPort = sClientPort.toUtf8();
		const char *pszClientPort = aClientPort.constData();
		jack_port_t *pJackPort = jack_port_by_name(pJackClient, pszClientPort);
		if (pJackPort && jack_port_get_aliases(pJackPort, aliases) >= iJackClientPortAlias) {
			sClientPort = QString::fromUtf8(aliases[iJackClientPortAlias - 1]);
			sPortName = sClientPort.section(':', 1);
		}
		delete [] aliases[0];
		delete [] aliases[1];
		setPortText(sPortName, false);
	}
	else
#endif
#ifdef CONFIG_JACK_METADATA
	if (qjackctlJackClientList::isJackClientPortMetadata()) {
		bool bRenameEnabled = false;
		QString sPortNameEx = portNameAlias(&bRenameEnabled);
		const QString& sPortName = portName();
		const QString& sClientPort = clientName() + ':' + sPortName;
		const QByteArray aClientPort = sClientPort.toUtf8();
		const char *pszClientPort = aClientPort.constData();
		jack_port_t *pJackPort = jack_port_by_name(pJackClient, pszClientPort);
		if (pJackPort) {
			jack_uuid_t port_uuid = ::jack_port_uuid(pJackPort);
			const QString& sPrettyName = prettyName(port_uuid);
			if (sPortNameEx != sPortName && sPortNameEx != sPrettyName) {
				if (sPrettyName.isEmpty() || bRename) {
					setPrettyName(pJackClient, port_uuid, sPortNameEx);
				} else {
					sPortNameEx = sPrettyName;
					setPortNameAlias(sPortNameEx);
				}
			}
			else
			if (sPortNameEx == sPortName
				&& !sPrettyName.isEmpty() && bRename) {
				removePrettyName(pJackClient, port_uuid);
			}
		}
		setPortText(sPortNameEx, bRenameEnabled);
	}
	else
#endif
	qjackctlPortItem::updatePortName(bRename);
}


// Tooltip text builder (virtual override).
QString qjackctlJackPort::tooltip (void) const
{

	jack_client_t *pJackClient = nullptr;
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pJackClient = pMainForm->jackClient();
	if (pJackClient) {
		const QString& sPortName = portName();
		const QString& sClientPort = clientName() + ':' + sPortName;
		const QByteArray aClientPort = sClientPort.toUtf8();
		const char *pszClientPort = aClientPort.constData();
		jack_port_t *pJackPort = jack_port_by_name(pJackClient, pszClientPort);
		if (pJackPort) {
			jack_latency_range_t latency_range;
			jack_port_get_latency_range(pJackPort,
				client()->isReadable() ? JackCaptureLatency : JackPlaybackLatency,
				&latency_range);
			QString sLatency = QString::number(latency_range.min);
			if (latency_range.max > latency_range.min)
				sLatency +=  '-' + QString::number(latency_range.max);
			return QObject::tr("%1 (%2 frames)").arg(portName()).arg(sLatency);
		}
	}

	return portName();
}


//----------------------------------------------------------------------
// class qjackctlJackClient -- Jack client list item.
//

// Constructor.
qjackctlJackClient::qjackctlJackClient (
	qjackctlJackClientList *pClientList )
	: qjackctlClientItem(pClientList)
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


// Jack port lookup.
qjackctlJackPort *qjackctlJackClient::findJackPort ( const QString& sPortName )
{
	QListIterator<qjackctlPortItem *> iter(ports());
	while (iter.hasNext()) {
		qjackctlJackPort *pPort
			= static_cast<qjackctlJackPort *> (iter.next());
		if (pPort && pPort->portName() == sPortName)
			return pPort;
	}

	return nullptr;
}


// Pretty/display name method (virtual override).
void qjackctlJackClient::updateClientName ( bool bRename )
{
	jack_client_t *pJackClient = nullptr;
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pJackClient = pMainForm->jackClient();
	if (pJackClient == nullptr)
		return;

#ifdef CONFIG_JACK_METADATA
	if (qjackctlJackClientList::isJackClientPortMetadata()) {
		bool bRenameEnabled = false;
		QString sClientNameEx = clientNameAlias(&bRenameEnabled);
		const QString& sClientName = clientName();
		const QByteArray aClientName = sClientName.toUtf8();
		const char *pszClientUuid
			= ::jack_get_uuid_for_client_name(pJackClient, aClientName.constData());
		if (pszClientUuid) {
			jack_uuid_t client_uuid = 0;
			::jack_uuid_parse(pszClientUuid, &client_uuid);
			const QString& sPrettyName = prettyName(client_uuid);
			if (sClientNameEx != sClientName && sClientNameEx != sPrettyName) {
				if (sPrettyName.isEmpty() || bRename) {
					setPrettyName(pJackClient, client_uuid, sClientNameEx);
				} else {
					sClientNameEx = sPrettyName;
					setClientNameAlias(sClientNameEx);
				}
			}
			else
			if (sClientNameEx == sClientName
				&& !sPrettyName.isEmpty() && bRename) {
				removePrettyName(pJackClient, client_uuid);
			}
			::jack_free((void *) pszClientUuid);
		}
		setClientText(sClientNameEx, bRenameEnabled);
	}
	else
#endif
	qjackctlClientItem::updateClientName(bRename);
}


//----------------------------------------------------------------------
// qjackctlJackClientList -- Jack client list.
//

// Constructor.
qjackctlJackClientList::qjackctlJackClientList (
	qjackctlClientListView *pListView, bool bReadable )
	: qjackctlClientList(pListView, bReadable)
{
}

// Default destructor.
qjackctlJackClientList::~qjackctlJackClientList (void)
{
}


// Jack port lookup.
qjackctlJackPort *qjackctlJackClientList::findJackClientPort (
	const QString& sClientPort )
{
	const int iColon = sClientPort.indexOf(':');
	if (iColon < 0)
		return nullptr;

	const QString& sClientName
		= sClientPort.left(iColon);
	const QString& sPortName
		= sClientPort.right(sClientPort.length() - iColon - 1);

	QListIterator<qjackctlClientItem *> iter(clients());
	while (iter.hasNext()) {
		qjackctlJackClient *pClient
			= static_cast<qjackctlJackClient *> (iter.next());
		if (pClient && pClient->clientName() == sClientName) {
			qjackctlJackPort *pPort = pClient->findJackPort(sPortName);
			if (pPort)
				return pPort;
		}
	}

	return nullptr;
}


// Client:port refreshner.
int qjackctlJackClientList::updateClientPorts (void)
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == nullptr)
		return 0;

	jack_client_t *pJackClient = pMainForm->jackClient();
	if (pJackClient == nullptr)
		return 0;

	qjackctlJackConnect *pJackConnect
		= static_cast<qjackctlJackConnect *> (listView()->binding());
	if (pJackConnect == nullptr)
		return 0;

	const char *pszJackType = JACK_DEFAULT_AUDIO_TYPE;
#ifdef CONFIG_JACK_MIDI
	if (pJackConnect->jackType() == QJACKCTL_JACK_MIDI)
		pszJackType = JACK_DEFAULT_MIDI_TYPE;
#endif

#ifdef CONFIG_JACK_PORT_ALIASES
	char *aliases[2];
	if (g_iJackClientPortAlias > 0) {
		const unsigned short alias_size = jack_port_name_size() + 1;
		aliases[0] = new char [alias_size];
		aliases[1] = new char [alias_size];
	}
#endif

	int iDirtyCount = 0;

	markClientPorts(0);

	const char **ppszClientPorts = jack_get_ports(pJackClient, 0,
		pszJackType, isReadable() ? JackPortIsOutput : JackPortIsInput);
	if (ppszClientPorts) {
		for (int iClientPort = 0; ppszClientPorts[iClientPort]; ++iClientPort) {
			QString sClientPort = QString::fromUtf8(ppszClientPorts[iClientPort]);
			qjackctlJackClient *pClient = nullptr;
			qjackctlJackPort *pPort = nullptr;
			const char *pszClientPort = ppszClientPorts[iClientPort];
			jack_port_t *pJackPort = jack_port_by_name(pJackClient, pszClientPort);
			int iColon = sClientPort.indexOf(':');
			if (pJackPort && iColon >= 0) {
				QString sClientName = sClientPort.left(iColon);
				QString sPortName = sClientPort.right(sClientPort.length() - iColon - 1);
				pClient = static_cast<qjackctlJackClient *> (findClient(sClientName));
				if (pClient)
					pPort = static_cast<qjackctlJackPort *> (pClient->findPort(sPortName));
				if (pClient == nullptr) {
					pClient = new qjackctlJackClient(this);
					pClient->setClientName(sClientName);
					++iDirtyCount;
				}
				if (pClient && pPort == nullptr) {
					pPort = new qjackctlJackPort(pClient, jack_port_flags(pJackPort));
					pPort->setPortName(sPortName);
					++iDirtyCount;
				}
			#ifdef CONFIG_JACK_PORT_ALIASES
				if (g_iJackClientPortAlias > 0 &&
					jack_port_get_aliases(pJackPort, aliases) >= g_iJackClientPortAlias) {
					sClientPort = QString::fromUtf8(aliases[g_iJackClientPortAlias - 1]);
					iColon = sClientPort.indexOf(':');
					if (iColon >= 0) {
						sClientName = sClientPort.left(iColon);
						sPortName = sClientPort.right(sClientPort.length() - iColon - 1);
					}
				}
				if (!g_bJackClientPortMetadata) {
					if (pClient && pClient->clientText() != sClientName)  {
						pClient->setClientText(sClientName, false);
						++iDirtyCount;
					}
					if (pPort && pPort->portText() != sPortName)  {
						pPort->setPortText(sPortName, false);
						++iDirtyCount;
					}
				}
			#endif
				if (pPort)
					pPort->markClientPort(1);
			}
		}
		jack_free(ppszClientPorts);
	}

	iDirtyCount += cleanClientPorts(0);

#ifdef CONFIG_JACK_PORT_ALIASES
	if (g_iJackClientPortAlias > 0) {
		delete [] aliases[0];
		delete [] aliases[1];
	}
#endif

	return iDirtyCount;
}


// Jack client port aliases mode.
int qjackctlJackClientList::g_iJackClientPortAlias = 0;

void qjackctlJackClientList::setJackClientPortAlias ( int iJackClientPortAlias )
{
	g_iJackClientPortAlias = iJackClientPortAlias;

	if (g_iJackClientPortAlias > 0)
		g_bJackClientPortMetadata = false;
}

int qjackctlJackClientList::jackClientPortAlias (void)
{
	return g_iJackClientPortAlias;
}


// Jack client port pretty-names (metadata) mode.
bool qjackctlJackClientList::g_bJackClientPortMetadata = false;

void qjackctlJackClientList::setJackClientPortMetadata ( bool bJackClientPortMetadata )
{
	g_bJackClientPortMetadata = bJackClientPortMetadata;

	if (g_bJackClientPortMetadata)
		g_iJackClientPortAlias = 0;
}

bool qjackctlJackClientList::isJackClientPortMetadata (void)
{
	return g_bJackClientPortMetadata;
}


//----------------------------------------------------------------------
// qjackctlJackConnect -- Output-to-Input client/ports connection object.
//

// Constructor.
qjackctlJackConnect::qjackctlJackConnect (
	qjackctlConnectView *pConnectView, int iJackType  )
	: qjackctlConnect(pConnectView)
{
	m_iJackType = iJackType;

	createIconPixmaps();

	setOClientList(new qjackctlJackClientList(
		connectView()->OListView(), true));
	setIClientList(new qjackctlJackClientList(
		connectView()->IListView(), false));
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
		m_apPixmaps[i] = nullptr;
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
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == nullptr)
		return false;

	jack_client_t *pJackClient = pMainForm->jackClient();
	if (pJackClient == nullptr)
		return false;

	qjackctlJackPort *pOJack = static_cast<qjackctlJackPort *> (pOPort);
	qjackctlJackPort *pIJack = static_cast<qjackctlJackPort *> (pIPort);

	return (jack_connect(pJackClient,
		pOJack->clientPortName().toUtf8().constData(),
		pIJack->clientPortName().toUtf8().constData()) == 0);
}


// Disconnection primitive.
bool qjackctlJackConnect::disconnectPorts ( qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == nullptr)
		return false;

	jack_client_t *pJackClient = pMainForm->jackClient();
	if (pJackClient == nullptr)
		return false;

	qjackctlJackPort *pOJack = static_cast<qjackctlJackPort *> (pOPort);
	qjackctlJackPort *pIJack = static_cast<qjackctlJackPort *> (pIPort);

	return (jack_disconnect(pJackClient,
		pOJack->clientPortName().toUtf8().constData(),
		pIJack->clientPortName().toUtf8().constData()) == 0);
}


// Update port connection references.
void qjackctlJackConnect::updateConnections (void)
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == nullptr)
		return;

	jack_client_t *pJackClient = pMainForm->jackClient();
	if (pJackClient == nullptr)
		return;

	qjackctlJackClientList *pIClientList
		= static_cast<qjackctlJackClientList *> (IClientList());
	if (pIClientList == nullptr)
		return;

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
			const QString& sClientPort
				= pOJack->clientName() + ':' + pOJack->portName();
			const QByteArray aClientPort = sClientPort.toUtf8();
			const char *pszClientPort = aClientPort.constData();
			jack_port_t *pJackPort
				= jack_port_by_name(pJackClient, pszClientPort);
			if (pJackPort) {
				const char **ppszClientPorts
					= jack_port_get_all_connections(pJackClient, pJackPort);
				if (ppszClientPorts) {
					// Now, for each input client port...
					for (int iClientPort = 0;
							ppszClientPorts[iClientPort]; ++iClientPort) {
						qjackctlPortItem *pIPort
							= pIClientList->findJackClientPort(
								ppszClientPorts[iClientPort]);
						if (pIPort) {
							pOPort->addConnect(pIPort);
							pIPort->addConnect(pOPort);
						}
					}
					jack_free(ppszClientPorts);
				}
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
