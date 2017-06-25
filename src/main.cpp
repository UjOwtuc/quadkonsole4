/***************************************************************************
 *   Copyright (C) 2005 by Simon Perreault                                 *
 *   nomis80@nomis80.org                                                   *
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

#include "version.h"
#include "qkapplication.h"
#include "settings.h"

#include <KDE/KUniqueApplication>
#include <KDE/KApplication>
#include <KDE/KAboutData>
#include <KDE/KCmdLineArgs>
#include <KDE/KLocale>

#include <K4AboutData>

#include <QtCore/QDir>

#ifndef QUADKONSOLE4_VERSION
#error QUADKONSOLE4_VERSION undefined
#endif

static const char description[] = I18N_NOOP("Embeds multiple Konsoles in a grid layout");
static const char version[] = QUADKONSOLE4_VERSION;


int main(int argc, char **argv)
{
	K4AboutData about("quadkonsole4", 0, ki18n("quadkonsole4"), version, ki18n(description), K4AboutData::License_GPL_V2, ki18n("(C) 2005 Simon Perreault\n(C) 2009 - 2017 Karsten Borgwaldt"), KLocalizedString(), "http://spambri.de/quadkonsole4", "quadkonsole4@spambri.de");
	about.addAuthor(ki18n("Simon Perreault"), KLocalizedString(), "nomis80@nomis80.org");
	about.addAuthor(ki18n("Karsten Borgwaldt"), KLocalizedString(), "kb@spambri.de");
	about.addCredit(ki18n("Michael Feige"), ki18n("Many ideas and feature requests"));

	KCmdLineArgs::init(argc, argv, &about);

	KCmdLineOptions options;
	options.add("r").add("rows <rows>", ki18n("Number of rows of terminal emulators"), "2");
	options.add("c").add("columns <columns>", ki18n("Number of columns of terminal emulators"), "2");
	options.add("C").add("cmd [number:]<command>", ki18n("Run command [in given view] (may be used multiple times)"));
	options.add("u").add("url [number:]<url>", ki18n("Open URL [in given view] (may be used multiple times)"));
	options.add("+[URL]", ki18n("Open specified URL in a running instance of QuadKonsole4"));
	KCmdLineArgs::addCmdLineOptions(options);
	KUniqueApplication::addCmdLineOptions();

	QKApplication app;
	app.setOrganizationDomain("spambri.de");
	app.setDesktopFileName("de.spambri.quadkonsole4");
	app.setApplicationVersion(version);
	return app.exec();
}
