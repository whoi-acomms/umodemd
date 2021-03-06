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
/*
Implementation of the udp profile (compressor)
*/

#include <linux/ip.h>

#include "rohc.h"
#include "comp.h"
#include "c_util.h"


static const char *udp_packet_types[] = {
	"IR", "IRDYN", "OU-0", "OU-1", "OU-2"
};



static const char *udp_extension_types[] = {
	"NOEXT", "EXT0", "EXT1", "EXT2", "EXT3"
};


/*
Structure that contain counters, flags and structures that need to saved between different
packages. A context have a structure like this for the two first ip-headers.
*/
struct sc_udp_header
{
	struct sc_wlsb *ip_id_window;
	struct iphdr old_ip;

	int tos_count, ttl_count, df_count, protocol_count;
	int rnd_count, nbo_count;
	int old_rnd,old_nbo;
	int id_delta;

};

/*
Structure that contain variables that are temporar, i.e. they will only be jused in this
packet, they have to be reinitilized every time a new packet arrive.
*/
struct udp_tmp_variables{
	// The following variables are set in encode
	int nr_of_ip_hdr;
	unsigned short changed_fields;
	unsigned short changed_fields2;
	int send_static;
	int send_dynamic;
	int nr_ip_id_bits;
	int nr_sn_bits;
	int nr_ip_id_bits2;
	int packet_type;
	int max_size;

	int send_udp_dynamic;
};

/*
Structure that contain counters, flags and structures that need to saved between different
packages. Every context have one of these
*/
struct sc_udp_context {
	unsigned int sn;
	struct sc_wlsb *sn_window;

	int ir_count, fo_count, so_count;
	int go_back_fo_count, go_back_ir_count, ir_dyn_count;
	int udp_checksum_change_count;

	struct sc_udp_header ip_flags, ip2_flags;
	struct udphdr old_udp;
	struct udp_tmp_variables tmp_variables;
	int is_initialized; // if ip2 is initialized

};


static void udp_decide_state(struct sc_context *context);

static void c_udp_update_variables(struct sc_context *context,const struct iphdr *ip,
	const struct iphdr *ip2);

static int udp_decide_packet(struct sc_context *context);

static int udp_decide_FO_packet(struct sc_context *context);

static int udp_decide_SO_packet(const struct sc_context *context);

static void udp_periodic_down_transition(struct sc_context *context);

static int udp_code_packet(struct sc_udp_context *udp_profile,const struct iphdr *ip,
	const struct iphdr *ip2,const struct udphdr *udp,unsigned char *dest, struct sc_context *context);


static int udp_code_IR_packet(struct sc_udp_context *udp_profile,const struct iphdr *ip,
	const struct iphdr *ip2,const struct udphdr *udp,unsigned char *dest,struct sc_context *context);

static int udp_code_IR_DYN_packet(struct sc_udp_context *udp_profile,const struct iphdr *ip,
	const struct iphdr *ip2,const struct udphdr *udp,unsigned char *dest, struct sc_context *context);


static int udp_code_static_part(struct sc_context *context,struct sc_udp_header *udp_hdr_profile,
	const struct iphdr *ip,unsigned char *dest, int counter);

static int udp_code_static_udp_part(struct sc_context *context,
	const struct udphdr *udp,
	unsigned char *dest,int counter);

static int udp_code_dynamic_part(struct sc_context *context,struct sc_udp_header *udp_hdr_profile,
	const struct iphdr *ip,unsigned char *dest, int counter,int *rnd, int *nbo);

static int udp_code_dynamic_udp_part(struct sc_context *context,
	const struct udphdr *udp,
	unsigned char *dest,
	int counter);

static int udp_code_UO0_packet(struct sc_context *context,const struct iphdr *ip,
	unsigned char *dest);

static int udp_code_UO1_packet(struct sc_context *context,const struct iphdr *ip,
	unsigned char *dest);

static int udp_code_UO2_packet(struct sc_context *context,struct sc_udp_context *udp_profile,
	const struct iphdr *ip,const struct iphdr *ip2,unsigned char *dest);

static int udp_code_EXT0_packet(struct sc_context *context,const struct iphdr *ip,
	unsigned char *dest,int counter);

static int udp_code_EXT1_packet(struct sc_context *context,const struct iphdr *ip,
	unsigned char *dest,int counter);

static int udp_code_EXT2_packet(struct sc_context *context,struct sc_udp_context *udp_profile,
	const struct iphdr *ip,unsigned char *dest,int counter);

static int udp_code_EXT3_packet(struct sc_context *context,struct sc_udp_context *udp_profile,
	const struct iphdr *ip,const struct iphdr *ip2,
	unsigned char *dest,int counter);

static int udp_header_flags(struct sc_context *context,unsigned short changed_fields,
	struct sc_udp_header *udp_hdr_profile,int counter,boolean is_outer,
	int nr_ip_id_bits,const struct iphdr *ip,unsigned char *dest,
	int context_rnd,int context_nbo);

static int udp_header_fields(struct sc_context *context,unsigned short changed_fields,
	struct sc_udp_header *udp_hdr_profile,int counter,boolean is_outer,
	int nr_ip_id_bits,const struct iphdr *ip,unsigned char *dest,int context_rnd);

static int udp_decide_extension(struct sc_context *context);

static int udp_changed_static_both_hdr(struct sc_context *context,const struct iphdr *ip,
	const struct iphdr *ip2);

static int udp_changed_static_one_hdr(unsigned short changed_fields,struct sc_udp_header *ip_header,
	const struct iphdr *ip,struct sc_context *context);


static int udp_changed_dynamic_both_hdr(struct sc_context *context,const struct iphdr *ip,
	const struct iphdr *ip2);

static int udp_changed_dynamic_one_hdr(unsigned short changed_fields,struct sc_udp_header *udp_hdr_profile,
	const struct iphdr *ip,int context_rnd,int context_nbo,struct sc_context *context);

static int udp_changed_udp_dynamic(struct sc_context *context,const struct udphdr *udp);

static boolean udp_is_changed(unsigned short changed_fields,unsigned short check_field);

static unsigned short udp_changed_fields(struct sc_udp_header *udp_hdr_profile,const struct iphdr *ip);

static void udp_change_state(struct sc_context *, C_STATE);
static void udp_change_mode(struct sc_context *, C_MODE);

static void udp_check_ip_identification(struct sc_udp_header *udp_hdr_profile, const struct iphdr *ip,
	int *context_rnd, int *context_nbo);

/*
Initilize one ip-header context
*/
void c_udp_init_context(struct sc_udp_header *ip_header,const struct iphdr *ip,int context_rnd,int context_nbo)
{
	ip_header -> ip_id_window = c_create_wlsb(16,C_WINDOW_WIDTH,0);
	ip_header->old_ip = *ip;
	ip_header->old_rnd = context_rnd;
	ip_header->old_nbo = context_nbo;
	ip_header->tos_count = MAX_FO_COUNT;
	ip_header->ttl_count = MAX_FO_COUNT;
	ip_header->df_count = MAX_FO_COUNT;
	ip_header->protocol_count = MAX_FO_COUNT;
	ip_header->rnd_count = MAX_FO_COUNT;
	ip_header->nbo_count = MAX_FO_COUNT;
}

/*
Initilize all temp variables
*/
void c_udp_init_tmp_variables(struct udp_tmp_variables *udp_tmp_variables){
	udp_tmp_variables->nr_of_ip_hdr = -1;
	udp_tmp_variables->changed_fields = -1;
	udp_tmp_variables->changed_fields2 = -1;
	udp_tmp_variables->send_static = -1;
	udp_tmp_variables->send_dynamic = -1;
	udp_tmp_variables->nr_ip_id_bits = -1;
	udp_tmp_variables->nr_sn_bits = -1;
	udp_tmp_variables->nr_ip_id_bits2 = -1;
	udp_tmp_variables->packet_type = -1;
	udp_tmp_variables->max_size = -1;
	udp_tmp_variables->send_udp_dynamic = -1;
}

/*
Function to allocate for a new context, it aslo initilize alot of variables.
This function is one of the functions that must exist for the framework to
work. Please notice that a pointer to this function has to exist in the
sc_profile struct thatwe have in the bottom of this file.
*/
int c_udp_create(struct sc_context *context, const struct iphdr *ip)
{
	struct sc_udp_context *udp_profile;
	struct udphdr *udp;
	struct iphdr *ip2;
	context->profile_context = kmalloc(sizeof(struct sc_udp_context),GFP_ATOMIC);

	if (context->profile_context == 0) {
	  rohc_debugf(0,"c_udp_create(): no mem for profile context\n");
	  return 0;
	}
	udp_profile = (struct sc_udp_context *)context->profile_context;
	udp_profile->sn = 0;

	udp_profile -> sn_window = c_create_wlsb(16,C_WINDOW_WIDTH,-1);



	udp_profile->ir_count = 0;
	udp_profile->fo_count = 0;
	udp_profile->so_count = 0;

	udp_profile->udp_checksum_change_count = 0;
	if (ip->protocol == 17){
		udp = (struct udphdr *)(ip+1);
	}else if(ip->protocol == 4){
		ip2=(struct iphdr *)ip+1;
		if(ip2->protocol == 17){
			udp = (struct udphdr *)(ip+2);
		}
		else{
			rohc_debugf(0,"c_udp_create(): no udp-header present can't use this profile\n");
			return 0;
		}
	}
	else{
		rohc_debugf(0,"c_udp_create(): no udp-header present can't use this profile\n");
		return 0;
	}
	udp_profile->old_udp = *udp;

	udp_profile->go_back_fo_count = 0;
	udp_profile->go_back_ir_count = 0;
	udp_profile->ir_dyn_count = 0;
	c_udp_init_context(&udp_profile->ip_flags,ip,context->rnd,context->nbo);
	c_udp_init_tmp_variables(&udp_profile->tmp_variables);
	udp_profile->is_initialized = 0;
	return 1;
}

