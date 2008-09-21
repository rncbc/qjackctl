// qjackctlPatchbay.cpp
//
/****************************************************************************
   Copyright (C) 2003-2008, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qjackctlPatchbay.h"

#include <QMessageBox>
#include <QHeaderView>
#include <QPainterPath>
#include <QPainter>
#include <QPolygon>
#include <QPixmap>
#include <QBitmap>
#include <QRegExp>
#include <QTimer>
#include <QMenu>
#include <QToolTip>
#include <QScrollBar>

#include <QDragEnterEvent>
#include <QDragMoveEvent>

// Interactivity socket form.
#include "qjackctlSocketForm.h"

// External patchbay loader.
#include "qjackctlPatchbayFile.h"

// Aliases accessors.
#include "qjackctlConnectAlias.h"


//----------------------------------------------------------------------
// class qjackctlPlugItem -- Socket plug list item.
//

// Constructor.
qjackctlPlugItem::qjackctlPlugItem ( qjackctlSocketItem *pSocket,
	const QString& sPlugName, qjackctlPlugItem *pPlugAfter )
	: QTreeWidgetItem(pSocket, pPlugAfter, QJACKCTL_PLUGITEM)
{
	QTreeWidgetItem::setText(0, sPlugName);

	m_pSocket   = pSocket;
	m_sPlugName = sPlugName;

	m_pSocket->plugs().append(this);

	int iPixmap;
	if (pSocket->socketType() == QJACKCTL_SOCKETTYPE_JACK_AUDIO)
		iPixmap = QJACKCTL_XPM_AUDIO_PLUG;
	else
		iPixmap = QJACKCTL_XPM_MIDI_PLUG;
	QTreeWidgetItem::setIcon(0, QIcon(pSocket->pixmap(iPixmap)));

	QTreeWidgetItem::setFlags(QTreeWidgetItem::flags()
		& ~Qt::ItemIsSelectable);
}

// Default destructor.
qjackctlPlugItem::~qjackctlPlugItem (void)
{
	int iPlug = m_pSocket->plugs().indexOf(this);
	if (iPlug >= 0)
		m_pSocket->plugs().removeAt(iPlug);
}


// Instance accessors.
const QString& qjackctlPlugItem::socketName (void) const
{
	return m_pSocket->socketName();
}

const QString& qjackctlPlugItem::plugName (void) const
{
	return m_sPlugName;
}


// Patchbay socket item accessor.
qjackctlSocketItem *qjackctlPlugItem::socket (void) const
{
	return m_pSocket;
}


//----------------------------------------------------------------------
// class qjackctlSocketItem -- Jack client list item.
//

// Constructor.
qjackctlSocketItem::qjackctlSocketItem ( qjackctlSocketList *pSocketList,
	const QString& sSocketName, const QString& sClientName,
	int iSocketType, qjackctlSocketItem *pSocketAfter )
	: QTreeWidgetItem(pSocketList->listView(), pSocketAfter, QJACKCTL_SOCKETITEM)
{
	QTreeWidgetItem::setText(0, sSocketName);

	m_pSocketList = pSocketList;
	m_sSocketName = sSocketName;
	m_sClientName = sClientName;
	m_iSocketType = iSocketType;
	m_bExclusive  = false;
	m_sSocketForward.clear();

	m_pSocketList->sockets().append(this);

	updatePixmap();
}

// Default destructor.
qjackctlSocketItem::~qjackctlSocketItem (void)
{
	QListIterator<qjackctlSocketItem *> iter(m_connects);
	while (iter.hasNext())
		(iter.next())->removeConnect(this);

	m_connects.clear();
	m_plugs.clear();

	int iSocket = m_pSocketList->sockets().indexOf(this);
	if (iSocket >= 0)
		m_pSocketList->sockets().removeAt(iSocket);
}


// Instance accessors.
const QString& qjackctlSocketItem::socketName (void) const
{
	return m_sSocketName;
}

const QString& qjackctlSocketItem::clientName (void) const
{
	return m_sClientName;
}

int qjackctlSocketItem::socketType (void) const
{
	return m_iSocketType;
}

bool qjackctlSocketItem::isExclusive (void) const
{
	return m_bExclusive;
}

const QString& qjackctlSocketItem::forward (void) const
{
	return m_sSocketForward;
}


void qjackctlSocketItem::setSocketName ( const QString& sSocketName )
{
	m_sSocketName = sSocketName;
}

void qjackctlSocketItem::setClientName ( const QString& sClientName )
{
	m_sClientName = sClientName;
}

void qjackctlSocketItem::setSocketType ( int iSocketType )
{
	m_iSocketType = iSocketType;
}

void qjackctlSocketItem::setExclusive ( bool bExclusive )
{
	m_bExclusive = bExclusive;
}

void qjackctlSocketItem::setForward ( const QString& sSocketForward )
{
	m_sSocketForward = sSocketForward;
}


// Socket flags accessor.
bool qjackctlSocketItem::isReadable (void) const
{
	return m_pSocketList->isReadable();
}


// Plug finder.
qjackctlPlugItem *qjackctlSocketItem::findPlug ( const QString& sPlugName )
{
	QListIterator<qjackctlPlugItem *> iter(m_plugs);
	while (iter.hasNext()) {
		qjackctlPlugItem *pPlug = iter.next();
		if (sPlugName == pPlug->plugName())
			return pPlug;
	}

	return NULL;
}


// Plug list accessor.
QList<qjackctlPlugItem *>& qjackctlSocketItem::plugs (void)
{
	return m_plugs;
}


// Connected socket list primitives.
void qjackctlSocketItem::addConnect( qjackctlSocketItem *pSocket )
{
	m_connects.append(pSocket);
}

void qjackctlSocketItem::removeConnect( qjackctlSocketItem *pSocket )
{
	int iSocket = m_connects.indexOf(pSocket);
	if (iSocket >= 0)
		m_connects.removeAt(iSocket);
}



// Connected socket finder.
qjackctlSocketItem *qjackctlSocketItem::findConnectPtr (
	qjackctlSocketItem *pSocketPtr )
{
	QListIterator<qjackctlSocketItem *> iter(m_connects);
	while (iter.hasNext()) {
		qjackctlSocketItem *pSocket = iter.next();
		if (pSocketPtr == pSocket)
			return pSocket;
	}

	return NULL;
}


// Connection cache list accessor.
const QList<qjackctlSocketItem *>& qjackctlSocketItem::connects (void) const
{
	return m_connects;
}


// Plug list cleaner.
void qjackctlSocketItem::clear (void)
{
	qDeleteAll(m_plugs);
	m_plugs.clear();
}


// Socket item pixmap peeker.
const QPixmap& qjackctlSocketItem::pixmap ( int iPixmap ) const
{
	return m_pSocketList->pixmap(iPixmap);
}


// Update pixmap to its proper context.
void qjackctlSocketItem::updatePixmap (void)
{
	int iPixmap;
	if (m_iSocketType == QJACKCTL_SOCKETTYPE_JACK_AUDIO) {
		iPixmap = (m_bExclusive
			? QJACKCTL_XPM_AUDIO_SOCKET_X
			: QJACKCTL_XPM_AUDIO_SOCKET);
	} else {
		iPixmap = (m_bExclusive
			? QJACKCTL_XPM_MIDI_SOCKET_X
			: QJACKCTL_XPM_MIDI_SOCKET);
	}
	QTreeWidgetItem::setIcon(0, QIcon(pixmap(iPixmap)));
}


// Socket item openness status.
void qjackctlSocketItem::setOpen ( bool bOpen )
{
#if QT_VERSION >= 0x040201
	QTreeWidgetItem::setExpanded(bOpen);
#else
	QTreeWidgetItem::treeWidget()->setItemExpanded(this, bOpen);
#endif
}


bool qjackctlSocketItem::isOpen (void) const
{
#if QT_VERSION >= 0x040201
	return QTreeWidgetItem::isExpanded();
#else
	return QTreeWidgetItem::treeWidget()->isItemExpanded(this);
#endif
}


//----------------------------------------------------------------------
// qjackctlSocketList -- Jack client list.
//

// Constructor.
qjackctlSocketList::qjackctlSocketList (
	qjackctlSocketListView *pListView, bool bReadable )
{
	QPixmap pmXSocket1(":/icons/xsocket1.png");

	m_pListView   = pListView;
	m_bReadable   = bReadable;
	m_pJackClient = NULL;
	m_pAlsaSeq    = NULL;

	if (bReadable) {
		m_sSocketCaption = tr("Output");
		m_apPixmaps[QJACKCTL_XPM_AUDIO_SOCKET]   = new QPixmap(":/icons/asocketo.png");
		m_apPixmaps[QJACKCTL_XPM_AUDIO_SOCKET_X] = createPixmapMerge(*m_apPixmaps[QJACKCTL_XPM_AUDIO_SOCKET], pmXSocket1);
		m_apPixmaps[QJACKCTL_XPM_AUDIO_CLIENT]   = new QPixmap(":/icons/acliento.png");
		m_apPixmaps[QJACKCTL_XPM_AUDIO_PLUG]     = new QPixmap(":/icons/aportlno.png");
		m_apPixmaps[QJACKCTL_XPM_MIDI_SOCKET]    = new QPixmap(":/icons/msocketo.png");
		m_apPixmaps[QJACKCTL_XPM_MIDI_SOCKET_X]  = createPixmapMerge(*m_apPixmaps[QJACKCTL_XPM_MIDI_SOCKET], pmXSocket1);
		m_apPixmaps[QJACKCTL_XPM_MIDI_CLIENT]    = new QPixmap(":/icons/mcliento.png");
		m_apPixmaps[QJACKCTL_XPM_MIDI_PLUG]      = new QPixmap(":/icons/mporto.png");
	} else {
		m_sSocketCaption = tr("Input");
		m_apPixmaps[QJACKCTL_XPM_AUDIO_SOCKET]   = new QPixmap(":/icons/asocketi.png");
		m_apPixmaps[QJACKCTL_XPM_AUDIO_SOCKET_X] = createPixmapMerge(*m_apPixmaps[QJACKCTL_XPM_AUDIO_SOCKET], pmXSocket1);
		m_apPixmaps[QJACKCTL_XPM_AUDIO_CLIENT]   = new QPixmap(":/icons/aclienti.png");
		m_apPixmaps[QJACKCTL_XPM_AUDIO_PLUG]     = new QPixmap(":/icons/aportlni.png");
		m_apPixmaps[QJACKCTL_XPM_MIDI_SOCKET]    = new QPixmap(":/icons/msocketi.png");
		m_apPixmaps[QJACKCTL_XPM_MIDI_SOCKET_X]  = createPixmapMerge(*m_apPixmaps[QJACKCTL_XPM_MIDI_SOCKET], pmXSocket1);
		m_apPixmaps[QJACKCTL_XPM_MIDI_CLIENT]    = new QPixmap(":/icons/mclienti.png");
		m_apPixmaps[QJACKCTL_XPM_MIDI_PLUG]      = new QPixmap(":/icons/mporti.png");
	}

	if (!m_sSocketCaption.isEmpty())
		m_sSocketCaption += ' ';
	m_sSocketCaption += tr("Socket");
}

// Default destructor.
qjackctlSocketList::~qjackctlSocketList (void)
{
	clear();

	for (int iPixmap = 0; iPixmap < QJACKCTL_XPM_PIXMAPS; iPixmap++)
		delete m_apPixmaps[iPixmap];
}


// Client finder.
qjackctlSocketItem *qjackctlSocketList::findSocket (
	const QString& sSocketName, int iSocketType )
{
	QListIterator<qjackctlSocketItem *> iter(m_sockets);
	while (iter.hasNext()) {
		qjackctlSocketItem *pSocket = iter.next();
		if (sSocketName == pSocket->socketName() &&
			iSocketType == pSocket->socketType())
			return pSocket;
	}

	return NULL;
}


// Socket list accessor.
QList<qjackctlSocketItem *>& qjackctlSocketList::sockets (void)
{
	return m_sockets;
}


// List view accessor.
qjackctlSocketListView *qjackctlSocketList::listView (void) const
{
	return m_pListView;
}


// Socket flags accessor.
bool qjackctlSocketList::isReadable (void) const
{
	return m_bReadable;
}


// Socket caption titler.
const QString& qjackctlSocketList::socketCaption (void) const
{
	return m_sSocketCaption;
}


// JACK client accessors.
void qjackctlSocketList::setJackClient ( jack_client_t *pJackClient )
{
	m_pJackClient = pJackClient;
}

jack_client_t *qjackctlSocketList::jackClient (void) const
{
	return m_pJackClient;
}


// ALSA sequencer accessors.
void qjackctlSocketList::setAlsaSeq ( snd_seq_t *pAlsaSeq )
{
	m_pAlsaSeq = pAlsaSeq;
}

snd_seq_t *qjackctlSocketList::alsaSeq (void) const
{
	return m_pAlsaSeq;
}


// Socket list cleaner.
void qjackctlSocketList::clear (void)
{
	qDeleteAll(m_sockets);
	m_sockets.clear();
}


// Socket list pixmap peeker.
const QPixmap& qjackctlSocketList::pixmap ( int iPixmap ) const
{
	return *m_apPixmaps[iPixmap];
}


// Merge two pixmaps with union of respective masks.
QPixmap *qjackctlSocketList::createPixmapMerge (
	const QPixmap& xpmDst, const QPixmap& xpmSrc )
{
	QPixmap *pXpmMerge = new QPixmap(xpmDst);
	if (pXpmMerge) {
		QBitmap bmMask = xpmDst.mask();
		QPainter(&bmMask).drawPixmap(0, 0, xpmSrc.mask());
		pXpmMerge->setMask(bmMask);
		QPainter(pXpmMerge).drawPixmap(0, 0, xpmSrc);
	}
	return pXpmMerge;
}


// JACK specific client:port connections snapshot.
void qjackctlSocketList::clientJackSnapshot ( int iSocketType )
{
	// Grab JACK client:ports...
	if (m_pJackClient == NULL)
		return;

	const char *pszJackPortType = JACK_DEFAULT_AUDIO_TYPE;
#ifdef CONFIG_JACK_MIDI
	if (iSocketType == QJACKCTL_SOCKETTYPE_JACK_MIDI)
		pszJackPortType = JACK_DEFAULT_MIDI_TYPE;
#endif

	const char **ppszClientPorts = jack_get_ports(m_pJackClient,
		NULL, pszJackPortType,
		(m_bReadable ? JackPortIsOutput : JackPortIsInput));
	if (ppszClientPorts) {
		int iClientPort = 0;
		while (ppszClientPorts[iClientPort]) {
			QString sClientPort = ppszClientPorts[iClientPort];
			qjackctlSocketItem *pSocket = NULL;
			qjackctlPlugItem   *pPlug   = NULL;
			int iColon = sClientPort.indexOf(':');
			if (iColon >= 0) {
				QString sClientName = sClientPort.left(iColon);
				QString sPortName = qjackctlClientAlias::escapeRegExpDigits(
					sClientPort.right(sClientPort.length() - iColon - 1));
				pSocket = findSocket(sClientName, iSocketType);
				if (pSocket)
					pPlug = pSocket->findPlug(sPortName);
				if (pSocket == NULL) {
					int iSocketCount = listView()->topLevelItemCount();
					if (iSocketCount > 0) {
						pSocket = static_cast<qjackctlSocketItem *> (
							listView()->topLevelItem(iSocketCount - 1));
					}
					pSocket = new qjackctlSocketItem(this, sClientName,
						qjackctlClientAlias::escapeRegExpDigits(sClientName),
						iSocketType, pSocket);
				}
				if (pSocket && pPlug == NULL) {
					int iPlugCount = pSocket->childCount();
					if (iPlugCount > 0) {
						pPlug = static_cast<qjackctlPlugItem *> (
							pSocket->child(iPlugCount - 1));
					}
					pPlug = new qjackctlPlugItem(pSocket, sPortName, pPlug);
				}
			}
			iClientPort++;
		}
		::free(ppszClientPorts);
	}
}

// ALSA specific client:port connections snapshot.
void qjackctlSocketList::clientAlsaSnapshot ( int iSocketType )
{
	if (m_pAlsaSeq == NULL)
		return;

#ifdef CONFIG_ALSA_SEQ

	// Grab ALSA subscribers...
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
		if (iAlsaClient > 0) {
			QString sClientName = snd_seq_client_info_get_name(pClientInfo);
			snd_seq_port_info_set_client(pPortInfo, iAlsaClient);
			snd_seq_port_info_set_port(pPortInfo, -1);
			while (snd_seq_query_next_port(m_pAlsaSeq, pPortInfo) >= 0) {
				unsigned int uiPortCapability
					= snd_seq_port_info_get_capability(pPortInfo);
				if (((uiPortCapability & uiAlsaFlags) == uiAlsaFlags) &&
					((uiPortCapability & SND_SEQ_PORT_CAP_NO_EXPORT) == 0)) {
					QString sPortName = qjackctlClientAlias::escapeRegExpDigits(
						snd_seq_port_info_get_name(pPortInfo));
					qjackctlSocketItem *pSocket = NULL;
					qjackctlPlugItem   *pPlug   = NULL;
					pSocket = findSocket(sClientName, iSocketType);
					if (pSocket)
						pPlug = pSocket->findPlug(sPortName);
					if (pSocket == NULL) {
						int iSocketCount = listView()->topLevelItemCount();
						if (iSocketCount > 0) {
							pSocket = static_cast<qjackctlSocketItem *> (
								listView()->topLevelItem(iSocketCount - 1));
						}
						pSocket = new qjackctlSocketItem(this, sClientName,
							qjackctlClientAlias::escapeRegExpDigits(sClientName),
							iSocketType, pSocket);
					}
					if (pSocket && pPlug == NULL) {
						int iPlugCount = pSocket->childCount();
						if (iPlugCount > 0) {
							pPlug = static_cast<qjackctlPlugItem *> (
								pSocket->child(iPlugCount - 1));
						}
						pPlug = new qjackctlPlugItem(pSocket, sPortName, pPlug);
					}
				}
			}
		}
	}

#endif	// CONFIG_ALSA_SEQ
}


// General client:port connections snapshot.
void qjackctlSocketList::clientPortsSnapshot (void)
{
	clientJackSnapshot(QJACKCTL_SOCKETTYPE_JACK_AUDIO);

#ifdef CONFIG_JACK_MIDI
	clientJackSnapshot(QJACKCTL_SOCKETTYPE_JACK_MIDI);
#endif
#ifdef CONFIG_ALSA_SEQ
	clientAlsaSnapshot(QJACKCTL_SOCKETTYPE_ALSA_MIDI);
#endif
}


// Return currently selected socket item.
qjackctlSocketItem *qjackctlSocketList::selectedSocketItem (void) const
{
	qjackctlSocketItem *pSocketItem = NULL;

	QTreeWidgetItem *pItem = m_pListView->currentItem();
	if (pItem) {
		if (pItem->type() == QJACKCTL_PLUGITEM) {
			pSocketItem = static_cast<qjackctlSocketItem *> (pItem->parent());
		} else {
			pSocketItem = static_cast<qjackctlSocketItem *> (pItem);
		}
	}

	return pSocketItem;
}


// Add a new socket item, using interactive form.
bool qjackctlSocketList::addSocketItem (void)
{
	bool bResult = false;

	qjackctlSocketForm socketForm(m_pListView);
	socketForm.setWindowTitle(tr("<New> - %1").arg(m_sSocketCaption));
	socketForm.setSocketCaption(m_sSocketCaption);
	socketForm.setPixmaps(m_apPixmaps);
	socketForm.setSocketList(this);
	socketForm.setJackClient(m_pJackClient);
	socketForm.setAlsaSeq(m_pAlsaSeq);
	qjackctlPatchbaySocket socket(m_sSocketCaption
		+ ' ' + QString::number(m_sockets.count() + 1),
		QString::null, QJACKCTL_SOCKETTYPE_JACK_AUDIO);
	socketForm.load(&socket);
	if (socketForm.exec()) {
		socketForm.save(&socket);
		qjackctlSocketItem *pSocketItem = selectedSocketItem();
	//  m_pListView->setUpdatesEnabled(false);
		if (pSocketItem)
#if QT_VERSION >= 0x040200
			pSocketItem->setSelected(false);
#else
			m_pListView->setItemSelected(pSocketItem, false);
#endif
		pSocketItem = new qjackctlSocketItem(this, socket.name(),
			socket.clientName(), socket.type(), pSocketItem);
		if (pSocketItem) {
			pSocketItem->setExclusive(socket.isExclusive());
			pSocketItem->setForward(socket.forward());
			qjackctlPlugItem *pPlugItem = NULL;
			QStringListIterator iter(socket.pluglist());
			while (iter.hasNext()) {
				pPlugItem = new qjackctlPlugItem(
					pSocketItem, iter.next(), pPlugItem);
			}
			bResult = true;
		}
#if QT_VERSION >= 0x040200
		pSocketItem->setSelected(true);
#else
		m_pListView->setItemSelected(pSocketItem, true);
#endif
		m_pListView->setCurrentItem(pSocketItem);
	//  m_pListView->setUpdatesEnabled(true);
	//  m_pListView->update();
		m_pListView->setDirty(true);
	}

	return bResult;
}


// Remove (delete) currently selected socket item.
bool qjackctlSocketList::removeSocketItem (void)
{
	bool bResult = false;

	qjackctlSocketItem *pSocketItem = selectedSocketItem();
	if (pSocketItem) {
		if (QMessageBox::warning(m_pListView,
			tr("Warning") + " - " QJACKCTL_SUBTITLE1,
			tr("%1 about to be removed:\n\n"
			"\"%2\"\n\nAre you sure?")
			.arg(m_sSocketCaption)
			.arg(pSocketItem->socketName()),
			tr("Yes"), tr("No")) == 0) {
			delete pSocketItem;
			bResult = true;
			m_pListView->setDirty(true);
		}
	}

	return bResult;
}


// View or change the properties of currently selected socket item.
bool qjackctlSocketList::editSocketItem (void)
{
	bool bResult = false;

	qjackctlSocketItem *pSocketItem = selectedSocketItem();
	if (pSocketItem) {
		qjackctlSocketForm socketForm(m_pListView);
		socketForm.setWindowTitle(pSocketItem->socketName()
			+ " - " + m_sSocketCaption);
		socketForm.setSocketCaption(m_sSocketCaption);
		socketForm.setPixmaps(m_apPixmaps);
		socketForm.setSocketList(this);
		socketForm.setJackClient(m_pJackClient);
		socketForm.setAlsaSeq(m_pAlsaSeq);
		qjackctlPatchbaySocket socket(pSocketItem->socketName(),
			pSocketItem->clientName(), pSocketItem->socketType());
		socket.setExclusive(pSocketItem->isExclusive());
		socket.setForward(pSocketItem->forward());
		QListIterator<qjackctlPlugItem *> iter(pSocketItem->plugs());
		while (iter.hasNext())
			socket.pluglist().append((iter.next())->plugName());
		socketForm.load(&socket);
		socketForm.setConnectCount(pSocketItem->connects().count());
		if (socketForm.exec()) {
			socketForm.save(&socket);
		//  m_pListView->setUpdatesEnabled(false);
			pSocketItem->clear();
			pSocketItem->setText(0, socket.name());
			pSocketItem->setSocketName(socket.name());
			pSocketItem->setClientName(socket.clientName());
			pSocketItem->setSocketType(socket.type());
			pSocketItem->setExclusive(socket.isExclusive());
			pSocketItem->setForward(socket.forward());
			pSocketItem->updatePixmap();
			qjackctlPlugItem *pPlugItem = NULL;
			QStringListIterator iter(socket.pluglist());
			while (iter.hasNext()) {
				pPlugItem = new qjackctlPlugItem(
					pSocketItem, iter.next(), pPlugItem);
			}
#if QT_VERSION >= 0x040200
			pSocketItem->setSelected(true);
#else
			m_pListView->setItemSelected(pSocketItem, true);
#endif
			m_pListView->setCurrentItem(pSocketItem);
		//  m_pListView->setUpdatesEnabled(true);
		//  m_pListView->triggerUpdate();
			m_pListView->setDirty(true);
			bResult = true;
		}
	}

	return bResult;
}


// Duplicate and change the properties of currently selected socket item.
bool qjackctlSocketList::copySocketItem (void)
{
	bool bResult = false;

	qjackctlSocketItem *pSocketItem = selectedSocketItem();
	if (pSocketItem) {
		qjackctlSocketForm socketForm(m_pListView);
		// Find a new distinguishable socket name, please.
		QString sSocketName;
		QString sSkel = pSocketItem->socketName();
		sSkel.remove(QRegExp("[0-9]+$")).append("%1");
		int iSocketType = pSocketItem->socketType();
		int iSocketNo = 1;
		do { sSocketName = sSkel.arg(++iSocketNo); }
		while (findSocket(sSocketName, iSocketType));
		// Show up as a new socket...
		socketForm.setWindowTitle(tr("%1 <Copy> - %2")
			.arg(pSocketItem->socketName()).arg(m_sSocketCaption));
		socketForm.setSocketCaption(m_sSocketCaption);
		socketForm.setPixmaps(m_apPixmaps);
		socketForm.setSocketList(this);
		socketForm.setJackClient(m_pJackClient);
		socketForm.setAlsaSeq(m_pAlsaSeq);
		qjackctlPatchbaySocket socket(sSocketName,
			pSocketItem->clientName(), iSocketType);
		if (pSocketItem->isExclusive())
			socket.setExclusive(true);
		QListIterator<qjackctlPlugItem *> iter(pSocketItem->plugs());
		while (iter.hasNext())
			socket.pluglist().append((iter.next())->plugName());
		socketForm.load(&socket);
		if (socketForm.exec()) {
			socketForm.save(&socket);
			pSocketItem = new qjackctlSocketItem(this, socket.name(),
				socket.clientName(), socket.type(), pSocketItem);
			if (pSocketItem) {
				pSocketItem->setExclusive(socket.isExclusive());
				pSocketItem->setForward(socket.forward());
				qjackctlPlugItem *pPlugItem = NULL;
				QStringListIterator iter(socket.pluglist());
				while (iter.hasNext()) {
					pPlugItem = new qjackctlPlugItem(
						pSocketItem, iter.next(), pPlugItem);
				}
				bResult = true;
			}
#if QT_VERSION >= 0x040200
			pSocketItem->setSelected(true);
#else
			m_pListView->setItemSelected(pSocketItem, true);
#endif
			m_pListView->setCurrentItem(pSocketItem);
		//  m_pListView->setUpdatesEnabled(true);
		//  m_pListView->triggerUpdate();
			m_pListView->setDirty(true);
		}
	}

	return bResult;
}


// Toggle exclusive currently selected socket item.
bool qjackctlSocketList::exclusiveSocketItem (void)
{
	bool bResult = false;

	qjackctlSocketItem *pSocketItem = selectedSocketItem();
	if (pSocketItem) {
		pSocketItem->setExclusive(!pSocketItem->isExclusive());
		pSocketItem->updatePixmap();
		bResult = true;
		m_pListView->setDirty(true);
	}

	return bResult;
}


// Move current selected socket item up one position.
bool qjackctlSocketList::moveUpSocketItem (void)
{
	bool bResult = false;

	qjackctlSocketItem *pSocketItem = selectedSocketItem();
	if (pSocketItem) {
		int iItem = m_pListView->indexOfTopLevelItem(pSocketItem);
		if (iItem > 0) {
			QTreeWidgetItem *pItem = m_pListView->takeTopLevelItem(iItem);
			if (pItem) {
				m_pListView->insertTopLevelItem(iItem - 1, pItem);
#if QT_VERSION >= 0x040200
				pSocketItem->setSelected(true);
#else
				m_pListView->setItemSelected(pSocketItem, true);
#endif
				m_pListView->setCurrentItem(pSocketItem);
			//  m_pListView->setUpdatesEnabled(true);
			//  m_pListView->update();
				m_pListView->setDirty(true);
				bResult = true;
			}
		}
	}

	return bResult;
}


// Move current selected socket item down one position.
bool qjackctlSocketList::moveDownSocketItem (void)
{
	bool bResult = false;

	qjackctlSocketItem *pSocketItem = selectedSocketItem();
	if (pSocketItem) {
		int iItem = m_pListView->indexOfTopLevelItem(pSocketItem);
		int iItemCount = m_pListView->topLevelItemCount();
		if (iItem < iItemCount - 1) {
			QTreeWidgetItem *pItem = m_pListView->takeTopLevelItem(iItem);
			if (pItem) {
				m_pListView->insertTopLevelItem(iItem + 1, pItem);
#if QT_VERSION >= 0x040200
				pSocketItem->setSelected(true);
#else
				m_pListView->setItemSelected(pSocketItem, true);
#endif
				m_pListView->setCurrentItem(pSocketItem);
			//  m_pListView->setUpdatesEnabled(true);
			//  m_pListView->update();
				m_pListView->setDirty(true);
				bResult = true;
			}
		}
	}

	return bResult;
}


//----------------------------------------------------------------------------
// qjackctlSocketListView -- Socket list view, supporting drag-n-drop.

// Constructor.
qjackctlSocketListView::qjackctlSocketListView (
	qjackctlPatchbayView *pPatchbayView, bool bReadable )
	: QTreeWidget(pPatchbayView)
{
	m_pPatchbayView = pPatchbayView;
	m_bReadable     = bReadable;

	m_pAutoOpenTimer   = 0;
	m_iAutoOpenTimeout = 0;

	m_pDragItem = NULL;
	m_pDropItem = NULL;

	QHeaderView *pHeader = QTreeWidget::header();
//	pHeader->setResizeMode(QHeaderView::Custom);
//	pHeader->setDefaultAlignment(Qt::AlignLeft);
//	pHeader->setDefaultSectionSize(120);
	pHeader->setMovable(false);
//	pHeader->setClickable(true);
//	pHeader->setSortIndicatorShown(true);
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
	QTreeWidget::setSortingEnabled(false);
	QTreeWidget::setMinimumWidth(120);
	QTreeWidget::setColumnCount(1);

	// Trap for help/tool-tips events.
	QTreeWidget::viewport()->installEventFilter(this);

	QString sText;
	if (m_bReadable)
		sText = tr("Output Sockets / Plugs");
	else
		sText = tr("Input Sockets / Plugs");
	QTreeWidget::headerItem()->setText(0, sText);
	QTreeWidget::setToolTip(sText);

	setAutoOpenTimeout(800);
}

// Default destructor.
qjackctlSocketListView::~qjackctlSocketListView (void)
{
	setAutoOpenTimeout(0);
}


// Patchbay view dirty flag accessors.
void qjackctlSocketListView::setDirty ( bool bDirty )
{
	m_pPatchbayView->setDirty(bDirty);
}

bool qjackctlSocketListView::dirty (void) const
{
	return m_pPatchbayView->dirty();
}


// Auto-open timeout method.
void qjackctlSocketListView::setAutoOpenTimeout ( int iAutoOpenTimeout )
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
int qjackctlSocketListView::autoOpenTimeout (void) const
{
	return m_iAutoOpenTimeout;
}


// Auto-open timer slot.
void qjackctlSocketListView::timeoutSlot (void)
{
	if (m_pAutoOpenTimer) {
		m_pAutoOpenTimer->stop();
		if (m_pDropItem && m_pDropItem->type() == QJACKCTL_SOCKETITEM) {
			qjackctlSocketItem *pSocketItem
				= static_cast<qjackctlSocketItem *> (m_pDropItem);
			if (pSocketItem && !pSocketItem->isOpen())
				pSocketItem->setOpen(true);
		}
	}
}


// Trap for help/tool-tip events.
bool qjackctlSocketListView::eventFilter ( QObject *pObject, QEvent *pEvent )
{
	QWidget *pViewport = QTreeWidget::viewport();
	if (static_cast<QWidget *> (pObject) == pViewport
		&& pEvent->type() == QEvent::ToolTip) {
		QHelpEvent *pHelpEvent = static_cast<QHelpEvent *> (pEvent);
		if (pHelpEvent) {
			QTreeWidgetItem *pItem = QTreeWidget::itemAt(pHelpEvent->pos());
			if (pItem && pItem->type() == QJACKCTL_SOCKETITEM) {
				qjackctlSocketItem *pSocketItem
					= static_cast<qjackctlSocketItem *> (pItem);
				if (pSocketItem) {
					QToolTip::showText(pHelpEvent->globalPos(),
						pSocketItem->clientName(), pViewport);
					return true;
				}
			}
			else
			if (pItem && pItem->type() == QJACKCTL_PLUGITEM) {
				qjackctlPlugItem *pPlugItem
					= static_cast<qjackctlPlugItem *> (pItem);
				if (pPlugItem) {
					QToolTip::showText(pHelpEvent->globalPos(),
						pPlugItem->plugName(), pViewport);
					return true;
				}
			}
		}
	}

	// Not handled here.
	return QTreeWidget::eventFilter(pObject, pEvent);
}


// Drag-n-drop stuff.
QTreeWidgetItem *qjackctlSocketListView::dragDropItem ( const QPoint& pos )
{
	QTreeWidgetItem *pItem = QTreeWidget::itemAt(pos);
	if (pItem) {
		if (m_pDropItem != pItem) {
			QTreeWidget::setCurrentItem(pItem);
			m_pDropItem = pItem;
			if (m_pAutoOpenTimer)
				m_pAutoOpenTimer->start(m_iAutoOpenTimeout);
			qjackctlPatchbay *pPatchbay = m_pPatchbayView->binding();
			if ((pItem->flags() & Qt::ItemIsDropEnabled) == 0
				|| pPatchbay == NULL || !pPatchbay->canConnectSelected())
				pItem = NULL;
		}
	} else {
		m_pDropItem = NULL;
		if (m_pAutoOpenTimer)
			m_pAutoOpenTimer->stop();
	}

	return pItem;
}

void qjackctlSocketListView::dragEnterEvent ( QDragEnterEvent *pDragEnterEvent )
{
	if (pDragEnterEvent->source() != this &&
		pDragEnterEvent->mimeData()->hasText() &&
		dragDropItem(pDragEnterEvent->pos())) {
		pDragEnterEvent->accept();
	} else {
		pDragEnterEvent->ignore();
	}
}


void qjackctlSocketListView::dragMoveEvent ( QDragMoveEvent *pDragMoveEvent )
{
	if (pDragMoveEvent->source() != this &&
		pDragMoveEvent->mimeData()->hasText() &&
		dragDropItem(pDragMoveEvent->pos())) {
		pDragMoveEvent->accept();
	} else {
		pDragMoveEvent->ignore();
	}
}


void qjackctlSocketListView::dragLeaveEvent ( QDragLeaveEvent * )
{
	m_pDropItem = 0;
	if (m_pAutoOpenTimer)
		m_pAutoOpenTimer->stop();
}


void qjackctlSocketListView::dropEvent ( QDropEvent *pDropEvent )
{
	if (pDropEvent->source() != this &&
		pDropEvent->mimeData()->hasText() &&
		dragDropItem(pDropEvent->pos())) {
		const QString sText = pDropEvent->mimeData()->text();
		qjackctlPatchbay *pPatchbay = m_pPatchbayView->binding();
		if (!sText.isEmpty() && pPatchbay)
			pPatchbay->connectSelected();
	}

	dragLeaveEvent(NULL);
}


// Handle mouse events for drag-and-drop stuff.
void qjackctlSocketListView::mousePressEvent ( QMouseEvent *pMouseEvent )
{
	QTreeWidget::mousePressEvent(pMouseEvent);

	if (pMouseEvent->button() == Qt::LeftButton) {
		m_posDrag   = pMouseEvent->pos();
		m_pDragItem = QTreeWidget::itemAt(m_posDrag);
	}
}


void qjackctlSocketListView::mouseMoveEvent ( QMouseEvent *pMouseEvent )
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
void qjackctlSocketListView::contextMenuEvent (
	QContextMenuEvent *pContextMenuEvent )
{
	m_pPatchbayView->contextMenu(
		pContextMenuEvent->globalPos(),
		(m_bReadable
			? m_pPatchbayView->OSocketList()
			: m_pPatchbayView->ISocketList())
	);
}


//----------------------------------------------------------------------
// qjackctlPatchworkView -- Socket connector widget.
//

// Constructor.
qjackctlPatchworkView::qjackctlPatchworkView (
	qjackctlPatchbayView *pPatchbayView )
	: QWidget(pPatchbayView)
{
	m_pPatchbayView = pPatchbayView;

	QWidget::setMinimumWidth(20);
//	QWidget::setMaximumWidth(120);
	QWidget::setSizePolicy(
		QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
}

// Default destructor.
qjackctlPatchworkView::~qjackctlPatchworkView (void)
{
}


// Legal socket item position helper.
int qjackctlPatchworkView::itemY ( QTreeWidgetItem *pItem ) const
{
	QRect rect;
	QTreeWidget *pList = pItem->treeWidget();
	QTreeWidgetItem *pParent = pItem->parent();
	qjackctlSocketItem *pSocketItem = NULL;
	if (pParent && pParent->type() == QJACKCTL_SOCKETITEM)
		pSocketItem = static_cast<qjackctlSocketItem *> (pParent);
	if (pSocketItem && !pSocketItem->isOpen()) {
		rect = pList->visualItemRect(pParent);
	} else {
		rect = pList->visualItemRect(pItem);
	}
	return rect.top() + rect.height() / 2;
}


// Draw visible socket connection relation lines
void qjackctlPatchworkView::drawConnectionLine ( QPainter *pPainter,
	int x1, int y1, int x2, int y2, int h1, int h2 )
{
	// Account for list view headers.
	y1 += h1;
	y2 += h2;

	// Invisible output plugs don't get a connecting dot.
	if (y1 > h1)
		pPainter->drawLine(x1, y1, x1 + 4, y1);

	// How do we'll draw it?
	if (m_pPatchbayView->isBezierLines()) {
		// Setup control points
		QPolygon spline(4);
		int cp = int(float(x2 - x1 - 8) * 0.4f);
		spline.putPoints(0, 4,
			x1 + 4, y1, x1 + 4 + cp, y1, 
			x2 - 4 - cp, y2, x2 - 4, y2);
		// The connection line, it self.
		QPainterPath path;
		path.moveTo(spline.at(0));
		path.cubicTo(spline.at(1), spline.at(2), spline.at(3));
		pPainter->strokePath(path, pPainter->pen());
	}   // Old style...
	else pPainter->drawLine(x1 + 4, y1, x2 - 4, y2);

	// Invisible input plugs don't get a connecting dot.
	if (y2 > h2)
		pPainter->drawLine(x2 - 4, y2, x2, y2);
}


// Draw socket forwrading line (for input sockets / right pane only)
void qjackctlPatchworkView::drawForwardLine ( QPainter *pPainter,
	int x, int dx, int y1, int y2, int h )
{
	// Account for list view headers.
	y1 += h;
	y2 += h;
	dx += 4;

	// Draw it...
	if (y1 < y2) {
		pPainter->drawLine(x - dx, y1 + 4, x, y1);
		pPainter->drawLine(x - dx, y1 + 4, x - dx, y2 - 4);
		pPainter->drawLine(x - dx, y2 - 4, x, y2);
		// Down arrow...
		pPainter->drawLine(x - dx, y2 - 8, x - dx - 2, y2 - 12);
		pPainter->drawLine(x - dx, y2 - 8, x - dx + 2, y2 - 12);
	} else {
		pPainter->drawLine(x - dx, y1 - 4, x, y1);
		pPainter->drawLine(x - dx, y1 - 4, x - dx, y2 + 4);
		pPainter->drawLine(x - dx, y2 + 4, x, y2);
		// Up arrow...
		pPainter->drawLine(x - dx, y2 + 8, x - dx - 2, y2 + 12);
		pPainter->drawLine(x - dx, y2 + 8, x - dx + 2, y2 + 12);
	}
}


// Draw visible socket connection relation arrows.
void qjackctlPatchworkView::paintEvent ( QPaintEvent * )
{
	if (m_pPatchbayView->OSocketList() == NULL ||
		m_pPatchbayView->ISocketList() == NULL)
		return;

	QPainter painter(this);
	int x1, y1, h1;
	int x2, y2, h2;
	int i, rgb[3] = { 0x99, 0x66, 0x33 };

	// Inline adaptive to darker background themes...
	if (QWidget::palette().window().color().value() < 0x7f)
		for (i = 0; i < 3; ++i) rgb[i] += 0x33;

	// Initialize color changer.
	i = 0;
	// Almost constants.
	x1 = 0;
	x2 = width();
	h1 = ((m_pPatchbayView->OListView())->header())->sizeHint().height();
	h2 = ((m_pPatchbayView->IListView())->header())->sizeHint().height();
	// For each client item...
	qjackctlSocketItem *pOSocket, *pISocket;
	QListIterator<qjackctlSocketItem *> osocket(
		(m_pPatchbayView->OSocketList())->sockets());
	while (osocket.hasNext()) {
		pOSocket = osocket.next();
		// Set new connector color.
		++i;
		painter.setPen(QColor(rgb[i % 3], rgb[(i / 3) % 3], rgb[(i / 9) % 3]));
		// Get starting connector arrow coordinates.
		y1 = itemY(pOSocket);
		// Get input socket connections...
		QListIterator<qjackctlSocketItem *> isocket(pOSocket->connects());
		while (isocket.hasNext()) {
			pISocket = isocket.next();
			// Obviously, there is a connection from pOPlug to pIPlug items:
			y2 = itemY(pISocket);
			drawConnectionLine(&painter, x1, y1, x2, y2, h1, h2);
		}
	}

	// Look for forwarded inputs...
	QList<qjackctlSocketItem *> iforwards;
	// Make a local copy of just the forwarding socket list, if any...
	QListIterator<qjackctlSocketItem *> isocket(
		(m_pPatchbayView->ISocketList())->sockets());
	while (isocket.hasNext()) {
		pISocket = isocket.next();
		// Check if its forwarded...
		if (pISocket->forward().isEmpty())
			continue;
		iforwards.append(pISocket);
	}
	// (Re)initialize color changer.
	i = 0;
	// Now traverse those for proper connection drawing...
	int dx = 0;
	QListIterator<qjackctlSocketItem *> iter(iforwards);
	while (iter.hasNext()) {
		pISocket = iter.next();
		qjackctlSocketItem *pISocketForward
			= m_pPatchbayView->ISocketList()->findSocket(
				pISocket->forward(), pISocket->socketType());
		if (pISocketForward == NULL)
			continue;
		// Set new connector color.
		++i;
		painter.setPen(QColor(rgb[i % 3], rgb[(i / 3) % 3], rgb[(i / 9) % 3]));
		// Get starting connector arrow coordinates.
		y1 = itemY(pISocketForward);
		y2 = itemY(pISocket);
		drawForwardLine(&painter, x2, dx, y1, y2, h2);
		dx += 2;
	}
}


// Context menu request event handler.
void qjackctlPatchworkView::contextMenuEvent (
	QContextMenuEvent *pContextMenuEvent )
{
	m_pPatchbayView->contextMenu(pContextMenuEvent->globalPos(), NULL);
}


// Widget event slots...

void qjackctlPatchworkView::contentsChanged (void)
{
	QWidget::update();
}


//----------------------------------------------------------------------------
// qjackctlPatchbayView -- Integrated patchbay widget.

// Constructor.
qjackctlPatchbayView::qjackctlPatchbayView ( QWidget *pParent )
	: QSplitter(Qt::Horizontal, pParent)
{
	m_pOListView = new qjackctlSocketListView(this, true);
	m_pPatchworkView = new qjackctlPatchworkView(this);
	m_pIListView = new qjackctlSocketListView(this, false);

	m_pPatchbay = NULL;

	m_bBezierLines = false;

	QObject::connect(m_pOListView, SIGNAL(itemExpanded(QTreeWidgetItem *)),
		m_pPatchworkView, SLOT(contentsChanged()));
	QObject::connect(m_pOListView, SIGNAL(itemCollapsed(QTreeWidgetItem *)),
		m_pPatchworkView, SLOT(contentsChanged()));
	QObject::connect(m_pOListView->verticalScrollBar(), SIGNAL(valueChanged(int)),
		m_pPatchworkView, SLOT(contentsChanged()));
//	QObject::connect(m_pOListView->header(), SIGNAL(sectionClicked(int)),
//		m_pPatchworkView, SLOT(contentsChanged()));

	QObject::connect(m_pIListView, SIGNAL(itemExpanded(QTreeWidgetItem *)),
		m_pPatchworkView, SLOT(contentsChanged()));
	QObject::connect(m_pIListView, SIGNAL(itemCollapsed(QTreeWidgetItem *)),
		m_pPatchworkView, SLOT(contentsChanged()));
	QObject::connect(m_pIListView->verticalScrollBar(), SIGNAL(valueChanged(int)),
		m_pPatchworkView, SLOT(contentsChanged()));
//	QObject::connect(m_pIListView->header(), SIGNAL(sectionClicked(int)),
//		m_pPatchworkView, SLOT(contentsChanged()));

	m_bDirty = false;
}


// Default destructor.
qjackctlPatchbayView::~qjackctlPatchbayView (void)
{
}


// Common context menu slot.
void qjackctlPatchbayView::contextMenu ( const QPoint& pos,
	qjackctlSocketList *pSocketList )
{
	qjackctlPatchbay *pPatchbay = binding();
	if (pPatchbay == NULL)
		return;

	QMenu menu(this);
	QAction *pAction;

	if (pSocketList) {
		qjackctlSocketItem *pSocketItem = pSocketList->selectedSocketItem();
		bool bEnabled = (pSocketItem != NULL);
		pAction = menu.addAction(QIcon(":/icons/add1.png"),
			tr("Add..."), pSocketList, SLOT(addSocketItem()));
		pAction = menu.addAction(QIcon(":/icons/edit1.png"),
			tr("Edit..."), pSocketList, SLOT(editSocketItem()));
		pAction->setEnabled(bEnabled);
		pAction = menu.addAction(QIcon(":/icons/copy1.png"),
			tr("Copy..."), pSocketList, SLOT(copySocketItem()));
		pAction->setEnabled(bEnabled);
		pAction = menu.addAction(QIcon(":/icons/remove1.png"),
			tr("Remove"), pSocketList, SLOT(removeSocketItem()));
		pAction->setEnabled(bEnabled);
		menu.addSeparator();
		pAction = menu.addAction(
			tr("Exclusive"), pSocketList, SLOT(exclusiveSocketItem()));
		pAction->setCheckable(true);
		pAction->setChecked(bEnabled && pSocketItem->isExclusive());
		pAction->setEnabled(bEnabled /* && (pSocketItem->connects().count() < 2) */);
		// Construct the forwarding menu,
		// overriding the last one, if any...
		QMenu *pForwardMenu = menu.addMenu(tr("Forward"));
		// Assume sockets iteration follows item index order (0,1,2...)
		// and remember that we only do this for input sockets...
		int iIndex = 0;
		if (pSocketItem && pSocketList == ISocketList()) {
			QListIterator<qjackctlSocketItem *> isocket(ISocketList()->sockets());
			while (isocket.hasNext()) {
				qjackctlSocketItem *pISocket = isocket.next();
				// Must be of same type of target one...
				int iSocketType = pISocket->socketType();
				if (iSocketType != pSocketItem->socketType())
					continue;
				const QString& sSocketName = pISocket->socketName();
				if (pSocketItem->socketName() == sSocketName)
					continue;
				int iPixmap = 0;
				switch (iSocketType) {
				case QJACKCTL_SOCKETTYPE_JACK_AUDIO:
					iPixmap = (pISocket->isExclusive()
						? QJACKCTL_XPM_AUDIO_SOCKET_X
						: QJACKCTL_XPM_AUDIO_SOCKET);
					break;
				case QJACKCTL_SOCKETTYPE_JACK_MIDI:
				case QJACKCTL_SOCKETTYPE_ALSA_MIDI:
					iPixmap = (pISocket->isExclusive()
						? QJACKCTL_XPM_MIDI_SOCKET_X
						: QJACKCTL_XPM_MIDI_SOCKET);
					break;
				}
				pAction = pForwardMenu->addAction(
					QIcon(ISocketList()->pixmap(iPixmap)), sSocketName);
				pAction->setChecked(pSocketItem->forward() == sSocketName);
				pAction->setData(iIndex);
				iIndex++;
			}
			// Null forward always present,
			// and has invalid index parameter (-1)...
			if (iIndex > 0)
				pForwardMenu->addSeparator();
			pAction = pForwardMenu->addAction(tr("(None)"));
			pAction->setCheckable(true);
			pAction->setChecked(pSocketItem->forward().isEmpty());
			pAction->setData(-1);
			// We have something here...
			QObject::connect(pForwardMenu,
				SIGNAL(triggered(QAction*)),
				SLOT(activateForwardMenu(QAction*)));
		}
		pForwardMenu->setEnabled(iIndex > 0);
		menu.addSeparator();
		int iItem = (pSocketList->listView())->indexOfTopLevelItem(pSocketItem);
		int iItemCount = (pSocketList->listView())->topLevelItemCount();
		pAction = menu.addAction(QIcon(":/icons/up1.png"),
			tr("Move Up"), pSocketList, SLOT(moveUpSocketItem()));
		pAction->setEnabled(bEnabled && iItem > 0);
		pAction = menu.addAction(QIcon(":/icons/down1.png"),
			tr("Move Down"), pSocketList, SLOT(moveDownSocketItem()));
		pAction->setEnabled(bEnabled && iItem < iItemCount - 1);
		menu.addSeparator();
	}

	pAction = menu.addAction(QIcon(":/icons/connect1.png"),
		tr("&Connect"), pPatchbay, SLOT(connectSelected()),
		tr("Alt+C", "Connect"));
	pAction->setEnabled(pPatchbay->canConnectSelected());
	pAction = menu.addAction(QIcon(":/icons/disconnect1.png"),
		tr("&Disconnect"), pPatchbay, SLOT(disconnectSelected()),
		tr("Alt+D", "Disconnect"));
	pAction->setEnabled(pPatchbay->canDisconnectSelected());
	pAction = menu.addAction(QIcon(":/icons/disconnectall1.png"),
		tr("Disconnect &All"), pPatchbay, SLOT(disconnectAll()),
		tr("Alt+A", "Disconect All"));
	pAction->setEnabled(pPatchbay->canDisconnectAll());

	menu.addSeparator();
	pAction = menu.addAction(QIcon(":/icons/refresh1.png"),
		tr("&Refresh"), pPatchbay, SLOT(refresh()),
		tr("Alt+R", "Refresh"));

	menu.exec(pos);
}


