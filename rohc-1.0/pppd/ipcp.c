/*
 * ipcp.c - PPP IP Control Protocol.
 *
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  Updated by ROHC Project 2003 at Lulea University of Technology, Sweden 
 *  for ROHC handshake..
 */

#define RCSID	"$Id: ipcp.c,v 1.11 2003/05/13 15:22:20 marjuh-8 Exp $"

/*
 * TODO:
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "pppd.h"
#include "fsm.h"
#include "ipcp.h"
#include "pathnames.h"

static const char rcsid[] = RCSID;

/* global vars */
ipcp_options ipcp_wantoptions[NUM_PPP];	/* Options that we want to request */
ipcp_options ipcp_gotoptions[NUM_PPP];	/* Options that peer ack'd */
ipcp_options ipcp_allowoptions[NUM_PPP]; /* Options we allow peer to request */
ipcp_options ipcp_hisoptions[NUM_PPP];	/* Options that we ack'd */

u_int32_t netmask = 0;		/* IP netmask to set on interface */

bool	disable_defaultip = 0;	/* Don't use hostname for default IP adrs */

/* Hook for a plugin to know when IP protocol has come up */
void (*ip_up_hook) __P((void)) = NULL;

/* Hook for a plugin to know when IP protocol has come down */
void (*ip_down_hook) __P((void)) = NULL;

/* Hook for a plugin to choose the remote IP address */
void (*ip_choose_hook) __P((u_int32_t *)) = NULL;

/* Notifiers for when IPCP goes up and down */
struct notifier *ip_up_notifier = NULL;
struct notifier *ip_down_notifier = NULL;

/* local vars */
static int default_route_set[NUM_PPP];	/* Have set up a default route */
static int proxy_arp_set[NUM_PPP];	/* Have created proxy arp entry */
static bool usepeerdns;			/* Ask peer for DNS addrs */
static int ipcp_is_up;			/* have called np_up() */
static bool ask_for_local;		/* request our address from peer */
#if 0
static char vj_value[8];		/* string form of vj option value */
#endif
static char netmask_str[20];		/* string form of netmask value */
static char rohc_value[50];		/* string form of  rohc option value */

/*
 * Callbacks for fsm code.  (CI = Configuration Information)
 */
static void ipcp_resetci __P((fsm *));	/* Reset our CI */
static int  ipcp_cilen __P((fsm *));	        /* Return length of our CI */
static void ipcp_addci __P((fsm *, u_char *, int *)); /* Add our CI */
static int  ipcp_ackci __P((fsm *, u_char *, int));	/* Peer ack'd our CI */
static int  ipcp_nakci __P((fsm *, u_char *, int));	/* Peer nak'd our CI */
static int  ipcp_rejci __P((fsm *, u_char *, int));	/* Peer rej'd our CI */
static int  ipcp_reqci __P((fsm *, u_char *, int *, int)); /* Rcv CI */
static void ipcp_up __P((fsm *));		/* We're UP */
static void ipcp_down __P((fsm *));		/* We're DOWN */
static void ipcp_finished __P((fsm *));	/* Don't need lower layer */

fsm ipcp_fsm[NUM_PPP];		/* IPCP fsm structure */

static fsm_callbacks ipcp_callbacks = { /* IPCP callback routines */
    ipcp_resetci,		/* Reset our Configuration Information */
    ipcp_cilen,			/* Length of our Configuration Information */
    ipcp_addci,			/* Add our Configuration Information */
    ipcp_ackci,			/* ACK our Configuration Information */
    ipcp_nakci,			/* NAK our Configuration Information */
    ipcp_rejci,			/* Reject our Configuration Information */
    ipcp_reqci,			/* Request peer's Configuration Information */
    ipcp_up,			/* Called when fsm reaches OPENED state */
    ipcp_down,			/* Called when fsm leaves OPENED state */
    NULL,			/* Called when we want the lower layer up */
    ipcp_finished,		/* Called when we want the lower layer down */
    NULL,			/* Called when Protocol-Reject received */
    NULL,			/* Retransmission is necessary */
    NULL,			/* Called to handle protocol-specific codes */
    "IPCP"			/* String name of protocol */
};

/*
 * Extra ROHC funtions
 */
static int rohc_get_profiles(u_int16_t * array, int array_size);
static int rohc_find_profile(u_int16_t profile, u_int16_t * array, int array_size);
static int set_rohc_profiles(char **argv);
static void log(char * message);
#define RLOG(x) log(x);
static char log_buf[512], log_buf2[20]; int log_count;
#define RLOGP(p, c) \
	{log_buf[0] = '\0'; \
	strcat(log_buf, "Profiles: "); \
	for (log_count = 0; log_count < (c); log_count++) { \
		sprintf(log_buf2, "%d ", p[log_count]); \
		strcat(log_buf, log_buf2); \
	} \
	log(log_buf); \
	}
	

/*
 * Command-line options.
 */
#if 0
static int setvjslots __P((char **));
#endif
static int setdnsaddr __P((char **));
static int setwinsaddr __P((char **));
static int setnetmask __P((char **));
int setipaddr __P((char *, char **, int));
static void printipaddr __P((option_t *, void (*)(void *, char *,...),void *));

static option_t ipcp_option_list[] = {
    { "noip", o_bool, &ipcp_protent.enabled_flag,
      "Disable IP and IPCP" },
    { "-ip", o_bool, &ipcp_protent.enabled_flag,
      "Disable IP and IPCP", OPT_ALIAS },


#if 0	// ROHC-VJ
    { "novj", o_bool, &ipcp_wantoptions[0].neg_vj,
      "Disable VJ compression", OPT_A2CLR, &ipcp_allowoptions[0].neg_vj },
    { "-vj", o_bool, &ipcp_wantoptions[0].neg_vj,
      "Disable VJ compression", OPT_ALIAS | OPT_A2CLR,
      &ipcp_allowoptions[0].neg_vj },

    { "novjccomp", o_bool, &ipcp_wantoptions[0].cflag,
      "Disable VJ connection-ID compression", OPT_A2CLR,
      &ipcp_allowoptions[0].cflag },
    { "-vjccomp", o_bool, &ipcp_wantoptions[0].cflag,
      "Disable VJ connection-ID compression", OPT_ALIAS | OPT_A2CLR,
      &ipcp_allowoptions[0].cflag },

    { "vj-max-slots", o_special, (void *)setvjslots,
      "Set maximum VJ header slots",
      OPT_PRIO | OPT_A2STRVAL | OPT_STATIC, vj_value },
#endif

    { "novj", o_bool, &ipcp_wantoptions[0].dummy,
      "Dummy VJ", OPT_A2CLR, &ipcp_allowoptions[0].dummy },
    { "novjccomp", o_bool, &ipcp_wantoptions[0].dummy,
      "Dummy VJ", OPT_A2CLR,
      &ipcp_allowoptions[0].dummy },

    { "norohc", o_bool, &ipcp_wantoptions[0].neg_rohc,
      "Disable ROHC compression", OPT_A2CLR, &ipcp_allowoptions[0].neg_rohc },
    { "-rohc", o_bool, &ipcp_wantoptions[0].neg_rohc,
      "Disable ROHC compression", OPT_ALIAS | OPT_A2CLR,
      &ipcp_allowoptions[0].neg_rohc },
    { "rohc-max-cid", o_int, &ipcp_wantoptions[0].max_cid,
      "Set maximum number of context (ROHC)", OPT_LLIMIT | OPT_ULIMIT,
      &ipcp_allowoptions[0].max_cid, ROHC_MAX_CID, 1 },
    { "rohc-profiles", o_special, (void *)set_rohc_profiles,
      "Set what profiles to use, use \"all\" for all",
      OPT_PRIO | OPT_A2STRVAL | OPT_STATIC, rohc_value },


/*
typedef struct {
	char	*name;
	enum opt_type type;
	void	*addr;
	char	*description;
	unsigned int flags;
	void	*addr2;
	int	upper_limit;
	int	lower_limit;
	const char *source;
	short int priority;
	short int winner;
	void	*addr3;
} option_t;
*/



    { "ipcp-accept-local", o_bool, &ipcp_wantoptions[0].accept_local,
      "Accept peer's address for us", 1 },
    { "ipcp-accept-remote", o_bool, &ipcp_wantoptions[0].accept_remote,
      "Accept peer's address for it", 1 },

    { "ipparam", o_string, &ipparam,
      "Set ip script parameter", OPT_PRIO },

    { "noipdefault", o_bool, &disable_defaultip,
      "Don't use name for default IP adrs", 1 },

    { "ms-dns", 1, (void *)setdnsaddr,
      "DNS address for the peer's use" },
    { "ms-wins", 1, (void *)setwinsaddr,
      "Nameserver for SMB over TCP/IP for peer" },

    { "ipcp-restart", o_int, &ipcp_fsm[0].timeouttime,
      "Set timeout for IPCP", OPT_PRIO },
    { "ipcp-max-terminate", o_int, &ipcp_fsm[0].maxtermtransmits,
      "Set max #xmits for term-reqs", OPT_PRIO },
    { "ipcp-max-configure", o_int, &ipcp_fsm[0].maxconfreqtransmits,
      "Set max #xmits for conf-reqs", OPT_PRIO },
    { "ipcp-max-failure", o_int, &ipcp_fsm[0].maxnakloops,
      "Set max #conf-naks for IPCP", OPT_PRIO },

    { "defaultroute", o_bool, &ipcp_wantoptions[0].default_route,
      "Add default route", OPT_ENABLE|1, &ipcp_allowoptions[0].default_route },
    { "nodefaultroute", o_bool, &ipcp_allowoptions[0].default_route,
      "disable defaultroute option", OPT_A2CLR,
      &ipcp_wantoptions[0].default_route },
    { "-defaultroute", o_bool, &ipcp_allowoptions[0].default_route,
      "disable defaultroute option", OPT_ALIAS | OPT_A2CLR,
      &ipcp_wantoptions[0].default_route },

    { "proxyarp", o_bool, &ipcp_wantoptions[0].proxy_arp,
      "Add proxy ARP entry", OPT_ENABLE|1, &ipcp_allowoptions[0].proxy_arp },
    { "noproxyarp", o_bool, &ipcp_allowoptions[0].proxy_arp,
      "disable proxyarp option", OPT_A2CLR,
      &ipcp_wantoptions[0].proxy_arp },
    { "-proxyarp", o_bool, &ipcp_allowoptions[0].proxy_arp,
      "disable proxyarp option", OPT_ALIAS | OPT_A2CLR,
      &ipcp_wantoptions[0].proxy_arp },

    { "usepeerdns", o_bool, &usepeerdns,
      "Ask peer for DNS address(es)", 1 },

    { "netmask", o_special, (void *)setnetmask,
      "set netmask", OPT_PRIO | OPT_A2STRVAL | OPT_STATIC, netmask_str },

    { "ipcp-no-addresses", o_bool, &ipcp_wantoptions[0].old_addrs,
      "Disable old-style IP-Addresses usage", OPT_A2CLR,
      &ipcp_allowoptions[0].old_addrs },
    { "ipcp-no-address", o_bool, &ipcp_wantoptions[0].neg_addr,
      "Disable IP-Address usage", OPT_A2CLR,
      &ipcp_allowoptions[0].neg_addr },

    { "IP addresses", o_wild, (void *) &setipaddr,
      "set local and remote IP addresses",
      OPT_NOARG | OPT_A2PRINTER, (void *) &printipaddr },

    { NULL }
};

