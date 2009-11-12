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


#include "mousemovefilter.h"

#include <qwidget.h>


MouseMoveFilter::MouseMoveFilter( QObject* parent )
    : QObject(parent)
{
}

bool MouseMoveFilter::eventFilter( QObject* o, QEvent* e )
{
    if ( e->type() == QEvent::MouseMove ) {
        QWidget* w = dynamic_cast<QWidget*>(o);
        w->setFocus();
    }
    return false;
}

#include "mousemovefilter.moc"
