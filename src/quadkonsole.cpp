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
#include "closedialog.h"

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
#include <KDE/KParts/ReadOnlyPart>

#include <QtGui/QMouseEvent>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QLayout>
#include <QtGui/QSplitter>

#include "ui_prefs_base.h"
#include "ui_prefs_shutdown.h"

// Only for restoring a session.
QuadKonsole::QuadKonsole()
	: mFilter(0)
{
	setupActions();
	setupUi(0, 0);
}


QuadKonsole::QuadKonsole(int rows, int columns, const QStringList& cmds)
	: mFilter(0)
{
	if (rows == 0)
		rows = Settings::numRows();
	if (columns == 0)
		columns = Settings::numCols();

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
	: mFilter(0)
{
	Konsole* k = new Konsole(this, part);
	mKonsoleParts.push_back(k);

	setupActions();
	setupUi(1, 1);
	k->setParent(mRowLayouts[0]->widget(0));
	actionCollection()->associateWidget(k->widget());
	showNormal();
}


QuadKonsole::~QuadKonsole()
{
	kDebug() << "deleting " << mKonsoleParts.size() << " parts" << endl;
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


void QuadKonsole::pasteSelection()
{
	emitPaste(QClipboard::Selection);
}


void QuadKonsole::setupActions()
{
	if (Settings::sloppyFocus())
	{
		mFilter = new MouseMoveFilter(this);
		qApp->installEventFilter(mFilter);
	}

	// Movement
	KAction* goRight = new KAction(KIcon("arrow-right"), i18n("Go &right"), this);
	goRight->setShortcut(KShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Right)));
	actionCollection()->addAction("go right", goRight);
	connect(goRight, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleRight()));

	KAction* goLeft = new KAction(KIcon("arrow-left"), i18n("Go &left"), this);
	goLeft->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Left));
	actionCollection()->addAction("go left", goLeft);
	connect(goLeft, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleLeft()));

	KAction* goUp = new KAction(KIcon("arrow-up"), i18n("Go &up"), this);
	goUp->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Up));
	actionCollection()->addAction("go up", goUp);
	connect(goUp, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleUp()));

	KAction* goDown = new KAction(KIcon("arrow-down"), i18n("Go &down"), this);
	goDown->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Down));
	actionCollection()->addAction("go down", goDown);
	connect(goDown, SIGNAL(triggered(bool)), this, SLOT(focusKonsoleDown()));

	// Adding and removing parts
	KAction* detach = new KAction(KIcon("document-new"), i18n("De&tach"), this);
	detach->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Return));
	actionCollection()->addAction("detach", detach);
	connect(detach, SIGNAL(triggered(bool)), this, SLOT(detach()));

	KAction* insertHorizontal = new KAction(KIcon("view-split-left-right"), i18n("Insert &horizontal"), this);
	insertHorizontal->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_H));
	actionCollection()->addAction("insert horizontal", insertHorizontal);
	connect(insertHorizontal, SIGNAL(triggered(bool)), this, SLOT(insertHorizontal()));

	KAction* insertVertical = new KAction(KIcon("view-split-top-bottom"), i18n("Insert &vertical"), this);
	insertVertical->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_K));
	actionCollection()->addAction("insert vertical", insertVertical);
	connect(insertVertical, SIGNAL(triggered(bool)), this, SLOT(insertVertical()));

	KAction* removePart = new KAction(KIcon("view-left-close"), i18n("Re&move part"), this);
	removePart->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_R));
	actionCollection()->addAction("remove part", removePart);
	connect(removePart, SIGNAL(triggered(bool)), this, SLOT(removePart()));

	// View
	KAction* resetLayouts = new KAction(KIcon("view-grid"), i18n("R&eset layouts"), this);
	actionCollection()->addAction("reset layouts", resetLayouts);
	connect(resetLayouts, SIGNAL(triggered(bool)), this, SLOT(resetLayouts()));
	
	KAction* switchView = new KAction(KIcon("document-open-folder"), i18n("S&witch view"), this);
	switchView->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O));
	actionCollection()->addAction("switch view", switchView);
	connect(switchView, SIGNAL(triggered(bool)), this, SLOT(switchView()));
	
	KStandardAction::paste(this, SLOT(pasteClipboard()), actionCollection());
	KAction *pasteSelection = new KAction(KIcon("edit-paste"), i18n("Paste &selection"), this);
	pasteSelection->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Insert));
	actionCollection()->addAction("pasteSelection", pasteSelection);
	connect(pasteSelection, SIGNAL(triggered(bool)), this, SLOT(pasteSelection()));

	// Standard actions
	KStandardAction::preferences(this, SLOT(optionsPreferences()), actionCollection());
	KStandardAction::quit(this, SLOT(quit()), actionCollection());
	KToggleAction* toggleMenu = KStandardAction::showMenubar(this, SLOT(toggleMenu()), actionCollection());
	toggleMenu->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M));

	// Debug
