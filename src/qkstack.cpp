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
#include "settings.h"

#include <KDE/KDebug>
#include <KDE/KUrl>
#include <KDE/KMimeTypeTrader>
#include <KDE/KIO/TransferJob>
#include <KDE/KIO/Scheduler>

#include <QtGui/QTabWidget>
#include <QtGui/QTabBar>

QKStack::QKStack(KParts::PartManager& partManager, QWidget* parent)
	: QTabWidget(parent),
	m_partManager(partManager)
{
	setupUi();
}


QKStack::QKStack(KParts::PartManager& partManager, KParts::ReadOnlyPart* part, QWidget* parent)
	: QTabWidget(parent),
	m_partManager(partManager)
{
	setupUi(part);
}


QKStack::~QKStack()
{}


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
		view = new QKView(m_partManager, name);
		m_loadedViews << name;
		index = addTab(view, view->icon(), view->partName());
		addViewActions(view);
	}
	return index;
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
			tabBar()->show();
			break;
		case Settings::EnumShowTabBar::whenNeeded :
			if (count() > 1)
				tabBar()->show();
			else
				tabBar()->hide();
			break;
		case Settings::EnumShowTabBar::never :
		default:
			tabBar()->hide();
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
	// do not close the first view
	if (index == 0)
		return;

	QKView* view = qobject_cast<QKView*>(widget(index));
	if (view)
	{
		removeTab(index);
		m_loadedViews.removeOne(view->partName());
		delete view;
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
		if (count() > 1)
			tabBar()->show();
		else
			tabBar()->hide();
	}
}


void QKStack::setupUi(KParts::ReadOnlyPart* part)
{
	setContentsMargins(0, 0, 0, 0);
	// do not accept focus at the tab bar. give it to the current view instead
	setFocusPolicy(Qt::NoFocus);
	// don't draw a frame
	setDocumentMode(true);
	setTabsClosable(true);

	QKView* view;
	QStringList partNames = Settings::views();
	if (part)
	{
		view = new QKView(m_partManager, part);
		partNames.removeOne(view->partName());
		m_loadedViews << view->partName();
		addTab(view, view->icon(), view->partName());
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
}


void QKStack::addViewActions(QKView* view)
{
	connect(view, SIGNAL(partCreated()), SLOT(slotPartCreated()));
	connect(view, SIGNAL(setStatusBarText(QString)), SLOT(slotSetStatusBarText(QString)));
	connect(view, SIGNAL(setWindowCaption(QString)), SLOT(slotSetWindowCaption(QString)));
	connect(view, SIGNAL(openUrlRequest(KUrl)), SLOT(slotOpenUrlRequest(KUrl)));
}

#include "qkstack.moc"
