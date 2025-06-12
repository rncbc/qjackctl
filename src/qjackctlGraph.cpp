// qjackctlGraph.cpp
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

#include "qjackctlAbout.h"
#include "qjackctlGraph.h"

#include "qjackctlGraphCommand.h"

#include "qjackctlAliases.h"

#include <QGraphicsScene>

#include <QGraphicsDropShadowEffect>

#include <QStyleOptionGraphicsItem>

#include <QTransform>
#include <QRubberBand>
#include <QUndoStack>
#include <QSettings>

#include <QPainter>
#include <QPalette>

#include <QLinearGradient>

#include <QGraphicsProxyWidget>
#include <QLineEdit>
#include <QScrollBar>

#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

#include <QGestureEvent>
#include <QPinchGesture>

#include <QVBoxLayout>
#include <QApplication>

#include <algorithm>

#include <cmath>


//----------------------------------------------------------------------------
// qjackctlGraphItem -- Base graphics item.

// Constructor.
qjackctlGraphItem::qjackctlGraphItem ( QGraphicsItem *parent )
	: QGraphicsPathItem(parent), m_marked(false), m_hilite(false)
{
	const QPalette pal;
	m_foreground = pal.buttonText().color();
	m_background = pal.button().color();
}


// Basic color accessors.
void qjackctlGraphItem::setForeground ( const QColor& color )
{
	m_foreground = color;
}

const QColor& qjackctlGraphItem::foreground (void) const
{
	return m_foreground;
}


void qjackctlGraphItem::setBackground ( const QColor& color )
{
	m_background = color;
}

const QColor& qjackctlGraphItem::background (void) const
{
	return m_background;
}


// Marking methods.
void qjackctlGraphItem::setMarked ( bool marked )
{
	m_marked = marked;
}


bool qjackctlGraphItem::isMarked (void) const
{
	return m_marked;
}


// Highlighting methods.
void qjackctlGraphItem::setHighlight ( bool hilite )
{
	m_hilite = hilite;

	QGraphicsPathItem::update();
}


bool qjackctlGraphItem::isHighlight (void) const
{
	return m_hilite;
}


// Raise item z-value (dynamic always-on-top).
void qjackctlGraphItem::raise (void)
{
	static qreal s_zvalue = 0.0;

	switch (type()) {
	case  qjackctlGraphPort::Type: {
		QGraphicsPathItem::setZValue(s_zvalue += 0.003);
		qjackctlGraphPort *port = static_cast<qjackctlGraphPort *> (this);
		if (port) {
			qjackctlGraphNode *node = port->portNode();
			if (node)
				node->setZValue(s_zvalue += 0.002);
		}
		break;
	}
	case qjackctlGraphConnect::Type:
	default:
		QGraphicsPathItem::setZValue(s_zvalue += 0.001);
		break;
	}
}


// Item-type hash (static)
uint qjackctlGraphItem::itemType ( const QByteArray& type_name )
{
	return qHash(type_name);
}


// Rectangular editor extents (virtual)
QRectF qjackctlGraphItem::editorRect (void) const
{
	return QRectF();
}


// Path and bounding rectangle override.
void qjackctlGraphItem::setPath ( const QPainterPath& path )
{
	m_rect = path.controlPointRect();

	QGraphicsPathItem::setPath(path);
}


// Bounding rectangle accessor.
const QRectF& qjackctlGraphItem::itemRect (void) const
{
	return m_rect;
}


//----------------------------------------------------------------------------
// qjackctlGraphPort -- Port graphics item.

// Constructor.
qjackctlGraphPort::qjackctlGraphPort ( qjackctlGraphNode *node,
	const QString& name, qjackctlGraphItem::Mode mode, uint type )
	: qjackctlGraphItem(node), m_node(node),
		m_name(name), m_mode(mode), m_type(type),
		m_index(node->ports().count()),
		m_selectx(0), m_hilitex(0)
{
	QGraphicsPathItem::setZValue(+1.0);

	const QPalette pal;
	setForeground(pal.buttonText().color());
	setBackground(pal.button().color());

	m_text = new QGraphicsTextItem(this);

	QGraphicsPathItem::setFlag(QGraphicsItem::ItemIsSelectable);
	QGraphicsPathItem::setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

	QGraphicsPathItem::setAcceptHoverEvents(true);

	QGraphicsPathItem::setToolTip(m_name);

	setPortTitle(m_name);
}


// Destructor.
qjackctlGraphPort::~qjackctlGraphPort (void)
{
	removeConnects();

	// No actual need to destroy any children here...
	//
	//delete m_text;
}


// Accessors.
qjackctlGraphNode *qjackctlGraphPort::portNode (void) const
{
	return m_node;
}


void qjackctlGraphPort::setPortName ( const QString& name )
{
	m_name = name;

	QGraphicsPathItem::setToolTip(m_name);
}


const QString& qjackctlGraphPort::portName (void) const
{
	return m_name;
}


void qjackctlGraphPort::setPortMode ( qjackctlGraphItem::Mode mode )
{
	m_mode = mode;
}


qjackctlGraphItem::Mode qjackctlGraphPort::portMode (void) const
{
	return m_mode;
}


bool qjackctlGraphPort::isInput (void) const
{
	return (m_mode & Input);
}


bool qjackctlGraphPort::isOutput (void) const
{
	return (m_mode & Output);
}


void qjackctlGraphPort::setPortType ( uint type )
{
	m_type = type;
}


uint qjackctlGraphPort::portType (void) const
{
	return m_type;
}


void qjackctlGraphPort::setPortTitle ( const QString& title )
{
	m_title = (title.isEmpty() ? m_name : title);

	static const int MAX_TITLE_LENGTH = 29;
	static const QString ellipsis(3, '.');

	QString text = m_title.simplified();
	if (text.length() >= MAX_TITLE_LENGTH + ellipsis.length())
		text = ellipsis + text.right(MAX_TITLE_LENGTH).trimmed();

	m_text->setPlainText(text);

	QPainterPath path;
	const QRectF& rect
		= m_text->boundingRect().adjusted(0, +2, 0, -2);
	path.addRoundedRect(rect, 5, 5);
	/*QGraphicsPathItem::*/setPath(path);
}


const QString& qjackctlGraphPort::portTitle (void) const
{
	return m_title;
}


void qjackctlGraphPort::setPortIndex ( int index )
{
	m_index = index;
}


int qjackctlGraphPort::portIndex (void) const
{
	return m_index;
}


QPointF qjackctlGraphPort::portPos (void) const
{
	QPointF pos = QGraphicsPathItem::scenePos();

	const QRectF& rect = itemRect();
	if (m_mode == Output)
		pos.setX(pos.x() + rect.width());
	pos.setY(pos.y() + rect.height() / 2);

	return pos;
}


// Connection-list methods.
void qjackctlGraphPort::appendConnect ( qjackctlGraphConnect *connect )
{
	m_connects.append(connect);
}


void qjackctlGraphPort::removeConnect ( qjackctlGraphConnect *connect )
{
	m_connects.removeAll(connect);
}


void qjackctlGraphPort::removeConnects (void)
{
	foreach (qjackctlGraphConnect *connect, m_connects) {
		if (connect->port1() != this)
			connect->setPort1(nullptr);
		if (connect->port2() != this)
			connect->setPort2(nullptr);
	}

	// Do not delete connects here as they are owned elsewhere...
	//
	//	qDeleteAll(m_connects);
	m_connects.clear();
}


qjackctlGraphConnect *qjackctlGraphPort::findConnect ( qjackctlGraphPort *port ) const
{
	foreach (qjackctlGraphConnect *connect, m_connects) {
		if (connect->port1() == port || connect->port2() == port)
			return connect;
	}

	return nullptr;
}


// Connect-list accessor.
const QList<qjackctlGraphConnect *>& qjackctlGraphPort::connects (void) const
{
	return m_connects;
}


void qjackctlGraphPort::paint ( QPainter *painter,
	const QStyleOptionGraphicsItem *option, QWidget */*widget*/ )
{
	const QPalette& pal = option->palette;

	const QRectF& port_rect = itemRect();
	QLinearGradient port_grad(0, port_rect.top(), 0, port_rect.bottom());
	QColor port_color;
	if (QGraphicsPathItem::isSelected()) {
		m_text->setDefaultTextColor(pal.highlightedText().color());
		painter->setPen(pal.highlightedText().color());
		port_color = pal.highlight().color();
	} else {
		const QColor& foreground
			= qjackctlGraphItem::foreground();
		const QColor& background
			= qjackctlGraphItem::background();
		const bool is_dark
			= (background.value() < 128);
		m_text->setDefaultTextColor(is_dark
			? foreground.lighter()
			: foreground.darker());
		if (qjackctlGraphItem::isHighlight()) {
			painter->setPen(foreground.lighter());
			port_color = background.lighter();
		} else {
			painter->setPen(foreground);
			port_color = background;
		}
	}
	port_grad.setColorAt(0.0, port_color);
	port_grad.setColorAt(1.0, port_color.darker(120));
	painter->setBrush(port_grad);

	painter->drawPath(QGraphicsPathItem::path());
}


QVariant qjackctlGraphPort::itemChange (
	GraphicsItemChange change, const QVariant& value )
{
	if (change == QGraphicsItem::ItemScenePositionHasChanged) {
		foreach (qjackctlGraphConnect *connect, m_connects) {
			connect->updatePath();
		}
	}
	else
	if (change == QGraphicsItem::ItemSelectedHasChanged && m_selectx < 1) {
		const bool is_selected = value.toBool();
		setHighlightEx(is_selected);
		foreach (qjackctlGraphConnect *connect, m_connects)
			connect->setSelectedEx(this, is_selected);
	}

	return value;
}


// Selection propagation method...
void qjackctlGraphPort::setSelectedEx ( bool is_selected )
{
	if (!is_selected) {
		foreach (qjackctlGraphConnect *connect, m_connects) {
			if (connect->isSelected()) {
				setHighlightEx(true);
				return;
			}
		}
	}

	++m_selectx;

	setHighlightEx(is_selected);

	if (QGraphicsPathItem::isSelected() != is_selected)
		QGraphicsPathItem::setSelected(is_selected);

	--m_selectx;
}


// Highlighting propagation method...
void qjackctlGraphPort::setHighlightEx ( bool is_highlight )
{
	if (m_hilitex > 0)
		return;

	++m_hilitex;

	qjackctlGraphItem::setHighlight(is_highlight);

	foreach (qjackctlGraphConnect *connect, m_connects)
		connect->setHighlightEx(this, is_highlight);

	--m_hilitex;
}


// Special port-type color business.
void qjackctlGraphPort::updatePortTypeColors ( qjackctlGraphCanvas *canvas )
{
	if (canvas) {
		const QColor& color = canvas->portTypeColor(m_type);
		if (color.isValid()) {
			const bool is_dark = (color.value() < 128);
			qjackctlGraphItem::setForeground(is_dark
				? color.lighter(180)
				: color.darker());
			qjackctlGraphItem::setBackground(color);
			if (m_mode & Output) {
				foreach (qjackctlGraphConnect *connect, m_connects) {
					connect->updatePortTypeColors();
					connect->update();
				}
			}
		}
	}
}



// Port sorting type.
qjackctlGraphPort::SortType  qjackctlGraphPort::g_sort_type  = qjackctlGraphPort::PortName;

