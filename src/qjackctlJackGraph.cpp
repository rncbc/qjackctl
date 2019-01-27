// qjackctlJackGraph.cpp
//
/****************************************************************************
   Copyright (C) 2003-2018, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qjackctlJackGraph.h"

#include "qjackctlMainForm.h"

#include <QMutexLocker>


//----------------------------------------------------------------------------
// qjackctlJackGraph -- JACK graph driver

QMutex qjackctlJackGraph::g_mutex;


#ifdef CONFIG_JACK_METADATA

// JACK client/port metad-ata property helpers.
//
#include <jack/metadata.h>
#include <jack/uuid.h>

static
QString qjackctlJackGraph_pretty_name ( jack_uuid_t uuid, const QString& name )
{
	QString pretty_name = name;

	char *value = NULL;
	char *type  = NULL;

	if (::jack_get_property(uuid,
			JACK_METADATA_PRETTY_NAME, &value, &type) == 0) {
		if (value) {
			pretty_name = QString::fromUtf8(value);
			::jack_free(value);
		}
		if (type)
			::jack_free(type);
	}

	return pretty_name;
}

#ifndef JACKEY_ORDER
#define JACKEY_ORDER "http://jackaudio.org/metadata/order"
#endif

static
int qjackctlJackGraph_port_index ( jack_uuid_t uuid, int index )
{
	int port_index = index;

	char *value = NULL;
	char *type  = NULL;

	if (::jack_get_property(uuid,
			JACKEY_ORDER, &value, &type) == 0) {
		if (value) {
			port_index = QString::fromUtf8(value).toInt();
			::jack_free(value);
		}
		if (type)
			::jack_free(type);
	}

	return port_index;
}

#endif


// Constructor.
qjackctlJackGraph::qjackctlJackGraph ( qjackctlGraphCanvas *canvas )
	: qjackctlGraphSect(canvas)
{
		resetPortTypeColors();
}


// JACK port (dis)connection.
void qjackctlJackGraph::connectPorts (
	qjackctlGraphPort *port1, qjackctlGraphPort *port2, bool connect )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	jack_client_t *client = pMainForm->jackClient();
	if (client == NULL)
		return;

	if (port1 == NULL || port2 == NULL)
		return;

	const qjackctlGraphNode *node1 = port1->portNode();
	const qjackctlGraphNode *node2 = port2->portNode();

	if (node1 == NULL || node2 == NULL)
		return;

	QMutexLocker locker(&g_mutex);

	const QByteArray client_port1
		= QString(node1->nodeName() + ':' + port1->portName()).toUtf8();
	const QByteArray client_port2
		= QString(node2->nodeName() + ':' + port2->portName()).toUtf8();

	const char *client_port_name1 = client_port1.constData();
	const char *client_port_name2 = client_port2.constData();

#ifdef CONFIG_DEBUG
	qDebug("qjackctlJackGraph::connectPorts(\"%s\", \"%s\", %d)",
		client_port_name1, client_port_name2, connect);
#endif

	if (connect) {
		::jack_connect(client, client_port_name1, client_port_name2);
	} else {
		::jack_disconnect(client, client_port_name1, client_port_name2);
	}
}


// JACK node type inquirer. (static)
bool qjackctlJackGraph::isNodeType ( uint node_type )
{
	return (node_type == qjackctlJackGraph::nodeType());
}


// JACK node type.
uint qjackctlJackGraph::nodeType (void)
{
	static
	const uint JackNodeType
		= qjackctlGraphItem::itemType("JACK_NODE_TYPE");

	return JackNodeType;
}


// JACK port type(s) inquirer. (static)
bool qjackctlJackGraph::isPortType ( uint port_type )
{
	return (port_type == audioPortType() || port_type == midiPortType());
}


uint qjackctlJackGraph::audioPortType (void)
{
	static
	const uint JackAudioPortType
		= qjackctlGraphItem::itemType(JACK_DEFAULT_AUDIO_TYPE);

	return JackAudioPortType;
}


uint qjackctlJackGraph::midiPortType (void)
{
	static
	const uint JackMidiPortType
		= qjackctlGraphItem::itemType(JACK_DEFAULT_MIDI_TYPE);

	return JackMidiPortType;
}


// JACK client:port finder and creator if not existing.
bool qjackctlJackGraph::findClientPort ( jack_client_t *client,
	const char *client_port, qjackctlGraphItem::Mode port_mode,
	qjackctlGraphNode **node, qjackctlGraphPort **port, bool add_new )
{
	jack_port_t *jack_port = ::jack_port_by_name(client, client_port);
	if (jack_port == NULL)
		return false;

	const QString& client_port_name
		= QString::fromUtf8(client_port);
	const int colon = client_port_name.indexOf(':');
	if (colon < 0)
		return false;

	const QString& client_name
		= client_port_name.left(colon);
	const QString& port_name
		= client_port_name.right(client_port_name.length() - colon - 1);

	const uint node_type
		= qjackctlJackGraph::nodeType();
	const char *port_type_name
		= ::jack_port_type(jack_port);
	const uint port_type
		= qjackctlGraphItem::itemType(port_type_name);

	qjackctlGraphItem::Mode node_mode = port_mode;

	*node = qjackctlGraphSect::findNode(client_name, node_mode, node_type);
	*port = NULL;

	if (*node == NULL) {
		const unsigned long port_flags = ::jack_port_flags(jack_port);
		if ((port_flags & (JackPortIsPhysical | JackPortIsTerminal)) != (JackPortIsPhysical | JackPortIsTerminal)) {
			node_mode = qjackctlGraphItem::Duplex;
			*node = qjackctlGraphSect::findNode(client_name, node_mode, node_type);
		}
	}

	if (*node)
		*port = (*node)->findPort(port_name, port_mode, port_type);

	if (add_new && *node == NULL) {
		*node = new qjackctlGraphNode(client_name, node_mode, node_type);
		(*node)->setNodeIcon(QIcon(":/images/graphJack.png"));
		qjackctlGraphSect::addItem(*node);
	}

	if (add_new && *port == NULL && *node) {
		*port = (*node)->addPort(port_name, port_mode, port_type);
		(*port)->updatePortTypeColors(canvas());
	}

#ifdef CONFIG_JACK_METADATA
	if (*node) {
		int nchanged = 0;
		const char *client_uuid_name
			= ::jack_get_uuid_for_client_name(client,
				client_name.toUtf8().constData());
		if (client_uuid_name) {
			jack_uuid_t client_uuid = 0;
			::jack_uuid_parse(client_uuid_name, &client_uuid);
			const QString& node_title
				= qjackctlJackGraph_pretty_name(client_uuid, client_name);
			if ((*node)->nodeTitle() != node_title) {
				(*node)->setNodeTitle(node_title);
				++nchanged;
			}
			::jack_free((void *) client_uuid_name);
		}
		if (*port) {
			const jack_uuid_t port_uuid
				= ::jack_port_uuid(jack_port);
			const QString& port_title
				= qjackctlJackGraph_pretty_name(port_uuid, port_name);
			if ((*port)->portTitle() != port_title) {
				(*port)->setPortTitle(port_title);
				++nchanged;
			}
			const int port_index
				= qjackctlJackGraph_port_index(port_uuid, 0);
			if ((*port)->portIndex() != port_index) {
				(*port)->setPortIndex(port_index);
				++nchanged;
			}
		}
		if (nchanged > 0)
			(*node)->updatePath();
	}
#endif

	return (*node && *port);
}


// JACK graph updaters.
void qjackctlJackGraph::updateItems (void)
{
	QMutexLocker locker(&g_mutex);

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	jack_client_t *client = pMainForm->jackClient();
	if (client == NULL)
		return;

#ifdef CONFIG_DEBUG
	qDebug("qjackctlJackGraph::updateItems()");
#endif

	// 1. Client/ports inventory...
	//
	const char **client_ports1
		= ::jack_get_ports(client, NULL, NULL, 0);
	if (client_ports1 == NULL)
		return;

	for (int i = 0; client_ports1[i]; ++i) {
		const char *client_port1 = client_ports1[i];
		jack_port_t *jack_port1 = ::jack_port_by_name(client, client_port1);
		if (jack_port1 == NULL)
			continue;
		const unsigned long port_flags1
			= ::jack_port_flags(jack_port1);
		qjackctlGraphItem::Mode port_mode1 = qjackctlGraphItem::None;
		if (port_flags1 & JackPortIsInput)
			port_mode1 = qjackctlGraphItem::Input;
		else
		if (port_flags1 & JackPortIsOutput)
			port_mode1 = qjackctlGraphItem::Output;
		qjackctlGraphNode *node1 = NULL;
		qjackctlGraphPort *port1 = NULL;
		if (findClientPort(client, client_port1,
				port_mode1, &node1, &port1, true)) {
			node1->setMarked(true);
			port1->setMarked(true);
		}
	}

	// 2. Connections inventory...
	//
	for (int i = 0; client_ports1[i]; ++i) {
		const char *client_port1 = client_ports1[i];
		jack_port_t *jack_port1 = ::jack_port_by_name(client, client_port1);
		if (jack_port1 == NULL)
			continue;
		const unsigned long port_flags1
			= ::jack_port_flags(jack_port1);
		if (port_flags1 & JackPortIsOutput) {
			const qjackctlGraphItem::Mode port_mode1
				= qjackctlGraphItem::Output;
			const char **client_ports2
				= ::jack_port_get_all_connections(client, jack_port1);
			if (client_ports2 == NULL)
				continue;
			qjackctlGraphNode *node1 = NULL;
			qjackctlGraphPort *port1 = NULL;
			if (findClientPort(client, client_port1,
					port_mode1, &node1, &port1, false)) {
				for (int j = 0; client_ports2[j]; ++j) {
					const char *client_port2 = client_ports2[j];
					const qjackctlGraphItem::Mode port_mode2
						= qjackctlGraphItem::Input;
					qjackctlGraphNode *node2 = NULL;
					qjackctlGraphPort *port2 = NULL;
					if (findClientPort(client, client_port2,
							port_mode2, &node2, &port2, false)) {
						qjackctlGraphConnect *connect = port1->findConnect(port2);
						if (connect == NULL) {
							connect = new qjackctlGraphConnect();
							connect->setPort1(port1);
							connect->setPort2(port2);
							connect->updatePortTypeColors();
							connect->updatePath();
							qjackctlGraphSect::addItem(connect);
						}
						if (connect)
							connect->setMarked(true);
					}
				}
			}
			::free(client_ports2);
		}
	}

	::free(client_ports1);

	// 3. Clean-up all un-marked items...
	//
	qjackctlGraphSect::resetItems(qjackctlJackGraph::nodeType());
}


void qjackctlJackGraph::clearItems (void)
{
	QMutexLocker locker(&g_mutex);

#ifdef CONFIG_DEBUG
	qDebug("qjackctlJackGraph::clearItems()");
#endif

	qjackctlGraphSect::clearItems(qjackctlJackGraph::nodeType());
}


// Special port-type colors defaults (virtual).
void qjackctlJackGraph::resetPortTypeColors (void)
{
	qjackctlGraphCanvas *canvas = qjackctlGraphSect::canvas();
	if (canvas) {
		canvas->setPortTypeColor(
			qjackctlJackGraph::audioPortType(),
			QColor(Qt::darkGreen).darker(120));
		canvas->setPortTypeColor(
			qjackctlJackGraph::midiPortType(),
			QColor(Qt::darkRed).darker(120));
	}
}


// end of qjackctlJackGraph.cpp
