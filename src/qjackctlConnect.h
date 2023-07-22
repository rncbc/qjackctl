// qjackctlConnect.h
//
/****************************************************************************
   Copyright (C) 2003-2022, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctlConnect_h
#define __qjackctlConnect_h

#include "qjackctlAliases.h"

#include <QTreeWidget>
#include <QSplitter>


// QListViewItem::rtti return values.
#define QJACKCTL_CLIENTITEM    1001
#define QJACKCTL_PORTITEM      1002


// Forward declarations.
class qjackctlPortItem;
class qjackctlClientItem;
class qjackctlClientList;
class qjackctlClientListView;
class qjackctlConnectorView;
class qjackctlConnectView;
class qjackctlConnect;


// Port list item.
class qjackctlPortItem : public QTreeWidgetItem
{
public:

	// Constructor.
	qjackctlPortItem(qjackctlClientItem *pClient);
	// Default destructor.
	virtual ~qjackctlPortItem();

	// Instance accessors.
	void setPortName(const QString& sPortName);
	const QString& clientName() const;
	const QString& portName() const;

	// Port name alias accessors.
	void setPortNameAlias(const QString& sPortNameAlias);
	QString portNameAlias(bool *pbRenameEnabled) const;

	// Proto-pretty/display name accessors.
	virtual void updatePortName(bool bRename = false);

	// Tooltip text building.
	virtual QString tooltip() const;

	// Complete client:port name helper.
	QString clientPortName() const;

	// Connections client item method.
	qjackctlClientItem *client() const;

	// Client port cleanup marker.
	void markPort(int iMark);
	void markClientPort(int iMark);

	int portMark() const;

	// Connected port list primitives.
	void addConnect(qjackctlPortItem *pPort);
	void removeConnect(qjackctlPortItem *pPort);

	// Connected port finders.
	qjackctlPortItem *findConnect(const QString& sClientPortName);
	qjackctlPortItem *findConnectPtr(qjackctlPortItem *pPortPtr);

	// Connection list accessor.
	const QList<qjackctlPortItem *>& connects() const;

	// Connectiopn highlight methods.
	bool isHilite() const;
	void setHilite (bool bHilite);

	// Proxy sort override method.
	// - Natural decimal sorting comparator.
	bool operator< (const QTreeWidgetItem& other) const;

protected:

	// Port name display name accessors.
	void setPortText(const QString& sPortText, bool bRenameEnabled);
	QString portText() const;

private:

	// Instance variables.
	qjackctlClientItem *m_pClient;

	QString m_sPortName;
	int     m_iPortMark;
	bool    m_bHilite;

	// Connection cache list.
	QList<qjackctlPortItem *> m_connects;
};


// Client list item.
class qjackctlClientItem : public QTreeWidgetItem
{
public:

	// Constructor.
	qjackctlClientItem(qjackctlClientList *pClientList);
	// Default destructor.
	virtual ~qjackctlClientItem();

	// Port list primitive methods.
	void addPort(qjackctlPortItem *pPort);
	void removePort(qjackctlPortItem *pPort);

	// Port finder.
	qjackctlPortItem *findPort(const QString& sPortName);

	// Instance accessors.
	void setClientName(const QString& sClientName);
	const QString& clientName() const;

	// Client name alias accessors.
	void setClientNameAlias(const QString& sClientNameAlias);
	QString clientNameAlias(bool *pbRenameEnabled) const;

	// Proto-pretty/display name method.
	virtual void updateClientName(bool bRename = false);

	// Readable flag accessor.
	bool isReadable() const;

	// Client list accessor.
	qjackctlClientList *clientList() const;

	// Port list accessor.
	QList<qjackctlPortItem *>& ports();

	// Client port cleanup marker.
	void markClient(int iMark);
	void markClientPorts(int iMark);
	int cleanClientPorts(int iMark);
	int clientMark() const;

	// Connectiopn highlight methods.
	bool isHilite() const;
	void setHilite (bool bHilite);

	// Client item openness status.
	void setOpen(bool bOpen);
	bool isOpen() const;

	// Proxy sort override method.
	// - Natural decimal sorting comparator.
	bool operator< (const QTreeWidgetItem& other) const;

protected:

	// Client name display name accessors.
	void setClientText(const QString& sClientText, bool bRenameEnabled);
	QString clientText() const;

private:

	// Instance variables.
	qjackctlClientList *m_pClientList;

	QString m_sClientName;
	int     m_iClientMark;
	int     m_iHilite;

	QList<qjackctlPortItem *> m_ports;
};


// Jack client list.
class qjackctlClientList : public QObject
{
public:

	// Constructor.
	qjackctlClientList(qjackctlClientListView *pListView, bool bReadable);
	// Default destructor.
	~qjackctlClientList();

	// Do proper contents cleanup.
	void clear();

	// Client list primitive methods.
	void addClient(qjackctlClientItem *pClient);
	void removeClient(qjackctlClientItem *pClient);

	// Client finder.
	qjackctlClientItem *findClient(const QString& sClientName);
	// Client:port finder.
	qjackctlPortItem *findClientPort(const QString& sClientPort);

	// List view accessor.
	qjackctlClientListView *listView() const;

	// Readable flag accessor.
	bool isReadable() const;

	// Client list accessor.
	QList<qjackctlClientItem *>& clients();

	// Client ports cleanup marker.
	void markClientPorts(int iMark);
	int cleanClientPorts(int iMark);

	// Client:port refreshner (return newest item count).
	virtual int updateClientPorts() = 0;

	// Client:port hilite update stabilization.
	void hiliteClientPorts (void);

	// Do proper contents refresh/update.
	void refresh();

	// Natural decimal sorting comparator.
	static bool lessThan(
		const QTreeWidgetItem& item1, const QTreeWidgetItem& item2);

private:

	// Instance variables.
	qjackctlClientListView *m_pListView;
	bool m_bReadable;

	QList<qjackctlClientItem *> m_clients;

	QTreeWidgetItem *m_pHiliteItem;
};


//----------------------------------------------------------------------------
// qjackctlClientListView -- Client list view, supporting drag-n-drop.

class qjackctlClientListView : public QTreeWidget
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlClientListView(qjackctlConnectView *pConnectView, bool bReadable);
	// Default destructor.
	~qjackctlClientListView();

	// Auto-open timer methods.
	void setAutoOpenTimeout(int iAutoOpenTimeout);
	int autoOpenTimeout() const;

	// Aliasing support methods.
	void setAliasList(qjackctlAliasList *pAliasList, bool bRenameEnabled);
	qjackctlAliasList *aliasList() const;
	bool isRenameEnabled() const;

	// Binding indirect accessor.
	qjackctlConnect *binding() const;

	// Dirty aliases notification.
	void emitAliasesChanged();

protected slots:

	// In-place aliasing slots.
	void startRenameSlot();
	void renamedSlot();
	// Auto-open timeout slot.
	void timeoutSlot();

protected:

	// Trap for help/tool-tip events.
	bool eventFilter(QObject *pObject, QEvent *pEvent);

	// Drag-n-drop stuff.
	QTreeWidgetItem *dragDropItem(const QPoint& pos);

	// Drag-n-drop stuff -- reimplemented virtual methods.
	void dragEnterEvent(QDragEnterEvent *pDragEnterEvent);
	void dragMoveEvent(QDragMoveEvent *pDragMoveEvent);
	void dragLeaveEvent(QDragLeaveEvent *);
	void dropEvent(QDropEvent *pDropEvent);

	// Handle mouse events for drag-and-drop stuff.
	void mousePressEvent(QMouseEvent *pMouseEvent);
	void mouseMoveEvent(QMouseEvent *pMouseEvent);

	// Context menu request event handler.
	void contextMenuEvent(QContextMenuEvent *);

private:

	// Bindings.
	qjackctlConnectView *m_pConnectView;

	// Auto-open timer.
	int     m_iAutoOpenTimeout;
	QTimer *m_pAutoOpenTimer;

	// Items we'll eventually drop something.
	QTreeWidgetItem *m_pDragItem;
	QTreeWidgetItem *m_pDropItem;
	// The point from where drag started.
	QPoint m_posDrag;

	// Aliasing support.
	qjackctlAliasList *m_pAliasList;
	bool m_bRenameEnabled;
};


//----------------------------------------------------------------------------
// qjackctlConnectorView -- Jack port connector widget.

class qjackctlConnectorView : public QWidget
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlConnectorView(qjackctlConnectView *pConnectView);
	// Default destructor.
	~qjackctlConnectorView();

protected slots:

	// Useful slots (should this be protected?).
	void contentsChanged();

protected:

	// Draw visible port connection relation arrows.
	void paintEvent(QPaintEvent *);

	// Context menu request event handler.
	virtual void contextMenuEvent(QContextMenuEvent *);

private:

	// Legal client/port item position helper.
	int itemY(QTreeWidgetItem *pItem) const;

	// Drawing methods.
	void drawConnectionLine(QPainter *pPainter,
		int x1, int y1, int x2, int y2, int h1, int h2, const QPen& pen);

	// Local instance variables.
	qjackctlConnectView *m_pConnectView;
};


//----------------------------------------------------------------------------
// qjackctlConnectView -- Connections view integrated widget.

class qjackctlConnectView : public QSplitter
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlConnectView(QWidget *pParent = 0);
	// Default destructor.
	~qjackctlConnectView();

	// Widget accesors.
	qjackctlClientListView *OListView() const
		{ return m_pOListView; }
	qjackctlClientListView *IListView() const
		{ return m_pIListView; }
	qjackctlConnectorView  *connectorView() const
		{ return m_pConnectorView; }

	// Connections object binding methods.
	void setBinding(qjackctlConnect *pConnect);
	qjackctlConnect *binding() const;

	// Client list accessors.
	qjackctlClientList *OClientList() const;
	qjackctlClientList *IClientList() const;

	// Common icon size pixmap accessors.
	void setIconSize (int iIconSize);
	int iconSize (void) const;

	// Dirty aliases notification.
	void emitAliasesChanged();

signals:

	// Contents change signal.
	void aliasesChanged();

private:

	// Child controls.
	qjackctlClientListView *m_pOListView;
	qjackctlClientListView *m_pIListView;
	qjackctlConnectorView  *m_pConnectorView;

	// The main binding object.
	qjackctlConnect *m_pConnect;

	// How large will be those icons.
	// 0 = 16x16 (default), 1 = 32x32, 2 = 64x64.
	int m_iIconSize;

	// The obnoxious dirty flag.
	bool m_bDirty;
};


//----------------------------------------------------------------------------
// qjackctlConnect -- Connections model integrated object.

class qjackctlConnect : public QObject
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlConnect(qjackctlConnectView *pConnectView);
	// Default destructor.
	~qjackctlConnect();

	// Explicit connection tests.
	bool canConnectSelected();
	bool canDisconnectSelected();
	bool canDisconnectAll();

	// Client list accessors.
	qjackctlClientList *OClientList() const;
	qjackctlClientList *IClientList() const;

public slots:

	// Incremental contents refreshner; check dirty status.
	void refresh();

	// Explicit connection slots.
	bool connectSelected();
	bool disconnectSelected();
	bool disconnectAll();

	// Expand all client ports.
	void expandAll();

	// Complete/incremental contents rebuilder; check dirty status if incremental.
	void updateContents(bool bClear);

signals:

	// Connection change signal.
	void connectChanged();

	// Pre-notification of (dis)connection.
	void connecting(qjackctlPortItem *, qjackctlPortItem *);
	void disconnecting(qjackctlPortItem *, qjackctlPortItem *);

protected:

	// Connect/Disconnection primitives.
	virtual bool connectPorts(
		qjackctlPortItem *pOPort, qjackctlPortItem *pIPort) = 0;
	virtual bool disconnectPorts(
		qjackctlPortItem *pOPort, qjackctlPortItem *pIPort) = 0;

	// Update port connection references.
	virtual void updateConnections() = 0;

	// These must be accessed by the descendant constructor.
	qjackctlConnectView *connectView() const;
	void setOClientList(qjackctlClientList *pOClientList);
	void setIClientList(qjackctlClientList *pIClientList);

	// Common pixmap factory helper-method.
	QPixmap *createIconPixmap (const QString& sIconName);

	// Update icon size implementation.
	virtual void updateIconPixmaps() = 0;

private:

	// Dunno. But this may avoid some conflicts.
	bool startMutex();
	void endMutex();

	// Connection methods (unguarded).
	bool canConnectSelectedEx();
	bool canDisconnectSelectedEx();
	bool canDisconnectAllEx();
	bool connectSelectedEx();
	bool disconnectSelectedEx();
	bool disconnectAllEx();

	// Connect/Disconnection local primitives.
	bool connectPortsEx(qjackctlPortItem *pOPort, qjackctlPortItem *pIPort);
	bool disconnectPortsEx(qjackctlPortItem *pOPort, qjackctlPortItem *pIPort);

	// Instance variables.
	qjackctlConnectView *m_pConnectView;
	// These must be created on the descendant constructor.
	qjackctlClientList *m_pOClientList;
	qjackctlClientList *m_pIClientList;
	int m_iMutex;
};


#endif  // __qjackctlConnect_h

// end of qjackctlConnect.h
