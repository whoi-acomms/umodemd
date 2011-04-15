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
#include <qfile.h>
#include "multiipprofile.h"

#define BUF_SIZE (2 * 1024)
#define PROTOCOL_IP_IN_IP	4

/*
 *      This is a version of ip_compute_csum() optimized for IP headers,
 *      which always checksum on 4 octet boundaries.
 *
 *      By Jorge Cwik <jorge@laser.satlink.net>, adapted for linux by
 *      Arnt Gulbrandsen.
 */
static inline unsigned short ip_fast_csum(unsigned char * iph,
                                          unsigned int ihl) {
        unsigned int sum;

        __asm__ __volatile__(" \n\
            movl (%1), %0      \n\
            subl $4, %2		\n\
            jbe 2f		\n\
            addl 4(%1), %0	\n\
            adcl 8(%1), %0	\n\
            adcl 12(%1), %0	\n\
1:          adcl 16(%1), %0	\n\
            lea 4(%1), %1	\n\
            decl %2		\n\
            jne 1b		\n\
            adcl $0, %0		\n\
            movl %0, %2		\n\
            shrl $16, %0	\n\
            addw %w2, %w0	\n\
            adcl $0, %0		\n\
            notl %0		\n\
2:				\n\
            "
        /* Since the input registers which are loaded with iph and ipl
           are modified, we must also specify them as outputs, or gcc
           will assume they contain their original values. */
        : "=r" (sum), "=r" (iph), "=r" (ihl)
        : "1" (iph), "2" (ihl));
        return(sum);
}


// --------------------------------------------------------------------------

MultiIpProfile::MultiIpProfile()
{
	currentFile = QString::null;
}

MultiIpProfile::~MultiIpProfile()
{
}

void MultiIpProfile::client(QString firstFile, const char * hostname,
	int frequency, bool reloop)
{
	this->firstFile = firstFile;
	i_reloop = reloop;
	
	int sec, usec;
	int socket;

	socket = createClientSocket(2893, hostname);
	calculateSleepTime(frequency, &sec, &usec);

	for (;;) {
		//printf("Sending package...\n");
		sendPackage(socket, 0, false);

		//printf("Sleeping\n");
		if (sec > 0) sleep(sec);
		if (usec > 20) usleep(usec);
	}

	//close(socket);
}

#define EXTRA_DATA 6

int MultiIpProfile::generatePackage(uint8_t * buf, int size2, int, bool)
{
	int size3 = loadNextFile(buf, size2 - EXTRA_DATA);
	if (size3 < 0) {
		fprintf(stderr, "Failed to load\n");
		exit(1);
		return 0;
	}

	// assume that we loaded an IP package

	uint16_t * p = (uint16_t *)buf;
	p[1] = htons( ntohs(p[1]) + EXTRA_DATA );
	p[5] = 0;
	p[5] = ip_fast_csum(buf, buf[0] & 0xf);

	uint16_t * end = (uint16_t *)(buf + size3);
	end[0] = p[5];

	uint32_t * num = (uint32_t *)(&end[1]);
	*num = htonl(current_number);
	current_number++;

	return size3 + EXTRA_DATA;
}

QString MultiIpProfile::getCurrentFileName()
{
	if (currentFile == QString::null) {
		currentFile = firstFile;
		return currentFile;
	}

	int sec = currentFile.findRev('-');
	int first = currentFile.findRev('-', sec - 1);

	QString begin = currentFile.left(first + 1);
	QString end = currentFile.mid(sec);
	QString num = currentFile.mid(first + 1, sec - first - 1);

	currentFile = begin + QString::number(num.toInt() + 1) + end;

	return currentFile;
}

void MultiIpProfile::getPackageInfo(uint8_t * buf, int size)
{
	if (size < 40) {
		i_lost++;
		return;
	}

	uint16_t * head2 = (uint16_t *)(buf + (buf[0] & 0xf) * 4);
	uint16_t * check = (uint16_t *)(&buf[size - EXTRA_DATA]);
	if (head2[5] != *check) {
		i_lost++;
		return;
	}

	check++;
	BaseProfile::getPackageInfo((uint8_t *)check, 4);
}

int MultiIpProfile::loadFile(QString filename, uint8_t * toBuf, int max_size)
{
	if (!QFile::exists(filename)) return -2;

	QFile f(filename);
	if (!f.open(IO_ReadOnly)) return -1;

	int size = f.size();
	if (size > max_size) size = max_size;
	
	int s = f.readBlock((char *)toBuf, size);
	f.close();

	if (s != size) return -1;
	return s;
}

int MultiIpProfile::loadNextFile(uint8_t * buf, int max_size)
{
	QString name = getCurrentFileName();
	int ret = loadFile(name, buf, max_size);
	if (ret > 0) return ret;
	if (ret == -2 && i_reloop) {
		name = currentFile = firstFile;
		ret = loadFile(name, buf, max_size);
	}
	if (ret <= 0) {
		fprintf(stderr, "error loading file %s\n", name.ascii());
		exit(1);
	}
	return ret;
}

int MultiIpProfile::socketType()
{
	return socket(PF_INET, SOCK_RAW, PROTOCOL_IP_IN_IP);
}

void MultiIpProfile::server()
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
