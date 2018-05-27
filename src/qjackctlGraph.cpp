// qjackctlGraph.cpp
//
/****************************************************************************
   Copyright (C) 2003-2018, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include <QGraphicsScene>

#include <QGraphicsDropShadowEffect>

#include <QStyleOptionGraphicsItem>

#include <QTransform>
#include <QRubberBand>
#include <QUndoStack>
#include <QSettings>

#include <QPainter>
#include <QPalette>

#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

#include <algorithm>

#include <math.h>


//----------------------------------------------------------------------------
// qjackctlGraphItem -- Base graphics item.

// Constructor.
qjackctlGraphItem::qjackctlGraphItem ( QGraphicsItem *parent )
	: QGraphicsPathItem(parent), m_marked(false)
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


// Item-type hash (static)
int qjackctlGraphItem::itemType ( const QByteArray& type_name )
{
	return qHash(type_name);
}


//----------------------------------------------------------------------------
// qjackctlGraphPort -- Port graphics item.

// Constructor.
qjackctlGraphPort::qjackctlGraphPort ( qjackctlGraphNode *node,
	const QString& name, qjackctlGraphItem::Mode mode, int type )
	: qjackctlGraphItem(node), m_node(node),
		m_name(name), m_mode(mode), m_type(type), m_selectx(0)
{
	QGraphicsPathItem::setZValue(+1);

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


void qjackctlGraphPort::setPortType ( int type )
{
	m_type = type;
}


int qjackctlGraphPort::portType (void) const
{
	return m_type;
}


void qjackctlGraphPort::setPortTitle ( const QString& title )
{
	m_text->setPlainText(title);

	QPainterPath path;
	const QRectF& rect
		= m_text->boundingRect();
	path.addRoundedRect(rect, 5, 5);
	QGraphicsPathItem::setPath(path);
}


QString qjackctlGraphPort::portTitle (void) const
{
	return m_text->toPlainText();
}


QPointF qjackctlGraphPort::portPos (void) const
{
	QPointF pos = QGraphicsPathItem::scenePos();

	const QRectF& rect
		= QGraphicsPathItem::boundingRect();
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
			connect->setPort1(0);
		if (connect->port2() != this)
			connect->setPort2(0);
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

	return NULL;
}


void qjackctlGraphPort::paint ( QPainter *painter,
	const QStyleOptionGraphicsItem *option, QWidget */*widget*/ )
{
	if (QGraphicsPathItem::isSelected()) {
		const QPalette& pal = option->palette;
		m_text->setDefaultTextColor(pal.highlightedText().color());
		painter->setPen(pal.highlightedText().color());
		painter->setBrush(pal.highlight().color());
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
		if (QGraphicsPathItem::isUnderMouse()) {
			painter->setPen(foreground.lighter());
			painter->setBrush(background.lighter());
		} else {
			painter->setPen(foreground);
			painter->setBrush(background);
		}
	}

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
		foreach (qjackctlGraphConnect *connect, m_connects)
			connect->setSelectedEx(this, is_selected);
		if (!is_selected)
			m_node->setSelectedEx(false);
	}

	return value;
}


// Selection propagation method...
void qjackctlGraphPort::setSelectedEx ( bool is_selected )
{
	++m_selectx;

	if (QGraphicsPathItem::isSelected() != is_selected)
		QGraphicsPathItem::setSelected(is_selected);

	--m_selectx;
}


