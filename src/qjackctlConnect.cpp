// qjackctlConnect.cpp
//
/****************************************************************************
   Copyright (C) 2003-2015, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include "qjackctlConnect.h"

#include <QApplication>
#include <QMessageBox>
#include <QHeaderView>
#include <QScrollBar>
#include <QToolTip>
#include <QPainter>
#include <QPolygon>
#include <QPainterPath>
#include <QTimer>
#include <QMenu>

#include <QDragEnterEvent>
#include <QDragMoveEvent>

#if QT_VERSION >= 0x050000
#include <QMimeData>
#include <QDrag>
#endif


//----------------------------------------------------------------------
// class qjackctlPortItem -- Port list item.
//

// Constructor.
qjackctlPortItem::qjackctlPortItem ( qjackctlClientItem *pClient )
	: QTreeWidgetItem(pClient, QJACKCTL_PORTITEM)
{
	m_pClient   = pClient;
//	m_sPortName = sPortName;
	m_iPortMark = 0;
	m_bHilite   = false;

	m_pClient->ports().append(this);
}


// Default destructor.
qjackctlPortItem::~qjackctlPortItem (void)
{
	const int iPort = m_pClient->ports().indexOf(this);
	if (iPort >= 0)
		m_pClient->ports().removeAt(iPort);

	QListIterator<qjackctlPortItem *> iter(m_connects);
	while (iter.hasNext())
		(iter.next())->removeConnect(this);

	m_connects.clear();
}


// Instance accessors.
void qjackctlPortItem::setPortName ( const QString& sPortName )
{
	m_sPortName = sPortName;

	updatePortName();
}

const QString& qjackctlPortItem::clientName (void) const
{
	return m_pClient->clientName();
}

const QString& qjackctlPortItem::portName (void) const
{
	return m_sPortName;
}


// Port name alias accessors.
void qjackctlPortItem::setPortNameAlias ( const QString& sPortNameAlias )
{
	// Check aliasing...
	qjackctlClientListView *pClientListView
		= (m_pClient->clientList())->listView();
	qjackctlConnectAlias *pAliases
		= pClientListView->aliases();
	if (pAliases) {
		const QString& sClientName = m_pClient->clientName();
		pAliases->setPortAlias(sClientName, m_sPortName, sPortNameAlias);
		pClientListView->setDirty(true);
	}
}

QString qjackctlPortItem::portNameAlias ( bool *pbRenameEnabled ) const
{
	QString sPortNameAlias = m_sPortName;
	bool bRenameEnabled = false;

	// Check aliasing...
	qjackctlClientListView *pClientListView
		= (m_pClient->clientList())->listView();
	qjackctlConnectAlias *pAliases
		= pClientListView->aliases();
	if (pAliases) {
		const QString& sClientName = m_pClient->clientName();
		sPortNameAlias = pAliases->portAlias(sClientName, m_sPortName);
		bRenameEnabled = pClientListView->isRenameEnabled();
	}

	if (pbRenameEnabled)
		*pbRenameEnabled = bRenameEnabled;

	return sPortNameAlias;
}


// Proto-pretty/alias display name method.
void qjackctlPortItem::updatePortName ( bool /*bRename*/ )
{
	bool bRenameEnabled = false;
	const QString& sPortNameEx = portNameAlias(&bRenameEnabled);
	setPortText(sPortNameEx, bRenameEnabled);
}


// Port display name accessors.
void qjackctlPortItem::setPortText (
	const QString& sPortText, bool bRenameEnabled )
{
	QTreeWidgetItem::setText(0, sPortText);

	const Qt::ItemFlags flags = QTreeWidgetItem::flags();
	if (bRenameEnabled)
		QTreeWidgetItem::setFlags(flags |  Qt::ItemIsEditable);
	else
		QTreeWidgetItem::setFlags(flags	& ~Qt::ItemIsEditable);
}

QString qjackctlPortItem::portText (void) const
{
	return QTreeWidgetItem::text(0);
}


// Complete client:port name helper.
QString qjackctlPortItem::clientPortName (void) const
{
	return m_pClient->clientName() + ':' + m_sPortName;
}


// Connect client item accessor.
qjackctlClientItem *qjackctlPortItem::client (void) const
{
	return m_pClient;
}


// Client:port set housekeeping marker.
void qjackctlPortItem::markPort ( int iMark )
{
	setHilite(false);
	m_iPortMark = iMark;
	if (iMark > 0)
		m_connects.clear();
}

void qjackctlPortItem::markClientPort ( int iMark )
{
	markPort(iMark);

	m_pClient->markClient(iMark);
}

int qjackctlPortItem::portMark (void) const
{
	return m_iPortMark;
}


// Connected port list primitives.
void qjackctlPortItem::addConnect ( qjackctlPortItem *pPort )
{
	m_connects.append(pPort);
}

void qjackctlPortItem::removeConnect ( qjackctlPortItem *pPort )
{
	pPort->setHilite(false);

	const int iPort = m_connects.indexOf(pPort);
	if (iPort >= 0)
		m_connects.removeAt(iPort);
}


// Connected port finder.
qjackctlPortItem *qjackctlPortItem::findConnect ( const QString& sClientPortName )
{
	QListIterator<qjackctlPortItem *> iter(m_connects);
	while (iter.hasNext()) {
		qjackctlPortItem *pPort = iter.next();
		if (sClientPortName == pPort->clientPortName())
			return pPort;
	}

	return NULL;
}

qjackctlPortItem *qjackctlPortItem::findConnectPtr ( qjackctlPortItem *pPortPtr )
{
	QListIterator<qjackctlPortItem *> iter(m_connects);
	while (iter.hasNext()) {
		qjackctlPortItem *pPort = iter.next();
		if (pPortPtr == pPort)
			return pPort;
	}

	return NULL;
}


// Connection cache list accessor.
const QList<qjackctlPortItem *>& qjackctlPortItem::connects (void) const
{
	return m_connects;
}


// Connectiopn highlight methods.
bool qjackctlPortItem::isHilite (void) const
{
	return m_bHilite;
}

void qjackctlPortItem::setHilite ( bool bHilite )
{
	// Update the port highlightning if changed...
	if ((m_bHilite && !bHilite) || (!m_bHilite && bHilite)) {
		m_bHilite = bHilite;
		// Propagate this to the parent...
		m_pClient->setHilite(bHilite);
	}

	// Set the new color.
	QTreeWidget *pTreeWidget = QTreeWidgetItem::treeWidget();
	if (pTreeWidget == NULL)
		return;

	const QPalette& pal = pTreeWidget->palette();
	QTreeWidgetItem::setTextColor(0, m_bHilite
		? (pal.base().color().value() < 0x7f ? Qt::cyan : Qt::blue)
		: pal.text().color());
}


// Proxy sort override method.
// - Natural decimal sorting comparator.
bool qjackctlPortItem::operator< ( const QTreeWidgetItem& other ) const
{
	return qjackctlClientList::lessThan(*this, other);
}


//----------------------------------------------------------------------
// class qjackctlClientItem -- Jack client list item.
//

// Constructor.
qjackctlClientItem::qjackctlClientItem ( qjackctlClientList *pClientList )
	: QTreeWidgetItem(pClientList->listView(), QJACKCTL_CLIENTITEM)
{
	m_pClientList = pClientList;
//	m_sClientName = sClientName;
	m_iClientMark = 0;
	m_iHilite     = 0;

	m_pClientList->clients().append(this);
}

// Default destructor.
qjackctlClientItem::~qjackctlClientItem (void)
{
	qDeleteAll(m_ports);
	m_ports.clear();

	const int iClient = m_pClientList->clients().indexOf(this);
	if (iClient >= 0)
		m_pClientList->clients().removeAt(iClient);
}


