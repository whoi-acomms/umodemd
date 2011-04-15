#    ROHC Project 2003 at Lulea University of Technology, Sweden.
#    Authors: Andreas Vernersson <andver-8@student.luth.se>
#             Daniel Pettersson <danpet-7@student.luth.se>
#             Erik Soderstrom <soderstrom@yahoo.com>
#             Fredrik Lindstrom <frelin-9@student.luth.se>
#             Johan Stenmark <johste-8@student.luth.se>
#             Martin Juhlin <juhlin@users.sourceforge.net>
#             Mikael Larsson <larmik-9@student.luth.se>
#             Robert Maxe <robmax-1@student.luth.se>
#             
#    Copyright (C) 2003 Andreas Vernersson, Daniel Pettersson, 
#    Erik Soderström, Fredrik Lindström, Johan Stenmark, 
#    Martin Juhlin, Mikael Larsson, Robert Maxe.  
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
SOURCE = src/c_udp.c src/c_uncompressed.c src/c_ip.c src/c_udp_lite.c src/c_util.c src/comp.c \
	src/d_udp.c src/d_uncompressed.c src/d_ip.c src/d_udp_lite.c src/d_util.c src/decomp.c src/feedback.c \
	include/rohc.h include/comp.h include/c_util.h include/decomp.h include/d_ip.h \
	include/d_uncompressed.h include/d_udp_lite.h include/d_udp.h include/d_util.h include/feedback.h \
	src/proc_rohc.c include/proc_rohc.h include/ppp_rohc.h src/proc_rohc.c


CFLAGS_KERNEL = -DROHC_COMP -D__KERNEL__ -I/usr/src/linux-2.4.20/include -Iinclude -Wall \
 -Wstrict-prototypes -Wno-trigraphs -g -O2 -fomit-frame-pointer -fno-strict-aliasing -fno-common \
 -pipe -mpreferred-stack-boundary=2 -march=i686 \
 -DMODULE -DMODVERSIONS -include /usr/src/linux-2.4.20/include/linux/modversions.h \
 -nostdinc -iwithprefix include -DEXPORT_SYMTAB -c

#CFLAGS_USER_GPROF = -DUSER_SPACE -Wall -Wstrict-prototypes -Iinclude -O2 -pg -fprofile-arcs
#CFLAGS_USER_OPT = -DUSER_SPACE -Wall -Wstrict-prototypes -Iinclude -O2 -fomit-frame-pointer
CFLAGS_USER = -DUSER_SPACE -Wall -Wstrict-prototypes -Iinclude -g

CC = gcc
CP = cp
DEPMOD = depmod -ae



all : testapp

module : rohc.o ppp_generic.o pppoe.o

rohc.o : rohc_module.c $(SOURCE)
	$(CC) $(CFLAGS_KERNEL) -DKBUILD_BASENAME=rohc $< -o $@

ppp_generic.o : ppp_generic.c
	$(CC) $(CFLAGS_KERNEL) -DKBUILD_BASENAME=ppp_generic $< -o $@

pppoe.o : pppoe.c
	$(CC) $(CFLAGS_KERNEL) -DKBUILD_BASENAME=pppoe $< -o $@

testapp : src/testapp.c $(SOURCE)
	$(CC) $(CFLAGS_USER) $< -o $@

install : rohc.o ppp_generic.o pppoe.o
	$(CP) rohc.o ppp_generic.o pppoe.o /lib/modules/2.4.20/kernel/drivers/net
	$(DEPMOD)

clean :
	rm -rf testapp rohc.o ppp_generic.o pppoe.o