/*
 * Protocol entry points from main code.
 */
static void ipcp_init __P((int));
static void ipcp_open __P((int));
static void ipcp_close __P((int, char *));
static void ipcp_lowerup __P((int));
static void ipcp_lowerdown __P((int));
static void ipcp_input __P((int, u_char *, int));
static void ipcp_protrej __P((int));
static int  ipcp_printpkt __P((u_char *, int,
			       void (*) __P((void *, char *, ...)), void *));
static void ip_check_options __P((void));
static int  ip_demand_conf __P((int));
static int  ip_active_pkt __P((u_char *, int));
static void create_resolv __P((u_int32_t, u_int32_t));

struct protent ipcp_protent = {
    PPP_IPCP,
    ipcp_init,
    ipcp_input,
    ipcp_protrej,
    ipcp_lowerup,
    ipcp_lowerdown,
    ipcp_open,
    ipcp_close,
    ipcp_printpkt,
    NULL,
    1,
    "IPCP",
    "IP",
    ipcp_option_list,
    ip_check_options,
    ip_demand_conf,
    ip_active_pkt
};

static void ipcp_clear_addrs __P((int, u_int32_t, u_int32_t));
static void ipcp_script __P((char *));		/* Run an up/down script */
static void ipcp_script_done __P((void *));

/*
 * Lengths of configuration options.
 */
#define CILEN_VOID	2
#define CILEN_COMPRESS	4	/* min length for compression protocol opt. */
#define CILEN_VJ	6	/* length for RFC1332 Van-Jacobson opt. */
#define CILEN_ADDR	6	/* new-style single address option */
#define CILEN_ADDRS	10	/* old-style dual address option */


#define CODENAME(x)	((x) == CONFACK ? "ACK" : \
			 (x) == CONFNAK ? "NAK" : "REJ")

/*
 * This state variable is used to ensure that we don't
 * run an ipcp-up/down script while one is already running.
 */
static enum script_state {
    s_down,
    s_up,
} ipcp_script_state;
static pid_t ipcp_script_pid;

/*
 * Make a string representation of a network IP address.
 */
char *
ip_ntoa(ipaddr)
u_int32_t ipaddr;
{
    static char b[64];

    slprintf(b, sizeof(b), "%I", ipaddr);
    return b;
}

/*
 * Option parsing.
 */

/*
 * setvjslots - set maximum number of connection slots for VJ compression
 */
#if 0	// ROHC-VJ
static int
setvjslots(argv)
    char **argv;
{
    int value;

    if (!int_option(*argv, &value))
	return 0;
    if (value < 2 || value > 16) {
	option_error("vj-max-slots value must be between 2 and 16");
	return 0;
    }
    ipcp_wantoptions [0].maxslotindex =
        ipcp_allowoptions[0].maxslotindex = value - 1;
    slprintf(vj_value, sizeof(vj_value), "%d", value);
    return 1;
}
#endif

/*
 * setdnsaddr - set the dns address(es)
 */
static int
setdnsaddr(argv)
    char **argv;
{
    u_int32_t dns;
    struct hostent *hp;

    dns = inet_addr(*argv);
    if (dns == (u_int32_t) -1) {
	if ((hp = gethostbyname(*argv)) == NULL) {
	    option_error("invalid address parameter '%s' for ms-dns option",
			 *argv);
	    return 0;
	}
	dns = *(u_int32_t *)hp->h_addr;
    }

    /* We take the last 2 values given, the 2nd-last as the primary
       and the last as the secondary.  If only one is given it
       becomes both primary and secondary. */
    if (ipcp_allowoptions[0].dnsaddr[1] == 0)
	ipcp_allowoptions[0].dnsaddr[0] = dns;
    else
	ipcp_allowoptions[0].dnsaddr[0] = ipcp_allowoptions[0].dnsaddr[1];

    /* always set the secondary address value. */
    ipcp_allowoptions[0].dnsaddr[1] = dns;

    return (1);
}

/*
 * setwinsaddr - set the wins address(es)
 * This is primrarly used with the Samba package under UNIX or for pointing
 * the caller to the existing WINS server on a Windows NT platform.
 */
static int
setwinsaddr(argv)
    char **argv;
{
    u_int32_t wins;
    struct hostent *hp;

    wins = inet_addr(*argv);
    if (wins == (u_int32_t) -1) {
	if ((hp = gethostbyname(*argv)) == NULL) {
	    option_error("invalid address parameter '%s' for ms-wins option",
			 *argv);
	    return 0;
	}
	wins = *(u_int32_t *)hp->h_addr;
    }

    /* We take the last 2 values given, the 2nd-last as the primary
       and the last as the secondary.  If only one is given it
       becomes both primary and secondary. */
    if (ipcp_allowoptions[0].winsaddr[1] == 0)
	ipcp_allowoptions[0].winsaddr[0] = wins;
    else
	ipcp_allowoptions[0].winsaddr[0] = ipcp_allowoptions[0].winsaddr[1];

    /* always set the secondary address value. */
    ipcp_allowoptions[0].winsaddr[1] = wins;

    return (1);
}

/*
 * setipaddr - Set the IP address
 * If doit is 0, the call is to check whether this option is
 * potentially an IP address specification.
 * Not static so that plugins can call it to set the addresses
 */
int
setipaddr(arg, argv, doit)
    char *arg;
    char **argv;
    int doit;
{
    struct hostent *hp;
    char *colon;
    u_int32_t local, remote;
    ipcp_options *wo = &ipcp_wantoptions[0];
    static int prio_local = 0, prio_remote = 0;

    /*
     * IP address pair separated by ":".
     */
    if ((colon = strchr(arg, ':')) == NULL)
	return 0;
    if (!doit)
	return 1;

    /*
     * If colon first character, then no local addr.
     */
    if (colon != arg && option_priority >= prio_local) {
	*colon = '\0';
	if ((local = inet_addr(arg)) == (u_int32_t) -1) {
	    if ((hp = gethostbyname(arg)) == NULL) {
		option_error("unknown host: %s", arg);
		return 0;
	    }
	    local = *(u_int32_t *)hp->h_addr;
	}
	if (bad_ip_adrs(local)) {
	    option_error("bad local IP address %s", ip_ntoa(local));
	    return 0;
	}
	if (local != 0)
	    wo->ouraddr = local;
	*colon = ':';
	prio_local = option_priority;
    }

    /*
     * If colon last character, then no remote addr.
     */
    if (*++colon != '\0' && option_priority >= prio_remote) {
	if ((remote = inet_addr(colon)) == (u_int32_t) -1) {
	    if ((hp = gethostbyname(colon)) == NULL) {
		option_error("unknown host: %s", colon);
		return 0;
	    }
	    remote = *(u_int32_t *)hp->h_addr;
	    if (remote_name[0] == 0)
		strlcpy(remote_name, colon, sizeof(remote_name));
	}
	if (bad_ip_adrs(remote)) {
	    option_error("bad remote IP address %s", ip_ntoa(remote));
	    return 0;
	}
	if (remote != 0)
	    wo->hisaddr = remote;
	prio_remote = option_priority;
    }

    return 1;
}

static void
printipaddr(opt, printer, arg)
    option_t *opt;
    void (*printer) __P((void *, char *, ...));
    void *arg;
{
	ipcp_options *wo = &ipcp_wantoptions[0];

	if (wo->ouraddr != 0)
		printer(arg, "%I", wo->ouraddr);
	printer(arg, ":");
	if (wo->hisaddr != 0)
		printer(arg, "%I", wo->hisaddr);
}

/*
 * setnetmask - set the netmask to be used on the interface.
 */
static int
setnetmask(argv)
    char **argv;
{
    u_int32_t mask;
    int n;
    char *p;

    /*
     * Unfortunately, if we use inet_addr, we can't tell whether
     * a result of all 1s is an error or a valid 255.255.255.255.
     */
    p = *argv;
    n = parse_dotted_ip(p, &mask);

    mask = htonl(mask);

    if (n == 0 || p[n] != 0 || (netmask & ~mask) != 0) {
	option_error("invalid netmask value '%s'", *argv);
	return 0;
    }

    netmask = mask;
    slprintf(netmask_str, sizeof(netmask_str), "%I", mask);

    return (1);
}

int
parse_dotted_ip(p, vp)
    char *p;
    u_int32_t *vp;
{
    int n;
    u_int32_t v, b;
    char *endp, *p0 = p;

    v = 0;
    for (n = 3;; --n) {
	b = strtoul(p, &endp, 0);
	if (endp == p)
	    return 0;
	if (b > 255) {
	    if (n < 3)
		return 0;
	    /* accept e.g. 0xffffff00 */
	    *vp = b;
	    return endp - p0;
	}
	v |= b << (n * 8);
	p = endp;
	if (n == 0)
	    break;
	if (*p != '.')
	    return 0;
	++p;
    }
    *vp = v;
    return p - p0;
}


/*
 * ipcp_init - Initialize IPCP.
 */
static void
ipcp_init(unit)
    int unit;
{
    fsm *f = &ipcp_fsm[unit];
    ipcp_options *wo = &ipcp_wantoptions[unit];
    ipcp_options *ao = &ipcp_allowoptions[unit];

    f->unit = unit;
    f->protocol = PPP_IPCP;
    f->callbacks = &ipcp_callbacks;
    fsm_init(&ipcp_fsm[unit]);

    memset(wo, 0, sizeof(*wo));
    memset(ao, 0, sizeof(*ao));

    wo->neg_addr = wo->old_addrs = 1;
    ao->neg_addr = ao->old_addrs = 1;

#if 0	// ROHC-VJ
    wo->neg_vj = 1;
    wo->vj_protocol = IPCP_VJ_COMP;
    wo->maxslotindex = MAX_STATES - 1; /* really max index */
    wo->cflag = 1;


    /* max slots and slot-id compression are currently hardwired in */
    /* ppp_if.c to 16 and 1, this needs to be changed (among other */
    /* things) gmc */

    ao->neg_vj = 1;
    ao->maxslotindex = MAX_STATES - 1;
    ao->cflag = 1;
#endif

    /*
     * XXX These control whether the user may use the proxyarp
     * and defaultroute options.
     */
    ao->proxy_arp = 1;
    ao->default_route = 1;

    /* ROHC */
    wo->neg_rohc = 1;
    wo->max_cid = 16;
    wo->mrru = 0;
    wo->max_header = 168;
    wo->profile_count = rohc_get_profiles(wo->profiles, ROHC_MAX_PROFILES);

    /* What we accept */
    ao->neg_rohc = 1;
    ao->max_cid = ROHC_MAX_CID;
    ao->mrru = 0;
    ao->max_header = 168;
    ao->profile_count = wo->profile_count;
    if (wo->profile_count > 0) {
	memcpy(ao->profiles, wo->profiles,
	    sizeof(u_int16_t) * wo->profile_count);
    } else {
	wo->neg_rohc = 0;
	ao->neg_rohc = 0;
	RLOG("init: Disable rohc");
    }

    RLOG("init");
}


/*
 * ipcp_open - IPCP is allowed to come up.
 */