/*
Function to deallocate a context.

This function is one of the functions that must exist for the framework to
work. Please notice that a pointer to this function has to exist in the
sc_profile struct thatwe have in the bottom of this file.
*/
void c_udp_destroy(struct sc_context *context){
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;
	c_destroy_wlsb(udp_profile->ip_flags.ip_id_window);
	c_destroy_wlsb(udp_profile->sn_window);

	if (context->profile_context != 0)
	  kfree(context->profile_context);

}

/*
The function are jused by the framework to check if a package belongs to this context
We check that its not framgmented. It is the source and destination addresses in the
ip-headers that decides if the package belong to this context, for udp the source and
destination ports also have to be the same for the package to be in the same context
as a cid.

This function is one of the functions that must exist for the framework to
work. Please notice that a pointer to this function has to exist in the
sc_profile struct that we have in the bottom of this file.
*/
int c_udp_check_context(struct sc_context *context, const struct iphdr *ip)
{
	boolean is_ip_same;
	boolean is_ip2_same;
	boolean is_udp_same;
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;

	struct udphdr *udp;
	struct iphdr *ip2;
	int reserved, dont_fragment, more_fragments, fragment_offset;
	unsigned short fragment;

	fragment = ntohs(ip->frag_off);

	reserved = (fragment >> 15);
	dont_fragment = (fragment >> 14) & 1;
	more_fragments = (fragment >> 13) & 1;
	fragment_offset = fragment & ((1<<14)-1);
	if(reserved != 0 || more_fragments != 0 || fragment_offset != 0) {
		rohc_debugf(0,"Fragment error (%d, %d, %d, %d)\n", reserved, dont_fragment, more_fragments, fragment_offset);
		return -1;
	}
	is_ip_same = (udp_profile->ip_flags.old_ip.saddr == ip->saddr) && (udp_profile->ip_flags.old_ip.daddr == ip->daddr);

	// 1
	// udp 17
	if (ip->protocol == 17){
		udp = (struct udphdr *)(ip+1);
		is_udp_same = ((udp_profile->old_udp.source == udp->source) &&
				(udp_profile->old_udp.dest == udp->dest));
		return (is_ip_same && is_udp_same);

	}else if(ip->protocol == 4){
		ip2=(struct iphdr *)ip+1;
		if(ip2->protocol == 17){
			udp = (struct udphdr *)(ip+2);
			is_udp_same = ((udp_profile->old_udp.source == udp->source) &&
					(udp_profile->old_udp.dest == udp->dest));
			is_ip2_same = (udp_profile->ip2_flags.old_ip.saddr == ip2->saddr) &&
					(udp_profile->ip2_flags.old_ip.daddr == ip2->daddr);
			return (is_ip_same && is_ip2_same && is_udp_same);

		}
		else{
			return 0;
		}

	}else{
		return 0;
	}

}

// Change the mode of this context
void udp_change_mode(struct sc_context *c, C_MODE new_mode) {
	if(c->c_mode != new_mode){
		c->c_mode = new_mode;
		udp_change_state(c, IR);
	}
}

// Change the state of this context
void udp_change_state(struct sc_context *c, C_STATE new_state)
{
	struct sc_udp_context *ip = (struct sc_udp_context *)c->profile_context;

	// Reset counters only if different state
	if (c->c_state != new_state) {
		ip->ir_count = 0;
		ip->fo_count = 0;
		ip->so_count = 0;
	}

	c->c_state = new_state;
}

/*
Encode packages to a pattern decided by several different factors.
1. Checks if we have double ip-headers
2. Check if the ip identification field are random and if it has network byte order
3. Decide what state we have, ir,fo or so
4. Decide how many bits needed to send ip-id and sn fields and most imortant updating
   the sliding windows.
5. Decide what packet to send.
6. Code the packet.



This function is one of the functions that must exist for the framework to
work. Please notice that a pointer to this function has to exist in the
sc_profile struct that we have in the bottom of this file.
*/
int c_udp_encode(struct sc_context *context,
	const struct iphdr *ip,
	int packet_size,
     	unsigned char *dest,
	int dest_size,
	int *payload_offset)

{

	int size;
	int reserved, dont_fragment, more_fragments, fragment_offset;
	unsigned short fragment;

	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;

	struct iphdr *ip2=(struct iphdr *)ip+1;
	struct udphdr *udp;
	udp_profile->tmp_variables.changed_fields2 = 0;
	udp_profile->tmp_variables.nr_ip_id_bits2 = 0;
	udp_profile->tmp_variables.packet_type = c_IR;
	udp_profile->tmp_variables.max_size = dest_size;
	// 1
	// udp 17
	if (ip->protocol == 17){
		ip2 = NULL;
		udp_profile->tmp_variables.nr_of_ip_hdr = 1;
		//h� kanske vi ska fr�a andreas
		udp = (struct udphdr *)(ip+1);

		*payload_offset = 28;
	}else if(ip->protocol == 4){
		if(!udp_profile->is_initialized){
			c_udp_init_context(&udp_profile->ip2_flags,ip2,context->rnd2,context->nbo2);
			udp_profile->is_initialized = 1;
		}
		if(ip2->protocol == 17){
			udp = (struct udphdr *)(ip+2);
			*payload_offset = 48;
			udp_profile->tmp_variables.nr_of_ip_hdr = 2;
		}
		else{
			return -1;
		}
	}else{
		return -1;
	}

	if (udp_profile == 0) {
	  rohc_debugf(0,"c_udp_encode(): udp_profile==null\n");
	  return 0;
	}

	fragment = ntohs(ip->frag_off);

	reserved = (fragment >> 15);
	dont_fragment = (fragment >> 14) & 1;
	more_fragments = (fragment >> 13) & 1;
	fragment_offset = fragment & ((1<<14)-1);
	if(reserved != 0 || more_fragments != 0 || fragment_offset != 0) {
		rohc_debugf(0,"Fragment error (%d, %d, %d, %d)\n", reserved, dont_fragment, more_fragments, fragment_offset);
		return -1;
	}

	// 2. Check NBO and RND of IP-ID
	if (udp_profile->sn != 0){ // skip first packet
		udp_check_ip_identification(&udp_profile->ip_flags,ip,&context->rnd, &context->nbo);
		if(udp_profile->tmp_variables.nr_of_ip_hdr > 1){
			udp_check_ip_identification(&udp_profile->ip2_flags,ip2,&context->rnd2, &context->nbo2);
		}
	}
	// Update the sequence number every time we encode something.
	udp_profile->sn = udp_profile->sn + 1;
	udp_profile->tmp_variables.changed_fields = udp_changed_fields(&udp_profile->ip_flags,ip);
	if(udp_profile->tmp_variables.nr_of_ip_hdr > 1){
		udp_profile->tmp_variables.changed_fields2 = udp_changed_fields(&udp_profile->ip2_flags,ip2);
	}

	udp_profile->tmp_variables.send_static = udp_changed_static_both_hdr(context,ip,ip2);
	udp_profile->tmp_variables.send_dynamic = udp_changed_dynamic_both_hdr(context,ip,ip2);
	udp_profile->tmp_variables.send_udp_dynamic = udp_changed_udp_dynamic(context,udp);

	rohc_debugf(2,"send_static = %d, send_dynamic = %d\n", udp_profile->tmp_variables.send_static,
				udp_profile->tmp_variables.send_dynamic);
	// 3
	udp_decide_state(context);
	rohc_debugf(2,"ip->id=%d context->sn=%d\n", ntohs(ip->id), udp_profile->sn);

	// 4
	c_udp_update_variables(context,ip,ip2);

	// 5
	udp_profile->tmp_variables.packet_type = udp_decide_packet(context);

	// 6
	size = udp_code_packet(udp_profile,ip,ip2,udp,dest,context);
	if(size < 0)
		return -1;
	if (udp_profile->tmp_variables.packet_type == c_UO2) {
		int extension = udp_decide_extension(context);

		rohc_debugf(1, "ROHC-UDP %s (%s): %d -> %d\n", udp_packet_types[udp_profile->tmp_variables.packet_type], udp_extension_types[extension],*payload_offset, size);
	} else {
		rohc_debugf(1, "ROHC-UDP %s: from %d -> %d\n", udp_packet_types[udp_profile->tmp_variables.packet_type], *payload_offset, size);
	}

	if(udp_profile->tmp_variables.packet_type == c_IR || udp_profile->tmp_variables.packet_type == c_IRDYN){
		udp_profile->old_udp = *udp;
	}

	udp_profile->ip_flags.old_ip = *ip;

	udp_profile->ip_flags.old_rnd = context->rnd;
	udp_profile->ip_flags.old_nbo = context->nbo;

	if(udp_profile->tmp_variables.nr_of_ip_hdr > 1){
		udp_profile->ip2_flags.old_ip = *ip2;
		udp_profile->ip2_flags.old_rnd = context->rnd2;
		udp_profile->ip2_flags.old_nbo = context->nbo2;
	}

	if (udp_profile->tmp_variables.packet_type == c_IR)
		context->num_send_ir ++;
	else if (udp_profile->tmp_variables.packet_type == c_IRDYN)
		context->num_send_ir_dyn ++;


	return size;
}


