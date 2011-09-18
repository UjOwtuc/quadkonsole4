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

#include "qkremote.h"
#include "version.h"

#include <KDE/KApplication>
#include <KDE/KAboutData>
#include <KDE/KCmdLineArgs>
#include <KDE/KLocale>

#ifndef QUADKONSOLE4_VERSION
#error QUADKONSOLE4_VERSION undefined
#endif

static const char description[] = I18N_NOOP("Controls a running instance of QuadKonsole4");
static const char version[] = QUADKONSOLE4_VERSION;

int main(int argc, char **argv)
{
	KAboutData about("qkremote", "quadkonsole4", ki18n("qkremote"), version, ki18n(description), KAboutData::License_GPL, ki18n("(C) 2009 - 2011 Karsten Borgwaldt"), KLocalizedString(), "http://kb.ccchl.de/quadkonsole4", "quadkonsole4@kb.ccchl.de");
	about.addAuthor(ki18n("Karsten Borgwaldt"), KLocalizedString(), "kb@kb.ccchl.de");
	KCmdLineArgs::init(argc, argv, &about);

// 	KCmdLineOptions options;
// 	KCmdLineArgs::addCmdLineOptions(options);
	KApplication app;

	if (app.isSessionRestored())
	{
		kRestoreMainWindows<QKRemote>();
	}
	else
	{
// 		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		QKRemote* widget = new QKRemote;
		widget->show();
// 		args->clear();
	}

	return app.exec();
}
