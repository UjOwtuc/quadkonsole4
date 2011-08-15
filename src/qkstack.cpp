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

#include <QtGui/QStackedWidget>

QKStack::QKStack(KParts::PartManager& partManager, QWidget* parent)
	: QStackedWidget(parent),
	m_partManager(partManager)
{
	setupUi();
}


QKStack::QKStack(KParts::PartManager& partManager, KParts::ReadOnlyPart* part, QWidget* parent)
	: QStackedWidget(parent),
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


void QKStack::addViews(const QStringList& partNames)
{
	QKView* view;
	QStringListIterator it(partNames);
	while (it.hasNext())
	{
		QString name = it.next();
		view = new QKView(m_partManager, name);
		m_loadedViews << name;
		addWidget(view);
		connect(view, SIGNAL(partCreated()), SLOT(slotPartCreated()));
		connect(view, SIGNAL(setStatusBarText(QString)), SLOT(slotSetStatusBarText(QString)));
		connect(view, SIGNAL(setWindowCaption(QString)), SLOT(slotSetWindowCaption(QString)));
		connect(view, SIGNAL(openUrlOutside(KUrl)), SLOT(switchView(KUrl)));
	}
}


void QKStack::switchView(KUrl url)
{
	QKView* view;
	if (url.isEmpty())
	{
		view = qobject_cast<QKView*>(currentWidget());
		url = view->getURL();
	}

	for (int i=1; i<count(); ++i)
	{
		view = qobject_cast<QKView*>(widget((currentIndex() +i) % count()));
		if (view->hasMimeType(url))
		{
			setCurrentIndex((currentIndex() +i) % count());
			view = qobject_cast<QKView*>(currentWidget());
			view->show();
			view->setURL(url);
			setFocusProxy(view);

			emit setStatusBarText(view->statusBarText());
			emit setWindowCaption(view->windowCaption());
			return;
		}
	}
	kDebug() << "no KPart want's to handle" << url << endl;
	switchView(url.upUrl());
	emit setStatusBarText(i18n("No view is able to open \"%1\"", url.pathOrUrl()));
}


void QKStack::settingsChanged()
{
	QStringList partNames = Settings::views();
	QStringListIterator it(m_loadedViews);
	while (it.hasNext())
		partNames.removeOne(it.next());

	addViews(partNames);
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


void QKStack::setupUi(KParts::ReadOnlyPart* part)
{
	setContentsMargins(0, 0, 0, 0);

	QKView* view;
	QStringList partNames = Settings::views();
	if (part)
	{
		view = new QKView(m_partManager, part, this);
		partNames.removeOne(view->partName());
		m_loadedViews << view->partName();
		addWidget(view);
		connect(view, SIGNAL(partCreated()), SLOT(slotPartCreated()));
		connect(view, SIGNAL(setStatusBarText(QString)), SLOT(slotSetStatusBarText(QString)));
		connect(view, SIGNAL(setWindowCaption(QString)), SLOT(slotSetWindowCaption(QString)));
		connect(view, SIGNAL(openUrlOutside(KUrl)), SLOT(switchView(KUrl)));
	}
	addViews(partNames);

	view = qobject_cast<QKView*>(currentWidget());
	view->show();
	setFocusProxy(view);

	connect(Settings::self(), SIGNAL(configChanged()), SLOT(settingsChanged()));
}

#include "qkstack.moc"
