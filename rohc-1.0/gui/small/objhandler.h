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
 * ObjHandler
 *
 * Hanterar samtliga objekt som skall visualiseras i GUI:t
 *
 */
#ifndef _OBJHANDLER_H
#define _OBJHANDLER_H

#include "proccom.h"
#include "parser.h"
#include "ROHCInstanceTable.h"
#include "ROHCContextTable.h"
#include "ROHCProfileTable.h"
#include <qcstring.h>
#include <qptrlist.h>
#include <qvaluelist.h>
#include <kmessagebox.h>
#include <small.h>

class ObjHandler {
	public:
		ObjHandler(Small *_gui);
		~ObjHandler();
    bool readProc();
    void writeProc(QCString store);
    void getContext(int nr);
    void getProfile(int nr);    
//		void setfuncs(variable, value);

	private:
		// Ett helt gäng med funktioner();
    Small *gui;
    ProcCom *procDevice;
    Parser *parser;
    QCString *procFile;
    QPtrList<QPtrList<QValueList<QCString> > > *parsedList;

//    updateClasses(LinkedList);
    int selectedProfile;
    int selectedContext;
		ROHCInstanceTable *instanceTable;
    QPtrList<ROHCProfileTable> *profileList;
    QPtrList<ROHCContextTable> *contextList;

//		LinkedList profile = new LinkedList(); // ROHCProfileTable profile;
//		LinkedList context = new LinkedList();  // ROHCContextTable context;

//		Gui our_gui;

};

#endif
