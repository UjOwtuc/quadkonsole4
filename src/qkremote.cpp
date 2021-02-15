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

#include "qkremote.h"

#include <KActionCollection>
#include <KMessageBox>
#include <KStandardAction>
#include <KLocalizedString>
#include <KService>

#include <QApplication>

const QString partname = QStringLiteral("qkremote_part");


QKRemote::QKRemote()
	: KParts::MainWindow()
{
	setupActions();

	KService::Ptr sp = KService::serviceByDesktopName(partname);
	if (sp)
	{
		m_part = sp->createInstance<KParts::ReadOnlyPart>(nullptr, QVariantList(), nullptr);
		if (m_part)
		{
			setCentralWidget(m_part->widget());
			setupGUI(Default, "qkremote_shell.rc");
		}
	}
	else
	{
		KMessageBox::error(this, i18n("Could not create a factory for %1.", partname));
		deleteLater();
		return;
	}
	setWindowIcon(QIcon("qkremote"));
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
