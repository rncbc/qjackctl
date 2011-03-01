// qjackctlConnectionsForm.cpp
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

#include "qjackctlAbout.h"
#include "qjackctlConnectionsForm.h"

#include "qjackctlSetup.h"

#include "qjackctlMainForm.h"
#include "qjackctlPatchbay.h"

#include <QMessageBox>

#include <QShowEvent>
#include <QHideEvent>


//----------------------------------------------------------------------------
// qjackctlConnectionsForm -- UI wrapper form.

// Constructor.
qjackctlConnectionsForm::qjackctlConnectionsForm (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QWidget(pParent, wflags)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	m_pAudioConnect = new qjackctlJackConnect(
		m_ui.AudioConnectView, QJACKCTL_JACK_AUDIO);
#ifdef CONFIG_JACK_MIDI
	m_pMidiConnect = new qjackctlJackConnect(
		m_ui.MidiConnectView, QJACKCTL_JACK_MIDI);
#else
	m_pMidiConnect = NULL;
#endif

#ifdef CONFIG_ALSA_SEQ
	m_pAlsaConnect = new qjackctlAlsaConnect(m_ui.AlsaConnectView);
#else
	m_pAlsaConnect = NULL;
#endif

	m_pSetup = NULL;

	// UI connections...

	QObject::connect(m_ui.AudioConnectPushButton,
		SIGNAL(clicked()),
		SLOT(audioConnectSelected()));
	QObject::connect(m_ui.AudioDisconnectPushButton,
		SIGNAL(clicked()),
		SLOT(audioDisconnectSelected()));
	QObject::connect(m_ui.AudioDisconnectAllPushButton,
		SIGNAL(clicked()),
		SLOT(audioDisconnectAll()));
	QObject::connect(m_ui.AudioExpandAllPushButton,
		SIGNAL(clicked()),
		SLOT(audioExpandAll()));
	QObject::connect(m_ui.AudioRefreshPushButton,
		SIGNAL(clicked()),
		SLOT(audioRefreshClear()));

	QObject::connect(m_ui.MidiConnectPushButton,
		SIGNAL(clicked()),
		SLOT(midiConnectSelected()));
	QObject::connect(m_ui.MidiDisconnectPushButton,
		SIGNAL(clicked()),
		SLOT(midiDisconnectSelected()));
	QObject::connect(m_ui.MidiDisconnectAllPushButton,
		SIGNAL(clicked()),
		SLOT(midiDisconnectAll()));
	QObject::connect(m_ui.MidiExpandAllPushButton,
		SIGNAL(clicked()),
		SLOT(midiExpandAll()));
	QObject::connect(m_ui.MidiRefreshPushButton,
		SIGNAL(clicked()),
		SLOT(midiRefreshClear()));

	QObject::connect(m_ui.AlsaConnectPushButton,
		SIGNAL(clicked()),
		SLOT(alsaConnectSelected()));
	QObject::connect(m_ui.AlsaDisconnectPushButton,
		SIGNAL(clicked()),
		SLOT(alsaDisconnectSelected()));
	QObject::connect(m_ui.AlsaDisconnectAllPushButton,
		SIGNAL(clicked()),
		SLOT(alsaDisconnectAll()));
	QObject::connect(m_ui.AlsaExpandAllPushButton,
		SIGNAL(clicked()),
		SLOT(alsaExpandAll()));
	QObject::connect(m_ui.AlsaRefreshPushButton,
		SIGNAL(clicked()),
		SLOT(alsaRefreshClear()));

	// Connect it to some UI feedback slots.
	QObject::connect(m_ui.AudioConnectView->OListView(),
		SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		SLOT(audioStabilize()));
	QObject::connect(m_ui.AudioConnectView->IListView(),
		SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		SLOT(audioStabilize()));
	QObject::connect(m_ui.MidiConnectView->OListView(),
		SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		SLOT(midiStabilize()));
	QObject::connect(m_ui.MidiConnectView->IListView(),
		SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		SLOT(midiStabilize()));
	QObject::connect(m_ui.AlsaConnectView->OListView(),
		SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		SLOT(alsaStabilize()));
	QObject::connect(m_ui.AlsaConnectView->IListView(),
		SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		SLOT(alsaStabilize()));

	// Dirty dispatcher (refresh deferral).
	QObject::connect(m_ui.AudioConnectView,
		SIGNAL(contentsChanged()),
		SLOT(audioRefresh()));
	QObject::connect(m_ui.MidiConnectView,
		SIGNAL(contentsChanged()),
		SLOT(midiRefresh()));
	QObject::connect(m_ui.AlsaConnectView,
		SIGNAL(contentsChanged()),
		SLOT(alsaRefresh()));

	// Actual connections...
	QObject::connect(m_pAudioConnect,
		SIGNAL(disconnecting(qjackctlPortItem *, qjackctlPortItem *)),
		SLOT(audioDisconnecting(qjackctlPortItem *, qjackctlPortItem *)));
#ifdef CONFIG_JACK_MIDI
	QObject::connect(m_pMidiConnect,
		SIGNAL(disconnecting(qjackctlPortItem *, qjackctlPortItem *)),
		SLOT(midiDisconnecting(qjackctlPortItem *, qjackctlPortItem *)));
#endif
#ifdef CONFIG_ALSA_SEQ
	QObject::connect(m_pAlsaConnect,
		SIGNAL(disconnecting(qjackctlPortItem *, qjackctlPortItem *)),
		SLOT(alsaDisconnecting(qjackctlPortItem *, qjackctlPortItem *)));
#endif

#ifndef CONFIG_JACK_MIDI
	m_ui.ConnectionsTabWidget->setTabEnabled(1, false);
#endif
#ifndef CONFIG_ALSA_SEQ
//	m_ui.ConnectionsTabWidget->setTabEnabled(2, false);
	m_ui.ConnectionsTabWidget->removeTab(2);
#endif

	// Start disabled.
	stabilizeAudio(false);
	stabilizeMidi(false);
	stabilizeAlsa(false);
}


