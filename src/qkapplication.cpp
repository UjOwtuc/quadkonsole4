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

#include "qkapplication.h"
#include "quadkonsole.h"
#include "settings.h"

#include <KDE/KUniqueApplication>
#include <KDE/KCmdLineArgs>

#include <QtCore/QDir>

#include "qkapplicationadaptor.h"

QKApplicationAdaptor* QKApplication::m_dbusAdaptor = 0;

QKApplication::QKApplication(bool GUIenabled, bool configUnique)
	: KUniqueApplication(GUIenabled, configUnique)
{
	KCmdLineArgs::setCwd(QDir::currentPath().toUtf8());
}


QKApplication::~QKApplication()
{}


int QKApplication::newInstance()
{
	KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
	static bool first = true;

	if (first)
		m_dbusAdaptor = new QKApplicationAdaptor(this);

	int rows = 0;
	int columns = 0;

	if (args->isSet("rows"))
		rows = args->getOption("rows").toInt();
	if (args->isSet("columns"))
		columns = args->getOption("columns").toInt();
	QStringList cmds = args->getOptionList("cmd");
	QStringList urls = args->getOptionList("url");
	for (int i=0; i<args->count(); ++i)
		urls << args->arg(i);

	QStringList::iterator it;
	for (it=urls.begin(); it!=urls.end(); ++it)
		*it = KCmdLineArgs::makeURL((*it).toAscii()).pathOrUrl();

	if (isSessionRestored())
	{
		// ignore all arguments. just restore the session
		kRestoreMainWindows<QuadKonsole>();
	}
	else if (!first && args->count())
	{
		// there is another window running and we got at least one url to open
		// so we pass them on to the first running instance
		// TODO select the MainWindow with focus for opening
		m_mainWindows.front()->openUrls(urls);
		m_mainWindows.front()->raise();
	}
	else
	{
		// without optionless arguments, we start a standard MainWindow
		QuadKonsole* mainWin = new QuadKonsole(rows, columns, cmds, urls);
		setupWindow(mainWin);
		setTopWidget(mainWin);

		if (Settings::startMaximized())
			mainWin->showMaximized();
		else
			mainWin->show();
	}

	first = false;
	args->clear();
	return 0;
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
