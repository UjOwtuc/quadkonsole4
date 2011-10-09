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

#include <KDE/KUrl>
#include <KDE/KService>
#include <KDE/KFileItemList>
#include <KDE/KParts/BrowserExtension>

#include <QtGui/QWidget>

class KMenu;
class QToolBar;
class QBoxLayout;
namespace KParts
{
	class ReadOnlyPart;
	class OpenUrlArguments;
	class BrowserArguments;
	class PartManager;
	class BrowserInterface;
}


class QKPartFactory
{
	public:
		static KService::Ptr getFactory(const QString& name);

	private:
		QKPartFactory() {};
		static QMap<QString, KService::Ptr> m_partFactories;
};


class QKView : public QWidget
{
	Q_OBJECT
	public:
		typedef enum { PartName=0 } userData_t;
		explicit QKView(KParts::PartManager& partManager, KParts::BrowserInterface* browserInterface, const QString& partname, QWidget* parent=0, Qt::WindowFlags f=0);
		explicit QKView(KParts::PartManager& partManager, KParts::BrowserInterface* browserInterface, KParts::ReadOnlyPart* part, QWidget* parent=0, Qt::WindowFlags f=0);
		virtual ~QKView();

		KUrl getURL() const;
		void setURL(const KUrl& url);
		void sendInput(const QString& text);
		KParts::ReadOnlyPart* part();
		const QString& partName() const { return m_partname; }
		QString partCaption() const;

		QString foregroundProcess() const;
		const QString& statusBarText() const { return m_statusBarText; }
		const QString& windowCaption() const { return m_windowCaption; }
		bool hasMimeType(const QString& type, const KUrl& url);
		QString partIcon() const;
		bool isModified() const;
		QString closeModifiedMsg() const;

	signals:
		void partCreated();
		void setStatusBarText(QString);
		void setWindowCaption(QString);
		void openUrlRequest(KUrl);
		void openUrlNotify();
		void popupMenu(QPoint, KFileItemList, KParts::BrowserExtension::PopupFlags flags, KParts::BrowserExtension::ActionGroupMap);
		void setLocationBarUrl(QString);
		void createNewWindow(KUrl, QString, KParts::ReadOnlyPart** target);

	public slots:
		void show();
		void settingsChanged();
		void partDestroyed();
		void updateUrl();

	protected slots:
		void createPart();
		void slotPopupMenu(const QPoint& where, const KUrl& url, mode_t mode, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs, KParts::BrowserExtension::PopupFlags flags, const KParts::BrowserExtension::ActionGroupMap& map);
		void slotPopupMenu(const QPoint& where, const KFileItemList& items, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs, KParts::BrowserExtension::PopupFlags flags, const KParts::BrowserExtension::ActionGroupMap& map);
		void selectionInfo(const KFileItemList& items);
		void openUrlRequest(const KUrl& url, KParts::OpenUrlArguments, KParts::BrowserArguments);
		void slotCreateNewWindow(const KUrl& url, const KParts::OpenUrlArguments& args, KParts::BrowserArguments, KParts::WindowArgs, KParts::ReadOnlyPart** target);
		void enableAction(const char* action, bool enable);
		void slotSetStatusBarText(const QString& text);
		void slotSetWindowCaption(const QString& text);
		void slotOpenUrlNotify();

	private:
		void setupUi();
		void setupPart();

		QString m_partname;
		QBoxLayout* m_layout;
		QToolBar* m_toolbar;
		KParts::ReadOnlyPart* m_part;
		QString m_statusBarText;
		QString m_windowCaption;
		KParts::PartManager& m_partManager;
		KParts::BrowserInterface* m_browserInterface;

		// update working dir for konsolepart
		QTimer* m_updateUrlTimer;
		QString m_workingDir;
};

#endif // QKVIEW_H