void qjackctlGraphPort::setSortType ( SortType sort_type )
{
	g_sort_type = sort_type;
}

qjackctlGraphPort::SortType qjackctlGraphPort::sortType (void)
{
	return g_sort_type;
}


// Port sorting order.
qjackctlGraphPort::SortOrder qjackctlGraphPort::g_sort_order = qjackctlGraphPort::Ascending;

void qjackctlGraphPort::setSortOrder( SortOrder sort_order )
{
	g_sort_order = sort_order;
}

qjackctlGraphPort::SortOrder qjackctlGraphPort::sortOrder (void)
{
	return g_sort_order;
}


// Natural decimal sorting comparator (static)
bool qjackctlGraphPort::lessThan ( qjackctlGraphPort *port1, qjackctlGraphPort *port2 )
{
	const int port_type_diff
		= int(port1->portType()) - int(port2->portType());
	if (port_type_diff)
		return (port_type_diff > 0);

	if (g_sort_order == Descending) {
		qjackctlGraphPort *port = port1;
		port1 = port2;
		port2 = port;
	}

	if (g_sort_type == PortIndex) {
		const int port_index_diff
			= port1->portIndex() - port2->portIndex();
		if (port_index_diff)
			return (port_index_diff < 0);
	}

	switch (g_sort_type) {
	case PortTitle:
		return qjackctlGraphPort::lessThan(port1->portTitle(), port2->portTitle());
	case PortName:
	default:
		return qjackctlGraphPort::lessThan(port1->portName(), port2->portName());
	}
}

bool qjackctlGraphPort::lessThan ( const QString& s1, const QString& s2 )
{
	const int n1 = s1.length();
	const int n2 = s2.length();

	int i1, i2;

	for (i1 = i2 = 0; i1 < n1 && i2 < n2; ++i1, ++i2) {

		// Skip (white)spaces...
		while (s1.at(i1).isSpace())
			++i1;
		while (s2.at(i2).isSpace())
			++i2;

		// Normalize (to uppercase) the next characters...
		QChar c1 = s1.at(i1).toUpper();
		QChar c2 = s2.at(i2).toUpper();

		if (c1.isDigit() && c2.isDigit()) {
			// Find the whole length numbers...
			int j1 = i1++;
			while (i1 < n1 && s1.at(i1).isDigit())
				++i1;
			int j2 = i2++;
			while (i2 < n2 && s2.at(i2).isDigit())
				++i2;
			// Compare as natural decimal-numbers...
			j1 = s1.mid(j1, i1 - j1).toInt();
			j2 = s2.mid(j2, i2 - j2).toInt();
			if (j1 != j2)
				return (j1 < j2);
			// Never go out of bounds...
			if (i1 >= n1 || i2 >= n2)
				break;
			// Go on with this next char...
			c1 = s1.at(i1).toUpper();
			c2 = s2.at(i2).toUpper();
		}

		// Compare this char...
		if (c1 != c2)
			return (c1 < c2);
	}

	// Probable exact match.
	return false;
}


// Rectangular editor extents.
QRectF qjackctlGraphPort::editorRect (void) const
{
	return QGraphicsPathItem::sceneBoundingRect();
}


//----------------------------------------------------------------------------
// qjackctlGraphNode -- Node graphics item.

// Constructor.
qjackctlGraphNode::qjackctlGraphNode (
	const QString& name, qjackctlGraphItem::Mode mode, uint type )
	: qjackctlGraphItem(nullptr),
		m_name(name), m_mode(mode), m_type(type)
{
	QGraphicsPathItem::setZValue(0.0);

	const QPalette pal;
	const int base_value = pal.base().color().value();
	const bool is_dark = (base_value < 128);

	const QColor& text_color = pal.text().color();
	QColor foreground_color(is_dark
		? text_color.darker()
		: text_color);
	qjackctlGraphItem::setForeground(foreground_color);

	const QColor& window_color = pal.window().color();
	QColor background_color(is_dark
		? window_color.lighter()
		: window_color);
	background_color.setAlpha(160);
	qjackctlGraphItem::setBackground(background_color);

	m_pixmap = new QGraphicsPixmapItem(this);
	m_text = new QGraphicsTextItem(this);

	QGraphicsPathItem::setFlag(QGraphicsItem::ItemIsMovable);
	QGraphicsPathItem::setFlag(QGraphicsItem::ItemIsSelectable);

	QGraphicsPathItem::setToolTip(m_name);
	setNodeTitle(m_name);

	const bool is_darkest = (base_value < 24);
	QColor shadow_color = (is_darkest ? Qt::white : Qt::black);
	shadow_color.setAlpha(180);

	QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
	effect->setColor(shadow_color);
	effect->setBlurRadius(is_darkest ? 8 : 16);
	effect->setOffset(is_darkest ? 0 : 2);
	QGraphicsPathItem::setGraphicsEffect(effect);

	qjackctlGraphItem::raise();
}


// Destructor.
qjackctlGraphNode::~qjackctlGraphNode (void)
{
	removePorts();

	// No actual need to destroy any children here...
	//
	//QGraphicsPathItem::setGraphicsEffect(nullptr);

	//	delete m_text;
	//	delete m_pixmap;
}


// accessors.
void qjackctlGraphNode::setNodeName ( const QString& name )
{
	m_name = name;

	QGraphicsPathItem::setToolTip(m_name);
}


const QString& qjackctlGraphNode::nodeName (void) const
{
	return m_name;
}


void qjackctlGraphNode::setNodeMode ( qjackctlGraphItem::Mode mode )
{
	m_mode = mode;
}


qjackctlGraphItem::Mode qjackctlGraphNode::nodeMode (void) const
{
	return m_mode;
}


void qjackctlGraphNode::setNodeType ( uint type )
{
	m_type = type;
}


uint qjackctlGraphNode::nodeType (void) const
{
	return m_type;
}


void qjackctlGraphNode::setNodeIcon ( const QIcon& icon )
{
	m_icon = icon;

	m_pixmap->setPixmap(m_icon.pixmap(24, 24));
}


const QIcon& qjackctlGraphNode::nodeIcon (void) const
{
	return m_icon;
}


void qjackctlGraphNode::setNodeTitle ( const QString& title )
{
	const QFont& font = m_text->font();
	m_text->setFont(QFont(font.family(), font.pointSize(), QFont::Bold));
	m_title = (title.isEmpty() ? m_name : title);

	static const int MAX_TITLE_LENGTH = 29;
	static const QString ellipsis(3, '.');

	QString text = m_title.simplified();
	if (text.length() >= MAX_TITLE_LENGTH  + ellipsis.length())
		text = text.left(MAX_TITLE_LENGTH).trimmed() + ellipsis;

	m_text->setPlainText(text);
}


QString qjackctlGraphNode::nodeTitle (void) const
{
	return m_title;	// m_text->toPlainText();
}


// Port-list methods.
qjackctlGraphPort *qjackctlGraphNode::addPort (
	const QString& name, qjackctlGraphItem::Mode mode, int type )
{
	qjackctlGraphPort *port = new qjackctlGraphPort(this, name, mode, type);

	m_ports.append(port);
	m_portkeys.insert(qjackctlGraphPort::PortKey(port), port);

	updatePath();

	return port;
}


qjackctlGraphPort *qjackctlGraphNode::addInputPort ( const QString& name, int type )
{
	return addPort(name, qjackctlGraphItem::Input, type);
}


qjackctlGraphPort *qjackctlGraphNode::addOutputPort ( const QString& name, int type )
{
	return addPort(name, qjackctlGraphItem::Output, type);
}


void qjackctlGraphNode::removePort ( qjackctlGraphPort *port )
{
	m_portkeys.remove(qjackctlGraphPort::PortKey(port));
	m_ports.removeAll(port);

	updatePath();
}


void qjackctlGraphNode::removePorts (void)
{
	foreach (qjackctlGraphPort *port, m_ports)
		port->removeConnects();

	// Do not delete ports here as they are node's child items...
	//
	//qDeleteAll(m_ports);
	m_ports.clear();
	m_portkeys.clear();
}


// Port finder (by name, mode and type)
qjackctlGraphPort *qjackctlGraphNode::findPort (
	const QString& name, qjackctlGraphItem::Mode mode, uint type )
{
	return static_cast<qjackctlGraphPort *> (
		m_portkeys.value(qjackctlGraphPort::ItemKey(name, mode, type), nullptr));
}


// Port-list accessor.
const QList<qjackctlGraphPort *>& qjackctlGraphNode::ports (void) const
{
	return m_ports;
}


// Reset port markings, destroy if unmarked.
void qjackctlGraphNode::resetMarkedPorts (void)
{
	QList<qjackctlGraphPort *> ports;

	foreach (qjackctlGraphPort *port, m_ports) {
		if (port->isMarked()) {
			port->setMarked(false);
		} else {
			ports.append(port);
		}
	}

	foreach (qjackctlGraphPort *port, ports) {
		port->removeConnects();
		removePort(port);
		delete port;
	}
}


// Path/shape updater.
void qjackctlGraphNode::updatePath (void)
{
	const QRectF& rect = m_text->boundingRect();
	int width = rect.width() / 2 + 24;
	int wi, wo;
	wi = wo = width;
	foreach (qjackctlGraphPort *port, m_ports) {
		const int w = port->itemRect().width();
		if (port->isOutput()) {
			if (wo < w) wo = w;
		} else {
			if (wi < w) wi = w;
		}
	}
	width = 4 * ((wi + wo) / 4);

	std::sort(m_ports.begin(), m_ports.end(), qjackctlGraphPort::Compare());

	int height = rect.height() + 2;
	int type = 0;
	int yi, yo;
	yi = yo = height;
	foreach (qjackctlGraphPort *port, m_ports) {
		const QRectF& port_rect = port->itemRect();
		const int w = port_rect.width();
		const int h = port_rect.height() + 1;
		if (type - port->portType()) {
			type = port->portType();
			height += 2;
			yi = yo = height;
		}
		if (port->isOutput()) {
			port->setPos(width + 6 - w, yo);
			yo += h;
			if (height < yo)
				height = yo;
		} else {
			port->setPos(-6, yi);
			yi += h;
			if (height < yi)
				height = yi;
		}
	}

	QPainterPath path;
	path.addRoundedRect(0, 0, width, height + 6, 5, 5);
	/*QGraphicsPathItem::*/setPath(path);
}


void qjackctlGraphNode::paint ( QPainter *painter,
	const QStyleOptionGraphicsItem *option, QWidget */*widget*/ )
{
	const QPalette& pal = option->palette;

	const QRectF& node_rect = itemRect();
	QLinearGradient node_grad(0, node_rect.top(), 0, node_rect.bottom());
	QColor node_color;
	if (QGraphicsPathItem::isSelected()) {
		const QColor& hilitetext_color = pal.highlightedText().color();
		m_text->setDefaultTextColor(hilitetext_color);
		painter->setPen(hilitetext_color);
		node_color = pal.highlight().color();
	} else {
		const QColor& foreground
			= qjackctlGraphItem::foreground();
		const QColor& background
			= qjackctlGraphItem::background();
		const bool is_dark
			= (background.value() < 192);
		m_text->setDefaultTextColor(is_dark
			? foreground.lighter()
			: foreground.darker());
		painter->setPen(foreground);
		node_color = background;
	}
	node_color.setAlpha(180);
	node_grad.setColorAt(0.6, node_color);
	node_grad.setColorAt(1.0, node_color.darker(120));
	painter->setBrush(node_grad);

	painter->drawPath(QGraphicsPathItem::path());

	m_pixmap->setPos(node_rect.x() + 4, node_rect.y() + 4);

	const QRectF& text_rect = m_text->boundingRect();
	const qreal w2 = (node_rect.width() - text_rect.width()) / 2;
	m_text->setPos(node_rect.x() + w2 + 4, node_rect.y() + 2);
}


