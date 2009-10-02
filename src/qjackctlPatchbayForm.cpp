// qjackctlPatchbayForm.cpp
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

#include "qjackctlAbout.h"
#include "qjackctlPatchbayForm.h"

#include "qjackctlPatchbayFile.h"
#include "qjackctlSetup.h"

#include "qjackctlMainForm.h"
#include "qjackctlSocketForm.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QPixmap>

#include <QShowEvent>
#include <QHideEvent>


//----------------------------------------------------------------------------
// qjackctlPatchbayForm -- UI wrapper form.

// Constructor.
qjackctlPatchbayForm::qjackctlPatchbayForm (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QWidget(pParent, wflags)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	m_pSetup = NULL;

	// Create the patchbay view object.
	m_pPatchbay = new qjackctlPatchbay(m_ui.PatchbayView);
	m_iUntitled = 0;

	m_bActivePatchbay = false;

	m_iUpdate = 0;

	// UI connections...

	QObject::connect(m_ui.NewPatchbayPushButton,
		SIGNAL(clicked()),
		SLOT(newPatchbay()));
	QObject::connect(m_ui.LoadPatchbayPushButton,
		SIGNAL(clicked()),
		SLOT(loadPatchbay()));
	QObject::connect(m_ui.SavePatchbayPushButton,
		SIGNAL(clicked()),
		SLOT(savePatchbay()));
	QObject::connect(m_ui.PatchbayComboBox,
		SIGNAL(activated(int)),
		SLOT(selectPatchbay(int)));
	QObject::connect(m_ui.ActivatePatchbayPushButton,
		SIGNAL(clicked()),
		SLOT(toggleActivePatchbay()));

	QObject::connect(m_ui.OSocketAddPushButton,
		SIGNAL(clicked()),
		SLOT(addOSocket()));
	QObject::connect(m_ui.OSocketEditPushButton,
		SIGNAL(clicked()),
		SLOT(editOSocket()));
	QObject::connect(m_ui.OSocketCopyPushButton,
		SIGNAL(clicked()),
		SLOT(copyOSocket()));
	QObject::connect(m_ui.OSocketRemovePushButton,
		SIGNAL(clicked()),
		SLOT(removeOSocket()));
	QObject::connect(m_ui.OSocketMoveUpPushButton,
		SIGNAL(clicked()),
		SLOT(moveUpOSocket()));
	QObject::connect(m_ui.OSocketMoveDownPushButton,
		SIGNAL(clicked()),
		SLOT(moveDownOSocket()));

	QObject::connect(m_ui.ISocketAddPushButton,
		SIGNAL(clicked()),
		SLOT(addISocket()));
	QObject::connect(m_ui.ISocketEditPushButton,
		SIGNAL(clicked()),
		SLOT(editISocket()));
	QObject::connect(m_ui.ISocketCopyPushButton,
		SIGNAL(clicked()),
		SLOT(copyISocket()));
	QObject::connect(m_ui.ISocketRemovePushButton,
		SIGNAL(clicked()),
		SLOT(removeISocket()));
	QObject::connect(m_ui.ISocketMoveUpPushButton,
		SIGNAL(clicked()),
		SLOT(moveUpISocket()));
	QObject::connect(m_ui.ISocketMoveDownPushButton,
		SIGNAL(clicked()),
		SLOT(moveDownISocket()));

	QObject::connect(m_ui.ConnectPushButton,
		SIGNAL(clicked()),
		SLOT(connectSelected()));
	QObject::connect(m_ui.DisconnectPushButton,
		SIGNAL(clicked()),
		SLOT(disconnectSelected()));
	QObject::connect(m_ui.DisconnectAllPushButton,
		SIGNAL(clicked()),
		SLOT(disconnectAll()));

	QObject::connect(m_ui.RefreshPushButton,
		SIGNAL(clicked()),
		SLOT(refreshForm()));

	// Connect it to some UI feedback slot.
	QObject::connect(m_ui.PatchbayView->OListView(),
		SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		SLOT(stabilizeForm()));
	QObject::connect(m_ui.PatchbayView->IListView(),
		SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		SLOT(stabilizeForm()));

	// Dirty patchbay dispatcher (stabilization deferral).
	QObject::connect(m_ui.PatchbayView,
		SIGNAL(contentsChanged()),
		SLOT(contentsChanged()));

	newPatchbayFile(false);
	stabilizeForm();
}