// Port finder.
qjackctlPortItem *qjackctlClientItem::findPort (const QString& sPortName)
{
	QListIterator<qjackctlPortItem *> iter(m_ports);
	while (iter.hasNext()) {
		qjackctlPortItem *pPort = iter.next();
		if (sPortName == pPort->portName())
			return pPort;
	}

	return NULL;
}


// Client list accessor.
qjackctlClientList *qjackctlClientItem::clientList (void) const
{
	return m_pClientList;
}


// Port list accessor.
QList<qjackctlPortItem *>& qjackctlClientItem::ports (void)
{
	return m_ports;
}


// Instance accessors.
void qjackctlClientItem::setClientName ( const QString& sClientName )
{
	m_sClientName = sClientName;

	updateClientName();
}

const QString& qjackctlClientItem::clientName (void) const
{
	return m_sClientName;
}


// Client name alias accessors.
void qjackctlClientItem::setClientNameAlias ( const QString& sClientNameAlias )
{
	qjackctlClientListView *pClientListView
		= m_pClientList->listView();
	qjackctlConnectAlias *pAliases
		= pClientListView->aliases();
	if (pAliases) {
		pAliases->setClientAlias(m_sClientName, sClientNameAlias);
		pClientListView->setDirty(true);
	}
}

QString qjackctlClientItem::clientNameAlias ( bool *pbRenameEnabled ) const
{
	QString sClientNameAlias = m_sClientName;
	bool bRenameEnabled = false;

	// Check aliasing...
	qjackctlClientListView *pClientListView
		= m_pClientList->listView();
	qjackctlConnectAlias *pAliases
		= pClientListView->aliases();
	if (pAliases) {
		sClientNameAlias = pAliases->clientAlias(m_sClientName);
		bRenameEnabled = pClientListView->isRenameEnabled();
	}

	if (pbRenameEnabled)
		*pbRenameEnabled = bRenameEnabled;

	return sClientNameAlias;
}


// Proto-pretty/alias display name method.
void qjackctlClientItem::updateClientName ( bool /*bRename*/ )
{
	bool bRenameEnabled = false;
	const QString& sClientNameEx = clientNameAlias(&bRenameEnabled);
	setClientText(sClientNameEx, bRenameEnabled);
}


// Client display name accessors.
void qjackctlClientItem::setClientText (
	const QString& sClientText, bool bRenameEnabled )
{
	QTreeWidgetItem::setText(0, sClientText);

	const Qt::ItemFlags flags = QTreeWidgetItem::flags();
	if (bRenameEnabled)
		QTreeWidgetItem::setFlags(flags |  Qt::ItemIsEditable);
	else
		QTreeWidgetItem::setFlags(flags	& ~Qt::ItemIsEditable);
}

QString qjackctlClientItem::clientText (void) const
{
	return QTreeWidgetItem::text(0);
}


// Readable flag client accessor.
bool qjackctlClientItem::isReadable (void) const
{
	return m_pClientList->isReadable();
}


// Client:port set housekeeping marker.
void qjackctlClientItem::markClient ( int iMark )
{
	setHilite(false);
	m_iClientMark = iMark;
}

void qjackctlClientItem::markClientPorts ( int iMark )
{
	markClient(iMark);

	QListIterator<qjackctlPortItem *> iter(m_ports);
	while (iter.hasNext())
		(iter.next())->markPort(iMark);
}

int qjackctlClientItem::cleanClientPorts ( int iMark )
{
	int iDirtyCount = 0;

	QMutableListIterator<qjackctlPortItem *> iter(m_ports);
	while (iter.hasNext()) {
		qjackctlPortItem *pPort = iter.next();
		if (pPort->portMark() == iMark) {
			iter.remove();
			delete pPort;
			iDirtyCount++;
		}
	}
	
	return iDirtyCount;
}

int qjackctlClientItem::clientMark (void) const
{
	return m_iClientMark;
}


// Connectiopn highlight methods.
bool qjackctlClientItem::isHilite (void) const
{
	return (m_iHilite > 0);
}

void qjackctlClientItem::setHilite ( bool bHilite )
{
	if (bHilite)
		m_iHilite++;
	else
	if (m_iHilite > 0)
		m_iHilite--;

	// Set the new color.
	QTreeWidget *pTreeWidget = QTreeWidgetItem::treeWidget();
	if (pTreeWidget == NULL)
		return;

	const QPalette& pal = pTreeWidget->palette();
	QTreeWidgetItem::setTextColor(0, m_iHilite > 0
		? (pal.base().color().value() < 0x7f ? Qt::darkCyan : Qt::darkBlue)
		: pal.text().color());
}


// Socket item openness status.
void qjackctlClientItem::setOpen ( bool bOpen )
{
	QTreeWidgetItem::setExpanded(bOpen);
}


bool qjackctlClientItem::isOpen (void) const
{
	return QTreeWidgetItem::isExpanded();
}


// Proxy sort override method.
// - Natural decimal sorting comparator.
bool qjackctlClientItem::operator< ( const QTreeWidgetItem& other ) const
{
	return qjackctlClientList::lessThan(*this, other);
}


//----------------------------------------------------------------------
// qjackctlClientList -- Client list.
//

// Constructor.
qjackctlClientList::qjackctlClientList (
	qjackctlClientListView *pListView, bool bReadable )
{
	m_pListView = pListView;
	m_bReadable = bReadable;

	m_pHiliteItem = 0;
}

// Default destructor.
qjackctlClientList::~qjackctlClientList (void)
{
	clear();
}


// Do proper contents cleanup.
void qjackctlClientList::clear (void)
{
	qDeleteAll(m_clients);
	m_clients.clear();

	if (m_pListView)
		m_pListView->clear();
}


// Client finder.
qjackctlClientItem *qjackctlClientList::findClient (
	const QString& sClientName )
{
	QListIterator<qjackctlClientItem *> iter(m_clients);
	while (iter.hasNext()) {
		qjackctlClientItem *pClient = iter.next();
		if (sClientName == pClient->clientName())
			return pClient;
	}

	return NULL;
}

// Client:port finder.
qjackctlPortItem *qjackctlClientList::findClientPort (
	const QString& sClientPort )
{
	qjackctlPortItem *pPort = 0;
	int iColon = sClientPort.indexOf(':');
	if (iColon >= 0) {
		qjackctlClientItem *pClient = findClient(sClientPort.left(iColon));
		if (pClient) {
			pPort = pClient->findPort(
				sClientPort.right(sClientPort.length() - iColon - 1));
		}
	}
	return pPort;
}


// Client list accessor.
QList<qjackctlClientItem *>& qjackctlClientList::clients (void)
{
	return m_clients;
}


// List view accessor.
qjackctlClientListView *qjackctlClientList::listView (void) const
{
	return m_pListView;
}


// Readable flag client accessor.
bool qjackctlClientList::isReadable (void) const
{
	return m_bReadable;
}


// Client:port set housekeeping marker.
void qjackctlClientList::markClientPorts ( int iMark )
{
	m_pHiliteItem = 0;

	QListIterator<qjackctlClientItem *> iter(m_clients);
	while (iter.hasNext())
		(iter.next())->markClientPorts(iMark);
}

int qjackctlClientList::cleanClientPorts ( int iMark )
{
	int iDirtyCount = 0;
	
	QMutableListIterator<qjackctlClientItem *> iter(m_clients);
	while (iter.hasNext()) {
		qjackctlClientItem *pClient = iter.next();
		if (pClient->clientMark() == iMark) {
			iter.remove();
			delete pClient;
			iDirtyCount++;
		} else {
			iDirtyCount += pClient->cleanClientPorts(iMark);
		}
	}

	return iDirtyCount;
}

