// qjackctlJackGraph.cpp
//
/****************************************************************************
   Copyright (C) 2003-2020, rncbc aka Rui Nuno Capela. All rights reserved.

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

// JACK client/port meta-data property helpers.
//
#include <jack/metadata.h>
#include <jack/uuid.h>

static
QString qjackctlJackGraph_get_property (
	jack_uuid_t uuid, const char *key, const QString& name )
{
	QString property = name;

	char *value = nullptr;
	char *type  = nullptr;

	if (::jack_get_property(uuid, key, &value, &type) == 0) {
		if (value) {
			property = QString::fromUtf8(value);
			::jack_free(value);
		}
		if (type)
			::jack_free(type);
	}

	return property;
}

static
void qjackctlJackGraph_set_property ( jack_client_t *client,
	jack_uuid_t uuid, const char *key, const QString& property )
{
	const char *value = property.toUtf8().constData();

	::jack_set_property(client, uuid, key, value, NULL);
}

static
void qjackctlJackGraph_remove_property (
	jack_client_t *client, jack_uuid_t uuid, const char *key )
{
	::jack_remove_property(client, uuid, key);
}


// Pretty-name property accessors.
//
static
QString qjackctlJackGraph_pretty_name (
	jack_uuid_t uuid, const QString& pretty_name )
{
	return qjackctlJackGraph_get_property(uuid,
		JACK_METADATA_PRETTY_NAME, pretty_name);
}

static
void qjackctlJackGraph_set_pretty_name (
	jack_client_t *client, jack_uuid_t uuid, const QString& pretty_name )
{
	qjackctlJackGraph_set_property(client, uuid,
		JACK_METADATA_PRETTY_NAME, pretty_name);
}

static
void qjackctlJackGraph_remove_pretty_name (
	jack_client_t *client, jack_uuid_t uuid )
{
	qjackctlJackGraph_remove_property(client, uuid,
		JACK_METADATA_PRETTY_NAME);
}


// Port-index property accessors.
//
#ifndef JACKEY_ORDER
#define JACKEY_ORDER "http://jackaudio.org/metadata/order"
#endif

static
int qjackctlJackGraph_port_index ( jack_uuid_t uuid, int index )
{
	return qjackctlJackGraph_get_property(uuid,
		JACKEY_ORDER, QString::number(index)).toInt();
}


#ifdef CONFIG_JACK_CV

// Signal-type property accessors (audio|"cv").
//
#ifndef JACKEY_SIGNAL_TYPE
#define JACKEY_SIGNAL_TYPE "http://jackaudio.org/metadata/signal-type"
#endif

static
QString qjackctlJackGraph_signal_type ( jack_uuid_t uuid )
{
	return qjackctlJackGraph_get_property(uuid,
		JACKEY_SIGNAL_TYPE, QString()).toLower();
}

static
bool qjackctlJackGraph_port_is_cv ( jack_uuid_t uuid )
{
	return qjackctlJackGraph_signal_type(uuid) == "cv";
}

#endif	// CONFIG_JACK_CV

#ifdef CONFIG_JACK_OSC

// Event-types property accessors (MIDI|"osc").
//
#ifndef JACKEY_EVENT_TYPES
#define JACKEY_EVENT_TYPES "http://jackaudio.org/metadata/event-types"
#endif

static
QStringList qjackctlJackGraph_event_types ( jack_uuid_t uuid )
{
	return qjackctlJackGraph_get_property(uuid,
		JACKEY_EVENT_TYPES, QString("midi")).toLower().split(',');
}

static
bool qjackctlJackGraph_port_is_osc ( jack_uuid_t uuid )
{
	return qjackctlJackGraph_event_types(uuid).contains("osc");
}

#endif	// CONFIG_JACK_OSC

#endif	// CONFIG_JACK_METADATA


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
	if (pMainForm == nullptr)
		return;

	jack_client_t *client = pMainForm->jackClient();
	if (client == nullptr)
		return;

	if (port1 == nullptr || port2 == nullptr)
		return;

	const qjackctlGraphNode *node1 = port1->portNode();
	const qjackctlGraphNode *node2 = port2->portNode();

	if (node1 == nullptr || node2 == nullptr)
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
	return port_type == audioPortType()
		|| port_type == midiPortType()
	#ifdef CONFIG_JACK_CV
		|| port_type == cvPortType()
	#endif
	#ifdef CONFIG_JACK_OSC
		|| port_type == oscPortType()
	#endif
	;
}


uint qjackctlJackGraph::audioPortType (void)
{
	return qjackctlGraphItem::itemType(JACK_DEFAULT_AUDIO_TYPE);
}

uint qjackctlJackGraph::midiPortType (void)
{
	return qjackctlGraphItem::itemType(JACK_DEFAULT_MIDI_TYPE);
}

uint qjackctlJackGraph::cvPortType (void)
{
	return qjackctlGraphItem::itemType("JACK_SIGNAL_TYPE_CV");
}

uint qjackctlJackGraph::oscPortType (void)
{
	return qjackctlGraphItem::itemType("JACK_EVENT_TYPE_OSC");
}


// JACK client:port finder and creator if not existing.
bool qjackctlJackGraph::findClientPort ( jack_client_t *client,
	const char *client_port, qjackctlGraphItem::Mode port_mode,
	qjackctlGraphNode **node, qjackctlGraphPort **port, bool add_new )
{
	jack_port_t *jack_port = ::jack_port_by_name(client, client_port);
	if (jack_port == nullptr)
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
	uint port_type
		= qjackctlGraphItem::itemType(port_type_name);
#ifdef CONFIG_JACK_METADATA
	const jack_uuid_t port_uuid
		= ::jack_port_uuid(jack_port);
#ifdef CONFIG_JACK_CV
	if (port_type == audioPortType()
		&& qjackctlJackGraph_port_is_cv(port_uuid))
		port_type = cvPortType();
#endif
#ifdef CONFIG_JACK_OSC
	if (port_type == midiPortType()
		&& qjackctlJackGraph_port_is_osc(port_uuid))
		port_type = oscPortType();
#endif
#endif	// CONFIG_JACK_METADATA
	qjackctlGraphItem::Mode node_mode = port_mode;

	*node = qjackctlGraphSect::findNode(client_name, node_mode, node_type);
	*port = nullptr;

	if (*node == nullptr) {
		const unsigned long port_flags = ::jack_port_flags(jack_port);
		const unsigned long port_flags_mask
			= (JackPortIsPhysical | JackPortIsTerminal);
		if ((port_flags & port_flags_mask) != port_flags_mask) {
			node_mode = qjackctlGraphItem::Duplex;
			*node = qjackctlGraphSect::findNode(client_name, node_mode, node_type);
		}
	}

	if (*node)
		*port = (*node)->findPort(port_name, port_mode, port_type);

	if (add_new && *node == nullptr) {
		*node = new qjackctlGraphNode(client_name, node_mode, node_type);
		(*node)->setNodeIcon(QIcon(":/images/graphJack.png"));
		qjackctlGraphSect::addItem(*node);
	}

	if (add_new && *port == nullptr && *node) {
		*port = (*node)->addPort(port_name, port_mode, port_type);
		(*port)->updatePortTypeColors(canvas());
	}

	if (add_new && *node) {
		int nchanged = 0;
		QString node_title = (*node)->nodeTitle();
		foreach (qjackctlAliasList *node_aliases, item_aliases(*node))
			node_title = node_aliases->clientAlias(client_name);
	#ifdef CONFIG_JACK_METADATA
		const char *client_uuid_name
			= ::jack_get_uuid_for_client_name(client,
				client_name.toUtf8().constData());
		if (client_uuid_name) {
			jack_uuid_t client_uuid = 0;
			::jack_uuid_parse(client_uuid_name, &client_uuid);
			const QString& pretty_name
				= qjackctlJackGraph_pretty_name(client_uuid, client_name);
			if (!pretty_name.isEmpty())
				node_title = pretty_name;
			::jack_free((void *) client_uuid_name);
		}
	#endif	// CONFIG_JACK_METADATA
		if ((*node)->nodeTitle() != node_title) {
			(*node)->setNodeTitle(node_title);
			++nchanged;
		}
		if (*port) {
			QString port_title = (*port)->portTitle();
			foreach (qjackctlAliasList *port_aliases, item_aliases(*port))
				port_title = port_aliases->portAlias(client_name, port_name);
		#ifdef CONFIG_JACK_METADATA
			const QString& pretty_name
				= qjackctlJackGraph_pretty_name(port_uuid, port_name);
			if (!pretty_name.isEmpty())
				port_title = pretty_name;
			const int port_index
				= qjackctlJackGraph_port_index(port_uuid, 0);
			if ((*port)->portIndex() != port_index) {
				(*port)->setPortIndex(port_index);
				++nchanged;
			}
		#endif	// CONFIG_JACK_METADATA
			if ((*port)->portTitle() != port_title) {
				(*port)->setPortTitle(port_title);
				++nchanged;
			}
		}
		if (nchanged > 0)
			(*node)->updatePath();
	}

	return (*node && *port);
}


// JACK graph updaters.
void qjackctlJackGraph::updateItems (void)
{
	QMutexLocker locker(&g_mutex);

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == nullptr)
		return;

	jack_client_t *client = pMainForm->jackClient();
	if (client == nullptr)
		return;

#ifdef CONFIG_DEBUG_0
	qDebug("qjackctlJackGraph::updateItems()");
#endif

	// 1. Client/ports inventory...
	//
	const char **client_ports1
		= ::jack_get_ports(client, nullptr, nullptr, 0);
	if (client_ports1 == nullptr)
		return;

	for (int i = 0; client_ports1[i]; ++i) {
		const char *client_port1 = client_ports1[i];
		jack_port_t *jack_port1 = ::jack_port_by_name(client, client_port1);
		if (jack_port1 == nullptr)
			continue;
		const unsigned long port_flags1
			= ::jack_port_flags(jack_port1);
		qjackctlGraphItem::Mode port_mode1 = qjackctlGraphItem::None;
		if (port_flags1 & JackPortIsInput)
			port_mode1 = qjackctlGraphItem::Input;
		else
		if (port_flags1 & JackPortIsOutput)
			port_mode1 = qjackctlGraphItem::Output;
		qjackctlGraphNode *node1 = nullptr;
		qjackctlGraphPort *port1 = nullptr;
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
		if (jack_port1 == nullptr)
			continue;
		const unsigned long port_flags1
			= ::jack_port_flags(jack_port1);
		if (port_flags1 & JackPortIsOutput) {
			const qjackctlGraphItem::Mode port_mode1
				= qjackctlGraphItem::Output;
			const char **client_ports2
				= ::jack_port_get_all_connections(client, jack_port1);
			if (client_ports2 == nullptr)
				continue;
			qjackctlGraphNode *node1 = nullptr;
			qjackctlGraphPort *port1 = nullptr;
			if (findClientPort(client, client_port1,
					port_mode1, &node1, &port1, false)) {
				for (int j = 0; client_ports2[j]; ++j) {
					const char *client_port2 = client_ports2[j];
					const qjackctlGraphItem::Mode port_mode2
						= qjackctlGraphItem::Input;
					qjackctlGraphNode *node2 = nullptr;
					qjackctlGraphPort *port2 = nullptr;
					if (findClientPort(client, client_port2,
							port_mode2, &node2, &port2, false)) {
						qjackctlGraphConnect *connect = port1->findConnect(port2);
						if (connect == nullptr) {
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

#ifdef CONFIG_DEBUG_0
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
	#ifdef CONFIG_JACK_CV
		canvas->setPortTypeColor(
			qjackctlJackGraph::cvPortType(),
			QColor(Qt::darkCyan).darker(120));
	#endif
	#ifdef CONFIG_JACK_OSC
		canvas->setPortTypeColor(
			qjackctlJackGraph::oscPortType(),
			QColor(Qt::darkYellow).darker(120));
	#endif
	}
}


// Client/port item aliases accessor.
QList<qjackctlAliasList *> qjackctlJackGraph::item_aliases (
	qjackctlGraphItem *item ) const
{
	QList<qjackctlAliasList *> alist;

	qjackctlAliases *aliases = nullptr;
	qjackctlGraphCanvas *canvas = qjackctlGraphSect::canvas();
	if (canvas)
		aliases = canvas->aliases();
	if (aliases == nullptr)
		return alist; // empty!

	uint item_type = 0;
	qjackctlGraphItem::Mode item_mode = qjackctlGraphItem::None;

	if (item->type() == qjackctlGraphNode::Type) {
		qjackctlGraphNode *node = static_cast<qjackctlGraphNode *> (item);
		if (node) {
			item_type = node->nodeType();
			item_mode = node->nodeMode();
		}
	}
	else
	if (item->type() == qjackctlGraphPort::Type) {
		qjackctlGraphPort *port = static_cast<qjackctlGraphPort *> (item);
		if (port) {
			item_type = port->portType();
			item_mode = port->portMode();
		}
	}

	if (!item_type || !item_mode)
		return alist; // empty again!

	if (item_type == qjackctlJackGraph::audioPortType()) {
		// JACK audio type...
		if (item_mode & qjackctlGraphItem::Input)
			alist.append(&(aliases->audioInputs));
		if (item_mode & qjackctlGraphItem::Output)
			alist.append(&(aliases->audioOutputs));
	}
	else
	if (item_type == qjackctlJackGraph::midiPortType()) {
		// JACK MIDI type...
		if (item_mode & qjackctlGraphItem::Input)
			alist.append(&(aliases->midiInputs));
		if (item_mode & qjackctlGraphItem::Output)
			alist.append(&(aliases->midiOutputs));
	}

	return alist; // hopefully non empty!
}


// Client/port renaming method (virtual override).
void qjackctlJackGraph::renameItem (
	qjackctlGraphItem *item, const QString& name )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == nullptr)
		return;

	jack_client_t *client = pMainForm->jackClient();
	if (client == nullptr)
		return;

#ifdef CONFIG_JACK_METADATA
	qjackctlGraphNode *node = nullptr;
	if (item->type() == qjackctlGraphNode::Type) {
		qjackctlGraphNode *node = static_cast<qjackctlGraphNode *> (item);
		if (node) {
			const QString& node_name
				= node->nodeName();
			const QByteArray client_name
				= node_name.toUtf8();
			const char *client_uuid_name
				= ::jack_get_uuid_for_client_name(client, client_name.constData());
			if (client_uuid_name) {
				jack_uuid_t client_uuid = 0;
				::jack_uuid_parse(client_uuid_name, &client_uuid);
				if (name.isEmpty())
					qjackctlJackGraph_remove_pretty_name(client, client_uuid);
				else
					qjackctlJackGraph_set_pretty_name(client, client_uuid, name);
				::jack_free((void *) client_uuid_name);
			}
		}
	}
	else
	if (item->type() == qjackctlGraphPort::Type) {
		qjackctlGraphPort *port = static_cast<qjackctlGraphPort *> (item);
		if (port)
			node = port->portNode();
		if (port && node) {
			const QString& port_name
				= port->portName();
			const QString& client_port
				= node->nodeName() + ':' + port_name;
			const QByteArray client_port_name
				= client_port.toUtf8();
			jack_port_t *jack_port
				= ::jack_port_by_name(client, client_port_name.constData());
			if (jack_port) {
				jack_uuid_t port_uuid = ::jack_port_uuid(jack_port);
				if (name.isEmpty())
					qjackctlJackGraph_remove_pretty_name(client, port_uuid);
				else
					qjackctlJackGraph_set_pretty_name(client, port_uuid, name);
			}
		}
	}
#endif	// CONFIG_JACK_METADATA

	qjackctlGraphSect::renameItem(item, name);
}


// end of qjackctlJackGraph.cpp
