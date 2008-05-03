// main.cpp
//
/****************************************************************************
   Copyright (C) 2003-2008, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include "qjackctlSetup.h"
#include "qjackctlMainForm.h"

#include <QApplication>
#include <QTranslator>
#include <QLocale>


//-------------------------------------------------------------------------
// Single application instance stuff (Qt/X11 only atm.)
//

#if defined(Q_WS_X11)

#include <QX11Info>

#include <X11/Xatom.h>
#include <X11/Xlib.h>

#define QJACKCTL_XUNIQUE "qjackctlMainForm_xunique"

static bool qjackctl_get_xunique (void)
{
	Display *pDisplay = QX11Info::display();
	Atom aSelection = XInternAtom(pDisplay, QJACKCTL_XUNIQUE, false);

	XGrabServer(pDisplay);
	Window window = XGetSelectionOwner(pDisplay, aSelection);
	XUngrabServer(pDisplay);

	if (window != None) {
		Screen *pScreen = XDefaultScreenOfDisplay(pDisplay);
		int iScreen = XScreenNumberOfScreen(pScreen);
		XEvent ev;
		memset(&ev, 0, sizeof(ev));
		ev.xclient.type = ClientMessage;
		ev.xclient.display = pDisplay;
		ev.xclient.window = window;
		ev.xclient.message_type = XInternAtom(pDisplay, "_NET_ACTIVE_WINDOW", false);
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = 0; // Source indication.
		ev.xclient.data.l[1] = 0; // Timestamp.
		ev.xclient.data.l[2] = 0; // Requestor's currently active window (none)
		ev.xclient.data.l[3] = 0;
		ev.xclient.data.l[4] = 0;
		XSelectInput(pDisplay, window, StructureNotifyMask);
		XSendEvent(pDisplay, RootWindow(pDisplay, iScreen), false,
			(SubstructureNotifyMask | SubstructureRedirectMask), &ev);
		XSync(pDisplay, false);
		XRaiseWindow(pDisplay, window);
	}

//	XFlush(pDisplay);

	return (window != None);
}

static void qjackctl_set_xunique ( QWidget *pWidget )
{
	Display *pDisplay = QX11Info::display();
	Atom aSelection = XInternAtom(pDisplay, QJACKCTL_XUNIQUE, false);

	XGrabServer(pDisplay);
	Window window = pWidget->winId();
	XSetSelectionOwner(pDisplay, aSelection, window, CurrentTime);
	XUngrabServer(pDisplay);

//	XFlush(pDisplay);
}

#endif


//-------------------------------------------------------------------------
// main - The main program trunk.
//

int main ( int argc, char **argv )
{
	QApplication app(argc, argv);

	// Load translation support.
	QTranslator translator(0);
	QLocale loc;
	if (loc.language() != QLocale::C) {
		QString sLocName = "qjackctl_" + loc.name();
		if (!translator.load(sLocName, ".")) {
			QString sLocPath = CONFIG_PREFIX "/share/locale";
			if (!translator.load(sLocName, sLocPath))
				fprintf(stderr, "Warning: no locale found: %s/%s.qm\n",
					sLocPath.toUtf8().constData(),
					sLocName.toUtf8().constData());
		}
		app.installTranslator(&translator);
	}

	// Construct default settings; override with command line arguments.
	qjackctlSetup settings;
	if (!settings.parse_args(app.argc(), app.argv())) {
		app.quit();
		return 1;
	}

	// Check if we'll just start an external program...
	if (!settings.sCmdLine.isEmpty()) {
		jack_client_t *pJackClient
			= jack_client_open("qjackctl-start", JackNoStartServer, NULL);
		if (pJackClient) {
			jack_client_close(pJackClient);
			int iExitStatus = ::system(settings.sCmdLine.toUtf8().constData());
			app.quit();
			return iExitStatus;
		}
	}

#if defined(Q_WS_X11)
	if (qjackctl_get_xunique()) {
		app.quit();
		return 2;
	}
#endif

	// What style do we create these forms?
	Qt::WindowFlags wflags = Qt::Window
#if QT_VERSION >= 0x040200
		| Qt::CustomizeWindowHint
#endif
		| Qt::WindowTitleHint
		| Qt::WindowSystemMenuHint
		| Qt::WindowMinMaxButtonsHint;
	if (settings.bKeepOnTop)
		wflags |= Qt::Tool;
	// Construct the main form, and show it to the world.
	qjackctlMainForm w(0, wflags);
//	app.setMainWidget(&w);
	w.setup(&settings);
	// If we have a systray icon, we'll skip this.
	if (!settings.bSystemTray) {
		w.show();
		w.adjustSize();
	}

#if defined(Q_WS_X11)
	qjackctl_set_xunique(&w);
#endif

	// Register the quit signal/slot.
	// app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
	app.setQuitOnLastWindowClosed(false);

	return app.exec();
}

// end of main.cpp
