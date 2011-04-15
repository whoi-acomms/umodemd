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
#include "baseprofile.h"

#define BUF_SIZE (20 * 1024)

BaseProfile::BaseProfile()
{
	calibrating = 0;
	first = 0;
	current = 0;
	print_limit = 0;

	current_number = 1;

	i_lastNum = 0;
	i_lost = 0;
	i_disorder = 0;
	
	bzero((char *) &serv2_addr, sizeof(struct sockaddr_in));
	bzero((char *) &cli2_addr, sizeof(struct sockaddr_in));
}

BaseProfile::~BaseProfile()
{
}

void BaseProfile::calculateSleepTime(int package_per_min, int * sec, int * usec)
{
	if (package_per_min == 0) {
		*sec = 60;
		*usec = 0;
	} else if (package_per_min <= 60) {
		*sec = 60 / package_per_min;
		*usec = 0;
	} else {
		double f = 60.0 * 1000.0 * 1000.0;
		f = f / package_per_min;
		*sec = 0;
		*usec = (int)f;
	}
}

int BaseProfile::createClientSocket(int port, const char * hostname)
{
	int sock;
	struct hostent *hostinfo;

	// server
	hostinfo = gethostbyname(hostname);
	if (hostinfo == NULL) {
		fprintf (stderr, "Unknown host %s.\n", hostname);
		exit (EXIT_FAILURE);
	}

	serv2_addr.sin_family = AF_INET;
	serv2_addr.sin_port = htons (port);
	serv2_addr.sin_addr = *(struct in_addr *) hostinfo->h_addr;

	// client
	cli2_addr.sin_family = AF_INET;
	cli2_addr.sin_port = htons(0);
	cli2_addr.sin_addr.s_addr = htons(INADDR_ANY);

	// Create
	sock = socketType();
	if (sock <0) {
		perror(__FILE__);
		exit(1);
	}

	if (bind(sock, (struct sockaddr *) &cli2_addr, sizeof(struct sockaddr_in)) < 0) {
		perror(__FILE__);
		exit(1);
	}

	return sock;
}

int BaseProfile::generatePackage(uint8_t * buf, int size, int length, bool rnd_size)
{
	int j = 0;
	*((uint32_t *)buf) = htonl(current_number);
	((uint32_t *)buf) += 1;

	current_number++;

	size -= sizeof(uint32_t);
	length -= sizeof(uint32_t);

	if (rnd_size) {
		int r = rand() % 90 + 10;
		int p = rand() % 100;

		if (p >= 50) r = - r;

		double l = length;
		l = l * (1.0 + (r / 1000.0));
		length = (int)l;
	}

	if (length > size) length = size;
	if (length > 1400) length = 1400; // ROHC max length :)
	if (length < 4) length = 4;
	for (int i = 0; i < length; i++) {
		buf[i] = j;
		j++;
		if (j > 255) j = 0;
	}
	return length + 4;
}

void BaseProfile::getPackageInfo(uint8_t * buf, int size)
{
	if (size < 4) {
		i_lost++;
		return;
	}

	int num = ntohl(*((uint32_t *)buf));

	fprintf(stderr, "num = %d last = %d lost = %d dis = %d\n", num, i_lastNum, i_lost, i_disorder);

	if (num == i_lastNum) {
		fprintf(stderr, "Same package twice?\n");
		i_lost++;
	} else if (num > i_lastNum) {
		fprintf(stderr, "normal package\n");
		int diff = num - i_lastNum;
		i_lastNum = num;
		i_lost += diff - 1;
	} else {
		fprintf(stderr, "lost package\n");
		i_lost--;
		i_disorder++;
	}
}

int BaseProfile::createServerSocket(uint16_t port)
{
	int sock;
	struct sockaddr_in name;

	/* Create the socket. */
	sock = socketType();
	if (sock < 0) {
		perror ("socket");
		exit (EXIT_FAILURE);
	}

	/* Give the socket a name. */
	name.sin_family = AF_INET;
	name.sin_port = htons (port);
	name.sin_addr.s_addr = htonl (INADDR_ANY);
	if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0) {
		perror ("bind");
		exit (EXIT_FAILURE);
	}

	return sock;
}
			
void BaseProfile::sendPackage(int socket, int package_size, bool rnd_size)
{
	int nosent;
	uint8_t package[2000];

	int len = generatePackage(package, 2000, package_size, rnd_size);

	nosent = sendto(socket, package, len, 0,
		(struct sockaddr *) &serv2_addr,
		sizeof(struct sockaddr_in));

	if (nosent != len) {
		perror(__FILE__);
		fprintf(stderr, "nosent = %d\n", nosent);
		exit(1);
	}
}

/*
 * To calculate when the server will output text to the gui.
 */
bool BaseProfile::timeToPrint(int & count)
{
	bool ret = false;
	count++;

	// To only output max 1 per/sec.
	switch(calibrating) {
		case 0:
		// start
		first = time(NULL);
		calibrating = 1;
		ret = true;
		count = 0;
		break;
	case 1:
		// first round
		current = time(NULL);
		if (difftime(current, first) >= 1) {
			print_limit = count;
			ret = true;
			calibrating = 2;
			count = 0;
			first = current;
		}
		break;
	case 2:
		// second round
		current = time(NULL);
		if (difftime(current, first) >= 1) {
			if (count > print_limit) print_limit = count;
			ret = true;
			calibrating = 3;
			count = 0;
		}
		break;
	case 3:
	default:
		// normal running
		if (count >= print_limit) {
			count = 0;
			ret = true;
		}
		break;
	}
	return ret;
}