QVariant qjackctlGraphNode::itemChange (
	GraphicsItemChange change, const QVariant& value )
{
	if (change == QGraphicsItem::ItemSelectedHasChanged) {
		const bool is_selected = value.toBool();
		foreach (qjackctlGraphPort *port, m_ports)
			port->setSelected(is_selected);
	}

	return value;
}


// Rectangular editor extents.
QRectF qjackctlGraphNode::editorRect (void) const
{
	return m_text->sceneBoundingRect();
}


//----------------------------------------------------------------------------
// qjackctlGraphConnect -- Connection-line graphics item.

// Constructor.
qjackctlGraphConnect::qjackctlGraphConnect (void)
	: qjackctlGraphItem(nullptr), m_port1(nullptr), m_port2(nullptr)
{
	QGraphicsPathItem::setZValue(-1.0);

	QGraphicsPathItem::setFlag(QGraphicsItem::ItemIsSelectable);

	qjackctlGraphItem::setBackground(qjackctlGraphItem::foreground());

#if 0//Disable drop-shadow effect...
	const QPalette pal;
	const bool is_darkest = (pal.base().color().value() < 24);
	QColor shadow_color = (is_darkest ? Qt::white : Qt::black);
	shadow_color.setAlpha(220);
	QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
	effect->setColor(shadow_color);
	effect->setBlurRadius(is_darkest ? 4 : 8);
	effect->setOffset(is_darkest ? 0 : 1);
	QGraphicsPathItem::setGraphicsEffect(effect);
#endif
	QGraphicsPathItem::setAcceptHoverEvents(true);

	qjackctlGraphItem::raise();
}


// Destructor.
qjackctlGraphConnect::~qjackctlGraphConnect (void)
{
	// No actual need to destroy any children here...
	//
	//QGraphicsPathItem::setGraphicsEffect(nullptr);
}


// Accessors.
void qjackctlGraphConnect::setPort1 ( qjackctlGraphPort *port )
{
	if (m_port1)
		m_port1->removeConnect(this);

	m_port1 = port;

	if (m_port1)
		m_port1->appendConnect(this);

	if (m_port1 && m_port1->isSelected())
		setSelectedEx(m_port1, true);
}


qjackctlGraphPort *qjackctlGraphConnect::port1 (void) const
{
	return m_port1;
}


void qjackctlGraphConnect::setPort2 ( qjackctlGraphPort *port )
{
	if (m_port2)
		m_port2->removeConnect(this);

	m_port2 = port;

	if (m_port2)
		m_port2->appendConnect(this);

	if (m_port2 && m_port2->isSelected())
		setSelectedEx(m_port2, true);
}


qjackctlGraphPort *qjackctlGraphConnect::port2 (void) const
{
	return m_port2;
}


// Active disconnection.
void qjackctlGraphConnect::disconnect (void)
{
	if (m_port1) {
		m_port1->removeConnect(this);
		m_port1 = nullptr;
	}

	if (m_port2) {
		m_port2->removeConnect(this);
		m_port2 = nullptr;
	}
}


// Path/shaper updaters.
void qjackctlGraphConnect::updatePathTo ( const QPointF& pos )
{
	const bool is_out0 = m_port1->isOutput();
	const QPointF pos0 = m_port1->portPos();

	const QPointF d1(1.0, 0.0);
	const QPointF pos1 = (is_out0 ? pos0 + d1 : pos  - d1);
	const QPointF pos4 = (is_out0 ? pos  - d1 : pos0 + d1);

	const QPointF d2(2.0, 0.0);
	const QPointF pos1_2(is_out0 ? pos1 + d2 : pos1 - d2);
	const QPointF pos3_4(is_out0 ? pos4 - d2 : pos4 + d2);

	qjackctlGraphNode *node1 = m_port1->portNode();
	const QRectF& rect1 = node1->itemRect();
	const qreal h1 = 0.5 * rect1.height();
	const qreal dh = pos0.y() - node1->scenePos().y() - h1;
	const qreal dx = pos3_4.x() - pos1_2.x();
	const qreal x_max = rect1.width() + h1;
	const qreal x_min = qMin(x_max, qAbs(dx));
	const qreal x_offset = (dx > 0.0 ? 0.5 : 1.0) * x_min;

	qreal y_offset = 0.0;
	if (g_connect_through_nodes) {
		// New "normal" connection line curves (inside/through nodes)...
		const qreal h2 = m_port1->itemRect().height();
		const qreal dy = qAbs(pos3_4.y() - pos1_2.y());
		y_offset = (dx > -h2 || dy > h2 ? 0.0 : (dh > 0.0 ? +h2 : -h2));
	} else {
		// Old "weird" connection line curves (outside/around nodes)...
		y_offset = (dx > 0.0 ? 0.0 : (dh > 0.0 ? +x_min : -x_min));
	}

	const QPointF pos2(pos1.x() + x_offset, pos1.y() + y_offset);
	const QPointF pos3(pos4.x() - x_offset, pos4.y() + y_offset);

	QPainterPath path;
	path.moveTo(pos1);
	path.lineTo(pos1_2);
	path.cubicTo(pos2, pos3, pos3_4);
	path.lineTo(pos4);

	const qreal arrow_angle = path.angleAtPercent(0.5) * M_PI / 180.0;
	const QPointF arrow_pos0 = path.pointAtPercent(0.5);
	const qreal arrow_size = 8.0;
	QVector<QPointF> arrow;
	arrow.append(arrow_pos0);
	arrow.append(arrow_pos0 - QPointF(
		::sin(arrow_angle + M_PI / 2.25) * arrow_size,
		::cos(arrow_angle + M_PI / 2.25) * arrow_size));
	arrow.append(arrow_pos0 - QPointF(
		::sin(arrow_angle + M_PI - M_PI / 2.25) * arrow_size,
		::cos(arrow_angle + M_PI - M_PI / 2.25) * arrow_size));
	arrow.append(arrow_pos0);
	path.addPolygon(QPolygonF(arrow));

	/*QGraphicsPathItem::*/setPath(path);
}


void qjackctlGraphConnect::updatePath (void)
{
	if (m_port2)
		updatePathTo(m_port2->portPos());
}


void qjackctlGraphConnect::paint ( QPainter *painter,
	const QStyleOptionGraphicsItem *option, QWidget */*widget*/ )
{
	QColor color;
	if (QGraphicsPathItem::isSelected())
		color = option->palette.highlight().color();
	else
	if (qjackctlGraphItem::isHighlight())
		color = qjackctlGraphItem::foreground().lighter();
	else
		color = qjackctlGraphItem::foreground();

	const QPalette pal;
	const bool is_darkest = (pal.base().color().value() < 24);
	QColor shadow_color = (is_darkest ? Qt::white : Qt::black);
	shadow_color.setAlpha(80);

	const QPainterPath& path
		= QGraphicsPathItem::path();
	painter->setBrush(Qt::NoBrush);
	painter->setPen(QPen(shadow_color, 3));
	painter->drawPath(path.translated(+1.0, +1.0));
	painter->setPen(QPen(color, 2));
	painter->drawPath(path);
}


QVariant qjackctlGraphConnect::itemChange (
	GraphicsItemChange change, const QVariant& value )
{
	if (change == QGraphicsItem::ItemSelectedHasChanged) {
		const bool is_selected = value.toBool();
		qjackctlGraphItem::setHighlight(is_selected);
		if (m_port1)
			m_port1->setSelectedEx(is_selected);
		if (m_port2)
			m_port2->setSelectedEx(is_selected);
	}

	return value;
}


QPainterPath qjackctlGraphConnect::shape (void) const
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 1, 0)) && (__cplusplus < 201703L)
	return QGraphicsPathItem::shape();
#else
	const QPainterPathStroker stroker
		= QPainterPathStroker(QPen(QColor(), 2));
	return stroker.createStroke(path());
#endif
}


// Selection propagation method...
void qjackctlGraphConnect::setSelectedEx ( qjackctlGraphPort *port, bool is_selected )
{
	setHighlightEx(port, is_selected);

	if (QGraphicsPathItem::isSelected() != is_selected) {
	#if 0//OLD_SELECT_BEHAVIOR
		QGraphicsPathItem::setSelected(is_selected);
		if (is_selected) {
			if (m_port1 && m_port1 != port)
				m_port1->setSelectedEx(is_selected);
			if (m_port2 && m_port2 != port)
				m_port2->setSelectedEx(is_selected);
		}
	#else
		if (!is_selected || (m_port1 && m_port2
			&& m_port1->isSelected() && m_port2->isSelected())) {
			QGraphicsPathItem::setSelected(is_selected);
		}
	#endif
	}
}

// Highlighting propagation method...
void qjackctlGraphConnect::setHighlightEx ( qjackctlGraphPort *port, bool is_highlight )
{
	qjackctlGraphItem::setHighlight(is_highlight);

	if (m_port1 && m_port1 != port)
		m_port1->setHighlight(is_highlight);
	if (m_port2 && m_port2 != port)
		m_port2->setHighlight(is_highlight);
}


// Special port-type color business.
void qjackctlGraphConnect::updatePortTypeColors (void)
{
	if (m_port1) {
		const QColor& color
			= m_port1->background().lighter();
		qjackctlGraphItem::setForeground(color);
		qjackctlGraphItem::setBackground(color);
	}
}


// Connector curve draw style (through vs. around nodes)
//
bool qjackctlGraphConnect::g_connect_through_nodes = false;

void qjackctlGraphConnect::setConnectThroughNodes ( bool on )
{
	g_connect_through_nodes = on;
}

bool qjackctlGraphConnect::isConnectThroughNodes (void)
{
	return g_connect_through_nodes;
}


//----------------------------------------------------------------------------
// qjackctlGraphCanvas -- Canvas graphics scene/view.

// Local constants.
static const char *CanvasGroup   = "/GraphCanvas";
static const char *CanvasRectKey = "/CanvasRect";
static const char *CanvasZoomKey = "/CanvasZoom";

static const char *NodePosGroup  = "/GraphNodePos";

static const char *ColorsGroup   = "/GraphColors";


