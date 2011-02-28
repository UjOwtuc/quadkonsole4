/***************************************************************************
 *   Copyright (C) 2009 by Karsten Borgwaldt                               *
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
#include <kde_terminal_interface.h>

#include <QtGui/QSplitter>

#include <iostream>

namespace
{
	KLibFactory *factory = 0;
}


Konsole::Konsole ( QWidget *parent, QSplitter *layout, int row, int column )
	: QWidget(parent),
	m_layout(layout),
	m_row(row),
	m_column(column)
{
	m_part = 0;
	createPart();
}


Konsole::~Konsole ( void )
{
	if (m_part)
		delete m_part;
}



void Konsole::sendInput(const QString& text)
{
	TerminalInterface *t = qobject_cast< TerminalInterface* >(m_part);
	if (t)
		t->sendInput(text);
}


void Konsole::partDestroyed ( void )
{
	if (m_part)
		disconnect(m_part, SIGNAL(destroyed()), this, SLOT(partDestroyed()));
// 	emit destroyed();
	createPart();
	m_part->widget()->setFocus();
}


void Konsole::createPart ( void )
{
	if (factory == 0)
	{
		factory = KPluginLoader("libkonsolepart").factory();
	}
	m_part = dynamic_cast<KParts::ReadOnlyPart*>(factory->create<QObject>(this, this));
	connect(m_part, SIGNAL(destroyed()), SLOT(partDestroyed()));
	TerminalInterface* t = qobject_cast<TerminalInterface*>(m_part);
	if (t)
		t->showShellInDir(QString());
	else
	{
		kdError() << "could not get TerminalInterface" << endl;
		exit(1);
	}

	m_part->widget()->setParent(this);
	m_layout->insertWidget(m_column, m_part->widget());

	int width = m_layout->geometry().width() / m_layout->count();
	QList<int> sizes;
	for (int i=0; i<m_layout->count(); ++i)
		sizes.append(width);
	m_layout->setSizes(sizes);
}

