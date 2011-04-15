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

#ifndef BASEPROFILE_H
#define BASEPROFILE_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <time.h>
#include <stdint.h>

/**
  *@author Martin Juhlin
  */

class BaseProfile {
public: 
	BaseProfile();
	virtual ~BaseProfile();

	void udp_client(const char * hostname, int port, int frequency,
		int package_length, bool random_length);
	
	void udp_server(int port);
	
protected:
	// Package generator
	int current_number;

	// Socket connector
	struct sockaddr_in cli2_addr, serv2_addr;


	// statistic for server
	int i_lastNum;
	int i_lost;
	int i_disorder;

protected:
	void calculateSleepTime(int package_per_min, int * sec, int * usec);

	virtual int socketType() = 0;

	int createClientSocket(int port, const char * hostname);
	virtual int generatePackage(uint8_t * buf, int buf_size, int pkg_size, bool rnd_pkg_size);
	void sendPackage(int socket, int package_size, bool rnd_size);

	int createServerSocket (uint16_t port);
	virtual void getPackageInfo(uint8_t * buf, int size);
	bool timeToPrint(int & count);

private:
	// To timeToPrint
	int calibrating, print_limit;
	time_t first, current;
};

#endif
