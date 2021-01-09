// qjackctlInterfaceComboBox.cpp
//
/****************************************************************************
   Copyright (C) 2003-2020, rncbc aka Rui Nuno Capela. All rights reserved.
   Copyright (C) 2015, Kjetil Matheussen. (portaudio_probe_thread)
   Copyright (C) 2013, Arnout Engelen. All rights reserved.

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
#include "qjackctlInterfaceComboBox.h"

#include "qjackctlSetup.h"

#include <QTreeView>
#include <QHeaderView>
#include <QLineEdit>

#include <QStandardItemModel>

#include <QFile>
#include <QTextStream>

#ifdef CONFIG_COREAUDIO
#include <iostream>
#include <cstring>
#include <map>
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CFString.h>
#endif

#ifdef CONFIG_PORTAUDIO
#include <iostream>
#include <cstring>
#include <portaudio.h>
#ifdef WIN32
#include <windows.h>
#endif
#endif

#ifdef CONFIG_ALSA_SEQ
#include <alsa/asoundlib.h>
#endif

// Constructor.
qjackctlInterfaceComboBox::qjackctlInterfaceComboBox ( QWidget *pParent )
	: QComboBox(pParent)
{
	QTreeView *pTreeView = new QTreeView(this);
	pTreeView->header()->hide();
	pTreeView->setRootIsDecorated(false);
	pTreeView->setAllColumnsShowFocus(true);
	pTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
	pTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
	pTreeView->setModel(new QStandardItemModel());
//	pTreeView->setMinimumWidth(320);
	QComboBox::setView(pTreeView);
}


void qjackctlInterfaceComboBox::showPopup (void)
{
	populateModel();

	QComboBox::showPopup();
}


QStandardItemModel *qjackctlInterfaceComboBox::model (void) const
{
	return static_cast<QStandardItemModel *> (QComboBox::model());
}


void qjackctlInterfaceComboBox::setup (
	QComboBox *pDriverComboBox, int iAudio, const QString& sDefName )
{
	m_pDriverComboBox = pDriverComboBox;
	m_iAudio = iAudio;
	m_sDefName = sDefName;
}


void qjackctlInterfaceComboBox::clearCards (void)
{
	model()->clear();
}


void qjackctlInterfaceComboBox::addCard (
	const QString& sName, const QString& sDescription )
{
	QList<QStandardItem *> items;
	if (sName == m_sDefName || sName.isEmpty())
		items.append(new QStandardItem(m_sDefName));
	else
		items.append(new QStandardItem(QIcon(":/images/device1.png"), sName));
	items.append(new QStandardItem(sDescription));
	model()->appendRow(items);
}


#ifdef CONFIG_COREAUDIO

// borrowed from jackpilot source
static OSStatus getDeviceUIDFromID( AudioDeviceID id,
	char *name, UInt32 nsize )
{
	UInt32 size = sizeof(CFStringRef);
	CFStringRef UI;
	OSStatus res = AudioDeviceGetProperty(id, 0, false,
		kAudioDevicePropertyDeviceUID, &size, &UI);
	if (res == noErr) 
		CFStringGetCString(UI,name,nsize,CFStringGetSystemEncoding());
	CFRelease(UI);
	return res;
}

#endif // CONFIG_COREAUDIO


#ifdef CONFIG_PORTAUDIO

#include <QApplication>
#include <QThread>
#include <QMutex>
#include <QMessageBox>

namespace {

class PortAudioProber : public QThread
{
public:

	static QStringList getNames(QWidget *pParent)
	{
		{
			QMutexLocker locker(&PortAudioProber::mutex);
			if (!PortAudioProber::names.isEmpty())
				return PortAudioProber::names;
		}

		QMessageBox mbox(QMessageBox::Information, tr("Probing..."),
			tr("Please wait, PortAudio is probing audio hardware."),
			QMessageBox::Abort, pParent);

		// Make it impossible to start another PortAudioProber while waiting.
		mbox.setWindowModality(Qt::WindowModal);

		PortAudioProber *pab = new PortAudioProber;

		pab->start();

		bool bTimedOut = true;

		for (int i = 0; i < 100; ++i) {

			if (mbox.isVisible())
				QApplication::processEvents();

			QThread::msleep(50);

			if (i == 10) // wait 1/2 second before showing message box
				mbox.show();

			if (mbox.clickedButton() != nullptr) {
				bTimedOut = false;
				break;
			}

			if (pab->isFinished()) {
				bTimedOut = false;
				break;
			}
		}

		if (bTimedOut) {
			QMessageBox::warning(pParent, tr("Warning"),
				tr("Audio hardware probing timed out."));
		}

		{
			QMutexLocker locker(&PortAudioProber::mutex);
			return names;
		}
	}

private:

	PortAudioProber() {}
	~PortAudioProber() {}

	static QMutex mutex;
	static QStringList names;

	void run()
	{
		if (Pa_Initialize() == paNoError) {

			// Fill HostApi info...
			const PaHostApiIndex iNumHostApi = Pa_GetHostApiCount();
			QString *hostNames = new QString[iNumHostApi];
			for (PaHostApiIndex i = 0; i < iNumHostApi; ++i)
				hostNames[i] = QString(Pa_GetHostApiInfo(i)->name);

			// Fill device info...
			const PaDeviceIndex iNumDevice = Pa_GetDeviceCount();

			{
#ifdef WIN32
				wchar_t wideDeviceName[MAX_PATH];
#endif
				QMutexLocker locker(&PortAudioProber::mutex);
				if (PortAudioProber::names.isEmpty()) {
					for (PaDeviceIndex i = 0; i < iNumDevice; ++i) {
						PaDeviceInfo *pDeviceInfo = const_cast<PaDeviceInfo *> (Pa_GetDeviceInfo(i));
						if (strcmp(pDeviceInfo->name, "JackRouter") == 0)
							continue;
#ifdef WIN32
						MultiByteToWideChar(CP_UTF8, 0, pDeviceInfo->name, -1, wideDeviceName, MAX_PATH-1);
						const QString sName = hostNames[pDeviceInfo->hostApi] + "::" + QString::fromWCharArray(wideDeviceName);
#else
						const QString sName = hostNames[pDeviceInfo->hostApi] + "::" + QString(pDeviceInfo->name);
#endif
						PortAudioProber::names.push_back(sName);
					}
				}
			}

			delete [] hostNames;
			Pa_Terminate();
		}
	}
};

QMutex PortAudioProber::mutex;
QStringList PortAudioProber::names;

} // namespace

#endif  // CONFIG_PORTAUDIO


void qjackctlInterfaceComboBox::populateModel (void)
{
	const bool bBlockSignals = QComboBox::blockSignals(true);

	QComboBox::setUpdatesEnabled(false);
	QComboBox::setDuplicatesEnabled(false);

	QLineEdit *pLineEdit = QComboBox::lineEdit();

	// FIXME: Only valid for ALSA, Sun and OSS devices,
	// for the time being... and also CoreAudio ones too.
	const QString& sDriver = m_pDriverComboBox->currentText();
	const bool bAlsa = (sDriver == "alsa");
	const bool bSun  = (sDriver == "sun");
	const bool bOss  = (sDriver == "oss");
#ifdef CONFIG_COREAUDIO
	const bool bCoreaudio = (sDriver == "coreaudio");
	std::map<QString, AudioDeviceID> coreaudioIdMap;
#endif
#ifdef CONFIG_PORTAUDIO
	const bool bPortaudio = (sDriver == "portaudio");
#endif
	QString sCurName = pLineEdit->text();
	QString sName, sSubName;
	
	int iCards = 0;

	clearCards();

	int iCurCard = -1;

	if (bAlsa) {
#ifdef CONFIG_ALSA_SEQ
		// Enumerate the ALSA cards and PCM harfware devices...
		snd_ctl_t *handle;
		snd_ctl_card_info_t *info;
		snd_pcm_info_t *pcminfo;
		snd_ctl_card_info_alloca(&info);
		snd_pcm_info_alloca(&pcminfo);
		const QString sPrefix("hw:%1");
		const QString sSuffix(" (%1)");
		const QString sSubSuffix("%1,%2");
		QString sName2, sSubName2;
		bool bCapture, bPlayback;
		int iCard = -1;
		while (snd_card_next(&iCard) >= 0 && iCard >= 0) {
			sName = sPrefix.arg(iCard);
			if (snd_ctl_open(&handle, sName.toUtf8().constData(), 0) >= 0
				&& snd_ctl_card_info(handle, info) >= 0) {
				sName2 = sPrefix.arg(snd_ctl_card_info_get_id(info));
				addCard(sName2, snd_ctl_card_info_get_name(info) + sSuffix.arg(sName));
				if (sCurName == sName || sCurName == sName2)
					iCurCard = iCards;
				++iCards;
				int iDevice = -1;
				while (snd_ctl_pcm_next_device(handle, &iDevice) >= 0
					&& iDevice >= 0) {
					// Capture devices..
					bCapture = false;
					if (m_iAudio == QJACKCTL_CAPTURE ||
						m_iAudio == QJACKCTL_DUPLEX) {
						snd_pcm_info_set_device(pcminfo, iDevice);
						snd_pcm_info_set_subdevice(pcminfo, 0);
						snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_CAPTURE);
						bCapture = (snd_ctl_pcm_info(handle, pcminfo) >= 0);
					}
					// Playback devices..
					bPlayback = false;
					if (m_iAudio == QJACKCTL_PLAYBACK ||
						m_iAudio == QJACKCTL_DUPLEX) {
						snd_pcm_info_set_device(pcminfo, iDevice);
						snd_pcm_info_set_subdevice(pcminfo, 0);
						snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_PLAYBACK);
						bPlayback = (snd_ctl_pcm_info(handle, pcminfo) >= 0);
					}
					// List iif compliant with the audio mode criteria...
					if ((m_iAudio == QJACKCTL_CAPTURE && bCapture && !bPlayback) ||
						(m_iAudio == QJACKCTL_PLAYBACK && !bCapture && bPlayback) ||
						(m_iAudio == QJACKCTL_DUPLEX && bCapture && bPlayback)) {
						sSubName  = sSubSuffix.arg(sName).arg(iDevice);
						sSubName2 = sSubSuffix.arg(sName2).arg(iDevice);
						addCard(sSubName2, snd_pcm_info_get_name(pcminfo) + sSuffix.arg(sSubName));
						if (sCurName == sSubName || sCurName == sSubName2)
							iCurCard = iCards;
						++iCards;
					}
				}
				snd_ctl_close(handle);
			}
		}
#endif 	// CONFIG_ALSA_SEQ
	}
	else
	if (bSun) {
		QFile file("/var/run/dmesg.boot");
		if (file.open(QIODevice::ReadOnly)) {
			QTextStream stream(&file);
			QString sLine;
			QRegularExpression rxDevice("audio([0-9]) at (.*)");
			QRegularExpressionMatch match;
			while (!stream.atEnd()) {
				sLine = stream.readLine();
				match = rxDevice.match(sLine);
				if (match.hasMatch()) {
					sName = "/dev/audio" + match.captured(1);
					addCard(sName, match.captured(2));
					if (sCurName == sName)
						iCurCard = iCards;
					++iCards;
				}
			}
			file.close();
		}
	}
	else
	if (bOss) {
		// Enumerate the OSS Audio devices...
		QFile file("/dev/sndstat");
		if (file.open(QIODevice::ReadOnly)) {
			QTextStream stream(&file);
			QString sLine;
			bool bAudioDevices = false;
			QRegularExpression rxHeader("Audio devices.*",
				QRegularExpression::CaseInsensitiveOption);
			QRegularExpression rxDevice("([0-9]+):[ ]+(.*)");
			QRegularExpressionMatch match;
			while (!stream.atEnd()) {
				sLine = stream.readLine();
				if (bAudioDevices) {
					match = rxDevice.match(sLine);
					if (match.hasMatch()) {
						sName = "/dev/dsp" + match.captured(1);
						addCard(sName, match.captured(2));
						if (sCurName == sName)
							iCurCard = iCards;
						++iCards;
					}
					else break;
				} else {
					match = rxHeader.match(sLine);
					if (match.hasMatch())
						bAudioDevices = true;
				}
			}
			file.close();
		}
	}
#ifdef CONFIG_COREAUDIO
	else if (bCoreaudio) {
		// Find out how many Core Audio devices are there, if any...
		// (code snippet gently "borrowed" from Stephane Letz jackdmp;)
		OSStatus err;
		Boolean isWritable;
		UInt32 outSize = sizeof(isWritable);
		err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,
				&outSize, &isWritable);
		if (err == noErr) {
			// Calculate the number of device available...
			int numCoreDevices = outSize / sizeof(AudioDeviceID);
			// Make space for the devices we are about to get...
			AudioDeviceID *coreDeviceIDs = new AudioDeviceID [numCoreDevices];
			err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices,
					&outSize, (void *) coreDeviceIDs);
			if (err == noErr) {
				// Look for the CoreAudio device name...
				char coreDeviceName[256];
				UInt32 nameSize = 256;
				for (int i = 0; i < numCoreDevices; i++) {
					err = AudioDeviceGetPropertyInfo(coreDeviceIDs[i],
							0, true, kAudioDevicePropertyDeviceName,
							&outSize, &isWritable);
					if (err == noErr) {
						err = AudioDeviceGetProperty(coreDeviceIDs[i],
								0, true, kAudioDevicePropertyDeviceName,
								&nameSize, (void *) coreDeviceName);
						if (err == noErr) {
							char drivername[128];
							UInt32 dnsize = 128;
							// this returns the unique id for the device
							// that must be used on the commandline for jack
							if (getDeviceUIDFromID(coreDeviceIDs[i],
								drivername, dnsize) == noErr) {
								sName = drivername;
							} else {
								sName = "Error";
							}
							coreaudioIdMap[sName] = coreDeviceIDs[i];
							// TODO: hide this ugly ID from the user,
							// only show human readable name
							// humanreadable \t UID
							sSubName = QString(coreDeviceName);
							addCard(sName, sSubName);
							if (sCurName == sName || sCurName == sSubName)
								iCurCard = iCards;
							++iCards;
						}
					}
				}
			}
			delete [] coreDeviceIDs;
		}
	}
#endif 	// CONFIG_COREAUDIO
#ifdef CONFIG_PORTAUDIO
	else if (bPortaudio) {
		const QStringList& names = PortAudioProber::getNames(this);
		const int iCards = names.size();
		for (int i = 0; i < iCards; ++i) {
			const QString& sName = names[i];
			if (sCurName == sName)
				iCurCard = iCards;
			addCard(sName, QString());
		}
	}
#endif  // CONFIG_PORTAUDIO

	addCard(m_sDefName, QString());
	if (sCurName == m_sDefName || sCurName.isEmpty())
		iCurCard = iCards;
	++iCards;

	QTreeView *pTreeView = static_cast<QTreeView *> (QComboBox::view());
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	pTreeView->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
	pTreeView->header()->resizeSections(QHeaderView::ResizeToContents);
#endif
	pTreeView->setMinimumWidth(
		pTreeView->sizeHint().width() + QComboBox::iconSize().width());

	QComboBox::setCurrentIndex(iCurCard);

	pLineEdit->setText(sCurName);

	QComboBox::setUpdatesEnabled(true);
	QComboBox::blockSignals(bBlockSignals);
}


// end of qjackctlInterfaceComboBox.cpp
