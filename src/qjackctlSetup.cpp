// qjackctlSetup.cpp
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

#include "qjackctlAbout.h"
#include "qjackctlSetup.h"

#include <QComboBox>
#include <QSplitter>
#include <QList>

#include <QTimer>
#include <QTextStream>
#include <QFileInfo>

#include <QApplication>
#include <QDesktopWidget>

#ifdef CONFIG_JACK_VERSION
#include <jack/jack.h>
#endif

#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
#define DEFAULT_DRIVER "portaudio"
#else
#define DEFAULT_DRIVER "alsa"
#endif


// Constructor.
qjackctlSetup::qjackctlSetup (void)
	: m_settings(QJACKCTL_DOMAIN, QJACKCTL_TITLE)
{
	bStartJackCmd = false;
	sDefPresetName = QObject::tr("(default)");

	loadSetup();
}

// Destructor;
qjackctlSetup::~qjackctlSetup (void)
{
//	saveSetup();
}


// Settings accessor.
QSettings& qjackctlSetup::settings (void)
{
	return m_settings;
}


// Explicit load method.
void qjackctlSetup::loadSetup (void)
{
	m_settings.beginGroup("/Presets");
	sDefPreset = m_settings.value("/DefPreset", sDefPresetName).toString();
	QString sPrefix = "/Preset%1";
	int i = 0;
	for (;;) {
		QString sItem = m_settings.value(sPrefix.arg(++i)).toString();
		if (sItem.isEmpty())
			break;
		presets.append(sItem);
	}
	m_settings.endGroup();

	m_settings.beginGroup("/Options");
	bSingleton               = m_settings.value("/Singleton", true).toBool();
//	sServerName              = m_settings.value("/ServerName").toString();
	bStartJack               = m_settings.value("/StartJack", false).toBool();
//	bStartJackCmd            = m_settings.value("/StartJackCmd", false).toBool();
	bStopJack                = m_settings.value("/StopJack", true).toBool();
	bStartupScript           = m_settings.value("/StartupScript", false).toBool();
	sStartupScriptShell      = m_settings.value("/StartupScriptShell").toString();
	bPostStartupScript       = m_settings.value("/PostStartupScript", false).toBool();
	sPostStartupScriptShell  = m_settings.value("/PostStartupScriptShell").toString();
	bShutdownScript          = m_settings.value("/ShutdownScript", false).toBool();
	sShutdownScriptShell     = m_settings.value("/ShutdownScriptShell").toString();
	bPostShutdownScript      = m_settings.value("/PostShutdownScript", false).toBool();
	sPostShutdownScriptShell = m_settings.value("/PostShutdownScriptShell").toString();
	bStdoutCapture           = m_settings.value("/StdoutCapture", true).toBool();
	sXrunRegex               = m_settings.value("/XrunRegex", "xrun of at least ([0-9|\\.]+) msecs").toString();
	bActivePatchbay          = m_settings.value("/ActivePatchbay", false).toBool();
	sActivePatchbayPath      = m_settings.value("/ActivePatchbayPath").toString();
	bActivePatchbayReset     = m_settings.value("/ActivePatchbayReset", false).toBool();
	bQueryDisconnect         = m_settings.value("/QueryDisconnect", true).toBool();
	bMessagesLog             = m_settings.value("/MessagesLog", false).toBool();
	sMessagesLogPath         = m_settings.value("/MessagesLogPath", "qjackctl.log").toString();
	iTimeDisplay             = m_settings.value("/TimeDisplay", 0).toInt();
	sMessagesFont            = m_settings.value("/MessagesFont").toString();
	bMessagesLimit           = m_settings.value("/MessagesLimit", true).toBool();
	iMessagesLimitLines      = m_settings.value("/MessagesLimitLines", 1000).toInt();
	sDisplayFont1            = m_settings.value("/DisplayFont1").toString();
	sDisplayFont2            = m_settings.value("/DisplayFont2").toString();
	bDisplayEffect           = m_settings.value("/DisplayEffect", true).toBool();
	bDisplayBlink            = m_settings.value("/DisplayBlink", true).toBool();
	iJackClientPortAlias     = m_settings.value("/JackClientPortAlias", 0).toInt();
	bJackClientPortMetadata  = m_settings.value("/JackClientPortMetadata", false).toBool();
	iConnectionsIconSize     = m_settings.value("/ConnectionsIconSize", QJACKCTL_ICON_16X16).toInt();
	sConnectionsFont         = m_settings.value("/ConnectionsFont").toString();
	bQueryClose              = m_settings.value("/QueryClose", true).toBool();
	bQueryShutdown           = m_settings.value("/QueryShutdown", true).toBool();
	bKeepOnTop               = m_settings.value("/KeepOnTop", false).toBool();
	bSystemTray              = m_settings.value("/SystemTray", false).toBool();
	bSystemTrayQueryClose    = m_settings.value("/SystemTrayQueryClose", true).toBool();
	bStartMinimized          = m_settings.value("/StartMinimized", false).toBool();
	bServerConfig            = m_settings.value("/ServerConfig", true).toBool();
	sServerConfigName        = m_settings.value("/ServerConfigName", ".jackdrc").toString();
	bAlsaSeqEnabled          = m_settings.value("/AlsaSeqEnabled", true).toBool();
	bDBusEnabled             = m_settings.value("/DBusEnabled", false).toBool();
	bJackDBusEnabled         = m_settings.value("/JackDBusEnabled", false).toBool();
	bAliasesEnabled          = m_settings.value("/AliasesEnabled", false).toBool();
	bAliasesEditing          = m_settings.value("/AliasesEditing", false).toBool();
	bLeftButtons             = m_settings.value("/LeftButtons", true).toBool();
	bRightButtons            = m_settings.value("/RightButtons", true).toBool();
	bTransportButtons        = m_settings.value("/TransportButtons", true).toBool();
	bTextLabels              = m_settings.value("/TextLabels", true).toBool();
	bGraphButton             = m_settings.value("/GraphButton", true).toBool();
	iBaseFontSize            = m_settings.value("/BaseFontSize", 0).toInt();
	m_settings.endGroup();

	m_settings.beginGroup("/Defaults");
	sPatchbayPath = m_settings.value("/PatchbayPath").toString();
	iMessagesStatusTabPage = m_settings.value("/MessagesStatusTabPage", 0).toInt();
	iConnectionsTabPage = m_settings.value("/ConnectionsTabPage", 0).toInt();
	bSessionSaveVersion = m_settings.value("/SessionSaveVersion", true).toBool();
	m_settings.endGroup();

	// Load recent patchbay list...
	m_settings.beginGroup("/Patchbays");
	sPrefix = "/Patchbay%1";
	i = 0;
	for (;;) {
		QString sItem = m_settings.value(sPrefix.arg(++i)).toString();
		if (sItem.isEmpty())
			break;
		if (QFileInfo(sItem).exists())
			patchbays.append(sItem);
	}
	m_settings.endGroup();

	// Load recent session directory list...
	m_settings.beginGroup("/SessionDirs");
	sPrefix = "/SessionDir%1";
	i = 0;
	for (;;) {
		QString sItem = m_settings.value(sPrefix.arg(++i)).toString();
		if (sItem.isEmpty())
			break;
		if (QFileInfo(sItem).isDir())
			sessionDirs.append(sItem);
	}
	m_settings.endGroup();
}


