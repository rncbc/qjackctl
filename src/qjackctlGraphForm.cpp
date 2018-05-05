// qjackctlGraphForm.cpp
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
#include "qjackctlGraphForm.h"

#include "qjackctlJackGraph.h"
#include "qjackctlAlsaGraph.h"

#include "qjackctlSetup.h"

#include "qjackctlMainForm.h"

#include "qjackctlSetup.h"

//#include <QTimer>
#include <QMenu>

#include <QMessageBox>

#include <QResizeEvent>
#include <QCloseEvent>

#include <math.h>


//----------------------------------------------------------------------------
// qjackctlGraphForm -- UI wrapper form.

// Constructor.
qjackctlGraphForm::qjackctlGraphForm (
	QWidget *parent, Qt::WindowFlags wflags )
	: QMainWindow(parent, wflags), m_config(NULL), m_jack(NULL), m_alsa(NULL)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	m_jack = new qjackctlJackGraph(m_ui.graphCanvas);
#ifdef CONFIG_ALSA_SEQ
	m_alsa = new qjackctlAlsaGraph(m_ui.graphCanvas);
#endif

	m_jack_changed = 0;
	m_alsa_changed = 0;

	m_ins = m_mids = m_outs = 0;

	QUndoStack *commands = m_ui.graphCanvas->commands();

	QAction *undo_action = commands->createUndoAction(this, tr("&Undo"));
    undo_action->setIcon(QIcon(":/images/graphUndo.png"));
    undo_action->setStatusTip(tr("Undo last (dis)connection"));
    undo_action->setShortcuts(QKeySequence::Undo);

	QAction *redo_action = commands->createRedoAction(this, tr("&Redo"));
    redo_action->setIcon(QIcon(":/images/graphRedo.png"));
    redo_action->setStatusTip(tr("Redo last (dis)connection"));
    redo_action->setShortcuts(QKeySequence::Redo);

	QAction *before_action = m_ui.editSelectAllAction;
	m_ui.editMenu->insertAction(before_action, undo_action);
	m_ui.editMenu->insertAction(before_action, redo_action);
	m_ui.editMenu->insertSeparator(before_action);

	before_action = m_ui.viewCenterAction;
	m_ui.ToolBar->insertAction(before_action, undo_action);
	m_ui.ToolBar->insertAction(before_action, redo_action);
	m_ui.ToolBar->insertSeparator(before_action);

	QObject::connect(m_ui.graphCanvas,
		SIGNAL(added(qjackctlGraphNode *)),
		SLOT(added(qjackctlGraphNode *)));
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
		SIGNAL(changed()),
		SLOT(stabilize()));

	// Some actions surely need those
	// shortcuts firmly attached...
	addAction(m_ui.viewMenubarAction);

	QObject::connect(m_ui.graphConnectAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(connectItems()));
	QObject::connect(m_ui.graphDisconnectAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(disconnectItems()));

	QObject::connect(m_ui.graphExitAction,
		SIGNAL(triggered(bool)),
		SLOT(graphExit()));

	QObject::connect(m_ui.editSelectAllAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(selectAll()));
	QObject::connect(m_ui.editSelectNoneAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(selectNone()));
	QObject::connect(m_ui.editSelectInvertAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(selectInvert()));

	QObject::connect(m_ui.viewMenubarAction,
		SIGNAL(triggered(bool)),
		SLOT(viewMenubar(bool)));
	QObject::connect(m_ui.viewStatusbarAction,
		SIGNAL(triggered(bool)),
		SLOT(viewStatusbar(bool)));
	QObject::connect(m_ui.viewToolbarAction,
		SIGNAL(triggered(bool)),
		SLOT(viewToolbar(bool)));

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

	QObject::connect(m_ui.helpAboutAction,
		SIGNAL(triggered(bool)),
		SLOT(helpAbout()));
	QObject::connect(m_ui.helpAboutQtAction,
		SIGNAL(triggered(bool)),
		SLOT(helpAboutQt()));

	QObject::connect(m_ui.ToolBar,
		SIGNAL(orientationChanged(Qt::Orientation)),
		SLOT(orientationChanged(Qt::Orientation)));
}


// Destructor.
qjackctlGraphForm::~qjackctlGraphForm (void)
{
	if (m_jack)
		delete m_jack;
#ifdef CONFIG_ALSA_SEQ
	if (m_alsa)
		delete m_alsa;
#endif
	if (m_config)
		delete m_config;
}