// Natural decimal sorting comparator (static)
bool qjackctlGraphPort::lessThan ( qjackctlGraphPort *port1, qjackctlGraphPort *port2 )
{
	const int port_type_diff
		= port1->portType() - port2->portType();
	if (port_type_diff)
		return (port_type_diff > 0);

	const QString& s1 = port1->portName();
	const QString& s2 = port2->portName();

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


//----------------------------------------------------------------------------
// qjackctlGraphNode -- Node graphics item.

// Constructor.
qjackctlGraphNode::qjackctlGraphNode (
	const QString& name, qjackctlGraphItem::Mode mode, int type )
	: qjackctlGraphItem(NULL),
		m_name(name), m_mode(mode), m_type(type), m_selectx(0)
{
	QGraphicsPathItem::setZValue(0);

	const QPalette pal;
	qjackctlGraphItem::setForeground(pal.text().color().darker());
	qjackctlGraphItem::setBackground(pal.window().color());

	m_pixmap = new QGraphicsPixmapItem(this);
	m_text = new QGraphicsTextItem(this);

	QGraphicsPathItem::setFlag(QGraphicsItem::ItemIsMovable);
	QGraphicsPathItem::setFlag(QGraphicsItem::ItemIsSelectable);

	QGraphicsPathItem::setToolTip(m_name);
	setNodeTitle(m_name);

	const bool is_dark = (pal.base().color().value() < 24);
	QColor shadow_color = (is_dark ? Qt::white : Qt::black);
	shadow_color.setAlpha(180);

	QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
	effect->setColor(shadow_color);
	effect->setBlurRadius(is_dark ? 8 : 16);
	effect->setOffset(is_dark ? 0 : 2);
	QGraphicsPathItem::setGraphicsEffect(effect);
}


// Destructor.
qjackctlGraphNode::~qjackctlGraphNode (void)
{
	removePorts();

	// No actual need to destroy any children here...
	//
	//	QGraphicsPathItem::setGraphicsEffect(0);

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


void qjackctlGraphNode::setNodeType ( int type )
{
	m_type = type;
}


int qjackctlGraphNode::nodeType (void) const
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
	m_text->setPlainText(title);
}


QString qjackctlGraphNode::nodeTitle (void) const
{
	return m_text->toPlainText();
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
	const QString& name, qjackctlGraphItem::Mode mode, int type )
{
	return static_cast<qjackctlGraphPort *> (
		m_portkeys.value(qjackctlGraphPort::ItemKey(name, mode, type), NULL));
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
		const int w = port->boundingRect().width();
		if (port->isOutput()) {
			if (wo < w) wo = w;
		} else {
			if (wi < w) wi = w;
		}
	}
	width = wi + wo;

	std::sort(m_ports.begin(), m_ports.end(), qjackctlGraphPort::Compare());

	int height = rect.height() + 6;
	int type = 0;
	int yi, yo;
	yi = yo = height;
	foreach (qjackctlGraphPort *port, m_ports) {
		const QRectF& port_rect = port->boundingRect();
		const int w = port_rect.width();
		const int h = port_rect.height() + 2;
		if (type - port->portType()) {
			type = port->portType();
			height += 4;
			yi = yo = height;
		}
		if (port->isOutput()) {
			port->setPos(+width / 2 + 6 - w, yo);
			yo += h;
			if (height < yo)
				height = yo;
		} else {
			port->setPos(-width / 2 - 6, yi);
			yi += h;
			if (height < yi)
				height = yi;
		}
	}

	QPainterPath path;
	path.addRoundedRect(-width / 2, 0, width, height + 6, 5, 5);
	QGraphicsPathItem::setPath(path);
}


void qjackctlGraphNode::paint ( QPainter *painter,
	const QStyleOptionGraphicsItem *option, QWidget */*widget*/ )
{
	if (QGraphicsPathItem::isSelected()) {
		const QPalette& pal = option->palette;
		m_text->setDefaultTextColor(pal.highlightedText().color());
		painter->setPen(pal.highlightedText().color());
		painter->setBrush(pal.highlight().color());
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
		painter->setBrush(background);
	}

	painter->drawPath(QGraphicsPathItem::path());

	const QRectF& node_rect = QGraphicsPathItem::boundingRect();
	m_pixmap->setPos(node_rect.x() + 4, node_rect.y() + 4);

	const QRectF& text_rect = m_text->boundingRect();
	m_text->setPos(- text_rect.width() / 2, text_rect.y() + 2);
}


QVariant qjackctlGraphNode::itemChange (
	GraphicsItemChange change, const QVariant& value )
{
	if (change == QGraphicsItem::ItemSelectedHasChanged && m_selectx < 1) {
		const bool is_selected = value.toBool();
		foreach (qjackctlGraphPort *port, m_ports)
			port->setSelected(is_selected);
	}

	return value;
}


// Selection propagation method...
void qjackctlGraphNode::setSelectedEx ( bool is_selected )
{
	++m_selectx;

	if (QGraphicsPathItem::isSelected() != is_selected)
		QGraphicsPathItem::setSelected(is_selected);

	--m_selectx;
}



