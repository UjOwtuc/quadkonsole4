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


#include "quadkonsole.h"

#include <KDE/KDebug>
#include <kde_terminal_interface.h>
#include <KDE/KIconLoader>
#include <KDE/KLibLoader>
#include <KDE/KLocale>
#include <KDE/KXmlGuiWindow>
#include <KDE/KActionCollection>
#include <KDE/KMenuBar>
#include <KDE/KMenu>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QLayout>

#include <algorithm>

#include "quadkonsole.moc"


QuadKonsole::QuadKonsole ( int rows, int columns, const QStringList &cmds )
	: KXmlGuiWindow()
{
	if (rows < 1)
	{
		kdError() << "Number of rows must be at last one." << endl;
		exit(1);
	}
	if (columns < 1)
	{
		kdError() << "Number of columns must be at least one." << endl;
		exit(1);
	}

	QWidget* centralWidget = new QWidget(this, 0);
	mLayout = new QGridLayout(centralWidget);
	setCentralWidget(centralWidget);

	// Try to find libkonsolepart
	mFactory = KPluginLoader("libkonsolepart").factory();
	if (mFactory)
	{
		// Create the parts
		for (int i=0; i<rows; ++i)
		{
			for (int j=0; j<columns; ++j)
			{
				mKonsoleParts.push_back(createPart(i, j, cmds));
			}
		}
	}
	else
	{
		kdFatal() << "No libkonsolepart found !" << endl;
	}

	// KAction's did not work before adding them to a (invisible) KMenuBar
	// for a standard menubar uncomment this:
	// KMenuBar *mb = menuBar();
	KMenuBar *mb = new KMenuBar(this);

	KMenu *m = new KMenu("QuadKonsole", mb);
	mb->addMenu(m);
	mb->show();
	m->show();

	KAction* goRight = new KAction(KIcon("arrow-right"), "Right", this);
	goRight->setShortcut(KShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Right)));
	m->addAction(goRight);
	connect(goRight, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleRight()));

	KAction* goLeft = new KAction(KIcon("arrow-left"), "Left", this);
	goLeft->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Left));
	m->addAction(goLeft);
	connect(goLeft, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleLeft()));

	KAction* goUp = new KAction(KIcon("arrow-up"), "Up", this);
	goUp->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Up));
	m->addAction(goUp);
	connect(goUp, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleUp()));

	KAction* goDown = new KAction(KIcon("arrow-down"), "Down", this);
	goDown->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Down));
	m->addAction(goDown);
	connect(goDown, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleDown()));

	KAction* pasteClipboard = new KAction(KIcon("edit-paste"), "Paste &clipboard", this);
	pasteClipboard->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Insert));
	m->addAction(pasteClipboard);
	connect(pasteClipboard, SIGNAL(triggered(bool)), this, SLOT(pasteClipboard()));

//	KAction* pasteSelection = new KAction(KIcon("edit-paste"), "Paste &selection", this);
//	pasteSelection->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Insert));
//	m->addAction(pasteSelection);
//	connect(pasteSelection, SIGNAL(triggered(bool)), this, SLOT(pasteSelection()));
}


QuadKonsole::~QuadKonsole ( void )
{
	for (PartVector::const_iterator i=mKonsoleParts.begin(); i!=mKonsoleParts.end(); ++i)
	{
		delete *i;
	}
}


void QuadKonsole::pasteSelection ( void )
{
	emitPaste(QClipboard::Selection);
}


void QuadKonsole::pasteClipboard ( void )
{
	emitPaste(QClipboard::Clipboard);
}


void QuadKonsole::emitPaste ( QClipboard::Mode mode )
{
	QString text = QApplication::clipboard()->text(mode);
	for (PartVector::iterator part=mKonsoleParts.begin(); part!=mKonsoleParts.end(); ++part)
	{
		if ((*part)->widget()->hasFocus())
		{
			TerminalInterface* t = qobject_cast<TerminalInterface*>(*part);
			t->sendInput(text);
			break;
		}
	}
}


