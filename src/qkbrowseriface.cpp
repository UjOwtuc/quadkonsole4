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

#include "qkbrowseriface.h"
#include "qkhistory.h"

#include <KDE/KDebug>
#include <KDE/KParts/BrowserInterface>


QKBrowserInterface::QKBrowserInterface(QKHistory& parent)
	: BrowserInterface(&parent),
	m_history(parent)
{}


uint QKBrowserInterface::historyLength() const
{
	return m_history.count();
}


void QKBrowserInterface::goHistory(int steps)
{
	kDebug() << "don't know how to _change_ anything" << endl;
}
