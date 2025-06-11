// qjackctlGraphForm.cpp
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

#include "qjackctlGraphForm.h"

#include "qjackctlJackGraph.h"
#include "qjackctlAlsaGraph.h"

#include "qjackctlMainForm.h"

#include <QMenu>

#include <QMessageBox>

#include <QHBoxLayout>
#include <QToolButton>
#include <QSlider>
#include <QSpinBox>

#include <QColorDialog>
#include <QActionGroup>
#include <QUndoStack>

#include <QResizeEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QCloseEvent>

#include <cmath>


//----------------------------------------------------------------------------
// qjackctlGraphZoomSlider -- Custom slider widget.

#include <QMouseEvent>

class qjackctlGraphZoomSlider : public QSlider
{
public:

	qjackctlGraphZoomSlider() : QSlider(Qt::Horizontal)
	{
		QSlider::setMinimum(10);
		QSlider::setMaximum(190);
		QSlider::setTickInterval(90);
		QSlider::setTickPosition(QSlider::TicksBothSides);
	}

protected:

	void mousePressEvent(QMouseEvent *ev)
	{
		QSlider::mousePressEvent(ev);

		if (ev->button() == Qt::MiddleButton)
			QSlider::setValue(100);
	}
};


//----------------------------------------------------------------------------
// qjackctlGraphForm -- UI wrapper form.

