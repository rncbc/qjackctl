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

#include <stdlib.h>
#include <math.h>

// Local pixmaps.
#include "clienti.xpm"
#include "cliento.xpm"
#include "portlti.xpm"
#include "portlto.xpm"
#include "portlni.xpm"
#include "portlno.xpm"

static int g_iXpmRefCount = 0;

static QPixmap *g_pXpmClientI = 0;  // Input client item pixmap.
static QPixmap *g_pXpmClientO = 0;  // Output client item pixmap.
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
            QListViewItem::setPixmap(0, *g_pXpmPortLTI);
        } else {
            QListViewItem::setPixmap(0, *g_pXpmPortLNI);
        }
    } else if (ulPortFlags & JackPortIsOutput) {
        if (ulPortFlags & JackPortIsTerminal) {
            QListViewItem::setPixmap(0, *g_pXpmPortLTO);
        } else {
            QListViewItem::setPixmap(0, *g_pXpmPortLNO);
        }
    }
}

// Default destructor.
qjackctlPortItem::~qjackctlPortItem (void)
{
    m_pClient->ports().remove(this);
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
    int i, iSuffix1, iSuffix2;

    sName1 = text(iColumn);
    sName2 = pPortItem->text(iColumn);

    for (i = sName1.length() - 1; i > 0; i--) {
        if (!sName1.at(i).isDigit())
            break;
    }
    sPrefix1 = sName1.left(i + 1);

    for (i = sName2.length() - 1; i > 0; i--) {
        if (!sName2.at(i).isDigit())
            break;
    }
    sPrefix2 = sName2.left(i + 1);

    if (sPrefix1 == sPrefix2) {
        iSuffix1 = sName1.right(sName1.length() - i - 1).toInt();
        iSuffix2 = sName2.right(sName2.length() - i - 1).toInt();
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
    } else if (m_pClientList->jackFlags() & JackPortIsOutput) {
        QListViewItem::setPixmap(0, *g_pXpmClientO);
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
qjackctlClientList::qjackctlClientList( QListView *pListView, jack_client_t *pJackClient, unsigned long ulJackFlags )
{
    if (g_iXpmRefCount == 0) {
        g_pXpmClientI = new QPixmap((const char **) clienti_xpm);
        g_pXpmClientO = new QPixmap((const char **) cliento_xpm);
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

    while ((pClient = m_clients.last()) != NULL)
        delete pClient;

    m_clients.clear();

    if (--g_iXpmRefCount == 0) {
        delete g_pXpmClientI;
        delete g_pXpmClientO;
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
QListView *qjackctlClientList::listView (void)
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

    const char **ppszClientPorts = jack_get_ports(m_pJackClient, 0, 0, m_ulJackFlags);
    if (ppszClientPorts == 0)
        return;

    markClientPorts(0);

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

    cleanClientPorts(0);

    ::free(ppszClientPorts);
}

//----------------------------------------------------------------------
// qjackctlPatchbay -- Output-to-Input client/ports connector widget.
//

// Constructor.
qjackctlPatchbay::qjackctlPatchbay ( QWidget *pParent, QListView *pOListView, QListView *pIListView, jack_client_t *pJackClient )
    : QWidget(pParent)
{
    m_pHBoxLayout = new QHBoxLayout(pParent, 0, 0);
    m_pHBoxLayout->setAutoAdd(true);

    m_pOClientList = new qjackctlClientList(pOListView, pJackClient, JackPortIsOutput);
    m_pIClientList = new qjackctlClientList(pIListView, pJackClient, JackPortIsInput);

    QObject::connect(pOListView, SIGNAL(expanded(QListViewItem *)),  this, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(pOListView, SIGNAL(collapsed(QListViewItem *)), this, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(pOListView, SIGNAL(contentsMoving(int, int)),   this, SLOT(contentsMoved(int, int)));

    QObject::connect(pIListView, SIGNAL(expanded(QListViewItem *)),  this, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(pIListView, SIGNAL(collapsed(QListViewItem *)), this, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(pIListView, SIGNAL(contentsMoving(int, int)),   this, SLOT(contentsMoved(int, int)));

    QWidget::raise();
    QWidget::show();
}

// Default destructor.
qjackctlPatchbay::~qjackctlPatchbay (void)
{
    delete m_pOClientList;
    delete m_pIClientList;
    delete m_pHBoxLayout;
}


// Draw visible port connection relation lines
void qjackctlPatchbay::drawConnectionLine ( QPainter& p, int x1, int y1, int x2, int y2, int h1, int h2 )
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
void qjackctlPatchbay::drawConnections (void)
{
    QPainter p(this);
    QRect r1, r2;
    int   x1, y1, h1, b1;
    int   x2, y2, h2, b2;
    int   i, c, rgb[3];

    // Initialize color changer.
    i = c = rgb[0] = rgb[1] = rgb[2] = 0;
    // Almost constants.
    x1 = 0;
    x2 = width();
    h1 = ((m_pOClientList->listView())->header())->sectionRect(0).height();
    h2 = ((m_pIClientList->listView())->header())->sectionRect(0).height();
    // For each client item...
    for (qjackctlClientItem *pOClient = m_pOClientList->clients().first();
            pOClient;
                pOClient = m_pOClientList->clients().next()) {
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
            // To avoid some unnecessary arrows...
            b1 = b2 = 0;
            // Get port connections...
            const char **ppszClientPorts = jack_port_get_all_connections(m_pOClientList->jackClient(), pOPort->jackPort());
            if (ppszClientPorts) {
                // Get starting connector arrow coordinates.
                r1 = (m_pOClientList->listView())->itemRect(pOPort);
                y1 = (r1.top() + r1.bottom()) / 2;
                // If this item is not visible, don't bother to update it.
                if (!pOPort->isVisible() || y1 < 1) {
                    r1 = (m_pOClientList->listView())->itemRect(pOPort->client());
                    y1 = (r1.top() + r1.bottom()) / 2;
                    b1++;
                }
                int iClientPort = 0;
                while (ppszClientPorts[iClientPort] && b2 == 0) {
                    qjackctlPortItem *pIPort = m_pIClientList->findClientPort(ppszClientPorts[iClientPort]);
                    if (pIPort) {
                        // There is a connection from pOPort to pIPort items:
                        r2 = (m_pIClientList->listView())->itemRect(pIPort);
                        y2 = (r2.top() + r2.bottom()) / 2;
                        if (!pIPort->isVisible() || y2 < 1) {
                            r2 = (m_pIClientList->listView())->itemRect(pIPort->client());
                            y2 = (r2.top() + r2.bottom()) / 2;
                            if (b1 > 0)
                                b2++;
                        }
                        drawConnectionLine(p, x1, y1, x2, y2, h1, h2);
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
    QListViewItem *pOItem = (m_pOClientList->listView())->selectedItem();
    if (!pOItem)
        return false;

    QListViewItem *pIItem = (m_pIClientList->listView())->selectedItem();
    if (!pIItem)
        return false;

    // We're not so kinky when one of the selection is a client item...
    if (pOItem->rtti() == QJACKCTL_CLIENTITEM || pIItem->rtti() == QJACKCTL_CLIENTITEM)
        return true;

    // One-to-one connection...
    qjackctlPortItem *pOPort = (qjackctlPortItem *) pOItem;
    qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
    QString sIClientPort = pIPort->clientPortName();
    // Get current port connections...
    const char **ppszClientPorts = jack_port_get_all_connections(m_pOClientList->jackClient(), pOPort->jackPort());
    if (!ppszClientPorts)
        return true;
    int iClientPort = 0;
    while (ppszClientPorts[iClientPort]) {
        if (sIClientPort == ppszClientPorts[iClientPort])
            return false;
        iClientPort++;
    }
    ::free(ppszClientPorts);

    return true;
}


// Connect current selected ports.
void qjackctlPatchbay::connectSelected (void)
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
                jack_connect(pOPort->jackClient(), pOPort->clientPortName().latin1(), pIPort->clientPortName().latin1());
                pOPort = pOClient->ports().next();
                pIPort = pIClient->ports().next();
            }
        } else {
            // Many(all)-to-one connection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            qjackctlPortItem *pOPort = pOClient->ports().first();
            while (pOPort) {
                jack_connect(pOPort->jackClient(), pOPort->clientPortName().latin1(), pIPort->clientPortName().latin1());
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
                jack_connect(pOPort->jackClient(), pOPort->clientPortName().latin1(), pIPort->clientPortName().latin1());
                pIPort = pIClient->ports().next();
            }
        } else {
            // One-to-one connection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            jack_connect(pOPort->jackClient(), pOPort->clientPortName().latin1(), pIPort->clientPortName().latin1());
        }
    }

    QWidget::update();
}


// Test if selected ports are disconnectable.
bool qjackctlPatchbay::canDisconnectSelected (void)
{
    QListViewItem *pOItem = (m_pOClientList->listView())->selectedItem();
    if (!pOItem)
        return false;

    QListViewItem *pIItem = (m_pIClientList->listView())->selectedItem();
    if (!pIItem)
        return false;

    // We're not so kinky when one of the selection is a client item...
    if (pOItem->rtti() == QJACKCTL_CLIENTITEM || pIItem->rtti() == QJACKCTL_CLIENTITEM)
        return true;

    // One-to-one connection...
    qjackctlPortItem *pOPort = (qjackctlPortItem *) pOItem;
    qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
    QString sIClientPort = pIPort->clientPortName();
    // Get current port connections...
    const char **ppszClientPorts = jack_port_get_all_connections(m_pOClientList->jackClient(), pOPort->jackPort());
    if (!ppszClientPorts)
        return false;
    int iClientPort = 0;
    while (ppszClientPorts[iClientPort]) {
        if (sIClientPort == ppszClientPorts[iClientPort])
            return true;
        iClientPort++;
    }
    ::free(ppszClientPorts);

    return false;
}


// Disconnect current selected ports.
void qjackctlPatchbay::disconnectSelected (void)
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
                jack_disconnect(pOPort->jackClient(), pOPort->clientPortName().latin1(), pIPort->clientPortName().latin1());
                pOPort = pOClient->ports().next();
                pIPort = pIClient->ports().next();
            }
        } else {
            // Many(all)-to-one disconnection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            qjackctlPortItem *pOPort = pOClient->ports().first();
            while (pOPort) {
                jack_disconnect(pOPort->jackClient(), pOPort->clientPortName().latin1(), pIPort->clientPortName().latin1());
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
                jack_disconnect(pOPort->jackClient(), pOPort->clientPortName().latin1(), pIPort->clientPortName().latin1());
                pIPort = pIClient->ports().next();
            }
        } else {
            // One-to-one disconnection...
            qjackctlPortItem *pIPort = (qjackctlPortItem *) pIItem;
            jack_disconnect(pOPort->jackClient(), pOPort->clientPortName().latin1(), pIPort->clientPortName().latin1());
        }
    }

    QWidget::update();
}


// Complete contents rebuilder.
void qjackctlPatchbay::refresh (void)
{
    m_pOClientList->updateClientPorts();
    m_pIClientList->updateClientPorts();
    QWidget::update();
}


// Widget fuctions...

void qjackctlPatchbay::paintEvent ( QPaintEvent * )
{
    drawConnections();
}


void qjackctlPatchbay::resizeEvent ( QResizeEvent * )
{
    QWidget::repaint(true);
}


QSizePolicy qjackctlPatchbay::sizePolicy (void) const
{
    return QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}


void qjackctlPatchbay::listViewChanged ( QListViewItem * )
{
    QWidget::update();
}


void qjackctlPatchbay::contentsMoved ( int, int )
{
    QWidget::update();
}


// end of qjackctlPatchbay.cpp

