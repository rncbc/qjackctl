// qjackctlPatchbay.cpp
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

#include "qjackctlPatchbay.h"

#include <qmessagebox.h>
#include <qtimer.h>

#include <stdlib.h>

// Interactivity socket form.
#include "qjackctlSocketForm.h"
// External patchbay loader.
#include "qjackctlPatchbayFile.h"


// Audio socket/plug pixmaps.
#include "icons/asocketo.xpm"
#include "icons/asocketi.xpm"
#include "icons/aplugo.xpm"
#include "icons/aplugi.xpm"

// MIDI socket/plug pixmaps.
#include "icons/msocketi.xpm"
#include "icons/msocketo.xpm"
#include "icons/mplugo.xpm"
#include "icons/mplugi.xpm"


static int g_iXpmRefCount = 0;

static QPixmap *g_pXpmASocketO = 0;  // Audio output socket item pixmap.
static QPixmap *g_pXpmASocketI = 0;  // Audio input socket item pixmap.
static QPixmap *g_pXpmAPlugO   = 0;  // Audio output plug pixmap.
static QPixmap *g_pXpmAPlugI   = 0;  // Audio input plug pixmap.

static QPixmap *g_pXpmMSocketO = 0;  // MIDI output socket item pixmap.
static QPixmap *g_pXpmMSocketI = 0;  // MIDI input socket item pixmap.
static QPixmap *g_pXpmMPlugO   = 0;  // MIDI output plug pixmap.
static QPixmap *g_pXpmMPlugI   = 0;  // MIDI input plug pixmap.


//----------------------------------------------------------------------
// class qjackctlPlugItem -- Socket plug list item.
//

// Constructor.
qjackctlPlugItem::qjackctlPlugItem ( qjackctlSocketItem *pSocket, const QString& sPlugName, qjackctlPlugItem *pPlugAfter )
    : QListViewItem(pSocket, pPlugAfter)
{
    QListViewItem::setText(0, sPlugName);
    
    m_pSocket   = pSocket;
    m_sPlugName = sPlugName;

    m_pSocket->plugs().append(this);

    if (m_pSocket->isReadable()) {
        QListViewItem::setPixmap(0, pSocket->socketType() == QJACKCTL_SOCKETTYPE_MIDI ? *g_pXpmMPlugO : *g_pXpmAPlugO);
        QListViewItem::setDragEnabled(true);
        QListViewItem::setDropEnabled(false);
    } else {
        QListViewItem::setPixmap(0, pSocket->socketType() == QJACKCTL_SOCKETTYPE_MIDI ? *g_pXpmMPlugI : *g_pXpmAPlugI);
        QListViewItem::setDragEnabled(false);
        QListViewItem::setDropEnabled(true);
    }
    
    QListViewItem::setSelectable(false);
}

// Default destructor.
qjackctlPlugItem::~qjackctlPlugItem (void)
{
    m_pSocket->plugs().remove(this);
}


// Instance accessors.
const QString& qjackctlPlugItem::socketName (void)
{
    return m_pSocket->socketName();
}

const QString& qjackctlPlugItem::plugName (void)
{
    return m_sPlugName;
}


// Patchbay socket item accessor.
qjackctlSocketItem *qjackctlPlugItem::socket (void)
{
    return m_pSocket;
}


// To virtually distinguish between list view items.
int qjackctlPlugItem::rtti (void) const
{
    return QJACKCTL_PLUGITEM;
}


//----------------------------------------------------------------------
// class qjackctlSocketItem -- Jack client list item.
//

// Constructor.
qjackctlSocketItem::qjackctlSocketItem ( qjackctlSocketList *pSocketList, const QString& sSocketName, const QString& sClientName, int iSocketType, qjackctlSocketItem *pSocketAfter )
    : QListViewItem(pSocketList->listView(), pSocketAfter)
{
    QListViewItem::setText(0, sSocketName);

    m_pSocketList = pSocketList;
    m_sSocketName = sSocketName;
    m_sClientName = sClientName;
    m_iSocketType = iSocketType;
    m_bExclusive  = false;

    m_plugs.setAutoDelete(false);
    m_connects.setAutoDelete(false);

    m_pSocketList->sockets().append(this);

    if (m_pSocketList->isReadable()) {
        QListViewItem::setPixmap(0, m_iSocketType == QJACKCTL_SOCKETTYPE_MIDI ? *g_pXpmMSocketO : *g_pXpmASocketO);
        QListViewItem::setDragEnabled(true);
        QListViewItem::setDropEnabled(false);
    } else {
        QListViewItem::setPixmap(0, m_iSocketType == QJACKCTL_SOCKETTYPE_MIDI ? *g_pXpmMSocketI : *g_pXpmASocketI);
        QListViewItem::setDragEnabled(false);
        QListViewItem::setDropEnabled(true);
    }

}

// Default destructor.
qjackctlSocketItem::~qjackctlSocketItem (void)
{
    m_connects.clear();
    m_plugs.clear();

    m_pSocketList->sockets().remove(this);
}


// Instance accessors.
const QString& qjackctlSocketItem::socketName (void)
{
    return m_sSocketName;
}

const QString& qjackctlSocketItem::clientName (void)
{
    return m_sClientName;
}

int qjackctlSocketItem::socketType (void)
{
    return m_iSocketType;
}

bool qjackctlSocketItem::isExclusive (void)
{
    return m_bExclusive;
}

void qjackctlSocketItem::setSocketName ( const QString& sSocketName )
{
    m_sSocketName = sSocketName;
}

void qjackctlSocketItem::setClientName ( const QString& sClientName )
{
    m_sClientName = sClientName;
}

void qjackctlSocketItem::setSocketType ( int iSocketType )
{
    m_iSocketType = iSocketType;
}

void qjackctlSocketItem::setExclusive ( bool bExclusive )
{
    m_bExclusive = bExclusive;
}


// Socket flags accessor.
bool qjackctlSocketItem::isReadable (void)
{
    return m_pSocketList->isReadable();
}


// Plug finder.
qjackctlPlugItem *qjackctlSocketItem::findPlug (const QString& sPlugName)
{
    for (qjackctlPlugItem *pPlug = m_plugs.first(); pPlug; pPlug = m_plugs.next()) {
        if (sPlugName == pPlug->plugName())
            return pPlug;
    }
    return 0;
}


// Plug list accessor.
QPtrList<qjackctlPlugItem>& qjackctlSocketItem::plugs (void)
{
    return m_plugs;
}


// Connected socket list primitives.
void qjackctlSocketItem::addConnect( qjackctlSocketItem *pSocket )
{
    m_connects.append(pSocket);
}

void qjackctlSocketItem::removeConnect( qjackctlSocketItem *pSocket )
{
    m_connects.remove(pSocket);
}



// Connected socket finder.
qjackctlSocketItem *qjackctlSocketItem::findConnectPtr ( qjackctlSocketItem *pSocketPtr )
{
    for (qjackctlSocketItem *pSocket = m_connects.first(); pSocket; pSocket = m_connects.next()) {
        if (pSocketPtr == pSocket)
            return pSocket;
    }
    return 0;
}


