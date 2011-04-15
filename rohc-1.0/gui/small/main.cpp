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

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

#include "small.h"

static const char *description =
	I18N_NOOP("ROHC-GUI");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE
	
	
static KCmdLineOptions options[] =
{
  { 0, 0, 0 }
  // INSERT YOUR COMMANDLINE OPTIONS HERE
};

int main(int argc, char *argv[])
{

  KAboutData aboutData( "ROHC-GUI", I18N_NOOP("ROHC-GUI"),
    VERSION, description, KAboutData::License_GPL,
    "(c) 2003, erik", 0, 0, "erik@red4");
  aboutData.addAuthor("erik",0, "erik@red4");
  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

  KApplication a;
  Small *small = new Small();
  a.setMainWidget(small);
  small->show();  

  return a.exec();
}
