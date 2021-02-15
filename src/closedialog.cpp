/**************************************************************************
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

#include "closedialog.h"
#include "qkstack.h"
#include "qkview.h"

#include <QPushButton>
#include <QTableWidget>
#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QDialog>

#include "ui_detach_processes.h"

CloseDialog::CloseDialog(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
	m_widget = new Ui::DetachProcesses;
	m_widget->setupUi(this);

	QPushButton* dont = new QPushButton(QIcon("application-exit"), i18n("Don't detach"));
	connect(dont, SIGNAL(clicked()), SLOT(dontDetach()));
	m_widget->buttonBox->addButton(dont, QDialogButtonBox::ActionRole);

	QPushButton* detachBtn = new QPushButton(QIcon("document-new"), i18n("Detach selected"));
	connect(detachBtn, SIGNAL(clicked()), SLOT(detach()));
	m_widget->buttonBox->addButton(detachBtn, QDialogButtonBox::ActionRole);
}


CloseDialog::~CloseDialog()
{
	delete m_widget;
}


void CloseDialog::addView(QKStack* stack, QKView* view)
{
	QTableWidget* pt = m_widget->processTable;
	int rowId = pt->rowCount();
	pt->insertRow(rowId);

	QCheckBox* cb = new QCheckBox;
	cb->setCheckState(Qt::Checked);
	pt->setCellWidget(rowId, 0, cb);

	pt->setItem(rowId, 1, new QTableWidgetItem(view->partCaption()));
	pt->setItem(rowId, 2, new QTableWidgetItem(view->foregroundProcess()));
	pt->resizeColumnsToContents();

	m_viewStackMap[view] = stack;
	m_result.append(view);
}


void CloseDialog::addViews(QKStack* stack, QList<QKView*> views)
{
	QListIterator<QKView*> it(views);
	while (it.hasNext())
		addView(stack, it.next());
}


void CloseDialog::dontDetach()
{
	m_result.clear();
	m_viewStackMap.clear();
	accept();
}


void CloseDialog::detach()
{
	QTableWidget* pt = m_widget->processTable;
	for (int i=0; i<pt->rowCount(); ++i)
	{
		QCheckBox* cb = qobject_cast<QCheckBox*>(pt->cellWidget(i, 0));
		if (cb->checkState() != Qt::Checked)
		{
			QKView* view = m_result.takeAt(i);
			m_viewStackMap.remove(view);
		}
	}
	accept();
}


int CloseDialog::size() const
{
	return m_widget->processTable->rowCount();
}


const QMap<QKView *, QKStack *> & CloseDialog::selectedForDetach() const
{
	return m_viewStackMap;
}
