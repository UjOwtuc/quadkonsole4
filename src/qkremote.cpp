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

#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KLibLoader>
#include <KDE/KMessageBox>
#include <KDE/KStandardAction>
#include <KDE/KStatusBar>
#include <KDE/KLocale>

#include <QtGui/QApplication>

QKRemote::QKRemote()
	: KParts::MainWindow()
{
	setupActions();

	KLibFactory *factory = KLibLoader::self()->factory("qkremotepart");
	if (factory)
	{
		m_part = static_cast<KParts::ReadOnlyPart*>(factory->create(this));
		if (m_part)
		{
			setCentralWidget(m_part->widget());
			setupGUI(Default, "qkremote_shell.rc");
		}
	}
	else
	{
		KMessageBox::error(this, i18n("Could not create a factory for %1.", QString("qkremotepart")));
		deleteLater();
		return;
	}
	setWindowIcon(KIcon("qkremote"));
	setAutoSaveSettings();
}


QKRemote::~QKRemote()
{}


void QKRemote::setupActions()
{
	KStandardAction::quit(qApp, SLOT(closeAllWindows()), actionCollection());

	createStandardStatusBarAction();
	setStandardToolBarMenuEnabled(true);
}

#include "qkremote.moc"