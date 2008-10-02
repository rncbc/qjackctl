// qjackctlSetup.h
//
/****************************************************************************
   Copyright (C) 2003-2008, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qjackctlConnectAlias.h"

#include <QWidget>


// Forward declarations.
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
	QString sServer;
	bool    bRealtime;
	bool    bSoftMode;
	bool    bMonitor;
	bool    bShorts;
	bool    bNoMemLock;
	bool    bUnlockMem;
	bool    bHWMon;
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
	bool    bVerbose;
	int     iPortMax;
	QString sMidiDriver;
};

// Common settings profile class.
class qjackctlSetup
{
public:

	// Constructor.
	qjackctlSetup();
	// destructor;
	~qjackctlSetup();

	// Command line arguments parser.
	bool parse_args(int argc, char **argv);
	// Command line usage helper.
	void print_usage(const char *arg0);

	// Default (translated) preset name.
	QString sDefPresetName;

	// Immediate server start option.
	bool bStartJack;

	// User supplied command line.
	QString sCmdLine;

	// Current (default) preset name.
	QString sDefPreset;
	// Available presets list.
	QStringList presets;

	// Options...
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
	bool    bXrunIgnoreFirst;
	bool    bActivePatchbay;
	QString sActivePatchbayPath;
#ifdef CONFIG_AUTO_REFRESH
	bool    bAutoRefresh;
	int     iTimeRefresh;
#endif
	bool    bMessagesLog;
	QString sMessagesLogPath;
	bool    bBezierLines;
	int     iTimeDisplay;
	int     iTimeFormat;
	QString sMessagesFont;
	bool    bMessagesLimit;
	int     iMessagesLimitLines;
	QString sDisplayFont1;
	QString sDisplayFont2;
	bool    bDisplayEffect;
	bool    bDisplayBlink;
	int     iJackClientPortAlias;
	int     iConnectionsIconSize;
	QString sConnectionsFont;
	bool    bQueryClose;
	bool    bKeepOnTop;
	bool    bSystemTray;
	bool    bStartMinimized;
	bool    bDelayedSetup;
	bool    bServerConfig;
	QString sServerConfigName;
	bool    bServerConfigTemp;
	bool    bQueryShutdown;
	bool    bAlsaSeqEnabled;
	bool    bAliasesEnabled;
	bool    bAliasesEditing;
	bool    bLeftButtons;
	bool    bRightButtons;
	bool    bTransportButtons;
	bool    bTextLabels;
	int     iBaseFontSize;

	// Defaults...
	QString sPatchbayPath;
	// Recent patchbay listing.
	QStringList patchbays;

	// Aliases containers.
	qjackctlConnectAlias aliasAudioOutputs;
	qjackctlConnectAlias aliasAudioInputs;
	qjackctlConnectAlias aliasMidiOutputs;
	qjackctlConnectAlias aliasMidiInputs;
	qjackctlConnectAlias aliasAlsaOutputs;
	qjackctlConnectAlias aliasAlsaInputs;

	// Aliases preset management methods.
	bool loadAliases(const QString& sPreset);
	bool saveAliases(const QString& sPreset);
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
	void saveWidgetGeometry(QWidget *pWidget, bool bMinimized = false);
	void loadWidgetGeometry(QWidget *pWidget, bool bMinimized = false);

private:

	// Our proper settings profile.
	QSettings m_settings;
};


// Delayed widget setup helper class.
class qjackctlDelayedSetup : public QObject
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlDelayedSetup(QWidget *pWidget,
		const QPoint& pos, const QSize& size, bool bVisible, int iDelay = 0);

protected slots:

	void setup();

private:

	QWidget *m_pWidget;
	QPoint   m_pos;
	QSize    m_size;
	bool     m_bVisible;
};


#endif  // __qjackctlSetup_h


// end of qjackctlSetup.h
