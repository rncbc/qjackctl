// qjackctl.h
//
/****************************************************************************
   Copyright (C) 2003-2023, rncbc aka Rui Nuno Capela. All rights reserved.

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

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#if defined(Q_WS_X11)
#define CONFIG_X11
#endif
#endif


// Forward decls.
class QWidget;
class QTranslator;

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#ifdef CONFIG_XUNIQUE
#ifdef CONFIG_X11
#include <QX11Info>
typedef unsigned long Window;
typedef unsigned long Atom;
#endif	// CONFIG_X11
#endif	// CONFIG_XUNIQUE
#else
#ifdef CONFIG_XUNIQUE
class QSharedMemory;
class QLocalServer;
#endif	// CONFIG_XUNIQUE
#endif


//-------------------------------------------------------------------------
// Singleton application instance stuff (Qt/X11 only atm.)
//

class qjackctlApplication : public QApplication
{
	Q_OBJECT

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

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#ifdef CONFIG_XUNIQUE
#ifdef CONFIG_X11
	void x11PropertyNotify(Window w);
	bool x11EventFilter(XEvent *pEv)
#endif	// CONFIG_X11
#endif	// CONFIG_XUNIQUE
#else
#ifdef CONFIG_XUNIQUE
protected slots:
	// Local server slots.
	void newConnectionSlot();
	void readyReadSlot();
protected:
	// Local server/shmem setup/cleanup.
	bool setupServer();
	void clearServer();
#endif	// CONFIG_XUNIQUE
#endif

private:

	// Translation support.
	QTranslator *m_pQtTranslator;
	QTranslator *m_pMyTranslator;

	// Instance variables.
	QWidget *m_pWidget;

#ifdef CONFIG_XUNIQUE
	QString m_sServerName;
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#ifdef CONFIG_X11
	Display *m_pDisplay;
	Atom     m_aUnique;
	Window   m_wOwner;
#endif	// CONFIG_X11
#else
	QString        m_sUnique;
	QSharedMemory *m_pMemory;
	QLocalServer  *m_pServer;
#endif
#endif	// CONFIG_XUNIQUE
};


#endif  // __qjackctl_h

// end of qjackctl.h
