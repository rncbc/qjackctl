// qjackctlGraphCommand.cpp
//
/****************************************************************************
   Copyright (C) 2003-2024, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include "qjackctlGraphCommand.h"


//----------------------------------------------------------------------------
// qjackctlGraphCommand -- Generic graph command pattern

// Constructor.
qjackctlGraphCommand::qjackctlGraphCommand ( qjackctlGraphCanvas *canvas,
	QUndoCommand *parent ) : QUndoCommand(parent),
		m_canvas(canvas)
{
}


// Command methods.
void qjackctlGraphCommand::undo (void)
{
	execute(true);
}


void qjackctlGraphCommand::redo (void)
{
	execute(false);
}


//----------------------------------------------------------------------------
// qjackctlGraphConnectCommand -- Connect graph command pattern

// Constructor.
qjackctlGraphConnectCommand::qjackctlGraphConnectCommand ( qjackctlGraphCanvas *canvas,
	qjackctlGraphPort *port1, qjackctlGraphPort *port2, bool is_connect,
	qjackctlGraphCommand *parent ) : qjackctlGraphCommand(canvas, parent),
		m_item(port1, port2, is_connect)
{
}


// Command executive
bool qjackctlGraphConnectCommand::execute ( bool is_undo )
{
	qjackctlGraphCanvas *canvas = qjackctlGraphCommand::canvas();
	if (canvas == nullptr)
		return false;

	qjackctlGraphNode *node1
		= canvas->findNode(
			m_item.addr1.node_name,
			qjackctlGraphItem::Output,
			m_item.addr1.node_type);
	if (node1 == nullptr)
		node1 = canvas->findNode(
			m_item.addr1.node_name,
			qjackctlGraphItem::Duplex,
			m_item.addr1.node_type);
	if (node1 == nullptr)
		return false;

	qjackctlGraphPort *port1
		= node1->findPort(
			m_item.addr1.port_name,
			qjackctlGraphItem::Output,
			m_item.addr1.port_type);
	if (port1 == nullptr)
		return false;

	qjackctlGraphNode *node2
		= canvas->findNode(
			m_item.addr2.node_name,
			qjackctlGraphItem::Input,
			m_item.addr2.node_type);
	if (node2 == nullptr)
		node2 = canvas->findNode(
			m_item.addr2.node_name,
			qjackctlGraphItem::Duplex,
			m_item.addr2.node_type);
	if (node2 == nullptr)
		return false;

	qjackctlGraphPort *port2
		= node2->findPort(
			m_item.addr2.port_name,
			qjackctlGraphItem::Input,
			m_item.addr2.port_type);
	if (port2 == nullptr)
		return false;

	const bool is_connect
		= (m_item.is_connect() && !is_undo) || (!m_item.is_connect() && is_undo);
	if (is_connect)
		canvas->emitConnected(port1, port2);
	else
		canvas->emitDisconnected(port1, port2);

	return true;
}


//----------------------------------------------------------------------------
// qjackctlGraphMoveCommand -- Move (node) graph command

// Constructor.
qjackctlGraphMoveCommand::qjackctlGraphMoveCommand ( qjackctlGraphCanvas *canvas,
	const QList<qjackctlGraphNode *>& nodes, const QPointF& pos1, const QPointF& pos2,
	qjackctlGraphCommand *parent ) : qjackctlGraphCommand(canvas, parent), m_nexec(0)
{
	qjackctlGraphCommand::setText(QObject::tr("Move"));

	const QPointF delta = (pos1 - pos2);

	foreach (qjackctlGraphNode *node, nodes) {
		Item *item = new Item;
		item->node_name = node->nodeName();
		item->node_mode = node->nodeMode();
		item->node_type = node->nodeType();
		const QPointF& pos = node->pos();
		item->node_pos1 = pos + delta;
		item->node_pos2 = pos;
		m_items.insert(node, item);
	}

	if (canvas && canvas->isRepelOverlappingNodes()) {
		foreach (qjackctlGraphNode *node, nodes)
			canvas->repelOverlappingNodes(node, this);
	}
}


// Destructor.
qjackctlGraphMoveCommand::~qjackctlGraphMoveCommand (void)
{
	qDeleteAll(m_items);
	m_items.clear();
}


// Add/replace (an already moved) node position for undo/redo...
void qjackctlGraphMoveCommand::addItem (
	qjackctlGraphNode *node, const QPointF& pos1, const QPointF& pos2 )
{
	Item *item = m_items.value(node, nullptr);
	if (item) {
	//	item->node_pos1 = pos1;
		item->node_pos2 = pos2;//node->pos();
	} else {
		item = new Item;
		item->node_name = node->nodeName();
		item->node_mode = node->nodeMode();
		item->node_type = node->nodeType();
		item->node_pos1 = pos1;
		item->node_pos2 = pos2;//node->pos();
		m_items.insert(node, item);
	}
}



// Command executive method.
bool qjackctlGraphMoveCommand::execute ( bool /* is_undo */ )
{
	qjackctlGraphCanvas *canvas = qjackctlGraphCommand::canvas();
	if (canvas == nullptr)
		return false;

	if (++m_nexec > 1) {
		foreach (qjackctlGraphNode *key, m_items.keys()) {
			Item *item = m_items.value(key, nullptr);
			if (item) {
				qjackctlGraphNode *node = canvas->findNode(
					item->node_name, item->node_mode, item->node_type);
				if (node) {
					const QPointF pos1 = item->node_pos1;
					node->setPos(pos1);
					item->node_pos1 = item->node_pos2;
					item->node_pos2 = pos1;
				}
			}
		}
	}

	canvas->emitChanged();
	return true;
}


