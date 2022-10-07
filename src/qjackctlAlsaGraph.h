// qjackctlAlsaGraph.h
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

#ifndef __qjackctlAlsaGraph_h
#define __qjackctlAlsaGraph_h

#include "qjackctlAbout.h"
#include "qjackctlGraph.h"


#ifdef CONFIG_ALSA_SEQ

#include <alsa/asoundlib.h>

#include <QMutex>


//----------------------------------------------------------------------------
// qjackctlAlsaGraph -- ALSA graph driver

class qjackctlAlsaGraph : public qjackctlGraphSect
{
public:

	// Constructor.
	qjackctlAlsaGraph(qjackctlGraphCanvas *canvas);

	// ALSA port (dis)connection.
	void connectPorts(
		qjackctlGraphPort *port1, qjackctlGraphPort *port2, bool is_connect);

	// ALSA graph updaters.
	void updateItems();
	void clearItems();

	// Special port-type colors defaults (virtual).
	void resetPortTypeColors();

	// ALSA node type inquirer.
	static bool isNodeType(uint node_type);
	// ALSA node type.
	static uint nodeType();

	// ALSA port type inquirer.
	static bool isPortType(uint port_type);
	// ALSA port type.
	static uint midiPortType();

protected:

	// ALSA client:port finder and creator if not existing.
	bool findClientPort(snd_seq_client_info_t *client_info,
		snd_seq_port_info_t *port_info, qjackctlGraphItem::Mode port_mode,
		qjackctlGraphNode **node, qjackctlGraphPort **port, bool add_new);

	// Client/port item aliases accessor.
	QList<qjackctlAliasList *> item_aliases(qjackctlGraphItem *item) const;

private:

	// Notifier sanity mutex.
	static QMutex g_mutex;
};


#endif	// CONFIG_ALSA_SEQ


#endif  // __qjackctlAlsaGraph_h

// end of qjackctlAlsaGraph.h