// Destructor.
qjackctlConnectionsForm::~qjackctlConnectionsForm (void)
{
	// Destroy our connections view...
	if (m_pAudioConnect)
		delete m_pAudioConnect;
#ifdef CONFIG_JACK_MIDI
	if (m_pMidiConnect)
		delete m_pMidiConnect;
#endif
#ifdef CONFIG_ALSA_SEQ
	if (m_pAlsaConnect)
		delete m_pAlsaConnect;
#endif
}


// Notify our parent that we're emerging.
void qjackctlConnectionsForm::showEvent ( QShowEvent *pShowEvent )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
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

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}

// Just about to notify main-window that we're closing.
void qjackctlConnectionsForm::closeEvent ( QCloseEvent * /*pCloseEvent*/ )
{
	QWidget::hide();

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}


// Set reference to global options, mostly needed for the
// initial sizes of the main splitter views and those
// client/port aliasing feature.
void qjackctlConnectionsForm::setup ( qjackctlSetup *pSetup )
{
	m_pSetup = pSetup;

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm) {
		QObject::connect(m_pAudioConnect, SIGNAL(connectChanged()),
			pMainForm, SLOT(jackConnectChanged()));
	#ifdef CONFIG_JACK_MIDI
		QObject::connect(m_pMidiConnect, SIGNAL(connectChanged()),
			pMainForm, SLOT(jackConnectChanged()));
	#endif
	#ifdef CONFIG_ALSA_SEQ
		QObject::connect(m_pAlsaConnect, SIGNAL(connectChanged()),
			pMainForm, SLOT(alsaConnectChanged()));
	#endif
	}

	// Load some splitter sizes...
	if (m_pSetup) {
		QList<int> sizes;
		sizes.append(180);
		sizes.append(60);
		sizes.append(180);
		m_pSetup->loadSplitterSizes(m_ui.AudioConnectView, sizes);
		m_pSetup->loadSplitterSizes(m_ui.MidiConnectView, sizes);
		m_pSetup->loadSplitterSizes(m_ui.AlsaConnectView, sizes);
	#ifdef CONFIG_ALSA_SEQ
		if (!m_pSetup->bAlsaSeqEnabled) {
		//	m_ui.ConnectionsTabWidget->setTabEnabled(2, false);
			m_ui.ConnectionsTabWidget->removeTab(2);
		}
	#endif
	}

	// Update initial client/port aliases...
	updateAliases();
}


// Connector view accessors.
qjackctlConnectView *qjackctlConnectionsForm::audioConnectView (void) const
{
	return m_ui.AudioConnectView;
}

qjackctlConnectView *qjackctlConnectionsForm::midiConnectView (void) const
{
	return m_ui.MidiConnectView;
}

qjackctlConnectView *qjackctlConnectionsForm::alsaConnectView (void) const
{
	return m_ui.AlsaConnectView;
}


