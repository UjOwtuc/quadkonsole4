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
#include <KDE/KFileItemList>
#include <KDE/KParts/ReadOnlyPart>
#include <KDE/KParts/BrowserExtension>
#include <kde_terminal_interface_v2.h>

#include <QtCore/QFileInfo>
#include <QtGui/QWidget>
#include <QtGui/QBoxLayout>
#include <QtGui/QToolBar>

namespace
{
	QMap<QString, KService::Ptr> partFactories;
}


QKView::QKView(const QString& partname, QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f),
	m_partname(partname)
{
	setupUi();
}


QKView::QKView(KParts::ReadOnlyPart* part, QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f)
{
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
		m_part->widget()->setFocus();
}


QString QKView::getURL() const
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
			return m_part->url().path();
	}
	return "";
}


void QKView::setURL(const QString& url)
{
	if (m_part)
	{
		if (getURL() != url)
		{
			TerminalInterfaceV2* t = qobject_cast<TerminalInterfaceV2*>(m_part);
			if (t)
				t->sendInput(QString("cd \"%1\"").arg(url));
			else
				m_part->openUrl(url);
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


void QKView::show()
{
	if (m_part == 0)
		createPart();
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
	if (partFactories[m_partname].isNull())
	{
		partFactories[m_partname] = KService::serviceByDesktopPath(m_partname);
		if (partFactories[m_partname].isNull())
		{
			KMessageBox::error(this, i18n("Could not create a factory for %1.", m_partname));
			return;
		}
	}

	m_part = partFactories[m_partname]->createInstance<KParts::ReadOnlyPart>();
	if (m_part == 0)
	{
		KMessageBox::error(this, i18n("The factory for %1 could not create a KPart.", m_partname));
		return;
	}

	setupPart();

	emit partCreated();
}


void QKView::partDestroyed()
{
	createPart();
}


void QKView::selectionInfo(KFileItemList items)
{
	kDebug() << "selected items: " << items << endl;
}


void QKView::openUrlRequest(KUrl url, KParts::OpenUrlArguments, KParts::BrowserArguments)
{
	kDebug() << "url " << url << endl;
	m_part->openUrl(url);
}


void QKView::enableAction(const char* action, bool enable)
{
	kDebug() << "action " << action << enable << endl;
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
	m_layout->addWidget(m_part->widget());
	setFocusProxy(m_part->widget());
	m_toolbar->addActions(m_part->actionCollection()->actions());
	m_part->widget()->setFocus();

	connect(m_part->widget(), SIGNAL(destroyed()), SLOT(partDestroyed()));

	TerminalInterfaceV2* t = qobject_cast<TerminalInterfaceV2*>(m_part);
	if (t)
		t->showShellInDir(QString());

	KParts::BrowserExtension* b = KParts::BrowserExtension::childObject(m_part);
	if (b)
	{
		//connect(b, SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)), SLOT(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)));
		connect(b, SIGNAL(selectionInfo(KFileItemList)), SLOT(selectionInfo(KFileItemList)));
		connect(b, SIGNAL(openUrlRequestDelayed(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)), SLOT(openUrlRequest(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)));
		connect(b, SIGNAL(enableAction(const char*,bool)), SLOT(enableAction(const char*,bool)));
	}
}

#include "qkview.moc"