// Explicit save method.
void qjackctlSetup::saveSetup (void)
{
	// Save all settings and options...
	m_settings.beginGroup("/Program");
	m_settings.setValue("/Version", CONFIG_BUILD_VERSION);
	m_settings.endGroup();

	m_settings.beginGroup("/Presets");
	m_settings.setValue("/DefPreset", sDefPreset);
	// Save last preset list.
	QString sPrefix = "/Preset%1";
	int i = 0;
	QStringListIterator iter(presets);
	while (iter.hasNext())
		m_settings.setValue(sPrefix.arg(++i), iter.next());
	// Cleanup old entries, if any...
	while (!m_settings.value(sPrefix.arg(++i)).toString().isEmpty())
		m_settings.remove(sPrefix.arg(i));
	m_settings.endGroup();

	m_settings.beginGroup("/Options");
	m_settings.setValue("/Singleton",               bSingleton);
//	m_settings.setValue("/ServerName",              sServerName);
	m_settings.setValue("/StartJack",               bStartJack);
//	m_settings.setValue("/StartJackCmd",            bStartJackCmd);
	m_settings.setValue("/StopJack",                bStopJack);
	m_settings.setValue("/StartupScript",           bStartupScript);
	m_settings.setValue("/StartupScriptShell",      sStartupScriptShell);
	m_settings.setValue("/PostStartupScript",       bPostStartupScript);
	m_settings.setValue("/PostStartupScriptShell",  sPostStartupScriptShell);
	m_settings.setValue("/ShutdownScript",          bShutdownScript);
	m_settings.setValue("/ShutdownScriptShell",     sShutdownScriptShell);
	m_settings.setValue("/PostShutdownScript",      bPostShutdownScript);
	m_settings.setValue("/PostShutdownScriptShell", sPostShutdownScriptShell);
	m_settings.setValue("/StdoutCapture",           bStdoutCapture);
	m_settings.setValue("/XrunRegex",               sXrunRegex);
	m_settings.setValue("/ActivePatchbay",          bActivePatchbay);
	m_settings.setValue("/ActivePatchbayPath",      sActivePatchbayPath);
	m_settings.setValue("/ActivePatchbayReset",     bActivePatchbayReset);
	m_settings.setValue("/QueryDisconnect",         bQueryDisconnect);
	m_settings.setValue("/MessagesLog",             bMessagesLog);
	m_settings.setValue("/MessagesLogPath",         sMessagesLogPath);
	m_settings.setValue("/TimeDisplay",             iTimeDisplay);
	m_settings.setValue("/MessagesFont",            sMessagesFont);
	m_settings.setValue("/MessagesLimit",           bMessagesLimit);
	m_settings.setValue("/MessagesLimitLines",      iMessagesLimitLines);
	m_settings.setValue("/DisplayFont1",            sDisplayFont1);
	m_settings.setValue("/DisplayFont2",            sDisplayFont2);
	m_settings.setValue("/DisplayEffect",           bDisplayEffect);
	m_settings.setValue("/DisplayBlink",            bDisplayBlink);
	m_settings.setValue("/JackClientPortAlias",     iJackClientPortAlias);
	m_settings.setValue("/JackClientPortMetadata",  bJackClientPortMetadata);
	m_settings.setValue("/ConnectionsIconSize",     iConnectionsIconSize);
	m_settings.setValue("/ConnectionsFont",         sConnectionsFont);
	m_settings.setValue("/QueryClose",              bQueryClose);
	m_settings.setValue("/QueryShutdown",           bQueryShutdown);
	m_settings.setValue("/KeepOnTop",               bKeepOnTop);
	m_settings.setValue("/SystemTray",              bSystemTray);
	m_settings.setValue("/SystemTrayQueryClose",    bSystemTrayQueryClose);
	m_settings.setValue("/StartMinimized",          bStartMinimized);
	m_settings.setValue("/ServerConfig",            bServerConfig);
	m_settings.setValue("/ServerConfigName",        sServerConfigName);
	m_settings.setValue("/AlsaSeqEnabled",          bAlsaSeqEnabled);
	m_settings.setValue("/DBusEnabled",             bDBusEnabled);
	m_settings.setValue("/JackDBusEnabled",         bJackDBusEnabled);
	m_settings.setValue("/AliasesEnabled",          bAliasesEnabled);
	m_settings.setValue("/AliasesEditing",          bAliasesEditing);
	m_settings.setValue("/LeftButtons",             bLeftButtons);
	m_settings.setValue("/RightButtons",            bRightButtons);
	m_settings.setValue("/TransportButtons",        bTransportButtons);
	m_settings.setValue("/TextLabels",              bTextLabels);
	m_settings.setValue("/GraphButton",             bGraphButton);
	m_settings.setValue("/BaseFontSize",            iBaseFontSize);
	m_settings.endGroup();

	m_settings.beginGroup("/Defaults");
	m_settings.setValue("/PatchbayPath", sPatchbayPath);
	m_settings.setValue("/MessagesStatusTabPage", iMessagesStatusTabPage);
	m_settings.setValue("/ConnectionsTabPage", iConnectionsTabPage);
	m_settings.setValue("/SessionSaveVersion", bSessionSaveVersion);
	m_settings.endGroup();

	// Save patchbay list...
	m_settings.beginGroup("/Patchbays");
	sPrefix = "/Patchbay%1";
	i = 0;
	QStringListIterator iter2(patchbays);
	while (iter2.hasNext())
		m_settings.setValue(sPrefix.arg(++i), iter2.next());
	// Cleanup old entries, if any...
	while (!m_settings.value(sPrefix.arg(++i)).toString().isEmpty())
		m_settings.remove(sPrefix.arg(i));
	m_settings.endGroup();

	// Save session directory list...
	m_settings.beginGroup("/SessionDirs");
	sPrefix = "/SessionDir%1";
	i = 0;
	QStringListIterator iter3(sessionDirs);
	while (iter3.hasNext())
		m_settings.setValue(sPrefix.arg(++i), iter3.next());
	// Cleanup old entries, if any...
	while (!m_settings.value(sPrefix.arg(++i)).toString().isEmpty())
		m_settings.remove(sPrefix.arg(i));
	m_settings.endGroup();

	// Commit all changes to disk.
	m_settings.sync();
}


