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

#ifdef CONFIG_JACK_SESSION

	if (!jack_session_notify)
		return false;

	jack_session_event_type_t etype = JackSessionSave;
	switch (iSessionType) {
	case 1:
		etype = JackSessionSaveAndQuit;
		break;
	case 2:
		etype = JackSessionSaveTemplate;
		break;
	}

	const QString sSessionPath = sSessionDir + '/';
	const QByteArray aSessionPath = sSessionPath.toLocal8Bit();
	jack_session_command_t *commands
		= jack_session_notify(pJackClient, NULL, etype, aSessionPath.constData());
	if (commands == NULL)
		return false;

	// First pass...
	for (int k = 0; commands[k].uuid; ++k) {
		jack_session_command_t *pCommand = &commands[k];
		ClientItem *pClientItem = new ClientItem;
		pClientItem->client_uuid = QString::fromLocal8Bit(pCommand->uuid);
		pClientItem->client_name = QString::fromLocal8Bit(pCommand->client_name);
		pClientItem->client_command = QString::fromLocal8Bit(pCommand->command);
		m_clients.insert(pClientItem->client_name, pClientItem);
	}

	jack_session_commands_free(commands);

	// Second pass...
	ClientList::ConstIterator iter = m_clients.constBegin();
	for ( ; iter != m_clients.constEnd(); ++iter) {
		ClientItem *pClientItem = iter.value();
		const QString sPorts = pClientItem->client_name + ":.*";
		const QByteArray aPorts = sPorts.toLocal8Bit();
		const char **ports
			= jack_get_ports(pJackClient, aPorts.constData(), NULL, 0);
		if (ports) {
			for (int i = 0; ports[i]; ++i) {
				const char *pszSrcClientPort = ports[i];
				PortItem *pPortItem = new PortItem;
				pPortItem->port_name
					= QString::fromLocal8Bit(pszSrcClientPort).section(':', 1);
				const char **connections
					= jack_port_get_all_connections(m_pJackClient,
						jack_port_by_name(m_pJackClient, pszSrcClientPort));
				if (connections) {
					for (int j = 0; connections[j]; ++j) {
						const QString sDstClientPort
							= QString::fromLocal8Bit(connections[j]);
						const QString sDstPort
							= sDstClientPort.section(':', 1);
						const QString sDstClient
							= sDstClientPort.section(':', 0, 0);
						ConnectItem *pConnectItem = new ConnectItem;
						pConnectItem->client_name = sDstClient;
						pConnectItem->port_name   = sDstPort;
						pPortItem->connects.append(pConnectItem);
						pPortItem->connected++;
					}
					jack_free(connections);
				} 
				pClientItem->ports.append(pPortItem);
				pClientItem->connected += pPortItem->connected;
			}
			jack_free(ports);
		}
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
	int msecs = 0;
	ClientList::ConstIterator iter = m_clients.constBegin();
	for ( ; iter != m_clients.constEnd(); ++iter) {
		const ClientItem *pClientItem = iter.value();
		const QString sClientDir = sSessionPath + pClientItem->client_name + '/';
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
	return update();
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
	for (iter = m_clients.constBegin(); iter != m_clients.constEnd(); ++iter) {
		ClientItem *pClientItem = iter.value();
		if (pClientItem->connected < 1)
			continue;
	#ifdef CONFIG_JACK_SESSION
		const QByteArray aSrcUuid = pClientItem->client_uuid.toLocal8Bit();
		const char *pszSrcUuid = aSrcUuid.constData(); 
		const char *pszSrcClient
			= jack_get_client_name_by_uuid(pJackClient, pszSrcUuid);
		const QString sSrcClient = (pszSrcClient
			? QString::fromLocal8Bit(pszSrcClient)
			: pClientItem->client_name);
	#else
		const QString sSrcClient = pClientItem->client_name;
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
				if (jack_connect(m_pJackClient,
						aSrcClientPort.constData(),
						aDstClientPort.constData()) == 0) {
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
		eClient.setAttribute("uuid", pClientItem->client_uuid);

		QDomElement eCommand = doc.createElement("command");
		eCommand.appendChild(doc.createTextNode(pClientItem->client_command));
		eClient.appendChild(eCommand);

		QListIterator<PortItem *> iterPort(pClientItem->ports);
		while (iterPort.hasNext()) {
			const PortItem *pPortItem = iterPort.next();
			QDomElement ePort = doc.createElement("port");
			ePort.setAttribute("name", pPortItem->port_name);
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
