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

#ifndef QKSTACK_H
#define QKSTACK_H

#include <KDE/KUrl>
#include <KDE/KFileItemList>
#include <KDE/KTabWidget>
#include <KDE/KParts/BrowserExtension>

class QKUrlHandler;
class QKView;
class QKBrowserInterface;
namespace KParts
{
	class ReadOnlyPart;
	class PartManager;
}
namespace KIO
{
	class Job;
}


class QKStack : public KTabWidget
{
	Q_OBJECT
	public:
		explicit QKStack(KParts::PartManager& partManager, QWidget* parent = 0);
		explicit QKStack(KParts::PartManager& partManager, KParts::ReadOnlyPart* part, QWidget* parent = 0);
		virtual ~QKStack();

		QString foregroundProcess() const;
		KParts::ReadOnlyPart* part() const;
		int addViews(const QStringList& partNames);
		QString url() const;
		QString partIcon() const;
		QList<QKView*> modifiedViews();

		virtual void setCurrentIndex(int index);
		virtual QKView* currentWidget() const;

	signals:
		void partCreated();
		void setStatusBarText(QString);
		void setWindowCaption(QString);
		void setLocationBarUrl(QString);

	public slots:
		void sendInput(const QString& text) const;
		void switchView();
		void switchView(const KUrl& url, const QString& mimeType, bool tryCurrent);
		void switchView(int index, const KUrl& url);
		void settingsChanged();
		void slotTabCloseRequested(int index);
		void goBack();
		void goForward();
		void goUp();
		void goHistory(int steps);
		void slotOpenUrlRequest(const KUrl& url, bool tryCurrent=true);
		void slotOpenUrlRequest(const QString& url);
		void slotOpenNewWindow(const KUrl& url, const QString& mimeType, KParts::ReadOnlyPart** target);

	private slots:
		void slotPartCreated();
		void slotSetStatusBarText(const QString& text);
		void slotSetWindowCaption(const QString& text);
		void slotUrlFiltered(QKUrlHandler* handler);
		void slotOpenUrlNotify();
		void slotCurrentChanged();
		void enableAction(const char* action, bool enable);
		void popupMenu(const QPoint& where, const KFileItemList& items, KParts::BrowserExtension::PopupFlags flags, const KParts::BrowserExtension::ActionGroupMap& map);
		void slotMiddleClick(QWidget* widget);

	private:
		void setupUi(KParts::ReadOnlyPart* part=0);
		void addViewActions(QKView* view);
		void checkEnableActions();
		int openViewByMimeType(const QString& mimeType, bool allowDuplicate=true);

		KParts::PartManager& m_partManager;
		QKBrowserInterface* m_browserInterface;
		QStringList m_loadedViews;
};

#endif // QKSTACK_H
