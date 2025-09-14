// qjackctlAboutForm.cpp
//
/****************************************************************************
   Copyright (C) 2003-2025, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifdef CONFIG_JACK_VERSION
#include <jack/jack.h>
#endif


//----------------------------------------------------------------------------
// qjackctlAboutForm -- UI wrapper form.

// Constructor.
qjackctlAboutForm::qjackctlAboutForm ( QWidget *pParent )
	: QDialog(pParent)
{
	// Setup UI struct...
	m_ui.setupUi(this);
#if QT_VERSION < QT_VERSION_CHECK(6, 1, 0)
	QDialog::setWindowIcon(QIcon(":/images/qjackctl.png"));
#endif
	QStringList list;
#ifdef CONFIG_DEBUG
	list << tr("Debugging option enabled.");
#endif
#ifndef CONFIG_SYSTEM_TRAY
	list << tr("System tray disabled.");
#endif
#ifndef CONFIG_JACK_TRANSPORT
	list << tr("Transport status control disabled.");
#endif
#ifndef CONFIG_JACK_REALTIME
	list << tr("Realtime status disabled.");
#endif
#ifndef CONFIG_JACK_XRUN_DELAY
	list << tr("XRUN delay status disabled.");
#endif
#ifndef CONFIG_JACK_MAX_DELAY
	list << tr("Maximum delay status disabled.");
#endif
#ifndef CONFIG_JACK_PORT_ALIASES
	list << tr("JACK Port aliases support disabled.");
#endif
#ifndef CONFIG_JACK_MIDI
	list << tr("JACK MIDI support disabled.");
#endif
#ifndef CONFIG_JACK_SESSION
	list << tr("JACK Session support disabled.");
#endif
#ifndef CONFIG_ALSA_SEQ
#if !defined(__WIN32__) && !defined(_WIN32) && !defined(WIN32) && !defined(__APPLE__)
	list << tr("ALSA/MIDI sequencer support disabled.");
#endif
#endif
#ifndef CONFIG_DBUS
#if !defined(__WIN32__) && !defined(_WIN32) && !defined(WIN32) && !defined(__APPLE__)
	list << tr("D-Bus interface support disabled.");
#endif
#endif

	// Stuff the about box...
	QString sText = "<p align=\"center\"><br />\n";
	sText += "<b><big>" QJACKCTL_TITLE "</big> - " + tr(QJACKCTL_SUBTITLE) + "</b><br />\n";
	sText += "<br />\n";
	sText += tr("Version") + ": <b>" PROJECT_VERSION "</b><br />\n";
//	sText += "<small>" + tr("Build") + ": " CONFIG_BUILD_DATE "<small><br />\n";
	if (!list.isEmpty()) {
		sText += "<small><font color=\"red\">";
		sText += list.join("<br />\n");
		sText += "</font></small>";
	}
	sText += "<br />\n";
	sText += tr("Using: Qt %1").arg(qVersion());
#if defined(QT_STATIC)
	sText += "-static";
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
	sText += ' ';
	sText += '(';
	sText += QApplication::platformName();
	sText += ')';
#endif
#ifdef CONFIG_JACK_VERSION
	sText += ", ";
	sText += tr("JACK %1").arg(jack_get_version_string());
#endif
	sText += "<br />\n";
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
	m_ui.AboutTextView->setText(sText);
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
