// qjackctlGraph.h
//
/****************************************************************************
   Copyright (C) 2003-2025, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctlGraph_h
#define __qjackctlGraph_h

#include <QGraphicsView>

#include <QGraphicsPathItem>

#include <QFrame>

#include <QColor>
#include <QIcon>

#include <QList>
#include <QHash>

class QGestureEvent;
class QPinchGesture;


// Forward decls.
class qjackctlGraphCanvas;
class qjackctlGraphNode;
class qjackctlGraphPort;
class qjackctlGraphConnect;
class qjackctlGraphCommand;
class qjackctlGraphMoveCommand;

class qjackctlAliases;
class qjackctlAliasList;

class QStyleOptionGraphicsItem;

class QRubberBand;
class QUndoCommand;
class QUndoStack;
class QSettings;

class QGraphicsProxyWidget;
class QLineEdit;

class QMouseEvent;
class QWheelEvent;
class QKeyEvent;


//----------------------------------------------------------------------------
// qjackctlGraphItem -- Base graphics item.

class qjackctlGraphItem : public QGraphicsPathItem
{
public:

	// Constructor.
	qjackctlGraphItem(QGraphicsItem *parent = nullptr);

	// Basic color accessors.
	void setForeground(const QColor& color);
	const QColor& foreground() const;

	void setBackground(const QColor& color);
	const QColor& background() const;

	// Marking methods.
	void setMarked(bool marked);
	bool isMarked() const;

	// Highlighting methods.
	void setHighlight(bool hilite);
	bool isHighlight() const;

	// Raise item z-value (dynamic always-on-top).
	void raise();

	// Item modes.
	enum Mode { None = 0,
		Input = 1, Output = 2,
		Duplex = Input | Output };

	// Item hash/map key.
	class ItemKey
	{
	public:

		// Constructors.
		ItemKey (const QString& name, Mode mode, uint type = 0)
			: m_name(name), m_mode(mode), m_type(type) {}
		ItemKey (const ItemKey& key)
			: m_name(key.name()), m_mode(key.mode()), m_type(key.type()) {}

		// Key accessors.
		const QString& name() const
			{ return m_name; }
		Mode mode() const
			{ return m_mode; }
		uint type() const
			{ return m_type; }

		// Hash/map key comparators.
		bool operator== (const ItemKey& key) const
		{
			return ItemKey::type() == key.type()
				&& ItemKey::mode() == key.mode()
				&& ItemKey::name() == key.name();
		}

	private:

		// Key fields.
		QString m_name;
		Mode    m_mode;
		uint    m_type;
	};

	typedef QHash<ItemKey, qjackctlGraphItem *> ItemKeys;

	// Item-type hash (static)
	static uint itemType(const QByteArray& type_name);

	// Rectangular editor extents.
	virtual QRectF editorRect() const;

	// Path and bounding rectangle override.
	void setPath(const QPainterPath& path);

	// Bounding rectangle accessor.
	const QRectF& itemRect() const;

private:

	// Instance variables.
	QColor m_foreground;
	QColor m_background;

	bool m_marked;
	bool m_hilite;

	QRectF m_rect;
};


// Item hash function.
inline uint qHash ( const qjackctlGraphItem::ItemKey& key )
{
	return qHash(key.name()) ^ qHash(uint(key.mode())) ^ qHash(key.type());
}


//----------------------------------------------------------------------------
// qjackctlGraphPort -- Port graphics item.

class qjackctlGraphPort : public qjackctlGraphItem
{
public:

	// Constructor.
	qjackctlGraphPort(qjackctlGraphNode *node,
		const QString& name, Mode mode, uint type = 0);

	// Destructor.
	~qjackctlGraphPort();

	// Graphics item type.
	enum { Type = QGraphicsItem::UserType + 2 };

	int type() const { return Type; }

	// Accessors.
	qjackctlGraphNode *portNode() const;

	void setPortName(const QString& name);
	const QString& portName() const;

	void setPortMode(Mode mode);
	Mode portMode() const;

	bool isInput() const;
	bool isOutput() const;

	void setPortType(uint type);
	uint portType() const;

	void setPortTitle(const QString& title);
	const QString& portTitle() const;

	void setPortIndex(int index);
	int portIndex() const;

	QPointF portPos() const;

	// Connection-list methods.
	void appendConnect(qjackctlGraphConnect *connect);
	void removeConnect(qjackctlGraphConnect *connect);
	void removeConnects();

	qjackctlGraphConnect *findConnect(qjackctlGraphPort *port) const;

	// Connect-list accessor.
	const QList<qjackctlGraphConnect *>& connects() const;

	// Selection propagation method...
	void setSelectedEx(bool is_selected);

	// Highlighting propagation method...
	void setHighlightEx(bool is_highlight);

	// Special port-type color business.
	void updatePortTypeColors(qjackctlGraphCanvas *canvas);

	// Port hash/map key.
	class PortKey : public ItemKey
	{
	public:
		// Constructors.
		PortKey(qjackctlGraphPort *port)
			: ItemKey(port->portName(), port->portMode(), port->portType()) {}
	};

	// Port sorting type.
	enum SortType { PortName = 0, PortTitle, PortIndex };

	static void setSortType(SortType sort_type);
	static SortType sortType();

	// Port sorting order.
	enum SortOrder { Ascending = 0, Descending };

	static void setSortOrder(SortOrder sort_order);
	static SortOrder sortOrder();

	// Port sorting comparators.
	struct Compare {
		bool operator()(qjackctlGraphPort *port1, qjackctlGraphPort *port2) const
			{ return qjackctlGraphPort::lessThan(port1, port2); }
	};

	struct ComparePos {
		bool operator()(qjackctlGraphPort *port1, qjackctlGraphPort *port2) const
			{ return (port1->scenePos().y() < port2->scenePos().y()); }
	};

	// Rectangular editor extents.
	QRectF editorRect() const;

	// Connector curve draw style (through vs. around nodes)
	static void setConnectThroughNodes(bool on);
	static bool isConnectThroughNodes();

protected:

	void paint(QPainter *painter,
		const QStyleOptionGraphicsItem *option, QWidget *widget);

	QVariant itemChange(GraphicsItemChange change, const QVariant& value);

	// Natural decimal sorting comparators.
	static bool lessThan(qjackctlGraphPort *port1, qjackctlGraphPort *port2);
	static bool lessThan(const QString& s1, const QString& s2);

private:

	// instance variables.
	qjackctlGraphNode *m_node;

	QString m_name;
	Mode    m_mode;
	uint    m_type;

	QString m_title;
	int     m_index;

	QGraphicsTextItem *m_text;

	QList<qjackctlGraphConnect *> m_connects;

	int m_selectx;
	int m_hilitex;

	static SortType  g_sort_type;
	static SortOrder g_sort_order;
};


//----------------------------------------------------------------------------
// qjackctlGraphNode -- Node graphics item.

class qjackctlGraphNode : public qjackctlGraphItem
{
public:

	// Constructor.
	qjackctlGraphNode(const QString& name, Mode mode, uint type = 0);

	// Destructor..
	~qjackctlGraphNode();
	// Graphics item type.
	enum { Type = QGraphicsItem::UserType + 1 };

	int type() const { return Type; }

	// Accessors.
	void setNodeName(const QString& name);
	const QString& nodeName() const;

	void setNodeMode(Mode mode);
	Mode nodeMode() const;

	void setNodeType(uint type);
	uint nodeType() const;

	void setNodeIcon(const QIcon& icon);
	const QIcon& nodeIcon() const;

	void setNodeTitle(const QString& title);
	QString nodeTitle() const;

	// Port-list methods.
	qjackctlGraphPort *addPort(const QString& name, Mode mode, int type = 0);

	qjackctlGraphPort *addInputPort(const QString& name, int type = 0);
	qjackctlGraphPort *addOutputPort(const QString& name, int type = 0);

	void removePort(qjackctlGraphPort *port);
	void removePorts();

	// Port finder (by name, mode and type)
	qjackctlGraphPort *findPort(const QString& name, Mode mode, uint type = 0);

	// Port-list accessor.
	const QList<qjackctlGraphPort *>& ports() const;

	// Reset port markings, destroy if unmarked.
	void resetMarkedPorts();

	// Path/shape updater.
	void updatePath();

	// Node hash key.
	class NodeKey : public ItemKey
	{
	public:
		// Constructors.
		NodeKey(qjackctlGraphNode *node)
			: ItemKey(node->nodeName(), node->nodeMode(), node->nodeType()) {}
	};

	// Rectangular editor extents.
	QRectF editorRect() const;

protected:

	void paint(QPainter *painter,
		const QStyleOptionGraphicsItem *option, QWidget *widget);

	QVariant itemChange(GraphicsItemChange change, const QVariant& value);

private:

	// Instance variables.
	QString m_name;
	Mode    m_mode;
	uint    m_type;

	QIcon   m_icon;

	QString m_title;

	QGraphicsPixmapItem *m_pixmap;
	QGraphicsTextItem   *m_text;

	qjackctlGraphPort::ItemKeys m_portkeys;
	QList<qjackctlGraphPort *>  m_ports;
};


//----------------------------------------------------------------------------
// qjackctlGraphConnect -- Connection-line graphics item.

class qjackctlGraphConnect : public qjackctlGraphItem
{
public:

	// Constructor.
	qjackctlGraphConnect();

	// Destructor..
	~qjackctlGraphConnect();

	// Graphics item type.
	enum { Type = QGraphicsItem::UserType + 3 };

	int type() const { return Type; }

	// Accessors.
	void setPort1(qjackctlGraphPort *port);
	qjackctlGraphPort *port1() const;

	void setPort2(qjackctlGraphPort *port);
	qjackctlGraphPort *port2() const;

	// Active disconnection.
	void disconnect();

	// Path/shaper updaters.
	void updatePathTo(const QPointF& pos);
	void updatePath();

	// Selection propagation method...
	void setSelectedEx(qjackctlGraphPort *port, bool is_selected);

	// Highlighting propagation method...
	void setHighlightEx(qjackctlGraphPort *port, bool is_highlight);

	// Special port-type color business.
	void updatePortTypeColors();

	// Connector curve draw style (through vs. around nodes)
	static void setConnectThroughNodes(bool on);
	static bool isConnectThroughNodes();

protected:

	void paint(QPainter *painter,
		const QStyleOptionGraphicsItem *option, QWidget *widget);

	QVariant itemChange(GraphicsItemChange change, const QVariant& value);

	QPainterPath shape() const;

private:

	// Instance variables.
	qjackctlGraphPort *m_port1;
	qjackctlGraphPort *m_port2;

	// Connector curve draw style (through vs. around nodes)
	static bool g_connect_through_nodes;
};


//----------------------------------------------------------------------------
// qjackctlGraphCanvas -- Canvas graphics scene/view.

class qjackctlGraphCanvas : public QGraphicsView
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlGraphCanvas(QWidget *parent = nullptr);

	// Destructor.
	~qjackctlGraphCanvas();

	// Accessors.
	QGraphicsScene *scene() const;
	QUndoStack *commands() const;

	void setSettings(QSettings *settings);
	QSettings *settings() const;

	// Canvas methods.
	void addItem(qjackctlGraphItem *item);
	void removeItem(qjackctlGraphItem *item);

	// Current item accessor.
	qjackctlGraphItem *currentItem() const;

	// Connection predicates.
	bool canConnect() const;
	bool canDisconnect() const;

	// Edit predicates.
	bool canRenameItem() const;
	bool canSearchItem() const;

	// Zooming methods.
	void setZoom(qreal zoom);
	qreal zoom() const;

	void setZoomRange(bool zoomrange);
	bool isZoomRange() const;

	// Clean-up all un-marked nodes...
	void resetNodes(uint node_type);
	void clearNodes(uint node_type);

	// Special node finder.
	qjackctlGraphNode *findNode(
		const QString& name, qjackctlGraphItem::Mode mode, uint type = 0) const;

	// Whether it's in the middle of something...
	bool isBusy() const;

	// Port (dis)connections notifiers.
	void emitConnected(qjackctlGraphPort *port1, qjackctlGraphPort *port2);
	void emitDisconnected(qjackctlGraphPort *port1, qjackctlGraphPort *port2);

	// Rename notifier.
	void emitRenamed(qjackctlGraphItem *item, const QString& name);

	// Other generic notifier.
	void emitChanged();

	// Graph canvas state methods.
	bool restoreState();
	bool saveState() const;

	// Graph node/port state methods.
	bool restoreNode(qjackctlGraphNode *node);
	bool saveNode(qjackctlGraphNode *node) const;

	// Repel overlapping nodes...
	void setRepelOverlappingNodes(bool on);
	bool isRepelOverlappingNodes() const;
	void repelOverlappingNodes(qjackctlGraphNode *node,
		qjackctlGraphMoveCommand *move_command = nullptr,
		const QPointF& delta = QPointF());
	void repelOverlappingNodesAll(
		qjackctlGraphMoveCommand *move_command = nullptr);

	// Graph colors management.
	void setPortTypeColor(uint port_type, const QColor& color);
	const QColor& portTypeColor(uint port_type);
	void updatePortTypeColors(uint port_type = 0);
	void clearPortTypeColors();

	// Clear all selection.
	void clearSelection();

	// Clear all state.
	void clear();

	// Client/port aliases accessors.
	void setAliases(qjackctlAliases *aliases);
	qjackctlAliases *aliases() const;

	// Snap into position helper.
	QPointF snapPos(const QPointF& pos) const;

	// Search placeholder text accessors.
	void setSearchPlaceholderText(const QString& text);
	QString searchPlaceholderText() const;

	// Update the canvas palette.
	void updatePalette();

signals:

	// Node factory notifications.
	void added(qjackctlGraphNode *node);
	void updated(qjackctlGraphNode *node);
	void removed(qjackctlGraphNode *node);

	// Port (dis)connection notifications.
	void connected(qjackctlGraphPort *port1, qjackctlGraphPort *port2);
	void disconnected(qjackctlGraphPort *port1, qjackctlGraphPort *port2);

	void connected(qjackctlGraphConnect *connect);

	// Generic change notification.
	void changed();

	// Rename notification.
	void renamed(qjackctlGraphItem *item, const QString& name);

public slots:

	// Dis/connect selected items.
	void connectItems();
	void disconnectItems();

	// Select actions.
	void selectAll();
	void selectNone();
	void selectInvert();

	// Edit actions.
	void renameItem();
	void searchItem();

	// Discrete zooming actions.
	void zoomIn();
	void zoomOut();
	void zoomFit();
	void zoomReset();

	// Update all nodes.
	void updateNodes();

	// Update all connectors.
	void updateConnects();

protected slots:

	// Rename item slots.
	void renameTextChanged(const QString&);
	void renameEditingFinished();

	// Search item slots.
	void searchTextChanged(const QString&);
	void searchEditingFinished();

protected:

	// Item finder (internal).
	qjackctlGraphItem *itemAt(const QPointF& pos) const;

	// Port (dis)connection commands.
	void connectPorts(
		qjackctlGraphPort *port1, qjackctlGraphPort *port2, bool is_connect);

	// Mouse event handlers.
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);

	void wheelEvent(QWheelEvent *event);

	// Keyboard event handler.
	void keyPressEvent(QKeyEvent *event);

	// Gesture event handlers.
	bool event(QEvent *event);
	bool gestureEvent(QGestureEvent *event);
	void pinchGesture(QPinchGesture *pinch);

	// Graph node key helper.
	QString nodeKey(qjackctlGraphNode *node) const;

	// Zoom in rectangle range.
	void zoomFitRange(const QRectF& range_rect);

	// Update editors position and size.
	void updateRenameEditor();
	void updateSearchEditor();

	// Bounding margins/limits...
	const QRectF& boundingRect(bool reset = false);
	void boundingPos(QPointF& pos);

	// Start search editor...
	void startSearchEditor(const QString& text = QString());

	void resizeEvent(QResizeEvent *event) override;

private:

	// Mouse pointer dragging states.
	enum DragState { DragNone = 0, DragStart, DragMove, DragScroll };

	// Instance variables.
	QGraphicsScene       *m_scene;
	DragState             m_state;
	QPointF               m_pos;
	qjackctlGraphItem    *m_item;
	qjackctlGraphConnect *m_connect;
	qjackctlGraphPort    *m_port2;
	QRubberBand          *m_rubberband;
	qreal                 m_zoom;
	bool                  m_zoomrange;
	bool                  m_gesture;

	qjackctlGraphNode::ItemKeys m_nodekeys;
	QList<qjackctlGraphNode *>  m_nodes;

	QUndoStack *m_commands;
	QSettings  *m_settings;

	QList<QGraphicsItem *> m_selected;
	int m_selected_nodes;

	bool m_repel_overlapping_nodes;

	// Graph port colors.
	QHash<uint, QColor> m_port_colors;

	// Item renaming stuff.
	qjackctlGraphItem *m_rename_item;
	QLineEdit         *m_rename_editor;
	int                m_renamed;

	// Original node position (for move command).
	QPointF m_pos1;

	// Allowed auto-scroll margins/limits (for move command).
	QRectF m_rect1;

	// Item search stuff.
	QLineEdit *m_search_editor;

	// Client/port aliases database.
	qjackctlAliases *m_aliases;
};


//----------------------------------------------------------------------------
// qjackctlGraphSect -- Generic graph driver

class qjackctlGraphSect
{
public:

	// Constructor.
	qjackctlGraphSect(qjackctlGraphCanvas *canvas);

	// Destructor (virtual)
	virtual ~qjackctlGraphSect() {}

	// Accessors.
	qjackctlGraphCanvas *canvas() const;

	// Generic sect/graph methods.
	void addItem(qjackctlGraphItem *item, bool is_new = true);
	void removeItem(qjackctlGraphItem *item);

	// Clean-up all un-marked items...
	void resetItems(uint node_type);
	void clearItems(uint node_type);

	// Special node finder.
	qjackctlGraphNode *findNode(
		const QString& name, qjackctlGraphItem::Mode mode, int type = 0) const;

	// Client/port renaming method.
	virtual void renameItem(qjackctlGraphItem *item, const QString& name);

protected:

	// Client/port item aliases accessor.
	virtual QList<qjackctlAliasList *> item_aliases(
		qjackctlGraphItem *item) const = 0;

private:

	// Instance variables.
	qjackctlGraphCanvas *m_canvas;

	QList<qjackctlGraphConnect *> m_connects;
};


//----------------------------------------------------------------------------
// qjackctlGraphThumb -- Thumb graphics scene/view.

class qjackctlGraphThumb : public QFrame
{
	Q_OBJECT

public:

	// Corner position.
	enum Position { None = 0, TopLeft, TopRight, BottomLeft, BottomRight };

	// Constructor.
	qjackctlGraphThumb(qjackctlGraphCanvas *canvas, Position position = BottomLeft);

	// Destructor.
	~qjackctlGraphThumb();

	// Accessors.
	qjackctlGraphCanvas *canvas() const;

	void setPosition(Position position);
	Position position() const;

	// Emit context-menu request.
	void requestContextMenu(const QPoint& pos);

	// Request re-positioning.
	void requestPosition(Position position);

	// Update the thumb-view palette.
	void updatePalette();

signals:

	// Context-menu request.
	void contextMenuRequested(const QPoint& pos);

	// Re-positioning request.
	void positionRequested(int position);

public slots:

	// Update view slot.
	void updateView();

protected:

	// Update position.
	void updatePosition();

	// Forward decl.
	class View;

private:

	// Inatance members.
	qjackctlGraphCanvas *m_canvas;
	Position m_position;
	View *m_view;
};


#endif  // __qjackctlGraph_h

// end of qjackctlGraph.h
