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

#include <QtGui/QTreeWidgetItem>

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
class QKRInstance;

class QKRMainWindow : public QObject, public QTreeWidgetItem
{
	Q_OBJECT
	public:
		QKRMainWindow(QKRInstance* parent, const QString& dbusName, const QString& name);
		virtual ~QKRMainWindow();

	public slots:
		void update();
		void sendInput(const QString& text);

	private:
		QString m_dbusName;
		QString m_name;
		uint m_numViews;
		QList<QTreeWidgetItem*> m_views;
};

class QKRInstance : public QObject, public QTreeWidgetItem
{
	Q_OBJECT
	public:
		QKRInstance(QTreeWidget* parent, const QString& dbusName);
		virtual ~QKRInstance();

	public slots:
		void update();
		void sendInput(const QString& text);

	private:
		QString m_dbusName;
		QMap<QString, QKRMainWindow*> m_mainWindows;
};

class QKRemotePart : public KParts::ReadOnlyPart
{
	Q_OBJECT
	public:
		static const char version[];
		static const char partName[];

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
		QWidget* m_widget;
		Ui::qkremoteWidget* m_remote;
		QDBusConnection* m_dbusConn;
		QKREventFilter* m_eventFilter;
		QTimer* m_updateTimer;
		QMap<QString, QKRInstance*> m_instances;
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
