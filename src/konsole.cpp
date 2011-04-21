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

#include "konsole.h"

#include <KDE/KParts/ReadOnlyPart>
#include <KDE/KLibLoader>
#include <KDE/KDebug>
#include <kde_terminal_interface_v2.h>

#include <QtGui/QSplitter>

#include <iostream>

namespace
{
	KLibFactory *factory = 0;
}


Konsole::Konsole(QWidget *parent, QSplitter *layout, int column)
	: QWidget(parent),
	m_layout(layout),
	m_column(column)
{
	m_part = 0;
	createPart();
}


Konsole::Konsole(QWidget *parent, KParts::ReadOnlyPart* part)
	: QWidget(parent),
	m_layout(0),
	m_column(0)
{
	m_part = part;
	part->setParent(this);
	m_part->widget()->setParent(this);
	connect(m_part, SIGNAL(destroyed()), SLOT(partDestroyed()));
}


Konsole::~Konsole()
{
	if (m_part)
	{
		disconnect(m_part, SIGNAL(destroyed()), this, SLOT(partDestroyed()));
		delete m_part;
	}
}


void Konsole::sendInput(const QString& text)
{
	TerminalInterfaceV2 *t = qobject_cast< TerminalInterfaceV2* >(m_part);
	if (t)
		t->sendInput(text);
	else
		kDebug() << "Part is no TerminalInterfaceV2" << endl;
}


void Konsole::setLayout(QSplitter* layout, int column)
{
	if (m_layout)
	{
		kDebug() << "changing layout is not supported yet" << endl;
		return;
	}
	m_column = column;
	m_layout = layout;
	m_layout->insertWidget(column, m_part->widget());
}


QString Konsole::foregroundProcessName()
{
	TerminalInterfaceV2* t = qobject_cast< TerminalInterfaceV2* >(m_part);
	if (t)
		return t->foregroundProcessName();
	else
		kDebug() << "Part is no TerminalInterfaceV2" << endl;

	return "";
}


void Konsole::partDestroyed()
{
	if (m_part)
		disconnect(m_part, SIGNAL(destroyed()), this, SLOT(partDestroyed()));

	createPart();
	m_part->widget()->setFocus();
}


void Konsole::createPart()
{
	// copied parts could have no layout for some nanoseconds
	if (m_layout == 0)
	{
		kDebug() << "no layout, cannot create a new part" << endl;
		return;
	}

	if (factory == 0)
	{
		factory = KPluginLoader("libkonsolepart").factory();
	}
	m_part = dynamic_cast<KParts::ReadOnlyPart*>(factory->create<QObject>(this, this));
	connect(m_part, SIGNAL(destroyed()), SLOT(partDestroyed()));
	TerminalInterfaceV2* t = qobject_cast<TerminalInterfaceV2*>(m_part);
	if (t)
		t->showShellInDir(QString());
	else
	{
		kdError() << "could not get TerminalInterface" << endl;
		exit(1);
	}

	m_part->widget()->setParent(this);
	m_layout->insertWidget(m_column, m_part->widget());

	emit partCreated();
}


QWidget* Konsole::widget()
{
	if (m_part)
		return m_part->widget();

	return 0;
}
