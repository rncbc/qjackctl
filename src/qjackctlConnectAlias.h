// qjackctlConnectAlias.h
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

#ifndef __qjackctlConnectAlias_h
#define __qjackctlConnectAlias_h

#include <qsettings.h>
#include <qregexp.h>
#include <qptrlist.h>
#include <qmap.h>

// Client item alias map.
class qjackctlClientAlias
{
public:

	// Constructor.
	qjackctlClientAlias (const QString& sClientName,
		const QString& sClientAlias = QString::null);
	
	// Default destructor.
	~qjackctlClientAlias ();

	// Client name accessor.
	QString clientName ();

	// Client name matcher.
	bool matchClientName (const QString& sClientName);

	// Client aliasing methods.
	const QString& clientAlias ();
	void setClientAlias (const QString& sClientAlias);

	// Port aliasing methods.
	QString portAlias (const QString& sPortName);
	void setPortAlias (const QString& sPortName,
		const QString& sPortAlias);

	// Save client/port aliases definitions.
	void saveSettings (QSettings& settings, const QString& sClientKey);

	// Escape and format a string as a regular expresion.
	static QString escapeRegExpDigits (const QString& s,
		unsigned int iThreshold = 3);

private:

	// Client name regexp.
	QRegExp m_rxClientName;
	// Client alias.
	QString m_sClientAlias;

	// Port aliases map.
	QMap<QString, QString> m_ports;
};


// Client list alias map.
class qjackctlConnectAlias : public QPtrList<qjackctlClientAlias>
{
public:

	// Constructor.
	qjackctlConnectAlias ();
	// Default destructor.
	~qjackctlConnectAlias ();

	// Client aliasing methods.
	QString clientAlias (const QString& sClientName);
	void setClientAlias (const QString& sClientName,
		const QString& sClientAlias);

	// Port aliasing methods.
	QString portAlias (const QString& sClientName,
		const QString& sPortName);
	void setPortAlias (const QString& sClientName,
		const QString& sPortName, const QString& sPortAlias);

	// Load/save aliases definitions.
	void loadSettings (QSettings& settings, const QString& sAliasesKey);
	void saveSettings (QSettings& settings, const QString& sAliasesKey);

protected:

	// Virtual override function to compare two list items.
	virtual int compareItems (QPtrCollection::Item pItem1,
		QPtrCollection::Item pItem2);

private:

	// Client item finder.
	qjackctlClientAlias *findClientName (const QString& sClientName);
};


#endif  // __qjackctlConnectAlias_h

// end of qjackctlConnectAlias.h
