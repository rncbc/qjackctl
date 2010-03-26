// qjackctlSystemTray.h
//
/****************************************************************************
   Copyright (C) 2003-2010, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctlSystemTray_h
#define __qjackctlSystemTray_h

#include <QWidget>

#if QT_VERSION >= 0x040200
#define QJACKCTL_QT4_SYSTEM_TRAY
#endif

#ifdef QJACKCTL_QT4_SYSTEM_TRAY
#include <QSystemTrayIcon>
#else
#include <QIcon>
#endif


//----------------------------------------------------------------------------
// qjackctlSystemTray -- Custom system tray widget.

#ifdef QJACKCTL_QT4_SYSTEM_TRAY
class qjackctlSystemTray : public QSystemTrayIcon
#else
class qjackctlSystemTray : public QWidget
#endif
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlSystemTray(QWidget *pParent = 0);
	// Default destructor.
	~qjackctlSystemTray();

	// Background mask methods.
	void setBackground(const QColor& background);
	const QColor& background() const;

    // Set system tray icon overlay.
    void setPixmapOverlay(const QPixmap& pmOverlay);
	const QPixmap& pixmapOverlay() const;

	// System tray icon/pixmaps update method.
	void updatePixmap();

#ifdef QJACKCTL_QT4_SYSTEM_TRAY

	// Redirect to hide.
	void close();

#endif

signals:

	// Clicked signal.
	void clicked();

	// Xrun reset signal.
	void middleClicked();

	// Context menu signal.
	void contextMenuRequested(const QPoint& pos);

#ifdef QJACKCTL_QT4_SYSTEM_TRAY

protected slots:

	// Handle systeam tray activity.
	void activated(QSystemTrayIcon::ActivationReason);

#else

protected:

	// Self-drawable methods.
	void paintEvent(QPaintEvent *);

	// Overriden mouse event method.
	void mousePressEvent(QMouseEvent *);

#endif

private:

	// Instance pixmap and background color.
#ifdef QJACKCTL_QT4_SYSTEM_TRAY
	QIcon   m_icon;
#endif
	QPixmap m_pixmap;
	QPixmap m_pixmapOverlay;
	QColor  m_background;
};


#endif  // __qjackctlSystemTray_h

// end of qjackctlSystemTray.h