// Connection cache list accessor.
QPtrList<qjackctlSocketItem>& qjackctlSocketItem::connects (void)
{
    return m_connects;
}


// To virtually distinguish between list view items.
int qjackctlSocketItem::rtti (void) const
{
    return QJACKCTL_SOCKETITEM;
}


// Plug list cleaner.
void qjackctlSocketItem::clear (void)
{
    qjackctlPlugItem *pPlug;

    while ((pPlug = m_plugs.last()) != 0)
        delete pPlug;

    m_plugs.clear();
}


//----------------------------------------------------------------------
// qjackctlSocketList -- Jack client list.
//

// Constructor.
qjackctlSocketList::qjackctlSocketList( qjackctlSocketListView *pListView, bool bReadable )
{
    if (g_iXpmRefCount == 0) {
        g_pXpmASocketO = new QPixmap((const char **) asocketo_xpm);
        g_pXpmASocketI = new QPixmap((const char **) asocketi_xpm);
        g_pXpmAPlugO   = new QPixmap((const char **) aplugo_xpm);
        g_pXpmAPlugI   = new QPixmap((const char **) aplugi_xpm);
        g_pXpmMSocketO = new QPixmap((const char **) msocketo_xpm);
        g_pXpmMSocketI = new QPixmap((const char **) msocketi_xpm);
        g_pXpmMPlugO   = new QPixmap((const char **) mplugo_xpm);
        g_pXpmMPlugI   = new QPixmap((const char **) mplugi_xpm);
    }
    g_iXpmRefCount++;

    m_pListView   = pListView;
    m_bReadable   = bReadable;
    m_pJackClient = NULL;
    m_pAlsaSeq    = NULL;
    
    if (bReadable) {
        m_sSocketCaption = tr("Output");
        m_pXpmASocket = g_pXpmASocketO;
        m_pXpmAPlug   = g_pXpmAPlugO;
        m_pXpmMSocket = g_pXpmMSocketO;
        m_pXpmMPlug   = g_pXpmMPlugO;
    } else {
        m_sSocketCaption = tr("Input");
        m_pXpmASocket = g_pXpmASocketI;
        m_pXpmAPlug   = g_pXpmAPlugI;
        m_pXpmMSocket = g_pXpmMSocketI;
        m_pXpmMPlug   = g_pXpmMPlugI;
    }
    if (!m_sSocketCaption.isEmpty())
        m_sSocketCaption += " ";
    m_sSocketCaption += tr("Socket");
    
    m_sockets.setAutoDelete(false);
}

// Default destructor.
qjackctlSocketList::~qjackctlSocketList (void)
{
    clear();

    if (--g_iXpmRefCount == 0) {
        delete g_pXpmASocketO;
        delete g_pXpmASocketI;
        delete g_pXpmAPlugO;
        delete g_pXpmAPlugI;
        delete g_pXpmMSocketO;
        delete g_pXpmMSocketI;
        delete g_pXpmMPlugO;
        delete g_pXpmMPlugI;
    }
}


// Client finder.
qjackctlSocketItem *qjackctlSocketList::findSocket ( const QString& sSocketName )
{
    for (qjackctlSocketItem *pSocket = m_sockets.first(); pSocket; pSocket = m_sockets.next()) {
        if (sSocketName == pSocket->socketName())
            return pSocket;
    }
    return 0;
}


// Socket list accessor.
QPtrList<qjackctlSocketItem>& qjackctlSocketList::sockets (void)
{
    return m_sockets;
}


// List view accessor.
qjackctlSocketListView *qjackctlSocketList::listView (void)
{
    return m_pListView;
}


// Socket flags accessor.
bool qjackctlSocketList::isReadable (void)
{
    return m_bReadable;
}


// Socket caption titler.
QString& qjackctlSocketList::socketCaption (void)
{
    return m_sSocketCaption;
}


// JACK client accessors.
void qjackctlSocketList::setJackClient ( jack_client_t *pJackClient )
{
    m_pJackClient = pJackClient;
}

jack_client_t *qjackctlSocketList::jackClient (void)
{
    return m_pJackClient;
}


// ALSA sequencer accessors.
void qjackctlSocketList::setAlsaSeq ( snd_seq_t *pAlsaSeq )
{
    m_pAlsaSeq = pAlsaSeq;
}

snd_seq_t *qjackctlSocketList::alsaSeq (void)
{
    return m_pAlsaSeq;
}


// Socket list cleaner.
void qjackctlSocketList::clear (void)
{
    qjackctlSocketItem *pSocket;

    while ((pSocket = m_sockets.last()) != 0)
        delete pSocket;

    m_sockets.clear();
}


// Client:port snapshot.
void qjackctlSocketList::clientPortsSnapshot (void)
{
    // Grab JACK client:port's...
    if (m_pJackClient) {
        const char **ppszClientPorts = jack_get_ports(m_pJackClient, 0, 0, (m_bReadable ? JackPortIsOutput : JackPortIsInput));
        if (ppszClientPorts) {
            int iClientPort = 0;
            while (ppszClientPorts[iClientPort]) {
                QString sClientPort = ppszClientPorts[iClientPort];
                qjackctlSocketItem *pSocket = 0;
                qjackctlPlugItem   *pPlug   = 0;
                int iColon = sClientPort.find(":");
                if (iColon >= 0) {
                    QString sClientName = sClientPort.left(iColon);
                    QString sPortName   = sClientPort.right(sClientPort.length() - iColon - 1);
                    pSocket = findSocket(sClientName);
                    if (pSocket)
                        pPlug = pSocket->findPlug(sPortName);
                    if (pSocket == 0)
                        pSocket = new qjackctlSocketItem(this, sClientName, sClientName, QJACKCTL_SOCKETTYPE_AUDIO, (qjackctlSocketItem *) m_pListView->lastItem());
                    if (pSocket && pPlug == 0) {
                        qjackctlPlugItem *pPlugAfter = (qjackctlPlugItem *) pSocket->firstChild();
                        while (pPlugAfter && pPlugAfter->nextSibling())
                            pPlugAfter = (qjackctlPlugItem *) pPlugAfter->nextSibling();
                        pPlug = new qjackctlPlugItem(pSocket, sPortName, pPlugAfter);
                    }
                }
                iClientPort++;
            }
            ::free(ppszClientPorts);
        }
    }

    // Grab ALSA subscribers's...
    if (m_pAlsaSeq) {
        // Grab the ALSA subscriptions...
        snd_seq_client_info_t *pClientInfo;
        snd_seq_port_info_t   *pPortInfo;
        unsigned int uiAlsaFlags;
        if (m_bReadable)
            uiAlsaFlags = SND_SEQ_PORT_CAP_READ  | SND_SEQ_PORT_CAP_SUBS_READ;
        else
            uiAlsaFlags = SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;
        snd_seq_client_info_alloca(&pClientInfo);
        snd_seq_port_info_alloca(&pPortInfo);
        snd_seq_client_info_set_client(pClientInfo, -1);
        while (snd_seq_query_next_client(m_pAlsaSeq, pClientInfo) >= 0) {
            int iAlsaClient = snd_seq_client_info_get_client(pClientInfo);
            if (iAlsaClient > 0) {
                QString sClientName = snd_seq_client_info_get_name(pClientInfo);
                snd_seq_port_info_set_client(pPortInfo, iAlsaClient);
                snd_seq_port_info_set_port(pPortInfo, -1);
                while (snd_seq_query_next_port(m_pAlsaSeq, pPortInfo) >= 0) {
                    unsigned int uiPortCapability = snd_seq_port_info_get_capability(pPortInfo);
                    if (((uiPortCapability & uiAlsaFlags) == uiAlsaFlags) &&
                        ((uiPortCapability & SND_SEQ_PORT_CAP_NO_EXPORT) == 0)) {
                        QString sPortName = snd_seq_port_info_get_name(pPortInfo);
                        qjackctlSocketItem *pSocket = 0;
                        qjackctlPlugItem   *pPlug   = 0;
                        pSocket = findSocket(sClientName);
                        if (pSocket)
                            pPlug = pSocket->findPlug(sPortName);
                        if (pSocket == 0)
                            pSocket = new qjackctlSocketItem(this, sClientName, sClientName, QJACKCTL_SOCKETTYPE_MIDI, (qjackctlSocketItem *) m_pListView->lastItem());
                        if (pSocket && pPlug == 0) {
                            qjackctlPlugItem *pPlugAfter = (qjackctlPlugItem *) pSocket->firstChild();
                            while (pPlugAfter && pPlugAfter->nextSibling())
                                pPlugAfter = (qjackctlPlugItem *) pPlugAfter->nextSibling();
                            pPlug = new qjackctlPlugItem(pSocket, sPortName, pPlugAfter);
                        }
                    }
                }
            }
        }
    }
}


