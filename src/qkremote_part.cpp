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

#include <QtCore/QTimer>
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


namespace
{
	const char interfaceName[] = "de.ccchl.quadkonsole4.QuadKonsole";
	const char introspectInterface[] = "org.freedesktop.DBus.Introspectable";
	const QString destinationBase = "de.ccchl.quadkonsole4-%1";
}


QKRMainWindow::QKRMainWindow(QKRInstance* parent, const QString& dbusName, const QString& name)
	: QObject(parent),
	QTreeWidgetItem(parent),
	m_dbusName(dbusName),
	m_name(name)
{
	update();

	if (m_numViews > 0)
	{
		setText(0, m_name);
		setIcon(0, KIcon("quadkonsole4"));
		setData(0, Qt::UserRole, dbusName);
		setData(1, Qt::UserRole, name);
		parent->addChild(this);
	}
}


QKRMainWindow::~QKRMainWindow()
{
	while (m_views.count())
		delete m_views.takeFirst();
}


void QKRMainWindow::update()
{
	QDBusInterface dbusIface(m_dbusName, QString("/quadkonsole4/")+m_name, interfaceName);
	QVariant numViews = dbusIface.property("numViews");

	if (numViews.canConvert<uint>())
		m_numViews = numViews.toUInt();
	else
		m_numViews = 0;

	QDBusReply<QStringList> urls = dbusIface.call(QDBus::BlockWithGui, "urls");
	if (! urls.isValid())
		kDebug() << "could not get URL list from" << m_dbusName << urls.error().message() << endl;

	QDBusReply<QStringList> icons = dbusIface.call(QDBus::BlockWithGui, "partIcons");
	if (! icons.isValid())
		kDebug() << "could not get part icons from" << m_dbusName << icons.error().message() << endl;

	while (static_cast<uint>(m_views.count()) < m_numViews)
	{
		QTreeWidgetItem* item = new QTreeWidgetItem(this);
		m_views.append(item);
		addChild(item);
	}
	for (uint i=0; i<m_numViews; ++i)
	{
		m_views[i]->setText(0, QString::number(i));

		if (static_cast<uint>(urls.value().count()) > i)
			m_views[i]->setText(1, urls.value().at(i));

		if (static_cast<uint>(icons.value().count()) > i)
			m_views[i]->setIcon(0, KIcon(icons.value().at(i)));

		m_views[i]->setData(0, Qt::UserRole, m_dbusName);
		m_views[i]->setData(1, Qt::UserRole, m_name);
		m_views[i]->setData(2, Qt::UserRole, i);
	}
	while (static_cast<uint>(m_views.count()) > m_numViews)
	{
		delete m_views.last();
		m_views.pop_back();
	}
}


void QKRMainWindow::sendInput(const QString& text)
{
	QDBusInterface iface(m_dbusName, QString("/quadkonsole4/")+m_name, interfaceName);
	for (uint i=0; i<static_cast<uint>(m_views.count()); ++i)
	{
		if (m_views.at(i)->isSelected())
			iface.call(QDBus::NoBlock, "sendInput", i, text);
	}
}


QKRInstance::QKRInstance(QTreeWidget* parent, const QString& dbusName)
	: QObject(parent),
	QTreeWidgetItem(parent),
	m_dbusName(dbusName)
{
	setText(0, m_dbusName);
	setIcon(0, KIcon("qkremote"));
	setData(0, Qt::UserRole, m_dbusName);
	parent->addTopLevelItem(this);

	update();
}


QKRInstance::~QKRInstance()
{
	QList<QKRMainWindow*> win = m_mainWindows.values();
	while (win.count())
		delete win.takeFirst();
	win.clear();
}


