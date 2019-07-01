// qjackctlSessionForm.cpp
//
/****************************************************************************
   Copyright (C) 2003-2019, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include "qjackctlSessionForm.h"
#include "qjackctlSession.h"

#include "qjackctlSetup.h"

#include "qjackctlMainForm.h"

#include <QMenu>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

#include <QTreeWidget>
#include <QHeaderView>

#include <QLineEdit>
#include <QToolButton>

#include <QBitmap>
#include <QPainter>

#include <QShowEvent>
#include <QHideEvent>

// Local prototypes.
static void remove_dir_list(const QList<QFileInfo>& list);
static void remove_dir(const QString& sDir);

// Remove specific file path.
static void remove_dir ( const QString& sDir )
{
	QDir dir(sDir);

	remove_dir_list(
		dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot));

	dir.rmdir(sDir);
}

static void remove_dir_list ( const QList<QFileInfo>& list )
{
	QListIterator<QFileInfo> iter(list);
	while (iter.hasNext()) {
		const QFileInfo& info = iter.next();
		const QString& sPath = info.absoluteFilePath();
		if (info.isDir()) {
			remove_dir(sPath);
		} else {
			QFile::remove(sPath);
		}
	}
}


//-------------------------------------------------------------------------
// qjackctlSessionInfraClientItemEditor

qjackctlSessionInfraClientItemEditor::qjackctlSessionInfraClientItemEditor (
	QWidget *pParent, const QModelIndex& index )
	: QWidget(pParent), m_index(index)
{
	m_pItemEdit = new QLineEdit(/*this*/);
	if (index.column() == 1) {
		m_pBrowseButton = new QToolButton(/*this*/);
		m_pBrowseButton->setFixedWidth(18);
		m_pBrowseButton->setText("...");
	}
	else m_pBrowseButton = NULL;
	m_pResetButton = new QToolButton(/*this*/);
	m_pResetButton->setFixedWidth(18);
	m_pResetButton->setText("x");

	QHBoxLayout *pLayout = new QHBoxLayout();
	pLayout->setSpacing(0);
	pLayout->setMargin(0);
	pLayout->addWidget(m_pItemEdit);
	if (m_pBrowseButton)
		pLayout->addWidget(m_pBrowseButton);
	pLayout->addWidget(m_pResetButton);
	QWidget::setLayout(pLayout);

	QWidget::setFocusPolicy(Qt::StrongFocus);
	QWidget::setFocusProxy(m_pItemEdit);

	QObject::connect(m_pItemEdit,
		SIGNAL(editingFinished()),
		SLOT(finishSlot()));

	if (m_pBrowseButton)
		QObject::connect(m_pBrowseButton,
			SIGNAL(clicked()),
			SLOT(browseSlot()));

	QObject::connect(m_pResetButton,
		SIGNAL(clicked()),
		SLOT(resetSlot()));
}


// Item text accessors.
void qjackctlSessionInfraClientItemEditor::setText ( const QString& sText )
{
	m_pItemEdit->setText(sText);
}


QString qjackctlSessionInfraClientItemEditor::text (void) const
{
	return m_pItemEdit->text();
}


// Item command browser.
void qjackctlSessionInfraClientItemEditor::browseSlot (void)
{
	const bool bBlockSignals = m_pItemEdit->blockSignals(true);
	const QString& sCommand
		= QFileDialog::getOpenFileName(parentWidget(), tr("Infra-command"));
	if (!sCommand.isEmpty())
		m_pItemEdit->setText(sCommand);
	m_pItemEdit->blockSignals(bBlockSignals);
}


// Item text clear/toggler.
void qjackctlSessionInfraClientItemEditor::resetSlot (void)
{
	if (m_pItemEdit->text() == m_sDefaultText)
		m_pItemEdit->clear();
	else
		m_pItemEdit->setText(m_sDefaultText);

	m_pItemEdit->setFocus();
}


// Item text finish notification.
void qjackctlSessionInfraClientItemEditor::finishSlot (void)
{
	const bool bBlockSignals = m_pItemEdit->blockSignals(true);
	emit finishSignal();
	m_index = QModelIndex();
	m_sDefaultText.clear();
	m_pItemEdit->blockSignals(bBlockSignals);
}