static void
ipcp_open(unit)
    int unit;
{
    fsm_open(&ipcp_fsm[unit]);
}


/*
 * ipcp_close - Take IPCP down.
 */
static void
ipcp_close(unit, reason)
    int unit;
    char *reason;
{
    fsm_close(&ipcp_fsm[unit], reason);
}


/*
 * ipcp_lowerup - The lower layer is up.
 */
static void
ipcp_lowerup(unit)
    int unit;
{
    fsm_lowerup(&ipcp_fsm[unit]);
}


/*
 * ipcp_lowerdown - The lower layer is down.
 */
static void
ipcp_lowerdown(unit)
    int unit;
{
    fsm_lowerdown(&ipcp_fsm[unit]);
}


/*
 * ipcp_input - Input IPCP packet.
 */
static void
ipcp_input(unit, p, len)
    int unit;
    u_char *p;
    int len;
{
    fsm_input(&ipcp_fsm[unit], p, len);
}


/*
 * ipcp_protrej - A Protocol-Reject was received for IPCP.
 *
 * Pretend the lower layer went down, so we shut up.
 */
static void
ipcp_protrej(unit)
    int unit;
{
    fsm_lowerdown(&ipcp_fsm[unit]);
}


/*
 * ipcp_resetci - Reset our CI.
 * Called by fsm_sconfreq, Send Configure Request.
 */
static void
ipcp_resetci(f)
    fsm *f;
{
    ipcp_options *wo = &ipcp_wantoptions[f->unit];
    ipcp_options *go = &ipcp_gotoptions[f->unit];
    ipcp_options *ao = &ipcp_allowoptions[f->unit];

    wo->req_addr = (wo->neg_addr || wo->old_addrs) &&
	(ao->neg_addr || ao->old_addrs);
    if (wo->ouraddr == 0)
	wo->accept_local = 1;
    if (wo->hisaddr == 0)
	wo->accept_remote = 1;
    wo->req_dns1 = usepeerdns;	/* Request DNS addresses from the peer */
    wo->req_dns2 = usepeerdns;
    *go = *wo;
    if (!ask_for_local)
	go->ouraddr = 0;
    if (ip_choose_hook) {
	ip_choose_hook(&wo->hisaddr);
	if (wo->hisaddr) {
	    wo->accept_remote = 0;
	}
    }
}


/*
 * ipcp_cilen - Return length of our CI.
 * Called by fsm_sconfreq, Send Configure Request.
 */
static int
ipcp_cilen(f)
    fsm *f;
{
    ipcp_options *go = &ipcp_gotoptions[f->unit];
    ipcp_options *wo = &ipcp_wantoptions[f->unit];
#if 0
    ipcp_options *ho = &ipcp_hisoptions[f->unit];
#endif

#define LENCIADDRS(neg)		(neg ? CILEN_ADDRS : 0)
#define LENCIVJ(neg, old)	(neg ? (old? CILEN_COMPRESS : CILEN_VJ) : 0)
#define LENCIADDR(neg)		(neg ? CILEN_ADDR : 0)
#define LENCIDNS(neg)		(neg ? (CILEN_ADDR) : 0)
#define LENCIROHC(neg, count)	(neg ? (12 + 2 * count) : 0)
    /*
     * First see if we want to change our options to the old
     * forms because we have received old forms from the peer.
     */
    if ((wo->neg_addr || wo->old_addrs) && !go->neg_addr && !go->old_addrs) {
	/* use the old style of address negotiation */
	go->old_addrs = 1;
    }
#if 0
    if (wo->neg_vj && !go->neg_vj && !go->old_vj) {
	/* try an older style of VJ negotiation */
	/* use the old style only if the peer did */
	if (ho->neg_vj && ho->old_vj) {
	    go->neg_vj = 1;
	    go->old_vj = 1;
	    go->vj_protocol = ho->vj_protocol;
	}
    }
#endif
    sprintf(log_buf, "returning length %d",
        LENCIROHC(go->neg_rohc, go->profile_count));
    RLOG(log_buf);
    return (LENCIADDRS(!go->neg_addr && go->old_addrs) +
#if 0
	    LENCIVJ(go->neg_vj, go->old_vj) +
#endif
	    LENCIROHC(go->neg_rohc, go->profile_count) +
	    LENCIADDR(go->neg_addr) +
	    LENCIDNS(go->req_dns1) +
	    LENCIDNS(go->req_dns2)) ;
}


/*
 * ipcp_addci - Add our desired CIs to a packet.
 * Called by fsm_sconfreq, Send Configure Request.
 */
static void
ipcp_addci(f, ucp, lenp)
    fsm *f;
    u_char *ucp;
    int *lenp;
{
    ipcp_options *go = &ipcp_gotoptions[f->unit];
    int len = *lenp;

#define ADDCIADDRS(opt, neg, val1, val2) \
    if (neg) { \
	if (len >= CILEN_ADDRS) { \
	    u_int32_t l; \
	    PUTCHAR(opt, ucp); \
	    PUTCHAR(CILEN_ADDRS, ucp); \
	    l = ntohl(val1); \
	    PUTLONG(l, ucp); \
	    l = ntohl(val2); \
	    PUTLONG(l, ucp); \
	    len -= CILEN_ADDRS; \
	} else \
	    go->old_addrs = 0; \
    }

#define ADDCIVJ(opt, neg, val, old, maxslotindex, cflag) \
    if (neg) { \
	int vjlen = old? CILEN_COMPRESS : CILEN_VJ; \
	if (len >= vjlen) { \
	    PUTCHAR(opt, ucp); \
	    PUTCHAR(vjlen, ucp); \
	    PUTSHORT(val, ucp); \
	    if (!old) { \
		PUTCHAR(maxslotindex, ucp); \
		PUTCHAR(cflag, ucp); \
	    } \
	    len -= vjlen; \
	} else \
	    neg = 0; \
    }

#define ADDCIROHC(opt, neg, max_cid, mrru, max_header, profile_count, profiles) \
	if (neg) { \
		int i; \
		int rohc_pro_len = profile_count * 2 + 2; \
		int rohc_len = rohc_pro_len + 10; \
		RLOG("Add creating package"); \
		if (len >= rohc_len) { \
			PUTCHAR(opt, ucp); \
			PUTCHAR(rohc_len, ucp); \
			PUTSHORT(CI_ROHC_PROTOCOL, ucp); \
			PUTSHORT(max_cid, ucp); \
			PUTSHORT(mrru, ucp); \
			PUTSHORT(max_header, ucp); \
			/* SUBOPTIONS - profiles */ \
			PUTCHAR(ROHC_SUBOPT_PROFILE, ucp) \
			PUTCHAR(rohc_pro_len, ucp); \
			for (i = 0; i < profile_count; i++) { \
				PUTSHORT(profiles[i], ucp); \
			} \
			len -= rohc_len; \
		} else \
			neg = 0; \
	}

#define ADDCIADDR(opt, neg, val) \
    if (neg) { \
	if (len >= CILEN_ADDR) { \
	    u_int32_t l; \
	    PUTCHAR(opt, ucp); \
	    PUTCHAR(CILEN_ADDR, ucp); \
	    l = ntohl(val); \
	    PUTLONG(l, ucp); \
	    len -= CILEN_ADDR; \
	} else \
	    neg = 0; \
    }

#define ADDCIDNS(opt, neg, addr) \
    if (neg) { \
	if (len >= CILEN_ADDR) { \
	    u_int32_t l; \
	    PUTCHAR(opt, ucp); \
	    PUTCHAR(CILEN_ADDR, ucp); \
	    l = ntohl(addr); \
	    PUTLONG(l, ucp); \
	    len -= CILEN_ADDR; \
	} else \
	    neg = 0; \
    }

    ADDCIADDRS(CI_ADDRS, !go->neg_addr && go->old_addrs, go->ouraddr,
	       go->hisaddr);

    RLOG("Add function");
    if (1) {
        u_char * beg = ucp;
    ADDCIROHC(CI_COMPRESSTYPE, go->neg_rohc, go->max_cid, go->mrru,
    	go->max_header, go->profile_count, go->profiles);

	sprintf(log_buf, "Length = %d", (ucp - beg)); RLOG(log_buf);
	while(beg != ucp) {
	    sprintf(log_buf, "char = %d %x", *beg, *beg);
	    RLOG(log_buf);
	    beg++;
	}
    }

#if 0
    ADDCIVJ(CI_COMPRESSTYPE, go->neg_vj, go->vj_protocol, go->old_vj,
	    go->maxslotindex, go->cflag);
#endif

    ADDCIADDR(CI_ADDR, go->neg_addr, go->ouraddr);

    ADDCIDNS(CI_MS_DNS1, go->req_dns1, go->dnsaddr[0]);

    ADDCIDNS(CI_MS_DNS2, go->req_dns2, go->dnsaddr[1]);

    *lenp -= len;
}


/*
 * ipcp_ackci - Ack our CIs.
 * Called by fsm_rconfack, Receive Configure ACK.
 *
 * Returns:
 *	0 - Ack was bad.
 *	1 - Ack was good.
 */
