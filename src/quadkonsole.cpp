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
#include "mousemovefilter.h"
#include "settings.h"
#include "closedialog.h"
#include "qkstack.h"
#include "prefsviews.h"
#include "qkview.h"

#include <KDE/KDebug>
#include <kde_terminal_interface_v2.h>
#include <KDE/KIconLoader>
#include <KDE/KLibLoader>
#include <KDE/KLocale>
#include <KDE/KXMLGUIFactory>
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
#include <KDE/KParts/PartManager>

#include <QtGui/QMouseEvent>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QLayout>
#include <QtGui/QSplitter>
#include <QtCore/QTimer>

#include "ui_prefs_base.h"
#include "ui_prefs_shutdown.h"

QString splitIndex(const QString& s, int* index)
{
	int pos = s.indexOf(':');
	bool hasIndex = false;
	int indexOut;

	if (pos > -1)
		indexOut = s.left(pos).toUInt(&hasIndex);

	if (hasIndex)
	{
		if (index)
			*index = indexOut;

		return s.mid(pos +1);
	}
	return s;
}


// Only for restoring a session.
QuadKonsole::QuadKonsole()
	: mFilter(0),
	m_partManager(this, this)
{
	setupActions();
	setupUi(0, 0);
	setupGUI();
}


QuadKonsole::QuadKonsole(int rows, int columns, const QStringList& cmds, const QStringList& urls)
	: mFilter(0),
	m_partManager(this, this)
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
	setupGUI();

	if (cmds.size())
		sendCommands(cmds);
	if (urls.size())
		openUrls(urls);
}


// for detaching parts
QuadKonsole::QuadKonsole(KParts::ReadOnlyPart* part, const QList<KUrl>& history, int historyPosition)
	: mFilter(0),
	m_partManager(this, this)
{
	QList<KParts::ReadOnlyPart*> parts;
	parts.append(part);

	setupActions();
	setupUi(1, 1, parts);
	m_stacks.front()->setHistory(history, historyPosition);
	setupGUI();
	showNormal();
}


QuadKonsole::~QuadKonsole()
{
	kDebug() << "deleting " << m_stacks.count() << " parts" << endl;
	m_partManager.disconnect();

	delete mFilter;
	while (m_stacks.count())
	{
		delete m_stacks.front();
		m_stacks.pop_front();
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

	KAction* tabLeft = new KAction(i18n("&Previous tab"), this);
	tabLeft->setShortcut(QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Left));
	actionCollection()->addAction("tab left", tabLeft);
	connect(tabLeft, SIGNAL(triggered(bool)), this, SLOT(tabLeft()));

	KAction* tabRight = new KAction(i18n("&Next tab"), this);
	tabRight->setShortcut(QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Right));
	actionCollection()->addAction("tab right", tabRight);
	connect(tabRight, SIGNAL(triggered(bool)), this, SLOT(tabRight()));

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

	KAction* toggleUrlBar = new KAction(KIcon("document-open-remote"), i18n("&Open URL"), this);
	toggleUrlBar->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_G));
	actionCollection()->addAction("toggle url bar", toggleUrlBar);
	connect(toggleUrlBar, SIGNAL(triggered(bool)), this, SLOT(toggleUrlBar()));

	KAction* closeView = new KAction(KIcon("document-close"), i18n("&Close view"), this);
	closeView->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Q));
	actionCollection()->addAction("close view", closeView);
	connect(closeView, SIGNAL(triggered(bool)), this, SLOT(closeView()));

	// Standard actions
	KStandardAction::back(this, SLOT(goBack()), actionCollection());
	KStandardAction::forward(this, SLOT(goForward()), actionCollection());
	KStandardAction::up(this, SLOT(goUp()), actionCollection());
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


void QuadKonsole::setupUi(int rows, int columns, QList< KParts::ReadOnlyPart* > parts)
{
	QWidget* centralWidget = new QWidget(this, 0);
	setCentralWidget(centralWidget);

	QGridLayout* grid = new QGridLayout(centralWidget);
	grid->setContentsMargins(0, 0, 0, 0);

	m_rows = new QSplitter(Qt::Vertical);
	m_rows->setChildrenCollapsible(false);
	grid->addWidget(m_rows, 0, 0);

	actionCollection()->addAssociatedWidget(centralWidget);

	for (int i=0; i<rows; ++i)
	{
		QSplitter* row = new QSplitter(Qt::Horizontal);
		row->setChildrenCollapsible(false);
		mRowLayouts.push_back(row);
		m_rows->addWidget(row);
		for (int j=0; j<columns; ++j)
		{
			KParts::ReadOnlyPart* part = 0;
			if (parts.count() > static_cast<long>(i*columns + j))
				part = parts.at(i*columns + j);

			addStack(i, j, part);
		}
	}
	kDebug() << "finished setting up layouts for " << m_stacks.count() << " parts" << endl;

	setWindowIcon(KIcon("quadkonsole4"));
	connect(&m_partManager, SIGNAL(activePartChanged(KParts::Part*)), SLOT(slotActivePartChanged(KParts::Part*)));
}


