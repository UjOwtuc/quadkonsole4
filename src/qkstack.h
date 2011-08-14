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

#ifndef QKSTACK_H
#define QKSTACK_H

#include <KDE/KUrl>

#include <QtGui/QStackedWidget>

class QKView;
namespace KParts
{
	class ReadOnlyPart;
	class PartManager;
}

class QKStack : public QStackedWidget
{
	Q_OBJECT
	public:
		explicit QKStack(KParts::PartManager& partManager, QWidget* parent = 0);
		explicit QKStack(KParts::PartManager& partManager, KParts::ReadOnlyPart* part, QWidget* parent = 0);
		virtual ~QKStack();

		bool hasFocus() const;
		void setFocus();
		QString foregroundProcess() const;
		KParts::ReadOnlyPart* part();
		void partDestroyed();

	signals:
		void partCreated();
		void setStatusBarText(QString);
		void setWindowCaption(QString);

	public slots:
		void sendInput(const QString& text);
		void switchView(KUrl url=KUrl());

	private slots:
		void slotPartCreated();
		void slotSetStatusBarText(QString text);
		void slotSetWindowCaption(QString text);

	private:
		void setupUi(KParts::PartManager& partManager, KParts::ReadOnlyPart* part=0);
};

#endif // QKSTACK_H
