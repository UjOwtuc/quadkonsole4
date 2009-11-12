/***************************************************************************
 *   Copyright (C) 2005 by Simon Perreault   *
 *   nomis80@nomis80.org   *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dcopclient.h>
#include <kparts/mainwindow.h>

#include <vector>

class KLibFactory;
class MouseMoveFilter;
class QGridLayout;

/**
 * @short Application Main Window
 * @author Simon Perreault <nomis80@nomis80.org>
 * @version 2.0
 */
class QuadKonsole : public KParts::MainWindow
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    QuadKonsole( int rows, int columns, bool clickfocus,
            const QCStringList& cmds = QCStringList() );

    ~QuadKonsole();

protected:
    virtual void customEvent( QCustomEvent* e );

private slots:
    void partDestroyed();
    void focusKonsoleRight();
    void focusKonsoleLeft();
    void focusKonsoleUp();
    void focusKonsoleDown();
    void pasteSelection();
    void pasteClipboard();

private:
    KParts::ReadOnlyPart* createPart( int row, int column, const QCString& cmd );
    void emitPaste( bool mode );

    typedef std::vector<KParts::ReadOnlyPart*> PartVector;
    PartVector mKonsoleParts;

    MouseMoveFilter* mFilter;
    KLibFactory* mFactory;
    QGridLayout* mLayout;
};

#endif // _QUADKONSOLE_H_
