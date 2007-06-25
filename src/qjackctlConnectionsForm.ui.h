// qjackctlConnectionsForm.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
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

#include <qmessagebox.h>


// Kind of constructor.
void qjackctlConnectionsForm::init (void)
{
	m_pJackClient   = NULL;
	m_pAudioConnect = NULL;
	m_pMidiConnect  = NULL;

	m_pAlsaSeq      = NULL;
	m_pAlsaConnect  = NULL;

	m_pSetup = NULL;

	// Connect it to some UI feedback slots.
	QObject::connect(AudioConnectView->OListView(),
		SIGNAL(selectionChanged()),
		SLOT(audioStabilize()));
	QObject::connect(AudioConnectView->IListView(),
		SIGNAL(selectionChanged()),
		SLOT(audioStabilize()));
	QObject::connect(MidiConnectView->OListView(),
		SIGNAL(selectionChanged()),
		SLOT(midiStabilize()));
	QObject::connect(MidiConnectView->IListView(),
		SIGNAL(selectionChanged()),
		SLOT(midiStabilize()));
	QObject::connect(AlsaConnectView->OListView(),
		SIGNAL(selectionChanged()),
		SLOT(alsaStabilize()));
	QObject::connect(AlsaConnectView->IListView(),
		SIGNAL(selectionChanged()),
		SLOT(alsaStabilize()));
    // Dirty dispatcher (refresh deferral).
    QObject::connect(AudioConnectView,
		SIGNAL(contentsChanged()),
		SLOT(audioRefresh()));
    QObject::connect(MidiConnectView,
		SIGNAL(contentsChanged()),
		SLOT(midiRefresh()));
    QObject::connect(AlsaConnectView,
		SIGNAL(contentsChanged()),
		SLOT(alsaRefresh()));

#ifndef CONFIG_JACK_MIDI
	ConnectionsTabWidget->setTabEnabled(MidiConnectTab, false);
#endif
#ifndef CONFIG_ALSA_SEQ
	ConnectionsTabWidget->setTabEnabled(AlsaConnectTab, false);
#endif
}


// Kind of destructor.
void qjackctlConnectionsForm::destroy (void)
{
	// Destroy our connections view...
	setJackClient(NULL);
	setAlsaSeq(NULL);
}


// Notify our parent that we're emerging.
void qjackctlConnectionsForm::showEvent ( QShowEvent *pShowEvent )
{
	qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
	if (pMainForm)
		pMainForm->stabilizeForm();

	audioRefresh();
	midiRefresh();

	alsaRefresh();

	QWidget::showEvent(pShowEvent);
}

// Notify our parent that we're closing.
void qjackctlConnectionsForm::hideEvent ( QHideEvent *pHideEvent )
{
	QWidget::hideEvent(pHideEvent);

	qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
	if (pMainForm)
		pMainForm->stabilizeForm();
}


// Set reference to global options, mostly needed for the
// initial sizes of the main splitter views and those
// client/port aliasing feature.
void qjackctlConnectionsForm::setup ( qjackctlSetup *pSetup )
{
	m_pSetup = pSetup;

	// Load some splitter sizes...
	if (m_pSetup) {
		m_pSetup->loadSplitterSizes(AudioConnectView);
		m_pSetup->loadSplitterSizes(MidiConnectView);
		m_pSetup->loadSplitterSizes(AlsaConnectView);
	}

	// Update initial client/port aliases...
	updateAliases();
}


// Window close event handlers.
bool qjackctlConnectionsForm::queryClose (void)
{
	bool bQueryClose = true;

	if (m_pSetup
		&& (AudioConnectView->isDirty() ||
			MidiConnectView->isDirty()  ||
			AlsaConnectView->isDirty())) {
		switch (QMessageBox::warning(this,
			tr("Warning") + " - " QJACKCTL_SUBTITLE1,
			tr("The preset aliases have been changed:") + "\n\n" +
			"\"" + m_sPreset +  "\"\n\n" +
			tr("Do you want to save the changes?"),
			tr("Save"), tr("Discard"), tr("Cancel"))) {
		case 0:     // Save...
			saveAliases();
			// Fall thru....
		case 1:     // Discard
			break;
		default:    // Cancel.
			bQueryClose = false;
		}
	}

	// Save some splitter sizes...
	if (m_pSetup && bQueryClose) {
		m_pSetup->saveSplitterSizes(AudioConnectView);
		m_pSetup->saveSplitterSizes(MidiConnectView);
		m_pSetup->saveSplitterSizes(AlsaConnectView);
	}

	return bQueryClose;
}


