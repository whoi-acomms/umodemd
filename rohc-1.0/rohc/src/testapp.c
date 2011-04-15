/*
    ROHC Project 2003 at Lulea University of Technology, Sweden.
    Authors: Andreas Vernersson <andver-8@student.luth.se>
             Daniel Pettersson <danpet-7@student.luth.se>
             Erik Soderstrom <soderstrom@yahoo.com>
             Fredrik Lindstrom <frelin-9@student.luth.se>
             Johan Stenmark <johste-8@student.luth.se>
             Martin Juhlin <juhlin@users.sourceforge.net>
             Mikael Larsson <larmik-9@student.luth.se>
             Robert Maxe <robmax-1@student.luth.se>
             
    Copyright (C) 2003 Andreas Vernersson, Daniel Pettersson, 
    Erik Soderström, Fredrik Lindström, Johan Stenmark, 
    Martin Juhlin, Mikael Larsson, Robert Maxe.  

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/    
/*************************
 * File: Userspacetest.c *
 *************************
 * Test application for ROHC in userspace
 * ---------------------------------------
 * Comments: 	This application is made only for testing
 * 		the ROHC implementation in Linux Userspace.
 *              (Instead of examining kernel dumps :))
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include "testapp.h"

//#include "feedback.c"

#include "comp.c"
#include "c_ip.c"
#include "c_uncompressed.c"
#include "c_udp.c"
#include "c_udp_lite.c"
#include "c_util.c"


#include "decomp.c"
#include "d_util.c"
#include "d_ip.c"
#include "d_udp.c"
#include "d_udp_lite.c"
#include "d_uncompressed.c"

#include "feedback.c"


#define MAXPACKAGESIZE 	5000
#define VERSION  	"Version 0.01, ROHC Testapplication in Userspace\n"
#define DROP_PACKAGES 0
#define BIT_CHANGE 0
#define LOWER_DROP_PACKET  100
#define UPPER_DROP_PACKET  300

#define HELP     	"\nHelp for ROHC Testapplication\n\
\n-v\tPrints current version of application\n\
\n-h\tPrints this help message\n\
\n-s <file>\tSequance test. Reading files with the name <file>-<num>-ip.bin, begining\n\
\t\t\twith one. If the -r is giving, only decompressor is tested.\n\
\n-t\tSets the type of package to use:\n\
\t\tudp\tfor udp\n\
\t\tlite\tfor udp-lite\n\
\t\tip\tfor ip\n\
\t\tuncomp\tfor uncompressed\n\
\n-f\tSets the number of packages in percent that will contain errors (Not implemented)\n\
\n-d\tSets debugmode for simulation\n\
\t\t1\tprinting the original package\n\
\t\t2\tprinting the compressed package\n\
\t\t4\tprinting the decompressed package\n\
\t\t8\tprinting the feedback from decompressor\n\
\n-r\tEnables rohc-simulation, will only test the decompressor\n\
\nExamples:\
\t1:\ttestapp -n 10 -d 5 -t udp -r -f 10\n\
\t\t2:\ttestapp -n 2 -d 7 -t ip -t uncomp -t udp\n\
"




 /*
 * hjï¿½pfunktion till main
 *
 */
int getType(char *string)
{
	if(!strcmp(string, "udp"))
		return UDP;
	else if(!strcmp(string,"lite"))
		return LITE;
	else if(!strcmp(string, "ip"))
		return IP;
	else if(!strcmp(string, "uncomp"))
		return UNCOMP;
	return 0;
}



 /* Fldet i Main-funktionen
 * 1. Parse:a inparametrarna
 * 2. Skapa en paketstrm beroende pï¿½inparameter
 * 3. Skriv ev ut paketet om rï¿½t debugniv
 * 4. Compressa ett paket
 * 5. Skriv ev ut comppaketet om rï¿½t debugniv
 * 6. Decompressa paketet
 * 7. Skriv ev ut decomppaketet om rï¿½t debugniv
 * 8. Compare package, print err if found
 *(9.) Hantera ev feedback och redirecta, print if debug
 * 10. GoTo 3:
 * 11. End538
 */
