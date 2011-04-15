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
/* Mini ROHC header file, for usage in ppp_generic.c only..
 *
 * For a more detailed description of the functions,
 * see comp.h and decomp.h
 */

#ifndef _PPP_ROHC_H
#define _PPP_ROHC_H

#define PPP_SMALL_ROHC 0x0003
#define PPP_LARGE_ROHC 0x0005

void    *rohc_alloc_compressor(int cid);
void    rohc_free_compressor(void *compressor);
int     rohc_compress(void *state, unsigned char *rptr, int isize,
			unsigned char *obuf, int osize);

void	*rohc_alloc_decompressor(void *compressor);
void	rohc_free_decompressor(void *state);
int	rohc_decompress(void * state, unsigned char *ibuf, int isize,
			unsigned char *obuf, int osize);
int rohc_decompress_both(void * state, unsigned char * ibuf, int isize,
			unsigned char * obuf, int osize, int large);

// Set values

void rohc_set_mrru(void * comp_state, int mrru);
void rohc_set_header(void * comp_state, int header);
void rohc_activate_profile(void * comp_state, int profile);

int rohc_c_using_small_cid(void * ch);
int rohc_c_is_enabled(void *compressor);
void rohc_c_set_mrru(void *compressor, int value);
void rohc_c_set_header(void *compressor, int value);

// store compressor and decompressor pointer to proc handler (rohc_proc.c)
void rohc_proc_set(void *compressor, void *decompressor);


#endif
