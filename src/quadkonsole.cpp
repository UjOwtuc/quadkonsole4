/***************************************************************************
 *   Copyright (C) 2005 by Simon Perreault                                 *
 *   nomis80@nomis80.org                                                   *
 *   Copyright (C) 2009 - 2017 by Karsten Borgwaldt                        *
 *   kb@spambri.de                                                         *
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
#include "qkbrowseriface.h"

#include <KActionCollection>
#include <KStandardAction>
#include <KToggleAction>
#include <KToolBar>
#include <KConfigDialog>
#include <KMessageBox>
#include <KHistoryComboBox>
#include <KLineEdit>
#include <KParts/ReadOnlyPart>
#include <KParts/PartManager>
#include <KParts/HistoryProvider>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QLayout>
#include <QSplitter>
#include <QTimer>
#include <QStatusBar>
#include <QMenuBar>

#include "ui_prefs_base.h"
#include "ui_prefs_shutdown.h"
#include "quadkonsoleadaptor.h"


namespace
{
	// action name => vertical layout (default), horizontal layout
	QMap<QString, QPair<QString, QString> > movementMap;
}


QString splitIndex(const QString& s, int* index)
{
	int pos = s.indexOf(':');
	bool hasIndex = false;
	int indexOut = -1;

	if (pos > -1)
		indexOut = s.left(pos).toUInt(&hasIndex);

	if (index)
		*index = hasIndex ? indexOut : -1;

	if (hasIndex)
		return s.mid(pos +1);
	return s;
}


QuadKonsole::QuadKonsole(int rows, int columns, const QStringList& cmds, const QStringList& urls)
	: m_mouseFilter(0),
	m_partManager(this),
	m_activeStack(0),
	m_zoomed(-1, -1),
	m_dbusAdaptor(new QuadKonsoleAdaptor(this))
{
	if (rows == 0)
		rows = Settings::numRows();
	if (columns == 0)
		columns = Settings::numCols();

	if (rows < 1)
	{
		qFatal("Number of rows must be at last one.");
		qApp->quit();
	}

	if (columns < 1)
	{
		qFatal("Number of columns must be at least one.");
		qApp->quit();
	}

	setupActions();
	initHistory();
	setupUi(rows, columns);

	m_stacks.front()->setFocus();
	m_activeStack = m_stacks.front();
	slotActivePartChanged(m_stacks.front()->part());

	if (cmds.size())
		sendCommands(cmds);
	if (urls.size())
		openUrls(urls);
	reconnectMovement();
}


// for detaching parts
QuadKonsole::QuadKonsole(KParts::ReadOnlyPart* part)
	: m_mouseFilter(0),
	m_partManager(this),
	m_activeStack(0),
	m_zoomed(-1, -1),
	m_dbusAdaptor(new QuadKonsoleAdaptor(this))
{
	QList<KParts::ReadOnlyPart*> parts;
	parts.append(part);

	setupActions();
	initHistory();
	setupUi(1, 1, parts);

	m_stacks.front()->setFocus();
	m_activeStack = m_stacks.front();
	slotActivePartChanged(m_stacks.front()->part());

	showNormal();
	reconnectMovement();
}


QuadKonsole::~QuadKonsole()
{
	m_partManager.disconnect();
}


void QuadKonsole::initHistory()
{
	m_urlBar = new KHistoryComboBox(true, 0);
	toolBar("locationToolBar")->addWidget(m_urlBar);
	m_urlBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	m_urlBar->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
	m_urlBar->setFocusPolicy(Qt::ClickFocus);
	connect(m_urlBar, SIGNAL(returnPressed()), SLOT(slotOpenUrl()));
	connect(m_urlBar, SIGNAL(returnPressed(QString)), SLOT(slotOpenUrl(QString)));

	connect(KParts::HistoryProvider::self(), SIGNAL(inserted(QString)), SLOT(refreshHistory(QString)));
}


void QuadKonsole::setupActions()
{
	if (Settings::sloppyFocus())
	{
		m_mouseFilter = new MouseMoveFilter(this);
		qApp->installEventFilter(m_mouseFilter);
	}

	// Movement
	QAction* goRight = new QAction(QIcon("arrow-right"), i18n("Go &right"), this);
	actionCollection()->addAction("go right", goRight);
	actionCollection()->setDefaultShortcut(goRight, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Right));
	connect(goRight, SIGNAL(triggered()), SLOT(focusKonsoleRight()));

	QAction* goLeft = new QAction(QIcon("arrow-left"), i18n("Go &left"), this);
	actionCollection()->addAction("go left", goLeft);
	actionCollection()->setDefaultShortcut(goLeft, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Left));
	connect(goLeft, SIGNAL(triggered()), SLOT(focusKonsoleLeft()));

	QAction* goUp = new QAction(QIcon("arrow-up"), i18n("Go &up"), this);
	actionCollection()->addAction("go up", goUp);
	actionCollection()->setDefaultShortcut(goUp, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Up));
	connect(goUp, SIGNAL(triggered()), SLOT(focusKonsoleUp()));

	QAction* goDown = new QAction(QIcon("arrow-down"), i18n("Go &down"), this);
	actionCollection()->addAction("go down", goDown);
	actionCollection()->setDefaultShortcut(goDown, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Down));
	connect(goDown, SIGNAL(triggered()), SLOT(focusKonsoleDown()));

	QAction* tabLeft = new QAction(i18n("&Previous tab"), this);
	actionCollection()->addAction("tab left", tabLeft);
	actionCollection()->setDefaultShortcut(tabLeft, QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Left));
	connect(tabLeft, SIGNAL(triggered(bool)), this, SLOT(tabLeft()));

	QAction* tabRight = new QAction(i18n("&Next tab"), this);
	actionCollection()->addAction("tab right", tabRight);
	actionCollection()->setDefaultShortcut(tabRight, QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Right));
	connect(tabRight, SIGNAL(triggered(bool)), this, SLOT(tabRight()));

	// Adding and removing parts
	QAction* detach = new QAction(QIcon("document-new"), i18n("De&tach"), this);
	actionCollection()->addAction("detach", detach);
	actionCollection()->setDefaultShortcut(detach, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Return));
	connect(detach, SIGNAL(triggered(bool)), this, SLOT(detach()));

	QAction* insertHorizontal = new QAction(QIcon("view-split-left-right"), i18n("Insert &horizontal"), this);
	actionCollection()->addAction("insert horizontal", insertHorizontal);
	actionCollection()->setDefaultShortcut(insertHorizontal, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_H));

	QAction* insertVertical = new QAction(QIcon("view-split-top-bottom"), i18n("Insert &vertical"), this);
	actionCollection()->addAction("insert vertical", insertVertical);
	actionCollection()->setDefaultShortcut(insertVertical, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_K));

	QAction* removeStack = new QAction(QIcon("view-left-close"), i18n("Re&move stack"), this);
	actionCollection()->addAction("remove part", removeStack);
	actionCollection()->setDefaultShortcut(removeStack, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_R));
	connect(removeStack, SIGNAL(triggered(bool)), this, SLOT(removeStack()));

	QAction* duplicateView = new QAction(QIcon("edit-copy"), i18n("&Duplicate view"), this);
	actionCollection()->addAction("duplicate view", duplicateView);
	actionCollection()->setDefaultShortcut(duplicateView, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_D));
	connect(duplicateView, SIGNAL(triggered()), SLOT(slotDuplicateView()));

	// View
	QAction* resetLayouts = new QAction(QIcon("view-grid"), i18n("R&eset layouts"), this);
	connect(resetLayouts, SIGNAL(triggered(bool)), this, SLOT(resetLayouts()));
	actionCollection()->addAction("reset layouts", resetLayouts);

	QAction* switchView = new QAction(QIcon("document-open-folder"), i18n("S&witch view"), this);
	actionCollection()->addAction("switch view", switchView);
	actionCollection()->setDefaultShortcut(switchView, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O));
	connect(switchView, SIGNAL(triggered(bool)), this, SLOT(switchView()));

	QAction* closeView = new QAction(QIcon("document-close"), i18n("&Close view"), this);
	actionCollection()->addAction("close view", closeView);
	actionCollection()->setDefaultShortcut(closeView, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Q));
	connect(closeView, SIGNAL(triggered(bool)), this, SLOT(closeView()));

	QAction* zoomView = new QAction(QIcon("zoom-in"), i18n("&Zoom"), this);
	actionCollection()->addAction("zoom view", zoomView);
	actionCollection()->setDefaultShortcut(zoomView, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z));
	connect(zoomView, SIGNAL(triggered(bool)), SLOT(zoomView()));

	// Standard actions
	KStandardAction::back(this, SLOT(goBack()), actionCollection());
	KStandardAction::forward(this, SLOT(goForward()), actionCollection());
	KStandardAction::up(this, SLOT(goUp()), actionCollection());
	KStandardAction::preferences(this, SLOT(optionsPreferences()), actionCollection());
	KStandardAction::quit(this, SLOT(deleteLater()), actionCollection());
	KToggleAction* toggleMenu = KStandardAction::showMenubar(this, SLOT(toggleMenu()), actionCollection());
	toggleMenu->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M));

	QAction* changeLayout = new QAction(QIcon(""), i18n("C&hange layout"), this);
	actionCollection()->addAction("change layout", changeLayout);
	actionCollection()->setDefaultShortcut(changeLayout, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_L));
	connect(changeLayout, SIGNAL(triggered(bool)), SLOT(changeLayout()));

	// Location toolbar
	QAction* toggleUrlBar = new QAction(QIcon("document-open-remote"), i18n("&Open URL"), this);
	actionCollection()->addAction("toolbar_url_combo", toggleUrlBar);
	actionCollection()->setDefaultShortcut(toggleUrlBar, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_G));
	connect(toggleUrlBar, SIGNAL(triggered(bool)), this, SLOT(slotActivateUrlBar()));

	connect(&m_partManager, SIGNAL(activePartChanged(KParts::Part*)), SLOT(slotActivePartChanged(KParts::Part*)));

	// Debug
#ifdef DEBUG
	qDebug() << "adding debugging actions";
	QAction* saveSession = new QAction(QIcon("document-save"), i18n("Save session"), this);
	actionCollection()->addAction("saveSession", saveSession);
	connect(saveSession, SIGNAL(triggered(bool)), this, SLOT(saveSession()));

	QAction* restoreSession = new QAction(QIcon("document-open"), i18n("Restore session"), this);
	actionCollection()->addAction("restoreSession", restoreSession);
	connect(restoreSession, SIGNAL(triggered(bool)), this, SLOT(restoreSession()));
#endif // DEBUG
}


void QuadKonsole::setupUi(int rows, int columns, QList<KParts::ReadOnlyPart*> parts)
{
	setupGUI(ToolBar | Keys | StatusBar | Save, "quadkonsole4ui.rc");

	QWidget* centralWidget = new QWidget(this, 0);
	setCentralWidget(centralWidget);

	QGridLayout* grid = new QGridLayout(centralWidget);
	grid->setContentsMargins(0, 0, 0, 0);

	if (Settings::layoutVertical())
		m_rows = new QSplitter(Qt::Vertical);
	else
		m_rows = new QSplitter(Qt::Horizontal);

	grid->addWidget(m_rows, 0, 0);
	m_rows->setChildrenCollapsible(false);
	KConfigGroup autoSave = autoSaveConfigGroup();
	actionCollection()->addAssociatedWidget(centralWidget);

	for (int i=0; i<rows; ++i)
	{
		QSplitter* row;
		if (Settings::layoutVertical())
			row = new QSplitter(Qt::Horizontal);
		else
			row = new QSplitter(Qt::Vertical);

		row->setChildrenCollapsible(false);
		m_rowLayouts.push_back(row);
		m_rows->addWidget(row);
		for (int j=0; j<columns; ++j)
		{
			KParts::ReadOnlyPart* part = 0;
			if (parts.count() > static_cast<long>(i*columns + j))
				part = parts.at(i*columns + j);

			addStack(i, j, part);
		}
	}
	setWindowIcon(QIcon("quadkonsole4"));
}


void QuadKonsole::focusKonsoleRight()
{
	int row, col;
	getFocusCoords(row, col);
	if (row >= 0 && col >= 0)
	{
		if (m_rows->orientation() == Qt::Vertical)
		{
			++col;
			if (col >= m_rowLayouts[row]->count())
			{
				col = 0;
				row = (row +1) % m_rowLayouts.count();
			}
		}
		else
		{
			++row;
			if (row >= m_rowLayouts.count())
			{
				row = 0;
				++col;
			}
			col %= m_rowLayouts[0]->count();
		}
		m_rowLayouts[row]->widget(col)->setFocus();
	}
}


void QuadKonsole::focusKonsoleLeft()
{
	int row, col;
	getFocusCoords(row, col);
	if (row >= 0 && col >= 0)
	{
		if (m_rows->orientation() == Qt::Vertical)
		{
			--col;
			if (col < 0)
			{
				row = (row + m_rowLayouts.size() -1) % m_rowLayouts.count();
				col = m_rowLayouts[row]->count() -1;
			}
		}
		else
		{
			--row;
			if (row < 0)
			{
				row = m_rowLayouts.count() -1;
				col += m_rowLayouts[row]->count() -1;
			}
			col %= m_rowLayouts[row]->count();
		}
		m_rowLayouts[row]->widget(col)->setFocus();
	}
}


void QuadKonsole::focusKonsoleUp()
{
	int row, col;
	getFocusCoords(row, col);
	if (row >= 0 && col >= 0)
	{
		if (m_rows->orientation() == Qt::Vertical)
		{
			row = (row + m_rowLayouts.size() -1) % m_rowLayouts.size();
			if (col >= m_rowLayouts[row]->count())
				col = m_rowLayouts[row]->count() -1;
		}
		else
			col = (col + m_rowLayouts[row]->count() -1) % m_rowLayouts[row]->count();
		m_rowLayouts[row]->widget(col)->setFocus();
	}
}


void QuadKonsole::focusKonsoleDown()
{
	int row, col;
	getFocusCoords(row, col);
	if (row >= 0 && col >= 0)
	{
		if (m_rows->orientation() == Qt::Vertical)
		{
			row = (row +1) % m_rowLayouts.size();
			if (col >= m_rowLayouts[row]->count())
				col = m_rowLayouts[row]->count() -1;
		}
		else
			col = (col +1) % m_rowLayouts[row]->count();
		m_rowLayouts[row]->widget(col)->setFocus();
	}
}


void QuadKonsole::detach(QKStack* stack, QKView* view)
{
	if (stack == 0)
	{
		stack = getFocusStack();
		if (stack == 0)
			return;
	}
	if (view == 0)
		view = stack->currentWidget();

	KParts::ReadOnlyPart* part = view->part();
	view->partDestroyed();
	QuadKonsole* external = new QuadKonsole(part);
	external->setAttribute(Qt::WA_DeleteOnClose);
	emit detached(external);

	stack->switchView(QUrl("~"), "inode/directory", true);
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


void QuadKonsole::fillMovementMap()
{
	if (movementMap.isEmpty())
	{
		movementMap.insert("insert horizontal",	QPair<QString, QString>(SLOT(insertHorizontal()), SLOT(insertVertical())));
		movementMap.insert("insert vertical",	QPair<QString, QString>(SLOT(insertVertical()), SLOT(insertHorizontal())));
	}
}


void QuadKonsole::reconnectMovement()
{
	fillMovementMap();

	QMapIterator<QString, QPair<QString, QString> > it(movementMap);
	while (it.hasNext())
	{
		it.next();
		QAction* action = actionCollection()->action(it.key());
		if (action)
		{
			action->disconnect(this);
			if (m_rows->orientation() == Qt::Vertical)
				connect(action, SIGNAL(triggered(bool)), it.value().first.toLatin1().data());
			else
				connect(action, SIGNAL(triggered(bool)), it.value().second.toLatin1().data());
		}
		else
			qDebug() << "action" << it.key() << "not in actionCollection";
	}
}


void QuadKonsole::zoomSplitter(QSplitter* layout, int item)
{
	QList<int> sizes = layout->sizes();
	for (int i=0; i<layout->count(); ++i)
	{
		if (i == item && layout->orientation() == Qt::Vertical)
			sizes[i] = layout->height();
		else if (i == item)
			sizes[i] = layout->width();
		else
			sizes[i] = 0;
	}
	layout->setSizes(sizes);
}


void QuadKonsole::resetLayouts()
{
	resetLayout(m_rows, m_rows->height() / m_rows->count());

	QListIterator<QSplitter*> it(m_rowLayouts);
	while (it.hasNext())
	{
		QSplitter* layout = it.next();
		resetLayout(layout, layout->width() / layout->count());
	}
	m_zoomed = qMakePair(-1, -1);
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
	if (Settings::sloppyFocus() && m_mouseFilter == 0)
	{
		m_mouseFilter = new MouseMoveFilter;
		qApp->installEventFilter(m_mouseFilter);
	}
	else if (! Settings::sloppyFocus())
	{
		delete m_mouseFilter;
		m_mouseFilter = 0;
	}

	if ((m_rows->orientation() == Qt::Vertical && ! Settings::layoutVertical()) || (m_rows->orientation() == Qt::Horizontal && Settings::layoutVertical()))
		changeLayout();
}


bool QuadKonsole::queryClose()
{
	if (Settings::queryClose())
	{
		CloseDialog dialog(this);

		QListIterator<QKStack*> stackIt(m_stacks);
		while (stackIt.hasNext())
		{
			QKStack* stack = stackIt.next();
			dialog.addViews(stack, stack->modifiedViews());
		}

		if (dialog.size())
		{
			if (! dialog.exec())
				return false;
			else
			{
				QMap<QKView*, QKStack*> doDetach = dialog.selectedForDetach();
				QMapIterator<QKView*, QKStack*> it(doDetach);
				while (it.hasNext())
				{
					it.next();
					detach(it.value(), it.key());
				}

				return true;
			}
		}
	}

	return true;
}


QKStack* QuadKonsole::getFocusStack()
{
	KParts::Part* activePart = m_partManager.activePart();
	QList<QKStack*>::iterator it;
	for (it=m_stacks.begin(); it!=m_stacks.end(); ++it)
	{
		QKView* view = (*it)->currentWidget();
		if (view && view->part() == activePart)
			return *it;
	}

	return 0;
}


void QuadKonsole::getFocusCoords(int& row, int& col)
{
	KParts::Part* activePart = m_partManager.activePart();
	for (row=0; row<m_rowLayouts.size(); ++row)
	{
		for (col=0; col<m_rowLayouts[row]->count(); ++col)
		{
			QKStack* stack = qobject_cast<QKStack*>(m_rowLayouts[row]->widget(col));
			if (stack->currentWidget() && activePart == stack->currentWidget()->part())
				return;
		}
	}

	qDebug() << "could not find focus in" << m_stacks.count() << "parts";
	row = -1;
	col = -1;
}


QKStack* QuadKonsole::addStack(int row, int col, KParts::ReadOnlyPart* part)
{
	QKStack* stack;
	stack = new QKStack(m_partManager, part);

	m_stacks.append(stack);
	m_rowLayouts[row]->insertWidget(col, stack);

	connect(stack, SIGNAL(partCreated()), SLOT(resetLayouts()));
	connect(stack, SIGNAL(setStatusBarText(QString)), SLOT(setStatusBarText(QString)));
	connect(stack, SIGNAL(setWindowCaption(QString)), SLOT(setWindowCaption(QString)));
	connect(stack, SIGNAL(setLocationBarUrl(QString)), SLOT(slotSetLocationBarUrl(QString)));
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
		QSplitter *newRow;

		if (m_rows->orientation() == Qt::Vertical)
			newRow = new QSplitter(Qt::Horizontal);
		else
			newRow = new QSplitter(Qt::Vertical);

		newRow->setChildrenCollapsible(false);
		m_rows->insertWidget(row, newRow);

		QList<QSplitter*>::iterator it = m_rowLayouts.begin();
		it += row;
		m_rowLayouts.insert(it, newRow);

		QKStack* stack = addStack(row, 0);
		stack->setFocus();
		resetLayouts();
	}
}


void QuadKonsole::removeStack()
{
	QKStack* stack = getFocusStack();
	if (stack)
	{
		if (Settings::queryClose())
		{
			CloseDialog dialog(this);
			dialog.addViews(stack, stack->modifiedViews());

			if (dialog.size())
			{
				if (! dialog.exec())
					return;
				else
				{
					QMap<QKView*, QKStack*> doDetach = dialog.selectedForDetach();
					QMapIterator<QKView*, QKStack*> it(doDetach);
					while (it.hasNext())
					{
						it.next();
						detach(stack, it.key());
					}
				}
			}
		}

		slotStackDestroyed(stack);
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
		int index = -1;
		QString cmd = splitIndex(cmds[i], &index);
		if (index == -1)
			index = i;

		sendInput(index, cmd + "\n");
	}
}


void QuadKonsole::sendInput(uint view, const QString& text)
{
	if (view < static_cast<uint>(m_stacks.count()))
		m_stacks[view]->sendInput(text);
}


void QuadKonsole::openUrls(const QStringList& urls, bool newTab)
{
	for (int i=0; i<urls.size(); ++i)
	{
		int index = -1;
		QString url = splitIndex(urls[i], &index);

		// no index given in URL, if a stack is zoomed, use that one
		if (index == -1 && m_zoomed.first > -1 && m_zoomed.second > -1)
		{
			QKStack* stack = qobject_cast<QKStack*>(m_rowLayouts[m_zoomed.first]->widget(m_zoomed.second));
			if (stack)
				index = m_stacks.indexOf(stack);
		}

		// no index in URL, no zoomed stack, use the focused stack
		if (index == -1 && m_activeStack)
			index = m_stacks.indexOf(m_activeStack);

		// no index in URL, no zoomed stack, no active stack, use the next one
		if (index == -1)
			index = i;

		if (index < m_stacks.count())
		{
			if (newTab)
				m_stacks[index]->slotOpenNewWindow(QUrl(url), "", 0);
			else
				m_stacks[index]->slotOpenUrlRequest(QUrl(url));
		}
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


QStringList QuadKonsole::urls() const
{
	QStringList result;
	QListIterator<QKStack*> it(m_stacks);
	while (it.hasNext())
		result << it.next()->url();

	return result;
}


QStringList QuadKonsole::partIcons() const
{
	QStringList result;
	QListIterator<QKStack*> it(m_stacks);
	while (it.hasNext())
		result << it.next()->partIcon();

	return result;
}


void QuadKonsole::changeLayout()
{
	QListIterator<QSplitter*> it(m_rowLayouts);
	while (it.hasNext())
		it.next()->setOrientation(m_rows->orientation());

	if (m_rows->orientation() == Qt::Vertical)
		m_rows->setOrientation(Qt::Horizontal);
	else
		m_rows->setOrientation(Qt::Vertical);

	reconnectMovement();
}


void QuadKonsole::slotActivateUrlBar()
{
	if (m_urlBar->isHidden())
		m_urlBar->show();

	m_urlBar->setFocus();
	m_urlBar->lineEdit()->selectAll();
}


void QuadKonsole::refreshHistory(const QString& item)
{
	m_urlBar->addToHistory(item);

	if (m_activeStack)
		m_urlBar->lineEdit()->setText(m_activeStack->url());
	m_urlBar->lineEdit()->setCursorPosition(0);
}


void QuadKonsole::slotActivePartChanged(KParts::Part* part)
{
	createGUI(part);
	unplugActionList("view_settings");
	// unplugActionList("view_edit");

	QKStack* stack = getFocusStack();
	if (stack)
	{
		m_activeStack = stack;
		QKView* view = stack->currentWidget();
		if (view)
		{
			m_urlBar->lineEdit()->setText(stack->url());
			m_urlBar->lineEdit()->setCursorPosition(0);
			plugActionList("view_settings", stack->currentWidget()->pluggableSettingsActions());
			// plugActionList("view_edit", stack->currentWidget()->pluggableEditActions());
		}
	}
}


void QuadKonsole::slotStackDestroyed(QKStack* removed)
{
	QKStack* stack = removed;
	if (! stack)
		stack = qobject_cast<QKStack*>(sender());

	if (stack)
	{
		if (stack == m_activeStack)
			m_activeStack = 0;

		m_stacks.removeAll(stack);
		stack->deleteLater();

		QList<QSplitter*>::iterator it;
		for (it=m_rowLayouts.begin(); it!=m_rowLayouts.end(); ++it)
		{
			QSplitter* splitter = *it;
			if (splitter->indexOf(stack) >= 0 && splitter->count() <= 1)
			{
				splitter->deleteLater();
				m_rowLayouts.erase(it);
				break;
			}
		}
	}
	else
		qDebug() << "no stack removed??";

	if (m_stacks.empty())
		deleteLater();
	else
	{
		m_stacks.front()->setFocus();
		slotActivePartChanged(m_stacks.front()->part());
	}
}


void QuadKonsole::slotSetLocationBarUrl(const QString& url)
{
	QKStack* stack = qobject_cast<QKStack*>(sender());
	if (getFocusStack() && getFocusStack() == stack)
	{
		m_urlBar->lineEdit()->setText(url);
		m_urlBar->lineEdit()->setCursorPosition(0);
	}
	else
		qDebug() << "won't set location bar url for an inactive stack";
}


void QuadKonsole::zoomView(int row, int col)
{
	if (m_zoomed.first == row && m_zoomed.second == col)
		resetLayouts();
	else
	{
		if (row < 0 || row >= m_rowLayouts.count())
		{
			qDebug() << "invalid row:" << row;
			return;
		}
		if (col < 0 || col >= m_rowLayouts.at(row)->count())
		{
			qDebug() << "ivalid column:" << col;
			return;
		}

		zoomSplitter(m_rows, row);
		zoomSplitter(m_rowLayouts.at(row), col);
		m_zoomed = qMakePair(row, col);
	}
}


void QuadKonsole::zoomView()
{
	int row, col;
	getFocusCoords(row, col);
	if (row > -1 && col > -1)
		zoomView(row, col);
}


void QuadKonsole::slotOpenUrl(const QString& url)
{
	if (m_activeStack)
	{
		m_activeStack->setFocus();
		if (! url.isEmpty())
			m_activeStack->slotOpenUrlRequest(url);
	}
}


void QuadKonsole::slotOpenUrl(const QUrl& url)
{
	if (m_activeStack)
	{
		m_activeStack->setFocus();
		m_activeStack->slotOpenUrlRequest(url);
	}
}


void QuadKonsole::slotNewWindow(const QUrl& url, const QString& mimeType, KParts::ReadOnlyPart** target)
{
	if (m_activeStack)
	{
		m_activeStack->setFocus();
		m_activeStack->slotOpenNewWindow(url, mimeType, target);
	}
}


void QuadKonsole::slotDuplicateView()
{
	if (m_activeStack && m_activeStack->currentWidget())
	{
		QString url = m_activeStack->url();
		int index = m_activeStack->addViews(QStringList(m_activeStack->currentWidget()->partName()));
		m_activeStack->setCurrentIndex(index);

		if (m_activeStack->currentWidget())
			m_activeStack->currentWidget()->setURL(QUrl(url));
	}
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
			delete m_stacks.takeFirst();
		while (m_rowLayouts.size())
			delete m_rowLayouts.takeFirst();
		readProperties(*sessionTest);
	}
	else
		KMessageBox::sorry(this, "Could not find a session to restore. Use \"Save session\" to save the current one.", "No saved session found");
}
#endif // DEBUG
