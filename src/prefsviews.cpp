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

#include "prefsviews.h"

#include <KDE/KDebug>
#include <KDE/KEditListWidget>
#include <KDE/KService>
#include <KDE/KComboBox>
#include <KDE/KLineEdit>
#include <KDE/KConfigDialogManager>

#include <QtGui/QWidget>

#include "ui_prefs_views.h"

PrefsViews::PrefsViews(QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f),
	m_comboBox(new KComboBox)
{
	KConfigDialogManager::propertyMap()->insert("KEditListWidget", "items");
	KConfigDialogManager::changedMap()->insert("KEditListWidget", SIGNAL(changed()));

	KEditListWidget::CustomEditor editor;
	editor.setLineEdit(new KLineEdit);
	editor.setRepresentationWidget(m_comboBox);
	m_comboBox->setLineEdit(editor.lineEdit());

	Ui::prefs_views prefs_views;
	prefs_views.setupUi(this);
	prefs_views.kcfg_views->setCustomEditor(editor);

	KService::List services = KService::allServices();
	KService::List::const_iterator it;
	for (it=services.constBegin(); it!=services.constEnd(); ++it)
	{
		if ((*it)->library().endsWith("part"))
			m_comboBox->addItem(KIcon((*it)->icon()), (*it)->entryPath());
	}
}


PrefsViews::~PrefsViews()
{}
