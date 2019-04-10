// qjackctlGraphCommand.h
//
/****************************************************************************
   Copyright (C) 2003-2019, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctlGraphCommand_h
#define __qjackctlGraphCommand_h

#include "qjackctlGraph.h"

#include <QUndoCommand>

#include <QString>
#include <QList>


//----------------------------------------------------------------------------
// qgraph1_command -- Generic graph command pattern

class qjackctlGraphCommand : public QUndoCommand
{
public:

	// Constructor.
	qjackctlGraphCommand(qjackctlGraphCanvas *canvas, QUndoCommand *parent = NULL);

	// Accessors.
	qjackctlGraphCanvas *canvas() const
		{ return m_canvas; }

	// Command methods.
	void undo();
	void redo();

protected:

	// Command executive method.
	virtual bool execute(bool is_undo = false) = 0;

private:

	// Command arguments.
	qjackctlGraphCanvas *m_canvas;
};


//----------------------------------------------------------------------------
// qjackctlGraphConnectCommand -- Connect graph command

class qjackctlGraphConnectCommand : public qjackctlGraphCommand
{
public:

	// Constructor.
	qjackctlGraphConnectCommand(qjackctlGraphCanvas *canvas,
		qjackctlGraphPort *port1, qjackctlGraphPort *port2,
		bool is_connect, qjackctlGraphCommand *parent = NULL);

protected:

	// Command item address
	struct Addr
	{
		// Constructors.
		Addr(qjackctlGraphPort *port)
		{
			qjackctlGraphNode *node = port->portNode();
			node_name = node->nodeName();
			node_type = node->nodeType();
			port_name = port->portName();
			port_type = port->portType();
		}
		// Copy constructor.
		Addr(const Addr& addr)
		{
			node_name = addr.node_name;
			node_type = addr.node_type;
			port_name = addr.port_name;
			port_type = addr.port_type;
		}
		// Member fields.
		QString node_name;
		uint    node_type;
		QString port_name;
		uint    port_type;
	};

	// Command item descriptor
	struct Item
	{
		// Constructor.
		Item(qjackctlGraphPort *port1, qjackctlGraphPort *port2, bool is_connect)
			: addr1(port1), addr2(port2), m_connect(is_connect) {}
		// Copy constructor.
		Item(const Item& item) : addr1(item.addr1), addr2(item.addr2),
			m_connect(item.is_connect()) {}

		// Accessors.
		bool is_connect() const
			{ return m_connect; }

		// Public member fields.
		Addr addr1;
		Addr addr2;

	private:

		// Private member fields.
		bool m_connect;
	};

	// Command executive method.
	bool execute(bool is_undo);

private:

	// Command arguments.
	Item m_item;
};


//----------------------------------------------------------------------------
// qjackctlGraphMoveCommand -- Move (node) graph command

class qjackctlGraphMoveCommand : public qjackctlGraphCommand
{
public:

	// Constructor.
	qjackctlGraphMoveCommand(qjackctlGraphCanvas *canvas,
		const QList<qjackctlGraphNode *>& nodes,
		const QPointF& pos1, const QPointF& pos2,
		qjackctlGraphCommand *parent = NULL);

	// Destructor.
	~qjackctlGraphMoveCommand();

protected:

	// Command item descriptor
	struct Item
	{
		QString node_name;
		qjackctlGraphItem::Mode node_mode;
		uint node_type;
	};

	// Command executive method.
	bool execute(bool is_undo);

private:

	// Command arguments.
	QList<Item *> m_items;

	QPointF m_pos1;
	QPointF m_pos2;

	int m_nexec;
};


//----------------------------------------------------------------------------
// qjackctlGraphRenameCommand -- Rename (item) graph command

class qjackctlGraphRenameCommand : public qjackctlGraphCommand
{
public:

	// Constructor.
	qjackctlGraphRenameCommand(qjackctlGraphCanvas *canvas,
		qjackctlGraphItem *item, const QString& name,
		qjackctlGraphCommand *parent = NULL);

protected:

	// Command item descriptor
	struct Item
	{
		int item_type;
		QString node_name;
		qjackctlGraphItem::Mode node_mode;
		uint node_type;
		QString port_name;
		qjackctlGraphItem::Mode port_mode;
		uint port_type;
	};

	// Command executive method.
	bool execute(bool is_undo);

private:

	// Command arguments.
	Item    m_item;
	QString m_name;
};


#endif  // __qjackctlGraphCommand_h

// end of qjackctlGraphCommand.h
