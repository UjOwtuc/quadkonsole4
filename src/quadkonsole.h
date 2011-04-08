/***************************************************************************
 *   Copyright (C) 2005 by Simon Perreault                                 *
 *   nomis80@nomis80.org                                                   *
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


#ifndef _QUADKONSOLE_H_
#define _QUADKONSOLE_H_

#include <KDE/KXmlGuiWindow>

#include <QtGui/QClipboard>

#include <vector>

class Konsole;
class QGridLayout;
class QSplitter;

namespace KParts
{
	class ReadOnlyPart;
}

/**
 * @short Application Main Window
 * @author Simon Perreault <nomis80@nomis80.org>
 * @version 2.0
 */
class QuadKonsole : public KXmlGuiWindow
{
	Q_OBJECT
	public:
		/**
		* Default Constructor
		*/
		QuadKonsole(int rows, int columns, const QStringList& cmds=QStringList());
		~QuadKonsole();

	public slots:
		void focusKonsoleRight();
		void focusKonsoleLeft();
		void focusKonsoleUp();
		void focusKonsoleDown();
		void reparent();
		void pasteClipboard();
		void resetLayouts();
		void toggleMenu();
		void optionsPreferences();
		void settingsChanged();
		void quit();
		void insertHorizontal();
		void insertVertical();
		void removePart();

	protected:
		bool queryClose();
		Konsole* getFocusPart();
		void getFocusCoords(int& row, int& col);

	private:
		QuadKonsole(KParts::ReadOnlyPart* part);
		void setupActions();
		void setupUi(int rows, int columns);
		void emitPaste(QClipboard::Mode mode);

		typedef std::vector<Konsole*> PartVector;
		PartVector mKonsoleParts;
		QSplitter* mRows;
		std::vector<QSplitter*> mRowLayouts;
};

#endif // _QUADKONSOLE_H_

