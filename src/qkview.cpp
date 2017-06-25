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

#include "qkview.h"
#include "qkstack.h"
#include "settings.h"

#include <KDE/KStandardDirs>
#include <KDE/KToggleAction>
#include <KDE/KXmlGuiWindow>
#include <KDE/KXMLGUIFactory>
#include <KDE/KDebug>
#include <KDE/KService>
#include <KDE/KMessageBox>
#include <KDE/KActionCollection>
#include <KDE/KFileItemList>
#include <KDE/KStatusBar>
#include <KDE/KShell>
#include <KDE/KIO/Job>
#include <KDE/KParts/ReadOnlyPart>
#include <KDE/KParts/BrowserExtension>
#include <KDE/KParts/PartManager>
#include <KDE/KParts/StatusBarExtension>
#include <kde_terminal_interface.h>

#include <KLocalizedString>

#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtCore/QDir>
#include <QtWidgets/QWidget>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QApplication>
#include <QtWidgets/QProgressBar>


QMap<QString, KService::Ptr> QKPartFactory::m_partFactories;
QStringList QKView::m_removeKonsoleActions;


KService::Ptr QKPartFactory::getFactory(const QString& name)
{
	if (m_partFactories.count(name) == 0)
	{
		kDebug() << "creating factory for" << name << endl;
		m_partFactories[name] = KService::serviceByDesktopPath(name);
		if (! m_partFactories[name])
		{
			KMessageBox::sorry(0, i18n("Could not create a factory for %1.", name));
			return QExplicitlySharedDataPointer<KService>();
		}
	}
	return m_partFactories[name];
}


QKView::QKView(KParts::PartManager& partManager, KParts::BrowserInterface* browserInterface, const QString& partname, QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f),
	m_partname(partname),
	m_part(0),
	m_writablePart(0),
	m_partManager(partManager),
	m_browserInterface(browserInterface),
	m_updateUrlTimer(0),
	m_workingDir(QString()),
	m_progress(0)
{
	setupUi();
}


QKView::QKView(KParts::PartManager& partManager, KParts::BrowserInterface* browserInterface, KParts::ReadOnlyPart* part, QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f),
	m_part(part),
	m_writablePart(0),
	m_partManager(partManager),
	m_browserInterface(browserInterface),
	m_updateUrlTimer(0),
	m_workingDir(QString()),
	m_progress(0)
{
	part->setParent(this);
	m_partname = part->property("QKPartName").toString();
	setupUi();
}


QKView::~QKView()
{}


QUrl QKView::getURL() const
{
	if (m_part)
	{
		TerminalInterface* t = qobject_cast<TerminalInterface*>(m_part);
		if (t)
		{
			int pid = t->terminalProcessId();
			QFileInfo info(QString("/proc/%1/cwd").arg(pid));
			return info.readLink();
		}
		else
			return m_part->url();
	}
	return QUrl();
}


void QKView::setURL(const QUrl& url)
{
	if (m_part && getURL() != url)
	{
		TerminalInterface* t = qobject_cast<TerminalInterface*>(m_part);
		if (t)
		{
			QString path;
			if (url.isLocalFile())
				path = url.path();
			else
				path = url.url();

			QString escaped = KShell::quoteArg(path);

			if (! url.host().isEmpty() && QFileInfo(url.path()).isDir())
			{
				if (t->foregroundProcessName().isEmpty())
				{
					t->sendInput(QString("cd %1\n").arg(escaped));
					slotOpenUrlNotify();
				}
				else
					t->sendInput(QString("cd %1").arg(escaped));
			}
			else
				t->sendInput(QString(" %1").arg(escaped));
		}
		else
			m_part->openUrl(url);

		m_windowCaption = url.url();
		emit setWindowCaption(m_windowCaption);
	}
}


void QKView::sendInput(const QString& text)
{
	TerminalInterface* t;
	if (m_part && (t = qobject_cast<TerminalInterface*>(m_part)))
		t->sendInput(text);
	else
		kDebug() << "don't know how to send input to my part" << endl;
}


KParts::ReadOnlyPart* QKView::part()
{
	return m_part;
}


QString QKView::partCaption() const
{
	KService::Ptr service = QKPartFactory::getFactory(m_partname);
	if (service && ! service->name().isEmpty())
		return service->name();

	// fallback to names like "konsolepart.desktop". still better than a tab without text
	return m_partname;
}


QString QKView::foregroundProcess() const
{
	TerminalInterface* t;
	if (m_part && (t = qobject_cast<TerminalInterface*>(m_part)))
		return t->foregroundProcessName();

	// emulate some kind of "process" for other KParts
	return getURL().url();
}


