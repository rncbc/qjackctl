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
    m_pJackClient  = NULL;
    m_pJackConnect = NULL;

    m_pAlsaSeq     = NULL;
    m_pAlsaConnect = NULL;

    // Connect it to some UI feedback slots.
    QObject::connect(JackConnectView->OListView(), SIGNAL(selectionChanged()), this, SLOT(jackStabilize()));
    QObject::connect(JackConnectView->IListView(), SIGNAL(selectionChanged()), this, SLOT(jackStabilize()));

    QObject::connect(AlsaConnectView->OListView(), SIGNAL(selectionChanged()), this, SLOT(alsaStabilize()));
    QObject::connect(AlsaConnectView->IListView(), SIGNAL(selectionChanged()), this, SLOT(alsaStabilize()));
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

    jackRefresh();
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


// Connections view font accessors.
QFont qjackctlConnectionsForm::connectionsFont (void)
{
    // Elect one list view to retrieve current font.
    return JackConnectView->OListView()->font();
}

void qjackctlConnectionsForm::setConnectionsFont ( const QFont & font )
{
    // Set fonts of all listviews...
    JackConnectView->OListView()->setFont(font);
    JackConnectView->IListView()->setFont(font);
    AlsaConnectView->OListView()->setFont(font);
    AlsaConnectView->IListView()->setFont(font);
}


// Connections view icon size accessor.
void qjackctlConnectionsForm::setConnectionsIconSize( int iIconSize )
{
    // Set icon sizes of all views...
    JackConnectView->setIconSize(iIconSize);
    AlsaConnectView->setIconSize(iIconSize);
}


// (Un)Bind a JACK client to this form.
void qjackctlConnectionsForm::setJackClient ( jack_client_t *pJackClient )
{
    m_pJackClient = pJackClient;

    if (pJackClient == NULL && m_pJackConnect) {
        delete m_pJackConnect;
        m_pJackConnect = NULL;
    }
    
    if (pJackClient && m_pJackConnect == NULL) {
        m_pJackConnect = new qjackctlJackConnect(JackConnectView, pJackClient);
        if (m_pJackConnect) {
            qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
            if (pMainForm)
                QObject::connect(m_pJackConnect, SIGNAL(connectChanged()), pMainForm, SLOT(jackConnectChanged()));
        }
    }

    stabilizeJack(pJackClient != NULL);
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



// Connect current selected ports.
void qjackctlConnectionsForm::jackConnectSelected (void)
{
    if (m_pJackConnect) {
        if (m_pJackConnect->connectSelected())
            refreshJack(false);
    }
}


// Disconnect current selected ports.
void qjackctlConnectionsForm::jackDisconnectSelected (void)
{
    if (m_pJackConnect) {
        if (m_pJackConnect->disconnectSelected())
            refreshJack(false);
    }
}


// Disconnect all connected ports.
void qjackctlConnectionsForm::jackDisconnectAll()
{
    if (m_pJackConnect) {
        if (m_pJackConnect->disconnectAll())
            refreshJack(false);
    }
}


// Refresh complete form by notifying the parent form.
void qjackctlConnectionsForm::jackRefresh ( void )
{
    refreshJack(false);
    refreshAlsa(false);
}


// A helper stabilization slot.
void qjackctlConnectionsForm::jackStabilize ( void )
{
    stabilizeJack(true);
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
    refreshJack(false);
}


// A helper stabilization slot.
void qjackctlConnectionsForm::alsaStabilize ( void )
{
    stabilizeAlsa(true);
}


// Either rebuild all connections now or notify main form for doing that later.
void qjackctlConnectionsForm::refreshJack ( bool bEnabled )
{
    if (m_pJackConnect == NULL)
        return;
        
    if (bEnabled) {
        m_pJackConnect->refresh();
        stabilizeJack(true);
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
void qjackctlConnectionsForm::stabilizeJack ( bool bEnabled )
{
    if (m_pJackConnect && bEnabled) {
        JackConnectPushButton->setEnabled(m_pJackConnect->canConnectSelected());
        JackDisconnectPushButton->setEnabled(m_pJackConnect->canDisconnectSelected());
        JackDisconnectAllPushButton->setEnabled(m_pJackConnect->canDisconnectAll());
        JackRefreshPushButton->setEnabled(true);
    } else {
        JackConnectPushButton->setEnabled(false);
        JackDisconnectPushButton->setEnabled(false);
        JackDisconnectAllPushButton->setEnabled(false);
        JackRefreshPushButton->setEnabled(false);
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


// end of qjackctlConnectionsForm.ui.h
