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

#include "version.h"
#include "quadkonsole.h"
#include "settings.h"

#include <KDE/KApplication>
#include <KDE/KAboutData>
#include <KDE/KCmdLineArgs>
#include <KDE/KLocale>

#ifndef QUADKONSOLE4_VERSION
#error QUADKONSOLE4_VERSION undefined
#endif

static const char description[] = I18N_NOOP("Embeds multiple Konsoles in a grid layout");
static const char version[] = QUADKONSOLE4_VERSION;

int main(int argc, char **argv)
{
	KAboutData about("quadkonsole4", 0, ki18n("quadkonsole4"), version, ki18n(description), KAboutData::License_GPL, ki18n("(C) 2005 Simon Perreault\n(C) 2009 - 2011 Karsten Borgwaldt"), KLocalizedString(), "http://kb.ccchl.de/quadkonsole4", "quadkonsole4@kb.ccchl.de");
	about.addAuthor(ki18n("Simon Perreault"), KLocalizedString(), "nomis80@nomis80.org");
	about.addAuthor(ki18n("Karsten Borgwaldt"), KLocalizedString(), "kb@kb.ccchl.de");
	KCmdLineArgs::init(argc, argv, &about);

	KCmdLineOptions options;
	options.add("rows <rows>", ki18n("Number of rows of terminal emulators"), "2");
	options.add("columns <columns>", ki18n("Number of columns of terminal emulators"), "2");
	options.add("cmd [number:]<command>", ki18n("Run command [in given view] (may be used multiple times)"));
	options.add("url [number:]<url>", ki18n("Open URL [in given view] (may be used multiple times)"));
	KCmdLineArgs::addCmdLineOptions(options);
	KApplication app;

	if (app.isSessionRestored())
	{
		kRestoreMainWindows<QuadKonsole>();
	}
	else
	{
		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		int rows = 0;
		int columns = 0;

		if (args->isSet("rows"))
			rows = args->getOption("rows").toInt();
		if (args->isSet("columns"))
			columns = args->getOption("columns").toInt();
		QStringList cmds = args->getOptionList("cmd");
		QStringList urls = args->getOptionList("url");

		QuadKonsole* mainWin = new QuadKonsole(rows, columns, cmds, urls);
		app.setTopWidget(mainWin);

		if (Settings::startMaximized())
			mainWin->showMaximized();
		else
			mainWin->show();

		args->clear();
	}

	return app.exec();
}