int debugmode = 0;
int feedback_size = 0;
static void *comp, *comp2;

int main(int argc, char *argv[])
{
	int packageType=0, nr_packages=1, errPackages=0,i=0, j=0, k=0, byte_compressed=0, byte_decompressed=0, rohc_mode=0;
	unsigned char *package;	packageList *packages[4], *tmp;
	int argCount = 0, nrOfTypes=0;
	unsigned char buffer[MAXPACKAGESIZE];
	unsigned char *b_decomp;
	//struct s_medium medium = {ROHC_SMALL_CID, 15, 3};
	struct sd_rohc * decomp;
	int sequence = 0;
	char * seq_filename = 0;

	crc_init_table(crc_table_3, crc_get_polynom(CRC_TYPE_3));
	crc_init_table(crc_table_7, crc_get_polynom(CRC_TYPE_7));
	crc_init_table(crc_table_8, crc_get_polynom(CRC_TYPE_8));

	comp = rohc_alloc_compressor(15);
	rohc_activate_profile(comp, 0);
	rohc_activate_profile(comp, 2);
	rohc_activate_profile(comp, 4);
	rohc_activate_profile(comp, 8);


	comp2 = rohc_alloc_compressor(15);
	rohc_activate_profile(comp2, 0);
	rohc_activate_profile(comp2, 2);
	rohc_activate_profile(comp2, 4);
	rohc_activate_profile(comp2, 8);

	printf("DROP_PACKAGES=%d\nBIT_CHANGE=%d\nLOWER_DROP_PACKET=%d\nUPPER_DROP_PACKET=%d\n",
		DROP_PACKAGES,BIT_CHANGE,LOWER_DROP_PACKET,UPPER_DROP_PACKET);

	if (argc <= 1) {
		printf(HELP);
		return 1;
	}

	for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount)
	{
		argCount = 1;
		if (!strcmp(*argv, "-v"))               // print copyright
			printf(VERSION);
		else if (!strcmp(*argv, "-h")) {       	// print help
			printf(HELP);
		}
		else if (!strcmp(*argv, "-t")) {       	// set packagetype
			printf("-t %s\n",*(argv + 1));
			packageType = getType(*(argv +1));
			if(packageType>0)
			{
				printf("PackageType: %d\n", packageType);
				packages[nrOfTypes++] = generatePackage(packageType, nr_packages);
			}
			argCount = 2;
        	}
        	else if (!strcmp(*argv, "-n")) {       	// set nr of packages
			//printf("-n %s\n",*(argv + 1));
			nr_packages = atoi(*(argv+1));
			printf("Nr of packages: %d\n", nr_packages);
			argCount = 2;
        	}
		else if (!strcmp(*argv, "-f")) {       	// set nr of errpackages
			//printf("-f %s\n",*(argv + 1));
			errPackages = atoi(*(argv+1));
			printf("Err packages: %d\n", errPackages);
			argCount = 2;
		}
		else if (!strcmp(*argv, "-d")) {       	// set debugmode
			//printf("-d %s\n",*(argv + 1));
			debugmode = atoi(*(argv+1));
			printf("debugmode: %d\n", debugmode);
			argCount = 2;
		}
		else if (!strcmp(*argv, "-r")) {       	// set debugmode
			printf("ROHC Decompressor test\n");
			rohc_mode = 1;

		} else if (!strcmp(*argv, "-s")) {
			sequence = 1;
			seq_filename = argv[1];
			argCount = 2;
		} else {
			printf(HELP);
			return 1;
		}

	}

	if (sequence) {
		printf("Sequance test, reading %s-1-ip.bin\n", seq_filename);
		sequence_test(seq_filename, rohc_mode);
		return 0;
	}

 	decomp = rohc_alloc_decompressor(comp);

