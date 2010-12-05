// qjackctlSession.cpp
//
/****************************************************************************
   Copyright (C) 2003-2010, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qjackctlAbout.h"
#include "qjackctlSession.h"

#include "qjackctlMainForm.h"

#ifdef CONFIG_JACK_SESSION
#include <errno.h>
#include <jack/session.h>
#endif

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDomDocument>
#include <QTextStream>

#include <QApplication>
#include <QProcess>
#include <QTime>


//----------------------------------------------------------------------------
// qjackctlSession -- JACK session container.

// Constructor.
qjackctlSession::qjackctlSession (void)
{
}


// Destructor.
qjackctlSession::~qjackctlSession (void)
{
	clear();
}


// Client list accessor (read-only)
const qjackctlSession::ClientList& qjackctlSession::clients (void) const
{
	return m_clients;
}


// House-keeper.
void qjackctlSession::clear (void)
{
	qDeleteAll(m_clients);
	m_clients.clear();
}


// Critical methods.
bool qjackctlSession::save ( const QString& sSessionDir, int iSessionType )
{
	// We're always best to
	// reset old session settings.
	clear();

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == NULL)
		return false;

	jack_client_t *pJackClient = pMainForm->jackClient();
	if (pJackClient == NULL)
		return false;

	const QString sSessionPath = sSessionDir + '/';

	// First pass: get all client/port connections, no matter what...
	const char **ports
		= jack_get_ports(pJackClient, NULL, NULL, 0);
	if (ports) {
		for (int i = 0; ports[i]; ++i) {
			const char *pszSrcClientPort = ports[i];
			const QString sSrcClientPort
				= QString::fromLocal8Bit(pszSrcClientPort);
			const QString sSrcClient
				= sSrcClientPort.section(':', 0, 0);
			ClientItem *pClientItem;
			if (m_clients.contains(sSrcClient)) {
				pClientItem = m_clients.value(sSrcClient);
			} else {
				pClientItem = new ClientItem;
				pClientItem->client_name = sSrcClient;
				m_clients.insert(pClientItem->client_name, pClientItem);
			}
			PortItem *pPortItem = new PortItem;
			pPortItem->port_name = sSrcClientPort.section(':', 1);
			jack_port_t *pJackPort
				= jack_port_by_name(pJackClient, pszSrcClientPort);
			pPortItem->port_type
				= (jack_port_flags(pJackPort) & JackPortIsInput);
			const char **connections
				= jack_port_get_all_connections(pJackClient, pJackPort);
			if (connections) {
				for (int j = 0; connections[j]; ++j) {
					const QString sDstClientPort
						= QString::fromLocal8Bit(connections[j]);
					ConnectItem *pConnectItem = new ConnectItem;
					pConnectItem->client_name
						= sDstClientPort.section(':', 0, 0);
					pConnectItem->port_name
						= sDstClientPort.section(':', 1);
					pConnectItem->connected = true;
					pPortItem->connects.append(pConnectItem);
				//	pPortItem->connected++;
				}
#ifdef CONFIG_JACK_FREE
				jack_free(connections);
#else
				free(connections);
#endif
			} 
			pClientItem->ports.append(pPortItem);
		//	pClientItem->connected += pPortItem->connected;
		}
#ifdef CONFIG_JACK_FREE
		jack_free(ports);
#else
		free(ports);
#endif
	}

#ifdef CONFIG_JACK_SESSION

	if (jack_session_notify) {
	
		jack_session_event_type_t etype = JackSessionSave;
		switch (iSessionType) {
		case 1:
			etype = JackSessionSaveAndQuit;
			break;
		case 2:
			etype = JackSessionSaveTemplate;
			break;
		}
	
		const QByteArray aSessionPath = sSessionPath.toLocal8Bit();
		const char *pszSessionPath = aSessionPath.constData();
		jack_session_command_t *commands
			= jack_session_notify(pJackClient, NULL, etype, pszSessionPath);
		if (commands == NULL)
			return false;
	
		// Second pass...
		for (int k = 0; commands[k].uuid; ++k) {
			jack_session_command_t *pCommand = &commands[k];
			const QString sClientName
				= QString::fromLocal8Bit(pCommand->client_name);
			ClientItem *pClientItem;
			if (m_clients.contains(sClientName)) {
				pClientItem = m_clients.value(sClientName);
			} else {
				pClientItem = new ClientItem;
				pClientItem->client_name = sClientName;
				m_clients.insert(pClientItem->client_name, pClientItem);
			}
			pClientItem->client_uuid
				= QString::fromLocal8Bit(pCommand->uuid);
			pClientItem->client_command
				= QString::fromLocal8Bit(pCommand->command);
		}
	
		jack_session_commands_free(commands);
	}

#endif	// CONFIG_JACK_SESSION

	// Save to file, immediate...
	return saveFile(sSessionPath + "session.xml");	
}


bool qjackctlSession::load ( const QString& sSessionDir )
{
	// Load from file, immediate...
	const QString sSessionPath = sSessionDir + '/';
	if (!loadFile(sSessionPath + "session.xml"))
		return false;

	// Start/load clients...
	int msecs = 200;
	ClientList::ConstIterator iter = m_clients.constBegin();
	for ( ; iter != m_clients.constEnd(); ++iter) {
		const ClientItem *pClientItem = iter.value();
		if (pClientItem->client_command.isEmpty())
			continue;
		const QString sClientDir
			= sSessionPath + pClientItem->client_name + '/';
		QString sCommand = pClientItem->client_command;
		sCommand.replace("${SESSION_DIR}", sClientDir);
		if (QProcess::startDetached(sCommand))
			msecs += 200;
	}

	// Wait a litle bit before continue...
	QTime t;
	t.start();
	while (t.elapsed() < msecs)
		QApplication::processEvents(/* QEventLoop::ExcludeUserInputEvents */);

	// Initial reconnection.
	update();

	// Formerly successful.
	return true;
}


