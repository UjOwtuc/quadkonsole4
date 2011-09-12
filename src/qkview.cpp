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

#include "qkview.h"
#include "qkstack.h"
#include "settings.h"

#include <kdeversion.h>
#include <KDE/KAction>
#include <KDE/KXmlGuiWindow>
#include <KDE/KXMLGUIFactory>
#include <KDE/KDebug>
#include <KDE/KService>
#include <KDE/KMessageBox>
#include <KDE/KActionCollection>
#include <KDE/KUrl>
#include <KDE/KFile>
#include <KDE/KFileItemList>

#include <KDE/KLineEdit>
#include <KDE/KUrlRequester>
#include <KDE/KParts/ReadOnlyPart>
#include <KDE/KParts/BrowserExtension>
#include <KDE/KParts/PartManager>
#include <KDE/KParts/StatusBarExtension>
#include <kde_terminal_interface_v2.h>

#include <QtCore/QFileInfo>
#include <QtGui/QWidget>
#include <QtGui/QBoxLayout>
#include <QtGui/QToolBar>
#include <QtGui/QApplication>


QMap<QString, KService::Ptr> QKPartFactory::m_partFactories;


KService::Ptr QKPartFactory::getFactory(const QString& name)
{
	if (m_partFactories.count(name) == 0)
	{
		kDebug() << "creating factory for" << name << endl;
		m_partFactories[name] = KService::serviceByDesktopPath(name);
		if (m_partFactories[name].isNull())
			KMessageBox::error(0, i18n("Could not create a factory for %1.", name));
	}
	return m_partFactories[name];
}


QKView::QKView(KParts::PartManager& partManager, KParts::BrowserInterface* browserInterface, const QString& partname, QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f),
	m_partname(partname),
	m_part(0),
	m_partManager(partManager),
	m_icon(0),
	m_browserInterface(browserInterface)
{
	setupUi();
}


QKView::QKView(KParts::PartManager& partManager, KParts::BrowserInterface* browserInterface, KParts::ReadOnlyPart* part, QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f),
	m_part(part),
	m_partManager(partManager),
	m_icon(0),
	m_browserInterface(browserInterface)
{
	m_partname = part->property("QKPartName").toString();
	setupUi();
}


QKView::~QKView()
{
	if (m_part)
	{
		m_partManager.removePart(m_part);
		m_part->disconnect();
		if (m_part->widget())
			m_part->widget()->disconnect();

		KXmlGuiWindow* window = qobject_cast<KXmlGuiWindow*>(m_partManager.parent());
		if (window)
			window->guiFactory()->removeClient(m_part);
	}
	delete m_urlbar;
	delete m_part;
	delete m_icon;
}


bool QKView::hasFocus() const
{
	if (m_part)
		return m_part->widget()->hasFocus();
	return false;
}


void QKView::setFocus()
{
	if (m_part)
	{
		m_partManager.setActivePart(m_part);
		m_partManager.setSelectedPart(m_part);
		m_part->widget()->setFocus();
	}
}


KUrl QKView::getURL() const
{
	if (m_part)
	{
		TerminalInterfaceV2* t = qobject_cast<TerminalInterfaceV2*>(m_part);
		if (t)
		{
			int pid = t->terminalProcessId();
			QFileInfo info(QString("/proc/%1/cwd").arg(pid));
			return info.readLink();
		}
		else
			return m_part->url();
	}
	return KUrl();
}


void QKView::setURL(const KUrl& url)
{
	if (m_part && getURL() != url)
	{
		TerminalInterfaceV2* t = qobject_cast<TerminalInterfaceV2*>(m_part);
		if (t)
		{
			QString escaped(url.pathOrUrl());
			escaped.replace("\"", "\\\"");

			if (! url.hasHost() && QFileInfo(url.path()).isDir())
				t->sendInput("cd ");

			t->sendInput(QString(" \"%1\"").arg(escaped));
		}
		else
			m_part->openUrl(url);

		m_windowCaption = url.pathOrUrl();
		emit setWindowCaption(m_windowCaption);
	}
}


void QKView::sendInput(const QString& text)
{
	TerminalInterfaceV2* t;
	if (m_part && (t = qobject_cast<TerminalInterfaceV2*>(m_part)))
		t->sendInput(text);
	else
		kDebug() << "don't know how to send input to my part" << endl;
}