// Constructor.
qjackctlGraphForm::qjackctlGraphForm (
	QWidget *parent, Qt::WindowFlags wflags )
	: QMainWindow(parent, wflags), m_config(nullptr), m_jack(nullptr), m_alsa(nullptr)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	m_jack_changed = 0;
	m_alsa_changed = 0;

	m_ins = m_mids = m_outs = 0;

	m_repel_overlapping_nodes = 0;

	m_thumb = nullptr;
	m_thumb_update = 0;

	QUndoStack *commands = m_ui.graphCanvas->commands();

	QAction *undo_action = commands->createUndoAction(this, tr("&Undo"));
	undo_action->setIcon(QIcon(":/images/graphUndo.png"));
	undo_action->setStatusTip(tr("Undo last edit action"));
	undo_action->setShortcuts(QKeySequence::Undo);

	QAction *redo_action = commands->createRedoAction(this, tr("&Redo"));
	redo_action->setIcon(QIcon(":/images/graphRedo.png"));
	redo_action->setStatusTip(tr("Redo last edit action"));
	redo_action->setShortcuts(QKeySequence::Redo);

	QAction *before_action = m_ui.editSelectAllAction;
	m_ui.editMenu->insertAction(before_action, undo_action);
	m_ui.editMenu->insertAction(before_action, redo_action);
	m_ui.editMenu->insertSeparator(before_action);

	before_action = m_ui.viewCenterAction;
	m_ui.ToolBar->insertAction(before_action, undo_action);
	m_ui.ToolBar->insertAction(before_action, redo_action);
	m_ui.ToolBar->insertSeparator(before_action);

	// Special zoom composite widget...
	QWidget *zoom_widget = new QWidget();
	zoom_widget->setMaximumWidth(240);
	zoom_widget->setToolTip(tr("Zoom"));

	QHBoxLayout *zoom_layout = new QHBoxLayout();
	zoom_layout->setContentsMargins(0, 0, 0, 0);
	zoom_layout->setSpacing(2);

	QToolButton *zoom_out = new QToolButton();
	zoom_out->setDefaultAction(m_ui.viewZoomOutAction);
	zoom_out->setFixedSize(22, 22);
	zoom_layout->addWidget(zoom_out);

	m_zoom_slider = new qjackctlGraphZoomSlider();
	m_zoom_slider->setFixedHeight(22);
	zoom_layout->addWidget(m_zoom_slider);

	QToolButton *zoom_in = new QToolButton();
	zoom_in->setDefaultAction(m_ui.viewZoomInAction);
	zoom_in->setFixedSize(22, 22);
	zoom_layout->addWidget(zoom_in);

	m_zoom_spinbox = new QSpinBox();
	m_zoom_spinbox->setFixedHeight(22);
	m_zoom_spinbox->setAlignment(Qt::AlignCenter);
	m_zoom_spinbox->setMinimum(10);
	m_zoom_spinbox->setMaximum(200);
	m_zoom_spinbox->setSuffix(" %");
	zoom_layout->addWidget(m_zoom_spinbox);

	zoom_widget->setLayout(zoom_layout);
	m_ui.StatusBar->addPermanentWidget(zoom_widget);

	QObject::connect(m_zoom_spinbox,
		SIGNAL(valueChanged(int)),
		SLOT(zoomValueChanged(int)));
	QObject::connect(m_zoom_slider,
		SIGNAL(valueChanged(int)),
		SLOT(zoomValueChanged(int)));

	QObject::connect(m_ui.graphCanvas,
		SIGNAL(added(qjackctlGraphNode *)),
		SLOT(added(qjackctlGraphNode *)));
	QObject::connect(m_ui.graphCanvas,
		SIGNAL(updated(qjackctlGraphNode *)),
		SLOT(updated(qjackctlGraphNode *)));
	QObject::connect(m_ui.graphCanvas,
		SIGNAL(removed(qjackctlGraphNode *)),
		SLOT(removed(qjackctlGraphNode *)));

	QObject::connect(m_ui.graphCanvas,
		SIGNAL(connected(qjackctlGraphPort *, qjackctlGraphPort *)),
		SLOT(connected(qjackctlGraphPort *, qjackctlGraphPort *)));
	QObject::connect(m_ui.graphCanvas,
		SIGNAL(disconnected(qjackctlGraphPort *, qjackctlGraphPort *)),
		SLOT(disconnected(qjackctlGraphPort *, qjackctlGraphPort *)));

	QObject::connect(m_ui.graphCanvas,
		SIGNAL(connected(qjackctlGraphConnect *)),
		SLOT(connected(qjackctlGraphConnect *)));

	QObject::connect(m_ui.graphCanvas,
		SIGNAL(renamed(qjackctlGraphItem *, const QString&)),
		SLOT(renamed(qjackctlGraphItem *, const QString&)));

	QObject::connect(m_ui.graphCanvas,
		SIGNAL(changed()),
		SLOT(changed()));

	// Some actions surely need those
	// shortcuts firmly attached...
	addAction(m_ui.viewMenubarAction);
	addAction(m_ui.editSearchItemAction);

	// HACK: Make old Ins/Del standard shortcuts
	// for connect/disconnect available again...
	QList<QKeySequence> shortcuts;
	shortcuts.append(m_ui.graphConnectAction->shortcut());
	shortcuts.append(QKeySequence("Ins"));
	m_ui.graphConnectAction->setShortcuts(shortcuts);
	shortcuts.clear();
	shortcuts.append(m_ui.graphDisconnectAction->shortcut());
	shortcuts.append(QKeySequence("Del"));
	m_ui.graphDisconnectAction->setShortcuts(shortcuts);

	QObject::connect(m_ui.graphConnectAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(connectItems()));
	QObject::connect(m_ui.graphDisconnectAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(disconnectItems()));

	QObject::connect(m_ui.graphCloseAction,
		SIGNAL(triggered(bool)),
		SLOT(close()));

	QObject::connect(m_ui.editSelectAllAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(selectAll()));
	QObject::connect(m_ui.editSelectNoneAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(selectNone()));
	QObject::connect(m_ui.editSelectInvertAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(selectInvert()));

	QObject::connect(m_ui.editRenameItemAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(renameItem()));
	QObject::connect(m_ui.editSearchItemAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(searchItem()));

	QObject::connect(m_ui.viewMenubarAction,
		SIGNAL(triggered(bool)),
		SLOT(viewMenubar(bool)));
	QObject::connect(m_ui.viewStatusbarAction,
		SIGNAL(triggered(bool)),
		SLOT(viewStatusbar(bool)));
	QObject::connect(m_ui.viewToolbarAction,
		SIGNAL(triggered(bool)),
		SLOT(viewToolbar(bool)));

	m_thumb_mode = new QActionGroup(this);
	m_thumb_mode->setExclusive(true);
	m_thumb_mode->addAction(m_ui.viewThumbviewTopLeftAction);
	m_thumb_mode->addAction(m_ui.viewThumbviewTopRightAction);
	m_thumb_mode->addAction(m_ui.viewThumbviewBottomLeftAction);
	m_thumb_mode->addAction(m_ui.viewThumbviewBottomRightAction);
	m_thumb_mode->addAction(m_ui.viewThumbviewNoneAction);

	m_ui.viewThumbviewTopLeftAction->setData(qjackctlGraphThumb::TopLeft);
	m_ui.viewThumbviewTopRightAction->setData(qjackctlGraphThumb::TopRight);
	m_ui.viewThumbviewBottomLeftAction->setData(qjackctlGraphThumb::BottomLeft);
	m_ui.viewThumbviewBottomRightAction->setData(qjackctlGraphThumb::BottomRight);
	m_ui.viewThumbviewNoneAction->setData(qjackctlGraphThumb::None);

	QObject::connect(m_ui.viewThumbviewTopLeftAction,
		SIGNAL(triggered(bool)),
		SLOT(viewThumbviewAction()));
	QObject::connect(m_ui.viewThumbviewTopRightAction,
		SIGNAL(triggered(bool)),
		SLOT(viewThumbviewAction()));
	QObject::connect(m_ui.viewThumbviewBottomLeftAction,
		SIGNAL(triggered(bool)),
		SLOT(viewThumbviewAction()));
	QObject::connect(m_ui.viewThumbviewBottomRightAction,
		SIGNAL(triggered(bool)),
		SLOT(viewThumbviewAction()));
	QObject::connect(m_ui.viewThumbviewNoneAction,
		SIGNAL(triggered(bool)),
		SLOT(viewThumbviewAction()));

	QObject::connect(m_ui.viewTextBesideIconsAction,
		SIGNAL(triggered(bool)),
		SLOT(viewTextBesideIcons(bool)));

	QObject::connect(m_ui.viewCenterAction,
		SIGNAL(triggered(bool)),
		SLOT(viewCenter()));
	QObject::connect(m_ui.viewRefreshAction,
		SIGNAL(triggered(bool)),
		SLOT(viewRefresh()));

	QObject::connect(m_ui.viewZoomInAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(zoomIn()));
	QObject::connect(m_ui.viewZoomOutAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(zoomOut()));
	QObject::connect(m_ui.viewZoomFitAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(zoomFit()));
	QObject::connect(m_ui.viewZoomResetAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(zoomReset()));

	QObject::connect(m_ui.viewZoomRangeAction,
		SIGNAL(triggered(bool)),
		SLOT(viewZoomRange(bool)));

	QObject::connect(m_ui.viewRepelOverlappingNodesAction,
		SIGNAL(triggered(bool)),
		SLOT(viewRepelOverlappingNodes(bool)));
	QObject::connect(m_ui.viewConnectThroughNodesAction,
		SIGNAL(triggered(bool)),
		SLOT(viewConnectThroughNodes(bool)));

	m_ui.viewColorsJackAudioAction->setData(qjackctlJackGraph::audioPortType());
	m_ui.viewColorsJackMidiAction->setData(qjackctlJackGraph::midiPortType());

	int iAddSeparator = 0;