//---------------------------------------------------------------------------
// Aliases preset management methods.

bool qjackctlSetup::loadAliases (void)
{
	QString sPreset = sDefPreset;

	QString sSuffix;
	if (sPreset != sDefPresetName && !sPreset.isEmpty()) {
		sSuffix = '/' + sPreset;
		// Check if on list.
		if (!presets.contains(sPreset))
			return false;
	}

	// Load preset aliases...
	const QString sAliasesKey = "/Aliases" + sSuffix;
	m_settings.beginGroup(sAliasesKey);
	m_settings.beginGroup("/Jack");	// FIXME: Audio
	aliases.audioOutputs.loadSettings(m_settings, "/Outputs");
	aliases.audioInputs.loadSettings(m_settings, "/Inputs");
	m_settings.endGroup();
	m_settings.beginGroup("/Midi");
	aliases.midiOutputs.loadSettings(m_settings, "/Outputs");
	aliases.midiInputs.loadSettings(m_settings, "/Inputs");
	m_settings.endGroup();
	m_settings.beginGroup("/Alsa");
	aliases.alsaOutputs.loadSettings(m_settings, "/Outputs");
	aliases.alsaInputs.loadSettings(m_settings, "/Inputs");
	m_settings.endGroup();
	m_settings.endGroup();

	aliases.dirty = false;
	aliases.key = sPreset;

	return true;
}

