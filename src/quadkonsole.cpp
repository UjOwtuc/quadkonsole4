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
#include "qkbrowseriface.h"

#include <KDE/KDebug>
#include <KDE/KLocale>
#include <KDE/KActionCollection>
#include <KDE/KMenuBar>
#include <KDE/KStandardAction>
#include <KDE/KToggleAction>
#include <KDE/KStatusBar>
#include <KDE/KToolBar>
#include <KDE/KConfigDialog>
#include <KDE/KMessageBox>
#include <KDE/KHistoryComboBox>
#include <KDE/KLineEdit>
#include <KDE/KParts/ReadOnlyPart>
#include <KDE/KParts/PartManager>
#include <KDE/KParts/HistoryProvider>

#ifdef HAVE_LIBKONQ
#include <konq_historyprovider.h>
#endif

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QLayout>
#include <QtGui/QSplitter>
#include <QtCore/QTimer>

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
	: m_mouseFilter(0),
	m_partManager(this),
	m_activeStack(0),
	m_zoomed(-1, -1),
	m_sidebarSplitter(0),
	m_sidebar(0),
	m_dbusAdaptor(new QuadKonsoleAdaptor(this))
{
	setupActions();
	initHistory();
	setupUi(0, 0);

	reconnectMovement();
}


QuadKonsole::QuadKonsole(int rows, int columns, const QStringList& cmds, const QStringList& urls)
	: m_mouseFilter(0),
	m_partManager(this),
	m_activeStack(0),
	m_zoomed(-1, -1),
	m_sidebarSplitter(0),
	m_sidebar(0),
	m_dbusAdaptor(new QuadKonsoleAdaptor(this))
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
QuadKonsole::QuadKonsole(KParts::ReadOnlyPart* part, const QStringList& history, int historyPosition)
	: m_mouseFilter(0),
	m_partManager(this),
	m_activeStack(0),
	m_zoomed(-1, -1),
	m_sidebarSplitter(0),
	m_sidebar(0),
	m_dbusAdaptor(new QuadKonsoleAdaptor(this))
{
	QList<KParts::ReadOnlyPart*> parts;
	parts.append(part);

	setupActions();
	initHistory();
	setupUi(1, 1, parts);
	m_stacks.front()->setHistory(history, historyPosition);

	m_stacks.front()->setFocus();
	m_activeStack = m_stacks.front();
	slotActivePartChanged(m_stacks.front()->part());

	showNormal();
	reconnectMovement();
}


QuadKonsole::~QuadKonsole()
{
	kDebug() << "deleting " << m_stacks.count() << " parts" << endl;
	m_partManager.disconnect();
	delete m_sidebar;
	delete m_mouseFilter;
	while (m_stacks.count())
		delete m_stacks.takeFirst();
}


