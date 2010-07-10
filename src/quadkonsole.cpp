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
#include "konsole.h"

#include <KDE/KDebug>
#include <kde_terminal_interface.h>
#include <KDE/KIconLoader>
#include <KDE/KLibLoader>
#include <KDE/KLocale>
#include <KDE/KXmlGuiWindow>
#include <KDE/KActionCollection>
#include <KDE/KMenuBar>
#include <KDE/KMenu>
#include <KDE/KStandardAction>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QLayout>

#include <algorithm>

#include "quadkonsole.moc"


QuadKonsole::QuadKonsole(int rows, int columns, const QStringList &cmds)
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
	actionCollection()->addAssociatedWidget(centralWidget);

	for (int i = 0; i < rows; ++i)
	{
		for (int j = 0; j < columns; ++j)
		{
			//mKonsoleParts.push_back(new Konsole(centralWidget, mLayout, i, j));
			Konsole *part = new Konsole(centralWidget, mLayout, i, j);
			actionCollection()->addAssociatedWidget(part->widget());
			mKonsoleParts.push_back(part);
		}
	}

	KAction* goRight = new KAction(KIcon("arrow-right"), "Right", this);

	goRight->setShortcut(KShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Right)));
	actionCollection()->addAction("go right", goRight);
	connect(goRight, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleRight()));

	KAction* goLeft = new KAction(KIcon("arrow-left"), "Left", this);
	goLeft->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Left));
	actionCollection()->addAction("go left", goLeft);
	connect(goLeft, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleLeft()));

	KAction* goUp = new KAction(KIcon("arrow-up"), "Up", this);
	goUp->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Up));
	actionCollection()->addAction("go up", goUp);
	connect(goUp, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleUp()));

	KAction* goDown = new KAction(KIcon("arrow-down"), "Down", this);
	goDown->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Down));
	actionCollection()->addAction("go down", goDown);
	connect(goDown, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleDown()));

	// The whole paste clipboard action does not work
	KStandardAction::paste(this, SLOT(pasteClipboard()), actionCollection());
	KAction *pasteClipboard = new KAction(KIcon("edit-paste"), "Paste &clipboard", this);
	pasteClipboard->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Insert));
	actionCollection()->addAction("pasteClipboard", pasteClipboard);
	connect(pasteClipboard, SIGNAL(triggered(bool)), this, SLOT(pasteClipboard()));

	setWindowIcon(KIcon("quadkonsole4"));
}


QuadKonsole::~QuadKonsole(void)
{
	for (PartVector::const_iterator i = mKonsoleParts.begin(); i != mKonsoleParts.end(); ++i)
	{
		delete *i;
	}
}


void QuadKonsole::pasteClipboard ( void )
{
	mKonsoleParts.front()->sendInput("input ...");
	emitPaste(QClipboard::Clipboard);
}


void QuadKonsole::emitPaste ( QClipboard::Mode mode )
{
	QString text = QApplication::clipboard()->text(mode);
	for (PartVector::iterator part=mKonsoleParts.begin(); part!=mKonsoleParts.end(); ++part)
	{
		if ((*part)->widget()->hasFocus())
		{
			(*part)->sendInput(text);
			//TerminalInterface* t = qobject_cast<TerminalInterface*>(*part);
			//t->sendInput(text);
			break;
		}
	}
}


void QuadKonsole::focusKonsoleRight(void)
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


void QuadKonsole::focusKonsoleLeft(void)
{
	for (PartVector::reverse_iterator part = mKonsoleParts.rbegin(); part != mKonsoleParts.rend(); ++part)
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


void QuadKonsole::focusKonsoleUp(void)
{
	for (int i = 0; i < mLayout->rowCount() * mLayout->columnCount(); ++i)
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


void QuadKonsole::focusKonsoleDown(void)
{
	for (int i = 0; i < mLayout->rowCount() * mLayout->columnCount(); ++i)
	{
		if (mKonsoleParts[i]->widget()->hasFocus())
		{
			i += mLayout->columnCount();

			if (i >= mLayout->rowCount() * mLayout->columnCount())
			{
				i -= mLayout->rowCount() * mLayout->columnCount();
			}

			mKonsoleParts[i]->widget()->setFocus();

			break;
		}
	}
}
