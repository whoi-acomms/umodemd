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
#include "liteprofile.h"

#define BUF_SIZE (20 * 1024)
#define UDP_LITE_PROTO 254

LiteProfile::LiteProfile()
{
	i_src_port = 4382;
	i_dest_port = 4378;
	i_coverage = 8;
}

LiteProfile::~LiteProfile()
{
}

void LiteProfile::client(const char * hostname, int cov, int frequency,
	int package_length, bool random_length)
{
	int sec, usec;
	i_coverage = cov;

	int socket;

	socket = createClientSocket(i_dest_port, hostname);
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


int LiteProfile::generatePackage(uint8_t * buf, int size, int length, bool rnd_size)
{
	if (length < 4) length = 4;
	
	*((uint16_t *)buf) = htons(i_src_port);
	((uint16_t *)buf) += 1;
	*((uint16_t *)buf) = htons(i_dest_port);
	((uint16_t *)buf) += 1;
        if (i_coverage == -1) {
        	*((uint16_t *)buf) = htons(rand() % length);
        } else {
		*((uint16_t *)buf) = htons(i_coverage);
	}
	((uint16_t *)buf) += 1;
	*((uint16_t *)buf) = htons(rand() % 0xffff); // just hate checksum
	((uint16_t *)buf) += 1;

	size -= 8;

	return BaseProfile::generatePackage(buf, size, length, rnd_size) + 8;
}


void LiteProfile::getPackageInfo(uint8_t * buf, int size)
{
	if (size < (20 + 8 + 4)) {
		i_lost++;
		return;
	}

	int s = (buf[0] & 0x0f) * 4;
	size -= s;
	if (size < 0) {
		i_lost++;
		return;
	}
	buf += s;

	int src = ntohs(*((uint16_t *)buf)); ((uint16_t *)buf) += 1;
	int dest = ntohs(*((uint16_t *)buf)); ((uint16_t *)buf) += 1;
	//int coverage = ntohs(*((uint16_t *)buf)); ((uint16_t *)buf) += 1;
	((uint16_t *)buf) += 2; // coverage + checksum

	if (src != i_src_port || dest != i_dest_port) {
		i_lost++;
		return;
	}

	size -= 8;
	BaseProfile::getPackageInfo(buf, size);
}

int LiteProfile::socketType()
{
	return socket(PF_INET, SOCK_RAW, UDP_LITE_PROTO);
}

void LiteProfile::server()
{
	uint8_t buffer[BUF_SIZE];
	int size;
	int socket = createServerSocket(4378);

	int count = 0;

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