bool qjackctlSetup::saveAliases (void)
{
	const QString& sPreset = aliases.key;

	QString sSuffix;
	if (sPreset != sDefPresetName && !sPreset.isEmpty()) {
		sSuffix = "/" + sPreset;
		// Append to list if not already.
		if (!presets.contains(sPreset))
			presets.prepend(sPreset);
	}

	// Save preset aliases...
	const QString sAliasesKey = "/Aliases" + sSuffix;
	m_settings.remove(sAliasesKey);
	m_settings.beginGroup(sAliasesKey);
	m_settings.beginGroup("/Jack");	// FIXME: Audio
	aliases.audioOutputs.saveSettings(m_settings, "/Outputs");
	aliases.audioInputs.saveSettings(m_settings, "/Inputs");
	m_settings.endGroup();
	m_settings.beginGroup("/Midi");
	aliases.midiOutputs.saveSettings(m_settings, "/Outputs");
	aliases.midiInputs.saveSettings(m_settings, "/Inputs");
	m_settings.endGroup();
	m_settings.beginGroup("/Alsa");
	aliases.alsaOutputs.saveSettings(m_settings, "/Outputs");
	aliases.alsaInputs.saveSettings(m_settings, "/Inputs");
	m_settings.endGroup();
	m_settings.endGroup();

	aliases.dirty = false;

	return true;
}


//---------------------------------------------------------------------------
// Preset management methods.

