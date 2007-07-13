// qjackctlMainForm.h
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

#ifndef __qjackctlMainForm_h
#define __qjackctlMainForm_h

#include "ui_qjackctlMainForm.h"

#include "qjackctlSetup.h"
#include "qjackctlPatchbay.h"

#include <QProcess>
#include <QDateTime>

// Forward declarations.
class qjackctlSetup;
class qjackctlMessagesForm;
class qjackctlStatusForm;
class qjackctlConnectionsForm;
class qjackctlPatchbayForm;
class qjackctlSystemTray;

class QSocketNotifier;


//----------------------------------------------------------------------------
// qjackctlMainForm -- UI wrapper form.

class qjackctlMainForm : public QWidget
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlMainForm(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);
	// Destructor.
	~qjackctlMainForm();

	static qjackctlMainForm *getInstance();

	bool setup(qjackctlSetup * pSetup);

	bool isActivePatchbay(const QString& sPatchbayPath) const;
	void updateActivePatchbay();
	void setActivePatchbay(const QString& sPatchbayPath);
	void setRecentPatchbays(const QStringList& patchbays);

	void stabilizeForm();

	void stabilize(int msecs);

	void resetXrunStats();
	void refreshXrunStats();

	void refreshJackConnections();
	void refreshAlsaConnections();

	void refreshPatchbay();

public slots:

	void startJack();
	void stopJack();

	void readStdout();
	void appendStdoutBuffer(const QString&);
	void flushStdoutBuffer();

	void jackStarted();
	void jackError(QProcess::ProcessError);
	void jackFinished();
	void jackCleanup();

	void stdoutNotifySlot(int);
	void alsaNotifySlot(int);

	void timerSlot();

	void jackConnectChanged();
	void alsaConnectChanged();

	void cableConnectSlot(const QString&, const QString&, unsigned int);

	void toggleMainForm();
	void toggleMessagesForm();
	void toggleStatusForm();
	void toggleConnectionsForm();
	void togglePatchbayForm();

	void showSetupForm();
	void showAboutForm();

	void transportRewind();
	void transportBackward();
	void transportPlay(bool);
	void transportStart();
	void transportStop();
	void transportForward();

	void systemTrayContextMenu(const QPoint&);
	void activatePresetsMenu(QAction *);

	void quitMainForm();

protected:

	bool queryClose();

	void closeEvent(QCloseEvent * pCloseEvent);
	void customEvent(QEvent *pEvent);

	QString formatExitStatus(int iExitStatus) const;

	void shellExecute(const QString& sShellCommand,
		const QString& sStartMessage, const QString& sStopMessage);

	void stopJackServer();

	QString& detectXrun(QString& s);
	void updateXrunStats(float fXrunLast);

	void appendMessages(const QString& s);
	void appendMessagesColor(const QString& s, const QString& c);
	void appendMessagesText(const QString& s);
	void appendMessagesError(const QString& s);

	void updateMessagesFont();
	void updateMessagesLimit();
	void updateConnectionsFont();
	void updateConnectionsIconSize();
	void updateBezierLines();
	void updateDisplayEffect();
	void updateTimeDisplayFonts();
	void updateTimeDisplayToolTips();
	void updateTimeFormat();
	void updateAliases();
	void updateButtons();

	void updateXrunCount();

	QString formatTime(float secs) const;
	QString formatElapsedTime(int iStatusItem, const QTime& t, bool bElapsed) const;
	void updateElapsedTimes();

	void portNotifyEvent();
	void xrunNotifyEvent();
	void buffNotifyEvent();
	void shutNotifyEvent();
	void exitNotifyEvent();

	bool startJackClient(bool bDetach);
	void stopJackClient();

	void refreshConnections();
	void refreshStatus();

	void updateStatusItem(int iStatusItem, const QString& sText);
	void updateTitleStatus();
	void updateServerState(int iState);
	void updateSystemTray();

	void showDirtySettingsWarning();
	void showDirtySetupWarning();

	void contextMenuEvent(QContextMenuEvent *);

private:

	// The Qt-designer UI struct...
	Ui::qjackctlMainForm m_ui;

	// Instance variables.
	qjackctlSetup *m_pSetup;

	QProcess *m_pJack;

	int m_iServerState;

	jack_client_t *m_pJackClient;
	bool m_bJackDetach;
	bool m_bJackShutdown;

	snd_seq_t *m_pAlsaSeq;

	int m_iStartDelay;
	int m_iTimerDelay;
	int m_iTimerRefresh;
	int m_iJackRefresh;
	int m_iAlsaRefresh;
	int m_iJackDirty;
	int m_iAlsaDirty;
	int m_iStatusRefresh;
	int m_iPatchbayRefresh;

	QSocketNotifier *m_pStdoutNotifier;
	QSocketNotifier *m_pAlsaNotifier;

	int   m_iXrunCallbacks;
	int   m_iXrunSkips;
	int   m_iXrunStats;
	int   m_iXrunCount;
	float m_fXrunTotal;
	float m_fXrunMax;
	float m_fXrunMin;
	float m_fXrunLast;
	QTime m_tXrunLast;
	QTime m_tResetLast;

	qjackctlMessagesForm    *m_pMessagesForm;
	qjackctlStatusForm      *m_pStatusForm;
	qjackctlConnectionsForm *m_pConnectionsForm;
	qjackctlPatchbayForm    *m_pPatchbayForm;

	qjackctlPatchbayRack m_patchbayRack;

	qjackctlSystemTray *m_pSystemTray;

	qjackctlPreset m_preset;

	QString m_sStdoutBuffer;
	QString m_sTimeDashes;
	QString m_sJackCmdLine;

	bool  m_bQuitForce;
	float m_fSkipAccel;

	// Kind-of singleton reference.
	static qjackctlMainForm *g_pMainForm;
};


#endif	// __qjackctlMainForm_h


// end of qjackctlMainForm.h
