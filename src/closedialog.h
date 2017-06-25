/**************************************************************************
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

#ifndef CLOSEDIALOG_H
#define CLOSEDIALOG_H

#include <KDE/KDialog>

class QKStack;
class QKView;
namespace Ui
{
	class DetachProcesses;
}

class CloseDialog : public KDialog
{
	Q_OBJECT
	public:
		explicit CloseDialog(QWidget* parent = 0, Qt::WindowFlags f = 0);
		virtual ~CloseDialog();

		void addView(QKStack* stack, QKView* view);
		void addViews(QKStack* stack, QList<QKView*> views);
		virtual bool exec(QMap<QKView*, QKStack*>& detach);
		int size() const;

	private slots:
		void dontDetach();
		void detach();

	private:
		Ui::DetachProcesses* m_widget;
		QMap<QKView*, QKStack*> m_viewStackMap;
		QList<QKView*> m_result;
};

#endif // CLOSEDIALOG_H
