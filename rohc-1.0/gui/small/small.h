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

#ifndef SMALL_H
#define SMALL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kapp.h>
#include <qwidget.h>
#include <qcstring.h>
#include <qlistbox.h>
#include <kmainwindow.h>
// #include "objhandler.h"
#include "saken.h"

/** Small is the base class of the project */
class Small : public KMainWindow
{
  Q_OBJECT 
  public:
    /** construtor */
    Small(QWidget* parent=0, const char *name=0);
    /** destructor */
    ~Small();
    void setVariable(int type, int no, QCString value, int position);
    void updateProfileMenu(int no, QCString value);
    void updateContextMenu(int no, QCString value);
    void popUp(QCString value);
    void setLabels(int type);
    void ClearListBox(int nr);
    void selectContext(int nr);
    void setGraph(QCString label);
    void centerContextItem(void);
    void noReset(void);
    QCString getDevice();  
   public slots:
      void editMode(bool state);
      void saveInstance();
      void timerDone();
      void profileSelected(int selected);
      void contextSelected(int selected);
      void contextClicked(QListBoxItem *item);

   private:

      void updateInstance(int no, QCString value);
      void updateProfile(int no, QCString value);
      void updateContext(int no, QCString value, int position);
      QColor color;
      QValueList<double> *samples;
      QValueList<double> *o_samples;

      bool inEditMode;
      Form1 *w;
      QTimer *timer;
};

#endif
