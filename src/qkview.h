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

#ifndef QKVIEW_H
#define QKVIEW_H

#include <KDE/KFileItemList>
#include <KDE/KUrl>

#include <QtGui/QWidget>

class QToolBar;
class QBoxLayout;
namespace KParts
{
	class ReadOnlyPart;
	class OpenUrlArguments;
	class BrowserArguments;
}

class QKView : public QWidget
{
	Q_OBJECT
	public:
		explicit QKView(const QString& partname, QWidget* parent=0, Qt::WindowFlags f=0);
		explicit QKView(KParts::ReadOnlyPart* part, QWidget* parent=0, Qt::WindowFlags f=0);
		virtual ~QKView();

		bool hasFocus() const;
		void setFocus();
		QString getURL() const;
		void setURL(const QString& url);
		void sendInput(const QString& text);
		KParts::ReadOnlyPart* part();

		QString foregroundProcess() const;

	public slots:
		void show();
		void settingsChanged();
		void partDestroyed();

	signals:
		void partCreated();

	protected slots:
		void createPart();
		void selectionInfo(KFileItemList items);
		void openUrlRequest(KUrl url, KParts::OpenUrlArguments, KParts::BrowserArguments);
		void enableAction(const char* action, bool enable);

	private:
		void setupUi(KParts::ReadOnlyPart* part=0);
		void setupPart();

		QString m_partname;
		QBoxLayout* m_layout;
		QToolBar* m_toolbar;
		KParts::ReadOnlyPart* m_part;
};

#endif // QKVIEW_H
