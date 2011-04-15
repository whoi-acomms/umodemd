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

#ifndef LITEPROFILE_H
#define LITEPROFILE_H

// UDP-Lite

#include "baseprofile.h"

class LiteProfile : public BaseProfile
{
public:
	LiteProfile();
	virtual ~LiteProfile();

	void client(const char * hostname, int coverage, int frequency,
		int package_length, bool random_length);

	void server();

protected:
	virtual int generatePackage(uint8_t * buf, int size, int length, bool rnd_size);
	virtual int socketType();

	virtual void getPackageInfo(uint8_t * buf, int size);

private:
	int i_src_port;
	int i_dest_port;
	int i_coverage;
};

#endif
