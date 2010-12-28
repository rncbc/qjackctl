// qjackctlSessionForm.cpp
//
/****************************************************************************
   Copyright (C) 2003-2010, rncbc aka Rui Nuno Capela. All rights reserved.

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


//----------------------------------------------------------------------------
// qjackctlSessionForm -- UI wrapper form.

// Constructor.
qjackctlSessionForm::qjackctlSessionForm (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QWidget(pParent, wflags)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	// Common (sigleton) session object.
	m_pSession = new qjackctlSession();

	//	Set recent menu stuff...
	m_pRecentMenu = new QMenu(tr("&Recent"));
	m_ui.RecentSessionPushButton->setMenu(m_pRecentMenu);

	// Session tree view...
	QHeaderView *pHeader = m_ui.SessionTreeView->header();
//	pHeader->setResizeMode(QHeaderView::ResizeToContents);
	pHeader->resizeSection(0, 200); // Client/Ports
	pHeader->resizeSection(1, 40);  // UUID
//	pHeader->setDefaultAlignment(Qt::AlignLeft);
	pHeader->setStretchLastSection(true);
	
	// UI connections...
	QObject::connect(m_ui.LoadSessionPushButton,
		SIGNAL(clicked()),
		SLOT(loadSession()));
	QObject::connect(m_ui.SaveSessionPushButton,
		SIGNAL(clicked()),
		SLOT(saveSession()));

	QObject::connect(m_ui.UpdateSessionPushButton,
		SIGNAL(clicked()),
		SLOT(updateSession()));

	// Start disabled.
	stabilizeForm(false);
}


// Destructor.
qjackctlSessionForm::~qjackctlSessionForm (void)
{
	delete m_pRecentMenu;
	delete m_pSession;
}


// Set reference to global options, mostly needed for
// the initial session save type and directories.
void qjackctlSessionForm::setup ( qjackctlSetup *pSetup )
{
	m_ui.SaveSessionComboBox->setCurrentIndex(pSetup->iSessionSaveType);

	m_sessionDirs = pSetup->sessionDirs;

	updateRecentMenu();
}


// Recent session directories accessor.
const QStringList& qjackctlSessionForm::sessionDirs (void) const
{
	return m_sessionDirs;
}


// Recent session save type accessor.
int qjackctlSessionForm::sessionSaveType (void) const
{
	return m_ui.SaveSessionComboBox->currentIndex();
}


// Recent menu accessor.
QMenu *qjackctlSessionForm::recentMenu (void) const
{
	return m_pRecentMenu;
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
		int i = pAction->data().toInt();
		if (i >= 0 && i < m_sessionDirs.count())
			loadSessionDir(m_sessionDirs.at(i));
	}
}


// Save current session to specific file path.
void qjackctlSessionForm::saveSession (void)
{
	saveSessionEx(m_ui.SaveSessionComboBox->currentIndex());
}

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


// Update the recent session list and menu.
void qjackctlSessionForm::updateRecent ( const QString& sSessionDir )
{
	int i = m_sessionDirs.indexOf(sSessionDir);
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
				QFileInfo(sSessionDir).baseName(),
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

	bool bLoadSession = m_pSession->load(sSessionDir);
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
	QList<QFileInfo> list
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
		remove_dir_list(list);
	}
	else
	if (!sessionDir.exists())
		sessionDir.mkpath(sSessionDir);

	qjackctlMainForm *pMainForm = qjackctlMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	jack_client_t *pJackClient = pMainForm->jackClient();
	if (pJackClient == NULL)
		return;

	pMainForm->appendMessages(
		tr("%1: saving session...").arg(sSessionDir));

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	bool bSaveSession = m_pSession->save(sSessionDir, iSessionType);
	if (bSaveSession)
		updateRecent(sessionDir.absolutePath());

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

	QIcon iconClient(":/images/client1.png");
	QIcon iconPort(":/images/port1.png");
	QIcon iconConnect(":/images/connect1.png");

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
}


// Stabilize form status.
void qjackctlSessionForm::stabilizeForm ( bool bEnabled )
{
	m_ui.LoadSessionPushButton->setEnabled(bEnabled);
	m_ui.RecentSessionPushButton->setEnabled(bEnabled && !m_pRecentMenu->isEmpty());
	m_ui.SaveSessionPushButton->setEnabled(bEnabled);
	m_ui.SaveSessionComboBox->setEnabled(bEnabled);
	m_ui.UpdateSessionPushButton->setEnabled(bEnabled);

	if (!bEnabled) {
		m_pSession->clear();
		m_ui.SessionTreeView->clear();
	}
}


// Context menu event handler.
void qjackctlSessionForm::contextMenuEvent (
	QContextMenuEvent *pContextMenuEvent )
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
	pAction = menu.addAction(QIcon(":/images/refresh1.png"),
		tr("&Refresh"), this, SLOT(updateSession()));

	menu.exec(pContextMenuEvent->globalPos());
}


// Keyboard event handler.
void qjackctlSessionForm::keyPressEvent ( QKeyEvent *pKeyEvent )
{
#ifdef CONFIG_DEBUG_0
	qDebug("qjackctlSessionForm::keyPressEvent(%d)", pKeyEvent->key());
#endif
	int iKey = pKeyEvent->key();
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