static int
ipcp_ackci(f, p, len)
    fsm *f;
    u_char *p;
    int len;
{
    ipcp_options *go = &ipcp_gotoptions[f->unit];
    u_short cilen, citype, cishort;
    u_int32_t cilong;
#if 0  // ROHC-VJ
    u_char cimaxslotindex, cicflag
#endif
    //int i, rohc_pro_len, rohc_len;

    /*
     * CIs must be in exactly the same order that we sent...
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */

#define ACKCIADDRS(opt, neg, val1, val2) \
    if (neg) { \
	u_int32_t l; \
	if ((len -= CILEN_ADDRS) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != CILEN_ADDRS || \
	    citype != opt) \
	    goto bad; \
	GETLONG(l, p); \
	cilong = htonl(l); \
	if (val1 != cilong) \
	    goto bad; \
	GETLONG(l, p); \
	cilong = htonl(l); \
	if (val2 != cilong) \
	    goto bad; \
    }

#define ACKCIVJ(opt, neg, val, old, maxslotindex, cflag) \
    if (neg) { \
	int vjlen = old? CILEN_COMPRESS : CILEN_VJ; \
	if ((len -= vjlen) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != vjlen || \
	    citype != opt)  \
	    goto bad; \
	GETSHORT(cishort, p); \
	if (cishort != val) \
	    goto bad; \
	if (!old) { \
	    GETCHAR(cimaxslotindex, p); \
	    if (cimaxslotindex != maxslotindex) \
		goto bad; \
	    GETCHAR(cicflag, p); \
	    if (cicflag != cflag) \
		goto bad; \
	} \
    }

#define CHARTEST(with) \
	GETCHAR(cichar, p); \
	if (cichar != (with)) {\
		goto bad; \
	} else

#define SHORTTEST(with) \
	GETSHORT(cishort, p); \
	if (cishort != (with)) {\
		goto bad; \
	} else

    // Kontrollerar att inställningarna är samma som vi sände (?)
#define ACKCIROHC(opt, neg, max_cid, mrru, max_header, profile_count, profiles) \
	if (neg) { \
		u_char cichar; \
		int i; \
		int rohc_pro_len = profile_count * 2 + 2; \
		int rohc_len = rohc_pro_len + 10; \
		if ((len -= rohc_len) < 0) \
			goto bad; \
		CHARTEST(opt); \
		CHARTEST(rohc_len); \
		SHORTTEST(CI_ROHC_PROTOCOL); \
		SHORTTEST(max_cid); \
		SHORTTEST(mrru); \
		SHORTTEST(max_header); \
		CHARTEST(ROHC_SUBOPT_PROFILE); \
		CHARTEST(rohc_pro_len); \
		for (i = 0; i < profile_count; i++) { \
			SHORTTEST(profiles[i]); \
		} \
		RLOG("Ack rohc package"); \
	}

#define ACKCIADDR(opt, neg, val) \
    if (neg) { \
	u_int32_t l; \
	if ((len -= CILEN_ADDR) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != CILEN_ADDR || \
	    citype != opt) \
	    goto bad; \
	GETLONG(l, p); \
	cilong = htonl(l); \
	if (val != cilong) \
	    goto bad; \
    }

#define ACKCIDNS(opt, neg, addr) \
    if (neg) { \
	u_int32_t l; \
	if ((len -= CILEN_ADDR) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != CILEN_ADDR || citype != opt) \
	    goto bad; \
	GETLONG(l, p); \
	cilong = htonl(l); \
	if (addr != cilong) \
	    goto bad; \
    }

    ACKCIADDRS(CI_ADDRS, !go->neg_addr && go->old_addrs, go->ouraddr,
	       go->hisaddr);

    RLOG("Ack function");
    ACKCIROHC(CI_COMPRESSTYPE, go->neg_rohc, go->max_cid, go->mrru,
    	go->max_header, go->profile_count, go->profiles);

#if 0  // ROHC-VJ
    ACKCIVJ(CI_COMPRESSTYPE, go->neg_vj, go->vj_protocol, go->old_vj,
	    go->maxslotindex, go->cflag);
#endif

    ACKCIADDR(CI_ADDR, go->neg_addr, go->ouraddr);

    ACKCIDNS(CI_MS_DNS1, go->req_dns1, go->dnsaddr[0]);

    ACKCIDNS(CI_MS_DNS2, go->req_dns2, go->dnsaddr[1]);

    /*
     * If there are any remaining CIs, then this packet is bad.
     */
    if (len != 0)
	goto bad;
    return (1);

bad:
    IPCPDEBUG(("ipcp_ackci: received bad Ack!"));
    return (0);
}

/*
 * ipcp_nakci - Peer has sent a NAK for some of our CIs.
 * This should not modify any state if the Nak is bad
 * or if IPCP is in the OPENED state.
 * Calback from fsm_rconfnakrej - Receive Configure-Nak or Configure-Reject.
 *
 * Returns:
 *	0 - Nak was bad.
 *	1 - Nak was good.
 */
static int
ipcp_nakci(f, p, len)
    fsm *f;
    u_char *p;
    int len;
{
    ipcp_options *go = &ipcp_gotoptions[f->unit];
    ipcp_options *ao = &ipcp_allowoptions[f->unit];
//    u_char cimaxslotindex, cicflag;
    u_char citype, cilen, *next, cislen;
    u_short cishort, cimaxcid, cimrru, cimaxheader;
    u_int32_t ciaddr1, ciaddr2, l, cidnsaddr;
    ipcp_options no;		/* options we've seen Naks for */
    ipcp_options try;		/* options to request next time */

    BZERO(&no, sizeof(no));
    try = *go;

    /*
     * Any Nak'd CIs must be in exactly the same order that we sent.
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */
#define NAKCIADDRS(opt, neg, code) \
    if ((neg) && \
	(cilen = p[1]) == CILEN_ADDRS && \
	len >= cilen && \
	p[0] == opt) { \
	len -= cilen; \
	INCPTR(2, p); \
	GETLONG(l, p); \
	ciaddr1 = htonl(l); \
	GETLONG(l, p); \
	ciaddr2 = htonl(l); \
	no.old_addrs = 1; \
	code \
    }

#define NAKCIVJ(opt, neg, code) \
    if (go->neg && \
	((cilen = p[1]) == CILEN_COMPRESS || cilen == CILEN_VJ) && \
	len >= cilen && \
	p[0] == opt) { \
	len -= cilen; \
	INCPTR(2, p); \
	GETSHORT(cishort, p); \
	no.neg = 1; \
        code \
    }

#define NAKCIROHC(opt, neg, code) \
    if (go->neg && \
	((cilen = p[1]) >= 10) && \
	len >= cilen && \
	p[0] == opt) { \
	len -= cilen; \
	INCPTR(2, p); \
	GETSHORT(cishort, p); \
	no.neg = 1; \
	RLOG("Nack function"); \
	code \
    }

#define NAKCIADDR(opt, neg, code) \
    if (go->neg && \
	(cilen = p[1]) == CILEN_ADDR && \
	len >= cilen && \
	p[0] == opt) { \
	len -= cilen; \
	INCPTR(2, p); \
	GETLONG(l, p); \
	ciaddr1 = htonl(l); \
	no.neg = 1; \
	code \
    }

#define NAKCIDNS(opt, neg, code) \
    if (go->neg && \
	((cilen = p[1]) == CILEN_ADDR) && \
	len >= cilen && \
	p[0] == opt) { \
	len -= cilen; \
	INCPTR(2, p); \
	GETLONG(l, p); \
	cidnsaddr = htonl(l); \
	no.neg = 1; \
	code \
    }

    /*
     * Accept the peer's idea of {our,his} address, if different
     * from our idea, only if the accept_{local,remote} flag is set.
     */
    NAKCIADDRS(CI_ADDRS, !go->neg_addr && go->old_addrs,
	       if (go->accept_local && ciaddr1) { /* Do we know our address? */
		   try.ouraddr = ciaddr1;
	       }
	       if (go->accept_remote && ciaddr2) { /* Does he know his? */
		   try.hisaddr = ciaddr2;
	       }
	);

    /*
     * ROHC - Accept values that is lower and remove
     * uncommon profiles.
     */
    RLOG("Nack function");
    NAKCIROHC(CI_COMPRESSTYPE, neg_rohc,
	    GETSHORT(cimaxcid, p);
	    GETSHORT(cimrru, p);
	    GETSHORT(cimaxheader, p);
	    if (cishort != CI_ROHC_PROTOCOL) {
		try.neg_rohc = 0;
		goto rohc_end;
	    }
	    if (cimaxcid < go->max_cid)
		try.max_cid = cimaxcid;
	    if (cimrru < go->mrru)
		try.mrru = cimrru;
	    if (cimaxheader < go->max_header)
		try.max_header = cimaxheader;
	    if (cilen == 10) {
		/* no profiles ? */
		try.neg_rohc = 0;
		goto rohc_end;
	    }
	    GETCHAR(citype, p);
	    GETCHAR(cislen, p);
	    if (citype != ROHC_SUBOPT_PROFILE)
		goto bad; /* rearrangement not allowed! */

	    try.profile_count = 0;
	    for (cislen -= 2; cislen > 0; cislen -= 2) {
		GETSHORT(cishort, p);
		if (rohc_find_profile(cishort, ao->profiles, ao->profile_count)) {
		    /* valid profile */
		    try.profiles[try.profile_count] = cishort;
		    try.profile_count++;
		} else {
		    /* invalid profile, ignore :) */
		    ;
		}
	    }
	    if (try.profile_count == 0) {
		/* no profiles, no fun */
		try.neg_rohc = 0;
	    }
	    /* ignore other suboptions */
	    rohc_end:
	    );

    /*
     * Accept the peer's value of maxslotindex provided that it
     * is less than what we asked for.  Turn off slot-ID compression
     * if the peer wants.  Send old-style compress-type option if
     * the peer wants.
     */
#if 0
    NAKCIVJ(CI_COMPRESSTYPE, neg_vj,
	    if (cilen == CILEN_VJ) {
		GETCHAR(cimaxslotindex, p);
		GETCHAR(cicflag, p);
		if (cishort == IPCP_VJ_COMP) {
		    try.old_vj = 0;
		    if (cimaxslotindex < go->maxslotindex)
			try.maxslotindex = cimaxslotindex;
		    if (!cicflag)
			try.cflag = 0;
		} else {
		    try.neg_vj = 0;
		}
	    } else {
		if (cishort == IPCP_VJ_COMP || cishort == IPCP_VJ_COMP_OLD) {
		    try.old_vj = 1;
		    try.vj_protocol = cishort;
		} else {
		    try.neg_vj = 0;
		}
	    }
	    );
#endif

    NAKCIADDR(CI_ADDR, neg_addr,
	      if (go->accept_local && ciaddr1) { /* Do we know our address? */
		  try.ouraddr = ciaddr1;
	      }
	      );

    NAKCIDNS(CI_MS_DNS1, req_dns1,
	    try.dnsaddr[0] = cidnsaddr;
	    );

    NAKCIDNS(CI_MS_DNS2, req_dns2,
	    try.dnsaddr[1] = cidnsaddr;
	    );

    /*
     * There may be remaining CIs, if the peer is requesting negotiation
     * on an option that we didn't include in our request packet.
     * If they want to negotiate about IP addresses, we comply.
     * If they want us to ask for compression, we refuse.
     */
    while (len > CILEN_VOID) {
	GETCHAR(citype, p);
	GETCHAR(cilen, p);
	if( (len -= cilen) < 0 )
	    goto bad;
	next = p + cilen - 2;

	switch (citype) {
	case CI_COMPRESSTYPE:
	    if (go->neg_rohc || no.neg_rohc || cilen < 10)
		goto bad;
	    no.neg_rohc = 1;
#if 0
	    if (go->neg_vj || no.neg_vj ||
		(cilen != CILEN_VJ && cilen != CILEN_COMPRESS))
		goto bad;
	    no.neg_vj = 1;
#endif
	    break;
	case CI_ADDRS:
	    if ((!go->neg_addr && go->old_addrs) || no.old_addrs
		|| cilen != CILEN_ADDRS)
		goto bad;
	    try.neg_addr = 0;
	    GETLONG(l, p);
	    ciaddr1 = htonl(l);
	    if (ciaddr1 && go->accept_local)
		try.ouraddr = ciaddr1;
	    GETLONG(l, p);
	    ciaddr2 = htonl(l);
	    if (ciaddr2 && go->accept_remote)
		try.hisaddr = ciaddr2;
	    no.old_addrs = 1;
	    break;
	case CI_ADDR:
	    if (go->neg_addr || no.neg_addr || cilen != CILEN_ADDR)
		goto bad;
	    try.old_addrs = 0;
	    GETLONG(l, p);
	    ciaddr1 = htonl(l);
	    if (ciaddr1 && go->accept_local)
		try.ouraddr = ciaddr1;
	    if (try.ouraddr != 0)
		try.neg_addr = 1;
	    no.neg_addr = 1;
	    break;
	}
	p = next;
    }

    /*
     * OK, the Nak is good.  Now we can update state.
     * If there are any remaining options, we ignore them.
     */
    if (f->state != OPENED)
	*go = try;

    return 1;

bad:
    IPCPDEBUG(("ipcp_nakci: received bad Nak!"));
    return 0;
}


/*
 * ipcp_rejci - Reject some of our CIs.
 * Callback from fsm_rconfnakrej.
 */
