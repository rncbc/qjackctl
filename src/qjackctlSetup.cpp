// qjackctlSetup.cpp
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

#include "qjackctlSetup.h"
#include "qjackctlAbout.h"


// Constructor.
qjackctlSetup::qjackctlSetup (void)
{
    settings.beginGroup("/qjackctl");
}

// destructor;
qjackctlSetup::~qjackctlSetup (void)
{
    settings.endGroup();
}


// Profile loader.
void qjackctlSetup::load (void)
{
    settings.beginGroup("/Settings");
    sServer     = settings.readEntry("/Server", "jackstart");
    bRealtime   = settings.readBoolEntry("/Realtime", true);
    bSoftMode   = settings.readBoolEntry("/SoftMode", false);
    bAsio       = settings.readBoolEntry("/Asio", false);
    bMonitor    = settings.readBoolEntry("/Monitor", false);
    iChan       = settings.readNumEntry("/Chan", 0);
    iPriority   = settings.readNumEntry("/Priority", 0);
    iFrames     = settings.readNumEntry("/Frames", 1024);
    iSampleRate = settings.readNumEntry("/SampleRate", 48000);
    iPeriods    = settings.readNumEntry("/Periods", 2);
    iWait       = settings.readNumEntry("/Wait", 21333);
    sDriver     = settings.readEntry("/Driver", "alsa");
    sInterface  = settings.readEntry("/Interface", "hw:0");
    iAudio      = settings.readNumEntry("/Audio", 0);
    iDither     = settings.readNumEntry("/Dither", 0);
    iTimeout    = settings.readNumEntry("/Timeout", 500);
    bHWMon      = settings.readBoolEntry("/HWMon", false);
    bHWMeter    = settings.readBoolEntry("/HWMeter", false);
    sTempDir    = settings.readEntry("/TempDir", "(default)");
    settings.endGroup();

    settings.beginGroup("/Options");
//  bDetails         = settings.readBoolEntry("/Details", false);
    bVerbose         = settings.readBoolEntry("/Verbose", false);
//  bForceArts       = settings.readBoolEntry("/ForceArts", true);
//  sForceArtsShell  = settings.readEntry("/ForceArtsShell", "artsshell -q terminate");
//  bForceJack       = settings.readBoolEntry("/ForceJack", true);
//  sForceJackShell  = settings.readEntry("/ForceJackShell", "killall -9 jackd");
    bStartupScript       = settings.readBoolEntry("/StartupScript", false);
    sStartupScriptShell  = settings.readEntry("/StartupScriptShell", QString::null);
    bShutdownScript      = settings.readBoolEntry("/ShutdownScript", false);
    sShutdownScriptShell = settings.readEntry("/ShutdownScriptShell", QString::null);
    sXrunRegex       = settings.readEntry("/XrunRegex", "xrun of at least ([0-9|\\.]+) msecs");
    bXrunIgnoreFirst = settings.readBoolEntry("/XrunIgnoreFirst", false);
    bAutoRefresh     = settings.readBoolEntry("/AutoRefresh", false);
    iTimeRefresh     = settings.readNumEntry("/TimeRefresh", 10);
    settings.endGroup();

    settings.beginGroup("/Defaults");
    iTimeDisplay  = settings.readNumEntry("/TimeDisplay", 0);
    sMessagesFont = settings.readEntry("/MessagesFont", QString::null);
    sPatchbayPath = settings.readEntry("/PatchbayPath", QString::null);
    settings.endGroup();
}