KParts::ReadOnlyPart* QKView::part()
{
	return m_part;
}


QString QKView::foregroundProcess() const
{
	TerminalInterfaceV2* t;
	if (m_part && (t = qobject_cast<TerminalInterfaceV2*>(m_part)))
		return t->foregroundProcessName();

	return "";
}


bool QKView::hasMimeType(const QString& type)
{
	KService::Ptr service = QKPartFactory::getFactory(m_partname);
	if (service.isNull())
		return false;

#if KDE_VERSION_MINOR >= 6
	if (service->hasMimeType(type))
		return true;
#else // KDE_VERSION_MINOR
	if (service->hasMimeType(KMimeType::mimeType(type).data()))
		return true;
#endif // KDE_VERSION_MINOR
	kDebug() << "KPart" << m_partname << "does not like mime type" << type << endl;
	return false;
}


void QKView::show()
{
	if (m_part == 0)
	{
		createPart();
		m_partManager.setActivePart(m_part);
		m_partManager.setSelectedPart(m_part);
	}
}


void QKView::settingsChanged()
{
	if (Settings::viewHasToolbar())
		m_toolbar->show();
	else
		m_toolbar->hide();
}


void QKView::createPart()
{
	KService::Ptr service = QKPartFactory::getFactory(m_partname);
	if (service.isNull())
		return;

	m_part = service->createInstance<KParts::ReadOnlyPart>(this);
	if (m_part == 0)
	{
		KMessageBox::error(this, i18n("The factory for %1 could not create a KPart.", m_partname));
		return;
	}
	m_part->setProperty("QKPartName", m_partname);

	if (! m_icon)
	{
		m_icon = new KIcon(service->icon());
		emit iconChanged();
	}

	setupPart();

	emit partCreated();
}


void QKView::partDestroyed()
{
	if (m_part)
	{
		m_partManager.removePart(m_part);
		m_part->disconnect();
		if (m_part->widget())
		{
			// part was detached
			KParts::BrowserExtension* b = KParts::BrowserExtension::childObject(m_part);
			if (b)
			{
				b->setBrowserInterface(0);
				b->disconnect(SIGNAL(popupMenu(QPoint,KUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)), this);
				b->disconnect(SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)), this);
				b->disconnect(SIGNAL(selectionInfo(KFileItemList)), this);
				b->disconnect(SIGNAL(openUrlRequestDelayed(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)), this);
				b->disconnect(SIGNAL(openUrlNotify()), this);
				b->disconnect(SIGNAL(enableAction(const char*,bool)), this);
			}
			m_partManager.removeManagedTopLevelWidget(m_part->widget());
			disconnect(m_part->widget(), SIGNAL(destroyed(QObject*)), this, SLOT(partDestroyed()));

		}
	}
	createPart();
}


void QKView::toggleUrlBar()
{
	if (m_urlbar->isVisible())
		m_urlbar->hide();
	else
	{
		m_urlbar->setUrl(getURL());
		m_urlbar->show();
		m_urlbar->setFocus();
		m_urlbar->lineEdit()->selectAll();
	}
}


void QKView::slotPopupMenu(const QPoint& where, const KUrl &url, mode_t mode, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs, KParts::BrowserExtension::PopupFlags flags, const KParts::BrowserExtension::ActionGroupMap& map)
{
	KFileItem item(url, args.mimeType(), mode);
	KFileItemList list;
	list.append(item);
	slotPopupMenu(where, list, args, browserArgs, flags, map);
}


void QKView::slotPopupMenu(const QPoint& where, const KFileItemList& items, const KParts::OpenUrlArguments& /*args*/, const KParts::BrowserArguments& /*browserArgs*/, KParts::BrowserExtension::PopupFlags flags, const KParts::BrowserExtension::ActionGroupMap& map)
{
	emit popupMenu(where, items, flags, map);
}


void QKView::selectionInfo(KFileItemList items)
{
	kDebug() << "selected items: " << items << endl;
	if (items.count() == 1)
		m_statusBarText = items.at(0).getStatusBarInfo();
	else
	{
		KIO::filesize_t size = 0;
		KFileItemList::const_iterator it;
		for (it=items.constBegin(); it!=items.constEnd(); ++it)
			size += it->size();

		m_statusBarText = i18n("%1 files selected (%2)", items.size(), size);
	}
	emit setStatusBarText(m_statusBarText);
}


