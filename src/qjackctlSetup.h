// qjackctlSetup.h
//
/****************************************************************************
   Copyright (C) 2003-2004, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctlSetup_h
#define __qjackctlSetup_h

#include <qstring.h>
#include <qsettings.h>
#include <qcombobox.h>

// Audio mode combobox item indexes.
#define QJACKCTL_DUPLEX     0
#define QJACKCTL_CAPTURE    1
#define QJACKCTL_PLAYBACK   2


// Server settings preset struct.
struct qjackctlPreset
{
    QString sServer;
    bool    bRealtime;
    bool    bSoftMode;
    bool    bMonitor;
    bool    bShorts;
    bool    bNoMemLock;
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
    int     iStartDelay;
    bool    bVerbose;
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
    bool    bStdoutCapture;
    QString sXrunRegex;
    bool    bXrunIgnoreFirst;
    bool    bActivePatchbay;
    QString sActivePatchbayPath;
    bool    bAutoRefresh;
    int     iTimeRefresh;
    int     iTimeDisplay;
    int     iTimeFormat;
    QString sMessagesFont;
    bool    bMessagesLimit;
    int     iMessagesLimitLines;
    QString sDisplayFont1;
    QString sDisplayFont2;
    bool    bQueryClose;
    bool    bKeepOnTop;
    bool    bServerConfig;
    QString sServerConfigName;
    bool    bServerConfigTemp;

    // Defaults...
    QString sPatchbayPath;

    // Preset management methods.
    bool loadPreset(qjackctlPreset& preset, const QString& sPreset);
    bool savePreset(qjackctlPreset& preset, const QString& sPreset);
    bool deletePreset(const QString& sPreset);

    // Combo box history persistence helper prototypes.
    void add2ComboBoxHistory(QComboBox *pComboBox, const QString& sNewText, int iLimit = 8, int iIndex = -1);
    void loadComboBoxHistory(QComboBox *pComboBox, int iLimit = 8);
    void saveComboBoxHistory(QComboBox *pComboBox, int iLimit = 8);

    // Widget geometry persistence helper prototypes.
    void saveWidgetGeometry(QWidget *pWidget);
    void loadWidgetGeometry(QWidget *pWidget);

private:

    // A recursive QSettings key entry remover.
    void deleteKey(const QString& sKey);

    // Our proper settings profile.
    QSettings m_settings;
};


#endif  // __qjackctlSetup_h


// end of qjackctlSetup.h

