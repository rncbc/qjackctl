// qjackctlPatchbay.h
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

#ifndef __qjackctlPatchbay_h
#define __qjackctlPatchbay_h

#include <qdragobject.h>
#include <qlistview.h>
#include <qheader.h>
#include <qlayout.h>
#include <qptrlist.h>
#include <qpixmap.h>
#include <qpainter.h>

#include <jack/jack.h>

// QListViewItem::rtti return values.
#define QJACKCTL_CLIENTITEM 1001
#define QJACKCTL_PORTITEM   1002

// Forward declarations.
class qjackctlPortItem;
class qjackctlClientItem;
class qjackctlClientList;
class qjackctlClientListView;
class qjackctlConnectorView;
class qjackctlPatchbayView;
class qjackctlPatchbay;

// Jack port list item.
class qjackctlPortItem : public QListViewItem
{
public:

    // Constructor.
    qjackctlPortItem(qjackctlClientItem *pClient, QString sPortName, jack_port_t *pJackPort);
    // Default destructor.
    ~qjackctlPortItem();

    // Instance accessors.
    QString& clientName();
    QString& portName();

    // Complete client:port name helper.
    QString clientPortName();

    // Jack handles accessors.
    jack_client_t *jackClient();
    jack_port_t   *jackPort();

    // Patchbay client item accessor.
    qjackctlClientItem *client();

    // Client port cleanup marker.
    void markPort(int iMark);
    void markClientPort(int iMark);

    int portMark();

    // Connected port list primitives.
    void addConnect(qjackctlPortItem *pPort);
    void removeConnect(qjackctlPortItem *pPort);

    // Connected port finders.
    qjackctlPortItem *findConnect(const QString& sClientPortName);
    qjackctlPortItem *findConnectPtr(qjackctlPortItem *pPortPtr);
    // Connection list accessor.
    QPtrList<qjackctlPortItem>& connects();

    // To virtually distinguish between list view items.
    virtual int rtti() const;

    // Special port name sorting virtual comparator.
    virtual int compare (QListViewItem* pPortItem, int iColumn, bool bAscending) const;

private:

    // Instance variables.
    qjackctlClientItem *m_pClient;
    QString      m_sPortName;
    int          m_iPortMark;
    jack_port_t *m_pJackPort;
    
    // Connection cache list.
    QPtrList<qjackctlPortItem> m_connects;
};


// Jack client list item.
class qjackctlClientItem : public QListViewItem
{
public:

    // Constructor.
    qjackctlClientItem(qjackctlClientList *pClientList, QString sClientName);
    // Default destructor.
    ~qjackctlClientItem();

    // Port list primitive methods.
    void addPort(qjackctlPortItem *pPort);
    void removePort(qjackctlPortItem *pPort);

    // Port finder.
    qjackctlPortItem *findPort(QString sPortName);

    // Instance accessors.
    QString& clientName();

    // Jack client accessors.
    jack_client_t *jackClient();
    unsigned long  jackFlags();

    // Port list accessor.
    QPtrList<qjackctlPortItem>& ports();

    // Client port cleanup marker.
    void markClient(int iMark);
    void markClientPorts(int iMark);
    void cleanClientPorts(int iMark);

    int clientMark();

    // To virtually distinguish between list view items.
    int rtti() const;

private:

    // Instance variables.
    qjackctlClientList *m_pClientList;
    QString m_sClientName;
    int     m_iClientMark;

    QPtrList<qjackctlPortItem> m_ports;
};


// Jack client list.
class qjackctlClientList : public QObject
{
    Q_OBJECT

public:

    // Constructor.
    qjackctlClientList(qjackctlClientListView *pListView, jack_client_t *pJackClient, unsigned long ulJackFlags);
    // Default destructor.
    ~qjackctlClientList();

    // Client list primitive methods.
    void addClient(qjackctlClientItem *pClient);
    void removeClient(qjackctlClientItem *pClient);

    // Client finder.
    qjackctlClientItem *findClient(QString sClientName);
    // Client:port finder.
    qjackctlPortItem   *findClientPort(QString sClientPort);

    // List view accessor.
    qjackctlClientListView *listView();

    // Jack client accessors.
    jack_client_t *jackClient();
    unsigned long  jackFlags();

    // Client list accessor.
    QPtrList<qjackctlClientItem>& clients();
    
    // Client ports cleanup marker.
    void markClientPorts(int iMark);
    void cleanClientPorts(int iMark);

    // Client:port refreshner.
    void updateClientPorts();

private:

    // Instance variables.
    qjackctlClientListView *m_pListView;
    jack_client_t *m_pJackClient;
    unsigned long  m_ulJackFlags;

    QPtrList<qjackctlClientItem> m_clients;
};


//----------------------------------------------------------------------------
// qjackctlClientListView -- Client list view, supporting drag-n-drop.

class qjackctlClientListView : public QListView
{
    Q_OBJECT

public:

    // Constructor.
    qjackctlClientListView(QWidget *pParent, qjackctlPatchbayView *pPatchbayView);
    // Default destructor.
    ~qjackctlClientListView();

