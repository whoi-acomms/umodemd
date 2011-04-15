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
#include "objhandler.h"

ObjHandler::ObjHandler(Small *_gui){
  gui = _gui;
  parser = new Parser();
  instanceTable = new ROHCInstanceTable();
  contextList = new QPtrList<ROHCContextTable>();
  profileList = new QPtrList<ROHCProfileTable>();
  selectedProfile=0;
  selectedContext=0;
}

ObjHandler::~ObjHandler(){
  ROHCContextTable *contextTemp;
  ROHCProfileTable *profileTemp;
  
  delete parser;
  delete instanceTable;

  while(!contextList->isEmpty()){
    contextTemp = contextList->getFirst();
    contextList->removeFirst();
    delete contextTemp;
  }
  
  while(!profileList->isEmpty()){
    profileTemp = profileList->getFirst();
    profileList->removeFirst();
    delete profileTemp;
  }

  delete contextList;
  delete profileList;

}

void ObjHandler::getProfile(int nr){

  ROHCProfileTable *rohcTable = profileList->take(nr);
  QCString temp;
  int no=0;
  if(!rohcTable)
    return;
  do{
     temp = rohcTable->first();
     gui->setVariable(2,no,temp,0);
     rohcTable->pop_front();
     rohcTable->append(temp);

     no++;
	}while(no<rohcTable->size());
  profileList->insert(nr,rohcTable);
  selectedProfile=nr;
}

void ObjHandler::getContext(int nr){

  ROHCContextTable *rohcTable = contextList->take(nr);
  QCString temp;
  int no=0;
  if(!rohcTable)
    return;
     gui->setLabels(rohcTable->compressor);
     temp=rohcTable->first();
     rohcTable->pop_front();
     gui->setGraph(temp);
     rohcTable->append(temp);

     no++;
     do{
          temp = rohcTable->first();
          gui->setVariable(3,no,temp,nr);
          rohcTable->pop_front();
          rohcTable->append(temp);
          no++;
	  	}while(no<rohcTable->size());
  contextList->insert(nr,rohcTable);
  selectedContext=nr;
}

void ObjHandler::writeProc(QCString store){
  procDevice = new ProcCom(gui->getDevice());
  procDevice->setData(store);
  delete procDevice;
}

bool ObjHandler::readProc(){
  int i = 1, j = 0, clear = 1, position=0, storedPos=0, cleared = 0, removed=0;
  unsigned int orgSize=0;
  QPtrList<QValueList <QCString> > *instance;
  QValueList<QCString> *instanceValues;
  ROHCContextTable *contextTable;
  ROHCProfileTable *profileTable;
  procDevice = new ProcCom(gui->getDevice());  
  procFile = procDevice->getData();
  if(procFile->isEmpty()){
    delete procFile;
    delete procDevice;
    return false;
  }
  parsedList = parser->parseString(*procFile);
  delete procFile;
  storedPos = selectedContext;
  while(!parsedList->isEmpty()){
    instance = parsedList->getFirst();
    orgSize = instance->count();
    parsedList->removeFirst();
    j=0;
    clear=1;
    position=0;
    while(!instance->isEmpty()){
      instanceValues = instance->getFirst();
      instance->removeFirst();
      j=0;
      if(i==1){
         instanceTable->updateAll(instanceValues);
         while(!instanceValues->isEmpty()){
          gui->setVariable(i,j++,instanceValues->first(),-1);
          instanceValues->pop_front();
        } 
      }
      else if(i==2){
         if(profileList->count() == orgSize){
            profileTable = profileList->at(position);
            profileTable->updateAll(instanceValues);

            if(position++ != selectedProfile)
              instanceValues->clear();
         }
         else{
           if(clear){
             profileList->clear();
             gui->ClearListBox(2);
             clear = 0;
           }
           profileTable = new ROHCProfileTable();
           profileTable->updateAll(instanceValues);
           profileList->append(profileTable);
           while(!instanceValues->isEmpty()){
            gui->setVariable(i,j++,instanceValues->first(),0);
            if(j == 4 && i == 2)
              gui->setVariable(2,10,instanceValues->first(),0);
            instanceValues->pop_front();
          }
        }
      }
      else if(i==3){
          if(contextList->count() == orgSize){

            contextTable = contextList->at(position);
            contextTable->fixInfo(instanceValues);
            contextTable->updateAll(instanceValues);
            if(position != storedPos){
              gui->setVariable(i,j++,instanceValues->first(),position);
              instanceValues->clear();
            }
            else
                gui->setLabels(contextTable->compressor);

            while(!instanceValues->isEmpty()){
                gui->setVariable(i,j++,instanceValues->first(),position);
                instanceValues->pop_front();
            }
            position++;                
          }
          else{
            if(position>=orgSize || contextList->count()<orgSize)
            {
               contextTable = new ROHCContextTable();
               contextList->append(contextTable);
            }
            else if(contextList->count() > orgSize && !removed){
              for(int i=0;i<(orgSize - contextList->count());i++){
                gui->ClearListBox(3);
                contextList->removeLast();
                contextList->count();
                contextList->count();
              }
              contextTable = contextList->at(position);
              removed=1;
            }
            else{
              contextTable = contextList->at(position);
            }
                          
            if(contextTable->fixInfo(instanceValues)){
              contextTable->updateAll(instanceValues);
              
              if(position != storedPos){
                gui->setVariable(i,j++,instanceValues->first(),position);
                instanceValues->clear();
              }
              else{
               gui->setLabels(contextTable->compressor);
              }
              
              while(!instanceValues->isEmpty()){
                gui->setVariable(i,j++,instanceValues->first(),position);
                instanceValues->pop_front();
              }       
            }
            else
              contextList->removeLast();
              position++;
            }                      
        }        
        while(!instanceValues->isEmpty()){
          gui->setVariable(i,j++,instanceValues->first(),-1);
          instanceValues->pop_front();
        }
      delete instanceValues;
    }
    gui->selectContext(storedPos);
    if(cleared)
      gui->centerContextItem();
    i++;
    delete instance;
  }
  delete parsedList;
  delete procDevice;
  return true;
}