//	printf("Create packages();\n");

	for(i = 0 ; i < nrOfTypes ;i++)
	{
		if(rohc_mode)
		{
			tmp = packages[i];
//			for(j = 1; j < 4 ; j++)
//			{
				packages[i] = packages[i]->nxt;
				for(k = 0; k < nr_packages ; k++)
				{
					package = packages[i]->package;
//					printDebugMsg(debugmode, package, INPUT, packages[i]->size);
//	printf("\tCompress package();\n");
//					printDebugMsg(debugmode, package, COMP, packages[i]->size);
		printf("\tDecompress package();\n");
//	returnerar hur mï¿½ga byte som ï¿½ skrivet.
					b_decomp = (char *) malloc(packages[i]->size);
					byte_decompressed = rohc_decompress(decomp, packages[i]->package, packages[i]->size, b_decomp, packages[i]->size);

//					printDebugMsg(debugmode, buffer, DECOMP, byte_decompressed);

					comparePackage(tmp->package, tmp->size, b_decomp, tmp->size);
					free(b_decomp);


//			printf("\tCompare packages();\n");

//					printDebugMsg(debugmode, package, FEEDBACK, packages[i]->size);
				}
//			}
			packages[i] = tmp;
		}
		else
		{
			int ip_size;
			struct iphdr *ip;
			unsigned char buffer2[1500];


			//struct iphdr *ip2;

			package = packages[0]->package;
			ip_size = packages[0]->size;

			ip = (struct iphdr*)package;
			//ip2=(struct iphdr *)ip+1;

			for(k = 0; k < nr_packages ; k++)
			{
				printf("PACKAGE %d\n", k);


				printDebugMsg(debugmode, package, INPUT, ip_size);

				printf("\tCompress package();\n");
				memset(buffer, 0xff, MAXPACKAGESIZE);

				byte_compressed = rohc_compress(comp, package, ip_size, buffer, MAXPACKAGESIZE);

				printDebugMsg(debugmode, buffer, COMP, byte_compressed);

				printf("\tDecompress package();\n");
				byte_decompressed = rohc_decompress(decomp, buffer, byte_compressed, buffer2, MAXPACKAGESIZE);

				//printDebugMsg(debugmode, package, DECOMP, packages[i]->size);

				printf("\tCompare packages();\n");
				comparePackage(package, ip_size, buffer2, byte_decompressed);

//			comparePackage(packages[0]->package, packages[0]->size,packages[0]->nxt->package, packages[0]->nxt->size);
			//comparePackage(packages[0]->package, packages[0]->size, packages[0]->package, packages[0]->size);
			//	feedbackRedir(package, packages[0]->size);
//				printDebugMsg(debugmode, package, 8);

			//	ip->id = htons(ntohs(ip->id) + 1);
			//	ip2->id = htons(ntohs(ip2->id) + 1);
			//	ip->tos = 1;
			}
		}
	}
//	printf("CleanUpAndEnd();\n\n");
	printf("\nProgram exits\n\n");
	for(i = 0 ; i < nrOfTypes ;i++)
	{
		tmp = packages[i];
		for(j = 0; j < 4 ; j++)
		{
			while(packages[i]) {
				tmp = packages[i]->nxt;
				free(packages[i]->package);
				free(packages[i]);
				packages[i] = tmp;
			}
		}
		free(tmp);

	}


	rohc_free_decompressor(decomp);

	return 0;
}


int readFromFile(FILE *file, packageList *pList)
{
	unsigned char bin_byte = 0;
	unsigned char buffer[MAXPACKAGESIZE];
	unsigned char *newPackage;
	int fileSize=0;
	int bytes = 0;

	if(file == NULL){
  		printf("E de ingen fil?\n");
		pList->package = NULL;
		pList->size = -1;
		return -1;
	}
	else
	{
		do
		{
			bytes = fread(&bin_byte, 1 , 1, file);
			if(bytes)
			{
				buffer[fileSize] = bin_byte;
//				printf("[%d] %d '%c'\n", fileSize, bin_byte, bin_byte);
				fileSize++;
			}
		}while(bytes && fileSize < MAXPACKAGESIZE);
//		printf("filesize %d bytes\n", fileSize);

		newPackage = (char *) malloc(fileSize+1);
//		strcpy(newPackage,buffer);
		memcpy(newPackage,buffer, fileSize);
		pList->package = newPackage;
		pList->size = fileSize;
		return 1;
 	}
}


