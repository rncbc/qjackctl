// qjackctlPatchbayRack.h
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

#ifndef __qjackctlPatchbayRack_h
#define __qjackctlPatchbayRack_h

#include "qjackctlAbout.h"

#include <QObject>
#include <QStringList>

#include <jack/jack.h>

#ifdef CONFIG_ALSA_SEQ
#include <alsa/asoundlib.h>
#else
typedef void snd_seq_t;
#endif

// Patchbay socket types.
#define QJACKCTL_SOCKETTYPE_DEFAULT    -1
#define QJACKCTL_SOCKETTYPE_JACK_AUDIO  0
#define QJACKCTL_SOCKETTYPE_JACK_MIDI   1
#define QJACKCTL_SOCKETTYPE_ALSA_MIDI   2

// Patchbay slot normalization modes.
#define QJACKCTL_SLOTMODE_OPEN          0
#define QJACKCTL_SLOTMODE_HALF          1
#define QJACKCTL_SLOTMODE_FULL          2

// Patchbay change signal flags.
#define QJACKCTL_CABLE_FAILED           0
#define QJACKCTL_CABLE_CHECKED          1
#define QJACKCTL_CABLE_CONNECTED        2
#define QJACKCTL_CABLE_DISCONNECTED     3


// Struct name says it all.
struct qjackctlAlsaMidiPort
{
	QString sClientName;
	QString sPortName;
	int iAlsaClient;
	int iAlsaPort;
};

// Patchbay socket definition.
class qjackctlPatchbaySocket
{
public:

	// Constructor.
	qjackctlPatchbaySocket(const QString& sSocketName, const QString& sClientName, int iSocketType);
	// Default destructor.
	~qjackctlPatchbaySocket();

	// Socket property accessors.
	const QString& name() const;
	const QString& clientName() const;
	int type() const;
	bool isExclusive() const;
	const QString& forward() const;

	// Socket property methods.
	void setName(const QString& sSocketName);
	void setClientName(const QString& sClientName);
	void setType(int iSocketType);
	void setExclusive(bool bExclusive);
	void setForward(const QString& sSocketForward);

	// Plug list primitive methods.
	void addPlug(const QString& sPlugName);
	void removePlug(const QString& sPlugName);

	// Plug list accessor.
	QStringList& pluglist();
	
	// Simple socket type methods.
	static int typeFromText(const QString& sSocketType);
	static QString textFromType(int iSocketType);

private:

	// Properties.
	QString m_sSocketName;
	QString m_sClientName;
	int m_iSocketType;
	bool m_bExclusive;
	QString m_sSocketForward;

	// Patchbay socket plug list.
	QStringList m_pluglist;
};


// Patchbay socket slot definition.
class qjackctlPatchbaySlot
{
public:

	// Constructor.
	qjackctlPatchbaySlot(const QString& sSlotName, int iSlotMode = QJACKCTL_SLOTMODE_OPEN);
	// Default destructor.
	~qjackctlPatchbaySlot();

	// Slot property accessors.
	const QString& name() const;
	int mode() const;

	// Slot property methods.
	void setName(const QString& sSlotName);
	void setMode(int iSlotMode);

	// Socket methods.
	void setOutputSocket(qjackctlPatchbaySocket *pSocket);
	void setInputSocket(qjackctlPatchbaySocket *pSocket);

	// Socket accessors.
	qjackctlPatchbaySocket *outputSocket() const;
	qjackctlPatchbaySocket *inputSocket() const;

private:

	// Slot properties.
	QString m_sSlotName;
	int m_iSlotMode;

	// Socket references.
	qjackctlPatchbaySocket *m_pOutputSocket;
	qjackctlPatchbaySocket *m_pInputSocket;
};


// Patchbay cable connection definition.
class qjackctlPatchbayCable
{
public:

	// Constructor.
	qjackctlPatchbayCable(qjackctlPatchbaySocket *pOutputSocket,
		qjackctlPatchbaySocket *pInputSocket);
	// Default destructor.
	~qjackctlPatchbayCable();

	// Socket methods.
	void setOutputSocket(qjackctlPatchbaySocket *pSocket);
	void setInputSocket(qjackctlPatchbaySocket *pSocket);

	// Socket accessors.
	qjackctlPatchbaySocket *outputSocket() const;
	qjackctlPatchbaySocket *inputSocket() const;

private:

	// Socket references.
	qjackctlPatchbaySocket *m_pOutputSocket;
	qjackctlPatchbaySocket *m_pInputSocket;
};