// Window close event handlers.
bool qjackctlConnectionsForm::queryClose (void)
{
	bool bQueryClose = true;

	if (m_pSetup
		&& (m_ui.AudioConnectView->isDirty() ||
			m_ui.MidiConnectView->isDirty()  ||
			m_ui.AlsaConnectView->isDirty())) {
		switch (QMessageBox::warning(this,
			tr("Warning") + " - " QJACKCTL_SUBTITLE1,
			tr("The preset aliases have been changed:\n\n"
			"\"%1\"\n\nDo you want to save the changes?")
			.arg(m_sPreset),
			QMessageBox::Save |
			QMessageBox::Discard |
			QMessageBox::Cancel)) {
		case QMessageBox::Save:
			saveAliases();
			// Fall thru....
		case QMessageBox::Discard:
			break;
		default:    // Cancel.
			bQueryClose = false;
		}
	}

	// Save some splitter sizes...
	if (m_pSetup && bQueryClose) {
		m_pSetup->saveSplitterSizes(m_ui.AudioConnectView);
		m_pSetup->saveSplitterSizes(m_ui.MidiConnectView);
		m_pSetup->saveSplitterSizes(m_ui.AlsaConnectView);
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
			m_ui.AudioConnectView->setDirty(false);
			m_ui.MidiConnectView->setDirty(false);
			m_ui.AlsaConnectView->setDirty(false);
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
			m_ui.AudioConnectView->setDirty(false);
			m_ui.MidiConnectView->setDirty(false);
			m_ui.AlsaConnectView->setDirty(false);
		}
	}

	return bResult;
}


// Tab page accessors.
void qjackctlConnectionsForm::setTabPage ( int iTabPage )
{
	m_ui.ConnectionsTabWidget->setCurrentIndex(iTabPage);
}

int qjackctlConnectionsForm::tabPage (void) const
{
	return m_ui.ConnectionsTabWidget->currentIndex();
}


// Connections view font accessors.
QFont qjackctlConnectionsForm::connectionsFont (void) const
{
	// Elect one list view to retrieve current font.
	return m_ui.AudioConnectView->OListView()->font();
}

void qjackctlConnectionsForm::setConnectionsFont ( const QFont & font )
{
	// Set fonts of all listviews...
	m_ui.AudioConnectView->OListView()->setFont(font);
	m_ui.AudioConnectView->IListView()->setFont(font);
	m_ui.MidiConnectView->OListView()->setFont(font);
	m_ui.MidiConnectView->IListView()->setFont(font);
	m_ui.AlsaConnectView->OListView()->setFont(font);
	m_ui.AlsaConnectView->IListView()->setFont(font);
}


// Connections view icon size accessor.
void qjackctlConnectionsForm::setConnectionsIconSize ( int iIconSize )
{
	// Set icon sizes of all views...
	m_ui.AudioConnectView->setIconSize(iIconSize);
	m_ui.MidiConnectView->setIconSize(iIconSize);
	m_ui.AlsaConnectView->setIconSize(iIconSize);
}


// Check if there's JACK audio connections.
bool qjackctlConnectionsForm::isAudioConnected (void) const
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
void qjackctlConnectionsForm::audioDisconnectAll (void)
{
	if (m_pAudioConnect) {
		if (m_pAudioConnect->disconnectAll())
			refreshAudio(false);
	}
}


// Expand all JACK audio client ports.
void qjackctlConnectionsForm::audioExpandAll (void)
{
	if (m_pAudioConnect) {
		m_pAudioConnect->expandAll();
		stabilizeAudio(true);
	}
}


// JACK audio ports disconnecting.
void qjackctlConnectionsForm::audioDisconnecting (
	qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->queryDisconnect(pOPort, pIPort, QJACKCTL_SOCKETTYPE_JACK_AUDIO);
}


// Refresh JACK audio form by notifying the parent form.
void qjackctlConnectionsForm::audioRefreshClear (void)
{
	refreshAudio(true, true);
}

void qjackctlConnectionsForm::audioRefresh (void)
{
	refreshAudio(false);
}

// A JACK audio helper stabilization slot.
void qjackctlConnectionsForm::audioStabilize (void)
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
bool qjackctlConnectionsForm::isMidiConnected (void) const
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
void qjackctlConnectionsForm::midiDisconnectAll (void)
{
	if (m_pMidiConnect) {
		if (m_pMidiConnect->disconnectAll())
			refreshMidi(false);
	}
}


// Expand all JACK MIDI client ports.
void qjackctlConnectionsForm::midiExpandAll (void)
{
	if (m_pMidiConnect) {
		m_pMidiConnect->expandAll();
		stabilizeMidi(true);
	}
}


// JACK MIDI ports disconnecting.
void qjackctlConnectionsForm::midiDisconnecting (
	qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->queryDisconnect(pOPort, pIPort, QJACKCTL_SOCKETTYPE_JACK_MIDI);
}