static int
ipcp_rejci(f, p, len)
    fsm *f;
    u_char *p;
    int len;
{
    int rohc_pro_len, rohc_len;
    ipcp_options *go = &ipcp_gotoptions[f->unit];
    u_char /*cimaxslotindex, ciflag, */ cilen;
    u_short cishort;
    u_int32_t cilong;
    ipcp_options try;		/* options to request next time */

    try = *go;
    /*
     * Any Rejected CIs must be in exactly the same order that we sent.
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */
#define REJCIADDRS(opt, neg, val1, val2) \
    if ((neg) && \
	(cilen = p[1]) == CILEN_ADDRS && \
	len >= cilen && \
	p[0] == opt) { \
	u_int32_t l; \
	len -= cilen; \
	INCPTR(2, p); \
	GETLONG(l, p); \
	cilong = htonl(l); \
	/* Check rejected value. */ \
	if (cilong != val1) \
	    goto bad; \
	GETLONG(l, p); \
	cilong = htonl(l); \
	/* Check rejected value. */ \
	if (cilong != val2) \
	    goto bad; \
	try.old_addrs = 0; \
    }

#define REJCIVJ(opt, neg, val, old, maxslot, cflag) \
    if (go->neg && \
	p[1] == (old? CILEN_COMPRESS : CILEN_VJ) && \
	len >= p[1] && \
	p[0] == opt) { \
	len -= p[1]; \
	INCPTR(2, p); \
	GETSHORT(cishort, p); \
	/* Check rejected value. */  \
	if (cishort != val) \
	    goto bad; \
	if (!old) { \
	   GETCHAR(cimaxslotindex, p); \
	   if (cimaxslotindex != maxslot) \
	     goto bad; \
	   GETCHAR(ciflag, p); \
	   if (ciflag != cflag) \
	     goto bad; \
        } \
	try.neg = 0; \
     }

#define REJCIROHC(opt, neg, max_cid, mrru, max_header, profile_count, profiles) \
	rohc_pro_len = profile_count * 2 + 2; \
	rohc_len = rohc_pro_len + 10; \
	if (go->neg && \
		p[1] >= 10 && /*== rohc_len && */ \
		len >= p[1] && \
		p[0] == opt) { \
		RLOG("Rohc reject begin"); \
		u_char cichar; \
		int i; \
		len -= p[1]; \
		INCPTR(2, p); \
		SHORTTEST(CI_ROHC_PROTOCOL); \
		SHORTTEST(max_cid); \
		SHORTTEST(mrru); \
		SHORTTEST(max_header); \
		CHARTEST(ROHC_SUBOPT_PROFILE); \
		CHARTEST(rohc_pro_len); \
		for (i = 0; i < profile_count; i++) { \
			SHORTTEST(profiles[i]); \
		} \
		try.neg = 0; \
		RLOG("Rohc reject"); \
	}


#define REJCIADDR(opt, neg, val) \
    if (go->neg && \
	(cilen = p[1]) == CILEN_ADDR && \
	len >= cilen && \
	p[0] == opt) { \
	u_int32_t l; \
	len -= cilen; \
	INCPTR(2, p); \
	GETLONG(l, p); \
	cilong = htonl(l); \
	/* Check rejected value. */ \
	if (cilong != val) \
	    goto bad; \
	try.neg = 0; \
    }

#define REJCIDNS(opt, neg, dnsaddr) \
    if (go->neg && \
	((cilen = p[1]) == CILEN_ADDR) && \
	len >= cilen && \
	p[0] == opt) { \
	u_int32_t l; \
	len -= cilen; \
	INCPTR(2, p); \
	GETLONG(l, p); \
	cilong = htonl(l); \
	/* Check rejected value. */ \
	if (cilong != dnsaddr) \
	    goto bad; \
	try.neg = 0; \
    }


    REJCIADDRS(CI_ADDRS, !go->neg_addr && go->old_addrs,
	       go->ouraddr, go->hisaddr);

    RLOG("Reject function");
	sprintf(log_buf, "p[0] = %d p[1] = %d %d - neg %d len %d",
	    p[0], p[1], CI_COMPRESSTYPE, go->neg_rohc, len);
	RLOG(log_buf);
    REJCIROHC(CI_COMPRESSTYPE, neg_rohc, go->max_cid, go->mrru,
    	go->max_header, go->profile_count, go->profiles);


	sprintf(log_buf, "rohc len: %d %x - %d %x",
	    rohc_len, rohc_len, rohc_pro_len, rohc_pro_len);
	RLOG(log_buf);
#if 0  // ROHC-VJ
    REJCIVJ(CI_COMPRESSTYPE, neg_vj, go->vj_protocol, go->old_vj,
	    go->maxslotindex, go->cflag);
#endif

    REJCIADDR(CI_ADDR, neg_addr, go->ouraddr);

    REJCIDNS(CI_MS_DNS1, req_dns1, go->dnsaddr[0]);

    REJCIDNS(CI_MS_DNS2, req_dns2, go->dnsaddr[1]);

    /*
     * If there are any remaining CIs, then this packet is bad.
     */
    if (len != 0)
	goto bad;
    /*
     * Now we can update state.
     */
    if (f->state != OPENED)
	*go = try;
    return 1;

bad:
    IPCPDEBUG(("ipcp_rejci: received bad Reject!"));
    return 0;
}


/*
 * ipcp_reqci - Check the peer's requested CIs and send appropriate response.
 * Callback from fsm_rconfreq, Receive Configure Request
 *
 * Returns: CONFACK, CONFNAK or CONFREJ and input packet modified
 * appropriately.  If reject_if_disagree is non-zero, doesn't return
 * CONFNAK; returns CONFREJ if it can't return CONFACK.
 */
