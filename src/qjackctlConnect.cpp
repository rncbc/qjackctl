// qjackctlConnect.cpp
//
/****************************************************************************
   Copyright (C) 2003-2004, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*****************************************************************************/

#include "qjackctlConnect.h"

#include <qpopupmenu.h>
#include <qmessagebox.h>
#include <qtimer.h>


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

    m_pClient->ports().append(this);

    if (pClient->isReadable()) {
        QListViewItem::setDragEnabled(true);
        QListViewItem::setDropEnabled(false);
    } else {
        QListViewItem::setDragEnabled(false);
        QListViewItem::setDropEnabled(true);
    }

    m_connects.setAutoDelete(false);
}

// Default destructor.
qjackctlPortItem::~qjackctlPortItem (void)
{
    m_pClient->ports().remove(this);
    m_connects.clear();
}


// Instance accessors.
QString& qjackctlPortItem::clientName (void)
{
    return m_pClient->clientName();
}

QString& qjackctlPortItem::portName (void)
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

    m_ports.setAutoDelete(false);

    m_pClientList->clients().append(this);

    if (m_pClientList->isReadable()) {
        QListViewItem::setDragEnabled(true);
        QListViewItem::setDropEnabled(false);
    } else {
        QListViewItem::setDragEnabled(false);
        QListViewItem::setDropEnabled(true);
    }

//  QListViewItem::setSelectable(false);
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
qjackctlClientList *qjackctlClientItem::clientlist (void)
{
    return m_pClientList;
}


// Port list accessor.
QPtrList<qjackctlPortItem>& qjackctlClientItem::ports (void)
{
    return m_ports;
}


// Instance accessors.
QString& qjackctlClientItem::clientName (void)
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


//----------------------------------------------------------------------
// qjackctlClientList -- Client list.
//