// Constructor.
qjackctlGraphCanvas::qjackctlGraphCanvas ( QWidget *parent )
	: QGraphicsView(parent), m_state(DragNone), m_item(nullptr),
		m_connect(nullptr), m_port2(nullptr), m_rubberband(nullptr),
		m_zoom(1.0), m_zoomrange(false), m_gesture(false),
		m_commands(nullptr), m_settings(nullptr),
		m_selected_nodes(0), m_repel_overlapping_nodes(false),
		m_rename_item(nullptr), m_rename_editor(nullptr), m_renamed(0),
		m_search_editor(nullptr), m_aliases(nullptr)
{
	m_scene = new QGraphicsScene();

	m_commands = new QUndoStack();

	QGraphicsView::setScene(m_scene);

	QGraphicsView::setRenderHint(QPainter::Antialiasing);
	QGraphicsView::setRenderHint(QPainter::SmoothPixmapTransform);

	QGraphicsView::setResizeAnchor(QGraphicsView::NoAnchor);
	QGraphicsView::setDragMode(QGraphicsView::NoDrag);

	updatePalette();

	m_rename_editor = new QLineEdit(this);
	m_rename_editor->setFrame(false);
	m_rename_editor->setEnabled(false);
	m_rename_editor->hide();

	QObject::connect(m_rename_editor,
		SIGNAL(textChanged(const QString&)),
		SLOT(renameTextChanged(const QString&)));
	QObject::connect(m_rename_editor,
		SIGNAL(editingFinished()),
		SLOT(renameEditingFinished()));

	QGraphicsView::grabGesture(Qt::PinchGesture);

	m_search_editor = new QLineEdit(this);
	m_search_editor->setClearButtonEnabled(true);
	m_search_editor->setEnabled(false);
	m_search_editor->hide();

	QObject::connect(m_search_editor,
		SIGNAL(textChanged(const QString&)),
		SLOT(searchTextChanged(const QString&)));
	QObject::connect(m_search_editor,
		SIGNAL(editingFinished()),
		SLOT(searchEditingFinished()));
}


// Destructor.
qjackctlGraphCanvas::~qjackctlGraphCanvas (void)
{
	clear();

	delete m_search_editor;
	delete m_rename_editor;

	delete m_commands;
	delete m_scene;
}


// Accessors.
QGraphicsScene *qjackctlGraphCanvas::scene (void) const
{
	return m_scene;
}


QUndoStack *qjackctlGraphCanvas::commands (void) const
{
	return m_commands;
}


void qjackctlGraphCanvas::setSettings ( QSettings *settings )
{
	m_settings = settings;
}


QSettings *qjackctlGraphCanvas::settings (void) const
{
	return m_settings;
}


// Canvas methods.
void qjackctlGraphCanvas::addItem ( qjackctlGraphItem *item )
{
	m_scene->addItem(item);

	if (item->type() == qjackctlGraphNode::Type) {
		qjackctlGraphNode *node = static_cast<qjackctlGraphNode *> (item);
		if (node) {
			m_nodes.append(node);
			m_nodekeys.insert(qjackctlGraphNode::NodeKey(node), node);
			if (restoreNode(node))
				emit updated(node);
			else
				emit added(node);
		}
	}
}


void qjackctlGraphCanvas::removeItem ( qjackctlGraphItem *item )
{
	clearSelection();

	if (item->type() == qjackctlGraphNode::Type) {
		qjackctlGraphNode *node = static_cast<qjackctlGraphNode *> (item);
		if (node && saveNode(node)) {
			emit removed(node);
			node->removePorts();
			m_nodekeys.remove(qjackctlGraphNode::NodeKey(node));
			m_nodes.removeAll(node);
		}
	}

	// Do not remove items from the scene
	// as they shall be removed upon delete...
	//
	//	m_scene->removeItem(item);
}


// Current item accessor.
qjackctlGraphItem *qjackctlGraphCanvas::currentItem (void) const
{
	qjackctlGraphItem *item = m_item;

	if (item && item->type() == qjackctlGraphConnect::Type)
		item = nullptr;

 	if (item == nullptr) {
		foreach (QGraphicsItem *item2, m_scene->selectedItems()) {
			if (item2->type() == qjackctlGraphConnect::Type)
				continue;
			item = static_cast<qjackctlGraphItem *> (item2);
			if (item2->type() == qjackctlGraphNode::Type)
				break;
		}
	}

	return item;
}


// Connection predicates.
bool qjackctlGraphCanvas::canConnect (void) const
{
	int nins = 0;
	int nouts = 0;

	foreach (QGraphicsItem *item, m_scene->selectedItems()) {
		if (item->type() == qjackctlGraphNode::Type) {
			qjackctlGraphNode *node = static_cast<qjackctlGraphNode *> (item);
			if (node) {
				if (node->nodeMode() & qjackctlGraphItem::Input)
					++nins;
				else
			//	if (node->nodeMode() & qjackctlGraphItem::Output)
					++nouts;
			}
		}
		else
		if (item->type() == qjackctlGraphPort::Type) {
			qjackctlGraphPort *port = static_cast<qjackctlGraphPort *> (item);
			if (port) {
				if (port->isInput())
					++nins;
				else
			//	if (port->isOutput())
					++nouts;
			}
		}
		if (nins > 0 && nouts > 0)
			return true;
	}

	return false;
}


bool qjackctlGraphCanvas::canDisconnect (void) const
{
	foreach (QGraphicsItem *item, m_scene->selectedItems()) {
		switch (item->type()) {
		case qjackctlGraphConnect::Type:
			return true;
		case qjackctlGraphNode::Type: {
			qjackctlGraphNode *node = static_cast<qjackctlGraphNode *> (item);
			foreach (qjackctlGraphPort *port, node->ports()) {
				if (!port->connects().isEmpty())
					return true;
			}
			// Fall thru...
		}
		default:
			break;
		}
	}

	return false;
}


// Edit predicates.
bool qjackctlGraphCanvas::canRenameItem (void) const
{
	qjackctlGraphItem *item = currentItem();

	return (item && (
		item->type() == qjackctlGraphNode::Type ||
		item->type() == qjackctlGraphPort::Type));
}


bool qjackctlGraphCanvas::canSearchItem (void) const
{
	return !m_nodes.isEmpty();
}


// Zooming methods.
void qjackctlGraphCanvas::setZoom ( qreal zoom )
{
	if (zoom < 0.1)
		zoom = 0.1;
	else
	if (zoom > 1.9)
		zoom = 1.9;

	const qreal scale = zoom / m_zoom;
	QGraphicsView::scale(scale, scale);

	QFont font = m_rename_editor->font();
	font.setPointSizeF(scale * font.pointSizeF());
	m_rename_editor->setFont(font);
	updateRenameEditor();

	m_zoom = zoom;

	emit changed();
}


qreal qjackctlGraphCanvas::zoom (void) const
{
	return m_zoom;
}


void qjackctlGraphCanvas::setZoomRange ( bool zoomrange )
{
	m_zoomrange = zoomrange;
}


bool qjackctlGraphCanvas::isZoomRange (void) const
{
	return m_zoomrange;
}


// Clean-up all un-marked nodes...
void qjackctlGraphCanvas::resetNodes ( uint node_type )
{
	QList<qjackctlGraphNode *> nodes;

	foreach (qjackctlGraphNode *node, m_nodes) {
		if (node->nodeType() == node_type) {
			if (node->isMarked()) {
				node->resetMarkedPorts();
				node->setMarked(false);
			} else {
				nodes.append(node);
			}
		}
	}

	foreach (qjackctlGraphNode *node, nodes)
		removeItem(node);

	qDeleteAll(nodes);
}


void qjackctlGraphCanvas::clearNodes ( uint node_type )
{
	QList<qjackctlGraphNode *> nodes;

	foreach (qjackctlGraphNode *node, m_nodes) {
		if (node->nodeType() == node_type)
			nodes.append(node);
	}

	foreach (qjackctlGraphNode *node, nodes) {
		m_nodekeys.remove(qjackctlGraphNode::NodeKey(node));
		m_nodes.removeAll(node);
	}

	qDeleteAll(nodes);
}


// Special node finder.
qjackctlGraphNode *qjackctlGraphCanvas::findNode (
	const QString& name, qjackctlGraphItem::Mode mode, uint type ) const
{
	return static_cast<qjackctlGraphNode *> (
		m_nodekeys.value(qjackctlGraphNode::ItemKey(name, mode, type), nullptr));
}


// Whether it's in the middle of something...
bool qjackctlGraphCanvas::isBusy (void) const
{
	return (m_state != DragNone || m_connect != nullptr
		||  m_item  != nullptr  || m_rename_item != nullptr);
}


// Port (dis)connections notifiers.
void qjackctlGraphCanvas::emitConnected (
	qjackctlGraphPort *port1, qjackctlGraphPort *port2 )
{
	emit connected(port1, port2);
}


void qjackctlGraphCanvas::emitDisconnected (
	qjackctlGraphPort *port1, qjackctlGraphPort *port2 )
{
	emit disconnected(port1, port2);
}


// Rename notifier.
void qjackctlGraphCanvas::emitRenamed ( qjackctlGraphItem *item, const QString& name )
{
	emit renamed(item, name);
}


// Other generic notifier.
void qjackctlGraphCanvas::emitChanged (void)
{
	emit changed();
}


// Item finder (internal).
qjackctlGraphItem *qjackctlGraphCanvas::itemAt ( const QPointF& pos ) const
{
	const QList<QGraphicsItem *>& items
		= m_scene->items(QRectF(pos - QPointF(2, 2), QSizeF(5, 5)));

	foreach (QGraphicsItem *item, items) {
		if (item->type() >= QGraphicsItem::UserType)
			return static_cast<qjackctlGraphItem *> (item);
	}

	return nullptr;
}


// Port (dis)connection command.
void qjackctlGraphCanvas::connectPorts (
	qjackctlGraphPort *port1, qjackctlGraphPort *port2, bool is_connect )
{
#if 0 // Sure the sect will check to this instead...?
	const bool is_connected // already connected?
		= (port1->findConnect(port2) != nullptr);
	if (( is_connect &&  is_connected) ||
		(!is_connect && !is_connected))
		return;
#endif
	if (port1->isOutput()) {
		m_commands->push(
			new qjackctlGraphConnectCommand(this, port1, port2, is_connect));
	} else {
		m_commands->push(
			new qjackctlGraphConnectCommand(this, port2, port1, is_connect));
	}
}


// Mouse event handlers.
void qjackctlGraphCanvas::mousePressEvent ( QMouseEvent *event )
{
	if (m_gesture)
		return;

	m_state = DragNone;
	m_item = nullptr;
	m_pos = QGraphicsView::mapToScene(event->pos());

	if (event->button() == Qt::LeftButton) {
		m_item = itemAt(m_pos);
		m_state = DragStart;
	}

	if (m_item == nullptr
		&& (((event->button() == Qt::LeftButton)
		  && (event->modifiers() & Qt::ControlModifier))
		  || (event->button() == Qt::MiddleButton))) {
	#if 1//NEW_DRAG_SCROLL_MODE
		// HACK: When about to drag-scroll,
		// always fake a left-button press...
		QGraphicsView::setDragMode(ScrollHandDrag);
		QMouseEvent event2(event->type(),
		#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
			event->position(), event->globalPosition(),
		#else
			event->localPos(), event->globalPos(),
		#endif
			Qt::LeftButton, Qt::LeftButton,
			event->modifiers() | Qt::ControlModifier);
		QGraphicsView::mousePressEvent(&event2);
	#else
		QGraphicsView::setCursor(Qt::ClosedHandCursor)
	#endif
		m_state = DragScroll;
	}
}