// Refresh JACK MIDI form by notifying the parent form.
void qjackctlConnectionsForm::midiRefreshClear (void)
{
	refreshMidi(true, true);
}

void qjackctlConnectionsForm::midiRefresh (void)
{
	refreshMidi(false);
}


// A JACK MIDI helper stabilization slot.
void qjackctlConnectionsForm::midiStabilize (void)
{
	stabilizeMidi(true);
}


// Check if there's ALSA MIDI connections.
bool qjackctlConnectionsForm::isAlsaConnected (void) const
{
	bool bIsConnected = false;

	if (m_pAlsaConnect)
		bIsConnected = m_pAlsaConnect->canDisconnectAll();

	return bIsConnected;
}


// Connect current selected ALSA MIDI ports.
void qjackctlConnectionsForm::alsaConnectSelected (void)
{
	if (m_pAlsaConnect) {
		if (m_pAlsaConnect->connectSelected())
			refreshAlsa(false);
	}
}


// Disconnect current selected ALSA MIDI ports.
void qjackctlConnectionsForm::alsaDisconnectSelected (void)
{
	if (m_pAlsaConnect) {
		if (m_pAlsaConnect->disconnectSelected())
			refreshAlsa(false);
	}
}


// Disconnect all connected ALSA MIDI ports.
void qjackctlConnectionsForm::alsaDisconnectAll (void)
{
	if (m_pAlsaConnect) {
		if (m_pAlsaConnect->disconnectAll())
			refreshAlsa(false);
	}
}


// Expand all ALSA MIDI client ports.
void qjackctlConnectionsForm::alsaExpandAll (void)
{
	if (m_pAlsaConnect) {
		m_pAlsaConnect->expandAll();
		stabilizeAlsa(true);
	}
}


// ALSA MIDI ports disconnecting.
void qjackctlConnectionsForm::alsaDisconnecting (
	qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->queryDisconnect(pOPort, pIPort, QJACKCTL_SOCKETTYPE_ALSA_MIDI);
}


// Refresh complete form by notifying the parent form.
void qjackctlConnectionsForm::alsaRefreshClear (void)
{
	refreshAlsa(true, true);
}

void qjackctlConnectionsForm::alsaRefresh (void)
{
	refreshAlsa(false);
}


// A helper stabilization slot.
void qjackctlConnectionsForm::alsaStabilize (void)
{
	stabilizeAlsa(true);
}


// Either rebuild all connections now
// or notify main form for doing that later.
void qjackctlConnectionsForm::refreshAudio ( bool bEnabled, bool bClear )
{
	if (m_pAudioConnect == NULL)
		return;

	if (bEnabled) {
	//	m_pAudioConnect->refresh();
		stabilizeAudio(bEnabled, bClear);
	} else {
		qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
		if (pMainForm)
			pMainForm->refreshJackConnections();
	}
}

void qjackctlConnectionsForm::refreshMidi ( bool bEnabled, bool bClear )
{
	if (m_pMidiConnect == NULL)
		return;

	if (bEnabled) {
	//	m_pMidiConnect->refresh();
		stabilizeMidi(bEnabled, bClear);
	} else {
		qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
		if (pMainForm)
			pMainForm->refreshJackConnections();
	}
}

void qjackctlConnectionsForm::refreshAlsa ( bool bEnabled, bool bClear )
{
	if (m_pAlsaConnect == NULL)
		return;

	if (bEnabled) {
	//	m_pAlsaConnect->refresh();
		stabilizeAlsa(bEnabled, bClear);
	} else {
		qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
		if (pMainForm)
			pMainForm->refreshAlsaConnections();
	}
}


// Proper enablement of connections command controls.
void qjackctlConnectionsForm::stabilizeAudio ( bool bEnabled, bool bClear )
{
	if (m_pAudioConnect)
		m_pAudioConnect->updateContents(!bEnabled || bClear);

	if (m_pAudioConnect && bEnabled) {
		m_ui.AudioConnectPushButton->setEnabled(
			m_pAudioConnect->canConnectSelected());
		m_ui.AudioDisconnectPushButton->setEnabled(
			m_pAudioConnect->canDisconnectSelected());
		m_ui.AudioDisconnectAllPushButton->setEnabled(
			m_pAudioConnect->canDisconnectAll());
		m_ui.AudioExpandAllPushButton->setEnabled(true);
		m_ui.AudioRefreshPushButton->setEnabled(true);
	} else {
		m_ui.AudioConnectPushButton->setEnabled(false);
		m_ui.AudioDisconnectPushButton->setEnabled(false);
		m_ui.AudioDisconnectAllPushButton->setEnabled(false);
		m_ui.AudioExpandAllPushButton->setEnabled(false);
		m_ui.AudioRefreshPushButton->setEnabled(false);
	}
}

