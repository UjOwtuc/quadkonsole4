/***************************************************************************
 *   Copyright (C) 2009 - 2017 by Karsten Borgwaldt                        *
 *   kb@spambri.de                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "qkhistory.h"
#include "settings.h"

#include <KParts/HistoryProvider>

#include <QString>
#include <QStringList>

QKHistory* QKHistory::m_instace = 0;


QKHistory* QKHistory::self()
{
	if (! m_instace)
		m_instace = new QKHistory;

	return m_instace;
}


QKHistory::QKHistory()
	: m_historyPosition(-1),
	m_locked(false)
{
	if (m_historyPosition == -1 && m_history.count())
		m_historyPosition = m_history.count() -1;
}


QKHistory::~QKHistory()
{}


void QKHistory::setHistory(const QStringList& history)
{
	m_history = history;
}


void QKHistory::setPosition(int pos)
{
	m_historyPosition = pos;
}


void QKHistory::lock(bool lock)
{
	m_locked = lock;
}


bool QKHistory::canGoBack() const
{
	return m_historyPosition > 0;
}


bool QKHistory::canGoForward() const
{
	return m_historyPosition < m_history.count() -1;
}


void QKHistory::addEntry(const QString& url)
{
	// ignore reloads and openUrlNotify on forward/back
	if (m_historyPosition >= 0 && m_historyPosition < m_history.count() && m_history.at(m_historyPosition) == url)
		return;

	if (! m_locked)
	{
		// remove forward history
		while (m_history.count() && m_historyPosition < m_history.count() -1)
			m_history.removeLast();

		m_history.append(url);
		KParts::HistoryProvider::self()->insert(url);
		m_history.removeDuplicates();

		while (static_cast<unsigned int>(m_history.count()) > Settings::historySize())
			m_history.removeFirst();

		m_historyPosition = m_history.count() -1;
	}
}


QString QKHistory::goBack()
{
	return go(1);
}


QString QKHistory::goForward()
{
	return go(-1);
}


QString QKHistory::go(int steps)
{
	if (m_historyPosition - steps < 0 || m_historyPosition - steps > m_history.count() -1)
		return QString();

	m_historyPosition -= steps;
	return m_history.at(m_historyPosition);
}
