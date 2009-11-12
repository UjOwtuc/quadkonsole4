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


#include "quadkonsole.h"

#include "mousemovefilter.h"

#include <kdebug.h>
#include <kde_terminal_interface.h>
#include <kiconloader.h>
#include <klibloader.h>
#include <klocale.h>
#include <kparts/mainwindow.h>

#include <qaction.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qlayout.h>

#include <algorithm>


QuadKonsole::QuadKonsole( int rows, int columns, bool clickfocus,
        const QCStringList& cmds )
    : KParts::MainWindow( 0, "QuadKonsole" )
    , mFilter(0)
{
    if ( rows < 1 ) {
        kdError() << "Number of rows must be at last one." << endl;
        exit(1);
    }
    if ( columns < 1 ) {
        kdError() << "Number of columns must be at least one." << endl;
        exit(1);
    }

    QWidget* centralWidget = new QWidget( this, "central widget" );
    mLayout = new QGridLayout( centralWidget, rows, columns, 0, 1 );
    setCentralWidget(centralWidget);

    if ( !clickfocus ) {
        mFilter = new MouseMoveFilter(this);
    }

    // Try to find libkonsolepart
    mFactory = KLibLoader::self()->factory( "libkonsolepart" );
    if (mFactory) {
        // Create the parts
        QCStringList::const_iterator cmd = cmds.constBegin();
        for ( int i = 0; i < rows; ++i ) {
            for ( int j = 0; j < columns; ++j ) {
                mKonsoleParts.push_back( createPart(i, j,
                            cmd != cmds.end() ? *cmd++ : QCString()) );
            }
        }
    }
    else {
        kdFatal() << "No libkonsolepart found !" << endl;
    }

    QAction* goRight = new QAction( QString::null, CTRL + SHIFT + Key_Right, this );
    connect( goRight, SIGNAL(activated()), this, SLOT(focusKonsoleRight()) );

    QAction* goLeft = new QAction( QString::null, CTRL + SHIFT + Key_Left, this );
    connect( goLeft, SIGNAL(activated()), this, SLOT(focusKonsoleLeft()) );

    QAction* goUp = new QAction( QString::null, CTRL + SHIFT + Key_Up, this );
    connect( goUp, SIGNAL(activated()), this, SLOT(focusKonsoleUp()) );

    QAction* goDown = new QAction( QString::null, CTRL + SHIFT + Key_Down, this );
    connect( goDown, SIGNAL(activated()), this, SLOT(focusKonsoleDown()) );

    QAction* pasteClipboard = new QAction( QString::null, SHIFT + Key_Insert, this );
    connect( pasteClipboard, SIGNAL(activated()), this, SLOT(pasteClipboard()) );

    QAction* pasteSelection = new QAction( QString::null, CTRL + SHIFT + Key_Insert, this );
    connect( pasteSelection, SIGNAL(activated()), this, SLOT(pasteSelection()) );

    setIcon( DesktopIcon("konsole") );
}

QuadKonsole::~QuadKonsole()
{
    for ( PartVector::const_iterator i = mKonsoleParts.begin(); i !=
            mKonsoleParts.end(); ++i ) {
        delete *i;
    }
}

void QuadKonsole::pasteSelection()
{
    emitPaste(true);
}

void QuadKonsole::pasteClipboard()
{
    emitPaste(false);
}

void QuadKonsole::emitPaste(bool mode)
{
    bool oldMode = QApplication::clipboard()->selectionModeEnabled();
    QApplication::clipboard()->setSelectionMode( mode );
    QString text = QApplication::clipboard()->text();
    for ( PartVector::iterator part = mKonsoleParts.begin(); part !=
            mKonsoleParts.end(); ++part )
    {
        if ( (*part)->widget()->hasFocus() ) {
            TerminalInterface* t = static_cast<TerminalInterface*>(
                    (*part)->qt_cast( "TerminalInterface" ) );
            t->sendInput(text);
            break;
        }
    }
    QApplication::clipboard()->setSelectionMode( oldMode );
}

