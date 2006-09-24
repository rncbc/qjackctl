// qjackctlSetup.cpp
//
/****************************************************************************
   Copyright (C) 2003-2006, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include <qtimer.h>


// Constructor.
qjackctlSetup::qjackctlSetup (void)
{
    bStartJack = false;
    sDefPresetName = QObject::tr("(default)");

    m_settings.beginGroup("/qjackctl");

    m_settings.beginGroup("/Presets");
    sDefPreset = m_settings.readEntry("/DefPreset", sDefPresetName);
    QString sPrefix = "/Preset%1";
    int i = 0;
    for (;;) {
        QString sItem = m_settings.readEntry(sPrefix.arg(++i), QString::null);
        if (sItem.isEmpty())
            break;
        presets.append(sItem);
    }
    m_settings.endGroup();

    m_settings.beginGroup("/Options");
    bStartJack               = m_settings.readBoolEntry("/StartJack", false);
    bStartupScript           = m_settings.readBoolEntry("/StartupScript", true);
    sStartupScriptShell      = m_settings.readEntry("/StartupScriptShell", "artsshell -q terminate");
    bPostStartupScript       = m_settings.readBoolEntry("/PostStartupScript", false);
    sPostStartupScriptShell  = m_settings.readEntry("/PostStartupScriptShell", QString::null);
    bShutdownScript          = m_settings.readBoolEntry("/ShutdownScript", false);
    sShutdownScriptShell     = m_settings.readEntry("/ShutdownScriptShell", QString::null);
    bPostShutdownScript      = m_settings.readBoolEntry("/PostShutdownScript", true);
    sPostShutdownScriptShell = m_settings.readEntry("/PostShutdownScriptShell", "killall jackd");
    bStdoutCapture           = m_settings.readBoolEntry("/StdoutCapture", true);
    sXrunRegex               = m_settings.readEntry("/XrunRegex", "xrun of at least ([0-9|\\.]+) msecs");
    bXrunIgnoreFirst         = m_settings.readBoolEntry("/XrunIgnoreFirst", false);
    bActivePatchbay          = m_settings.readBoolEntry("/ActivePatchbay", false);
    sActivePatchbayPath      = m_settings.readEntry("/ActivePatchbayPath", QString::null);
    bAutoRefresh             = m_settings.readBoolEntry("/AutoRefresh", false);
    iTimeRefresh             = m_settings.readNumEntry("/TimeRefresh", 10);
    bBezierLines             = m_settings.readBoolEntry("/BezierLines", false);
    iTimeDisplay             = m_settings.readNumEntry("/TimeDisplay", 0);
    iTimeFormat              = m_settings.readNumEntry("/TimeFormat", 0);
    sMessagesFont            = m_settings.readEntry("/MessagesFont", QString::null);
    bMessagesLimit           = m_settings.readBoolEntry("/MessagesLimit", true);
    iMessagesLimitLines      = m_settings.readNumEntry("/MessagesLimitLines", 1000);
    sDisplayFont1            = m_settings.readEntry("/DisplayFont1", QString::null);
    sDisplayFont2            = m_settings.readEntry("/DisplayFont2", QString::null);
    bDisplayEffect           = m_settings.readBoolEntry("/DisplayEffect", true);
    iConnectionsIconSize     = m_settings.readNumEntry("/ConnectionsIconSize", QJACKCTL_ICON_16X16);
    sConnectionsFont         = m_settings.readEntry("/ConnectionsFont", QString::null);
    bQueryClose              = m_settings.readBoolEntry("/QueryClose", true);
    // hack: default keep on top to false for Macs, because window focus looks bad
#ifdef __APPLE__
    bKeepOnTop               = m_settings.readBoolEntry("/KeepOnTop", false);
#else
    bKeepOnTop               = m_settings.readBoolEntry("/KeepOnTop", true);
#endif
    bSystemTray              = m_settings.readBoolEntry("/SystemTray", false);
    bDelayedSetup            = m_settings.readBoolEntry("/DelayedSetup", false);
    bServerConfig            = m_settings.readBoolEntry("/ServerConfig", true);
    sServerConfigName        = m_settings.readEntry("/ServerConfigName", ".jackdrc");
    bServerConfigTemp        = m_settings.readBoolEntry("/ServerConfigTemp", false);
    bQueryShutdown           = m_settings.readBoolEntry("/QueryShutdown", true);
    bAliasesEnabled          = m_settings.readBoolEntry("/AliasesEnabled", false);
    bAliasesEditing          = m_settings.readBoolEntry("/AliasesEditing", false);
    bLeftButtons             = m_settings.readBoolEntry("/LeftButtons", true);
    bRightButtons            = m_settings.readBoolEntry("/RightButtons", true);
    bTransportButtons        = m_settings.readBoolEntry("/TransportButtons", true);
    bTextLabels              = m_settings.readBoolEntry("/TextLabels", true);
    m_settings.endGroup();

    m_settings.beginGroup("/Defaults");
    sPatchbayPath = m_settings.readEntry("/PatchbayPath", QString::null);
    m_settings.endGroup();

	// Load recent patchbay list...
	m_settings.beginGroup("/Patchbays");
	sPrefix = "/Patchbay%1";
	i = 0;
	for (;;) {
		QString sItem = m_settings.readEntry(sPrefix.arg(++i), QString::null);
		if (sItem.isEmpty())
			break;
		patchbays.append(sItem);
	}
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
	QString sPrefix = "/Preset%1";
	int i = 0;
	for (QStringList::Iterator iter = presets.begin();
		iter != presets.end(); iter++)
	    m_settings.writeEntry(sPrefix.arg(++i), *iter);
	// Cleanup old entries, if any...
	for (++i; !m_settings.readEntry(sPrefix.arg(i)).isEmpty(); i++)
	    m_settings.removeEntry(sPrefix.arg(i));
	m_settings.endGroup();

    m_settings.beginGroup("/Options");
    m_settings.writeEntry("/StartJack",               bStartJack);
    m_settings.writeEntry("/StartupScript",           bStartupScript);
    m_settings.writeEntry("/StartupScriptShell",      sStartupScriptShell);
    m_settings.writeEntry("/PostStartupScript",       bPostStartupScript);
    m_settings.writeEntry("/PostStartupScriptShell",  sPostStartupScriptShell);
    m_settings.writeEntry("/ShutdownScript",          bShutdownScript);
    m_settings.writeEntry("/ShutdownScriptShell",     sShutdownScriptShell);
    m_settings.writeEntry("/PostShutdownScript",      bPostShutdownScript);
    m_settings.writeEntry("/PostShutdownScriptShell", sPostShutdownScriptShell);
    m_settings.writeEntry("/StdoutCapture",           bStdoutCapture);
    m_settings.writeEntry("/XrunRegex",               sXrunRegex);
    m_settings.writeEntry("/XrunIgnoreFirst",         bXrunIgnoreFirst);
    m_settings.writeEntry("/ActivePatchbay",          bActivePatchbay);
    m_settings.writeEntry("/ActivePatchbayPath",      sActivePatchbayPath);
    m_settings.writeEntry("/AutoRefresh",             bAutoRefresh);
    m_settings.writeEntry("/TimeRefresh",             iTimeRefresh);
    m_settings.writeEntry("/BezierLines",             bBezierLines);
    m_settings.writeEntry("/TimeDisplay",             iTimeDisplay);
    m_settings.writeEntry("/TimeFormat",              iTimeFormat);
    m_settings.writeEntry("/MessagesFont",            sMessagesFont);
    m_settings.writeEntry("/MessagesLimit",           bMessagesLimit);
    m_settings.writeEntry("/MessagesLimitLines",      iMessagesLimitLines);
    m_settings.writeEntry("/DisplayFont1",            sDisplayFont1);
    m_settings.writeEntry("/DisplayFont2",            sDisplayFont2);
    m_settings.writeEntry("/DisplayEffect",           bDisplayEffect);
    m_settings.writeEntry("/ConnectionsIconSize",     iConnectionsIconSize);
    m_settings.writeEntry("/ConnectionsFont",         sConnectionsFont);
    m_settings.writeEntry("/QueryClose",              bQueryClose);
    m_settings.writeEntry("/KeepOnTop",               bKeepOnTop);
    m_settings.writeEntry("/SystemTray",              bSystemTray);
    m_settings.writeEntry("/DelayedSetup",            bDelayedSetup);
    m_settings.writeEntry("/ServerConfig",            bServerConfig);
    m_settings.writeEntry("/ServerConfigName",        sServerConfigName);
    m_settings.writeEntry("/ServerConfigTemp",        bServerConfigTemp);
    m_settings.writeEntry("/QueryShutdown",           bQueryShutdown);
    m_settings.writeEntry("/AliasesEnabled",          bAliasesEnabled);
    m_settings.writeEntry("/AliasesEditing",          bAliasesEditing);
    m_settings.writeEntry("/LeftButtons",             bLeftButtons);
    m_settings.writeEntry("/RightButtons",            bRightButtons);
    m_settings.writeEntry("/TransportButtons",        bTransportButtons);
    m_settings.writeEntry("/TextLabels",              bTextLabels);
    m_settings.endGroup();

    m_settings.beginGroup("/Defaults");
    m_settings.writeEntry("/PatchbayPath", sPatchbayPath);
    m_settings.endGroup();

	// Save patchbay list...
	m_settings.beginGroup("/Patchbays");
	sPrefix = "/Patchbay%1";
	i = 0;
	for (QStringList::Iterator iter = patchbays.begin();
			iter != patchbays.end(); ++iter)
		m_settings.writeEntry(sPrefix.arg(++i), *iter);
	// Cleanup old entries, if any...
	for (++i; !m_settings.readEntry(sPrefix.arg(i)).isEmpty(); i++)
		m_settings.removeEntry(sPrefix.arg(i));
	m_settings.endGroup();

    m_settings.endGroup();
}


//---------------------------------------------------------------------------
// Aliases preset management methods.

bool qjackctlSetup::loadAliases ( const QString& sPreset )
{
    QString sSuffix;
    if (sPreset != sDefPresetName && !sPreset.isEmpty()) {
        sSuffix = "/" + sPreset;
        // Check if on list.
        if (presets.find(sPreset) == presets.end())
            return false;
    }

	// Load preset aliases...
	const QString sAliasesKey = "/Aliases" + sSuffix;
    m_settings.beginGroup(sAliasesKey);
	  m_settings.beginGroup("/Jack");
	 	aliasJackOutputs.loadSettings(m_settings, "/Outputs");
	 	aliasJackInputs.loadSettings(m_settings, "/Inputs");
	  m_settings.endGroup();
	  m_settings.beginGroup("/Alsa");
	 	aliasAlsaOutputs.loadSettings(m_settings, "/Outputs");
	 	aliasAlsaInputs.loadSettings(m_settings, "/Inputs");
	  m_settings.endGroup();
    m_settings.endGroup();

    return true;
}

bool qjackctlSetup::saveAliases ( const QString& sPreset )
{
    QString sSuffix;
    if (sPreset != sDefPresetName && !sPreset.isEmpty()) {
        sSuffix = "/" + sPreset;
        // Append to list if not already.
        if (presets.find(sPreset) == presets.end())
            presets.prepend(sPreset);
    }

	// Save preset aliases...
	const QString sAliasesKey = "/Aliases" + sSuffix;
	deleteKey(sAliasesKey);
    m_settings.beginGroup(sAliasesKey);
	  m_settings.beginGroup("/Jack");
	 	aliasJackOutputs.saveSettings(m_settings, "/Outputs");
	 	aliasJackInputs.saveSettings(m_settings, "/Inputs");
	  m_settings.endGroup();
	  m_settings.beginGroup("/Alsa");
	 	aliasAlsaOutputs.saveSettings(m_settings, "/Outputs");
	 	aliasAlsaInputs.saveSettings(m_settings, "/Inputs");
	  m_settings.endGroup();
    m_settings.endGroup();

    return true;
}


//---------------------------------------------------------------------------
// Preset management methods.

bool qjackctlSetup::loadPreset ( qjackctlPreset& preset, const QString& sPreset )
{
    QString sSuffix;
    if (sPreset != sDefPresetName && !sPreset.isEmpty()) {
        sSuffix = "/" + sPreset;
        // Check if on list.
        if (presets.find(sPreset) == presets.end())
            return false;
    }

    m_settings.beginGroup("/Settings" + sSuffix);
    preset.sServer      = m_settings.readEntry("/Server", "jackd");
    preset.bRealtime    = m_settings.readBoolEntry("/Realtime", true);
    preset.bSoftMode    = m_settings.readBoolEntry("/SoftMode", false);
    preset.bMonitor     = m_settings.readBoolEntry("/Monitor", false);
    preset.bShorts      = m_settings.readBoolEntry("/Shorts", false);
    preset.bNoMemLock   = m_settings.readBoolEntry("/NoMemLock", false);
    preset.bUnlockMem   = m_settings.readBoolEntry("/UnlockMem", false);
    preset.bHWMon       = m_settings.readBoolEntry("/HWMon", false);
    preset.bHWMeter     = m_settings.readBoolEntry("/HWMeter", false);
    preset.bIgnoreHW    = m_settings.readBoolEntry("/IgnoreHW", false);
    preset.iPriority    = m_settings.readNumEntry("/Priority", 0);
    preset.iFrames      = m_settings.readNumEntry("/Frames", 1024);
    preset.iSampleRate  = m_settings.readNumEntry("/SampleRate", 48000);
    preset.iPeriods     = m_settings.readNumEntry("/Periods", 2);
    preset.iWordLength  = m_settings.readNumEntry("/WordLength", 16);
    preset.iWait        = m_settings.readNumEntry("/Wait", 21333);
    preset.iChan        = m_settings.readNumEntry("/Chan", 0);
    preset.sDriver      = m_settings.readEntry("/Driver", "alsa");
    preset.sInterface   = m_settings.readEntry("/Interface", QString::null);
    preset.iAudio       = m_settings.readNumEntry("/Audio", 0);
    preset.iDither      = m_settings.readNumEntry("/Dither", 0);
    preset.iTimeout     = m_settings.readNumEntry("/Timeout", 500);
    preset.sInDevice    = m_settings.readEntry("/InDevice", QString::null);
    preset.sOutDevice   = m_settings.readEntry("/OutDevice", QString::null);
    preset.iInChannels  = m_settings.readNumEntry("/InChannels", 0);
    preset.iOutChannels = m_settings.readNumEntry("/OutChannels", 0);
    preset.iInLatency   = m_settings.readNumEntry("/InLatency", 0);
    preset.iOutLatency  = m_settings.readNumEntry("/OutLatency", 0);
    preset.iStartDelay  = m_settings.readNumEntry("/StartDelay", 2);
    preset.bVerbose     = m_settings.readBoolEntry("/Verbose", false);
    preset.iPortMax     = m_settings.readNumEntry("/PortMax", 256);
    m_settings.endGroup();

    return true;
}

bool qjackctlSetup::savePreset ( qjackctlPreset& preset, const QString& sPreset )
{
    QString sSuffix;
    if (sPreset != sDefPresetName && !sPreset.isEmpty()) {
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
    m_settings.writeEntry("/NoMemLock",   preset.bNoMemLock);
    m_settings.writeEntry("/UnlockMem",   preset.bUnlockMem);
    m_settings.writeEntry("/HWMon",       preset.bHWMon);
    m_settings.writeEntry("/HWMeter",     preset.bHWMeter);
    m_settings.writeEntry("/IgnoreHW",    preset.bIgnoreHW);
    m_settings.writeEntry("/Priority",    preset.iPriority);
    m_settings.writeEntry("/Frames",      preset.iFrames);
    m_settings.writeEntry("/SampleRate",  preset.iSampleRate);
    m_settings.writeEntry("/Periods",     preset.iPeriods);
    m_settings.writeEntry("/WordLength",  preset.iWordLength);
    m_settings.writeEntry("/Wait",        preset.iWait);
    m_settings.writeEntry("/Chan",        preset.iChan);
    m_settings.writeEntry("/Driver",      preset.sDriver);
    m_settings.writeEntry("/Interface",   preset.sInterface);
    m_settings.writeEntry("/Audio",       preset.iAudio);
    m_settings.writeEntry("/Dither",      preset.iDither);
    m_settings.writeEntry("/Timeout",     preset.iTimeout);
    m_settings.writeEntry("/InDevice",    preset.sInDevice);
    m_settings.writeEntry("/OutDevice",   preset.sOutDevice);
    m_settings.writeEntry("/InChannels",  preset.iInChannels);
    m_settings.writeEntry("/OutChannels", preset.iOutChannels);
    m_settings.writeEntry("/InLatency",   preset.iInLatency);
    m_settings.writeEntry("/OutLatency",  preset.iOutLatency);
    m_settings.writeEntry("/StartDelay",  preset.iStartDelay);
    m_settings.writeEntry("/Verbose",     preset.bVerbose);
    m_settings.writeEntry("/PortMax",     preset.iPortMax);
    m_settings.endGroup();

    return true;
}

bool qjackctlSetup::deletePreset ( const QString& sPreset )
{
    QString sSuffix;
    if (sPreset != sDefPresetName && !sPreset.isEmpty()) {
        sSuffix = "/" + sPreset;
        QStringList::Iterator iter = presets.find(sPreset);
        if (iter == presets.end())
            return false;
        presets.remove(iter);
        deleteKey("/Settings" + sSuffix);
    	deleteKey("/Aliases" + sSuffix);
    }
    return true;
}


//---------------------------------------------------------------------------
// A recursive QSettings key entry remover.

void qjackctlSetup::deleteKey ( const QString& sKey )
{
    // First, delete all stand-alone entries...
    QStringList entries = m_settings.entryList(sKey);
    for (QStringList::Iterator entry = entries.begin(); entry != entries.end(); ++entry) {
        const QString& sEntry = *entry;
        if (!sEntry.isEmpty())
            m_settings.removeEntry(sKey + "/" + sEntry);
    }

    // Then, we'll recurse under sub-keys...
    QStringList subkeys = m_settings.subkeyList(sKey);
    for (QStringList::Iterator subkey = subkeys.begin(); subkey != subkeys.end(); ++subkey) {
        const QString& sSubKey = *subkey;
        if (!sSubKey.isEmpty())
            deleteKey(sKey + "/" + sSubKey);
    }

    // Finally we remove our-selves.
    m_settings.removeEntry(sKey);
}


//-------------------------------------------------------------------------
// Command-line argument stuff.
//

// Help about command line options.
void qjackctlSetup::print_usage ( const char *arg0 )
{
    const QString sEot = "\n\t";
    const QString sEol = "\n\n";

    fprintf(stderr, QObject::tr("Usage") + ": %s [" + QObject::tr("options") + "] [" +
        QObject::tr("command-and-args") + "]" + sEol, arg0);
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
    int iCmdArgs = 0;

    for (int i = 1; i < argc; i++) {

        if (iCmdArgs > 0) {
            sCmdLine += " ";
            sCmdLine += argv[i];
            iCmdArgs++;
            continue;
        }

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
            // Here starts the optional command line...
            sCmdLine += sArg;
            iCmdArgs++;
        }
    }

    // HACK: If there's a command line, it must be spawned on background...
    if (iCmdArgs > 0)
        sCmdLine += " &";

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
// Splitter widget sizes persistence helper methods.

void qjackctlSetup::loadSplitterSizes ( QSplitter *pSplitter )
{
	// Try to restore old splitter sizes...
	if (pSplitter) {
		m_settings.beginGroup("/Splitter/" + QString(pSplitter->name()));
		QValueList<int> sizes;
		QStringList list = m_settings.readListEntry("/sizes");
		QStringList::Iterator iter = list.begin();
		while (iter != list.end())
		    sizes.append((*iter++).toInt());
		if (!sizes.isEmpty())
		    pSplitter->setSizes(sizes);
        m_settings.endGroup();
	}
}


void qjackctlSetup::saveSplitterSizes ( QSplitter *pSplitter )
{
    // Try to save current splitter sizes...
    if (pSplitter) {
        m_settings.beginGroup("/Splitter/" + QString(pSplitter->name()));
        QStringList list;
		QValueList<int> sizes = pSplitter->sizes();
		QValueList<int>::Iterator iter = sizes.begin();
		while (iter != sizes.end())
		    list.append(QString::number(*iter++));
		if (!list.isEmpty())
	        m_settings.writeEntry("/sizes", list);
        m_settings.endGroup();
    }
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
		new qjackctlDelayedSetup(pWidget, fpos, fsize, bVisible,
			(bDelayedSetup ? 1000 : 0));
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


//---------------------------------------------------------------------------
// Delayed setup option.

// Delayed widget setup helper class.
qjackctlDelayedSetup::qjackctlDelayedSetup ( QWidget *pWidget,
	const QPoint& pos, const QSize& size, bool bVisible, int iDelay )
	: m_pos(pos), m_size(size)
{
	m_pWidget  = pWidget;
	m_bVisible = bVisible;

	if (iDelay > 0) {
		QTimer::singleShot(iDelay, this, SLOT(setup()));
	} else {
		setup();
	}
}


void qjackctlDelayedSetup::setup (void)
{
	if (m_pWidget) {
		if (m_pos.x() > 0 && m_pos.y() > 0)
			m_pWidget->move(m_pos);
		if (m_size.width() > 0 && m_size.height() > 0)
			m_pWidget->resize(m_size);
		else
			m_pWidget->adjustSize();
		if (m_bVisible)
			m_pWidget->show();
	}
	deleteLater();
}


// end of qjackctlSetup.cpp
