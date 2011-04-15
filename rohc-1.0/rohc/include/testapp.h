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
/*
 * File:        testapp.h
 * Description: Header-file for usermode test application..
 */

#ifndef _TESTAPP_H
#define _TESTAPP_H

// packageType
#define UDP 		1
#define LITE 		2
#define IP 		3
#define UNCOMP 		4

// debugType
#define NODEBUG		0
#define INPUT		1
#define COMP		2
#define DECOMP		4
#define FEEDBACK	8

/*
 * Linked list with information about orginal packet
 * and generated packets
 */
typedef struct generatedPackage{
	int size;
	int type;
	unsigned char * package;
	struct generatedPackage *nxt;
}packageList;




/* Flödet i Main-funktionen
 * 1. Parse:a inparametrarna
 * 2. Skapa en paketström beroende på inparameter
 * 3. Skriv ev ut paketet om rätt debugnivå
 * 4. Compressa ett paket
 * 5. Skriv ev ut comppaketet om rätt debugnivå
 * 6. Decompressa paketet
 * 7. Skriv ev ut decomppaketet om rätt debugnivå
 * 8. Compare package, print err if found
 *(9.) Hantera ev feedback och redirecta, print if debug
 * 10. GoTo 3:
 * 11. End
 */
 // int main(argc c, argv v[]);


/*
 * Skapar en serie paket av typen "packageType"
 * nr_Packages anger hur många paket som skapas.
 * packageType avgör vilken typ av paket som skapas (IP,UDP etc)
 *
 * Returnerar en pekare till paketströmmen
 */
packageList *generatePackage(int packageType, int nr_Packages);

/*
 * Jämför paket "orgPackage" med paket "finalPackage"
 * om paketen är lika returneras true annars false
 */
int comparePackage(unsigned char *orgPackage, int orgSize, unsigned char *finalPackage, int finalSize);

/*
 * Anropas från dekompressorn och skriver ut ev debuginfo
 * och skickar datan till kompressorn. Detta är när feedback
 * kommer in till dekomp som denna anropas
 *
 */
void feedbackRedir(unsigned char *ibuf, int feedbacksize);

/*
 * Anropas av dekompressorn när feedback ska piggybackas på
 * kompressorn, detta för att se vad som piggybackas
 * Anropas när feedback ska skickas
 */
void feedbackRedir_piggy(unsigned char *ibuf, int feedbacksize);


/*
 * Skriver ut ett pakets innehåll beroende på vilken
 * debugnivå som programmet exekveras i.
 * debugLevel är den debugnivå som programmet körs i.
 * packageToPrint är det paket som ska skrivas ut.
 *
 */
void printDebugMsg(int debugLevel, unsigned char *packageToPrint, int debugType, int size);

/*
 * Kommer nog inte att användas...
 *
 *
 */
int writeToFile(int file, unsigned char *packageToWrite);

/* (M) Sequance test
 */
void sequence_test(char * filename, int rohc_mode);

/* (M) Sequence test of compressor and decompressor
 */

void sequence_test_comp_and_decomp(char * filename);
/* (M) Sequence test for the decompressor
 */
void sequence_test_decompressor_only(char * filename);

/* (M) Read a serie of files
 */
packageList * read_sequence(char * filename_begin, char * filename_end);

/* (M) Allocate memory and read a file. write the size in "size".
 */
unsigned char * read_file(char * filename, int * size);

/* (M) Count packages in a packageList
 */
int package_list_size(packageList *);

/* (M) Duplicate memory
 */
void * memdup(void *, int size);

#endif
