// qjackctlSystemTray.cpp
//
/****************************************************************************
   Copyright (C) 2003-2009, rncbc aka Rui Nuno Capela. All rights reserved.

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
const WindowFlags WindowCloseButtonHint = WindowFlags(0);
#if QT_VERSION < 0x040200
const WindowFlags CustomizeWindowHint   = WindowFlags(0);
#endif
}
#endif


#ifndef QJACKCTL_QT4_SYSTEM_TRAY

#include <QMouseEvent>
#include <QPaintEvent>

#if defined(Q_WS_X11)

#include <QX11Info>

#include <X11/Xatom.h>
#include <X11/Xlib.h>

// System Tray Protocol Specification opcodes.
#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#endif	// Q_WS_X11

#endif


//----------------------------------------------------------------------------
// qjackctlSystemTray -- Custom system tray widget.

#ifdef QJACKCTL_QT4_SYSTEM_TRAY

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

#else

// Constructor.
qjackctlSystemTray::qjackctlSystemTray ( QWidget *pParent )
	: QWidget(pParent, Qt::Window
		| Qt::CustomizeWindowHint
		| Qt::X11BypassWindowManagerHint
		| Qt::FramelessWindowHint
		| Qt::WindowStaysOnTopHint)
{
#if QT_VERSION >= 0x040200
	QWidget::setAttribute(Qt::WA_AlwaysShowToolTips);
#endif
//	QWidget::setAttribute(Qt::WA_NoSystemBackground);

//	QWidget::setBackgroundRole(QPalette::NoRole);
//	QWidget::setAutoFillBackground(true);

	QWidget::setFixedSize(22, 22);
//	QWidget::setMinimumSize(22, 22);

#if defined(Q_WS_X11)

	Display *dpy = QX11Info::display();
	WId trayWin  = winId();

	// System Tray Protocol Specification.
	Screen *screen = XDefaultScreenOfDisplay(dpy);
	int iScreen = XScreenNumberOfScreen(screen);
	char szAtom[32];
	snprintf(szAtom, sizeof(szAtom), "_NET_SYSTEM_TRAY_S%d", iScreen);
	Atom selectionAtom = XInternAtom(dpy, szAtom, false);
	XGrabServer(dpy);
	Window managerWin = XGetSelectionOwner(dpy, selectionAtom);
	if (managerWin != None)
		XSelectInput(dpy, managerWin, StructureNotifyMask);
	XUngrabServer(dpy);
	XFlush(dpy);
	if (managerWin != None) {
		XEvent ev;
		memset(&ev, 0, sizeof(ev));
		ev.xclient.type = ClientMessage;
		ev.xclient.window = managerWin;
		ev.xclient.message_type = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", false);
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = CurrentTime;
		ev.xclient.data.l[1] = SYSTEM_TRAY_REQUEST_DOCK;
		ev.xclient.data.l[2] = trayWin;
		ev.xclient.data.l[3] = 0;
		ev.xclient.data.l[4] = 0;
		XSendEvent(dpy, managerWin, false, NoEventMask, &ev);
		XSync(dpy, false);
	}

	// Follwing simple KDE specs:
	Atom trayAtom;
	// For older KDE's (hopefully)...
	int data = 1;
	trayAtom = XInternAtom(dpy, "KWM_DOCKWINDOW", false);
	XChangeProperty(dpy, trayWin, trayAtom, trayAtom, 32, PropModeReplace, (unsigned char *) &data, 1);
	// For not so older KDE's...
	WId forWin = pParent ? pParent->topLevelWidget()->winId() : QX11Info::appRootWindow();
	trayAtom = XInternAtom(dpy, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", false);
	XChangeProperty(dpy, trayWin, trayAtom, XA_WINDOW, 32, PropModeReplace, (unsigned char *) &forWin, 1);

#endif	// Q_WS_X11

	// Set things inherited...
	if (pParent) {
		QWidget::setWindowIcon(pParent->windowIcon());
		QWidget::setToolTip(pParent->windowTitle());
	}

	setBackground(Qt::transparent);
}

#endif


// Default destructor.
qjackctlSystemTray::~qjackctlSystemTray (void)
{
}


// System tray icon/pixmaps update method.
void qjackctlSystemTray::updatePixmap (void)
{
	// Renitialize icon as fit...
#ifdef QJACKCTL_QT4_SYSTEM_TRAY
	m_pixmap = m_icon.pixmap(22, 22);
#else
	m_pixmap = QWidget::windowIcon().pixmap(QWidget::size());
#endif

    // Merge with the overlay pixmap...
	if (!m_pixmapOverlay.mask().isNull()) {
		QBitmap mask = m_pixmap.mask();
		QPainter(&mask).drawPixmap(0, 0, m_pixmapOverlay.mask());
		m_pixmap.setMask(mask);
		QPainter(&m_pixmap).drawPixmap(0, 0, m_pixmapOverlay);
	}

#ifdef QJACKCTL_QT4_SYSTEM_TRAY

	if (m_background != Qt::transparent) {
		QPixmap pixmap(m_pixmap);
		m_pixmap.fill(m_background);
		QPainter(&m_pixmap).drawPixmap(0, 0, pixmap);
	}

    // Setup system tray icon directly...
	QSystemTrayIcon::setIcon(QIcon(m_pixmap));

#else

    // Setup widget drawable pixmap transparency...
	if (!m_pixmap.mask().isNull() && m_background == Qt::transparent) {
		QBitmap mask(m_pixmap.size());
		mask.fill(Qt::color0);
		QBitmap maskPixmap = m_pixmap.mask();
		QPainter(&mask).drawPixmap(
			(mask.width()  - maskPixmap.width())  / 2,
			(mask.height() - maskPixmap.height()) / 2,
			maskPixmap);
		QWidget::setMask(mask);
	} else {
		QWidget::setMask(QBitmap());
	}

	QWidget::update();

#endif
}



// Background mask methods.
void qjackctlSystemTray::setBackground ( const QColor& background )
{
	// Set background color, now.
	m_background = background;

#ifndef QJACKCTL_QT4_SYSTEM_TRAY

	QPalette pal(QWidget::palette());
	pal.setColor(QWidget::backgroundRole(), m_background);
	QWidget::setPalette(pal);

#endif

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


#ifndef QJACKCTL_QT4_SYSTEM_TRAY

// Self-drawable methods.
void qjackctlSystemTray::paintEvent ( QPaintEvent * )
{
	const QRect& rect = QWidget::rect();

	QPainter(this).drawPixmap(
		rect.x() + (rect.width()  - m_pixmap.width())  / 2,
		rect.y() + (rect.height() - m_pixmap.height()) / 2,
		m_pixmap);
}


// Inherited mouse event.
void qjackctlSystemTray::mousePressEvent ( QMouseEvent *pMouseEvent )
{
	if (!QWidget::rect().contains(pMouseEvent->pos()))
		return;

	switch (pMouseEvent->button()) {

	case Qt::LeftButton:
		// Toggle parent widget visibility.
		emit clicked();
		break;

	case Qt::MidButton:
		emit middleClicked();
		break;

	case Qt::RightButton:
		// Just signal we're on to context menu.
		emit contextMenuRequested(pMouseEvent->globalPos());
		break;

	default:
		break;
	}
}

#endif

// end of qjackctlSystemTray.cpp