void QuadKonsole::initHistory()
{
	m_urlBar = new KHistoryComboBox(true, 0);
	toolBar("locationToolBar")->addWidget(m_urlBar);
	m_urlBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	m_urlBar->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
	m_urlBar->setFont(KGlobalSettings::generalFont());
	m_urlBar->setFocusPolicy(Qt::ClickFocus);
	connect(m_urlBar, SIGNAL(returnPressed()), SLOT(slotOpenUrl()));
	connect(m_urlBar, SIGNAL(returnPressed(QString)), SLOT(slotOpenUrl(QString)));

#ifdef HAVE_LIBKONQ
	KonqHistoryProvider* history = new KonqHistoryProvider;
	history->loadHistory();

	const KonqHistoryList& historyList = history->entries();
	QListIterator<KonqHistoryEntry> it(historyList);
	while (it.hasNext())
		m_urlBar->addToHistory(it.next().typedUrl);
#endif
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
	KAction* goRight = new KAction(KIcon("arrow-right"), i18n("Go &right"), this);
	goRight->setShortcut(KShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Right)));
	actionCollection()->addAction("go right", goRight);
	connect(goRight, SIGNAL(triggered()), SLOT(focusKonsoleRight()));

	KAction* goLeft = new KAction(KIcon("arrow-left"), i18n("Go &left"), this);
	goLeft->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Left));
	actionCollection()->addAction("go left", goLeft);
	connect(goLeft, SIGNAL(triggered()), SLOT(focusKonsoleLeft()));

	KAction* goUp = new KAction(KIcon("arrow-up"), i18n("Go &up"), this);
	goUp->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Up));
	actionCollection()->addAction("go up", goUp);
	connect(goUp, SIGNAL(triggered()), SLOT(focusKonsoleUp()));

	KAction* goDown = new KAction(KIcon("arrow-down"), i18n("Go &down"), this);
	goDown->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Down));
	actionCollection()->addAction("go down", goDown);
	connect(goDown, SIGNAL(triggered()), SLOT(focusKonsoleDown()));

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

	KAction* insertVertical = new KAction(KIcon("view-split-top-bottom"), i18n("Insert &vertical"), this);
	insertVertical->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_K));
	actionCollection()->addAction("insert vertical", insertVertical);

	KAction* removeStack = new KAction(KIcon("view-left-close"), i18n("Re&move stack"), this);
	removeStack->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_R));
	actionCollection()->addAction("remove part", removeStack);
	connect(removeStack, SIGNAL(triggered(bool)), this, SLOT(removeStack()));

	KAction* duplicateView = new KAction(KIcon("edit-copy"), i18n("&Duplicate view"), this);
	duplicateView->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_D));
	actionCollection()->addAction("duplicate view", duplicateView);
	connect(duplicateView, SIGNAL(triggered()), SLOT(slotDuplicateView()));

	// View
	KAction* resetLayouts = new KAction(KIcon("view-grid"), i18n("R&eset layouts"), this);
	actionCollection()->addAction("reset layouts", resetLayouts);
	connect(resetLayouts, SIGNAL(triggered(bool)), this, SLOT(resetLayouts()));

	KAction* switchView = new KAction(KIcon("document-open-folder"), i18n("S&witch view"), this);
	switchView->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O));
	actionCollection()->addAction("switch view", switchView);
	connect(switchView, SIGNAL(triggered(bool)), this, SLOT(switchView()));

	KAction* closeView = new KAction(KIcon("document-close"), i18n("&Close view"), this);
	closeView->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Q));
	actionCollection()->addAction("close view", closeView);
	connect(closeView, SIGNAL(triggered(bool)), this, SLOT(closeView()));

	KAction* zoomView = new KAction(KIcon("zoom-in"), i18n("&Zoom"), this);
	zoomView->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z));
	actionCollection()->addAction("zoom view", zoomView);
	connect(zoomView, SIGNAL(triggered(bool)), SLOT(zoomView()));

	// Standard actions
	KStandardAction::back(this, SLOT(goBack()), actionCollection());
	KStandardAction::forward(this, SLOT(goForward()), actionCollection());
	KStandardAction::up(this, SLOT(goUp()), actionCollection());
	KStandardAction::preferences(this, SLOT(optionsPreferences()), actionCollection());
	KStandardAction::quit(this, SLOT(quit()), actionCollection());
	KToggleAction* toggleMenu = KStandardAction::showMenubar(this, SLOT(toggleMenu()), actionCollection());
	toggleMenu->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M));

	KAction* changeLayout = new KAction(KIcon(""), i18n("C&hange layout"), this);
	changeLayout->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_L));
	actionCollection()->addAction("change layout", changeLayout);
	connect(changeLayout, SIGNAL(triggered(bool)), SLOT(changeLayout()));


	// Location toolbar
	KAction* toggleUrlBar = new KAction(KIcon("document-open-remote"), i18n("&Open URL"), this);
	toggleUrlBar->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_G));
	actionCollection()->addAction("toolbar_url_combo", toggleUrlBar);
	connect(toggleUrlBar, SIGNAL(triggered(bool)), this, SLOT(slotActivateUrlBar()));


	// sidebar
	KAction* toggleSidebar = new KAction(KIcon("view-sidetree"), i18n("Show sidebar"), this);
	toggleSidebar->setShortcut(Qt::Key_F9);
	actionCollection()->addAction("toggle sidebar", toggleSidebar);
	connect(toggleSidebar, SIGNAL(triggered()), SLOT(slotToggleSidebar()));

	connect(&m_partManager, SIGNAL(activePartChanged(KParts::Part*)), SLOT(slotActivePartChanged(KParts::Part*)));

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


