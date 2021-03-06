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

#ifndef QKAPPLICATION_H
#define QKAPPLICATION_H

#include <QObject>

#include <QPointer>

class QuadKonsole;
class QKApplicationAdaptor;


class QKApplication : public QObject
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "de.spambri.quadkonsole4.QKApplication")
	public:
		QKApplication();
		virtual ~QKApplication();

		void exec();

	public slots:
		Q_SCRIPTABLE uint windowCount() const;

	private slots:
		void setupWindow(QuadKonsole* mainWindow);
		void windowDestroyed();

	private:
		static QKApplicationAdaptor* m_dbusAdaptor;
		QList< QPointer<QuadKonsole> > m_mainWindows;
};


#endif // QKAPPLICATION_H
