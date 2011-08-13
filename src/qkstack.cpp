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

#include <QtGui/QStackedWidget>

QKStack::QKStack(QWidget* parent)
	: QStackedWidget(parent)
{
	setupUi();
}


QKStack::QKStack(KParts::ReadOnlyPart* part, QWidget* parent)
	: QStackedWidget(parent)
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


void QKStack::switchView()
{
	QKView* view = qobject_cast<QKView*>(currentWidget());
	QString url = view->getURL();
	setCurrentIndex((currentIndex() +1) % count());
	view = qobject_cast<QKView*>(currentWidget());
	view->show();
	view->setURL(url);
	setFocusProxy(view);
}


void QKStack::slotPartCreated()
{
	emit partCreated();
}


void QKStack::setupUi(KParts::ReadOnlyPart* part)
{
	setContentsMargins(0, 0, 0, 0);

	QStringList partNames = Settings::views();
	QKView* view;
	if (part)
		view = new QKView(part);
	else
		view = new QKView(partNames.front());

	addWidget(view);
	view->show();
	setFocusProxy(view);
	connect(view, SIGNAL(partCreated()), SLOT(slotPartCreated()));

	partNames.pop_front();
	while (partNames.count())
	{
		view = new QKView(partNames.front());
		partNames.pop_front();
		addWidget(view);
	}
}

#include "qkstack.moc"
