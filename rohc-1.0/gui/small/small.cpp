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

#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qlineedit.h>
#include <kmessagebox.h>
#include <qtimer.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlistbox.h>
#include <qslider.h>
#include <qlabel.h>
#include <qstring.h>
#include <qcstring.h>
#include "small.h"
#include "saken.h"
#include "objhandler.h"
#include <SignalPlotter.h>
#include <qvaluelist.h>
#include <stdlib.h>

#define INSTANCE 1
#define PROFILE  2
#define CONTEXT  3

ObjHandler *object;
int read_proc=51;
int reset_graph=1;
int updatedContext=0;

Small::Small(QWidget *parent, const char *name) : KMainWindow(parent, name)
{
        QColor o_color = QColor(0xE0FF8C);
        const QString title = QString("Compressed headersize");
        const QString o_title = QString("Original headersize");
        this->w = new Form1(this, "Form1");
        setCaption("Performance and Benchmarking application");
        object = new ObjHandler(this);

        timer = new QTimer(this);
        setCentralWidget(w);
        inEditMode=false;

        connect(w->pushButton2_2, SIGNAL(toggled(bool)), this, SLOT(editMode(bool)));
        connect(w->pushButton2, SIGNAL(clicked()), this, SLOT(saveInstance()));

        connect(timer, SIGNAL(timeout()), this, SLOT(timerDone()));

        connect(w->listBox2, SIGNAL(highlighted(int)), this, SLOT(profileSelected(int)));
        connect(w->listBox3, SIGNAL(highlighted(int)), this, SLOT(contextSelected(int)));
        connect(w->listBox3, SIGNAL(clicked(QListBoxItem *)), this, SLOT(contextClicked(QListBoxItem *)));

        color = QColor(0x63A9FF);
        w->signalPlotter1->addBeam(color);

//        w->signalPlotter1->addBeam(o_color);

//        w->signalPlotter1->addBeam(color);
        w->signalPlotter2->addBeam(o_color);
        w->signalPlotter2->setTitle(title);
        w->signalPlotter1->setTitle(o_title);

        color = QColor(0xFF0000);
        o_color = QColor(0x0000FF);

        w->signalPlotter3->addBeam(color);
        w->signalPlotter4->addBeam(o_color);

        samples = new QValueList<double>();
        o_samples = new QValueList<double>();        
        w->signalPlotter1->setAutoRange(1);
        w->signalPlotter2->setAutoRange(1);        
        w->signalPlotter3->setAutoRange(1);
        w->signalPlotter4->setAutoRange(1);
        w->signalPlotter5->setAutoRange(1);
        w->signalPlotter6->setAutoRange(1);

        timer->start(100, TRUE );
}

Small::~Small()
{
  delete object;
  delete timer;
  delete w;
  delete samples;
  delete o_samples;
}

void Small::popUp(QCString value)
{
  KMessageBox::information(this, value);
}
void Small::updateProfileMenu(int no, QCString value)
{
  QString *newItem = new QString(value);
  w->listBox2->removeItem(no);
  w->listBox2->insertItem(*newItem,no);
  delete newItem;
}

void Small::updateContextMenu(int no, QCString value)
{
  QString *newItem = new QString(value);
  w->listBox2->removeItem(no);
  w->listBox2->insertItem(*newItem,no);
  delete newItem;

}

void Small::setVariable(int type, int no, QCString value, int position)
{
  switch(type){
    case INSTANCE:
      updateInstance(no,value);
      break;
    case PROFILE:
      updateProfile(no,value);
      break;
    case CONTEXT:
      updateContext(no,value, position);    
      break;
    }
}

void Small::updateInstance(int no, QCString value)
{
  bool checked = false;
          switch(no){
      case 0:
        w->textLabel10->setText(value);
        break;
      case 1:
        w->textLabel11->setText(value);
        break;
      case 2:
        w->textLabel12->setText(value);
        if(value == "ENABLED")
          checked = true;
        w->checkBox2  ->setChecked(checked);
        checked=false;

        break;
      case 3:
        w->textLabel13->setText(value);
        break;
      case 4:
        w->textLabel14->setText(value);
        break;
      case 5:
        w->textLabel15->setText(value);
        break;
      case 6:
        w->spinBox2->setSpecialValueText(value);
        break;
      case 7:
        w->spinBox3->setSpecialValueText(value);
        break;
      case 8:
        if(value == "YES")
          checked = true;
        w->checkBox1->setChecked(checked);
        checked=false;
        break;
      case 9:
        w->comboBox1->setCurrentItem(value.toInt());
        break;
      case 10:
//        w->spinBox5->setSpecialValueText(value);
        w->slider2->setValue(value.toInt());
      case 11:
        if(value == "YES")
          checked = true;
//        w->checkBox2->setChecked(checked);
        checked=false;
        break;
      default:
        break;
    }
}

