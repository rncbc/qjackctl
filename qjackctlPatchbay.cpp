// qjackctlPatchbay.cpp
//
/****************************************************************************
   Copyright (C) 2003, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qjackctlPatchbay.h"

#include <qpopupmenu.h>
#include <qtimer.h>

#include <stdlib.h>
#include <math.h>

// Local pixmaps.
#include "clienti.xpm"
#include "cliento.xpm"
#include "portpti.xpm"
#include "portpto.xpm"
#include "portpni.xpm"
#include "portpno.xpm"
#include "portlti.xpm"
#include "portlto.xpm"
#include "portlni.xpm"
#include "portlno.xpm"

static int g_iXpmRefCount = 0;

static QPixmap *g_pXpmClientI = 0;  // Input client item pixmap.
static QPixmap *g_pXpmClientO = 0;  // Output client item pixmap.
static QPixmap *g_pXpmPortPTI = 0;  // Physcal Terminal Input port pixmap.
static QPixmap *g_pXpmPortPTO = 0;  // Physical Terminal input port pixmap.
static QPixmap *g_pXpmPortPNI = 0;  // Physical Non-terminal Input port pixmap.
static QPixmap *g_pXpmPortPNO = 0;  // Physical Non-terminal input port pixmap.
static QPixmap *g_pXpmPortLTI = 0;  // Logical Terminal Input port pixmap.
static QPixmap *g_pXpmPortLTO = 0;  // Logical Terminal input port pixmap.
static QPixmap *g_pXpmPortLNI = 0;  // Logical Non-terminal Input port pixmap.
static QPixmap *g_pXpmPortLNO = 0;  // Logical Non-terminal input port pixmap.


//----------------------------------------------------------------------
// class qjackctlPortItem -- Jack port list item.
//

// Constructor.
qjackctlPortItem::qjackctlPortItem ( qjackctlClientItem *pClient, QString sPortName, jack_port_t *pJackPort )
    : QListViewItem(pClient, sPortName)
{
    m_pClient   = pClient;
    m_sPortName = sPortName;
    m_iPortMark = 0;
    m_pJackPort = pJackPort;

    m_pClient->ports().append(this);

    unsigned long ulPortFlags = jack_port_flags(m_pJackPort);
    if (ulPortFlags & JackPortIsInput) {
        if (ulPortFlags & JackPortIsTerminal) {
            QListViewItem::setPixmap(0, (ulPortFlags & JackPortIsPhysical ? *g_pXpmPortPTI : *g_pXpmPortLTI));
        } else {
            QListViewItem::setPixmap(0, (ulPortFlags & JackPortIsPhysical ? *g_pXpmPortPNI : *g_pXpmPortLNI));
        }
        QListViewItem::setDragEnabled(false);
        QListViewItem::setDropEnabled(true);
    } else if (ulPortFlags & JackPortIsOutput) {
        if (ulPortFlags & JackPortIsTerminal) {
            QListViewItem::setPixmap(0, (ulPortFlags & JackPortIsPhysical ? *g_pXpmPortPTO : *g_pXpmPortLTO));
        } else {
            QListViewItem::setPixmap(0, (ulPortFlags & JackPortIsPhysical ? *g_pXpmPortPNO : *g_pXpmPortLNO));
        }
        QListViewItem::setDragEnabled(true);
        QListViewItem::setDropEnabled(false);
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


// Jack handles accessors.
jack_client_t *qjackctlPortItem::jackClient (void)
{
    return m_pClient->jackClient();
}

jack_port_t *qjackctlPortItem::jackPort (void)
{
    return m_pJackPort;
}

// Patchbay client item accessor.
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


// Special port name sorting virtual comparator.
int qjackctlPortItem::compare (QListViewItem* pPortItem, int iColumn, bool bAscending) const
{
    QString sName1, sName2;
    QString sPrefix1, sPrefix2;
    int iSuffix1, iSuffix2;
    int i, iNumber, iDigits;
    int iDecade, iFactor;

    sName1 = text(iColumn);
    sName2 = pPortItem->text(iColumn);

    iNumber  = 0;
    iDigits  = 0;
    iDecade  = 1;
    iFactor  = 1;
    iSuffix1 = 0;
    for (i = sName1.length() - 1; i >= 0; i--) {
        QCharRef ch = sName1.at(i);
        if (ch.isDigit()) {
            iNumber += ch.digitValue() * iDecade;
            iDecade *= 10;
            iDigits++;
        } else {
            sPrefix1.insert(0, ch.lower());
            if (iDigits > 0) {
                iSuffix1 += iNumber * iFactor;
                iFactor *= 100;
                iNumber  = 0;
                iDigits  = 0;
                iDecade  = 1;
            }
        }
    }

    iNumber  = 0;
    iDigits  = 0;
    iDecade  = 1;
    iFactor  = 1;
    iSuffix2 = 0;
    for (i = sName2.length() - 1; i >= 0; i--) {
        QCharRef ch = sName2.at(i);
        if (ch.isDigit()) {
            iNumber += ch.digitValue() * iDecade;
            iDecade *= 10;
            iDigits++;
        } else {
            sPrefix2.insert(0, ch.lower());
            if (iDigits > 0) {
                iSuffix2 += iNumber * iFactor;
                iFactor *= 100;
                iNumber  = 0;
                iDigits  = 0;
                iDecade  = 1;
            }
        }
    }

    if (sPrefix1 == sPrefix2) {
        if (iSuffix1 < iSuffix2)
            return (bAscending ? -1 :  1);
        else
        if (iSuffix1 > iSuffix2)
            return (bAscending ?  1 : -1);
        else
            return 0;
    } else {
        if (sPrefix1 < sPrefix2)
            return (bAscending ? -1 :  1);
        else
            return (bAscending ?  1 : -1);
    }
}


//----------------------------------------------------------------------
// class qjackctlClientItem -- Jack client list item.
//

// Constructor.
qjackctlClientItem::qjackctlClientItem ( qjackctlClientList *pClientList, QString sClientName )
    : QListViewItem(pClientList->listView(), sClientName)
{
    m_pClientList = pClientList;
    m_sClientName = sClientName;
    m_iClientMark = 0;

    m_ports.setAutoDelete(false);

    m_pClientList->clients().append(this);

    if (m_pClientList->jackFlags() & JackPortIsInput) {
        QListViewItem::setPixmap(0, *g_pXpmClientI);
        QListViewItem::setDragEnabled(false);
        QListViewItem::setDropEnabled(true);
    } else if (m_pClientList->jackFlags() & JackPortIsOutput) {
        QListViewItem::setPixmap(0, *g_pXpmClientO);
        QListViewItem::setDragEnabled(true);
        QListViewItem::setDropEnabled(false);
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
qjackctlPortItem *qjackctlClientItem::findPort (QString sPortName)
{
    for (qjackctlPortItem *pPort = m_ports.first(); pPort; pPort = m_ports.next()) {
        if (sPortName == pPort->portName())
            return pPort;
    }
    return 0;
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


// Jack client accessor.
jack_client_t *qjackctlClientItem::jackClient (void)
{
    return m_pClientList->jackClient();
}

unsigned long qjackctlClientItem::jackFlags (void)
{
    return m_pClientList->jackFlags();
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
// qjackctlClientList -- Jack client list.
//

// Constructor.
qjackctlClientList::qjackctlClientList( qjackctlClientListView *pListView, jack_client_t *pJackClient, unsigned long ulJackFlags )
{
    if (g_iXpmRefCount == 0) {
        g_pXpmClientI = new QPixmap((const char **) clienti_xpm);
        g_pXpmClientO = new QPixmap((const char **) cliento_xpm);
        g_pXpmPortPTI = new QPixmap((const char **) portpti_xpm);
        g_pXpmPortPTO = new QPixmap((const char **) portpto_xpm);
        g_pXpmPortPNI = new QPixmap((const char **) portpni_xpm);
        g_pXpmPortPNO = new QPixmap((const char **) portpno_xpm);
        g_pXpmPortLTI = new QPixmap((const char **) portlti_xpm);
        g_pXpmPortLTO = new QPixmap((const char **) portlto_xpm);
        g_pXpmPortLNI = new QPixmap((const char **) portlni_xpm);
        g_pXpmPortLNO = new QPixmap((const char **) portlno_xpm);
    }
    g_iXpmRefCount++;

    m_pListView   = pListView;
    m_pJackClient = pJackClient;
    m_ulJackFlags = ulJackFlags;

    m_clients.setAutoDelete(false);
}

// Default destructor.
qjackctlClientList::~qjackctlClientList (void)
{
    qjackctlClientItem *pClient;

    while ((pClient = m_clients.last()) != 0)
        delete pClient;

    m_clients.clear();

    if (--g_iXpmRefCount == 0) {
        delete g_pXpmClientI;
        delete g_pXpmClientO;
        delete g_pXpmPortPTI;
        delete g_pXpmPortPTO;
        delete g_pXpmPortPNI;
        delete g_pXpmPortPNO;
        delete g_pXpmPortLTI;
        delete g_pXpmPortLTO;
        delete g_pXpmPortLNI;
        delete g_pXpmPortLNO;
    }
}


// Client finder.
qjackctlClientItem *qjackctlClientList::findClient ( QString sClientName )
{
    for (qjackctlClientItem *pClient = m_clients.first(); pClient; pClient = m_clients.next()) {
        if (sClientName == pClient->clientName())
            return pClient;
    }
    return 0;
}

// Client:port finder.
qjackctlPortItem *qjackctlClientList::findClientPort ( QString sClientPort )
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


// Jack client accessor.
jack_client_t *qjackctlClientList::jackClient (void)
{
    return m_pJackClient;
}

unsigned long qjackctlClientList::jackFlags (void)
{
    return m_ulJackFlags;
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

// Client:port refreshner.
void qjackctlClientList::updateClientPorts (void)
{
    if (m_pJackClient == 0)
        return;

    markClientPorts(0);

    const char **ppszClientPorts = jack_get_ports(m_pJackClient, 0, 0, m_ulJackFlags);
    if (ppszClientPorts) {
        int iClientPort = 0;
        while (ppszClientPorts[iClientPort]) {
            jack_port_t *pJackPort = jack_port_by_name(m_pJackClient, ppszClientPorts[iClientPort]);
            QString sClientPort = ppszClientPorts[iClientPort];
            qjackctlClientItem *pClient = 0;
            qjackctlPortItem   *pPort   = 0;
            int iColon = sClientPort.find(":");
            if (iColon >= 0) {
                QString sClientName = sClientPort.left(iColon);
                QString sPortName   = sClientPort.right(sClientPort.length() - iColon - 1);
                pClient = findClient(sClientName);
                if (pClient)
                    pPort = pClient->findPort(sPortName);
                if (pClient == 0)
                    pClient = new qjackctlClientItem(this, sClientName);
                if (pClient && pPort == 0)
                    pPort = new qjackctlPortItem(pClient, sPortName, pJackPort);
                if (pPort)
                    pPort->markClientPort(1);
            }
            iClientPort++;
        }
        ::free(ppszClientPorts);
    }

    cleanClientPorts(0);
}


//----------------------------------------------------------------------------
// qjackctlClientListView -- Client list view, supporting drag-n-drop.

// Constructor.
qjackctlClientListView::qjackctlClientListView ( QWidget *pParent, qjackctlPatchbayView *pPatchbayView )
    : QListView(pParent)
{
    m_pPatchbayView = pPatchbayView;
    
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
            qjackctlPatchbay *pPatchbay = m_pPatchbayView->patchbay();
            if (!pItem->dropEnabled() || pPatchbay == 0 || !pPatchbay->canConnectSelected())
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
        qjackctlPatchbay *pPatchbay = m_pPatchbayView->patchbay();
        if (pPatchbay)
            pPatchbay->connectSelected();
    }
}


QDragObject *qjackctlClientListView::dragObject (void)
{
    QTextDrag *pDragObject = 0;
    if (m_pPatchbayView->patchbay()) {
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
    m_pPatchbayView->contextMenu(pContextMenuEvent->globalPos());
}



//----------------------------------------------------------------------
// qjackctlConnectorView -- Jack port connector widget.
//

// Constructor.
qjackctlConnectorView::qjackctlConnectorView ( QWidget *pParent, qjackctlPatchbayView *pPatchbayView)
    : QWidget(pParent)
{
    m_pPatchbayView = pPatchbayView;
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
    if (m_pPatchbayView->OClientList() == 0 || m_pPatchbayView->IClientList() == 0)
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
    h1 = ((m_pPatchbayView->OListView())->header())->sectionRect(0).height();
    h2 = ((m_pPatchbayView->IListView())->header())->sectionRect(0).height();
    // For each client item...
    for (qjackctlClientItem *pOClient = m_pPatchbayView->OClientList()->clients().first();
            pOClient;
                pOClient = m_pPatchbayView->OClientList()->clients().next()) {
        // Set new connector color.
        c += 0x33;
        rgb[i++] = (c & 0xff);
        p.setPen(QColor(rgb[2], rgb[1], rgb[0]));
        if (i > 2)
            i = 0;
        // For each port item
        for (qjackctlPortItem *pOPort = pOClient->ports().first();
                pOPort;
                    pOPort = pOClient->ports().next()) {
            // Get starting connector arrow coordinates.
            r1 = (m_pPatchbayView->OListView())->itemRect(pOPort);
            y1 = (r1.top() + r1.bottom()) / 2;
            // If this item is not visible, don't bother to update it.
            if (!pOPort->isVisible() || y1 < 1) {
                r1 = (m_pPatchbayView->OListView())->itemRect(pOClient);
                y1 = (r1.top() + r1.bottom()) / 2;
            }
            // Get port connections...
            for (qjackctlPortItem *pIPort = pOPort->connects().first();
                    pIPort;
                        pIPort = pOPort->connects().next()) {
                // Obviously, there is a connection from pOPort to pIPort items:
                r2 = (m_pPatchbayView->IListView())->itemRect(pIPort);
                y2 = (r2.top() + r2.bottom()) / 2;
                if (!pIPort->isVisible() || y2 < 1) {
                    r2 = (m_pPatchbayView->IListView())->itemRect(pIPort->client());
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
    m_pPatchbayView->contextMenu(pContextMenuEvent->globalPos());
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
// qjackctlPatchbayView -- Integrated patchbay widget.

// Constructor.
qjackctlPatchbayView::qjackctlPatchbayView ( QWidget *pParent, const char *pszName )
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

    m_pPatchbay = 0;

    QObject::connect(m_pOListView, SIGNAL(expanded(QListViewItem *)),  m_pConnectorView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pOListView, SIGNAL(collapsed(QListViewItem *)), m_pConnectorView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pOListView, SIGNAL(contentsMoving(int, int)),   m_pConnectorView, SLOT(contentsMoved(int, int)));

    QObject::connect(m_pIListView, SIGNAL(expanded(QListViewItem *)),  m_pConnectorView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pIListView, SIGNAL(collapsed(QListViewItem *)), m_pConnectorView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pIListView, SIGNAL(contentsMoving(int, int)),   m_pConnectorView, SLOT(contentsMoved(int, int)));
}


// Default destructor.
qjackctlPatchbayView::~qjackctlPatchbayView (void)
{
}


// Common context menu slot.
void qjackctlPatchbayView::contextMenu ( const QPoint& pos )
{
    qjackctlPatchbay *pPatchbay = patchbay();
    if (pPatchbay == 0)
        return;

    int iItemID;
    QPopupMenu* pContextMenu = new QPopupMenu(this);

    iItemID = pContextMenu->insertItem(tr("&Connect"), pPatchbay, SLOT(connectSelected()), tr("Alt+C", "Connect"));
    pContextMenu->setItemEnabled(iItemID, pPatchbay->canConnectSelected());
    iItemID = pContextMenu->insertItem(tr("&Disconnect"), pPatchbay, SLOT(disconnectSelected()), tr("Alt+D", "Disconnect"));
    pContextMenu->setItemEnabled(iItemID, pPatchbay->canDisconnectSelected());
    iItemID = pContextMenu->insertItem(tr("Disconnect &All"), pPatchbay, SLOT(disconnectAll()), tr("Alt+A", "Disconect All"));
    pContextMenu->setItemEnabled(iItemID, pPatchbay->canDisconnectAll());

    pContextMenu->insertSeparator();
    iItemID = pContextMenu->insertItem(tr("&Refresh"), pPatchbay, SLOT(refresh()), tr("Alt+R", "Refresh"));

    pContextMenu->exec(pos);
}


// Patchbay binding methods.
void qjackctlPatchbayView::setPatchbay ( qjackctlPatchbay *pPatchbay )
{
    m_pPatchbay = pPatchbay;
}

qjackctlPatchbay *qjackctlPatchbayView::patchbay (void)
{
    return m_pPatchbay;
}


// Patchbay client list accessors.
qjackctlClientList *qjackctlPatchbayView::OClientList (void)
{
    if (m_pPatchbay)
        return m_pPatchbay->OClientList();
    else
        return 0;
}

qjackctlClientList *qjackctlPatchbayView::IClientList (void)
{
    if (m_pPatchbay)
        return m_pPatchbay->OClientList();
    else
        return 0;
}


//----------------------------------------------------------------------
// qjackctlPatchbay -- Output-to-Input client/ports connection object.
//

// Constructor.
qjackctlPatchbay::qjackctlPatchbay ( qjackctlPatchbayView *pPatchbayView, jack_client_t *pJackClient )
{
    m_pPatchbayView = pPatchbayView;
    
    m_pOClientList = new qjackctlClientList(m_pPatchbayView->OListView(), pJackClient, JackPortIsOutput);
    m_pIClientList = new qjackctlClientList(m_pPatchbayView->IListView(), pJackClient, JackPortIsInput);

    m_iExclusive = 0;

    m_pPatchbayView->setPatchbay(this);
}

// Default destructor.
qjackctlPatchbay::~qjackctlPatchbay (void)
{
    // Force end of works here.
    m_iExclusive++;

    m_pPatchbayView->setPatchbay(0);

    delete m_pOClientList;
    delete m_pIClientList;
    m_pOClientList = 0;
    m_pIClientList = 0;
}


// Connection primitive.
void qjackctlPatchbay::connectPorts ( qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
    if (pOPort->findConnectPtr(pIPort) == 0) {
        jack_connect(pOPort->jackClient(), pOPort->clientPortName().latin1(), pIPort->clientPortName().latin1());
        pOPort->addConnect(pIPort);
        pIPort->addConnect(pOPort);
    }
}

// Disconnection primitive.
void qjackctlPatchbay::disconnectPorts ( qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
    if (pOPort->findConnectPtr(pIPort) != 0) {
        jack_disconnect(pOPort->jackClient(), pOPort->clientPortName().latin1(), pIPort->clientPortName().latin1());
        pOPort->removeConnect(pIPort);
        pIPort->removeConnect(pOPort);
    }
}


// Update port connection references.
void qjackctlPatchbay::updateConnections (void)
{
    // For each output client item...
    for (qjackctlClientItem *pOClient = m_pOClientList->clients().first();
            pOClient;
                pOClient = m_pOClientList->clients().next()) {
        // For each output port item...
        for (qjackctlPortItem *pOPort = pOClient->ports().first();
                pOPort;
                    pOPort = pOClient->ports().next()) {
            // Are there already any connections?
            if (pOPort->connects().count() > 0)
                continue;
            // Get port connections...
            const char **ppszClientPorts = jack_port_get_all_connections(m_pOClientList->jackClient(), pOPort->jackPort());
            if (ppszClientPorts) {
                // Now, for each input client port...
                int iClientPort = 0;
                while (ppszClientPorts[iClientPort]) {
                    qjackctlPortItem *pIPort = m_pIClientList->findClientPort(ppszClientPorts[iClientPort]);
                    if (pIPort) {
                        pOPort->addConnect(pIPort);
                        pIPort->addConnect(pOPort);
                    }
                    iClientPort++;
                }
                ::free(ppszClientPorts);
            }
        }
    }
}


// Test if selected ports are connectable.
bool qjackctlPatchbay::canConnectSelected (void)
{
    bool bResult = false;

    if (startExclusive()) {
        bResult = canConnectSelectedEx();
        endExclusive();
    }

    return bResult;
}

bool qjackctlPatchbay::canConnectSelectedEx (void)
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
void qjackctlPatchbay::connectSelected (void)
{
    if (startExclusive()) {
        connectSelectedEx();
        endExclusive();
    }

    m_pPatchbayView->ConnectorView()->update();
}

void qjackctlPatchbay::connectSelectedEx (void)
{
    QListViewItem *pOItem = (m_pOClientList->listView())->selectedItem();
    if (!pOItem)
        return;

    QListViewItem *pIItem = (m_pIClientList->listView())->selectedItem();
    if (!pIItem)
        return;

    if (pOItem->rtti() == QJACKCTL_CLIENTITEM) {
        qjackctlClientItem *pOClient = (qjackctlClientItem *) pOItem;
        if (pIItem->rtti() == QJACKCTL_CLIENTITEM) {
            // Each-to-each connections...
            qjackctlClientItem *pIClient = (qjackctlClientItem *) pIItem;
            qjackctlPortItem *pOPort = pOClient->ports().first();
            qjackctlPortItem *pIPort = pIClient->ports().first();
            while (pOPort && pIPort) {
                connectPorts(pOPort, pIPort);
                pOPort = pOClient->ports().next();
                pIPort = pIClient->ports().next();
            }
        } else {
            // Many(all)-to-one connection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            qjackctlPortItem *pOPort = pOClient->ports().first();
            while (pOPort) {
                connectPorts(pOPort, pIPort);
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
                connectPorts(pOPort, pIPort);
                pIPort = pIClient->ports().next();
            }
        } else {
            // One-to-one connection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            connectPorts(pOPort, pIPort);
        }
    }
}


// Test if selected ports are disconnectable.
bool qjackctlPatchbay::canDisconnectSelected (void)
{
    bool bResult = false;

    if (startExclusive()) {
        bResult = canDisconnectSelectedEx();
        endExclusive();
    }

    return bResult;
}

bool qjackctlPatchbay::canDisconnectSelectedEx (void)
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
void qjackctlPatchbay::disconnectSelected (void)
{
    if (startExclusive()) {
        disconnectSelectedEx();
        endExclusive();
    }

    m_pPatchbayView->ConnectorView()->update();
}

void qjackctlPatchbay::disconnectSelectedEx (void)
{
    QListViewItem *pOItem = (m_pOClientList->listView())->selectedItem();
    if (!pOItem)
        return;

    QListViewItem *pIItem = (m_pIClientList->listView())->selectedItem();
    if (!pIItem)
        return;

    if (pOItem->rtti() == QJACKCTL_CLIENTITEM) {
        qjackctlClientItem *pOClient = (qjackctlClientItem *) pOItem;
        if (pIItem->rtti() == QJACKCTL_CLIENTITEM) {
            // Each-to-each dicconnection...
            qjackctlClientItem *pIClient = (qjackctlClientItem *) pIItem;
            qjackctlPortItem *pOPort = pOClient->ports().first();
            qjackctlPortItem *pIPort = pIClient->ports().first();
            while (pOPort && pIPort) {
                disconnectPorts(pOPort, pIPort);
                pOPort = pOClient->ports().next();
                pIPort = pIClient->ports().next();
            }
        } else {
            // Many(all)-to-one disconnection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            qjackctlPortItem *pOPort = pOClient->ports().first();
            while (pOPort) {
                disconnectPorts(pOPort, pIPort);
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
                disconnectPorts(pOPort, pIPort);
                pIPort = pIClient->ports().next();
            }
        } else {
            // One-to-one disconnection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            disconnectPorts(pOPort, pIPort);
        }
    }
}


// Test if any port is disconnectable.
bool qjackctlPatchbay::canDisconnectAll (void)
{
    bool bResult = false;

    if (startExclusive()) {
        bResult = canDisconnectAllEx();
        endExclusive();
    }

    return bResult;
}

bool qjackctlPatchbay::canDisconnectAllEx (void)
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
void qjackctlPatchbay::disconnectAll (void)
{
    if (startExclusive()) {
        disconnectAllEx();
        endExclusive();
    }

    m_pPatchbayView->ConnectorView()->update();
}

void qjackctlPatchbay::disconnectAllEx (void)
{
    qjackctlClientItem *pOClient = m_pOClientList->clients().first();
    while (pOClient) {
        qjackctlPortItem *pOPort = pOClient->ports().first();
        while (pOPort) {
            qjackctlPortItem *pIPort;
            while ((pIPort = pOPort->connects().first()) != 0)
                disconnectPorts(pOPort, pIPort);
            pOPort = pOClient->ports().next();
        }
        pOClient = m_pOClientList->clients().next();
    }
}


// Complete contents rebuilder.
void qjackctlPatchbay::refresh (void)
{
    if (startExclusive()) {
        m_pOClientList->updateClientPorts();
        m_pIClientList->updateClientPorts();
        updateConnections();
        endExclusive();
    }

    m_pPatchbayView->ConnectorView()->update();
}


// Dunno. But this may avoid some conflicts.
bool qjackctlPatchbay::startExclusive (void)
{
    bool bExclusive = (m_iExclusive == 0);
    if (bExclusive)
        m_iExclusive++;
    return bExclusive;
}

void qjackctlPatchbay::endExclusive (void)
{
    if (m_iExclusive > 0)
        m_iExclusive--;
}


// Patchbay client list accessors.
qjackctlClientList *qjackctlPatchbay::OClientList (void)
{
    return m_pOClientList;
}

qjackctlClientList *qjackctlPatchbay::IClientList (void)
{
    return m_pIClientList;
}


// end of qjackctlPatchbay.cpp