// Select the forwarding socket name from context menu.
void qjackctlPatchbayView::activateForwardMenu ( QAction *pAction )
{
	int iIndex = pAction->data().toInt();

	// Get currently input socket (assume its nicely selected)
	qjackctlSocketItem *pSocketItem = ISocketList()->selectedSocketItem();
	if (pSocketItem) {
		// Check first for forward from nil...
		if (iIndex < 0) {
			pSocketItem->setForward(QString::null);
			setDirty(true);
			return;
		}
		// Hopefully, its a real socket about to be forwraded...
		QListIterator<qjackctlSocketItem *> isocket(ISocketList()->sockets());
		while (isocket.hasNext()) {
			qjackctlSocketItem *pISocket = isocket.next();
			// Must be of same type of target one...
			if (pISocket->socketType() != pSocketItem->socketType())
				continue;
			const QString& sSocketName = pISocket->socketName();
			if (pSocketItem->socketName() == sSocketName)
				continue;
			if (iIndex == 0) {
				pSocketItem->setForward(sSocketName);
				setDirty(true);
				break;
			}
			iIndex--;
		}
	}
}


// Patchbay binding methods.
void qjackctlPatchbayView::setBinding ( qjackctlPatchbay *pPatchbay )
{
	m_pPatchbay = pPatchbay;
}

