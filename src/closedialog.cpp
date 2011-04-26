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

#include "closedialog.h"

#include <KDE/KDialog>
#include <KDE/KPushButton>

#include <QtGui/QTableWidget>
#include <QtGui/QBoxLayout>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QCheckBox>

#include "ui_detach_processes.h"

CloseDialog::CloseDialog(QWidget* parent, Qt::WindowFlags f)
	: KDialog(parent, f)
{
	m_widget = new Ui::DetachProcesses;
	m_widget->setupUi(mainWidget());

	setButtons(Cancel | User1 | User2);

	setButtonIcon(User1, KIcon("application-exit"));
	setButtonText(User1, i18n("Don't detach"));
	connect(this, SIGNAL(user1Clicked()), SLOT(dontDetach()));

	setButtonIcon(User2, KIcon("document-new"));
	setButtonText(User2, i18n("Detach selected"));
	connect(this, SIGNAL(user2Clicked()), SLOT(detach()));
}


CloseDialog::~CloseDialog()
{
	delete m_widget;
}


void CloseDialog::addProcess(int id, const QString& name)
{
	QTableWidget* pt = m_widget->processTable;
	int rowId = pt->rowCount();
	pt->insertRow(rowId);

	QCheckBox* cb = new QCheckBox;
	cb->setCheckState(Qt::Checked);
	pt->setCellWidget(rowId, 0, cb);

	QTableWidgetItem* item = new QTableWidgetItem(name);
	item->setData(Qt::UserRole, id);
	pt->setItem(rowId, 1, item);
}


void CloseDialog::dontDetach()
{
	m_result.clear();
	accept();
}


void CloseDialog::detach()
{
	QTableWidget* pt = m_widget->processTable;
	for (int i=0; i<pt->rowCount(); ++i)
	{
		QCheckBox* cb = qobject_cast<QCheckBox*>(pt->cellWidget(i, 0));
		if (cb->checkState() == Qt::Checked)
			m_result << pt->item(i, 1)->data(Qt::UserRole).toInt();
	}
	accept();
}


bool CloseDialog::exec(QList<int>& detach)
{
	qobject_cast<QDialog*>(this)->exec();
	detach = m_result;

	return result();
}


int CloseDialog::size() const
{
	return m_widget->processTable->rowCount();
}