bool QKView::hasMimeType(const QString& type, const QUrl& url)
{
	if (m_partname == "konsolepart.desktop" && url.scheme() != "file")
		return false;

	KService::Ptr service = QKPartFactory::getFactory(m_partname);
	if (! service)
		return false;

	if (service->hasMimeType(type))
		return true;
	kDebug() << "KPart" << m_partname << "does not like mime type" << type << endl;
	return false;
}


QString QKView::partIcon() const
{
	KService::Ptr service = QKPartFactory::getFactory(m_partname);
	if (service)
		return service->icon();
	return "";
}


bool QKView::isModified() const
{
	TerminalInterface* t = qobject_cast<TerminalInterface*>(m_part);
	if (t)
	{
		if (t->foregroundProcessName().size())
			return true;
	}
	else if (m_writablePart && m_writablePart->isModified())
	{
		return true;
	}
	else if (m_part && (m_part->metaObject()->indexOfProperty("modified") != 1))
	{
		const QVariant prop = m_part->property("modified");
		return prop.isValid() && prop.toBool();
	}
	return false;
}


QString QKView::closeModifiedMsg() const
{
	QString msg;

	TerminalInterface* t = qobject_cast<TerminalInterface*>(m_part);
	if (t)
		msg = i18n("The process \"%1\" is still running. Do you want to terminate it?", foregroundProcess());
	else
		msg = i18n("The view \"%1\" contains unsaved changes at \"%2\". Do you want to close it without saving?", partCaption(), getURL().url());

	return msg;
}


const QList<QAction*>& QKView::pluggableSettingsActions() const
{
	return m_settingsActions;
}


const QList< QAction* >& QKView::pluggableEditActions() const
{
	return m_editActions;
}


void QKView::show()
{
	QWidget::show();
	if (m_part == 0)
	{
		createPart();
		m_partManager.setActivePart(m_part);
		m_partManager.setSelectedPart(m_part);
	}
}


void QKView::settingsChanged()
{
	if (Settings::viewHasToolbar())
		m_toolbar->show();
	else
		m_toolbar->hide();
}


void QKView::createPart()
{
	KService::Ptr service = QKPartFactory::getFactory(m_partname);
	if (! service)
		return;

	m_editActions.clear();
	m_settingsActions.clear();

	QString error;
	if (service->serviceTypes().contains("KParts/ReadWritePart"))
	{
		kDebug() << "part" << m_partname << "has a ReadWritePart" << endl;

		m_writablePart = service->createInstance<KParts::ReadWritePart>(this, this, QVariantList(), &error);
		// m_writablePart->setReadWrite(false);
		m_part = m_writablePart;

		// KToggleAction* toggleEditable = new KToggleAction(KIcon("document-edit"), i18n("&Enable editing"), this);
		// connect(toggleEditable, SIGNAL(toggled(bool)), SLOT(slotToggleEditable(bool)));
		// m_editActions.append(toggleEditable);
	}
	else
		m_part = service->createInstance<KParts::ReadOnlyPart>(this, this, QVariantList(), &error);
	if (m_part == 0)
	{
		KMessageBox::error(this, i18n("The factory for %1 could not create a KPart: %2", m_partname, error));
		return;
	}
	m_part->setProperty("QKPartName", m_partname);
	m_part->widget()->setFocusPolicy(Qt::WheelFocus);

	setupPart();
	emit partCreated();
}


void QKView::partDestroyed()
{
	m_statusBarText.clear();
	m_windowCaption.clear();

	m_progress->hide();
	if (m_part)
	{
		KXmlGuiWindow* win = qobject_cast<KXmlGuiWindow*>(window());
		if (win)
			win->guiFactory()->resetContainer("session-popup-menu");

		m_partManager.removePart(m_part);
		m_part->disconnect();
		if (m_part->widget())
		{
			// part was detached
			KParts::BrowserExtension* b = KParts::BrowserExtension::childObject(m_part);
			if (b)
			{
				b->setBrowserInterface(0);
				b->disconnect(SIGNAL(loadingProgress(int)), m_progress);
				b->disconnect(SIGNAL(popupMenu(QPoint,KUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)), this);
				b->disconnect(SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)), this);
				b->disconnect(SIGNAL(selectionInfo(KFileItemList)), this);
				b->disconnect(SIGNAL(openUrlRequestDelayed(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)), this);
				b->disconnect(SIGNAL(openUrlNotify()), this);
				b->disconnect(SIGNAL(enableAction(const char*,bool)), this);
				b->disconnect(SIGNAL(setLocationBarUrl(QString)), this);
				b->disconnect(SIGNAL(createNewWindow(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::WindowArgs,KParts::ReadOnlyPart**)), this);

			}
			m_partManager.removeManagedTopLevelWidget(m_part->widget());
			disconnect(m_part->widget(), SIGNAL(destroyed(QObject*)), this, SLOT(partDestroyed()));
		}
	}
	createPart();
}