// Client:port hilite update stabilization.
void qjackctlClientList::hiliteClientPorts (void)
{
	qjackctlClientItem *pClient;
	qjackctlPortItem *pPort;

	QTreeWidgetItem *pItem = m_pListView->currentItem();

	// Dehilite the previous selected items.
	if (m_pHiliteItem && pItem != m_pHiliteItem) {
		if (m_pHiliteItem->type() == QJACKCTL_CLIENTITEM) {
			pClient = static_cast<qjackctlClientItem *> (m_pHiliteItem);
			QListIterator<qjackctlPortItem *> iter(pClient->ports());
			while (iter.hasNext()) {
				pPort = iter.next();
				QListIterator<qjackctlPortItem *> it(pPort->connects());
				while (it.hasNext())
					(it.next())->setHilite(false);
			}
		} else {
			pPort = static_cast<qjackctlPortItem *> (m_pHiliteItem);
			QListIterator<qjackctlPortItem *> it(pPort->connects());
			while (it.hasNext())
				(it.next())->setHilite(false);
		}
	}

	// Hilite the now current selected items.
	if (pItem) {
		if (pItem->type() == QJACKCTL_CLIENTITEM) {
			pClient = static_cast<qjackctlClientItem *> (pItem);
			QListIterator<qjackctlPortItem *> iter(pClient->ports());
			while (iter.hasNext()) {
				pPort = iter.next();
				QListIterator<qjackctlPortItem *> it(pPort->connects());
				while (it.hasNext())
					(it.next())->setHilite(true);
			}
		} else {
			pPort = static_cast<qjackctlPortItem *> (pItem);
			QListIterator<qjackctlPortItem *> it(pPort->connects());
			while (it.hasNext())
				(it.next())->setHilite(true);
		}
	}

	// Do remember this one, ever.
	m_pHiliteItem = pItem;
}


// Natural decimal sorting comparator.
bool qjackctlClientList::lessThan (
	const QTreeWidgetItem& i1, const QTreeWidgetItem& i2 )
{
	const QString& s1 = i1.text(0);
	const QString& s2 = i2.text(0);

	const int cch1 = s1.length();
	const int cch2 = s2.length();

	int ich1, ich2;

	for (ich1 = ich2 = 0; ich1 < cch1 && ich2 < cch2; ich1++, ich2++) {

		// Skip (white)spaces...
		while (s1.at(ich1).isSpace())
			ich1++;
		while (s2.at(ich2).isSpace())
			ich2++;

		// Normalize (to uppercase) the next characters...
		QChar ch1 = s1.at(ich1).toUpper();
		QChar ch2 = s2.at(ich2).toUpper();

		if (ch1.isDigit() && ch2.isDigit()) {
			// Find the whole length numbers...
			const int iDigits1 = ich1++;
			while (ich1 < cch1 && s1.at(ich1).isDigit())
				ich1++;
			const int iDigits2 = ich2++;
			while (ich2 < cch2 && s2.at(ich2).isDigit())
				ich2++;
			// Compare as natural decimal-numbers...
			const int n1 = s1.mid(iDigits1, ich1 - iDigits1).toInt();
			const int n2 = s2.mid(iDigits2, ich2 - iDigits2).toInt();
			if (n1 != n2)
				return (n1 < n2);
			// Never go out of bounds...
			if (ich1 >= cch1 || ich1 >= cch2)
				break;
			// Go on with this next char...
			ch1 = s1.at(ich1).toUpper();
			ch2 = s2.at(ich2).toUpper();
		}

		// Compare this char...
		if (ch1 != ch2)
			return (ch1 < ch2);
	}

	// Probable exact match.
	return false;
}


// Do proper contents refresh/update.
void qjackctlClientList::refresh (void)
{
	QHeaderView *pHeader = m_pListView->header();
	m_pListView->sortItems(
		pHeader->sortIndicatorSection(),
		pHeader->sortIndicatorOrder());
}


//----------------------------------------------------------------------------
// qjackctlClientListView -- Client list view, supporting drag-n-drop.

