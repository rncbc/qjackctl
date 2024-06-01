// qjackctlGraphForm.h
//
/****************************************************************************
   Copyright (C) 2003-2024, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctlGraphForm_h
#define __qjackctlGraphForm_h

#include "ui_qjackctlGraphForm.h"


// Forward decls.
class qjackctlGraphConfig;

class qjackctlGraphSect;
class qjackctlAlsaGraph;
class qjackctlJackGraph;

class qjackctlGraphPort;
class qjackctlGraphConnect;
class qjackctlGraphThumb;

class qjackctlSetup;

class QResizeEvent;
class QCloseEvent;

class QSlider;
class QSpinBox;

class QActionGroup;


// Forwards decls.
class QSettings;
class QMainWindow;


//----------------------------------------------------------------------------
// qjackctlGraphForm -- UI wrapper form.

class qjackctlGraphForm : public QMainWindow
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlGraphForm(QWidget *parent = nullptr,
		Qt::WindowFlags wflags = Qt::WindowFlags());
	// Destructor.
	~qjackctlGraphForm();

	// Initializer.
	void setup(qjackctlSetup *pSetup);

public slots:

	// Graph view change slot.
	void changed();

	// Graph section slots.
	void jack_shutdown();
	void jack_changed();
	void alsa_changed();

	// Graph refreshner.
	void refresh();

protected slots:

	// Node life-cycle slots
	void added(qjackctlGraphNode *node);
	void updated(qjackctlGraphNode *node);
	void removed(qjackctlGraphNode *node);

	// Port (dis)connection slots.
	void connected(qjackctlGraphPort *port1, qjackctlGraphPort *port2);
	void disconnected(qjackctlGraphPort *port1, qjackctlGraphPort *port2);

	void connected(qjackctlGraphConnect *connect);

	// Item renaming slot.
	void renamed(qjackctlGraphItem *item, const QString& name);

	// Graph selection change slot.
	void stabilize();

	// Tool-bar orientation change slot.
	void orientationChanged(Qt::Orientation orientation);

	// Main menu slots.
	void viewMenubar(bool on);
	void viewToolbar(bool on);
	void viewStatusbar(bool on);

	void viewThumbviewAction();
	void viewThumbview(int thumbview);

	void viewTextBesideIcons(bool on);

	void viewCenter();
	void viewRefresh();

	void viewZoomRange(bool on);

	void viewSortTypeAction();
	void viewSortOrderAction();

	void viewColorsAction();
	void viewColorsReset();

	void viewRepelOverlappingNodes(bool on);
	void viewConnectThroughNodes(bool on);

	void helpAbout();
	void helpAboutQt();

	void thumbviewContextMenu(const QPoint& pos);

	void zoomValueChanged(int zoom_value);

protected:

	// Context-menu event handler.
	void contextMenuEvent(QContextMenuEvent *pContextMenuEvent);

	// Widget resize event handler.
	void resizeEvent(QResizeEvent *pResizeEvent);

	// Widget show/hide/close event handlers.
	void showEvent(QShowEvent *pShowEvent);
	void hideEvent(QHideEvent *pHideEvent);
	void closeEvent(QCloseEvent *pCloseEvent);

	// Special port-type color method.
	void updateViewColorsAction(QAction *action);
	void updateViewColors();

	// Item sect predicate.
	qjackctlGraphSect *item_sect(qjackctlGraphItem *item) const;

private:

	// The Qt-designer UI struct...
	Ui::qjackctlGraphForm m_ui;

	// Instance variables.
	qjackctlGraphConfig *m_config;

	// Instance variables.
	qjackctlJackGraph *m_jack;
	qjackctlAlsaGraph *m_alsa;

	int m_jack_changed;
	int m_alsa_changed;

	int m_ins, m_mids, m_outs;

	int m_repel_overlapping_nodes;

	QSlider  *m_zoom_slider;
	QSpinBox *m_zoom_spinbox;

	QActionGroup *m_sort_type;
	QActionGroup *m_sort_order;

	QActionGroup *m_thumb_mode;
	qjackctlGraphThumb *m_thumb;
	int m_thumb_update;
};


//----------------------------------------------------------------------------
// qjackctlGraphConfig --  Canvas state memento.

class qjackctlGraphConfig
{
public:

	// Constructor.
	qjackctlGraphConfig(QSettings *settings);

	// Accessors.
	QSettings *settings() const;

	void setMenubar(bool menubar);
	bool isMenubar() const;

	void setToolbar(bool toolbar);
	bool isToolbar() const;

	void setStatusbar(bool statusbar);
	bool isStatusbar() const;

	void setThumbview(int thumbview);
	int thumbview() const;

	void setTextBesideIcons(bool texticons);
	bool isTextBesideIcons() const;

	void setZoomRange(bool zoomrange);
	bool isZoomRange() const;

	void setSortType(int sorttype);
	int sortType() const;

	void setSortOrder(int sortorder);
	int sortOrder() const;

	void setRepelOverlappingNodes(bool repelnodes);
	bool isRepelOverlappingNodes() const;

	void setConnectThroughNodes(bool repelnodes);
	bool isConnectThroughNodes() const;

	// Graph main-widget state methods.
	bool restoreState(QMainWindow *widget);
	bool saveState(QMainWindow *widget) const;

private:

	// Instance variables.
	QSettings *m_settings;

	bool       m_menubar;
	bool       m_toolbar;
	bool       m_statusbar;
	int        m_thumbview;
	bool       m_texticons;
	bool       m_zoomrange;
	int        m_sorttype;
	int        m_sortorder;

	bool       m_repelnodes;
	bool       m_cthrunodes;
};


#endif	// __qjackctlGraphForm_h


// end of qjackctlGraphForm.h