//----------------------------------------------------------------------------
// qjackctlGraphConnect -- Connection-line graphics item.

// Constructor.
qjackctlGraphConnect::qjackctlGraphConnect (void)
	: qjackctlGraphItem(NULL), m_port1(NULL), m_port2(NULL)
{
	QGraphicsPathItem::setZValue(-1);

	QGraphicsPathItem::setFlag(QGraphicsItem::ItemIsSelectable);

	qjackctlGraphItem::setBackground(qjackctlGraphItem::foreground());

	const QPalette pal;
	const bool is_dark = (pal.base().color().value() < 24);
	QColor shadow_color = (is_dark ? Qt::white : Qt::black);
	shadow_color.setAlpha(220);

	QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
	effect->setColor(shadow_color);
	effect->setBlurRadius(is_dark ? 4 : 8);
	effect->setOffset(is_dark ? 0 : 1);
	QGraphicsPathItem::setGraphicsEffect(effect);

	QGraphicsPathItem::setAcceptHoverEvents(true);
}


// Destructor.
qjackctlGraphConnect::~qjackctlGraphConnect (void)
{
	if (m_port1)
		m_port1->removeConnect(this);
	if (m_port2)
		m_port2->removeConnect(this);

	// No actual need to destroy any children here...
	//
	//	QGraphicsPathItem::setGraphicsEffect(0);
}


// Accessors.
void qjackctlGraphConnect::setPort1 ( qjackctlGraphPort *port )
{
	if (m_port1)
		m_port1->removeConnect(this);

	m_port1 = port;

	if (m_port1)
		m_port1->appendConnect(this);
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
}


qjackctlGraphPort *qjackctlGraphConnect::port2 (void) const
{
	return m_port2;
}


// Path/shaper updaters.
void qjackctlGraphConnect::updatePathTo ( const QPointF& pos )
{
	const bool is_out0 = m_port1->isOutput();
	const QPointF pos0 = m_port1->portPos();
	const QPointF pos1 = (is_out0 ? pos0 : pos);
	const QPointF pos4 = (is_out0 ? pos : pos0);
	const qreal dx = pos4.x() - pos1.x();
	const qreal dy = pos4.y() - pos1.y();
	const QRectF& rect = m_port1->portNode()->boundingRect();
	const qreal y_max = rect.height() + rect.width();
	const qreal y_min = qMin(y_max, qAbs(dx));
	const qreal x_offset = (dx > 0.0 ? 0.5 : 1.0) * y_min;
	const qreal y_offset = (dx > 0.0 ? 0.0 : (dy > 0.0 ? +y_min : -y_min));
	const QPointF pos2(pos1.x() + x_offset, pos1.y() + y_offset);
	const QPointF pos3(pos4.x() - x_offset, pos4.y() + y_offset);

	QPainterPath path;
	path.moveTo(pos1);
	path.cubicTo(pos2, pos3, pos4);
	const qreal arrow_angle = path.angleAtPercent(0.5) * M_PI / 180.0;
	const QPointF arrow_pos0 = path.pointAtPercent(0.5);
	path.cubicTo(pos3, pos2, pos1);
	const qreal arrow_size = 12.0;
	QVector<QPointF> arrow;
	arrow.append(arrow_pos0);
	arrow.append(arrow_pos0 - QPointF(
		::sin(arrow_angle + M_PI / 2.3) * arrow_size,
		::cos(arrow_angle + M_PI / 2.3) * arrow_size));
	arrow.append(arrow_pos0 - QPointF(
		::sin(arrow_angle + M_PI - M_PI / 2.3) * arrow_size,
		::cos(arrow_angle + M_PI - M_PI / 2.3) * arrow_size));
	arrow.append(arrow_pos0);
	path.addPolygon(QPolygonF(arrow));

	QGraphicsPathItem::setPath(path);
}


void qjackctlGraphConnect::updatePath (void)
{
	updatePathTo(m_port2->portPos());
}


