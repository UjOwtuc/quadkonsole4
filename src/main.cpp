/***************************************************************************
 *   Copyright (C) 2005 by Simon Perreault   *
 *   nomis80@nomis80.org   *
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


#include "quadkonsole.h"

#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>

static const char description[] =
    I18N_NOOP("Embeds multiple Konsoles in a grid layout");

static const char version[] = "2.0";

static KCmdLineOptions options[] =
{
    { "rows <rows>",
        I18N_NOOP( "Number of rows of terminal emulators" ),    "2" },
    { "columns <columns>",
        I18N_NOOP( "Number of columns of terminal emulators" ), "2" },
    { "clickfocus",
        I18N_NOOP( "Click to focus instead of focus follows mouse" ), 0 },
    { "cmd <command>",
        I18N_NOOP( "Run command (may be used multiple times)" ), 0 },
    KCmdLineLastOption
};

int main(int argc, char **argv)
{
    KAboutData about("quadkonsole", I18N_NOOP("QuadKonsole"), version,
            description, KAboutData::License_GPL, "(C) 2005 Simon Perreault", 0,
            0, "nomis80@nomis80.org");
    about.addAuthor( "Simon Perreault", 0, "nomis80@nomis80.org" );
    KCmdLineArgs::init(argc, argv, &about);
    KCmdLineArgs::addCmdLineOptions( options );
    KApplication app;

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    int rows = args->getOption("rows").toInt();
    int columns = args->getOption("columns").toInt();
    bool clickfocus = args->isSet("clickfocus");
    QCStringList cmds = args->getOptionList("cmd");

    QuadKonsole* mainWin = new QuadKonsole( rows, columns, clickfocus, cmds );
    app.setMainWidget( mainWin );
    mainWin->showMaximized();

    args->clear();

    // mainWin has WDestructiveClose flag by default, so it will delete itself.
    return app.exec();
}

