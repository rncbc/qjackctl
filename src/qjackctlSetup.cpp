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
    bStartJack = false;

    m_settings.beginGroup("/qjackctl");

    m_settings.beginGroup("/Presets");
    sDefPreset = m_settings.readEntry("/DefPreset", "(default)");
    const QString sPrefix = "/Preset";
    int i = 0;
    for (;;) {
        QString sItem = m_settings.readEntry(sPrefix + QString::number(++i), QString::null);
        if (sItem.isEmpty())
            break;
        presets.append(sItem);
    }
    m_settings.endGroup();

    m_settings.beginGroup("/Options");
    bStartJack              = m_settings.readBoolEntry("/StartJack", false);
    bStartupScript          = m_settings.readBoolEntry("/StartupScript", true);
    sStartupScriptShell     = m_settings.readEntry("/StartupScriptShell", "artsshell -q terminate");
    bPostStartupScript      = m_settings.readBoolEntry("/PostStartupScript", false);
    sPostStartupScriptShell = m_settings.readEntry("/PostStartupScriptShell", QString::null);
    bShutdownScript         = m_settings.readBoolEntry("/ShutdownScript", false);
    sShutdownScriptShell    = m_settings.readEntry("/ShutdownScriptShell", QString::null);
    bStdoutCapture          = m_settings.readBoolEntry("/StdoutCapture", true);
    sXrunRegex              = m_settings.readEntry("/XrunRegex", "xrun of at least ([0-9|\\.]+) msecs");
    bXrunIgnoreFirst        = m_settings.readBoolEntry("/XrunIgnoreFirst", false);
    bActivePatchbay         = m_settings.readBoolEntry("/ActivePatchbay", false);
    sActivePatchbayPath     = m_settings.readEntry("/ActivePatchbayPath", QString::null);
    bAutoRefresh            = m_settings.readBoolEntry("/AutoRefresh", false);
    iTimeRefresh            = m_settings.readNumEntry("/TimeRefresh", 10);
    iTimeDisplay            = m_settings.readNumEntry("/TimeDisplay", 0);
    sMessagesFont           = m_settings.readEntry("/MessagesFont", QString::null);
    sDisplayFont1           = m_settings.readEntry("/DisplayFont1", QString::null);
    sDisplayFont2           = m_settings.readEntry("/DisplayFont2", QString::null);
    bQueryClose             = m_settings.readBoolEntry("/QueryClose", true);
    m_settings.endGroup();

    m_settings.beginGroup("/Defaults");
    sPatchbayPath = m_settings.readEntry("/PatchbayPath", QString::null);
    m_settings.endGroup();
}