// Destructor.
qjackctlPatchbayForm::~qjackctlPatchbayForm (void)
{
	// May delete the patchbay view object.
	delete m_pPatchbay;
}


// Notify our parent that we're emerging.
void qjackctlPatchbayForm::showEvent ( QShowEvent *pShowEvent )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();

	stabilizeForm();

	QWidget::showEvent(pShowEvent);
}

// Notify our parent that we're closing.
void qjackctlPatchbayForm::hideEvent ( QHideEvent *pHideEvent )
{
	QWidget::hideEvent(pHideEvent);

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}

// Just about to notify main-window that we're closing.
void qjackctlPatchbayForm::closeEvent ( QCloseEvent * /*pCloseEvent*/ )
{
	QWidget::hide();

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}


// Set reference to global options, mostly needed for the
// initial sizes of the main splitter views...
void qjackctlPatchbayForm::setup ( qjackctlSetup *pSetup )
{
	m_pSetup = pSetup;

	// Load main splitter sizes...
	if (m_pSetup) {
		QList<int> sizes;
		sizes.append(180);
		sizes.append(60);
		sizes.append(180);
		m_pSetup->loadSplitterSizes(m_ui.PatchbayView, sizes);
	}
}


// Patchbay view accessor.
qjackctlPatchbayView *qjackctlPatchbayForm::patchbayView (void) const
{
	return m_ui.PatchbayView;
}


// Window close event handlers.
bool qjackctlPatchbayForm::queryClose (void)
{
	bool bQueryClose = true;

	if (m_ui.PatchbayView->dirty()) {
		switch (QMessageBox::warning(this,
			tr("Warning") + " - " QJACKCTL_SUBTITLE1,
			tr("The patchbay definition has been changed:\n\n"
			"\"%1\"\n\nDo you want to save the changes?")
			.arg(m_sPatchbayName),
			QMessageBox::Save |
			QMessageBox::Discard |
			QMessageBox::Cancel)) {
		case QMessageBox::Save:
			savePatchbay();
			// Fall thru....
		case QMessageBox::Discard:
			break;
		default:    // Cancel.
			bQueryClose = false;
		}
	}

	// Save main splitter sizes...
	if (m_pSetup && bQueryClose)
		m_pSetup->saveSplitterSizes(m_ui.PatchbayView);

	return bQueryClose;
}


// Contents change deferrer slot...
void qjackctlPatchbayForm::contentsChanged (void)
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->refreshPatchbay();
}


// Refresh complete form.
void qjackctlPatchbayForm::refreshForm ( void )
{
	m_pPatchbay->refresh();
	stabilizeForm();
}


// A helper stabilization slot.
void qjackctlPatchbayForm::stabilizeForm ( void )
{
	m_ui.SavePatchbayPushButton->setEnabled(m_ui.PatchbayView->dirty());
	m_ui.ActivatePatchbayPushButton->setEnabled(
		QFileInfo(m_sPatchbayPath).exists());

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	m_bActivePatchbay = (pMainForm
		&& pMainForm->isActivePatchbay(m_sPatchbayPath));
	m_ui.ActivatePatchbayPushButton->setChecked(m_bActivePatchbay);

	if (m_ui.PatchbayView->dirty()) {
		m_ui.PatchbayComboBox->setItemText(
			m_ui.PatchbayComboBox->currentIndex(),
			tr("%1 [modified]").arg(m_sPatchbayName));
	}

	// Take care that IT might be destroyed already...
	if (m_ui.PatchbayView->binding() == NULL)
		return;

	qjackctlSocketList *pSocketList;
	qjackctlSocketItem *pSocketItem;

	pSocketList = m_pPatchbay->OSocketList();
	pSocketItem = pSocketList->selectedSocketItem();
	if (pSocketItem) {
		int iItem = (pSocketList->listView())->indexOfTopLevelItem(pSocketItem);
		int iItemCount = (pSocketList->listView())->topLevelItemCount();
		m_ui.OSocketEditPushButton->setEnabled(true);
		m_ui.OSocketCopyPushButton->setEnabled(true);
		m_ui.OSocketRemovePushButton->setEnabled(true);
		m_ui.OSocketMoveUpPushButton->setEnabled(iItem > 0);
		m_ui.OSocketMoveDownPushButton->setEnabled(iItem < iItemCount - 1);
	} else {
		m_ui.OSocketEditPushButton->setEnabled(false);
		m_ui.OSocketCopyPushButton->setEnabled(false);
		m_ui.OSocketRemovePushButton->setEnabled(false);
		m_ui.OSocketMoveUpPushButton->setEnabled(false);
		m_ui.OSocketMoveDownPushButton->setEnabled(false);
	}

	pSocketList = m_pPatchbay->ISocketList();
	pSocketItem = pSocketList->selectedSocketItem();
	if (pSocketItem) {
		int iItem = (pSocketList->listView())->indexOfTopLevelItem(pSocketItem);
		int iItemCount = (pSocketList->listView())->topLevelItemCount();
		m_ui.ISocketEditPushButton->setEnabled(true);
		m_ui.ISocketCopyPushButton->setEnabled(true);
		m_ui.ISocketRemovePushButton->setEnabled(true);
		m_ui.ISocketMoveUpPushButton->setEnabled(iItem > 0);
		m_ui.ISocketMoveDownPushButton->setEnabled(iItem < iItemCount - 1);
	} else {
		m_ui.ISocketEditPushButton->setEnabled(false);
		m_ui.ISocketCopyPushButton->setEnabled(false);
		m_ui.ISocketRemovePushButton->setEnabled(false);
		m_ui.ISocketMoveUpPushButton->setEnabled(false);
		m_ui.ISocketMoveDownPushButton->setEnabled(false);
	}

	m_ui.ConnectPushButton->setEnabled(
		m_pPatchbay->canConnectSelected());
	m_ui.DisconnectPushButton->setEnabled(
		m_pPatchbay->canDisconnectSelected());
	m_ui.DisconnectAllPushButton->setEnabled(
		m_pPatchbay->canDisconnectAll());
}


