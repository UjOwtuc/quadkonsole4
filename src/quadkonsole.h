/***************************************************************************
 *   Copyright (C) 2005 by Simon Perreault                                 *
 *   nomis80@nomis80.org                                                   *
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


#ifndef _QUADKONSOLE_H_
#define _QUADKONSOLE_H_

#include "konsole.h"

#include <KDE/KXmlGuiWindow>
#include <KDE/KParts/MainWindow>
#include <KDE/KLibLoader>

#include <QtCore/QEvent>
#include <QtGui/QClipboard>
#include <QtGui/QSplitter>

#include <vector>

class QGridLayout;


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
		QuadKonsole( int rows, int columns, const QStringList &cmds=QStringList() );
		~QuadKonsole();

	public slots:
		void focusKonsoleRight( void );
		void focusKonsoleLeft ( void );
		void focusKonsoleUp ( void );
		void focusKonsoleDown ( void );
		void reparent();
		void pasteClipboard ( void );

	private:
		void emitPaste( QClipboard::Mode mode );

		typedef std::vector<Konsole*> PartVector;
		PartVector mKonsoleParts;
		QSplitter *mRows;
		std::vector<QSplitter *> mRowLayouts;
};

class ExternalMainWindow : public KXmlGuiWindow
{
	Q_OBJECT
	public:
		ExternalMainWindow(KParts::ReadOnlyPart* part);
		~ExternalMainWindow();

	private slots:
		void close();

	private:
		KParts::ReadOnlyPart* m_part;
};

#endif // _QUADKONSOLE_H_

