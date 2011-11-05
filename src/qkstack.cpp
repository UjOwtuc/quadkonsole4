/***************************************************************************
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

#include "qkstack.h"
#include "qkview.h"
#include "qkbrowseriface.h"
#include "qkurlhandler.h"
#include "settings.h"

#include <KDE/KDebug>
#include <KDE/KRun>
#include <KDE/KUrl>
#include <KDE/KMimeTypeTrader>
#include <KDE/KXmlGuiWindow>
#include <KDE/KTabWidget>
#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KMessageBox>
#include <KDE/KParts/PartManager>
#include <KDE/KParts/HistoryProvider>

#ifdef HAVE_LIBKONQ
#include <konq_popupmenu.h>
#else
#include <KDE/KMenu>
#endif

QKStack::QKStack(KParts::PartManager& partManager, QWidget* parent)
	: KTabWidget(parent),
	m_partManager(partManager)
{
	setupUi();
	m_browserInterface = new QKBrowserInterface(*QKHistory::self());
}


QKStack::QKStack(KParts::PartManager& partManager, KParts::ReadOnlyPart* part, QWidget* parent)
	: KTabWidget(parent),
	m_partManager(partManager)
{
	setupUi(part);
	m_browserInterface = new QKBrowserInterface(*QKHistory::self());
}


QKStack::~QKStack()
{
	this->disconnect();
	while (count())
	{
		QKView* view = currentWidget();
		if (view)
			view->disconnect();

		removeTab(currentIndex());
		delete view;
	}
	delete m_browserInterface;
}


QString QKStack::foregroundProcess() const
{
	return currentWidget()->foregroundProcess();
}


KParts::ReadOnlyPart* QKStack::part() const
{
	return currentWidget()->part();
}


void QKStack::sendInput(const QString& text) const
{
	currentWidget()->sendInput(text);
}


int QKStack::addViews(const QStringList& partNames)
{
	QKView* view;
	QStringListIterator it(partNames);
	int index = 0;
	while (it.hasNext())
	{
		QString name = it.next();
		view = new QKView(m_partManager, m_browserInterface, name);
		addViewActions(view);
		m_loadedViews << name;
		index = addTab(view, view->partCaption());
	}
	return index;
}


QString QKStack::url() const
{
	if (currentWidget())
		return currentWidget()->getURL().pathOrUrl();

	return "";
}


QString QKStack::partIcon() const
{
	QKView* view = currentWidget();
	if (view)
		return view->partIcon();
	return "";
}


QList<QKView*> QKStack::modifiedViews()
{
	QList<QKView*> modified;
	for (int i=0; i<count(); ++i)
	{
		QKView* view = qobject_cast<QKView*>(widget(i));
		if (view->isModified())
			modified << view;
	}
	return modified;
}


void QKStack::setCurrentIndex(int index)
{
	static_cast<KTabWidget*>(this)->setCurrentIndex(index);

	QKView* view = currentWidget();
	if (view)
	{
		view->show();
		setFocusProxy(view);
	}
}


QKView* QKStack::currentWidget() const
{
	QKView* view = qobject_cast<QKView*>( static_cast<const KTabWidget*>(this)->currentWidget() );
	if (! view)
	{
		kDebug() << "could not get current QKView" << endl;
		return 0;
	}

	return view;
}


const QKView* QKStack::view(int index) const
{
	QKView* view = qobject_cast<QKView*>(widget(index));
	return view;
}


void QKStack::saveProperties(KConfigGroup& config)
{
	config.writeEntry("currentIndex", currentIndex());
	for (int i=0; i<count(); ++i)
	{
		QKView* view = qobject_cast<QKView*>(widget(i));
		if (view)
		{
			config.writeEntry(QString("part_%1").arg(i), view->partName());
			config.writeEntry(QString("url_%1").arg(i), view->getURL());
		}
	}
}


void QKStack::readProperties(const KConfigGroup& config)
{
	int i;
	for (i=0; config.hasKey(QString("part_%1").arg(i)); ++i)
	{
		int index = addViews(QStringList(config.readEntry<QString>(QString("part_%1").arg(i), "konsolepart.desktop")));
		moveTab(index, i);
		switchView(i, config.readEntry<KUrl>(QString("url_%1").arg(i), KUrl()));
	}
	setCurrentIndex(config.readEntry("currentIndex", 0));

	// close all tabs that were created before readProperties()
	for (; i<count(); ++i)
		slotTabCloseRequested(i);
}


void QKStack::switchView()
{
	slotOpenUrlRequest(currentWidget()->getURL(), false);
}


void QKStack::switchView(const KUrl& url, const QString& mimeType, bool tryCurrent)
{
	QKView* view = currentWidget();

	if (tryCurrent && view->hasMimeType(mimeType, url))
		switchView(currentIndex(), url);
	else
	{
		int targetIndex = -1;

		// check for a loaded view that supports mimeType
		for (int i=1; i<count(); ++i)
		{
			view = qobject_cast<QKView*>(widget((currentIndex() +i) % count()));
			if (view->hasMimeType(mimeType, url))
			{
				kDebug() << "switching to" << view->partName() << "for mime type" << mimeType << endl;
				targetIndex = (currentIndex() +i) % count();
				break;
			}
		}

		if (targetIndex == -1)
		{
			kDebug() << "no other loaded KPart want's to handle" << url << endl;

			// load a new KPart to display mimeType
			if (Settings::autoloadView())
				targetIndex = openViewByMimeType(mimeType, false);
		}

		if (targetIndex == -1)
		{
			if (Settings::runExternal() && KRun::runUrl(url, mimeType, window(), false, false))
				return;

			// unable to open mimeType (either no KPart at all or I must not load it)
			// if I am able to display another view, I'll just do it
			if (count() > 1)
			{
				if (url == url.upUrl())
				{
					// something is really going wrong. I can't open url within any loaded/loadable KPart and it is already the root folder of that url.
					// display the next view but don't bother to keep trying on this url
					switchView(KUrl("~"), "inode/directory", false);
				}
				else
					switchView((currentIndex() +1) % count(), url.upUrl());
			}

			emit setStatusBarText(i18n("No view is able to open \"%1\"", url.pathOrUrl()));
		}
		else
		{
			// something want's to open url, switch to it
			switchView(targetIndex, url);
		}
	}
}


void QKStack::switchView(int index, const KUrl& url)
{
	if (index != currentIndex())
		setCurrentIndex(index);

	if (! url.isEmpty())
	{
		currentWidget()->setURL(url);
		QKHistory::self()->addEntry(url.pathOrUrl());
		checkEnableActions();
	}
}


void QKStack::settingsChanged()
{
	QStringList partNames = Settings::views();
	QStringListIterator it(m_loadedViews);
	while (it.hasNext())
		partNames.removeOne(it.next());

	addViews(partNames);

	switch (Settings::showTabBar())
	{
		case Settings::EnumShowTabBar::always :
			setTabBarHidden(false);
			break;
		case Settings::EnumShowTabBar::whenNeeded :
			setTabBarHidden(count() <= 1);
			break;
		case Settings::EnumShowTabBar::never :
		default:
			setTabBarHidden(true);
			break;
	}

	if (Settings::tabBarPosition() == Settings::EnumTabBarPosition::above)
		setTabPosition(North);
	else
		setTabPosition(South);
}


void QKStack::slotTabCloseRequested(int index)
{
	QKView* view = qobject_cast<QKView*>(widget(index));
	if (view)
	{
		if (Settings::queryClose() && view->isModified())
		{
			if (KMessageBox::questionYesNo(this, view->closeModifiedMsg()) == KMessageBox::No)
				return;
		}
		removeTab(index);
		m_loadedViews.removeOne(view->partName());
		delete view;
	}

	// closed last view
	if (count() == 0)
	{
		// for some reason the destroyed signal is not emitted when calling deleteLater
		emit destroyed(this);
		deleteLater();
	}

	if (Settings::showTabBar() == Settings::EnumShowTabBar::whenNeeded && count() <= 1)
		setTabBarHidden(true);
}


void QKStack::goBack()
{
	goHistory(1);
}


void QKStack::goForward()
{
	goHistory(-1);
}


void QKStack::goUp()
{
	slotOpenUrlRequest(currentWidget()->getURL().upUrl());
}


void QKStack::goHistory(int steps)
{
	QString url = QKHistory::self()->go(steps);
	if (! url.isEmpty())
	{
		QKHistory::self()->lock();
		slotOpenUrlRequest(KUrl(url));
		QKHistory::self()->lock(false);
	}
	checkEnableActions();
}


void QKStack::slotPartCreated()
{
	emit partCreated();
}


void QKStack::slotSetStatusBarText(const QString& text)
{
	emit setStatusBarText(text);
}


void QKStack::slotSetWindowCaption(const QString& text)
{
	emit setWindowCaption(text);
}


void QKStack::slotOpenUrlRequest(const KUrl& url, bool tryCurrent)
{
	QKUrlHandler* handler = new QKUrlHandler(url, this);
	handler->setProperty("tryCurrent", tryCurrent);
	connect(handler, SIGNAL(finished(QKUrlHandler*)), SLOT(slotUrlFiltered(QKUrlHandler*)));
}


void QKStack::slotOpenUrlRequest(const QString& url)
{
	slotOpenUrlRequest(KUrl(url), true);
}


void QKStack::slotOpenNewWindow(const KUrl& url, const QString& mimeType, KParts::ReadOnlyPart** target)
{
	int index = -1;
	if (mimeType.isEmpty())
	{
		index = addViews(QStringList(currentWidget()->partName()));
		setCurrentIndex(index);
		if (target)
			*target = currentWidget()->part();
		slotOpenUrlRequest(url, true);
	}
	else
	{
		index = openViewByMimeType(mimeType);
		if (index > -1)
		{
			switchView(index, url);
			if (target)
				*target = currentWidget()->part();
		}
	}
}


void QKStack::slotUrlFiltered(QKUrlHandler* handler)
{
	if (handler->error().isEmpty())
	{
		if (handler->partName().isEmpty())
		{
			bool tryCurrent = handler->property("tryCurrent").toBool();
			switchView(handler->url(), handler->mimetype(), tryCurrent);
		}
		else
		{
			int index = addViews(QStringList(handler->partName()));
			switchView(index, handler->url());
		}
	}
	else
	{
		emit setStatusBarText(i18n("Could not open url '%1': %2", handler->url().pathOrUrl(), handler->error()));
	}
	handler->deleteLater();
}


void QKStack::slotOpenUrlNotify()
{
	checkEnableActions();
	QKHistory::self()->addEntry(currentWidget()->getURL().pathOrUrl());
	emit setLocationBarUrl(currentWidget()->getURL().pathOrUrl());
}


void QKStack::slotCurrentChanged()
{
	QKView* view = currentWidget();
	if (view)
	{
		view->show();
		setFocusProxy(view);
		view->setFocus();

		emit setStatusBarText(view->statusBarText());
		emit setWindowCaption(view->windowCaption());
		emit setLocationBarUrl(view->getURL().pathOrUrl());
	}

	if (Settings::showTabBar() == Settings::EnumShowTabBar::whenNeeded)
	{
		setTabBarHidden(count() <= 1);
	}

	checkEnableActions();
}


void QKStack::enableAction(const char* action, bool enable)
{
	KXmlGuiWindow* window = qobject_cast<KXmlGuiWindow*>(m_partManager.parent());
	if (window)
	{
		QAction* a = window->actionCollection()->action(action);
		if (a)
			a->setEnabled(enable);
	}
}


void QKStack::popupMenu(const QPoint& where, const KFileItemList& items, KParts::BrowserExtension::PopupFlags flags, const KParts::BrowserExtension::ActionGroupMap& map)
{
	QKView* view = currentWidget();
	if (view)
	{
#ifdef HAVE_LIBKONQ
		KActionCollection popupActions(static_cast<QWidget*>(0));
		KAction* back = popupActions.addAction("go_back", KStandardAction::back(this, SLOT(goBack()), &popupActions));
		back->setEnabled(QKHistory::self()->canGoBack());

		KAction* forward = popupActions.addAction("go_forward", KStandardAction::forward(this, SLOT(goForward()), &popupActions));
		forward->setEnabled(QKHistory::self()->canGoForward());

		KAction* up = popupActions.addAction("go_up", KStandardAction::up(this, SLOT(goUp()), &popupActions));
		up->setEnabled(view->getURL().upUrl() != view->getURL());

		popupActions.addAction("cut", KStandardAction::cut(0, 0, this));
		popupActions.addAction("copy", KStandardAction::copy(0, 0, this));
		popupActions.addAction("paste", KStandardAction::paste(0, 0, this));
		QAction* separator = new QAction(&popupActions);
		separator->setSeparator(true);
		popupActions.addAction("separator", separator);

		KonqPopupMenu::Flags konqFlags = 0;
		KonqPopupMenu* menu = new KonqPopupMenu(items, view->getURL(), popupActions, 0, konqFlags, flags, view->part()->widget(), 0, map);
#else // HAVE_LIBKONQ
		KMenu* menu = new KMenu(this);
		QList<QActionGroup*> groups = view->part()->actionCollection()->actionGroups();
		QListIterator<QActionGroup*> it(groups);
		while (it.hasNext())
		{
			QActionGroup* group = it.next();
			if (group->isVisible() && group->isEnabled())
			{
				KMenu* submenu = new KMenu;
				submenu->addActions(group->actions());
				menu->addMenu(submenu);
			}
		}
		menu->addActions(view->part()->actionCollection()->actionsWithoutGroup());
#endif // HAVE_LIBKONQ
		menu->exec(where);
		delete menu;
	}
}


void QKStack::slotMiddleClick(QWidget* widget)
{
	int index;
	index = indexOf(widget);
	if (index < 0)
		kDebug() << "unable to find widget for removal by MMB" << endl;

	slotTabCloseRequested(index);
}


void QKStack::setupUi(KParts::ReadOnlyPart* part)
{
	setContentsMargins(0, 0, 0, 0);
	// do not accept focus at the tab bar. give it to the current view instead
	setFocusPolicy(Qt::NoFocus);
	// don't draw a frame
	setDocumentMode(true);
	setTabsClosable(true);
	setMovable(true);

	connect(this, SIGNAL(mouseMiddleClick(QWidget*)), SLOT(slotMiddleClick(QWidget*)));

	QKView* view;
	QStringList partNames = Settings::views();
	if (part)
	{
		view = new QKView(m_partManager, m_browserInterface, part);
		addViewActions(view);
		partNames.removeOne(view->partName());
		m_loadedViews << view->partName();
		addTab(view, view->partCaption());
	}
	addViews(partNames);

	view = currentWidget();
	view->show();
	view->setFocus();
	setFocusProxy(view);

	settingsChanged();
	connect(Settings::self(), SIGNAL(configChanged()), SLOT(settingsChanged()));
	connect(this, SIGNAL(currentChanged(int)), SLOT(slotCurrentChanged()));
	connect(this, SIGNAL(tabCloseRequested(int)), SLOT(slotTabCloseRequested(int)));

	enableAction("go_back", false);
	enableAction("go_forward", false);
}


void QKStack::addViewActions(QKView* view)
{
	connect(view, SIGNAL(partCreated()), SLOT(slotPartCreated()));
	connect(view, SIGNAL(setStatusBarText(QString)), SLOT(slotSetStatusBarText(QString)));
	connect(view, SIGNAL(setWindowCaption(QString)), SLOT(slotSetWindowCaption(QString)));
	connect(view, SIGNAL(openUrlRequest(KUrl)), SLOT(slotOpenUrlRequest(KUrl)));
	connect(view, SIGNAL(openUrlNotify()), SLOT(slotOpenUrlNotify()));
	connect(view, SIGNAL(popupMenu(QPoint,KFileItemList,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)), SLOT(popupMenu(QPoint,KFileItemList,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)));
	connect(view, SIGNAL(createNewWindow(KUrl,QString,KParts::ReadOnlyPart**)), SLOT(slotOpenNewWindow(KUrl,QString,KParts::ReadOnlyPart**)));

	// check for current widget before propagating?
	connect(view, SIGNAL(setLocationBarUrl(QString)), SIGNAL(setLocationBarUrl(QString)));
}


void QKStack::checkEnableActions()
{
	enableAction("go_back", QKHistory::self()->canGoBack());
	enableAction("go_forward", QKHistory::self()->canGoForward());
}


int QKStack::openViewByMimeType(const QString& mimeType, bool allowDuplicate)
{
	KService::List services = KMimeTypeTrader::self()->query(mimeType, "KParts/ReadOnlyPart");

	int openedIndex = -1;
	if (services.count())
	{
		QString name = services.front()->entryPath();

		if (! allowDuplicate && name == currentWidget()->partName())
		{
			openedIndex = currentIndex();
		}
		else
		{
			kDebug() << "loading KPart" << name << "for mime type" << mimeType << endl;
			openedIndex = addViews(QStringList(name));
		}
	}
	else
		kDebug() << "unable to find a service for mime type" << mimeType << endl;

	return openedIndex;
}

#include "qkstack.moc"