// Patchbay rack profile definition.
class qjackctlPatchbayRack : public QObject
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlPatchbayRack();
	// Default destructor.
	~qjackctlPatchbayRack();

	// Common socket list primitive methods.
	void addSocket(QList<qjackctlPatchbaySocket *>& socketlist,
		qjackctlPatchbaySocket *pSocket);
	void removeSocket(QList<qjackctlPatchbaySocket *>& socketlist,
		qjackctlPatchbaySocket *pSocket);

	// Slot list primitive methods.
	void addSlot(qjackctlPatchbaySlot *pSlot);
	void removeSlot(qjackctlPatchbaySlot *pSlot);

	// Cable list primitive methods.
	void addCable(qjackctlPatchbayCable *pCable);
	void removeCable(qjackctlPatchbayCable *pCable);

	// Common socket finder.
	qjackctlPatchbaySocket *findSocket(
		QList<qjackctlPatchbaySocket *>& socketlist,
		const QString& sSocketName,
		int iSocketType = QJACKCTL_SOCKETTYPE_DEFAULT);

	// Slot finders.
	qjackctlPatchbaySlot *findSlot(const QString& sSlotName);

	// Cable finder.
	qjackctlPatchbayCable *findCable(
		const QString& sOutputSocket, const QString& sInputSocket);
	qjackctlPatchbayCable *findCable(qjackctlPatchbayCable *pCablePtr);

	// Cable finder (logical matching by client/port names).
	qjackctlPatchbayCable *findCable ( 
		const QString& sOClientName, const QString& sOPortName,
		const QString& sIClientName, const QString& sIPortName,
		int iSocketType);

	// Patchbay cleaner.
	void clear();

	// Patchbay rack socket list accessors.
	QList<qjackctlPatchbaySocket *>& osocketlist();
	QList<qjackctlPatchbaySocket *>& isocketlist();
	// Patchbay rack slots list accessor.
	QList<qjackctlPatchbaySlot *>& slotlist();
	// Patchbay cable connections list accessor.
	QList<qjackctlPatchbayCable *>& cablelist();

	// Overloaded cable connection persistence scan cycle methods.
	void connectJackScan(jack_client_t *pJackClient);
	void connectAlsaScan(snd_seq_t *pAlsaSeq);

	// Patchbay snapshot methods.
	void connectJackSnapshot(jack_client_t *pJackClient);
	void connectAlsaSnapshot(snd_seq_t *pAlsaSeq);

	// Patchbay reset/disconnect-all methods.
	void disconnectAllJackPorts(jack_client_t *pJackClient);
	void disconnectAllAlsaPorts(snd_seq_t *pAlsaSeq);

signals:

	// Cable connection change signal.
	void cableConnected(const QString& sOutputPort,
		const QString& sInputPort, unsigned int uiCableFlags);

private:

	// Audio connection scan related private methods.
	const char *findJackPort(const char **ppszJackPorts,
		const QString& sClientName, const QString& sPortName, int n = 0);
	void connectJackPorts(
		const char *pszOutputPort, const char *pszInputPort);
	void disconnectJackPorts(
		const char *pszOutputPort, const char *pszInputPort);
	void checkJackPorts(
		const char *pszOutputPort, const char *pszInputPort);
	void connectJackSocketPorts(
		qjackctlPatchbaySocket *pOutputSocket, const char *pszOutputPort,
		qjackctlPatchbaySocket *pInputSocket, const char *pszInputPort);
	void connectJackCable(
		qjackctlPatchbaySocket *pOutputSocket,
		qjackctlPatchbaySocket *pInputSocket);

	// MIDI connection scan related private methods.
	void loadAlsaPorts(QList<qjackctlAlsaMidiPort *>& midiports, bool bReadable);
	qjackctlAlsaMidiPort *findAlsaPort(QList<qjackctlAlsaMidiPort *>& midiports,
		const QString& sClientName, const QString& sPortName, int n);
	QString getAlsaPortName(qjackctlAlsaMidiPort *pAlsaPort);
	void setAlsaPort(qjackctlAlsaMidiPort *pAlsaPort,
		int iAlsaClient, int iAlsaPort);
	void connectAlsaPorts(
		qjackctlAlsaMidiPort *pOutputPort, qjackctlAlsaMidiPort *pInputPort);
	void disconnectAlsaPorts(
		qjackctlAlsaMidiPort *pOutputPort, qjackctlAlsaMidiPort *pInputPort);
	void checkAlsaPorts(
		qjackctlAlsaMidiPort *pOutputPort, qjackctlAlsaMidiPort *pInputPort);
	void connectAlsaSocketPorts(
		qjackctlPatchbaySocket *pOutputSocket, qjackctlAlsaMidiPort *pOutputPort,
		qjackctlPatchbaySocket *pInputSocket, qjackctlAlsaMidiPort *pInputPort);
	void connectAlsaCable(
		qjackctlPatchbaySocket *pOutputSocket,
		qjackctlPatchbaySocket *pInputSocket);

	void loadAlsaConnections(QList<qjackctlAlsaMidiPort *>& midiports,
		qjackctlAlsaMidiPort *pAlsaPort, bool bReadable);

	// Audio socket/ports forwarding executive methods.
	void connectJackForwardPorts(
		const char *pszPort, const char *pszPortForward);
	void connectJackForward(
		qjackctlPatchbaySocket *pSocket,
		qjackctlPatchbaySocket *pSocketForward);

	// MIDI socket/ports forwarding executive methods.
	void connectAlsaForwardPorts(
		qjackctlAlsaMidiPort *pPort, qjackctlAlsaMidiPort *pPortForward);
	void connectAlsaForward(
		qjackctlPatchbaySocket *pSocket,
		qjackctlPatchbaySocket *pSocketForward);

	// Common socket forwarding scan method.
	void connectForwardScan(int iSocketType);

	// JACK snapshot executive.
	void connectJackSnapshotEx(int iSocketType);

	// JACK reset/disconnect-all executive.
	void disconnectAllJackPortsEx(int iSocketType);

	// Patchbay sockets lists.
	QList<qjackctlPatchbaySocket *> m_osocketlist;
	QList<qjackctlPatchbaySocket *> m_isocketlist;
	// Patchbay rack slots list.
	QList<qjackctlPatchbaySlot *> m_slotlist;
	// Patchbay cable connections list.
	QList<qjackctlPatchbayCable *> m_cablelist;

	// Audio connection persistence cache variables.
	jack_client_t *m_pJackClient;
	const char **m_ppszOAudioPorts;
	const char **m_ppszIAudioPorts;
	const char **m_ppszOMidiPorts;
	const char **m_ppszIMidiPorts;

	// MIDI connection persistence cache variables.
	snd_seq_t *m_pAlsaSeq;
	QList<qjackctlAlsaMidiPort *> m_omidiports;
	QList<qjackctlAlsaMidiPort *> m_imidiports;
};


#endif  // __qjackctlPatchbayRack_h

// qjackctlPatchbayRack.h
