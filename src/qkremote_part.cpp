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

#include "qkremote_part.h"
#include "version.h"

#include <KDE/KLocale>
#include <KDE/KComponentData>
#include <KDE/KParts/GenericFactory>

#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>

#include "ui_qkremotewidget.h"

#include "quadkonsoleinterface.h"

#ifndef QUADKONSOLE4_VERSION
#error QUADKONSOLE4_VERSION undefined
#endif

typedef KParts::GenericFactory<QKRemotePart> QKRemotePartFactory;
K_EXPORT_COMPONENT_FACTORY(qkremotepart, QKRemotePartFactory)


namespace
{

}

const char QKRemotePart::version[] = QUADKONSOLE4_VERSION;
const char QKRemotePart::partName[] = "qkremotepart";
const QString QKRemotePart::dbusInterfaceName = "de.ccchl.quadkonsole4-%1";


QKRemotePart::QKRemotePart( QWidget *parentWidget, QObject *parent, const QStringList & /*args*/ )
	: KParts::ReadOnlyPart(parent)
{
	// we need an instance
	setComponentData(QKRemotePartFactory::componentData());

	m_widget = new QWidget(parentWidget);
	m_remote = new Ui::qkremoteWidget;
	m_remote->setupUi(m_widget);
	m_widget->setFocusProxy(m_remote->inputLine);
	setWidget(m_widget);

	m_eventFilter = new QKREventFilter(m_remote, this);

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
	connect(m_eventFilter, SIGNAL(keypress(QKeyEvent*)), SLOT(slotKeypress(QKeyEvent*)));
	connect(m_remote->autoUpdate, SIGNAL(toggled(bool)), SLOT(slotToggleUpdateTimer(bool)));

	setXMLFile("qkremote_part.rc");

	QStringList headerLabels;
	headerLabels << i18n("Available slaves") << i18n("Current URL");
	m_remote->availableSlaves->setHeaderLabels(headerLabels);

	qApp->installEventFilter(m_eventFilter);
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
	KAboutData *aboutData = new KAboutData(partName, "quadkonsole4", ki18n("qkremotepart"), version);
	aboutData->addAuthor(ki18n("Karsten Borgwaldt"), KLocalizedString(), "kb@kb.ccchl.de");
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

	QTreeWidgetItemIterator it(m_remote->availableSlaves, QTreeWidgetItemIterator::Selected);
	while (*it)
	{
		if ((*it)->data(1, Qt::UserRole).canConvert<QString>())
		{
			QString dbusName = (*it)->data(0, Qt::UserRole).toString();
			QString window = (*it)->data(1, Qt::UserRole).toString();
			uint view = (*it)->data(2, Qt::UserRole).toUInt();

			de::ccchl::quadkonsole4::QuadKonsole qk(dbusName, window, QDBusConnection::sessionBus());
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
				de::ccchl::quadkonsole4::QuadKonsole qk(dbusName, window, QDBusConnection::sessionBus());
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

	QStringList services = m_dbusConn->interface()->registeredServiceNames();
	services = services.filter(dbusInterfaceName.arg(""));

	QStringListIterator it(services);
	while (it.hasNext())
	{
		QString instance = it.next();
		for (int win=1; win<10; ++win)
		{
			QString window = QString("/quadkonsole4/MainWindow_%1").arg(win);

			de::ccchl::quadkonsole4::QuadKonsole qk(instance, window, *m_dbusConn);
			uint numViews = qk.numViews();

			if (numViews > 0)
				addSlave(instance, window, numViews, qk);
		}
	}

	QList<int> toRemove;
	for (int i=0; i<m_remote->availableSlaves->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = m_remote->availableSlaves->topLevelItem(i);
		if (item && ! services.contains(item->text(0)))
			toRemove.append(i);
	}
	while (toRemove.size())
	{
		delete m_remote->availableSlaves->takeTopLevelItem(toRemove.front());
		toRemove.removeFirst();
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


void QKRemotePart::addSlave(const QString& instance, const QString& window, uint numViews, de::ccchl::quadkonsole4::QuadKonsole& dbusInterface)
{
	QTreeWidgetItem* instanceItem = 0;
	QList<QTreeWidgetItem*> matches = m_remote->availableSlaves->findItems(instance, Qt::MatchExactly, 0);
	if (matches.count())
		instanceItem = matches.front();
	else
	{
		instanceItem = new QTreeWidgetItem(m_remote->availableSlaves, QStringList(instance));
		instanceItem->setData(0, Qt::UserRole, instance);
		m_remote->availableSlaves->expandItem(instanceItem);
	}

	if (instance == dbusInterfaceName.arg(getpid()))
		instanceItem->setText(1, ki18n("(this process)").toString());
	else
		instanceItem->setText(1, QString());

	QTreeWidgetItem* windowItem = 0;
	for (int i=0; i<instanceItem->childCount(); ++i)
	{
		QTreeWidgetItem* w = instanceItem->child(i);
		if (w && w->text(0) == window)
		{
			windowItem = w;
			break;
		}
	}
	if (! windowItem)
	{
		windowItem = new QTreeWidgetItem(instanceItem, QStringList(window));
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

	QStringList urls = dbusInterface.urls();
	QStringList icons = dbusInterface.partIcons();
	for (int i=0; i<windowItem->childCount(); ++i)
	{
		QTreeWidgetItem* item = windowItem->child(i);
		int viewNumber = item->data(2, Qt::UserRole).toUInt();
		if (urls.count() > viewNumber)
			item->setText(1, urls.at(viewNumber));
		if (icons.count() > viewNumber)
			item->setIcon(0, KIcon(icons.at(viewNumber)));
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