// Destructor;
qjackctlSetup::~qjackctlSetup (void)
{
    // Save all settings and options...
    m_settings.beginGroup("/Program");
    m_settings.writeEntry("/Version", QJACKCTL_VERSION);
    m_settings.endGroup();

    m_settings.beginGroup("/Presets");
    m_settings.writeEntry("/DefPreset", sDefPreset);
    // Save last preset list.
    const QString sPrefix = "/Preset";
    int i = 0;
    for (QStringList::Iterator iter = presets.begin(); iter != presets.end(); iter++)
        m_settings.writeEntry(sPrefix + QString::number(++i), *iter);
    // Cleanup old entries, if any...
    for (++i; !m_settings.readEntry(sPrefix + QString::number(i)).isEmpty(); i++)
        m_settings.removeEntry(sPrefix + QString::number(i));
    m_settings.endGroup();

    m_settings.beginGroup("/Options");
    m_settings.writeEntry("/StartJack",               bStartJack);
    m_settings.writeEntry("/StartupScript",           bStartupScript);
    m_settings.writeEntry("/StartupScriptShell",      sStartupScriptShell);
    m_settings.writeEntry("/PostStartupScript",       bPostStartupScript);
    m_settings.writeEntry("/PostStartupScriptShell",  sPostStartupScriptShell);
    m_settings.writeEntry("/ShutdownScript",          bShutdownScript);
    m_settings.writeEntry("/ShutdownScriptShell",     sShutdownScriptShell);
    m_settings.writeEntry("/StdoutCapture",           bStdoutCapture);
    m_settings.writeEntry("/XrunRegex",               sXrunRegex);
    m_settings.writeEntry("/XrunIgnoreFirst",         bXrunIgnoreFirst);
    m_settings.writeEntry("/ActivePatchbay",          bActivePatchbay);
    m_settings.writeEntry("/ActivePatchbayPath",      sActivePatchbayPath);
    m_settings.writeEntry("/AutoRefresh",             bAutoRefresh);
    m_settings.writeEntry("/TimeRefresh",             iTimeRefresh);
    m_settings.writeEntry("/TimeDisplay",             iTimeDisplay);
    m_settings.writeEntry("/MessagesFont",            sMessagesFont);
    m_settings.writeEntry("/DisplayFont1",            sDisplayFont1);
    m_settings.writeEntry("/DisplayFont2",            sDisplayFont2);
    m_settings.writeEntry("/QueryClose",              bQueryClose);
    m_settings.endGroup();

    m_settings.beginGroup("/Defaults");
    m_settings.writeEntry("/PatchbayPath", sPatchbayPath);
    m_settings.endGroup();

    m_settings.endGroup();
}


//---------------------------------------------------------------------------
// Preset management methods.

bool qjackctlSetup::loadPreset ( qjackctlPreset& preset, const QString& sPreset )
{
    QString sSuffix;
    if (sPreset != "(default)" && !sPreset.isEmpty()) {
        sSuffix = "/" + sPreset;
        // Check if on list.
        if (presets.find(sPreset) == presets.end())
            return false;
    }

    m_settings.beginGroup("/Settings" + sSuffix);
    preset.sServer      = m_settings.readEntry("/Server", "jackstart");
    preset.bRealtime    = m_settings.readBoolEntry("/Realtime", true);
    preset.bSoftMode    = m_settings.readBoolEntry("/SoftMode", false);
    preset.bMonitor     = m_settings.readBoolEntry("/Monitor", false);
    preset.bShorts      = m_settings.readBoolEntry("/Shorts", false);
    preset.iChan        = m_settings.readNumEntry("/Chan", 0);
    preset.iPriority    = m_settings.readNumEntry("/Priority", 0);
    preset.iFrames      = m_settings.readNumEntry("/Frames", 1024);
    preset.iSampleRate  = m_settings.readNumEntry("/SampleRate", 48000);
    preset.iPeriods     = m_settings.readNumEntry("/Periods", 2);
    preset.iWait        = m_settings.readNumEntry("/Wait", 21333);
    preset.sDriver      = m_settings.readEntry("/Driver", "alsa");
    preset.sInterface   = m_settings.readEntry("/Interface", "hw:0");
    preset.iAudio       = m_settings.readNumEntry("/Audio", 0);
    preset.iDither      = m_settings.readNumEntry("/Dither", 0);
    preset.iTimeout     = m_settings.readNumEntry("/Timeout", 500);
    preset.bHWMon       = m_settings.readBoolEntry("/HWMon", false);
    preset.bHWMeter     = m_settings.readBoolEntry("/HWMeter", false);
    preset.iInChannels  = m_settings.readNumEntry("/InChannels", 0);
    preset.iOutChannels = m_settings.readNumEntry("/OutChannels", 0);
    preset.iStartDelay  = m_settings.readNumEntry("/StartDelay", 2);
    preset.bVerbose     = m_settings.readBoolEntry("/Verbose", false);
    m_settings.endGroup();

    return true;
}