static int
ipcp_reqci(f, inp, len, reject_if_disagree)
    fsm *f;
    u_char *inp;		/* Requested CIs */
    int *len;			/* Length of requested CIs */
    int reject_if_disagree;
{
    ipcp_options *wo = &ipcp_wantoptions[f->unit];
    ipcp_options *ho = &ipcp_hisoptions[f->unit];
    ipcp_options *ao = &ipcp_allowoptions[f->unit];
    u_char *cip, *next;		/* Pointer to current and next CIs */
    u_short cilen, citype;	/* Parsed len, type */
    u_short cishort;		/* Parsed short value */
    u_int32_t tl, ciaddr1, ciaddr2;/* Parsed address values */
    int rc = CONFACK;		/* Final packet return code */
    int orc;			/* Individual option return code */
    u_char *p;			/* Pointer to next char to parse */
    u_char *ucp = inp;		/* Pointer to current output char */
    int l = *len;		/* Length left */
#if 0
    u_char maxslotindex, cflag;
#endif
    int d;
    int rolen;			/* total length of all rohc suboptions */
    int rhaveprofile;		/* have suboption rohc profiles */
    int rtype;			/* suboption type */
    int rlen;			/* suboption length */
    int i;			/* counter */
    int r_cid;			/* othersides max cid */
    int r_mrru;			/* othersides max mrru */
    int r_head;			/* othersides max header */
    int rebuild_package;	/* error in package, must rebuild */
    u_int16_t profiles[ROHC_MAX_PROFILES]; /* valid profiles */
    int profile_count;		/* valid profile count */

    /*
     * Reset all his options.
     */
    BZERO(ho, sizeof(*ho));

    /*
     * Process all his options.
     */
    next = inp;
    while (l) {
	orc = CONFACK;			/* Assume success */
	cip = p = next;			/* Remember begining of CI */
	if (l < 2 ||			/* Not enough data for CI header or */
	    p[1] < 2 ||			/*  CI length too small or */
	    p[1] > l) {			/*  CI length too big? */
	    IPCPDEBUG(("ipcp_reqci: bad CI length!"));
	    orc = CONFREJ;		/* Reject bad CI */
	    cilen = l;			/* Reject till end of packet */
	    l = 0;			/* Don't loop again */
	    goto endswitch;
	}
	GETCHAR(citype, p);		/* Parse CI type */
	GETCHAR(cilen, p);		/* Parse CI length */
	l -= cilen;			/* Adjust remaining length */
	next += cilen;			/* Step to next CI */

	switch (citype) {		/* Check CI type */
	case CI_ADDRS:
	    if (!ao->old_addrs || ho->neg_addr ||
		cilen != CILEN_ADDRS) {	/* Check CI length */
		orc = CONFREJ;		/* Reject CI */
		break;
	    }

	    /*
	     * If he has no address, or if we both have his address but
	     * disagree about it, then NAK it with our idea.
	     * In particular, if we don't know his address, but he does,
	     * then accept it.
	     */
	    GETLONG(tl, p);		/* Parse source address (his) */
	    ciaddr1 = htonl(tl);
	    if (ciaddr1 != wo->hisaddr
		&& (ciaddr1 == 0 || !wo->accept_remote)) {
		orc = CONFNAK;
		if (!reject_if_disagree) {
		    DECPTR(sizeof(u_int32_t), p);
		    tl = ntohl(wo->hisaddr);
		    PUTLONG(tl, p);
		}
	    } else if (ciaddr1 == 0 && wo->hisaddr == 0) {
		/*
		 * If neither we nor he knows his address, reject the option.
		 */
		orc = CONFREJ;
		wo->req_addr = 0;	/* don't NAK with 0.0.0.0 later */
		break;
	    }

	    /*
	     * If he doesn't know our address, or if we both have our address
	     * but disagree about it, then NAK it with our idea.
	     */
	    GETLONG(tl, p);		/* Parse desination address (ours) */
	    ciaddr2 = htonl(tl);
	    if (ciaddr2 != wo->ouraddr) {
		if (ciaddr2 == 0 || !wo->accept_local) {
		    orc = CONFNAK;
		    if (!reject_if_disagree) {
			DECPTR(sizeof(u_int32_t), p);
			tl = ntohl(wo->ouraddr);
			PUTLONG(tl, p);
		    }
		} else {
		    wo->ouraddr = ciaddr2;	/* accept peer's idea */
		}
	    }

	    ho->old_addrs = 1;
	    ho->hisaddr = ciaddr1;
	    ho->ouraddr = ciaddr2;
	    break;

	case CI_ADDR:
	    if (!ao->neg_addr || ho->old_addrs ||
		cilen != CILEN_ADDR) {	/* Check CI length */
		orc = CONFREJ;		/* Reject CI */
		break;
	    }

	    /*
	     * If he has no address, or if we both have his address but
	     * disagree about it, then NAK it with our idea.
	     * In particular, if we don't know his address, but he does,
	     * then accept it.
	     */
	    GETLONG(tl, p);	/* Parse source address (his) */
	    ciaddr1 = htonl(tl);
	    if (ciaddr1 != wo->hisaddr
		&& (ciaddr1 == 0 || !wo->accept_remote)) {
		orc = CONFNAK;
		if (!reject_if_disagree) {
		    DECPTR(sizeof(u_int32_t), p);
		    tl = ntohl(wo->hisaddr);
		    PUTLONG(tl, p);
		}
	    } else if (ciaddr1 == 0 && wo->hisaddr == 0) {
		/*
		 * Don't ACK an address of 0.0.0.0 - reject it instead.
		 */
		orc = CONFREJ;
		wo->req_addr = 0;	/* don't NAK with 0.0.0.0 later */
		break;
	    }

	    ho->neg_addr = 1;
	    ho->hisaddr = ciaddr1;
	    break;

	case CI_MS_DNS1:
	case CI_MS_DNS2:
	    /* Microsoft primary or secondary DNS request */
	    d = citype == CI_MS_DNS2;

	    /* If we do not have a DNS address then we cannot send it */
	    if (ao->dnsaddr[d] == 0 ||
		cilen != CILEN_ADDR) {	/* Check CI length */
		orc = CONFREJ;		/* Reject CI */
		break;
	    }
	    GETLONG(tl, p);
	    if (htonl(tl) != ao->dnsaddr[d]) {
                DECPTR(sizeof(u_int32_t), p);
		tl = ntohl(ao->dnsaddr[d]);
		PUTLONG(tl, p);
		orc = CONFNAK;
            }
            break;

	case CI_MS_WINS1:
	case CI_MS_WINS2:
	    /* Microsoft primary or secondary WINS request */
	    d = citype == CI_MS_WINS2;

	    /* If we do not have a DNS address then we cannot send it */
	    if (ao->winsaddr[d] == 0 ||
		cilen != CILEN_ADDR) {	/* Check CI length */
		orc = CONFREJ;		/* Reject CI */
		break;
	    }
	    GETLONG(tl, p);
	    if (htonl(tl) != ao->winsaddr[d]) {
                DECPTR(sizeof(u_int32_t), p);
		tl = ntohl(ao->winsaddr[d]);
		PUTLONG(tl, p);
		orc = CONFNAK;
            }
            break;

	case CI_COMPRESSTYPE:
	    RLOG("Evaluating package");
	    if (!ao->neg_rohc || /* Allow rohc? */
	        cilen < 14) {
		orc = CONFREJ;
		break;
	    }
	    RLOG("EP: 1");

	    GETSHORT(cishort, p);

	    if (cishort != CI_ROHC_PROTOCOL || cilen < 14) {
		orc = CONFREJ;
		break;
	    }
	    RLOG("EP: 2");

	    GETSHORT(r_cid, p);
	    GETSHORT(r_mrru, p);
	    GETSHORT(r_head, p);

	    sprintf(log_buf, "EP: cid = %d mrru = %d head = %d",
		r_cid, r_mrru, r_head);
	    RLOG(log_buf);

	    /* Check suboptions */
	    rolen = cilen - 10;
	    rhaveprofile = 0;
	    rebuild_package = 0;
	    profile_count = 0;
	    while(rolen) {
		RLOG("EP: 3");
	    	if (rolen < 2) {
		    orc = CONFREJ;
		    break;
		}
		RLOG("EP: 4");
		GETCHAR(rtype, p);
		GETCHAR(rlen, p);

		if (rlen < 2 || rlen > rolen) {
		    sprintf(log_buf, "EP: Why? rlen = %d  rolen = %d",
			rlen, rolen);
		    RLOG(log_buf);
		    orc = CONFREJ;
		    break;
		}

		RLOG("EP: 5");
		switch(rtype) {
		case ROHC_SUBOPT_PROFILE:
		    RLOG("EP: 6");
		    if (rhaveprofile || (rlen % 2) > 0) {
		    	orc = CONFREJ;
			break;
		    }
		    RLOG("EP: 7");
		    for (i = 2; i < rlen; i += 2) {
			GETSHORT(cishort, p);
			if (rohc_find_profile(cishort,
			    ao->profiles, ao->profile_count)) {
			    /* valid profile */
			    profiles[profile_count] = cishort;
			    profile_count++;
			} else {
			    /* can't find profile */
			    RLOG("EP: AAAAAAAAAAAAH!");
			    rebuild_package = 1;
			}
		    }
		    RLOGP(profiles, profile_count);

		    rhaveprofile = 1;
		    break;
		default:
		    RLOG("EP: 8");
		    orc = CONFNAK;
		    rebuild_package = 1;
		    break;
		}
		rolen -= rlen;
	    }
	    RLOG("EP: 9");
	    if (!rhaveprofile || profile_count == 0) {
		orc = CONFREJ;
		break;
	    }

	    RLOG("EP: 10");
	    /* check values */
	    rebuild_package += r_cid > ao->max_cid ? 1 : 0;
	    rebuild_package += r_mrru > ao->mrru ? 1 : 0;
	    rebuild_package += r_head > ao->max_header ? 1 : 0;

	    if (rebuild_package && !reject_if_disagree) {
		RLOG("EP: 11 - Rebuild");
		/* Some error - rebuilding package */
		orc = CONFNAK;
		if (r_cid > ao->max_cid) r_cid = ao->max_cid;
		if (r_mrru > ao->mrru) r_mrru = ao->mrru;
		if (r_head > ao->max_header) r_head = ao->max_header;

		p = cip + 4;
		PUTSHORT(r_cid, p);
		PUTSHORT(r_mrru, p);
		PUTSHORT(r_head, p);
		PUTCHAR(ROHC_SUBOPT_PROFILE, p);
		PUTCHAR(12 + 2 * profile_count, p);
		for (i = 0; i < profile_count; i++) {
		    PUTSHORT(profiles[i], p);
		}
		cilen = p - cip;

	    } else if (!rebuild_package) {
    		RLOG("EP: 12 - OK");
		ho->neg_rohc = 1;
		ho->max_cid = r_cid;
		ho->mrru = r_mrru;
		ho->max_header = r_head;
		ho->profile_count = profile_count;
		memcpy(ho->profiles, profiles, profile_count * sizeof(u_int16_t));
	    }

	    RLOG("EP: 13 - end?");
	    break;

#if 0
	    if (!ao->neg_vj ||
		(cilen != CILEN_VJ && cilen != CILEN_COMPRESS)) {
		orc = CONFREJ;
		break;
	    }
	    GETSHORT(cishort, p);

	    if (!(cishort == IPCP_VJ_COMP ||
		  (cishort == IPCP_VJ_COMP_OLD && cilen == CILEN_COMPRESS))) {
		orc = CONFREJ;
		break;
	    }

	    ho->neg_vj = 1;
	    ho->vj_protocol = cishort;
	    if (cilen == CILEN_VJ) {
		GETCHAR(maxslotindex, p);
		if (maxslotindex > ao->maxslotindex) {
		    orc = CONFNAK;
		    if (!reject_if_disagree){
			DECPTR(1, p);
			PUTCHAR(ao->maxslotindex, p);
		    }
		}
		GETCHAR(cflag, p);
		if (cflag && !ao->cflag) {
		    orc = CONFNAK;
		    if (!reject_if_disagree){
			DECPTR(1, p);
			PUTCHAR(wo->cflag, p);
		    }
		}
		ho->maxslotindex = maxslotindex;
		ho->cflag = cflag;
	    } else {
		ho->old_vj = 1;
		ho->maxslotindex = MAX_STATES - 1;
		ho->cflag = 1;
	    }
	    break;
#endif
	default:
	    orc = CONFREJ;
	    break;
	}
endswitch:
	if (orc == CONFACK &&		/* Good CI */
	    rc != CONFACK)		/*  but prior CI wasnt? */
	    continue;			/* Don't send this one */

	if (orc == CONFNAK) {		/* Nak this CI? */
	    if (reject_if_disagree)	/* Getting fed up with sending NAKs? */
		orc = CONFREJ;		/* Get tough if so */
	    else {
		if (rc == CONFREJ)	/* Rejecting prior CI? */
		    continue;		/* Don't send this one */
		if (rc == CONFACK) {	/* Ack'd all prior CIs? */
		    rc = CONFNAK;	/* Not anymore... */
		    ucp = inp;		/* Backup */
		}
	    }
	}

	if (orc == CONFREJ &&		/* Reject this CI */
	    rc != CONFREJ) {		/*  but no prior ones? */
	    rc = CONFREJ;
	    ucp = inp;			/* Backup */
	}

	/* Need to move CI? */
	if (ucp != cip)
	    BCOPY(cip, ucp, cilen);	/* Move it */

	/* Update output pointer */
	INCPTR(cilen, ucp);
    }

    /*
     * If we aren't rejecting this packet, and we want to negotiate
     * their address, and they didn't send their address, then we
     * send a NAK with a CI_ADDR option appended.  We assume the
     * input buffer is long enough that we can append the extra
     * option safely.
     */
    if (rc != CONFREJ && !ho->neg_addr && !ho->old_addrs &&
	wo->req_addr && !reject_if_disagree) {
	if (rc == CONFACK) {
	    rc = CONFNAK;
	    ucp = inp;			/* reset pointer */
	    wo->req_addr = 0;		/* don't ask again */
	}
	PUTCHAR(CI_ADDR, ucp);
	PUTCHAR(CILEN_ADDR, ucp);
	tl = ntohl(wo->hisaddr);
	PUTLONG(tl, ucp);
    }

    *len = ucp - inp;			/* Compute output length */
    IPCPDEBUG(("ipcp: returning Configure-%s", CODENAME(rc)));
    return (rc);			/* Return final code */
}


/*
 * ip_check_options - check that any IP-related options are OK,
 * and assign appropriate defaults.
 */
static void
ip_check_options()
{
    struct hostent *hp;
    u_int32_t local;
    ipcp_options *wo = &ipcp_wantoptions[0];

    /*
     * Default our local IP address based on our hostname.
     * If local IP address already given, don't bother.
     */
    if (wo->ouraddr == 0 && !disable_defaultip) {
	/*
	 * Look up our hostname (possibly with domain name appended)
	 * and take the first IP address as our local IP address.
	 * If there isn't an IP address for our hostname, too bad.
	 */
	wo->accept_local = 1;	/* don't insist on this default value */
	if ((hp = gethostbyname(hostname)) != NULL) {
	    local = *(u_int32_t *)hp->h_addr;
	    if (local != 0 && !bad_ip_adrs(local))
		wo->ouraddr = local;
	}
    }
    ask_for_local = wo->ouraddr != 0 || !disable_defaultip;
}


/*
 * ip_demand_conf - configure the interface as though
 * IPCP were up, for use with dial-on-demand.
 */
static int
ip_demand_conf(u)
    int u;
{
    ipcp_options *wo = &ipcp_wantoptions[u];

    if (wo->hisaddr == 0) {
	/* make up an arbitrary address for the peer */
	wo->hisaddr = htonl(0x0a707070 + ifunit);
	wo->accept_remote = 1;
    }
    if (wo->ouraddr == 0) {
	/* make up an arbitrary address for us */
	wo->ouraddr = htonl(0x0a404040 + ifunit);
	wo->accept_local = 1;
	ask_for_local = 0;	/* don't tell the peer this address */
    }
    if (!sifaddr(u, wo->ouraddr, wo->hisaddr, GetMask(wo->ouraddr)))
	return 0;
    if (!sifup(u))
	return 0;
    if (!sifnpmode(u, PPP_IP, NPMODE_QUEUE))
	return 0;
    if (wo->default_route)
	if (sifdefaultroute(u, wo->ouraddr, wo->hisaddr))
	    default_route_set[u] = 1;
    if (wo->proxy_arp)
	if (sifproxyarp(u, wo->hisaddr))
	    proxy_arp_set[u] = 1;

    notice("local  IP address %I", wo->ouraddr);
    notice("remote IP address %I", wo->hisaddr);

    return 1;
}


