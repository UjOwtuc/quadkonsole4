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

#include "qkurlhandler.h"

#include <KDE/KLocale>
#include <KDE/KFileItem>
#include <KDE/KUrl>
#include <KDE/KUriFilter>
#include <KDE/KIO/StatJob>
#include <KDE/KIO/TransferJob>
#include <KDE/KIO/Scheduler>

#include <QtCore/QFile>


QKUrlHandler::QKUrlHandler(const KUrl& url, QObject* parent)
	: QObject(parent)
{
	KUriFilterData data(url.pathOrUrl());
	KUriFilter::self()->filterUri(data);
	m_url = data.uri();

	KIO::JobFlags flags = KIO::HideProgressInfo;
	KIO::StatJob* stat = KIO::stat(m_url, true, 0, flags);
	connect(stat, SIGNAL(result(KJob*)), SLOT(slotStatResult(KJob*)));
}


QKUrlHandler::~QKUrlHandler()
{}


const KUrl& QKUrlHandler::url() const
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
			if (QFile(m_url.pathOrUrl()).exists())
			{
				KFileItem item(KFileItem::Unknown, KFileItem::Unknown, m_url);
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