#ifdef DEBUG
	kDebug() << "adding debugging actions" << endl;
	KAction* saveSession = new KAction(KIcon("document-save"), i18n("Save session"), this);
	actionCollection()->addAction("saveSession", saveSession);
	connect(saveSession, SIGNAL(triggered(bool)), this, SLOT(saveSession()));

	KAction* restoreSession = new KAction(KIcon("document-open"), i18n("Restore session"), this);
	actionCollection()->addAction("restoreSession", restoreSession);
	connect(restoreSession, SIGNAL(triggered(bool)), this, SLOT(restoreSession()));
#endif // DEBUG
}


void QuadKonsole::setupUi(int rows, int columns)
{
	QWidget* centralWidget = new QWidget(this, 0);
	QGridLayout* grid = new QGridLayout(centralWidget);

	mRows = new QSplitter(Qt::Vertical);
	mRows->setChildrenCollapsible(false);
	grid->addWidget(mRows, 0, 0);

	setCentralWidget(centralWidget);
	actionCollection()->addAssociatedWidget(centralWidget);

	for (int i = 0; i < rows; ++i)
	{
		QSplitter* row = new QSplitter(Qt::Horizontal);
		row->setChildrenCollapsible(false);
		mRowLayouts.push_back(row);
		mRows->addWidget(row);
		for (int j = 0; j < columns; ++j)
		{
			Konsole* part = 0;
			if (mKonsoleParts.size() > static_cast<unsigned long>(i*columns + j))
				part = mKonsoleParts[i*columns + j];

			addPart(i, j, part);
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
	kDebug() << "pasting" << mode << endl;
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


void QuadKonsole::detach(Konsole* part)
{
	if (part == 0)
	{
		part = getFocusPart();
		if (part == 0)
			return;
	}

	actionCollection()->removeAssociatedWidget(part->widget());
	QuadKonsole* external = new QuadKonsole(part->part());
	part->partDestroyed();
	external->setAttribute(Qt::WA_DeleteOnClose);
}


void QuadKonsole::resetLayout(QSplitter* layout, int targetSize)
{
	bool resize = false;
	QList<int> currentSizes = layout->sizes();
	QList<int>::const_iterator it;
	for (it=currentSizes.begin(); it!=currentSizes.end(); ++it)
	{
		if (abs(*it - targetSize) >= 10)
		{
			kDebug() << "forcing resize:" << *it << "=>" << targetSize << endl;
			resize = true;
			break;
		}
	}

	if (resize)
	{
		QList<int> sizes;
		for (int i=0; i<layout->count(); ++i)
			sizes.append(targetSize);
		layout->setSizes(sizes);
	}
}


void QuadKonsole::resetLayouts()
{
	kDebug() << "resetting layouts" << endl;

	resetLayout(mRows, mRows->height() / mRows->count());

	std::vector<QSplitter*>::iterator it;
	for (it=mRowLayouts.begin(); it!=mRowLayouts.end(); ++it)
		resetLayout(*it, (*it)->width() / (*it)->count());
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


bool QuadKonsole::queryClose()
{
	if (Settings::queryClose())
	{
		CloseDialog dialog(this);
		for (unsigned int i=0; i<mKonsoleParts.size(); ++i)
		{
			QString process = mKonsoleParts[i]->foregroundProcessName();
			if (process.size())
			{
				dialog.addProcess(i, process);
			}
		}

		if (dialog.size())
		{
			QList<int> doDetach;
			if (! dialog.exec(doDetach))
				return false;
			else
			{
				QListIterator<int> it(doDetach);
				while (it.hasNext())
					detach(mKonsoleParts[it.next()]);

				return true;
			}
		}
	}

	return true;
}


void QuadKonsole::saveProperties(KConfigGroup& config)
{
	QList<int> rowSizes = mRows->sizes();
	config.writeEntry("rowSizes", rowSizes);
	kDebug() << "saving session. rowSizes =" << rowSizes << endl;

	for (unsigned int i=0; i<mRowLayouts.size(); ++i)
	{
		kDebug() << QString("row_%1").arg(i) << "=" << mRowLayouts[i]->sizes() << endl;
		config.writeEntry(QString("row_%1").arg(i), mRowLayouts[i]->sizes());
	}
}


void QuadKonsole::readProperties(const KConfigGroup& config)
{
	QList<int> rowSizes = config.readEntry("rowSizes", QList<int>());
	if (rowSizes.empty())
	{
		kDebug() << "could not read properties: empty rowSizes" << endl;
		return;
	}

	kDebug() << "adjusting number of rows:" << mRowLayouts.size() << "=>" << rowSizes.size() << endl;
	// adjust number of rows
	while (mRowLayouts.size() < static_cast<unsigned int>(rowSizes.size()))
		insertVertical(0, 0);

	for (unsigned int i=0; i<static_cast<unsigned int>(rowSizes.size()); ++i)
	{
		kDebug() << "restoring row" << i << endl;
		QList<int> sizes = config.readEntry(QString("row_%1").arg(i), QList<int>());
		while (mRowLayouts[i]->count() < sizes.size())
			insertHorizontal(i, 0);

		mRowLayouts[i]->setSizes(sizes);
	}

	mRows->setSizes(rowSizes);
}


Konsole* QuadKonsole::getFocusPart()
{
	for (PartVector::iterator it=mKonsoleParts.begin(); it!=mKonsoleParts.end(); ++it)
	{
		if ((*it)->widget() && (*it)->widget()->hasFocus())
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
	kDebug() << "could not find focus in" << mKonsoleParts.size() << "parts" << endl;
	row = -1;
	col = -1;
}


Konsole* QuadKonsole::addPart(int row, int col, Konsole* part)
{
	QWidget* container = new QWidget;
	mRowLayouts[row]->insertWidget(col, container);
	
	QBoxLayout* layout = new QBoxLayout(QBoxLayout::Down, container);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);

	if (part != 0)
	{
		part->setLayout(layout);
		part->setParent(container);
		container->setFocusProxy(part->widget());
	}
	else
	{
		part = new Konsole(container, layout);
		mKonsoleParts.push_back(part);
	}
	connect(part, SIGNAL(partCreated()), SLOT(resetLayouts()));
	actionCollection()->addAssociatedWidget(part->widget());

	return part;
}


void QuadKonsole::insertHorizontal(int row, int col)
{
	if (row == -1 || col == -1)
		getFocusCoords(row, col);

	if (row >= 0 && col >= 0)
	{
		Konsole* part = addPart(row, col);
		part->widget()->setFocus();

		resetLayouts();
	}
}


void QuadKonsole::insertVertical(int row, int col)
{
	if (row == -1 || col == -1)
		getFocusCoords(row, col);

	if (row >= 0 && col >= 0)
	{
		QSplitter *newRow = new QSplitter(Qt::Horizontal);
		newRow->setChildrenCollapsible(false);
		mRows->insertWidget(row, newRow);

		std::vector<QSplitter*>::iterator it = mRowLayouts.begin();
		it += row;
		mRowLayouts.insert(it, newRow);

		Konsole* part = addPart(row, 0);
		part->widget()->setFocus();
		resetLayouts();
	}
}


void QuadKonsole::removePart(int row, int col)
{
	if (row == -1 || col == -1)
		getFocusCoords(row, col);

	if (row >= 0 && col >= 0)
	{
		// Konsole* part = getFocusPart();
		Konsole* part = qobject_cast<Konsole*>(mRowLayouts[row]->widget(col));

		for (PartVector::iterator it=mKonsoleParts.begin(); it!=mKonsoleParts.end(); ++it)
		{
			if ((*it) && (*it)->widget() && (*it)->widget()->hasFocus())
			{
				mKonsoleParts.erase(it);
				break;
			}
		}

		delete part;
		delete mRowLayouts[row]->widget(col);
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


void QuadKonsole::switchView()
{
	Konsole* konsole = getFocusPart();
	konsole->focusNext();
}


#ifdef DEBUG
namespace
{
	KConfig* master = 0;
	KConfigGroup* sessionTest = 0;
}

void QuadKonsole::saveSession()
{
	delete master;
	delete sessionTest;

	master = new KConfig("", KConfig::SimpleConfig);
	sessionTest = new KConfigGroup(master, "SessionTest");

	saveProperties(*sessionTest);
}


void QuadKonsole::restoreSession()
{
	if (sessionTest)
	{
		while (mKonsoleParts.size())
		{
			delete mKonsoleParts.front();
			mKonsoleParts.erase(mKonsoleParts.begin());
		}
		while (mRowLayouts.size())
		{
			delete mRowLayouts.front();
			mRowLayouts.erase(mRowLayouts.begin());
		}
		readProperties(*sessionTest);
	}
	else
		KMessageBox::error(this, "There is no session to be restored. You need to save one before restoring.", "No saved session found");
}
#endif // DEBUG

#include "quadkonsole.moc"