// Set reference to global options, mostly needed for the
// initial sizes of the main splitter views and those
// client/port aliasing feature.
void qjackctlGraphForm::setup ( qjackctlSetup *pSetup )
{
	m_config = new qjackctlGraphConfig(&pSetup->settings());

	m_ui.graphCanvas->setSettings(m_config->settings());

	m_config->restoreState(this);

	m_ui.viewMenubarAction->setChecked(m_config->isMenubar());
	m_ui.viewToolbarAction->setChecked(m_config->isToolbar());
	m_ui.viewStatusbarAction->setChecked(m_config->isStatusbar());

	m_ui.viewTextBesideIconsAction->setChecked(m_config->isTextBesideIcons());

	viewMenubar(m_config->isMenubar());
	viewToolbar(m_config->isToolbar());
	viewStatusbar(m_config->isStatusbar());

	viewTextBesideIcons(m_config->isTextBesideIcons());

	m_ui.graphCanvas->restoreState();

	stabilize();

	// Make it ready :-)
	m_ui.StatusBar->showMessage(tr("Ready"), 3000);

	// Trigger refresh cycle...
	jack_changed();
	alsa_changed();

//	QTimer::singleShot(300, this, SLOT(refresh()));
}


// Main menu slots.
void qjackctlGraphForm::graphExit (void)
{
	close();
}


void qjackctlGraphForm::viewMenubar ( bool on )
{
	m_ui.MenuBar->setVisible(on);
}


void qjackctlGraphForm::viewToolbar ( bool on )
{
	m_ui.ToolBar->setVisible(on);
}


void qjackctlGraphForm::viewStatusbar ( bool on )
{
	m_ui.StatusBar->setVisible(on);
}


