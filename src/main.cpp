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

    // Parse command line option arguments.
    bool bStartJack = false;
    for (int i = 1; i < app.argc(); i++) {
        QString sArg = app.argv()[i];
        // Start audio server immediate option.
        if (sArg == "-s" || sArg == "--start")
            bStartJack = true;
        else
        // Version information.
        if (sArg == "-v" || sArg == "--version") {
            fprintf(stderr, "Qt: %s\n", qVersion());
            fprintf(stderr, "qjackctl: %s\n", QJACKCTL_VERSION);
            app.quit();
            return 0;
        }
        else {
            // Help about command line options.
            fprintf(stderr, QObject::tr("Usage") + ": %s [" + QObject::tr("options") + "]\n\n", app.argv()[0]);
            fprintf(stderr, "%s - %s\n\n", QJACKCTL_TITLE, QJACKCTL_SUBTITLE);
            fprintf(stderr, QObject::tr("Options") + ":\n");
            fprintf(stderr, "  -h, --help\t" + QObject::tr("Show help about command line options") + "\n");
            fprintf(stderr, "  -s, --start\t" + QObject::tr("Start JACK audio server immediately") + "\n");
            fprintf(stderr, "  -v, --version\t" + QObject::tr("Show version information") + "\n");
            app.quit();
            return (-1);
        }
    }

    // Construct and show the main form.
    qjackctlMainForm w;
    w.show();

    // Register the quit signal/slot.
    app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

    // Do we start jack immediately?
    if (bStartJack)
        w.startJack();
        
    return app.exec();
}

// end of main.cpp