/*
 * Skapar en serie paket av typen "packageType"
 * nr_Packages anger hur mï¿½ga paket som skapas.
 * packageType avgr vilken typ av paket som skapas (IP,UDP etc)
 *
 * Returnerar en pekare till paketstrmmen
 */
packageList *generatePackage(int packageType, int nr_Packages)
{
	packageList *tmp, *tmpPList, *pList = (packageList *) malloc(sizeof(packageList));
	FILE *std_file, *ir_file, *fo_file, *so_file;
//	printf("Creating packages\n");
	switch(packageType) {
		case UDP:
			pList->type = UDP;
			std_file = fopen("testdata/udp.bin", "r");
			ir_file = fopen("testdata/udp-ir.bin", "r");
			fo_file = fopen("testdata/udp-fo.bin", "r");
			so_file = fopen("testdata/udp-so.bin", "r");
			break;
		case LITE:
			pList->type = LITE;
			std_file = fopen("testdata/lite.bin", "r");
			ir_file = fopen("testdata/lite-ir.bin", "r");
			fo_file = fopen("testdata/lite-fo.bin", "r");
			so_file = fopen("testdata/lite-so.bin", "r");
			break;
		case IP:
			pList->type = IP;
			std_file = fopen("testdata/ip.bin", "r");
			ir_file = fopen("testdata/ip-ir.bin", "r");
			fo_file = fopen("testdata/ip-fo.bin", "r");
			so_file = fopen("testdata/ip-so.bin", "r");
			break;
		case UNCOMP:
			pList->type = UNCOMP;
			std_file = fopen("testdata/uncomp.bin", "r");
			ir_file = fopen("testdata/uncomp-ir.bin", "r");
			fo_file = fopen("testdata/uncomp-fo.bin", "r");
			so_file = fopen("testdata/uncomp-so.bin", "r");
			break;
		default:
			return NULL;
			break;
	}
	readFromFile(std_file, pList);
	tmp = pList;
	tmpPList = (packageList *) malloc(sizeof(packageList));
	pList->nxt = tmpPList;
	pList = tmpPList;
	readFromFile(ir_file, pList);
	tmpPList = (packageList *) malloc(sizeof(packageList));
	pList->nxt = tmpPList;
	pList = tmpPList;
	readFromFile(fo_file, pList);
	tmpPList = (packageList *) malloc(sizeof(packageList));
	pList->nxt = tmpPList;
	pList = tmpPList;
	readFromFile(so_file, pList);
	pList->nxt = NULL;
	return tmp;

}

/*
 * Jï¿½fr paket "orgPackage" med paket "finalPackage"
 * om paketen ï¿½ lika returneras true annars false
 */
int comparePackage(unsigned char *orgPackage, int orgSize,unsigned char *finalPackage, int finalSize)
{
	int valid = 1;
	int endOfPackage=0;
	int i =0, j = 0, k = 0;
	char orgstr[4][7], finalstr[4][7];
	printf("------------------------------ Compare ------------------------------\n");
	if(orgSize != finalSize)
	{
		printf("Size error, original size is %d and finalsize is %d\n", orgSize, finalSize);
		valid = 0;
	}
	if(orgSize > finalSize)
		endOfPackage = finalSize;
	else
		endOfPackage = orgSize;
	for(i = 0; i < endOfPackage; i++){
		if(orgPackage[i] != finalPackage[i])
		{
			sprintf(orgstr[j], "#0x%.2x#",orgPackage[i]);
			sprintf(finalstr[j], "#0x%.2x#",finalPackage[i]);
			valid = 0;
		}
		else
		{
			sprintf(orgstr[j], "[0x%.2x]",orgPackage[i]);
			sprintf(finalstr[j], "[0x%.2x]",finalPackage[i]);
		}
		// Fixing the output, so its human reable
		if(j>=3 || (i+1)>= endOfPackage)
		{
			for(k = 0; k<4;k++){
				if(k<(j+1))
					printf("%s  ", orgstr[k]);
				else 	// Fill out if not package fills last for 4bytes
					printf("        ");
			}
			printf("      ");
			for(k = 0; k<(j+1);k++)
				printf("%s  ", finalstr[k]);
			printf("\n");
			j=0;
		}
		else
			j++;
	};
	if(valid)
		printf("-------------------- Package is correct decoded ---------------------\n");
	else
		printf("------------------ Package is NOT correct decoded --------------------\n");
	return 0;
}