// Constructor.
qjackctlClientListView::qjackctlClientListView (
	qjackctlConnectView *pConnectView, bool bReadable )
	: QTreeWidget(pConnectView)
{
	m_pConnectView = pConnectView;

	m_pAutoOpenTimer   = 0;
	m_iAutoOpenTimeout = 0;

	m_pDragItem = NULL;
	m_pDragItem = NULL;

	m_pAliases = NULL;
	m_bRenameEnabled = false;

	QHeaderView *pHeader = QTreeWidget::header();
	pHeader->setDefaultAlignment(Qt::AlignLeft);
//	pHeader->setDefaultSectionSize(120);
#if QT_VERSION >= 0x050000
//	pHeader->setSectionResizeMode(QHeaderView::Custom);
	pHeader->setSectionsMovable(false);
	pHeader->setSectionsClickable(true);
#else
//	pHeader->setResizeMode(QHeaderView::Custom);
	pHeader->setMovable(false);
	pHeader->setClickable(true);
#endif
	pHeader->setSortIndicatorShown(true);
	pHeader->setStretchLastSection(true);

	QTreeWidget::setRootIsDecorated(true);
	QTreeWidget::setUniformRowHeights(true);
//	QTreeWidget::setDragEnabled(true);
	QTreeWidget::setAcceptDrops(true);
	QTreeWidget::setDropIndicatorShown(true);
	QTreeWidget::setAutoScroll(true);
	QTreeWidget::setSelectionMode(QAbstractItemView::SingleSelection);
	QTreeWidget::setSizePolicy(
		QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	QTreeWidget::setSortingEnabled(true);
	QTreeWidget::setMinimumWidth(120);
	QTreeWidget::setColumnCount(1);

	QString sText;
	if (bReadable)
		sText = tr("Readable Clients / Output Ports");
	else
		sText = tr("Writable Clients / Input Ports");
	QTreeWidget::headerItem()->setText(0, sText);
	QTreeWidget::sortItems(0, Qt::AscendingOrder);
	QTreeWidget::setToolTip(sText);

	// Trap for help/tool-tips events.
	QTreeWidget::viewport()->installEventFilter(this);

	QObject::connect(QTreeWidget::itemDelegate(),
		SIGNAL(commitData(QWidget*)),
		SLOT(renamedSlot()));

	setAutoOpenTimeout(800);
}

// Default destructor.
qjackctlClientListView::~qjackctlClientListView (void)
{
	setAutoOpenTimeout(0);
}


// Binding indirect accessor.
qjackctlConnect *qjackctlClientListView::binding() const
{
	return m_pConnectView->binding();
}


// Auto-open timeout method.
void qjackctlClientListView::setAutoOpenTimeout ( int iAutoOpenTimeout )
{
	m_iAutoOpenTimeout = iAutoOpenTimeout;

	if (m_pAutoOpenTimer)
		delete m_pAutoOpenTimer;
	m_pAutoOpenTimer = NULL;

	if (m_iAutoOpenTimeout > 0) {
		m_pAutoOpenTimer = new QTimer(this);
		QObject::connect(m_pAutoOpenTimer,
			SIGNAL(timeout()),
			SLOT(timeoutSlot()));
	}
}


// Auto-open timeout accessor.
int qjackctlClientListView::autoOpenTimeout (void) const
{
	return m_iAutoOpenTimeout;
}


// Aliasing support methods.
void qjackctlClientListView::setAliases (
	qjackctlConnectAlias *pAliases, bool bRenameEnabled )
{
	m_pAliases = pAliases;
	m_bRenameEnabled = bRenameEnabled;

	// For each client item, if any.
	const int iItemCount = QTreeWidget::topLevelItemCount();
	for (int iItem = 0; iItem < iItemCount; ++iItem) {
		QTreeWidgetItem *pItem = QTreeWidget::topLevelItem(iItem);
		if (pItem->type() != QJACKCTL_CLIENTITEM)
			continue;
		qjackctlClientItem *pClientItem
			= static_cast<qjackctlClientItem *> (pItem);
		if (pClientItem == NULL)
			continue;
		pClientItem->updateClientName();
		// For each port item...
		const int iChildCount = pClientItem->childCount();
		for (int iChild = 0; iChild < iChildCount; ++iChild) {
			QTreeWidgetItem *pChildItem = pClientItem->child(iChild);
			if (pChildItem->type() != QJACKCTL_PORTITEM)
				continue;
			qjackctlPortItem *pPortItem
				= static_cast<qjackctlPortItem *> (pChildItem);
			if (pPortItem == NULL)
				continue;
			pPortItem->updatePortName();
		}
	}
}

qjackctlConnectAlias *qjackctlClientListView::aliases (void) const
{
	return m_pAliases;
}

bool qjackctlClientListView::isRenameEnabled (void) const
{
	return m_bRenameEnabled;
}


// In-place aliasing slot.
void qjackctlClientListView::startRenameSlot (void)
{
	QTreeWidgetItem *pItem = QTreeWidget::currentItem();
	if (pItem)
		QTreeWidget::editItem(pItem, 0);
}


// In-place aliasing slot.
void qjackctlClientListView::renamedSlot (void)
{
	if (m_pAliases == NULL)
		return;

	QTreeWidgetItem *pItem = QTreeWidget::currentItem();
	if (pItem == NULL)
		return;

	const QString& sText = pItem->text(0);
	if (pItem->type() == QJACKCTL_CLIENTITEM) {
		qjackctlClientItem *pClientItem
			= static_cast<qjackctlClientItem *> (pItem);
		pClientItem->setClientNameAlias(sText);
		pClientItem->updateClientName(true);
	} else {
		qjackctlPortItem *pPortItem
			= static_cast<qjackctlPortItem *> (pItem);
		pPortItem->setPortNameAlias(sText);
		pPortItem->updatePortName(true);
	}

//	m_pConnectView->setDirty(true);
}


// Auto-open timer slot.
void qjackctlClientListView::timeoutSlot (void)
{
	if (m_pAutoOpenTimer) {
		m_pAutoOpenTimer->stop();
		if (m_pDropItem && m_pDropItem->type() == QJACKCTL_CLIENTITEM) {
			qjackctlClientItem *pClientItem
				= static_cast<qjackctlClientItem *> (m_pDropItem);
			if (pClientItem && !pClientItem->isOpen())
				pClientItem->setOpen(true);
		}
	}
}


// Trap for help/tool-tip events.
bool qjackctlClientListView::eventFilter ( QObject *pObject, QEvent *pEvent )
{
	QWidget *pViewport = QTreeWidget::viewport();
	if (static_cast<QWidget *> (pObject) == pViewport
		&& pEvent->type() == QEvent::ToolTip) {
		QHelpEvent *pHelpEvent = static_cast<QHelpEvent *> (pEvent);
		if (pHelpEvent) {
			QTreeWidgetItem *pItem = QTreeWidget::itemAt(pHelpEvent->pos());
			if (pItem && pItem->type() == QJACKCTL_CLIENTITEM) {
				qjackctlClientItem *pClientItem
					= static_cast<qjackctlClientItem *> (pItem);
				if (pClientItem) {
					QToolTip::showText(pHelpEvent->globalPos(),
						pClientItem->clientName(), pViewport);
					return true;
				}
			}
			else
			if (pItem && pItem->type() == QJACKCTL_PORTITEM) {
				qjackctlPortItem *pPortItem
					= static_cast<qjackctlPortItem *> (pItem);
				if (pPortItem) {
					QToolTip::showText(pHelpEvent->globalPos(),
						pPortItem->portName(), pViewport);
					return true;
				}
			}
		}
	}

	// Not handled here.
	return QTreeWidget::eventFilter(pObject, pEvent);
}


// Drag-n-drop stuff.
QTreeWidgetItem *qjackctlClientListView::dragDropItem ( const QPoint& pos )
{
	QTreeWidgetItem *pItem = QTreeWidget::itemAt(pos);
	if (pItem) {
		if (m_pDropItem != pItem) {
			QTreeWidget::setCurrentItem(pItem);
			m_pDropItem = pItem;
			if (m_pAutoOpenTimer)
				m_pAutoOpenTimer->start(m_iAutoOpenTimeout);
			qjackctlConnect *pConnect = m_pConnectView->binding();
			if ((pItem->flags() & Qt::ItemIsDropEnabled) == 0
				|| pConnect == NULL || !pConnect->canConnectSelected())
				pItem = NULL;
		}
	} else {
		m_pDropItem = NULL;
		if (m_pAutoOpenTimer)
			m_pAutoOpenTimer->stop();
	}

	return pItem;
}

void qjackctlClientListView::dragEnterEvent ( QDragEnterEvent *pDragEnterEvent )
{
	if (pDragEnterEvent->source() != this &&
		pDragEnterEvent->mimeData()->hasText() &&
		dragDropItem(pDragEnterEvent->pos())) {
		pDragEnterEvent->accept();
	} else {
		pDragEnterEvent->ignore();
	}
}


void qjackctlClientListView::dragMoveEvent ( QDragMoveEvent *pDragMoveEvent )
{
	if (pDragMoveEvent->source() != this &&
		pDragMoveEvent->mimeData()->hasText() &&
		dragDropItem(pDragMoveEvent->pos())) {
		pDragMoveEvent->accept();
	} else {
		pDragMoveEvent->ignore();
	}
}


void qjackctlClientListView::dragLeaveEvent ( QDragLeaveEvent * )
{
	m_pDropItem = 0;
	if (m_pAutoOpenTimer)
		m_pAutoOpenTimer->stop();
}


void qjackctlClientListView::dropEvent( QDropEvent *pDropEvent )
{
	if (pDropEvent->source() != this &&
		pDropEvent->mimeData()->hasText() &&
		dragDropItem(pDropEvent->pos())) {
		const QString sText = pDropEvent->mimeData()->text();
		qjackctlConnect *pConnect = m_pConnectView->binding();
		if (!sText.isEmpty() && pConnect)
			pConnect->connectSelected();
	}

	dragLeaveEvent(0);
}


// Handle mouse events for drag-and-drop stuff.
void qjackctlClientListView::mousePressEvent ( QMouseEvent *pMouseEvent )
{
	QTreeWidget::mousePressEvent(pMouseEvent);

	if (pMouseEvent->button() == Qt::LeftButton) {
		m_posDrag   = pMouseEvent->pos();
		m_pDragItem = QTreeWidget::itemAt(m_posDrag);
	}
}


void qjackctlClientListView::mouseMoveEvent ( QMouseEvent *pMouseEvent )
{
	QTreeWidget::mouseMoveEvent(pMouseEvent);

	if ((pMouseEvent->buttons() & Qt::LeftButton) && m_pDragItem
		&& ((pMouseEvent->pos() - m_posDrag).manhattanLength()
			>= QApplication::startDragDistance())) {
		// We'll start dragging something alright...
		QMimeData *pMimeData = new QMimeData();
		pMimeData->setText(m_pDragItem->text(0));
		QDrag *pDrag = new QDrag(this);
		pDrag->setMimeData(pMimeData);
		pDrag->setPixmap(m_pDragItem->icon(0).pixmap(16));
		pDrag->setHotSpot(QPoint(-4, -12));
		pDrag->start(Qt::LinkAction);
		// We've dragged and maybe dropped it by now...
		m_pDragItem = NULL;
	}
}


// Context menu request event handler.
void qjackctlClientListView::contextMenuEvent ( QContextMenuEvent *pContextMenuEvent )
{
	qjackctlConnect *pConnect = m_pConnectView->binding();
	if (pConnect == 0)
		return;

	QMenu menu(this);
	QAction *pAction;

	pAction = menu.addAction(QIcon(":/images/connect1.png"),
		tr("&Connect"), pConnect, SLOT(connectSelected()),
		tr("Alt+C", "Connect"));
	pAction->setEnabled(pConnect->canConnectSelected());
	pAction = menu.addAction(QIcon(":/images/disconnect1.png"),
		tr("&Disconnect"), pConnect, SLOT(disconnectSelected()),
		tr("Alt+D", "Disconnect"));
	pAction->setEnabled(pConnect->canDisconnectSelected());
	pAction = menu.addAction(QIcon(":/images/disconnectall1.png"),
		tr("Disconnect &All"), pConnect, SLOT(disconnectAll()),
		tr("Alt+A", "Disconect All"));
	pAction->setEnabled(pConnect->canDisconnectAll());
	if (m_bRenameEnabled) {
		menu.addSeparator();
		pAction = menu.addAction(QIcon(":/images/edit1.png"),
			tr("Re&name"), this, SLOT(startRenameSlot()),
			tr("Alt+N", "Rename"));
		QTreeWidgetItem *pItem = QTreeWidget::currentItem();
		pAction->setEnabled(pItem && (pItem->flags() & Qt::ItemIsEditable));
	}
	menu.addSeparator();
	pAction = menu.addAction(QIcon(":/images/refresh1.png"),
		tr("&Refresh"), pConnect, SLOT(refresh()),
		tr("Alt+R", "Refresh"));

	menu.exec(pContextMenuEvent->globalPos());
}



// Dirty flag methods.
void qjackctlClientListView::setDirty ( bool bDirty )
{
	m_pConnectView->setDirty(bDirty);
}

bool qjackctlClientListView::isDirty (void) const
{
	return m_pConnectView->isDirty();
}


//----------------------------------------------------------------------
// qjackctlConnectorView -- Jack port connector widget.
//

// Constructor.
qjackctlConnectorView::qjackctlConnectorView (
	qjackctlConnectView *pConnectView )
	: QWidget(pConnectView)
{
	m_pConnectView = pConnectView;

	QWidget::setMinimumWidth(20);
//  QWidget::setMaximumWidth(120);
	QWidget::setSizePolicy(
		QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
}

// Default destructor.
qjackctlConnectorView::~qjackctlConnectorView (void)
{
}


// Legal client/port item position helper.
int qjackctlConnectorView::itemY ( QTreeWidgetItem *pItem ) const
{
	QRect rect;
	QTreeWidget *pList = pItem->treeWidget();
	QTreeWidgetItem *pParent = pItem->parent();
	qjackctlClientItem *pClientItem = NULL;
	if (pParent && pParent->type() == QJACKCTL_CLIENTITEM)
		pClientItem = static_cast<qjackctlClientItem *> (pParent);
	if (pClientItem && !pClientItem->isOpen()) {
		rect = pList->visualItemRect(pClientItem);
	} else {
		rect = pList->visualItemRect(pItem);
	}
	return rect.top() + rect.height() / 2;
}


// Draw visible port connection relation lines
void qjackctlConnectorView::drawConnectionLine ( QPainter *pPainter,
	int x1, int y1, int x2, int y2, int h1, int h2, const QPen& pen )
{
	// Set apropriate pen...
	pPainter->setPen(pen);

	// Account for list view headers.
	y1 += h1;
	y2 += h2;

	// Invisible output ports don't get a connecting dot.
	if (y1 > h1)
		pPainter->drawLine(x1, y1, x1 + 4, y1);

	// How do we'll draw it?
	if (m_pConnectView->isBezierLines()) {
		// Setup control points
		QPolygon spline(4);
		const int cp = int(float(x2 - x1 - 8) * 0.4f);
		spline.putPoints(0, 4,
			x1 + 4, y1, x1 + 4 + cp, y1, 
			x2 - 4 - cp, y2, x2 - 4, y2);
		// The connection line, it self.
		QPainterPath path;
		path.moveTo(spline.at(0));
		path.cubicTo(spline.at(1), spline.at(2), spline.at(3));
		pPainter->strokePath(path, pen);
	}
	else pPainter->drawLine(x1 + 4, y1, x2 - 4, y2);

	// Invisible input ports don't get a connecting dot.
	if (y2 > h2)
		pPainter->drawLine(x2 - 4, y2, x2, y2);
}


// Draw visible port connection relation arrows.
void qjackctlConnectorView::paintEvent ( QPaintEvent * )
{
	if (m_pConnectView == NULL)
		return;
	if (m_pConnectView->OListView() == NULL ||
		m_pConnectView->IListView() == NULL)
		return;

	qjackctlClientListView *pOListView = m_pConnectView->OListView();
	qjackctlClientListView *pIListView = m_pConnectView->IListView();

	const int yc = QWidget::pos().y();
	const int yo = pOListView->pos().y();
	const int yi = pIListView->pos().y();

	QPainter painter(this);
	int x1, y1, h1;
	int x2, y2, h2;
	int i, rgb[3] = { 0x33, 0x66, 0x99 };

	// Draw all lines anti-aliased...
	painter.setRenderHint(QPainter::Antialiasing);

	// Inline adaptive to darker background themes...
	if (QWidget::palette().window().color().value() < 0x7f)
		for (i = 0; i < 3; ++i) rgb[i] += 0x33;

	// Initialize color changer.
	i = 0;
	// Almost constants.
	x1 = 0;
	x2 = QWidget::width();
	h1 = (pOListView->header())->sizeHint().height();
	h2 = (pIListView->header())->sizeHint().height();
	// For each output client item...
	const int iItemCount = pOListView->topLevelItemCount();
	for (int iItem = 0; iItem < iItemCount; ++iItem) {
		QTreeWidgetItem *pItem = pOListView->topLevelItem(iItem);
		if (pItem->type() != QJACKCTL_CLIENTITEM)
			continue;
		qjackctlClientItem *pOClient
			= static_cast<qjackctlClientItem *> (pItem);
		if (pOClient == NULL)
			continue;
		// Set new connector color.
		++i;
		QPen pen(QColor(rgb[i % 3], rgb[(i / 3) % 3], rgb[(i / 9) % 3]));
		// For each port item
		const int iChildCount = pOClient->childCount();
		for (int iChild = 0; iChild < iChildCount; ++iChild) {
			QTreeWidgetItem *pChild = pOClient->child(iChild);
			if (pChild->type() != QJACKCTL_PORTITEM)
				continue;
			qjackctlPortItem *pOPort
				= static_cast<qjackctlPortItem *> (pChild);
			if (pOPort) {
				// Set proposed line width...
				const int w1 = (pOPort->isHilite() ? 2 : 1);
				// Get starting connector arrow coordinates.
				y1 = itemY(pOPort) + (yo - yc);
				// Get port connections...
				QListIterator<qjackctlPortItem *> iter(pOPort->connects());
				while (iter.hasNext()) {
					qjackctlPortItem *pIPort = iter.next();
					// Obviously, should be a connection
					// from pOPort to pIPort items:
					y2 = itemY(pIPort) + (yi - yc);
					pen.setWidth(pIPort->isHilite() ? 2 : w1);
					drawConnectionLine(&painter, x1, y1, x2, y2, h1, h2, pen);
				}
			}
		}
	}
}


// Context menu request event handler.
void qjackctlConnectorView::contextMenuEvent (
	QContextMenuEvent *pContextMenuEvent )
{
	qjackctlConnect *pConnect = m_pConnectView->binding();
	if (pConnect == 0)
		return;

	QMenu menu(this);
	QAction *pAction;

	pAction = menu.addAction(QIcon(":/images/connect1.png"),
		tr("&Connect"), pConnect, SLOT(connectSelected()),
		tr("Alt+C", "Connect"));
	pAction->setEnabled(pConnect->canConnectSelected());
	pAction = menu.addAction(QIcon(":/images/disconnect1.png"),
		tr("&Disconnect"), pConnect, SLOT(disconnectSelected()),
		tr("Alt+D", "Disconnect"));
	pAction->setEnabled(pConnect->canDisconnectSelected());
	pAction = menu.addAction(QIcon(":/images/disconnectall1.png"),
		tr("Disconnect &All"), pConnect, SLOT(disconnectAll()),
		tr("Alt+A", "Disconect All"));
	pAction->setEnabled(pConnect->canDisconnectAll());

	menu.addSeparator();
	pAction = menu.addAction(QIcon(":/images/refresh1.png"),
		tr("&Refresh"), pConnect, SLOT(refresh()),
		tr("Alt+R", "Refresh"));

	menu.exec(pContextMenuEvent->globalPos());
}


// Widget event slots...
void qjackctlConnectorView::contentsChanged (void)
{
	QWidget::update();
}


//----------------------------------------------------------------------------
// qjackctlConnectView -- Integrated connections view widget.

// Constructor.
qjackctlConnectView::qjackctlConnectView ( QWidget *pParent )
	: QSplitter(Qt::Horizontal, pParent)
{
	m_pOListView     = new qjackctlClientListView(this, true);
	m_pConnectorView = new qjackctlConnectorView(this);
	m_pIListView     = new qjackctlClientListView(this, false);

	m_pConnect = NULL;

	m_bBezierLines = false;
	m_iIconSize    = 0;

	QSplitter::setHandleWidth(2);

	QObject::connect(m_pOListView, SIGNAL(itemExpanded(QTreeWidgetItem *)),
		m_pConnectorView, SLOT(contentsChanged()));
	QObject::connect(m_pOListView, SIGNAL(itemCollapsed(QTreeWidgetItem *)),
		m_pConnectorView, SLOT(contentsChanged()));
	QObject::connect(m_pOListView->verticalScrollBar(), SIGNAL(valueChanged(int)),
		m_pConnectorView, SLOT(contentsChanged()));
	QObject::connect(m_pOListView->header(), SIGNAL(sectionClicked(int)),
		m_pConnectorView, SLOT(contentsChanged()));

	QObject::connect(m_pIListView, SIGNAL(itemExpanded(QTreeWidgetItem *)),
		m_pConnectorView, SLOT(contentsChanged()));
	QObject::connect(m_pIListView, SIGNAL(itemCollapsed(QTreeWidgetItem *)),
		m_pConnectorView, SLOT(contentsChanged()));
	QObject::connect(m_pIListView->verticalScrollBar(), SIGNAL(valueChanged(int)),
		m_pConnectorView, SLOT(contentsChanged()));
	QObject::connect(m_pIListView->header(), SIGNAL(sectionClicked(int)),
		m_pConnectorView, SLOT(contentsChanged()));

	m_bDirty = false;
}


// Default destructor.
qjackctlConnectView::~qjackctlConnectView (void)
{
}


// Connect binding methods.
void qjackctlConnectView::setBinding ( qjackctlConnect *pConnect )
{
	m_pConnect = pConnect;
}

qjackctlConnect *qjackctlConnectView::binding (void) const
{
	return m_pConnect;
}


// Connect client list accessors.
qjackctlClientList *qjackctlConnectView::OClientList (void) const
{
	if (m_pConnect)
		return m_pConnect->OClientList();
	else
		return NULL;
}

qjackctlClientList *qjackctlConnectView::IClientList (void) const
{
	if (m_pConnect)
		return m_pConnect->OClientList();
	else
		return NULL;
}


// Connector line style accessors.
void qjackctlConnectView::setBezierLines ( bool bBezierLines )
{
	m_bBezierLines = bBezierLines;
}

bool qjackctlConnectView::isBezierLines (void) const
{
	return m_bBezierLines;
}


// Common icon size methods.
void qjackctlConnectView::setIconSize ( int iIconSize )
{
	// Update only if changed.
	if (iIconSize == m_iIconSize)
		return;

	// Go for it...
	m_iIconSize = iIconSize;

	// Update item sizes properly...
	const int px = (16 << m_iIconSize);
	const QSize iconSize(px, px);
	m_pOListView->setIconSize(iconSize);
	m_pIListView->setIconSize(iconSize);

	// Call binding descendant implementation,
	// and do a complete content reset...
	if (m_pConnect)
		m_pConnect->updateContents(true);
}

int qjackctlConnectView::iconSize (void) const
{
	return m_iIconSize;
}


// Dirty flag methods.
void qjackctlConnectView::setDirty ( bool bDirty )
{
	m_bDirty = bDirty;

	if (bDirty)
		emit contentsChanged();
}

bool qjackctlConnectView::isDirty (void) const
{
	return m_bDirty;
}


//----------------------------------------------------------------------
// qjackctlConnect -- Output-to-Input client/ports connection object.
//

// Constructor.
qjackctlConnect::qjackctlConnect ( qjackctlConnectView *pConnectView )
{
	m_pConnectView = pConnectView;

	m_pOClientList = NULL;
	m_pIClientList = NULL;

	m_iMutex = 0;

	m_pConnectView->setBinding(this);
}

// Default destructor.
qjackctlConnect::~qjackctlConnect (void)
{
	// Force end of works here.
	m_iMutex++;

	m_pConnectView->setBinding(NULL);

	if (m_pOClientList)
		delete m_pOClientList;
	if (m_pIClientList)
		delete m_pIClientList;

	m_pOClientList = NULL;
	m_pIClientList = NULL;

	m_pConnectView->ConnectorView()->update();
}


// These must be accessed by the descendant constructor.
qjackctlConnectView *qjackctlConnect::connectView (void) const
{
	return m_pConnectView;
}

void qjackctlConnect::setOClientList ( qjackctlClientList *pOClientList )
{
	m_pOClientList = pOClientList;
}

void qjackctlConnect::setIClientList ( qjackctlClientList *pIClientList )
{
	m_pIClientList = pIClientList;
}


// Connection primitive.
bool qjackctlConnect::connectPortsEx (
	qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
	if (pOPort->findConnectPtr(pIPort) != NULL)
		return false;
	
	emit connecting(pOPort, pIPort);

	if (!connectPorts(pOPort, pIPort))
		return false;

	pOPort->addConnect(pIPort);
	pIPort->addConnect(pOPort);
	return true;
}

// Disconnection primitive.
bool qjackctlConnect::disconnectPortsEx (
	qjackctlPortItem *pOPort, qjackctlPortItem *pIPort )
{
	if (pOPort->findConnectPtr(pIPort) == NULL)
		return false;

	emit disconnecting(pOPort, pIPort);

	if (!disconnectPorts(pOPort, pIPort))
		return false;

	pOPort->removeConnect(pIPort);
	pIPort->removeConnect(pOPort);
	return true;
}


// Test if selected ports are connectable.
bool qjackctlConnect::canConnectSelected (void)
{
	bool bResult = false;

	if (startMutex()) {
		bResult = canConnectSelectedEx();
		endMutex();
	}

	return bResult;
}

bool qjackctlConnect::canConnectSelectedEx (void)
{
	// Take this opportunity to highlight any current selections.
	m_pOClientList->hiliteClientPorts();
	m_pIClientList->hiliteClientPorts();

	// Now with our predicate work...
	QTreeWidgetItem *pOItem = (m_pOClientList->listView())->currentItem();
	if (pOItem == NULL)
		return false;

	QTreeWidgetItem *pIItem = (m_pIClientList->listView())->currentItem();
	if (pIItem == NULL)
		return false;

	if (pOItem->type() == QJACKCTL_CLIENTITEM) {
		qjackctlClientItem *pOClient
			= static_cast<qjackctlClientItem *> (pOItem);
		if (pIItem->type() == QJACKCTL_CLIENTITEM) {
			// Each-to-each connections...
			qjackctlClientItem *pIClient
				= static_cast<qjackctlClientItem *> (pIItem);
			QListIterator<qjackctlPortItem *> oport(pOClient->ports());
			QListIterator<qjackctlPortItem *> iport(pIClient->ports());
			while (oport.hasNext() && iport.hasNext()) {
				qjackctlPortItem *pOPort = oport.next();
				qjackctlPortItem *pIPort = iport.next();
				if (pOPort->findConnectPtr(pIPort) == NULL)
					return true;
			}
		} else {
			// Many(all)-to-one/many connection...
			QListIterator<qjackctlPortItem *> oport(pOClient->ports());
			while (oport.hasNext()
				&& pIItem && pIItem->type() == QJACKCTL_PORTITEM) {
				qjackctlPortItem *pOPort = oport.next();
				qjackctlPortItem *pIPort
					= static_cast<qjackctlPortItem *> (pIItem);
				if (pOPort->findConnectPtr(pIPort) == NULL)
					return true;
				pIItem = (m_pIClientList->listView())->itemBelow(pIItem);
			}
		}
	} else {
		qjackctlPortItem *pOPort
			= static_cast<qjackctlPortItem *> (pOItem);
		if (pIItem->type() == QJACKCTL_CLIENTITEM) {
			// One-to-many(all) connection...
			qjackctlClientItem *pIClient
				= static_cast<qjackctlClientItem *> (pIItem);
			QListIterator<qjackctlPortItem *> iport(pIClient->ports());
			while (iport.hasNext()) {
				qjackctlPortItem *pIPort = iport.next();
				if (pOPort->findConnectPtr(pIPort) == NULL)
					return true;
			}
		} else {
			// One-to-one connection...
			qjackctlPortItem *pIPort
				= static_cast<qjackctlPortItem *> (pIItem);
			return (pOPort->findConnectPtr(pIPort) == NULL);
		}
	}

	return false;
}


// Connect current selected ports.
bool qjackctlConnect::connectSelected (void)
{
	bool bResult = false;

	if (startMutex()) {
		bResult = connectSelectedEx();
		endMutex();
	}

	m_pConnectView->ConnectorView()->update();

	if (bResult)
		emit connectChanged();

	return bResult;
}

bool qjackctlConnect::connectSelectedEx (void)
{
	QTreeWidgetItem *pOItem = (m_pOClientList->listView())->currentItem();
	if (pOItem == NULL)
		return false;

	QTreeWidgetItem *pIItem = (m_pIClientList->listView())->currentItem();
	if (pIItem == NULL)
		return false;

	if (pOItem->type() == QJACKCTL_CLIENTITEM) {
		qjackctlClientItem *pOClient
			= static_cast<qjackctlClientItem *> (pOItem);
		if (pIItem->type() == QJACKCTL_CLIENTITEM) {
			// Each-to-each connections...
			qjackctlClientItem *pIClient
				= static_cast<qjackctlClientItem *> (pIItem);
			QListIterator<qjackctlPortItem *> oport(pOClient->ports());
			QListIterator<qjackctlPortItem *> iport(pIClient->ports());
			while (oport.hasNext() && iport.hasNext()) {
				qjackctlPortItem *pOPort = oport.next();
				qjackctlPortItem *pIPort = iport.next();
				connectPortsEx(pOPort, pIPort);
			}
		} else {
			// Many(all)-to-one/many connection...
			QListIterator<qjackctlPortItem *> oport(pOClient->ports());
			while (oport.hasNext()
				&& pIItem && pIItem->type() == QJACKCTL_PORTITEM) {
				qjackctlPortItem *pOPort = oport.next();
				qjackctlPortItem *pIPort
					= static_cast<qjackctlPortItem *> (pIItem);
				connectPortsEx(pOPort, pIPort);
				pIItem = (m_pIClientList->listView())->itemBelow(pIItem);
			}
		}
	} else {
		qjackctlPortItem *pOPort
			= static_cast<qjackctlPortItem *> (pOItem);
		if (pIItem->type() == QJACKCTL_CLIENTITEM) {
			// One-to-many(all) connection...
			qjackctlClientItem *pIClient
				= static_cast<qjackctlClientItem *> (pIItem);
			QListIterator<qjackctlPortItem *> iport(pIClient->ports());
			while (iport.hasNext()) {
				qjackctlPortItem *pIPort = iport.next();
				connectPortsEx(pOPort, pIPort);
			}
		} else {
			// One-to-one connection...
			qjackctlPortItem *pIPort
				= static_cast<qjackctlPortItem *> (pIItem);
			connectPortsEx(pOPort, pIPort);
		}
	}

	return true;
}


// Test if selected ports are disconnectable.
bool qjackctlConnect::canDisconnectSelected (void)
{
	bool bResult = false;

	if (startMutex()) {
		bResult = canDisconnectSelectedEx();
		endMutex();
	}

	return bResult;
}

bool qjackctlConnect::canDisconnectSelectedEx (void)
{
	QTreeWidgetItem *pOItem = (m_pOClientList->listView())->currentItem();
	if (!pOItem)
		return false;

	QTreeWidgetItem *pIItem = (m_pIClientList->listView())->currentItem();
	if (!pIItem)
		return false;

	if (pOItem->type() == QJACKCTL_CLIENTITEM) {
		qjackctlClientItem *pOClient
			= static_cast<qjackctlClientItem *> (pOItem);
		if (pIItem->type() == QJACKCTL_CLIENTITEM) {
			// Each-to-each connections...
			qjackctlClientItem *pIClient
				= static_cast<qjackctlClientItem *> (pIItem);
			QListIterator<qjackctlPortItem *> oport(pOClient->ports());
			QListIterator<qjackctlPortItem *> iport(pIClient->ports());
			while (oport.hasNext() && iport.hasNext()) {
				qjackctlPortItem *pOPort = oport.next();
				qjackctlPortItem *pIPort = iport.next();
				if (pOPort->findConnectPtr(pIPort) != NULL)
					return true;
			}
		} else {
			// Many(all)-to-one/many connection...
			QListIterator<qjackctlPortItem *> oport(pOClient->ports());
			while (oport.hasNext()
				&& pIItem && pIItem->type() == QJACKCTL_PORTITEM) {
				qjackctlPortItem *pOPort = oport.next();
				qjackctlPortItem *pIPort
					= static_cast<qjackctlPortItem *> (pIItem);
				if (pOPort->findConnectPtr(pIPort) != NULL)
					return true;
				pIItem = (m_pIClientList->listView())->itemBelow(pIItem);
			}
		}
	} else {
		qjackctlPortItem *pOPort
			= static_cast<qjackctlPortItem *> (pOItem);
		if (pIItem->type() == QJACKCTL_CLIENTITEM) {
			// One-to-many(all) connection...
			qjackctlClientItem *pIClient
				= static_cast<qjackctlClientItem *> (pIItem);
			QListIterator<qjackctlPortItem *> iport(pIClient->ports());
			while (iport.hasNext()) {
				qjackctlPortItem *pIPort = iport.next();
				if (pOPort->findConnectPtr(pIPort) != NULL)
					return true;
			}
		} else {
			// One-to-one connection...
			qjackctlPortItem *pIPort
				= static_cast<qjackctlPortItem *> (pIItem);
			return (pOPort->findConnectPtr(pIPort) != NULL);
		}
	}

	return false;
}


// Disconnect current selected ports.
bool qjackctlConnect::disconnectSelected (void)
{
	bool bResult = false;

	if (startMutex()) {
		bResult = disconnectSelectedEx();
		endMutex();
	}

	m_pConnectView->ConnectorView()->update();

	if (bResult)
		emit connectChanged();

	return bResult;
}

bool qjackctlConnect::disconnectSelectedEx (void)
{
	QTreeWidgetItem *pOItem = (m_pOClientList->listView())->currentItem();
	if (pOItem == NULL)
		return false;

	QTreeWidgetItem *pIItem = (m_pIClientList->listView())->currentItem();
	if (pIItem == NULL)
		return false;

	if (pOItem->type() == QJACKCTL_CLIENTITEM) {
		qjackctlClientItem *pOClient
			= static_cast<qjackctlClientItem *> (pOItem);
		if (pIItem->type() == QJACKCTL_CLIENTITEM) {
			// Each-to-each connections...
			qjackctlClientItem *pIClient
				= static_cast<qjackctlClientItem *> (pIItem);
			QListIterator<qjackctlPortItem *> oport(pOClient->ports());
			QListIterator<qjackctlPortItem *> iport(pIClient->ports());
			while (oport.hasNext() && iport.hasNext()) {
				qjackctlPortItem *pOPort = oport.next();
				qjackctlPortItem *pIPort = iport.next();
				disconnectPortsEx(pOPort, pIPort);
			}
		} else {
			// Many(all)-to-one/many connection...
			QListIterator<qjackctlPortItem *> oport(pOClient->ports());
			while (oport.hasNext()
				&& pIItem && pIItem->type() == QJACKCTL_PORTITEM) {
				qjackctlPortItem *pOPort = oport.next();
				qjackctlPortItem *pIPort
					= static_cast<qjackctlPortItem *> (pIItem);
				disconnectPortsEx(pOPort, pIPort);
				pIItem = (m_pIClientList->listView())->itemBelow(pIItem);
			}
		}
	} else {
		qjackctlPortItem *pOPort
			= static_cast<qjackctlPortItem *> (pOItem);
		if (pIItem->type() == QJACKCTL_CLIENTITEM) {
			// One-to-many(all) connection...
			qjackctlClientItem *pIClient
				= static_cast<qjackctlClientItem *> (pIItem);
			QListIterator<qjackctlPortItem *> iport(pIClient->ports());
			while (iport.hasNext()) {
				qjackctlPortItem *pIPort = iport.next();
				disconnectPortsEx(pOPort, pIPort);
			}
		} else {
			// One-to-one connection...
			qjackctlPortItem *pIPort
				= static_cast<qjackctlPortItem *> (pIItem);
			disconnectPortsEx(pOPort, pIPort);
		}
	}

	return true;
}


// Test if any port is disconnectable.
bool qjackctlConnect::canDisconnectAll (void)
{
	bool bResult = false;

	if (startMutex()) {
		bResult = canDisconnectAllEx();
		endMutex();
	}

	return bResult;
}

bool qjackctlConnect::canDisconnectAllEx (void)
{
	QListIterator<qjackctlClientItem *> iter(m_pOClientList->clients());
	while (iter.hasNext()) {
		qjackctlClientItem *pOClient = iter.next();
		QListIterator<qjackctlPortItem *> oport(pOClient->ports());
		while (oport.hasNext()) {
			qjackctlPortItem *pOPort = oport.next();
			if (pOPort->connects().count() > 0)
				return true;
		}
	}
	return false;
}


// Disconnect all ports.
bool qjackctlConnect::disconnectAll (void)
{
	if (QMessageBox::warning(m_pConnectView,
		tr("Warning") + " - " QJACKCTL_SUBTITLE1,
		tr("This will suspend sound processing\n"
		"from all client applications.\n\nAre you sure?"),
		QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
		return false;
	}

	bool bResult = false;

	if (startMutex()) {
		bResult = disconnectAllEx();
		endMutex();
	}

	m_pConnectView->ConnectorView()->update();

	if (bResult)
		emit connectChanged();

	return bResult;
}

bool qjackctlConnect::disconnectAllEx (void)
{
	QListIterator<qjackctlClientItem *> iter(m_pOClientList->clients());
	while (iter.hasNext()) {
		qjackctlClientItem *pOClient = iter.next();
		QListIterator<qjackctlPortItem *> oport(pOClient->ports());
		while (oport.hasNext()) {
			qjackctlPortItem *pOPort = oport.next();
			QListIterator<qjackctlPortItem *> iport(pOPort->connects());
			while (iport.hasNext()) {
				qjackctlPortItem *pIPort = iport.next();
				disconnectPortsEx(pOPort, pIPort);
			}
		}
	}
	return true;
}


// Expand all client ports.
void qjackctlConnect::expandAll (void)
{
	(m_pOClientList->listView())->expandAll();
	(m_pIClientList->listView())->expandAll();
	(m_pConnectView->ConnectorView())->update();
}


// Complete/incremental contents rebuilder; check dirty status if incremental.
void qjackctlConnect::updateContents ( bool bClear )
{
	int iDirtyCount = 0;

	if (startMutex()) {
		// Do we do a complete rebuild?
		if (bClear) {
			m_pOClientList->clear();
			m_pIClientList->clear();
			updateIconPixmaps();
		}
		// Add (newer) client:ports and respective connections...
		if (m_pOClientList->updateClientPorts() > 0) {
			m_pOClientList->refresh();
			iDirtyCount++;
		}
		if (m_pIClientList->updateClientPorts() > 0) {
			m_pIClientList->refresh();
			iDirtyCount++;
		}			
		updateConnections();
		endMutex();
	}

	(m_pConnectView->ConnectorView())->update();

	if (!bClear && iDirtyCount > 0)
		emit connectChanged();
}


// Incremental contents rebuilder; check dirty status.
void qjackctlConnect::refresh (void)
{
	updateContents(false);
}


// Dunno. But this may avoid some conflicts.
bool qjackctlConnect::startMutex (void)
{
	const bool bMutex = (m_iMutex == 0);
	if (bMutex)
		m_iMutex++;
	return bMutex;
}

void qjackctlConnect::endMutex (void)
{
	if (m_iMutex > 0)
		m_iMutex--;
}


// Connect client list accessors.
qjackctlClientList *qjackctlConnect::OClientList (void) const
{
	return m_pOClientList;
}

qjackctlClientList *qjackctlConnect::IClientList (void) const
{
	return m_pIClientList;
}


// Common pixmap factory-method.
QPixmap *qjackctlConnect::createIconPixmap ( const QString& sIconName )
{
	QString sName = sIconName;

	const int iSize = m_pConnectView->iconSize() * 32;
	if (iSize > 0)
		sName += QString("_%1x%2").arg(iSize).arg(iSize);

	return new QPixmap(":/images/" + sName + ".png");
}


// end of qjackctlConnect.cpp