#ifdef CONFIG_JACK_CV
	m_ui.viewColorsJackCvAction->setData(qjackctlJackGraph::cvPortType());
	m_ui.viewColorsMenu->insertAction(
		m_ui.viewColorsResetAction, m_ui.viewColorsJackCvAction);
	++iAddSeparator;
#endif
#ifdef CONFIG_JACK_OSC
	m_ui.viewColorsJackOscAction->setData(qjackctlJackGraph::oscPortType());
	m_ui.viewColorsMenu->insertAction(
		m_ui.viewColorsResetAction, m_ui.viewColorsJackOscAction);
	++iAddSeparator;
#endif
	if (iAddSeparator > 0)
		m_ui.viewColorsMenu->insertSeparator(m_ui.viewColorsResetAction);

#ifdef CONFIG_ALSA_SEQ
	m_ui.viewColorsAlsaMidiAction->setData(qjackctlAlsaGraph::midiPortType());
	m_ui.viewColorsMenu->insertAction(
		m_ui.viewColorsResetAction, m_ui.viewColorsAlsaMidiAction);
	m_ui.viewColorsMenu->insertSeparator(m_ui.viewColorsResetAction);
#endif

	QObject::connect(m_ui.viewColorsJackAudioAction,
		SIGNAL(triggered(bool)),
		SLOT(viewColorsAction()));
	QObject::connect(m_ui.viewColorsJackMidiAction,
		SIGNAL(triggered(bool)),
		SLOT(viewColorsAction()));
#ifdef CONFIG_JACK_CV
	QObject::connect(m_ui.viewColorsJackCvAction,
		SIGNAL(triggered(bool)),
		SLOT(viewColorsAction()));
#endif
#ifdef CONFIG_JACK_OSC
	QObject::connect(m_ui.viewColorsJackOscAction,
		SIGNAL(triggered(bool)),
		SLOT(viewColorsAction()));
#endif
#ifdef CONFIG_ALSA_SEQ
	QObject::connect(m_ui.viewColorsAlsaMidiAction,
		SIGNAL(triggered(bool)),
		SLOT(viewColorsAction()));
#endif
	QObject::connect(m_ui.viewColorsResetAction,
		SIGNAL(triggered(bool)),
		SLOT(viewColorsReset()));

	m_sort_type = new QActionGroup(this);
	m_sort_type->setExclusive(true);
	m_sort_type->addAction(m_ui.viewSortPortNameAction);
	m_sort_type->addAction(m_ui.viewSortPortTitleAction);
	m_sort_type->addAction(m_ui.viewSortPortIndexAction);

	m_ui.viewSortPortNameAction->setData(qjackctlGraphPort::PortName);
	m_ui.viewSortPortTitleAction->setData(qjackctlGraphPort::PortTitle);
	m_ui.viewSortPortIndexAction->setData(qjackctlGraphPort::PortIndex);

	QObject::connect(m_ui.viewSortPortNameAction,
		SIGNAL(triggered(bool)),
		SLOT(viewSortTypeAction()));
	QObject::connect(m_ui.viewSortPortTitleAction,
		SIGNAL(triggered(bool)),
		SLOT(viewSortTypeAction()));
	QObject::connect(m_ui.viewSortPortIndexAction,
		SIGNAL(triggered(bool)),
		SLOT(viewSortTypeAction()));

	m_sort_order = new QActionGroup(this);
	m_sort_order->setExclusive(true);
	m_sort_order->addAction(m_ui.viewSortAscendingAction);
	m_sort_order->addAction(m_ui.viewSortDescendingAction);

	m_ui.viewSortAscendingAction->setData(qjackctlGraphPort::Ascending);
	m_ui.viewSortDescendingAction->setData(qjackctlGraphPort::Descending);

	QObject::connect(m_ui.viewSortAscendingAction,
		SIGNAL(triggered(bool)),
		SLOT(viewSortOrderAction()));
	QObject::connect(m_ui.viewSortDescendingAction,
		SIGNAL(triggered(bool)),
		SLOT(viewSortOrderAction()));

	QObject::connect(m_ui.helpAboutAction,
		SIGNAL(triggered(bool)),
		SLOT(helpAbout()));
	QObject::connect(m_ui.helpAboutQtAction,
		SIGNAL(triggered(bool)),
		SLOT(helpAboutQt()));

	QObject::connect(m_ui.ToolBar,
		SIGNAL(orientationChanged(Qt::Orientation)),
		SLOT(orientationChanged(Qt::Orientation)));

	m_ui.graphCanvas->setSearchPlaceholderText(
		m_ui.editSearchItemAction->statusTip() + QString(3, '.'));
}


// Destructor.
qjackctlGraphForm::~qjackctlGraphForm (void)
{
	if (m_thumb)
		delete m_thumb;

	delete m_thumb_mode;

	delete m_sort_order;
	delete m_sort_type;

	if (m_jack)
		delete m_jack;
#ifdef CONFIG_ALSA_SEQ
	if (m_alsa)
		delete m_alsa;
#endif
	if (m_config)
		delete m_config;
}