    // Auto-open timer methods.
    void setAutoOpenTimeout(int iAutoOpenTimeout);
    int autoOpenTimeout();

protected slots:

    // Auto-open timeout slot.
    void timeoutSlot();
    // Context menu slot.
    void contextMenuSlot(QListViewItem *, const QPoint&, int);

protected:

    // Drag-n-drop stuff -- reimplemented virtual methods.
    virtual void dragEnterEvent(QDragEnterEvent *pDragEnterEvent);
    virtual void dragMoveEvent(QDragMoveEvent *pDragMoveEvent);
    virtual void dragLeaveEvent(QDragLeaveEvent *);
    virtual void dropEvent(QDropEvent *pDropEvent);
    virtual QDragObject *dragObject();

private:

    // Bindings.
    qjackctlPatchbayView *m_pPatchbayView;

    // Drag-n-drop stuff.
    QListViewItem *dragDropItem(const QPoint& epos);

    // Auto-open timer.
    int    m_iAutoOpenTimeout;
    QTimer *m_pAutoOpenTimer;
    // Item we'll eventually drop something.
    QListViewItem *m_pDragDropItem;
};


//----------------------------------------------------------------------------
// qjackctlConnectorView -- Jack port connector widget.

class qjackctlConnectorView : public QWidget
{
    Q_OBJECT

public:

    // Constructor.
    qjackctlConnectorView(QWidget *pParent, qjackctlPatchbayView *pPatchbayView);
    // Default destructor.
    ~qjackctlConnectorView();

public slots:

    // Useful slots (should this be protected?).
    void listViewChanged(QListViewItem *);
    void contentsMoved(int, int);

protected:

    // Specific event handlers.
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);

private:

    // Drawing methods.
    void drawConnectionLine(QPainter& p, int x1, int y1, int x2, int y2, int h1, int h2);
    void drawConnections();
    
    // Local instance variables.
    qjackctlPatchbayView *m_pPatchbayView;
};


//----------------------------------------------------------------------------
// qjackctlPatchbayView -- Patchbay integrated widget.

class qjackctlPatchbayView : public QWidget
{
    Q_OBJECT

public:

    // Constructor.
    qjackctlPatchbayView(QWidget *pParent = 0, const char *pszName = 0);
    // Default destructor.
    ~qjackctlPatchbayView();

    // Widget accesors.
    QGridLayout            *GridLayout()    { return m_pGridLayout; }
    qjackctlClientListView *OListView()     { return m_pOListView; }
    qjackctlClientListView *IListView()     { return m_pIListView; }
    qjackctlConnectorView  *ConnectorView() { return m_pConnectorView; }

    // Patchbay object binding methods.
    void setPatchbay(qjackctlPatchbay *pPatchbay);
    qjackctlPatchbay *patchbay();
    
    // Client list accessors.
    qjackctlClientList *OClientList();
    qjackctlClientList *IClientList();

private:

    // Child controls.
    QGridLayout            *m_pGridLayout;
    qjackctlClientListView *m_pOListView;
    qjackctlClientListView *m_pIListView;
    qjackctlConnectorView  *m_pConnectorView;

    // The main binding object.
    qjackctlPatchbay *m_pPatchbay;
};


//----------------------------------------------------------------------------
// qjackctlPatchbay -- Patchbay integrated object.

class qjackctlPatchbay : public QObject
{
    Q_OBJECT

public:

    // Constructor.
    qjackctlPatchbay(qjackctlPatchbayView *pPatchbayView, jack_client_t *pJackClient);
    // Default destructor.
    ~qjackctlPatchbay();

    // Explicit connection tests.
    bool canConnectSelected();
    bool canDisconnectSelected();
    bool canDisconnectAll();

    // Client list accessors.
    qjackctlClientList *OClientList();
    qjackctlClientList *IClientList();

public slots:

    // Complete contents refreshner.
    void refresh (void);

    // Explicit connection slots.
    void connectSelected();
    void disconnectSelected();
    void disconnectAll();

private:

    // Dunno. But this may avoid some conflicts.
    bool startExclusive();
    void endExclusive();

    // Update port connection references.
    void updateConnections();

    // Connect/Disconnection primitives.
    void connectPorts(qjackctlPortItem *pOPort, qjackctlPortItem *pIPort);
    void disconnectPorts(qjackctlPortItem *pOPort, qjackctlPortItem *pIPort);

    // Connection methods (unguarded).
    bool canConnectSelectedEx();
    bool canDisconnectSelectedEx();
    bool canDisconnectAllEx();
    void connectSelectedEx();
    void disconnectSelectedEx();
    void disconnectAllEx();

    // Instance variables.
    qjackctlPatchbayView *m_pPatchbayView;
    qjackctlClientList *m_pOClientList;
    qjackctlClientList *m_pIClientList;
    int m_iExclusive;
};


#endif  // __qjackctlPatchbay_h

// end of qjackctlPatchbay.h
