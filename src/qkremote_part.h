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

#ifndef QKREMOTEPART_H
#define QKREMOTEPART_H

#include <KParts/ReadOnlyPart>

#include <QtWidgets/QTreeWidgetItem>
#include <QtCore/QThread>

// dbus interface
class DeSpambriQuadkonsole4QuadKonsoleInterface;
namespace Ui
{
	class qkremoteWidget;
}
class K4AboutData;
class QDBusConnection;
class QKeyEvent;


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


class QKRemotePart : public KParts::ReadOnlyPart
{
	Q_OBJECT
	public:
		static const char version[];
		static const char partName[];
		static const QString dbusInterfaceName;

		QKRemotePart(QWidget* parentWidget,QObject* parent, const QStringList &);
		virtual ~QKRemotePart();

		static K4AboutData *createAboutData();

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

	private slots:
		void slotToggleUpdateTimer(bool state);

	private:
		void addSlave(const QString& instance, const QString& window, uint numViews, DeSpambriQuadkonsole4QuadKonsoleInterface& dbusInterface);

		QWidget* m_widget;
		Ui::qkremoteWidget* m_remote;
		QDBusConnection* m_dbusConn;
		QKREventFilter* m_eventFilter;
		QTimer* m_updateTimer;
		QThread* m_dbusThread;
};


#endif // QKREMOTEPART_H
