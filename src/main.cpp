// main.cpp
//
/****************************************************************************
   Copyright (C) 2003, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include <qapplication.h>
#include <qtextcodec.h>

#include "qjackctlMainForm.h"
#include "qjackctlAbout.h"

#include "config.h"

int main ( int argc, char **argv )
{
    QApplication app(argc, argv);

    // Load translation support.
    QTranslator translator(0);
    QString sLocale = QTextCodec::locale();
    if (sLocale != "C") {
        QString sLocName = "qjackctl_" + sLocale;
        if (!translator.load(sLocName, ".")) {
            QString sLocPath = CONFIG_PREFIX "/share/locale";
            if (!translator.load(sLocName, sLocPath))
                fprintf(stderr, "Warning: no locale found: %s/%s.qm\n", sLocPath.latin1(), sLocName.latin1());
        }
        app.installTranslator(&translator);
    }

#ifndef CONFIG_NO_START_SERVER
    // Better set our environment for no JACK automagic now.
    setenv("JACK_NO_START_SERVER", "1", 1);
#endif

    // Construct default settings; override with command line arguments.
    qjackctlSetup settings;
    if (!settings.parse_args(app.argc(), app.argv())) {
        app.quit();
        return 1;
    }

    // Check if we'll just start an external program...
    if (!settings.sCmdLine.isEmpty()) {
        jack_client_t *pJackClient = jack_client_new("qjackctl-start");
        if (pJackClient) {
            jack_client_close(pJackClient);
            int iExitStatus = ::system(settings.sCmdLine.latin1());
            app.quit();
            return iExitStatus;
        }
    }

    // Construct the main form, and show it to the world.
    qjackctlMainForm w;
    w.setup(&settings);
    // If we have a systray icon, we'll skip this.
    if (!settings.bSystemTray)
        w.show();

    // Register the quit signal/slot.
    app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

    return app.exec();
}

// end of main.cpp