char fixAscii(char c) {
	if (c >= '0' && c<='9')
		return c;
	if (c >= 'a' && c<='z')
		return c;
	if (c >= 'A' && c<='Z')
		return c;
	return '.';

}


/*
 * Skriver ut ett pakets innehï¿½l beroende pï¿½vilken
 * debugnivï¿½som programmet exekveras i.
 * debugLevel ï¿½ den debugnivï¿½som programmet krs i.
 * packageToPrint ï¿½ det paket som ska skrivas ut.
 *
 */
void printDebugMsg(int debugLevel, unsigned char *packageToPrint, int debugType, int size)
{
	int i = 0;
	int offset =0;
	if(debugLevel & debugType)
	{
		// Print package
		printf("-------------------------------\n");
		while (size > 0) {
			for (i=0; i<4; i++) {
				//printf("%d\n", offset + i);
				if (i < size)
					printf("[0x%02x] ", packageToPrint[i]);
				else
					printf("[----] ");

			}

			for (i=0; i<4; i++) {
				if (i < size)
					printf("%c", fixAscii(packageToPrint[i]));
				else
					printf(".");
			}

			size-=4;
			packageToPrint += 4;
			offset+=4;
			printf("\n");
		}

		printf("\n-------------------------------\n");


	}
	else
	{
		// Do nothing
//		printf("Not needed debuglevel!\n");
	}
}

/*
 * Kommer nog inte att anvï¿½das...
 *
 *
 */
int writeToFile(int file, unsigned char *packageToWrite)
{
	return 0;
}

/* Test a serie of files.
 */
void sequence_test(char * filename, int rohc_mode)
{
	if (rohc_mode) {
		sequence_test_decompressor_only(filename);
	} else {
		sequence_test_comp_and_decomp(filename);
	}
}

static int  feedback_mode = 0;


void feedbackRedir_piggy(unsigned char *ibuf, int feedbacksize)
{
	if (feedback_mode==0) {
		printDebugMsg(1, ibuf, 1, feedbacksize);
		//c_deliver_feedback(comp, ibuf, feedbacksize);
		feedback_size = feedbacksize;
		c_piggyback_feedback(comp, ibuf, feedbacksize);
	} else {
		printDebugMsg(1, ibuf, 1, feedbacksize);
		//c_deliver_feedback(comp, ibuf, feedbacksize);
		feedback_size = feedbacksize;
		c_piggyback_feedback(comp2, ibuf, feedbacksize);

	}
}


/*
 * Anropas frï¿½ dekompressorn och skriver ut ev debuginfo
 * och skickar datan till dekompressorn
 *
 */
void feedbackRedir(unsigned char *ibuf, int feedbacksize)
{
	if (feedback_mode==0) {
		printf("stream 0: inkommen feedback data\n");
		printDebugMsg(1, ibuf, 1, feedbacksize);
		feedback_size = 0;
		c_deliver_feedback(comp, ibuf, feedbacksize);
	} else {
		printf("stream 1: inkommen feedback data\n");
		printDebugMsg(1, ibuf, 1, feedbacksize);
		feedback_size = 0;
		c_deliver_feedback(comp2, ibuf, feedbacksize);

	}
}



/* Sequence test of compressor and decompressor
 */

#define MAX_ROHC_SIZE	(5*1024)


char buffer[80000];

