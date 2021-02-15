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

#include "qkremote_part.h"
#include "version.h"

#include <qglobal.h>
#include <QTimer>
#include <QApplication>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <KPluginFactory>
#include <KAboutData>
#include <KLocalizedString>

#include "ui_qkremotewidget.h"

#include "qkapplicationinterface.h"
#include "quadkonsoleinterface.h"

#ifndef QUADKONSOLE4_VERSION
#error QUADKONSOLE4_VERSION undefined
#endif

K_PLUGIN_FACTORY(QKRemotePartFactory, registerPlugin<QKRemotePart>();)
K_EXPORT_PLUGIN(QKRemotePartFactory("qkremote"))

const char QKRemotePart::version[] = QUADKONSOLE4_VERSION;
const char QKRemotePart::partName[] = "qkremotepart";


QKRemotePart::QKRemotePart(QWidget *parentWidget, QObject *parent, const QVariantList & /*args*/)
	: KParts::ReadOnlyPart(parent),
	m_dbusThread(new QThread(this))
{
	// we need an instance
// 	setComponentData(QKRemotePartFactory::componentData());

	m_widget = new QWidget(parentWidget);
	m_remote = new Ui::qkremoteWidget;
	m_remote->setupUi(m_widget);
	m_widget->setFocusProxy(m_remote->inputLine);
	setWidget(m_widget);

// 	m_eventFilter = new QKREventFilter(m_remote, this);

	m_dbusConn = new QDBusConnection(QDBusConnection::connectToBus(QDBusConnection::SessionBus, partName));
	m_dbusConn->registerService(partName);

	m_updateTimer = new QTimer(this);
	if (m_remote->autoUpdate->isChecked())
		m_updateTimer->start(m_remote->updateInterval->value() * 1000);

	connect(m_updateTimer, SIGNAL(timeout()), SLOT(refreshAvailableSlaves()));
	connect(m_remote->refreshButton, SIGNAL(clicked(bool)), SLOT(refreshAvailableSlaves()));
	connect(m_remote->identifyButton, SIGNAL(clicked(bool)), SLOT(identifyViews()));
	connect(m_remote->availableSlaves, SIGNAL(itemActivated(QTreeWidgetItem*,int)), SLOT(focusInputLine()));
	connect(m_remote->inputLine, SIGNAL(textEdited(QString)), SLOT(sendInput(QString)));
// 	connect(m_eventFilter, SIGNAL(keypress(QKeyEvent*)), SLOT(slotKeypress(QKeyEvent*)));
	connect(m_remote->autoUpdate, SIGNAL(toggled(bool)), SLOT(slotToggleUpdateTimer(bool)));

	setXMLFile("qkremote_part.rc");

	QStringList headerLabels;
	headerLabels << i18n("Available slaves") << i18n("Current URL");
	m_remote->availableSlaves->setHeaderLabels(headerLabels);
	m_remote->availableSlaves->sortByColumn(0, Qt::AscendingOrder);

	m_remote->inputLine->setPlaceholderText(i18n("Type or paste here to broadcast"));
// 	m_remote->inputLine->installEventFilter(m_eventFilter);

	// initial update of running quadkonsole4 instances
	QTimer::singleShot(1, this, SLOT(refreshAvailableSlaves()));

// 	qApp->installEventFilter(m_eventFilter);
}


QKRemotePart::~QKRemotePart()
{
	delete m_eventFilter;
}


KAboutData *QKRemotePart::createAboutData()
{
	// the non-i18n name here must be the same as the directory in
	// which the part's rc file is installed ('partrcdir' in the
	// Makefile)
	KAboutData *aboutData = new KAboutData(partName, i18n("QKRemote"), version);
	aboutData->addAuthor(i18n("Karsten Borgwaldt"), "", QStringLiteral("kb@spambri.de"));
	return aboutData;
}


bool QKRemotePart::openFile()
{
	return false;
}


void QKRemotePart::slotKeypress(QKeyEvent* event)
{
	if (event->type() != QEvent::KeyPress || event->text().isEmpty())
		return;

	sendInput(event->text());
}


void QKRemotePart::sendInput(const QString& text)
{
	m_remote->inputLine->clear();
	setStatusBarText("");

	qDebug() << "sendInput:" << text;
	QTreeWidgetItemIterator it(m_remote->availableSlaves, QTreeWidgetItemIterator::Selected);
	while (*it)
	{
		if ((*it)->data(1, Qt::UserRole).canConvert<QString>())
		{
			QString dbusName = (*it)->data(0, Qt::UserRole).toString();
			QString window = (*it)->data(1, Qt::UserRole).toString();
			uint view = (*it)->data(2, Qt::UserRole).toUInt();

			de::spambri::quadkonsole4::QuadKonsole qk(dbusName, window, QDBusConnection::sessionBus());
			qk.sendInput(view, text);
		}
		++it;
	}
}


void QKRemotePart::focusInputLine()
{
	m_remote->inputLine->setFocus();
}