void qjackctlGraphConnect::paint ( QPainter *painter,
	const QStyleOptionGraphicsItem *option, QWidget */*widget*/ )
{
	if (QGraphicsPathItem::isSelected()) {
		const QPalette& pal
			= option->palette;
		const QColor& color
			= pal.highlight().color();
		painter->setPen(QPen(color, 2));
		painter->setBrush(color);
	} else {
		const QColor& color
			= qjackctlGraphItem::foreground();
		if (QGraphicsPathItem::isUnderMouse())
			painter->setPen(color.lighter());
		else
			painter->setPen(color);
	}
	painter->setBrush(qjackctlGraphItem::background());

	painter->drawPath(QGraphicsPathItem::path());
}


QVariant qjackctlGraphConnect::itemChange (
	GraphicsItemChange change, const QVariant& value )
{
	if (change == QGraphicsItem::ItemSelectedHasChanged) {
		const bool is_selected = value.toBool();
		if (m_port1)
			m_port1->setSelectedEx(is_selected);
		if (m_port2)
			m_port2->setSelectedEx(is_selected);
	}

	return value;
}


// Selection propagation method...
void qjackctlGraphConnect::setSelectedEx ( qjackctlGraphPort *port, bool is_selected )
{
	if (QGraphicsPathItem::isSelected() != is_selected) {
		QGraphicsPathItem::setSelected(is_selected);
		if (m_port1 && m_port1 != port)
			m_port1->setSelectedEx(is_selected);
		if (m_port2 && m_port2 != port)
			m_port2->setSelectedEx(is_selected);
	}
}


//----------------------------------------------------------------------------
// qjackctlGraphCommand -- Generic graph command pattern

// Constructor.
qjackctlGraphCommand::qjackctlGraphCommand ( qjackctlGraphCanvas *canvas,
	qjackctlGraphPort *port1, qjackctlGraphPort *port2, bool is_connect,
	QUndoCommand *parent ) : QUndoCommand(parent),
		m_item(canvas, port1, port2, is_connect)
{
}


// Command methods.
void qjackctlGraphCommand::undo (void)
{
	execute(true);
}


void qjackctlGraphCommand::redo (void)
{
	execute(false);
}


// Command executive
bool qjackctlGraphCommand::execute ( bool is_undo )
{
	qjackctlGraphCanvas *canvas = m_item.canvas();
	if (canvas == NULL)
		return false;

	qjackctlGraphNode *node1
		= canvas->findNode(
			m_item.addr1.node_name,
			qjackctlGraphItem::Duplex,
			m_item.addr1.node_type);
	if (node1 == NULL)
		node1 = canvas->findNode(
			m_item.addr1.node_name,
			qjackctlGraphItem::Output,
			m_item.addr1.node_type);
	if (node1 == NULL)
		return false;

	qjackctlGraphPort *port1
		= node1->findPort(
			m_item.addr1.port_name,
			qjackctlGraphItem::Output,
			m_item.addr1.port_type);
	if (port1 == NULL)
		return false;

	qjackctlGraphNode *node2
		= canvas->findNode(
			m_item.addr2.node_name,
			qjackctlGraphItem::Duplex,
			m_item.addr2.node_type);
	if (node2 == NULL)
		node2 = canvas->findNode(
			m_item.addr2.node_name,
			qjackctlGraphItem::Input,
			m_item.addr2.node_type);
	if (node2 == NULL)
		return false;

	qjackctlGraphPort *port2
		= node2->findPort(
			m_item.addr2.port_name,
			qjackctlGraphItem::Input,
			m_item.addr2.port_type);
	if (port2 == NULL)
		return false;

	const bool is_connect
		= (m_item.is_connect() && !is_undo) || (!m_item.is_connect() && is_undo);
	if (is_connect)
		canvas->emitConnected(port1, port2);
	else
		canvas->emitDisconnected(port1, port2);

	return true;
}


//----------------------------------------------------------------------------
// qjackctlGraphCanvas -- Canvas graphics scene/view.

// Local constants.
static const char *CanvasGroup   = "/GraphCanvas";
static const char *CanvasRectKey = "/CanvasRect";
static const char *CanvasZoomKey = "/CanvasZoom";

static const char *NodePosGroup  = "/GraphNodePos";


