// qjackctlAboutForm.cpp
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

#include "qjackctlAbout.h"
#include "qjackctlAboutForm.h"

#include <QMessageBox>


//----------------------------------------------------------------------------
// qjackctlAboutForm -- UI wrapper form.

// Constructor.
qjackctlAboutForm::qjackctlAboutForm (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QDialog(pParent, wflags)
{
	// Setup UI struct...
	m_ui.setupUi(this);

    // Stuff the about box...
    QString sText = "<p align=\"center\"><br />\n";
    sText += "<b>" + tr(QJACKCTL_SUBTITLE) + "</b><br />\n";
    sText += "<br />\n";
    sText += tr("Version") + ": <b>" QJACKCTL_VERSION "</b><br />\n";
    sText += tr("Build") + ": " __DATE__ " " __TIME__ "<br />\n";
#ifdef CONFIG_DEBUG
    sText += "<small><font color=\"red\">";
    sText += tr("Debugging option enabled.");
    sText += "</font></small><br />\n";
#endif
#ifndef CONFIG_SYSTEM_TRAY
    sText += "<small><font color=\"red\">";
    sText += tr("System tray disabled.");
    sText += "</font></small><br />\n";
#endif
#ifndef CONFIG_JACK_TRANSPORT
    sText += "<small><font color=\"red\">";
    sText += tr("Transport status control disabled.");
    sText += "</font></small><br />\n";
#endif
#ifndef CONFIG_JACK_REALTIME
    sText += "<small><font color=\"red\">";
    sText += tr("Realtime status disabled.");
    sText += "</font></small><br />\n";
#endif
#ifndef CONFIG_JACK_XRUN_DELAY
    sText += "<small><font color=\"red\">";
    sText += tr("XRUN delay status disabled.");
    sText += "</font></small><br />\n";
#endif
#ifndef CONFIG_JACK_MAX_DELAY
    sText += "<small><font color=\"red\">";
    sText += tr("Maximum delay status disabled.");
    sText += "</font></small><br />\n";
#endif
#ifndef CONFIG_JACK_PORT_ALIASES
    sText += "<small><font color=\"red\">";
    sText += tr("JACK Port aliases support disabled.");
    sText += "</font></small><br />\n";
#endif
#ifndef CONFIG_JACK_MIDI
    sText += "<small><font color=\"red\">";
    sText += tr("JACK MIDI support disabled.");
    sText += "</font></small><br />\n";
#endif
#ifndef CONFIG_JACK_SESSION
	sText += "<small><font color=\"red\">";
	sText += tr("JACK Session support disabled.");
	sText += "</font></small><br />";
#endif
#ifndef CONFIG_ALSA_SEQ
#if !defined(WIN32)
    sText += "<small><font color=\"red\">";
    sText += tr("ALSA/MIDI sequencer support disabled.");
    sText += "</font></small><br />\n";
#endif
#endif
#ifndef CONFIG_DBUS
#if !defined(WIN32)
    sText += "<small><font color=\"red\">";
    sText += tr("D-Bus interface support disabled.");
    sText += "</font></small><br />\n";
#endif
#endif
    sText += "<br />\n";
    sText += tr("Website") + ": <a href=\"" QJACKCTL_WEBSITE "\">" QJACKCTL_WEBSITE "</a><br />\n";
    sText += "<br />\n";
    sText += "<small>";
    sText += QJACKCTL_COPYRIGHT "<br />\n";
    sText += "<br />\n";
    sText += tr("This program is free software; you can redistribute it and/or modify it") + "<br />\n";
    sText += tr("under the terms of the GNU General Public License version 2 or later.");
    sText += "</small>";
    sText += "</p>\n";
#if QT_VERSION >= 0x040200
    m_ui.AboutTextView->setText(sText);
#else
    m_ui.AboutTextView->setHtml(sText);
#endif
	// UI connections...
	QObject::connect(m_ui.AboutQtButton,
		SIGNAL(clicked()),
		SLOT(aboutQt()));
	QObject::connect(m_ui.ClosePushButton,
		SIGNAL(clicked()),
		SLOT(close()));
}


// Destructor.
qjackctlAboutForm::~qjackctlAboutForm (void)
{
}


// About Qt request.
void qjackctlAboutForm::aboutQt()
{
	QMessageBox::aboutQt(this);
}


// end of qjackctlAboutForm.cpp