// Constructor.
qjackctlClientList::qjackctlClientList( qjackctlClientListView *pListView, bool bReadable )
{
    m_pListView = pListView;
    m_bReadable = bReadable;

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


//----------------------------------------------------------------------------
// qjackctlClientListView -- Client list view, supporting drag-n-drop.

// Constructor.
qjackctlClientListView::qjackctlClientListView ( QWidget *pParent, qjackctlConnectView *pConnectView )
    : QListView(pParent)
{
    m_pConnectView = pConnectView;
    
    m_pAutoOpenTimer   = 0;
    m_iAutoOpenTimeout = 0;
    m_pDragDropItem    = 0;
}

// Default destructor.
qjackctlClientListView::~qjackctlClientListView (void)
{
    setAutoOpenTimeout(0);
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
    if (QTextDrag::canDecode(pDragEnterEvent) && dragDropItem(pDragEnterEvent->pos())) {
        pDragEnterEvent->accept();
    } else {
        pDragEnterEvent->ignore();
    }
}


void qjackctlClientListView::dragMoveEvent ( QDragMoveEvent *pDragMoveEvent )
{
    QListViewItem *pItem = dragDropItem(pDragMoveEvent->pos());
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
    QString sText;
    if (QTextDrag::decode(pDropEvent, sText) && dragDropItem(pDropEvent->pos())) {
        m_pDragDropItem = 0;
        if (m_pAutoOpenTimer)
            m_pAutoOpenTimer->stop();
        qjackctlConnect *pConnect = m_pConnectView->binding();
        if (pConnect)
            pConnect->connectSelected();
    }
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
    m_pConnectView->contextMenu(pContextMenuEvent->globalPos());
}


//----------------------------------------------------------------------
// qjackctlConnectorView -- Jack port connector widget.
//

// Constructor.
qjackctlConnectorView::qjackctlConnectorView ( QWidget *pParent, qjackctlConnectView *pConnectView)
    : QWidget(pParent)
{
    m_pConnectView = pConnectView;
}

// Default destructor.
qjackctlConnectorView::~qjackctlConnectorView (void)
{
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

    // The connection line, it self.
    p.drawLine(x1 + 4, y1, x2 - 4, y2);

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
    QRect r1, r2;
    int   x1, y1, h1;
    int   x2, y2, h2;
    int   i, c, rgb[3];

    // Initialize color changer.
    i = c = rgb[0] = rgb[1] = rgb[2] = 0;
    // Almost constants.
    x1 = 0;
    x2 = width();
    h1 = ((m_pConnectView->OListView())->header())->sectionRect(0).height();
    h2 = ((m_pConnectView->IListView())->header())->sectionRect(0).height();
    // For each client item...
    for (qjackctlClientItem *pOClient = m_pConnectView->OClientList()->clients().first();
            pOClient;
                pOClient = m_pConnectView->OClientList()->clients().next()) {
        // Set new connector color.
        c += 0x39;
        c &= 0xff;
        rgb[i++] = c;
        p.setPen(QColor(rgb[2], rgb[1], rgb[0]));
        if (i > 2)
            i = 0;
        // For each port item
        for (qjackctlPortItem *pOPort = pOClient->ports().first();
                pOPort;
                    pOPort = pOClient->ports().next()) {
            // Get starting connector arrow coordinates.
            r1 = (m_pConnectView->OListView())->itemRect(pOPort);
            y1 = (r1.top() + r1.bottom()) / 2;
            // If this item is not visible, don't bother to update it.
            if (!pOPort->isVisible() || y1 < 1) {
                r1 = (m_pConnectView->OListView())->itemRect(pOClient);
                y1 = (r1.top() + r1.bottom()) / 2;
            }
            // Get port connections...
            for (qjackctlPortItem *pIPort = pOPort->connects().first();
                    pIPort;
                        pIPort = pOPort->connects().next()) {
                // Obviously, there is a connection from pOPort to pIPort items:
                r2 = (m_pConnectView->IListView())->itemRect(pIPort);
                y2 = (r2.top() + r2.bottom()) / 2;
                if (!pIPort->isVisible() || y2 < 1) {
                    r2 = (m_pConnectView->IListView())->itemRect(pIPort->client());
                    y2 = (r2.top() + r2.bottom()) / 2;
                }
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
    m_pConnectView->contextMenu(pContextMenuEvent->globalPos());
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
    : QWidget(pParent, pszName)
{
    QSizePolicy sizepolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    m_pGridLayout = new QGridLayout(pParent->layout(), 1, 1, 0, 0);

    m_pOListView = new qjackctlClientListView(pParent, this);
    m_pOListView->addColumn(tr("Readable Clients") + " / " + tr("Output Ports"));
    m_pOListView->header()->setClickEnabled(false);
    m_pOListView->header()->setResizeEnabled(false);
    m_pOListView->setMinimumSize(QSize(152, 60));
    m_pOListView->setAllColumnsShowFocus(true);
    m_pOListView->setRootIsDecorated(true);
    m_pOListView->setResizeMode(QListView::AllColumns);
    m_pOListView->setSizePolicy(sizepolicy);

    m_pConnectorView = new qjackctlConnectorView(pParent, this);
    m_pConnectorView->setMinimumSize(QSize(22, 60));
    m_pConnectorView->setMaximumWidth(120);
    m_pConnectorView->setSizePolicy(sizepolicy);

    m_pIListView = new qjackctlClientListView(pParent, this);
    m_pIListView->addColumn(tr("Writable Clients") + " / " + tr("Input Ports"));
    m_pIListView->header()->setClickEnabled(false);
    m_pIListView->header()->setResizeEnabled(false);
    m_pIListView->setMinimumSize(QSize(152, 60));
    m_pIListView->setAllColumnsShowFocus(true);
    m_pIListView->setRootIsDecorated(true);
    m_pIListView->setResizeMode(QListView::AllColumns);
    m_pIListView->setAcceptDrops(true);
    m_pIListView->setAutoOpenTimeout(1000);
    m_pIListView->setDragAutoScroll(true);
    m_pOListView->setSizePolicy(sizepolicy);

    m_pGridLayout->addWidget(m_pOListView,     0, 0);
    m_pGridLayout->addWidget(m_pConnectorView, 0, 1);
    m_pGridLayout->addWidget(m_pIListView,     0, 2);

    m_pConnect = 0;

    QObject::connect(m_pOListView, SIGNAL(expanded(QListViewItem *)),  m_pConnectorView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pOListView, SIGNAL(collapsed(QListViewItem *)), m_pConnectorView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pOListView, SIGNAL(contentsMoving(int, int)),   m_pConnectorView, SLOT(contentsMoved(int, int)));

    QObject::connect(m_pIListView, SIGNAL(expanded(QListViewItem *)),  m_pConnectorView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pIListView, SIGNAL(collapsed(QListViewItem *)), m_pConnectorView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pIListView, SIGNAL(contentsMoving(int, int)),   m_pConnectorView, SLOT(contentsMoved(int, int)));
}


// Default destructor.
qjackctlConnectView::~qjackctlConnectView (void)
{
}


// Common context menu slot.
void qjackctlConnectView::contextMenu ( const QPoint& pos )
{
    qjackctlConnect *pConnect = binding();
    if (pConnect == 0)
        return;

    int iItemID;
    QPopupMenu* pContextMenu = new QPopupMenu(this);

    iItemID = pContextMenu->insertItem(tr("&Connect"), pConnect, SLOT(connectSelected()), tr("Alt+C", "Connect"));
    pContextMenu->setItemEnabled(iItemID, pConnect->canConnectSelected());
    iItemID = pContextMenu->insertItem(tr("&Disconnect"), pConnect, SLOT(disconnectSelected()), tr("Alt+D", "Disconnect"));
    pContextMenu->setItemEnabled(iItemID, pConnect->canDisconnectSelected());
    iItemID = pContextMenu->insertItem(tr("Disconnect &All"), pConnect, SLOT(disconnectAll()), tr("Alt+A", "Disconect All"));
    pContextMenu->setItemEnabled(iItemID, pConnect->canDisconnectAll());

    pContextMenu->insertSeparator();
    iItemID = pContextMenu->insertItem(tr("&Refresh"), pConnect, SLOT(refresh()), tr("Alt+R", "Refresh"));

    pContextMenu->exec(pos);
    
    delete pContextMenu;
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


//----------------------------------------------------------------------
// qjackctlConnect -- Output-to-Input client/ports connection object.
//

// Constructor.
qjackctlConnect::qjackctlConnect ( qjackctlConnectView *pConnectView )
{
    m_pConnectView = pConnectView;
    
    m_pOClientList = 0;
    m_pIClientList = 0;

    m_iExclusive = 0;

    m_pConnectView->setBinding(this);
}

// Default destructor.
qjackctlConnect::~qjackctlConnect (void)
{
    // Force end of works here.
    m_iExclusive++;

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

    if (startExclusive()) {
        bResult = canConnectSelectedEx();
        endExclusive();
    }

    return bResult;
}

bool qjackctlConnect::canConnectSelectedEx (void)
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
    
    if (startExclusive()) {
        bResult = connectSelectedEx();
        endExclusive();
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

    if (startExclusive()) {
        bResult = canDisconnectSelectedEx();
        endExclusive();
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
    
    if (startExclusive()) {
        bResult = disconnectSelectedEx();
        endExclusive();
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
    
    if (startExclusive()) {
        bResult = canDisconnectAllEx();
        endExclusive();
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
    if (QMessageBox::warning(m_pConnectView, tr("Warning"),
        tr("This will suspend sound processing") + "\n" +
        tr("from all client applications.") + "\n\n" +
        tr("Are you sure?"),
        tr("Yes"), tr("No")) > 0) {
        return false;
    }

    bool bResult = false;

    if (startExclusive()) {
        bResult = disconnectAllEx();
        endExclusive();
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


// Complete contents rebuilder; return dirty status.
void qjackctlConnect::refresh (void)
{
    int iDirtyCount = 0;

    if (startExclusive()) {
        iDirtyCount += m_pOClientList->updateClientPorts();
        iDirtyCount += m_pIClientList->updateClientPorts();
        updateConnections();
        endExclusive();
    }

    (m_pConnectView->ConnectorView())->update();

    if (iDirtyCount > 0)
        emit connectChanged();
}


// Dunno. But this may avoid some conflicts.
bool qjackctlConnect::startExclusive (void)
{
    bool bExclusive = (m_iExclusive == 0);
    if (bExclusive)
        m_iExclusive++;
    return bExclusive;
}

void qjackctlConnect::endExclusive (void)
{
    if (m_iExclusive > 0)
        m_iExclusive--;
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

// end of qjackctlConnect.cpp

