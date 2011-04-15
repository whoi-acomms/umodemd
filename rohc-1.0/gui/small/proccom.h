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

/* ProcCom
 *
 * Sköter all kommunikation med PROC:en
 * getData	(hämtar datan från PROC:en) 	// fopen && fread
 * setData	(sätter datan i PROC:en)		// fwrite
 */

#ifndef _PROCCOM_H
#define _PROCCOM_H

#include <qcstring.h>
#include <stdio.h>


class ProcCom {

	public:
		ProcCom(QCString fileName);
		~ProcCom();

		// Öppnar filen, läser den, stänger sedan filen. Returnerar strängen från filen.
		QCString *getData();
		// Öppnar filen, skriver till filen, stänger filen. Returnerar true vid lyckad skrivning.
		bool setData(QCString parameterString);

	private:
		// Öppnar filen
		bool openProc();
		// Stänger filen
		void closeProc();

		FILE *fd;
		QCString file;		// Namnet på proc:en som ska öppnas.
//?		QCString input;	// Buffert att lagra input i.
//?		QCString output;	// Buffert att lagra output i.

};

#endif