// Set reference to global options, mostly needed for
// the initial state of the main dockable views and
// those client/port aliasing feature.
void qjackctlGraphForm::setup ( qjackctlSetup *pSetup )
{
	m_config = new qjackctlGraphConfig(&pSetup->settings());

	m_ui.graphCanvas->setSettings(m_config->settings());
	m_ui.graphCanvas->setAliases(&(pSetup->aliases));

	m_config->restoreState(this);

	// Raise the operational sects...
	m_jack = new qjackctlJackGraph(m_ui.graphCanvas);
#ifdef CONFIG_ALSA_SEQ
	if (pSetup->bAlsaSeqEnabled)
		m_alsa = new qjackctlAlsaGraph(m_ui.graphCanvas);
#endif

	m_ui.viewMenubarAction->setChecked(m_config->isMenubar());
	m_ui.viewToolbarAction->setChecked(m_config->isToolbar());
	m_ui.viewStatusbarAction->setChecked(m_config->isStatusbar());

	m_ui.viewTextBesideIconsAction->setChecked(m_config->isTextBesideIcons());
	m_ui.viewZoomRangeAction->setChecked(m_config->isZoomRange());
	m_ui.viewRepelOverlappingNodesAction->setChecked(m_config->isRepelOverlappingNodes());
	m_ui.viewConnectThroughNodesAction->setChecked(m_config->isConnectThroughNodes());

	const qjackctlGraphPort::SortType sort_type
		= qjackctlGraphPort::SortType(m_config->sortType());
	qjackctlGraphPort::setSortType(sort_type);
	switch (sort_type) {
	case qjackctlGraphPort::PortIndex:
		m_ui.viewSortPortIndexAction->setChecked(true);
		break;
	case qjackctlGraphPort::PortTitle:
		m_ui.viewSortPortTitleAction->setChecked(true);
		break;
	case qjackctlGraphPort::PortName:
	default:
		m_ui.viewSortPortNameAction->setChecked(true);
		break;
	}

	const qjackctlGraphPort::SortOrder sort_order
		= qjackctlGraphPort::SortOrder(m_config->sortOrder());
	qjackctlGraphPort::setSortOrder(sort_order);
	switch (sort_order) {
	case qjackctlGraphPort::Descending:
		m_ui.viewSortDescendingAction->setChecked(true);
		break;
	case qjackctlGraphPort::Ascending:
	default:
		m_ui.viewSortAscendingAction->setChecked(true);
		break;
	}

	viewMenubar(m_config->isMenubar());
	viewToolbar(m_config->isToolbar());
	viewStatusbar(m_config->isStatusbar());

	viewThumbview(m_config->thumbview());

	viewTextBesideIcons(m_config->isTextBesideIcons());
	viewZoomRange(m_config->isZoomRange());
	viewRepelOverlappingNodes(m_config->isRepelOverlappingNodes());
	viewConnectThroughNodes(m_config->isConnectThroughNodes());

	m_ui.graphCanvas->restoreState();

	updateViewColors();

	stabilize();

	// Make it ready :-)
	m_ui.StatusBar->showMessage(tr("Ready"), 3000);

	// Trigger refresh cycle...
	jack_changed();
	alsa_changed();
}


// Update the canvas palette.
void qjackctlGraphForm::updatePalette (void)
{
	m_ui.graphCanvas->updatePalette();

	if (m_thumb)
		m_thumb->updatePalette();

	viewRefresh();
}


// Main menu slots.
void qjackctlGraphForm::viewMenubar ( bool on )
{
	m_ui.MenuBar->setVisible(on);

	++m_thumb_update;
}


void qjackctlGraphForm::viewToolbar ( bool on )
{
	m_ui.ToolBar->setVisible(on);

	++m_thumb_update;
}


void qjackctlGraphForm::viewStatusbar ( bool on )
{
	m_ui.StatusBar->setVisible(on);

	++m_thumb_update;
}


void qjackctlGraphForm::viewThumbviewAction (void)
{
	QAction *action = qobject_cast<QAction *> (sender());
	if (action)
		viewThumbview(action->data().toInt());
}


void qjackctlGraphForm::viewThumbview ( int thumbview )
{
	const qjackctlGraphThumb::Position position
		= qjackctlGraphThumb::Position(thumbview);
	if (position == qjackctlGraphThumb::None) {
		if (m_thumb) {
			m_thumb->hide();
			delete m_thumb;
			m_thumb = nullptr;
			m_thumb_update = 0;
		}
	} else {
		if (m_thumb) {
			m_thumb->setPosition(position);
		} else {
			m_thumb = new qjackctlGraphThumb(m_ui.graphCanvas, position);
			QObject::connect(m_thumb,
				SIGNAL(contextMenuRequested(const QPoint&)),
				SLOT(thumbviewContextMenu(const QPoint&)),
				Qt::QueuedConnection);
			QObject::connect(m_thumb,
				SIGNAL(positionRequested(int)),
				SLOT(viewThumbview(int)),
				Qt::QueuedConnection);
			++m_thumb_update;
		}
	}

	switch (position) {
	case qjackctlGraphThumb::TopLeft:
		m_ui.viewThumbviewTopLeftAction->setChecked(true);
		break;
	case qjackctlGraphThumb::TopRight:
		m_ui.viewThumbviewTopRightAction->setChecked(true);
		break;
	case qjackctlGraphThumb::BottomLeft:
		m_ui.viewThumbviewBottomLeftAction->setChecked(true);
		break;
	case qjackctlGraphThumb::BottomRight:
		m_ui.viewThumbviewBottomRightAction->setChecked(true);
		break;
	case qjackctlGraphThumb::None:
	default:
		m_ui.viewThumbviewNoneAction->setChecked(true);
		break;
	}
}
 
 
void qjackctlGraphForm::viewTextBesideIcons ( bool on )
{
	if (on) {
		m_ui.ToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	} else {
		m_ui.ToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	}

	++m_thumb_update;
}


void qjackctlGraphForm::viewCenter (void)
{
	const QRectF& scene_rect
		= m_ui.graphCanvas->scene()->itemsBoundingRect();
	m_ui.graphCanvas->centerOn(scene_rect.center());

	stabilize();
}