// Patchbay path accessor.
const QString& qjackctlPatchbayForm::patchbayPath (void) const
{
	return m_sPatchbayPath;
}


// Reset patchbay definition from scratch.
void qjackctlPatchbayForm::newPatchbayFile ( bool bSnapshot )
{
	m_pPatchbay->clear();
	m_sPatchbayPath.clear();
	m_sPatchbayName = tr("Untitled%1").arg(m_iUntitled++);
	if (bSnapshot)
		m_pPatchbay->connectionsSnapshot();
//	updateRecentPatchbays();
}


// Load patchbay definitions from specific file path.
bool qjackctlPatchbayForm::loadPatchbayFile ( const QString& sFileName )
{
	// Check if we're going to discard safely the current one...
	if (!queryClose())
		return false;

	// We'll have a temporary rack...
	qjackctlPatchbayRack rack;
	// Step 1: load from file...
	if (!qjackctlPatchbayFile::load(&rack, sFileName)) {
		QMessageBox::critical(this,
			tr("Error") + " - " QJACKCTL_SUBTITLE1,
			tr("Could not load patchbay definition file: \n\n\"%1\"")
			.arg(sFileName),
			QMessageBox::Cancel);
		// Reset/disable further trials.
		m_sPatchbayPath.clear();
		return false;
	}
	// Step 2: load from rack...
	m_pPatchbay->loadRack(&rack);

	// Step 3: stabilize form...
	m_sPatchbayPath = sFileName;
	m_sPatchbayName = QFileInfo(sFileName).completeBaseName();
//	updateRecentPatchbays();

	return true;
}


// Save current patchbay definition to specific file path.
bool qjackctlPatchbayForm::savePatchbayFile ( const QString& sFileName )
{
	// We'll have a temporary rack...
	qjackctlPatchbayRack rack;
	// Step 1: save to rack...
	m_pPatchbay->saveRack(&rack);
	// Step 2: save to file...
	if (!qjackctlPatchbayFile::save(&rack, sFileName)) {
		QMessageBox::critical(this,
			tr("Error") + " - " QJACKCTL_SUBTITLE1,
			tr("Could not save patchbay definition file: \n\n\"%1\"")
			.arg(sFileName),
			QMessageBox::Cancel);
		return false;
	}
	// Step 3: stabilize form...
	m_sPatchbayPath = sFileName;
	m_sPatchbayName = QFileInfo(sFileName).completeBaseName();
//	updateRecentPatchbays();

	// Step 4: notify main form if applicable ...
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	m_bActivePatchbay = (pMainForm
		&& pMainForm->isActivePatchbay(m_sPatchbayPath));
	if (m_bActivePatchbay)
		pMainForm->updateActivePatchbay();

	return true;
}


