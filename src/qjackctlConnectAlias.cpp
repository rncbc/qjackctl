// qjackctlConnectAlias.cpp
//
/****************************************************************************
   Copyright (C) 2005, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*****************************************************************************/

#include "qjackctlConnectAlias.h"


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
QString qjackctlClientAlias::clientName (void)
{
	return m_rxClientName.pattern();
}


// Client name matcher.
bool qjackctlClientAlias::matchClientName ( const QString& sClientName )
{
	return m_rxClientName.exactMatch(sClientName);
}


// Client aliasing methods.
const QString& qjackctlClientAlias::clientAlias (void)
{
	return m_sClientAlias;
}

void qjackctlClientAlias::setClientAlias ( const QString& sClientAlias )
{
	m_sClientAlias = sClientAlias;
}


// Port aliasing methods.
QString qjackctlClientAlias::portAlias ( const QString& sPortName )
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
	settings.writeEntry("/Name", m_rxClientName.pattern());
	settings.writeEntry("/Alias", m_sClientAlias);
	int iPort = 0;
	QMap<QString, QString>::ConstIterator iter = m_ports.begin();
	while (iter != m_ports.end()) {
		settings.beginGroup("/Port" + QString::number(++iPort));
		settings.writeEntry("/Name", iter.key());
		settings.writeEntry("/Alias", iter.data());
		settings.endGroup();
		++iter;
	}
	settings.endGroup();
}


// Escape and format a string as a regular expresion.
QString qjackctlClientAlias::escapeRegExpDigits ( const QString& s,
	unsigned int iThreshold )
{
	QString sDigits;
	QString sResult;
	QString sEscape = QRegExp::escape(s);
	unsigned int iDigits = 0;

	for (unsigned int i = 0; i < sEscape.length(); i++) {
		QCharRef ch = sEscape.at(i);
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


//----------------------------------------------------------------------
// class qjackctlConnectAlias -- Client list alias map.

// Constructor.
qjackctlConnectAlias::qjackctlConnectAlias (void)
{
	setAutoDelete(true);
}

// Default destructor.
qjackctlConnectAlias::~qjackctlConnectAlias (void)
{
	clear();
}


// Client finders.
qjackctlClientAlias *qjackctlConnectAlias::findClientName ( const QString& sClientName )
{
	for (qjackctlClientAlias *pClient = first();
			pClient; pClient = next()) {
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


// Virtual override function to compare two list items.
int qjackctlConnectAlias::compareItems ( QPtrCollection::Item pItem1,
	QPtrCollection::Item pItem2 )
{
	qjackctlClientAlias *pClient1 = (qjackctlClientAlias *) pItem1;
	qjackctlClientAlias *pClient2 = (qjackctlClientAlias *) pItem2;
	return QString::compare(pClient1->clientAlias(), pClient2->clientAlias());
}


// Load/save aliases definitions.
void qjackctlConnectAlias::loadSettings ( QSettings& settings,
	const QString& sAliasesKey )
{
	clear();

	QStringList clients = settings.subkeyList(sAliasesKey);
	for (QStringList::ConstIterator client = clients.begin();
			client != clients.end(); ++client) {
		QString sClientKey   = sAliasesKey + "/" + *client;
		QString sClientName  = settings.readEntry(sClientKey + "/Name");
		QString sClientAlias = settings.readEntry(sClientKey + "/Alias");
		if (!sClientName.isEmpty() && !sClientAlias.isEmpty()) {
			qjackctlClientAlias *pClient =
				new qjackctlClientAlias(sClientName, sClientAlias);
			append(pClient);
			QStringList ports = settings.subkeyList(sClientKey);
			for (QStringList::ConstIterator port = ports.begin();
					port != ports.end(); ++port) {
				QString sPortKey   = sClientKey + "/" + *port;
				QString sPortName  = settings.readEntry(sPortKey + "/Name");
				QString sPortAlias = settings.readEntry(sPortKey + "/Alias");
				if (!sPortName.isEmpty() && !sPortAlias.isEmpty())
					pClient->setPortAlias(sPortName, sPortAlias);
			}
		}
	}
}

void qjackctlConnectAlias::saveSettings ( QSettings& settings,
	const QString& sAliasesKey )
{
	sort();

	int iClient = 0;
	for (qjackctlClientAlias *pClient = first();
			pClient; pClient = next()) {
		pClient->saveSettings(settings, 
			sAliasesKey + "/Client" + QString::number(++iClient));
	}
}


// end of qjackctlConnectAlias.cpp
