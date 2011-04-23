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

#include "quadkonsole.h"
#include "konsole.h"
#include "mousemovefilter.h"
#include "settings.h"

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
#include <KDE/KToggleAction>
#include <KDE/KStatusBar>
#include <KDE/KToolBar>
#include <KDE/KConfigDialog>
#include <KDE/KMessageBox>

#include <QtGui/QMouseEvent>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QLayout>
#include <QtGui/QSplitter>

#include <algorithm>
#include <cerrno>

#include "ui_prefs_base.h"
#include "ui_prefs_shutdown.h"


QuadKonsole::QuadKonsole()
	: KXmlGuiWindow(),
	mFilter(0)
{
	setupActions();
	setupUi(Settings::numRows(), Settings::numCols());
}


QuadKonsole::QuadKonsole(int rows, int columns, const QStringList& cmds)
	: KXmlGuiWindow(),
	mFilter(0)
{
	if (rows == 0)
		rows = Settings::numRows();
	if (columns == 0)
		columns = Settings::numCols();

	setupActions();
	setupUi(rows, columns);

	if (cmds.size())
	{
		int max = std::min(cmds.size(), static_cast<int>(mKonsoleParts.size()));
		for (int i=0; i<max; ++i)
			mKonsoleParts[i]->sendInput(cmds[i] + "\n");
	}
}


QuadKonsole::QuadKonsole(KParts::ReadOnlyPart* part)
	: KXmlGuiWindow(),
	mFilter(0)
{
	Konsole* k = new Konsole(this, part);
	mKonsoleParts.push_back(k);

	setupActions();
	setupUi(1, 1);
	k->setLayout(mRowLayouts[0], 0);
	showNormal();
}


QuadKonsole::~QuadKonsole()
{
	delete mFilter;
	for (PartVector::const_iterator i = mKonsoleParts.begin(); i != mKonsoleParts.end(); ++i)
	{
		delete *i;
	}
}


void QuadKonsole::pasteClipboard()
{
	emitPaste(QClipboard::Clipboard);
}


void QuadKonsole::setupActions()
{
	if (Settings::sloppyFocus())
	{
		mFilter = new MouseMoveFilter(this);
		qApp->installEventFilter(mFilter);
	}

	// Movement
	KAction* goRight = new KAction(KIcon("arrow-right"), i18n("&Right"), this);
	goRight->setShortcut(KShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Right)));
	actionCollection()->addAction("go right", goRight);
	connect(goRight, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleRight()));

	KAction* goLeft = new KAction(KIcon("arrow-left"), i18n("&Left"), this);
	goLeft->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Left));
	actionCollection()->addAction("go left", goLeft);
	connect(goLeft, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleLeft()));

	KAction* goUp = new KAction(KIcon("arrow-up"), i18n("&Up"), this);
	goUp->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Up));
	actionCollection()->addAction("go up", goUp);
	connect(goUp, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleUp()));

	KAction* goDown = new KAction(KIcon("arrow-down"), i18n("&Down"), this);
	goDown->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Down));
	actionCollection()->addAction("go down", goDown);
	connect(goDown, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleDown()));

	// Adding and removing parts
	KAction* detach = new KAction(KIcon("document-new"), i18n("De&tach"), this);
	detach->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Return));
	actionCollection()->addAction("detach", detach);
	connect(detach, SIGNAL(triggered(bool)), this, SLOT(detach()));

	KAction *insertHorizontal = new KAction(KIcon("view-split-left-right"), i18n("Insert &horizontal"), this);
	insertHorizontal->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_H));
	actionCollection()->addAction("insert horizontal", insertHorizontal);
	connect(insertHorizontal, SIGNAL(triggered(bool)), this, SLOT(insertHorizontal()));

	KAction *insertVertical = new KAction(KIcon("view-split-top-bottom"), i18n("Insert &vertical"), this);
	insertVertical->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_K));
	actionCollection()->addAction("insert vertical", insertVertical);
	connect(insertVertical, SIGNAL(triggered(bool)), this, SLOT(insertVertical()));

	KAction* removePart = new KAction(KIcon("view-left-close"), i18n("Re&move part"), this);
	removePart->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_R));
	actionCollection()->addAction("remove part", removePart);
	connect(removePart, SIGNAL(triggered(bool)), this, SLOT(removePart()));

	// View
	KAction *resetLayouts = new KAction(KIcon("view-grid"), i18n("R&eset layouts"), this);
	actionCollection()->addAction("reset layouts", resetLayouts);
	connect(resetLayouts, SIGNAL(triggered(bool)), this, SLOT(resetLayouts()));

	// The whole paste clipboard action does not work
	KStandardAction::paste(this, SLOT(pasteClipboard()), actionCollection());
	KAction *pasteClipboard = new KAction(KIcon("edit-paste"), i18n("Paste &clipboard"), this);
	pasteClipboard->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Insert));
	actionCollection()->addAction("pasteClipboard", pasteClipboard);
	connect(pasteClipboard, SIGNAL(triggered(bool)), this, SLOT(pasteClipboard()));

	// Standard actions
	KStandardAction::preferences(this, SLOT(optionsPreferences()), actionCollection());
	KStandardAction::quit(this, SLOT(quit()), actionCollection());
	KToggleAction* toggleMenu = KStandardAction::showMenubar(this, SLOT(toggleMenu()), actionCollection());
	toggleMenu->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M));
}


