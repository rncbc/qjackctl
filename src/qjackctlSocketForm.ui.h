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
    m_pJackClient = NULL;
    m_ulSocketFlags = 0;
    m_pXpmPlug = NULL;
}


// Kind of destructor.
void qjackctlSocketForm::destroy()
{

}


// Socket caption utility method.
void qjackctlSocketForm::setSocketCaption ( const QString& sSocketCaption )
{
    SocketTabWidget->setTabLabel(SocketTabWidget->page(0), sSocketCaption);
}


// Pixmap utility methods.
void qjackctlSocketForm::setSocketPixmap ( QPixmap *pXpmSocket )
{
    SocketTabWidget->setTabIconSet(SocketTabWidget->page(0), QIconSet(*pXpmSocket));
}

void qjackctlSocketForm::setPlugPixmap ( QPixmap *pXpmPlug )
{
    m_pXpmPlug = pXpmPlug;
}


// JACK client accessor.
void qjackctlSocketForm::setJackClient ( jack_client_t *pJackClient, unsigned long ulSocketFlags )
{
    m_pJackClient = pJackClient;
    m_ulSocketFlags = ulSocketFlags;

    ClientNameComboBox->clear();

    if (m_pJackClient == NULL)
        return;
        
    const char **ppszClientPorts = jack_get_ports(m_pJackClient, 0, 0, m_ulSocketFlags);
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
    
    clientNameChanged();
}


// Load dialog controls from socket properties.
void qjackctlSocketForm::load( qjackctlPatchbaySocket *pSocket )
{
    SocketNameLineEdit->setText(pSocket->name());
    ClientNameComboBox->setCurrentText(pSocket->clientName());
    
    PlugListBox->clear();
    for (QStringList::Iterator iter = pSocket->pluglist().begin(); iter != pSocket->pluglist().end(); iter++) {
        if (m_pXpmPlug) {
            PlugListBox->insertItem(*m_pXpmPlug, *iter);
        } else {
            PlugListBox->insertItem(*iter);
        }
    }

    stabilizeForm();
}


// Save dialog controls into socket properties.
void qjackctlSocketForm::save( qjackctlPatchbaySocket *pSocket )
{
    pSocket->setName(SocketNameLineEdit->text());
    pSocket->setClientName(ClientNameComboBox->currentText());
    
    pSocket->pluglist().clear();
    for (int i = 0; i < (int) PlugListBox->count(); i++) {
        QListBoxItem *pItem = PlugListBox->item(i);
        if (pItem)
            pSocket->addPlug(pItem->text());
    }    
}


// Stabilize current state form.
void qjackctlSocketForm::stabilizeForm()
{
    OkPushButton->setEnabled(validateForm());
    
    QListBoxItem *pItem = PlugListBox->selectedItem();
    if (pItem) {
        PlugRemovePushButton->setEnabled(true);
        PlugUpPushButton->setEnabled(pItem->prev() != NULL);
        PlugDownPushButton->setEnabled(pItem->next() != NULL);
    } else {
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
    bValid = bValid && (PlugListBox->count() > 0);
    
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
        if (m_pXpmPlug)
            PlugListBox->insertItem(*m_pXpmPlug, sPlugName);
        else
            PlugListBox->insertItem(sPlugName);
        PlugNameComboBox->setCurrentText(QString::null);
    }
}


// Remove current selected Plug.
void qjackctlSocketForm::removePlug()
{
    QListBoxItem *pItem = PlugListBox->selectedItem();
    if (pItem)
        PlugListBox->removeItem(PlugListBox->index(pItem));
}


// Move current selected Plug one position up.
void qjackctlSocketForm::moveUpPlug()
{
    QListBoxItem *pItem = PlugListBox->selectedItem();
    if (pItem) {
        int iItem = PlugListBox->index(pItem);
        if (iItem > 0) {
            PlugListBox->setSelected(pItem, false);
        //  PlugListBox->setUpdatesEnabled(false);
            PlugListBox->takeItem(pItem);
            PlugListBox->insertItem(pItem, iItem - 1);
        //  PlugListBox->setUpdatesEnabled(true);
        //  PlugListBox->triggerUpdate(false);
            PlugListBox->setSelected(pItem, true);
        }
    }
}


// Move current selected Plug one position down
void qjackctlSocketForm::moveDownPlug()
{
    QListBoxItem *pItem = PlugListBox->selectedItem();
    if (pItem) {
        int iItem = PlugListBox->index(pItem);
        if (iItem < (int) PlugListBox->count() - 1) {
            PlugListBox->setSelected(pItem, false);
        //  PlugListBox->setUpdatesEnabled(false);
            PlugListBox->takeItem(pItem);
            PlugListBox->insertItem(pItem, iItem + 1);
        //  PlugListBox->setUpdatesEnabled(true);
        //  PlugListBox->triggerUpdate(false);
            PlugListBox->setSelected(pItem, true);
        }
    }
}


// Update client list if available.
void qjackctlSocketForm::clientNameChanged()
{
    QString sClientName = ClientNameComboBox->currentText();
    
    PlugNameComboBox->clear();
    
    if (m_pJackClient == NULL)
        return;

    if (!sClientName.isEmpty()) {
        const char **ppszClientPorts = jack_get_ports(m_pJackClient, 0, 0, m_ulSocketFlags);
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

    stabilizeForm();
}


// end of qjackctlSocketForm.ui.h