void qjackctlGraphCanvas::mouseMoveEvent ( QMouseEvent *event )
{
	if (m_gesture)
		return;

	int nchanged = 0;

	QPointF pos = QGraphicsView::mapToScene(event->pos());

	switch (m_state) {
	case DragStart:
		if ((pos - m_pos).manhattanLength() > 8.0) {
			m_state = DragMove;
			if (m_item) {
				// Start new connection line...
				if (m_item->type() == qjackctlGraphPort::Type) {
					qjackctlGraphPort *port = static_cast<qjackctlGraphPort *> (m_item);
					if (port) {
						QGraphicsView::setCursor(Qt::DragLinkCursor);
						m_selected_nodes = 0;
						m_scene->clearSelection();
						m_connect = new qjackctlGraphConnect();
						m_connect->setPort1(port);
						m_connect->setSelected(true);
						m_connect->raise();
						m_scene->addItem(m_connect);
						m_item = nullptr;
						++m_selected_nodes;
						++nchanged;
					}
				}
				else
				// Start moving nodes around...
				if (m_item->type() == qjackctlGraphNode::Type) {
					QGraphicsView::setCursor(Qt::SizeAllCursor);
					if (!m_item->isSelected()) {
						if ((event->modifiers()
							 & (Qt::ShiftModifier | Qt::ControlModifier)) == 0) {
							m_selected_nodes = 0;
							m_scene->clearSelection();
						}
						m_item->setSelected(true);
						++nchanged;
					}
					// Original node position (for move command)...
					m_pos1 = snapPos(m_pos);
				}
				else m_item = nullptr;
			}
			// Otherwise start lasso rubber-banding...
			if (m_rubberband == nullptr && m_item == nullptr && m_connect == nullptr) {
				QGraphicsView::setCursor(Qt::CrossCursor);
				m_rubberband = new QRubberBand(QRubberBand::Rectangle, this);
			}
			// Set allowed auto-scroll margins/limits...
			boundingRect(true);
		}
		break;
	case DragMove:
		// Allow auto-scroll only if within allowed margins/limits...
		boundingPos(pos);
		QGraphicsView::ensureVisible(QRectF(pos, QSizeF(2, 2)), 8, 8);
		// Move new connection line...
		if (m_connect)
			m_connect->updatePathTo(pos);
		// Move rubber-band lasso...
		if (m_rubberband) {
			const QRect rect(
				QGraphicsView::mapFromScene(m_pos),
				QGraphicsView::mapFromScene(pos));
			m_rubberband->setGeometry(rect.normalized());
			m_rubberband->show();
			if (!m_zoomrange) {
				if (event->modifiers()
					& (Qt::ControlModifier | Qt::ShiftModifier)) {
					foreach (QGraphicsItem *item, m_selected) {
						item->setSelected(!item->isSelected());
						++nchanged;
					}
					m_selected.clear();
				} else {
					m_selected_nodes = 0;
					m_scene->clearSelection();
					++nchanged;
				}
				const QRectF range_rect(m_pos, pos);
				foreach (QGraphicsItem *item,
						m_scene->items(range_rect.normalized())) {
					if (item->type() >= QGraphicsItem::UserType) {
						if (item->type() != qjackctlGraphNode::Type)
							++m_selected_nodes;
						else
						if (m_selected_nodes > 0)
							continue;
						const bool is_selected = item->isSelected();
						if (event->modifiers() & Qt::ControlModifier) {
							m_selected.append(item);
							item->setSelected(!is_selected);
						}
						else
						if (!is_selected) {
							if (event->modifiers() & Qt::ShiftModifier)
								m_selected.append(item);
							item->setSelected(true);
						}
						++nchanged;
					}
				}
			}
		}
		// Move current selected nodes...
		if (m_item && m_item->type() == qjackctlGraphNode::Type) {
			pos = snapPos(pos);
			const QPointF delta = (pos - m_pos);
			foreach (QGraphicsItem *item, m_scene->selectedItems()) {
				if (item->type() == qjackctlGraphNode::Type) {
					qjackctlGraphNode *node = static_cast<qjackctlGraphNode *> (item);
					if (node)
						node->setPos(snapPos(node->pos() + delta));
				}
			}
			m_pos = pos;
		}
		else
		if (m_connect) {
			// Hovering ports high-lighting...
			const qreal zval = m_connect->zValue();
			m_connect->setZValue(-1.0);
			if (m_port2) {
				m_port2->setHighlight(false);
				m_port2 = nullptr;
			}
			QGraphicsItem *item = itemAt(pos);
			if (item && item->type() == qjackctlGraphPort::Type) {
				qjackctlGraphPort *port1 = m_connect->port1();
				qjackctlGraphPort *port2 = static_cast<qjackctlGraphPort *> (item);
				if (port1 && port2 && !port2->isHighlight() &&
					port1->portType() == port2->portType() &&
					port1->portMode() != port2->portMode()) {
					m_port2 = port2;
					m_port2->setHighlight(true);
				}
			}
			 m_connect->setZValue(zval);
		}
		break;
	case DragScroll: {
	#if 1//NEW_DRAG_SCROLL_MODE
		QGraphicsView::mouseMoveEvent(event);
	#else
		QScrollBar *hbar = QGraphicsView::horizontalScrollBar();
		QScrollBar *vbar = QGraphicsView::verticalScrollBar();
		const QPoint delta = (pos - m_pos).toPoint();
		hbar->setValue(hbar->value() - delta.x());
		vbar->setValue(vbar->value() - delta.y());
		m_pos = pos;
	#endif
		break;
	}
	default:
		break;
	}

	if (nchanged > 0)
		emit changed();
}


void qjackctlGraphCanvas::mouseReleaseEvent ( QMouseEvent *event )
{
	if (m_gesture)
		return;

	int nchanged = 0;

	switch (m_state) {
	case DragStart:
		// Make individual item (de)selections...
		if ((event->modifiers()
			& (Qt::ShiftModifier | Qt::ControlModifier)) == 0) {
			m_selected_nodes = 0;
			m_scene->clearSelection();
			++nchanged;
		}
		if (m_item) {
			bool is_selected = true;
			if (event->modifiers() & Qt::ControlModifier)
				is_selected = !m_item->isSelected();
			m_item->setSelected(is_selected);
			if (m_item->type() != qjackctlGraphNode::Type && is_selected)
				++m_selected_nodes;
			m_item = nullptr; // Not needed anymore!
			++nchanged;
		}
		// Fall thru...
	case DragMove:
		// Close new connection line...
		if (m_connect) {
			 m_connect->setZValue(-1.0);
			const QPointF& pos
				= QGraphicsView::mapToScene(event->pos());
			qjackctlGraphItem *item = itemAt(pos);
			if (item && item->type() == qjackctlGraphPort::Type) {
				qjackctlGraphPort *port1 = m_connect->port1();
				qjackctlGraphPort *port2 = static_cast<qjackctlGraphPort *> (item);
				if (port1 && port2
				//	&& port1->portNode() != port2->portNode()
					&& port1->portMode() != port2->portMode()
					&& port1->portType() == port2->portType()
					&& port1->findConnect(port2) == nullptr) {
					port2->setSelected(true);
				#if 1 // Sure the sect will commit to this instead...?
					m_connect->setPort2(port2);
					m_connect->updatePortTypeColors();
					m_connect->updatePathTo(port2->portPos());
					emit connected(m_connect);
					m_connect = nullptr;
					++m_selected_nodes;
				#else
				//	m_selected_nodes = 0;
				//	m_scene->clearSelection();
				#endif
					// Submit command; notify eventual observers...
					m_commands->beginMacro(tr("Connect"));
					connectPorts(port1, port2, true);
					m_commands->endMacro();
					++nchanged;
				}
			}
			// Done with the hovering connection...
			if (m_port2) {
				m_port2->setHighlight(false);
				m_port2 = nullptr;
			}
			if (m_connect) {
				m_connect->disconnect();
				delete m_connect;
				m_connect = nullptr;
			}
		}
		// Maybe some node(s) were moved...
		if (m_item && m_item->type() == qjackctlGraphNode::Type) {
			const QPointF& pos
				= QGraphicsView::mapToScene(event->pos());
			QList<qjackctlGraphNode *> nodes;
			foreach (QGraphicsItem *item, m_scene->selectedItems()) {
				if (item->type() == qjackctlGraphNode::Type) {
					qjackctlGraphNode *node = static_cast<qjackctlGraphNode *> (item);
					if (node)
						nodes.append(node);
				}
			}
			m_commands->push(
				new qjackctlGraphMoveCommand(this, nodes, m_pos1, m_pos));
			++nchanged;
		}
		// Close rubber-band lasso...
		if (m_rubberband) {
			delete m_rubberband;
			m_rubberband = nullptr;
			m_selected.clear();
			// Zooming in range?...
			if (m_zoomrange) {
				const QRectF range_rect(m_pos,
					QGraphicsView::mapToScene(event->pos()));
				zoomFitRange(range_rect);
				nchanged = 0;
			}
		}
		break;
	case DragScroll:
	default:
		break;
	}

#if 1//NEW_DRAG_SCROLL_MODE
	if (QGraphicsView::dragMode() == ScrollHandDrag) {
		QGraphicsView::mouseReleaseEvent(event);
		QGraphicsView::setDragMode(NoDrag);
	}
#endif
	m_state = DragNone;
	m_item = nullptr;

	// Reset cursor...
	QGraphicsView::setCursor(Qt::ArrowCursor);

	if (nchanged > 0)
		emit changed();
}


void qjackctlGraphCanvas::mouseDoubleClickEvent ( QMouseEvent *event )
{
	m_pos  = QGraphicsView::mapToScene(event->pos());
	m_item = itemAt(m_pos);

	if (m_item && canRenameItem()) {
		renameItem();
	} else {
		QGraphicsView::centerOn(m_pos);
	}
}


void qjackctlGraphCanvas::wheelEvent ( QWheelEvent *event )
{
	if (event->modifiers() & Qt::ControlModifier) {
		const int delta
		#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
			= event->delta();
		#else
			= event->angleDelta().y();
		#endif
		setZoom(zoom() + qreal(delta) / 1200.0);
	}
	else QGraphicsView::wheelEvent(event);
}


// Keyboard event handler.
void qjackctlGraphCanvas::keyPressEvent ( QKeyEvent *event )
{
	if (event->key() == Qt::Key_Escape) {
		m_scene->clearSelection();
		m_search_editor->editingFinished();
		clear();
		emit changed();
	}
	else
	if (!m_search_editor->isEnabled()
		&& (event->modifiers() & Qt::ControlModifier) == 0
		&& !event->text().trimmed().isEmpty()) {
		startSearchEditor(event->text());
	}
}


// Connect selected items.
void qjackctlGraphCanvas::connectItems (void)
{
	QList<qjackctlGraphPort *> outs;
	QList<qjackctlGraphPort *> ins;

	foreach (QGraphicsItem *item, m_scene->selectedItems()) {
		if (item->type() == qjackctlGraphPort::Type) {
			qjackctlGraphPort *port = static_cast<qjackctlGraphPort *> (item);
			if (port) {
				if (port->isOutput())
					outs.append(port);
				else
					ins.append(port);
			}
		}
	}

	if (outs.isEmpty() || ins.isEmpty())
		return;

//	m_selected_nodes = 0;
//	m_scene->clearSelection();

	std::sort(outs.begin(), outs.end(), qjackctlGraphPort::ComparePos());
	std::sort(ins.begin(),  ins.end(),  qjackctlGraphPort::ComparePos());

	QListIterator<qjackctlGraphPort *> iter1(outs);
	QListIterator<qjackctlGraphPort *> iter2(ins);

	m_commands->beginMacro(tr("Connect"));

	const int nports = qMax(outs.count(), ins.count());
	for (int n = 0; n < nports; ++n) {
		// Wrap a'round...
		if (!iter1.hasNext())
			iter1.toFront();
		if (!iter2.hasNext())
			iter2.toFront();
		qjackctlGraphPort *port1 = iter1.next();
		qjackctlGraphPort *port2 = iter2.next();
		// Skip over non-matching port-types...
		bool wrapped = false;
		while (port1 && port2 && port1->portType() != port2->portType()) {
			if (!iter2.hasNext()) {
				if (wrapped)
					break;
				iter2.toFront();
				wrapped = true;
			}
			port2 = iter2.next();
		}
		// Submit command; notify eventual observers...
		if (!wrapped && port1 && port2 && port1->portNode() != port2->portNode())
			connectPorts(port1, port2, true);
	}

	m_commands->endMacro();
}


