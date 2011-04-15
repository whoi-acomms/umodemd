/*
 * Package generator for generating UDP and TCP flows
 * Copyright (C) 2003  Martin Juhlin <juhlin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <kcmdlineargs.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include "processparser.h"
#include "streamexec.h"

QString StreamExec::appFileName;

StreamExec::StreamExec(KListView * parent, ProcessParser * pp,
	Type type, int port, int frequency)
	: KListViewItem(parent)
{
	m_process = 0;
	m_pp = pp;

	defaultText(type, port, frequency);
}

StreamExec::~StreamExec()
{
	if (m_process) terminate();
}

void StreamExec::closed()
{
	if (!m_process) return;
	delete m_process;
	m_process = 0;
}

void StreamExec::defaultText(Type type, int port, int frequency)
{
	setText(0, displayName(type, port));

	QString freq;
	if (frequency < 1) {
		freq = "";
	} else if (frequency < 61) {
		freq = QString::number(frequency) + " pkg/min";
	} else {
		freq = QString::number(frequency / 60) + " pkg/sec";
	}
	setText(1, freq);

	switch(type) {
	case LiteClient:
	case MultiIpClient:
	case UDPClient:
		setText(2, "-");
		setText(3, "-");
		setText(4, "-");
		setText(5, "-");
		break;
	case LiteServer:
	case MultiIpServer:
	case UDPServer:
		setText(2, "0");
		setText(3, "0");
		setText(4, "0");
		setText(5, "0%");
		break;
	}
}

QString StreamExec::displayName(Type type, int port)
{
	switch(type) {
	case UDPClient:
		return "UDP Client:" + QString::number(port);
	case UDPServer:
		return "UDP Server:" + QString::number(port);
	case LiteClient:
		return "UDP-Lite Client";
	case LiteServer:
		return "UDP-Lite Server";
	case MultiIpClient:
		return "Multi-IP Client";
	case MultiIpServer:
		return "Multi-IP Server";
	}
	return "<unknown>";
}

void StreamExec::liteClient(
	QString host, int frequency,
	int length, bool random_length, int coverage)
{
	KProcess * m_process = process();

	QString slength = QString::number(length);
	QString sfrequency = QString::number(frequency);
	QString scoverage = QString::number(coverage);
	
	*m_process << "--lite" << scoverage << "--host" << host
		<< "--freq" << sfrequency
		<< "--length" << slength;
	if (random_length)
		*m_process << "--random";

	start();
}

void StreamExec::liteServer()
{
	KProcess * m_process = process();
	*m_process << "--servlite";
	start();
}

void StreamExec::multiClient(QString host, int frequency, QString firstFile, bool reloop)
{
	KProcess * m_process = process();
	QString sfrequency = QString::number(frequency);

	*m_process << "--multi" << firstFile << "--host" << host
			<< "--freq" << sfrequency;
	if (reloop) *m_process << "--reloop";

	start();
}

void StreamExec::multiServer()
{
	KProcess * m_process = process();
	*m_process << "--servmulti";
	start();
}

void StreamExec::newBufferData(const QString & new_buf_data)
{
	bool updated = false;
	int total = 0, lost = 0, disorder = 0, pos;
	buf += new_buf_data;
	while( (pos = buf.find('\n')) > 0 ) {
		QString s = buf.left(pos);
		buf.remove(0, pos + 1);

		if (s.left(5) != "tot: ") continue;

		// extract total
                s.remove(0, 5);
		total = s.left( s.find('l') ).stripWhiteSpace().toInt();

		// extract lost
		s.remove(0, s.find(':') + 2);
		lost = s.left( s.find('d') ).stripWhiteSpace().toInt();

		// extract reorded
		s = s.remove(0, s.find(':') + 2).stripWhiteSpace();
		disorder = s.toInt();

		qDebug("disorder = %d \"%s\"\n", disorder, s.ascii());

		updated = true;
	}
	if (updated) setInfo(total, lost, disorder);
}

KProcess * StreamExec::process()
{
	m_process = new KProcess;

	*m_process << appFileName;

	return m_process;
}

void StreamExec::setAppFileName(const char * s)
{
	appFileName = s;
}

void StreamExec::setInfo(int total, int lost, int disorder)
{
	setText(2, QString::number(total));
	if (lost < 0) {
		setText(3, "Unknown, two flows?");
	} else {
		setText(3, QString::number(lost));
	}
	setText(4, QString::number(disorder));

	if (total > 0 && lost >= 0) {
		double d = lost;
		d /= total;
		d *= 100.0;		
		setText(5, QString::number(d, 'f', 1));
		
	} else {
		setText(5, "-");
	}
}

void StreamExec::start(KProcess::Communication com)
{
	bool ret = m_process->start(KProcess::NotifyOnExit, com);
	if (ret == false) {
		KMessageBox::error(0, "Can't start subprocess");
		delete m_process;
		m_process = 0;
		return;
	}
	m_pp->insert(this, m_process);
}

void StreamExec::terminate()
{
	if (!m_process) return;
	
	KProcess * p = m_process;
	m_process = 0; // Eliminate recalling from ProcessParser

	p->kill();

	delete p;
}

void StreamExec::udpClient(
	int port, QString host, int frequency,
	int length, bool random_length)
{
	KProcess * m_process = process();

	QString sport = QString::number(port);
	QString slength = QString::number(length);
	QString sfrequency = QString::number(frequency);

	*m_process << "--udp" << sport << "--host" << host
		<< "--freq" << sfrequency
		<< "--length" << slength;
	if (random_length)
		*m_process << "--random";

	start();
}

void StreamExec::udpServer(int port)
{
	KProcess * m_process = process();

	QString sport = QString::number(port);
	*m_process << "--servudp" << sport;

	start();
}