qjackctlPatchbay *qjackctlPatchbayView::binding (void) const
{
	return m_pPatchbay;
}


// Patchbay client list accessors.
qjackctlSocketList *qjackctlPatchbayView::OSocketList (void) const
{
	if (m_pPatchbay)
		return m_pPatchbay->OSocketList();
	else
		return NULL;
}

qjackctlSocketList *qjackctlPatchbayView::ISocketList (void) const
{
	if (m_pPatchbay)
		return m_pPatchbay->ISocketList();
	else
		return NULL;
}


// Patchwork line style accessors.
void qjackctlPatchbayView::setBezierLines ( bool bBezierLines )
{
	m_bBezierLines = bBezierLines;
}

bool qjackctlPatchbayView::isBezierLines (void) const
{
	return m_bBezierLines;
}


// Dirty flag methods.
void qjackctlPatchbayView::setDirty ( bool bDirty )
{
	m_bDirty = bDirty;
	if (bDirty)
		emit contentsChanged();
}

bool qjackctlPatchbayView::dirty (void) const
{
	return m_bDirty;
}


//----------------------------------------------------------------------
// qjackctlPatchbay -- Output-to-Input client/plugs connection object.
//

// Constructor.
qjackctlPatchbay::qjackctlPatchbay ( qjackctlPatchbayView *pPatchbayView )
{
	m_pPatchbayView = pPatchbayView;

	m_pOSocketList = new qjackctlSocketList(m_pPatchbayView->OListView(), true);
	m_pISocketList = new qjackctlSocketList(m_pPatchbayView->IListView(), false);

	m_pPatchbayView->setBinding(this);
}

