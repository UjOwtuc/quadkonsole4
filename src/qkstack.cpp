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
#include "settings.h"

#include <KDE/KDebug>
#include <KDE/KUrl>
#include <KDE/KMimeTypeTrader>
#include <KDE/KXmlGuiWindow>
#include <KDE/KTabWidget>
#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KParts/PartManager>
#include <KDE/KIO/TransferJob>
#include <KDE/KIO/Scheduler>

#ifdef HAVE_LIBKONQ
#include <konq_popupmenu.h>
#else
#include <KDE/KMenu>
#endif

QKStack::QKStack(KParts::PartManager& partManager, QWidget* parent)
	: KTabWidget(parent),
	m_partManager(partManager),
	m_browserInterface(new QKBrowserInterface(this))
{
	setupUi();
	m_blockHistory = false;
}


QKStack::QKStack(KParts::PartManager& partManager, KParts::ReadOnlyPart* part, QWidget* parent)
	: KTabWidget(parent),
	m_partManager(partManager),
	m_browserInterface(new QKBrowserInterface(this))
{
	setupUi(part);
	m_blockHistory = false;
}


QKStack::~QKStack()
{
	this->disconnect();
	while (count())
	{
		QWidget* w = currentWidget();

		QKView* view;
		if ((view = qobject_cast<QKView*>(w)))
			view->disconnect();

		removeTab(currentIndex());
		delete w;
	}
	delete m_browserInterface;
}


bool QKStack::hasFocus() const
{
	if (count())
	{
		QKView* view = qobject_cast<QKView*>(currentWidget());
		return view->hasFocus();
	}
	return false;
}


void QKStack::setFocus()
{
	QKView* view = qobject_cast<QKView*>(currentWidget());
	view->setFocus();
	checkEnableActions();
}


QString QKStack::foregroundProcess() const
{
	QKView* view = qobject_cast<QKView*>(currentWidget());
	return view->foregroundProcess();
}


KParts::ReadOnlyPart* QKStack::part()
{
	QKView* view = qobject_cast<QKView*>(currentWidget());
	return view->part();
}


void QKStack::partDestroyed()
{
	QKView* view = qobject_cast<QKView*>(currentWidget());
	view->partDestroyed();
}


void QKStack::sendInput(const QString& text)
{
	QKView* view = qobject_cast<QKView*>(currentWidget());
	view->sendInput(text);
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
		m_loadedViews << name;
		if (view->icon())
			index = addTab(view, *(view->icon()), view->partName());
		else
			index = addTab(view, view->partName());
		addViewActions(view);
	}
	return index;
}


void QKStack::setHistory(const QList< KUrl >& history, int historyPosition)
{
	m_history = history;
	m_historyPosition = historyPosition;
	checkEnableActions();
}


QString QKStack::url() const
{
	QKView* view = qobject_cast<QKView*>(currentWidget());
	if (view)
		return view->getURL().pathOrUrl();
	return "";
}


QString QKStack::partIcon() const
{
	QKView* view = qobject_cast<QKView*>(currentWidget());
	if (view)
		return view->partIcon();
	return "";
}


void QKStack::switchView()
{
	QKView* view = qobject_cast<QKView*>(currentWidget());
	if (view)
	{
		KUrl url = view->getURL();
		if (url.protocol() == "file")
		{
			KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
			switchView(url, item.mimetype(), false);
		}
		else
		{
			KIO::JobFlags flags = KIO::HideProgressInfo;
			KIO::TransferJob* job = KIO::get(url, KIO::NoReload, flags);
			job->setProperty("tryCurrent", false);
			connect(job, SIGNAL(mimetype(KIO::Job*,QString)), SLOT(slotMimetype(KIO::Job*,QString)));
		}
	}
}


