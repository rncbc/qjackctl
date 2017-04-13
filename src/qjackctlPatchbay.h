// qjackctlPatchbay.h
//
/****************************************************************************
   Copyright (C) 2003-2017, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctlPatchbay_h
#define __qjackctlPatchbay_h

#include <QTreeWidget>
#include <QSplitter>


// Our external patchbay models.
#include "qjackctlPatchbayRack.h"

// QListViewItem::rtti return values.
#define QJACKCTL_SOCKETITEM 2001
#define QJACKCTL_PLUGITEM   2002

// Forward declarations.
class qjackctlPlugItem;
class qjackctlSocketItem;
class qjackctlSocketList;
class qjackctlSocketListView;
class qjackctlPatchworkView;
class qjackctlPatchbayView;
class qjackctlPatchbay;

// Pixmap-set indexes.
#define QJACKCTL_XPM_AUDIO_SOCKET   0
#define QJACKCTL_XPM_AUDIO_SOCKET_X 1
#define QJACKCTL_XPM_AUDIO_CLIENT   2
#define QJACKCTL_XPM_AUDIO_PLUG     3
#define QJACKCTL_XPM_MIDI_SOCKET    4
#define QJACKCTL_XPM_MIDI_SOCKET_X  5
#define QJACKCTL_XPM_MIDI_CLIENT    6
#define QJACKCTL_XPM_MIDI_PLUG      7
#define QJACKCTL_XPM_PIXMAPS        8


// Patchbay plug (port) list item.
class qjackctlPlugItem : public QTreeWidgetItem
{
public:

	// Constructor.
	qjackctlPlugItem(qjackctlSocketItem *pSocket,
		const QString& sPlugName, qjackctlPlugItem *pPlugAfter);
	// Default destructor.
	~qjackctlPlugItem();

	// Instance accessors.
	const QString& socketName() const;
	const QString& plugName() const;

	// Patchbay socket item accessor.
	qjackctlSocketItem *socket() const;

private:

	// Instance variables.
	qjackctlSocketItem *m_pSocket;
	QString m_sPlugName;
};


// Patchbay socket (client) list item.
class qjackctlSocketItem : public QTreeWidgetItem
{
public:

	// Constructor.
	qjackctlSocketItem(qjackctlSocketList *pSocketList,
		const QString& sSocketName, const QString& sClientName,
		int iSocketType,qjackctlSocketItem *pSocketAfter);

	// Default destructor.
	~qjackctlSocketItem();

	// Instance accessors.
	const QString& socketName() const;
	const QString& clientName() const;
	int socketType() const;
	bool isExclusive() const;
	const QString& forward() const;

	void setSocketName (const QString& sSocketName);
	void setClientName (const QString& sClientName);
	void setSocketType (int iSocketType);
	void setExclusive  (bool bExclusive);
	void setForward    (const QString& sSocketForward);

	// Socket flags accessor.
	bool isReadable() const;

	// Connected plug list primitives.
	void addConnect(qjackctlSocketItem *pSocket);
	void removeConnect(qjackctlSocketItem *pSocket);

	// Connected plug  finders.
	qjackctlSocketItem *findConnectPtr(qjackctlSocketItem *pSocketPtr);

	// Connection list accessor.
	const QList<qjackctlSocketItem *>& connects() const;

	// Plug list primitive methods.
	void addPlug(qjackctlPlugItem *pPlug);
	void removePlug(qjackctlPlugItem *pPlug);

	// Plug finder.
	qjackctlPlugItem *findPlug(const QString& sPlugName);

	// Plug list accessor.
	QList<qjackctlPlugItem *>& plugs();

	// Plug list cleaner.
	void clear();

	// Retrieve a context pixmap.
	const QPixmap& pixmap(int iPixmap) const;

	// Update pixmap to its proper context.
	void updatePixmap();

	// Client item openness status.
	void setOpen(bool bOpen);
	bool isOpen() const;

private:

	// Instance variables.
	qjackctlSocketList *m_pSocketList;
	QString m_sSocketName;
	QString m_sClientName;
	int m_iSocketType;
	bool m_bExclusive;
	QString m_sSocketForward;

	// Plug (port) list.
	QList<qjackctlPlugItem *> m_plugs;

	// Connection cache list.
	QList<qjackctlSocketItem *> m_connects;
};


// Patchbay socket (client) list.
class qjackctlSocketList : public QObject
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlSocketList(qjackctlSocketListView *pListView, bool bReadable);
	// Default destructor.
	~qjackctlSocketList();

	// Socket list primitive methods.
	void addSocket(qjackctlSocketItem *pSocket);
	void removeSocket(qjackctlSocketItem *pSocket);

	// Socket finder.
	qjackctlSocketItem *findSocket(const QString& sSocketName, int iSocketType);

	// List view accessor.
	qjackctlSocketListView *listView() const;

	// Socket flags accessor.
	bool isReadable() const;

	// Socket aesthetics accessors.
	const QString& socketCaption() const;

	// Socket list cleaner.
	void clear();

	// Socket list accessor.
	QList<qjackctlSocketItem *>& sockets();

	// Find the current selected socket item in list.
	qjackctlSocketItem *selectedSocketItem() const;

	// Retrieve a context pixmap.
	const QPixmap& pixmap(int iPixmap) const;

public slots:

	// Socket item interactivity methods.
	bool addSocketItem();
	bool removeSocketItem();
	bool editSocketItem();
	bool copySocketItem();
	bool exclusiveSocketItem();
	bool moveUpSocketItem();
	bool moveDownSocketItem();

private:

	// Merge two pixmaps with union of respective masks.
	QPixmap *createPixmapMerge(const QPixmap& xpmDst, const QPixmap& xpmSrc);

	// Instance variables.
	qjackctlSocketListView *m_pListView;
	bool m_bReadable;
	QString  m_sSocketCaption;

	QPixmap *m_apPixmaps[QJACKCTL_XPM_PIXMAPS];

	QList<qjackctlSocketItem *> m_sockets;
};


//----------------------------------------------------------------------------
// qjackctlSocketListView -- Socket list view, supporting drag-n-drop.

class qjackctlSocketListView : public QTreeWidget
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlSocketListView(qjackctlPatchbayView *pPatchbayView, bool bReadable);
	// Default destructor.
	~qjackctlSocketListView();

	// Patchbay dirty flag accessors.
	void setDirty (bool bDirty);
	bool dirty() const;
	
	// Auto-open timer methods.
	void setAutoOpenTimeout(int iAutoOpenTimeout);
	int autoOpenTimeout() const;

protected slots:

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
	qjackctlPatchbayView *m_pPatchbayView;

	bool m_bReadable;

	// Auto-open timer.
	int    m_iAutoOpenTimeout;
	QTimer *m_pAutoOpenTimer;

	// Items we'll eventually drop something.
	QTreeWidgetItem *m_pDragItem;
	QTreeWidgetItem *m_pDropItem;
	// The point from where drag started.
	QPoint m_posDrag;
};


//----------------------------------------------------------------------------
// qjackctlPatchworkView -- Socket plugging connector widget.

class qjackctlPatchworkView : public QWidget
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlPatchworkView(qjackctlPatchbayView *pPatchbayView);
	// Default destructor.
	~qjackctlPatchworkView();

protected slots:

	// Useful slots (should this be protected?).
	void contentsChanged();

protected:

	// Draw visible port connection relation arrows.
	void paintEvent(QPaintEvent *);

	// Context menu request event handler.
	void contextMenuEvent(QContextMenuEvent *);

private:

	// Legal socket item position helper.
	int itemY(QTreeWidgetItem *pItem) const;

	// Drawing methods.
	void drawForwardLine(QPainter *pPainter,
		int x, int dx, int y1, int y2, int h);
	void drawConnectionLine(QPainter *pPainter,
		int x1, int y1, int x2, int y2, int h1, int h2);

	// Local instance variables.
	qjackctlPatchbayView *m_pPatchbayView;
};


//----------------------------------------------------------------------------
// qjackctlPatchbayView -- Patchbay integrated widget.

class qjackctlPatchbayView : public QSplitter
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlPatchbayView(QWidget *pParent = 0);
	// Default destructor.
	~qjackctlPatchbayView();

	// Widget accesors.
	qjackctlSocketListView *OListView()     { return m_pOListView; }
	qjackctlSocketListView *IListView()     { return m_pIListView; }
	qjackctlPatchworkView  *PatchworkView() { return m_pPatchworkView; }

	// Patchbay object binding methods.
	void setBinding(qjackctlPatchbay *pPatchbay);
	qjackctlPatchbay *binding() const;

	// Socket list accessors.
	qjackctlSocketList *OSocketList() const;
	qjackctlSocketList *ISocketList() const;

	// Patchwork line style accessors.
	void setBezierLines(bool bBezierLines);
	bool isBezierLines() const;

	// Patchbay dirty flag accessors.
	void setDirty (bool bDirty);
	bool dirty() const;

signals:

	// Contents change signal.
	void contentsChanged();

public slots:

	// Common context menu slots.
	void contextMenu(const QPoint& pos, qjackctlSocketList *pSocketList);
	void activateForwardMenu(QAction *);

private:

	// Child controls.
	qjackctlSocketListView *m_pOListView;
	qjackctlSocketListView *m_pIListView;
	qjackctlPatchworkView  *m_pPatchworkView;

	// The main binding object.
	qjackctlPatchbay *m_pPatchbay;

	// How we'll draw patchwork lines.
	bool m_bBezierLines;

	// The obnoxious dirty flag.
	bool m_bDirty;
};


//----------------------------------------------------------------------------
// qjackctlPatchbay -- Patchbay integrated object.

class qjackctlPatchbay : public QObject
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlPatchbay(qjackctlPatchbayView *pPatchbayView);
	// Default destructor.
	~qjackctlPatchbay();

	// Explicit connection tests.
	bool canConnectSelected();
	bool canDisconnectSelected();
	bool canDisconnectAll();

	// Socket list accessors.
	qjackctlSocketList *OSocketList() const;
	qjackctlSocketList *ISocketList() const;

	// External rack transfer methods.
	void loadRack(qjackctlPatchbayRack *pPatchbayRack);
	void saveRack(qjackctlPatchbayRack *pPatchbayRack);

public slots:

	// Complete contents refreshner.
	void refresh();

	// Explicit connection slots.
	bool connectSelected();
	bool disconnectSelected();
	bool disconnectAll();

	// Expand all socket items.
	void expandAll();

	// Complete patchbay clearer.
	void clear();

	// Do actual and complete connections snapshot.
	void connectionsSnapshot();

private:

	// Internal rack transfer methods.
	void loadRackSockets (qjackctlSocketList *pSocketList,
		QList<qjackctlPatchbaySocket *>& socketlist);
	void saveRackSockets (qjackctlSocketList *pSocketList,
		QList<qjackctlPatchbaySocket *>& socketlist);

	// Connect/Disconnection primitives.
	void connectSockets(qjackctlSocketItem *pOSocket,
		qjackctlSocketItem *pISocket);
	void disconnectSockets(qjackctlSocketItem *pOSocket,
		qjackctlSocketItem *pISocket);

	// Instance variables.
	qjackctlPatchbayView *m_pPatchbayView;
	qjackctlSocketList *m_pOSocketList;
	qjackctlSocketList *m_pISocketList;
};


#endif  // __qjackctlPatchbay_h

// end of qjackctlPatchbay.h