// Default destructor.
qjackctlPatchbay::~qjackctlPatchbay (void)
{
	m_pPatchbayView->setBinding(NULL);

	delete m_pOSocketList;
	m_pOSocketList = NULL;

	delete m_pISocketList;
	m_pISocketList = NULL;

	(m_pPatchbayView->PatchworkView())->update();
}


// Connection primitive.
void qjackctlPatchbay::connectSockets ( qjackctlSocketItem *pOSocket,
	qjackctlSocketItem *pISocket )
{
	if (pOSocket->findConnectPtr(pISocket) == NULL) {
		pOSocket->addConnect(pISocket);
		pISocket->addConnect(pOSocket);
	}
}

// Disconnection primitive.
void qjackctlPatchbay::disconnectSockets ( qjackctlSocketItem *pOSocket,
	qjackctlSocketItem *pISocket )
{
	if (pOSocket->findConnectPtr(pISocket) != NULL) {
		pOSocket->removeConnect(pISocket);
		pISocket->removeConnect(pOSocket);
	}
}


// Test if selected plugs are connectable.
bool qjackctlPatchbay::canConnectSelected (void)
{
	QTreeWidgetItem *pOItem = (m_pOSocketList->listView())->currentItem();
	if (pOItem == NULL)
		return false;

	QTreeWidgetItem *pIItem = (m_pISocketList->listView())->currentItem();
	if (pIItem == NULL)
		return false;

	qjackctlSocketItem *pOSocket = NULL;
	switch (pOItem->type()) {
	case QJACKCTL_SOCKETITEM:
		pOSocket = static_cast<qjackctlSocketItem *> (pOItem);
		break;
	case QJACKCTL_PLUGITEM:
		pOSocket = (static_cast<qjackctlPlugItem *> (pOItem))->socket();
		break;
	default:
		return false;
	}

	qjackctlSocketItem *pISocket = NULL;
	switch (pIItem->type()) {
	case QJACKCTL_SOCKETITEM:
		pISocket = static_cast<qjackctlSocketItem *> (pIItem);
		break;
	case QJACKCTL_PLUGITEM:
		pISocket = (static_cast<qjackctlPlugItem *> (pIItem))->socket();
		break;
	default:
		return false;
	}

	// Sockets must be of the same type...
	if (pOSocket->socketType() != pISocket->socketType())
		return false;

	// Exclusive sockets may not accept more than one cable.
	if (pOSocket->isExclusive() && pOSocket->connects().count() > 0)
		return false;
	if (pISocket->isExclusive() && pISocket->connects().count() > 0)
		return false;

	// One-to-one connection...
	return (pOSocket->findConnectPtr(pISocket) == NULL);
}


