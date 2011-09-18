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

#include <KDE/KComponentData>
#include <KDE/KParts/GenericFactory>

#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>

#include "ui_qkremotewidget.h"

#ifndef QUADKONSOLE4_VERSION
#error QUADKONSOLE4_VERSION undefined
#endif

typedef KParts::GenericFactory<QKRemotePart> QKRemotePartFactory;
K_EXPORT_COMPONENT_FACTORY(qkremotepart, QKRemotePartFactory)


const char QKRemotePart::version[] = QUADKONSOLE4_VERSION;
const char QKRemotePart::partName[] = "qkremotepart";
const QString QKRemotePart::destinationBase = "de.ccchl.quadkonsole4-%1";
const char QKRemotePart::interfaceName[] = "de.ccchl.quadkonsole4.QuadKonsole";
const char QKRemotePart::introspectInterface[] = "org.freedesktop.DBus.Introspectable";
const char QKRemotePart::propertiesInterface[] = "org.freedesktop.DBus.Properties";


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

	connect(m_remote->refreshButton, SIGNAL(clicked(bool)), SLOT(refreshAvailableSlaves()));
	connect(m_remote->identifyButton, SIGNAL(clicked(bool)), SLOT(identifyViews()));
	connect(m_remote->availableSlaves, SIGNAL(itemActivated(QTreeWidgetItem*,int)), SLOT(focusInputLine()));
	connect(m_remote->inputLine, SIGNAL(textEdited(QString)), SLOT(sendInput(QString)));
	connect(m_eventFilter, SIGNAL(keypress(QKeyEvent*)), SLOT(slotKeypress(QKeyEvent*)));

	setXMLFile("qkremote_part.rc");

	m_remote->availableSlaves->setHeaderLabel(i18n("Available slaves"));
	refreshAvailableSlaves();

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

	QMap< QPair<QString, QString>, QList<uint> > targets;

	QList<QTreeWidgetItem*> selected = m_remote->availableSlaves->selectedItems();
	QListIterator<QTreeWidgetItem*> it(selected);
	while (it.hasNext())
	{
		bool viewSelected = false;
		QTreeWidgetItem* item = it.next();

		QString dbusName = item->data(0, Qt::UserRole).toString();
		QString window = item->data(1, Qt::UserRole).toString();
		uint view = item->data(2, Qt::UserRole).toUInt(&viewSelected);

		if (window.isEmpty())
			setStatusBarText(i18n("No window selected for %1", dbusName));
		else if (!viewSelected)
			setStatusBarText(i18n("No view selected for %1 %2", dbusName, window));
		else
			targets[QPair<QString, QString>(dbusName, window)].append(view);
	}

	QMapIterator< QPair<QString, QString>, QList<uint> > targetIt(targets);
	while (targetIt.hasNext())
	{
		QPair<QString, QString> dest = targetIt.next().key();
		QDBusInterface iface(dest.first, QString("/quadkonsole4/")+dest.second, interfaceName);
		QListIterator<uint> viewIt(targetIt.value());
		while (viewIt.hasNext())
			iface.call(QDBus::BlockWithGui, "sendInput", viewIt.next(), text);
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
				QDBusInterface iface(dbusName, QString("/quadkonsole4/")+window, interfaceName);
				iface.call(QDBus::BlockWithGui, "identifyStacks", format);
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
	m_remote->availableSlaves->clear();

	QStringList services = m_dbusConn->interface()->registeredServiceNames();
	services = services.filter(destinationBase.arg(""));

	QStringListIterator it(services);
	while (it.hasNext())
	{
		QString dest = it.next();
		addSlave(dest);
	}
	m_remote->refreshButton->setEnabled(true);
	m_remote->availableSlaves->expandAll();
}


void QKRemotePart::addSlave(const QString& dbusName)
{
	QDBusInterface iface(dbusName, "/quadkonsole4", introspectInterface);
	QDBusReply<QString> reply = iface.call(QDBus::BlockWithGui, "Introspect");
	if (reply.isValid())
	{
		QString label(dbusName);
		if (dbusName == destinationBase.arg(getpid()))
			label += " (this process)";

		QTreeWidgetItem* item = new QTreeWidgetItem(m_remote->availableSlaves, QStringList(label));
		item->setData(0, Qt::UserRole, dbusName);
		m_remote->availableSlaves->addTopLevelItem(item);

		QDomDocument doc;
		doc.setContent(reply.value());
		QDomElement root = doc.documentElement();
		QDomNodeList nodes = root.elementsByTagName("node");

		for (uint i=0; i<nodes.length(); ++i)
		{
			QDomNamedNodeMap attrs = nodes.item(i).attributes();
			QDomNode name = attrs.namedItem("name");
			addMainWindow(dbusName, item, name.nodeValue());
		}
	}
	else
		kDebug() << "invalid reply to Introspect from" << dbusName << endl;
}


void QKRemotePart::addMainWindow(const QString& dbusName, QTreeWidgetItem* parent, QString name)
{
	QTreeWidgetItem* iface = new QTreeWidgetItem(parent, QStringList(name));
	iface->setData(0, Qt::UserRole, dbusName);
	iface->setData(1, Qt::UserRole, name);
	parent->addChild(iface);

	QDBusInterface dbusIface(dbusName, QString("/quadkonsole4/")+name, interfaceName);
	QVariant numViews = dbusIface.property("numViews");

	if (numViews.canConvert<uint>())
	{
		for (uint i=0; i<numViews.toUInt(); ++i)
		{
			QTreeWidgetItem* view = new QTreeWidgetItem(iface, QStringList(QString::number(i)));
			view->setData(0, Qt::UserRole, dbusName);
			view->setData(1, Qt::UserRole, name);
			view->setData(2, Qt::UserRole, i);
			iface->addChild(view);
		}
	}
	else
		kDebug() << "could not get numViews for" << dbusName << endl;
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
