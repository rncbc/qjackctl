// qjackctlGraphForm.h
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

#ifndef __qjackctlGraphForm_h
#define __qjackctlGraphForm_h

#include "ui_qjackctlGraphForm.h"


// Forward decls.
class qjackctlGraphConfig;

class qjackctlAlsaGraph;
class qjackctlJackGraph;

class qjackctlGraphPort;

class QResizeEvent;
class QCloseEvent;


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
	qjackctlGraphForm(QWidget *parent = 0, Qt::WindowFlags wflags = 0);

	// Destructor.
	~qjackctlGraphForm();

protected slots:

	// Node life-cycle slots
	void added(qjackctlGraphNode *node);
	void removed(qjackctlGraphNode *node);

	// Port (dis)connection slots.
	void connected(qjackctlGraphPort *port1, qjackctlGraphPort *port2);
	void disconnected(qjackctlGraphPort *port1, qjackctlGraphPort *port2);

	// Graph section slots.
	void jack_shutdown();
	void jack_changed();
	void alsa_changed();

	// Pseudo-asynchronous timed refreshner.
	void refresh();

	// Graph selection change slot.
	void stabilize();

	// Tool-bar orientation change slot.
	void orientationChanged(Qt::Orientation orientation);

	// Main menu slots.
	void graphExit();

	void viewMenubar(bool on);
	void viewToolbar(bool on);
	void viewStatusbar(bool on);

	void viewTextBesideIcons(bool on);

	void viewCenter();
	void viewRefresh();

	void helpAbout();
	void helpAboutQt();

protected:

	// Context-menu event handler.
	void contextMenuEvent(QContextMenuEvent *pContextMenuEvent);

	// Widget resize event handler.
	void resizeEvent(QResizeEvent *pResizeEvent);

	// Widget close event handler.
	void closeEvent(QCloseEvent *pCloseEvent);

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
};


//----------------------------------------------------------------------------
// qjackctlGraphConfig --  Canvas state memento.

class qjackctlGraphConfig
{
public:

	// Constructor.
	qjackctlGraphConfig(QSettings *settings, bool owner = false);
	qjackctlGraphConfig(const QString& org_name, const QString& app_name);

	// Destructor.
	~qjackctlGraphConfig();

	// Accessors.
	void setSettings(QSettings *settings, bool owner = false);
	QSettings *settings() const;

	void setMenubar(bool menubar);
	bool isMenubar() const;

	void setToolbar(bool toolbar);
	bool isToolbar() const;

	void setStatusbar(bool statusbar);
	bool isStatusbar() const;

	void setTextBesideIcons(bool texticons);
	bool isTextBesideIcons() const;

	// Graph main-widget state methods.
	bool restoreState(QMainWindow *widget);
	bool saveState(QMainWindow *widget) const;

private:

	// Instance variables.
	QSettings *m_settings;
	bool       m_owner;

	bool       m_menubar;
	bool       m_toolbar;
	bool       m_statusbar;
	bool       m_texticons;
};


#endif	// __qjackctlGraphForm_h


// end of qjackctlGraphForm.h