// Connect current selected plugs.
bool qjackctlPatchbay::connectSelected (void)
{
	QTreeWidgetItem *pOItem = (m_pOSocketList->listView())->currentItem();
	if (pOItem == NULL)
		return false;

	QTreeWidgetItem *pIItem = (m_pISocketList->listView())->currentItem();
	if (pIItem == NULL)
		return false;

	qjackctlSocketItem *pOSocket = NULL;
	switch (pOItem->type()) {
	case QJACKCTL_SOCKETITEM:
		pOSocket = static_cast<qjackctlSocketItem *> (pOItem);
		break;
	case QJACKCTL_PLUGITEM:
		pOSocket = (static_cast<qjackctlPlugItem *> (pOItem))->socket();
		break;
	default:
		return false;
	}

	qjackctlSocketItem *pISocket = NULL;
	switch (pIItem->type()) {
	case QJACKCTL_SOCKETITEM:
		pISocket = static_cast<qjackctlSocketItem *> (pIItem);
		break;
	case QJACKCTL_PLUGITEM:
		pISocket = (static_cast<qjackctlPlugItem *> (pIItem))->socket();
		break;
	default:
		return false;
	}

	// Sockets must be of the same type...
	if (pOSocket->socketType() != pISocket->socketType())
		return false;

	// Exclusive sockets may not accept more than one cable.
	if (pOSocket->isExclusive() && pOSocket->connects().count() > 0)
		return false;
	if (pISocket->isExclusive() && pISocket->connects().count() > 0)
		return false;

	// One-to-one connection...
	connectSockets(pOSocket, pISocket);

	// Making one list dirty will take care of the rest...
	(m_pOSocketList->listView())->setDirty(true);

	return true;
}