/*
Function that update the profile when feedback has arrived.

This function is one of the functions that must exist for the framework to
work. Please notice that a pointer to this function has to exist in the
sc_profile struct that we have in the bottom of this file.
*/
void c_udp_feedback(struct sc_context *context, struct sc_feedback *feedback)
{

	struct sc_udp_context *ip_context = (struct sc_udp_context *)context->profile_context;
	unsigned char *p = feedback->data + feedback->specific_offset;


	if (feedback->type == 1) { // ack
		unsigned int sn = p[0];

		c_ack_sn_wlsb(ip_context->ip_flags.ip_id_window, sn);
		c_ack_sn_wlsb(ip_context->sn_window, sn);

	} else if (feedback->type == 2) { // FEEDBACK-2
		unsigned int crc = 0, crc_used=0;

		int sn_not_valid = 0;
		unsigned char mode = (p[0]>>4) & 3;
		unsigned int sn = ((p[0] & 15) << 8) + p[1];
		int remaining = feedback->specific_size-2;

		p+=2;

		while (remaining > 0) {
			int opt = p[0]>>4;
			int optlen = p[0] & 0x0f;

			switch (opt) {
			case 1: // CRC
				crc = p[1];
				crc_used = 1;
				p[1] = 0; // set to zero for crc computation..
			break;
			//case 2: // Reject
			//break;
			case 3: // SN-Not-Valid
				sn_not_valid=1;
			break;
			case 4: // SN  -- TODO: how is several SN options combined?
				sn = (sn<<8) + p[1];
			break;
			//case 7: // Loss
			//break;
			default:
				rohc_debugf(0,"c_udp_feedback(): Unknown feedback type: %d\n", opt);
			break;
			}

			remaining -= 1 + optlen;
			p += 1 + optlen;
		}

		if (crc_used) { // check crc..
			if (crc_calculate(CRC_TYPE_8, feedback->data, feedback->size ) != crc) {
				rohc_debugf(0,"c_udp_feedback(): crc check failed..(size=%d)\n", feedback->size);
				return;
			}
		}

		if (mode != 0) {
			if (crc_used) {
				udp_change_mode(context, mode);
				rohc_debugf(1,"c_udp_feedback(): changing mode to %d\n", mode);
			} else {
				rohc_debugf(0,"c_udp_feedback(): mode change requested but no crc was given..\n");
			}
		}

		switch (feedback->acktype) {
		case ACK:
			if (sn_not_valid == 0) {
				c_ack_sn_wlsb(ip_context->ip_flags.ip_id_window, sn);
				c_ack_sn_wlsb(ip_context->sn_window, sn);
			}
		break;

		case NACK:
			if (context->c_state == SO){
				change_state(context, FO);
				ip_context->ir_dyn_count = 0;
			}
			if(context->c_state == FO){
				ip_context->ir_dyn_count = 0;
			}
		break;

		case STATIC_NACK:
			udp_change_state(context, IR);
		break;

		case RESERVED:
			rohc_debugf(0, "c_udp_feedback(): reserved field used\n");
		break;
		}

	} else {
		rohc_debugf(0,"c_udp_feedback(): Feedback type not implemented (%d)\n", feedback->type);
	}

}

/*
Decide the state that that should be used for this packet
The three states are:
IR (initialization and refresh).
FO (first order).
SO (second order).
*/
void udp_decide_state(struct sc_context *context)
{
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;
	if(udp_profile->tmp_variables.send_udp_dynamic){
		udp_change_state(context,IR);
	}else if(context->c_state == IR && udp_profile->tmp_variables.send_dynamic
		&& udp_profile->ir_count >= MAX_IR_COUNT){
		udp_change_state(context, FO);
	}else if(context->c_state == IR && udp_profile->tmp_variables.send_static
		&& udp_profile->ir_count >= MAX_IR_COUNT){
		udp_change_state(context, FO);
	}else if (context->c_state == IR && udp_profile->ir_count >= MAX_IR_COUNT) {
		udp_change_state(context, SO);
	}else if (context->c_state == FO && udp_profile->tmp_variables.send_dynamic
		&& udp_profile->fo_count >= MAX_FO_COUNT) {
		udp_change_state(context, FO);
	}else if (context->c_state == FO && udp_profile->tmp_variables.send_static
		&& udp_profile->fo_count >= MAX_FO_COUNT) {
		udp_change_state(context, FO);
	}else if (context->c_state == FO && udp_profile->fo_count >= MAX_FO_COUNT) {
		udp_change_state(context, SO);
	}else if(context->c_state == SO && udp_profile->tmp_variables.send_dynamic){
		udp_change_state(context, FO);
	}else if(context->c_state == SO && udp_profile->tmp_variables.send_static){
		udp_change_state(context, FO);
	}
	if(context->c_mode == U){
		udp_periodic_down_transition(context);
	}

}

/*
A function just to update some variables, only used in encode. Everything in this
function could be in encode but to make it more readable we have it here instead.
*/
void c_udp_update_variables(struct sc_context *context,
	const struct iphdr *ip,
	const struct iphdr *ip2)
{
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;

	if (context->nbo)
		udp_profile->ip_flags.id_delta = (ntohs(ip->id) - udp_profile->sn);
	else
		udp_profile->ip_flags.id_delta = ip->id - udp_profile->sn;

	if(udp_profile->tmp_variables.nr_of_ip_hdr > 1){
		if (context->nbo2)
			udp_profile->ip2_flags.id_delta = (ntohs(ip2->id) - udp_profile->sn);
		else
			udp_profile->ip2_flags.id_delta = ip2->id - udp_profile->sn;
	}


	rohc_debugf(2,"Get k\n");
	rohc_debugf(2,"id_delta: %d\n",udp_profile->ip_flags.id_delta);
	rohc_debugf(2,"Given that sn: %d\n",udp_profile->sn);
	udp_profile->tmp_variables.nr_ip_id_bits = c_get_k_wlsb(udp_profile->ip_flags.ip_id_window, udp_profile->ip_flags.id_delta);
	udp_profile->tmp_variables.nr_sn_bits = c_get_k_wlsb(udp_profile->sn_window, udp_profile->sn);

	rohc_debugf(2,"Adding\n");
	c_add_wlsb(udp_profile->ip_flags.ip_id_window, udp_profile->sn, 0, udp_profile->ip_flags.id_delta); // TODO: replace 0 with time(0)
	c_add_wlsb(udp_profile->sn_window, udp_profile->sn, 0, udp_profile->sn); // TODO: replace 0 with time(0)

	rohc_debugf(2,"ip_id bits=%d\n", udp_profile->tmp_variables.nr_ip_id_bits);
	rohc_debugf(2,"sn bits=%d\n", udp_profile->tmp_variables.nr_sn_bits);
	if(udp_profile->tmp_variables.nr_of_ip_hdr > 1){
		udp_profile->tmp_variables.nr_ip_id_bits2 = c_get_k_wlsb(udp_profile->ip2_flags.ip_id_window, udp_profile->ip2_flags.id_delta);
		rohc_debugf(2,"ip_id bits2=%d\n", udp_profile->tmp_variables.nr_ip_id_bits);
		c_add_wlsb(udp_profile->ip2_flags.ip_id_window, udp_profile->sn, 0, udp_profile->ip2_flags.id_delta); // TODO: replace 0 with time(0)
	}
}

/*Function that change state periodicly after a certain number of packets */
void udp_periodic_down_transition(struct sc_context *context)
{
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;
	if(udp_profile->go_back_fo_count == CHANGE_TO_FO_COUNT){
		udp_profile->go_back_fo_count = 0;
		udp_profile->ir_dyn_count = 0;
		udp_change_state(context, FO);

	} else if(udp_profile->go_back_ir_count == CHANGE_TO_IR_COUNT){
		udp_profile->go_back_ir_count = 0;
		udp_change_state(context, IR);
	}
	if (context->c_state == SO)
		udp_profile->go_back_fo_count++;
	if(context->c_state == SO || context->c_state == FO)
		udp_profile->go_back_ir_count++;
}

/*
Decide what packet to send in the different states,
In IR state, ir packets are used. In FO and SO the functions udp_decide_FO_packet and udp_decide_SO_packet is used.
*/
int udp_decide_packet(struct sc_context *context)
{
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;
	int packet_type=IR;
	switch(context->c_state)
	{
		case IR:
			rohc_debugf(2,"c_udp_encode(): IR state\n");
			udp_profile->ir_count++;
			packet_type = c_IR;
			break;
		case FO:
			rohc_debugf(2,"c_udp_encode(): FO state\n");
			udp_profile->fo_count++;
			packet_type = udp_decide_FO_packet(context);
			break;

		case SO:
			rohc_debugf(2,"c_udp_encode(): SO state\n");
			udp_profile->so_count++;
			packet_type = udp_decide_SO_packet(context);
			break;
	}
	return packet_type;
}

/*
Decide which packet to send in first order state
The packets that can be used is IRDYN and UO2 packets.
*/
int udp_decide_FO_packet(struct sc_context *context)
{
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;
	int nr_of_ip_hdr = udp_profile->tmp_variables.nr_of_ip_hdr;
	if(udp_profile->ir_dyn_count < MAX_FO_COUNT){
		udp_profile->ir_dyn_count++;
		return c_IRDYN;
	}else if (udp_profile->tmp_variables.send_static) { // if static field changed we must go back to ir
		return c_UO2;
	}else if((nr_of_ip_hdr == 1 && udp_profile->tmp_variables.send_dynamic > 2)){
		return c_IRDYN;
	}else if((nr_of_ip_hdr > 1 && udp_profile->tmp_variables.send_dynamic > 4)){
		return c_IRDYN;
	}
	else{
		return c_UO2;
	}
}

/*
Decide which packet to send in second order state
Packet that can be used are UO0, UO1, UO2 with or without extensions.
*/
int udp_decide_SO_packet(const struct sc_context *context)
{
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;
	int nr_ip_id_bits = udp_profile->tmp_variables.nr_ip_id_bits;
	int nr_ip_id_bits2 = udp_profile->tmp_variables.nr_ip_id_bits2;
	int nr_sn_bits = udp_profile->tmp_variables.nr_sn_bits;
	
	if(udp_profile->tmp_variables.nr_of_ip_hdr == 1){
		if(context->rnd == 1 && nr_sn_bits <= 4){
			return c_UO0;
		}
		else if(nr_sn_bits <= 4 && nr_ip_id_bits == 0){
			return c_UO0;
		}
		else if(nr_sn_bits == 5 && nr_ip_id_bits == 0){
			return c_UO2;
		}
		else if(nr_sn_bits <= 5 && nr_ip_id_bits <= 6){
			return c_UO1;
		}
		else{
			return c_UO2;
		}
	}
	else{
		if(context->rnd == 1 && context->rnd2 == 1 && nr_sn_bits <= 4){
			return c_UO0;
		}
		else if(nr_sn_bits <= 4 && (context->rnd == 1 || nr_ip_id_bits == 0)
		&& (context->rnd2 == 1 || nr_ip_id_bits2 ==0)){

			return c_UO0;
		}
		else if(nr_sn_bits <= 5 && nr_ip_id_bits <= 6 && (context->rnd2 == 1 || nr_ip_id_bits2 == 0)){
			return c_UO1;
		}
		else{
			return c_UO2;
		}
	}
}

