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

#ifndef QKSTACK_H
#define QKSTACK_H

#include <KConfigGroup>
#include <KFileItem>
#include <KParts/BrowserExtension>

#include <QTabWidget>
#include <QEvent>

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


class QKStack : public QTabWidget
{
	Q_OBJECT
	public:
		explicit QKStack(KParts::PartManager& partManager, bool restoringSession=false, QWidget* parent = 0);
		explicit QKStack(KParts::PartManager& partManager, KParts::ReadOnlyPart* part, QWidget* parent = 0);
		virtual ~QKStack();

		QString foregroundProcess() const;
		KParts::ReadOnlyPart* part() const;
		int addViews(const QStringList& partNames);
		QString url() const;
		QString partIcon() const;
		QList<QKView*> modifiedViews();
		const QStringList& loadedViews() const { return m_loadedViews; }

		virtual void setCurrentIndex(int index);
		virtual QKView* currentWidget() const;
		const QKView* view(int index) const;

	signals:
		void partCreated();
		void setStatusBarText(QString);
		void setWindowCaption(QString);
		void setLocationBarUrl(QString);

	public slots:
		void sendInput(const QString& text) const;
		void switchView();
		void switchView(const QUrl& url, const QString& mimeType, bool tryCurrent);
		void switchView(int index, const QUrl& url);
		void settingsChanged();
		void slotTabCloseRequested(int index);
		void goBack();
		void goForward();
		void goUp();
		void goHistory(int steps);
		void slotOpenUrlRequest(const QUrl& url, bool tryCurrent=true);
		void slotOpenUrlRequest(const QString& url);
		void slotOpenNewWindow(const QUrl& url, const QString& mimeType, KParts::ReadOnlyPart** target);

	private slots:
		void slotPartCreated();
		void slotSetStatusBarText(const QString& text);
		void slotSetWindowCaption(const QString& text);
		void slotUrlFiltered(QKUrlHandler* handler);
		void slotOpenUrlNotify();
		void slotCurrentChanged();
		void enableAction(const char* action, bool enable);

	private:
		void setupUi(KParts::ReadOnlyPart* part=0, bool restoringSession=false);
		void addViewActions(QKView* view);
		void checkEnableActions();
		int openViewByMimeType(const QString& mimeType, bool allowDuplicate=true);

		KParts::PartManager& m_partManager;
		QKBrowserInterface* m_browserInterface;
		QStringList m_loadedViews;
};

#endif // QKSTACK_H
