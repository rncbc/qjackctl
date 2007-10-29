// qjackctlStatusForm.cpp
//
/****************************************************************************
   Copyright (C) 2003-2007, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include "qjackctlStatusForm.h"

#include "qjackctlStatus.h"
#include "qjackctlSetup.h"

#include "qjackctlMainForm.h"

#include <QHeaderView>

#include <QShowEvent>
#include <QHideEvent>


//----------------------------------------------------------------------------
// qjackctlStatusForm -- UI wrapper form.

// Constructor.
qjackctlStatusForm::qjackctlStatusForm (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QWidget(pParent, wflags)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	// Create the list view items 'a priori'...
	QString s = " ";
	QString c = ":" + s;
	QString n = "--";
	QTreeWidgetItem *pViewItem;

	// Status list view...
	QHeaderView *pHeader = m_ui.StatsListView->header();
//	pHeader->setResizeMode(QHeaderView::Custom);
	pHeader->setDefaultAlignment(Qt::AlignLeft);
//	pHeader->setDefaultSectionSize(320);
	pHeader->setMovable(false);
	pHeader->setStretchLastSection(true);

	m_apStatus[STATUS_SERVER_STATE] = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("Server state") + c << n);
	m_apStatus[STATUS_DSP_LOAD]     = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("DSP Load") + c << n);
	m_apStatus[STATUS_SAMPLE_RATE]  = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("Sample Rate") + c << n);
	m_apStatus[STATUS_BUFFER_SIZE]  = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("Buffer Size") + c << n);
	m_apStatus[STATUS_REALTIME]     = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("Realtime Mode") + c << n);

	pViewItem = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("Transport state") + c << n);
	m_apStatus[STATUS_TRANSPORT_STATE] = pViewItem;
	m_apStatus[STATUS_TRANSPORT_TIME] = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("Transport Timecode") + c << n);
	m_apStatus[STATUS_TRANSPORT_BBT]  = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("Transport BBT") + c << n);
	m_apStatus[STATUS_TRANSPORT_BPM]  = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("Transport BPM") + c << n);

	pViewItem = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("XRUN count since last server startup") + c << n);
	m_apStatus[STATUS_XRUN_COUNT] = pViewItem;
	m_apStatus[STATUS_XRUN_TIME]  = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("XRUN last time detected") + c << n);
	m_apStatus[STATUS_XRUN_LAST]  = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("XRUN last") + c << n);
	m_apStatus[STATUS_XRUN_MAX]   = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("XRUN maximum") + c << n);
	m_apStatus[STATUS_XRUN_MIN]   = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("XRUN minimum") + c << n);
	m_apStatus[STATUS_XRUN_AVG]   = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("XRUN average") + c << n);
	m_apStatus[STATUS_XRUN_TOTAL] = new QTreeWidgetItem(pViewItem,
		QStringList() << s + tr("XRUN total") + c << n);

#ifdef CONFIG_JACK_MAX_DELAY
	m_apStatus[STATUS_MAX_DELAY]  = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("Maximum scheduling delay") + c << n);
#endif
	m_apStatus[STATUS_RESET_TIME] = new QTreeWidgetItem(m_ui.StatsListView,
		QStringList() << s + tr("Time of last reset") + c << n);

	m_ui.StatsListView->resizeColumnToContents(0);	// Description.
	m_ui.StatsListView->resizeColumnToContents(1);	// Value.

	// UI connections...

	QObject::connect(m_ui.ResetPushButton,
		SIGNAL(clicked()),
		SLOT(resetXrunStats()));
	QObject::connect(m_ui.RefreshPushButton,
		SIGNAL(clicked()),
		SLOT(refreshXrunStats()));
}


// Destructor.
qjackctlStatusForm::~qjackctlStatusForm (void)
{
}


// Notify our parent that we're emerging.
void qjackctlStatusForm::showEvent ( QShowEvent *pShowEvent )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();

	QWidget::showEvent(pShowEvent);
}

// Notify our parent that we're closing.
void qjackctlStatusForm::hideEvent ( QHideEvent *pHideEvent )
{
	QWidget::hideEvent(pHideEvent);

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}

// Just about to notify main-window that we're closing.
void qjackctlStatusForm::closeEvent ( QCloseEvent * /*pCloseEvent*/ )
{
	QWidget::hide();

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}


// Ask our parent to reset status.
void qjackctlStatusForm::resetXrunStats (void)
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->resetXrunStats();
}

// Ask our parent to refresh our status.
void qjackctlStatusForm::refreshXrunStats (void)
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->refreshXrunStats();
}


// Update one status item value.
void qjackctlStatusForm::updateStatusItem ( int iStatusItem,
	const QString& sText )
{
	m_apStatus[iStatusItem]->setText(1, sText);
}


// end of qjackctlStatusForm.cpp
