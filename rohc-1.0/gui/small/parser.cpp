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
/* Parser
 *
 * Bryt upp sträng till parametrar med värden
 *
 */

#include "parser.h"

Parser::Parser(){

}

Parser::~Parser(){

}

// Indata enligt ProcSyntax
// Returnerar ...?
QPtrList<QPtrList<QValueList<QCString> > > * Parser::parseString(QCString unparsed){

	QPtrList<QPtrList<QValueList<QCString> > > *l = new QPtrList<QPtrList<QValueList<QCString> > >();
	QPtrList<QValueList<QCString> > *lInstance = new QPtrList<QValueList<QCString> >();
	QPtrList<QValueList<QCString> > *lProfile = new QPtrList<QValueList<QCString> >();
	QPtrList<QValueList<QCString> > *lContext = new QPtrList<QValueList<QCString> >();
	QCString extracted;
	QCString remainder;
	int index = 0, eof = 0;
	remainder = unparsed;
	while(index >= 0 && !eof){
		index = remainder.find("---", 0, TRUE);
    if(index>=0){
  		remainder = remainder.right(remainder.length()-(index+3));
   		index = remainder.find(NEWLINE, 0, TRUE);
  		extracted = remainder.left(index);
    }
    else{
          eof = 1;
    }
		if(extracted == "Instance"){
			index = remainder.find("---", 0, TRUE); // find next section start
			extracted = remainder.left(index);
			index = extracted.findRev('\n', extracted.length(), TRUE);
			extracted = extracted.left(index);
      extracted = extracted.right(extracted.length() - 9);
			lInstance->append(parseInternal(extracted));
		} else if(extracted == "Profile"){
			index = remainder.find("---", 0, TRUE);
			extracted = remainder.left(index);
			index = extracted.findRev('\n', extracted.length(), TRUE);
			extracted = extracted.left(index);
      extracted = extracted.right(extracted.length() - 8);
      lProfile->append(parseInternal(extracted));
		} else if(extracted == "Context"){
			index = remainder.find("---", 0, TRUE);
      if(index<0)
        extracted = remainder;
      else
        extracted = remainder.left(index);
      if(!eof)
  			index = extracted.findRev('\n', extracted.length(), TRUE);
			extracted = extracted.left(index);
      extracted = extracted.right(extracted.length() - 8);
			lContext->append(parseInternal(extracted));
		}
	}
	l->append(lInstance);
	l->append(lProfile);
	l->append(lContext);
	return l;
}

QValueList<QCString> * Parser::parseInternal(QCString unparsed){

	QValueList<QCString> *l = new QValueList<QCString>();
	QCString extracted;
	QCString remainder;
	int index, length;
	remainder = unparsed;
	length = unparsed.length();
	for(int i = 0; i < length; i++){
		index = remainder.find(NEWLINE, 0, TRUE);
		if(index < 0) break;
		extracted = remainder.left(index);
		remainder = remainder.right(remainder.length()-index-1);

		index = extracted.find(SEPARATOR, 0, TRUE);
		if(index < 0) continue;
		extracted = extracted.right(extracted.length()-index-1);
		l->append(extracted);
	}
	l->append(remainder);
	return l;
}

