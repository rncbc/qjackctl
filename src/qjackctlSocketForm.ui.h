// qjackctlSocketForm.ui.h
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

#include <stdlib.h>

#include "config.h"


// Kind of constructor.
void qjackctlSocketForm::init()
{
    m_bReadable    = false;
    m_pJackClient  = NULL;
    m_pAlsaSeq     = NULL;
    m_pXpmASocket  = NULL;
    m_pXpmMSocket  = NULL;
    m_pXpmAPlug    = NULL;
    m_pXpmMPlug    = NULL;
    m_pAddPlugMenu = NULL;

    PlugListView->setSorting(-1);
}


// Kind of destructor.
void qjackctlSocketForm::destroy()
{
    if (m_pAddPlugMenu)
        delete m_pAddPlugMenu;
    m_pAddPlugMenu = NULL;
}


// Socket caption utility method.
void qjackctlSocketForm::setSocketCaption ( const QString& sSocketCaption )
{
    SocketTabWidget->setTabLabel(SocketTabWidget->page(0), sSocketCaption);
    (PlugListView->header())->setLabel(0, sSocketCaption + " " + tr("Plugs / Ports"), 180);
}

// Socket mode accessor.
void qjackctlSocketForm::setReadable ( bool bReadable )
{
    m_bReadable = bReadable;
}


// Pixmap utility methods.
void qjackctlSocketForm::setSocketPixmaps ( QPixmap *pXpmASocket, QPixmap *pXpmMSocket )
{
    m_pXpmASocket = pXpmASocket;
    m_pXpmMSocket = pXpmMSocket;
}

void qjackctlSocketForm::setPlugPixmaps ( QPixmap *pXpmAPlug, QPixmap *pXpmMPlug )
{
    m_pXpmAPlug = pXpmAPlug;
    m_pXpmMPlug = pXpmMPlug;
}


// JACK client accessor.
void qjackctlSocketForm::setJackClient ( jack_client_t *pJackClient )
{
    m_pJackClient = pJackClient;
}


// ALSA sequencer accessor.
void qjackctlSocketForm::setAlsaSeq ( snd_seq_t *pAlsaSeq )
{
    m_pAlsaSeq = pAlsaSeq;
}


// Load dialog controls from socket properties.
void qjackctlSocketForm::load( qjackctlPatchbaySocket *pSocket )
{
    SocketNameLineEdit->setText(pSocket->name());
    SocketTypeGroup->setButton(pSocket->type());
    ClientNameComboBox->setCurrentText(pSocket->clientName());

    PlugListView->clear();
    QListViewItem *pItem = NULL;
    for (QStringList::Iterator iter = pSocket->pluglist().begin(); iter != pSocket->pluglist().end(); iter++) {
        pItem = new QListViewItem(PlugListView, pItem);
        if (pItem) {
            pItem->setText(0, *iter);
            pItem->setRenameEnabled(0, true);
        }
    }

    socketTypeChanged();
    stabilizeForm();
}


// Save dialog controls into socket properties.
void qjackctlSocketForm::save( qjackctlPatchbaySocket *pSocket )
{
    pSocket->setName(SocketNameLineEdit->text());
    pSocket->setType(SocketTypeGroup->id(SocketTypeGroup->selected()));
    pSocket->setClientName(ClientNameComboBox->currentText());

    pSocket->pluglist().clear();
    for (QListViewItem *pItem = PlugListView->firstChild(); pItem; pItem = pItem->nextSibling())
        pSocket->addPlug(pItem->text(0));
}


// Stabilize current state form.
void qjackctlSocketForm::stabilizeForm()
{
    OkPushButton->setEnabled(validateForm());

    QListViewItem *pItem = PlugListView->selectedItem();
    if (pItem) {
        PlugEditPushButton->setEnabled(true);
        PlugRemovePushButton->setEnabled(true);
        PlugUpPushButton->setEnabled(pItem->itemAbove() != NULL);
        PlugDownPushButton->setEnabled(pItem->nextSibling() != NULL);
    } else {
        PlugEditPushButton->setEnabled(false);
        PlugRemovePushButton->setEnabled(false);
        PlugUpPushButton->setEnabled(false);
        PlugDownPushButton->setEnabled(false);
    }
    PlugAddPushButton->setEnabled(!PlugNameComboBox->currentText().isEmpty());
}