// Dirty-(re)load patchbay definitions from know rack.
void qjackctlPatchbayForm::loadPatchbayRack ( qjackctlPatchbayRack *pRack )
{
	// Step 1: load from rack...
	m_pPatchbay->loadRack(pRack);

	// Override dirty flag.
	m_ui.PatchbayView->setDirty(true);

	// Done.
	stabilizeForm();
}


// Create a new patchbay definition from scratch.
void qjackctlPatchbayForm::newPatchbay (void)
{
	// Assume a snapshot from scratch...
	bool bSnapshot = false;

	// Ask user what he/she wants to do...
	if (m_pPatchbay->jackClient() || m_pPatchbay->alsaSeq()) {
		switch (QMessageBox::information(this,
			tr("New Patchbay definition") + " - " QJACKCTL_SUBTITLE1,
			tr("Create patchbay definition as a snapshot\n"
			"of all actual client connections?"),
			QMessageBox::Yes |
			QMessageBox::No |
			QMessageBox::Cancel)) {
		case QMessageBox::Yes:
			bSnapshot = true;
			break;
		case QMessageBox::No:
			bSnapshot = false;
			break;
		default:	// Cancel.
			return;
		}
	}

	// Check if we can discard safely the current one...
	if (!queryClose())
		return;

	// Reset patchbay editor.
	newPatchbayFile(bSnapshot);
	updateRecentPatchbays();
	stabilizeForm();
}


// Load patchbay definitions from file.
void qjackctlPatchbayForm::loadPatchbay (void)
{
	QString sFileName = QFileDialog::getOpenFileName(
		this, tr("Load Patchbay Definition"),			// Parent & Caption.
		m_sPatchbayPath,								// Start here.
		tr("Patchbay Definition files") + " (*.xml)"	// Filter (XML files)
	);

	if (sFileName.isEmpty())
		return;

	// Load it right away.
	if (loadPatchbayFile(sFileName))
		updateRecentPatchbays();

	stabilizeForm();
}


// Save current patchbay definition to file.
void qjackctlPatchbayForm::savePatchbay (void)
{
	QString sFileName = QFileDialog::getSaveFileName(
		this, tr("Save Patchbay Definition"),			// Parent & Caption.
		m_sPatchbayPath,								// Start here.
		tr("Patchbay Definition files") + " (*.xml)"	// Filter (XML files)
	);

	if (sFileName.isEmpty())
		return;

	// Enforce .xml extension...
	if (QFileInfo(sFileName).suffix().isEmpty())
		sFileName += ".xml";

	// Save it right away.
	if (savePatchbayFile(sFileName))
		updateRecentPatchbays();

	stabilizeForm();
}


// A new patchbay has been selected
void qjackctlPatchbayForm::selectPatchbay ( int iPatchbay )
{

	// Remember and avoid reloading the previous (first) selected one.
	if (iPatchbay > 0) {
		// Take care whether the first is untitled, and
		// thus not present in the recenet ptachbays list
		if (m_sPatchbayPath.isEmpty())
			iPatchbay--;
		if (iPatchbay >= 0 && iPatchbay < m_recentPatchbays.count()) {
			// If we cannot load the new one, backout...
			loadPatchbayFile(m_recentPatchbays[iPatchbay]);
			updateRecentPatchbays();
		}
	}

	stabilizeForm();
}


// Set current active patchbay definition file.
void qjackctlPatchbayForm::toggleActivePatchbay (void)
{
	// Check if we're going to discard safely the current one...
	if (!queryClose())
		return;

	// Activate it...
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm) {
		pMainForm->setActivePatchbay(
			m_bActivePatchbay ? QString::null : m_sPatchbayPath);
	}

	// Need to force/refresh the patchbay list...
	updateRecentPatchbays();
	stabilizeForm();
}


// Set/initialize the MRU patchbay list.
void qjackctlPatchbayForm::setRecentPatchbays ( const QStringList& patchbays )
{
	m_recentPatchbays = patchbays;
}