//-------------------------------------------------------------------------
// qjackctlSessionInfraClientItemDelegate

qjackctlSessionInfraClientItemDelegate::qjackctlSessionInfraClientItemDelegate (
	QObject *pParent ) : QItemDelegate(pParent)
{
}


QWidget *qjackctlSessionInfraClientItemDelegate::createEditor (
	QWidget *pParent, const QStyleOptionViewItem& /*option*/,
	const QModelIndex& index ) const
{
	qjackctlSessionInfraClientItemEditor *pItemEditor
		= new qjackctlSessionInfraClientItemEditor(pParent, index);
	pItemEditor->setDefaultText(
		index.model()->data(index, Qt::DisplayRole).toString());
	QObject::connect(pItemEditor,
		SIGNAL(finishSignal()),
		SLOT(commitEditor()));
	return pItemEditor;
}


void qjackctlSessionInfraClientItemDelegate::setEditorData ( QWidget *pEditor,
	const QModelIndex& index ) const
{
	qjackctlSessionInfraClientItemEditor *pItemEditor
		= qobject_cast<qjackctlSessionInfraClientItemEditor *> (pEditor);
	pItemEditor->setText(
		index.model()->data(index, Qt::DisplayRole).toString());
}


void qjackctlSessionInfraClientItemDelegate::setModelData ( QWidget *pEditor,
	QAbstractItemModel *pModel, const QModelIndex& index ) const
{
	qjackctlSessionInfraClientItemEditor *pItemEditor
		= qobject_cast<qjackctlSessionInfraClientItemEditor *> (pEditor);
	pModel->setData(index, pItemEditor->text());
}


void qjackctlSessionInfraClientItemDelegate::commitEditor (void)
{
	qjackctlSessionInfraClientItemEditor *pItemEditor
		= qobject_cast<qjackctlSessionInfraClientItemEditor *> (sender());

	const QString& sText = pItemEditor->text();
	const QString& sDefaultText = pItemEditor->defaultText();

	if (sText != sDefaultText)
		emit commitData(pItemEditor);

	emit closeEditor(pItemEditor);
}


//----------------------------------------------------------------------------
// qjackctlSessionForm -- UI wrapper form.

// Constructor.
qjackctlSessionForm::qjackctlSessionForm (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QWidget(pParent, wflags)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	m_pSetup = NULL;

	// Common (sigleton) session object.
	m_pSession = new qjackctlSession();

	//	Set recent menu stuff...
	m_pRecentMenu = new QMenu(tr("&Recent"));
	m_ui.RecentSessionPushButton->setMenu(m_pRecentMenu);

	m_pSaveMenu = new QMenu(tr("&Save"));
	m_pSaveMenu->setIcon(QIcon(":/images/save1.png"));
	m_pSaveMenu->addAction(QIcon(":/images/save1.png"),
		tr("&Save..."),
		this, SLOT(saveSessionSave()));
#ifdef CONFIG_JACK_SESSION
	m_pSaveMenu->addAction(
		tr("Save and &Quit..."),
		this, SLOT(saveSessionSaveAndQuit()));
	m_pSaveMenu->addAction(
		tr("Save &Template..."),
		this, SLOT(saveSessionSaveTemplate()));
#endif
	m_ui.SaveSessionPushButton->setMenu(m_pSaveMenu);

	// Session tree view...
	QHeaderView *pHeader = m_ui.SessionTreeView->header();
//	pHeader->setDefaultAlignment(Qt::AlignLeft);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
//	pHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
//	pHeader->setResizeMode(QHeaderView::ResizeToContents);
#endif
	pHeader->resizeSection(0, 200); // Client/Ports
	pHeader->resizeSection(1, 40);  // UUID
	pHeader->setStretchLastSection(true);

	m_ui.SessionTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

	// Infra-client list view...
	pHeader = m_ui.InfraClientListView->header();
//	pHeader->setDefaultAlignment(Qt::AlignLeft);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
//	pHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
//	pHeader->setResizeMode(QHeaderView::ResizeToContents);
#endif
	pHeader->resizeSection(0, 120); // Infra-client
	pHeader->setStretchLastSection(true);

	m_ui.InfraClientListView->setItemDelegate(
		new qjackctlSessionInfraClientItemDelegate(m_ui.InfraClientListView));
	m_ui.InfraClientListView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_ui.InfraClientListView->sortItems(0, Qt::AscendingOrder);

	// UI connections...
	QObject::connect(m_ui.LoadSessionPushButton,
		SIGNAL(clicked()),
		SLOT(loadSession()));
	QObject::connect(m_ui.UpdateSessionPushButton,
		SIGNAL(clicked()),
		SLOT(updateSession()));

	QObject::connect(m_ui.SessionTreeView,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(sessionViewContextMenu(const QPoint&)));

	QObject::connect(m_ui.InfraClientListView,
		SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
		SLOT(selectInfraClient()));
	QObject::connect(m_ui.AddInfraClientPushButton,
		SIGNAL(clicked()),
		SLOT(addInfraClient()));
	QObject::connect(m_ui.EditInfraClientPushButton,
		SIGNAL(clicked()),
		SLOT(editInfraClient()));
	QObject::connect(m_ui.RemoveInfraClientPushButton,
		SIGNAL(clicked()),
		SLOT(removeInfraClient()));
	QObject::connect(m_ui.InfraClientListView->itemDelegate(),
		SIGNAL(commitData(QWidget *)),
		SLOT(editInfraClientCommit()));
	QObject::connect(m_ui.InfraClientListView,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(infraClientContextMenu(const QPoint&)));

	// Start disabled.
	stabilizeForm(false);
}


