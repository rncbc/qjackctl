// qjackctlConnectAlias.cpp
//
/****************************************************************************
   Copyright (C) 2003-2014, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qjackctlConnectAlias.h"

#include <QStringList>


//----------------------------------------------------------------------
// class qjackctlClientAlias -- Client item alias map.

// Constructor.
qjackctlClientAlias::qjackctlClientAlias ( const QString& sClientName,
	const QString& sClientAlias )
{
	if (sClientAlias.isEmpty()) {
		m_rxClientName.setPattern(escapeRegExpDigits(sClientName));
		m_sClientAlias = sClientName;
	} else {
		m_rxClientName.setPattern(sClientName);
		m_sClientAlias = sClientAlias;
	}
}

// Default destructor.
qjackctlClientAlias::~qjackctlClientAlias (void)
{
	m_ports.clear();
}


// Client name method.
QString qjackctlClientAlias::clientName (void) const
{
	return m_rxClientName.pattern();
}


// Client name matcher.
bool qjackctlClientAlias::matchClientName ( const QString& sClientName ) const
{
	return m_rxClientName.exactMatch(sClientName);
}


// Client aliasing methods.
const QString& qjackctlClientAlias::clientAlias (void) const
{
	return m_sClientAlias;
}

void qjackctlClientAlias::setClientAlias ( const QString& sClientAlias )
{
	m_sClientAlias = sClientAlias;
}


// Port aliasing methods.
QString qjackctlClientAlias::portAlias ( const QString& sPortName ) const
{
	QString sPortAlias = m_ports[sPortName];
	if (sPortAlias.isEmpty())
	    sPortAlias = sPortName;
	return sPortAlias;
}

void qjackctlClientAlias::setPortAlias ( const QString& sPortName,
	const QString& sPortAlias )
{
	m_ports[sPortName] = sPortAlias;
}


// Save client/port aliases definitions.
void qjackctlClientAlias::saveSettings ( QSettings& settings,
	const QString& sClientKey )
{
	settings.beginGroup(sClientKey);
	settings.setValue("/Name", m_rxClientName.pattern());
	settings.setValue("/Alias", m_sClientAlias);
	int iPort = 0;
	QMap<QString, QString>::ConstIterator iter = m_ports.begin();
	while (iter != m_ports.end()) {
		settings.beginGroup("/Port" + QString::number(++iPort));
		settings.setValue("/Name", iter.key());
		settings.setValue("/Alias", iter.value());
		settings.endGroup();
		++iter;
	}
	settings.endGroup();
}


// Escape and format a string as a regular expresion.
QString qjackctlClientAlias::escapeRegExpDigits ( const QString& s,
	int iThreshold )
{
	QString sDigits;
	QString sResult;
	QString sEscape = QRegExp::escape(s);
	int iDigits = 0;

	for (int i = 0; i < sEscape.length(); i++) {
		const QChar& ch = sEscape.at(i);
		if (ch.isDigit()) {
			if (iDigits < iThreshold)
				sDigits += ch;
			else
				sDigits = "[0-9]+";
			iDigits++;
		} else {
			if (iDigits > 0) {
				sResult += sDigits;
				sDigits = QString::null;
				iDigits = 0;
			}
			sResult += ch;
		}
	}

	if (iDigits > 0)
		sResult += sDigits;

	return sResult;
}


// Need for generic sort.
bool qjackctlClientAlias::operator< ( const qjackctlClientAlias& other )
{
	return (m_sClientAlias < other.clientAlias());
}


//----------------------------------------------------------------------
// class qjackctlConnectAlias -- Client list alias map.

// Constructor.
qjackctlConnectAlias::qjackctlConnectAlias (void)
{
}

// Default destructor.
qjackctlConnectAlias::~qjackctlConnectAlias (void)
{
	qDeleteAll(*this);
	clear();
}


// Client finders.
qjackctlClientAlias *qjackctlConnectAlias::findClientName (
	const QString& sClientName )
{
	QListIterator<qjackctlClientAlias *> iter(*this);
	while (iter.hasNext()) {
		qjackctlClientAlias *pClient = iter.next();
		if (pClient->matchClientName(sClientName))
		    return pClient;
	}

	return NULL;
}

// Client aliasing methods.
void qjackctlConnectAlias::setClientAlias ( const QString& sClientName,
	const QString& sClientAlias )
{
	qjackctlClientAlias *pClient = findClientName(sClientName);
	if (pClient == NULL) {
		pClient = new qjackctlClientAlias(sClientName);
		append(pClient);
	}
	pClient->setClientAlias(sClientAlias);
}

QString qjackctlConnectAlias::clientAlias ( const QString& sClientName )
{
	qjackctlClientAlias *pClient = findClientName(sClientName);
	if (pClient == NULL)
		return sClientName;

	return pClient->clientAlias();
}


// Client/port aliasing methods.
void qjackctlConnectAlias::setPortAlias ( const QString& sClientName,
	const QString& sPortName, const QString& sPortAlias )
{
	qjackctlClientAlias *pClient = findClientName(sClientName);
	if (pClient == NULL) {
		pClient = new qjackctlClientAlias(sClientName);
		append(pClient);
	}
	pClient->setPortAlias(sPortName, sPortAlias);
}

QString qjackctlConnectAlias::portAlias ( const QString& sClientName,
	const QString& sPortName )
{
	qjackctlClientAlias *pClient = findClientName(sClientName);
	if (pClient == NULL)
		return sPortName;

	return pClient->portAlias(sPortName);
}


// Load/save aliases definitions.
void qjackctlConnectAlias::loadSettings ( QSettings& settings,
	const QString& sAliasesKey )
{
	clear();

	settings.beginGroup(sAliasesKey);
	QStringListIterator iter(settings.childGroups());
	while (iter.hasNext()) {
		QString sClientKey   = iter.next();
		QString sClientName  = settings.value(sClientKey + "/Name").toString();
		QString sClientAlias = settings.value(sClientKey + "/Alias").toString();
		if (!sClientName.isEmpty() && !sClientAlias.isEmpty()) {
			qjackctlClientAlias *pClient =
				new qjackctlClientAlias(sClientName, sClientAlias);
			append(pClient);
			settings.beginGroup(sClientKey);
			QStringListIterator it(settings.childGroups());
			while (it.hasNext()) {
				QString sPortKey   = it.next();
				QString sPortName  = settings.value(sPortKey + "/Name").toString();
				QString sPortAlias = settings.value(sPortKey + "/Alias").toString();
				if (!sPortName.isEmpty() && !sPortAlias.isEmpty())
					pClient->setPortAlias(sPortName, sPortAlias);
			}
			settings.endGroup();
		}
	}
	settings.endGroup();
}

void qjackctlConnectAlias::saveSettings ( QSettings& settings,
	const QString& sAliasesKey )
{
	qSort(*this);

	settings.beginGroup(sAliasesKey);
	int iClient = 0;
	QListIterator<qjackctlClientAlias *> iter(*this);
	while (iter.hasNext()) {
		(iter.next())->saveSettings(settings,
			"Client" + QString::number(++iClient));
	}
	settings.endGroup();
}


// end of qjackctlConnectAlias.cpp
