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

#ifndef PROCESSPARSER_H
#define PROCESSPARSER_H

#include <qobject.h>
#include <qptrdict.h>

class KProcess;
class StreamExec;

/**
 * Parse process output
 */

class ProcessParser : QObject
{
	Q_OBJECT
public:
	ProcessParser();
	virtual ~ProcessParser();

	void insert(StreamExec *, KProcess *);

public slots:
	void streamExited(KProcess * proc);
	void receivedStdout(KProcess *proc, char *buffer, int buflen);
	void receivedStderr(KProcess *proc, char *buffer, int buflen);

private:
	QPtrDict<StreamExec> dict;
};

#endif