// Validate form fields.
bool qjackctlSocketForm::validateForm()
{
    bool bValid = true;

    bValid = bValid && !SocketNameLineEdit->text().isEmpty();
    bValid = bValid && !ClientNameComboBox->currentText().isEmpty();
    bValid = bValid && (PlugListView->childCount() > 0);

    return bValid;
}

// Validate form fields and accept it valid.
void qjackctlSocketForm::acceptForm()
{
    if (validateForm())
        accept();
}


// Add new Plug to socket list.
void qjackctlSocketForm::addPlug()
{
    QString sPlugName = PlugNameComboBox->currentText();
    if (!sPlugName.isEmpty()) {
        QListViewItem *pItem = PlugListView->selectedItem();
        if (pItem)
            pItem->setSelected(false);
        else
            pItem = PlugListView->lastItem();
        pItem = new QListViewItem(PlugListView, pItem);
        if (pItem) {
            pItem->setText(0, sPlugName);
            pItem->setRenameEnabled(0, true);
            QPixmap *pXpmPlug = NULL;
            switch (SocketTypeGroup->id(SocketTypeGroup->selected())) {
              case 0: // QJACKCTL_SOCKETTYPE_AUDIO
                pXpmPlug = m_pXpmAPlug;
                break;
              case 1: // QJACKCTL_SOCKETTYPE_MIDI
                pXpmPlug = m_pXpmMPlug;
                break;
            }
            if (pXpmPlug)
                pItem->setPixmap(0, *pXpmPlug);
            pItem->setSelected(true);
            PlugListView->setCurrentItem(pItem);
        }
        PlugNameComboBox->setCurrentText(QString::null);
    }

    stabilizeForm();
}


// Rename current selected Plug.
void qjackctlSocketForm::editPlug()
{
    QListViewItem *pItem = PlugListView->selectedItem();
    if (pItem)
        pItem->startRename(0);

    stabilizeForm();
}


// Remove current selected Plug.
void qjackctlSocketForm::removePlug()
{
    QListViewItem *pItem = PlugListView->selectedItem();
    if (pItem)
        delete pItem;

    stabilizeForm();
}


// Move current selected Plug one position up.
void qjackctlSocketForm::moveUpPlug()
{
    QListViewItem *pItem = PlugListView->selectedItem();
    if (pItem) {
        QListViewItem *pItemAbove = pItem->itemAbove();
        if (pItemAbove) {
            pItem->setSelected(false);
            pItemAbove = pItemAbove->itemAbove();
            if (pItemAbove) {
                pItem->moveItem(pItemAbove);
            } else {
                PlugListView->takeItem(pItem);
                PlugListView->insertItem(pItem);
            }
            pItem->setSelected(true);
            PlugListView->setCurrentItem(pItem);
        }
    }

    stabilizeForm();
}


// Move current selected Plug one position down
void qjackctlSocketForm::moveDownPlug()
{
    QListViewItem *pItem = PlugListView->selectedItem();
    if (pItem) {
        QListViewItem *pItemBelow = pItem->nextSibling();
        if (pItemBelow) {
            pItem->setSelected(false);
            pItem->moveItem(pItemBelow);
            pItem->setSelected(true);
            PlugListView->setCurrentItem(pItem);
        }
    }

    stabilizeForm();
}


// Add new Plug from context menu.
void qjackctlSocketForm::activateAddPlugMenu ( int iItemID )
{
    if (m_pAddPlugMenu == NULL)
        return;
    int iIndex = m_pAddPlugMenu->itemParameter(iItemID);
    if (iIndex >= 0 && iIndex < PlugNameComboBox->count()) {
        PlugNameComboBox->setCurrentItem(iIndex);
        addPlug();
    }
}


