// qjackctlSessionSaveForm.h
//
/****************************************************************************
   Copyright (C) 2003-2024, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qjackctlSessionSaveForm_h
#define __qjackctlSessionSaveForm_h

#include "ui_qjackctlSessionSaveForm.h"


// forward decls.
class qjackctlSetup;


//----------------------------------------------------------------------------
// qjackctlSessionSaveForm -- UI wrapper form.

class qjackctlSessionSaveForm : public QDialog
{
	Q_OBJECT

public:

	// Constructor.
	qjackctlSessionSaveForm(QWidget *pParent,
		const QString& sTitle, const QStringList& sessionDirs);
	// Destructor.
	~qjackctlSessionSaveForm();

	// Retrieve the accepted session directory, if the case arises.
	const QString& sessionDir() const;

	// Session directory versioning option.
	void setSessionSaveVersion(bool bSessionSaveVersion);
	bool isSessionSaveVersion() const;

protected slots:

	void accept();
	void reject();

	void changeSessionName(const QString& sSessionName);
	void changeSessionDir(const QString& sSessionDir);
	void browseSessionDir();

protected:

	void stabilizeForm();

private:

	// The Qt-designer UI struct...
	Ui::qjackctlSessionSaveForm m_ui;

	// Instance variables...
	QString m_sSessionDir;
};


#endif	// __qjackctlSessionSaveForm_h


// end of qjackctlSessionSaveForm.h