/*
 * ipcp_up - IPCP has come UP.
 *
 * Configure the IP network interface appropriately and bring it up.
 */
static void
ipcp_up(f)
    fsm *f;
{
    u_int32_t mask;
    ipcp_options *ho = &ipcp_hisoptions[f->unit];
    ipcp_options *go = &ipcp_gotoptions[f->unit];
    ipcp_options *wo = &ipcp_wantoptions[f->unit];

    IPCPDEBUG(("ipcp: up"));

    /*
     * We must have a non-zero IP address for both ends of the link.
     */
    if (!ho->neg_addr && !ho->old_addrs)
	ho->hisaddr = wo->hisaddr;

    if (go->ouraddr == 0) {
	error("Could not determine local IP address");
	ipcp_close(f->unit, "Could not determine local IP address");
	return;
    }
    if (ho->hisaddr == 0) {
	ho->hisaddr = htonl(0x0a404040 + ifunit);
	warn("Could not determine remote IP address: defaulting to %I",
	     ho->hisaddr);
    }
    script_setenv("IPLOCAL", ip_ntoa(go->ouraddr), 0);
    script_setenv("IPREMOTE", ip_ntoa(ho->hisaddr), 1);

    if (usepeerdns && (go->dnsaddr[0] || go->dnsaddr[1])) {
	script_setenv("USEPEERDNS", "1", 0);
	if (go->dnsaddr[0])
	    script_setenv("DNS1", ip_ntoa(go->dnsaddr[0]), 0);
	if (go->dnsaddr[1])
	    script_setenv("DNS2", ip_ntoa(go->dnsaddr[1]), 0);
	create_resolv(go->dnsaddr[0], go->dnsaddr[1]);
    }

    /*
     * Check that the peer is allowed to use the IP address it wants.
     */
    if (!auth_ip_addr(f->unit, ho->hisaddr)) {
	error("Peer is not authorized to use remote address %I", ho->hisaddr);
	ipcp_close(f->unit, "Unauthorized remote IP address");
	return;
    }

    /* set tcp compression */
#if 0
    sifvjcomp(f->unit, ho->neg_vj, ho->cflag, ho->maxslotindex);
#endif
    sprintf(log_buf, "neg: %d  cid: %d  mrru:%d  head: %d",
	ho->neg_rohc, ho->max_cid, ho->mrru,
	ho->max_header);
    log(log_buf);
    RLOGP(ho->profiles, ho->profile_count);
    sifrohccomp(f->unit, ho->neg_rohc, ho->max_cid, ho->mrru,
    	ho->max_header, ho->profile_count, ho->profiles);

    /*
     * If we are doing dial-on-demand, the interface is already
     * configured, so we put out any saved-up packets, then set the
     * interface to pass IP packets.
     */
    if (demand) {
	if (go->ouraddr != wo->ouraddr || ho->hisaddr != wo->hisaddr) {
	    ipcp_clear_addrs(f->unit, wo->ouraddr, wo->hisaddr);
	    if (go->ouraddr != wo->ouraddr) {
		warn("Local IP address changed to %I", go->ouraddr);
		script_setenv("OLDIPLOCAL", ip_ntoa(wo->ouraddr), 0);
		wo->ouraddr = go->ouraddr;
	    } else
		script_unsetenv("OLDIPLOCAL");
	    if (ho->hisaddr != wo->hisaddr) {
		warn("Remote IP address changed to %I", ho->hisaddr);
		script_setenv("OLDIPREMOTE", ip_ntoa(wo->hisaddr), 0);
		wo->hisaddr = ho->hisaddr;
	    } else
		script_unsetenv("OLDIPREMOTE");

	    /* Set the interface to the new addresses */
	    mask = GetMask(go->ouraddr);
	    if (!sifaddr(f->unit, go->ouraddr, ho->hisaddr, mask)) {
		if (debug)
		    warn("Interface configuration failed");
		ipcp_close(f->unit, "Interface configuration failed");
		return;
	    }

	    /* assign a default route through the interface if required */
	    if (ipcp_wantoptions[f->unit].default_route) 
		if (sifdefaultroute(f->unit, go->ouraddr, ho->hisaddr))
		    default_route_set[f->unit] = 1;

	    /* Make a proxy ARP entry if requested. */
	    if (ipcp_wantoptions[f->unit].proxy_arp)
		if (sifproxyarp(f->unit, ho->hisaddr))
		    proxy_arp_set[f->unit] = 1;

	}
	demand_rexmit(PPP_IP);
	sifnpmode(f->unit, PPP_IP, NPMODE_PASS);

    } else {
	/*
	 * Set IP addresses and (if specified) netmask.
	 */
	mask = GetMask(go->ouraddr);

#if !(defined(SVR4) && (defined(SNI) || defined(__USLC__)))
	if (!sifaddr(f->unit, go->ouraddr, ho->hisaddr, mask)) {
	    if (debug)
		warn("Interface configuration failed");
	    ipcp_close(f->unit, "Interface configuration failed");
	    return;
	}
#endif

	/* bring the interface up for IP */
	if (!sifup(f->unit)) {
	    if (debug)
		warn("Interface failed to come up");
	    ipcp_close(f->unit, "Interface configuration failed");
	    return;
	}

#if (defined(SVR4) && (defined(SNI) || defined(__USLC__)))
	if (!sifaddr(f->unit, go->ouraddr, ho->hisaddr, mask)) {
	    if (debug)
		warn("Interface configuration failed");
	    ipcp_close(f->unit, "Interface configuration failed");
	    return;
	}
#endif
	sifnpmode(f->unit, PPP_IP, NPMODE_PASS);

	/* assign a default route through the interface if required */
	if (ipcp_wantoptions[f->unit].default_route)
	    if (sifdefaultroute(f->unit, go->ouraddr, ho->hisaddr))
		default_route_set[f->unit] = 1;

	/* Make a proxy ARP entry if requested. */
	if (ipcp_wantoptions[f->unit].proxy_arp)
	    if (sifproxyarp(f->unit, ho->hisaddr))
		proxy_arp_set[f->unit] = 1;

	ipcp_wantoptions[0].ouraddr = go->ouraddr;

	notice("local  IP address %I", go->ouraddr);
	notice("remote IP address %I", ho->hisaddr);
	if (go->dnsaddr[0])
	    notice("primary   DNS address %I", go->dnsaddr[0]);
	if (go->dnsaddr[1])
	    notice("secondary DNS address %I", go->dnsaddr[1]);
    }

    np_up(f->unit, PPP_IP);
    ipcp_is_up = 1;

    notify(ip_up_notifier, 0);
    if (ip_up_hook)
	ip_up_hook();

    /*
     * Execute the ip-up script, like this:
     *	/etc/ppp/ip-up interface tty speed local-IP remote-IP
     */
    if (ipcp_script_state == s_down && ipcp_script_pid == 0) {
	ipcp_script_state = s_up;
	ipcp_script(_PATH_IPUP);
    }
}


/*
 * ipcp_down - IPCP has gone DOWN.
 *
 * Take the IP network interface down, clear its addresses
 * and delete routes through it.
 */
static void
ipcp_down(f)
    fsm *f;
{
    IPCPDEBUG(("ipcp: down"));
    /* XXX a bit IPv4-centric here, we only need to get the stats
     * before the interface is marked down. */
    update_link_stats(f->unit);
    notify(ip_down_notifier, 0);
    if (ip_down_hook)
	ip_down_hook();
    if (ipcp_is_up) {
	ipcp_is_up = 0;
	np_down(f->unit, PPP_IP);
    }
    sifvjcomp(f->unit, 0, 0, 0);

    /*
     * If we are doing dial-on-demand, set the interface
     * to queue up outgoing packets (for now).
     */
    if (demand) {
	sifnpmode(f->unit, PPP_IP, NPMODE_QUEUE);
    } else {
	sifnpmode(f->unit, PPP_IP, NPMODE_DROP);
	sifdown(f->unit);
	ipcp_clear_addrs(f->unit, ipcp_gotoptions[f->unit].ouraddr,
			 ipcp_hisoptions[f->unit].hisaddr);
    }

    /* Execute the ip-down script */
    if (ipcp_script_state == s_up && ipcp_script_pid == 0) {
	ipcp_script_state = s_down;
	ipcp_script(_PATH_IPDOWN);
    }
}


/*
 * ipcp_clear_addrs() - clear the interface addresses, routes,
 * proxy arp entries, etc.
 */
static void
ipcp_clear_addrs(unit, ouraddr, hisaddr)
    int unit;
    u_int32_t ouraddr;  /* local address */
    u_int32_t hisaddr;  /* remote address */
{
    if (proxy_arp_set[unit]) {
	cifproxyarp(unit, hisaddr);
	proxy_arp_set[unit] = 0;
    }
    if (default_route_set[unit]) {
	cifdefaultroute(unit, ouraddr, hisaddr);
	default_route_set[unit] = 0;
    }
    cifaddr(unit, ouraddr, hisaddr);
}


/*
 * ipcp_finished - possibly shut down the lower layers.
 */
static void
ipcp_finished(f)
    fsm *f;
{
    np_finished(f->unit, PPP_IP);
}


/*
 * ipcp_script_done - called when the ip-up or ip-down script
 * has finished.
 */
static void
ipcp_script_done(arg)
    void *arg;
{
    ipcp_script_pid = 0;
    switch (ipcp_script_state) {
    case s_up:
	if (ipcp_fsm[0].state != OPENED) {
	    ipcp_script_state = s_down;
	    ipcp_script(_PATH_IPDOWN);
	}
	break;
    case s_down:
	if (ipcp_fsm[0].state == OPENED) {
	    ipcp_script_state = s_up;
	    ipcp_script(_PATH_IPUP);
	}
	break;
    }
}


/*
 * ipcp_script - Execute a script with arguments
 * interface-name tty-name speed local-IP remote-IP.
 */
static void
ipcp_script(script)
    char *script;
{
    char strspeed[32], strlocal[32], strremote[32];
    char *argv[8];

    slprintf(strspeed, sizeof(strspeed), "%d", baud_rate);
    slprintf(strlocal, sizeof(strlocal), "%I", ipcp_gotoptions[0].ouraddr);
    slprintf(strremote, sizeof(strremote), "%I", ipcp_hisoptions[0].hisaddr);

    argv[0] = script;
    argv[1] = ifname;
    argv[2] = devnam;
    argv[3] = strspeed;
    argv[4] = strlocal;
    argv[5] = strremote;
    argv[6] = ipparam;
    argv[7] = NULL;
    ipcp_script_pid = run_program(script, argv, 0, ipcp_script_done, NULL);
}

/*
 * create_resolv - create the replacement resolv.conf file
 */
static void
create_resolv(peerdns1, peerdns2)
    u_int32_t peerdns1, peerdns2;
{
    FILE *f;

    f = fopen(_PATH_RESOLV, "w");
    if (f == NULL) {
	error("Failed to create %s: %m", _PATH_RESOLV);
	return;
    }

    if (peerdns1)
	fprintf(f, "nameserver %s\n", ip_ntoa(peerdns1));

    if (peerdns2)
	fprintf(f, "nameserver %s\n", ip_ntoa(peerdns2));

    if (ferror(f))
	error("Write failed to %s: %m", _PATH_RESOLV);

    fclose(f);
}