/*Code the given packet type*/

/*
Code the given packet type
Every given type has its own help function
The ir and ir-dyn packet are totally coded by their own functions. The general format
for other packets are coded in this function according to the following:

     0   1   2   3   4   5   6   7
    --- --- --- --- --- --- --- ---
1  :         Add-CID octet         :                    |
   +---+---+---+---+---+---+---+---+                    |
2  |   first octet of base header  |                    |
   +---+---+---+---+---+---+---+---+                    |
   :                               :                    |
3  /   0, 1, or 2 octets of CID    /                    |
   :                               :                    |
   +---+---+---+---+---+---+---+---+                    |
4  /   remainder of base header    /                    |
   +---+---+---+---+---+---+---+---+                    |
   :                               :                    |
5  /           Extension           /                    |
   :                               :                    |
    --- --- --- --- --- --- --- ---                     |
   :                               :                    |
6  +   IP-ID of outer IPv4 header  +
   :                               :     (see section 5.7 or [RFC-3095])
    --- --- --- --- --- --- --- ---
7  /    AH data for outer list     /                    |
    --- --- --- --- --- --- --- ---                     |
   :                               :                    |
8  +         GRE checksum          +                    |
   :                               :                    |
    --- --- --- --- --- --- --- ---                     |
   :                               :                    |
9  +   IP-ID of inner IPv4 header  +                    |
   :                               :                    |
    --- --- --- --- --- --- --- ---                     |
10 /    AH data for inner list     /                    |
    --- --- --- --- --- --- --- ---                     |
   :                               :                    |
11 +         GRE checksum          +                    |
   :                               :                    |
    --- --- --- --- --- --- --- ---
   :            List of            :
12 /        Dynamic chains         /    variable, given by static chain
   :   for additional IP headers   :           (includes no SN)
    --- --- --- --- --- --- --- ---
    :                               :
13 +         UDP Checksum          +  2 octets,
   :                               :  if context(UDP Checksum) != 0
    --- --- --- --- --- --- --- ---
7, 8, 10, 11, 12 is not supported.
Each profile code 1, 2, 3, 4, 5 is coded in each packets function
6 and 9 is coded in this function
*/
int udp_code_packet(struct sc_udp_context *udp_profile,
	const struct iphdr *ip,
	const struct iphdr *ip2,
	const struct udphdr *udp,
	unsigned char *dest,
	struct sc_context *context)
{
	int counter = 0;

	//struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;

	switch(udp_profile->tmp_variables.packet_type){
		case c_IR:
			rohc_debugf(2,"udp_code_packet(): IR packet..\n");
			return udp_code_IR_packet(udp_profile,ip,ip2,udp,dest,context);
			break;
		case c_IRDYN:
			rohc_debugf(2,"udp_code_packet(): IRDYN packet..\n");
			return udp_code_IR_DYN_packet(udp_profile,ip,ip2,udp,dest, context);
			break;
		case c_UO0:
			rohc_debugf(2,"udp_code_packet(): OU-0 packet..\n");
			counter = udp_code_UO0_packet(context, ip, dest);
			break;
		case c_UO1:
			rohc_debugf(2,"udp_code_packet(): OU-1 packet..\n");
			counter = udp_code_UO1_packet(context, ip, dest);
			break;

		case c_UO2:
			rohc_debugf(2,"udp_code_packet(): OU-2 packet..\n");
			counter = udp_code_UO2_packet(context,udp_profile,ip,ip2,dest);
			break;
		default:
			rohc_debugf(0,"udp_code_packet(): Unknown packet..\n");
			return -1;
	}
	rohc_debugf(2,"RND=%d RND2=%d\n",context->rnd, context->rnd2);

	// 9
	if(context->rnd == 1){
		memcpy(&dest[counter], &ip->id, 2);
		counter += 2;
	}

	// 6
	if(udp_profile->tmp_variables.nr_of_ip_hdr > 1){
		if(context->rnd2 == 1){
			memcpy(&dest[counter], &ip2->id, 2);
			counter += 2;
		}
	}
	// 13
	if(udp->check != 0){
		memcpy(&dest[counter], &udp->check, 2);
		counter += 2;
	}

	return counter;

}
/*
Code the IR-packet to this pattern:

IR packet (5.7.7.1):

     0   1   2   3   4   5   6   7
    --- --- --- --- --- --- --- ---
1  |         Add-CID octet         |  if for small CIDs and CID != 0
   +---+---+---+---+---+---+---+---+
2  | 1   1   1   1   1   1   0 | D |
   +---+---+---+---+---+---+---+---+
   |                               |
3  /    0-2 octets of CID info     /  1-2 octets if for large CIDs
   |                               |
   +---+---+---+---+---+---+---+---+
4  |            Profile            |  1 octet
   +---+---+---+---+---+---+---+---+
5  |              CRC              |  1 octet
   +---+---+---+---+---+---+---+---+
   |                               |
6  |         Static chain          |  variable length
   |                               |
   +---+---+---+---+---+---+---+---+
   |                               |
7  |         Dynamic chain         |  present if D = 1, variable length
   |                               |
   +---+---+---+---+---+---+---+---+
   |                               |
   |           Payload             |  variable length
   |                               |
    - - - - - - - - - - - - - - - -

The numbers before every field is used to find where in the code we try
to code that field.
*/
int udp_code_IR_packet(struct sc_udp_context *udp_profile,
	const struct iphdr *ip,
	const struct iphdr *ip2,
	const struct udphdr *udp,
	unsigned char *dest,
	struct sc_context *context)

{
	unsigned char type;
	int crc_position;
	int counter = 0;
	int first_position;

	rohc_debugf(2,"Coding IR packet (cid=%d)\n", context->cid);
	//Both 1 and 3, 2 will be placed in dest[first_position]
	counter = code_cid_values(context,dest,udp_profile->tmp_variables.max_size,&first_position);

	// 2
	type = 0xfc;

	// Set the type and set the d flag if the dynamic part
	// will be present in the ir packet
	//if(udp_changed_dynamic_both_hdr(changed_f,context)){
		type += 1;
	//}

	dest[first_position] = type;
	// if large cid
	// Set profile_id and crc to zero so far
	// 3
	dest[counter] = context -> profile_id;
	counter++;

	// 5
	// Becuase we calculate the crc over the ir packet with the crc field set to zero
	// The real crc is calculated in the end of this function
	crc_position = counter;
	dest[counter] = 0;
	counter++;

	// 6
	counter = udp_code_static_part(context,&udp_profile->ip_flags,ip,dest,counter);

	if(udp_profile->tmp_variables.nr_of_ip_hdr > 1){
		counter = udp_code_static_part(context,&udp_profile->ip2_flags,ip2,dest,counter);
	}


	counter = udp_code_static_udp_part(context,udp,dest,counter);

	// Add the dynamic part
	// Set type of service, time to live id, and some small flags
	//if(udp_changed_dynamic_both_hdr(changed_f,context) > 0){
	//rohc_debugf(1,"Crc length=%d\n", counter);


	// 7

	//Observe that if we don't wan't dynamic part in ir-packet we should not send the following
	//******************************************************************************************************
	counter = udp_code_dynamic_part(context,&udp_profile->ip_flags,ip,dest,counter,&context->rnd,&context->nbo);
	if(udp_profile->tmp_variables.nr_of_ip_hdr > 1){
		counter = udp_code_dynamic_part(context,&udp_profile->ip2_flags,ip2,dest,counter,&context->rnd2,&context->nbo2);
	}
	if(counter < 0)
		return -1;

	counter = udp_code_dynamic_udp_part(context,udp,dest,counter);
	//******************************************************************************************************


	//}

	// 5 Count the crc and set the crc field to the checksum.
	rohc_debugf(2,"Crc length=%d\n", counter);
	dest[crc_position]= crc_calculate(CRC_TYPE_8,dest,counter);
	return counter;

}
/*
Code the IRDYN-packet to this pattern:

IR-dyn packet (5.7.7.2):

     0   1   2   3   4   5   6   7
    --- --- --- --- --- --- --- ---
1  :         Add-CID octet         : if for small CIDs and CID != 0
   +---+---+---+---+---+---+---+---+
2  | 1   1   1   1   1   0   0   0 | IR-DYN packet type
   +---+---+---+---+---+---+---+---+
   :                               :
3  /     0-2 octets of CID info    / 1-2 octets if for large CIDs
   :                               :
   +---+---+---+---+---+---+---+---+
4  |            Profile            | 1 octet
   +---+---+---+---+---+---+---+---+
5  |              CRC              | 1 octet
   +---+---+---+---+---+---+---+---+
   |                               |
6  /         Dynamic chain         / variable length
   |                               |
   +---+---+---+---+---+---+---+---+
   :                               :
   /           Payload             / variable length
   :                               :
    - - - - - - - - - - - - - - - -


The numbers before every field is used to find where in the code we try
to code that field.

*/
int udp_code_IR_DYN_packet(struct sc_udp_context *udp_profile,
	const struct iphdr *ip,
	const struct iphdr *ip2,
	const struct udphdr *udp,
	unsigned char *dest,
	struct sc_context *context)


{

	int crc_position;
	int counter = 0;
	int first_position;

	//Both 1 and 3, 2 will be placed in dest[first_position]
	counter = code_cid_values(context,dest,udp_profile->tmp_variables.max_size,&first_position);

	// 2
	dest[first_position] = 0xf8;