bool qjackctlSetup::savePreset ( qjackctlPreset& preset, const QString& sPreset )
{
    QString sSuffix;
    if (sPreset != "(default)" && !sPreset.isEmpty()) {
        sSuffix = "/" + sPreset;
        // Append to list if not already.
        if (presets.find(sPreset) == presets.end())
            presets.prepend(sPreset);
    }

    m_settings.beginGroup("/Settings" + sSuffix);
    m_settings.writeEntry("/Server",      preset.sServer);
    m_settings.writeEntry("/Realtime",    preset.bRealtime);
    m_settings.writeEntry("/SoftMode",    preset.bSoftMode);
    m_settings.writeEntry("/Monitor",     preset.bMonitor);
    m_settings.writeEntry("/Shorts",      preset.bShorts);
    m_settings.writeEntry("/Chan",        preset.iChan);
    m_settings.writeEntry("/Priority",    preset.iPriority);
    m_settings.writeEntry("/Frames",      preset.iFrames);
    m_settings.writeEntry("/SampleRate",  preset.iSampleRate);
    m_settings.writeEntry("/Periods",     preset.iPeriods);
    m_settings.writeEntry("/Wait",        preset.iWait);
    m_settings.writeEntry("/Driver",      preset.sDriver);
    m_settings.writeEntry("/Interface",   preset.sInterface);
    m_settings.writeEntry("/Audio",       preset.iAudio);
    m_settings.writeEntry("/Dither",      preset.iDither);
    m_settings.writeEntry("/Timeout",     preset.iTimeout);
    m_settings.writeEntry("/HWMon",       preset.bHWMon);
    m_settings.writeEntry("/HWMeter",     preset.bHWMeter);
    m_settings.writeEntry("/InChannels",  preset.iInChannels);
    m_settings.writeEntry("/OutChannels", preset.iOutChannels);
    m_settings.writeEntry("/StartDelay",  preset.iStartDelay);
    m_settings.writeEntry("/Verbose",     preset.bVerbose);
    m_settings.endGroup();

    return true;
}

bool qjackctlSetup::deletePreset ( const QString& sPreset )
{
    QString sSuffix;
    if (sPreset != "(default)" && !sPreset.isEmpty()) {
        sSuffix = "/" + sPreset;
        QStringList::Iterator iter = presets.find(sPreset);
        if (iter == presets.end())
            return false;
        presets.remove(iter);
        m_settings.removeEntry("/Settings" + sSuffix);
    }
    return true;
}

//-------------------------------------------------------------------------
// Command-line argument stuff.
//

// Help about command line options.
void qjackctlSetup::print_usage ( const char *arg0 )
{
    const QString sEot = "\n\t";
    const QString sEol = "\n\n";

    fprintf(stderr, QObject::tr("Usage") + ": %s [" + QObject::tr("options")    + "]" + sEol, arg0);
    fprintf(stderr, "%s - %s\n\n", QJACKCTL_TITLE, QJACKCTL_SUBTITLE);
    fprintf(stderr, QObject::tr("Options") + ":" + sEol);
    fprintf(stderr, "  -s, --start" + sEot +
        QObject::tr("Start JACK audio server immediately") + sEol);
    fprintf(stderr, "  -p, --preset=[label]" + sEot +
        QObject::tr("Set default setings preset name") + sEol);
    fprintf(stderr, "  -h, --help" + sEot +
        QObject::tr("Show help about command line options") + sEol);
    fprintf(stderr, "  -v, --version" + sEot +
        QObject::tr("Show version information") + sEol);
}