void qjackctlGraphForm::viewRefresh (void)
{
	if (m_jack)
		m_jack->clearItems();
#ifdef CONFIG_ALSA_SEQ
	if (m_alsa)
		m_alsa->clearItems();
#endif

	jack_changed();
	alsa_changed();

	if (m_ui.graphCanvas->isRepelOverlappingNodes())
		++m_repel_overlapping_nodes; // fake nodes added!

	++m_thumb_update;

	refresh();
}


void qjackctlGraphForm::viewZoomRange ( bool on )
{
	m_ui.graphCanvas->setZoomRange(on);
}


void qjackctlGraphForm::viewColorsAction (void)
{
	QAction *action = qobject_cast<QAction *> (sender());
	if (action == nullptr)
		return;

	const uint port_type = action->data().toUInt();
	if (0 >= port_type)
		return;

	const QColor& color = QColorDialog::getColor(
		m_ui.graphCanvas->portTypeColor(port_type), this,
		tr("Colors - %1").arg(action->text().remove('&')));
	if (color.isValid()) {
		m_ui.graphCanvas->setPortTypeColor(port_type, color);
		m_ui.graphCanvas->updatePortTypeColors(port_type);
		updateViewColorsAction(action);
	}

	++m_thumb_update;
}


void qjackctlGraphForm::viewColorsReset (void)
{
	m_ui.graphCanvas->clearPortTypeColors();
	if (m_jack)
		m_jack->resetPortTypeColors();
#ifdef CONFIG_ALSA_SEQ
	if (m_alsa)
		m_alsa->resetPortTypeColors();
#endif
	m_ui.graphCanvas->updatePortTypeColors();

	updateViewColors();
}


void qjackctlGraphForm::viewSortTypeAction (void)
{
	QAction *action = qobject_cast<QAction *> (sender());
	if (action == nullptr)
		return;

	const qjackctlGraphPort::SortType sort_type
		= qjackctlGraphPort::SortType(action->data().toInt());
	qjackctlGraphPort::setSortType(sort_type);

	m_ui.graphCanvas->updateNodes();
}


void qjackctlGraphForm::viewSortOrderAction (void)
{
	QAction *action = qobject_cast<QAction *> (sender());
	if (action == nullptr)
		return;

	const qjackctlGraphPort::SortOrder sort_order
		= qjackctlGraphPort::SortOrder(action->data().toInt());
	qjackctlGraphPort::setSortOrder(sort_order);

	m_ui.graphCanvas->updateNodes();
}


void qjackctlGraphForm::viewRepelOverlappingNodes ( bool on )
{
	m_ui.graphCanvas->setRepelOverlappingNodes(on);
	if (on) ++m_repel_overlapping_nodes;
}


void qjackctlGraphForm::viewConnectThroughNodes ( bool on )
{
	qjackctlGraphConnect::setConnectThroughNodes(on);
	m_ui.graphCanvas->updateConnects();
}


void qjackctlGraphForm::helpAbout (void)
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->showAboutForm();
}


void qjackctlGraphForm::helpAboutQt (void)
{
	QMessageBox::aboutQt(this);
}


void qjackctlGraphForm::thumbviewContextMenu ( const QPoint& pos )
{
	stabilize();

	QMenu menu(this);
	menu.addMenu(m_ui.viewThumbviewMenu);
	menu.addSeparator();
	menu.addAction(m_ui.viewCenterAction);
	menu.addMenu(m_ui.viewZoomMenu);

	menu.exec(pos);

	stabilize();
}


void qjackctlGraphForm::zoomValueChanged ( int zoom_value )
{
	m_ui.graphCanvas->setZoom(0.01 * qreal(zoom_value));
}


// Node life-cycle slots.
void qjackctlGraphForm::added ( qjackctlGraphNode *node )
{
	const qjackctlGraphCanvas *canvas
		= m_ui.graphCanvas;
	const QRectF& rect
		= canvas->mapToScene(canvas->viewport()->rect()).boundingRect();
	const QPointF& pos = rect.center();
	const qreal w = 0.33 * qMax(rect.width(),  800.0);
	const qreal h = 0.33 * qMax(rect.height(), 600.0);

	qreal x = pos.x();
	qreal y = pos.y();

	switch (node->nodeMode()) {
	case qjackctlGraphItem::Input:
		++m_ins &= 0x0f;
		x += w;
		y += 0.33 * h * (m_ins & 1 ? +m_ins : -m_ins);
		break;
	case qjackctlGraphItem::Output:
		++m_outs &= 0x0f;
		x -= w;
		y += 0.33 * h * (m_outs & 1 ? +m_outs : -m_outs);
		break;
	default: {
		int dx = 0;
		int dy = 0;
		for (int i = 0; i < m_mids; ++i) {
			if ((qAbs(dx) > qAbs(dy)) || (dx == dy && dx < 0))
				dy += (dx < 0 ? +1 : -1);
			else
				dx += (dy < 0 ? -1 : +1);
		}
		x += 0.33 * w * qreal(dx);
		y += 0.33 * h * qreal(dy);
		++m_mids &= 0x1f;
		break;
	}}

	x -= qreal(::rand() & 0x1f);
	y -= qreal(::rand() & 0x1f);

	node->setPos(canvas->snapPos(QPointF(x, y)));

	updated(node);
}


void qjackctlGraphForm::updated ( qjackctlGraphNode */*node*/ )
{
	if (m_ui.graphCanvas->isRepelOverlappingNodes())
		++m_repel_overlapping_nodes;
}


