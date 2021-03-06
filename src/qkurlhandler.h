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

#ifndef QKURLHANDLER_H
#define QKURLHANDLER_H

#include <QtCore/QObject>
#include <QtCore/QUrl>

class KJob;
namespace KIO
{
	class Job;
}

class QKUrlHandler : public QObject
{
	Q_OBJECT
	public:
		explicit QKUrlHandler(const QUrl& url, bool autorun=true, QObject* parent = 0);
		virtual ~QKUrlHandler();

		const QUrl& url() const;
		const QString& mimetype() const;
		const QString& error() const;
		const QString& partName() const;

	signals:
		void finished(QKUrlHandler*);

	public slots:
		void run();

	private slots:
		void slotFoundMimetype(KIO::Job* job, const QString& mimetype);
		void slotStatResult(KJob* job);
		void slotJobFinished(KJob* job);

	private:
		QUrl m_url;
		QString m_mimetype;
		QString m_error;
		QString m_partName;
};

#endif // QKURLHANDLER_H