void QKView::updateUrl()
{
	if (hasFocus() && m_workingDir != getURL().url())
	{
		m_workingDir = getURL().url();
		emit openUrlNotify();
	}
}


void QKView::slotPopupMenu(const QPoint& where, const QUrl &url, mode_t mode, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs, KParts::BrowserExtension::PopupFlags flags, const KParts::BrowserExtension::ActionGroupMap& map)
{
	KFileItem item(url, args.mimeType(), mode);
	KFileItemList list;
	list.append(item);
	slotPopupMenu(where, list, args, browserArgs, flags, map);
}


void QKView::slotPopupMenu(const QPoint& where, const KFileItemList& items, const KParts::OpenUrlArguments& /*args*/, const KParts::BrowserArguments& /*browserArgs*/, KParts::BrowserExtension::PopupFlags flags, const KParts::BrowserExtension::ActionGroupMap& map)
{
	emit popupMenu(where, items, flags, map);
}


void QKView::selectionInfo(const KFileItemList& items)
{
	kDebug() << "selected items: " << items << endl;
	if (items.count() == 1)
		m_statusBarText = items.at(0).getStatusBarInfo();
	else
	{
		KIO::filesize_t size = 0;
		KFileItemList::const_iterator it;
		for (it=items.constBegin(); it!=items.constEnd(); ++it)
			size += it->size();

		m_statusBarText = i18n("%1 files selected (%2)", items.size(), size);
	}
	emit setStatusBarText(m_statusBarText);
}


void QKView::openUrlRequest(const QUrl& url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs)
{
	kDebug() << "url" << url << endl;
	if (browserArgs.newTab() || browserArgs.forcesNewWindow())
		emit createNewWindow(url, args.mimeType(), 0);
	else
		emit openUrlRequest(url);
}


void QKView::slotCreateNewWindow(const QUrl& url, const KParts::OpenUrlArguments& args, KParts::BrowserArguments, KParts::WindowArgs, KParts::ReadOnlyPart** target)
{
	kDebug() << "url" << url << endl;
	emit createNewWindow(url, args.mimeType(), target);
}


void QKView::enableAction(const char* action, bool enable)
{
	kDebug() << action << enable << endl;
	KXmlGuiWindow* window = qobject_cast<KXmlGuiWindow*>(m_partManager.parent());
	if (window)
	{
		QAction* a = window->actionCollection()->action(action);
		if (a)
			a->setEnabled(enable);
	}
}


void QKView::slotSetStatusBarText(const QString& text)
{
	m_statusBarText = text;
	emit setStatusBarText(text);
}


void QKView::slotSetWindowCaption(const QString& text)
{
	m_windowCaption = text;
	emit setWindowCaption(text);
}


void QKView::slotOpenUrlNotify()
{
	QTimer::singleShot(10, this, SIGNAL(openUrlNotify()));
}


void QKView::slotJobStarted(KIO::Job* job)
{
	m_progress->show();
	m_progress->setValue(0);
	if (job)
	{
		connect(job, SIGNAL(percent(KJob*,ulong)), SLOT(slotProgress(KIO::Job*,ulong)));
		connect(job, SIGNAL(finished(KJob*)), SLOT(slotJobFinished()));
		connect(job, SIGNAL(destroyed(QObject*)), SLOT(slotJobFinished()));
	}
}


void QKView::slotProgress(KIO::Job* , ulong percent)
{
	kDebug() << percent << "%" << endl;
	m_progress->setValue(percent);
}


void QKView::slotJobFinished()
{
	m_progress->hide();
}


void QKView::slotToggleEditable(bool set)
{
	if (m_writablePart)
		m_writablePart->setReadWrite(set);
}


void QKView::setupUi()
{
	setContentsMargins(0, 0, 0, 0);

	m_layout = new QBoxLayout(QBoxLayout::TopToBottom, this);

	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0, 0, 0, 0);

	m_toolbar = new QToolBar;
	m_layout->addWidget(m_toolbar);

	m_progress = new QProgressBar;
	connect(this, SIGNAL(destroyed(QObject*)), m_progress, SLOT(deleteLater()));
	m_progress->setRange(0, 100);
	KXmlGuiWindow* window = qobject_cast<KXmlGuiWindow*>(m_partManager.parent());
	if (window)
		window->statusBar()->addWidget(m_progress);
	m_progress->hide();


	if (m_part)
		setupPart();

	if (! Settings::viewHasToolbar())
		m_toolbar->hide();
	connect(Settings::self(), SIGNAL(configChanged()), SLOT(settingsChanged()));
}