KParts::ReadOnlyPart* QuadKonsole::createPart( int row, int column,
        const QCString& cmd )
{
    // Create a part
    KParts::ReadOnlyPart* part =
        dynamic_cast<KParts::ReadOnlyPart*>( mFactory->create(this,
                    QString("konsolepart-%1-%2").arg(row).arg(column),
                    "KParts::ReadOnlyPart") );
    connect( part, SIGNAL(destroyed()), SLOT(partDestroyed()) );

    // Execute cmd
    TerminalInterface* t = static_cast<TerminalInterface*>(
            part->qt_cast("TerminalInterface") );
    if ( !cmd.isEmpty() && t ) {
        QString shell( getenv("SHELL") );
        QStrList l;
        l.append( QStringList::split('/', shell).last() );
        l.append("-c");
        l.append(cmd);
        t->startProgram( shell, l );
    }

    // Make this part's widget activate on mouse over
    if (mFilter) {
        part->widget()->setMouseTracking(true);
        part->widget()->installEventFilter(mFilter);
    }

    // Add widget to layout
    part->widget()->reparent( centralWidget(), QPoint(0,0) );
    mLayout->addWidget( part->widget(), row, column );
    part->widget()->show();

    return part;
}

class PartDeletedEvent : public QCustomEvent
{
    public:
        PartDeletedEvent( int row, int column )
            : QCustomEvent( QEvent::User + 1 )
            , mRow(row)
            , mColumn(column)
        {
        }

        inline int row() const { return mRow; }
        inline int column() const { return mColumn; }

    private:
        int mRow, mColumn;
};

void QuadKonsole::partDestroyed()
{
    kdDebug() << k_funcinfo << endl;

    PartVector::iterator part = std::find( mKonsoleParts.begin(),
            mKonsoleParts.end(), sender() );
    if ( part != mKonsoleParts.end() ) {
        *part = 0;

        // A terminal emulator has been destroyed. Plan to start a new one.
        int index = part - mKonsoleParts.begin();
        int row = index / mLayout->numCols();
        int col = index % mLayout->numCols();

        QApplication::postEvent( this, new PartDeletedEvent(row, col) );
    }
}

void QuadKonsole::customEvent( QCustomEvent* e )
{
    kdDebug() << k_funcinfo << endl;

    if ( e->type() == QEvent::User + 1 ) {
        // Start a new part.
        PartDeletedEvent* pde = static_cast<PartDeletedEvent*>(e);
        mKonsoleParts[ pde->row() * mLayout->numCols() + pde->column() ] =
            createPart( pde->row(), pde->column(), QCString() );
    }
}

void QuadKonsole::focusKonsoleRight()
{
    for ( PartVector::iterator part = mKonsoleParts.begin(); part !=
            mKonsoleParts.end(); ++part ) {
        if ( (*part)->widget()->hasFocus() ) {
            ++part;
            if ( part == mKonsoleParts.end() ) {
                part = mKonsoleParts.begin();
            }
            (*part)->widget()->setFocus();
            break;
        }
    }
}

void QuadKonsole::focusKonsoleLeft()
{
    for ( PartVector::reverse_iterator part = mKonsoleParts.rbegin(); part !=
            mKonsoleParts.rend(); ++part ) {
        if ( (*part)->widget()->hasFocus() ) {
            ++part;
            if ( part == mKonsoleParts.rend() ) {
                part = mKonsoleParts.rbegin();
            }
            (*part)->widget()->setFocus();
            break;
        }
    }
}

void QuadKonsole::focusKonsoleUp()
{
    for ( int i = 0; i < mLayout->numRows() * mLayout->numCols(); ++i ) {
        if ( mKonsoleParts[i]->widget()->hasFocus() ) {
            i -= mLayout->numCols();
            if ( i < 0) {
                i += mLayout->numRows() * mLayout->numCols();
            }
            mKonsoleParts[i]->widget()->setFocus();
            break;
        }
    }
}

void QuadKonsole::focusKonsoleDown()
{
    for ( int i = 0; i < mLayout->numRows() * mLayout->numCols(); ++i ) {
        if ( mKonsoleParts[i]->widget()->hasFocus() ) {
            i += mLayout->numCols();
            if ( i >= mLayout->numRows() * mLayout->numCols()) {
                i -= mLayout->numRows() * mLayout->numCols();
            }
            mKonsoleParts[i]->widget()->setFocus();
            break;
        }
    }
}

#include "quadkonsole.moc"
