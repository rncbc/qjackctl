// qjackctlSetup.h
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

#ifndef __qjackctlSetup_h
#define __qjackctlSetup_h

#include <qstring.h>
#include <qsettings.h>
#include <qcombobox.h>

// Server settings preset struct.
struct qjackctlPreset
{
    QString sServer;
    bool    bRealtime;
    bool    bSoftMode;
    bool    bMonitor;
    bool    bShorts;
    int     iChan;
    int     iPriority;
    int     iFrames;
    int     iSampleRate;
    int     iPeriods;
    int     iWait;
    QString sDriver;
    QString sInterface;
    int     iAudio;
    int     iDither;
    int     iTimeout;
    bool    bHWMon;
    bool    bHWMeter;
    int     iInChannels;
    int     iOutChannels;
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

    // Immediate server start option.
    bool bStartJack;

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
    QString sXrunRegex;
    bool    bXrunIgnoreFirst;
    bool    bActivePatchbay;
    QString sActivePatchbayPath;
    bool    bAutoRefresh;
    int     iTimeRefresh;
    int     iTimeDisplay;
    QString sMessagesFont;
    bool    bQueryClose;

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

    // Our proper settings profile.
    QSettings m_settings;
};


//---------------------------------------------------------------------------
// Combo box history persistence helper prototypes.

void qjackctl_add2ComboBoxHistory(QComboBox *pComboBox, const QString& sNewText, int iLimit = 8, int iIndex = -1);
void qjackctl_loadComboBoxHistory(QSettings *pSettings, QComboBox *pComboBox, int iLimit = 8);
void qjackctl_saveComboBoxHistory(QSettings *pSettings, QComboBox *pComboBox, int iLimit = 8);

//---------------------------------------------------------------------------
// Widget geometry persistence helper prototypes.

void qjackctl_saveWidgetGeometry(QSettings *pSettings, QWidget *pWidget);
void qjackctl_loadWidgetGeometry(QSettings *pSettings, QWidget *pWidget);


#endif  // __qjackctlSetup_h


// end of qjackctlSetup.h