	// 4
	dest[counter] = context -> profile_id;
	counter++;

	// 5
	// Becuase we calculate the crc over the ir packet with the crc field set to zero
	// The real crc is calculated in the end of this function
	crc_position = counter;
	dest[counter] = 0;
	counter++;

	// 6
	counter = udp_code_dynamic_part(context,&udp_profile->ip_flags,ip,dest,counter,&context->rnd,&context->nbo);
	if(udp_profile->tmp_variables.nr_of_ip_hdr > 1){
		counter = udp_code_dynamic_part(context,&udp_profile->ip2_flags,ip2,dest,counter,&context->rnd2,&context->nbo2);
	}
	if(counter < 0)
		return -1;
	counter = udp_code_dynamic_udp_part(context,udp,dest,counter);



	// 5
	// Count the crc and set the crc field to the checksum.
	dest[crc_position]= crc_calculate(CRC_TYPE_8,dest,counter);
	return counter;
}

/*
Code the static part:
Static part IPv4 (5.7.7.4):

   +---+---+---+---+---+---+---+---+
1  |  Version = 4  |       0       |
   +---+---+---+---+---+---+---+---+
2  |           Protocol            |
   +---+---+---+---+---+---+---+---+
3  /        Source Address         /   4 octets
   +---+---+---+---+---+---+---+---+
4  /      Destination Address      /   4 octets
   +---+---+---+---+---+---+---+---+
*/
int udp_code_static_part(struct sc_context *context,
	struct sc_udp_header *udp_hdr_profile,
	const struct iphdr *ip,
	unsigned char *dest,
 	int counter)
{

	// 1
	dest[counter] = 0x40;
	counter++;

	// 2
	dest[counter] = ip -> protocol; rohc_debugf(3,"code_IR, protocol=%d\n", dest[counter]);
	counter++;
	udp_hdr_profile -> protocol_count++;

	// 3
	memcpy(&dest[counter], &ip -> saddr, 4);
	counter += 4;

	// 4
	memcpy(&dest[counter], &ip -> daddr, 4);
	counter += 4;
	return counter;
}
/*
Static part of UDP header (5.7.7.5):

   +---+---+---+---+---+---+---+---+
1  /          Source Port          /   2 octets
   +---+---+---+---+---+---+---+---+
2  /       Destination Port        /   2 octets
   +---+---+---+---+---+---+---+---+
*/
int udp_code_static_udp_part(struct sc_context *context,
	const struct udphdr *udp,
	unsigned char *dest,
	int counter)
{
	//1
	memcpy(&dest[counter], &udp->source, 2);
	counter += 2;
	//2
	memcpy(&dest[counter], &udp->dest, 2);
	counter += 2;

	return counter;
}

/*
Code the dynamic part:
Dynamic part IPv4 (5.7.7.4):

   +---+---+---+---+---+---+---+---+
1  |        Type of Service        |
  +---+---+---+---+---+---+---+---+
2  |         Time to Live          |
   +---+---+---+---+---+---+---+---+
3  /        Identification         /   2 octets
   +---+---+---+---+---+---+---+---+
4  | DF|RND|NBO|         0         |
   +---+---+---+---+---+---+---+---+
5  / Generic extension header list /  variable length
   +---+---+---+---+---+---+---+---+
*/
int udp_code_dynamic_part(struct sc_context *context,
	struct sc_udp_header *udp_hdr_profile,
	const struct iphdr *ip,
	unsigned char *dest,
	int counter,
	int *rnd,
	int *nbo)
{

	unsigned char df_rnd_nbo = 0;
	int reserved, dont_fragment, more_fragments, fragment_offset;
	unsigned short fragment;

	// 1
	dest[counter] = ip -> tos;
	counter++;
	udp_hdr_profile->tos_count++;

	// 2
	dest[counter] = ip -> ttl;
	counter++;
	udp_hdr_profile->ttl_count++;

	// 3
	memcpy(&dest[counter], &ip->id, 2);
	counter += 2;


	// 4

	fragment = ntohs(ip->frag_off);

	reserved = (fragment >> 15);
	dont_fragment = (fragment >> 14) & 1;
	more_fragments = (fragment >> 13) & 1;
	fragment_offset = fragment & ((1<<14)-1);
	if(reserved != 0 || more_fragments != 0 || fragment_offset != 0) {
		rohc_debugf(0,"Fragment error (%d, %d, %d, %d)\n", reserved, dont_fragment, more_fragments, fragment_offset);
		return -1;
	}
	df_rnd_nbo = (dont_fragment << 7);
	udp_hdr_profile->df_count++;
	// Check random flags and network byte order flag in the context struct
	if(*rnd){
		df_rnd_nbo |= 0x40;
	}
	if(*nbo){
		df_rnd_nbo |= 0x20;
	}
	udp_hdr_profile->rnd_count++;
	udp_hdr_profile->nbo_count++;
	dest[counter] = df_rnd_nbo;
	counter++;

	// 5 is not supported

	return counter;
}
/*
Dynamic part of UDP header (5.7.7.5):

   +---+---+---+---+---+---+---+---+
1  /           Checksum            /   2 octets
   +---+---+---+---+---+---+---+---+
2  |             SN                |   2 octets
   +---+---+---+---+---+---+---+---+

*/
int udp_code_dynamic_udp_part(struct sc_context *context,
	const struct udphdr *udp,
	unsigned char *dest,
	int counter)
{
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;
	// 1
	memcpy(&dest[counter], &udp->check, 2);
	counter += 2;
	udp_profile->udp_checksum_change_count++;
	// 2
	dest[counter] = (udp_profile->sn) >> 8;
	counter++;
	dest[counter] = (udp_profile->sn) & 0xff;
	counter++;
	return counter;
}
/*
Code UO0-packets acording to the following pattern:

     0   1   2   3   4   5   6   7
    --- --- --- --- --- --- --- ---
1  :         Add-CID octet         :
   +---+---+---+---+---+---+---+---+
2  |   first octet of base header  |
   +---+---+---+---+---+---+---+---+
   :                               :
3  /   0, 1, or 2 octets of CID    /
   :                               :
   +---+---+---+---+---+---+---+---+

   UO-0 (5.7.1)

     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
2  | 0 |      SN       |    CRC    |
   +===+===+===+===+===+===+===+===+
*/

int udp_code_UO0_packet(struct sc_context *context,
	const struct iphdr *ip,
	unsigned char *dest)
{
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;
	unsigned char f_byte;
	int counter = 0;
	int first_position;
	int ch_sum;
	//Both 1 and 3, 2 will be placed in dest[first_position]
	counter = code_cid_values(context,dest,udp_profile->tmp_variables.max_size,&first_position);

	// 2
	f_byte = (udp_profile -> sn) & 0xf;
	f_byte = f_byte << 3;
	if(udp_profile->tmp_variables.nr_of_ip_hdr == 1)
		ch_sum= crc_calculate(CRC_TYPE_3,(unsigned char *)ip,sizeof(struct iphdr)+sizeof(struct udphdr));
	else
		ch_sum= crc_calculate(CRC_TYPE_3,(unsigned char *)ip,2*sizeof(struct iphdr)+sizeof(struct udphdr));
	f_byte = f_byte + ch_sum;
	dest[first_position] = f_byte;
	return counter;
}

/*
Code UO1-packets acording to the following pattern:

     0   1   2   3   4   5   6   7
    --- --- --- --- --- --- --- ---
1  :         Add-CID octet         :
   +---+---+---+---+---+---+---+---+
2  |   first octet of base header  |
   +---+---+---+---+---+---+---+---+
   :                               :
3  /   0, 1, or 2 octets of CID    /
   :                               :
   +---+---+---+---+---+---+---+---+

   OU-1 (5.11.3)

     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
2  | 1   0 |         IP-ID         |
   +===+===+===+===+===+===+===+===+
4  |        SN         |    CRC    |
   +---+---+---+---+---+---+---+---+

*/
int udp_code_UO1_packet(struct sc_context *context,
	const struct iphdr *ip,
	unsigned char *dest)
{
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;
	unsigned char f_byte;
	unsigned char s_byte;
	int counter = 0;
	int first_position;
	int ch_sum;
	//Both 1 and 3, 2 will be placed in dest[first_position]
	counter = code_cid_values(context,dest,udp_profile->tmp_variables.max_size,&first_position);
	f_byte = (udp_profile -> ip_flags.id_delta) & 0x3f;
	f_byte |= 0x80;
	// Add the 5 bits of sn
	s_byte = (udp_profile -> sn) & 0x1f;
	s_byte = s_byte << 3;
	if(udp_profile->tmp_variables.nr_of_ip_hdr == 1)
		ch_sum= crc_calculate(CRC_TYPE_3,(unsigned char *)ip,sizeof(struct iphdr)+sizeof(struct udphdr));
	else
		ch_sum= crc_calculate(CRC_TYPE_3,(unsigned char *)ip,2*sizeof(struct iphdr)+sizeof(struct udphdr));
	s_byte |= ch_sum;
	dest[first_position] = f_byte;
	dest[counter] = s_byte;
	counter++;
	return counter;
}

