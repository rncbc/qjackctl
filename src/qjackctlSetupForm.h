// qjackctlSetupForm.h
//
/****************************************************************************
   Copyright (C) 2003-2013, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctlSetupForm_h
#define __qjackctlSetupForm_h

#include "ui_qjackctlSetupForm.h"

// Forward declarations.
class qjackctlSetup;

class QButtonGroup;


//----------------------------------------------------------------------------
// qjackctlSetupForm -- UI wrapper form.

class qjackctlSetupForm : public QDialog
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlSetupForm(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);
	// Destructor.
	~qjackctlSetupForm();

	void setup(qjackctlSetup * pSetup);

public slots:

	void changeCurrentPreset(const QString&);
	void saveCurrentPreset();
	void deleteCurrentPreset();

	void changeAudio(int);
	void changeDriver(const QString&);

	void symbolStartupScript();
	void symbolPostStartupScript();
	void symbolShutdownScript();
	void symbolPostShutdownScript();

	void browseStartupScript();
	void browsePostStartupScript();
	void browseShutdownScript();
	void browsePostShutdownScript();
	void browseActivePatchbayPath();
	void browseMessagesLogPath();

	void chooseDisplayFont1();
	void chooseDisplayFont2();
	void toggleDisplayEffect(bool);
	void chooseMessagesFont();
	void chooseConnectionsFont();

	void settingsChanged();
	void optionsChanged();

protected slots:

	void accept();
	void reject();

protected:

	// A combo-box text item setter helper.
	void setComboBoxCurrentText (
		QComboBox *pComboBox, const QString& sText ) const;

	void changePreset(const QString& sPreset);
	bool savePreset(const QString& sPreset);
	bool deletePreset(const QString& sPreset);
	void resetPresets();

	void computeLatency();

	void changeDriverAudio( const QString& sDriver, int iAudio );
	void changeDriverUpdate( const QString& sDriver, bool bUpdate );

	void symbolMenu(QLineEdit * pLineEdit, QToolButton * pToolButton );

	void stabilizeForm();

private:

	// The Qt-designer UI struct...
	Ui::qjackctlSetupForm m_ui;

	// Instance variables.
	qjackctlSetup *m_pSetup;

	QButtonGroup *m_pTimeDisplayButtonGroup;

	int m_iDirtySetup;
	int m_iDirtySettings;
	int m_iDirtyOptions;

	QString m_sPreset;
};


#endif	// __qjackctlSetupForm_h


// end of qjackctlSetupForm.h