//----------------------------------------------------------------------------
// qjackctlGraphRenameCommand -- Rename (item) graph command

// Constructor.
qjackctlGraphRenameCommand::qjackctlGraphRenameCommand ( qjackctlGraphCanvas *canvas,
	qjackctlGraphItem *item, const QString& name, qjackctlGraphCommand *parent )
	: qjackctlGraphCommand(canvas, parent), m_name(name)
{
	qjackctlGraphCommand::setText(QObject::tr("Rename"));

	m_item.item_type = item->type();

	qjackctlGraphNode *node = nullptr;
	qjackctlGraphPort *port = nullptr;

	if (m_item.item_type == qjackctlGraphNode::Type)
		node = static_cast<qjackctlGraphNode *> (item);
	else
	if (m_item.item_type == qjackctlGraphPort::Type)
		port = static_cast<qjackctlGraphPort *> (item);

	if (port)
		node = port->portNode();

	if (node) {
		m_item.node_name = node->nodeName();
		m_item.node_mode = node->nodeMode();
		m_item.node_type = node->nodeType();
	}

	if (port) {
		m_item.port_name = port->portName();
		m_item.port_mode = port->portMode();
		m_item.port_type = port->portType();
	}
}


// Command executive method.
bool qjackctlGraphRenameCommand::execute ( bool /*is_undo*/ )
{
	qjackctlGraphCanvas *canvas = qjackctlGraphCommand::canvas();
	if (canvas == nullptr)
		return false;

	QString name = m_name;
	qjackctlGraphItem *item = nullptr;

	qjackctlGraphNode *node = canvas->findNode(
		m_item.node_name, m_item.node_mode, m_item.node_type);

	if (m_item.item_type == qjackctlGraphNode::Type && node) {
		m_name = node->nodeTitle();
		item = node;
	}
	else
	if (m_item.item_type == qjackctlGraphPort::Type && node) {
		qjackctlGraphPort *port = node->findPort(
			m_item.port_name, m_item.port_mode, m_item.port_type);
		if (port) {
			m_name = port->portTitle();
			item = port;
		}
	}

	if (item == nullptr)
		return false;

	canvas->emitRenamed(item, name);
	return true;
}


// end of qjackctlGraphCommand.cpp