void QKRInstance::update()
{
	QDBusInterface iface(m_dbusName, "/quadkonsole4", introspectInterface);
	QDBusReply<QString> reply = iface.call(QDBus::BlockWithGui, "Introspect");
	if (reply.isValid())
	{
		QString label(m_dbusName);
		if (m_dbusName == destinationBase.arg(getpid()))
			label += " (this process)";

		QDomDocument doc;
		doc.setContent(reply.value());
		QDomElement root = doc.documentElement();
		QDomNodeList nodes = root.elementsByTagName("node");

		QStringList mainWindows;
		for (uint i=0; i<nodes.length(); ++i)
		{
			QDomNamedNodeMap attrs = nodes.item(i).attributes();
			QDomNode name = attrs.namedItem("name");
			if (m_mainWindows.contains(name.nodeValue()))
				m_mainWindows[name.nodeValue()]->update();
			else
				m_mainWindows[name.nodeValue()] = new QKRMainWindow(this, m_dbusName, name.nodeValue());

			if (m_mainWindows[name.nodeValue()]->numViews() > 0)
				mainWindows << name.nodeValue();
		}

		QStringList cached = m_mainWindows.keys();
		QStringListIterator it(cached);
		while (it.hasNext())
		{
			QString name = it.next();
			if (! mainWindows.contains(name))
				delete m_mainWindows.take(name);
		}
	}
	else
		kDebug() << "invalid reply to Introspect from" << m_dbusName << endl;
}


void QKRInstance::sendInput(const QString& text)
{
	QMapIterator<QString, QKRMainWindow*> it(m_mainWindows);
	while (it.hasNext())
		it.next().value()->sendInput(text);
}


const char QKRemotePart::version[] = QUADKONSOLE4_VERSION;
const char QKRemotePart::partName[] = "qkremotepart";


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
	m_updateTimer->start(0);

	connect(m_updateTimer, SIGNAL(timeout()), SLOT(refreshAvailableSlaves()));
	connect(m_remote->refreshButton, SIGNAL(clicked(bool)), SLOT(refreshAvailableSlaves()));
	connect(m_remote->identifyButton, SIGNAL(clicked(bool)), SLOT(identifyViews()));
	connect(m_remote->availableSlaves, SIGNAL(itemActivated(QTreeWidgetItem*,int)), SLOT(focusInputLine()));
	connect(m_remote->inputLine, SIGNAL(textEdited(QString)), SLOT(sendInput(QString)));
	connect(m_eventFilter, SIGNAL(keypress(QKeyEvent*)), SLOT(slotKeypress(QKeyEvent*)));

	setXMLFile("qkremote_part.rc");

	QStringList headerLabels;
	headerLabels << i18n("Available slaves") << i18n("Current URL");
	m_remote->availableSlaves->setHeaderLabels(headerLabels);

	qApp->installEventFilter(m_eventFilter);
}


QKRemotePart::~QKRemotePart()
{
	delete m_eventFilter;
	QList<QKRInstance*> instances = m_instances.values();
	while (instances.count())
		delete instances.takeFirst();
	m_instances.clear();
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

	QMapIterator<QString, QKRInstance*> it(m_instances);
	while (it.hasNext())
		it.next().value()->sendInput(text);
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
	m_updateTimer->setInterval(2500);
	m_remote->refreshButton->setEnabled(false);

	QStringList services = m_dbusConn->interface()->registeredServiceNames();
	services = services.filter(destinationBase.arg(""));

	QStringListIterator it(services);
	while (it.hasNext())
	{
		QString dest = it.next();
		if (m_instances.count(dest))
			m_instances[dest]->update();
		else
			m_instances[dest] = new QKRInstance(m_remote->availableSlaves, dest);
	}

	QStringList cached = m_instances.keys();
	QStringListIterator cacheIt(cached);
	while (cacheIt.hasNext())
	{
		QString name = cacheIt.next();
		if (! services.contains(name))
		{
			QKRInstance* instance = m_instances.take(name);
			delete instance;
		}
	}
	m_remote->refreshButton->setEnabled(true);
	m_remote->availableSlaves->expandAll();
	m_remote->availableSlaves->resizeColumnToContents(0);
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
