// qjackctlPatchbayFile.cpp
//
/****************************************************************************
   Copyright (C) 2003-2011, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include "qjackctlPatchbayFile.h"

#include <QDomDocument>
#include <QTextStream>
#include <QFileInfo>


//----------------------------------------------------------------------
// Specific patchbay socket list save (write) subroutine.
static void load_socketlist ( QList<qjackctlPatchbaySocket *>& socketlist,
	QDomElement& eSockets )
{
	for (QDomNode nSocket = eSockets.firstChild();
			!nSocket.isNull();
				nSocket = nSocket.nextSibling()) {
		// Convert slot node to element...
		QDomElement eSocket = nSocket.toElement();
		if (eSocket.isNull())
			continue;
		if (eSocket.tagName() == "socket") {
			QString sSocketName = eSocket.attribute("name");
			QString sClientName = eSocket.attribute("client");
			QString sSocketType = eSocket.attribute("type");
			QString sExclusive  = eSocket.attribute("exclusive");
			QString sSocketForward = eSocket.attribute("forward");
			int iSocketType = qjackctlPatchbaySocket::typeFromText(sSocketType);
			bool bExclusive = (sExclusive == "on" || sExclusive == "yes" || sExclusive == "1");
			qjackctlPatchbaySocket *pSocket
				= new qjackctlPatchbaySocket(sSocketName, sClientName, iSocketType);
			if (pSocket) {
				pSocket->setExclusive(bExclusive);
				pSocket->setForward(sSocketForward);
				// Now's time to handle pluglist...
				for (QDomNode nPlug = eSocket.firstChild();
						!nPlug.isNull();
							nPlug = nPlug.nextSibling()) {
					// Convert plug node to element...
					QDomElement ePlug = nPlug.toElement();
					if (ePlug.isNull())
						continue;
					if (ePlug.tagName() == "plug")
						pSocket->addPlug(ePlug.text());
				}
				socketlist.append(pSocket);
			}
		}
	}
}


//----------------------------------------------------------------------
// Specific patchbay socket list save (write) subroutine.
static void save_socketlist ( QList<qjackctlPatchbaySocket *>& socketlist,
	QDomElement& eSockets, QDomDocument& doc )
{
	QListIterator<qjackctlPatchbaySocket *> sockit(socketlist);
	while (sockit.hasNext()) {
		qjackctlPatchbaySocket *pSocket = sockit.next();
		QDomElement eSocket = doc.createElement("socket");
		eSocket.setAttribute("name", pSocket->name());
		eSocket.setAttribute("client", pSocket->clientName());
		eSocket.setAttribute("type",
			qjackctlPatchbaySocket::textFromType(pSocket->type()));
		eSocket.setAttribute("exclusive", (pSocket->isExclusive() ? "on" : "off"));
		if (!pSocket->forward().isEmpty())
			eSocket.setAttribute("forward", pSocket->forward());
		QDomElement ePlug;
		QStringListIterator iter(pSocket->pluglist());
		while (iter.hasNext()) {
			const QString& sPlug = iter.next();
			QDomElement ePlug = doc.createElement("plug");
			QDomText text = doc.createTextNode(sPlug);
			ePlug.appendChild(text);
			eSocket.appendChild(ePlug);
		}
		eSockets.appendChild(eSocket);
	}
}


//----------------------------------------------------------------------
// class qjackctlPatchbayFile -- Patchbay file helper implementation.
//

// Specific patchbay load (read) method.
bool qjackctlPatchbayFile::load ( qjackctlPatchbayRack *pPatchbay,
	const QString& sFilename )
{
	// Open file...
	QFile file(sFilename);
	if (!file.open(QIODevice::ReadOnly))
		return false;
	// Parse it a-la-DOM :-)
	QDomDocument doc("patchbay");
	if (!doc.setContent(&file)) {
		file.close();
		return false;
	}
	file.close();

	// Now e're better reset any old patchbay settings.
	pPatchbay->clear();

	// Get root element.
	QDomElement eDoc = doc.documentElement();

	// Now parse for slots, sockets and cables...
	for (QDomNode nRoot = eDoc.firstChild();
			!nRoot.isNull();
				nRoot = nRoot.nextSibling()) {

		// Convert node to element, if any.
		QDomElement eRoot = nRoot.toElement();
		if (eRoot.isNull())
			continue;

		// Check for output-socket spec lists...
		if (eRoot.tagName() == "output-sockets")
			load_socketlist(pPatchbay->osocketlist(), eRoot);
		else
		// Check for input-socket spec lists...
		if (eRoot.tagName() == "input-sockets")
			load_socketlist(pPatchbay->isocketlist(), eRoot);
		else
		// Check for slots spec list...
		if (eRoot.tagName() == "slots") {
			for (QDomNode nSlot = eRoot.firstChild();
					!nSlot.isNull();
						nSlot = nSlot.nextSibling()) {
				// Convert slot node to element...
				QDomElement eSlot = nSlot.toElement();
				if (eSlot.isNull())
					continue;
				if (eSlot.tagName() == "slot") {
					QString sSlotName = eSlot.attribute("name");
					QString sSlotMode = eSlot.attribute("mode");
					int iSlotMode = QJACKCTL_SLOTMODE_OPEN;
					if (sSlotMode == "half")
						iSlotMode = QJACKCTL_SLOTMODE_HALF;
					else if (sSlotMode == "full")
						iSlotMode = QJACKCTL_SLOTMODE_FULL;
					qjackctlPatchbaySlot *pSlot
						= new qjackctlPatchbaySlot(sSlotName, iSlotMode);
					pSlot->setOutputSocket(
						pPatchbay->findSocket(pPatchbay->osocketlist(),
						eSlot.attribute("output")));
					pSlot->setInputSocket(
						pPatchbay->findSocket(pPatchbay->isocketlist(),
						eSlot.attribute("input")));
					pPatchbay->addSlot(pSlot);
				}
			}
		}
		else
		// Check for cable spec list...
		if (eRoot.tagName() == "cables") {
			for (QDomNode nCable = eRoot.firstChild();
				!nCable.isNull();
					nCable = nCable.nextSibling()) {
				// Convert cable node to element...
				QDomElement eCable = nCable.toElement();
				if (eCable.isNull())
					continue;
				if (eCable.tagName() == "cable") {
					int iSocketType
						= qjackctlPatchbaySocket::typeFromText(
							eCable.attribute("type"));
					qjackctlPatchbaySocket *pOutputSocket
						= pPatchbay->findSocket(pPatchbay->osocketlist(),
							eCable.attribute("output"), iSocketType);
					qjackctlPatchbaySocket *pIntputSocket
						= pPatchbay->findSocket(pPatchbay->isocketlist(),
							eCable.attribute("input"), iSocketType);
					if (pOutputSocket && pIntputSocket) {
						pPatchbay->addCable(
							new qjackctlPatchbayCable(
								pOutputSocket,
								pIntputSocket));
					}
				}
			}
		}
	}

	return true;
}


// Specific patchbay save (write) method.
bool qjackctlPatchbayFile::save ( qjackctlPatchbayRack *pPatchbay,
	const QString& sFilename )
{
	QFileInfo fi(sFilename);

	QDomDocument doc("patchbay");
	QDomElement eRoot = doc.createElement("patchbay");
	eRoot.setAttribute("name", fi.baseName());
	eRoot.setAttribute("version", QJACKCTL_VERSION);
	doc.appendChild(eRoot);

	// Save output-sockets spec...
	QDomElement eOutputSockets = doc.createElement("output-sockets");
	save_socketlist(pPatchbay->osocketlist(), eOutputSockets, doc);
	eRoot.appendChild(eOutputSockets);

	// Save input-sockets spec...
	QDomElement eInputSockets = doc.createElement("input-sockets");
	save_socketlist(pPatchbay->isocketlist(), eInputSockets, doc);
	eRoot.appendChild(eInputSockets);

	// Save slots spec...
	QDomElement eSlots = doc.createElement("slots");
	QListIterator<qjackctlPatchbaySlot *> slotit(pPatchbay->slotlist());
	while (slotit.hasNext()) {
		qjackctlPatchbaySlot *pSlot = slotit.next();
		QDomElement eSlot = doc.createElement("slot");
		eSlot.setAttribute("name", pSlot->name());
		QString sSlotMode = "open";
		switch (pSlot->mode()) {
		case QJACKCTL_SLOTMODE_HALF:
			sSlotMode = "half";
			break;
		case QJACKCTL_SLOTMODE_FULL:
			sSlotMode = "full";
			break;
		}
		eSlot.setAttribute("mode", sSlotMode);
		if (pSlot->outputSocket())
			eSlot.setAttribute("output", pSlot->outputSocket()->name());
		if (pSlot->inputSocket())
			eSlot.setAttribute("input", pSlot->inputSocket()->name());
		// Add this slot...
		eSlots.appendChild(eSlot);
	}
	eRoot.appendChild(eSlots);

	// Save cables spec...
	QDomElement eCables = doc.createElement("cables");
	QListIterator<qjackctlPatchbayCable *> cablit(pPatchbay->cablelist());
	while (cablit.hasNext()) {
		qjackctlPatchbayCable *pCable = cablit.next();
		if (pCable->outputSocket() && pCable->inputSocket()) {
			QDomElement eCable = doc.createElement("cable");
			eCable.setAttribute("type",
				qjackctlPatchbaySocket::textFromType(
					pCable->outputSocket()->type()));
			eCable.setAttribute("output", pCable->outputSocket()->name());
			eCable.setAttribute("input", pCable->inputSocket()->name());
			eCables.appendChild(eCable);
		}
	}
	eRoot.appendChild(eCables);

	// Finally, we're ready to save to external file.
	QFile file(sFilename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;
	QTextStream ts(&file);
	ts << doc.toString() << endl;
	file.close();

	return true;
}


// qjackctlPatchbayFile.cpp
