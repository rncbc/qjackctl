// qjackctlPatchbayForm.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
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

#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qfileinfo.h>

#include "config.h"

#include "qjackctlPatchbayFile.h"


// Kind of constructor.
void qjackctlPatchbayForm::init (void)
{
    // Create the patchbay view object.
    m_pPatchbay = new qjackctlPatchbay(PatchbayView);
    m_iUntitled = 0;
    
    // Connect it to some UI feedback slot.
    QObject::connect(PatchbayView->OListView(), SIGNAL(selectionChanged(QListViewItem *)), this, SLOT(stabilizeForm()));
    QObject::connect(PatchbayView->IListView(), SIGNAL(selectionChanged(QListViewItem *)), this, SLOT(stabilizeForm()));
    QObject::connect(PatchbayView->OListView(), SIGNAL(currentChanged(QListViewItem *)), this, SLOT(stabilizeForm()));
    QObject::connect(PatchbayView->IListView(), SIGNAL(currentChanged(QListViewItem *)), this, SLOT(stabilizeForm()));

    newPatchbayFile(false);
}


// Kind of destructor.
void qjackctlPatchbayForm::destroy (void)
{
    // May delete the patchbay view object.
    delete m_pPatchbay;
}


// Notify our parent that we're emerging.
void qjackctlPatchbayForm::showEvent ( QShowEvent *pShowEvent )
{
    qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
    if (pMainForm)
        pMainForm->stabilizeForm();

    stabilizeForm();

    QWidget::showEvent(pShowEvent);
}


// Notify our parent that we're closing.
void qjackctlPatchbayForm::hideEvent ( QHideEvent *pHideEvent )
{
    QWidget::hideEvent(pHideEvent);

    qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
    if (pMainForm)
        pMainForm->stabilizeForm();
}


// Window close event handlers.
bool qjackctlPatchbayForm::queryClose (void)
{
    bool bQueryClose = true;

    if (PatchbayView->dirty()) {
        switch (QMessageBox::warning(this, tr("Warning"),
            tr("The patchbay definition has been changed:") + "\n\n" +
            "\"" + m_sPatchbayName +  "\"\n\n" +
            tr("Do you want to save the changes?"),
            tr("Save"), tr("Discard"), tr("Cancel"))) {
        case 0:     // Save...
            savePatchbay();
            // Fall thru....
        case 1:     // Discard
            break;
        default:    // Cancel.
            bQueryClose = false;
        }
    }

    return bQueryClose;
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
    SavePatchbayPushButton->setEnabled(PatchbayView->dirty());
    ActivatePatchbayPushButton->setEnabled(QFileInfo(m_sPatchbayPath).exists());

    QString sText = m_sPatchbayName;
    if (PatchbayView->dirty()) {
        sText += " [" + tr("modified") + "]";
    } else {
        qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
        if (pMainForm && pMainForm->isActivePatchbay(m_sPatchbayPath))
            sText += " [" + tr("active") + "]";
    }
    PatchbayTextLabel->setText(sText);

    qjackctlSocketItem *pSocketItem = (m_pPatchbay->OSocketList())->selectedSocketItem();
    if (pSocketItem) {
        OSocketEditPushButton->setEnabled(true);
        OSocketRemovePushButton->setEnabled(true);
        OSocketMoveUpPushButton->setEnabled(pSocketItem->itemAbove() != NULL);
        OSocketMoveDownPushButton->setEnabled(pSocketItem->nextSibling() != NULL);
    } else {
        OSocketEditPushButton->setEnabled(false);
        OSocketRemovePushButton->setEnabled(false);
        OSocketMoveUpPushButton->setEnabled(false);
        OSocketMoveDownPushButton->setEnabled(false);
    }
    
    pSocketItem = (m_pPatchbay->ISocketList())->selectedSocketItem();
    if (pSocketItem) {
        ISocketEditPushButton->setEnabled(true);
        ISocketRemovePushButton->setEnabled(true);
        ISocketMoveUpPushButton->setEnabled(pSocketItem->itemAbove() != NULL);
        ISocketMoveDownPushButton->setEnabled(pSocketItem->nextSibling() != NULL);
    } else {
        ISocketEditPushButton->setEnabled(false);
        ISocketRemovePushButton->setEnabled(false);
        ISocketMoveUpPushButton->setEnabled(false);
        ISocketMoveDownPushButton->setEnabled(false);
    }
    
    ConnectPushButton->setEnabled(m_pPatchbay->canConnectSelected());
    DisconnectPushButton->setEnabled(m_pPatchbay->canDisconnectSelected());
    DisconnectAllPushButton->setEnabled(m_pPatchbay->canDisconnectAll());
}


// Patchbay path accessor.
QString& qjackctlPatchbayForm::patchbayPath (void)
{
    return m_sPatchbayPath;
}


// Reset patchbay definition from scratch.
void qjackctlPatchbayForm::newPatchbayFile ( bool bSnapshot )
{
    m_pPatchbay->clear();
    m_sPatchbayPath = QString::null;
    m_sPatchbayName = tr("Untitled") + QString::number(m_iUntitled++);
    if (bSnapshot)
        m_pPatchbay->connectionsSnapshot();
    stabilizeForm();
}


// Load patchbay definitions from specific file path.
void qjackctlPatchbayForm::loadPatchbayFile ( const QString& sFileName )
{
    // We'll have a temporary rack...
    qjackctlPatchbayRack rack;
    // Step 1: load from file...
    if (!qjackctlPatchbayFile::load(&rack, sFileName)) {
        QMessageBox::critical(this, tr("Load error"),
            tr("Could not load patchbay definition file:") + "\n\n\"" + sFileName + "\"",
            tr("Cancel"));
        // Reset/disable further trials.
        m_sPatchbayPath = QString::null;
        return;
    }
    // Step 2: load from rack...
    m_pPatchbay->loadRack(&rack);
    // Step 3: stabilize form...
    m_sPatchbayPath = sFileName;
    m_sPatchbayName = QFileInfo(sFileName).baseName();
    stabilizeForm();
}