void qjackctlGraphForm::removed ( qjackctlGraphNode */*node*/ )
{
#if 0// FIXME: DANGEROUS! Node might have been deleted by now...
	if (node) {
		switch (node->nodeMode()) {
		case qjackctlGraphItem::Input:
			--m_ins;
			break;
		case qjackctlGraphItem::Output:
			--m_outs;
			break;
		default:
			--m_mids;
			break;
		}
	}
#endif
}


// Port (dis)connection slots.
void qjackctlGraphForm::connected (
	qjackctlGraphPort *port1, qjackctlGraphPort *port2 )
{
	if (qjackctlJackGraph::isPortType(port1->portType())) {
		if (m_jack)
			m_jack->connectPorts(port1, port2, true);
		jack_changed();
	}
#ifdef CONFIG_ALSA_SEQ
	else
	if (qjackctlAlsaGraph::isPortType(port1->portType())) {
		if (m_alsa)
			m_alsa->connectPorts(port1, port2, true);
		alsa_changed();
	}
#endif

	stabilize();
}


void qjackctlGraphForm::disconnected (
	qjackctlGraphPort *port1, qjackctlGraphPort *port2 )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->queryDisconnect(port1, port2);

	if (qjackctlJackGraph::isPortType(port1->portType())) {
		if (m_jack)
			m_jack->connectPorts(port1, port2, false);
		jack_changed();
	}
#ifdef CONFIG_ALSA_SEQ
	else
	if (qjackctlAlsaGraph::isPortType(port1->portType())) {
		if (m_alsa)
			m_alsa->connectPorts(port1, port2, false);
		alsa_changed();
	}
#endif

	stabilize();
}


void qjackctlGraphForm::connected ( qjackctlGraphConnect *connect )
{
	qjackctlGraphPort *port1 = connect->port1();
	if (port1 == nullptr)
		return;

	if (qjackctlJackGraph::isPortType(port1->portType())) {
		if (m_jack)
			m_jack->addItem(connect, false);
	}
#ifdef CONFIG_ALSA_SEQ
	else
	if (qjackctlAlsaGraph::isPortType(port1->portType())) {
		if (m_alsa)
			m_alsa->addItem(connect, false);
	}
#endif
}


// Item renaming slot.
void qjackctlGraphForm::renamed ( qjackctlGraphItem *item, const QString& name )
{
	qjackctlGraphSect *sect = item_sect(item);
	if (sect)
		sect->renameItem(item, name);
}


// Graph view change slot.
void qjackctlGraphForm::changed (void)
{
	++m_thumb_update;

	stabilize();
}


// Graph section slots.
void qjackctlGraphForm::jack_shutdown (void)
{
	m_ui.graphCanvas->clearSelection();

	m_jack_changed = 0;

	if (m_jack)
		m_jack->clearItems();

	stabilize();
}


void qjackctlGraphForm::jack_changed (void)
{
	++m_jack_changed;
}

void qjackctlGraphForm::alsa_changed (void)
{
	++m_alsa_changed;
}


// Graph refreshner.
void qjackctlGraphForm::refresh (void)
{
	if (m_ui.graphCanvas->isBusy())
		return;

	int nchanged = 0;

	if (m_jack_changed > 0) {
		m_jack_changed = 0;
		if (m_jack)
			m_jack->updateItems();
		++nchanged;
	}
#ifdef CONFIG_ALSA_SEQ
	else
	if (m_alsa_changed > 0) {
		m_alsa_changed = 0;
		if (m_alsa)
			m_alsa->updateItems();
		++nchanged;
	}
#endif

	if (nchanged > 0)
		stabilize();
	else
	if (m_repel_overlapping_nodes > 0) {
		m_repel_overlapping_nodes = 0;
		m_ui.graphCanvas->repelOverlappingNodesAll();
		stabilize();
		++nchanged;
	}

	if (m_thumb_update > 0 || nchanged > 0) {
		m_thumb_update = 0;
		if (m_thumb)
			m_thumb->updateView();
	}
}


// Graph selection change slot.
void qjackctlGraphForm::stabilize (void)
{
	const qjackctlGraphCanvas *canvas = m_ui.graphCanvas;

	m_ui.graphConnectAction->setEnabled(canvas->canConnect());
	m_ui.graphDisconnectAction->setEnabled(canvas->canDisconnect());

	m_ui.editSelectNoneAction->setEnabled(
		!canvas->scene()->selectedItems().isEmpty());
	m_ui.editRenameItemAction->setEnabled(
		canvas->canRenameItem());
	m_ui.editSearchItemAction->setEnabled(
		canvas->canSearchItem());

#if 0
	const QRectF& outter_rect
		= canvas->scene()->sceneRect().adjusted(-2.0, -2.0, +2.0, +2.0);
	const QRectF& inner_rect
		= canvas->mapToScene(canvas->viewport()->rect()).boundingRect();
	const bool is_contained
		= outter_rect.contains(inner_rect) ||
			canvas->horizontalScrollBar()->isVisible() ||
			canvas->verticalScrollBar()->isVisible();
#else
	const bool is_contained = true;
#endif
	const qreal zoom = canvas->zoom();
	m_ui.viewCenterAction->setEnabled(is_contained);
	m_ui.viewZoomInAction->setEnabled(zoom < 1.9);
	m_ui.viewZoomOutAction->setEnabled(zoom > 0.1);
	m_ui.viewZoomFitAction->setEnabled(is_contained);
	m_ui.viewZoomResetAction->setEnabled(zoom != 1.0);

	const int zoom_value = int(100.0f * zoom);
	const bool is_spinbox_blocked = m_zoom_spinbox->blockSignals(true);
	const bool is_slider_blocked = m_zoom_slider->blockSignals(true);
	m_zoom_spinbox->setValue(zoom_value);
	m_zoom_slider->setValue(zoom_value);
	m_zoom_spinbox->blockSignals(is_spinbox_blocked);
	m_zoom_slider->blockSignals(is_slider_blocked);
}