// Disconnect selected items.
void qjackctlGraphCanvas::disconnectItems (void)
{
	QList<qjackctlGraphConnect *> connects;
	QList<qjackctlGraphNode *> nodes;

	foreach (QGraphicsItem *item, m_scene->selectedItems()) {
		switch (item->type()) {
		case qjackctlGraphConnect::Type: {
			qjackctlGraphConnect *connect = static_cast<qjackctlGraphConnect *> (item);
			if (!connects.contains(connect))
				connects.append(connect);
			break;
		}
		case qjackctlGraphNode::Type:
			nodes.append(static_cast<qjackctlGraphNode *> (item));
			// Fall thru...
		default:
			break;
		}
	}

	if (connects.isEmpty()) {
		foreach (qjackctlGraphNode *node, nodes) {
			foreach (qjackctlGraphPort *port, node->ports()) {
				foreach (qjackctlGraphConnect *connect, port->connects()) {
					if (!connects.contains(connect))
						connects.append(connect);
				}
			}
		}
	}

	if (connects.isEmpty())
		return;

//	m_selected_nodes = 0;
//	m_scene->clearSelection();

	m_item = nullptr;

	m_commands->beginMacro(tr("Disconnect"));

	foreach (qjackctlGraphConnect *connect, connects) {
		// Submit command; notify eventual observers...
		qjackctlGraphPort *port1 = connect->port1();
		qjackctlGraphPort *port2 = connect->port2();
		if (port1 && port2)
			connectPorts(port1, port2, false);
	}

	m_commands->endMacro();
}


// Select actions.
void qjackctlGraphCanvas::selectAll (void)
{
	foreach (QGraphicsItem *item, m_scene->items()) {
		if (item->type() == qjackctlGraphNode::Type)
			item->setSelected(true);
		else
			++m_selected_nodes;
	}

	emit changed();
}


void qjackctlGraphCanvas::selectNone (void)
{
	m_selected_nodes = 0;
	m_scene->clearSelection();

	emit changed();
}


void qjackctlGraphCanvas::selectInvert (void)
{
	foreach (QGraphicsItem *item, m_scene->items()) {
		if (item->type() == qjackctlGraphNode::Type)
			item->setSelected(!item->isSelected());
		else
			++m_selected_nodes;
	}

	emit changed();
}


// Edit actions.
void qjackctlGraphCanvas::renameItem (void)
{
	qjackctlGraphItem *item = currentItem();

	if (item && item->type() == qjackctlGraphNode::Type) {
		qjackctlGraphNode *node = static_cast<qjackctlGraphNode *> (item);
		if (node) {
			QPalette pal;
			const QColor& foreground
				= node->foreground();
			QColor background = node->background();
			const bool is_dark
				= (background.value() < 192);
			pal.setColor(QPalette::Text, is_dark
				? foreground.lighter()
				: foreground.darker());
			background.setAlpha(255);
			pal.setColor(QPalette::Base, background);
			m_rename_editor->setPalette(pal);
			QFont font = m_rename_editor->font();
			font.setBold(true);
			m_rename_editor->setFont(font);
			m_rename_editor->setPlaceholderText(node->nodeName());
			m_rename_editor->setText(node->nodeTitle());
		}
	}
	else
	if (item && item->type() == qjackctlGraphPort::Type) {
		qjackctlGraphPort *port = static_cast<qjackctlGraphPort *> (item);
		if (port) {
			QPalette pal;
			const QColor& foreground
				= port->foreground();
			const QColor& background
				= port->background();
			const bool is_dark
				= (background.value() < 128);
			pal.setColor(QPalette::Text, is_dark
				? foreground.lighter()
				: foreground.darker());
			pal.setColor(QPalette::Base, background.lighter());
			m_rename_editor->setPalette(pal);
			QFont font = m_rename_editor->font();
			font.setBold(false);
			m_rename_editor->setFont(font);
			m_rename_editor->setPlaceholderText(port->portName());
			m_rename_editor->setText(port->portTitle());
		}
	}
	else return;

	m_selected_nodes = 0;
	m_scene->clearSelection();

	m_rename_editor->show();
	m_rename_editor->setEnabled(true);
	m_rename_editor->selectAll();
	m_rename_editor->setFocus();
	m_renamed = 0;

	m_rename_item = item;

	updateRenameEditor();
}


void qjackctlGraphCanvas::searchItem (void)
{
	if (!m_search_editor->isEnabled())
		startSearchEditor();
}


// Update editors position and size.
void qjackctlGraphCanvas::updateRenameEditor (void)
{
	if (m_rename_item
		&& m_rename_editor->isEnabled()
		&& m_rename_editor->isVisible()) {
		const QRectF& rect
			= m_rename_item->editorRect().adjusted(+2.0, +2.0, -2.0, -2.0);
		const QPoint& pos1
			= QGraphicsView::mapFromScene(rect.topLeft());
		const QPoint& pos2
			= QGraphicsView::mapFromScene(rect.bottomRight());
		m_rename_editor->setGeometry(
			pos1.x(),  pos1.y(),
			pos2.x() - pos1.x(),
			pos2.y() - pos1.y());
	}
}


void qjackctlGraphCanvas::updateSearchEditor (void)
{
	// Position the search editor to the bottom-right of the canvas...
	const QSize& size = QGraphicsView::viewport()->size();
	const QSize& hint = m_search_editor->sizeHint();
	const int w = hint.width() * 2;
	const int h = hint.height();
	m_search_editor->setGeometry(size.width() - w, size.height() - h, w, h);
}


// Discrete zooming actions.
void qjackctlGraphCanvas::zoomIn (void)
{
	setZoom(zoom() + 0.1);
}


void qjackctlGraphCanvas::zoomOut (void)
{
	setZoom(zoom() - 0.1);
}


void qjackctlGraphCanvas::zoomFit (void)
{
	zoomFitRange(m_scene->itemsBoundingRect());
}


void qjackctlGraphCanvas::zoomReset (void)
{
	setZoom(1.0);
}


// Update all nodes.
void qjackctlGraphCanvas::updateNodes (void)
{
	foreach (QGraphicsItem *item, m_scene->items()) {
		if (item->type() == qjackctlGraphNode::Type) {
			qjackctlGraphNode *node = static_cast<qjackctlGraphNode *> (item);
			if (node)
				node->updatePath();
		}
	}
}


// Update all connectors.
void qjackctlGraphCanvas::updateConnects (void)
{
	foreach (QGraphicsItem *item, m_scene->items()) {
		if (item->type() == qjackctlGraphConnect::Type) {
			qjackctlGraphConnect *connect = static_cast<qjackctlGraphConnect *> (item);
			if (connect)
				connect->updatePath();
		}
	}
}


// Zoom in rectangle range.
void qjackctlGraphCanvas::zoomFitRange ( const QRectF& range_rect )
{
	QGraphicsView::fitInView(
		range_rect, Qt::KeepAspectRatio);

	const QTransform& transform
		= QGraphicsView::transform();
	if (transform.isScaling()) {
		qreal zoom = transform.m11();
		if (zoom < 0.1) {
			const qreal scale = 0.1 / zoom;
			QGraphicsView::scale(scale, scale);
			zoom = 0.1;
		}
		else
		if (zoom > 2.0) {
			const qreal scale = 2.0 / zoom;
			QGraphicsView::scale(scale, scale);
			zoom = 2.0;
		}
		m_zoom = zoom;
	}

	emit changed();
}


// Graph node position methods.
bool qjackctlGraphCanvas::restoreNode ( qjackctlGraphNode *node )
{
	if (m_settings == nullptr || node == nullptr)
		return false;

	m_settings->beginGroup(NodePosGroup);
	QPointF node_pos = m_settings->value('/' + nodeKey(node)).toPointF();
	m_settings->endGroup();

	if (node_pos.isNull())
		return false;

	boundingPos(node_pos);

	node->setPos(node_pos);
	return true;
}


bool qjackctlGraphCanvas::saveNode ( qjackctlGraphNode *node ) const
{
	if (m_settings == nullptr || node == nullptr)
		return false;

	m_settings->beginGroup(NodePosGroup);
	m_settings->setValue('/' + nodeKey(node), node->pos());
	m_settings->endGroup();

	return true;
}


bool qjackctlGraphCanvas::restoreState (void)
{
	if (m_settings == nullptr)
		return false;

	m_settings->beginGroup(ColorsGroup);
	const QRegularExpression rx("^0x");
	QStringListIterator key(m_settings->childKeys());
	while (key.hasNext()) {
		const QString& sKey = key.next();
		const QColor& color = QString(m_settings->value(sKey).toString());
		if (color.isValid()) {
			QString sx(sKey);
			bool ok = false;
			const uint port_type = sx.remove(rx).toUInt(&ok, 16);
			if (ok) m_port_colors.insert(port_type, color);
		}
	}
	m_settings->endGroup();

	m_settings->beginGroup(CanvasGroup);
	const QRectF& rect = m_settings->value(CanvasRectKey).toRectF();
	const qreal zoom = m_settings->value(CanvasZoomKey, 1.0).toReal();
	m_settings->endGroup();

	if (rect.isValid())
		m_rect1 = rect; // QGraphicsView::setSceneRect(m_rect1);

	setZoom(zoom);

	return true;
}


bool qjackctlGraphCanvas::saveState (void) const
{
	if (m_settings == nullptr)
		return false;

	m_settings->beginGroup(NodePosGroup);
	const QList<QGraphicsItem *> items(m_scene->items());
	foreach (QGraphicsItem *item, items) {
		if (item->type() == qjackctlGraphNode::Type) {
			qjackctlGraphNode *node = static_cast<qjackctlGraphNode *> (item);
			if (node)
				m_settings->setValue('/' + nodeKey(node), node->pos());
		}
	}
	m_settings->endGroup();

	m_settings->beginGroup(CanvasGroup);
	m_settings->setValue(CanvasZoomKey, zoom());
	m_settings->setValue(CanvasRectKey, m_rect1);
	m_settings->endGroup();

	m_settings->beginGroup(ColorsGroup);
	QStringListIterator key(m_settings->childKeys());
	while (key.hasNext()) m_settings->remove(key.next());
	QHash<uint, QColor>::ConstIterator iter = m_port_colors.constBegin();
	const QHash<uint, QColor>::ConstIterator& iter_end = m_port_colors.constEnd();
	for ( ; iter != iter_end; ++iter) {
		const uint port_type = iter.key();
		const QColor& color = iter.value();
		m_settings->setValue("0x" + QString::number(port_type, 16), color.name());
	}
	m_settings->endGroup();

	return true;
}