// Return currently selected socket item.
qjackctlSocketItem *qjackctlSocketList::selectedSocketItem (void)
{
    qjackctlSocketItem *pSocketItem = NULL;

    QListViewItem *pListItem = m_pListView->selectedItem();
//  if (pListItem == NULL)
//      pListItem = m_pListView->currentItem();
    if (pListItem) {
        if (pListItem->rtti() == QJACKCTL_PLUGITEM) {
            pSocketItem = (qjackctlSocketItem *) pListItem->parent();
        } else {
            pSocketItem = (qjackctlSocketItem *) pListItem;
        }
    }

    return pSocketItem;
}


// Add a new socket item, using interactive form.
bool qjackctlSocketList::addSocketItem (void)
{
    bool bResult = false;

    qjackctlSocketForm *pSocketForm = new qjackctlSocketForm(m_pListView);
    if (pSocketForm) {
        pSocketForm->setCaption("<" + tr("New") + "> - " + m_sSocketCaption);
        pSocketForm->setSocketCaption(m_sSocketCaption);
        pSocketForm->setSocketPixmaps(m_pXpmASocket, m_pXpmMSocket);
        pSocketForm->setPlugPixmaps(m_pXpmAPlug, m_pXpmMPlug);
        pSocketForm->setReadable(m_bReadable);
        pSocketForm->setJackClient(m_pJackClient);
        pSocketForm->setAlsaSeq(m_pAlsaSeq);
        qjackctlPatchbaySocket socket(m_sSocketCaption + " " + QString::number(m_sockets.count() + 1), QString::null, QJACKCTL_SOCKETTYPE_AUDIO);
        pSocketForm->load(&socket);
        if (pSocketForm->exec()) {
            pSocketForm->save(&socket);
            qjackctlSocketItem *pSocketItem = selectedSocketItem();
        //  m_pListView->setUpdatesEnabled(false);
            if (pSocketItem)
                pSocketItem->setSelected(false);
            pSocketItem = new qjackctlSocketItem(this, socket.name(), socket.clientName(), socket.type(), pSocketItem);
            if (pSocketItem) {
                if (socket.isExclusive())
                    pSocketItem->setExclusive(true);
                qjackctlPlugItem *pPlugItem = NULL;
                for (QStringList::Iterator iter = socket.pluglist().begin(); iter != socket.pluglist().end(); iter++)
                    pPlugItem = new qjackctlPlugItem(pSocketItem, *iter, pPlugItem);
                bResult = true;
            }
            m_pListView->setCurrentItem(pSocketItem);
            m_pListView->setSelected(pSocketItem, true);
        //  m_pListView->setUpdatesEnabled(true);
        //  m_pListView->triggerUpdate();
            m_pListView->setDirty(true);
        }
        delete pSocketForm;
    }
    
    return bResult;
}


// Remove (delete) currently selected socket item.
bool qjackctlSocketList::removeSocketItem (void)
{
    bool bResult = false;

    qjackctlSocketItem *pSocketItem = selectedSocketItem();
    if (pSocketItem) {
        if (QMessageBox::warning(m_pListView, tr("Warning"),
            m_sSocketCaption + " " + tr("about to be removed") + ":\n\n" +
            "\"" + pSocketItem->socketName() + "\"\n\n" +
            tr("Are you sure?"),
            tr("Yes"), tr("No")) == 0) {
            delete pSocketItem;
            bResult = true;
            m_pListView->setDirty(true);
        }
    }
    
    return bResult;
}


// View or change the properties of currently selected socket item.
bool qjackctlSocketList::editSocketItem (void)
{
    bool bResult = false;

    qjackctlSocketItem *pSocketItem = selectedSocketItem();
    if (pSocketItem) {
        qjackctlSocketForm *pSocketForm = new qjackctlSocketForm(m_pListView);
        if (pSocketForm) {
            pSocketForm->setCaption(pSocketItem->socketName() + " - " + m_sSocketCaption);
            pSocketForm->setSocketCaption(m_sSocketCaption);
            pSocketForm->setSocketPixmaps(m_pXpmASocket, m_pXpmMSocket);
            pSocketForm->setPlugPixmaps(m_pXpmAPlug, m_pXpmMPlug);
            pSocketForm->setReadable(m_bReadable);
            pSocketForm->setJackClient(m_pJackClient);
            pSocketForm->setAlsaSeq(m_pAlsaSeq);
            pSocketForm->enableExclusive(pSocketItem->connects().count() < 2);
            qjackctlPatchbaySocket socket(pSocketItem->socketName(), pSocketItem->clientName(), pSocketItem->socketType());
            if (pSocketItem->isExclusive())
                socket.setExclusive(true);
            for (qjackctlPlugItem *pPlugItem = pSocketItem->plugs().first(); pPlugItem; pPlugItem = pSocketItem->plugs().next())
                socket.pluglist().append(pPlugItem->plugName());
            pSocketForm->load(&socket);
            if (pSocketForm->exec()) {
                pSocketForm->save(&socket);
            //  m_pListView->setUpdatesEnabled(false);
                pSocketItem->clear();
                pSocketItem->setText(0, socket.name());
                pSocketItem->setSocketName(socket.name());
                pSocketItem->setClientName(socket.clientName());
                pSocketItem->setSocketType(socket.type());
                pSocketItem->setExclusive(socket.isExclusive());
                qjackctlPlugItem *pPlugItem = NULL;
                for (QStringList::Iterator iter = socket.pluglist().begin(); iter != socket.pluglist().end(); iter++)
                    pPlugItem = new qjackctlPlugItem(pSocketItem, *iter, pPlugItem);
                m_pListView->setCurrentItem(pSocketItem);
                m_pListView->setSelected(pSocketItem, true);
            //  m_pListView->setUpdatesEnabled(true);
            //  m_pListView->triggerUpdate();
                m_pListView->setDirty(true);
                bResult = true;
            }
            delete pSocketForm;
        }
    }

    return bResult;
}


