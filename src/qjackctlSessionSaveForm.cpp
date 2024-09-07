// qjackctlSessionSaveForm.cpp
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

#include "qjackctlSessionSaveForm.h"

#include "qjackctlAbout.h"
#include "qjackctlSetup.h"

#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

#include <QValidator>
#include <QPushButton>


//----------------------------------------------------------------------------
// qjackctlSessionSaveForm -- UI wrapper form.

// Constructor.
qjackctlSessionSaveForm::qjackctlSessionSaveForm (
	QWidget *pParent, const QString& sTitle, const QStringList& sessionDirs )
	: QDialog(pParent)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	// Setup UI entities...
	QDialog::setWindowTitle(sTitle);

	m_ui.SessionNameLineEdit->setValidator(
		new QRegularExpressionValidator(
			QRegularExpression("[^/]+"),
			m_ui.SessionNameLineEdit));

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
	m_ui.SessionDirComboBox->lineEdit()->setClearButtonEnabled(true);
#endif

	if (!sessionDirs.isEmpty()) {
		m_ui.SessionDirComboBox->addItems(sessionDirs);
		const QFileInfo fi(sessionDirs.first());
		m_sSessionDir = fi.absoluteFilePath();
		if (m_sSessionDir != QDir::homePath())
			m_ui.SessionNameLineEdit->setText(fi.fileName());
	}

	if (m_sSessionDir.isEmpty())
		m_sSessionDir = QDir::homePath();

	m_ui.SessionDirComboBox->setEditText(m_sSessionDir);

	adjustSize();

	// UI signal/slot connections...
	QObject::connect(m_ui.SessionNameLineEdit,
		SIGNAL(textChanged(const QString&)),
		SLOT(changeSessionName(const QString&)));
	QObject::connect(m_ui.SessionDirComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(changeSessionDir(const QString&)));
	QObject::connect(m_ui.SessionDirToolButton,
		SIGNAL(clicked()),
		SLOT(browseSessionDir()));
	QObject::connect(m_ui.DialogButtonBox,
		SIGNAL(accepted()),
		SLOT(accept()));
	QObject::connect(m_ui.DialogButtonBox,
		SIGNAL(rejected()),
		SLOT(reject()));

	// Done.
	stabilizeForm();
}


// Destructor.
qjackctlSessionSaveForm::~qjackctlSessionSaveForm (void)
{
}


// Retrieve the accepted session directory, if the case arises.
const QString& qjackctlSessionSaveForm::sessionDir (void) const
{
	return m_sSessionDir;
}



// Session directory versioning option.
void qjackctlSessionSaveForm::setSessionSaveVersion ( bool bSessionSaveVersion )
{
	m_ui.SessionSaveVersionCheckBox->setChecked(bSessionSaveVersion);
}

bool qjackctlSessionSaveForm::isSessionSaveVersion (void) const
{
	return m_ui.SessionSaveVersionCheckBox->isChecked();
}


// Accept settings (OK button slot).
void qjackctlSessionSaveForm::accept (void)
{
	// Check if session directory is new...
	const QString& sSessionDir
		= m_ui.SessionDirComboBox->currentText();
	if (sSessionDir.isEmpty())
		return;

	QDir dir;
	while (!dir.exists(sSessionDir)) {
		// Ask user...
		if (QMessageBox::warning(this,
			tr("Warning"),
			tr("Session directory does not exist:\n\n"
			"\"%1\"\n\n"
			"Do you want to create it?").arg(sSessionDir),
			QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
			return;
		// Proceed...
		dir.mkpath(sSessionDir);
	}

	// Save options...
	m_sSessionDir = sSessionDir;

	// Just go with dialog acceptance.
	QDialog::accept();
}


// Reject settings (Cancel button slot).
void qjackctlSessionSaveForm::reject (void)
{
	QDialog::reject();
}


// Session name in-flight change.
void qjackctlSessionSaveForm::changeSessionName ( const QString& sSessionName )
{
	QFileInfo fi(m_ui.SessionDirComboBox->currentText());
	if (fi.canonicalFilePath() == QDir::homePath())
		fi.setFile(QDir(fi.absoluteFilePath()), sSessionName);
	else
		fi.setFile(QDir(fi.absolutePath()), sSessionName);

	const bool bBlockSignals
		= m_ui.SessionDirComboBox->blockSignals(true);
	m_ui.SessionDirComboBox->setEditText(fi.absoluteFilePath());
	m_ui.SessionDirComboBox->blockSignals(bBlockSignals);

	stabilizeForm();
}


// Session directory in-flight change.
void qjackctlSessionSaveForm::changeSessionDir ( const QString& sSessionDir )
{
	if (sSessionDir.isEmpty()) {
		m_ui.SessionDirComboBox->setEditText(m_sSessionDir);
	} else {
		QString sSessionName;
		const QFileInfo fi(sSessionDir);
		if (fi.canonicalFilePath() != QDir::homePath())
			sSessionName = fi.fileName();
		const bool bBlockSignals
			= m_ui.SessionNameLineEdit->blockSignals(true);
		m_ui.SessionNameLineEdit->setText(sSessionName);
		m_ui.SessionNameLineEdit->blockSignals(bBlockSignals);
	}

	stabilizeForm();
}


// Browse for session directory.
void qjackctlSessionSaveForm::browseSessionDir (void)
{
	const QString& sTitle 
		= tr("Session Directory");

	QString sSessionDir = m_ui.SessionDirComboBox->currentText();

#if 0//QT_VERSION < QT_VERSION_CHECK(4, 4, 0)
	// Construct open-directory dialog...
	QFileDialog fileDialog(pParentWidget, sTitle, sSessionDir);
	// Set proper open-file modes...
	fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	fileDialog.setFileMode(QFileDialog::DirectoryOnly);
	// Stuff sidebar...
	QList<QUrl> urls(fileDialog.sidebarUrls());
	urls.append(QUrl::fromLocalFile(sSessionDir);
	fileDialog.setSidebarUrls(urls);
	fileDialog.setOptions(options);
	// Show dialog...
	if (!fileDialog.exec())
		return;
	sSessionDir = fileDialog.selectedFiles().first();
#else
	sSessionDir = QFileDialog::getExistingDirectory(this, sTitle, sSessionDir);
#endif

	if (sSessionDir.isEmpty())
		return;

	changeSessionDir(sSessionDir);

	const bool bBlockSignals
		= m_ui.SessionDirComboBox->blockSignals(true);
	m_ui.SessionDirComboBox->setEditText(sSessionDir);
	m_ui.SessionDirComboBox->blockSignals(bBlockSignals);
	m_ui.SessionDirComboBox->setFocus();

	stabilizeForm();
}


// Stabilize current form state.
void qjackctlSessionSaveForm::stabilizeForm (void)
{
	const QString& sSessionDir
		= m_ui.SessionDirComboBox->currentText();
	bool bValid = !m_ui.SessionNameLineEdit->text().isEmpty();
	bValid = bValid && !sSessionDir.isEmpty();
	if (bValid) {
		QFileInfo fi(sSessionDir);
		bValid = bValid && (fi.canonicalFilePath() != QDir::homePath());
		while (bValid && !fi.exists())
			fi.setFile(fi.path());
		bValid = bValid && fi.isDir() && fi.isReadable() && fi.isWritable();
	}
	m_ui.DialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(bValid);
}


// end of qjackctlSessionSaveForm.cpp