// Parse command line arguments into m_settings.
bool qjackctlSetup::parse_args ( int argc, char **argv )
{
    const QString sEol = "\n\n";

    for (int i = 1; i < argc; i++) {

        QString sArg = argv[i];
        QString sVal = QString::null;
        int iEqual = sArg.find("=");
        if (iEqual >= 0) {
            sVal = sArg.right(sArg.length() - iEqual - 1);
            sArg = sArg.left(iEqual);
        }
        else if (i < argc)
            sVal = argv[i + 1];

        if (sArg == "-s" || sArg == "--start") {
            bStartJack = true;
        }
        else if (sArg == "-p" || sArg == "--preset") {
        	if (sVal.isNull()) {
                fprintf(stderr, QObject::tr("Option -p requires an argument (preset).") + sEol);
                return false;
            }
            sDefPreset = sVal;
            if (iEqual < 0)
                i++;
        }
        else if (sArg == "-h" || sArg == "--help") {
            print_usage(argv[0]);
            return false;
        }
        else if (sArg == "-v" || sArg == "--version") {
            fprintf(stderr, "Qt: %s\n", qVersion());
            fprintf(stderr, "qjackctl: %s\n", QJACKCTL_VERSION);
            return false;
        }
        else {
            fprintf(stderr, QObject::tr("Unknown option") + " %s" + sEol, argv[i]);
            print_usage(argv[0]);
            return false;
        }
    }

    // Alright with argument parsing.
    return true;
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

    m_settings.beginGroup("/History/" + QString(pComboBox->name()));
    for (int i = 0; i < iLimit; i++) {
        QString sText = m_settings.readEntry("/Item" + QString::number(i + 1), QString::null);
        if (sText.isEmpty())
            break;
        add2ComboBoxHistory(pComboBox, sText, iLimit);
    }
    m_settings.endGroup();

    pComboBox->setUpdatesEnabled(true);
}


void qjackctlSetup::saveComboBoxHistory ( QComboBox *pComboBox, int iLimit )
{
    add2ComboBoxHistory(pComboBox, pComboBox->currentText(), iLimit, 0);

    m_settings.beginGroup("/History/" + QString(pComboBox->name()));
    for (int i = 0; i < iLimit && i < pComboBox->count(); i++) {
        QString sText = pComboBox->text(i);
        if (sText.isEmpty())
            break;
        m_settings.writeEntry("/Item" + QString::number(i + 1), sText);
    }
    m_settings.endGroup();
}


//---------------------------------------------------------------------------
// Widget geometry persistence helper methods.

void qjackctlSetup::loadWidgetGeometry ( QWidget *pWidget )
{
    // Try to restore old form window positioning.
    if (pWidget) {
        QPoint fpos;
        QSize  fsize;
        bool bVisible;
        m_settings.beginGroup("/Geometry/" + QString(pWidget->name()));
        fpos.setX(m_settings.readNumEntry("/x", -1));
        fpos.setY(m_settings.readNumEntry("/y", -1));
        fsize.setWidth(m_settings.readNumEntry("/width", -1));
        fsize.setHeight(m_settings.readNumEntry("/height", -1));
        bVisible = m_settings.readBoolEntry("/visible", false);
        m_settings.endGroup();
        if (fpos.x() > 0 && fpos.y() > 0)
            pWidget->move(fpos);
        if (fsize.width() > 0 && fsize.height() > 0)
            pWidget->resize(fsize);
        else
            pWidget->adjustSize();
        if (bVisible)
            pWidget->show();
    }
}


void qjackctlSetup::saveWidgetGeometry ( QWidget *pWidget )
{
    // Try to save form window position...
    // (due to X11 window managers ideossincrasies, we better
    // only save the form geometry while its up and visible)
    if (pWidget) {
        m_settings.beginGroup("/Geometry/" + QString(pWidget->name()));
        bool bVisible = pWidget->isVisible();
        if (bVisible) {
            QPoint fpos  = pWidget->pos();
            QSize  fsize = pWidget->size();
            m_settings.writeEntry("/x", fpos.x());
            m_settings.writeEntry("/y", fpos.y());
            m_settings.writeEntry("/width", fsize.width());
            m_settings.writeEntry("/height", fsize.height());
        }
        m_settings.writeEntry("/visible", bVisible);
        m_settings.endGroup();
    }
}


// end of qjackctlSetup.cpp
