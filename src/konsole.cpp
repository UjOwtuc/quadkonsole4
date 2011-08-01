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

#include "konsole.h"

#include <KDE/KParts/ReadOnlyPart>
#include <KDE/KService>
#include <KDE/KDebug>
#include <KDE/KLocale>
#include <KDE/KMessageBox>
#include <KDE/KAction>
#include <kde_terminal_interface_v2.h>
#include <KDE/KParts/BrowserExtension>

#include <QtCore/QFileInfo>
#include <QtGui/QWidget>
#include <QtGui/QLayout>
#include <QtGui/QStackedWidget>

namespace
{
	KService::Ptr konsoleService;
	KService::Ptr dolphinService;
}


Konsole::Konsole(QWidget* parent, QLayout* layout)
	: QObject(parent),
	m_parent(parent),
	m_layout(layout),
	m_stack(new QStackedWidget)
{
	layout->addWidget(m_stack);
	m_konsolePart = 0;
	m_dolphinPart = 0;
	createPart();
}


Konsole::Konsole(QWidget *parent, KParts::ReadOnlyPart* part)
	: QObject(parent),
	m_parent(parent),
	m_layout(0),
	m_stack(new QStackedWidget)
{
	m_konsolePart = part;
	m_dolphinPart = 0;
	part->setParent(parent);
	m_konsolePart->widget()->setParent(parent);
	connect(m_konsolePart, SIGNAL(destroyed()), SLOT(partDestroyed()));
}


Konsole::~Konsole()
{
	if (m_konsolePart)
	{
		disconnect(m_konsolePart, SIGNAL(destroyed()), this, SLOT(partDestroyed()));
		delete m_konsolePart;
	}
	delete m_dolphinPart;
	delete m_stack;
}


void Konsole::sendInput(const QString& text)
{
	TerminalInterfaceV2 *t = qobject_cast< TerminalInterfaceV2* >(m_konsolePart);
	if (t)
		t->sendInput(text);
	else
		kDebug() << "Part is no TerminalInterfaceV2" << endl;
}


void Konsole::setParent(QWidget* parent)
{
	m_parent = parent;
	m_konsolePart->setParent(parent);
	m_konsolePart->widget()->setParent(parent);
}


void Konsole::setLayout(QLayout* layout)
{
	if (m_layout)
	{
		kDebug() << "changing layout is not supported yet" << endl;
		return;
	}
	m_layout = layout;
	m_layout->addWidget(m_stack);
}


QString Konsole::foregroundProcessName()
{
	TerminalInterfaceV2* t = qobject_cast< TerminalInterfaceV2* >(m_konsolePart);
	if (t)
		return t->foregroundProcessName();
	else
		kDebug() << "Part is no TerminalInterfaceV2" << endl;

	return "";
}


QString Konsole::workingDir()
{
	TerminalInterfaceV2* t = qobject_cast< TerminalInterfaceV2* >(m_konsolePart);
	if (t)
	{
		int pid = t->terminalProcessId();
		QFileInfo info(QString("/proc/%1/cwd").arg(pid));
		return info.readLink();
	}
	else
		kDebug() << "Part is no TerminalInterfaceV2" << endl;
	
	return "/";
}


void Konsole::partDestroyed()
{
	if (m_konsolePart)
	{
		disconnect(m_konsolePart, SIGNAL(destroyed()), this, SLOT(partDestroyed()));
		m_konsolePart = 0;
	}

	createPart();
	m_konsolePart->widget()->setFocus();
}


void Konsole::createPart()
{
	if (m_layout == 0)
	{
		kDebug() << "no layout" << endl;
		return;
	}

	if (konsoleService.isNull())
	{
		kDebug() << "loading KPart factory" << endl;
		konsoleService = KService::serviceByDesktopPath("konsolepart.desktop");
		if (konsoleService.isNull())
		{
			KMessageBox::error(m_parent, i18n("Unable to create a factory for \"libkonsolepart\". Is Konsole installed?"));
			return;
		}
	}
	m_konsolePart = konsoleService->createInstance<KParts::ReadOnlyPart>(this);
	connect(m_konsolePart, SIGNAL(destroyed()), SLOT(partDestroyed()));
	TerminalInterfaceV2* t = qobject_cast<TerminalInterfaceV2*>(m_konsolePart);
	if (t)
		t->showShellInDir(QString());
	else
	{
		kdError() << "could not get TerminalInterface" << endl;
		exit(1);
	}

	m_konsolePart->widget()->setParent(m_parent);
	m_stack->insertWidget(0, m_konsolePart->widget());
	m_stack->setFocusProxy(m_konsolePart->widget());
	m_parent->setFocusProxy(m_konsolePart->widget());

	emit partCreated();
}


void Konsole::focusNext()
{
	if (m_dolphinPart == 0)
	{
		if (dolphinService.isNull())
			dolphinService = KService::serviceByDesktopPath("dolphinpart.desktop");
		
		if (! dolphinService.isNull())
		{
			m_dolphinPart = dolphinService->createInstance<KParts::ReadOnlyPart>(this);
			m_stack->addWidget(m_dolphinPart->widget());
		}
	}
	
	if (m_dolphinPart == 0)
		return;
	
	if (m_stack->currentWidget() == m_konsolePart->widget())
	{
		m_dolphinPart->openUrl(workingDir());
		m_stack->setFocusProxy(m_dolphinPart->widget());
		m_stack->setCurrentWidget(m_dolphinPart->widget());
	}
	else
	{
		KUrl url = m_dolphinPart->url();
		sendInput(QString("cd ") + url.directory());
		m_stack->setFocusProxy(m_konsolePart->widget());
		m_stack->setCurrentWidget(m_konsolePart->widget());
	}
}


QWidget* Konsole::widget()
{
	return m_stack;
}

#include "konsole.moc"

