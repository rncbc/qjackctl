// qjackctlStatusForm.ui.h
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

#include "qjackctlStatus.h"
#include "qjackctlSetup.h"


// Kind of constructor.
void qjackctlStatusForm::init (void)
{
    // Create the list view items 'a priori'...
    QString s = " ";
    QString c = ":" + s;
    QString z = "0";
    QString n = "--";
    QListViewItem *pViewItem;

    StatsListView->setSorting(3); // Initially unsorted.

    m_apStatus[STATUS_RESET_TIME] = new QListViewItem(StatsListView, s + tr("Time of last reset") + c, z);

    pViewItem = new QListViewItem(StatsListView, s + tr("XRUN count since last server startup") + c, z);
    m_apStatus[STATUS_XRUN_COUNT] = pViewItem;
    m_apStatus[STATUS_XRUN_TOTAL] = new QListViewItem(pViewItem, s + tr("XRUN total") + c, z);
    m_apStatus[STATUS_XRUN_AVG]   = new QListViewItem(pViewItem, s + tr("XRUN average") + c, z);
    m_apStatus[STATUS_XRUN_MIN]   = new QListViewItem(pViewItem, s + tr("XRUN minimum") + c, z);
    m_apStatus[STATUS_XRUN_MAX]   = new QListViewItem(pViewItem, s + tr("XRUN maximum") + c, z);
    m_apStatus[STATUS_XRUN_LAST]  = new QListViewItem(pViewItem, s + tr("XRUN last") + c, z);
    m_apStatus[STATUS_XRUN_TIME]  = new QListViewItem(pViewItem, s + tr("XRUN last time detected") + c, n);

    pViewItem = new QListViewItem(StatsListView, s + tr("Transport state") + c, n);
    m_apStatus[STATUS_TRANSPORT_STATE] = pViewItem;
    m_apStatus[STATUS_TRANSPORT_BPM]  = new QListViewItem(pViewItem, s + tr("Transport BPM") + c, n);
    m_apStatus[STATUS_TRANSPORT_BBT]  = new QListViewItem(pViewItem, s + tr("Transport BBT") + c, n);
    m_apStatus[STATUS_TRANSPORT_TIME] = new QListViewItem(pViewItem, s + tr("Transport Timecode") + c, n);

    m_apStatus[STATUS_REALTIME]     = new QListViewItem(StatsListView, s + tr("Realtime Mode") + c, n);
    m_apStatus[STATUS_BUFFER_SIZE]  = new QListViewItem(StatsListView, s + tr("Buffer Size") + c, n);
    m_apStatus[STATUS_SAMPLE_RATE]  = new QListViewItem(StatsListView, s + tr("Sample Rate") + c, n);
    m_apStatus[STATUS_CPU_LOAD]     = new QListViewItem(StatsListView, s + tr("CPU Load") + c, n);
    m_apStatus[STATUS_SERVER_STATE] = new QListViewItem(StatsListView, s + tr("Server state") + c, n);
}


// Kind of destructor.
void qjackctlStatusForm::destroy (void)
{
}


// Notify our parent that we're emerging.
void qjackctlStatusForm::showEvent ( QShowEvent *pShowEvent )
{
    qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parent();
    if (pMainForm)
        pMainForm->stabilizeForm();

    QWidget::showEvent(pShowEvent);
}

// Notify our parent that we're closing.
void qjackctlStatusForm::hideEvent ( QHideEvent *pHideEvent )
{
    QWidget::hideEvent(pHideEvent);

    qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parent();
    if (pMainForm)
        pMainForm->stabilizeForm();
}


// Ask our parent to reset status.
void qjackctlStatusForm::resetXrunStats (void)
{
    qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parent();
    if (pMainForm)
        pMainForm->resetXrunStats();
}

// Ask our parent to refresh our status.
void qjackctlStatusForm::refreshXrunStats (void)
{
    qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parent();
    if (pMainForm)
        pMainForm->refreshXrunStats();
}


// Update one status item value.
void qjackctlStatusForm::updateStatus ( int iStatusItem, const QString& sText )
{
    m_apStatus[iStatusItem]->setText(1, sText);
}


// end of qjackctlStatusForm.ui.h