// Profile unloader.
void qjackctlSetup::save (void)
{
    // Save all settings and options...
    settings.beginGroup("/Program");
    settings.writeEntry("/Version", QJACKCTL_VERSION);
    settings.endGroup();

    settings.beginGroup("/Settings");
    settings.writeEntry("/Server",     sServer);
    settings.writeEntry("/Realtime",   bRealtime);
    settings.writeEntry("/SoftMode",   bSoftMode);
    settings.writeEntry("/Asio",       bAsio);
    settings.writeEntry("/Monitor",    bMonitor);
    settings.writeEntry("/Chan",       iChan);
    settings.writeEntry("/Priority",   iPriority);
    settings.writeEntry("/Frames",     iFrames);
    settings.writeEntry("/SampleRate", iSampleRate);
    settings.writeEntry("/Periods",    iPeriods);
    settings.writeEntry("/Wait",       iWait);
    settings.writeEntry("/Driver",     sDriver);
    settings.writeEntry("/Interface",  sInterface);
    settings.writeEntry("/Audio",      iAudio);
    settings.writeEntry("/Dither",     iDither);
    settings.writeEntry("/Timeout",    iTimeout);
    settings.writeEntry("/HWMon",      bHWMon);
    settings.writeEntry("/HWMeter",    bHWMeter);
    settings.writeEntry("/TempDir",    sTempDir);
    settings.endGroup();

    settings.beginGroup("/Options");
//  settings.writeEntry("/Details",         bDetails);
    settings.writeEntry("/Verbose",         bVerbose);
//  settings.writeEntry("/ForceArts",       bForceArts);
//  settings.writeEntry("/ForceArtsShell",  sForceArtsShell);
//  settings.writeEntry("/ForceJack",       bForceJack);
//  settings.writeEntry("/ForceJackShell",  sForceJackShell);
    settings.writeEntry("/StartupScript",       bStartupScript);
    settings.writeEntry("/StartupScriptShell",  sStartupScriptShell);
    settings.writeEntry("/ShutdownScript",      bShutdownScript);
    settings.writeEntry("/ShutdownScriptShell", sShutdownScriptShell);
    settings.writeEntry("/XrunRegex",       sXrunRegex);
    settings.writeEntry("/XrunIgnoreFirst", bXrunIgnoreFirst);
    settings.writeEntry("/AutoRefresh",     bAutoRefresh);
    settings.writeEntry("/TimeRefresh",     iTimeRefresh);
    settings.endGroup();

    settings.beginGroup("/Defaults");
    settings.writeEntry("/TimeDisplay",  iTimeDisplay);
    settings.writeEntry("/MessagesFont", sMessagesFont);
    settings.writeEntry("/PatchbayPath", sPatchbayPath);
    settings.endGroup();
}


//---------------------------------------------------------------------------
// Combo box history persistence helper implementation.

void qjackctlSetup::add2ComboBoxHistory ( QComboBox *pComboBox, const QString& sNewText, int iLimit, int iIndex )
{
    int iCount = pComboBox->count();
    for (int i = 0; i < iCount; i++) {
        QString sText = pComboBox->text(i);
        if (sText == sNewText) {
            pComboBox->removeItem(i);
            iCount--;
            break;
        }
    }
    while (iCount >= iLimit)
        pComboBox->removeItem(--iCount);
    pComboBox->insertItem(sNewText, iIndex);
}


void qjackctlSetup::loadComboBoxHistory ( QComboBox *pComboBox, int iLimit )
{
    pComboBox->setUpdatesEnabled(false);
    pComboBox->setDuplicatesEnabled(false);

    settings.beginGroup("/" + QString(pComboBox->name()));
    for (int i = 0; i < iLimit; i++) {
        QString sText = settings.readEntry("/Item" + QString::number(i + 1), QString::null);
        if (sText.isEmpty())
            break;
        add2ComboBoxHistory(pComboBox, sText, iLimit);
    }
    settings.endGroup();

    pComboBox->setUpdatesEnabled(true);
}


void qjackctlSetup::saveComboBoxHistory ( QComboBox *pComboBox, int iLimit )
{
    add2ComboBoxHistory(pComboBox, pComboBox->currentText(), iLimit, 0);

    settings.beginGroup("/" + QString(pComboBox->name()));
    for (int i = 0; i < iLimit && i < pComboBox->count(); i++) {
        QString sText = pComboBox->text(i);
        if (sText.isEmpty())
            break;
        settings.writeEntry("/Item" + QString::number(i + 1), sText);
    }
    settings.endGroup();
}


//---------------------------------------------------------------------------
// Widget geometry persistence helper methods.

void qjackctlSetup::loadWidgetGeometry ( QWidget *pWidget )
{
    // Try to restore old form window positioning.
    if (pWidget) {
        QPoint fpos;
        QSize  fsize;
        settings.beginGroup("/Geometry/" + QString(pWidget->name()));
        fpos.setX(settings.readNumEntry("/x", -1));
        fpos.setY(settings.readNumEntry("/y", -1));
        fsize.setWidth(settings.readNumEntry("/width", -1));
        fsize.setHeight(settings.readNumEntry("/height", -1));
        settings.endGroup();
        if (fpos.x() > 0 && fpos.y() > 0)
            pWidget->move(fpos);
        if (fsize.width() > 0 && fsize.height() > 0) {
            pWidget->resize(fsize);
        } else {
            pWidget->adjustSize();
        }
    }
}


void qjackctlSetup::saveWidgetGeometry ( QWidget *pWidget )
{
    // Try to save form window position...
    if (pWidget) {
        QPoint fpos  = pWidget->pos();
        QSize  fsize = pWidget->size();
        settings.beginGroup("/Geometry/" + QString(pWidget->name()));
        settings.writeEntry("/x", fpos.x());
        settings.writeEntry("/y", fpos.y());
        settings.writeEntry("/width", fsize.width());
        settings.writeEntry("/height", fsize.height());
        settings.endGroup();
    }
}


// end of qjackctlSetup.cpp
