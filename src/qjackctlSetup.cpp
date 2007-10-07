// qjackctlSetup.cpp
//
/****************************************************************************
   Copyright (C) 2003-2007, rncbc aka Rui Nuno Capela. All rights reserved.

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


// Constructor.
qjackctlSetup::qjackctlSetup (void)
	: m_settings(QJACKCTL_DOMAIN, QJACKCTL_TITLE)
{
	bStartJack = false;
	sDefPresetName = QObject::tr("(default)");

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
	bStartJack               = m_settings.value("/StartJack", false).toBool();
	bStartupScript           = m_settings.value("/StartupScript", true).toBool();
	sStartupScriptShell      = m_settings.value("/StartupScriptShell", "artsshell -q terminate").toString();
	bPostStartupScript       = m_settings.value("/PostStartupScript", false).toBool();
	sPostStartupScriptShell  = m_settings.value("/PostStartupScriptShell").toString();
	bShutdownScript          = m_settings.value("/ShutdownScript", false).toBool();
	sShutdownScriptShell     = m_settings.value("/ShutdownScriptShell").toString();
	bPostShutdownScript      = m_settings.value("/PostShutdownScript", true).toBool();
	sPostShutdownScriptShell = m_settings.value("/PostShutdownScriptShell", "killall jackd").toString();
	bStdoutCapture           = m_settings.value("/StdoutCapture", true).toBool();
	sXrunRegex               = m_settings.value("/XrunRegex", "xrun of at least ([0-9|\\.]+) msecs").toString();
	bXrunIgnoreFirst         = m_settings.value("/XrunIgnoreFirst", false).toBool();
	bActivePatchbay          = m_settings.value("/ActivePatchbay", false).toBool();
	sActivePatchbayPath      = m_settings.value("/ActivePatchbayPath").toString();
#ifdef CONFIG_AUTO_REFRESH
	bAutoRefresh             = m_settings.value("/AutoRefresh", false).toBool();
	iTimeRefresh             = m_settings.value("/TimeRefresh", 10).toInt();
#endif
	bBezierLines             = m_settings.value("/BezierLines", false).toBool();
	iTimeDisplay             = m_settings.value("/TimeDisplay", 0).toInt();
	iTimeFormat              = m_settings.value("/TimeFormat", 0).toInt();
	sMessagesFont            = m_settings.value("/MessagesFont").toString();
	bMessagesLimit           = m_settings.value("/MessagesLimit", true).toBool();
	iMessagesLimitLines      = m_settings.value("/MessagesLimitLines", 1000).toInt();
	sDisplayFont1            = m_settings.value("/DisplayFont1").toString();
	sDisplayFont2            = m_settings.value("/DisplayFont2").toString();
	bDisplayEffect           = m_settings.value("/DisplayEffect", true).toBool();
	iConnectionsIconSize     = m_settings.value("/ConnectionsIconSize", QJACKCTL_ICON_16X16).toInt();
	sConnectionsFont         = m_settings.value("/ConnectionsFont").toString();
	bQueryClose              = m_settings.value("/QueryClose", true).toBool();
	bKeepOnTop               = m_settings.value("/KeepOnTop", false).toBool();
	bSystemTray              = m_settings.value("/SystemTray", false).toBool();
	bDelayedSetup            = m_settings.value("/DelayedSetup", false).toBool();
	bServerConfig            = m_settings.value("/ServerConfig", true).toBool();
	sServerConfigName        = m_settings.value("/ServerConfigName", ".jackdrc").toString();
	bServerConfigTemp        = m_settings.value("/ServerConfigTemp", false).toBool();
	bQueryShutdown           = m_settings.value("/QueryShutdown", true).toBool();
	bAlsaSeqEnabled          = m_settings.value("/AlsaSeqEnabled", true).toBool();
	bAliasesEnabled          = m_settings.value("/AliasesEnabled", false).toBool();
	bAliasesEditing          = m_settings.value("/AliasesEditing", false).toBool();
	bLeftButtons             = m_settings.value("/LeftButtons", true).toBool();
	bRightButtons            = m_settings.value("/RightButtons", true).toBool();
	bTransportButtons        = m_settings.value("/TransportButtons", true).toBool();
	bTextLabels              = m_settings.value("/TextLabels", true).toBool();
	m_settings.endGroup();

	m_settings.beginGroup("/Defaults");
	sPatchbayPath = m_settings.value("/PatchbayPath").toString();
	m_settings.endGroup();

	// Load recent patchbay list...
	m_settings.beginGroup("/Patchbays");
	sPrefix = "/Patchbay%1";
	i = 0;
	for (;;) {
		QString sItem = m_settings.value(sPrefix.arg(++i)).toString();
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
	m_settings.setValue("/Version", QJACKCTL_VERSION);
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
	m_settings.setValue("/StartJack",               bStartJack);
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
	m_settings.setValue("/XrunIgnoreFirst",         bXrunIgnoreFirst);
	m_settings.setValue("/ActivePatchbay",          bActivePatchbay);
	m_settings.setValue("/ActivePatchbayPath",      sActivePatchbayPath);
#ifdef CONFIG_AUTO_REFRESH
	m_settings.setValue("/AutoRefresh",             bAutoRefresh);
	m_settings.setValue("/TimeRefresh",             iTimeRefresh);
#endif
	m_settings.setValue("/BezierLines",             bBezierLines);
	m_settings.setValue("/TimeDisplay",             iTimeDisplay);
	m_settings.setValue("/TimeFormat",              iTimeFormat);
	m_settings.setValue("/MessagesFont",            sMessagesFont);
	m_settings.setValue("/MessagesLimit",           bMessagesLimit);
	m_settings.setValue("/MessagesLimitLines",      iMessagesLimitLines);
	m_settings.setValue("/DisplayFont1",            sDisplayFont1);
	m_settings.setValue("/DisplayFont2",            sDisplayFont2);
	m_settings.setValue("/DisplayEffect",           bDisplayEffect);
	m_settings.setValue("/ConnectionsIconSize",     iConnectionsIconSize);
	m_settings.setValue("/ConnectionsFont",         sConnectionsFont);
	m_settings.setValue("/QueryClose",              bQueryClose);
	m_settings.setValue("/KeepOnTop",               bKeepOnTop);
	m_settings.setValue("/SystemTray",              bSystemTray);
	m_settings.setValue("/DelayedSetup",            bDelayedSetup);
	m_settings.setValue("/ServerConfig",            bServerConfig);
	m_settings.setValue("/ServerConfigName",        sServerConfigName);
	m_settings.setValue("/ServerConfigTemp",        bServerConfigTemp);
	m_settings.setValue("/QueryShutdown",           bQueryShutdown);
	m_settings.setValue("/AlsaSeqEnabled",          bAlsaSeqEnabled);
	m_settings.setValue("/AliasesEnabled",          bAliasesEnabled);
	m_settings.setValue("/AliasesEditing",          bAliasesEditing);
	m_settings.setValue("/LeftButtons",             bLeftButtons);
	m_settings.setValue("/RightButtons",            bRightButtons);
	m_settings.setValue("/TransportButtons",        bTransportButtons);
	m_settings.setValue("/TextLabels",              bTextLabels);
	m_settings.endGroup();

	m_settings.beginGroup("/Defaults");
	m_settings.setValue("/PatchbayPath", sPatchbayPath);
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
}


//---------------------------------------------------------------------------
// Aliases preset management methods.

bool qjackctlSetup::loadAliases ( const QString& sPreset )
{
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
	aliasAudioOutputs.loadSettings(m_settings, "/Outputs");
	aliasAudioInputs.loadSettings(m_settings, "/Inputs");
	m_settings.endGroup();
	m_settings.beginGroup("/Midi");
	aliasMidiOutputs.loadSettings(m_settings, "/Outputs");
	aliasMidiInputs.loadSettings(m_settings, "/Inputs");
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
		if (!presets.contains(sPreset))
			presets.prepend(sPreset);
	}

	// Save preset aliases...
	const QString sAliasesKey = "/Aliases" + sSuffix;
	m_settings.remove(sAliasesKey);
	m_settings.beginGroup(sAliasesKey);
	m_settings.beginGroup("/Jack");	// FIXME: Audio
	aliasAudioOutputs.saveSettings(m_settings, "/Outputs");
	aliasAudioInputs.saveSettings(m_settings, "/Inputs");
	m_settings.endGroup();
	m_settings.beginGroup("/Midi");
	aliasMidiOutputs.saveSettings(m_settings, "/Outputs");
	aliasMidiInputs.saveSettings(m_settings, "/Inputs");
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
		sSuffix = '/' + sPreset;
		// Check if on list.
		if (!presets.contains(sPreset))
			return false;
	}

	m_settings.beginGroup("/Settings" + sSuffix);
	preset.sServer      = m_settings.value("/Server", "jackd").toString();
	preset.bRealtime    = m_settings.value("/Realtime", true).toBool();
	preset.bSoftMode    = m_settings.value("/SoftMode", false).toBool();
	preset.bMonitor     = m_settings.value("/Monitor", false).toBool();
	preset.bShorts      = m_settings.value("/Shorts", false).toBool();
	preset.bNoMemLock   = m_settings.value("/NoMemLock", false).toBool();
	preset.bUnlockMem   = m_settings.value("/UnlockMem", false).toBool();
	preset.bHWMon       = m_settings.value("/HWMon", false).toBool();
	preset.bHWMeter     = m_settings.value("/HWMeter", false).toBool();
	preset.bIgnoreHW    = m_settings.value("/IgnoreHW", false).toBool();
	preset.iPriority    = m_settings.value("/Priority", 0).toInt();
	preset.iFrames      = m_settings.value("/Frames", 1024).toInt();
	preset.iSampleRate  = m_settings.value("/SampleRate", 48000).toInt();
	preset.iPeriods     = m_settings.value("/Periods", 2).toInt();
	preset.iWordLength  = m_settings.value("/WordLength", 16).toInt();
	preset.iWait        = m_settings.value("/Wait", 21333).toInt();
	preset.iChan        = m_settings.value("/Chan", 0).toInt();
	preset.sDriver      = m_settings.value("/Driver", "alsa").toString();
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
	preset.bVerbose     = m_settings.value("/Verbose", false).toBool();
	preset.iPortMax     = m_settings.value("/PortMax", 256).toInt();
	preset.sMidiDriver  = m_settings.value("/MidiDriver").toString();
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
	m_settings.setValue("/Server",      preset.sServer);
	m_settings.setValue("/Realtime",    preset.bRealtime);
	m_settings.setValue("/SoftMode",    preset.bSoftMode);
	m_settings.setValue("/Monitor",     preset.bMonitor);
	m_settings.setValue("/Shorts",      preset.bShorts);
	m_settings.setValue("/NoMemLock",   preset.bNoMemLock);
	m_settings.setValue("/UnlockMem",   preset.bUnlockMem);
	m_settings.setValue("/HWMon",       preset.bHWMon);
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
	m_settings.setValue("/Verbose",     preset.bVerbose);
	m_settings.setValue("/PortMax",     preset.iPortMax);
	m_settings.setValue("/MidiDriver",  preset.sMidiDriver);
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
void qjackctlSetup::print_usage ( const char *arg0 )
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
		QObject::tr("Set default setings preset name") + sEol;
	out << "  -h, --help" + sEot +
		QObject::tr("Show help about command line options") + sEol;
	out << "  -v, --version" + sEot +
		QObject::tr("Show version information") + sEol;
}


// Parse command line arguments into m_settings.
bool qjackctlSetup::parse_args ( int argc, char **argv )
{
	QTextStream out(stderr);
	const QString sEol = "\n\n";
	int iCmdArgs = 0;

	for (int i = 1; i < argc; i++) {

		if (iCmdArgs > 0) {
			sCmdLine += ' ';
			sCmdLine += argv[i];
			iCmdArgs++;
			continue;
		}

		QString sArg = argv[i];
		QString sVal = QString::null;
		int iEqual = sArg.indexOf('=');
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
				out << QObject::tr("Option -p requires an argument (preset).") + sEol;
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
			out << QObject::tr("Qt: %1\n").arg(qVersion());
			out << QObject::tr(QJACKCTL_TITLE ": %1\n").arg(QJACKCTL_VERSION);
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

void qjackctlSetup::loadComboBoxHistory ( QComboBox *pComboBox, int iLimit )
{
	// Load combobox list from configuration settings file...
	m_settings.beginGroup("/History/" + pComboBox->objectName());

	if (m_settings.childKeys().count() > 0) {
		pComboBox->setUpdatesEnabled(false);
		pComboBox->setDuplicatesEnabled(false);
		pComboBox->clear();
		for (int i = 0; i < iLimit; i++) {
			const QString& sText = m_settings.value(
				"/Item" + QString::number(i + 1)).toString();
			if (sText.isEmpty())
				break;
			pComboBox->addItem(sText);
		}
		pComboBox->setUpdatesEnabled(true);
	}

	m_settings.endGroup();
}


void qjackctlSetup::saveComboBoxHistory ( QComboBox *pComboBox, int iLimit )
{
	// Add current text as latest item...
	const QString& sCurrentText = pComboBox->currentText();
	int iCount = pComboBox->count();
	for (int i = 0; i < iCount; i++) {
		const QString& sText = pComboBox->itemText(i);
		if (sText == sCurrentText) {
			pComboBox->removeItem(i);
			iCount--;
			break;
		}
	}
	while (iCount >= iLimit)
		pComboBox->removeItem(--iCount);
	pComboBox->insertItem(0, sCurrentText);
	iCount++;

	// Save combobox list to configuration settings file...
	m_settings.beginGroup("/History/" + pComboBox->objectName());
	for (int i = 0; i < iCount; i++) {
		const QString& sText = pComboBox->itemText(i);
		if (sText.isEmpty())
			break;
		m_settings.setValue("/Item" + QString::number(i + 1), sText);
	}
	m_settings.endGroup();
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

void qjackctlSetup::loadWidgetGeometry ( QWidget *pWidget )
{
	// Try to restore old form window positioning.
	if (pWidget) {
		QPoint fpos;
		QSize  fsize;
		bool bVisible;
		m_settings.beginGroup("/Geometry/" + pWidget->objectName());
		fpos.setX(m_settings.value("/x", -1).toInt());
		fpos.setY(m_settings.value("/y", -1).toInt());
		fsize.setWidth(m_settings.value("/width", -1).toInt());
		fsize.setHeight(m_settings.value("/height", -1).toInt());
		bVisible = m_settings.value("/visible", false).toBool();
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
		m_settings.beginGroup("/Geometry/" + pWidget->objectName());
		bool bVisible = pWidget->isVisible();
		if (bVisible) {
			QPoint fpos  = pWidget->pos();
			QSize  fsize = pWidget->size();
			m_settings.setValue("/x", fpos.x());
			m_settings.setValue("/y", fpos.y());
			m_settings.setValue("/width", fsize.width());
			m_settings.setValue("/height", fsize.height());
		}
		m_settings.setValue("/visible", bVisible);
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