bool qjackctlSetup::loadPreset ( qjackctlPreset& preset, const QString& sPreset )
{
	QString sSuffix;
	if (sPreset != sDefPresetName && !sPreset.isEmpty()) {
		sSuffix = '/' + sPreset;
		// Check if on list.
		if (!presets.contains(sPreset))
			return false;
	}

	m_settings.beginGroup("/Settings" + sSuffix);
#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
	preset.sServerPrefix = m_settings.value("/Server", "jackd -S").toString();
#else
	preset.sServerPrefix = m_settings.value("/Server", "jackd").toString();
#endif
	preset.sServerName  = m_settings.value("/ServerName").toString();
	preset.bRealtime    = m_settings.value("/Realtime", true).toBool();
	preset.bSoftMode    = m_settings.value("/SoftMode", false).toBool();
	preset.bMonitor     = m_settings.value("/Monitor", false).toBool();
	preset.bShorts      = m_settings.value("/Shorts", false).toBool();
	preset.bNoMemLock   = m_settings.value("/NoMemLock", false).toBool();
	preset.bUnlockMem   = m_settings.value("/UnlockMem", false).toBool();
	preset.bHWMeter     = m_settings.value("/HWMeter", false).toBool();
	preset.bIgnoreHW    = m_settings.value("/IgnoreHW", false).toBool();
	preset.iPriority    = m_settings.value("/Priority", 0).toInt();
	preset.iFrames      = m_settings.value("/Frames", 1024).toInt();
	preset.iSampleRate  = m_settings.value("/SampleRate", 48000).toInt();
	preset.iPeriods     = m_settings.value("/Periods", 2).toInt();
	preset.iWordLength  = m_settings.value("/WordLength", 16).toInt();
	preset.iWait        = m_settings.value("/Wait", 21333).toInt();
	preset.iChan        = m_settings.value("/Chan", 0).toInt();
	preset.sDriver      = m_settings.value("/Driver", DEFAULT_DRIVER).toString();
	preset.sInterface   = m_settings.value("/Interface").toString();
	preset.iAudio       = m_settings.value("/Audio", 0).toInt();
	preset.iDither      = m_settings.value("/Dither", 0).toInt();
	preset.iTimeout     = m_settings.value("/Timeout", 500).toInt();
	preset.sInDevice    = m_settings.value("/InDevice").toString();
	preset.sOutDevice   = m_settings.value("/OutDevice").toString();
	preset.iInChannels  = m_settings.value("/InChannels", 0).toInt();
	preset.iOutChannels = m_settings.value("/OutChannels", 0).toInt();
	preset.iInLatency   = m_settings.value("/InLatency", 0).toInt();
	preset.iOutLatency  = m_settings.value("/OutLatency", 0).toInt();
	preset.iStartDelay  = m_settings.value("/StartDelay", 2).toInt();
	preset.bSync        = m_settings.value("/Sync", false).toBool();
	preset.bVerbose     = m_settings.value("/Verbose", false).toBool();
	preset.iPortMax     = m_settings.value("/PortMax", 256).toInt();
	preset.sMidiDriver  = m_settings.value("/MidiDriver").toString();
	preset.sServerSuffix = m_settings.value("/ServerSuffix").toString();
	m_settings.endGroup();

#ifdef CONFIG_JACK_MIDI
	if (!preset.sMidiDriver.isEmpty() &&
		preset.sMidiDriver != "raw" &&
		preset.sMidiDriver != "seq")
		preset.sMidiDriver.clear();
#endif

	return true;
}