// Destructor.
qjackctlSessionForm::~qjackctlSessionForm (void)
{
	delete m_pSaveMenu;
	delete m_pRecentMenu;
	delete m_pSession;
}


// Set reference to global options, mostly needed for
// the initial session save type and directories.
void qjackctlSessionForm::setup ( qjackctlSetup *pSetup )
{
	m_pSetup = pSetup;

	if (m_pSetup) {
		m_ui.SaveSessionVersionCheckBox->setChecked(
			m_pSetup->bSessionSaveVersion);
		m_sessionDirs = m_pSetup->sessionDirs;
		// Setup infra-clients table view...
		QList<int> sizes;
		sizes.append(320);
		sizes.append(120);
		m_pSetup->loadSplitterSizes(m_ui.InfraClientSplitter, sizes);
		// Load infra-clients table-view...
		m_pSession->loadInfraClients(m_pSetup->settings());
	}

	updateRecentMenu();
	updateInfraClients();
}


// Maybe ask whether we can close.
bool qjackctlSessionForm::queryClose (void)
{
	bool bQueryClose = true;

	// Maybe just save some splitter sizes...
	if (m_pSetup && bQueryClose) {
		// Rebuild infra-clients list...
		m_pSession->clearInfraClients();
		qjackctlSession::InfraClientList& list = m_pSession->infra_clients();
		const int iItemCount = m_ui.InfraClientListView->topLevelItemCount();
		for (int i = 0; i < iItemCount; ++i) {
			QTreeWidgetItem *pItem = m_ui.InfraClientListView->topLevelItem(i);
			if (pItem) {
				const QString& sKey = pItem->text(0);
				const QString& sValue = pItem->text(1);
				if (!sValue.isEmpty()) {
					qjackctlSession::InfraClientItem *pInfraClientItem
						= new qjackctlSession::InfraClientItem;
					pInfraClientItem->client_name = sKey;
					pInfraClientItem->client_command = sValue;
					list.insert(sKey, pInfraClientItem);
				}
			}
		}
		// Save infra-clients table-view...
		m_pSession->saveInfraClients(m_pSetup->settings());
		m_pSetup->saveSplitterSizes(m_ui.InfraClientSplitter);
	}

	return bQueryClose;
}


// Recent session directories accessor.
const QStringList& qjackctlSessionForm::sessionDirs (void) const
{
	return m_sessionDirs;
}


// Session save versioning option.
bool qjackctlSessionForm::isSaveSessionVersion (void) const
{
	return m_ui.SaveSessionVersionCheckBox->isChecked();
}
 
 
// Recent menu accessor.
QMenu *qjackctlSessionForm::recentMenu (void) const
{
	return m_pRecentMenu;
}


// Save menu accessor.
QMenu *qjackctlSessionForm::saveMenu (void) const
{
	return m_pSaveMenu;
}


