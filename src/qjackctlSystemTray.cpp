// qjackctlSystemTray.cpp
//
/****************************************************************************
   Copyright (C) 2003-2004, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*****************************************************************************/

#include "qjackctlSystemTray.h"

#include <qtooltip.h>
#include <qbitmap.h>
#include <qimage.h>

#include "config.h"

#ifdef CONFIG_KDE
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#endif


//----------------------------------------------------------------------------
// qjackctlSystemTray -- Custom system tray widget.

// Constructor.
qjackctlSystemTray::qjackctlSystemTray ( QWidget *pParent , const char *pszName )
    : QLabel(pParent, pszName, Qt::WType_TopLevel)
{
    QLabel::setBackgroundMode(Qt::X11ParentRelative);
    QLabel::setBackgroundOrigin(QWidget::WindowOrigin);

#ifdef CONFIG_KDE
    Display *dpy  = qt_xdisplay();
    WId trayWin   = winId();
    WId forWin    = pParent ? pParent->topLevelWidget()->winId() : qt_xrootwin();
    Atom trayAtom = XInternAtom(dpy, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", false);
    XChangeProperty(dpy, trayWin, trayAtom, XA_WINDOW, 32, PropModeReplace, (unsigned char *) &forWin, 1);
#endif

    if (pParent) {
        QPixmap pm;
        pm.convertFromImage(pParent->icon()->convertToImage().smoothScale(22, 22), 0);
        QLabel::setPixmap(pm);
        QToolTip::add(this, pParent->caption());
    }
}


// Default destructor.
qjackctlSystemTray::~qjackctlSystemTray (void)
{
}


// Inherited mouse event.
void qjackctlSystemTray::mousePressEvent ( QMouseEvent *pMouseEvent )
{
    if (!QLabel::rect().contains(pMouseEvent->pos()))
        return;

    QWidget *pParent = parentWidget();
    if (pParent == 0)
        return;
        
    switch (pMouseEvent->button()) {

      case LeftButton:
        // Toggle parent widget visibility.
        if (pParent->isVisible())
            pParent->hide();
        else
            pParent->show();
        break;

      case RightButton:
        // Just signal we're on to context menu.
        emit contextMenuRequested(this, pMouseEvent->globalPos());
        break;
        
      default:
        break;
    }
}


// Set system tray icon overlay.
void qjackctlSystemTray::setPixmapOverlay ( const QPixmap& pmOverlay )
{
    QWidget *pParent = parentWidget();
    if (pParent == 0)
        return;

    // Get base pixmap from parent widget.
    QPixmap pm;
    pm.convertFromImage(pParent->icon()->convertToImage().smoothScale(22, 22), 0);

    // Merge with the overlay pixmap.
    QBitmap bmMask(*pm.mask());
    bitBlt(&bmMask, 0, 0, pmOverlay.mask(), 0, 0, -1, -1, Qt::OrROP);
    pm.setMask(bmMask);
    bitBlt(&pm, 0, 0, &pmOverlay);

    QLabel::setPixmap(pm);
}


// end of qjackctlSystemTray.cpp

