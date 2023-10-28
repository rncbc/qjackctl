// qjackctlAlsaGraph.cpp
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

#include "qjackctlAlsaGraph.h"

#include "qjackctlMainForm.h"


#ifdef CONFIG_ALSA_SEQ

#include <QMutexLocker>


//----------------------------------------------------------------------------
// qjackctlAlsaGraph -- ALSA graph driver

QMutex qjackctlAlsaGraph::g_mutex;


// Constructor.
qjackctlAlsaGraph::qjackctlAlsaGraph ( qjackctlGraphCanvas *canvas )
	: qjackctlGraphSect(canvas)
{
	resetPortTypeColors();
}


// ALSA port (dis)connection.
void qjackctlAlsaGraph::connectPorts (
	qjackctlGraphPort *port1, qjackctlGraphPort *port2, bool is_connect )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == nullptr)
		return;

	snd_seq_t *seq = pMainForm->alsaSeq();
	if (seq == nullptr)
		return;

	if (port1 == nullptr || port2 == nullptr)
		return;

	const qjackctlGraphNode *node1 = port1->portNode();
	const qjackctlGraphNode *node2 = port2->portNode();

	if (node1 == nullptr || node2 == nullptr)
		return;

	QMutexLocker locker(&g_mutex);

	const int client_id1
		= node1->nodeName().section(':', 0, 0).toInt();
	const int port_id1
		= port1->portName().section(':', 0, 0).toInt();

	const int client_id2
		= node2->nodeName().section(':', 0, 0).toInt();
	const int port_id2
		= port2->portName().section(':', 0, 0).toInt();

#ifdef CONFIG_DEBUG
	qDebug("qjackctlAlsaGraph::connectPorts(%d:%d, %d:%d, %d)",
		client_id1, port_id1, client_id2, port_id2, is_connect);
#endif

	snd_seq_port_subscribe_t *seq_subs;
	snd_seq_addr_t seq_addr;

	snd_seq_port_subscribe_alloca(&seq_subs);

	seq_addr.client = client_id1;
	seq_addr.port = port_id1;
	snd_seq_port_subscribe_set_sender(seq_subs, &seq_addr);

	seq_addr.client = client_id2;
	seq_addr.port = port_id2;
	snd_seq_port_subscribe_set_dest(seq_subs, &seq_addr);

	if (is_connect) {
		snd_seq_subscribe_port(seq, seq_subs);
	} else {
		snd_seq_unsubscribe_port(seq, seq_subs);
	}
}


// ALSA node type inquirer. (static)
bool qjackctlAlsaGraph::isNodeType ( uint node_type )
{
	return (node_type == qjackctlAlsaGraph::nodeType());
}


// ALSA node type.
uint qjackctlAlsaGraph::nodeType (void)
{
	static
	const uint AlsaNodeType
		= qjackctlGraphItem::itemType("ALSA_NODE_TYPE");

	return AlsaNodeType;
}


// ALSA port type inquirer. (static)
bool qjackctlAlsaGraph::isPortType ( uint port_type )
{
	return (port_type == qjackctlAlsaGraph::midiPortType());
}


// ALSA port type.
uint qjackctlAlsaGraph::midiPortType (void)
{
	static
	const uint AlsaMidiPortType
		= qjackctlGraphItem::itemType("ALSA_PORT_TYPE");

	return AlsaMidiPortType;
}


