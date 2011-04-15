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
#include "proccom.h"
#include <qcstring.h>
#include <stdio.h>
#include <stdlib.h>
//#include <kmessagebox.h>

ProcCom::ProcCom(QCString fileName){
	file=fileName;
}

ProcCom::~ProcCom(){

}


// Öppnar filen, läser den, stänger sedan filen. Returnerar strängen från filen.
QCString *ProcCom::getData(){
  QCString *procString = new QCString();
  QCString *systemCall = new QCString();
  char *buf;
  int size;
  int sysCall=0;
  systemCall->append("cat ");
  systemCall->append(file);
  systemCall->append(" > /tmp/rohc_proc_temp");  
//  sysCall = system("cat /proc/rohc/rohc_device1 > /tmp/rohc_proc_temp");
  sysCall = system(*systemCall);
  if(sysCall == -1){
    delete systemCall;
    return procString;
  }
  FILE *f = fopen("/tmp/rohc_proc_temp", "r");

  if (f) {
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    if(!size){
      delete systemCall;
      return procString;
    }
    fseek(f, 0, SEEK_SET);
    
    buf = malloc(size+1);
    buf[size] = 0;
    fread(buf, size, 1, f);
    fclose(f);

    procString->append(buf);

    free(buf);
  }
  delete systemCall;
  return procString;
}

// Öppnar filen, skriver till filen, stänger filen. Returnerar true vid lyckad skrivning.
bool ProcCom::setData(QCString parameterString){
	int length = qstrlen(parameterString);
	if(openProc()){
		fwrite(parameterString,1,length,fd);
		closeProc();
		return true;
	}
	else
		return false;
}

// Öppnar filen
bool ProcCom::openProc(){
	fd = fopen(file,"r+");
	if(fd)
		return true;
	else{
//		KMessageBox::information(this, "Err! Procdevice [%s] is not available", file);
		return false;
	}
}


// Stänger filen
void ProcCom::closeProc(){
	fclose(fd);
}