// Tool-bar orientation change slot.
void qjackctlGraphForm::orientationChanged ( Qt::Orientation orientation )
{
	if (m_config && m_config->isTextBesideIcons()
		&& orientation == Qt::Horizontal) {
		m_ui.ToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	} else {
		m_ui.ToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	}
}


// Context-menu event handler.
void qjackctlGraphForm::contextMenuEvent ( QContextMenuEvent *pContextMenuEvent )
{
	m_ui.graphCanvas->clear();

	stabilize();

	QMenu menu(this);
	menu.addAction(m_ui.graphConnectAction);
	menu.addAction(m_ui.graphDisconnectAction);
	menu.addSeparator();
	menu.addActions(m_ui.editMenu->actions());
	menu.addSeparator();
	menu.addMenu(m_ui.viewZoomMenu);

	menu.exec(pContextMenuEvent->globalPos());

	stabilize();
}


// Widget resize event handler.
void qjackctlGraphForm::resizeEvent ( QResizeEvent *pResizeEvent )
{
	QMainWindow::resizeEvent(pResizeEvent);

	if (m_thumb) {
		m_thumb_update = 0;
		m_thumb->updateView();
	}

	stabilize();
}


// Notify our parent that we're emerging.
void qjackctlGraphForm::showEvent ( QShowEvent *pShowEvent )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeFormEx();

	++m_thumb_update;

	QWidget::showEvent(pShowEvent);
}


// Notify our parent that we're closing.
void qjackctlGraphForm::hideEvent ( QHideEvent *pHideEvent )
{
	QWidget::hideEvent(pHideEvent);

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeFormEx();
}


// Widget close event handler.
void qjackctlGraphForm::closeEvent ( QCloseEvent *pCloseEvent )
{
	m_ui.graphCanvas->saveState();

	if (m_config && QMainWindow::isVisible()) {
		m_config->setThumbview(m_thumb ? m_thumb->position() : qjackctlGraphThumb::None);
		m_config->setConnectThroughNodes(m_ui.viewConnectThroughNodesAction->isChecked());
		m_config->setRepelOverlappingNodes(m_ui.viewRepelOverlappingNodesAction->isChecked());
		m_config->setSortOrder(int(qjackctlGraphPort::sortOrder()));
		m_config->setSortType(int(qjackctlGraphPort::sortType()));
		m_config->setTextBesideIcons(m_ui.viewTextBesideIconsAction->isChecked());
		m_config->setZoomRange(m_ui.viewZoomRangeAction->isChecked());
		m_config->setStatusbar(m_ui.StatusBar->isVisible());
		m_config->setToolbar(m_ui.ToolBar->isVisible());
		m_config->setMenubar(m_ui.MenuBar->isVisible());
		m_config->saveState(this);
	}

	QMainWindow::closeEvent(pCloseEvent);
}


// Special port-type color methods.
void qjackctlGraphForm::updateViewColorsAction ( QAction *action  )
{
	const uint port_type = action->data().toUInt();
	if (0 >= port_type)
		return;

	const QColor& color = m_ui.graphCanvas->portTypeColor(port_type);
	if (!color.isValid())
		return;

	QPixmap pm(22, 22);
	QPainter(&pm).fillRect(0, 0, pm.width(), pm.height(), color);
	action->setIcon(QIcon(pm));
}


void qjackctlGraphForm::updateViewColors (void)
{
	updateViewColorsAction(m_ui.viewColorsJackAudioAction);
	updateViewColorsAction(m_ui.viewColorsJackMidiAction);
#ifdef CONFIG_ALSA_SEQ
	updateViewColorsAction(m_ui.viewColorsAlsaMidiAction);
#endif
#ifdef CONFIG_JACK_CV
	updateViewColorsAction(m_ui.viewColorsJackCvAction);
#endif
#ifdef CONFIG_JACK_OSC
	updateViewColorsAction(m_ui.viewColorsJackOscAction);
#endif
}


// Item sect predicate.
qjackctlGraphSect *qjackctlGraphForm::item_sect ( qjackctlGraphItem *item ) const
{
	if (item->type() == qjackctlGraphNode::Type) {
		qjackctlGraphNode *node = static_cast<qjackctlGraphNode *> (item);
		if (node && qjackctlJackGraph::isNodeType(node->nodeType()))
			return m_jack;
	#ifdef CONFIG_ALSA_SEQ
		else
		if (node && qjackctlAlsaGraph::isNodeType(node->nodeType()))
			return m_alsa;
	#endif
	}
	else
	if (item->type() == qjackctlGraphPort::Type) {
		qjackctlGraphPort *port = static_cast<qjackctlGraphPort *> (item);
		if (port && qjackctlJackGraph::isPortType(port->portType()))
			return m_jack;
	#ifdef CONFIG_ALSA_SEQ
		else
		if (port && qjackctlAlsaGraph::isPortType(port->portType()))
			return m_alsa;
	#endif
	}

	return nullptr; // No deal!
}


//----------------------------------------------------------------------------
// qjackctlGraphConfig --  Canvas state memento.

// Local constants.
static const char *GraphLayoutGroup = "/GraphLayout";
static const char *GraphViewGroup   = "/GraphView";
static const char *ViewMenubarKey   = "/Menubar";
static const char *ViewToolbarKey   = "/Toolbar";
static const char *ViewStatusbarKey = "/Statusbar";
static const char *ViewThumbviewKey = "/Thumbview";
static const char *ViewTextBesideIconsKey = "/TextBesideIcons";
static const char *ViewZoomRangeKey = "/ZoomRange";
static const char *ViewSortTypeKey  = "/SortType";
static const char *ViewSortOrderKey = "/SortOrder";
static const char *ViewRepelOverlappingNodesKey = "/RepelOverlappingNodes";
static const char *ViewConnectThroughNodesKey = "/ConnectThroughNodes";