void QKStack::switchView(KUrl url, const QString& mimeType, bool tryCurrent)
{
	QKView* view = qobject_cast<QKView*>(currentWidget());

	if (tryCurrent && view->hasMimeType(mimeType))
		switchView(currentIndex(), url);
	else
	{
		int targetIndex = -1;

		// check for a loaded view that supports mimeType
		for (int i=1; i<count(); ++i)
		{
			view = qobject_cast<QKView*>(widget((currentIndex() +i) % count()));
			if (view->hasMimeType(mimeType))
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
			{
				view = qobject_cast<QKView*>(currentWidget());
				KService::List services = KMimeTypeTrader::self()->query(mimeType, "KParts/ReadOnlyPart");

				// only load a new part if it is different from the current one
				if (services.count() && services.front()->entryPath() != view->partName())
				{
					kDebug() << "loading KPart" << services.front()->entryPath() << "for mime type" << mimeType << endl;
					int index = addViews(QStringList(services.front()->entryPath()));
					targetIndex = index;
				}
				else if (services.isEmpty())
					kDebug() << "unable to find a service for mime type" << mimeType << endl;
			}
		}

		if (targetIndex == -1)
		{
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

	addHistoryEntry(url);

	QKView* view = qobject_cast<QKView*>(currentWidget());
	view->show();
	view->setURL(url);
	setFocusProxy(view);

	emit setStatusBarText(view->statusBarText());
	emit setWindowCaption(view->windowCaption());
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


void QKStack::toggleUrlBar()
{
	QKView* view = qobject_cast<QKView*>(currentWidget());
	if (view)
		view->toggleUrlBar();
}


void QKStack::slotTabCloseRequested(int index)
{
	QKView* view = qobject_cast<QKView*>(widget(index));
	if (view)
	{
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
	QKView* view = qobject_cast<QKView*>(currentWidget());
	if (view)
		slotOpenUrlRequest(view->getURL().upUrl());
}


void QKStack::goHistory(int steps)
{
	if (m_history.isEmpty())
		return;

	int old_pos = m_historyPosition;

	m_historyPosition -= steps;
	if (m_historyPosition < 0)
		m_historyPosition = 0;
	else if (m_historyPosition >= m_history.count())
		m_historyPosition = m_history.count() -1;

	if (old_pos != m_historyPosition)
	{
		m_blockHistory = true;
		slotOpenUrlRequest(m_history[m_historyPosition]);
		m_blockHistory = false;
	}
}


void QKStack::slotPartCreated()
{
	emit partCreated();
}


void QKStack::slotSetStatusBarText(QString text)
{
	emit setStatusBarText(text);
}


void QKStack::slotSetWindowCaption(QString text)
{
	emit setWindowCaption(text);
}


void QKStack::slotMimetype(KIO::Job* job, QString mimeType)
{
	KIO::TransferJob* transfer = qobject_cast<KIO::TransferJob*>(job);
	if (transfer)
	{
		transfer->disconnect(SIGNAL(mimetype(KIO::Job*,QString)), this);
		transfer->putOnHold();
		KIO::Scheduler::publishSlaveOnHold();
		switchView(transfer->url(), mimeType, transfer->property("tryCurrent").toBool());
	}
}


void QKStack::slotOpenUrlRequest(KUrl url)
{
	if (url.protocol() == "file")
	{
		KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
		switchView(url, item.mimetype(), true);
	}
	else
	{
		KIO::JobFlags flags = KIO::HideProgressInfo;
		KIO::TransferJob* job = KIO::get(url, KIO::NoReload, flags);
		job->setProperty("tryCurrent", true);
		connect(job, SIGNAL(mimetype(KIO::Job*,QString)), SLOT(slotMimetype(KIO::Job*,QString)));
	}
}


void QKStack::slotOpenUrlNotify()
{
	QKView* view = qobject_cast<QKView*>(currentWidget());
	if (view)
		addHistoryEntry(view->getURL());
}


void QKStack::slotCurrentChanged()
{
	QKView* view = qobject_cast<QKView*>(currentWidget());
	if (view)
	{
		view->show();
		view->setFocus();
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
	QKView* view = qobject_cast<QKView*>(currentWidget());
	if (view)
	{
#ifdef HAVE_LIBKONQ
		KActionCollection popupActions(static_cast<QWidget*>(0));
		KAction* back = popupActions.addAction("go_back", KStandardAction::back(this, SLOT(goBack()), &popupActions));
		back->setEnabled(m_historyPosition > 0);

		KAction* forward = popupActions.addAction("go_forward", KStandardAction::forward(this, SLOT(goForward()), &popupActions));
		forward->setEnabled(m_historyPosition < m_history.count() -1);

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


void QKStack::slotIconChanged()
{
	QKView* view = qobject_cast<QKView*>(sender());
	if (view)
	{
		int index = indexOf(view);
		setTabIcon(index, *(view->icon()));
		kDebug() << "setting icon for tab page" << index << endl;
	}
	else
		kDebug() << "which view changed it's icon???" << endl;
}


void QKStack::setupUi(KParts::ReadOnlyPart* part)
{
	setContentsMargins(0, 0, 0, 0);
	// do not accept focus at the tab bar. give it to the current view instead
	setFocusPolicy(Qt::NoFocus);
	// don't draw a frame
	setDocumentMode(true);
	setTabsClosable(true);

	connect(this, SIGNAL(mouseMiddleClick(QWidget*)), SLOT(slotMiddleClick(QWidget*)));

	QKView* view;
	QStringList partNames = Settings::views();
	if (part)
	{
		view = new QKView(m_partManager, m_browserInterface, part);
		partNames.removeOne(view->partName());
		m_loadedViews << view->partName();
		addTab(view, *(view->icon()), view->partName());
		addViewActions(view);
	}
	addViews(partNames);

	view = qobject_cast<QKView*>(currentWidget());
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
	connect(view, SIGNAL(iconChanged()), SLOT(slotIconChanged()));
}


void QKStack::checkEnableActions()
{
	if (m_historyPosition > 0)
		enableAction("go_back", true);
	else
		enableAction("go_back", false);

	if (m_historyPosition < m_history.count() -1)
		enableAction("go_forward", true);
	else
		enableAction("go_forward", false);
}


void QKStack::addHistoryEntry(const KUrl& url)
{
	if (! m_blockHistory)
	{
		// sanity checks (there was a crash related to history cleaning)
		if (m_historyPosition < 0)
			m_historyPosition = 0;

		if (! m_history.isEmpty())
		{
			// remove forward entries
			while (m_historyPosition < m_history.count() -1)
				m_history.pop_back();

			// filter duplicate entries
			if (m_history.back() != url)
				m_history.append(url);
		}
		else
			m_history.append(url);

		m_historyPosition = m_history.count() -1;

		while (static_cast<unsigned>(m_history.size()) > Settings::historySize())
			m_history.pop_front();
	}
	checkEnableActions();
}

#include "qkstack.moc"