// Test if selected plugs are disconnectable.
bool qjackctlPatchbay::canDisconnectSelected (void)
{
	QTreeWidgetItem *pOItem = (m_pOSocketList->listView())->currentItem();
	if (pOItem == NULL)
		return false;

	QTreeWidgetItem *pIItem = (m_pISocketList->listView())->currentItem();
	if (pIItem == NULL)
		return false;

	qjackctlSocketItem *pOSocket = NULL;
	switch (pOItem->type()) {
	case QJACKCTL_SOCKETITEM:
		pOSocket = static_cast<qjackctlSocketItem *> (pOItem);
		break;
	case QJACKCTL_PLUGITEM:
		pOSocket = (static_cast<qjackctlPlugItem *> (pOItem))->socket();
		break;
	default:
		return false;
	}

	qjackctlSocketItem *pISocket = NULL;
	switch (pIItem->type()) {
	case QJACKCTL_SOCKETITEM:
		pISocket = static_cast<qjackctlSocketItem *> (pIItem);
		break;
	case QJACKCTL_PLUGITEM:
		pISocket = (static_cast<qjackctlPlugItem *> (pIItem))->socket();
		break;
	default:
		return false;
	}

	// Sockets must be of the same type...
	if (pOSocket->socketType() != pISocket->socketType())
		return false;

	return (pOSocket->findConnectPtr(pISocket) != 0);
}


