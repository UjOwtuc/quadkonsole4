/***************************************************************************
 *   Copyright (C) 2009 - 2011 by Karsten Borgwaldt                        *
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

#include <QtCore/QObject>

class QLayout;
class QStackedWidget;
namespace KParts
{
	class ReadOnlyPart;
}

/**
 * @short Contains a terminal emulator
 * @author Karsten Borgwaldt <kb@kb.ccchl.de>
 * @version 2.1
 */
class Konsole : public QObject
{
	Q_OBJECT

	public:
		Konsole(QWidget* parent, QLayout* layout);
		Konsole(QWidget* parent, KParts::ReadOnlyPart* part);
		virtual ~Konsole();

		void sendInput(const QString& text);
		virtual void setParent(QWidget* parent);
		void setLayout(QLayout* layout);

		QString foregroundProcessName();
		QString workingDir();

		KParts::ReadOnlyPart* part() { return m_konsolePart; }
		QWidget* widget();

	signals:
		void destroyed();
		void partCreated();

	public slots:
		void partDestroyed();
		void focusNext();
		
	private:
		void createPart();

		QWidget* m_parent;
		QLayout* m_layout;
		QStackedWidget* m_stack;
		KParts::ReadOnlyPart* m_konsolePart;
		KParts::ReadOnlyPart* m_dolphinPart;
};

#endif // KONSOLE_H
