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

#include "prefsviews.h"

#include <KDE/KDebug>
#include <KDE/KService>
#include <KDE/KComboBox>
#include <KDE/KLineEdit>
#include <KDE/KEditListBox>
#include <KDE/KConfigDialogManager>


#include <QtWidgets/QWidget>
#include <QtWidgets/QListWidget>

#include "ui_prefs_views.h"

PrefsViews::PrefsViews(QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f),
	m_comboBox(new KComboBox)
{
	KConfigDialogManager::propertyMap()->insert("KEditListBox", "items");
	KConfigDialogManager::changedMap()->insert("KEditListBox", SIGNAL(changed()));

	KEditListBox::CustomEditor editor;
	editor.setLineEdit(new KLineEdit);
	editor.setRepresentationWidget(m_comboBox);
	editor.lineEdit()->setReadOnly(true);
	m_comboBox->setLineEdit(editor.lineEdit());

	Ui::prefs_views prefs_views;
	prefs_views.setupUi(this);
	prefs_views.kcfg_views->setCustomEditor(editor);
	m_mimeList = prefs_views.mimeList;

	KService::List services = KService::allServices();
	KService::List::const_iterator it;
	for (it=services.constBegin(); it!=services.constEnd(); ++it)
	{
		KService::Ptr s = *it;
		if (s->serviceTypes().contains("KParts/ReadOnlyPart", Qt::CaseInsensitive))
			m_comboBox->addItem(QIcon(s->icon()), s->entryPath());
	}

	connect(m_comboBox, SIGNAL(currentIndexChanged(QString)), SLOT(showMimeTypes(QString)));
	connect(m_comboBox, SIGNAL(editTextChanged(QString)), SLOT(showMimeTypes(QString)));
}


PrefsViews::~PrefsViews()
{}


void PrefsViews::showMimeTypes(QString partname)
{
	m_mimeList->clear();
	KService::Ptr service = KService::serviceByDesktopPath(partname);
	if (service)
		m_mimeList->addItems(service->serviceTypes());
}
