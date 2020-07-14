// qjackctlJackGraph.h
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

#ifndef __qjackctlJackGraph_h
#define __qjackctlJackGraph_h

#include "qjackctlAbout.h"
#include "qjackctlGraph.h"

#include <jack/jack.h>

#include <QMutex>


//----------------------------------------------------------------------------
// qjackctlJackGraph -- JACK graph driver

class qjackctlJackGraph : public qjackctlGraphSect
{
public:

	// Constructor.
	qjackctlJackGraph(qjackctlGraphCanvas *canvas);

	// JACK port (dis)connection.
	void connectPorts(
		qjackctlGraphPort *port1, qjackctlGraphPort *port2, bool connect);

	// JACK graph updaters.
	void updateItems();
	void clearItems();

	// Special port-type colors defaults (virtual).
	void resetPortTypeColors();

	// JACK node type inquirer.
	static bool isNodeType(uint node_type);
	// JACK node type.
	static uint nodeType();

	// JACK port type(s) inquirer.
	static bool isPortType(uint port_type);
	// JACK port types.
	static uint audioPortType();
	static uint midiPortType();
	// JACK port (meta-data provided) types.
	static uint cvPortType();
	static uint oscPortType();

	// Client/port renaming method (virtual override).
	void renameItem(qjackctlGraphItem *item, const QString& name);

protected:

	// JACK client:port finder and creator if not existing.
	bool findClientPort(jack_client_t *client,
		const char *client_port, qjackctlGraphItem::Mode port_mode,
		qjackctlGraphNode **node, qjackctlGraphPort **port, bool add_new);

	// Client/port item aliases accessor.
	QList<qjackctlAliasList *> item_aliases(qjackctlGraphItem *item) const;

private:

	// Callback sanity mutex.
	static QMutex g_mutex;
};


#endif  // __qjackctlJackGraph_h

// end of qjackctlJackGraph.h