// Move current selected socket item up one position.
bool qjackctlSocketList::moveUpSocketItem (void)
{
    bool bResult = false;

    qjackctlSocketItem *pSocketItem = selectedSocketItem();
    if (pSocketItem) {
        QListViewItem *pListItem = pSocketItem->itemAbove();
        if (pListItem) {
            pSocketItem->setSelected(false);
        //  m_pListView->setUpdatesEnabled(false);
            if (pListItem->rtti() == QJACKCTL_PLUGITEM)
                pListItem = pListItem->parent();
            pListItem = pListItem->itemAbove();
            if (pListItem) {
                if (pListItem->rtti() == QJACKCTL_PLUGITEM)
                    pListItem = pListItem->parent();
                pSocketItem->moveItem(pListItem);
            } else {
                m_pListView->takeItem(pSocketItem);
                m_pListView->insertItem(pSocketItem);
            }
            m_pListView->setCurrentItem(pSocketItem);
            m_pListView->setSelected(pSocketItem, true);
        //  m_pListView->setUpdatesEnabled(true);
        //  m_pListView->triggerUpdate();
            m_pListView->setDirty(true);
            bResult = true;
        }
    }

    return bResult;
}


// Move current selected socket item down one position.
bool qjackctlSocketList::moveDownSocketItem (void)
{
    bool bResult = false;

    qjackctlSocketItem *pSocketItem = selectedSocketItem();
    if (pSocketItem) {
        QListViewItem *pListItem = pSocketItem->nextSibling();
        if (pListItem) {
            pSocketItem->setSelected(false);
        //  m_pListView->setUpdatesEnabled(false);
            pSocketItem->moveItem(pListItem);
            m_pListView->setCurrentItem(pSocketItem);
            m_pListView->setSelected(pSocketItem, true);
        //  m_pListView->setUpdatesEnabled(true);
        //  m_pListView->triggerUpdate();
            m_pListView->setDirty(true);
            bResult = true;
        }
    }

    return bResult;
}


//----------------------------------------------------------------------------
// qjackctlSocketListView -- Socket list view, supporting drag-n-drop.

// Constructor.
qjackctlSocketListView::qjackctlSocketListView ( QWidget *pParent, qjackctlPatchbayView *pPatchbayView, bool bReadable )
    : QListView(pParent)
{
    m_pPatchbayView = pPatchbayView;
    m_bReadable     = bReadable;
    
    m_pAutoOpenTimer   = 0;
    m_iAutoOpenTimeout = 0;
    m_pDragDropItem    = 0;
    
    QListView::setSorting(-1);
}

// Default destructor.
qjackctlSocketListView::~qjackctlSocketListView (void)
{
    setAutoOpenTimeout(0);
}


// Patchbay view dirty flag accessors.
void qjackctlSocketListView::setDirty ( bool bDirty )
{
    m_pPatchbayView->setDirty(bDirty);
    if (bDirty) {
        emit selectionChanged(NULL);
        m_pPatchbayView->PatchworkView()->update();
    }
}

bool qjackctlSocketListView::dirty (void)
{
    return m_pPatchbayView->dirty();
}


// Auto-open timeout method.
void qjackctlSocketListView::setAutoOpenTimeout ( int iAutoOpenTimeout )
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
int qjackctlSocketListView::autoOpenTimeout (void)
{
    return m_iAutoOpenTimeout;
}


// Auto-open timer slot.
void qjackctlSocketListView::timeoutSlot (void)
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
QListViewItem *qjackctlSocketListView::dragDropItem ( const QPoint& epos )
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
            qjackctlPatchbay *pPatchbay = m_pPatchbayView->binding();
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

void qjackctlSocketListView::dragEnterEvent ( QDragEnterEvent *pDragEnterEvent )
{
    if (QTextDrag::canDecode(pDragEnterEvent) && dragDropItem(pDragEnterEvent->pos())) {
        pDragEnterEvent->accept();
    } else {
        pDragEnterEvent->ignore();
    }
}


void qjackctlSocketListView::dragMoveEvent ( QDragMoveEvent *pDragMoveEvent )
{
    QListViewItem *pItem = dragDropItem(pDragMoveEvent->pos());
    if (pItem) {
        pDragMoveEvent->accept(QListView::itemRect(pItem));
    } else {
        pDragMoveEvent->ignore();
    }
}


void qjackctlSocketListView::dragLeaveEvent ( QDragLeaveEvent * )
{
    m_pDragDropItem = 0;
    if (m_pAutoOpenTimer)
        m_pAutoOpenTimer->stop();
}


void qjackctlSocketListView::dropEvent( QDropEvent *pDropEvent )
{
    QString sText;
    if (QTextDrag::decode(pDropEvent, sText) && dragDropItem(pDropEvent->pos())) {
        m_pDragDropItem = 0;
        if (m_pAutoOpenTimer)
            m_pAutoOpenTimer->stop();
        qjackctlPatchbay *pPatchbay = m_pPatchbayView->binding();
        if (pPatchbay)
            pPatchbay->connectSelected();
    }
}


