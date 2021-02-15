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

#ifndef QKHISTORY_H
#define QKHISTORY_H

#include <QObject>
#include <QString>
#include <QStringList>

#ifdef HAVE_LIBKONQ
#include <konq_historyprovider.h>
class QKHistory : public KonqHistoryProvider
#else
class QKHistory : public QObject
#endif
{
	public:
		static QKHistory* self();
		virtual ~QKHistory();

		void setHistory(const QStringList& history);
		void setPosition(int pos);
		const QStringList& history() const { return m_history; }
		int position() const { return m_historyPosition; }
		int count() const { return m_history.count(); }

		void lock(bool lock=true);

		bool canGoBack() const;
		bool canGoForward() const;
		void addEntry(const QString& url);

		QString goBack();
		QString goForward();
		QString go(int steps);

	private:
		QKHistory();

		static QKHistory* m_instace;

		QStringList m_history;
		int m_historyPosition;
		bool m_locked;
};

#endif // QKHISTORY_H