// Load aliases from current preset.
bool qjackctlConnectionsForm::loadAliases (void)
{
	bool bResult = false;
	
	if (m_pSetup && queryClose()) {
		m_sPreset = m_pSetup->sDefPreset;
		bResult = m_pSetup->loadAliases(m_sPreset);
		if (bResult) {
			AudioConnectView->setDirty(false);
			MidiConnectView->setDirty(false);
			AlsaConnectView->setDirty(false);
		}
	}

	return bResult;
}


// Save aliases to current preset.
bool qjackctlConnectionsForm::saveAliases (void)
{
	bool bResult = false;

	if (m_pSetup) {
		bResult = m_pSetup->saveAliases(m_sPreset);
		if (bResult) {
			AudioConnectView->setDirty(false);
			MidiConnectView->setDirty(false);
			AlsaConnectView->setDirty(false);
		}
	}

	return bResult;
}


// Connections view font accessors.
QFont qjackctlConnectionsForm::connectionsFont (void)
{
	// Elect one list view to retrieve current font.
	return AudioConnectView->OListView()->font();
}

void qjackctlConnectionsForm::setConnectionsFont ( const QFont & font )
{
	// Set fonts of all listviews...
	AudioConnectView->OListView()->setFont(font);
	AudioConnectView->IListView()->setFont(font);
	MidiConnectView->OListView()->setFont(font);
	MidiConnectView->IListView()->setFont(font);
	AlsaConnectView->OListView()->setFont(font);
	AlsaConnectView->IListView()->setFont(font);
}


// Connections view icon size accessor.
void qjackctlConnectionsForm::setConnectionsIconSize( int iIconSize )
{
	// Set icon sizes of all views...
	AudioConnectView->setIconSize(iIconSize);
	MidiConnectView->setIconSize(iIconSize);
	AlsaConnectView->setIconSize(iIconSize);
}


// (Un)Bind a JACK client to this form.
void qjackctlConnectionsForm::setJackClient ( jack_client_t *pJackClient )
{
	m_pJackClient = pJackClient;

	if (pJackClient == NULL) {
		if (m_pAudioConnect) {
			delete m_pAudioConnect;
			m_pAudioConnect = NULL;
		}
		if (m_pMidiConnect) {
			delete m_pMidiConnect;
			m_pMidiConnect = NULL;
		}
	}

	if (pJackClient) {
		qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
		if (m_pAudioConnect == NULL && pMainForm) {
			m_pAudioConnect = new qjackctlJackConnect(
				AudioConnectView, pJackClient, QJACKCTL_JACK_AUDIO);
			QObject::connect(m_pAudioConnect, SIGNAL(connectChanged()),
				pMainForm, SLOT(jackConnectChanged()));
		}
		if (m_pMidiConnect == NULL && pMainForm) {
			m_pMidiConnect = new qjackctlJackConnect(
				MidiConnectView, pJackClient, QJACKCTL_JACK_MIDI);
			QObject::connect(m_pMidiConnect, SIGNAL(connectChanged()),
				pMainForm, SLOT(jackConnectChanged()));
		}
	}

	stabilizeAudio(pJackClient != NULL);
	stabilizeMidi(pJackClient != NULL);
}


// (Un)Bind a ALSA sequencer descriptor to this form.
void qjackctlConnectionsForm::setAlsaSeq ( snd_seq_t *pAlsaSeq )
{
	m_pAlsaSeq = pAlsaSeq;

	if (pAlsaSeq == NULL && m_pAlsaConnect) {
		delete m_pAlsaConnect;
		m_pAlsaConnect = NULL;
	}

	if (pAlsaSeq && m_pAlsaConnect == NULL) {
		m_pAlsaConnect = new qjackctlAlsaConnect(AlsaConnectView, pAlsaSeq);
		if (m_pAlsaConnect) {
			qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
			if (pMainForm)
				QObject::connect(m_pAlsaConnect, SIGNAL(connectChanged()), pMainForm, SLOT(alsaConnectChanged()));
		}
	}

	stabilizeAlsa(pAlsaSeq != NULL);
}


// Check if there's JACK audio connections.
bool qjackctlConnectionsForm::isAudioConnected (void)
{
	bool bIsAudioConnected = false;

	if (m_pAudioConnect)
		bIsAudioConnected = m_pAudioConnect->canDisconnectAll();

	return bIsAudioConnected;
}