void QKRemotePart::identifyViews()
{
	m_remote->identifyButton->setEnabled(false);

	QTreeWidgetItemIterator::IteratorFlags flags = QTreeWidgetItemIterator::All;
	if (m_remote->availableSlaves->selectedItems().length())
		flags = QTreeWidgetItemIterator::Selected;

	QTreeWidgetItemIterator it(m_remote->availableSlaves);
	QStringList sent;
	while (*it)
	{
		if ((*it)->data(1, Qt::UserRole).canConvert<QString>())
		{
			QString dbusName = (*it)->data(0, Qt::UserRole).toString();
			QString window = (*it)->data(1, Qt::UserRole).toString();
			QString format = "%1 (" + window + ")";
			if (! sent.contains(dbusName + window))
			{
				de::spambri::quadkonsole4::QuadKonsole qk(dbusName, window, QDBusConnection::sessionBus());
				qk.identifyStacks(format);
				sent << dbusName + window;
			}
		}
		++it;
	}
	m_remote->identifyButton->setEnabled(true);
}


void QKRemotePart::refreshAvailableSlaves()
{
	m_remote->refreshButton->setEnabled(false);
	m_updateTimer->stop();

	QString instance = "de.spambri.quadkonsole4";
	de::spambri::quadkonsole4::QKApplication qkApp(instance, "/QKApplication", *m_dbusConn);
	qkApp.moveToThread(m_dbusThread);
	uint numWindows = QDBusReply<uint>(qkApp.call(QDBus::BlockWithGui, "windowCount"));
	uint found = 0;

	for (int win=1; found<numWindows && win<100; ++win)
	{
		QString window = QString("/quadkonsole4/MainWindow_%1").arg(win);

		de::spambri::quadkonsole4::QuadKonsole qk(instance, window, *m_dbusConn);
		qk.moveToThread(m_dbusThread);
		uint numViews = QDBusReply<uint>(qk.call(QDBus::BlockWithGui, "numViews")).value();
		if (numViews > 0)
		{
			addSlave(instance, window, numViews, qk);
			++found;
		}
	}

	m_remote->availableSlaves->resizeColumnToContents(0);

	if (m_remote->autoUpdate->isChecked())
		m_updateTimer->start(m_remote->updateInterval->value() * 1000);
	m_remote->refreshButton->setEnabled(true);
}


void QKRemotePart::slotToggleUpdateTimer(bool state)
{
	if (state)
		m_updateTimer->start(m_remote->updateInterval->value() * 1000);
	else
		m_updateTimer->stop();
}


void QKRemotePart::addSlave(const QString& instance, const QString& window, uint numViews, de::spambri::quadkonsole4::QuadKonsole& dbusInterface)
{
	QTreeWidgetItem* windowItem = 0;
	QList<QTreeWidgetItem*> matches = m_remote->availableSlaves->findItems(window, Qt::MatchExactly, 0);
	if (matches.size())
		windowItem = matches.front();
	else
	{
		windowItem = new QTreeWidgetItem(m_remote->availableSlaves, QStringList(window));
		windowItem->setIcon(0, QIcon("quadkonsole4"));
		windowItem->setData(0, Qt::UserRole, instance);
		windowItem->setData(1, Qt::UserRole, window);
		m_remote->availableSlaves->expandItem(windowItem);
	}

	if (windowItem->childCount() > static_cast<int>(numViews))
	{
		// at least one view was removed and there is no way telling which one
		// so, we remove all views from the tree widget an re-add them
		// this clears the selection of those items and forces to update the numbers
		while (windowItem->childCount())
			delete windowItem->takeChild(0);
	}

	for (int i=windowItem->childCount(); i<static_cast<int>(numViews); ++i)
	{
		QTreeWidgetItem* view = new QTreeWidgetItem(windowItem, QStringList(QString::number(i)));

		view->setData(0, Qt::UserRole, instance);
		view->setData(1, Qt::UserRole, window);
		view->setData(2, Qt::UserRole, i);
	}

	QStringList urls = QDBusReply<QStringList>(dbusInterface.call(QDBus::BlockWithGui, "urls")).value();
	QStringList icons = QDBusReply<QStringList>(dbusInterface.call(QDBus::BlockWithGui, "partIcons")).value();
	for (int i=0; i<windowItem->childCount(); ++i)
	{
		QTreeWidgetItem* item = windowItem->child(i);
		int viewNumber = item->data(2, Qt::UserRole).toUInt();
		if (urls.count() > viewNumber)
			item->setText(1, urls.at(viewNumber));
		if (icons.count() > viewNumber)
			item->setIcon(0, QIcon(icons.at(viewNumber)));
	}
}


QKREventFilter::QKREventFilter(Ui::qkremoteWidget* settingsWidget, QObject* parent)
	: QObject(parent),
	m_settingsWidget(settingsWidget)
{}


QKREventFilter::~QKREventFilter()
{}


bool QKREventFilter::eventFilter(QObject*, QEvent* e)
{
	if (m_settingsWidget->inputLine->hasFocus() && (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease))
	{
		QKeyEvent* ke = static_cast<QKeyEvent*>(e);
		if (ke->text().length())
		{
			emit keypress(ke);
			return true;
		}
	}
	return false;
}

#include "qkremote_part.moc"
