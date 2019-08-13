// qjackctlAliases.cpp
//
/****************************************************************************
   Copyright (C) 2003-2019, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qjackctlAliases.h"

#include <QStringList>

#include <algorithm>


//----------------------------------------------------------------------
// class qjackctlAliasItem -- Client/port item alias map.

// Constructor.
qjackctlAliasItem::qjackctlAliasItem (
	const QString& sClientName, const QString& sClientAlias )
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
qjackctlAliasItem::~qjackctlAliasItem (void)
{
	m_ports.clear();
}


// Client name method.
QString qjackctlAliasItem::clientName (void) const
{
	return m_rxClientName.pattern();
}


// Client name matcher.
bool qjackctlAliasItem::matchClientName ( const QString& sClientName ) const
{
	return m_rxClientName.exactMatch(sClientName);
}


// Client aliasing methods.
const QString& qjackctlAliasItem::clientAlias (void) const
{
	return m_sClientAlias;
}


void qjackctlAliasItem::setClientAlias ( const QString& sClientAlias )
{
	m_sClientAlias = sClientAlias;
}


// Port aliasing methods.
QString qjackctlAliasItem::portAlias ( const QString& sPortName ) const
{
	return m_ports.value(sPortName, sPortName);
}


void qjackctlAliasItem::setPortAlias (
	const QString& sPortName, const QString& sPortAlias )
{
	if (sPortAlias.isEmpty())
		m_ports.remove(sPortName);
	else
		m_ports.insert(sPortName, sPortAlias);
}


// Save client/port aliases definitions.
void qjackctlAliasItem::saveSettings (
	QSettings& settings, const QString& sClientKey )
{
	settings.beginGroup(sClientKey);
	settings.setValue("/Name", m_rxClientName.pattern());
	settings.setValue("/Alias", m_sClientAlias);
	int iPort = 0;
	QMap<QString, QString>::ConstIterator iter = m_ports.constBegin();
	const QMap<QString, QString>::ConstIterator& iter_end
		= m_ports.constEnd();
	for ( ; iter != iter_end; ++iter) {
		const QString& sPortName
			= iter.key();
		const QString& sPortAlias
			= iter.value();
		if (!sPortName.isEmpty() && !sPortAlias.isEmpty()) {
			settings.beginGroup("/Port" + QString::number(++iPort));
			settings.setValue("/Name", sPortName);
			settings.setValue("/Alias", sPortAlias);
			settings.endGroup();
		}
	}
	settings.endGroup();
}


// Escape and format a string as a regular expresion.
QString qjackctlAliasItem::escapeRegExpDigits (
	const QString& s, int iThreshold )
{
	const QString& sEscape
		= QRegExp::escape(s);

	QString sDigits;
	QString sResult;
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
				sDigits.clear();
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
bool qjackctlAliasItem::operator< ( const qjackctlAliasItem& other )
{
	return (m_sClientAlias < other.clientAlias());
}


//----------------------------------------------------------------------
// class qjackctlAliasList -- Client/port list alias map.

// Constructor.
qjackctlAliasList::qjackctlAliasList (void)
{
}

// Default destructor.
qjackctlAliasList::~qjackctlAliasList (void)
{
	qDeleteAll(*this);
	clear();
}


// Client finders.
qjackctlAliasItem *qjackctlAliasList::findClientName (
	const QString& sClientName )
{
	QListIterator<qjackctlAliasItem *> iter(*this);
	while (iter.hasNext()) {
		qjackctlAliasItem *pClient = iter.next();
		if (pClient->matchClientName(sClientName))
			return pClient;
	}

	return nullptr;
}


// Client aliasing methods.
void qjackctlAliasList::setClientAlias (
	const QString& sClientName, const QString& sClientAlias )
{
	qjackctlAliasItem *pClient = findClientName(sClientName);
	if (pClient == nullptr) {
		pClient = new qjackctlAliasItem(sClientName);
		append(pClient);
	}
	pClient->setClientAlias(sClientAlias);
}


QString qjackctlAliasList::clientAlias ( const QString& sClientName )
{
	qjackctlAliasItem *pClient = findClientName(sClientName);
	if (pClient == nullptr)
		return sClientName;

	return pClient->clientAlias();
}


// Client/port aliasing methods.
void qjackctlAliasList::setPortAlias ( const QString& sClientName,
	const QString& sPortName, const QString& sPortAlias )
{
	qjackctlAliasItem *pClient = findClientName(sClientName);
	if (pClient == nullptr) {
		pClient = new qjackctlAliasItem(sClientName);
		append(pClient);
	}
	pClient->setPortAlias(sPortName, sPortAlias);
}


QString qjackctlAliasList::portAlias (
	const QString& sClientName, const QString& sPortName )
{
	qjackctlAliasItem *pClient = findClientName(sClientName);
	if (pClient == nullptr)
		return sPortName;

	return pClient->portAlias(sPortName);
}


// Load/save aliases definitions.
void qjackctlAliasList::loadSettings (
	QSettings& settings, const QString& sAliasesKey )
{
	clear();

	settings.beginGroup(sAliasesKey);
	QStringListIterator client_iter(settings.childGroups());
	while (client_iter.hasNext()) {
		const QString& sClientKey
			= client_iter.next();
		const QString& sClientName
			= settings.value(sClientKey + "/Name").toString();
		const QString& sClientAlias
			= settings.value(sClientKey + "/Alias").toString();
		if (!sClientName.isEmpty() && !sClientAlias.isEmpty()) {
			qjackctlAliasItem *pClient =
				new qjackctlAliasItem(sClientName, sClientAlias);
			append(pClient);
			settings.beginGroup(sClientKey);
			QStringListIterator port_iter(settings.childGroups());
			while (port_iter.hasNext()) {
				const QString& sPortKey
					= port_iter.next();
				const QString& sPortName
					= settings.value(sPortKey + "/Name").toString();
				const QString& sPortAlias
					= settings.value(sPortKey + "/Alias").toString();
				if (!sPortName.isEmpty() && !sPortAlias.isEmpty())
					pClient->setPortAlias(sPortName, sPortAlias);
			}
			settings.endGroup();
		}
	}
	settings.endGroup();
}


void qjackctlAliasList::saveSettings (
	QSettings& settings, const QString& sAliasesKey )
{
	std::sort(begin(), end());

	settings.beginGroup(sAliasesKey);
	int iClient = 0;
	QListIterator<qjackctlAliasItem *> iter(*this);
	while (iter.hasNext()) {
		(iter.next())->saveSettings(settings,
			"Client" + QString::number(++iClient));
	}
	settings.endGroup();
}


// end of qjackctlAliases.cpp
