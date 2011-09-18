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

#ifndef QKREMOTEPART_H
#define QKREMOTEPART_H

#include <KDE/KParts/ReadOnlyPart>

namespace Ui
{
	class qkremoteWidget;
}
class QWidget;
class KAboutData;
class QDBusConnection;
class QTreeWidgetItem;
class QKeyEvent;
class QKREventFilter;
class QHBoxLayout;
class QLabel;

class QKRemotePart : public KParts::ReadOnlyPart
{
	Q_OBJECT
	public:
		static const char version[];
		static const char partName[];
		static const QString destinationBase;
		static const char interfaceName[];
		static const char introspectInterface[];
		static const char propertiesInterface[];

		QKRemotePart(QWidget *parentWidget,QObject *parent, const QStringList &);
		virtual ~QKRemotePart();

		static KAboutData *createAboutData();

	protected:
		/**
		* This must be implemented by each part
		*/
		virtual bool openFile();

	public slots:
		void refreshAvailableSlaves();
		void slotKeypress(QKeyEvent* event);
		void sendInput(const QString& text);
		void focusInputLine();
		void identifyViews();

	private:
		void addSlave(const QString& dbusName);
		void addMainWindow(const QString& dbusName, QTreeWidgetItem* parent, QString name);

		QWidget* m_widget;
		Ui::qkremoteWidget* m_remote;
		QDBusConnection* m_dbusConn;
		QKREventFilter* m_eventFilter;
};


class QKREventFilter : public QObject
{
	Q_OBJECT
	public:
		explicit QKREventFilter(Ui::qkremoteWidget* settingsWidget, QObject* parent = 0);
		virtual ~QKREventFilter();

		bool eventFilter(QObject* o, QEvent* e);

	signals:
		void keypress(QKeyEvent*);

	private:
		Ui::qkremoteWidget* m_settingsWidget;
};

#endif // QKREMOTEPART_H
