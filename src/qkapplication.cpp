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

#include "qkapplication.h"
#include "quadkonsole.h"
#include "settings.h"

#include <QApplication>
#include <QDir>

#include "qkapplicationadaptor.h"

QKApplicationAdaptor* QKApplication::m_dbusAdaptor = 0;

QKApplication::QKApplication()
	: QObject()
{
	qDebug() << "theme" << QIcon::themeName();
	qDebug() << "search paths" << QIcon::themeSearchPaths();
	QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("quadkonsole4"), QApplication::windowIcon()));
	// TODO
// 	if (restoringSession())
// 	{
// 		// restore all MainWindows before forking and calling newInstance()
// 		// forking seems to invalidate the session KConfigGroup
// 		// called from newInstance(), QuadKonsole::readProperties() cannot find any values
// 		int n = 1;
// 		while (KMainWindow::canBeRestored(n))
// 		{
// 			qDebug() << "restore window" << n;
// 			QuadKonsole* mainWin = new QuadKonsole;
// 			mainWin->restore(n);
// 			setupWindow(mainWin);
// 			++n;
// 		}
// 	}
}


QKApplication::~QKApplication()
{}


void QKApplication::exec()
{
// 	KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
	static bool first = true;

	if (first)
	{
		m_dbusAdaptor = new QKApplicationAdaptor(this);
		QDBusConnection::sessionBus().registerObject("/QKApplication", this);
	}

	int rows = 0;
	int columns = 0;

// 	if (args->isSet("rows"))
// 		rows = args->getOption("rows").toInt();
// 	if (args->isSet("columns"))
// 		columns = args->getOption("columns").toInt();
// 	QStringList cmds = args->getOptionList("cmd");
// 	QStringList urls = args->getOptionList("url");
// 	for (int i=0; i<args->count(); ++i)
// 		urls << args->arg(i);
/*
	// convert URLs to absolute paths but keep the specified index if given in URL
	QStringList::iterator it;
	for (it=urls.begin(); it!=urls.end(); ++it)
	{
		int index;
		QString url = splitIndex(*it, &index);
		url = KCmdLineArgs::makeURL(url.toUtf8()).path();
		if (index > -1)
			url = QString::number(index) + ":" + url;

		*it = url;
	}

	if (restoringSession())
	{
		// no need to create anything. this has been done by the original process before forking
	}
	else if (!first && args->count())
	{
		// there is another window running and we got at least one url to open
		// so we pass them on to the first running instance
		QPointer<QuadKonsole> topWindow;
		if (focusWidget())
		{
			QuadKonsole* win = qobject_cast<QuadKonsole*>(focusWidget()->window());
			if (win)
				topWindow = win;
			else
				qDebug() << "found input focus, but containing window is no QuadKonsole";
		}

		if (topWindow.isNull())
		{
			qDebug() << "no opened window has focus, selectin the newest";
			topWindow = m_mainWindows.back();
		}

		topWindow->openUrls(urls, true);
		topWindow->raise();
	}
	else
	{*/
		// without optionless arguments, we start a standard MainWindow
// 		QuadKonsole* mainWin = new QuadKonsole(rows, columns, cmds, urls);
		QuadKonsole* mainWin = new QuadKonsole(rows, columns, QStringList(), QStringList());
		setupWindow(mainWin);

// 		setTopWidget(mainWin);

		if (Settings::startMaximized())
			mainWin->showMaximized();
		else
			mainWin->show();
// 	}

// 	first = false;
// 	args->clear();
}


uint QKApplication::windowCount() const
{
	return m_mainWindows.count();
}


void QKApplication::setupWindow(QuadKonsole* mainWindow)
{
	connect(mainWindow, SIGNAL(destroyed()), SLOT(windowDestroyed()));
	connect(mainWindow, SIGNAL(detached(QuadKonsole*)), SLOT(setupWindow(QuadKonsole*)));
	m_mainWindows.append(QPointer<QuadKonsole>(mainWindow));
}


void QKApplication::windowDestroyed()
{
	m_mainWindows.removeAll(0);
}

#include "qkapplication.moc"
