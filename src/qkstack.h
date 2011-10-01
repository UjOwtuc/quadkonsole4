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
		KParts::ReadOnlyPart* part();
		void partDestroyed();
		int addViews(const QStringList& partNames);
		int historyLength() const { return m_history.count(); }
		const QStringList& history() { return m_history; }
		int historyPosition() { return m_historyPosition; }
		void setHistory(const QStringList& history, int historyPosition);
		QString url() const;
		QString partIcon() const;

		virtual void setCurrentIndex(int index);

	signals:
		void partCreated();
		void setStatusBarText(QString);
		void setWindowCaption(QString);
		void historyChanged();

	public slots:
		void sendInput(const QString& text);
		void switchView();
		void switchView(KUrl url, const QString& mimeType, bool tryCurrent);
		void switchView(int index, const KUrl& url);
		void settingsChanged();
		void slotTabCloseRequested(int index);
		void goBack();
		void goForward();
		void goUp();
		void goHistory(int steps);
		void slotOpenUrlRequest(KUrl url, bool tryCurrent=true);
		void slotOpenUrlRequest(const QString& url);

	private slots:
		void slotPartCreated();
		void slotSetStatusBarText(QString text);
		void slotSetWindowCaption(QString text);
		void slotUrlFiltered(QKUrlHandler* handler);
		void slotOpenUrlNotify();
		void slotCurrentChanged();
		void enableAction(const char* action, bool enable);
		void popupMenu(const QPoint& where, const KFileItemList& items, KParts::BrowserExtension::PopupFlags flags, const KParts::BrowserExtension::ActionGroupMap& map);
		void slotMiddleClick(QWidget* widget);
		void slotIconChanged();

	private:
		void setupUi(KParts::ReadOnlyPart* part=0);
		void addViewActions(QKView* view);
		void checkEnableActions();
		void addHistoryEntry(const KUrl& url);

		KParts::PartManager& m_partManager;
		QKBrowserInterface* m_browserInterface;
		QStringList m_loadedViews;
		QStringList m_history;
		int m_historyPosition;
		bool m_blockHistory;
};

#endif // QKSTACK_H