void Small::updateProfile(int no, QCString value)
{
  QString *infoLine;
  switch(no){
      case 0:
        w->textLabel21->setText(value);
        break;
      case 1:
        w->textLabel22->setText(value);
        break;
      case 2:
        w->textLabel23->setText(value);
        break;
      case 3:
        w->textLabel3_4->setText(value);
//        w->spinBox4->setSpecialValueText(value);
        break;
      case 10:
        infoLine = new QString(value);
        w->listBox2->insertItem(*infoLine,-1);
        delete infoLine;
        break;
      default:
        break;
    }
}
void Small::updateContext(int no, QCString value, int position)
{
  QString *infoLine;
  switch(no){
      case 0:
        infoLine = new QString(value);
        if(position>=0 && w->listBox3->text(position)!=NULL){
          if(w->listBox3->text(position) != *infoLine){
             updatedContext=1;
             w->listBox3->changeItem(*infoLine,position);
          }
        }
        else
          w->listBox3->insertItem(*infoLine,-1);
        delete infoLine;
        break;
      case 1:
        w->textLabel42->setText(value);
        break;
      case 2:
        w->textLabel43->setText(value);
        break;
      case 3:
        w->textLabel44->setText(value);
        break;
      case 4:
        w->textLabel45->setText(value);
        break;
      case 5:
        w->textLabel46->setText(value);
        break;
      case 6:
        w->textLabel47->setText(value);
        break;
      case 7:
        w->textLabel48->setText(value);
        break;
      case 8:
        w->textLabel49->setText(value);
        break;
      case 9:
        w->textLabel50->setText(value);
        break;
      case 10:
        w->textLabel51->setText(value);
        break;
      case 11:
        w->textLabel52->setText(value);
        break;
      case 12:
        w->textLabel53->setText(value);
        break;
      case 13:
        w->textLabel54->setText(value);
        break;
      case 14:
        w->textLabel55->setText(value);
        break;
      case 15:
        w->textLabel56->setText(value);
        break;
      case 16:
        w->textLabel57->setText(value);
        break;
      default:
        break;        
    }
}

void Small::setGraph(QCString label){
  int i=0;

  if(reset_graph && !updatedContext){
    samples->append(0.0);
    for(i=0;i<61;i++){
      w->signalPlotter1->addSample(*samples);
      w->signalPlotter2->addSample(*samples);
      w->signalPlotter3->addSample(*samples);
      w->signalPlotter4->addSample(*samples);
      w->signalPlotter5->addSample(*samples);
      w->signalPlotter6->addSample(*samples);
    }
    samples->pop_front();
    reset_graph=0;
  }
//  else{
//    reset_graph=0;
//    updatedContext=0;
//  }

  w->textLabel7_2->setText(label);
}

void Small::ClearListBox(int nr){
      switch(nr){
         case 1:
          break;
         case 2:
          w->listBox2->clear();
          break;
         case 3:
          w->listBox3->removeItem(w->listBox3->count()-1);
          break;
         default:
          break;

        }

}

void Small::selectContext(int nr){
  reset_graph=1;
  if(updatedContext)
    reset_graph=0;
  if(w->listBox3->count()>1)
    w->listBox3->setSelected(nr,true);
}

void Small::profileSelected(int selected){
  object->getProfile(selected);
}

void Small::contextClicked(QListBoxItem *item){
  reset_graph=1;
  updatedContext=0;
}

void Small::contextSelected(int selected){
  object->getContext(selected);
}