/*
Code UO0-packets acording to the following pattern:

     0   1   2   3   4   5   6   7
    --- --- --- --- --- --- --- ---
1  :         Add-CID octet         :
   +---+---+---+---+---+---+---+---+
2  |   first octet of base header  |
   +---+---+---+---+---+---+---+---+
   :                               :
3  /   0, 1, or 2 octets of CID    /
   :                               :
   +---+---+---+---+---+---+---+---+

   OU-2 (5.11.3):

     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
2  | 1   1   0 |        SN         |
   +===+===+===+===+===+===+===+===+
4  | X |            CRC            |
   +---+---+---+---+---+---+---+---+

   +---+---+---+---+---+---+---+---+
   :                               :
5  /           Extension           /
   :                               :
    --- --- --- --- --- --- --- ---
*/
int udp_code_UO2_packet(struct sc_context *context,
	struct sc_udp_context *udp_profile,
	const struct iphdr *ip,
	const struct iphdr *ip2,
	unsigned char *dest)
{

	unsigned char f_byte;
	unsigned char s_byte;
	int counter = 0;
	int first_position;
	int ch_sum;
	int extension;
	int nr_sn_bits = udp_profile->tmp_variables.nr_sn_bits;
	//Both 1 and 3, 2 will be placed in dest[first_position]
	counter = code_cid_values(context,dest,udp_profile->tmp_variables.max_size,&first_position);

	// 2
	f_byte = 0xc0;
	// Add the 4 bits of sn

	// 4
	if(udp_profile->tmp_variables.nr_of_ip_hdr == 1)
		ch_sum= crc_calculate(CRC_TYPE_7,(unsigned char *)ip,sizeof(struct iphdr)+sizeof(struct udphdr));
	else
		ch_sum= crc_calculate(CRC_TYPE_7,(unsigned char *)ip,2*sizeof(struct iphdr)+sizeof(struct udphdr));
	s_byte = ch_sum;

	// 5
	extension = udp_decide_extension(context);
	if(extension == c_NOEXT){
		rohc_debugf(1,"code_OU2_packet(): no extension\n");
		// 2
		f_byte |= ((udp_profile -> sn) & 0x1f);
		dest[first_position] = f_byte;
		// 4
		dest[counter] = s_byte;
		counter++;
		return counter;
	}
	// 4
	//We have a extension
	s_byte |= 0x80;
	dest[counter] = s_byte;
	counter++;

	// 5
	if(extension == c_EXT0) {
		rohc_debugf(1,"code_OU2_packet(): using extension 0\n");
		// 2
		f_byte |= ((udp_profile -> sn & 0xff) >> 3);
		dest[first_position] = f_byte;
		// 5
		counter = udp_code_EXT0_packet(context,ip,dest,counter);
	} else if(extension == c_EXT1) {
		rohc_debugf(1,"code_OU2_packet(): using extension 1\n");
		// 2
		f_byte |= ((udp_profile -> sn & 0xff) >> 3);
		dest[first_position] = f_byte;
		// 5
		counter = udp_code_EXT1_packet(context,ip,dest,counter);
	} else if(extension == c_EXT2){
		rohc_debugf(1,"code_OU2_packet(): using extension 2\n");
		// 2
		f_byte |= ((udp_profile -> sn & 0xff) >> 3);
		dest[first_position] = f_byte;
		// 5
		counter = udp_code_EXT2_packet(context,udp_profile,ip,dest,counter);
	}
	else if(extension == c_EXT3) {
		rohc_debugf(1,"code_OU2_packet(): using extension 3\n");
		// 2
		// check if s-field needs to be used
		if(nr_sn_bits > 5)
			f_byte |= ((udp_profile -> sn) >> 8);
		else
			f_byte |= (udp_profile -> sn & 0x1f);

		dest[first_position] = f_byte;
		// 5
		counter = udp_code_EXT3_packet(context,udp_profile,ip,ip2,dest, counter);
	} else {
		rohc_debugf(0,"code_OU2_packet(): Unknown extension (%d)..\n", extension);
		//printf("
		//counter = -1;
	}
	return counter;
}


/*
Coding the extension 0 part of the uo0-packet

   Extension 0 (5.11.4):

      +---+---+---+---+---+---+---+---+
1     | 0   0 |    SN     |   IP-ID   |
      +---+---+---+---+---+---+---+---+

*/
int udp_code_EXT0_packet(struct sc_context *context,const struct iphdr *ip,
				unsigned char *dest, int counter)
{
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;
	unsigned char f_byte;

	// 1
	f_byte = (udp_profile -> sn) & 0x07;
	f_byte = f_byte << 3;
	f_byte |= (udp_profile ->ip_flags. id_delta) & 0x07;
	dest[counter] = f_byte;
	counter++;
	return counter;

}
/*
Coding the extension 1 part of the uo1-packet

   Extension 1 (5.11.4):

      +---+---+---+---+---+---+---+---+
 1    | 0   1 |    SN     |   IP-ID   |
      +---+---+---+---+---+---+---+---+
 2    |             IP-ID             |
      +---+---+---+---+---+---+---+---+
*/
int udp_code_EXT1_packet(struct sc_context *context,
	const struct iphdr *ip,
	unsigned char *dest,
	int counter)
{
	//Code for the extension
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;
	unsigned char f_byte;
	unsigned char s_byte;
	unsigned short id;

	// 1
	f_byte = (udp_profile -> sn) & 0x07;
	f_byte = f_byte << 3;
	id = (udp_profile ->ip_flags. id_delta) & 0x0700;
	id = id >> 8;
	f_byte |= id;
	f_byte |= 0x40;

	// 2
	s_byte = (udp_profile ->ip_flags. id_delta) & 0xff;
	dest[counter] = f_byte;
	counter++;
	dest[counter] = s_byte;
	counter++;
	return counter;
}


/*
Coding the extension 1 part of the uo1-packet

   Extension 2 (5.11.4):

      +---+---+---+---+---+---+---+---+
 1    | 1   0 |    SN     |   IP-ID2  |
      +---+---+---+---+---+---+---+---+
 2    |            IP-ID2             |
      +---+---+---+---+---+---+---+---+
 3    |             IP-ID             |
      +---+---+---+---+---+---+---+---+

         IP-ID2: For outer IP-ID field.
*/
int udp_code_EXT2_packet(struct sc_context *context,
	struct sc_udp_context *udp_profile,
	const struct iphdr *ip,
	unsigned char *dest,
	int counter)
{
	//Code for the extension

	unsigned char f_byte;
	unsigned char s_byte;
	unsigned short id;

	//its a bit confusing here but ip-id2 in the packet
	//description means the first headers ip-id and ip-flags
	//mean the first header and ip2-flags means the second
	//ip-header
	// 1
	f_byte = (udp_profile -> sn) & 0x07;
	f_byte = f_byte << 3;
	id = (udp_profile->ip_flags.id_delta) & 0x0700;
	id = id >> 8;
	f_byte |= id;
	f_byte |= 0x80;
	dest[counter] = f_byte;
	counter++;

	// 2
	s_byte = (udp_profile->ip_flags .id_delta) & 0xff;
	dest[counter] = s_byte;
	counter++;

	// 3
	dest[counter] = (udp_profile ->ip2_flags.id_delta) & 0xff;
	counter++;
	return counter;
}

/*
Coding the extension 3 part of the uo2-packet

    Extension 3 (5.7.5 && 5.11.4):

      0     1     2     3     4     5     6     7
   +-----+-----+-----+-----+-----+-----+-----+-----+
1  |  1     1  |  S  |   Mode    |  I  | ip  | ip2 |
   +-----+-----+-----+-----+-----+-----+-----+-----+
2  |            Inner IP header flags        |     |  if ip = 1
   +-----+-----+-----+-----+-----+-----+-----+-----+
3  |            Outer IP header flags              |
   +-----+-----+-----+-----+-----+-----+-----+-----+
4  |                      SN                       |  if S = 1
   +-----+-----+-----+-----+-----+-----+-----+-----+
   |                                               |
5  /            Inner IP header fields             /  variable,
   |                                               |
   +-----+-----+-----+-----+-----+-----+-----+-----+
6  |                     IP-ID                     |  2 octets, if I = 1
   +-----+-----+-----+-----+-----+-----+-----+-----+
   |                                               |
7  /            Outer IP header fields             /  variable,
   |                                               |
   +-----+-----+-----+-----+-----+-----+-----+-----+

*/
int udp_code_EXT3_packet(struct sc_context *context,
	struct sc_udp_context *udp_profile,
	const struct iphdr *ip,
	const struct iphdr *ip2,
	unsigned char *dest,
	int counter)
{
	//Code for the extension
	unsigned char f_byte;
	unsigned short changed_fields = udp_profile->tmp_variables.changed_fields;
	unsigned short changed_fields2 = udp_profile->tmp_variables.changed_fields2;
	int nr_ip_id_bits = udp_profile->tmp_variables.nr_ip_id_bits;
	int nr_ip_id_bits2 = udp_profile->tmp_variables.nr_ip_id_bits2;
	int nr_sn_bits = udp_profile->tmp_variables.nr_sn_bits;

	boolean have_inner = 0;
	boolean have_outer = 0;

	// 1
	f_byte = 0xc0;
	if(nr_sn_bits > 5){
		f_byte |= 0x20;
	}

	f_byte = f_byte | (context -> c_mode) << 3;
	//If random bit is set we have the ip-id field outside this function
	rohc_debugf(1,"rnd_count_up: %d \n",udp_profile->ip_flags.rnd_count);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if(udp_profile->tmp_variables.nr_of_ip_hdr == 1){
		if ((nr_ip_id_bits > 0 && (context->rnd == 0))||(udp_profile->ip_flags.rnd_count < MAX_FO_COUNT && context->rnd == 0)){
			f_byte = f_byte | 0x04;
		}
		if(udp_changed_dynamic_one_hdr(changed_fields,&udp_profile->ip_flags,ip,context->rnd,context->nbo,context) ||
	 	  udp_changed_static_one_hdr(changed_fields,&udp_profile->ip_flags,ip,context))
		{
			have_inner = 1;
			f_byte = f_byte | 0x02;
		}
	}
	else{
		if ((nr_ip_id_bits > 0 && (context->rnd2 == 0))||(udp_profile->ip2_flags.rnd_count < MAX_FO_COUNT && context->rnd2 == 0)){
			f_byte = f_byte | 0x04;

		}
		if(udp_changed_dynamic_one_hdr(changed_fields,&udp_profile->ip_flags,ip,context->rnd,context->nbo,context) ||
	 	  udp_changed_static_one_hdr(changed_fields,&udp_profile->ip_flags,ip,context))
		{
			have_outer = 1;
			f_byte = f_byte | 0x01;
		}
		if(udp_changed_dynamic_one_hdr(changed_fields2,&udp_profile->ip2_flags,ip2,context->rnd2,context->nbo2,context) ||
	  	 udp_changed_static_one_hdr(changed_fields2,&udp_profile->ip2_flags,ip2,context))
		{
			have_inner = 1;
			f_byte = f_byte | 0x02;
		}

	}
	dest[counter] = f_byte;
	counter++;


	if(udp_profile->tmp_variables.nr_of_ip_hdr == 1){
		// 2
		if(have_inner){
			counter = udp_header_flags(context,changed_fields,&udp_profile->ip_flags,counter,0,
						nr_ip_id_bits,ip,dest,context->rnd,context->nbo);
		}
		// 4
		if(nr_sn_bits > 5){
			dest[counter] = (udp_profile -> sn) & 0xff;
			counter++;
		}
		// 5
		if(have_inner){
			counter = udp_header_fields(context,changed_fields,&udp_profile->ip_flags,counter,0,
						nr_ip_id_bits,ip,dest,context->rnd);
		}
		//6
		if(((nr_ip_id_bits > 0) && (context->rnd == 0)) || ((udp_profile->ip_flags.rnd_count-1 < MAX_FO_COUNT) && (context->rnd == 0))){

			memcpy(&dest[counter], &ip->id, 2);
			counter+=2;
		}
	}
	else{
		// 2
		if(have_inner){
			counter = udp_header_flags(context,changed_fields2,&udp_profile->ip2_flags,counter,0,
						nr_ip_id_bits2,ip2,dest,context->rnd2,context->nbo2);
		}
		// 3
		if(have_outer){
			counter = udp_header_flags(context,changed_fields,&udp_profile->ip_flags,counter,1,
						nr_ip_id_bits,ip,dest,context->rnd,context->nbo);

		}
		// 4
		if(nr_sn_bits > 5){
			dest[counter] = (udp_profile -> sn) & 0xff;
			counter++;
		}
		// 5
		if(have_inner){
			counter = udp_header_fields(context,changed_fields2,&udp_profile->ip2_flags,counter,0,
						nr_ip_id_bits2,ip2,dest,context->rnd2);
		}

		//6
		if(((nr_ip_id_bits2 > 0) && (context->rnd2 == 0)) || ((udp_profile->ip2_flags.rnd_count-1 < MAX_FO_COUNT) && (context->rnd2 == 0))){

			memcpy(&dest[counter], &ip2->id, 2);
			counter+=2;
		}

		// 7
		if(have_outer){
			counter = udp_header_fields(context,changed_fields,&udp_profile->ip_flags,counter,1,
						nr_ip_id_bits,ip,dest,context->rnd);
		}
	}
	//No ip extension until list compression
	return counter;
	//////////////////////////////////////////////////////////////////////////////////////////////////////
}