// Notify our parent that we're emerging.
void qjackctlSessionForm::showEvent ( QShowEvent *pShowEvent )
{
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();

	QWidget::showEvent(pShowEvent);
}


// Notify our parent that we're closing.
void qjackctlSessionForm::hideEvent ( QHideEvent *pHideEvent )
{
	QWidget::hideEvent(pHideEvent);

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}


// Just about to notify main-window that we're closing.
void qjackctlSessionForm::closeEvent ( QCloseEvent * /*pCloseEvent*/ )
{
	if (m_pSetup) {
		m_pSetup->sessionDirs = sessionDirs();
		m_pSetup->bSessionSaveVersion = isSaveSessionVersion();
	}

	QWidget::hide();

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}


// Open/load session from specific file path.
void qjackctlSessionForm::loadSession (void)
{
#if 0
	QFileDialog loadDialog(this, tr("Load Session"));
	loadDialog.setAcceptMode(QFileDialog::AcceptOpen);
	loadDialog.setFileMode(QFileDialog::Directory);
	loadDialog.setViewMode(QFileDialog::List);
	loadDialog.setOptions(QFileDialog::ShowDirsOnly);
	loadDialog.setNameFilter(tr("Session directory"));
	loadDialog.setHistory(m_sessionDirs);
	if (!m_sessionDirs.isEmpty())
		loadDialog.setDirectory(m_sessionDirs.first());
	if (!loadDialog.exec())
		return;
	QString sSessionDir = loadDialog.selectedFiles().first();
#else
	QString sSessionDir;
	if (!m_sessionDirs.isEmpty())
		sSessionDir = m_sessionDirs.first();
	sSessionDir = QFileDialog::getExistingDirectory(
		this, tr("Load Session"), sSessionDir);
#endif

	loadSessionDir(sSessionDir);
}


// Load a recent session.
void qjackctlSessionForm::recentSession (void)
{
	QAction *pAction = qobject_cast<QAction *> (sender());
	if (pAction) {
		const int i = pAction->data().toInt();
		if (i >= 0 && i < m_sessionDirs.count())
			loadSessionDir(m_sessionDirs.at(i));
	}
}


// Save current session to specific file path.
void qjackctlSessionForm::saveSessionSave (void)
{
	saveSessionEx(0);
}

void qjackctlSessionForm::saveSessionSaveAndQuit (void)
{
	saveSessionEx(1);
}

void qjackctlSessionForm::saveSessionSaveTemplate (void)
{
	saveSessionEx(2);
}

void qjackctlSessionForm::saveSessionEx ( int iSessionType )
{
	QString sTitle = tr("Save Session");
	switch (iSessionType) {
	case 1:
		sTitle += ' ' + tr("and Quit");
		break;
	case 2:
		sTitle += ' ' + tr("Template");
		break;
	}

#if 0
	QFileDialog saveDialog(this, sTitle);
	saveDialog.setAcceptMode(QFileDialog::AcceptSave);
	saveDialog.setFileMode(QFileDialog::Directory);
	saveDialog.setViewMode(QFileDialog::List);
	saveDialog.setOptions(QFileDialog::ShowDirsOnly);
	saveDialog.setNameFilter(tr("Session directory"));
	saveDialog.setHistory(m_sessionDirs);
	if (!m_sessionDirs.isEmpty())
		saveDialog.setDirectory(m_sessionDirs.first());
	if (!saveDialog.exec())
		return;
	QString sSessionDir = saveDialog.selectedFiles().first();
#else
	QString sSessionDir;
	if (!m_sessionDirs.isEmpty())
		sSessionDir = m_sessionDirs.first();
	sSessionDir = QFileDialog::getExistingDirectory(
		this, sTitle, sSessionDir);
#endif

	saveSessionDir(sSessionDir, iSessionType);
}


// Save current session to specific file path.
void qjackctlSessionForm::saveSessionVersion ( bool bOn )
{
	m_ui.SaveSessionVersionCheckBox->setChecked(bOn);
}


// Update the recent session list and menu.
void qjackctlSessionForm::updateRecent ( const QString& sSessionDir )
{
	const int i = m_sessionDirs.indexOf(sSessionDir);
	if (i >= 0)
		m_sessionDirs.removeAt(i);
	m_sessionDirs.prepend(sSessionDir);
	updateRecentMenu();
}