// Graph node key mangler.
QString qjackctlGraphCanvas::nodeKey ( qjackctlGraphNode *node ) const
{
	QString node_key = node->nodeName();

	switch (node->nodeMode()) {
	case qjackctlGraphItem::Input:
		node_key += ":Input";
		break;
	case qjackctlGraphItem::Output:
		node_key += ":Output";
		break;
	default:
		break;
	}

	return node_key;
}


// Graph port colors management.
void qjackctlGraphCanvas::setPortTypeColor (
	uint port_type, const QColor& port_color )
{
	m_port_colors.insert(port_type, port_color);
}


const QColor& qjackctlGraphCanvas::portTypeColor ( uint port_type )
{
	return m_port_colors[port_type];
}


void qjackctlGraphCanvas::updatePortTypeColors ( uint port_type )
{
	foreach (QGraphicsItem *item, m_scene->items()) {
		if (item->type() == qjackctlGraphPort::Type) {
			qjackctlGraphPort *port = static_cast<qjackctlGraphPort *> (item);
			if (port && (0 >= port_type || port->portType() == port_type)) {
				port->updatePortTypeColors(this);
				port->update();
			}
		}
	}
}


void qjackctlGraphCanvas::clearPortTypeColors (void)
{
	m_port_colors.clear();
}


// Clear all selection.
void qjackctlGraphCanvas::clearSelection (void)
{
	m_item = nullptr;
	m_selected_nodes = 0;
	m_scene->clearSelection();

	m_rename_item = nullptr;
	m_rename_editor->setEnabled(false);
	m_rename_editor->hide();
	m_renamed = 0;
}


// Clear all state.
void qjackctlGraphCanvas::clear (void)
{
	m_selected_nodes = 0;
	if (m_rubberband) {
		delete m_rubberband;
		m_rubberband = nullptr;
		m_selected.clear();
	}
	if (m_port2) {
		m_port2->setHighlight(false);
		m_port2 = nullptr;
	}
	if (m_connect) {
		m_connect->disconnect();
		delete m_connect;
		m_connect = nullptr;
	}
	if (m_state == DragScroll)
		QGraphicsView::setDragMode(QGraphicsView::NoDrag);
	m_state = DragNone;
	m_item = nullptr;
	m_rename_item = nullptr;
	m_rename_editor->setEnabled(false);
	m_rename_editor->hide();
	m_renamed = 0;

	// Reset cursor...
	QGraphicsView::setCursor(Qt::ArrowCursor);
}


// Rename item slots.
void qjackctlGraphCanvas::renameTextChanged ( const QString& /* text */)
{
	if (m_rename_item
		&& m_rename_editor->isEnabled()
		&& m_rename_editor->isVisible())
		++m_renamed;
}


void qjackctlGraphCanvas::renameEditingFinished (void)
{
	if (m_rename_item
		&& m_rename_editor->isEnabled()
		&& m_rename_editor->isVisible()) {
		// If changed then notify...
		if (m_renamed > 0) {
			m_commands->push(
				new qjackctlGraphRenameCommand(this,
					m_rename_item, m_rename_editor->text()));
		}
		// Reset all renaming stuff...
		m_rename_item = nullptr;
		m_rename_editor->setEnabled(false);
		m_rename_editor->hide();
		m_renamed = 0;
	}
}


// Client/port aliases accessors.
void qjackctlGraphCanvas::setAliases ( qjackctlAliases *aliases )
{
	m_aliases = aliases;
}


qjackctlAliases *qjackctlGraphCanvas::aliases (void) const
{
	return m_aliases;
}


//----------------------------------------------------------------------------
// qjackctlGraphSect -- Generic graph driver

// Constructor.
qjackctlGraphSect::qjackctlGraphSect ( qjackctlGraphCanvas *canvas )
	: m_canvas(canvas)
{
}


// Accessors.
qjackctlGraphCanvas *qjackctlGraphSect::canvas (void) const
{
	return m_canvas;
}


// Generic sect/graph methods.
void qjackctlGraphSect::addItem ( qjackctlGraphItem *item, bool is_new )
{
	if (is_new)
		m_canvas->addItem(item);

	if (item->type() == qjackctlGraphConnect::Type) {
		qjackctlGraphConnect *connect = static_cast<qjackctlGraphConnect *> (item);
		if (connect)
			m_connects.append(connect);
	}
}


void qjackctlGraphSect::removeItem ( qjackctlGraphItem *item )
{
	if (item->type() == qjackctlGraphConnect::Type) {
		qjackctlGraphConnect *connect = static_cast<qjackctlGraphConnect *> (item);
		if (connect) {
			connect->disconnect();
			m_connects.removeAll(connect);
		}
	}

	m_canvas->removeItem(item);
}


// Clean-up all un-marked items...
void qjackctlGraphSect::resetItems ( uint node_type )
{
	const QList<qjackctlGraphConnect *> connects(m_connects);

	foreach (qjackctlGraphConnect *connect, connects) {
		if (connect->isMarked()) {
			connect->setMarked(false);
		} else {
			removeItem(connect);
			delete connect;
		}
	}

	m_canvas->resetNodes(node_type);
}


void qjackctlGraphSect::clearItems ( uint node_type )
{
	qjackctlGraphSect::resetItems(node_type);

//	qDeleteAll(m_connects);
	m_connects.clear();

	m_canvas->clearNodes(node_type);
}


// Special node finder.
qjackctlGraphNode *qjackctlGraphSect::findNode (
	const QString& name, qjackctlGraphItem::Mode mode, int type ) const
{
	return m_canvas->findNode(name, mode, type);
}


// Client/port renaming method.
void qjackctlGraphSect::renameItem (
	qjackctlGraphItem *item, const QString& name )
{
	int nchanged = 0;

	qjackctlGraphNode *node = nullptr;

	if (item->type() == qjackctlGraphNode::Type) {
		node = static_cast<qjackctlGraphNode *> (item);
		if (node) {
			node->setNodeTitle(name);
			const QString& node_title = node->nodeTitle();
			foreach (qjackctlAliasList *node_aliases, item_aliases(item)) {
				node_aliases->setClientAlias(node->nodeName(), node_title);
				++nchanged;
			}
		}
	}
	else
	if (item->type() == qjackctlGraphPort::Type) {
		qjackctlGraphPort *port = static_cast<qjackctlGraphPort *> (item);
		if (port)
			node = port->portNode();
		if (port && node) {
			port->setPortTitle(name);
			foreach (qjackctlAliasList *port_aliases, item_aliases(item)) {
				port_aliases->setPortAlias(
					node->nodeName(), port->portName(), name);
				++nchanged;
			}
		}
	}

	if (node)
		node->updatePath();

	if (nchanged > 0) {
		qjackctlAliases *aliases = nullptr;
		if (m_canvas)
			aliases = m_canvas->aliases();
		if (aliases)
			aliases->dirty = true;
	}
}


// Search item slots.
void qjackctlGraphCanvas::searchTextChanged ( const QString& text )
{
	clearSelection();

	if (text.isEmpty())
		return;

	const QRegularExpression& rx
		= QRegularExpression(text, QRegularExpression::CaseInsensitiveOption);
	if (!rx.isValid())
		return;

	for (qjackctlGraphNode *node : m_nodes) {
		if (rx.match(node->nodeTitle()).hasMatch()) {
			node->setSelected(true);
			QGraphicsView::ensureVisible(node);
		}
	}
}


void qjackctlGraphCanvas::searchEditingFinished (void)
{
	m_search_editor->setEnabled(false);
	m_search_editor->hide();
	m_search_editor->clearFocus();

	QGraphicsView::setFocus();
}


// Start search editor...
void qjackctlGraphCanvas::startSearchEditor ( const QString& text )
{
	m_search_editor->setEnabled(true);
	m_search_editor->setText(text);
	m_search_editor->raise();
	m_search_editor->show();
	m_search_editor->setFocus();
}


void qjackctlGraphCanvas::resizeEvent( QResizeEvent *event )
{
	QGraphicsView::resizeEvent(event);

	updateSearchEditor();
}


// Repel overlapping nodes...
void qjackctlGraphCanvas::setRepelOverlappingNodes ( bool on )
{
	m_repel_overlapping_nodes = on;
}


bool qjackctlGraphCanvas::isRepelOverlappingNodes (void) const
{
	return m_repel_overlapping_nodes;
}


void qjackctlGraphCanvas::repelOverlappingNodes ( qjackctlGraphNode *node,
	qjackctlGraphMoveCommand *move_command, const QPointF& delta )
{
	const qreal MIN_NODE_GAP = 8.0f;

	node->setMarked(true);

	QRectF rect1 = node->sceneBoundingRect();
	rect1.adjust(
		-2.0 * MIN_NODE_GAP, -MIN_NODE_GAP,
		+2.0 * MIN_NODE_GAP, +MIN_NODE_GAP);

	foreach (qjackctlGraphNode *node2, m_nodes) {
		if (node2->isMarked())
			continue;
		const QPointF& pos1
			= node2->pos();
		QPointF pos2 = pos1;
		const QRectF& rect2
			= node2->sceneBoundingRect();
		const QRectF& recti
			= rect2.intersected(rect1);
		if (!recti.isNull()) {
			const QPointF delta2
				= (delta.isNull() ? rect2.center() - rect1.center() : delta);
			if (recti.width() < (1.5 * recti.height())) {
				qreal dx = recti.width();
				if ((delta2.x() < 0.0 && recti.width() >= rect1.width()) ||
					(delta2.x() > 0.0 && recti.width() >= rect2.width())) {
					dx += qAbs(rect2.right() - rect1.right());
				}
				else
				if ((delta2.x() > 0.0 && recti.width() >= rect1.width()) ||
					(delta2.x() < 0.0 && recti.width() >= rect2.width())) {
					dx += qAbs(rect2.left() - rect1.left());
				}
				if (delta2.x() < 0.0)
					pos2.setX(pos1.x() - dx);
				else
					pos2.setX(pos1.x() + dx);
			} else {
				qreal dy = recti.height();
				if ((delta2.y() < 0.0 && recti.height() >= rect1.height()) ||
					(delta2.y() > 0.0 && recti.height() >= rect2.height())) {
					dy += qAbs(rect2.bottom() - rect1.bottom());
				}
				else
				if ((delta2.y() > 0.0 && recti.height() >= rect1.height()) ||
					(delta2.y() < 0.0 && recti.height() >= rect2.height())) {
					dy += qAbs(rect2.top() - rect1.top());
				}
				if (delta2.y() < 0.0)
					pos2.setY(pos1.y() - dy);
				else
					pos2.setY(pos1.y() + dy);
			}
			// Repel this node...
			node2->setPos(pos2);
			// Add this node for undo/redo...
			if (move_command)
				move_command->addItem(node2, pos1, pos2);
			// Repel this node neighbors, if any...
			repelOverlappingNodes(node2, move_command, delta2);
		}
	}

	node->setMarked(false);
}


void qjackctlGraphCanvas::repelOverlappingNodesAll (
	qjackctlGraphMoveCommand *move_command )
{
	foreach (qjackctlGraphNode *node, m_nodes)
		repelOverlappingNodes(node, move_command);
}


// Gesture event handlers.
//
bool qjackctlGraphCanvas::event ( QEvent *event )
{
	if (event->type() == QEvent::Gesture)
		return gestureEvent(static_cast<QGestureEvent *> (event));
	else
		return QGraphicsView::event(event);
}


