/***************************************************************************
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

#include "qkstack.h"
#include "qkview.h"
#include "qkbrowseriface.h"
#include "qkurlhandler.h"
#include "settings.h"

#include <KRun>
#include <KMimeTypeTrader>
#include <KXmlGuiWindow>
#include <KActionCollection>
#include <KMessageBox>
#include <KLocalizedString>
#include <KParts/PartManager>
#include <KParts/HistoryProvider>

#include <QTabBar>


QKStack::QKStack(KParts::PartManager& partManager, bool restoringSession, QWidget* parent)
	: QTabWidget(parent),
	m_partManager(partManager)
{
	setupUi(0, restoringSession);
	m_browserInterface = new QKBrowserInterface(*QKHistory::self());
}


QKStack::QKStack(KParts::PartManager& partManager, KParts::ReadOnlyPart* part, QWidget* parent)
	: QTabWidget(parent),
	m_partManager(partManager)
{
	setupUi(part);
	m_browserInterface = new QKBrowserInterface(*QKHistory::self());
}


QKStack::~QKStack()
{
	this->disconnect();
}


QString QKStack::foregroundProcess() const
{
	return currentWidget()->foregroundProcess();
}


KParts::ReadOnlyPart* QKStack::part() const
{
	auto w = currentWidget();
	if (w)
		return w->part();
	return 0;
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
		return currentWidget()->getURL().url();

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
	static_cast<QTabWidget*>(this)->setCurrentIndex(index);

	QKView* view = currentWidget();
	if (view)
	{
		view->show();
		setFocusProxy(view);
	}
}


QKView* QKStack::currentWidget() const
{
	QKView* view = qobject_cast<QKView*>( static_cast<const QTabWidget*>(this)->currentWidget() );
	if (! view)
	{
		qDebug() << "could not get current QKView";
		return 0;
	}

	return view;
}


const QKView* QKStack::view(int index) const
{
	QKView* view = qobject_cast<QKView*>(widget(index));
	return view;
}


void QKStack::switchView()
{
	slotOpenUrlRequest(currentWidget()->getURL(), false);
}


void QKStack::switchView(const QUrl& url, const QString& mimeType, bool tryCurrent)
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
				qDebug() << "switching to" << view->partName() << "for mime type" << mimeType;
				targetIndex = (currentIndex() +i) % count();
				break;
			}
		}

		if (targetIndex == -1)
		{
			qDebug() << "no other loaded KPart want's to handle" << url;

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
				// TODO check
				if (url.path() == "/")
				{
					// something is really going wrong. I can't open url within any loaded/loadable KPart and it is already the root folder of that url.
					// display the next view but don't bother to keep trying on this url
					switchView(QUrl("~"), "inode/directory", false);
				}
				else
				{
					QString path = url.path();
					path.replace(QRegExp("/[^/][^/]*$/"), QString());
					QUrl target = url;
					target.setPath(path);
					switchView((currentIndex() +1) % count(), target);
				}
			}

			emit setStatusBarText(i18n("No view is able to open \"%1\"", url.url()));
		}
		else
		{
			// something want's to open url, switch to it
			switchView(targetIndex, url);
		}
	}
}


void QKStack::switchView(int index, const QUrl& url)
{
	if (index != currentIndex())
		setCurrentIndex(index);

	if (! url.isEmpty())
	{
		currentWidget()->setURL(url);
		QKHistory::self()->addEntry(url.url());
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
			tabBar()->setVisible(true);
			break;
		case Settings::EnumShowTabBar::whenNeeded :
			tabBar()->setVisible(count() > 1);
			break;
		case Settings::EnumShowTabBar::never :
		default:
			tabBar()->setVisible(false);
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
		view->deleteLater();
	}

	// closed last view
	if (count() == 0)
	{
		// for some reason the destroyed signal is not emitted when calling deleteLater
		emit destroyed(this);
		deleteLater();
	}

	if (Settings::showTabBar() == Settings::EnumShowTabBar::whenNeeded && count() <= 1)
		tabBar()->setVisible(false);
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
	// TODO
	slotOpenUrlRequest(currentWidget()->getURL());
}


void QKStack::goHistory(int steps)
{
	QString url = QKHistory::self()->go(steps);
	if (! url.isEmpty())
	{
		QKHistory::self()->lock();
		slotOpenUrlRequest(QUrl(url));
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


void QKStack::slotOpenUrlRequest(const QUrl& url, bool tryCurrent)
{
	QKUrlHandler* handler = new QKUrlHandler(url, true, this);
	handler->setProperty("tryCurrent", tryCurrent);
	connect(handler, SIGNAL(finished(QKUrlHandler*)), SLOT(slotUrlFiltered(QKUrlHandler*)));
}


void QKStack::slotOpenUrlRequest(const QString& url)
{
	slotOpenUrlRequest(QUrl(url), true);
}


void QKStack::slotOpenNewWindow(const QUrl& url, const QString& mimeType, KParts::ReadOnlyPart** target)
{
	if (mimeType.isEmpty() && ! target)
	{
		// nobody is waiting for us, try to get the mimeType before opening a new view
		QKUrlHandler* handler = new QKUrlHandler(url, true, this);
		handler->setProperty("newWindow", true);
		handler->setProperty("tryCurrent", false);
		connect(handler, SIGNAL(finished(QKUrlHandler*)), SLOT(slotUrlFiltered(QKUrlHandler*)));
	}
	else
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
}


void QKStack::slotUrlFiltered(QKUrlHandler* handler)
{
	if (handler->error().isEmpty())
	{
		if (handler->property("newWindow").toBool() && handler->mimetype().length())
		{
			slotOpenNewWindow(handler->url(), handler->mimetype(), 0);
		}
		else if (handler->partName().isEmpty())
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
		emit setStatusBarText(i18n("Could not open url '%1': %2", handler->url().url(), handler->error()));
	}
	handler->deleteLater();
}


void QKStack::slotOpenUrlNotify()
{
	checkEnableActions();
	QKHistory::self()->addEntry(currentWidget()->getURL().url());
	emit setLocationBarUrl(currentWidget()->getURL().url());
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
		emit setLocationBarUrl(view->getURL().url());
	}

	if (Settings::showTabBar() == Settings::EnumShowTabBar::whenNeeded)
	{
		tabBar()->setVisible(count() > 1);
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


void QKStack::setupUi(KParts::ReadOnlyPart* part, bool restoringSession)
{
	setContentsMargins(0, 0, 0, 0);
	// do not accept focus at the tab bar. give it to the current view instead
	setFocusPolicy(Qt::NoFocus);
	// don't draw a frame
	setDocumentMode(true);
	setTabsClosable(true);
	setMovable(true);

	// do not create any views from config if session restoration is in progress
	if (! restoringSession)
	{
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
		if (view)
		{
			view->show();
			view->setFocus();
			setFocusProxy(view);
		}

		settingsChanged();
	}

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
	connect(view, SIGNAL(openUrlRequest(QUrl)), SLOT(slotOpenUrlRequest(QUrl)));
	connect(view, SIGNAL(openUrlNotify()), SLOT(slotOpenUrlNotify()));
	connect(view, SIGNAL(createNewWindow(QUrl,QString,KParts::ReadOnlyPart**)), SLOT(slotOpenNewWindow(QUrl,QString,KParts::ReadOnlyPart**)));
	connect(view, SIGNAL(destroyed()), SLOT(slotCurrentChanged()));

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
			qDebug() << "loading KPart" << name << "for mime type" << mimeType;
			openedIndex = addViews(QStringList(name));
		}
	}
	else
		qDebug() << "unable to find a service for mime type" << mimeType;

	return openedIndex;
}

// #include "qkstack.moc"