// Connect current selected JACK audio ports.
void qjackctlConnectionsForm::audioConnectSelected (void)
{
	if (m_pAudioConnect) {
		if (m_pAudioConnect->connectSelected())
			refreshAudio(false);
	}
}


// Disconnect current selected JACK audio ports.
void qjackctlConnectionsForm::audioDisconnectSelected (void)
{
	if (m_pAudioConnect) {
		if (m_pAudioConnect->disconnectSelected())
			refreshAudio(false);
	}
}


// Disconnect all connected JACK audio ports.
void qjackctlConnectionsForm::audioDisconnectAll()
{
	if (m_pAudioConnect) {
		if (m_pAudioConnect->disconnectAll())
			refreshAudio(false);
	}
}


// Refresh JACK audio form by notifying the parent form.
void qjackctlConnectionsForm::audioRefresh ( void )
{
	refreshAudio(false);
}


// A JACK audio helper stabilization slot.
void qjackctlConnectionsForm::audioStabilize ( void )
{
	stabilizeAudio(true);
}


// Connect current selected JACK MIDI ports.
void qjackctlConnectionsForm::midiConnectSelected (void)
{
	if (m_pMidiConnect) {
		if (m_pMidiConnect->connectSelected())
			refreshMidi(false);
	}
}


// Check if there's JACK MIDI connections.
bool qjackctlConnectionsForm::isMidiConnected (void)
{
	bool bIsMidiConnected = false;

	if (m_pMidiConnect)
		bIsMidiConnected = m_pMidiConnect->canDisconnectAll();

	return bIsMidiConnected;
}


// Disconnect current selected JACK MIDI ports.
void qjackctlConnectionsForm::midiDisconnectSelected (void)
{
	if (m_pMidiConnect) {
		if (m_pMidiConnect->disconnectSelected())
			refreshMidi(false);
	}
}


// Disconnect all connected JACK MIDI ports.
void qjackctlConnectionsForm::midiDisconnectAll()
{
	if (m_pMidiConnect) {
		if (m_pMidiConnect->disconnectAll())
			refreshMidi(false);
	}
}


// Refresh JACK MIDI form by notifying the parent form.
void qjackctlConnectionsForm::midiRefresh ( void )
{
	refreshMidi(false);
}


// A JACK MIDI helper stabilization slot.
void qjackctlConnectionsForm::midiStabilize ( void )
{
	stabilizeMidi(true);
}


// Check if there's ALSA MIDI connections.
bool qjackctlConnectionsForm::isAlsaConnected (void)
{
	bool bIsConnected = false;

	if (m_pAlsaConnect)
		bIsConnected = m_pAlsaConnect->canDisconnectAll();

	return bIsConnected;
}


// Connect current selected ports.
void qjackctlConnectionsForm::alsaConnectSelected (void)
{
	if (m_pAlsaConnect) {
		if (m_pAlsaConnect->connectSelected())
			refreshAlsa(false);
	}
}


// Disconnect current selected ports.
void qjackctlConnectionsForm::alsaDisconnectSelected (void)
{
	if (m_pAlsaConnect) {
		if (m_pAlsaConnect->disconnectSelected())
			refreshAlsa(false);
	}
}


// Disconnect all connected ports.
void qjackctlConnectionsForm::alsaDisconnectAll()
{
	if (m_pAlsaConnect) {
		if (m_pAlsaConnect->disconnectAll())
			refreshAlsa(false);
	}
}


// Refresh complete form by notifying the parent form.
void qjackctlConnectionsForm::alsaRefresh ( void )
{
	refreshAlsa(false);
}


// A helper stabilization slot.
void qjackctlConnectionsForm::alsaStabilize ( void )
{
	stabilizeAlsa(true);
}


// Either rebuild all connections now
// or notify main form for doing that later.
void qjackctlConnectionsForm::refreshAudio ( bool bEnabled )
{
	if (m_pAudioConnect == NULL)
		return;

	if (bEnabled) {
		m_pAudioConnect->refresh();
		stabilizeAudio(true);
	} else {
		qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
		if (pMainForm)
			pMainForm->refreshJackConnections();
	}
}

void qjackctlConnectionsForm::refreshMidi ( bool bEnabled )
{
	if (m_pMidiConnect == NULL)
		return;

	if (bEnabled) {
		m_pMidiConnect->refresh();
		stabilizeMidi(true);
	} else {
		qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
		if (pMainForm)
			pMainForm->refreshJackConnections();
	}
}