// Update patchbay MRU variables and widgets.
void qjackctlPatchbayForm::updateRecentPatchbays (void)
{
	// TRye not to be reeentrant.
	if (m_iUpdate > 0)
		return;

	m_iUpdate++;

	// Update the visible combobox...
	const QIcon icon(":/icons/patchbay1.png");
	m_ui.PatchbayComboBox->clear();

	if (m_sPatchbayPath.isEmpty()) {
		// Display a probable untitled patchbay...
		m_ui.PatchbayComboBox->addItem(icon, m_sPatchbayName);
	} else  {
		// Remove from list if already there (avoid duplicates)...
		int iIndex = m_recentPatchbays.indexOf(m_sPatchbayPath);
		if (iIndex >= 0)
			m_recentPatchbays.removeAt(iIndex);
		// Put it to front...
		m_recentPatchbays.push_front(m_sPatchbayPath);
	}

	// Time to keep the list under limits.
	while (m_recentPatchbays.count() > 8)
		m_recentPatchbays.pop_back();

	// Update the main setup list...
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->setRecentPatchbays(m_recentPatchbays);

	QStringListIterator iter(m_recentPatchbays);
	while (iter.hasNext()) {
		const QString& sPatchbayPath = iter.next();
		QString sText = QFileInfo(sPatchbayPath).completeBaseName();
		if (pMainForm && pMainForm->isActivePatchbay(sPatchbayPath))
			sText += " [" + tr("active") + "]";
		m_ui.PatchbayComboBox->addItem(icon, sText);
	}

	// Sure this one must be currently selected.
	m_ui.PatchbayComboBox->setCurrentIndex(0);
//	stabilizeForm();

	m_iUpdate--;
}


// (Un)Bind a JACK client to this form.
void qjackctlPatchbayForm::setJackClient ( jack_client_t *pJackClient )
{
	m_pPatchbay->setJackClient(pJackClient);
}


// (Un)Bind a ALSA sequencer to this form.
void qjackctlPatchbayForm::setAlsaSeq ( snd_seq_t *pAlsaSeq )
{
	m_pPatchbay->setAlsaSeq(pAlsaSeq);
}


// Output socket list push button handlers gallore...

void qjackctlPatchbayForm::addOSocket (void)
{
	(m_pPatchbay->OSocketList())->addSocketItem();
}


void qjackctlPatchbayForm::removeOSocket (void)
{
	(m_pPatchbay->OSocketList())->removeSocketItem();
}


void qjackctlPatchbayForm::editOSocket (void)
{
	(m_pPatchbay->OSocketList())->editSocketItem();
}


void qjackctlPatchbayForm::copyOSocket (void)
{
	(m_pPatchbay->OSocketList())->copySocketItem();
}


void qjackctlPatchbayForm::moveUpOSocket (void)
{
	(m_pPatchbay->OSocketList())->moveUpSocketItem();
}


void qjackctlPatchbayForm::moveDownOSocket (void)
{
	(m_pPatchbay->OSocketList())->moveDownSocketItem();
}


// Input socket list push button handlers gallore...

void qjackctlPatchbayForm::addISocket (void)
{
	(m_pPatchbay->ISocketList())->addSocketItem();
}


void qjackctlPatchbayForm::removeISocket (void)
{
	(m_pPatchbay->ISocketList())->removeSocketItem();
}


void qjackctlPatchbayForm::editISocket (void)
{
	(m_pPatchbay->ISocketList())->editSocketItem();
}


void qjackctlPatchbayForm::copyISocket (void)
{
	(m_pPatchbay->ISocketList())->copySocketItem();
}


void qjackctlPatchbayForm::moveUpISocket (void)
{
	(m_pPatchbay->ISocketList())->moveUpSocketItem();
}


void qjackctlPatchbayForm::moveDownISocket (void)
{
	(m_pPatchbay->ISocketList())->moveDownSocketItem();
}


// Connect current selected ports.
void qjackctlPatchbayForm::connectSelected (void)
{
	m_pPatchbay->connectSelected();
}


// Disconnect current selected ports.
void qjackctlPatchbayForm::disconnectSelected (void)
{
	m_pPatchbay->disconnectSelected();
}


// Disconnect all connected ports.
void qjackctlPatchbayForm::disconnectAll (void)
{
	m_pPatchbay->disconnectAll();
}


// Keyboard event handler.
void qjackctlPatchbayForm::keyPressEvent ( QKeyEvent *pKeyEvent )
{
#ifdef CONFIG_DEBUG_0
	qDebug("qjackctlPatchbayForm::keyPressEvent(%d)", pKeyEvent->key());
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

	// Make sure we've get focus back...
	QWidget::setFocus();
}


// end of qjackctlPatchbayForm.cpp