void sequence_test_comp_and_decomp(char * filename)
{
	int ok = 0, packages=0;
	int len, not_negative = 0;
	//struct s_medium medium = {ROHC_SMALL_CID, 15, 3};
	struct sd_rohc * decomp = rohc_alloc_decompressor(comp);
	struct sd_rohc * decomp2 = rohc_alloc_decompressor(comp2);

	void *ip_packet, * decomp_packet;
	unsigned char * rohc_packet;
	int ip_size, rohc_size, decomp_size;
	char buf[500];
	int i, washere = 0, count_err = 0;
	int random_packet = 0;
	int rnd = 0;
	rohc_packet = malloc(MAX_ROHC_SIZE);
	decomp_packet = malloc(MAX_ROHC_SIZE);

	srand(21);

	for (i = 1;; i++) {

		sprintf(buf, "%s-%d-ip.bin", filename, i);
		ip_packet = read_file(buf, &ip_size);
		if (!ip_packet) {

			printf("Error reading file %s\n", filename);
			break;
			//exit(0);
		}

		feedback_mode = 1;
		rnd = rand();
		rohc_size = rohc_compress(comp, ip_packet, ip_size, rohc_packet, MAX_ROHC_SIZE);

		if(DROP_PACKAGES||BIT_CHANGE){
			washere = 0;
			if(BIT_CHANGE){
				random_packet = rnd%4;

				if(random_packet > 0) {
					printf("BIT_CHANGE  \n");
					//kommentera bort vid random drop
					//printf("Dropping package %d\n\n\n\n", i);
					//washere=1;
					count_err++;
					rohc_packet[rnd%20] = rohc_packet[rnd%20] + 1;

				}

			}
			if(DROP_PACKAGES){
				if (i>LOWER_DROP_PACKET && i<UPPER_DROP_PACKET) {
					printf("Dropping package %d\n\n\n\n", i);
					count_err++;
					washere = 1;

				}
				if (i>1000 && i<2000) {
					printf("Dropping package %d\n\n\n\n", i);
					count_err++;
					washere = 1;

				}

			}

		}
		if(!washere){
		decomp_size = rohc_decompress(decomp2, rohc_packet, rohc_size, decomp_packet, MAX_ROHC_SIZE);

		packages ++;
		if (decomp_size != ip_size || memcmp(ip_packet, decomp_packet, ip_size) != 0) {
			printf("[ERR] Stream 0: Package %d failed\n", i);
			printDebugMsg(15, rohc_packet, COMP, rohc_size);
			comparePackage(ip_packet, ip_size, decomp_packet, decomp_size);
		} else {
			printf("Stream 0: Package %d is OK\n", i);
			ok++;

		}
		if (decomp_size>0)
			not_negative++;

		//printf("\n\n");
		}

		feedback_mode = 0;

		rohc_size = rohc_compress(comp2, ip_packet, ip_size, rohc_packet, MAX_ROHC_SIZE);

		if(DROP_PACKAGES||BIT_CHANGE){
			washere = 0;
			if(BIT_CHANGE){
				random_packet = rnd%4;

				if(random_packet > 0) {
					printf("BIT_CHANGE  \n");

					count_err++;
					rohc_packet[rnd%20] = rohc_packet[rnd%20] + 1;
					//kommentera bort vid random drop
					//printf("Dropping package %d\n\n\n\n", i);
					//free(ip_packet);
					//continue;
				}

			}
			if(DROP_PACKAGES){
				if (i>LOWER_DROP_PACKET && i<UPPER_DROP_PACKET) {
					printf("Dropping package %d\n\n\n\n", i);
					count_err++;
					washere = 1;
					free(ip_packet);
					continue;

				}
				if (i>1000 && i<2000) {
					printf("Dropping package %d\n\n\n\n", i);
					count_err++;
					washere = 1;
					free(ip_packet);
					continue;
				}


			}
		}


		decomp_size = rohc_decompress(decomp, rohc_packet, rohc_size, decomp_packet, MAX_ROHC_SIZE);
		packages ++;
		//comparePackage(ip_packet, ip_size, decomp_packet, decomp_size);
		if (decomp_size != ip_size || memcmp(ip_packet, decomp_packet, ip_size) != 0) {
			printf("[ERR] Stream 1: Package %d failed\n", i);
			printDebugMsg(15, rohc_packet, COMP, rohc_size);
			comparePackage(ip_packet, ip_size, decomp_packet, decomp_size);
		} else {
		  	printf("Stream 1: Package %d is OK\n", i);
			ok++;
		}
		if (decomp_size>0)
			not_negative++;

		printf("\n\n\n");

		free(ip_packet);

//		if (i==10)
//			break;

	}
	printf("Package count=%d. OK package count=%d. Package failed=%d. Package sent to app=%d.\n", packages, ok, count_err, not_negative);

	free(decomp_packet);
	free(rohc_packet);

	buffer[0] = 0;

	len = rohc_c_info(buffer);
	len = rohc_c_statistics(comp, buffer);
	len = rohc_d_statistics(decomp, buffer);

	i=0;
	while (rohc_c_context(comp, i, buffer) != -2) {
		i++;
	}

	i = 0;
	while (rohc_d_context(decomp, i, buffer) != -2) {
		i++;
	}

	printf("/proc/rohc (length=%d):\n%s\n", strlen(buffer), buffer);


	rohc_free_compressor(comp2);
	rohc_free_decompressor(decomp2);

	rohc_free_compressor(comp);
	rohc_free_decompressor(decomp);

}