bool qjackctlSetup::savePreset ( qjackctlPreset& preset, const QString& sPreset )
{
	QString sSuffix;
	if (sPreset != sDefPresetName && !sPreset.isEmpty()) {
		sSuffix = '/' + sPreset;
		// Append to list if not already.
		if (!presets.contains(sPreset))
			presets.prepend(sPreset);
	}

	m_settings.beginGroup("/Settings" + sSuffix);
	m_settings.setValue("/Server",      preset.sServerPrefix);
	m_settings.setValue("/ServerName",  preset.sServerName);
	m_settings.setValue("/Realtime",    preset.bRealtime);
	m_settings.setValue("/SoftMode",    preset.bSoftMode);
	m_settings.setValue("/Monitor",     preset.bMonitor);
	m_settings.setValue("/Shorts",      preset.bShorts);
	m_settings.setValue("/NoMemLock",   preset.bNoMemLock);
	m_settings.setValue("/UnlockMem",   preset.bUnlockMem);
	m_settings.setValue("/HWMeter",     preset.bHWMeter);
	m_settings.setValue("/IgnoreHW",    preset.bIgnoreHW);
	m_settings.setValue("/Priority",    preset.iPriority);
	m_settings.setValue("/Frames",      preset.iFrames);
	m_settings.setValue("/SampleRate",  preset.iSampleRate);
	m_settings.setValue("/Periods",     preset.iPeriods);
	m_settings.setValue("/WordLength",  preset.iWordLength);
	m_settings.setValue("/Wait",        preset.iWait);
	m_settings.setValue("/Chan",        preset.iChan);
	m_settings.setValue("/Driver",      preset.sDriver);
	m_settings.setValue("/Interface",   preset.sInterface);
	m_settings.setValue("/Audio",       preset.iAudio);
	m_settings.setValue("/Dither",      preset.iDither);
	m_settings.setValue("/Timeout",     preset.iTimeout);
	m_settings.setValue("/InDevice",    preset.sInDevice);
	m_settings.setValue("/OutDevice",   preset.sOutDevice);
	m_settings.setValue("/InChannels",  preset.iInChannels);
	m_settings.setValue("/OutChannels", preset.iOutChannels);
	m_settings.setValue("/InLatency",   preset.iInLatency);
	m_settings.setValue("/OutLatency",  preset.iOutLatency);
	m_settings.setValue("/StartDelay",  preset.iStartDelay);
	m_settings.setValue("/Sync",        preset.bSync);
	m_settings.setValue("/Verbose",     preset.bVerbose);
	m_settings.setValue("/PortMax",     preset.iPortMax);
	m_settings.setValue("/MidiDriver",  preset.sMidiDriver);
	m_settings.setValue("/ServerSuffix", preset.sServerSuffix);
	m_settings.endGroup();

	return true;
}

bool qjackctlSetup::deletePreset ( const QString& sPreset )
{
	QString sSuffix;
	if (sPreset != sDefPresetName && !sPreset.isEmpty()) {
		sSuffix = '/' + sPreset;
		int iPreset = presets.indexOf(sPreset);
		if (iPreset < 0)
			return false;
		presets.removeAt(iPreset);
		m_settings.remove("/Settings" + sSuffix);
		m_settings.remove("/Aliases" + sSuffix);
	}
	return true;
}


//-------------------------------------------------------------------------
// Command-line argument stuff.
//

// Help about command line options.
void qjackctlSetup::print_usage ( const QString& arg0 )
{
	QTextStream out(stderr);
	const QString sEot = "\n\t";
	const QString sEol = "\n\n";

	out << QObject::tr("Usage: %1"
		" [options] [command-and-args]").arg(arg0) + sEol;
	out << QJACKCTL_TITLE " - " + QObject::tr(QJACKCTL_SUBTITLE) + sEol;
	out << QObject::tr("Options:") + sEol;
	out << "  -s, --start" + sEot +
		QObject::tr("Start JACK audio server immediately") + sEol;
	out << "  -p, --preset=[label]" + sEot +
		QObject::tr("Set default settings preset name") + sEol;
	out << "  -a, --active-patchbay=[path]" + sEot +
		QObject::tr("Set active patchbay definition file") + sEol;
	out << "  -n, --server-name=[label]" + sEot +
		QObject::tr("Set default JACK audio server name") + sEol;
	out << "  -h, --help" + sEot +
		QObject::tr("Show help about command line options") + sEol;
	out << "  -v, --version" + sEot +
		QObject::tr("Show version information") + sEol;
}