// Update (re)connections utility method.
bool qjackctlSession::update (void)
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == NULL)
		return false;

	jack_client_t *pJackClient = pMainForm->jackClient();
	if (pJackClient == NULL)
		return false;

	// We'll count how many connection updates...
	int iUpdate = 0;

	// Now, make the saved connections...
	ClientList::ConstIterator iter = m_clients.constBegin();
	for ( ; iter != m_clients.constEnd(); ++iter) {
		ClientItem *pClientItem = iter.value();
		if (pClientItem->connected < 1)
			continue;
		QString sSrcClient = pClientItem->client_name;
	#ifdef CONFIG_JACK_SESSION
		if (!pClientItem->client_uuid.isEmpty()) {
			const QByteArray aSrcUuid
				= pClientItem->client_uuid.toLocal8Bit();
			const char *pszSrcUuid = aSrcUuid.constData(); 
			const char *pszSrcClient
				= jack_get_client_name_by_uuid(pJackClient, pszSrcUuid);
			if (pszSrcClient)
				sSrcClient = QString::fromLocal8Bit(pszSrcClient);
		}
	#endif
		QListIterator<PortItem *> iterPort(pClientItem->ports);
		while (iterPort.hasNext()) {
			PortItem *pPortItem = iterPort.next();
			if (pPortItem->connected < 1)
				continue;
			const QString sSrcPort = pPortItem->port_name;
			const QString sSrcClientPort = sSrcClient + ':' + sSrcPort;
			const QByteArray aSrcClientPort = sSrcClientPort.toLocal8Bit();
			QListIterator<ConnectItem *> iterConnect(pPortItem->connects);
			while (iterConnect.hasNext()) {
				ConnectItem *pConnectItem = iterConnect.next();
				if (pConnectItem->connected)
					continue;
				QString sDstClient = pConnectItem->client_name;
			#ifdef CONFIG_JACK_SESSION
				if (m_clients.contains(sDstClient)) {
					const QString sDstUuid
						= m_clients.value(sDstClient)->client_uuid;
					const QByteArray aDstUuid = sDstUuid.toLocal8Bit();
					const char *pszDstUuid = aDstUuid.constData();
					const char *pszDstClient
						= jack_get_client_name_by_uuid(pJackClient, pszDstUuid);
					if (pszDstClient)
						sDstClient = QString::fromLocal8Bit(pszDstClient);
				}
			#endif
				const QString sDstPort = pConnectItem->port_name;
				const QString sDstClientPort = sDstClient + ':' + sDstPort;
				const QByteArray aDstClientPort = sDstClientPort.toLocal8Bit();
				int retc;
				if (pPortItem->port_type) {
					// Input port...
					retc = jack_connect(pJackClient,
						aDstClientPort.constData(),
						aSrcClientPort.constData());
				#ifdef CONFIG_DEBUG
					qDebug("qjackctlSession::update() "
						"jack_connect: \"%s\" => \"%s\" (%d)",
						aDstClientPort.constData(),
						aSrcClientPort.constData(), retc);
				#endif
				} else {
					// Output port...
					retc = jack_connect(pJackClient,
						aSrcClientPort.constData(),
						aDstClientPort.constData());
				#ifdef CONFIG_DEBUG
					qDebug("qjackctlSession::update() "
						"jack_connect: \"%s\" => \"%s\" (%d)",
						aSrcClientPort.constData(),
						aDstClientPort.constData(), retc);
				#endif
				}
				// Comply with connection result...
				if (retc == 0 || retc == EEXIST) {
					pConnectItem->connected = true;
					pPortItem->connected--;
					pClientItem->connected--;
					iUpdate++;
				}
			}
		}
	}

	// Report back.
	return (iUpdate > 0);
}