// Constructor.
qjackctlGraphCanvas::qjackctlGraphCanvas ( QWidget *parent )
	: QGraphicsView(parent), m_state(DragNone), m_item(NULL),
		m_connect(NULL), m_rubberband(NULL),
		m_zoom(1.0), m_zoomrange(false),
		m_commands(NULL), m_settings(NULL)
{
	m_scene = new QGraphicsScene();

	m_commands = new QUndoStack();

	QGraphicsView::setScene(m_scene);

	QGraphicsView::setRenderHint(QPainter::Antialiasing);
	QGraphicsView::setRenderHint(QPainter::SmoothPixmapTransform);

	QGraphicsView::setResizeAnchor(QGraphicsView::NoAnchor);
	QGraphicsView::setDragMode(QGraphicsView::NoDrag);
}


// Destructor.
qjackctlGraphCanvas::~qjackctlGraphCanvas (void)
{
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
			if (!restoreNodePos(node))
				emit added(node);
		}
	}
}


void qjackctlGraphCanvas::removeItem ( qjackctlGraphItem *item )
{
	if (item->type() == qjackctlGraphNode::Type) {
		qjackctlGraphNode *node = static_cast<qjackctlGraphNode *> (item);
		if (node) {
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
		if (item->type() == qjackctlGraphConnect::Type)
			return true;
	}

	return false;
}


// Zooming methods.
void qjackctlGraphCanvas::setZoom ( qreal zoom )
{
	if (zoom < 0.1)
		zoom = 0.1;
	else
	if (zoom > 2.0)
		zoom = 2.0;

	const qreal scale = zoom / m_zoom;
	QGraphicsView::scale(scale, scale);
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
void qjackctlGraphCanvas::resetNodes ( int node_type )
{
	QList<qjackctlGraphNode *> nodes;

	foreach (qjackctlGraphNode *node, m_nodes) {
		if (node->nodeType() == node_type) {
			if (node->isMarked()) {
				node->resetMarkedPorts();
				node->setMarked(false);
			} else {
				removeItem(node);
				nodes.append(node);
			}
		}
	}

	qDeleteAll(nodes);
}


void qjackctlGraphCanvas::clearNodes ( int node_type )
{
	QList<qjackctlGraphNode *> nodes;

	foreach (qjackctlGraphNode *node, m_nodes) {
		if (node->nodeType() == node_type) {
			m_nodekeys.remove(qjackctlGraphNode::NodeKey(node));
			m_nodes.removeAll(node);
			nodes.append(node);
		}
	}

	qDeleteAll(nodes);
}


// Special node finder.
qjackctlGraphNode *qjackctlGraphCanvas::findNode (
	const QString& name, qjackctlGraphItem::Mode mode, int type ) const
{
	return static_cast<qjackctlGraphNode *> (
		m_nodekeys.value(qjackctlGraphNode::ItemKey(name, mode, type), NULL));
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


// Item finder (internal).
qjackctlGraphItem *qjackctlGraphCanvas::itemAt ( const QPointF& pos ) const
{
	const QList<QGraphicsItem *>& items
		= m_scene->items(QRectF(pos - QPointF(2, 2), QSizeF(5, 5)));

	foreach (QGraphicsItem *item, items) {
		if (item->type() >= QGraphicsItem::UserType)
			return static_cast<qjackctlGraphItem *> (item);
	}

	return NULL;
}


// Port (dis)connection command.
void qjackctlGraphCanvas::connectPorts (
	qjackctlGraphPort *port1, qjackctlGraphPort *port2, bool is_connect )
{
	if (port1->isOutput())
		m_commands->push(new qjackctlGraphCommand(this, port1, port2, is_connect));
	else
		m_commands->push(new qjackctlGraphCommand(this, port2, port1, is_connect));
}


// Mouse event handlers.
void qjackctlGraphCanvas::mousePressEvent ( QMouseEvent *event )
{
	m_state = DragNone;
	m_item = NULL;

	if (event->button() == Qt::LeftButton) {
		m_state = DragStart;
		m_pos = QGraphicsView::mapToScene(event->pos());
		qjackctlGraphItem *item = itemAt(m_pos);
		if (item && item->type() >= QGraphicsItem::UserType) {
			m_item = static_cast<qjackctlGraphItem *> (item);
		}
	}

	if (m_state == DragStart && m_item == NULL
		&& (event->modifiers() & Qt::ControlModifier)) {
		QGraphicsView::setDragMode(QGraphicsView::ScrollHandDrag);
		QGraphicsView::mousePressEvent(event);
		m_state = DragScroll;
	}
}


void qjackctlGraphCanvas::mouseMoveEvent ( QMouseEvent *event )
{
	int nchanged = 0;

	const QPointF& pos
		= QGraphicsView::mapToScene(event->pos());

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
						m_scene->clearSelection();
						m_connect = new qjackctlGraphConnect();
						m_connect->setPort1(port);
						m_connect->setSelected(true);
						m_scene->addItem(m_connect);
						m_item = NULL;
						++nchanged;
					}
				}
				else
				// Start moving nodes around...
				if (m_item->type() == qjackctlGraphNode::Type) {
					QGraphicsView::setCursor(Qt::SizeAllCursor);
					if (!m_item->isSelected()) {
						if ((event->modifiers()
							& (Qt::ShiftModifier | Qt::ControlModifier)) == 0)
							m_scene->clearSelection();
						m_item->setSelected(true);
						++nchanged;
					}
				}
				else m_item = NULL;
			}
			// Otherwise start lasso rubber-banding...
			if (m_rubberband == NULL && m_item == NULL && m_connect == NULL) {
				QGraphicsView::setCursor(Qt::CrossCursor);
				m_rubberband = new QRubberBand(QRubberBand::Rectangle, this);
			}
		}
		break;
	case DragMove:
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
				if ((event->modifiers()
					& (Qt::ControlModifier | Qt::ShiftModifier)) == 0) {
					m_scene->clearSelection();
					++nchanged;
				}
				const QRectF range_rect(m_pos, pos);
				foreach (QGraphicsItem *item,
						m_scene->items(range_rect.normalized())) {
					if (item->type() >= QGraphicsItem::UserType) {
						item->setSelected(true);
						++nchanged;
					}
				}
			}
		}
		// Move current selected nodes...
		if (m_item && m_item->type() == qjackctlGraphNode::Type) {
			QPointF pos2 = pos;
			pos2.setX(4.0 * ::round(0.25 * pos2.x()));
			pos2.setY(4.0 * ::round(0.25 * pos2.y()));
			const QPointF delta = (pos2 - m_pos);
			foreach (QGraphicsItem *item, m_scene->selectedItems()) {
				if (item->type() == qjackctlGraphNode::Type) {
					qjackctlGraphNode *node = static_cast<qjackctlGraphNode *> (item);
					if (node)
						node->setPos(node->pos() + delta);
				}
			}
			m_pos = pos2;
		}
		break;
	case DragScroll:
	default:
		QGraphicsView::mouseMoveEvent(event);
		break;
	}

	if (nchanged > 0)
		emit changed();
}


