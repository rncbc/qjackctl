// qjackctlSystemTray.cpp
//
/****************************************************************************
   Copyright (C) 2003-2013, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include "qjackctlSystemTray.h"

#include <QBitmap>
#include <QPainter>

#if QT_VERSION < 0x040500
namespace Qt {
const WindowFlags WindowCloseButtonHint = WindowFlags(0x08000000);
}
#endif


//----------------------------------------------------------------------------
// qjackctlSystemTray -- Custom system tray widget.

// Constructor.
qjackctlSystemTray::qjackctlSystemTray ( QWidget *pParent )
	: QSystemTrayIcon(pParent)
{
	// Set things inherited...
	if (pParent) {
		m_icon = pParent->windowIcon();
		QSystemTrayIcon::setIcon(m_icon);
		QSystemTrayIcon::setToolTip(pParent->windowTitle());
	}

	QObject::connect(this,
		SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		SLOT(activated(QSystemTrayIcon::ActivationReason)));

	setBackground(Qt::transparent);

	QSystemTrayIcon::show();
}


// Redirect to hide.
void qjackctlSystemTray::close (void)
{
	QSystemTrayIcon::hide();
}


// Handle systeam tray activity.
void qjackctlSystemTray::activated ( QSystemTrayIcon::ActivationReason reason )
{
	switch (reason) {
	case QSystemTrayIcon::Context:
		emit contextMenuRequested(QCursor::pos());
		break;
	case QSystemTrayIcon::Trigger:
		emit clicked();
		break;
	case QSystemTrayIcon::MiddleClick:
		emit middleClicked();
		break;
	case QSystemTrayIcon::DoubleClick:
	case QSystemTrayIcon::Unknown:
	default:
		break;
	}
}


// Default destructor.
qjackctlSystemTray::~qjackctlSystemTray (void)
{
}


// System tray icon/pixmaps update method.
void qjackctlSystemTray::updatePixmap (void)
{
  // Get the default systray icon size...
  QRect dimension = QSystemTrayIcon::geometry();
	// Renitialize icon as fit...
	m_pixmap = m_icon.pixmap(dimension.width(), dimension.height());

    // Merge with the overlay pixmap...
	if (!m_pixmapOverlay.mask().isNull()) {
		QBitmap mask = m_pixmap.mask();
		QPainter(&mask).drawPixmap(0, 0, m_pixmapOverlay.mask());
		m_pixmap.setMask(mask);
    // paint the status symbol in the bottom left...
		QPainter(&m_pixmap).drawPixmap(0, dimension.height() - m_pixmapOverlay.height(), m_pixmapOverlay);
	}

	if (m_background != Qt::transparent) {
		QPixmap pixmap(m_pixmap);
		m_pixmap.fill(m_background);
		QPainter(&m_pixmap).drawPixmap(0, 0, pixmap);
	}

    // Setup system tray icon directly...
	QSystemTrayIcon::setIcon(QIcon(m_pixmap));
}



// Background mask methods.
void qjackctlSystemTray::setBackground ( const QColor& background )
{
	// Set background color, now.
	m_background = background;

	updatePixmap();
}

const QColor& qjackctlSystemTray::background (void) const
{
	return m_background;
}


// Set system tray icon overlay.
void qjackctlSystemTray::setPixmapOverlay ( const QPixmap& pmOverlay )
{
	m_pixmapOverlay = pmOverlay;

	updatePixmap();
}

const QPixmap& qjackctlSystemTray::pixmapOverlay (void) const
{
	return m_pixmapOverlay;
}


// end of qjackctlSystemTray.cpp