// Plug list context menu handler.
void qjackctlSocketForm::contextMenu( QListViewItem *pItem, const QPoint& pos, int )
{
    int iItemID;

    // (Re)build the add plug sub-menu...
    if (m_pAddPlugMenu)
        delete m_pAddPlugMenu;
    m_pAddPlugMenu = new QPopupMenu(this);
    for (int iIndex = 0; iIndex < PlugNameComboBox->count(); iIndex++) {
        iItemID = m_pAddPlugMenu->insertItem(PlugNameComboBox->text(iIndex));
        m_pAddPlugMenu->setItemParameter(iItemID, iIndex);
    }
    QObject::connect(m_pAddPlugMenu, SIGNAL(activated(int)), this, SLOT(activateAddPlugMenu(int)));

    // Build the plug conext menu...
    QPopupMenu* pContextMenu = new QPopupMenu(this);

    bool bEnabled = (pItem != NULL);
    iItemID = pContextMenu->insertItem(tr("Add Plug"), m_pAddPlugMenu);
    iItemID = pContextMenu->insertItem(tr("Remove"), this, SLOT(removePlug()));
    pContextMenu->setItemEnabled(iItemID, bEnabled);
    iItemID = pContextMenu->insertItem(tr("Edit"), this, SLOT(editPlug()));
    pContextMenu->setItemEnabled(iItemID, bEnabled);
    pContextMenu->insertSeparator();
    iItemID = pContextMenu->insertItem(tr("Move Up"), this, SLOT(moveUpPlug()));
    pContextMenu->setItemEnabled(iItemID, (bEnabled && pItem->itemAbove() != NULL));
    iItemID = pContextMenu->insertItem(tr("Move Down"), this, SLOT(moveDownPlug()));
    pContextMenu->setItemEnabled(iItemID, (bEnabled && pItem->nextSibling() != NULL));

    pContextMenu->exec(pos);
}


// Socket type change slot.
void qjackctlSocketForm::socketTypeChanged()
{
    QString sOldClientName = ClientNameComboBox->currentText();

    ClientNameComboBox->clear();

    QPixmap *pXpmPlug = NULL;

    switch (SocketTypeGroup->id(SocketTypeGroup->selected())) {
      case 0: // QJACKCTL_SOCKETTYPE_AUDIO
        SocketTabWidget->setTabIconSet(SocketTabWidget->page(0), QIconSet(*m_pXpmASocket));
        pXpmPlug = m_pXpmAPlug;
        if (m_pJackClient) {
            // Grab all client ports.
            const char **ppszClientPorts = jack_get_ports(m_pJackClient, 0, 0, (m_bReadable ? JackPortIsOutput : JackPortIsInput));
            if (ppszClientPorts) {
                int iClientPort = 0;
                while (ppszClientPorts[iClientPort]) {
                    QString sClientPort = ppszClientPorts[iClientPort];
                    int iColon = sClientPort.find(":");
                    if (iColon >= 0) {
                        QString sClientName = sClientPort.left(iColon);
                        bool bExists = false;
                        for (int i = 0; i < ClientNameComboBox->count() && !bExists; i++)
                            bExists = (sClientName == ClientNameComboBox->text(i));
                        if (!bExists)
                            ClientNameComboBox->insertItem(sClientName);
                    }
                    iClientPort++;
                }
                ::free(ppszClientPorts);
            }
        }
        break;
      case 1: // QJACKCTL_SOCKETTYPE_MIDI
        SocketTabWidget->setTabIconSet(SocketTabWidget->page(0), QIconSet(*m_pXpmMSocket));
        pXpmPlug = m_pXpmMPlug;
        if (m_pAlsaSeq) {
            // Readd all subscribers...
            snd_seq_client_info_t *pClientInfo;
            snd_seq_port_info_t   *pPortInfo;
            unsigned int uiAlsaFlags;
            if (m_bReadable)
                uiAlsaFlags = SND_SEQ_PORT_CAP_READ  | SND_SEQ_PORT_CAP_SUBS_READ;
            else
                uiAlsaFlags = SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;
            snd_seq_client_info_alloca(&pClientInfo);
            snd_seq_port_info_alloca(&pPortInfo);
            snd_seq_client_info_set_client(pClientInfo, -1);
            while (snd_seq_query_next_client(m_pAlsaSeq, pClientInfo) >= 0) {
                int iAlsaClient = snd_seq_client_info_get_client(pClientInfo);
                QString sClient = snd_seq_client_info_get_name(pClientInfo);
                if (iAlsaClient > 0) {
                    bool bExists = false;
                    snd_seq_port_info_set_client(pPortInfo, iAlsaClient);
                    snd_seq_port_info_set_port(pPortInfo, -1);
                    while (!bExists && snd_seq_query_next_port(m_pAlsaSeq, pPortInfo) >= 0) {
                        unsigned int uiPortCapability = snd_seq_port_info_get_capability(pPortInfo);
                        if (((uiPortCapability & uiAlsaFlags) == uiAlsaFlags) &&
                            ((uiPortCapability & SND_SEQ_PORT_CAP_NO_EXPORT) == 0)) {
                            for (int i = 0; i < ClientNameComboBox->count() && !bExists; i++)
                                bExists = (sClient == ClientNameComboBox->text(i));
                            if (!bExists) {
                                ClientNameComboBox->insertItem(sClient);
                                bExists = true;
                            }
                        }
                    }
                }
            }
        }
        break;
    }

    ClientNameComboBox->setCurrentText(sOldClientName);
    clientNameChanged();

    if (pXpmPlug) {
        QListViewItem *pItem = PlugListView->firstChild();
        while (pItem) {
            pItem->setPixmap(0, *pXpmPlug);
            pItem = pItem->nextSibling();
        }
    }
}


