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
#include "settings.h"

#include <KDE/KDebug>
#include <KDE/KService>
#include <KDE/KMessageBox>
#include <KDE/KActionCollection>
#include <KDE/KUrl>
#include <KDE/KFile>
#include <KDE/KFileItemList>
#include <KDE/KMenu>
#include <KDE/KParts/ReadOnlyPart>
#include <KDE/KParts/BrowserExtension>
#include <KDE/KParts/PartManager>
#include <kde_terminal_interface_v2.h>

#include <QtCore/QFileInfo>
#include <QtGui/QWidget>
#include <QtGui/QBoxLayout>
#include <QtGui/QToolBar>
#include <QtGui/QApplication>


QMap<QString, KService::Ptr> QKPartFactory::m_partFactories;


KService::Ptr QKPartFactory::getFactory(const QString& name)
{
	if (m_partFactories[name].isNull())
	{
		kDebug() << "creating factory for" << name << endl;
		m_partFactories[name] = KService::serviceByDesktopPath(name);
		if (m_partFactories[name].isNull())
			KMessageBox::error(0, i18n("Could not create a factory for %1.", name));
	}
	return m_partFactories[name];
}


QKView::QKView(KParts::PartManager& partManager, const QString& partname, QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f),
	m_partname(partname),
	m_partManager(partManager)
{
	setupUi();
}


QKView::QKView(KParts::PartManager& partManager, KParts::ReadOnlyPart* part, QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f),
	m_partManager(partManager)
{
	m_partname = part->property("QKPartName").toString();
	setupUi(part);
}


QKView::~QKView()
{}


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
		else if (hasMimeType(url))
		{
			m_part->openUrl(url);
		}
		else
		{
			emit openUrlOutside(url);
		}
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


bool QKView::hasMimeType(const KUrl& url)
{
	KService::Ptr service = QKPartFactory::getFactory(m_partname);
	KFileItem fileItem(KFileItem::Unknown, KFileItem::Unknown, url);
	if (service.isNull())
		return false;

	if (service->hasMimeType(fileItem.mimetype()))
		return true;

	kDebug() << "KPart" << m_partname << "does not like mime type" << fileItem.mimetype() << endl;
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

	m_part = service->createInstance<KParts::ReadOnlyPart>();
	if (m_part == 0)
	{
		KMessageBox::error(this, i18n("The factory for %1 could not create a KPart.", m_partname));
		return;
	}
	m_part->setProperty("QKPartName", m_partname);

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
			m_partManager.removeManagedTopLevelWidget(m_part->widget());
			disconnect(m_part->widget(), SIGNAL(destroyed(QObject*)), this, SLOT(partDestroyed()));
		}
	}
	createPart();
}


void QKView::popupMenu(QPoint where, KFileItemList)
{
	KMenu* popup = new KMenu(this);
	QList<QActionGroup*> groups = m_part->actionCollection()->actionGroups();
	QListIterator<QActionGroup*> it(groups);
	while (it.hasNext())
	{
		QActionGroup* group = it.next();
		if (group->isVisible() && group->isEnabled())
		{
			KMenu* submenu = new KMenu;
			submenu->addActions(group->actions());
			popup->addMenu(submenu);
		}
	}
	popup->addActions(m_part->actionCollection()->actionsWithoutGroup());
	popup->popup(where);
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
	setURL(url);
	m_windowCaption = url.pathOrUrl();
	emit setWindowCaption(m_windowCaption);
}


void QKView::enableAction(const char* action, bool enable)
{
	kDebug() << "action " << action << enable << endl;
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


void QKView::setupUi(KParts::ReadOnlyPart* part)
{
	setContentsMargins(0, 0, 0, 0);

	m_layout = new QBoxLayout(QBoxLayout::TopToBottom, this);

	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0, 0, 0, 0);

	m_toolbar = new QToolBar;
	m_layout->addWidget(m_toolbar);
	m_part = part;

	if (m_part)
		setupPart();

	if (! Settings::viewHasToolbar())
		m_toolbar->hide();
	connect(Settings::self(), SIGNAL(configChanged()), SLOT(settingsChanged()));
}


void QKView::setupPart()
{
	m_partManager.addPart(m_part);
	m_part->setManager(&m_partManager);
	m_layout->addWidget(m_part->widget());
	setFocusProxy(m_part->widget());


	m_toolbar->addActions(m_part->actionCollection()->actions());
	m_toolbar->addActions(m_part->widget()->actions());
	m_part->widget()->setFocus();

	connect(m_part->widget(), SIGNAL(destroyed()), SLOT(partDestroyed()));
	connect(m_part, SIGNAL(setStatusBarText(QString)), SLOT(slotSetStatusBarText(QString)));
	connect(m_part, SIGNAL(setWindowCaption(QString)), SLOT(slotSetWindowCaption(QString)));

	TerminalInterfaceV2* t = qobject_cast<TerminalInterfaceV2*>(m_part);
	if (t)
		t->showShellInDir(QString());

	KParts::BrowserExtension* b = KParts::BrowserExtension::childObject(m_part);
	if (b)
	{
		connect(b, SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)), SLOT(popupMenu(QPoint,KFileItemList)));
		connect(b, SIGNAL(selectionInfo(KFileItemList)), SLOT(selectionInfo(KFileItemList)));
		connect(b, SIGNAL(openUrlRequestDelayed(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)), SLOT(openUrlRequest(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)));
		connect(b, SIGNAL(enableAction(const char*,bool)), SLOT(enableAction(const char*,bool)));
	}
}

#include "qkview.moc"