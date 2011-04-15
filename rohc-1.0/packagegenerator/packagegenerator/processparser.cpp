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

#include <kprocess.h>
#include "streamexec.h"
#include "processparser.h"

ProcessParser::ProcessParser()
	: QObject(0, "ProcsessParser")
{
}

ProcessParser::~ProcessParser()
{
}

void ProcessParser::insert(StreamExec * se, KProcess * proc)
{
	dict.insert(proc, se);

	connect(proc, SIGNAL(processExited(KProcess *)),
		this, SLOT(streamExited(KProcess *)));
	connect(proc, SIGNAL(receivedStdout(KProcess *, char *, int)),
		this, SLOT(receivedStdout(KProcess *, char *, int)));
	connect(proc, SIGNAL(receivedStderr(KProcess *, char *, int)),
		this, SLOT(receivedStderr(KProcess *, char *, int)));
}

void ProcessParser::streamExited(KProcess * proc)
{
	StreamExec * ex = dict.find(proc);
	dict.remove(proc);
	ex->closed();
	delete ex; // Update GUI
}

void ProcessParser::receivedStdout(KProcess *proc, char *buffer, int buflen)
{
	QString buf = QString::fromLocal8Bit(buffer, buflen);
	qDebug("received stdout %p : \"%s\"\n", (void *)proc, buf.ascii());
}

void ProcessParser::receivedStderr(KProcess *proc, char *buffer, int buflen)
{
	StreamExec * ex = dict.find(proc);
	QString buf = QString::fromLocal8Bit(buffer, buflen);
	if (!ex) {
		qDebug("Unknown stream %p, buf = \"%s\"\n", (void *)proc, buf.ascii());
		return;
	}
	ex->newBufferData(buf);
}