void QuadKonsole::setupUi(int rows, int columns)
{
	if (rows < 1)
	{
		kdError() << "Number of rows must be at last one." << endl;
		qApp->quit();
	}

	if (columns < 1)
	{
		kdError() << "Number of columns must be at least one." << endl;
		qApp->quit();
	}

	QWidget* centralWidget = new QWidget(this, 0);
	QGridLayout* grid = new QGridLayout(centralWidget);

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
		for (int j = 0; j < columns; ++j)
		{
			Konsole* part;
			if (mKonsoleParts.size() > static_cast<unsigned long>(i*columns + j))
				part = mKonsoleParts[i*columns + j];
			else
			{
				part = new Konsole(centralWidget, mRowLayouts[i], j);
				mKonsoleParts.push_back(part);
			}

			actionCollection()->addAssociatedWidget(part->widget());
			connect(part, SIGNAL(partCreated()), SLOT(resetLayouts()));
		}
	}
	kDebug() << "finished setting up layouts for " << mKonsoleParts.size() << " parts" << endl;

	setWindowIcon(KIcon("quadkonsole4"));

	setupGUI();

	// unused
	statusBar()->hide();
}


void QuadKonsole::emitPaste(QClipboard::Mode mode)
{
	QString text = QApplication::clipboard()->text(mode);
	Konsole* part = getFocusPart();
	if (part)
		part->sendInput(text);
}


void QuadKonsole::focusKonsoleRight()
{
	int row, col;
	getFocusCoords(row, col);
	if (row >= 0 && col >= 0)
	{
		++col;
		if (col >= mRowLayouts[row]->count())
		{
			row = (row +1) % mRowLayouts.size();
			col = 0;
		}
		mRowLayouts[row]->widget(col)->setFocus();
	}
}


void QuadKonsole::focusKonsoleLeft()
{
	int row, col;
	getFocusCoords(row, col);
	if (row >= 0 && col >= 0)
	{
		--col;
		if (col < 0)
		{
			row = (row + mRowLayouts.size() -1) % mRowLayouts.size();
			col = mRowLayouts[row]->count() -1;
		}
		mRowLayouts[row]->widget(col)->setFocus();
	}
}


void QuadKonsole::focusKonsoleUp()
{
	int row, col;
	getFocusCoords(row, col);
	if (row >= 0 && col >= 0)
	{
		row = (row + mRowLayouts.size() -1) % mRowLayouts.size();
		if (col >= mRowLayouts[row]->count())
			col = mRowLayouts[row]->count() -1;
		mRowLayouts[row]->widget(col)->setFocus();
	}
}


void QuadKonsole::focusKonsoleDown()
{
	int row, col;
	getFocusCoords(row, col);
	if (row >= 0 && col >= 0)
	{
		row = (row +1) % mRowLayouts.size();
		if (col >= mRowLayouts[row]->count())
			col = mRowLayouts[row]->count() -1;
		mRowLayouts[row]->widget(col)->setFocus();
	}
}


void QuadKonsole::detach()
{
	Konsole* part = getFocusPart();
	QuadKonsole* external = new QuadKonsole(part->part());
	actionCollection()->removeAssociatedWidget(part->widget());
	part->partDestroyed();
	external->setAttribute(Qt::WA_DeleteOnClose);
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
		sizes.clear();
		width = (*it)->width() / (*it)->count();
		for (int i=0; i<(*it)->count(); ++i)
			sizes.append(width);
		(*it)->setSizes(sizes);
	}
}


void QuadKonsole::toggleMenu()
{
	if (menuBar()->isVisible())
		menuBar()->hide();
	else
		menuBar()->show();
}


void QuadKonsole::optionsPreferences()
{
	if (KConfigDialog::showDialog("settings"))
		return;

	KConfigDialog* dialog = new KConfigDialog(this, "settings", Settings::self());

	QWidget* generalSettings = new QWidget;
	Ui::prefs_base prefs_base;
	prefs_base.setupUi(generalSettings);
	dialog->addPage(generalSettings, i18n("General"), "quadkonsole4");

	QWidget* shutdownSettings = new QWidget;
	Ui::prefs_shutdown prefs_shutdown;
	prefs_shutdown.setupUi(shutdownSettings);
	dialog->addPage(shutdownSettings, i18n("Shutdown"), "application-exit");

	connect(dialog, SIGNAL(settingsChanged(QString)), this, SLOT(settingsChanged()));
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->show();
}