/*
Function coding header flags:

   For inner flags:

   +-----+-----+-----+-----+-----+-----+-----+-----+
1  |            Inner IP header flags        |     |  if ip = 1
   | TOS | TTL | DF  | PR  | IPX | NBO | RND | 0** |  0** reserved
   +-----+-----+-----+-----+-----+-----+-----+-----+


   or for outer flags:

   +-----+-----+-----+-----+-----+-----+-----+-----+
2  |            Outer IP header flags              |
   | TOS2| TTL2| DF2 | PR2 |IPX2 |NBO2 |RND2 |  I2 |  if ip2 = 1
   +-----+-----+-----+-----+-----+-----+-----+-----+
*/
int udp_header_flags(struct sc_context *context,
	unsigned short changed_fields,
	struct sc_udp_header *udp_hdr_profile,
	int counter,
	boolean is_outer,
	int nr_ip_id_bits,
	const struct iphdr *ip,
	unsigned char *dest,
	int context_rnd,
	int context_nbo)
{
	int ip_h_f = 0;

	// 1 and 2
	if(udp_is_changed(changed_fields,MOD_TOS) || (udp_hdr_profile->tos_count < MAX_FO_COUNT)){
		ip_h_f |= 0x80;
	}
	if(udp_is_changed(changed_fields,MOD_TTL) || (udp_hdr_profile->ttl_count < MAX_FO_COUNT)){
		ip_h_f |= 0x40;
	}
	if(udp_is_changed(changed_fields,MOD_PROTOCOL) || (udp_hdr_profile->protocol_count < MAX_FO_COUNT)){
		ip_h_f |= 0x10;
	}

	rohc_debugf(1,"DF=%d\n", GET_DF(ip->frag_off));
	udp_hdr_profile->df_count++;
	ip_h_f |= GET_DF(ip->frag_off) << 5;

	udp_hdr_profile->nbo_count++;
	ip_h_f |= (context_nbo) << 2;

	udp_hdr_profile->rnd_count++;
	ip_h_f |= (context_rnd) << 1;

	// Only 2
	if(is_outer){
		if ((nr_ip_id_bits > 0 && (context->rnd == 0))||(udp_hdr_profile->rnd_count-1 < MAX_FO_COUNT && context->rnd == 0) ){
			ip_h_f |= 0x01;

		}
	}


	// 1 and 2
	dest[counter] = ip_h_f;
	counter++;

	return counter;
}

/*
Function coding header fields:

   For inner fields and outer fields (The function is called two times
   one for inner and one for outer with different arguments):

   +-----+-----+-----+-----+-----+-----+-----+-----+
1  |         Type of Service/Traffic Class         |  if TOS = 1
    ..... ..... ..... ..... ..... ..... ..... .....
2  |         Time to Live/Hop Limit                |  if TTL = 1
    ..... ..... ..... ..... ..... ..... ..... .....
3  |         Protocol/Next Header                  |  if PR = 1
    ..... ..... ..... ..... ..... ..... ..... .....
4  /         IP extension headers                  /  variable, if IPX = 1
    ..... ..... ..... ..... ..... ..... ..... .....

   Ip id is coded here for outer header fields
   +-----+-----+-----+-----+-----+-----+-----+-----+
5  |                  IP-ID                        |  2 octets, if I = 1
   +-----+-----+-----+-----+-----+-----+-----+-----+

4 is not supported.
4 is not supported.

*/
int udp_header_fields(struct sc_context *context,
	unsigned short changed_fields,
	struct sc_udp_header *udp_hdr_profile,
	int counter,
	boolean is_outer,
	int nr_ip_id_bits,
	const struct iphdr *ip,
	unsigned char *dest,
	int context_rnd)
{
	// 1
	if(udp_is_changed(changed_fields,MOD_TOS) || (udp_hdr_profile->tos_count < MAX_FO_COUNT)){
		udp_hdr_profile->tos_count++;
		dest[counter] = ip->tos;
		counter++;
	}

	// 2
	if(udp_is_changed(changed_fields,MOD_TTL) || (udp_hdr_profile->ttl_count < MAX_FO_COUNT)){
		udp_hdr_profile->ttl_count++;
		dest[counter] = ip->ttl;
		counter++;
	}

	// 3
	if(udp_is_changed(changed_fields,MOD_PROTOCOL) || (udp_hdr_profile->protocol_count < MAX_FO_COUNT) ){
		udp_hdr_profile->protocol_count++;
		dest[counter] = ip->protocol;
		counter++;
	}

	// 5
	if(is_outer){
		if(((nr_ip_id_bits > 0) && (context_rnd == 0)) || ((udp_hdr_profile->rnd_count-1 < MAX_FO_COUNT) && (context_rnd == 0))){

			memcpy(&dest[counter], &ip->id, 2);
			counter+=2;

		}
	}


	return counter;
}



/*Decide what extension shall be used in the uo2 packet*/
int udp_decide_extension(struct sc_context *context)
{

	//if (ip_context->ttl_count < MAX_FO_COUNT || ip_context->tos_count < MAX_FO_COUNT || ip_context->df_count < MAX_FO_COUNT)
	//	return c_EXT3;

	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;
	int nr_ip_id_bits = udp_profile->tmp_variables.nr_ip_id_bits;
	int nr_ip_id_bits2 = udp_profile->tmp_variables.nr_ip_id_bits2;
	int nr_sn_bits = udp_profile->tmp_variables.nr_sn_bits;
	if(udp_profile->tmp_variables.nr_of_ip_hdr == 1){
		if(udp_profile->tmp_variables.send_static > 0){
			rohc_debugf(2,"Vi har �drat static\n");
			return c_EXT3;

		}
		if(udp_profile->tmp_variables.send_dynamic > 0){
			return c_EXT3;
		}
		if(nr_sn_bits < 5 && (nr_ip_id_bits == 0 || context->rnd == 1)){
			return c_NOEXT;
		}
		else if(nr_sn_bits <= 8 && nr_ip_id_bits <= 3){
			return c_EXT0;
		}
		else if(nr_sn_bits <= 8 && nr_ip_id_bits <= 11){
			return c_EXT1;
		}
		else{
			return c_EXT3;
		}
	}
	else{
		if(udp_profile->tmp_variables.send_static > 0 || udp_profile->tmp_variables.send_dynamic > 0){
			return c_EXT3;

		}
		if(nr_sn_bits < 5 && (nr_ip_id_bits == 0 || context->rnd == 1)
			&& (nr_ip_id_bits2 == 0 || context->rnd2 == 1)){
			return c_NOEXT;
		}
		else if(nr_sn_bits <= 8 && nr_ip_id_bits <= 3
			&& (nr_ip_id_bits2 == 0 || context->rnd2 == 1)){
			return c_EXT0;
		}
		else if(nr_sn_bits <= 8 && nr_ip_id_bits <= 11
			&& (nr_ip_id_bits2 == 0 || context->rnd2 == 1)){
			return c_EXT1;
		}
		else if(nr_sn_bits <= 3 && nr_ip_id_bits <= 11
		 	&& nr_ip_id_bits2 <= 8){
			return c_EXT2;
		}

		else{
			return c_EXT3;
		}
	}
	
}
/*
Check if the static part of the context has been changed in any of the two first headers
*/
int udp_changed_static_both_hdr(struct sc_context *context,
	const struct iphdr *ip,
	const struct iphdr *ip2)
{

	int return_value = 0;
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;
	unsigned short changed_fields = udp_profile->tmp_variables.changed_fields;
	unsigned short changed_fields2 = udp_profile->tmp_variables.changed_fields2;

	return_value = udp_changed_static_one_hdr(changed_fields,&udp_profile->ip_flags,ip,context);
	if(udp_profile->tmp_variables.nr_of_ip_hdr > 1){
		return_value += udp_changed_static_one_hdr(changed_fields2,&udp_profile->ip2_flags,ip2,context);
	}
	return return_value;
}