void QuadKonsole::emitPaste(QClipboard::Mode mode)
{
	kDebug() << "pasting" << mode << endl;
	QString text = QApplication::clipboard()->text(mode);
	QKStack* part = getFocusStack();
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
		QKStack* stack = qobject_cast<QKStack*>(mRowLayouts[row]->widget(col));
		if (stack)
			stack->setFocus();
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
		QKStack* stack = qobject_cast<QKStack*>(mRowLayouts[row]->widget(col));
		if (stack)
			stack->setFocus();
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
		QKStack* stack = qobject_cast<QKStack*>(mRowLayouts[row]->widget(col));
		if (stack)
			stack->setFocus();
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
		QKStack* stack = qobject_cast<QKStack*>(mRowLayouts[row]->widget(col));
		if (stack)
			stack->setFocus();
	}
}


void QuadKonsole::detach(QKStack* stack)
{
	if (stack == 0)
	{
		stack = getFocusStack();
		if (stack == 0)
			return;
	}

	KParts::ReadOnlyPart* part = stack->part();
	stack->partDestroyed();
	QuadKonsole* external = new QuadKonsole(part, stack->history(), stack->historyPosition());
	external->setAttribute(Qt::WA_DeleteOnClose);
	stack->switchView(KUrl("~"), "inode/directory", true);
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

	resetLayout(m_rows, m_rows->height() / m_rows->count());

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

	PrefsViews* viewsSettings = new PrefsViews;
	dialog->addPage(viewsSettings, i18n("Views"), "edit-find");

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
		for (int i=0; i<m_stacks.count(); ++i)
		{
			QString process = m_stacks[i]->foregroundProcess();
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
					detach(m_stacks[it.next()]);

				return true;
			}
		}
	}

	return true;
}


void QuadKonsole::saveProperties(KConfigGroup& config)
{
	QList<int> rowSizes = m_rows->sizes();
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

	m_rows->setSizes(rowSizes);
}


QKStack* QuadKonsole::getFocusStack()
{
	QList<QKStack*>::iterator it;
	for (it=m_stacks.begin(); it!=m_stacks.end(); ++it)
	{
		if ((*it)->hasFocus())
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
			QKStack* stack = qobject_cast<QKStack*>(mRowLayouts[row]->widget(col));
			if (stack->hasFocus())
			{
				return;
			}
		}
	}
	kDebug() << "could not find focus in" << m_stacks.count() << "parts" << endl;
	row = -1;
	col = -1;
}


QKStack* QuadKonsole::addStack(int row, int col, KParts::ReadOnlyPart* part)
{
	QKStack* stack;
	if (part)
		stack = new QKStack(m_partManager, part);
	else
		stack = new QKStack(m_partManager);

	m_stacks.append(stack);
	mRowLayouts[row]->insertWidget(col, stack);

	connect(stack, SIGNAL(partCreated()), SLOT(resetLayouts()));
	connect(stack, SIGNAL(setStatusBarText(QString)), SLOT(setStatusBarText(QString)));
	connect(stack, SIGNAL(setWindowCaption(QString)), SLOT(setWindowCaption(QString)));
	connect(stack, SIGNAL(destroyed(QObject*)), SLOT(slotStackDestroyed()));

	return stack;
}


void QuadKonsole::insertHorizontal()
{
	int row, col;
	getFocusCoords(row, col);
	insertHorizontal(row, col);
}


void QuadKonsole::insertHorizontal(int row, int col)
{
	if (row >= 0 && col >= 0)
	{
		QKStack* stack = addStack(row, col);
		stack->setFocus();

		resetLayouts();
	}
}


void QuadKonsole::insertVertical()
{
	int row, col;
	getFocusCoords(row, col);
	insertVertical(row, col);
}


void QuadKonsole::insertVertical(int row, int col)
{
	if (row >= 0 && col >= 0)
	{
		QSplitter *newRow = new QSplitter(Qt::Horizontal);
		newRow->setChildrenCollapsible(false);
		m_rows->insertWidget(row, newRow);

		std::vector<QSplitter*>::iterator it = mRowLayouts.begin();
		it += row;
		mRowLayouts.insert(it, newRow);

		QKStack* stack = addStack(row, 0);
		stack->setFocus();
		resetLayouts();
	}
}