void QuadKonsole::settingsChanged()
{
	if (Settings::sloppyFocus() && mFilter == 0)
	{
		mFilter = new MouseMoveFilter;
		qApp->installEventFilter(mFilter);
	}
	else if (! Settings::sloppyFocus())
	{
		delete mFilter;
		mFilter = 0;
	}
}


void QuadKonsole::quit()
{
	deleteLater();
}


void QuadKonsole::updateColumnIds(int row)
{
	// update column attributes of all widgets in this row
	for (int i=0; i<mRowLayouts[row]->count(); ++i)
	{
		for (PartVector::iterator it=mKonsoleParts.begin(); it!=mKonsoleParts.end(); ++it)
		{
			if ((*it)->widget() == mRowLayouts[row]->widget(i))
				(*it)->setColumn(i);
		}
	}
}


bool QuadKonsole::queryClose()
{
	if (Settings::queryClose())
	{
		QStringList processes;
		for (PartVector::iterator it=mKonsoleParts.begin(); it!=mKonsoleParts.end(); ++it)
		{
			QString process = (*it)->foregroundProcessName();
			if (process.size())
			{
				processes << process;
			}
		}

		if (processes.count())
		{
			QString warning = i18n("The followin processes are still running. Are you sure you want to exit QuadKonsole4?");
			switch (KMessageBox::questionYesNoList(this, warning, processes))
			{
				case KMessageBox::Yes :
					return true;
				default :
					return false;
			}
		}
	}

	return true;
}


Konsole* QuadKonsole::getFocusPart()
{
	for (PartVector::iterator it=mKonsoleParts.begin(); it!=mKonsoleParts.end(); ++it)
	{
		if ((*it)->widget()->hasFocus())
			return *it;
	}
	kDebug() << "could not find focus" << endl;
	return 0;
}


void QuadKonsole::getFocusCoords(int& row, int& col)
{
	for (row=0; static_cast<unsigned long>(row)<mRowLayouts.size(); ++row)
	{
		for (col=0; col<mRowLayouts[row]->count(); ++col)
		{
			if (mRowLayouts[row]->widget(col)->hasFocus())
			{
				return;
			}
		}
	}
	kDebug() << "could not find focus" << endl;
	row = -1;
	col = -1;
}


void QuadKonsole::insertHorizontal()
{
	int row, col;
	getFocusCoords(row, col);
	if (row >= 0 && col >= 0)
	{
		Konsole* part = new Konsole(centralWidget(), mRowLayouts[row], col);
		mKonsoleParts.push_back(part);
		actionCollection()->addAssociatedWidget(part->widget());
		connect(part, SIGNAL(partCreated()), SLOT(resetLayouts()));
		part->widget()->setFocus();

		updateColumnIds(row);
		resetLayouts();
	}
}


void QuadKonsole::insertVertical()
{
	int row, col;
	getFocusCoords(row, col);
	if (row >= 0 && col >= 0)
	{
		QSplitter *newRow = new QSplitter(Qt::Horizontal, centralWidget());
		newRow->setChildrenCollapsible(false);
		mRows->insertWidget(row, newRow);

		std::vector<QSplitter*>::iterator it = mRowLayouts.begin();
		it += row;
		mRowLayouts.insert(it, newRow);

		Konsole* part = new Konsole(centralWidget(), newRow, col);
		mKonsoleParts.push_back(part);
		actionCollection()->addAssociatedWidget(part->widget());
		connect(part, SIGNAL(partCreated()), SLOT(resetLayouts()));
		part->widget()->setFocus();
		resetLayouts();
	}
}


void QuadKonsole::removePart()
{
	int row, col;
	getFocusCoords(row, col);
	if (row >= 0 && col >= 0)
	{
		Konsole* part = getFocusPart();
		for (PartVector::iterator it=mKonsoleParts.begin(); it!=mKonsoleParts.end(); ++it)
		{
			if ((*it)->widget()->hasFocus())
			{
				mKonsoleParts.erase(it);
				break;
			}
		}

		delete part;
		if (mRowLayouts[row]->count() < 1)
		{
			QSplitter* splitter = mRowLayouts[row];
			std::vector<QSplitter*>::iterator it = mRowLayouts.begin();
			it += row;
			mRowLayouts.erase(it);
			delete splitter;
		}

		if (mKonsoleParts.empty())
			quit();
		else
		{
			row = row % mRowLayouts.size();
			col = col % mRowLayouts[row]->count();
			mRowLayouts[row]->widget(col)->setFocus();
		}
	}
}

#include "quadkonsole.moc"