// Constructors.
qjackctlGraphConfig::qjackctlGraphConfig ( QSettings *settings )
	: m_settings(settings), m_menubar(false),
		m_toolbar(false), m_statusbar(false),
		m_thumbview(0), m_texticons(false), m_zoomrange(false),
		m_sorttype(0), m_sortorder(0),
		m_repelnodes(false), m_cthrunodes(false)
{
}


QSettings *qjackctlGraphConfig::settings (void) const
{
	return m_settings;
}


void qjackctlGraphConfig::setMenubar ( bool menubar )
{
	m_menubar = menubar;
}

bool qjackctlGraphConfig::isMenubar (void) const
{
	return m_menubar;
}


void qjackctlGraphConfig::setToolbar ( bool toolbar )
{
	m_toolbar = toolbar;
}

bool qjackctlGraphConfig::isToolbar (void) const
{
	return m_toolbar;
}


void qjackctlGraphConfig::setStatusbar ( bool statusbar )
{
	m_statusbar = statusbar;
}

bool qjackctlGraphConfig::isStatusbar (void) const
{
	return m_statusbar;
}


void qjackctlGraphConfig::setThumbview ( int thumbview )
{
	m_thumbview = thumbview;
}

int qjackctlGraphConfig::thumbview (void) const
{
	return m_thumbview;
}


void qjackctlGraphConfig::setTextBesideIcons ( bool texticons )
{
	m_texticons = texticons;
}

bool qjackctlGraphConfig::isTextBesideIcons (void) const
{
	return m_texticons;
}


void qjackctlGraphConfig::setZoomRange ( bool zoomrange )
{
	m_zoomrange = zoomrange;
}

bool qjackctlGraphConfig::isZoomRange (void) const
{
	return m_zoomrange;
}


void qjackctlGraphConfig::setSortType ( int sorttype )
{
	m_sorttype = sorttype;
}

int qjackctlGraphConfig::sortType (void) const
{
	return m_sorttype;
}


void qjackctlGraphConfig::setSortOrder ( int sortorder )
{
	m_sortorder = sortorder;
}

int qjackctlGraphConfig::sortOrder (void) const
{
	return m_sortorder;
}


void qjackctlGraphConfig::setRepelOverlappingNodes ( bool repelnodes )
{
	m_repelnodes = repelnodes;
}


bool qjackctlGraphConfig::isRepelOverlappingNodes (void) const
{
	return m_repelnodes;
}


void qjackctlGraphConfig::setConnectThroughNodes ( bool cthrunodes )
{
	m_cthrunodes = cthrunodes;
}


bool qjackctlGraphConfig::isConnectThroughNodes (void) const
{
	return m_cthrunodes;
}


// Graph main-widget state methods.
bool qjackctlGraphConfig::restoreState ( QMainWindow *widget )
{
	if (m_settings == nullptr || widget == nullptr)
		return false;

	m_settings->beginGroup(GraphViewGroup);
	m_menubar = m_settings->value(ViewMenubarKey, true).toBool();
	m_toolbar = m_settings->value(ViewToolbarKey, true).toBool();
	m_statusbar = m_settings->value(ViewStatusbarKey, true).toBool();
	m_thumbview = m_settings->value(ViewThumbviewKey, 0).toInt();
	m_texticons = m_settings->value(ViewTextBesideIconsKey, true).toBool();
	m_zoomrange = m_settings->value(ViewZoomRangeKey, false).toBool();
	m_sorttype  = m_settings->value(ViewSortTypeKey, 0).toInt();
	m_sortorder = m_settings->value(ViewSortOrderKey, 0).toInt();
	m_repelnodes = m_settings->value(ViewRepelOverlappingNodesKey, false).toBool();
	m_cthrunodes = m_settings->value(ViewConnectThroughNodesKey, false).toBool();
	m_settings->endGroup();

	m_settings->beginGroup(GraphLayoutGroup);
	const QByteArray& layout_state
		= m_settings->value('/' + widget->objectName()).toByteArray();
	m_settings->endGroup();

	if (!layout_state.isEmpty())
		widget->restoreState(layout_state);

	return true;
}


bool qjackctlGraphConfig::saveState ( QMainWindow *widget ) const
{
	if (m_settings == nullptr || widget == nullptr)
		return false;

	m_settings->beginGroup(GraphViewGroup);
	m_settings->setValue(ViewMenubarKey, m_menubar);
	m_settings->setValue(ViewToolbarKey, m_toolbar);
	m_settings->setValue(ViewStatusbarKey, m_statusbar);
	m_settings->setValue(ViewThumbviewKey, m_thumbview);
	m_settings->setValue(ViewTextBesideIconsKey, m_texticons);
	m_settings->setValue(ViewZoomRangeKey, m_zoomrange);
	m_settings->setValue(ViewSortTypeKey, m_sorttype);
	m_settings->setValue(ViewSortOrderKey, m_sortorder);
	m_settings->setValue(ViewRepelOverlappingNodesKey, m_repelnodes);
	m_settings->setValue(ViewConnectThroughNodesKey, m_cthrunodes);
	m_settings->endGroup();

	m_settings->beginGroup(GraphLayoutGroup);
	const QByteArray& layout_state = widget->saveState();
	m_settings->setValue('/' + widget->objectName(), layout_state);
	m_settings->endGroup();

	return true;
}


// end of qjackctlGraphForm.cpp
