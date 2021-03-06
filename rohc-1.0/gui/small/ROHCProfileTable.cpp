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
    Erik Soderstr�m, Fredrik Lindstr�m, Johan Stenmark, 
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
#include "ROHCProfileTable.h"
#include <qcstring.h>
#include <qvaluelist.h>

ROHCProfileTable::ROHCProfileTable(){
  lastList = new QValueList<QCString>();
}

ROHCProfileTable::~ROHCProfileTable(){
	delete lastList;
}

QValueList<QCString> *ROHCProfileTable::fixInfo(QValueList<QCString> *orgList){
	QCString temp; // = new QCString();
	int i = 0;
	for(i=0;i<4;i++){
		temp.append(orgList->first());
    temp.append(" , ");
		orgList->pop_front();
	}
	orgList->push_front(temp);
	return orgList;

}


// Returnerar vid fall av linkedlist som parameterh�llare en l�nkad lista med integers
// som s�ger vilka positioner i listan som �r �ndrad.
void ROHCProfileTable::updateAll(QValueList<QCString> *newList){
//	int no=0;
	QValueList<QCString> *temp = new QValueList<QCString>(*newList);
  delete lastList;
  lastList = temp;
/*	QValueList<int> *result = new QValueList<int>();
  QCString last, newS;
	if(!lastList->isEmpty()){
    lastList->begin();
    newList->begin();
    if(!lastList->operator==(*newList)){

      do{
          last=lastList->first();
          newS=newList->first();
          lastList->pop_front();
          newList->pop_front();
          if(last != newS){
		    		result->push_back(no);
  			  }
          lastList->append(newS);
          newList->append(newS);
  			  no++;
	  	}while(no<newList->size());
    }
    return result;
	}
	else{
		do{
			result->push_back(no++);
		}while(newList->count() > no);
		lastList = temp;
    return result;
	}
  */
}
