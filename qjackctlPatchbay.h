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

#include <qobject.h>
#include <qptrlist.h>
#include <qlistview.h>
#include <qheader.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpainter.h>

#include <jack/jack.h>

// Forward declarations.
class qjackctlPortItem;
class qjackctlClientItem;
class qjackctlClientList;

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

    // List view accessors.
    QListView *listView();

    // Jack handles accessors.
    jack_client_t *jackClient();
    jack_port_t   *jackPort();

    // Patchbay client item accessor.
    qjackctlClientItem *client();

    // Client port cleanup marker.
    void markPort(int iMark);
    void markClientPort(int iMark);

    int portMark();

private:

    // Special port name sorting virtual comparator.
    int compare (QListViewItem* pPortItem, int iColumn, bool bAscending) const;

    // Instance variables.
    qjackctlClientItem *m_pClient;
    QString      m_sPortName;
    int          m_iPortMark;
    jack_port_t *m_pJackPort;
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

    // List view accessors.
    QListView *listView();

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
    qjackctlClientList(QListView *pListView, jack_client_t *pJackClient, unsigned long ulJackFlags);
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
    QListView *listView();

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
    QListView     *m_pListView;
    jack_client_t *m_pJackClient;
    unsigned long  m_ulJackFlags;
    
    QPtrList<qjackctlClientItem> m_clients;
};


// Jack port connector widget.
class qjackctlPatchbay : public QWidget
{
    Q_OBJECT

public:

    // Constructor.
    qjackctlPatchbay(QWidget *pParent, QListView *pOListView, QListView *pIListView, jack_client_t *pJackClient);
    // Default destructor.
    ~qjackctlPatchbay();

    // Complete contents refreshner.
    void refresh (void);

    // Sizing policies.
    virtual QSizePolicy sizePolicy() const;

    // Explicit connection tests.
    bool canConnectSelected();
    bool canDisconnectSelected();

public slots:

    // Explicit connection slots.
    void connectSelected();
    void disconnectSelected();

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

    // Instance variables.
    QHBoxLayout *m_pHBoxLayout;
    qjackctlClientList *m_pOClientList;
    qjackctlClientList *m_pIClientList;
};


#endif  // __qjackctlPatchbay_h

// end of qjackctlPatchbay.h