// Disconnect current selected plugs.
bool qjackctlPatchbay::disconnectSelected (void)
{
	QTreeWidgetItem *pOItem = (m_pOSocketList->listView())->currentItem();
	if (!pOItem)
		return false;

	QTreeWidgetItem *pIItem = (m_pISocketList->listView())->currentItem();
	if (!pIItem)
		return false;

	qjackctlSocketItem *pOSocket = NULL;
	switch (pOItem->type()) {
	case QJACKCTL_SOCKETITEM:
		pOSocket = static_cast<qjackctlSocketItem *> (pOItem);
		break;
	case QJACKCTL_PLUGITEM:
		pOSocket = (static_cast<qjackctlPlugItem *> (pOItem))->socket();
		break;
	default:
		return false;
	}

	qjackctlSocketItem *pISocket = NULL;
	switch (pIItem->type()) {
	case QJACKCTL_SOCKETITEM:
		pISocket = static_cast<qjackctlSocketItem *> (pIItem);
		break;
	case QJACKCTL_PLUGITEM:
		pISocket = (static_cast<qjackctlPlugItem *> (pIItem))->socket();
		break;
	default:
		return false;
	}

	// Sockets must be of the same type...
	if (pOSocket->socketType() != pISocket->socketType())
		return false;

	// One-to-one disconnection...
	disconnectSockets(pOSocket, pISocket);

	// Making one list dirty will take care of the rest...
	(m_pOSocketList->listView())->setDirty(true);

	return true;
}


// Test if any plug is disconnectable.
bool qjackctlPatchbay::canDisconnectAll (void)
{
	QListIterator<qjackctlSocketItem *> osocket(m_pOSocketList->sockets());
	while (osocket.hasNext()) {
		qjackctlSocketItem *pOSocket = osocket.next();
		if (pOSocket->connects().count() > 0)
			return true;
	}

	return false;
}


// Disconnect all plugs.
bool qjackctlPatchbay::disconnectAll (void)
{
	if (QMessageBox::warning(m_pPatchbayView,
		tr("Warning") + " - " QJACKCTL_SUBTITLE1,
		tr("This will disconnect all sockets.\n\n"
		"Are you sure?"),
		tr("Yes"), tr("No")) > 0) {
		return false;
	}

	QListIterator<qjackctlSocketItem *> osocket(m_pOSocketList->sockets());
	while (osocket.hasNext()) {
		qjackctlSocketItem *pOSocket = osocket.next();
		QListIterator<qjackctlSocketItem *> isocket(pOSocket->connects());
		while (isocket.hasNext())
			disconnectSockets(pOSocket, isocket.next());
	}

	// Making one list dirty will take care of the rest...
	(m_pOSocketList->listView())->setDirty(true);

	return true;
}



// Complete contents rebuilder.
void qjackctlPatchbay::refresh (void)
{
	(m_pOSocketList->listView())->update();
	(m_pISocketList->listView())->update();
	(m_pPatchbayView->PatchworkView())->update();
}


// Complete contents clearer.
void qjackctlPatchbay::clear (void)
{
	// Clear socket lists.
	m_pOSocketList->clear();
	m_pISocketList->clear();

	// Reset dirty flag.
	m_pPatchbayView->setDirty(false);

	// May refresh everything.
	refresh();
}


// Patchbay client list accessors.
qjackctlSocketList *qjackctlPatchbay::OSocketList (void) const
{
	return m_pOSocketList;
}

qjackctlSocketList *qjackctlPatchbay::ISocketList (void) const
{
	return m_pISocketList;
}


// External rack transfer method: copy patchbay structure from master rack model.
void qjackctlPatchbay::loadRackSockets ( qjackctlSocketList *pSocketList,
	QList<qjackctlPatchbaySocket *>& socketlist )
{
	pSocketList->clear();
	qjackctlSocketItem *pSocketItem = NULL;

	QListIterator<qjackctlPatchbaySocket *> sockit(socketlist);
	while (sockit.hasNext()) {
		qjackctlPatchbaySocket *pSocket = sockit.next();
		pSocketItem = new qjackctlSocketItem(pSocketList, pSocket->name(),
			pSocket->clientName(), pSocket->type(), pSocketItem);
		if (pSocketItem) {
			pSocketItem->setExclusive(pSocket->isExclusive());
			pSocketItem->setForward(pSocket->forward());
			pSocketItem->updatePixmap();
			qjackctlPlugItem *pPlugItem = NULL;
			QStringListIterator iter(pSocket->pluglist());
			while (iter.hasNext()) {
				pPlugItem = new qjackctlPlugItem(
					pSocketItem, iter.next(), pPlugItem);
			}
		}
	}
}

void qjackctlPatchbay::loadRack ( qjackctlPatchbayRack *pPatchbayRack )
{
	(m_pOSocketList->listView())->setUpdatesEnabled(false);
	(m_pISocketList->listView())->setUpdatesEnabled(false);

	// Load ouput sockets.
	loadRackSockets(m_pOSocketList, pPatchbayRack->osocketlist());

	// Load input sockets.
	loadRackSockets(m_pISocketList, pPatchbayRack->isocketlist());

	// Now ready to load from cable model.
	QListIterator<qjackctlPatchbayCable *> iter(pPatchbayRack->cablelist());
	while (iter.hasNext()) {
		qjackctlPatchbayCable *pCable = iter.next();
		// Get proper sockets...
		qjackctlPatchbaySocket *pOSocket = pCable->outputSocket();
		qjackctlPatchbaySocket *pISocket = pCable->inputSocket();
		if (pOSocket && pISocket) {
			qjackctlSocketItem *pOSocketItem
				= m_pOSocketList->findSocket(pOSocket->name(), pOSocket->type());
			qjackctlSocketItem *pISocketItem
				= m_pISocketList->findSocket(pISocket->name(), pISocket->type());
			if (pOSocketItem && pISocketItem)
				connectSockets(pOSocketItem, pISocketItem);
		}
	}

	(m_pOSocketList->listView())->setUpdatesEnabled(true);
	(m_pISocketList->listView())->setUpdatesEnabled(true);

	(m_pOSocketList->listView())->update();
	(m_pISocketList->listView())->update();
	(m_pPatchbayView->PatchworkView())->update();

	// Reset dirty flag.
	m_pPatchbayView->setDirty(false);
}