// Parse command line arguments into m_settings.
bool qjackctlSetup::parse_args ( const QStringList& args )
{
	QTextStream out(stderr);
	const QString sEol = "\n\n";
	const int argc = args.count();
	int iCmdArgs = 0;

	for (int i = 1; i < argc; ++i) {

		if (iCmdArgs > 0) {
			sCmdLine += ' ';
			sCmdLine += args.at(i);
			++iCmdArgs;
			continue;
		}

		QString sArg = args.at(i);
		QString sVal;
		const int iEqual = sArg.indexOf('=');
		if (iEqual >= 0) {
			sVal = sArg.right(sArg.length() - iEqual - 1);
			sArg = sArg.left(iEqual);
		}
		else if (i < argc - 1)
			sVal = args.at(i + 1);

		if (sArg == "-s" || sArg == "--start") {
			bStartJackCmd = true;
		}
		else if (sArg == "-p" || sArg == "--preset") {
			if (sVal.isNull()) {
				out << QObject::tr("Option -p requires an argument (preset).") + sEol;
				return false;
			}
			sDefPreset = sVal;
			if (iEqual < 0)
				++i;
		}
		else if (sArg == "-a" || sArg == "--active-patchbay") {
			if (sVal.isNull()) {
				out << QObject::tr("Option -a requires an argument (path).") + sEol;
				return false;
			}
			bActivePatchbay = true;
			sActivePatchbayPath = sVal;
			if (iEqual < 0)
				++i;
		}
		else if (sArg == "-n" || sArg == "--server-name") {
			if (sVal.isNull()) {
				out << QObject::tr("Option -n requires an argument (name).") + sEol;
				return false;
			}
			sServerName = sVal;
			if (iEqual < 0)
				++i;
		}
		else if (sArg == "-h" || sArg == "--help") {
			print_usage(args.at(0));
			return false;
		}
		else if (sArg == "-v" || sArg == "--version") {
			out << QString("Qt: %1\n")
				.arg(qVersion());
			out << QString("%1: %2\n")
				.arg(QJACKCTL_TITLE)
				.arg(CONFIG_BUILD_VERSION);
		#ifdef CONFIG_JACK_VERSION
			out << QString("JACK: %1\n")
				.arg(jack_get_version_string());
		#endif
			return false;
		}	// FIXME: Avoid auto-start jackd stuffed args!
		else if (sArg != "-T" && sArg != "-ndefault") {
			// Here starts the optional command line...
			sCmdLine += sArg;
			++iCmdArgs;
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

void qjackctlSetup::loadComboBoxHistory ( QComboBox *pComboBox, int iLimit )
{
	const bool bBlockSignals = pComboBox->blockSignals(true);

	// Load combobox list from configuration settings file...
	m_settings.beginGroup("/History/" + pComboBox->objectName());

	if (m_settings.childKeys().count() > 0) {
		pComboBox->setUpdatesEnabled(false);
		pComboBox->setDuplicatesEnabled(false);
		pComboBox->clear();
		for (int i = 0; i < iLimit; ++i) {
			const QString& sText = m_settings.value(
				"/Item" + QString::number(i + 1)).toString();
			if (sText.isEmpty())
				break;
			pComboBox->addItem(sText);
		}
		pComboBox->setUpdatesEnabled(true);
	}

	m_settings.endGroup();

	pComboBox->blockSignals(bBlockSignals);
}


void qjackctlSetup::saveComboBoxHistory ( QComboBox *pComboBox, int iLimit )
{
	const bool bBlockSignals = pComboBox->blockSignals(true);

	// Add current text as latest item...
	const QString sCurrentText = pComboBox->currentText();
	int iCount = pComboBox->count();
	for (int i = 0; i < iCount; i++) {
		const QString& sText = pComboBox->itemText(i);
		if (sText == sCurrentText) {
			pComboBox->removeItem(i);
			--iCount;
			break;
		}
	}
	while (iCount >= iLimit)
		pComboBox->removeItem(--iCount);
	pComboBox->insertItem(0, sCurrentText);
	pComboBox->setCurrentIndex(0);
	++iCount;

	// Save combobox list to configuration settings file...
	m_settings.beginGroup("/History/" + pComboBox->objectName());
	for (int i = 0; i < iCount; ++i) {
		const QString& sText = pComboBox->itemText(i);
		if (sText.isEmpty())
			break;
		m_settings.setValue("/Item" + QString::number(i + 1), sText);
	}
	m_settings.endGroup();

	pComboBox->blockSignals(bBlockSignals);
}


//---------------------------------------------------------------------------
// Splitter widget sizes persistence helper methods.

void qjackctlSetup::loadSplitterSizes ( QSplitter *pSplitter,
	QList<int>& sizes )
{
	// Try to restore old splitter sizes...
	if (pSplitter) {
		m_settings.beginGroup("/Splitter/" + pSplitter->objectName());
		QStringList list = m_settings.value("/sizes").toStringList();
		if (!list.isEmpty()) {
			sizes.clear();
			QStringListIterator iter(list);
			while (iter.hasNext())
				sizes.append(iter.next().toInt());
		}
		pSplitter->setSizes(sizes);
		m_settings.endGroup();
	}
}


void qjackctlSetup::saveSplitterSizes ( QSplitter *pSplitter )
{
	// Try to save current splitter sizes...
	if (pSplitter) {
		m_settings.beginGroup("/Splitter/" + pSplitter->objectName());
		QStringList list;
		QList<int> sizes = pSplitter->sizes();
		QListIterator<int> iter(sizes);
		while (iter.hasNext())
			list.append(QString::number(iter.next()));
		if (!list.isEmpty())
			m_settings.setValue("/sizes", list);
		m_settings.endGroup();
	}
}


//---------------------------------------------------------------------------
// Widget geometry persistence helper methods.

void qjackctlSetup::loadWidgetGeometry ( QWidget *pWidget, bool bVisible )
{
	// Try to restore old form window positioning.
	if (pWidget) {
	//	if (bVisible) pWidget->show(); -- force initial exposure?
		m_settings.beginGroup("/Geometry/" + pWidget->objectName());
	#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		const QByteArray& geometry
			= m_settings.value("/geometry").toByteArray();
		if (geometry.isEmpty()) {
			QWidget *pParent = pWidget->parentWidget();
			if (pParent)
				pParent = pParent->window();
			if (pParent == nullptr)
				pParent = QApplication::desktop();
			if (pParent) {
				QRect wrect(pWidget->geometry());
				wrect.moveCenter(pParent->geometry().center());
				pWidget->move(wrect.topLeft());
			}
		} else {
			pWidget->restoreGeometry(geometry);
		}
	#else//--LOAD_OLD_GEOMETRY
		QPoint wpos;
		QSize  wsize;
		wpos.setX(m_settings.value("/x", -1).toInt());
		wpos.setY(m_settings.value("/y", -1).toInt());
		wsize.setWidth(m_settings.value("/width", -1).toInt());
		wsize.setHeight(m_settings.value("/height", -1).toInt());
		if (wpos.x() > 0 && wpos.y() > 0)
			pWidget->move(wpos);
		if (wsize.width() > 0 && wsize.height() > 0)
			pWidget->resize(wsize);
	#endif
	//	else
	//	pWidget->adjustSize();
		if (!bVisible)
			bVisible = m_settings.value("/visible", false).toBool();
		if (bVisible && !bStartMinimized)
			pWidget->show();
		else
			pWidget->hide();
		m_settings.endGroup();
	}
}


void qjackctlSetup::saveWidgetGeometry ( QWidget *pWidget, bool bVisible )
{
	// Try to save form window position...
	// (due to X11 window managers ideossincrasies, we better
	// only save the form geometry while its up and visible)
	if (pWidget) {
		m_settings.beginGroup("/Geometry/" + pWidget->objectName());
	#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		m_settings.setValue("/geometry", pWidget->saveGeometry());
	#else//--SAVE_OLD_GEOMETRY
		const QPoint& wpos  = pWidget->pos();
		const QSize&  wsize = pWidget->size();
		m_settings.setValue("/x", wpos.x());
		m_settings.setValue("/y", wpos.y());
		m_settings.setValue("/width", wsize.width());
		m_settings.setValue("/height", wsize.height());
	#endif
		if (!bVisible) bVisible = pWidget->isVisible();
		m_settings.setValue("/visible", bVisible && !bStartMinimized);
		m_settings.endGroup();
	}
}


// end of qjackctlSetup.cpp