void qjackctlGraphForm::viewTextBesideIcons ( bool on )
{
	if (on) {
		m_ui.ToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	} else {
		m_ui.ToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	}
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
	jack_changed();
	alsa_changed();

	refresh();
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


// Context-menu event handler.
void qjackctlGraphForm::contextMenuEvent (
	QContextMenuEvent *pContextMenuEvent )
{
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


// Node life-cycle slots
void qjackctlGraphForm::added ( qjackctlGraphNode *node )
{
	if (m_ui.graphCanvas->restoreNodePos(node))
		return;

	const qjackctlGraphCanvas *canvas
		= m_ui.graphCanvas;
	const QRectF& rect
		= canvas->mapToScene(canvas->viewport()->rect()).boundingRect();
	const QPointF& pos = rect.center();
	const qreal w = 0.3 * rect.width();
	const qreal h = 0.3 * rect.height();

	qreal x = pos.x();
	qreal y = pos.y();

	switch (node->nodeMode()) {
	case qjackctlGraphItem::Input:
		x += w;
		y += h * int(0.5 * (m_ins  + 1) * (m_ins  & 1 ? -1 : +1));
		++m_ins;
		break;
	case qjackctlGraphItem::Output:
		x -= w;
		y += h * int(0.5 * (m_outs + 1) * (m_outs & 1 ? -1 : +1));
		++m_outs;
		break;
	default:
		y += h * int(0.5 * (m_mids + 1) * (m_mids & 1 ? -1 : +1));
		++m_mids;
		break;
	}

	x = 4.0 * ::round(0.25 * x);
	y = 4.0 * ::round(0.25 * y);

	node->setPos(x, y);

	stabilize();
}


void qjackctlGraphForm::removed ( qjackctlGraphNode *node )
{
	if (node) { // FIXME: DANGEROUS! Node might have been deleted by now...
		m_ui.graphCanvas->saveNodePos(node);
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

	stabilize();
}


// Port (dis)connection slots.
void qjackctlGraphForm::connected (
	qjackctlGraphPort *port1, qjackctlGraphPort *port2 )
{
	if (qjackctlJackGraph::isPortType(port1->portType())) {
		m_jack->connectPorts(port1, port2, true);
		jack_changed();
	}
#ifdef CONFIG_ALSA_SEQ
	else
	if (qjackctlAlsaGraph::isPortType(port1->portType())) {
		m_alsa->connectPorts(port1, port2, true);
		alsa_changed();
	}
#endif

	stabilize();
}


void qjackctlGraphForm::disconnected (
	qjackctlGraphPort *port1, qjackctlGraphPort *port2 )
{
	if (qjackctlJackGraph::isPortType(port1->portType())) {
		m_jack->connectPorts(port1, port2, false);
		jack_changed();
	}
#ifdef CONFIG_ALSA_SEQ
	else
	if (qjackctlAlsaGraph::isPortType(port1->portType())) {
		m_alsa->connectPorts(port1, port2, false);
		alsa_changed();
	}
#endif

	stabilize();
}


// Graph section slots.
void qjackctlGraphForm::jack_shutdown (void)
{
	m_jack_changed = 0;
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
	if (m_jack_changed > 0) {
		m_jack_changed = 0;
		m_jack->updateItems();
		stabilize();
	}
	else
	if (m_alsa_changed > 0) {
		m_alsa_changed = 0;
		m_alsa->updateItems();
		stabilize();
	}

//	QTimer::singleShot(300, this, SLOT(refresh()));
}



// Graph selection change slot.
void qjackctlGraphForm::stabilize (void)
{
	const qjackctlGraphCanvas *canvas = m_ui.graphCanvas;

	m_ui.graphConnectAction->setEnabled(canvas->canConnect());
	m_ui.graphDisconnectAction->setEnabled(canvas->canDisconnect());

	m_ui.editSelectNoneAction->setEnabled(
		!canvas->scene()->selectedItems().isEmpty());

	const QRectF& outter_rect
		= canvas->scene()->sceneRect().adjusted(-2.0, -2.0, +2.0, +2.0);
	const QRectF& inner_rect
		= canvas->mapToScene(canvas->viewport()->rect()).boundingRect();
	const bool is_contained
		= outter_rect.contains(inner_rect);
	const qreal zoom = canvas->zoom();

	m_ui.viewZoomInAction->setEnabled(1.9 >= zoom);
	m_ui.viewZoomOutAction->setEnabled(zoom >= 0.2);
	m_ui.viewZoomFitAction->setEnabled(is_contained);
	m_ui.viewZoomResetAction->setEnabled(zoom != 1.0);
	m_ui.viewCenterAction->setEnabled(is_contained);
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


// Widget resize event handler.
void qjackctlGraphForm::resizeEvent ( QResizeEvent *pResizeEvent )
{
	QMainWindow::resizeEvent(pResizeEvent);

	stabilize();
}


// Widget close event handler.
void qjackctlGraphForm::closeEvent ( QCloseEvent *pCloseEvent )
{
	m_ui.graphCanvas->saveState();

	if (m_config && QMainWindow::isVisible()) {
		m_config->setTextBesideIcons(m_ui.viewTextBesideIconsAction->isChecked());
		m_config->setStatusbar(m_ui.StatusBar->isVisible());
		m_config->setToolbar(m_ui.ToolBar->isVisible());
		m_config->setMenubar(m_ui.MenuBar->isVisible());
		m_config->saveState(this);
	}

	QMainWindow::closeEvent(pCloseEvent);
}


//----------------------------------------------------------------------------
// qjackctlGraphConfig --  Canvas state memento.

// Local constants.
static const char *LayoutGroup      = "/GraphLayout";
static const char *ViewGroup        = "/GraphView";
static const char *ViewMenubarKey   = "/Menubar";
static const char *ViewToolbarKey   = "/Toolbar";
static const char *ViewStatusbarKey = "/Statusbar";
static const char *ViewTextBesideIconsKey = "/TextBesideIcons";


// Constructors.
qjackctlGraphConfig::qjackctlGraphConfig ( QSettings *settings )
	: m_settings(settings), m_menubar(false),
		m_toolbar(false), m_statusbar(false), m_texticons(false)
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


void qjackctlGraphConfig::setTextBesideIcons ( bool texticons )
{
	m_texticons = texticons;
}

bool qjackctlGraphConfig::isTextBesideIcons (void) const
{
	return m_texticons;
}


// Graph main-widget state methods.
bool qjackctlGraphConfig::restoreState ( QMainWindow *widget )
{
	if (m_settings == NULL || widget == NULL)
		return false;

	m_settings->beginGroup(ViewGroup);
	m_menubar = m_settings->value(ViewMenubarKey, true).toBool();
	m_toolbar = m_settings->value(ViewToolbarKey, true).toBool();
	m_statusbar = m_settings->value(ViewStatusbarKey, true).toBool();
	m_texticons = m_settings->value(ViewTextBesideIconsKey, true).toBool();
	m_settings->endGroup();

	m_settings->beginGroup(LayoutGroup);
	const QByteArray& layout_state
		= m_settings->value('/' + widget->objectName()).toByteArray();
	m_settings->endGroup();

	if (!layout_state.isEmpty())
		widget->restoreState(layout_state);

	return true;
}


bool qjackctlGraphConfig::saveState ( QMainWindow *widget ) const
{
	if (m_settings == NULL || widget == NULL)
		return false;

	m_settings->beginGroup(ViewGroup);
	m_settings->setValue(ViewMenubarKey, m_menubar);
	m_settings->setValue(ViewToolbarKey, m_toolbar);
	m_settings->setValue(ViewStatusbarKey, m_statusbar);
	m_settings->setValue(ViewTextBesideIconsKey, m_texticons);
	m_settings->endGroup();

	m_settings->beginGroup(LayoutGroup);
	const QByteArray& layout_state = widget->saveState();
	m_settings->setValue('/' + widget->objectName(), layout_state);
	m_settings->endGroup();

	return true;
}


// end of qjackctlGraphForm.cpp
