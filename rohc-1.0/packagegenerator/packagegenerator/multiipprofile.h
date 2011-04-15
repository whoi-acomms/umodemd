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

#ifndef MULTIIPPROFILE_H
#define MULTIIPPROFILE_H

#include <qstring.h>
#include "baseprofile.h"

/**
  *@author Martin Juhlin
  */

class MultiIpProfile : public BaseProfile  {
public: 
	MultiIpProfile();
	virtual ~MultiIpProfile();

	void client(QString firstFile, const char * hostname, int frequency, bool reloop);
	void server();
	
protected:
	virtual int generatePackage(uint8_t * buf, int size, int length, bool rnd_size);
	void getPackageInfo(uint8_t * buf, int size);
	virtual int socketType();
	
private:
	bool i_reloop;
	QString firstFile;
	QString currentFile;

private:
	QString getCurrentFileName();
	int loadFile(QString filename, uint8_t * toBuf, int max_size);
	int loadNextFile(uint8_t * buf, int max_size);
};

#endif