/* Sequence test for the decompressor
 */
void sequence_test_decompressor_only(char * filename)
{
	int i, out_size;
	//struct s_medium medium = {ROHC_SMALL_CID, 15, 3};
	struct sd_rohc * decomp = rohc_alloc_decompressor(comp);
	packageList * ip = read_sequence(filename, "ip");
	packageList * rohc = read_sequence(filename, "rohc");
	void * in, * out;

	if (!ip || !rohc) {
		printf("No package? (%p %p)\n", ip, rohc);
		return;
	}
	if (package_list_size(ip) != package_list_size(rohc)) {
		printf("Missing package ? Ip : %d Rohc %d\n",
			package_list_size(ip),
			package_list_size(rohc));
	}

	for (i = 1; ip; i++) {
		in = memdup(rohc->package, rohc->size);
		out = malloc(ip->size);

		out_size = rohc_decompress(decomp, in, rohc->size, out, ip->size);

		if (memcmp(rohc->package, in, rohc->size) != 0) {
			printf("Input buffer is changed\n");
			comparePackage(rohc->package, rohc->size, in, rohc->size);
		}
		if (ip->size != out_size || memcmp(ip->package, out, ip->size) != 0) {
			printf("Package %d failed\n", i);
			printDebugMsg(15, rohc->package, COMP, rohc->size);
			comparePackage(ip->package, ip->size, out, out_size);
		} else {
			printf("Package %d is OK\n", i);
		}

		free(in);
		free(out);

		ip = ip->nxt;
		rohc = rohc->nxt;
	}

	rohc_free_decompressor(decomp);
}

/* Read a serie of files
 */
packageList * read_sequence(char * filename_begin, char * filename_end)
{
	int i, size;
	packageList * first = 0, * p, * prev;
	unsigned char buf[500], * f;

	for(i = 1;; i++) {
		sprintf(buf, "%s-%d-%s.bin", filename_begin, i, filename_end);
		f = read_file(buf, &size);
		if (f == 0) break;

		p = malloc(sizeof(packageList));
		p->size = size;
		p->type = 0;
		p->package = f;
		p->nxt = 0;

		if (first) {
			prev->nxt = p;
			prev = p;
		} else {
			first = prev = p;
		}
	}
	return first;
}

/* Allocate memory and read a file. write the size in "size".
 */
unsigned char * read_file(char * filename, int * size)
{
	unsigned char * buf;
	FILE * f = fopen(filename, "rb");
	if (f == NULL) return 0;

	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	fseek(f, 0, SEEK_SET);

	buf = malloc(*size);
	fread(buf, 1, *size, f);

	fclose(f);

	printf("Loaded %s with size %d\n", filename, *size);

	return buf;
}

/* (M) Count packages in a packageList
 */
int package_list_size(packageList * p)
{
	int ret = 0;
	while(p) {
		ret++;
		p = p->nxt;
	}
	return ret;
}

/* (M) Duplicate memory
 */
void * memdup(void * src, int size)
{
	void * ret = malloc(size);
	memcpy(ret, src, size);
	return ret;
}