void qjackctlGraphCanvas::mouseReleaseEvent ( QMouseEvent *event )
{
	int nchanged = 0;

	switch (m_state) {
	case DragStart:
		// Make individual item (de)selections...
		if ((event->modifiers()
			& (Qt::ShiftModifier | Qt::ControlModifier)) == 0) {
			m_scene->clearSelection();
			++nchanged;
		}
		if (m_item) {
			if (event->modifiers() & Qt::ControlModifier)
				m_item->setSelected(!m_item->isSelected());
			else
				m_item->setSelected(true);
			++nchanged;
		}
		// Fall thru...
	case DragMove:
		// Close new connection line...
		if (m_connect) {
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
					&& port1->findConnect(port2) == NULL) {
				#if 0 // Sure the sect will commit to this instead...
					port2->setSelected(true);
					m_connect->setPort2(port2);
					m_connect->updatePathTo(port2->portPos());
					m_connect = NULL;
				#else
					m_scene->clearSelection();
				#endif
					// Submit command; notify eventual observers...
					m_commands->beginMacro(tr("Connect"));
					connectPorts(port1, port2, true);
					m_commands->endMacro();
					++nchanged;
				}
			}
			if (m_connect) {
				delete m_connect;
				m_connect = NULL;
			}
		}
		// Maybe some node(s) were moved...
		if (m_item && m_item->type() == qjackctlGraphNode::Type)
			++nchanged;
		// Close rubber-band lasso...
		if (m_rubberband) {
			delete m_rubberband;
			m_rubberband = NULL;
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
		QGraphicsView::mouseReleaseEvent(event);
		QGraphicsView::setDragMode(QGraphicsView::NoDrag);
		break;
	}

	m_state = DragNone;
	m_item = NULL;

	// Reset cursor...
	QGraphicsView::setCursor(Qt::ArrowCursor);

	if (nchanged > 0)
		emit changed();
}


