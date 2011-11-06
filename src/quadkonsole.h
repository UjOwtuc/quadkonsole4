/***************************************************************************
 *   Copyright (C) 2005 by Simon Perreault                                 *
 *   nomis80@nomis80.org                                                   *
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


#ifndef QUADKONSOLE_H
#define QUADKONSOLE_H

#include <KDE/KParts/MainWindow>
#include <KDE/KParts/PartManager>

#include <QtGui/QClipboard>

// dbus adaptor
class QuadKonsoleAdaptor;
class QKStack;
class QKView;
class QGridLayout;
class QSplitter;
class MouseMoveFilter;
class KHistoryComboBox;

namespace KParts
{
	class ReadOnlyPart;
}

/**
 * @short Application Main Window
 * @author Simon Perreault <nomis80@nomis80.org>
 * @version 2.0
 */
class QuadKonsole : public KParts::MainWindow
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "de.ccchl.quadkonsole4.QuadKonsole")
	public:
		QuadKonsole(int rows, int columns, const QStringList& cmds=QStringList(), const QStringList& urls=QStringList());
		QuadKonsole();
		~QuadKonsole();

	signals:
		void detached(QuadKonsole* mainWindow);

	public slots:
		Q_SCRIPTABLE uint numViews() const { return m_stacks.size(); }
		void resetLayouts();
		void toggleMenu();
		void optionsPreferences();
		void settingsChanged();
		void insertHorizontal(int row, int col);
		void insertVertical(int row, int col);
		void sendCommands(const QStringList& cmds);
		Q_SCRIPTABLE void sendInput(uint view, const QString& text);
		void openUrls(const QStringList& urls);
		Q_SCRIPTABLE void identifyStacks(QString format);
		Q_SCRIPTABLE QStringList urls() const;
		Q_SCRIPTABLE QStringList partIcons() const;
		void changeLayout();
		void slotActivateUrlBar();
		void refreshHistory(const QString& item);
		void zoomView(int row, int col);
		void slotToggleSidebar();
		void slotAutoSave();

	private slots:
		void focusKonsoleRight();
		void focusKonsoleLeft();
		void focusKonsoleUp();
		void focusKonsoleDown();
		void insertHorizontal();
		void insertVertical();
		void detach(QKStack* stack=0, QKView* view=0);
		void removeStack();
		void switchView();
		void goBack();
		void goForward();
		void goUp();
		void closeView();
		void tabLeft();
		void tabRight();
		void setStatusBarText(const QString& text);
		void setWindowCaption(const QString& text);
		void slotActivePartChanged(KParts::Part* part);
		void slotStackDestroyed(QKStack* removed=0);
		void slotSetLocationBarUrl(const QString& url);
		void zoomView();
		void slotOpenUrl(const QString& url=QString());
		void slotOpenUrl(const KUrl& url);
		void slotNewWindow(const KUrl& url, const QString& mimeType, KParts::ReadOnlyPart** target);
		void slotDuplicateView();
#ifdef DEBUG
		void saveSession();
		void restoreSession();
#endif // DEBUG

	protected:
		bool queryClose();
		QKStack* getFocusStack();
		void getFocusCoords(int& row, int& col);
		QKStack* addStack(int row, int col, KParts::ReadOnlyPart* part=0);
		void saveProperties(KConfigGroup& config);
		void readProperties(const KConfigGroup& config);

	private:
		QuadKonsole(KParts::ReadOnlyPart* part);
		void initHistory();
		void setupActions();
		void setupUi(int rows, int columns, QList<KParts::ReadOnlyPart*> parts=QList<KParts::ReadOnlyPart*>());
		void resetLayout(QSplitter* layout, int targetSize);
		static void fillMovementMap();
		void reconnectMovement();
		void zoomSplitter(QSplitter* layout, int item);

		QList<QKStack*> m_stacks;
		QSplitter* m_rows;
		QList<QSplitter*> m_rowLayouts;
		MouseMoveFilter* m_mouseFilter;
		KParts::PartManager m_partManager;
		KHistoryComboBox* m_urlBar;
		QKStack* m_activeStack;
		QPair<int, int> m_zoomed;
		QSplitter* m_sidebarSplitter;
		QKView* m_sidebar;
		QuadKonsoleAdaptor* m_dbusAdaptor;
		bool m_restoringSession;
};


/** Split urls of the form "<stack_number>:<url>"
 * \param s URL to examine
 * \param index index contained in the URL or -1
 * \return URL part of the given string
 */
QString splitIndex(const QString& s, int* index);

#endif // QUADKONSOLE_H