bool qjackctlGraphCanvas::gestureEvent ( QGestureEvent *event )
{
	if (QGesture *pinch = event->gesture(Qt::PinchGesture))
		pinchGesture(static_cast<QPinchGesture *> (pinch));

	return true;
}


void qjackctlGraphCanvas::pinchGesture ( QPinchGesture *pinch )
{
	switch (pinch->state()) {
	case Qt::GestureStarted: {
		const qreal scale_factor = zoom();
		pinch->setScaleFactor(scale_factor);
		pinch->setLastScaleFactor(scale_factor);
		pinch->setTotalScaleFactor(scale_factor);
		m_gesture = true;
		break;
	}
	case Qt::GestureFinished:
		m_gesture = false;
		// Fall thru...
	case Qt::GestureUpdated:
		if (pinch->changeFlags() & QPinchGesture::ScaleFactorChanged)
			setZoom(pinch->totalScaleFactor());
		// Fall thru...
	default:
		break;
	}
}


// Bounding margins/limits...
//
const QRectF& qjackctlGraphCanvas::boundingRect ( bool reset )
{
	if (!m_rect1.isValid() || reset) {
		const QRect& rect = QGraphicsView::rect();
		const qreal mx = 0.33 * rect.width();
		const qreal my = 0.33 * rect.height();
		m_rect1 = m_scene->itemsBoundingRect()
			.marginsAdded(QMarginsF(mx, my, mx, my));
	}

	return m_rect1;
}


void qjackctlGraphCanvas::boundingPos ( QPointF& pos )
{
	const QRectF& rect = boundingRect();
	if (!rect.contains(pos)) {
		pos.setX(qBound(rect.left(), pos.x(), rect.right()));
		pos.setY(qBound(rect.top(),  pos.y(), rect.bottom()));
	}
}


// Snap into position helpers.
//
QPointF qjackctlGraphCanvas::snapPos ( const QPointF& pos ) const
{
	return QPointF(
		4.0 * ::round(0.25 * pos.x()),
		4.0 * ::round(0.25 * pos.y()));
}


//----------------------------------------------------------------------------
// qjackctlGraphThumb::View -- Thumb graphics scene/view.

class qjackctlGraphThumb::View : public QGraphicsView
{
public:

	// Constructor.
	View(qjackctlGraphThumb *thumb)
		: QGraphicsView((thumb->canvas())->viewport()),
			m_thumb(thumb), m_drag_state(DragNone)
	{
		QGraphicsView::setInteractive(false);

		QGraphicsView::setRenderHints(QPainter::Antialiasing);
		QGraphicsView::setRenderHint(QPainter::SmoothPixmapTransform);

		QGraphicsView::setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		QGraphicsView::setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

		updatePalette();

		QGraphicsView::setScene((thumb->canvas())->scene());
	}

	void updatePalette()
	{
		qjackctlGraphCanvas *canvas = m_thumb->canvas();
		QPalette pal = canvas->palette();
		const QPalette::ColorRole role
			= canvas->backgroundRole();
		const QColor& color = pal.color(role);
		pal.setColor(role, color.darker(120));
		QGraphicsView::setPalette(pal);
		QGraphicsView::setBackgroundRole(role);
	}

protected:

	// Compute the view(port) rectangle.
	QRect viewRect() const
	{
		qjackctlGraphCanvas *canvas = m_thumb->canvas();
		const QRect& vrect
			= canvas->viewport()->rect();
		const QRectF srect(
			canvas->mapToScene(vrect.topLeft()),
			canvas->mapToScene(vrect.bottomRight()));
		return QGraphicsView::viewport()->rect().intersected(QRect(
			QGraphicsView::mapFromScene(srect.topLeft()),
			QGraphicsView::mapFromScene(srect.bottomRight())))
			.adjusted(0, 0, -1, -1);
	}

	// View paint method.
	void paintEvent(QPaintEvent *event)
	{
		QGraphicsView::paintEvent(event);

		QPainter painter(QGraphicsView::viewport());
	//	const QPalette& pal = QGraphicsView::palette();
	//	painter.setPen(pal.midlight().color());
		const QRect& vrect
			= QGraphicsView::viewport()->rect();
		const QRect& vrect2 = viewRect();
		const QColor shade(0, 0, 0, 64);
		QRect rect;
		// top shade...
		rect.setTopLeft(vrect.topLeft());
		rect.setBottomRight(QPoint(vrect.right(), vrect2.top() - 1));
		if (rect.isValid())
			painter.fillRect(rect, shade);
		// left shade...
		rect.setTopLeft(QPoint(vrect.left(), vrect2.top()));
		rect.setBottomRight(vrect2.bottomLeft());
		if (rect.isValid())
			painter.fillRect(rect, shade);
		// right shade...
		rect.setTopLeft(vrect2.topRight());
		rect.setBottomRight(QPoint(vrect.right(), vrect2.bottom()));
		if (rect.isValid())
			painter.fillRect(rect, shade);
		// bottom shade...
		rect.setTopLeft(QPoint(vrect.left(), vrect2.bottom() + 1));
		rect.setBottomRight(vrect.bottomRight());
		if (rect.isValid())
			painter.fillRect(rect, shade);
	}

	// Handle mouse events.
	//
	void mousePressEvent(QMouseEvent *event)
	{
		QGraphicsView::mousePressEvent(event);

		if (event->button() == Qt::LeftButton) {
			m_drag_pos = event->pos();
			m_drag_state = DragStart;
			QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
		}
	}

	void mouseMoveEvent(QMouseEvent *event)
	{
		QGraphicsView::mouseMoveEvent(event);

		if (m_drag_state == DragStart
			&& (event->pos() - m_drag_pos).manhattanLength()
				> QApplication::startDragDistance()) {
			m_drag_state = DragMove;
			QApplication::changeOverrideCursor(QCursor(Qt::DragMoveCursor));
		}

		if (m_drag_state == DragMove) {
			const QRect& rect = QGraphicsView::rect();
			if (!rect.contains(event->pos())) {
				const int mx = rect.width()  + 4;
				const int my = rect.height() + 4;
				const Position position = m_thumb->position();
				if (event->pos().x() < rect.left() - mx) {
					if (position == TopRight)
						m_thumb->requestPosition(TopLeft);
					else
					if (position == BottomRight)
						m_thumb->requestPosition(BottomLeft);
				}
				else
				if (event->pos().x() > rect.right() + mx) {
					if (position == TopLeft)
						m_thumb->requestPosition(TopRight);
					else
					if (position == BottomLeft)
						m_thumb->requestPosition(BottomRight);
				}
				else
				if (event->pos().y() < rect.top() - my) {
					if (position == BottomLeft)
						m_thumb->requestPosition(TopLeft);
					else
					if (position == BottomRight)
						m_thumb->requestPosition(TopRight);
				}
				else
				if (event->pos().y() > rect.bottom() + my) {
					if (position == TopLeft)
						m_thumb->requestPosition(BottomLeft);
					else
					if (position == TopRight)
						m_thumb->requestPosition(BottomRight);
				}
			}
			else
			if (event->modifiers() & Qt::ControlModifier) {
				m_thumb->canvas()->centerOn(
					QGraphicsView::mapToScene(event->pos()));
			}
		}
	}

	void mouseReleaseEvent(QMouseEvent *event)
	{
		QGraphicsView::mouseReleaseEvent(event);

		if (m_drag_state != DragNone) {
			if ((m_drag_state == DragStart) ||
				(event->modifiers() & Qt::ControlModifier)) {
				m_thumb->canvas()->centerOn(
					QGraphicsView::mapToScene(event->pos()));
			}
			m_drag_state = DragNone;
			QApplication::restoreOverrideCursor();
		}
	}

	void wheelEvent(QWheelEvent *) {} // Ignore wheel events.

	void contextMenuEvent(QContextMenuEvent *event)
	{
		m_thumb->requestContextMenu(event->globalPos());
	}

private:

	// Instance members.
	qjackctlGraphThumb *m_thumb;

	enum { DragNone = 0, DragStart, DragMove } m_drag_state;

	QPoint m_drag_pos;
};


//----------------------------------------------------------------------------
// qjackctlGraphThumb -- Thumb graphics scene/view.

// Constructor.
qjackctlGraphThumb::qjackctlGraphThumb ( qjackctlGraphCanvas *canvas, Position position )
	: QFrame(canvas), m_canvas(canvas), m_position(position), m_view(nullptr)
{
	m_view = new View(this);

	QVBoxLayout *layout = new QVBoxLayout();
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_view);
	QFrame::setLayout(layout);

	QFrame::setFrameStyle(QFrame::Panel);
	QFrame::setForegroundRole(QPalette::Window);

	QObject::connect(m_canvas->horizontalScrollBar(),
		SIGNAL(valueChanged(int)), SLOT(updateView()));
	QObject::connect(m_canvas->verticalScrollBar(),
		SIGNAL(valueChanged(int)), SLOT(updateView()));
}


// Destructor.
qjackctlGraphThumb::~qjackctlGraphThumb (void)
{
}


// Accessors.
qjackctlGraphCanvas *qjackctlGraphThumb::canvas (void) const
{
	return m_canvas;
}


void qjackctlGraphThumb::setPosition ( Position position )
{
	m_position = position;

	updatePosition();
}


qjackctlGraphThumb::Position qjackctlGraphThumb::position (void) const
{
	return m_position;
}


// Request re-positioning.
void qjackctlGraphThumb::requestPosition ( Position position )
{
	emit positionRequested(int(position));
}


// Emit context-menu request.
void qjackctlGraphThumb::requestContextMenu ( const QPoint& pos )
{
	emit contextMenuRequested(pos);
}


// Update view.
void qjackctlGraphThumb::updatePosition (void)
{
	const QRect& rect = m_canvas->viewport()->rect();
	const int w = rect.width()  / 4;
	const int h = rect.height() / 4;
	QFrame:setFixedSize(w + 1, h + 1); // 1px slack.

	switch (m_position) {
	case TopLeft:
		QFrame::move(0, 0);
		break;
	case TopRight:
		QFrame::move(rect.width() - w, 0);
		break;
	case BottomLeft:
		QFrame::move(0, rect.height() - h);
		break;
	case BottomRight:
		QFrame::move(rect.width() - w, rect.height() - h);
		break;
	case None:
	default:
		break;
	}

	QFrame::show();
}


// Update view.
void qjackctlGraphThumb::updateView (void)
{
	updatePosition();

	const qreal m = 24.0;
	m_view->fitInView(
		m_canvas->scene()->itemsBoundingRect()
			.marginsAdded(QMarginsF(m, m, m, m)),
		Qt::KeepAspectRatio);
}


// Update the thumb-view palette.
void qjackctlGraphThumb::updatePalette (void)
{
	m_view->updatePalette();
}


// Search placeholder text accessors.
void qjackctlGraphCanvas::setSearchPlaceholderText ( const QString& text )
{
	m_search_editor->setPlaceholderText(text);
}

QString qjackctlGraphCanvas::searchPlaceholderText (void) const
{
	return m_search_editor->placeholderText();
}


// Update the canvas palette.
void qjackctlGraphCanvas::updatePalette (void)
{
	QPalette pal;// = QGraphicsView::palette();
	const QPalette::ColorRole role = QPalette::Window;
	const QColor& color = pal.color(role);
	pal.setColor(role, color.darker(120));
	QGraphicsView::setPalette(pal);
	QGraphicsView::setBackgroundRole(role);
}


// end of qjackctlGraph.cpp