// Update client list if available.
void qjackctlSocketForm::clientNameChanged()
{
    QString sClientName = ClientNameComboBox->currentText();

    PlugNameComboBox->clear();

    if (sClientName.isEmpty())
        return;

    switch (SocketTypeGroup->id(SocketTypeGroup->selected())) {
      case 0: // QJACKCTL_SOCKETTYPE_AUDIO
        if (m_pJackClient) {
            const char **ppszClientPorts = jack_get_ports(m_pJackClient, 0, 0, (m_bReadable ? JackPortIsOutput : JackPortIsInput));
            if (ppszClientPorts) {
                int iClientPort = 0;
                while (ppszClientPorts[iClientPort]) {
                    QString sClientPort = ppszClientPorts[iClientPort];
                    int iColon = sClientPort.find(":");
                    if (iColon >= 0 && sClientName == sClientPort.left(iColon))
                        PlugNameComboBox->insertItem(sClientPort.right(sClientPort.length() - iColon - 1));
                    iClientPort++;
                }
                ::free(ppszClientPorts);
            }
        }
        break;
      case 1: // QJACKCTL_SOCKETTYPE_MIDI
        if (m_pAlsaSeq) {
            // Fill sequencer plugs...
            snd_seq_client_info_t *pClientInfo;
            snd_seq_port_info_t   *pPortInfo;
            unsigned int uiAlsaFlags;
            if (m_bReadable)
                uiAlsaFlags = SND_SEQ_PORT_CAP_READ  | SND_SEQ_PORT_CAP_SUBS_READ;
            else
                uiAlsaFlags = SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;
            snd_seq_client_info_alloca(&pClientInfo);
            snd_seq_port_info_alloca(&pPortInfo);
            snd_seq_client_info_set_client(pClientInfo, -1);
            while (snd_seq_query_next_client(m_pAlsaSeq, pClientInfo) >= 0) {
                int iAlsaClient = snd_seq_client_info_get_client(pClientInfo);
                QString sClient = snd_seq_client_info_get_name(pClientInfo);
                if (iAlsaClient > 0 && sClient == sClientName) {
                    snd_seq_port_info_set_client(pPortInfo, iAlsaClient);
                    snd_seq_port_info_set_port(pPortInfo, -1);
                    while (snd_seq_query_next_port(m_pAlsaSeq, pPortInfo) >= 0) {
                        unsigned int uiPortCapability = snd_seq_port_info_get_capability(pPortInfo);
                        if (((uiPortCapability & uiAlsaFlags) == uiAlsaFlags) &&
                            ((uiPortCapability & SND_SEQ_PORT_CAP_NO_EXPORT) == 0)) {
                            QString sPort = snd_seq_port_info_get_name(pPortInfo);
                            PlugNameComboBox->insertItem(sPort);
                        }
                    }
                }
            }
        }
        break;
    }

    stabilizeForm();
}


// end of qjackctlSocketForm.ui.h