void QuadKonsole::removePart()
{
	int row, col;
	getFocusCoords(row, col);

	if (row >= 0 && col >= 0)
	{
		QKStack* stack = getFocusStack();
		if (Settings::queryClose())
		{
			if (stack->foregroundProcess().size())
			{
				if (KMessageBox::questionYesNo(this, i18n("The process \"%1\" is still running. Do you want to terminate it?", stack->foregroundProcess())) == KMessageBox::No)
					return;
			}
		}
		m_stacks.removeAll(stack);

		delete stack;
		if (mRowLayouts[row]->count() < 1)
		{
			QSplitter* splitter = mRowLayouts[row];
			std::vector<QSplitter*>::iterator it = mRowLayouts.begin();
			it += row;
			mRowLayouts.erase(it);
			delete splitter;
		}

		if (m_stacks.isEmpty())
			quit();
		else
		{
			row = row % mRowLayouts.size();
			col = col % mRowLayouts[row]->count();
			QKStack* stack = qobject_cast<QKStack*>(mRowLayouts[row]->widget(col));
			if (stack)
				stack->setFocus();
		}
	}
}


void QuadKonsole::switchView()
{
	QKStack* stack = getFocusStack();
	if (stack)
		stack->switchView();
}


void QuadKonsole::setStatusBarText(const QString& text)
{
	QKStack* stack = qobject_cast<QKStack*>(sender());
	if (stack && stack->hasFocus())
		statusBar()->showMessage(text);
}


void QuadKonsole::setWindowCaption(const QString& text)
{
	QKStack* stack = qobject_cast<QKStack*>(sender());
	if (stack && stack->hasFocus())
		setCaption(text);
}


void QuadKonsole::toggleUrlBar()
{
	QKStack* stack = getFocusStack();
	if (stack)
		stack->toggleUrlBar();
}


void QuadKonsole::goBack()
{
	QKStack* stack = getFocusStack();
	if (stack)
		stack->goBack();
}


void QuadKonsole::goForward()
{
	QKStack* stack = getFocusStack();
	if (stack)
		stack->goForward();
}


void QuadKonsole::goUp()
{
	QKStack* stack = getFocusStack();
	if (stack)
		stack->goUp();
}


void QuadKonsole::closeView()
{
	QKStack* stack = getFocusStack();
	if (stack)
		stack->slotTabCloseRequested(stack->currentIndex());
}


void QuadKonsole::tabLeft()
{
	QKStack* stack = getFocusStack();
	if (stack)
	{
		int index = stack->currentIndex() -1;
		if (index < 0)
			index = stack->count() -1;
		stack->setCurrentIndex(index);
	}
}


void QuadKonsole::tabRight()
{
	QKStack* stack = getFocusStack();
	if (stack)
		stack->setCurrentIndex((stack->currentIndex() +1) % stack->count());
}


void QuadKonsole::sendCommands(const QStringList& cmds)
{
	for (int i=0; i<cmds.size(); ++i)
	{
		int index = i;
		QString cmd = splitIndex(cmds[i], &index);
		sendInput(index, cmd + "\n");
	}
}


void QuadKonsole::sendInput(uint view, const QString& text)
{
	if (view < static_cast<uint>(m_stacks.count()))
		m_stacks[view]->sendInput(text);
}


void QuadKonsole::openUrls(const QStringList& urls)
{
	for (int i=0; i<urls.size(); ++i)
	{
		int index = i;
		QString url = splitIndex(urls[i], &index);

		if (index < m_stacks.count())
			m_stacks[index]->slotOpenUrlRequest(KUrl(url));
	}
}


void QuadKonsole::identifyStacks(QString format)
{
	QTimer* idTimer = new QTimer;
	connect(idTimer, SIGNAL(timeout()), idTimer, SLOT(deleteLater()));

	if (! format.contains("%1"))
		format += " %1";

	for (int i=0; i<m_stacks.size(); ++i)
	{
		QLabel* label = new QLabel(format.arg(i));
		label->setAlignment(Qt::AlignCenter);
		label->setMargin(20);
		label->setFont(QFont(fontInfo().family(), 3* fontInfo().pointSize()));
		m_stacks[i]->currentWidget()->layout()->addWidget(label);
		connect(idTimer, SIGNAL(timeout()), label, SLOT(deleteLater()));
	}

	idTimer->setSingleShot(true);
	idTimer->start(3500);
}


void QuadKonsole::slotActivePartChanged(KParts::Part* part)
{
	if (part)
	{
		part->setFactory(guiFactory());
		if (! guiFactory()->clients().contains(part))
			guiFactory()->addClient(part);

		if (! factory()->clients().contains(part))
			factory()->addClient(part);
	}
	createGUI(part);
}


void QuadKonsole::slotStackDestroyed()
{
	QKStack* stack = qobject_cast<QKStack*>(sender());
	if (stack)
	{
		m_stacks.removeAll(stack);
		std::vector<QSplitter*>::iterator it;
		for (it=mRowLayouts.begin(); it!=mRowLayouts.end(); ++it)
		{
			QSplitter* splitter = *it;
			if (splitter->indexOf(stack) >= 0 && splitter->count() <= 1)
			{
				splitter->deleteLater();
				mRowLayouts.erase(it);
				break;
			}
		}
	}
	else
		kDebug() << "sender is no QKStack" << endl;

	if (m_stacks.empty())
		quit();
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
		while (m_stacks.count())
		{
			delete m_stacks.front();
			m_stacks.pop_front();
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