QDragObject *qjackctlSocketListView::dragObject (void)
{
    QTextDrag *pDragObject = 0;
    if (m_pPatchbayView->binding()) {
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
void qjackctlSocketListView::contextMenuEvent ( QContextMenuEvent *pContextMenuEvent )
{
    m_pPatchbayView->contextMenu(
        pContextMenuEvent->globalPos(),
        (m_bReadable ? m_pPatchbayView->OSocketList() : m_pPatchbayView->ISocketList())
    );
}


//----------------------------------------------------------------------
// qjackctlPatchworkView -- Socket connector widget.
//

// Constructor.
qjackctlPatchworkView::qjackctlPatchworkView ( QWidget *pParent, qjackctlPatchbayView *pPatchbayView)
    : QWidget(pParent)
{
    m_pPatchbayView = pPatchbayView;
}

// Default destructor.
qjackctlPatchworkView::~qjackctlPatchworkView (void)
{
}

// Draw visible socket connection relation lines
void qjackctlPatchworkView::drawConnectionLine ( QPainter& p, int x1, int y1, int x2, int y2, int h1, int h2 )
{
    // Account for list view headers.
    y1 += h1;
    y2 += h2;

    // Invisible output plugs don't get a connecting dot.
    if (y1 > h1)
        p.drawLine(x1, y1, x1 + 4, y1);

    // The connection line, it self.
    p.drawLine(x1 + 4, y1, x2 - 4, y2);

    // Invisible input plugs don't get a connecting dot.
    if (y2 > h2)
        p.drawLine(x2 - 4, y2, x2, y2);
}


// Draw visible socket connection relation arrows.
void qjackctlPatchworkView::drawConnections (void)
{
    if (m_pPatchbayView->OSocketList() == 0 || m_pPatchbayView->ISocketList() == 0)
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
    for (qjackctlSocketItem *pOSocket = m_pPatchbayView->OSocketList()->sockets().first();
            pOSocket;
                pOSocket = m_pPatchbayView->OSocketList()->sockets().next()) {
        // Set new connector color.
        c += 0x39;
        c &= 0xff;
        rgb[i++] = c;
        p.setPen(QColor(rgb[2], rgb[1], rgb[0]));
        if (i > 2)
            i = 0;
        // Get starting connector arrow coordinates.
        r1 = (m_pPatchbayView->OListView())->itemRect(pOSocket);
        y1 = (r1.top() + r1.bottom()) / 2;
        // Get input socket connections...
        for (qjackctlSocketItem *pISocket = pOSocket->connects().first();
                pISocket;
                    pISocket = pOSocket->connects().next()) {
            // Obviously, there is a connection from pOPlug to pIPlug items:
            r2 = (m_pPatchbayView->IListView())->itemRect(pISocket);
            y2 = (r2.top() + r2.bottom()) / 2;
            drawConnectionLine(p, x1, y1, x2, y2, h1, h2);
        }
    }
}


// Widget event handlers...

void qjackctlPatchworkView::paintEvent ( QPaintEvent * )
{
    drawConnections();
}


void qjackctlPatchworkView::resizeEvent ( QResizeEvent * )
{
    QWidget::repaint(true);
}


// Context menu request event handler.
void qjackctlPatchworkView::contextMenuEvent ( QContextMenuEvent *pContextMenuEvent )
{
    m_pPatchbayView->contextMenu(pContextMenuEvent->globalPos(), NULL);
}


// Widget event slots...

void qjackctlPatchworkView::listViewChanged ( QListViewItem * )
{
    QWidget::update();
}


void qjackctlPatchworkView::contentsMoved ( int, int )
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

    m_pOListView = new qjackctlSocketListView(pParent, this, true);
    m_pOListView->addColumn(tr("Output Sockets") + " / " + tr("Plugs"));
    m_pOListView->header()->setClickEnabled(false);
    m_pOListView->header()->setResizeEnabled(false);
    m_pOListView->setMinimumSize(QSize(152, 60));
    m_pOListView->setAllColumnsShowFocus(true);
    m_pOListView->setRootIsDecorated(true);
    m_pOListView->setResizeMode(QListView::AllColumns);
    m_pOListView->setSizePolicy(sizepolicy);

    m_pPatchworkView = new qjackctlPatchworkView(pParent, this);
    m_pPatchworkView->setMinimumSize(QSize(22, 60));
    m_pPatchworkView->setMaximumWidth(120);
    m_pPatchworkView->setSizePolicy(sizepolicy);

    m_pIListView = new qjackctlSocketListView(pParent, this, false);
    m_pIListView->addColumn(tr("Input Sockets") + " / " + tr("Plugs"));
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
    m_pGridLayout->addWidget(m_pPatchworkView, 0, 1);
    m_pGridLayout->addWidget(m_pIListView,     0, 2);

    m_pPatchbay = 0;

    QObject::connect(m_pOListView, SIGNAL(currentChanged(QListViewItem *)), m_pPatchworkView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pOListView, SIGNAL(expanded(QListViewItem *)),  m_pPatchworkView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pOListView, SIGNAL(collapsed(QListViewItem *)), m_pPatchworkView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pOListView, SIGNAL(contentsMoving(int, int)),   m_pPatchworkView, SLOT(contentsMoved(int, int)));

    QObject::connect(m_pIListView, SIGNAL(currentChanged(QListViewItem *)), m_pPatchworkView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pIListView, SIGNAL(expanded(QListViewItem *)),  m_pPatchworkView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pIListView, SIGNAL(collapsed(QListViewItem *)), m_pPatchworkView, SLOT(listViewChanged(QListViewItem *)));
    QObject::connect(m_pIListView, SIGNAL(contentsMoving(int, int)),   m_pPatchworkView, SLOT(contentsMoved(int, int)));

    m_bDirty = false;
}


// Default destructor.
qjackctlPatchbayView::~qjackctlPatchbayView (void)
{
}


// Common context menu slot.
void qjackctlPatchbayView::contextMenu ( const QPoint& pos, qjackctlSocketList *pSocketList )
{
    qjackctlPatchbay *pPatchbay = binding();
    if (pPatchbay == 0)
        return;

    int iItemID;
    QPopupMenu* pContextMenu = new QPopupMenu(this);

    if (pSocketList) {
        qjackctlSocketItem *pSocketItem = pSocketList->selectedSocketItem();
        bool bEnabled = (pSocketItem != NULL);
        iItemID = pContextMenu->insertItem(tr("Add..."), pSocketList, SLOT(addSocketItem()));
        iItemID = pContextMenu->insertItem(tr("Edit..."), pSocketList, SLOT(editSocketItem()));
        pContextMenu->setItemEnabled(iItemID, bEnabled);
        iItemID = pContextMenu->insertItem(tr("Remove"), pSocketList, SLOT(removeSocketItem()));
        pContextMenu->setItemEnabled(iItemID, bEnabled);
        pContextMenu->insertSeparator();
        iItemID = pContextMenu->insertItem(tr("Move Up"), pSocketList, SLOT(moveUpSocketItem()));
        pContextMenu->setItemEnabled(iItemID, (bEnabled && pSocketItem->itemAbove() != NULL));
        iItemID = pContextMenu->insertItem(tr("Move Down"), pSocketList, SLOT(moveDownSocketItem()));
        pContextMenu->setItemEnabled(iItemID, (bEnabled && pSocketItem->nextSibling() != NULL));
        pContextMenu->insertSeparator();
    }

    iItemID = pContextMenu->insertItem(tr("&Connect"), pPatchbay, SLOT(connectSelected()), tr("Alt+C", "Connect"));
    pContextMenu->setItemEnabled(iItemID, pPatchbay->canConnectSelected());
    iItemID = pContextMenu->insertItem(tr("&Disconnect"), pPatchbay, SLOT(disconnectSelected()), tr("Alt+D", "Disconnect"));
    pContextMenu->setItemEnabled(iItemID, pPatchbay->canDisconnectSelected());
    iItemID = pContextMenu->insertItem(tr("Disconnect &All"), pPatchbay, SLOT(disconnectAll()), tr("Alt+A", "Disconect All"));
    pContextMenu->setItemEnabled(iItemID, pPatchbay->canDisconnectAll());

    pContextMenu->insertSeparator();
    iItemID = pContextMenu->insertItem(tr("&Refresh"), pPatchbay, SLOT(refresh()), tr("Alt+R", "Refresh"));

    pContextMenu->exec(pos);
    
    delete pContextMenu;
}


// Patchbay binding methods.
void qjackctlPatchbayView::setBinding ( qjackctlPatchbay *pPatchbay )
{
    m_pPatchbay = pPatchbay;
}

qjackctlPatchbay *qjackctlPatchbayView::binding (void)
{
    return m_pPatchbay;
}


// Patchbay client list accessors.
qjackctlSocketList *qjackctlPatchbayView::OSocketList (void)
{
    if (m_pPatchbay)
        return m_pPatchbay->OSocketList();
    else
        return 0;
}

qjackctlSocketList *qjackctlPatchbayView::ISocketList (void)
{
    if (m_pPatchbay)
        return m_pPatchbay->ISocketList();
    else
        return 0;
}


// Dirty flag methods.
void qjackctlPatchbayView::setDirty ( bool bDirty )
{
    m_bDirty = bDirty;
}

bool qjackctlPatchbayView::dirty (void)
{
    return m_bDirty;
}


//----------------------------------------------------------------------
// qjackctlPatchbay -- Output-to-Input client/plugs connection object.
//

// Constructor.
qjackctlPatchbay::qjackctlPatchbay ( qjackctlPatchbayView *pPatchbayView )
{
    m_pPatchbayView = pPatchbayView;
    
    m_pOSocketList = new qjackctlSocketList(m_pPatchbayView->OListView(), true);
    m_pISocketList = new qjackctlSocketList(m_pPatchbayView->IListView(), false);

    m_pPatchbayView->setBinding(this);
}

// Default destructor.
qjackctlPatchbay::~qjackctlPatchbay (void)
{
    m_pPatchbayView->setBinding(0);

    delete m_pOSocketList;
    delete m_pISocketList;
    m_pOSocketList = 0;
    m_pISocketList = 0;

    m_pPatchbayView->PatchworkView()->repaint(true);
}


// Connection primitive.
void qjackctlPatchbay::connectSockets ( qjackctlSocketItem *pOSocket, qjackctlSocketItem *pISocket )
{
    if (pOSocket->findConnectPtr(pISocket) == 0) {
        pOSocket->addConnect(pISocket);
        pISocket->addConnect(pOSocket);
    }
}

// Disconnection primitive.
void qjackctlPatchbay::disconnectSockets ( qjackctlSocketItem *pOSocket, qjackctlSocketItem *pISocket )
{
    if (pOSocket->findConnectPtr(pISocket) != 0) {
        pOSocket->removeConnect(pISocket);
        pISocket->removeConnect(pOSocket);
    }
}


// Test if selected plugs are connectable.
bool qjackctlPatchbay::canConnectSelected (void)
{
    QListViewItem *pOItem = (m_pOSocketList->listView())->selectedItem();
    if (!pOItem)
        return false;

    QListViewItem *pIItem = (m_pISocketList->listView())->selectedItem();
    if (!pIItem)
        return false;

    qjackctlSocketItem *pOSocket = NULL;
    switch (pOItem->rtti()) {
    case QJACKCTL_SOCKETITEM:
        pOSocket = (qjackctlSocketItem *) pOItem;
        break;
    case QJACKCTL_PLUGITEM:
        pOSocket = ((qjackctlPlugItem *) pOItem)->socket();
        break;
    default:
        return false;
    }
    
    qjackctlSocketItem *pISocket = NULL;
    switch (pIItem->rtti()) {
    case QJACKCTL_SOCKETITEM:
        pISocket = (qjackctlSocketItem *) pIItem;
        break;
    case QJACKCTL_PLUGITEM:
        pISocket = ((qjackctlPlugItem *) pIItem)->socket();
        break;
    default:
        return false;
    }

    // Sockets must be of the same type...
    if (pOSocket->socketType() != pISocket->socketType())
        return false;

    // Exclusive sockets may not accept more than one cable.
    if (pOSocket->isExclusive() && pOSocket->connects().count() > 0)
        return false;
    if (pISocket->isExclusive() && pISocket->connects().count() > 0)
        return false;

    // One-to-one connection...
    return (pOSocket->findConnectPtr(pISocket) == 0);
}


// Connect current selected plugs.
bool qjackctlPatchbay::connectSelected (void)
{
    QListViewItem *pOItem = (m_pOSocketList->listView())->selectedItem();
    if (!pOItem)
        return false;

    QListViewItem *pIItem = (m_pISocketList->listView())->selectedItem();
    if (!pIItem)
        return false;

    qjackctlSocketItem *pOSocket = NULL;
    switch (pOItem->rtti()) {
    case QJACKCTL_SOCKETITEM:
        pOSocket = (qjackctlSocketItem *) pOItem;
        break;
    case QJACKCTL_PLUGITEM:
        pOSocket = ((qjackctlPlugItem *) pOItem)->socket();
        break;
    default:
        return false;
    }
    
    qjackctlSocketItem *pISocket = NULL;
    switch (pIItem->rtti()) {
    case QJACKCTL_SOCKETITEM:
        pISocket = (qjackctlSocketItem *) pIItem;
        break;
    case QJACKCTL_PLUGITEM:
        pISocket = ((qjackctlPlugItem *) pIItem)->socket();
        break;
    default:
        return false;
    }

    // Sockets must be of the same type...
    if (pOSocket->socketType() != pISocket->socketType())
        return false;

    // Exclusive sockets may not accept more than one cable.
    if (pOSocket->isExclusive() && pOSocket->connects().count() > 0)
        return false;
    if (pISocket->isExclusive() && pISocket->connects().count() > 0)
        return false;

    // One-to-one connection...
    connectSockets(pOSocket, pISocket);

    // Making one list dirty will take care of the rest...
    (m_pOSocketList->listView())->setDirty(true);

    return true;
}


// Test if selected plugs are disconnectable.
bool qjackctlPatchbay::canDisconnectSelected (void)
{
    QListViewItem *pOItem = (m_pOSocketList->listView())->selectedItem();
    if (!pOItem)
        return false;

    QListViewItem *pIItem = (m_pISocketList->listView())->selectedItem();
    if (!pIItem)
        return false;

    qjackctlSocketItem *pOSocket = NULL;
    switch (pOItem->rtti()) {
    case QJACKCTL_SOCKETITEM:
        pOSocket = (qjackctlSocketItem *) pOItem;
        break;
    case QJACKCTL_PLUGITEM:
        pOSocket = ((qjackctlPlugItem *) pOItem)->socket();
        break;
    default:
        return false;
    }
    
    qjackctlSocketItem *pISocket = NULL;
    switch (pIItem->rtti()) {
    case QJACKCTL_SOCKETITEM:
        pISocket = (qjackctlSocketItem *) pIItem;
        break;
    case QJACKCTL_PLUGITEM:
        pISocket = ((qjackctlPlugItem *) pIItem)->socket();
        break;
    default:
        return false;
    }

    // Sockets must be of the same type...
    if (pOSocket->socketType() != pISocket->socketType())
        return false;

    return (pOSocket->findConnectPtr(pISocket) != 0);
}


// Disconnect current selected plugs.
bool qjackctlPatchbay::disconnectSelected (void)
{
    QListViewItem *pOItem = (m_pOSocketList->listView())->selectedItem();
    if (!pOItem)
        return false;

    QListViewItem *pIItem = (m_pISocketList->listView())->selectedItem();
    if (!pIItem)
        return false;

    qjackctlSocketItem *pOSocket = NULL;
    switch (pOItem->rtti()) {
    case QJACKCTL_SOCKETITEM:
        pOSocket = (qjackctlSocketItem *) pOItem;
        break;
    case QJACKCTL_PLUGITEM:
        pOSocket = ((qjackctlPlugItem *) pOItem)->socket();
        break;
    default:
        return false;
    }
    
    qjackctlSocketItem *pISocket = NULL;
    switch (pIItem->rtti()) {
    case QJACKCTL_SOCKETITEM:
        pISocket = (qjackctlSocketItem *) pIItem;
        break;
    case QJACKCTL_PLUGITEM:
        pISocket = ((qjackctlPlugItem *) pIItem)->socket();
        break;
    default:
        return false;
    }

    // Sockets must be of the same type...
    if (pOSocket->socketType() != pISocket->socketType())
        return false;

    // One-to-one disconnection...
    disconnectSockets(pOSocket, pISocket);

    // Making one list dirty will take care of the rest...
    (m_pOSocketList->listView())->setDirty(true);

    return true;
}


// Test if any plug is disconnectable.
bool qjackctlPatchbay::canDisconnectAll (void)
{
    qjackctlSocketItem *pOSocket = m_pOSocketList->sockets().first();
    while (pOSocket) {
        if (pOSocket->connects().count() > 0)
            return true;
        pOSocket = m_pOSocketList->sockets().next();
    }
    return false;
}


// Disconnect all plugs.
bool qjackctlPatchbay::disconnectAll (void)
{
    if (QMessageBox::warning(m_pPatchbayView, tr("Warning"),
        tr("This will disconnect all sockets.") + "\n\n" +
        tr("Are you sure?"),
        tr("Yes"), tr("No")) > 0) {
        return false;
    }

    qjackctlSocketItem *pOSocket = m_pOSocketList->sockets().first();
    while (pOSocket) {
        qjackctlSocketItem *pISocket;
        while ((pISocket = pOSocket->connects().first()) != 0)
            disconnectSockets(pOSocket, pISocket);
        pOSocket = m_pOSocketList->sockets().next();
    }

    // Making one list dirty will take care of the rest...
    (m_pOSocketList->listView())->setDirty(true);

    return true;
}



// Complete contents rebuilder.
void qjackctlPatchbay::refresh (void)
{
    (m_pOSocketList->listView())->update();
    (m_pISocketList->listView())->update();
    (m_pPatchbayView->PatchworkView())->update();
}


// Complete contents clearer.
void qjackctlPatchbay::clear (void)
{
    // Clear socket lists.
    m_pOSocketList->clear();
    m_pISocketList->clear();

    // Reset dirty flag.
    m_pPatchbayView->setDirty(false);
    
    // May refresh everything.
    refresh();
}


// Patchbay client list accessors.
qjackctlSocketList *qjackctlPatchbay::OSocketList (void)
{
    return m_pOSocketList;
}

qjackctlSocketList *qjackctlPatchbay::ISocketList (void)
{
    return m_pISocketList;
}


// External rack transfer method: copy patchbay structure from master rack model.
void qjackctlPatchbay::loadRackSockets ( qjackctlSocketList *pSocketList, QPtrList<qjackctlPatchbaySocket>& socketlist )
{
    pSocketList->clear();
    qjackctlSocketItem *pSocketItem = NULL;
    for (qjackctlPatchbaySocket *pSocket = socketlist.first(); pSocket; pSocket = socketlist.next()) {
        pSocketItem = new qjackctlSocketItem(pSocketList, pSocket->name(), pSocket->clientName(), pSocket->type(), pSocketItem);
        if (pSocketItem) {
            if (pSocket->isExclusive())
                pSocketItem->setExclusive(true);
            qjackctlPlugItem *pPlugItem = NULL;
            for (QStringList::Iterator iter = pSocket->pluglist().begin(); iter != pSocket->pluglist().end(); iter++)
                pPlugItem = new qjackctlPlugItem(pSocketItem, *iter, pPlugItem);
        }
    }
}

void qjackctlPatchbay::loadRack ( qjackctlPatchbayRack *pPatchbayRack )
{
    (m_pOSocketList->listView())->setUpdatesEnabled(false);
    (m_pISocketList->listView())->setUpdatesEnabled(false);

    // Load ouput sockets.
    loadRackSockets(m_pOSocketList, pPatchbayRack->osocketlist());

    // Load input sockets.
    loadRackSockets(m_pISocketList, pPatchbayRack->isocketlist());
    
    // Now ready to load from cable model.
    for (qjackctlPatchbayCable *pCable = pPatchbayRack->cablelist().first();
            pCable;
                pCable = pPatchbayRack->cablelist().next()) {
        // Get proper sockets...
        if (pCable->outputSocket() && pCable->inputSocket()) {
            qjackctlSocketItem *pOSocketItem = m_pOSocketList->findSocket((pCable->outputSocket())->name());
            qjackctlSocketItem *pISocketItem = m_pISocketList->findSocket((pCable->inputSocket())->name());
            if (pOSocketItem && pISocketItem)
                connectSockets(pOSocketItem, pISocketItem);
        }
    }

    (m_pOSocketList->listView())->setUpdatesEnabled(true);
    (m_pISocketList->listView())->setUpdatesEnabled(true);

    (m_pOSocketList->listView())->triggerUpdate();
    (m_pISocketList->listView())->triggerUpdate();

    // Reset dirty flag.
    m_pPatchbayView->setDirty(false);
}

// External rack transfer method: copy patchbay structure into master rack model.
void qjackctlPatchbay::saveRackSockets ( qjackctlSocketList *pSocketList, QPtrList<qjackctlPatchbaySocket>& socketlist )
{
    socketlist.clear();

    for (qjackctlSocketItem *pSocketItem = pSocketList->sockets().first(); pSocketItem; pSocketItem = pSocketList->sockets().next()) {
        qjackctlPatchbaySocket *pSocket = new qjackctlPatchbaySocket(pSocketItem->socketName(), pSocketItem->clientName(), pSocketItem->socketType());
        if (pSocket) {
            if (pSocketItem->isExclusive())
                pSocket->setExclusive(true);
            for (qjackctlPlugItem *pPlugItem = pSocketItem->plugs().first(); pPlugItem; pPlugItem = pSocketItem->plugs().next())
                pSocket->pluglist().append(pPlugItem->plugName());
            socketlist.append(pSocket);
        }
    }
}

void qjackctlPatchbay::saveRack ( qjackctlPatchbayRack *pPatchbayRack )
{
    // Save ouput sockets.
    saveRackSockets(m_pOSocketList, pPatchbayRack->osocketlist());

    // Save input sockets.
    saveRackSockets(m_pISocketList, pPatchbayRack->isocketlist());

    // Now ready to save into cable model.
    pPatchbayRack->cablelist().clear();
    // Start from output sockets...
    for (qjackctlSocketItem *pOSocketItem = m_pOSocketList->sockets().first();
            pOSocketItem;
                pOSocketItem = m_pOSocketList->sockets().next()) {
        // Then to input sockets...
        for (qjackctlSocketItem *pISocketItem = pOSocketItem->connects().first();
                pISocketItem;
                    pISocketItem = pOSocketItem->connects().next()) {
            // Now find proper racked sockets...
            qjackctlPatchbaySocket *pOSocket = pPatchbayRack->findSocket(pPatchbayRack->osocketlist(), pOSocketItem->socketName());
            qjackctlPatchbaySocket *pISocket = pPatchbayRack->findSocket(pPatchbayRack->isocketlist(), pISocketItem->socketName());
            if (pOSocket && pISocket)
                pPatchbayRack->addCable(new qjackctlPatchbayCable(pOSocket, pISocket));             
        }        
    }            

    // Reset dirty flag.
    m_pPatchbayView->setDirty(false);
}


// JACK client property accessors.
void qjackctlPatchbay::setJackClient ( jack_client_t *pJackClient )
{
    m_pOSocketList->setJackClient(pJackClient);
    m_pISocketList->setJackClient(pJackClient);
}

jack_client_t *qjackctlPatchbay::jackClient (void)
{
    return m_pOSocketList->jackClient();
}

// ALSA sequencer property accessors.
void qjackctlPatchbay::setAlsaSeq ( snd_seq_t *pAlsaSeq )
{
    m_pOSocketList->setAlsaSeq(pAlsaSeq);
    m_pISocketList->setAlsaSeq(pAlsaSeq);
}

snd_seq_t *qjackctlPatchbay::alsaSeq (void)
{
    return m_pOSocketList->alsaSeq();
}


// Audio connections snapshot.
void qjackctlPatchbay::socketPlugAudioSnapshot ( qjackctlSocketItem *pOSocket, qjackctlPlugItem *pOPlug )
{
    // Audio and MIDI engine descriptors...
    jack_client_t *pJackClient = jackClient();

    // Get current JACK port connections...
    QString sOClientPort = pOSocket->socketName() + ":" + pOPlug->plugName();
    const char **ppszIClientPorts = jack_port_get_all_connections(pJackClient, jack_port_by_name(pJackClient, sOClientPort.latin1()));
    if (ppszIClientPorts) {
        // Now, for each input client port...
        int iIClientPort = 0;
        while (ppszIClientPorts[iIClientPort]) {
            QString sIClientPort = ppszIClientPorts[iIClientPort];
            int iColon = sIClientPort.find(":");
            if (iColon >= 0) {
                qjackctlSocketItem *pISocket = m_pISocketList->findSocket(sIClientPort.left(iColon));
                if (pISocket)
                    connectSockets(pOSocket, pISocket);
            }
            iIClientPort++;
        }
        ::free(ppszIClientPorts);
    }
}


// MIDI connections snapshot.
void qjackctlPatchbay::socketPlugMidiSnapshot ( qjackctlSocketItem *pOSocket, qjackctlPlugItem *pOPlug )
{
    // ALSA sequencer descriptors...
    snd_seq_t *pAlsaSeq = alsaSeq();

    // ALSA subscriber structures.
    snd_seq_query_subscribe_t *pAlsaSubs;
    snd_seq_addr_t seq_addr;
    snd_seq_query_subscribe_alloca(&pAlsaSubs);

    // ALSA readable (output) client ports...
    snd_seq_client_info_t *pOClientInfo;
    snd_seq_port_info_t   *pOPortInfo;
    unsigned int uiOFlags = SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;

    snd_seq_client_info_alloca(&pOClientInfo);
    snd_seq_port_info_alloca(&pOPortInfo);

    // ALSA sequencer writable (input) client ports...
    snd_seq_client_info_t *pIClientInfo;
    snd_seq_client_info_alloca(&pIClientInfo);

    // Get current MIDI port connections...
    snd_seq_client_info_set_client(pOClientInfo, -1);
    while (snd_seq_query_next_client(pAlsaSeq, pOClientInfo) >= 0) {
        int iOClient = snd_seq_client_info_get_client(pOClientInfo);
        if (iOClient > 0) {
            QString sOClientName = snd_seq_client_info_get_name(pOClientInfo);
            if (sOClientName == pOSocket->clientName()) {
                snd_seq_port_info_set_client(pOPortInfo, iOClient);
                snd_seq_port_info_set_port(pOPortInfo, -1);
                while (snd_seq_query_next_port(pAlsaSeq, pOPortInfo) >= 0) {
                    unsigned int uiOCapability = snd_seq_port_info_get_capability(pOPortInfo);
                    if (((uiOCapability & uiOFlags) == uiOFlags) &&
                        ((uiOCapability & SND_SEQ_PORT_CAP_NO_EXPORT) == 0)) {
                        QString sOPortName = snd_seq_port_info_get_name(pOPortInfo);
                        if (sOPortName == pOPlug->plugName()) {
                            int iOPort = snd_seq_port_info_get_port(pOPortInfo);
                            // Now, look for subscribers of his port...
                            snd_seq_query_subscribe_set_type(pAlsaSubs, SND_SEQ_QUERY_SUBS_READ);
                            snd_seq_query_subscribe_set_index(pAlsaSubs, 0);
                            seq_addr.client = iOClient;
                            seq_addr.port   = iOPort;
                            snd_seq_query_subscribe_set_root(pAlsaSubs, &seq_addr);
                            while (snd_seq_query_port_subscribers(pAlsaSeq, pAlsaSubs) >= 0) {
                                seq_addr = *snd_seq_query_subscribe_get_addr(pAlsaSubs);
                                if (snd_seq_get_any_client_info(pAlsaSeq, seq_addr.client, pIClientInfo) == 0) {
                                    QString sIClientName = snd_seq_client_info_get_name(pIClientInfo);
                                    qjackctlSocketItem *pISocket = m_pISocketList->findSocket(sIClientName);
                                    if (pISocket)
                                        connectSockets(pOSocket, pISocket);
                                }
                                snd_seq_query_subscribe_set_index(pAlsaSubs, snd_seq_query_subscribe_get_index(pAlsaSubs) + 1);
                            }
                        }
                    }
                }
            }
        }
    }
}


// Connections snapshot.
void qjackctlPatchbay::connectionsSnapshot (void)
{
    // Take snapshot of client port list.
    m_pOSocketList->clientPortsSnapshot();
    m_pISocketList->clientPortsSnapshot();

    // Then, starting from output sockets, try to grab the connections...
    for (qjackctlSocketItem *pOSocket = m_pOSocketList->sockets().first();
            pOSocket;
                pOSocket = m_pOSocketList->sockets().next()) {

        // For each output plug item...
        for (qjackctlPlugItem *pOPlug = pOSocket->plugs().first();
                pOPlug;
                    pOPlug = pOSocket->plugs().next()) {

            // Check for socket type...
            switch (pOSocket->socketType()) {

              case QJACKCTL_SOCKETTYPE_AUDIO:
                // Get current JACK port connections...
                socketPlugAudioSnapshot(pOSocket, pOPlug);
                break;

              case QJACKCTL_SOCKETTYPE_MIDI:
                // Get current MIDI port connections...
                socketPlugMidiSnapshot(pOSocket, pOPlug);
                break;
           }
        }
    }

    // Surely it is kind of tainted.
    m_pPatchbayView->setDirty(true);
}


// end of qjackctlPatchbay.cpp

