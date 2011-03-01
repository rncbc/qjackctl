// qjackctlConnectionsForm.h
//
/****************************************************************************
   Copyright (C) 2003-2011, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctlConnectionsForm_h
#define __qjackctlConnectionsForm_h

#include "ui_qjackctlConnectionsForm.h"

#include "qjackctlJackConnect.h"
#include "qjackctlAlsaConnect.h"

// Forward declarations.
class qjackctlSetup;


//----------------------------------------------------------------------------
// qjackctlConnectionsForm -- UI wrapper form.

class qjackctlConnectionsForm : public QWidget
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlConnectionsForm(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);
	// Destructor.
	~qjackctlConnectionsForm();

	void setup(qjackctlSetup *pSetup);

	qjackctlConnectView *audioConnectView() const;
	qjackctlConnectView *midiConnectView() const;
	qjackctlConnectView *alsaConnectView() const;

	bool queryClose();

	enum TabPage { AudioTab = 0, MidiTab = 1, AlsaTab = 2 };

	void setTabPage(int iTabPage);
	int tabPage() const;

	QFont connectionsFont() const;
	void setConnectionsFont(const QFont& font);

	void setConnectionsIconSize(int iIconSize);

	bool isAudioConnected() const;
	bool isMidiConnected() const;
	bool isAlsaConnected() const;

	void refreshAudio(bool bEnabled, bool bClear = false);
	void refreshMidi(bool bEnabled, bool bClear = false);
	void refreshAlsa(bool bEnabled, bool bClear = false);

	void stabilizeAudio(bool bEnabled, bool bClear = false);
	void stabilizeMidi(bool bEnabled, bool bClear = false);
	void stabilizeAlsa(bool bEnabled, bool bClear = false);

	void setupAliases(qjackctlSetup *pSetup);
	void updateAliases();
	bool loadAliases();
	bool saveAliases();

public slots:

	void audioConnectSelected();
	void audioDisconnectSelected();
	void audioDisconnectAll();
	void audioExpandAll();
	void audioRefreshClear();
	void audioRefresh();
	void audioStabilize();

	void midiConnectSelected();
	void midiDisconnectSelected();
	void midiDisconnectAll();
	void midiExpandAll();
	void midiRefreshClear();
	void midiRefresh();
	void midiStabilize();

	void alsaConnectSelected();
	void alsaDisconnectSelected();
	void alsaDisconnectAll();
	void alsaExpandAll();
	void alsaRefreshClear();
	void alsaRefresh();
	void alsaStabilize();

protected slots:

	void audioDisconnecting(qjackctlPortItem *, qjackctlPortItem *);
	void midiDisconnecting(qjackctlPortItem *, qjackctlPortItem *);
	void alsaDisconnecting(qjackctlPortItem *, qjackctlPortItem *);

protected:

	void showEvent(QShowEvent *);
	void hideEvent(QHideEvent *);
	void closeEvent(QCloseEvent *);

	void keyPressEvent(QKeyEvent *);

private:

	// The Qt-designer UI struct...
	Ui::qjackctlConnectionsForm m_ui;

	// Instance variables.
	qjackctlJackConnect *m_pAudioConnect;
	qjackctlJackConnect *m_pMidiConnect;
	qjackctlAlsaConnect *m_pAlsaConnect;
	qjackctlSetup       *m_pSetup;
	QString              m_sPreset;
};


#endif	// __qjackctlConnectionsForm_h


// end of qjackctlConnectionsForm.h