// External rack transfer method: copy patchbay structure into master rack model.
void qjackctlPatchbay::saveRackSockets ( qjackctlSocketList *pSocketList,
	QList<qjackctlPatchbaySocket *>& socketlist )
{
	// Have QTreeWidget item order into account:
	qjackctlSocketListView *pListView = pSocketList->listView();
	if (pListView == NULL)
		return;

	socketlist.clear();

	int iItemCount = pListView->topLevelItemCount();
	for (int iItem = 0; iItem < iItemCount; ++iItem) {
		QTreeWidgetItem *pItem = pListView->topLevelItem(iItem);
		if (pItem->type() != QJACKCTL_SOCKETITEM)
			continue;
		qjackctlSocketItem *pSocketItem
			= static_cast<qjackctlSocketItem *> (pItem);
		if (pSocketItem == NULL)
			continue;
		qjackctlPatchbaySocket *pSocket
			= new qjackctlPatchbaySocket(pSocketItem->socketName(),
				pSocketItem->clientName(), pSocketItem->socketType());
		if (pSocket) {
			pSocket->setExclusive(pSocketItem->isExclusive());
			pSocket->setForward(pSocketItem->forward());
			QListIterator<qjackctlPlugItem *> iter(pSocketItem->plugs());
			while (iter.hasNext())
				pSocket->pluglist().append((iter.next())->plugName());
			socketlist.append(pSocket);
		}
	}
}

void qjackctlPatchbay::saveRack ( qjackctlPatchbayRack *pPatchbayRack )
{
	// Save ouput sockets.
	saveRackSockets(m_pOSocketList, pPatchbayRack->osocketlist());

	// Save input sockets.
	saveRackSockets(m_pISocketList, pPatchbayRack->isocketlist());

	// Now ready to save into cable model.
	pPatchbayRack->cablelist().clear();

	// Start from output sockets...
	QListIterator<qjackctlSocketItem *> osocket(m_pOSocketList->sockets());
	while (osocket.hasNext()) {
		qjackctlSocketItem *pOSocketItem = osocket.next();
		// Then to input sockets...
		QListIterator<qjackctlSocketItem *> isocket(pOSocketItem->connects());
		while (isocket.hasNext()) {
			qjackctlSocketItem *pISocketItem = isocket.next();
			// Now find proper racked sockets...
			qjackctlPatchbaySocket *pOSocket = pPatchbayRack->findSocket(
				pPatchbayRack->osocketlist(), pOSocketItem->socketName());
			qjackctlPatchbaySocket *pISocket = pPatchbayRack->findSocket(
				pPatchbayRack->isocketlist(), pISocketItem->socketName());
			if (pOSocket && pISocket) {
				pPatchbayRack->addCable(
					new qjackctlPatchbayCable(pOSocket, pISocket));
			}
		}
	}

	// Reset dirty flag.
	m_pPatchbayView->setDirty(false);
}


// JACK client property accessors.
void qjackctlPatchbay::setJackClient ( jack_client_t *pJackClient )
{
	m_pOSocketList->setJackClient(pJackClient);
	m_pISocketList->setJackClient(pJackClient);
}

jack_client_t *qjackctlPatchbay::jackClient (void) const
{
	return m_pOSocketList->jackClient();
}

// ALSA sequencer property accessors.
void qjackctlPatchbay::setAlsaSeq ( snd_seq_t *pAlsaSeq )
{
	m_pOSocketList->setAlsaSeq(pAlsaSeq);
	m_pISocketList->setAlsaSeq(pAlsaSeq);
}

snd_seq_t *qjackctlPatchbay::alsaSeq (void) const
{
	return m_pOSocketList->alsaSeq();
}


// Audio connections snapshot.
void qjackctlPatchbay::socketPlugJackSnapshot (
	qjackctlSocketItem *pOSocket, qjackctlPlugItem *pOPlug )
{
	// Audio and MIDI engine descriptors...
	jack_client_t *pJackClient = jackClient();

	// Get current JACK port connections...
	QString sOClientPort = pOSocket->socketName() + ':' + pOPlug->plugName();
	const char **ppszIClientPorts = jack_port_get_all_connections(pJackClient,
		jack_port_by_name(pJackClient, sOClientPort.toUtf8().constData()));
	if (ppszIClientPorts) {
		// Now, for each input client port...
		int iIClientPort = 0;
		while (ppszIClientPorts[iIClientPort]) {
			QString sIClientPort = ppszIClientPorts[iIClientPort];
			int iColon = sIClientPort.indexOf(':');
			if (iColon >= 0) {
				qjackctlSocketItem *pISocket
					= m_pISocketList->findSocket(
						sIClientPort.left(iColon), pOSocket->socketType());
				if (pISocket)
					connectSockets(pOSocket, pISocket);
			}
			iIClientPort++;
		}
		::free(ppszIClientPorts);
	}
}


// MIDI connections snapshot.
void qjackctlPatchbay::socketPlugAlsaSnapshot (
	qjackctlSocketItem *pOSocket, qjackctlPlugItem *pOPlug )
{
#ifdef CONFIG_ALSA_SEQ

	// ALSA sequencer descriptors...
	snd_seq_t *pAlsaSeq = alsaSeq();

	// ALSA subscriber structures.
	snd_seq_query_subscribe_t *pAlsaSubs;
	snd_seq_addr_t seq_addr;
	snd_seq_query_subscribe_alloca(&pAlsaSubs);

	// ALSA readable (output) client ports...
	snd_seq_client_info_t *pOClientInfo;
	snd_seq_port_info_t   *pOPortInfo;
	unsigned int uiOFlags = SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;

	snd_seq_client_info_alloca(&pOClientInfo);
	snd_seq_port_info_alloca(&pOPortInfo);

	// ALSA sequencer writable (input) client ports...
	snd_seq_client_info_t *pIClientInfo;
	snd_seq_client_info_alloca(&pIClientInfo);

	// Get current MIDI port connections...
	snd_seq_client_info_set_client(pOClientInfo, -1);
	while (snd_seq_query_next_client(pAlsaSeq, pOClientInfo) >= 0) {
		int iOClient = snd_seq_client_info_get_client(pOClientInfo);
		if (iOClient > 0) {
			QString sOClientName = snd_seq_client_info_get_name(pOClientInfo);
			if (sOClientName == pOSocket->clientName()) {
				snd_seq_port_info_set_client(pOPortInfo, iOClient);
				snd_seq_port_info_set_port(pOPortInfo, -1);
				while (snd_seq_query_next_port(pAlsaSeq, pOPortInfo) >= 0) {
					unsigned int uiOCapability
						= snd_seq_port_info_get_capability(pOPortInfo);
					if (((uiOCapability & uiOFlags) == uiOFlags) &&
						((uiOCapability & SND_SEQ_PORT_CAP_NO_EXPORT) == 0)) {
						QString sOPortName
							= snd_seq_port_info_get_name(pOPortInfo);
						if (sOPortName == pOPlug->plugName()) {
							int iOPort = snd_seq_port_info_get_port(pOPortInfo);
							// Now, look for subscribers of his port...
							snd_seq_query_subscribe_set_type(pAlsaSubs,
								SND_SEQ_QUERY_SUBS_READ);
							snd_seq_query_subscribe_set_index(pAlsaSubs, 0);
							seq_addr.client = iOClient;
							seq_addr.port   = iOPort;
							snd_seq_query_subscribe_set_root(pAlsaSubs, &seq_addr);
							while (snd_seq_query_port_subscribers(pAlsaSeq, pAlsaSubs) >= 0) {
								seq_addr = *snd_seq_query_subscribe_get_addr(pAlsaSubs);
								if (snd_seq_get_any_client_info(pAlsaSeq,
										seq_addr.client, pIClientInfo) == 0) {
									QString sIClientName
										= snd_seq_client_info_get_name(pIClientInfo);
									qjackctlSocketItem *pISocket
										= m_pISocketList->findSocket(
											sIClientName, pOSocket->socketType());
									if (pISocket)
										connectSockets(pOSocket, pISocket);
								}
								snd_seq_query_subscribe_set_index(pAlsaSubs,
									snd_seq_query_subscribe_get_index(pAlsaSubs) + 1);
							}
						}
					}
				}
			}
		}
	}

#endif	// CONFIG_ALSA_SEQ
}


// Connections snapshot.
void qjackctlPatchbay::connectionsSnapshot (void)
{
	// Take snapshot of client port list.
	m_pOSocketList->clientPortsSnapshot();
	m_pISocketList->clientPortsSnapshot();

	// Then, starting from output sockets, try to grab the connections...
	QListIterator<qjackctlSocketItem *> osocket(m_pOSocketList->sockets());
	while (osocket.hasNext()) {
		qjackctlSocketItem *pOSocket = osocket.next();
		// For each output plug item...
		QListIterator<qjackctlPlugItem *> oplug(pOSocket->plugs());
		while (oplug.hasNext()) {
			qjackctlPlugItem *pOPlug = oplug.next();
			// Check for socket type...
			switch (pOSocket->socketType()) {
			case QJACKCTL_SOCKETTYPE_JACK_AUDIO:
			case QJACKCTL_SOCKETTYPE_JACK_MIDI:
				// Get current JACK port connections...
				socketPlugJackSnapshot(pOSocket, pOPlug);
				break;
			case QJACKCTL_SOCKETTYPE_ALSA_MIDI:
				// Get current MIDI port connections...
				socketPlugAlsaSnapshot(pOSocket, pOPlug);
				break;
			}
		}
	}

	// Surely it is kind of tainted.
	m_pPatchbayView->setDirty(true);
}


// end of qjackctlPatchbay.cpp
