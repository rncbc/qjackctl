// qjackctlConnectionsForm.h
//
/****************************************************************************
   Copyright (C) 2003-2009, rncbc aka Rui Nuno Capela. All rights reserved.

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

	QFont connectionsFont() const;
	void setConnectionsFont(const QFont& font);

	void setConnectionsIconSize(int iIconSize);

	void setJackClient(jack_client_t * pJackClient);
	void setAlsaSeq(snd_seq_t * pAlsaSeq);

	bool isAudioConnected() const;
	bool isMidiConnected() const;
	bool isAlsaConnected() const;

	void refreshAudio(bool bEnabled);
	void refreshMidi(bool bEnabled);
	void refreshAlsa(bool bEnabled);

	void stabilizeAudio(bool bEnabled);
	void stabilizeMidi(bool bEnabled);
	void stabilizeAlsa(bool bEnabled);

	void setupAliases(qjackctlSetup *pSetup);
	void updateAliases();
	bool loadAliases();
	bool saveAliases();

public slots:

	void audioConnectSelected();
	void audioDisconnectSelected();
	void audioDisconnectAll();
	void audioRefresh();
	void audioStabilize();

	void midiConnectSelected();
	void midiDisconnectSelected();
	void midiDisconnectAll();
	void midiRefresh();
	void midiStabilize();

	void alsaConnectSelected();
	void alsaDisconnectSelected();
	void alsaDisconnectAll();
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
	jack_client_t       *m_pJackClient;
	snd_seq_t           *m_pAlsaSeq;
	qjackctlJackConnect *m_pAudioConnect;
	qjackctlJackConnect *m_pMidiConnect;
	qjackctlAlsaConnect *m_pAlsaConnect;
	qjackctlSetup       *m_pSetup;
	QString              m_sPreset;
};


#endif	// __qjackctlConnectionsForm_h


// end of qjackctlConnectionsForm.h
