// qjackctlConnectionsForm.ui.h
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

#include "config.h"

#include "qjackctlSetup.h"


// Kind of constructor.
void qjackctlConnectionsForm::init (void)
{
    m_pJackClient = NULL;
    m_pJackConnections = NULL;

    // Connect it to some UI feedback slot.
    QObject::connect(ConnectionsView->OListView(), SIGNAL(selectionChanged()), this, SLOT(stabilizeForm()));
    QObject::connect(ConnectionsView->IListView(), SIGNAL(selectionChanged()), this, SLOT(stabilizeForm()));
}


// Kind of destructor.
void qjackctlConnectionsForm::destroy (void)
{
    // Destroy our connections view...
    setJackClient(NULL);
}


// Notify our parent that we're emerging.
void qjackctlConnectionsForm::showEvent ( QShowEvent *pShowEvent )
{
    qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parent();
    if (pMainForm)
        pMainForm->stabilizeForm();

    refreshForm();
        
    QWidget::showEvent(pShowEvent);
}

// Notify our parent that we're closing.
void qjackctlConnectionsForm::hideEvent ( QHideEvent *pHideEvent )
{
    QWidget::hideEvent(pHideEvent);

    qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parent();
    if (pMainForm)
        pMainForm->stabilizeForm();
}


// (Un)Bind a JACK client to this form.
void qjackctlConnectionsForm::setJackClient ( jack_client_t *pJackClient )
{
    m_pJackClient = pJackClient;

    if (pJackClient == NULL && m_pJackConnections) {
        delete m_pJackConnections;
        m_pJackConnections = NULL;
    }
    
    if (pJackClient && m_pJackConnections == NULL)
        m_pJackConnections = new qjackctlConnections(ConnectionsView, pJackClient);
        
    stabilize(pJackClient != NULL);
}


// Connect current selected ports.
void qjackctlConnectionsForm::connectSelected (void)
{
    if (m_pJackConnections) {
        if (m_pJackConnections->connectSelected())
            refresh(false);
    }
}


// Disconnect current selected ports.
void qjackctlConnectionsForm::disconnectSelected (void)
{
    if (m_pJackConnections) {
        if (m_pJackConnections->disconnectSelected())
            refresh(false);
    }
}


// Disconnect all connected ports.
void qjackctlConnectionsForm::disconnectAll()
{
    if (m_pJackConnections) {
        if (m_pJackConnections->disconnectAll())
            refresh(false);
    }
}


// Refresh complete form by notifying the parent form.
void qjackctlConnectionsForm::refreshForm ( void )
{
    refresh(false);
}


// A helper stabilization slot.
void qjackctlConnectionsForm::stabilizeForm ( void )
{
    stabilize(true);
}


// Either rebuild all connections now or notify main form for doing that later.
void qjackctlConnectionsForm::refresh ( bool bEnabled )
{
    if (m_pJackConnections && bEnabled) {
        m_pJackConnections->refresh();
        stabilize(true);
    } else {
        qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parent();
        if (pMainForm)
            pMainForm->refreshConnections();
    }
}


// Proper enablement of connections command controls.
void qjackctlConnectionsForm::stabilize ( bool bEnabled )
{
    if (m_pJackConnections && bEnabled) {
        ConnectPushButton->setEnabled(m_pJackConnections->canConnectSelected());
        DisconnectPushButton->setEnabled(m_pJackConnections->canDisconnectSelected());
        DisconnectAllPushButton->setEnabled(m_pJackConnections->canDisconnectAll());
        RefreshPushButton->setEnabled(true);
    } else {
        ConnectPushButton->setEnabled(false);
        DisconnectPushButton->setEnabled(false);
        DisconnectAllPushButton->setEnabled(false);
        RefreshPushButton->setEnabled(false);
    }
}


// end of qjackctlConnectionsForm.ui.h