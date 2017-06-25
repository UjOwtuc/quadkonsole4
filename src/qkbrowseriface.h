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

#ifndef QKBROWSERIFACE_H
#define QKBROWSERIFACE_H

#include "qkhistory.h"

#include <KDE/KParts/BrowserInterface>

class QKBrowserInterface : public KParts::BrowserInterface
{
	Q_OBJECT
	Q_PROPERTY(uint historyLength READ historyLength)
	public:
		explicit QKBrowserInterface(QKHistory& parent);
		uint historyLength() const;

	public slots:
		void goHistory(int steps);

	private:
		const QKHistory& m_history;
};

#endif // QKBROWSERIFACE_H