// Update/stabilize recent sessions menu.
void qjackctlSessionForm::updateRecentMenu (void)
{
	int iRecent = m_sessionDirs.count();
	for (; iRecent > 8; --iRecent)
		m_sessionDirs.pop_back();

	m_pRecentMenu->clear();

	for (int i = 0; i < iRecent; ++i) {
		const QString& sSessionDir = m_sessionDirs.at(i);
		if (QDir(sSessionDir).exists()) {
			QAction *pAction = m_pRecentMenu->addAction(
				QFileInfo(sSessionDir).fileName(),
				this, SLOT(recentSession()));
			pAction->setData(i);
		}
	}

	if (iRecent > 0) {
		m_pRecentMenu->addSeparator();
		m_pRecentMenu->addAction(tr("&Clear"),
			this, SLOT(clearRecentMenu()));
	}

	m_ui.RecentSessionPushButton->setEnabled(iRecent > 0);
}


// Clear recent sessions menu.
void qjackctlSessionForm::clearRecentMenu (void)
{
	m_sessionDirs.clear();
	updateRecentMenu();
}


// Open/load session from specific file path.
void qjackctlSessionForm::loadSessionDir ( const QString& sSessionDir )
{
	if (sSessionDir.isEmpty())
		return;

	const QDir sessionDir(sSessionDir);
	if (!sessionDir.exists("session.xml")) {
		QMessageBox::critical(this,
			tr("Warning") + " - " QJACKCTL_SUBTITLE1,
			tr("A session could not be found in this folder:\n\n\"%1\"")
			.arg(sSessionDir));
		return;
	}

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	jack_client_t *pJackClient = pMainForm->jackClient();
	if (pJackClient == NULL)
		return;

	pMainForm->appendMessages(
		tr("%1: loading session...").arg(sSessionDir));

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	const bool bLoadSession = m_pSession->load(sSessionDir);
	if (bLoadSession)
		updateRecent(sessionDir.absolutePath());

	updateSessionView();

	QApplication::restoreOverrideCursor();

	pMainForm->appendMessages(
		tr("%1: load session %2.").arg(sSessionDir)
		.arg(bLoadSession ? "OK" : "FAILED"));
}


