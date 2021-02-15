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

#include "qkapplication.h"
#include "settings.h"
#include "version.h"

#include <KLocalizedString>
#include <KAboutData>
#include <KDBusService>

#include <QApplication>
#include <QCommandLineParser>

#ifndef QUADKONSOLE4_VERSION
#error QUADKONSOLE4_VERSION undefined
#endif

static const char description[] = I18N_NOOP("Embeds multiple Konsoles in a grid layout");
static const char version[] = QUADKONSOLE4_VERSION;

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	KLocalizedString::setApplicationDomain("quadkonsole4");

	KAboutData about(QStringLiteral("quadkonsole4"), i18n("QuadKonsole4"), version, description, KAboutLicense::GPL_V2, i18n("(C) 2005 Simon Perreault\n(C) 2009 - 2017 Karsten Borgwaldt"), "", QStringLiteral("http://spambri.de/quadkonsole4"), QStringLiteral("quadkonsole4@spambri.de"));
	about.addAuthor(i18n("Simon Perreault"), "", QStringLiteral("nomis80@nomis80.org"));
	about.addAuthor(i18n("Karsten Borgwaldt"), "", QStringLiteral("kb@spambri.de"));
	about.addCredit(i18n("Michael Feige"), i18n("Many ideas and feature requests"));
	KAboutData::setApplicationData(about);

	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addVersionOption();

	parser.addOption(QCommandLineOption(QStringList() << "r" << "rows", i18n("Number of rows of terminal emulators"), i18n("rows"), QString::number(2)));
	parser.addOption(QCommandLineOption(QStringList() << "c" << "columns", i18n("Number of columns of terminal emulators"), i18n("columns"), QString::number(2)));
	parser.addOption(QCommandLineOption(QStringList() << "C" << "cmd" << i18n("Run command [in given view] (may be used multiple times)")));
	parser.addOption(QCommandLineOption(QStringList() << "u" << "url" << i18n("Open URL [in given view] (may be used multiple times)")));
	parser.addPositionalArgument(i18n("URL"), i18n("Open specified URL in a running instance of QuadKonsole4"));
	about.setupCommandLine(&parser);
	parser.process(app);

	about.processCommandLine(&parser);

	KDBusService service(KDBusService::Unique);
	QKApplication qkapp;
	qkapp.exec();

	return app.exec();
}
