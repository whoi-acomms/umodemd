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
#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/kernel.h>

#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/termios.h>
#include <linux/in.h>
#include <linux/udp.h>
#include <linux/fcntl.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/timer.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <net/checksum.h>
#include <asm/unaligned.h>

#include "src/c_ip.c"
#include "src/c_udp.c"
#include "src/c_udp_lite.c"
#include "src/c_uncompressed.c"

#include "src/c_util.c"
#include "src/comp.c"

#include "src/d_util.c"
#include "src/d_ip.c"
#include "src/d_udp.c"
#include "src/d_udp_lite.c"
#include "src/d_uncompressed.c"
#include "src/decomp.c"

#include "src/feedback.c"

#include "src/proc_rohc.c"

#include "exports.h"
/*

EXPORT_SYMBOL(rohc_alloc_compressor);
EXPORT_SYMBOL(rohc_free_compressor);
EXPORT_SYMBOL(rohc_compress);
EXPORT_SYMBOL(rohc_using_small_cid);

EXPORT_SYMBOL(rohc_alloc_decompressor);
EXPORT_SYMBOL(rohc_free_decompressor);
EXPORT_SYMBOL(rohc_decompress);
EXPORT_SYMBOL(rohc_decompress_both);

EXPORT_SYMBOL(rohc_set_mrru);
EXPORT_SYMBOL(rohc_set_header);
EXPORT_SYMBOL(rohc_activate_profile);

EXPORT_SYMBOL(rohc_proc_set);

EXPORT_SYMBOL(rohc_c_is_enabled);
*/

#ifdef MODULE

int init_module(void)
{
	printk(KERN_INFO "rohc.o: Copyright (c) 2003, ROHC Project 2003 at Lulea University of Technology, Sweden\n");

	crc_init_table(crc_table_3, crc_get_polynom(CRC_TYPE_3));
	crc_init_table(crc_table_7, crc_get_polynom(CRC_TYPE_7));
	crc_init_table(crc_table_8, crc_get_polynom(CRC_TYPE_8));

	return rohc_proc_init();
}

void cleanup_module(void)
{
	rohc_proc_remove();
	return;
}

#endif /* MODULE */


MODULE_LICENSE("GPL");
