/***************************************************************************
 *   Copyright (C) 2009 by Karsten Borgwaldt                               *
 *   kb@kb.ccchl.de                                                        *
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

#ifndef KONSOLE_H
#define KONSOLE_H

#include <KDE/KParts/ReadOnlyPart>

#include <QtGui/QWidget>
#include <QtGui/QSplitter>
#include <QtGui/QDockWidget>


/**
 * @short Contains a terminal emulator
 * @author Karsten Borgwaldt <kb@kb.ccchl.de>
 * @version 2.1
 */
class Konsole : public QWidget
{
	Q_OBJECT

	public:
		Konsole(QWidget *parent, QSplitter *layout, int row, int column);
		Konsole(QWidget *parent, KParts::ReadOnlyPart* part);
		~Konsole();

		void sendInput(const QString &text);
		void setLayout(QSplitter* layout, int row, int column);
		QString foregroundProcessName();

		KParts::ReadOnlyPart* part() { return m_part; }
		QWidget* widget() { if (m_part) { return m_part->widget(); } return 0; }

	signals:
		void destroyed();
		void partCreated();

	public slots:
		void partDestroyed();

	private slots:
		void partCreateCompleted();

	private:
		void createPart();

		QSplitter* m_layout;
		KParts::ReadOnlyPart* m_part;
		int m_row;
		int m_column;
};

#endif // KONSOLE_H
