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

// Description: Creates and handles rohc's proc device..

#ifndef USER_SPACE

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/config.h>
#include <linux/types.h>
#include <asm/byteorder.h>
#include <linux/seq_file.h>

static struct proc_dir_entry *proc_rohc_dir;
static struct proc_dir_entry *entry;

#endif

#include "proc_rohc.h"
#include "comp.h"
#include "decomp.h"

#define MAX_INPUT_LEN 1000
#define VAR_LENGTH 50

static struct sc_rohc *compressor = NULL;
static struct sd_rohc *decompressor = NULL;

int get_token(char *dest, char *src, int *ppos, int len, int *eof, int mode) {
	int ofs = 0;
	src += *ppos;
	while (*src!=0 && *src!='\n' && *src!='=' && ofs+1<VAR_LENGTH && *ppos < len) {
		*dest = *src;
		dest++;
		src++;
		ppos[0]++;
		ofs++;
	}

	*dest = 0;

	switch (*src) {
	case 0: *eof = 1; break;
	case '\n': if (mode==0) return 0;
		ppos[0]++;
	break;
	case '=':  if (mode==1) return 0;
		ppos[0]++;
	break;
	}

	if (*ppos >= len)
		*eof = 1;

	return 1;
}

static void rohc_parser(char *buffer, int len)
{
	int pos = 0;//, value;
	int eof=0;
	char var[VAR_LENGTH], val[VAR_LENGTH];

	while (!eof) {
		if (!get_token(var, buffer, &pos, len, &eof, 0)) {
			rohc_debugf(2, "rohc_proc_parser: get_token 1 failed\n");
			return;
		}


		if (eof) {
			rohc_debugf(2, "rohc_proc_parser: eof failed\n");
			break;
		}

		if (!get_token(val, buffer, &pos, len, &eof, 1)) {
			rohc_debugf(2, "rohc_proc_parser: get_token 2 failed\n");
			return;
		}

		rohc_debugf(0, "rohc_proc_parser: '%s'='%s'\n", var, val);
		if (compressor && decompressor) {
			if (strcmp(var, "MRRU")==0) {
				// anropa motsvarande funktion
				// mrru, segmentering, aktiva profiler.. max_cid, large_cid, feedback_freq, connection_type
    				rohc_c_set_mrru(compressor,simple_strtol(val, 0, 10));

			} else if(strcmp(var, "MAX_CID")==0) {
				rohc_c_set_max_cid(compressor,simple_strtol(val, 0, 10));

	 		} else if(strcmp(var, "LARGE_CID")==0) {
				rohc_c_set_large_cid(compressor,simple_strtol(val, 0, 10));

			} else if(strcmp(var, "CONNECTION_TYPE")==0) {
				rohc_c_set_connection_type(compressor,simple_strtol(val, 0, 10));

			} else if(strcmp(var, "FEEDBACK_FREQ")==0) {
				usergui_interactions(decompressor, simple_strtol(val, 0, 10));

			} else if (strcmp(var, "ROHC_ENABLE")==0) {
				rohc_debugf(0, "rohc_enable\n");

				if (strcmp(val, "YES")==0) {
					rohc_debugf(0, "rohc_enable=yes\n");
					// enable compression..
					rohc_c_set_enable(compressor, 1);
				} else if (strcmp(val, "NO")==0) {
					rohc_debugf(0, "rohc_enable=no\n");
					// disable compression..
					rohc_c_set_enable(compressor, 0);
				}
			}
		} else {
			rohc_debugf(0, "Tried to set value in rohc, but rohc is not allocated yet\n");
		}
	}
}
// -----------------------------------------------------------------


#ifndef USER_SPACE

static int proc_state = 0; // 0 == unused, 1==comp, 2==decomp..
static int proc_offset = 0;

static int proc_rohc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int rohc_len;

	MOD_INC_USE_COUNT;

	if (count < 1000) {
		rohc_debugf(0, "warning! proc_rohc_read() called with buffer size=%d\n", count);
		*eof = 1;
		MOD_DEC_USE_COUNT;
		return 0;
	}

	rohc_debugf(2, "proc_rohc_read(): count=%d off=%d rohc_state=%d\n", count, (int)off, proc_state);

	rohc_len = 0;
	memset(page, 0, count); // page[0] = 0;
	*start = page;

	if (off == 0) {
		proc_offset = 0;
		rohc_len = rohc_c_info(page);

		if (compressor != NULL && decompressor != NULL) {
			rohc_len = rohc_c_statistics(compressor, page);
			rohc_len = rohc_d_statistics(decompressor, page);
			proc_state = 1;
		} else {
			proc_state = 3;
		}
	}


	if (proc_state == 1 && rohc_len == 0) {
		do {
			rohc_len = rohc_c_context(compressor, proc_offset, page);
			proc_offset++;

		} while (rohc_len == -1);

		if (rohc_len == -2) {
			proc_state = 2;
			proc_offset = 0;
			rohc_len = strlen(page);
		}
	}

	if (proc_state == 2 && rohc_len == 0) {
		do {
			rohc_len = rohc_d_context(decompressor, proc_offset, page);
			proc_offset++;
		} while (rohc_len == -1);

		if (rohc_len == -2) {
			strcat(page, "\n");
			proc_state = 3;
			rohc_len = strlen(page);
		}
	}

	if (proc_state == 3 && rohc_len == 0) {
		eof[0] = 1;
		rohc_len = 0; // <-- important!
	}

	MOD_DEC_USE_COUNT;

	return rohc_len;
}

static int proc_rohc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
  char kbuffer[MAX_INPUT_LEN];
  int len;

  rohc_debugf(2,"ROHC: write\n");

  MOD_INC_USE_COUNT;

  //fix max input len
  if(count > MAX_INPUT_LEN)
    len = MAX_INPUT_LEN;
  else
    len = count;

  if(copy_from_user(&kbuffer, buffer, len)) {
    MOD_DEC_USE_COUNT;
    return -EFAULT;
  }

  rohc_parser(kbuffer, len);

  //  fb_data->cause[len] = '\0';

  MOD_DEC_USE_COUNT;

  return len;
  //  return 0;
}
#endif

void rohc_proc_set(struct sc_rohc *c, struct sd_rohc *d) {
	compressor = c;
	decompressor = d;
}

int rohc_proc_init(void) {
#ifndef USER_SPACE
	proc_rohc_dir = proc_mkdir("rohc", NULL);
	entry = create_proc_entry("rohc_device1", 0644, proc_rohc_dir);
	if(entry == NULL) {
		return -ENOMEM;
	}

	//  entry->data = &rohc_utdata;
	entry->data = NULL; //&rohc_data;
	entry->read_proc = proc_rohc_read;
	entry->write_proc = proc_rohc_write;
	entry->owner = THIS_MODULE;

#endif
	rohc_debugf(1,"ROHC: ---- PROC LOADED ----\n");
	return 0;
}


void rohc_proc_remove(void)
{
#ifndef USER_SPACE
  	remove_proc_entry("rohc_device1", entry);
	remove_proc_entry("rohc", NULL);
#endif
	rohc_debugf(1,"ROHC: ---- PROC REMOVED ----\n");
}