void qjackctlConnectionsForm::refreshAlsa ( bool bEnabled )
{
	if (m_pAlsaConnect == NULL)
		return;

	if (bEnabled) {
		m_pAlsaConnect->refresh();
		stabilizeAlsa(true);
	} else {
		qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
		if (pMainForm)
			pMainForm->refreshAlsaConnections();
	}
}


// Proper enablement of connections command controls.
void qjackctlConnectionsForm::stabilizeAudio ( bool bEnabled )
{
	if (m_pAudioConnect && bEnabled) {
		AudioConnectPushButton->setEnabled(
			m_pAudioConnect->canConnectSelected());
		AudioDisconnectPushButton->setEnabled(
			m_pAudioConnect->canDisconnectSelected());
		AudioDisconnectAllPushButton->setEnabled(
			m_pAudioConnect->canDisconnectAll());
		AudioRefreshPushButton->setEnabled(true);
	} else {
		AudioConnectPushButton->setEnabled(false);
		AudioDisconnectPushButton->setEnabled(false);
		AudioDisconnectAllPushButton->setEnabled(false);
		AudioRefreshPushButton->setEnabled(false);
	}
}

void qjackctlConnectionsForm::stabilizeMidi ( bool bEnabled )
{
	if (m_pMidiConnect && bEnabled) {
		MidiConnectPushButton->setEnabled(
			m_pMidiConnect->canConnectSelected());
		MidiDisconnectPushButton->setEnabled(
			m_pMidiConnect->canDisconnectSelected());
		MidiDisconnectAllPushButton->setEnabled(
			m_pMidiConnect->canDisconnectAll());
		MidiRefreshPushButton->setEnabled(true);
	} else {
		MidiConnectPushButton->setEnabled(false);
		MidiDisconnectPushButton->setEnabled(false);
		MidiDisconnectAllPushButton->setEnabled(false);
		MidiRefreshPushButton->setEnabled(false);
	}
}

void qjackctlConnectionsForm::stabilizeAlsa ( bool bEnabled )
{
	if (m_pAlsaConnect && bEnabled) {
		AlsaConnectPushButton->setEnabled(m_pAlsaConnect->canConnectSelected());
		AlsaDisconnectPushButton->setEnabled(m_pAlsaConnect->canDisconnectSelected());
		AlsaDisconnectAllPushButton->setEnabled(m_pAlsaConnect->canDisconnectAll());
		AlsaRefreshPushButton->setEnabled(true);
	} else {
		AlsaConnectPushButton->setEnabled(false);
		AlsaDisconnectPushButton->setEnabled(false);
		AlsaDisconnectAllPushButton->setEnabled(false);
		AlsaRefreshPushButton->setEnabled(false);
	}
}


// Client/port aliasing feature update.
void qjackctlConnectionsForm::updateAliases (void)
{
	// Set alias maps for all listviews...
	if (m_pSetup && m_pSetup->bAliasesEnabled) {
		bool bRenameEnabled = m_pSetup->bAliasesEditing;
		AudioConnectView->OListView()->setAliases(
			&(m_pSetup->aliasAudioOutputs), bRenameEnabled);
		AudioConnectView->IListView()->setAliases(
			&(m_pSetup->aliasAudioInputs),  bRenameEnabled);
		MidiConnectView->OListView()->setAliases(
			&(m_pSetup->aliasMidiOutputs), bRenameEnabled);
		MidiConnectView->IListView()->setAliases(
			&(m_pSetup->aliasMidiInputs),  bRenameEnabled);
		AlsaConnectView->OListView()->setAliases(
			&(m_pSetup->aliasAlsaOutputs), bRenameEnabled);
		AlsaConnectView->IListView()->setAliases(
			&(m_pSetup->aliasAlsaInputs),  bRenameEnabled);
	} else {
		AudioConnectView->OListView()->setAliases(NULL, false);
		AudioConnectView->IListView()->setAliases(NULL, false);
		MidiConnectView->OListView()->setAliases(NULL, false);
		MidiConnectView->IListView()->setAliases(NULL, false);
		AlsaConnectView->OListView()->setAliases(NULL, false);
		AlsaConnectView->IListView()->setAliases(NULL, false);
	}
}


// end of qjackctlConnectionsForm.ui.h