// Specific session file load (read) method.
bool qjackctlSession::loadFile ( const QString& sFilename )
{
	// Open file...
	QFile file(sFilename);

	if (!file.open(QIODevice::ReadOnly))
		return false;

	// Parse it a-la-DOM :-)
	QDomDocument doc("qjackctlSession");
	if (!doc.setContent(&file)) {
		file.close();
		return false;
	}
	file.close();

	// Now, we're always best to
	// reset old session settings.
	clear();

	// Get root element.
	QDomElement eDoc = doc.documentElement();

	// Parse for clients...
	for (QDomNode nClient = eDoc.firstChild();
			!nClient.isNull();
				nClient = nClient.nextSibling()) {

		// Client element...
		QDomElement eClient = nClient.toElement();
		if (eClient.isNull())
			continue;

		if (eClient.tagName() != "client")
			continue;

		ClientItem *pClientItem = new ClientItem;
		pClientItem->client_name = eClient.attribute("name");
		pClientItem->client_uuid = eClient.attribute("uuid");

		// Client properties...
		for (QDomNode nChild = eClient.firstChild();
				!nChild.isNull();
					nChild = nChild.nextSibling()) {

			// Client child element...
			QDomElement eChild = nChild.toElement();
			if (eChild.isNull())
				continue;

			if (eChild.tagName() == "command")
				pClientItem->client_command = eChild.text();
			else
			if (eChild.tagName() == "port") {
				// Port properties...
				PortItem *pPortItem = new PortItem;
				pPortItem->port_name = eChild.attribute("name");
				pPortItem->port_type = (eChild.attribute("type") == "in");
				// Connection child element...
				for (QDomNode nConnect = eChild.firstChild();
						!nConnect.isNull();
							nConnect = nConnect.nextSibling()) {
					QDomElement eConnect = nConnect.toElement();
					if (eConnect.isNull())
						continue;
					if (eConnect.tagName() != "connect")
						continue;
					ConnectItem *pConnectItem = new ConnectItem;
					pConnectItem->client_name = eConnect.attribute("client");
					pConnectItem->port_name   = eConnect.attribute("port");
					pConnectItem->connected   = false;
					pPortItem->connects.append(pConnectItem);
					pPortItem->connected++;
				}
				pClientItem->ports.append(pPortItem);
				pClientItem->connected += pPortItem->connected;
			}
		}

		m_clients.insert(pClientItem->client_name, pClientItem);
	}

	return true;
}


// Specific session file save (write) method.
bool qjackctlSession::saveFile ( const QString& sFilename )
{
	QFileInfo fi(sFilename);

	QDomDocument doc("qjackctlSession");
	QDomElement eSession = doc.createElement("session");
	eSession.setAttribute("name", QFileInfo(fi.absolutePath()).baseName());
	doc.appendChild(eSession);

	// Save clients spec...
	ClientList::ConstIterator iter = m_clients.constBegin();

	for (; iter != m_clients.constEnd(); ++iter) {

		const ClientItem *pClientItem = iter.value();
		QDomElement eClient = doc.createElement("client");
		eClient.setAttribute("name", pClientItem->client_name);
		
		if (!pClientItem->client_uuid.isEmpty())
			eClient.setAttribute("uuid", pClientItem->client_uuid);

		if (!pClientItem->client_command.isEmpty()) {
			QDomElement eCommand = doc.createElement("command");
			eCommand.appendChild(doc.createTextNode(pClientItem->client_command));
			eClient.appendChild(eCommand);
		}

		QListIterator<PortItem *> iterPort(pClientItem->ports);
		while (iterPort.hasNext()) {
			const PortItem *pPortItem = iterPort.next();
			QDomElement ePort = doc.createElement("port");
			ePort.setAttribute("name", pPortItem->port_name);
			ePort.setAttribute("type", pPortItem->port_type ? "in" : "out");
			QListIterator<ConnectItem *> iterConnect(pPortItem->connects);
			while (iterConnect.hasNext()) {
				const ConnectItem *pConnectItem = iterConnect.next();
				QDomElement eConnect = doc.createElement("connect");
				eConnect.setAttribute("client", pConnectItem->client_name);
				eConnect.setAttribute("port", pConnectItem->port_name);
				ePort.appendChild(eConnect);
			}
			eClient.appendChild(ePort);
		}
		eSession.appendChild(eClient);
	}

	// Finally, save to external file.
	QFile file(sFilename);

	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;

	QTextStream ts(&file);
	ts << doc.toString() << endl;
	file.close();

	return true;
}


// end of qjackctlSession.cpp
