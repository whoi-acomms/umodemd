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

#ifndef STREAMEXEC_H
#define STREAMEXEC_H

#include <klistview.h>
#include <kprocess.h>

class ProcessParser;

/**
  * An item in the listview
  *@author Martin Juhlin
  */

class StreamExec : public KListViewItem
{
public:
	enum Type {UDPClient, UDPServer, LiteClient, LiteServer,
		MultiIpClient, MultiIpServer};

	StreamExec(KListView * parent, ProcessParser *, Type, int port, int frequency);
	virtual ~StreamExec();

	void udpClient(int port, QString host, int frequency,
		int length, bool random_length);
	void liteClient(QString host, int frequency,
		int length, bool random_length, int coverage);
	void multiClient(QString host, int frequency, QString firstFile, bool reloop);
	void udpServer(int port);
	void liteServer();
	void multiServer();

	// To terminate the process
	void terminate();

	// Called by ProcessParser then the process is closed
	void closed();

	// New bufferdata from the process, called by ProcsesParser
	void newBufferData(const QString & new_buf_data);

	static void setAppFileName(const char *);
private:
	QString buf;
	KProcess * m_process;
	static QString appFileName;
	ProcessParser * m_pp;

private:
	void defaultText(Type type, int port, int frequency);

	QString displayName(Type type, int port);
	
	KProcess * process();

	void setInfo(int total, int lost, int disorder);

	void start(KProcess::Communication com = KProcess::Stderr);
};

#endif