/*
 * ipcp_printpkt - print the contents of an IPCP packet.
 */
static char *ipcp_codenames[] = {
    "ConfReq", "ConfAck", "ConfNak", "ConfRej",
    "TermReq", "TermAck", "CodeRej"
};

static int
ipcp_printpkt(p, plen, printer, arg)
    u_char *p;
    int plen;
    void (*printer) __P((void *, char *, ...));
    void *arg;
{
    int code, id, len, olen;
    u_char *pstart, *optend;
    u_short cishort;
    u_int32_t cilong;

    if (plen < HEADERLEN)
	return 0;
    pstart = p;
    GETCHAR(code, p);
    GETCHAR(id, p);
    GETSHORT(len, p);
    if (len < HEADERLEN || len > plen)
	return 0;

    if (code >= 1 && code <= sizeof(ipcp_codenames) / sizeof(char *))
	printer(arg, " %s", ipcp_codenames[code-1]);
    else
	printer(arg, " code=0x%x", code);
    printer(arg, " id=0x%x", id);
    len -= HEADERLEN;
    switch (code) {
    case CONFREQ:
    case CONFACK:
    case CONFNAK:
    case CONFREJ:
	/* print option list */
	while (len >= 2) {
	    GETCHAR(code, p);
	    GETCHAR(olen, p);
	    p -= 2;
	    if (olen < 2 || olen > len) {
		break;
	    }
	    printer(arg, " <");
	    len -= olen;
	    optend = p + olen;
	    switch (code) {
	    case CI_ADDRS:
		if (olen == CILEN_ADDRS) {
		    p += 2;
		    GETLONG(cilong, p);
		    printer(arg, "addrs %I", htonl(cilong));
		    GETLONG(cilong, p);
		    printer(arg, " %I", htonl(cilong));
		}
		break;
	    case CI_COMPRESSTYPE:
		if (olen >= CILEN_COMPRESS) {
		    p += 2;
		    GETSHORT(cishort, p);
		    printer(arg, "compress ");
		    switch (cishort) {
		    case IPCP_VJ_COMP:
			printer(arg, "VJ");
			break;
		    case IPCP_VJ_COMP_OLD:
			printer(arg, "old-VJ");
			break;
		    case CI_ROHC_PROTOCOL:
		        printer(arg, "ROHC");
			break;
		    default:
			printer(arg, "0x%x", cishort);
		    }
		}
		break;
	    case CI_ADDR:
		if (olen == CILEN_ADDR) {
		    p += 2;
		    GETLONG(cilong, p);
		    printer(arg, "addr %I", htonl(cilong));
		}
		break;
	    case CI_MS_DNS1:
	    case CI_MS_DNS2:
	        p += 2;
		GETLONG(cilong, p);
		printer(arg, "ms-dns%d %I", code - CI_MS_DNS1 + 1,
			htonl(cilong));
		break;
	    case CI_MS_WINS1:
	    case CI_MS_WINS2:
	        p += 2;
		GETLONG(cilong, p);
		printer(arg, "ms-wins %I", htonl(cilong));
		break;
	    }
	    while (p < optend) {
		GETCHAR(code, p);
		printer(arg, " %.2x", code);
	    }
	    printer(arg, ">");
	}
	break;

    case TERMACK:
    case TERMREQ:
	if (len > 0 && *p >= ' ' && *p < 0x7f) {
	    printer(arg, " ");
	    print_string((char *)p, len, printer, arg);
	    p += len;
	    len = 0;
	}
	break;
    }

    /* print the rest of the bytes in the packet */
    for (; len > 0; --len) {
	GETCHAR(code, p);
	printer(arg, " %.2x", code);
    }

    return p - pstart;
}

/*
 * ip_active_pkt - see if this IP packet is worth bringing the link up for.
 * We don't bring the link up for IP fragments or for TCP FIN packets
 * with no data.
 */
#define IP_HDRLEN	20	/* bytes */
#define IP_OFFMASK	0x1fff
#ifndef IPPROTO_TCP
#define IPPROTO_TCP	6
#endif
#define TCP_HDRLEN	20
#define TH_FIN		0x01

/*
 * We use these macros because the IP header may be at an odd address,
 * and some compilers might use word loads to get th_off or ip_hl.
 */

#define net_short(x)	(((x)[0] << 8) + (x)[1])
#define get_iphl(x)	(((unsigned char *)(x))[0] & 0xF)
#define get_ipoff(x)	net_short((unsigned char *)(x) + 6)
#define get_ipproto(x)	(((unsigned char *)(x))[9])
#define get_tcpoff(x)	(((unsigned char *)(x))[12] >> 4)
#define get_tcpflags(x)	(((unsigned char *)(x))[13])

static int
ip_active_pkt(pkt, len)
    u_char *pkt;
    int len;
{
    u_char *tcp;
    int hlen;

    len -= PPP_HDRLEN;
    pkt += PPP_HDRLEN;
    if (len < IP_HDRLEN)
	return 0;
    if ((get_ipoff(pkt) & IP_OFFMASK) != 0)
	return 0;
    if (get_ipproto(pkt) != IPPROTO_TCP)
	return 1;
    hlen = get_iphl(pkt) * 4;
    if (len < hlen + TCP_HDRLEN)
	return 0;
    tcp = pkt + hlen;
    if ((get_tcpflags(tcp) & TH_FIN) != 0 && len == hlen + get_tcpoff(tcp) * 4)
	return 0;
    return 1;
}

/*** Extra ROHC functions ***/

#define ROHC_PROC_FILE "/proc/rohc/rohc_device1"
#define ROHC_PROC_PROFILE_ID "Profiles:"
#define is_id(x) strncmp(x, ROHC_PROC_PROFILE_ID, sizeof(ROHC_PROC_PROFILE_ID) - 1)

static int get_word(char ** input, char * buf, int max, char split)
{
	while(max) {
		if (**input == 0) {
			*buf = 0;
			return 0;
		}
		if (**input == split) {
			*buf = 0;
			*input += 1;
			return 0;
		}
		*buf = **input;
		buf++; *input += 1;
		max--;
	}
	return 1;
}

static int is_only_numbers(char * input)
{
	while(*input) {
		if (*input < '0' || *input > '9') return 0;
		input++;
	}
	return 1;
}

int rohc_parse_num_array(char * s, u_int16_t * array, int size)
{
	char text[10];
	int count = 0, msg = -3, value;

	printf("input = \"%s\"\n", s);

	while(*s) {
		msg = -1;
		if (get_word(&s, text, 10, ' ')) break;
		if (!is_only_numbers(text)) break;
		if (!int_option(text, &value)) break;
		if (count == size) {
			msg = -2;
			break;
		}
		array[count] = value;
		count++;

		msg = 0;
	}
	return msg < 0 ? msg : count;
}

/* To readline() */
struct input
{
	FILE * f;
	int size;
	int read;
	char buf[1024];
};

/*
 * read from a textfile, line by line.
 */
static int readline(struct input * in, char * dest, int max)
{
	int s; char * p;
	if (in->read) {
		memcpy(in->buf, &in->buf[in->read], in->size - in->read);
		in->size -= in->read;
		in->read = 0;
	}
	if (in->size < 512 && in->f) {
		s = fread(&in->buf[in->size], 1, 512, in->f);
		in->size += s;
		if (s != 512) {
			fclose(in->f);
			in->f = 0;
		}
	}
	p = in->buf;
	while(in->read < in->size) {
		in->read++;
		if (*p == '\n') {
			*dest = 0;
			return 1;
		}
		*dest = *p;
		dest++; p++;
	}
	*dest = 0;
	return 0;
}

/*
 * Get all the rohc profiles
 */
static int rohc_get_profiles(u_int16_t * array, int array_size)
{
	int ret;
	char dest[256];
	struct input in = {0, 0};
	in.f = fopen(ROHC_PROC_FILE, "rb");

	if (in.f == NULL) {
		printf("no file\n");
		return -1;
	}

	while(readline(&in, dest, 256)) {
		printf("read: \"%s\"\n", dest);
		if (is_id(dest) == 0) {
			ret = rohc_parse_num_array(
				dest + sizeof(ROHC_PROC_PROFILE_ID) - 1,
				array, array_size);
			printf("ret = %d\n", ret);
			if (ret < -1) ret = -1;
			return ret;
		}
	}

	return -1;
}

/*
 * Find a ROHC profile ID in a array. Return non-zero if found
 */
static int rohc_find_profile(u_int16_t profile, u_int16_t * array, int array_size)
{
	int i;
	sprintf(log_buf, "RFP: profile %d", profile);
	RLOG(log_buf);
	RLOGP(array, array_size);
	for (i = 0; i < array_size; i++) {
		if (array[i] == profile) {
			return 1;
		}
	}
	return 0;
}

/*
 * Parse the profile input parameter
 */
static int set_rohc_profiles(char **argv)
{
	char err[250], text[10], * s;
	u_int16_t profiles[ROHC_MAX_PROFILES];
	int profile_count;
	int value, i = 0, msg = 0, ret = 0, size, len;

	profile_count = rohc_get_profiles(profiles, ROHC_MAX_PROFILES);

	if (strcmp(*argv, "all") == 0) {
		i = sizeof(u_int16_t) * profile_count;
		memcpy(ipcp_wantoptions[0].profiles, profiles, i);
		memcpy(ipcp_allowoptions[0].profiles, profiles, i);
		ipcp_wantoptions[0].profile_count = profile_count;
		ipcp_wantoptions[0].profile_count = profile_count;
		slprintf(rohc_value, sizeof(rohc_value), "all");
		return 1;
	}

	s = *argv;

	ipcp_wantoptions[0].profile_count = 0;
	ipcp_allowoptions[0].profiles[i] = 0;
	while(!*s) {
		msg = 0;
		if (get_word(&s, text, 10, ',')) break;
		if (!int_option(*argv, &value)) break;
		if (profile_count == ROHC_MAX_PROFILES) {
			msg = 1;
			break;
		}
		if (!rohc_find_profile(value, profiles, profile_count)) {
			sprintf(err, "rohc-profiles: invalid profile %d", value);
			msg = 2;
			break;
		}
		i = ipcp_wantoptions[0].profile_count;
		ipcp_wantoptions[0].profiles[i] = value;
		ipcp_wantoptions[0].profile_count += 1;

		ipcp_allowoptions[0].profiles[i] = value;
		ipcp_allowoptions[0].profiles[i] += 1;

		msg = -1;
	}

	switch(msg) {
	case 0:
		option_error("rohc-profiles: invalid input");
		break;
	case 1:
		option_error("rohc-profiles: too many profiles");
		break;
	case 2:
		option_error(err);
		break;
	default: /* OK */
		size = sizeof(rohc_value);
		for (i = 0; i < ipcp_wantoptions[0].profile_count; i++) {
			slprintf(s, size, (i ? ",%d" : "%d"),
				ipcp_wantoptions[0].profiles[i]);

			len = strlen(s);
			s += len;
			size -= len;
		}
		ret = 1;
	}
	return ret;
}

static void log(char * message)
{
	FILE * f = fopen("/tmp/ppp-message", "at");
	if (f == NULL) return;
	fprintf(f, "%s\n", message);
	fclose(f);
}
