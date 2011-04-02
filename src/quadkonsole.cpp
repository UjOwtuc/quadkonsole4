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
#include <kde_terminal_interface_v2.h>
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
#include <cerrno>

#include "quadkonsole.moc"

QuadKonsole::QuadKonsole(int rows, int columns, const QStringList &cmds)
		: KXmlGuiWindow()
{
	setupUi(rows, columns);
}


QuadKonsole::QuadKonsole(KParts::ReadOnlyPart* part)
		: KXmlGuiWindow()
{
	Konsole* k = new Konsole(this, part);
	mKonsoleParts.push_back(k);
	setupUi(1, 1);
	k->setLayout(mRowLayouts[0], 0, 0);
	showNormal();
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


void QuadKonsole::setupUi(int rows, int columns)
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
	QGridLayout *grid = new QGridLayout(centralWidget);
	mRows = new QSplitter(Qt::Vertical, centralWidget);
	mRows->setChildrenCollapsible(false);
	grid->addWidget(mRows, 0, 0);
	setCentralWidget(centralWidget);
	actionCollection()->addAssociatedWidget(centralWidget);

	for (int i = 0; i < rows; ++i)
	{
		QSplitter *row = new QSplitter(Qt::Horizontal, centralWidget);
		row->setChildrenCollapsible(false);
		mRowLayouts.push_back(row);
		mRows->addWidget(row);
		mRows->setStretchFactor(i, 1);
		for (int j = 0; j < columns; ++j)
		{
			Konsole* part;
			if (mKonsoleParts.size() > static_cast<unsigned long>(i*columns + j))
				part = mKonsoleParts[i*columns + j];
			else
			{
				part = new Konsole(centralWidget, mRowLayouts[i], i, j);
				mKonsoleParts.push_back(part);
			}

			actionCollection()->addAssociatedWidget(part->widget());
			mRowLayouts[i]->setStretchFactor(j, 1);
			connect(part, SIGNAL(partCreated()), SLOT(resetLayouts()));
		}
	}
	kDebug() << "finished setting up layouts for " << mKonsoleParts.size() << " parts" << endl;

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

	KAction* reparent = new KAction(KIcon("document-new"), "Reparent", this);
	reparent->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Return));
	actionCollection()->addAction("reparent", reparent);
	connect(reparent, SIGNAL(triggered(bool)), this, SLOT(reparent()));

	// The whole paste clipboard action does not work
	KStandardAction::paste(this, SLOT(pasteClipboard()), actionCollection());
	KAction *pasteClipboard = new KAction(KIcon("edit-paste"), "Paste &clipboard", this);
	pasteClipboard->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Insert));
	actionCollection()->addAction("pasteClipboard", pasteClipboard);
	connect(pasteClipboard, SIGNAL(triggered(bool)), this, SLOT(pasteClipboard()));

	setWindowIcon(KIcon("quadkonsole4"));
}


void QuadKonsole::emitPaste(QClipboard::Mode mode)
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


void QuadKonsole::focusKonsoleRight()
{
	for (PartVector::iterator part = mKonsoleParts.begin(); part != mKonsoleParts.end(); ++part)
	{
		if ((*part)->widget()->hasFocus())
		{
			++part;
			if (part == mKonsoleParts.end())
				part = mKonsoleParts.begin();

			(*part)->widget()->setFocus();
			break;
		}
	}
}


void QuadKonsole::focusKonsoleLeft()
{
	for (PartVector::reverse_iterator part = mKonsoleParts.rbegin(); part != mKonsoleParts.rend(); ++part)
	{
		if ((*part)->widget()->hasFocus())
		{
			++part;
			if (part == mKonsoleParts.rend())
				part = mKonsoleParts.rbegin();

			(*part)->widget()->setFocus();
			break;
		}
	}
}


void QuadKonsole::focusKonsoleUp()
{
	for (unsigned int i = 0; i < mRowLayouts.size() * mRowLayouts[0]->count(); ++i)
	{
		if (mKonsoleParts[i]->widget()->hasFocus())
		{
			int index = i - mRowLayouts[0]->count();
			if (index < 0)
				index += mRowLayouts.size() * mRowLayouts[0]->count();

			mKonsoleParts[index]->widget()->setFocus();
			break;
		}
	}
}


void QuadKonsole::focusKonsoleDown(void)
{
	for (unsigned int i = 0; i < mRowLayouts.size() * mRowLayouts[0]->count(); ++i)
	{
		if (mKonsoleParts[i]->widget()->hasFocus())
		{
			int index = i + mRowLayouts[0]->count();
			if (index >= static_cast<int>(mRowLayouts.size() * mRowLayouts[0]->count()))
				index -= mRowLayouts.size() * mRowLayouts[0]->count();

			mKonsoleParts[index]->widget()->setFocus();
			break;
		}
	}
}


void QuadKonsole::reparent()
{
	kDebug() << "reparent rows=" << mRowLayouts.size() << " row[0].columns=" << mRowLayouts[0]->count() << endl;
	for (unsigned int i = 0; i < mRowLayouts.size() * mRowLayouts[0]->count(); ++i)
	{
		if (mKonsoleParts[i]->widget()->hasFocus())
		{
			QuadKonsole* external = new QuadKonsole(mKonsoleParts[i]->part());
			actionCollection()->removeAssociatedWidget(mKonsoleParts[i]->widget());
			mKonsoleParts[i]->partDestroyed();
			break;
		}
	}
}


void QuadKonsole::resetLayouts()
{
	kDebug() << "resetting layouts" << endl;
	QList<int> sizes;
	int width = mRows->height() / mRows->count();
	for (int i=0; i<mRows->count(); ++i)
		sizes.append(width);
	mRows->setSizes(sizes);

	std::vector<QSplitter*>::iterator it;
	for (it=mRowLayouts.begin(); it!=mRowLayouts.end(); ++it)
	{
		width = (*it)->width() / (*it)->count();
		for (int i=0; i<(*it)->count(); ++i)
			sizes.append(width);
		(*it)->setSizes(sizes);
	}
}

