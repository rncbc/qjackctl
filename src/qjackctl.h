// qjackctl.h
//
/****************************************************************************
   Copyright (C) 2003-2019, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctl_h
#define __qjackctl_h

#include "qjackctlAbout.h"

#include <QApplication>

#if QT_VERSION < 0x050000
#if defined(Q_WS_X11)
#define CONFIG_X11
#endif
#else
#if defined(QT_X11EXTRAS_LIB)
#define CONFIG_X11
#endif
#endif


// Forward decls.
class QWidget;
class QTranslator;

#ifdef CONFIG_X11
#ifdef CONFIG_XUNIQUE

#include <unistd.h> /* for gethostname() */

#include <QX11Info>

#include <X11/Xatom.h>
#include <X11/Xlib.h>

#if QT_VERSION >= 0x050100
class qjackctlXcbEventFilter;
#endif

#endif
#endif


//-------------------------------------------------------------------------
// Singleton application instance stuff (Qt/X11 only atm.)
//

class qjackctlApplication : public QApplication
{
public:

	// Constructor.
	qjackctlApplication(int& argc, char **argv);

	// Destructor.
	~qjackctlApplication();

	// Main application widget accessors.
	void setMainWidget(QWidget *pWidget);
	QWidget *mainWidget() const
		{ return m_pWidget; }

	// Check if another instance is running,
	// and raise its proper main widget...
	bool setup(const QString& sServerName);

#ifdef CONFIG_X11
#ifdef CONFIG_XUNIQUE
	void x11PropertyNotify(Window w);
#endif	// CONFIG_XUNIQUE
#endif	// CONFIG_X11

private:

	// Translation support.
	QTranslator *m_pQtTranslator;
	QTranslator *m_pMyTranslator;

	// Instance variables.
	QWidget *m_pWidget;

#ifdef CONFIG_X11
#ifdef CONFIG_XUNIQUE
	Display *m_pDisplay;
	Atom     m_aUnique;
	Window   m_wOwner;
#if QT_VERSION >= 0x050100
	qjackctlXcbEventFilter *m_pXcbEventFilter;
#endif
#endif	// CONFIG_XUNIQUE
#endif	// CONFIG_X11
};


#endif  // __qjackctl_h

// end of qjackctl.h