void QuadKonsole::setupUi(int rows, int columns, QList<KParts::ReadOnlyPart*> parts)
{
	setupGUI(ToolBar | Keys | StatusBar | Save, "quadkonsole4ui.rc");

	QWidget* centralWidget = new QWidget(this, 0);
	setCentralWidget(centralWidget);

	QGridLayout* grid = new QGridLayout(centralWidget);
	grid->setContentsMargins(0, 0, 0, 0);

	m_sidebarSplitter = new QSplitter(Qt::Horizontal);
	grid->addWidget(m_sidebarSplitter, 0, 0);

	// TODO search for a valid sidebar instead of hard coded name
	m_sidebar = new QKView(m_partManager, new QKBrowserInterface(*(QKGlobalHistory::self())), "konq_sidebartng.desktop");
	m_sidebar->setFocusPolicy(Qt::ClickFocus);
	connect(m_sidebar, SIGNAL(openUrlRequest(KUrl)), SLOT(slotOpenUrl(KUrl)));
	connect(m_sidebar, SIGNAL(createNewWindow(KUrl,QString,KParts::ReadOnlyPart**)), SLOT(slotNewWindow(KUrl,QString,KParts::ReadOnlyPart**)));
	m_sidebarSplitter->addWidget(m_sidebar);
	if (Settings::showSidebar())
		m_sidebar->show();
	else
		m_sidebar->hide();

	if (Settings::layoutVertical())
		m_rows = new QSplitter(Qt::Vertical);
	else
		m_rows = new QSplitter(Qt::Horizontal);

	m_rows->setChildrenCollapsible(false);
	m_sidebarSplitter->addWidget(m_rows);
	m_sidebarSplitter->setCollapsible(1, false);
	m_sidebarSplitter->setStretchFactor(0, 1);
	m_sidebarSplitter->setStretchFactor(1, 4);

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
	kDebug() << "finished setting up layouts for" << m_stacks.count() << "parts" << endl;

	setWindowIcon(KIcon("quadkonsole4"));
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
				row = 0;
			col = (col +1) % m_rowLayouts[0]->count();
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
				row = m_rowLayouts.count() -1;
			col = (col + m_rowLayouts[row]->count() -1) % m_rowLayouts[row]->count();
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
	QuadKonsole* external = new QuadKonsole(part, stack->history().history(), stack->history().position());
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
			kDebug() << "action" << it.key() << "not in actionCollection" << endl;
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

	if (Settings::showSidebar())
		m_sidebar->show();
	else
		m_sidebar->hide();
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

		QListIterator<QKStack*> stackIt(m_stacks);
		while (stackIt.hasNext())
		{
			QKStack* stack = stackIt.next();
			dialog.addViews(stack, stack->modifiedViews());
		}

		if (dialog.size())
		{
			QMap<QKView*, QKStack*> doDetach;
			if (! dialog.exec(doDetach))
				return false;
			else
			{
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


void QuadKonsole::saveProperties(KConfigGroup& config)
{
	bool orientationVertical;
	if (m_rows->orientation() == Qt::Vertical)
		orientationVertical = true;
	else
		orientationVertical = false;
	config.writeEntry("layoutOrientationVertical", orientationVertical);
	config.writeEntry("sidebarSizes", m_sidebarSplitter->sizes());

	QList<int> rowSizes = m_rows->sizes();
	config.writeEntry("rowSizes", rowSizes);
	kDebug() << "saving session. rowSizes =" << rowSizes << endl;

	for (int i=0; i<m_rowLayouts.size(); ++i)
	{
		kDebug() << QString("row_%1").arg(i) << "=" << m_rowLayouts[i]->sizes() << endl;
		config.writeEntry(QString("row_%1").arg(i), m_rowLayouts[i]->sizes());
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

	kDebug() << "adjusting number of rows:" << m_rowLayouts.size() << "=>" << rowSizes.size() << endl;
	// adjust number of rows
	while (m_rowLayouts.size() < rowSizes.size())
		insertVertical(0, 0);

	for (int i=0; i<rowSizes.size(); ++i)
	{
		kDebug() << "restoring row" << i << endl;
		QList<int> sizes = config.readEntry(QString("row_%1").arg(i), QList<int>());
		while (m_rowLayouts[i]->count() < sizes.size())
			insertHorizontal(i, 0);

		m_rowLayouts[i]->setSizes(sizes);
	}

	m_sidebarSplitter->setSizes(config.readEntry("sidebarSizes", QList<int>()));
	m_rows->setSizes(rowSizes);

	bool orientationVertical = config.readEntry("orientationVertical", true);
	if (orientationVertical && m_rows->orientation() == Qt::Horizontal)
		changeLayout();
	else if (! orientationVertical && m_rows->orientation() == Qt::Vertical)
		changeLayout();

	m_activeStack = m_stacks.front();
	m_stacks.front()->setFocus();
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
	for (row=0; row<m_rowLayouts.size(); ++row)
	{
		for (col=0; col<m_rowLayouts[row]->count(); ++col)
		{
			QKStack* stack = qobject_cast<QKStack*>(m_rowLayouts[row]->widget(col));
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
				QMap<QKView*, QKStack*> doDetach;
				if (! dialog.exec(doDetach))
					return;
				else
				{
					kDebug() << "detaching" << doDetach.count() << "views" << endl;
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
	unplugActionList("view_edit");

	QKStack* stack = getFocusStack();
	if (stack)
	{
		m_activeStack = stack;
		m_urlBar->lineEdit()->setText(stack->url());
		m_urlBar->lineEdit()->setCursorPosition(0);
		plugActionList("view_settings", stack->currentWidget()->pluggableSettingsActions());
		plugActionList("view_edit", stack->currentWidget()->pluggableEditActions());
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
		kDebug() << "no stack removed??" << endl;

	if (m_stacks.empty())
		quit();
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
		kDebug() << "won't set location bar url for an inactive stack" << endl;
}


void QuadKonsole::zoomView(int row, int col)
{
	if (m_zoomed.first == row && m_zoomed.second == col)
		resetLayouts();
	else
	{
		if (row < 0 || row >= m_rowLayouts.count())
		{
			kDebug() << "invalid row:" << row << endl;
			return;
		}
		if (col < 0 || col >= m_rowLayouts.at(row)->count())
		{
			kDebug() << "ivalid column:" << col << endl;
			return;
		}

		zoomSplitter(m_rows, row);
		zoomSplitter(m_rowLayouts.at(row), col);
		m_zoomed = qMakePair(row, col);
	}
}


void QuadKonsole::slotToggleSidebar()
{
	if (m_sidebar->isVisible())
		m_sidebar->hide();
	else
		m_sidebar->show();
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


void QuadKonsole::slotOpenUrl(const KUrl& url)
{
	if (m_activeStack)
	{
		m_activeStack->setFocus();
		m_activeStack->slotOpenUrlRequest(url);
	}
}


void QuadKonsole::slotNewWindow(const KUrl& url, const QString& mimeType, KParts::ReadOnlyPart** target)
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
			m_activeStack->currentWidget()->setURL(KUrl(url));
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
		KMessageBox::error(this, "There is no session to be restored. You need to save one before restoring.", "No saved session found");
}
#endif // DEBUG

#include "quadkonsole.moc"
