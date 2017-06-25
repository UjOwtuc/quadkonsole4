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

#include "qkurlhandler.h"

#include <KDE/KService>
#include <KDE/KLocale>
#include <KDE/KFileItem>
#include <KDE/KUriFilter>
#include <KDE/KIO/StatJob>
#include <KDE/KIO/TransferJob>
#include <KDE/KIO/Scheduler>

#include <QtCore/QFile>
#include <QtCore/QTimer>


QKUrlHandler::QKUrlHandler(const QUrl& url, bool autorun, QObject* parent)
	: QObject(parent),
	m_url(url)
{
	if (autorun)
		QTimer::singleShot(1, this, SLOT(run()));
}


QKUrlHandler::~QKUrlHandler()
{}


const QUrl& QKUrlHandler::url() const
{
	return m_url;
}


const QString& QKUrlHandler::mimetype() const
{
	return m_mimetype;
}


const QString& QKUrlHandler::error() const
{
	return m_error;
}


const QString& QKUrlHandler::partName() const
{
	return m_partName;
}


void QKUrlHandler::run()
{
	QString rawUrl = m_url.url();
	// how about a setting for prefix?
	if (rawUrl.startsWith("part:"))
	{
		QString name = rawUrl.mid(5);
		int pos = name.indexOf(":");
		if (pos > -1)
		{
			rawUrl = name.mid(pos +1);
			name = name.left(pos);
		}
		else
		{
			m_url = "";
			rawUrl = "";
		}

		QStringList matches;
		matches << name + ".desktop" << name + "part.desktop" << name + "_part.desktop";
		KService::List services = KService::allServices();
		KService::List::const_iterator it;
		for (it=services.constBegin(); it!=services.constEnd(); ++it)
		{
			KService::Ptr s = *it;
			if (s->serviceTypes().contains("KParts/ReadOnlyPart", Qt::CaseInsensitive) && matches.contains(s->entryPath()))
			{
				// someone wants to handle multiple matches?
				m_partName = s->entryPath();
				break;
			}
		}

		if (m_partName.isEmpty())
		{
			m_error = i18n("Could not find a part matching '%1'", name);
			emit finished(this);
		}
	}

	if (! rawUrl.isEmpty())
	{
		KUriFilterData data(rawUrl);
		KUriFilter::self()->filterUri(data);
		m_url = data.uri();

		KIO::JobFlags flags = KIO::HideProgressInfo;
		KIO::StatJob* stat = KIO::stat(m_url, KIO::StatJob::SourceSide, 0, flags);
		connect(stat, SIGNAL(result(KJob*)), SLOT(slotStatResult(KJob*)));
	}
	else
		emit finished(this);
}


void QKUrlHandler::slotFoundMimetype(KIO::Job* job, const QString& mimetype)
{
	m_mimetype = mimetype;
	KIO::TransferJob* transfer = qobject_cast<KIO::TransferJob*>(job);
	if (transfer)
	{
		transfer->disconnect(SIGNAL(mimetype(KIO::Job*,QString)), this);
		transfer->putOnHold();
		KIO::Scheduler::publishSlaveOnHold();
	}
	emit finished(this);
}


void QKUrlHandler::slotJobFinished(KJob* job)
{
	if (job->error())
	{
		m_error = job->errorString();
		emit finished(this);
	}
}


void QKUrlHandler::slotStatResult(KJob* job)
{
	KIO::StatJob* stat = qobject_cast<KIO::StatJob*>(job);
	if (stat)
	{
		if (stat->statResult().isDir())
		{
			m_mimetype = "inode/directory";
			emit finished(this);
		}
		else if (m_url.isLocalFile())
		{
			if (QFile(m_url.url()).exists())
			{
				KFileItem item(stat->statResult(), m_url);
				m_mimetype = item.mimetype();
			}
			else
				m_error = i18n("no such file or directory");
			emit finished(this);
		}
		else
		{
			KIO::JobFlags flags = KIO::HideProgressInfo;
			KIO::TransferJob* transfer = KIO::get(m_url, KIO::NoReload, flags);
			connect(transfer, SIGNAL(mimetype(KIO::Job*,QString)), SLOT(slotFoundMimetype(KIO::Job*,QString)));
			connect(transfer, SIGNAL(result(KJob*)), SLOT(slotJobFinished(KJob*)));
		}
	}
}
