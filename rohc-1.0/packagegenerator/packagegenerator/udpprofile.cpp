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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "udpprofile.h"

#define BUF_SIZE (20 * 1024)

UdpProfile::UdpProfile()
{
}

UdpProfile::~UdpProfile()
{
}

void UdpProfile::client(const char * hostname, int port, int frequency,
	int package_length, bool random_length)
{
	int sec, usec;

	int socket = createClientSocket(port, hostname);
	calculateSleepTime(frequency, &sec, &usec);

	for (;;) {
		//printf("Sending package...\n");
		sendPackage(socket, package_length, random_length);

		//printf("Sleeping\n");
		if (sec > 0) sleep(sec);
		if (usec > 20) usleep(usec);
	}

	//close(socket);
}

void UdpProfile::server(int port)
{
	uint8_t buffer[BUF_SIZE];
	int count = 0, size;
	int socket = createServerSocket(port);

	for (;;) {
		//printf("Waiting for package\n");
		size = recvfrom(socket, buffer, BUF_SIZE, 0, 0, 0);

		getPackageInfo(buffer, size);

		if(timeToPrint(count)) {
			fprintf(stderr, "tot: %d lost: %d disorder: %d\n",
				i_lastNum, i_lost, i_disorder);
		}
	}
}

int UdpProfile::socketType()
{
	return socket(PF_INET, SOCK_DGRAM, 0);
}