void QKView::setupPart()
{
	m_layout->addWidget(m_part->widget());
	setFocusProxy(m_part->widget());

	m_part->widget()->setFocus();

	connect(m_part->widget(), SIGNAL(destroyed()), SLOT(partDestroyed()));
	connect(m_part, SIGNAL(setStatusBarText(QString)), SLOT(slotSetStatusBarText(QString)));
	connect(m_part, SIGNAL(setWindowCaption(QString)), SLOT(slotSetWindowCaption(QString)));
	connect(m_part, SIGNAL(started(KIO::Job*)), SLOT(slotJobStarted(KIO::Job*)));
	connect(m_part, SIGNAL(canceled(QString)), m_progress, SLOT(hide()));
	connect(m_part, SIGNAL(completed()), m_progress, SLOT(hide()));
	connect(m_part, SIGNAL(completed(bool)), m_progress, SLOT(hide()));

	KXmlGuiWindow* window = qobject_cast<KXmlGuiWindow*>(m_partManager.parent());

	TerminalInterface* t = qobject_cast<TerminalInterface*>(m_part);
	if (t)
	{
		kDebug() << "part" << m_partname << "has a TerminalInterface" << endl;
		t->showShellInDir(QString());
		m_updateUrlTimer = new QTimer(this);
		m_updateUrlTimer->start(1500);
		connect(m_updateUrlTimer, SIGNAL(timeout()), SLOT(updateUrl()));

		disableKonsoleActions();
	}

	KParts::BrowserExtension* b = KParts::BrowserExtension::childObject(m_part);
	if (b)
	{
		kDebug() << "part" << m_partname << "has a BrowserExtension" << endl;
		b->setBrowserInterface(m_browserInterface);

		connect(b, SIGNAL(loadingProgress(int)), m_progress, SLOT(setValue(int)));
		connect(b, SIGNAL(popupMenu(QPoint,KUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)), SLOT(slotPopupMenu(QPoint,KUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)));
		connect(b, SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)), SLOT(slotPopupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)));
		connect(b, SIGNAL(selectionInfo(KFileItemList)), SLOT(selectionInfo(KFileItemList)));
		connect(b, SIGNAL(openUrlRequestDelayed(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)), SLOT(openUrlRequest(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)));
		connect(b, SIGNAL(openUrlNotify()), SLOT (slotOpenUrlNotify()));
		connect(b, SIGNAL(enableAction(const char*,bool)), SLOT(enableAction(const char*,bool)));
		connect(b, SIGNAL(setLocationBarUrl(QString)), SIGNAL(setLocationBarUrl(QString)));
		connect(b, SIGNAL(createNewWindow(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::WindowArgs,KParts::ReadOnlyPart**)), SLOT(slotCreateNewWindow(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::WindowArgs,KParts::ReadOnlyPart**)));
	}

	KParts::StatusBarExtension* sb = KParts::StatusBarExtension::childObject(m_part);
	if (sb)
	{
		kDebug() << "part" << m_partname << "has a StatusBarExtension" << endl;
		if (window)
			sb->setStatusBar(window->statusBar());
	}
	m_partManager.addPart(m_part);
}


void QKView::disableKonsoleActions()
{
	KActionCollection* ac = m_part->actionCollection();

	if (m_removeKonsoleActions.isEmpty())
	{
		m_removeKonsoleActions << "split-view-left-right"
			<< "split-view-top-bottom"
			<< "close-active-view"
			<< "close-other-views"
			<< "detach-view"
			<< "expand-active-view"
			<< "shrink-active-view"
			<< "next-view"
			<< "previous-view"
			<< "next-container"
			<< "move-view-left"
			<< "move-view-right"
			<< "switch-to-tab-0"
			<< "switch-to-tab-1"
			<< "switch-to-tab-2"
			<< "switch-to-tab-3"
			<< "switch-to-tab-4"
			<< "switch-to-tab-5"
			<< "switch-to-tab-6"
			<< "switch-to-tab-7"
			<< "switch-to-tab-8"
			<< "switch-to-tab-9";
	}

	QStringListIterator it(m_removeKonsoleActions);
	while (it.hasNext())
	{
		QAction* action = ac->action(it.next());
		if (action)
			ac->removeAction(action);
	}
}

#include "qkview.moc"