// Save current patchbay definition to specific file path.
void qjackctlPatchbayForm::savePatchbayFile ( const QString& sFileName )
{
    // We'll have a temporary rack...
    qjackctlPatchbayRack rack;
    // Step 1: save to rack...
    m_pPatchbay->saveRack(&rack);
    // Step 2: save to file...
    if (!qjackctlPatchbayFile::save(&rack, sFileName)) {
        QMessageBox::critical(this, tr("Save error"),
            tr("Could not save patchbay definition file:") + "\n\n\"" + sFileName + "\"",
            tr("Cancel"));
        return;
    }
    // Step 3: stabilize form...
    m_sPatchbayPath = sFileName;
    m_sPatchbayName = QFileInfo(sFileName).baseName();
    stabilizeForm();
}


// Create a new patchbay definition from scratch.
void qjackctlPatchbayForm::newPatchbay()
{
    // Ask for a snapshot for scratch...
    bool bSnapshot = false;
    if (m_pPatchbay->jackClient()) {
        bSnapshot = (QMessageBox::information(this,
            tr("New Patchbay definition"),
            tr("JACK is currently running.") + "\n\n" +
            tr("Create patchbay definition as snapshot") + "\n" +
            tr("of all actual client connections?"),
            tr("Yes"), tr("No")) == 0);
    }

    // Check if we can discard safely the current one...
    if (!queryClose())
        return;

    // Reset patchbay editor.
    newPatchbayFile(bSnapshot);
}


// Load patchbay definitions from file.
void qjackctlPatchbayForm::loadPatchbay()
{
    QString sFileName = QFileDialog::getOpenFileName(
            m_sPatchbayPath,                                // Start here.
            tr("Patchbay Definition files") + " (*.xml)",   // Filter (XML files)
            this, 0,                                        // Parent and name (none)
            tr("Load Patchbay Definition")                  // Caption.
    );
    
    if (sFileName.isEmpty())
        return;

    // Check if we're going to discard safely the current one...
    if (!queryClose())
        return;

    // Load it right away.
    loadPatchbayFile(sFileName);
}


// Save current patchbay definition to file.
void qjackctlPatchbayForm::savePatchbay()
{
    QString sFileName = QFileDialog::getSaveFileName(
            m_sPatchbayPath,                                // Start here.
            tr("Patchbay Definition files") + " (*.xml)",   // Filter (XML files)
            this, 0,                                        // Parent and name (none)
            tr("Save Patchbay Definition")                  // Caption.
    );
    
    if (sFileName.isEmpty())
        return;
        
    // Enforce .xml extension...
    if (QFileInfo(sFileName).extension().isEmpty())
        sFileName += ".xml";
    
    // Save it right away.
    savePatchbayFile(sFileName);
}


// Set current active patchbay definition file.
void qjackctlPatchbayForm::activatePatchbay()
{
    // Check if we're going to discard safely the current one...
    if (!queryClose())
        return;

    // Activate it...
    qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
    if (pMainForm) {
        pMainForm->activatePatchbay(m_sPatchbayPath);
        stabilizeForm();
    }
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

void qjackctlPatchbayForm::addOSocket()
{
    (m_pPatchbay->OSocketList())->addSocketItem();
    stabilizeForm();
}


void qjackctlPatchbayForm::removeOSocket()
{
    (m_pPatchbay->OSocketList())->removeSocketItem();
    stabilizeForm();
}


void qjackctlPatchbayForm::editOSocket()
{
    (m_pPatchbay->OSocketList())->editSocketItem();
    stabilizeForm();
}


void qjackctlPatchbayForm::moveUpOSocket()
{
    (m_pPatchbay->OSocketList())->moveUpSocketItem();
    stabilizeForm();
}


void qjackctlPatchbayForm::moveDownOSocket()
{
    (m_pPatchbay->OSocketList())->moveDownSocketItem();
    stabilizeForm();
}


// Input socket list push button handlers gallore...

void qjackctlPatchbayForm::addISocket()
{
    (m_pPatchbay->ISocketList())->addSocketItem();
    stabilizeForm();
}


void qjackctlPatchbayForm::removeISocket()
{    
    (m_pPatchbay->ISocketList())->removeSocketItem();
    stabilizeForm();
}


void qjackctlPatchbayForm::editISocket()
{
    (m_pPatchbay->ISocketList())->editSocketItem();
    stabilizeForm();
}


void qjackctlPatchbayForm::moveUpISocket()
{
    (m_pPatchbay->ISocketList())->moveUpSocketItem();
    stabilizeForm();
}


void qjackctlPatchbayForm::moveDownISocket()
{
    (m_pPatchbay->ISocketList())->moveDownSocketItem();
    stabilizeForm();
}


// Connect current selected ports.
void qjackctlPatchbayForm::connectSelected (void)
{
    m_pPatchbay->connectSelected();
    stabilizeForm();
}


// Disconnect current selected ports.
void qjackctlPatchbayForm::disconnectSelected (void)
{
    m_pPatchbay->disconnectSelected();
    stabilizeForm();
}


// Disconnect all connected ports.
void qjackctlPatchbayForm::disconnectAll()
{
    m_pPatchbay->disconnectAll();
    stabilizeForm();
}


// end of qjackctlPatchbayForm.ui.h