void qjackctlGraphCanvas::mouseDoubleClickEvent ( QMouseEvent *event )
{
	const QPointF& pos
		= QGraphicsView::mapToScene(event->pos());
	QGraphicsView::centerOn(pos);
}


void qjackctlGraphCanvas::wheelEvent ( QWheelEvent *event )
{
	if (event->modifiers() & Qt::ControlModifier) {
		const int delta
		#if QT_VERSION < 0x050000
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
		if (m_rubberband) {
			delete m_rubberband;
			m_rubberband = NULL;
		}
		if (m_connect) {
			delete m_connect;
			m_connect = NULL;
		}
		if (m_state == DragScroll)
			QGraphicsView::setDragMode(QGraphicsView::NoDrag);
		m_state = DragNone;
		m_item = NULL;
		emit changed();
		// Reset cursor...
		QGraphicsView::setCursor(Qt::ArrowCursor);
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

	m_scene->clearSelection();

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
		// Submit command; notify eventual observers...
		qjackctlGraphPort *port1 = iter1.next();
		qjackctlGraphPort *port2 = iter2.next();
		if (port1 && port2 && port1->portType() == port2->portType())
			connectPorts(port1, port2, true);
	}

	m_commands->endMacro();
}


// Disconnect selected items.
void qjackctlGraphCanvas::disconnectItems (void)
{
	QList<qjackctlGraphConnect *> connects;

	foreach (QGraphicsItem *item, m_scene->selectedItems()) {
		if (item->type() == qjackctlGraphConnect::Type) {
			qjackctlGraphConnect *connect = static_cast<qjackctlGraphConnect *> (item);
			if (connect)
				connects.append(connect);
		}
	}

	m_scene->clearSelection();

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
	}

	emit changed();
}


void qjackctlGraphCanvas::selectNone (void)
{
	m_scene->clearSelection();

	emit changed();
}


void qjackctlGraphCanvas::selectInvert (void)
{
	foreach (QGraphicsItem *item, m_scene->items()) {
		if (item->type() == qjackctlGraphNode::Type)
			item->setSelected(!item->isSelected());
	}

	emit changed();
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
bool qjackctlGraphCanvas::restoreNodePos ( qjackctlGraphNode *node )
{
	if (m_settings == NULL || node == NULL)
		return false;

	m_settings->beginGroup(NodePosGroup);
	const QPointF& node_pos
		= m_settings->value('/' + nodeKey(node)).toPointF();
	m_settings->endGroup();

	if (node_pos.isNull())
		return false;

	node->setPos(node_pos);
	return true;
}


bool qjackctlGraphCanvas::saveNodePos ( qjackctlGraphNode *node ) const
{
	if (m_settings == NULL || node == NULL)
		return false;

	m_settings->beginGroup(NodePosGroup);
	m_settings->setValue('/' + nodeKey(node), node->pos());
	m_settings->endGroup();

	return true;
}


bool qjackctlGraphCanvas::restoreState (void)
{
	if (m_settings == NULL)
		return false;

	m_settings->beginGroup(CanvasGroup);
	m_settings->setValue(CanvasRectKey, QGraphicsView::sceneRect());
	const QRectF& rect = m_settings->value(CanvasRectKey).toRectF();
	const qreal zoom = m_settings->value(CanvasZoomKey).toReal();
	m_settings->endGroup();

	if (rect.isValid())
		QGraphicsView::setSceneRect(rect);

	setZoom(zoom);

	return true;
}


bool qjackctlGraphCanvas::saveState (void) const
{
	if (m_settings == NULL)
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
	m_settings->setValue(CanvasRectKey, QGraphicsView::sceneRect());
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
		// Fall thru...
	default:
		break;
	}

	return node_key;
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
void qjackctlGraphSect::addItem ( qjackctlGraphItem *item )
{
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
		if (connect)
			m_connects.removeAll(connect);
	}

	m_canvas->removeItem(item);
}


// Clean-up all un-marked items...
void qjackctlGraphSect::resetItems ( int node_type )
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


void qjackctlGraphSect::clearItems ( int node_type )
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


// end of qjackctlGraph.cpp
