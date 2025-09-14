// qjackctlSetup.cpp
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

#include "qjackctlSetup.h"

#include <QComboBox>
#include <QSplitter>
#include <QList>

#include <QTimer>
#include <QTextStream>
#include <QFileInfo>

#include <QApplication>

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
#include <QCommandLineParser>
#include <QCommandLineOption>
#if defined(Q_OS_WINDOWS)
#include <QMessageBox>
#endif
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QDesktopWidget>
#endif

#ifdef CONFIG_JACK_VERSION
#include <jack/jack.h>
#endif


// Default (translated) preset name.
QString qjackctlSetup::g_sDefName;

// Default (translated) preset name. (static)
const QString& qjackctlSetup::defName (void)
{
	if (g_sDefName.isEmpty())
		g_sDefName = QObject::tr("(default)");

	return g_sDefName;
}


// Constructor.
qjackctlSetup::qjackctlSetup (void)
	: m_settings(QJACKCTL_DOMAIN, QJACKCTL_TITLE)
{
	bStartJackCmd = false;

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
	sDefPreset = m_settings.value("/DefPreset", defName()).toString();
	sOldPreset = m_settings.value("/OldPreset").toString();
	QString sPrefix = "/Preset%1";
	int i = 0;
	for (;;) {
		QString sItem = m_settings.value(sPrefix.arg(++i)).toString();
		if (sItem.isEmpty())
			break;
		presets.append(sItem);
	}
	m_settings.endGroup();

#ifdef __APPLE__
	// alternative custom defaults, as the mac theme does not look good with our custom widgets
	sCustomColorTheme = "KXStudio";
	sCustomStyleTheme = "Fusion";
#endif

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
	sCustomColorTheme        = m_settings.value("/CustomColorTheme", sCustomColorTheme).toString();
	sCustomStyleTheme        = m_settings.value("/CustomStyleTheme", sCustomStyleTheme).toString();
	iJackClientPortAlias     = m_settings.value("/JackClientPortAlias", 0).toInt();
	bJackClientPortMetadata  = m_settings.value("/JackClientPortMetadata", false).toBool();
	iConnectionsIconSize     = m_settings.value("/ConnectionsIconSize", QJACKCTL_ICON_16X16).toInt();
	sConnectionsFont         = m_settings.value("/ConnectionsFont").toString();
	bQueryClose              = m_settings.value("/QueryClose", true).toBool();
	bQueryShutdown           = m_settings.value("/QueryShutdown", true).toBool();
	bQueryRestart            = m_settings.value("/QueryRestart", false).toBool();
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
	m_settings.setValue("/Version", PROJECT_VERSION);
	m_settings.endGroup();

	m_settings.beginGroup("/Presets");
	m_settings.setValue("/DefPreset", sDefPreset);
	m_settings.setValue("/OldPreset", sOldPreset);
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
	m_settings.setValue("/CustomColorTheme",        sCustomColorTheme);
	m_settings.setValue("/CustomStyleTheme",        sCustomStyleTheme);
	m_settings.setValue("/JackClientPortAlias",     iJackClientPortAlias);
	m_settings.setValue("/JackClientPortMetadata",  bJackClientPortMetadata);
	m_settings.setValue("/ConnectionsIconSize",     iConnectionsIconSize);
	m_settings.setValue("/ConnectionsFont",         sConnectionsFont);
	m_settings.setValue("/QueryClose",              bQueryClose);
	m_settings.setValue("/QueryShutdown",           bQueryShutdown);
	m_settings.setValue("/QueryRestart",            bQueryRestart);
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
	if (sPreset != defName() && !sPreset.isEmpty()) {
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
	if (sPreset != defName() && !sPreset.isEmpty()) {
		sSuffix = "/" + sPreset;
		// Append to list if not already.
		if (!presets.contains(sPreset))
			presets.prepend(sPreset);
	}

	// Save preset aliases...
	const QString sAliasesKey = "/Aliases" + sSuffix;
//	m_settings.remove(sAliasesKey);
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
// Preset struct methods.

void qjackctlPreset::clear (void)
{
	sServerPrefix.clear();
	sServerName  .clear();
	bRealtime    = true;
	bSoftMode    = false;
	bMonitor     = false;
	bShorts      = false;
	bNoMemLock   = false;
	bUnlockMem   = false;
	bHWMeter     = false;
	bIgnoreHW    = false;
	iPriority    = 0;
	iFrames      = 0;
	iSampleRate  = 0;
	iPeriods     = 0;
	iWordLength  = 0;
	iWait        = 0;
	iChan        = 0;
	sDriver      .clear();
	sInterface   .clear();
	iAudio       = 0;
	iDither      = 0;
	iTimeout     = 0;
	sInDevice    .clear();
	sOutDevice   .clear();
	iInChannels  = 0;
	iOutChannels = 0;
	iInLatency   = 0;
	iOutLatency  = 0;
	iStartDelay  = 2;
	bSync        = false;
	bVerbose     = false;
	iPortMax     = 0;
	sMidiDriver  .clear();
	sServerSuffix.clear();
	ucClockSource = 0;
	ucSelfConnectMode = 0;

	fixup();
}


void qjackctlPreset::load ( QSettings& settings, const QString& sSuffix )
{
	settings.beginGroup("/Settings" + sSuffix);

	sServerPrefix = settings.value("/Server",       sServerPrefix).toString();
	sServerName   = settings.value("/ServerName",   sServerName).toString();
	bRealtime     = settings.value("/Realtime",     bRealtime).toBool();
	bSoftMode     = settings.value("/SoftMode",     bSoftMode).toBool();
	bMonitor      = settings.value("/Monitor",      bMonitor).toBool();
	bShorts       = settings.value("/Shorts",       bShorts).toBool();
	bNoMemLock    = settings.value("/NoMemLock",    bNoMemLock).toBool();
	bUnlockMem    = settings.value("/UnlockMem",    bUnlockMem).toBool();
	bHWMeter      = settings.value("/HWMeter",      bHWMeter).toBool();
	bIgnoreHW     = settings.value("/IgnoreHW",     bIgnoreHW).toBool();
	iPriority     = settings.value("/Priority",     iPriority).toInt();
	iFrames       = settings.value("/Frames",       iFrames).toInt();
	iSampleRate   = settings.value("/SampleRate",   iSampleRate).toInt();
	iPeriods      = settings.value("/Periods",      iPeriods).toInt();
	iWordLength   = settings.value("/WordLength",   iWordLength).toInt();
	iWait         = settings.value("/Wait",         iWait).toInt();
	iChan         = settings.value("/Chan",         iChan).toInt();
	sDriver       = settings.value("/Driver",       sDriver).toString();
	sInterface    = settings.value("/Interface",    sInterface).toString();
	iAudio        = settings.value("/Audio",        iAudio).toInt();
	iDither       = settings.value("/Dither",       iDither).toInt();
	iTimeout      = settings.value("/Timeout",      iTimeout).toInt();
	sInDevice     = settings.value("/InDevice",     sInDevice).toString();
	sOutDevice    = settings.value("/OutDevice",    sOutDevice).toString();
	iInChannels   = settings.value("/InChannels",   iInChannels).toInt();
	iOutChannels  = settings.value("/OutChannels",  iOutChannels).toInt();
	iInLatency    = settings.value("/InLatency",    iInLatency).toInt();
	iOutLatency   = settings.value("/OutLatency",   iOutLatency).toInt();
	iStartDelay   = settings.value("/StartDelay",   iStartDelay).toInt();
	bSync         = settings.value("/Sync",         bSync).toBool();
	bVerbose      = settings.value("/Verbose",      bVerbose).toBool();
	iPortMax      = settings.value("/PortMax",      iPortMax).toInt();
	sMidiDriver   = settings.value("/MidiDriver",   sMidiDriver).toString();
	sServerSuffix = settings.value("/ServerSuffix", sServerSuffix).toString();
	ucClockSource  = settings.value("/ClockSource",  ucClockSource).value<uchar>();
	ucSelfConnectMode = settings.value("/SelfConnectMode", ucSelfConnectMode).value<uchar>();

	settings.endGroup();

	fixup();
}

void qjackctlPreset::save ( QSettings& settings, const QString& sSuffix )
{
	settings.beginGroup("/Settings" + sSuffix);

	settings.setValue("/Server",       sServerPrefix);
	settings.setValue("/ServerName",   sServerName);
	settings.setValue("/Realtime",     bRealtime);
	settings.setValue("/SoftMode",     bSoftMode);
	settings.setValue("/Monitor",      bMonitor);
	settings.setValue("/Shorts",       bShorts);
	settings.setValue("/NoMemLock",    bNoMemLock);
	settings.setValue("/UnlockMem",    bUnlockMem);
	settings.setValue("/HWMeter",      bHWMeter);
	settings.setValue("/IgnoreHW",     bIgnoreHW);
	settings.setValue("/Priority",     iPriority);
	settings.setValue("/Frames",       iFrames);
	settings.setValue("/SampleRate",   iSampleRate);
	settings.setValue("/Periods",      iPeriods);
	settings.setValue("/WordLength",   iWordLength);
	settings.setValue("/Wait",         iWait);
	settings.setValue("/Chan",         iChan);
	settings.setValue("/Driver",       sDriver);
	settings.setValue("/Interface",    sInterface);
	settings.setValue("/Audio",        iAudio);
	settings.setValue("/Dither",       iDither);
	settings.setValue("/Timeout",      iTimeout);
	settings.setValue("/InDevice",     sInDevice);
	settings.setValue("/OutDevice",    sOutDevice);
	settings.setValue("/InChannels",   iInChannels);
	settings.setValue("/OutChannels",  iOutChannels);
	settings.setValue("/InLatency",    iInLatency);
	settings.setValue("/OutLatency",   iOutLatency);
	settings.setValue("/StartDelay",   iStartDelay);
	settings.setValue("/Sync",         bSync);
	settings.setValue("/Verbose",      bVerbose);
	settings.setValue("/PortMax",      iPortMax);
	settings.setValue("/MidiDriver",   sMidiDriver);
	settings.setValue("/ServerSuffix", sServerSuffix);
	settings.setValue("/ClockSource",  ucClockSource);
	settings.setValue("/SelfConnectMode", ucSelfConnectMode);

	settings.endGroup();
}

void qjackctlPreset::fixup (void)
{
	if (sServerPrefix.isEmpty()) {
		sServerPrefix = "jackd";
	#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
		sServerPrefix += " -S -X winmme";
	#endif
	}

	if (sDriver.isEmpty()) {
	#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
		sDriver =  "portaudio";
	#elif defined(__APPLE__)
		sDriver = "coreaudio";
	#elif defined(__FreeBSD__)
		sDriver = "oss";
	#else
		sDriver = "alsa";
	#endif
	}

#ifdef CONFIG_JACK_MIDI
	if (!sMidiDriver.isEmpty()
		&& sMidiDriver != "raw"
		&& sMidiDriver != "seq")
		sMidiDriver.clear();
#endif
}


//---------------------------------------------------------------------------
// Preset management methods.

bool qjackctlSetup::loadPreset ( qjackctlPreset& preset, const QString& sPreset )
{
	QString sSuffix;
	if (sPreset != defName() && !sPreset.isEmpty()) {
		sSuffix = '/' + sPreset;
		// Check if on list.
		if (!presets.contains(sPreset))
			return false;
	}

	preset.load(m_settings, sSuffix);
	return true;
}

bool qjackctlSetup::savePreset ( qjackctlPreset& preset, const QString& sPreset )
{
	QString sSuffix;
	if (sPreset != defName() && !sPreset.isEmpty()) {
		sSuffix = '/' + sPreset;
		// Append to list if not already.
		if (!presets.contains(sPreset))
			presets.prepend(sPreset);
	}

	preset.save(m_settings, sSuffix);
	return true;
}

bool qjackctlSetup::deletePreset ( const QString& sPreset )
{
	QString sSuffix;
	if (sPreset != defName() && !sPreset.isEmpty()) {
		sSuffix = '/' + sPreset;
		const int iPreset = presets.indexOf(sPreset);
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

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)

void qjackctlSetup::show_error( const QString& msg )
{
#if defined(Q_OS_WINDOWS)
	QMessageBox::information(nullptr, QApplication::applicationName(), msg);
#else
	const QByteArray tmp = msg.toUtf8() + '\n';
	::fputs(tmp.constData(), stderr);
#endif
}

#else

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
		QObject::tr("Start JACK audio server immediately.") + sEol;
	out << "  -p, --preset=[label]" + sEot +
		QObject::tr("Set default settings preset name.") + sEol;
	out << "  -a, --active-patchbay=[path]" + sEot +
		QObject::tr("Set active patchbay definition file.") + sEol;
	out << "  -n, --server-name=[label]" + sEot +
		QObject::tr("Set default JACK audio server name.") + sEol;
	out << "  -h, --help" + sEot +
		QObject::tr("Show help about command line options.") + sEol;
	out << "  -v, --version" + sEot +
		QObject::tr("Show version information.") + sEol;
}

#endif


// Parse command line arguments into m_settings.
bool qjackctlSetup::parse_args ( const QStringList& args )
{
	int iCmdArgs = 0;

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)

	QCommandLineParser parser;
	parser.setApplicationDescription(
		QJACKCTL_TITLE " - " + QObject::tr(QJACKCTL_SUBTITLE));

	parser.addOption({{"s", "start"},
		QObject::tr("Start JACK audio server immediately.")});
	parser.addOption({{"p", "preset"},
		QObject::tr("Set default settings preset name."), "label"});
	parser.addOption({{"a", "active-patchbay"},
		QObject::tr("Set active patchbay definition file."), "path"});
	parser.addOption({{"n", "server-name"},
		QObject::tr("Set default JACK audio server name."), "name"});
	const QCommandLineOption& helpOption = parser.addHelpOption();
	const QCommandLineOption& versionOption = parser.addVersionOption();
	parser.addPositionalArgument("command-and-args",
		QObject::tr("Launch command with arguments."),
		QObject::tr("[command-and-args]"));

	if (!parser.parse(args)) {
		show_error(parser.errorText());
		return false;
	}

	if (parser.isSet(helpOption)) {
		show_error(parser.helpText());
		return false;
	}

	if (parser.isSet(versionOption)) {
		QString sVersion = QString("%1 %2\n")
			.arg(QJACKCTL_TITLE)
			.arg(QCoreApplication::applicationVersion());
		sVersion += QString("Qt: %1").arg(qVersion());
	#if defined(QT_STATIC)
		sVersion += "-static";
	#endif
	#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
		sVersion += ' ';
		sVersion += '(';
		sVersion += QApplication::platformName();
		sVersion += ')';
	#endif
		sVersion += '\n';
	#ifdef CONFIG_JACK_VERSION
		sVersion += QString("JACK: %1\n").arg(jack_get_version_string());
	#endif
		show_error(sVersion);
		return false;
	}

	if (parser.isSet("start")) {
		bStartJackCmd = true;
	}

	if (parser.isSet("preset")) {
		const QString& sVal = parser.value("preset");
		if (sVal.isEmpty()) {
			show_error(QObject::tr("Option -p requires an argument (preset)."));
			return false;
		}
		sDefPreset = sVal;
	}

	if (parser.isSet("active-patchbay")) {
		const QString& sVal = parser.value("active-patchbay");
		if (sVal.isEmpty()) {
			show_error(QObject::tr("Option -a requires an argument (path)."));
			return false;
		}
		bActivePatchbay = true;
		sActivePatchbayPath = sVal;
	}

	if (parser.isSet("server-name")) {
		const QString& sVal = parser.value("server-name");
		if (sVal.isEmpty()) {
			show_error(QObject::tr("Option -n requires an argument (name)."));
			return false;
		}
		sServerName = sVal;
	}

	foreach (const QString& sArg, parser.positionalArguments()) {
		if (sArg != "-T" && sArg != "-ndefault") {
			cmdLine.append(sArg);
			++iCmdArgs;
		}
	}

#else

	QTextStream out(stderr);
	const QString sEol = "\n\n";
	const int argc = args.count();

	for (int i = 1; i < argc; ++i) {

		QString sArg = args.at(i);

		if (iCmdArgs > 0) {
			sCmdArgs.append(sArg);
			++iCmdArgs;
			continue;
		}

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
			out << QString("Qt: %1").arg(qVersion());
		#if defined(QT_STATIC)
			out << "-static";
		#endif
			out << '\n';
		#ifdef CONFIG_JACK_VERSION
			out << QString("JACK: %1\n")
				.arg(jack_get_version_string());
		#endif
			out << QString("%1: %2\n")
				.arg(QJACKCTL_TITLE)
				.arg(PROJECT_VERSION);
			return false;
		}	// FIXME: Avoid auto-start jackd stuffed args!
		else if (sArg != "-T" && sArg != "-ndefault") {
			// Here starts the optional command line...
			cmdLine.append(sArg);
			++iCmdArgs;
		}
	}

#endif

	// HACK: If there's a command line, it must be spawned on background...
	if (iCmdArgs > 0)
		cmdLine.append("&");

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

	int iCount = pComboBox->count();

	// Add current text as latest item (if not blank)...
	const QString& sCurrentText = pComboBox->currentText();
	if (!sCurrentText.isEmpty()) {
		for (int i = 0; i < iCount; ++i) {
			const QString& sText = pComboBox->itemText(i);
			if (sText == sCurrentText) {
				pComboBox->removeItem(i);
				--iCount;
				break;
			}
		}
		pComboBox->insertItem(0, sCurrentText);
		pComboBox->setCurrentIndex(0);
		++iCount;
	}

	while (iCount >= iLimit)
		pComboBox->removeItem(--iCount);

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
		#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
			if (pParent == nullptr)
				pParent = QApplication::desktop();
		#endif
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