void qjackctlConnectionsForm::stabilizeMidi ( bool bEnabled, bool bClear )
{
	if (m_pMidiConnect)
		m_pMidiConnect->updateContents(!bEnabled || bClear);

	if (m_pMidiConnect && bEnabled) {
		m_ui.MidiConnectPushButton->setEnabled(
			m_pMidiConnect->canConnectSelected());
		m_ui.MidiDisconnectPushButton->setEnabled(
			m_pMidiConnect->canDisconnectSelected());
		m_ui.MidiDisconnectAllPushButton->setEnabled(
			m_pMidiConnect->canDisconnectAll());
		m_ui.MidiExpandAllPushButton->setEnabled(true);
		m_ui.MidiRefreshPushButton->setEnabled(true);
	} else {
		m_ui.MidiConnectPushButton->setEnabled(false);
		m_ui.MidiDisconnectPushButton->setEnabled(false);
		m_ui.MidiDisconnectAllPushButton->setEnabled(false);
		m_ui.MidiExpandAllPushButton->setEnabled(false);
		m_ui.MidiRefreshPushButton->setEnabled(false);
	}
}

void qjackctlConnectionsForm::stabilizeAlsa ( bool bEnabled, bool bClear )
{
	if (m_pAlsaConnect)
		m_pAlsaConnect->updateContents(!bEnabled || bClear);

	if (m_pAlsaConnect && bEnabled) {
		m_ui.AlsaConnectPushButton->setEnabled(
			m_pAlsaConnect->canConnectSelected());
		m_ui.AlsaDisconnectPushButton->setEnabled(
			m_pAlsaConnect->canDisconnectSelected());
		m_ui.AlsaDisconnectAllPushButton->setEnabled(
			m_pAlsaConnect->canDisconnectAll());
		m_ui.AlsaExpandAllPushButton->setEnabled(true);
		m_ui.AlsaRefreshPushButton->setEnabled(true);
	} else {
		m_ui.AlsaConnectPushButton->setEnabled(false);
		m_ui.AlsaDisconnectPushButton->setEnabled(false);
		m_ui.AlsaDisconnectAllPushButton->setEnabled(false);
		m_ui.AlsaExpandAllPushButton->setEnabled(false);
		m_ui.AlsaRefreshPushButton->setEnabled(false);
	}
}


// Client/port aliasing feature update.
void qjackctlConnectionsForm::updateAliases (void)
{
	// Set alias maps for all listviews...
	if (m_pSetup && m_pSetup->bAliasesEnabled) {
		bool bRenameEnabled = m_pSetup->bAliasesEditing;
		m_ui.AudioConnectView->OListView()->setAliases(
			&(m_pSetup->aliasAudioOutputs), bRenameEnabled);
		m_ui.AudioConnectView->IListView()->setAliases(
			&(m_pSetup->aliasAudioInputs),  bRenameEnabled);
		m_ui.MidiConnectView->OListView()->setAliases(
			&(m_pSetup->aliasMidiOutputs), bRenameEnabled);
		m_ui.MidiConnectView->IListView()->setAliases(
			&(m_pSetup->aliasMidiInputs),  bRenameEnabled);
		m_ui.AlsaConnectView->OListView()->setAliases(
			&(m_pSetup->aliasAlsaOutputs), bRenameEnabled);
		m_ui.AlsaConnectView->IListView()->setAliases(
			&(m_pSetup->aliasAlsaInputs),  bRenameEnabled);
	} else {
		m_ui.AudioConnectView->OListView()->setAliases(NULL, false);
		m_ui.AudioConnectView->IListView()->setAliases(NULL, false);
		m_ui.MidiConnectView->OListView()->setAliases(NULL, false);
		m_ui.MidiConnectView->IListView()->setAliases(NULL, false);
		m_ui.AlsaConnectView->OListView()->setAliases(NULL, false);
		m_ui.AlsaConnectView->IListView()->setAliases(NULL, false);
	}
}


// Keyboard event handler.
void qjackctlConnectionsForm::keyPressEvent ( QKeyEvent *pKeyEvent )
{
#ifdef CONFIG_DEBUG_0
	qDebug("qjackctlConnectionsForm::keyPressEvent(%d)", pKeyEvent->key());
#endif
	int iKey = pKeyEvent->key();
	switch (iKey) {
	case Qt::Key_Escape:
		close();
		break;
	default:
		QWidget::keyPressEvent(pKeyEvent);
		break;
	}
}


// end of qjackctlConnectionsForm.cpp
