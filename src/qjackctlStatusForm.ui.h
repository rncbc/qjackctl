// qjackctlStatusForm.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
/****************************************************************************
   Copyright (C) 2003-2005, rncbc aka Rui Nuno Capela. All rights reserved.

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

// Custom tooltip class.
class qjackctlStatusToolTip : public QToolTip
{
public:

	// Constructor.
	qjackctlStatusToolTip(QListView *pListView)
		: QToolTip(pListView->viewport()) { m_pListView = pListView; }

protected:

	// Tooltip handler.
	void maybeTip(const QPoint& pos) 
	{
		QListViewItem *pItem = m_pListView->itemAt(pos);
		if (pItem) {
			QRect rect(m_pListView->itemRect(pItem));
			if (rect.isValid())
				QToolTip::tip(rect, pItem->text(0).remove(':'));
		}
	}

private:

	// The actual parent widget holder.
	QListView *m_pListView;
};


// Kind of constructor.
void qjackctlStatusForm::init (void)
{
    // Create the list view items 'a priori'...
    QString s = " ";
    QString c = ":" + s;
    QString n = "--";
    QListViewItem *pViewItem;

    StatsListView->setSorting(3); // Initially unsorted.

    m_apStatus[STATUS_RESET_TIME] = new QListViewItem(StatsListView, s + tr("Time of last reset") + c, n);
#ifdef CONFIG_JACK_MAX_DELAY
    m_apStatus[STATUS_MAX_DELAY]  = new QListViewItem(StatsListView, s + tr("Maximum scheduling delay") + c, n);
#endif
    pViewItem = new QListViewItem(StatsListView, s + tr("XRUN count since last server startup") + c, n);
    m_apStatus[STATUS_XRUN_COUNT] = pViewItem;
    m_apStatus[STATUS_XRUN_TOTAL] = new QListViewItem(pViewItem, s + tr("XRUN total") + c, n);
    m_apStatus[STATUS_XRUN_AVG]   = new QListViewItem(pViewItem, s + tr("XRUN average") + c, n);
    m_apStatus[STATUS_XRUN_MIN]   = new QListViewItem(pViewItem, s + tr("XRUN minimum") + c, n);
    m_apStatus[STATUS_XRUN_MAX]   = new QListViewItem(pViewItem, s + tr("XRUN maximum") + c, n);
    m_apStatus[STATUS_XRUN_LAST]  = new QListViewItem(pViewItem, s + tr("XRUN last") + c, n);
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

	// Create the tooltip handler...
	m_pToolTip = (QToolTip *) new qjackctlStatusToolTip(StatsListView);
}


// Kind of destructor.
void qjackctlStatusForm::destroy (void)
{
	delete (qjackctlStatusToolTip *) m_pToolTip;
}


// Notify our parent that we're emerging.
void qjackctlStatusForm::showEvent ( QShowEvent *pShowEvent )
{
    qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
    if (pMainForm)
        pMainForm->stabilizeForm();

    QWidget::showEvent(pShowEvent);
}

// Notify our parent that we're closing.
void qjackctlStatusForm::hideEvent ( QHideEvent *pHideEvent )
{
    QWidget::hideEvent(pHideEvent);

    qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
    if (pMainForm)
        pMainForm->stabilizeForm();
}


// Ask our parent to reset status.
void qjackctlStatusForm::resetXrunStats (void)
{
    qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
    if (pMainForm)
        pMainForm->resetXrunStats();
}

// Ask our parent to refresh our status.
void qjackctlStatusForm::refreshXrunStats (void)
{
    qjackctlMainForm *pMainForm = (qjackctlMainForm *) QWidget::parentWidget();
    if (pMainForm)
        pMainForm->refreshXrunStats();
}


// Update one status item value.
void qjackctlStatusForm::updateStatusItem ( int iStatusItem, const QString& sText )
{
    m_apStatus[iStatusItem]->setText(1, sText);
}


// end of qjackctlStatusForm.ui.h