void Small::timerDone()
{
  QString qstemp;
  int time=0;

  double sample;
  double o_sample;
  
  if(inEditMode){
    time=w->spinBox5_2->value();
    time*=1000;
    timer->start(time, TRUE );
    return;
  }
  reset_graph=0;
  if(!object->readProc()){
      reset_graph=1;
      time=w->spinBox5_2->value();
      time*=1000;
      timer->start(time, TRUE );
      return;
  }
  
  qstemp = w->textLabel42->text();
  sample = qstemp.toDouble();

  qstemp = w->textLabel43->text();
  o_sample = qstemp.toDouble();

  o_samples->append(o_sample);
  samples->append(sample);

  w->signalPlotter1->addSample(*samples);
  w->signalPlotter2->addSample(*o_samples);

  samples->pop_front();
  o_samples->pop_front();

    //// Next 2 graph
      
  qstemp = w->textLabel46->text();
  sample = qstemp.toDouble();
                                                                  
  qstemp = w->textLabel47->text();
  o_sample = qstemp.toDouble();

  o_samples->append(o_sample);
  samples->append(sample);

  w->signalPlotter3->addSample(*samples);
  w->signalPlotter4->addSample(*o_samples);

  samples->pop_front();
  o_samples->pop_front();

    w->signalPlotter1->changeRange(0, 0, 100);
    w->signalPlotter2->changeRange(0, 0, 100);

    w->signalPlotter3->changeRange(0, 0, 100);
    w->signalPlotter4->changeRange(0, 0, 100);

  time=w->spinBox5_2->value();
  time*=1000;
  timer->start(time, TRUE );
  
}
void Small::centerContextItem(void){
  w->listBox3->centerCurrentItem();
}

void Small::setLabels(int type)
{
  switch(type){
    case 1:  // Compressor
      w->textLabel36->setText("Number of sent packets");
      w->textLabel37->setText("Number of sent IR packets");
      w->textLabel38->setText("Number of sent IR-DYN packets");
      w->textLabel39->setText("Number of received feedbacks");
      w->textLabel40->setText("");
      w->textLabel41->setText("");
      w->textLabel56->setText("");
      w->textLabel57->setText("");

      break;
    case 2: // Decompressor
      w->textLabel36->setText("Number of received packets");
      w->textLabel37->setText("Number of received IR packets");
      w->textLabel38->setText("Number of received IR-DYN packets");
      w->textLabel39->setText("Number of sent feedbacks");
      w->textLabel40->setText("Number of decompression failures");
      w->textLabel41->setText("Number of decompression repairs");
      break;
    }
}

void Small::saveInstance(){
  QCString store;
  QCString temp;
  store.append("MAX_CID=");
  store.append(w->spinBox2->text());
  store.append("\nMRRU=");
  store.append(w->spinBox3->text());
  store.append("\nLARGE_CID=");
  if(w->checkBox1->isChecked())
    store.append("YES");
  else
    store.append("NO");
  store.append("\nCONNECTION_TYPE=");
//  store.append(w->comboBox1->text(w->comboBox1->currentItem()));
//  temp = new QCString();
//  temp = w->comboBox1->currentItem();
  temp.setNum(w->comboBox1->currentItem());
  store.append(temp);
  store.append("\nFEEDBACK_FREQ=");
  temp.setNum(w->slider2->value());
  store.append(temp);

  store.append("\nROHC_ENABLE=");
  if(w->checkBox2->isChecked())
    store.append("YES\n");
  else
    store.append("NO\n");

//  KMessageBox::information(this, store);
  editMode(false);
  w->pushButton2_2->setOn(false);
  object->writeProc(store);  
}

QCString Small::getDevice()
{
  return  (QCString )w->lineEdit1->text();
}
void Small::editMode(bool state)
{
  if(state){    // Edit mode
    w->spinBox2->setEnabled(true);
    w->checkBox1->setEnabled(true);
    w->comboBox1->setEnabled(true);
    w->slider2->setEnabled(true);
    w->checkBox2->setEnabled(true);
    w->pushButton2->setEnabled(true);
    inEditMode=true;
  }
  else{         // Normal mode
    w->spinBox2->setEnabled(false);
    w->checkBox1->setEnabled(false);
    w->comboBox1->setEnabled(false);
    w->slider2->setEnabled(false);
    w->checkBox2->setEnabled(false);
    w->pushButton2->setEnabled(false);
    inEditMode=false;
  }
}