KParts::ReadOnlyPart* QuadKonsole::createPart( int row, int column, const QStringList& )
{
	// Create a part
	KParts::ReadOnlyPart* part = dynamic_cast<KParts::ReadOnlyPart*>(mFactory->create<QObject>(this, this));
	connect(part, SIGNAL(destroyed()), SLOT(partDestroyed()));

	// Execute cmd
	TerminalInterface* t = qobject_cast<TerminalInterface*>(part);
	if (t)
		t->showShellInDir(QString());
	else
	{
		kdError() << "could not get TerminalInterface for part row=" << row << " column=" << column << endl;
		exit(1);
	}

	// Add widget to layout
	part->widget()->setParent(centralWidget());
	mLayout->addWidget(part->widget(), row, column);
	part->widget()->show();

	return part;
}


class PartDeletedEvent : public QEvent
{
	public:
		PartDeletedEvent ( int row, int column )
			: QEvent(QEvent::User),
			mRow(row),
			mColumn(column)
		{}

		inline int row ( void ) const { return mRow; }
		inline int column ( void ) const { return mColumn; }

	private:
		int mRow, mColumn;
};


void QuadKonsole::partDestroyed ( void )
{
	kdDebug() << k_funcinfo << endl;

	PartVector::iterator part = std::find(mKonsoleParts.begin(), mKonsoleParts.end(), sender());
	if (part != mKonsoleParts.end())
	{
		*part = 0;

		// A terminal emulator has been destroyed. Plan to start a new one.
		int index = part - mKonsoleParts.begin();
		int row = index / mLayout->columnCount();
		int col = index % mLayout->columnCount();

		QApplication::postEvent(this, new PartDeletedEvent(row, col));
	}
}


void QuadKonsole::customEvent ( QEvent* e )
{
	kdDebug() << k_funcinfo << endl;

	if (e->type() == QEvent::User)
	{
		// Start a new part.
		PartDeletedEvent* pde = static_cast<PartDeletedEvent*>(e);
		mKonsoleParts[pde->row() * mLayout->columnCount() + pde->column()] = createPart(pde->row(), pde->column(), QStringList());
		mKonsoleParts[pde->row() * mLayout->columnCount() + pde->column()]->widget()->setFocus();
	}
}


void QuadKonsole::focusKonsoleRight ( void )
{
	for (PartVector::iterator part = mKonsoleParts.begin(); part != mKonsoleParts.end(); ++part)
	{
		if ((*part)->widget()->hasFocus())
		{
			++part;
			if (part == mKonsoleParts.end())
			{
				part = mKonsoleParts.begin();
			}
			(*part)->widget()->setFocus();
			break;
		}
	}
}


void QuadKonsole::focusKonsoleLeft ( void )
{
	for (PartVector::reverse_iterator part=mKonsoleParts.rbegin(); part!=mKonsoleParts.rend(); ++part)
	{
		if ((*part)->widget()->hasFocus())
		{
			++part;
			if (part == mKonsoleParts.rend())
			{
				part = mKonsoleParts.rbegin();
			}
			(*part)->widget()->setFocus();
			break;
		}
	}
}


void QuadKonsole::focusKonsoleUp ( void )
{
	for (int i=0; i<mLayout->rowCount() * mLayout->columnCount(); ++i)
	{
		if (mKonsoleParts[i]->widget()->hasFocus())
		{
			i -= mLayout->columnCount();
			if (i < 0)
			{
				i += mLayout->rowCount() * mLayout->columnCount();
			}
			mKonsoleParts[i]->widget()->setFocus();
			break;
		}
	}
}


void QuadKonsole::focusKonsoleDown ( void )
{
	for (int i=0; i<mLayout->rowCount() * mLayout->columnCount(); ++i)
	{
		if (mKonsoleParts[i]->widget()->hasFocus())
		{
			i += mLayout->columnCount();
			if ( i >= mLayout->rowCount() * mLayout->columnCount())
			{
				i -= mLayout->rowCount() * mLayout->columnCount();
			}
			mKonsoleParts[i]->widget()->setFocus();
			break;
		}
	}
}