void QKView::openUrlRequest(KUrl url, KParts::OpenUrlArguments, KParts::BrowserArguments)
{
	kDebug() << "url " << url << endl;
	emit openUrlRequest(url);
}


void QKView::enableAction(const char* action, bool enable)
{
	kDebug() << action << enable << endl;
	KXmlGuiWindow* window = qobject_cast<KXmlGuiWindow*>(m_partManager.parent());
	if (window)
	{
		QAction* a = window->actionCollection()->action(action);
		if (a)
			a->setEnabled(enable);
	}
}


void QKView::slotSetStatusBarText(QString text)
{
	m_statusBarText = text;
	emit setStatusBarText(text);
}


void QKView::slotSetWindowCaption(QString text)
{
	m_windowCaption = text;
	emit setWindowCaption(text);
}


void QKView::slotOpenUrl(QString url)
{
	KUrl real(url);
	if (real.protocol().isEmpty() && ! url.startsWith('/'))
		real = QString("http://") + url;

	emit openUrlRequest(real);
}


void QKView::setupUi()
{
	KService::Ptr service = QKPartFactory::getFactory(m_partname);
	if (! service.isNull())
		m_icon = new QIcon(service->icon());

	setContentsMargins(0, 0, 0, 0);

	m_layout = new QBoxLayout(QBoxLayout::TopToBottom, this);

	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0, 0, 0, 0);

	m_toolbar = new QToolBar;
	m_layout->addWidget(m_toolbar);

	m_urlbar = new KUrlRequester;
	m_urlbar->hide();
	connect(m_urlbar, SIGNAL(returnPressed(QString)), SLOT(slotOpenUrl(QString)));
	connect(m_urlbar, SIGNAL(returnPressed()), m_urlbar, SLOT(hide()));
	connect(m_urlbar, SIGNAL(urlSelected(KUrl)), SIGNAL(openUrlRequest(KUrl)));
	connect(m_urlbar, SIGNAL(urlSelected(KUrl)), m_urlbar, SLOT(hide()));

	m_layout->addWidget(m_urlbar);

	if (m_part)
		setupPart();

	if (! Settings::viewHasToolbar())
		m_toolbar->hide();
	connect(Settings::self(), SIGNAL(configChanged()), SLOT(settingsChanged()));
}


void QKView::setupPart()
{
	m_layout->addWidget(m_part->widget());
	setFocusProxy(m_part->widget());

	m_part->widget()->setFocus();

	connect(m_part->widget(), SIGNAL(destroyed()), SLOT(partDestroyed()));
	connect(m_part, SIGNAL(setStatusBarText(QString)), SLOT(slotSetStatusBarText(QString)));
	connect(m_part, SIGNAL(setWindowCaption(QString)), SLOT(slotSetWindowCaption(QString)));

	TerminalInterfaceV2* t = qobject_cast<TerminalInterfaceV2*>(m_part);
	if (t)
	{
		kDebug() << "part" << m_partname << "has a TerminalInterfaceV2" << endl;
		t->showShellInDir(QString());
	}

	KParts::BrowserExtension* b = KParts::BrowserExtension::childObject(m_part);
	if (b)
	{
		kDebug() << "part" << m_partname << "has a BrowserExtension" << endl;
		b->setBrowserInterface(m_browserInterface);

		connect(b, SIGNAL(popupMenu(QPoint,KUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)), SLOT(slotPopupMenu(QPoint,KUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)));
		connect(b, SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)), SLOT(slotPopupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)));
		connect(b, SIGNAL(selectionInfo(KFileItemList)), SLOT(selectionInfo(KFileItemList)));
		connect(b, SIGNAL(openUrlRequestDelayed(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)), SLOT(openUrlRequest(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)));
		connect(b, SIGNAL(openUrlNotify()), SIGNAL(openUrlNotify()));
		connect(b, SIGNAL(enableAction(const char*,bool)), SLOT(enableAction(const char*,bool)));
	}

	KParts::StatusBarExtension* sb = KParts::StatusBarExtension::childObject(m_part);
	if (sb)
	{
		kDebug() << "part" << m_partname << "has a StatusBarExtension" << endl;
		KXmlGuiWindow* window = qobject_cast<KXmlGuiWindow*>(m_partManager.parent());
		if (window)
			sb->setStatusBar(window->statusBar());
	}
	m_partManager.addPart(m_part);
}

#include "qkview.moc"