// ALSA client:port finder and creator if not existing.
bool qjackctlAlsaGraph::findClientPort (
	snd_seq_client_info_t *client_info,
	snd_seq_port_info_t *port_info,
	qjackctlGraphItem::Mode port_mode,
	qjackctlGraphNode **node,
	qjackctlGraphPort **port,
	bool add_new )
{
	const int client_id
		= snd_seq_client_info_get_client(client_info);
	const int port_id
		= snd_seq_port_info_get_port(port_info);

	const QString& client_name
		= QString::number(client_id) + ':'
		+ QString::fromUtf8(snd_seq_client_info_get_name(client_info));
	const QString& port_name
		= QString::number(port_id) + ':'
		+ QString::fromUtf8(snd_seq_port_info_get_name(port_info));

	const uint node_type
		= qjackctlAlsaGraph::nodeType();
	const uint port_type
		= qjackctlAlsaGraph::midiPortType();

	qjackctlGraphItem::Mode node_mode = port_mode;

	*node = qjackctlGraphSect::findNode(client_name, node_mode, node_type);
	*port = nullptr;

	if (*node == nullptr && client_id >= 128) {
		node_mode = qjackctlGraphItem::Duplex;
		*node = qjackctlGraphSect::findNode(client_name, node_mode, node_type);
	}

	if (*node)
		*port = (*node)->findPort(port_name, port_mode, port_type);

	if (add_new && *node == nullptr) {
		*node = new qjackctlGraphNode(client_name, node_mode, node_type);
		(*node)->setNodeIcon(QIcon(":/images/graphAlsa.png"));
		qjackctlGraphSect::addItem(*node);
	}

	if (add_new && *port == nullptr && *node) {
		*port = (*node)->addPort(port_name, port_mode, port_type);
		(*port)->updatePortTypeColors(qjackctlGraphSect::canvas());
	}

	if (add_new && *node) {
		int nchanged = 0;
		QString node_title = (*node)->nodeTitle();
		for (qjackctlAliasList *node_aliases : item_aliases(*node))
			node_title = node_aliases->clientAlias(client_name);
		if ((*node)->nodeTitle() != node_title) {
			(*node)->setNodeTitle(node_title);
			++nchanged;
		}
		if (*port) {
			QString port_title = (*port)->portTitle();
			for (qjackctlAliasList *port_aliases : item_aliases(*port))
				port_title = port_aliases->portAlias(client_name, port_name);
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


// ALSA graph updater.
void qjackctlAlsaGraph::updateItems (void)
{
	QMutexLocker locker(&g_mutex);

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == nullptr)
		return;

	snd_seq_t *seq = pMainForm->alsaSeq();
	if (seq == nullptr)
		return;

#ifdef CONFIG_DEBUG_0
	qDebug("qjackctlAlsaGraph::updateItems()");
#endif

	// 1. Client/ports inventory...
	//
	snd_seq_client_info_t *client_info1;
	snd_seq_port_info_t *port_info1;

	snd_seq_client_info_alloca(&client_info1);
	snd_seq_port_info_alloca(&port_info1);

	snd_seq_client_info_set_client(client_info1, -1);

	while (snd_seq_query_next_client(seq, client_info1) >= 0) {
		const int client_id
			= snd_seq_client_info_get_client(client_info1);
		if (0 >= client_id)	// Skip 0:System client...
			continue;
		snd_seq_port_info_set_client(port_info1, client_id);
		snd_seq_port_info_set_port(port_info1, -1);
		while (snd_seq_query_next_port(seq, port_info1) >= 0) {
			const unsigned int port_caps1
				= snd_seq_port_info_get_capability(port_info1);
			if (port_caps1 & SND_SEQ_PORT_CAP_NO_EXPORT)
				continue;
			qjackctlGraphItem::Mode port_mode1 = qjackctlGraphItem::None;
			const unsigned int port_is_input
				= (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE);
			if ((port_caps1 & port_is_input) == port_is_input) {
				port_mode1 = qjackctlGraphItem::Input;
				qjackctlGraphNode *node1 = nullptr;
				qjackctlGraphPort *port1 = nullptr;
				if (findClientPort(client_info1, port_info1,
						port_mode1, &node1, &port1, true)) {
					node1->setMarked(true);
					port1->setMarked(true);
				}
			}
			const unsigned int port_is_output
				= (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ);
			if ((port_caps1 & port_is_output) == port_is_output) {
				port_mode1 = qjackctlGraphItem::Output;
				qjackctlGraphNode *node1 = nullptr;
				qjackctlGraphPort *port1 = nullptr;
				if (findClientPort(client_info1, port_info1,
						port_mode1, &node1, &port1, true)) {
					node1->setMarked(true);
					port1->setMarked(true);
				}
			}
		}
	}

	// 2. Connections inventory...
	//
	snd_seq_client_info_t *client_info2;
	snd_seq_port_info_t *port_info2;

	snd_seq_client_info_alloca(&client_info2);
	snd_seq_port_info_alloca(&port_info2);

	snd_seq_query_subscribe_t *seq_subs;
	snd_seq_addr_t seq_addr;

	snd_seq_query_subscribe_alloca(&seq_subs);

	snd_seq_client_info_set_client(client_info1, -1);

	while (snd_seq_query_next_client(seq, client_info1) >= 0) {
		const int client_id
			= snd_seq_client_info_get_client(client_info1);
		if (0 >= client_id)	// Skip 0:system client...
			continue;
		snd_seq_port_info_set_client(port_info1, client_id);
		snd_seq_port_info_set_port(port_info1, -1);
		while (snd_seq_query_next_port(seq, port_info1) >= 0) {
			const unsigned int port_caps1
				= snd_seq_port_info_get_capability(port_info1);
			if (port_caps1 & SND_SEQ_PORT_CAP_NO_EXPORT)
				continue;
			if (port_caps1 & (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ)) {
				const qjackctlGraphItem::Mode port_mode1
					= qjackctlGraphItem::Output;
				qjackctlGraphNode *node1 = nullptr;
				qjackctlGraphPort *port1 = nullptr;
				if (!findClientPort(client_info1, port_info1,
						port_mode1, &node1, &port1, false))
					continue;
				snd_seq_query_subscribe_set_type(seq_subs, SND_SEQ_QUERY_SUBS_READ);
				snd_seq_query_subscribe_set_index(seq_subs, 0);
				seq_addr.client = client_id;
				seq_addr.port = snd_seq_port_info_get_port(port_info1);
				snd_seq_query_subscribe_set_root(seq_subs, &seq_addr);
				while (snd_seq_query_port_subscribers(seq, seq_subs) >= 0) {
					seq_addr = *snd_seq_query_subscribe_get_addr(seq_subs);
					if (snd_seq_get_any_client_info(seq,
							seq_addr.client, client_info2) >= 0 &&
						snd_seq_get_any_port_info(seq,
							seq_addr.client, seq_addr.port, port_info2) >= 0) {
						const qjackctlGraphItem::Mode port_mode2
							= qjackctlGraphItem::Input;
						qjackctlGraphNode *node2 = nullptr;
						qjackctlGraphPort *port2 = nullptr;
						if (findClientPort(client_info2, port_info2,
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
					snd_seq_query_subscribe_set_index(seq_subs,
						snd_seq_query_subscribe_get_index(seq_subs) + 1);
				}
			}
		}
	}

	// 3. Clean-up all un-marked items...
	//
	qjackctlGraphSect::resetItems(qjackctlAlsaGraph::nodeType());
}


void qjackctlAlsaGraph::clearItems (void)
{
	QMutexLocker locker(&g_mutex);

#ifdef CONFIG_DEBUG_0
	qDebug("qjackctlAlsaGraph::clearItems()");
#endif

	qjackctlGraphSect::clearItems(qjackctlAlsaGraph::nodeType());
}


// Special port-type colors defaults (virtual).
void qjackctlAlsaGraph::resetPortTypeColors (void)
{
	qjackctlGraphCanvas *canvas = qjackctlGraphSect::canvas();
	if (canvas) {
		canvas->setPortTypeColor(
			qjackctlAlsaGraph::midiPortType(),
			QColor(Qt::darkMagenta).darker(120));
	}
}


// Client/port item aliases accessor.
QList<qjackctlAliasList *> qjackctlAlsaGraph::item_aliases (
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
	bool is_node = false;

	if (item->type() == qjackctlGraphNode::Type) {
		qjackctlGraphNode *node = static_cast<qjackctlGraphNode *> (item);
		if (node && qjackctlAlsaGraph::isNodeType(node->nodeType())) {
			item_type = node->nodeType();
			item_mode = node->nodeMode();
			is_node = true;
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

	if (is_node || item_type == qjackctlAlsaGraph::midiPortType()) {
		// ALSA MIDI type...
		if (item_mode & qjackctlGraphItem::Input)
			alist.append(&(aliases->alsaInputs));
		if (item_mode & qjackctlGraphItem::Output)
			alist.append(&(aliases->alsaOutputs));
	}

	return alist; // hopefully non empty!
}


#endif	// CONFIG_ALSA_SEQ


// end of qjackctlAlsaGraph.cpp
