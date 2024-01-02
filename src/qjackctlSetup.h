// qjackctlSetup.h
//
/****************************************************************************
   Copyright (C) 2003-2024, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctlSetup_h
#define __qjackctlSetup_h

#include "qjackctlAbout.h"
#include "qjackctlAliases.h"

#include <QStringList>

// Forward declarations.
class QWidget;
class QComboBox;
class QSplitter;


// Audio mode combobox item indexes.
#define QJACKCTL_DUPLEX     0
#define QJACKCTL_CAPTURE    1
#define QJACKCTL_PLAYBACK   2

// Icon size combobox item indexes.
#define QJACKCTL_ICON_16X16 0
#define QJACKCTL_ICON_32X32 1
#define QJACKCTL_ICON_64X64 2


// Server settings preset struct.
struct qjackctlPreset
{
	qjackctlPreset() { clear(); }

	void clear();
	void load(QSettings& settings, const QString& sSuffix);
	void save(QSettings& settings, const QString& sSuffix);
	void fixup();

	QString sServerPrefix;
	QString sServerName;
	bool    bRealtime;
	bool    bSoftMode;
	bool    bMonitor;
	bool    bShorts;
	bool    bNoMemLock;
	bool    bUnlockMem;
	bool    bHWMeter;
	bool    bIgnoreHW;
	int     iPriority;
	int     iFrames;
	int     iSampleRate;
	int     iPeriods;
	int     iWordLength;
	int     iWait;
	int     iChan;
	QString sDriver;
	QString sInterface;
	int     iAudio;
	int     iDither;
	int     iTimeout;
	QString sInDevice;
	QString sOutDevice;
	int     iInChannels;
	int     iOutChannels;
	int     iInLatency;
	int     iOutLatency;
	int     iStartDelay;
	bool    bSync;
	bool    bVerbose;
	int     iPortMax;
	QString sMidiDriver;
	QString sServerSuffix;
	uchar   ucClockSource;
	uchar   ucSelfConnectMode;
};

// Common settings profile class.
class qjackctlSetup
{
public:

	// Constructor.
	qjackctlSetup();
	// Destructor;
	~qjackctlSetup();

	// The settings object accessor.
	QSettings& settings();

	// Explicit I/O methods.
	void loadSetup();
	void saveSetup();

	// Command line arguments parser.
	bool parse_args(const QStringList& args);
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
	void show_error(const QString& msg);
#else
	// Command line usage helper.
	void print_usage(const QString& arg0);
#endif

	// Default (translated) preset name.
	static const QString& defName();

	// Immediate server start options.
	bool bStartJack;
	bool bStartJackCmd;

	// Server stop options.
	bool bStopJack;

	// User supplied command line.
	QStringList cmdLine;

	// Current/previous (default) preset name.
	QString sDefPreset;
	QString sOldPreset;

	// Available presets list.
	QStringList presets;

	// Options...
	bool    bSingleton;
	QString sServerName;
	bool    bStartupScript;
	QString sStartupScriptShell;
	bool    bPostStartupScript;
	QString sPostStartupScriptShell;
	bool    bShutdownScript;
	QString sShutdownScriptShell;
	bool    bPostShutdownScript;
	QString sPostShutdownScriptShell;
	bool    bStdoutCapture;
	QString sXrunRegex;
	bool    bActivePatchbay;
	QString sActivePatchbayPath;
	bool    bActivePatchbayReset;
	bool    bQueryDisconnect;
	bool    bMessagesLog;
	QString sMessagesLogPath;
	int     iTimeDisplay;
	QString sMessagesFont;
	bool    bMessagesLimit;
	int     iMessagesLimitLines;
	QString sDisplayFont1;
	QString sDisplayFont2;
	bool    bDisplayEffect;
	bool    bDisplayBlink;
	QString sCustomColorTheme;
	QString sCustomStyleTheme;
	int     iJackClientPortAlias;
	bool    bJackClientPortMetadata;
	int     iConnectionsIconSize;
	QString sConnectionsFont;
	bool    bQueryClose;
	bool    bQueryShutdown;
	bool    bQueryRestart;
	bool    bKeepOnTop;
	bool    bSystemTray;
	bool    bSystemTrayQueryClose;
	bool    bStartMinimized;
	bool    bServerConfig;
	QString sServerConfigName;
	bool    bAlsaSeqEnabled;
	bool    bDBusEnabled;
	bool    bJackDBusEnabled;
	bool    bAliasesEnabled;
	bool    bAliasesEditing;
	bool    bLeftButtons;
	bool    bRightButtons;
	bool    bTransportButtons;
	bool    bTextLabels;
	bool    bGraphButton;
	int     iBaseFontSize;

	// Defaults...
	QString sPatchbayPath;
	// Recent patchbay listing.
	QStringList patchbays;

	// Recent session directories.
	QStringList sessionDirs;

	// Last open tab page...
	int iMessagesStatusTabPage;
	int iConnectionsTabPage;

	// Last session save type...
	bool bSessionSaveVersion;

	// Aliases containers.
	qjackctlAliases aliases;

	// Aliases preset management methods.
	bool loadAliases();
	bool saveAliases();

	// Preset management methods.
	bool loadPreset(qjackctlPreset& preset, const QString& sPreset);
	bool savePreset(qjackctlPreset& preset, const QString& sPreset);
	bool deletePreset(const QString& sPreset);

	// Combo box history persistence helper prototypes.
	void loadComboBoxHistory(QComboBox *pComboBox, int iLimit = 8);
	void saveComboBoxHistory(QComboBox *pComboBox, int iLimit = 8);

	// Splitter widget sizes persistence helper methods.
	void loadSplitterSizes(QSplitter *pSplitter, QList<int>& sizes);
	void saveSplitterSizes(QSplitter *pSplitter);

	// Widget geometry persistence helper prototypes.
	void saveWidgetGeometry(QWidget *pWidget, bool bVisible = false);
	void loadWidgetGeometry(QWidget *pWidget, bool bVisible = false);

private:

	// Our proper settings profile.
	QSettings m_settings;

	// Default (translated) preset name.
	static QString g_sDefName;
};


#endif  // __qjackctlSetup_h


// end of qjackctlSetup.h

