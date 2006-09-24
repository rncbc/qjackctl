// qjackctlConnect.cpp
//
/****************************************************************************
   Copyright (C) 2003-2006, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include "qjackctlConnect.h"

#include <qpopupmenu.h>
#include <qmessagebox.h>
#include <qtimer.h>

//----------------------------------------------------------------------
// class qjackctlConnectToolTip -- custom list view tooltips.

// Constructor.
qjackctlConnectToolTip::qjackctlConnectToolTip ( qjackctlClientListView *pListView )
	: QToolTip(pListView->viewport())
{
	m_pListView = pListView;
}

// Tooltip handler.
void qjackctlConnectToolTip::maybeTip ( const QPoint& pos )
{
	QListViewItem *pItem = m_pListView->itemAt(pos);
	if (pItem == 0)
		return;

	QRect rect(m_pListView->itemRect(pItem));
	if (!rect.isValid())
		return;

	if (pItem->rtti() == QJACKCTL_CLIENTITEM) {
		qjackctlClientItem *pClient = (qjackctlClientItem *) pItem;
		QToolTip::tip(rect, pClient->clientName());
	} else {
		qjackctlPortItem *pPort = (qjackctlPortItem *) pItem;
		QToolTip::tip(rect, pPort->portName());
	}
}


//----------------------------------------------------------------------
// class qjackctlPortItem -- Port list item.
//

// Constructor.
qjackctlPortItem::qjackctlPortItem ( qjackctlClientItem *pClient, const QString& sPortName )
    : QListViewItem(pClient, sPortName)
{
    m_pClient   = pClient;
    m_sPortName = sPortName;
    m_iPortMark = 0;
    m_bHilite   = false;

    m_pClient->ports().append(this);

    QListViewItem::setDragEnabled(true);
    QListViewItem::setDropEnabled(true);

    m_connects.setAutoDelete(false);

	// Check aliasing...
	qjackctlConnectAlias *pAliases = ((pClient->clientList())->listView())->aliases();
	if (pAliases) {
		QListViewItem::setText(0, pAliases->portAlias(pClient->clientName(), sPortName));
		QListViewItem::setRenameEnabled(0, ((pClient->clientList())->listView())->renameEnabled());
	}
}

// Default destructor.
qjackctlPortItem::~qjackctlPortItem (void)
{
    m_pClient->ports().remove(this);
	for (qjackctlPortItem *pPort = m_connects.first(); pPort; pPort = m_connects.next())
		pPort->removeConnect(this);
    m_connects.clear();
}


// Instance accessors.
void qjackctlPortItem::setPortName ( const QString& sPortName )
{
    QListViewItem::setText(0, sPortName);

    m_sPortName = sPortName;
}

const QString& qjackctlPortItem::clientName (void)
{
    return m_pClient->clientName();
}

const QString& qjackctlPortItem::portName (void)
{
    return m_sPortName;
}


// Complete client:port name helper.
QString qjackctlPortItem::clientPortName (void)
{
    return m_pClient->clientName() + ":" + m_sPortName;
}


// Connect client item accessor.
qjackctlClientItem *qjackctlPortItem::client (void)
{
    return m_pClient;
}


// Client:port set housekeeping marker.
void qjackctlPortItem::markPort ( int iMark )
{
    setHilite(false);
    m_iPortMark = iMark;
    if (iMark > 0)
        m_connects.clear();
}

void qjackctlPortItem::markClientPort ( int iMark )
{
    markPort(iMark);

    m_pClient->markClient(iMark);
}

int qjackctlPortItem::portMark (void)
{
    return m_iPortMark;
}


// Connected port list primitives.
void qjackctlPortItem::addConnect( qjackctlPortItem *pPort )
{
    m_connects.append(pPort);
}

void qjackctlPortItem::removeConnect( qjackctlPortItem *pPort )
{
    pPort->setHilite(false);
    m_connects.remove(pPort);
}


// Connected port finder.
qjackctlPortItem *qjackctlPortItem::findConnect ( const QString& sClientPortName )
{
    for (qjackctlPortItem *pPort = m_connects.first(); pPort; pPort = m_connects.next()) {
        if (sClientPortName == pPort->clientPortName())
            return pPort;
    }
    return 0;
}

qjackctlPortItem *qjackctlPortItem::findConnectPtr ( qjackctlPortItem *pPortPtr )
{
    for (qjackctlPortItem *pPort = m_connects.first(); pPort; pPort = m_connects.next()) {
        if (pPortPtr == pPort)
            return pPort;
    }
    return 0;
}


// Connection cache list accessor.
QPtrList<qjackctlPortItem>& qjackctlPortItem::connects (void)
{
    return m_connects;
}


// To virtually distinguish between list view items.
int qjackctlPortItem::rtti (void) const
{
    return QJACKCTL_PORTITEM;
}


// Connectiopn highlight methods.
bool qjackctlPortItem::isHilite (void)
{
    return m_bHilite;
}

void qjackctlPortItem::setHilite ( bool bHilite )
{
    // Update the port highlightning if changed...
    if ((m_bHilite && !bHilite) || (!m_bHilite && bHilite)) {
        m_bHilite = bHilite;
        QListViewItem::repaint();
        // Propagate this to the parent...
        m_pClient->setHilite(bHilite);
    }
}


// To highlight current connected ports when complementary-selected.
void qjackctlPortItem::paintCell( QPainter *p, const QColorGroup& cg, int column, int width, int align )
{
    QColorGroup cgCell(cg);
    if (m_bHilite)
        cgCell.setColor(QColorGroup::Text, Qt::blue);
    QListViewItem::paintCell(p, cgCell, column, width, align);
}


// Special port name sorting virtual comparator.
int qjackctlPortItem::compare (QListViewItem* pPortItem, int iColumn, bool bAscending) const
{
	return qjackctlClientListView::compare(text(iColumn), pPortItem->text(iColumn), bAscending);
}


//----------------------------------------------------------------------
// class qjackctlClientItem -- Jack client list item.
//

// Constructor.
qjackctlClientItem::qjackctlClientItem ( qjackctlClientList *pClientList, const QString& sClientName )
    : QListViewItem(pClientList->listView(), sClientName)
{
    m_pClientList = pClientList;
    m_sClientName = sClientName;
    m_iClientMark = 0;
    m_iHilite     = 0;

    m_ports.setAutoDelete(false);

    m_pClientList->clients().append(this);

    QListViewItem::setDragEnabled(true);
    QListViewItem::setDropEnabled(true);

//  QListViewItem::setSelectable(false);

	// Check aliasing...
	qjackctlConnectAlias *pAliases = (pClientList->listView())->aliases();
	if (pAliases) {
		QListViewItem::setText(0, pAliases->clientAlias(sClientName));
		QListViewItem::setRenameEnabled(0, (pClientList->listView())->renameEnabled());
	}
}

// Default destructor.
qjackctlClientItem::~qjackctlClientItem (void)
{
    m_ports.clear();

    m_pClientList->clients().remove(this);
}


// Port finder.
qjackctlPortItem *qjackctlClientItem::findPort (const QString& sPortName)
{
    for (qjackctlPortItem *pPort = m_ports.first(); pPort; pPort = m_ports.next()) {
        if (sPortName == pPort->portName())
            return pPort;
    }
    return 0;
}


// Client list accessor.
qjackctlClientList *qjackctlClientItem::clientList (void)
{
    return m_pClientList;
}


// Port list accessor.
QPtrList<qjackctlPortItem>& qjackctlClientItem::ports (void)
{
    return m_ports;
}


// Instance accessors.
void qjackctlClientItem::setClientName ( const QString& sClientName )
{
    QListViewItem::setText(0, sClientName);

    m_sClientName = sClientName;
}

const QString& qjackctlClientItem::clientName (void)
{
    return m_sClientName;
}


// Readable flag client accessor.
bool qjackctlClientItem::isReadable (void)
{
    return m_pClientList->isReadable();
}


// Client:port set housekeeping marker.
void qjackctlClientItem::markClient ( int iMark )
{
    setHilite(false);
    m_iClientMark = iMark;
}

void qjackctlClientItem::markClientPorts ( int iMark )
{
    markClient(iMark);

    for (qjackctlPortItem *pPort = m_ports.first(); pPort; pPort = m_ports.next())
        pPort->markPort(iMark);
}

void qjackctlClientItem::cleanClientPorts ( int iMark )
{
    for (qjackctlPortItem *pPort = m_ports.last(); pPort; pPort = m_ports.prev()) {
        if (pPort->portMark() == iMark)
            delete pPort;
    }
}

int qjackctlClientItem::clientMark (void)
{
    return m_iClientMark;
}


// To virtually distinguish between list view items.
int qjackctlClientItem::rtti (void) const
{
    return QJACKCTL_CLIENTITEM;
}


// Connectiopn highlight methods.
bool qjackctlClientItem::isHilite (void)
{
    return (m_iHilite > 0);
}

void qjackctlClientItem::setHilite ( bool bHilite )
{
    int iHilite = m_iHilite;
    if (bHilite)
        m_iHilite++;
    else
    if (m_iHilite > 0)
        m_iHilite--;
    // Update the client highlightning if changed...
    if (iHilite == 0 || m_iHilite == 0)
        QListViewItem::repaint();
}


// To highlight current connected clients when complementary-selected.
void qjackctlClientItem::paintCell( QPainter *p, const QColorGroup& cg, int column, int width, int align )
{
    QColorGroup cgCell(cg);
    if (m_iHilite > 0)
        cgCell.setColor(QColorGroup::Text, Qt::darkBlue);
    QListViewItem::paintCell(p, cgCell, column, width, align);
}


// Special client name sorting virtual comparator.
int qjackctlClientItem::compare (QListViewItem* pClientItem, int iColumn, bool bAscending) const
{
	return qjackctlClientListView::compare(text(iColumn), pClientItem->text(iColumn), bAscending);
}


//----------------------------------------------------------------------
// qjackctlClientList -- Client list.
//

// Constructor.
qjackctlClientList::qjackctlClientList( qjackctlClientListView *pListView, bool bReadable )
{
    m_pListView = pListView;
    m_bReadable = bReadable;

    m_pHiliteItem = 0;

    m_clients.setAutoDelete(false);
}

// Default destructor.
qjackctlClientList::~qjackctlClientList (void)
{
    qjackctlClientItem *pClient;

    while ((pClient = m_clients.last()) != 0)
        delete pClient;

    m_clients.clear();
}


// Client finder.
qjackctlClientItem *qjackctlClientList::findClient ( const QString& sClientName )
{
    for (qjackctlClientItem *pClient = m_clients.first(); pClient; pClient = m_clients.next()) {
        if (sClientName == pClient->clientName())
            return pClient;
    }
    return 0;
}

// Client:port finder.
qjackctlPortItem *qjackctlClientList::findClientPort ( const QString& sClientPort )
{
    qjackctlPortItem *pPort = 0;
    int iColon = sClientPort.find(":");
    if (iColon >= 0) {
        qjackctlClientItem *pClient = findClient(sClientPort.left(iColon));
        if (pClient)
            pPort = pClient->findPort(sClientPort.right(sClientPort.length() - iColon - 1));
    }
    return pPort;
}


// Client list accessor.
QPtrList<qjackctlClientItem>& qjackctlClientList::clients (void)
{
    return m_clients;
}


// List view accessor.
qjackctlClientListView *qjackctlClientList::listView (void)
{
    return m_pListView;
}


// Readable flag client accessor.
bool qjackctlClientList::isReadable (void)
{
    return m_bReadable;
}


// Client:port set housekeeping marker.
void qjackctlClientList::markClientPorts ( int iMark )
{
    m_pHiliteItem = 0;

    for (qjackctlClientItem *pClient = m_clients.first(); pClient; pClient = m_clients.next())
        pClient->markClientPorts(iMark);
}

void qjackctlClientList::cleanClientPorts ( int iMark )
{
    for (qjackctlClientItem *pClient = m_clients.last(); pClient; pClient = m_clients.prev()) {
        if (pClient->clientMark() == iMark) {
            delete pClient;
        } else {
            pClient->cleanClientPorts(iMark);
        }
    }
}

// Client:port hilite update stabilization.
void qjackctlClientList::hiliteClientPorts (void)
{
    qjackctlClientItem *pClient;
    qjackctlPortItem *pPort, *p;

    QListViewItem *pItem = m_pListView->selectedItem();

    // Dehilite the previous selected items.
    if (m_pHiliteItem && pItem != m_pHiliteItem) {
        if (m_pHiliteItem->rtti() == QJACKCTL_CLIENTITEM) {
            pClient = (qjackctlClientItem *) m_pHiliteItem;
            for (pPort = pClient->ports().first(); pPort; pPort = pClient->ports().next()) {
                for (p = pPort->connects().first(); p; p = pPort->connects().next())
                    p->setHilite(false);
            }
        } else {
            pPort = (qjackctlPortItem *) m_pHiliteItem;
            for (p = pPort->connects().first(); p; p = pPort->connects().next())
                p->setHilite(false);
        }
    }

    // Hilite the now current selected items.
    if (pItem) {
        if (pItem->rtti() == QJACKCTL_CLIENTITEM) {
            pClient = (qjackctlClientItem *) pItem;
            for (pPort = pClient->ports().first(); pPort; pPort = pClient->ports().next()) {
                for (p = pPort->connects().first(); p; p = pPort->connects().next())
                    p->setHilite(true);
            }
        } else {
            pPort = (qjackctlPortItem *) pItem;
            for (p = pPort->connects().first(); p; p = pPort->connects().next())
                p->setHilite(true);
        }
    }

    // Do remember this one, ever.
    m_pHiliteItem = pItem;
}


//----------------------------------------------------------------------------
// qjackctlClientListView -- Client list view, supporting drag-n-drop.

// Constructor.
qjackctlClientListView::qjackctlClientListView ( qjackctlConnectView *pConnectView, bool bReadable )
    : QListView(pConnectView)
{
    m_pConnectView = pConnectView;

    m_pAutoOpenTimer   = 0;
    m_iAutoOpenTimeout = 0;
    m_pDragDropItem    = 0;

	m_pAliases = 0;
	m_bRenameEnabled = false;

	m_pToolTip = new qjackctlConnectToolTip(this);

    QListView::header()->setClickEnabled(false);
    QListView::header()->setResizeEnabled(false);
    QListView::setMinimumWidth(120);
    QListView::setAllColumnsShowFocus(true);
	QListView::setColumnWidthMode(0, QListView::Maximum);
    QListView::setRootIsDecorated(true);
    QListView::setResizeMode(QListView::AllColumns);
    QListView::setAcceptDrops(true);
    QListView::setDragAutoScroll(true);
	QListView::setSizePolicy(
		QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

	QListView::setShowToolTips(false);

    if (bReadable)
        QListView::addColumn(tr("Readable Clients") + " / " + tr("Output Ports"));
    else
        QListView::addColumn(tr("Writable Clients") + " / " + tr("Input Ports"));

    setAutoOpenTimeout(800);

    QObject::connect(this, SIGNAL(itemRenamed(QListViewItem*,int)),
		this, SLOT(renamedSlot(QListViewItem*,int)));
}

// Default destructor.
qjackctlClientListView::~qjackctlClientListView (void)
{
    setAutoOpenTimeout(0);

	delete m_pToolTip;
}


// Auto-open timeout method.
void qjackctlClientListView::setAutoOpenTimeout ( int iAutoOpenTimeout )
{
    m_iAutoOpenTimeout = iAutoOpenTimeout;

    if (m_pAutoOpenTimer)
        delete m_pAutoOpenTimer;
    m_pAutoOpenTimer = 0;

    if (m_iAutoOpenTimeout > 0) {
        m_pAutoOpenTimer = new QTimer(this);
        QObject::connect(m_pAutoOpenTimer, SIGNAL(timeout()), this, SLOT(timeoutSlot()));
    }
}


// Auto-open timeout accessor.
int qjackctlClientListView::autoOpenTimeout (void)
{
    return m_iAutoOpenTimeout;
}


// Aliasing support methods.
void qjackctlClientListView::setAliases ( qjackctlConnectAlias *pAliases, bool bRenameEnabled )
{
	m_pAliases = pAliases;
	m_bRenameEnabled = bRenameEnabled;

	// Update all current listview items, if any.
	QListViewItemIterator iter(this);
	while (iter.current()) {
		QListViewItem *pItem = iter.current();
		if (pItem->rtti() == QJACKCTL_CLIENTITEM) {
			qjackctlClientItem *pClient = (qjackctlClientItem *) pItem;
			if (m_pAliases) {
				pClient->setText(0, m_pAliases->clientAlias(pClient->clientName()));
				pClient->setRenameEnabled(0, m_bRenameEnabled);
			} else {
				pClient->setText(0, pClient->clientName());
				pClient->setRenameEnabled(0, false);
			}
		} else {
			qjackctlPortItem *pPort = (qjackctlPortItem *) pItem;
			if (m_pAliases) {
				pPort->setText(0, m_pAliases->portAlias(pPort->clientName(), pPort->portName()));
				pPort->setRenameEnabled(0, m_bRenameEnabled);
			} else {
				pPort->setText(0, pPort->portName());
				pPort->setRenameEnabled(0, false);
			}
		}
		++iter;
	}
}

qjackctlConnectAlias *qjackctlClientListView::aliases (void)
{
	return m_pAliases;
}

bool qjackctlClientListView::renameEnabled (void)
{
	return m_bRenameEnabled;
}

// In-place aliasing slot.
void qjackctlClientListView::startRenameSlot (void)
{
	QListViewItem *pItem = selectedItem();
	if (pItem)
	    pItem->startRename(0);
}


// In-place aliasing slot.
void qjackctlClientListView::renamedSlot ( QListViewItem *pItem, int )
{
	if (pItem && m_pAliases) {
		const QString& sText = pItem->text(0);
		if (pItem->rtti() == QJACKCTL_CLIENTITEM) {
			qjackctlClientItem *pClient = (qjackctlClientItem *) pItem;
			m_pAliases->setClientAlias(pClient->clientName(), sText);
			if (sText.isEmpty())
				pClient->setText(0, pClient->clientName());
		} else {
			qjackctlPortItem *pPort = (qjackctlPortItem *) pItem;
			m_pAliases->setPortAlias(pPort->clientName(), pPort->portName(), sText);
			if (sText.isEmpty())
				pPort->setText(0, pPort->portName());
		}
		m_pConnectView->setDirty(true);
	}
}


// Auto-open timer slot.
void qjackctlClientListView::timeoutSlot (void)
{
    if (m_pAutoOpenTimer) {
        m_pAutoOpenTimer->stop();
        if (m_pDragDropItem && !m_pDragDropItem->isOpen()) {
            m_pDragDropItem->setOpen(true);
            m_pDragDropItem->repaint();
        }
    }
}

// Drag-n-drop stuff.
QListViewItem *qjackctlClientListView::dragDropItem ( const QPoint& epos )
{
    QPoint vpos(epos);
    int m = QListView::header()->sectionRect(0).height();
    vpos.setY(vpos.y() - m);
    QListViewItem *pItem = QListView::itemAt(vpos);
    if (pItem) {
        if (m_pDragDropItem != pItem) {
            QListView::setSelected(pItem, true);
            m_pDragDropItem = pItem;
            if (m_pAutoOpenTimer)
                m_pAutoOpenTimer->start(m_iAutoOpenTimeout);
            qjackctlConnect *pConnect = m_pConnectView->binding();
            if (!pItem->dropEnabled() || pConnect == 0 || !pConnect->canConnectSelected())
                pItem = 0;
        }
    } else {
        m_pDragDropItem = 0;
        if (m_pAutoOpenTimer)
            m_pAutoOpenTimer->stop();
    }
    vpos = QListView::viewportToContents(vpos);
    QListView::ensureVisible(vpos.x(), vpos.y(), m, m);
    return pItem;
}

void qjackctlClientListView::dragEnterEvent ( QDragEnterEvent *pDragEnterEvent )
{
    if (pDragEnterEvent->source() != this &&
        QTextDrag::canDecode(pDragEnterEvent) &&
        dragDropItem(pDragEnterEvent->pos())) {
        pDragEnterEvent->accept();
    } else {
        pDragEnterEvent->ignore();
    }
}


void qjackctlClientListView::dragMoveEvent ( QDragMoveEvent *pDragMoveEvent )
{
    QListViewItem *pItem = 0;
    if (pDragMoveEvent->source() != this)
        pItem = dragDropItem(pDragMoveEvent->pos());
    if (pItem) {
        pDragMoveEvent->accept(QListView::itemRect(pItem));
    } else {
        pDragMoveEvent->ignore();
    }
}


void qjackctlClientListView::dragLeaveEvent ( QDragLeaveEvent * )
{
    m_pDragDropItem = 0;
    if (m_pAutoOpenTimer)
        m_pAutoOpenTimer->stop();
}


void qjackctlClientListView::dropEvent( QDropEvent *pDropEvent )
{
    if (pDropEvent->source() != this) {
        QString sText;
        if (QTextDrag::decode(pDropEvent, sText) && dragDropItem(pDropEvent->pos())) {
            qjackctlConnect *pConnect = m_pConnectView->binding();
            if (pConnect)
                pConnect->connectSelected();
        }
    }

    dragLeaveEvent(0);
}


QDragObject *qjackctlClientListView::dragObject (void)
{
    QTextDrag *pDragObject = 0;
    if (m_pConnectView->binding()) {
        QListViewItem *pItem = QListView::currentItem();
        if (pItem && pItem->dragEnabled()) {
            pDragObject = new QTextDrag(pItem->text(0), this);
            const QPixmap *pPixmap = pItem->pixmap(0);
            if (pPixmap)
                pDragObject->setPixmap(*pPixmap, QPoint(-4, -12));
        }
    }
    return pDragObject;
}


// Context menu request event handler.
void qjackctlClientListView::contextMenuEvent ( QContextMenuEvent *pContextMenuEvent )
{
    qjackctlConnect *pConnect = m_pConnectView->binding();
    if (pConnect == 0)
        return;

    int iItemID;
    QPopupMenu* pContextMenu = new QPopupMenu(this);

    iItemID = pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("connect1.png")),
		tr("&Connect"), pConnect, SLOT(connectSelected()), tr("Alt+C", "Connect"));
    pContextMenu->setItemEnabled(iItemID, pConnect->canConnectSelected());
    iItemID = pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("disconnect1.png")),
		tr("&Disconnect"), pConnect, SLOT(disconnectSelected()), tr("Alt+D", "Disconnect"));
    pContextMenu->setItemEnabled(iItemID, pConnect->canDisconnectSelected());
    iItemID = pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("disconnectall1.png")),
		tr("Disconnect &All"), pConnect, SLOT(disconnectAll()), tr("Alt+A", "Disconect All"));
    pContextMenu->setItemEnabled(iItemID, pConnect->canDisconnectAll());
	if (m_bRenameEnabled) {
		pContextMenu->insertSeparator();
		iItemID = pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("edit1.png")),
			tr("Re&name"), this, SLOT(startRenameSlot()), tr("Alt+N", "Rename"));
		pContextMenu->setItemEnabled(iItemID, selectedItem() && selectedItem()->renameEnabled(0));
	}
    pContextMenu->insertSeparator();
    iItemID = pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("refresh1.png")),
		tr("&Refresh"), pConnect, SLOT(refresh()), tr("Alt+R", "Refresh"));

    pContextMenu->exec(pContextMenuEvent->globalPos());

    delete pContextMenu;
}


// Natural decimal sorting comparator.
int qjackctlClientListView::compare (const QString& s1, const QString& s2, bool bAscending)
{
    int ich1, ich2;

    int cch1 = s1.length();
    int cch2 = s2.length();

    for (ich1 = ich2 = 0; ich1 < cch1 && ich2 < cch2; ich1++, ich2++) {

        // Skip (white)spaces...
        while (s1.at(ich1).isSpace())
            ich1++;
        while (s2.at(ich2).isSpace())
            ich2++;

		// Normalize (to uppercase) the next characters...
        QChar ch1 = s1.at(ich1).upper();
        QChar ch2 = s2.at(ich2).upper();

        if (ch1.isDigit() && ch2.isDigit()) {
            // Find the whole length numbers...
            int iDigits1 = ich1++;
            while (s1.at(ich1).isDigit())
                ich1++;
            int iDigits2 = ich2++;
            while (s2.at(ich2).isDigit())
                ich2++;
            // Compare as natural decimal-numbers...
            int iNumber1 = s1.mid(iDigits1, ich1 - iDigits1).toInt();
            int iNumber2 = s2.mid(iDigits2, ich2 - iDigits2).toInt();
            if (iNumber1 < iNumber2)
                return (bAscending ? -1 :  1);
            else if (iNumber1 > iNumber2)
                return (bAscending ?  1 : -1);
            // Go on with this next char...
            ch1 = s1.at(ich1).upper();
            ch2 = s2.at(ich2).upper();
        }

        // Compare this char...
        if (ch1 < ch2)
            return (bAscending ? -1 :  1);
        else if (ch1 > ch2)
            return (bAscending ?  1 : -1);
    }

    // Both strings seem to match, but longer is greater.
    if (cch1 < cch2)
        return (bAscending ? -1 :  1);
    else if (cch1 > cch2)
        return (bAscending ?  1 : -1);

    // Exact match.
    return 0;
}


//----------------------------------------------------------------------
// qjackctlConnectorView -- Jack port connector widget.
//

// Constructor.
qjackctlConnectorView::qjackctlConnectorView ( qjackctlConnectView *pConnectView )
    : QWidget(pConnectView)
{
    m_pConnectView = pConnectView;

    QWidget::setMinimumWidth(20);
//  QWidget::setMaximumWidth(120);
	QWidget::setSizePolicy(
		QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding));
}

// Default destructor.
qjackctlConnectorView::~qjackctlConnectorView (void)
{
}


// Legal client/port item position helper.
int qjackctlConnectorView::itemY ( QListViewItem *pItem ) const
{
	QListViewItem *pParent = pItem->parent();
	if (pParent == 0 || pParent->isOpen()) {
		return (pItem->listView())->itemPos(pItem)
			+ pItem->height() / 2 - (pItem->listView())->contentsY();
	} else {
		return (pParent->listView())->itemPos(pParent)
			+ pParent->height() / 2 - (pParent->listView())->contentsY();
	}
}


// Draw visible port connection relation lines
void qjackctlConnectorView::drawConnectionLine ( QPainter& p, int x1, int y1, int x2, int y2, int h1, int h2 )
{
    // Account for list view headers.
    y1 += h1;
    y2 += h2;

    // Invisible output ports don't get a connecting dot.
    if (y1 > h1)
        p.drawLine(x1, y1, x1 + 4, y1);

    // How do we'll draw it?
    if (m_pConnectView->isBezierLines()) {
        // Setup control points
        QPointArray spline(4);
        int cp = (int)((double)(x2 - x1 - 8) * 0.4);
        spline.putPoints(0, 4, x1 + 4, y1, x1 + 4 + cp, y1, x2 - 4 - cp, y2, x2 - 4, y2);
        // The connection line, it self.
        p.drawCubicBezier(spline);
    }
    else p.drawLine(x1 + 4, y1, x2 - 4, y2);

    // Invisible input ports don't get a connecting dot.
    if (y2 > h2)
        p.drawLine(x2 - 4, y2, x2, y2);
}


// Draw visible port connection relation arrows.
void qjackctlConnectorView::drawConnections (void)
{
    if (m_pConnectView->OClientList() == 0 || m_pConnectView->IClientList() == 0)
        return;

    QPainter p(this);
    int x1, y1, h1;
    int x2, y2, h2;
	int i, rgb[3] = { 0x33, 0x66, 0x99 };
    int w;

    // Initialize color changer.
	i = 0;
    // Almost constants.
    x1 = 0;
    x2 = width();
    h1 = ((m_pConnectView->OListView())->header())->sectionRect(0).height();
    h2 = ((m_pConnectView->IListView())->header())->sectionRect(0).height();
    // about some specialty line width...
    w = m_pConnectView->iconSize();
    if (w < 2)
        w = 0;
    // For each client item...
	QPtrList<qjackctlClientItem>& oclients
		= m_pConnectView->OClientList()->clients();
	for (qjackctlClientItem *pOClient = oclients.first();
			pOClient; pOClient = oclients.next()) {
		// Set new connector color.
		++i; p.setPen(QColor(rgb[i % 3], rgb[(i / 3) % 3], rgb[(i / 9) % 3]));
		// For each port item
		for (qjackctlPortItem *pOPort = pOClient->ports().first();
				pOPort; pOPort = pOClient->ports().next()) {
			// Get starting connector arrow coordinates.
			y1 = itemY(pOPort);
			// Get port connections...
			for (qjackctlPortItem *pIPort = pOPort->connects().first();
					pIPort; pIPort = pOPort->connects().next()) {
				// Obviously, there is a connection from pOPort to pIPort items:
				y2 = itemY(pIPort);
				drawConnectionLine(p, x1, y1, x2, y2, h1, h2);
			}
		}
	}
}


// Widget event handlers...

void qjackctlConnectorView::paintEvent ( QPaintEvent * )
{
    drawConnections();
}


void qjackctlConnectorView::resizeEvent ( QResizeEvent * )
{
    QWidget::repaint(true);
}


// Context menu request event handler.
void qjackctlConnectorView::contextMenuEvent ( QContextMenuEvent *pContextMenuEvent )
{
    qjackctlConnect *pConnect = m_pConnectView->binding();
    if (pConnect == 0)
        return;

    int iItemID;
    QPopupMenu* pContextMenu = new QPopupMenu(this);

    iItemID = pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("connect1.png")),
		tr("&Connect"), pConnect, SLOT(connectSelected()), tr("Alt+C", "Connect"));
    pContextMenu->setItemEnabled(iItemID, pConnect->canConnectSelected());
    iItemID = pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("disconnect1.png")),
		tr("&Disconnect"), pConnect, SLOT(disconnectSelected()), tr("Alt+D", "Disconnect"));
    pContextMenu->setItemEnabled(iItemID, pConnect->canDisconnectSelected());
    iItemID = pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("disconnectall1.png")),
		tr("Disconnect &All"), pConnect, SLOT(disconnectAll()), tr("Alt+A", "Disconect All"));
    pContextMenu->setItemEnabled(iItemID, pConnect->canDisconnectAll());

    pContextMenu->insertSeparator();
    iItemID = pContextMenu->insertItem(QIconSet(QPixmap::fromMimeSource("refresh1.png")),
		tr("&Refresh"), pConnect, SLOT(refresh()), tr("Alt+R", "Refresh"));

    pContextMenu->exec(pContextMenuEvent->globalPos());

    delete pContextMenu;
}


// Widget event slots...

void qjackctlConnectorView::listViewChanged ( QListViewItem * )
{
    QWidget::update();
}


void qjackctlConnectorView::contentsMoved ( int, int )
{
    QWidget::update();
}


//----------------------------------------------------------------------------
// qjackctlConnectView -- Integrated connections view widget.

// Constructor.
qjackctlConnectView::qjackctlConnectView ( QWidget *pParent, const char *pszName )
    : QSplitter(Qt::Horizontal, pParent, pszName)
{
    m_pOListView     = new qjackctlClientListView(this, true);
    m_pConnectorView = new qjackctlConnectorView(this);
    m_pIListView     = new qjackctlClientListView(this, false);

    m_pConnect = 0;

    m_bBezierLines = false;
    m_iIconSize    = 0;

    QObject::connect(m_pOListView, SIGNAL(expanded(QListViewItem *)),  m_pConnectorView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pOListView, SIGNAL(collapsed(QListViewItem *)), m_pConnectorView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pOListView, SIGNAL(contentsMoving(int, int)),   m_pConnectorView, SLOT(contentsMoved(int, int)));

    QObject::connect(m_pIListView, SIGNAL(expanded(QListViewItem *)),  m_pConnectorView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pIListView, SIGNAL(collapsed(QListViewItem *)), m_pConnectorView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pIListView, SIGNAL(contentsMoving(int, int)),   m_pConnectorView, SLOT(contentsMoved(int, int)));

    m_bDirty = false;

#if QT_VERSION >= 0x030200
    QSplitter::setChildrenCollapsible(false);
#endif	
}


// Default destructor.
qjackctlConnectView::~qjackctlConnectView (void)
{
}


// Connect binding methods.
void qjackctlConnectView::setBinding ( qjackctlConnect *pConnect )
{
    m_pConnect = pConnect;
}

qjackctlConnect *qjackctlConnectView::binding (void)
{
    return m_pConnect;
}


// Connect client list accessors.
qjackctlClientList *qjackctlConnectView::OClientList (void)
{
    if (m_pConnect)
        return m_pConnect->OClientList();
    else
        return 0;
}

qjackctlClientList *qjackctlConnectView::IClientList (void)
{
    if (m_pConnect)
        return m_pConnect->OClientList();
    else
        return 0;
}


// Connector line style accessors.
void qjackctlConnectView::setBezierLines ( bool bBezierLines )
{
    m_bBezierLines = bBezierLines;
}

bool qjackctlConnectView::isBezierLines (void)
{
    return m_bBezierLines;
}


// Common icon size methods.
void qjackctlConnectView::setIconSize ( int iIconSize )
{
    // Update only if changed.
    if (iIconSize == m_iIconSize)
        return;

    // Go for it...
    m_iIconSize = iIconSize;

    // Call binding descendant implementation,
    // and do a complete content reset...
    if (m_pConnect)
        m_pConnect->updateContents(true);
}

int qjackctlConnectView::iconSize (void)
{
    return m_iIconSize;
}


// Dirty flag methods.
void qjackctlConnectView::setDirty ( bool bDirty )
{
    m_bDirty = bDirty;
    if (bDirty)
        emit contentsChanged();
}

bool qjackctlConnectView::dirty (void)
{
    return m_bDirty;
}


//----------------------------------------------------------------------
// qjackctlConnect -- Output-to-Input client/ports connection object.
//

// Constructor.
qjackctlConnect::qjackctlConnect ( qjackctlConnectView *pConnectView )
{
    m_pConnectView = pConnectView;

    m_pOClientList = 0;
    m_pIClientList = 0;

    m_iMutex = 0;

    m_pConnectView->setBinding(this);
}

// Default destructor.
qjackctlConnect::~qjackctlConnect (void)
{
    // Force end of works here.
    m_iMutex++;

    m_pConnectView->setBinding(0);

    if (m_pOClientList)
        delete m_pOClientList;
    if (m_pIClientList)
        delete m_pIClientList;

    m_pOClientList = 0;
    m_pIClientList = 0;

    m_pConnectView->ConnectorView()->repaint(true);
}


// These must be accessed by the descendant constructor.
qjackctlConnectView *qjackctlConnect::connectView (void)
{
    return m_pConnectView;
}

void qjackctlConnect::setOClientList ( qjackctlClientList *pOClientList )
{
    m_pOClientList = pOClientList;
}

void qjackctlConnect::setIClientList ( qjackctlClientList *pIClientList )
{
    m_pIClientList = pIClientList;
}


// Connection primitive.
void qjackctlConnect::connectPortsEx ( qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
    if (pOPort->findConnectPtr(pIPort) == 0) {
        connectPorts(pOPort, pIPort);
        pOPort->addConnect(pIPort);
        pIPort->addConnect(pOPort);
    }
}

// Disconnection primitive.
void qjackctlConnect::disconnectPortsEx ( qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
    if (pOPort->findConnectPtr(pIPort) != 0) {
        disconnectPorts(pOPort, pIPort);
        pOPort->removeConnect(pIPort);
        pIPort->removeConnect(pOPort);
    }
}


// Test if selected ports are connectable.
bool qjackctlConnect::canConnectSelected (void)
{
    bool bResult = false;

    if (startMutex()) {
        bResult = canConnectSelectedEx();
        endMutex();
    }

    return bResult;
}

bool qjackctlConnect::canConnectSelectedEx (void)
{
    // Take this opportunity to highlight any current selections.
    m_pOClientList->hiliteClientPorts();
    m_pIClientList->hiliteClientPorts();

    // Now with our predicate work...
    QListViewItem *pOItem = (m_pOClientList->listView())->selectedItem();
    if (!pOItem)
        return false;

    QListViewItem *pIItem = (m_pIClientList->listView())->selectedItem();
    if (!pIItem)
        return false;

    if (pOItem->rtti() == QJACKCTL_CLIENTITEM) {
        qjackctlClientItem *pOClient = (qjackctlClientItem *) pOItem;
        if (pIItem->rtti() == QJACKCTL_CLIENTITEM) {
            // Each-to-each connections...
            qjackctlClientItem *pIClient = (qjackctlClientItem *) pIItem;
            qjackctlPortItem *pOPort = pOClient->ports().first();
            qjackctlPortItem *pIPort = pIClient->ports().first();
            while (pOPort && pIPort) {
                if (pOPort->findConnectPtr(pIPort) == 0)
                    return true;
                pOPort = pOClient->ports().next();
                pIPort = pIClient->ports().next();
            }
        } else {
            // Many(all)-to-one connection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            qjackctlPortItem *pOPort = pOClient->ports().first();
            while (pOPort) {
                if (pOPort->findConnectPtr(pIPort) == 0)
                    return true;
                pOPort = pOClient->ports().next();
            }
        }
    } else {
        qjackctlPortItem *pOPort = (qjackctlPortItem *) pOItem;
        if (pIItem->rtti() == QJACKCTL_CLIENTITEM) {
            // One-to-many(all) connection...
            qjackctlClientItem *pIClient = (qjackctlClientItem *) pIItem;
            qjackctlPortItem *pIPort = pIClient->ports().first();
            while (pIPort) {
                if (pOPort->findConnectPtr(pIPort) == 0)
                    return true;
                pIPort = pIClient->ports().next();
            }
        } else {
            // One-to-one connection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            return (pOPort->findConnectPtr(pIPort) == 0);
        }
    }

    return false;
}


// Connect current selected ports.
bool qjackctlConnect::connectSelected (void)
{
    bool bResult = false;

    if (startMutex()) {
        bResult = connectSelectedEx();
        endMutex();
    }
    m_pConnectView->ConnectorView()->update();

    if (bResult)
        emit connectChanged();

    return bResult;
}

bool qjackctlConnect::connectSelectedEx (void)
{
    QListViewItem *pOItem = (m_pOClientList->listView())->selectedItem();
    if (!pOItem)
        return false;

    QListViewItem *pIItem = (m_pIClientList->listView())->selectedItem();
    if (!pIItem)
        return false;

    if (pOItem->rtti() == QJACKCTL_CLIENTITEM) {
        qjackctlClientItem *pOClient = (qjackctlClientItem *) pOItem;
        if (pIItem->rtti() == QJACKCTL_CLIENTITEM) {
            // Each-to-each connections...
            qjackctlClientItem *pIClient = (qjackctlClientItem *) pIItem;
            qjackctlPortItem *pOPort = pOClient->ports().first();
            qjackctlPortItem *pIPort = pIClient->ports().first();
            while (pOPort && pIPort) {
                connectPortsEx(pOPort, pIPort);
                pOPort = pOClient->ports().next();
                pIPort = pIClient->ports().next();
            }
        } else {
            // Many(all)-to-one connection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            qjackctlPortItem *pOPort = pOClient->ports().first();
            while (pOPort) {
                connectPortsEx(pOPort, pIPort);
                pOPort = pOClient->ports().next();
            }
        }
    } else {
        qjackctlPortItem *pOPort = (qjackctlPortItem *) pOItem;
        if (pIItem->rtti() == QJACKCTL_CLIENTITEM) {
            // One-to-many(all) connection...
            qjackctlClientItem *pIClient = (qjackctlClientItem *) pIItem;
            qjackctlPortItem *pIPort = pIClient->ports().first();
            while (pIPort) {
                connectPortsEx(pOPort, pIPort);
                pIPort = pIClient->ports().next();
            }
        } else {
            // One-to-one connection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            connectPortsEx(pOPort, pIPort);
        }
    }

    return true;
}


// Test if selected ports are disconnectable.
bool qjackctlConnect::canDisconnectSelected (void)
{
    bool bResult = false;

    if (startMutex()) {
        bResult = canDisconnectSelectedEx();
        endMutex();
    }

    return bResult;
}

bool qjackctlConnect::canDisconnectSelectedEx (void)
{
    QListViewItem *pOItem = (m_pOClientList->listView())->selectedItem();
    if (!pOItem)
        return false;

    QListViewItem *pIItem = (m_pIClientList->listView())->selectedItem();
    if (!pIItem)
        return false;

    if (pOItem->rtti() == QJACKCTL_CLIENTITEM) {
        qjackctlClientItem *pOClient = (qjackctlClientItem *) pOItem;
        if (pIItem->rtti() == QJACKCTL_CLIENTITEM) {
            // Each-to-each connections...
            qjackctlClientItem *pIClient = (qjackctlClientItem *) pIItem;
            qjackctlPortItem *pOPort = pOClient->ports().first();
            qjackctlPortItem *pIPort = pIClient->ports().first();
            while (pOPort && pIPort) {
                if (pOPort->findConnectPtr(pIPort) != 0)
                    return true;
                pOPort = pOClient->ports().next();
                pIPort = pIClient->ports().next();
            }
        } else {
            // Many(all)-to-one connection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            qjackctlPortItem *pOPort = pOClient->ports().first();
            while (pOPort) {
                if (pOPort->findConnectPtr(pIPort) != 0)
                    return true;
                pOPort = pOClient->ports().next();
            }
        }
    } else {
        qjackctlPortItem *pOPort = (qjackctlPortItem *) pOItem;
        if (pIItem->rtti() == QJACKCTL_CLIENTITEM) {
            // One-to-many(all) connection...
            qjackctlClientItem *pIClient = (qjackctlClientItem *) pIItem;
            qjackctlPortItem *pIPort = pIClient->ports().first();
            while (pIPort) {
                if (pOPort->findConnectPtr(pIPort) != 0)
                    return true;
                pIPort = pIClient->ports().next();
            }
        } else {
            // One-to-one connection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            return (pOPort->findConnectPtr(pIPort) != 0);
        }
    }

    return false;
}


// Disconnect current selected ports.
bool qjackctlConnect::disconnectSelected (void)
{
    bool bResult = false;

    if (startMutex()) {
        bResult = disconnectSelectedEx();
        endMutex();
    }
    m_pConnectView->ConnectorView()->update();

    if (bResult)
        emit connectChanged();

    return bResult;
}

bool qjackctlConnect::disconnectSelectedEx (void)
{
    QListViewItem *pOItem = (m_pOClientList->listView())->selectedItem();
    if (!pOItem)
        return false;

    QListViewItem *pIItem = (m_pIClientList->listView())->selectedItem();
    if (!pIItem)
        return false;

    if (pOItem->rtti() == QJACKCTL_CLIENTITEM) {
        qjackctlClientItem *pOClient = (qjackctlClientItem *) pOItem;
        if (pIItem->rtti() == QJACKCTL_CLIENTITEM) {
            // Each-to-each dicconnection...
            qjackctlClientItem *pIClient = (qjackctlClientItem *) pIItem;
            qjackctlPortItem *pOPort = pOClient->ports().first();
            qjackctlPortItem *pIPort = pIClient->ports().first();
            while (pOPort && pIPort) {
                disconnectPortsEx(pOPort, pIPort);
                pOPort = pOClient->ports().next();
                pIPort = pIClient->ports().next();
            }
        } else {
            // Many(all)-to-one disconnection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            qjackctlPortItem *pOPort = pOClient->ports().first();
            while (pOPort) {
                disconnectPortsEx(pOPort, pIPort);
                pOPort = pOClient->ports().next();
            }
        }
    } else {
        qjackctlPortItem *pOPort = (qjackctlPortItem *) pOItem;
        if (pIItem->rtti() == QJACKCTL_CLIENTITEM) {
            // One-to-many(all) disconnection...
            qjackctlClientItem *pIClient = (qjackctlClientItem *) pIItem;
            qjackctlPortItem *pIPort = pIClient->ports().first();
            while (pIPort) {
                disconnectPortsEx(pOPort, pIPort);
                pIPort = pIClient->ports().next();
            }
        } else {
            // One-to-one disconnection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            disconnectPortsEx(pOPort, pIPort);
        }
    }

    return true;
}


// Test if any port is disconnectable.
bool qjackctlConnect::canDisconnectAll (void)
{
    bool bResult = false;

    if (startMutex()) {
        bResult = canDisconnectAllEx();
        endMutex();
    }

    return bResult;
}

bool qjackctlConnect::canDisconnectAllEx (void)
{
    qjackctlClientItem *pOClient = m_pOClientList->clients().first();
    while (pOClient) {
        qjackctlPortItem *pOPort = pOClient->ports().first();
        while (pOPort) {
            if (pOPort->connects().count() > 0)
                return true;
            pOPort = pOClient->ports().next();
        }
        pOClient = m_pOClientList->clients().next();
    }
    return false;
}


// Disconnect all ports.
bool qjackctlConnect::disconnectAll (void)
{
    if (QMessageBox::warning(m_pConnectView,
		tr("Warning") + " - " QJACKCTL_SUBTITLE1,
        tr("This will suspend sound processing") + "\n" +
        tr("from all client applications.") + "\n\n" +
        tr("Are you sure?"),
        tr("Yes"), tr("No")) > 0) {
        return false;
    }

    bool bResult = false;

    if (startMutex()) {
        bResult = disconnectAllEx();
        endMutex();
    }
    m_pConnectView->ConnectorView()->update();

    if (bResult)
        emit connectChanged();

    return bResult;
}

bool qjackctlConnect::disconnectAllEx (void)
{
    qjackctlClientItem *pOClient = m_pOClientList->clients().first();
    while (pOClient) {
        qjackctlPortItem *pOPort = pOClient->ports().first();
        while (pOPort) {
            qjackctlPortItem *pIPort;
            while ((pIPort = pOPort->connects().first()) != 0)
                disconnectPortsEx(pOPort, pIPort);
            pOPort = pOClient->ports().next();
        }
        pOClient = m_pOClientList->clients().next();
    }

    return true;
}


// Complete/incremental contents rebuilder; check dirty status if incremental.
void qjackctlConnect::updateContents ( bool bClear )
{
    int iDirtyCount = 0;

    if (startMutex()) {
    // Do we do a complete rebuild?
    if (bClear) {
        (m_pOClientList->listView())->clear();
        (m_pIClientList->listView())->clear();
        updateIconPixmaps();
    }
    // Add (newer) client:ports and respective connections...
    iDirtyCount += m_pOClientList->updateClientPorts();
    iDirtyCount += m_pIClientList->updateClientPorts();
    updateConnections();
        endMutex();
    }

    (m_pConnectView->ConnectorView())->update();

    if (!bClear && iDirtyCount > 0)
        emit connectChanged();
}


// Incremental contents rebuilder; check dirty status.
void qjackctlConnect::refresh (void)
{
    updateContents(false);
}


// Dunno. But this may avoid some conflicts.
bool qjackctlConnect::startMutex (void)
{
    bool bMutex = (m_iMutex == 0);
    if (bMutex)
        m_iMutex++;
    return bMutex;
}

void qjackctlConnect::endMutex (void)
{
    if (m_iMutex > 0)
        m_iMutex--;
}


// Connect client list accessors.
qjackctlClientList *qjackctlConnect::OClientList (void)
{
    return m_pOClientList;
}

qjackctlClientList *qjackctlConnect::IClientList (void)
{
    return m_pIClientList;
}


// Common pixmap factory-method.
QPixmap *qjackctlConnect::createIconPixmap ( const QString& sIconName )
{
    QString sName = sIconName;
    int     iSize = m_pConnectView->iconSize() * 32;

    if (iSize > 0)
        sName += QString("_%1x%2").arg(iSize).arg(iSize);

    return new QPixmap(QPixmap::fromMimeSource(sName + ".png"));
}


// end of qjackctlConnect.cpp