/*
Check if the static part of the context has been changed in one of header
*/
int udp_changed_static_one_hdr(unsigned short changed_fields,
	struct sc_udp_header *ip_header,
	const struct iphdr *ip,
	struct sc_context *context)
{
	int return_value = 0;
	struct sc_udp_context *org_udp_profile = (struct sc_udp_context *)context->profile_context;
	

	if (udp_is_changed(changed_fields,MOD_PROTOCOL) || (ip_header->protocol_count < MAX_FO_COUNT)){
		rohc_debugf(2,"protocol_count %d\n",ip_header->protocol_count);
		if (udp_is_changed(changed_fields,MOD_PROTOCOL)) {
			ip_header->protocol_count = 0;
			org_udp_profile->fo_count = 0;
		}


		return_value += 1;
	}
	return return_value;

}


/*
Check if the dynamic parts are changed, it returns the number of
dynamic bytes that are changed in both of the two first headers
*/
int udp_changed_dynamic_both_hdr(struct sc_context *context,
	const struct iphdr *ip,
	const struct iphdr *ip2)
{
	int return_value = 0;
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;
	unsigned short changed_fields = udp_profile->tmp_variables.changed_fields;
	unsigned short changed_fields2 = udp_profile->tmp_variables.changed_fields2;

	return_value = udp_changed_dynamic_one_hdr(changed_fields,&udp_profile->ip_flags,ip,context->rnd,context->nbo,context);
	//rohc_debugf(1,"return_value = %d \n",return_value);
	if(udp_profile->tmp_variables.nr_of_ip_hdr > 1){

		return_value += udp_changed_dynamic_one_hdr(changed_fields2,&udp_profile->ip2_flags,ip2,context->rnd2,context->nbo2,context);
	}
	return return_value;
}

/*
Check if the dynamic part of the context has been changed in one of header
*/
int udp_changed_dynamic_one_hdr(unsigned short changed_fields,
	struct sc_udp_header *udp_hdr_profile,
	const struct iphdr *ip,
	int context_rnd,
	int context_nbo,
	struct sc_context *context)
{
	int return_value = 0;
	int small_flags = 0;
	struct sc_udp_context *org_udp_profile = (struct sc_udp_context *)context->profile_context;
	
	if (udp_is_changed(changed_fields,MOD_TOS) || (udp_hdr_profile->tos_count < MAX_FO_COUNT)){

		if (udp_is_changed(changed_fields,MOD_TOS)){
			udp_hdr_profile->tos_count = 0;
			org_udp_profile->fo_count = 0;
		}
			
		return_value += 1;
	}
	if(udp_is_changed(changed_fields,MOD_TTL) || (udp_hdr_profile->ttl_count < MAX_FO_COUNT)){
		if (udp_is_changed(changed_fields,MOD_TTL)){
			udp_hdr_profile->ttl_count = 0;
			org_udp_profile->fo_count = 0;
		}
			
		return_value += 1;
	}

	if((GET_DF(ip->frag_off) != GET_DF(udp_hdr_profile->old_ip.frag_off)) || (udp_hdr_profile->df_count < MAX_FO_COUNT)){
		if (GET_DF(ip->frag_off) != GET_DF(udp_hdr_profile->old_ip.frag_off)){
			udp_hdr_profile->ttl_count = 0;
			org_udp_profile->fo_count = 0;
		}
		return_value += 1;
	}

	rohc_debugf(2,"rnd_count=%d nbo_count=%d\n", udp_hdr_profile->rnd_count, udp_hdr_profile->nbo_count);
	if(context_rnd != udp_hdr_profile->old_rnd || udp_hdr_profile->rnd_count < MAX_FO_COUNT){

		if (context_rnd != udp_hdr_profile->old_rnd) {
			rohc_debugf(1,"RND changed, reseting count..\n");
			udp_hdr_profile->rnd_count = 0;
			org_udp_profile->fo_count = 0;
		}

		small_flags += 1;

	}
	rohc_debugf(2,"nbo = %d, old_nbo = %d \n",context_nbo,udp_hdr_profile->old_nbo);
	if(context_nbo != udp_hdr_profile->old_nbo || udp_hdr_profile->nbo_count < MAX_FO_COUNT){
		if (context_nbo != udp_hdr_profile->old_nbo) {
			rohc_debugf(1,"NBO changed, reseting count..\n");
			udp_hdr_profile->nbo_count = 0;
			org_udp_profile->fo_count = 0;
		}
			
		small_flags += 1;
	} 
	if(small_flags > 0) {
		return_value += 1;
	}
	//rohc_debugf(1,"return_value = %d \n",return_value);
	rohc_debugf(2,"udp_changed_dynamic_both_hdr()=%d\n", return_value);
	return return_value;
	

}

/* Check if the udp-checksum have changed*/
int udp_changed_udp_dynamic(struct sc_context *context,const struct udphdr *udp){
	struct sc_udp_context *udp_profile = (struct sc_udp_context *)context->profile_context;
	if(((udp->check != 0) && (udp_profile->old_udp.check == 0)) ||
	   ((udp->check == 0) && (udp_profile->old_udp.check != 0)) ||
	   (udp_profile->udp_checksum_change_count < MAX_IR_COUNT)){
		if(((udp->check != 0) && (udp_profile->old_udp.check == 0)) ||
	   	   ((udp->check == 0) && (udp_profile->old_udp.check != 0))){
			udp_profile->udp_checksum_change_count = 0;
		}
		return 1;
	}
	else{
		return 0;
	}
}


/*
Check if a specified field is changed, it is checked against the bitfield
created in the function udp_changed_fields
*/
boolean udp_is_changed(unsigned short changed_fields,unsigned short check_field)
{
	if((changed_fields & check_field) > 0)
		return 1;
	else 
		return 0;
}
/*
This function returns a bitpattern, the bits that are set is changed
*/
unsigned short udp_changed_fields(struct sc_udp_header *udp_hdr_profile,const struct iphdr *ip)
{
	unsigned short ret_value = 0;

	//Kan version och header length bitordersskiftningar ha n�on betydelse fr oss??
	if(udp_hdr_profile -> old_ip.tos != ip -> tos)
		ret_value |= MOD_TOS;
	if(udp_hdr_profile -> old_ip.tot_len != ip -> tot_len)
		ret_value |= MOD_TOT_LEN;
	if(udp_hdr_profile -> old_ip.id != ip -> id)
		ret_value |= MOD_ID;
	if(udp_hdr_profile -> old_ip.frag_off != ip -> frag_off)
		ret_value |= MOD_FRAG_OFF;
	if(udp_hdr_profile -> old_ip.ttl != ip -> ttl)
		ret_value |= MOD_TTL;
	if(udp_hdr_profile -> old_ip.protocol != ip -> protocol)
		ret_value |= MOD_PROTOCOL;
	if(udp_hdr_profile -> old_ip.check != ip -> check)
		ret_value |= MOD_CHECK;
	if(udp_hdr_profile -> old_ip.saddr != ip -> saddr)
		ret_value |= MOD_SADDR;
	if(udp_hdr_profile -> old_ip.daddr != ip -> daddr)
		ret_value |= MOD_DADDR;
		
	return ret_value;
}
/*
Function to determine if the ip-identification has nbo and if it is random
*/
void udp_check_ip_identification(struct sc_udp_header *udp_hdr_profile,
	const struct iphdr *ip,
	int *context_rnd,
	int *context_nbo)
{

	int id1, id2;
	int nbo = -1;

	id1 = ntohs(udp_hdr_profile->old_ip.id);
	id2 = ntohs(ip->id);


	rohc_debugf(2,"1) id1=0x%04x id2=0x%04x\n", id1, id2);

	if (id2-id1 < IPID_MAX_DELTA && id2-id1 > 0)
	{
		nbo = 1;
	} else if ((id1 + IPID_MAX_DELTA > 0xffff) && (id2 < ((id1+IPID_MAX_DELTA) & 0xffff))) {
		nbo = 1;
	}

	if (nbo == -1) {
		// change byte ordering and check nbo=0
		id1 = (id1>>8) | ((id1<<8) & 0xff00);
		id2 = (id2>>8) | ((id2<<8) & 0xff00);

		rohc_debugf(2,"2) id1=0x%04x id2=0x%04x\n", id1, id2);

		if (id2-id1 < IPID_MAX_DELTA && id2-id1 > 0)
		{
			nbo = 0;
		} else if ((id1 + IPID_MAX_DELTA > 0xffff) && (id2 < ((id1+IPID_MAX_DELTA) & 0xffff))) {
			nbo = 0;
		}
	}

	if (nbo == -1) {
		rohc_debugf(2,"check_ip_id(): RND detected!\n");
		*context_rnd = 1;

	} else {
		rohc_debugf(2,"check_ip_id(): NBO=%d\n", nbo);
		*context_rnd = 0;
		*context_nbo = nbo;
	}

}

/*
A struct thate every function must have the first fieldtell what protocol this profile has.
The second row is the profile id-number, Then two strings that is for printing version and
description. And finaly pointers to functions that have to exist in every profile.
*/
struct sc_profile c_udp_profile = {
	17, // IP-Protocol
	2, // Profile ID
	"1.0b", // Version
	"UDP / Compressor", // Description
	c_udp_create,
	c_udp_destroy,
	c_udp_check_context,
	c_udp_encode,
	c_udp_feedback
};