// Save current session to specific file path.
void qjackctlSessionForm::saveSessionDir (
	const QString& sSessionDir, int iSessionType )
{
	if (sSessionDir.isEmpty())
		return;

	QDir sessionDir(sSessionDir);
	const QList<QFileInfo> list
		= sessionDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
	if (!list.isEmpty()) {
		if (sessionDir.exists("session.xml")) {
			if (QMessageBox::warning(this,
				tr("Warning") + " - " QJACKCTL_SUBTITLE1,
				tr("A session already exists in this folder:\n\n\"%1\"\n\n"
				"Are you sure to overwrite the existing session?").arg(sSessionDir),
				QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
				return;
		} else {
			if (QMessageBox::warning(this,
				tr("Warning") + " - " QJACKCTL_SUBTITLE1,
				tr("This folder already exists and is not empty:\n\n\"%1\"\n\n"
				"Are you sure to overwrite the existing folder?").arg(sSessionDir),
				QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
				return;
		}
	//	remove_dir_list(list);
	}

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	jack_client_t *pJackClient = pMainForm->jackClient();
	if (pJackClient == NULL)
		return;

	pMainForm->appendMessages(
		tr("%1: saving session...").arg(sSessionDir));

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	if (!list.isEmpty()) {
		if (isSaveSessionVersion()) {
			int iSessionDirNo = 0;
			const QString sSessionDirMask = sSessionDir + ".%1";
			QFileInfo fi(sSessionDirMask.arg(++iSessionDirNo));
			while (fi.exists())
				fi.setFile(sSessionDirMask.arg(++iSessionDirNo));
			sessionDir.rename(sSessionDir, fi.absoluteFilePath());
		}
		else remove_dir_list(list);
		sessionDir.refresh();
	}

	if (!sessionDir.exists())
		sessionDir.mkpath(sSessionDir);

	bool bSaveSession = m_pSession->save(sSessionDir, iSessionType);
	if (bSaveSession)
		updateRecent(sSessionDir);

	updateSessionView();

	QApplication::restoreOverrideCursor();

	pMainForm->appendMessages(
		tr("%1: save session %2.").arg(sSessionDir)
		.arg(bSaveSession ? "OK" : "FAILED"));
}


// Set icon error status according to given flag.
QIcon qjackctlSessionForm::iconStatus ( const QIcon& icon, bool bStatus )
{
	QPixmap pm(icon.pixmap(16, 16));

	// Merge with the overlay pixmap...
	if (bStatus) {
		const QPixmap pmOverlay(":/images/error1.png");
		if (!pmOverlay.mask().isNull()) {
			QBitmap mask = pm.mask();
			QPainter(&mask).drawPixmap(0, 0, pmOverlay.mask());
			pm.setMask(mask);
			QPainter(&pm).drawPixmap(0, 0, pmOverlay);
		}
	}

	return QIcon(pm);
}


// Update/populate session tree view.
void qjackctlSessionForm::updateSessionView (void)
{
	m_ui.SessionTreeView->clear();

	QList<QTreeWidgetItem *> items;

	const QIcon iconClient(":/images/client1.png");
	const QIcon iconPort(":/images/port1.png");
	const QIcon iconConnect(":/images/connect1.png");

	qjackctlSession::ClientList::ConstIterator iterClient
		= m_pSession->clients().constBegin();
	for ( ; iterClient != m_pSession->clients().constEnd(); ++iterClient) {
		qjackctlSession::ClientItem *pClientItem = iterClient.value();
		QTreeWidgetItem *pTopLevelItem = new QTreeWidgetItem();
		pTopLevelItem->setIcon(0,
			iconStatus(iconClient, pClientItem->connected));
		pTopLevelItem->setText(0, pClientItem->client_name);
		pTopLevelItem->setText(1, pClientItem->client_uuid);
		pTopLevelItem->setText(2, pClientItem->client_command);
		QListIterator<qjackctlSession::PortItem *>
			iterPort(pClientItem->ports);
		QTreeWidgetItem *pChildItem = NULL;
		while (iterPort.hasNext()) {
			qjackctlSession::PortItem *pPortItem = iterPort.next();
			pChildItem = new QTreeWidgetItem(pTopLevelItem, pChildItem);
			pChildItem->setIcon(0,
				iconStatus(iconPort, pPortItem->connected));
			pChildItem->setText(0, pPortItem->port_name);
			QListIterator<qjackctlSession::ConnectItem *>
				iterConnect(pPortItem->connects);
			QTreeWidgetItem *pLeafItem = NULL;
			while (iterConnect.hasNext()) {
				qjackctlSession::ConnectItem *pConnectItem = iterConnect.next();
				pLeafItem = new QTreeWidgetItem(pChildItem, pLeafItem);
				pLeafItem->setIcon(0,
					iconStatus(iconConnect, !pConnectItem->connected));
				pLeafItem->setText(0,
					pConnectItem->client_name + ':' +
					pConnectItem->port_name);
			}
		}
		items.append(pTopLevelItem);
	}

	m_ui.SessionTreeView->insertTopLevelItems(0, items);
	m_ui.SessionTreeView->expandAll();
}


// Update/populate session connections and tree view.
void qjackctlSessionForm::updateSession (void)
{
	m_pSession->update();
	updateSessionView();
	updateInfraClients();
}


// Context menu event handler.
void qjackctlSessionForm::sessionViewContextMenu ( const QPoint& pos )
{
	QMenu menu(this);
	QAction *pAction;

	bool bEnabled = false;
	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm)
		bEnabled = (pMainForm->jackClient() != NULL);

	pAction = menu.addAction(QIcon(":/images/open1.png"),
		tr("&Load..."), this, SLOT(loadSession()));
	pAction->setEnabled(bEnabled);
	pAction = menu.addMenu(m_pRecentMenu);
	pAction->setEnabled(bEnabled && !m_pRecentMenu->isEmpty());
	menu.addSeparator();
	pAction = menu.addAction(QIcon(":/images/save1.png"),
		tr("&Save..."), this, SLOT(saveSessionSave()));
	pAction->setEnabled(bEnabled);
#ifdef CONFIG_JACK_SESSION
	pAction = menu.addAction(
		tr("Save and &Quit..."), this, SLOT(saveSessionSaveAndQuit()));
	pAction->setEnabled(bEnabled);
	pAction = menu.addAction(
		tr("Save &Template..."), this, SLOT(saveSessionSaveTemplate()));
	pAction->setEnabled(bEnabled);
#endif
	menu.addSeparator();
	pAction = menu.addAction(
		tr("&Versioning"), this, SLOT(saveSessionVersion(bool)));
	pAction->setCheckable(true);
	pAction->setChecked(isSaveSessionVersion());
	pAction->setEnabled(bEnabled);
	menu.addSeparator();
	pAction = menu.addAction(QIcon(":/images/refresh1.png"),
		tr("Re&fresh"), this, SLOT(updateSession()));

	menu.exec(m_ui.SessionTreeView->mapToGlobal(pos));
}


// Add a new infra-client entry.
void qjackctlSessionForm::addInfraClient (void)
{
#ifdef CONFIG_DEBUG_0
	qDebug("qjackctlSessionForm::addInfraClient()");
#endif

	const QString& sNewInfraClient = tr("New Client");

	QTreeWidgetItem *pItem = NULL;

	const QList<QTreeWidgetItem *>& items
		= m_ui.InfraClientListView->findItems(sNewInfraClient, Qt::MatchExactly);
	if (items.isEmpty()) {
		pItem = m_ui.InfraClientListView->currentItem();
		pItem = new QTreeWidgetItem(m_ui.InfraClientListView, pItem);
		pItem->setIcon(0,
			iconStatus(QIcon(":/images/client1.png"), true));
		pItem->setText(0, sNewInfraClient);
		pItem->setFlags(pItem->flags() | Qt::ItemIsEditable);
	} else {
		pItem = items.first();
	}

	m_ui.InfraClientListView->editItem(pItem, 0);
}


// Edit current infra-client entry.
void qjackctlSessionForm::editInfraClient (void)
{
#ifdef CONFIG_DEBUG_0
	qDebug("qjackctlSessionForm::editInfraClient()");
#endif

	QTreeWidgetItem *pItem = m_ui.InfraClientListView->currentItem();
	if (pItem)
		m_ui.InfraClientListView->editItem(pItem, 1);
}


void qjackctlSessionForm::editInfraClientCommit (void)
{
#ifdef CONFIG_DEBUG_0
	qDebug("qjackctlSessionForm::editInfraClientCommit()");
#endif

	QTreeWidgetItem *pItem = m_ui.InfraClientListView->currentItem();
	if (pItem) {
		const QString& sKey = pItem->text(0);
		if (!sKey.isEmpty()) {
			const QString& sValue = pItem->text(1);
			qjackctlSession::InfraClientItem *pInfraClientItem = NULL;
			qjackctlSession::InfraClientList& list = m_pSession->infra_clients();
			qjackctlSession::InfraClientList::Iterator iter	= list.find(sKey);
			if (iter == list.end()) {
				pInfraClientItem = new qjackctlSession::InfraClientItem;
				pInfraClientItem->client_name = sKey;
				pInfraClientItem->client_command = sValue;
				list.insert(sKey, pInfraClientItem);
			} else {
				pInfraClientItem = iter.value();
				pInfraClientItem->client_command = sValue;
			}
			pItem->setIcon(0, iconStatus(QIcon(":/images/client1.png"),
				pInfraClientItem->client_command.isEmpty()));
		}
	}
}


// Remove current infra-client entry.
void qjackctlSessionForm::removeInfraClient (void)
{
#ifdef CONFIG_DEBUG_0
	qDebug("qjackctlSessionForm::removeInfraClient()");
#endif

	QTreeWidgetItem *pItem = m_ui.InfraClientListView->currentItem();
	if (pItem) {
		const QString& sKey = pItem->text(0);
		qjackctlSession::InfraClientList& list = m_pSession->infra_clients();
		qjackctlSession::InfraClientList::Iterator iter	= list.find(sKey);
		if (iter != list.end())
			list.erase(iter);
		delete pItem;
		updateInfraClients();
	}
}


// Select current infra-client entry.
void qjackctlSessionForm::selectInfraClient (void)
{
#ifdef CONFIG_DEBUG_0
	qDebug("qjackctlSessionForm::selectInfraClient()");
#endif

	QTreeWidgetItem *pItem = m_ui.InfraClientListView->currentItem();
	m_ui.AddInfraClientPushButton->setEnabled(true);
	m_ui.EditInfraClientPushButton->setEnabled(pItem != NULL);
	m_ui.RemoveInfraClientPushButton->setEnabled(pItem != NULL);
}


// Update/populate infra-clients commands list view.
void qjackctlSessionForm::updateInfraClients (void)
{
#ifdef CONFIG_DEBUG_0
	qDebug("qjackctlSessionForm::updateInfraClients()");
#endif

	const int iOldItem = m_ui.InfraClientListView->indexOfTopLevelItem(
		m_ui.InfraClientListView->currentItem());

	m_ui.InfraClientListView->clear();

	const QIcon iconClient(":/images/client1.png");

	QTreeWidgetItem *pItem = NULL;
	qjackctlSession::InfraClientList& list = m_pSession->infra_clients();
	qjackctlSession::InfraClientList::ConstIterator iter = list.constBegin();
	const qjackctlSession::InfraClientList::ConstIterator& iter_end
		= list.constEnd();
	for( ; iter != iter_end; ++iter) {
		qjackctlSession::InfraClientItem *pInfraClientItem = iter.value();
		pItem = new QTreeWidgetItem(m_ui.InfraClientListView, pItem);
		pItem->setIcon(0, iconStatus(iconClient,
			pInfraClientItem->client_command.isEmpty()));
		pItem->setText(0, pInfraClientItem->client_name);
		pItem->setText(1, pInfraClientItem->client_command);
		pItem->setFlags(pItem->flags() | Qt::ItemIsEditable);
	}

	int iItemCount = m_ui.InfraClientListView->topLevelItemCount();
	if (iOldItem >= 0 && iOldItem < iItemCount) {
		m_ui.InfraClientListView->setCurrentItem(
			m_ui.InfraClientListView->topLevelItem(iOldItem));
	}
}


// Infra-client list context menu.
void qjackctlSessionForm::infraClientContextMenu ( const QPoint& pos )
{
	QMenu menu(this);
	QAction *pAction;

	QTreeWidgetItem *pItem = m_ui.InfraClientListView->currentItem();
	pAction = menu.addAction(QIcon(":/images/add1.png"),
		tr("&Add"), this, SLOT(addInfraClient()));
//	pAction->setEnabled(true);
	pAction = menu.addAction(QIcon(":/images/edit1.png"),
		tr("&Edit"), this, SLOT(editInfraClient()));
	pAction->setEnabled(pItem != NULL);
	pAction = menu.addAction(QIcon(":/images/remove1.png"),
		tr("Re&move"), this, SLOT(removeInfraClient()));
	pAction->setEnabled(pItem != NULL);
	menu.addSeparator();
	pAction = menu.addAction(QIcon(":/images/refresh1.png"),
		tr("Re&fresh"), this, SLOT(updateInfraClients()));
//	pAction->setEnabled(true);

	menu.exec(m_ui.InfraClientListView->mapToGlobal(pos));
}


// Stabilize form status.
void qjackctlSessionForm::stabilizeForm ( bool bEnabled )
{
	m_ui.LoadSessionPushButton->setEnabled(bEnabled);
	m_ui.RecentSessionPushButton->setEnabled(bEnabled && !m_pRecentMenu->isEmpty());
	m_ui.SaveSessionPushButton->setEnabled(bEnabled);
	m_ui.SaveSessionVersionCheckBox->setEnabled(bEnabled);
	m_ui.UpdateSessionPushButton->setEnabled(bEnabled);

	if (!bEnabled) {
		m_pSession->clear();
		m_ui.SessionTreeView->clear();
	}

	selectInfraClient();
}


// Keyboard event handler.
void qjackctlSessionForm::keyPressEvent ( QKeyEvent *pKeyEvent )
{
#ifdef CONFIG_DEBUG_0
	qDebug("qjackctlSessionForm::keyPressEvent(%d)", pKeyEvent->key());
#endif
	const int iKey = pKeyEvent->key();
	switch (iKey) {
	case Qt::Key_Escape:
		close();
		break;
	default:
		QWidget::keyPressEvent(pKeyEvent);
		break;
	}
}


// end of qjackctlSessionForm.cpp
