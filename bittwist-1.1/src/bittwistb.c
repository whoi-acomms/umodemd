/*
 * bittwistb - pcap based ethernet bridge
 * Copyright (C) 2006 - 2011 Addy Yeow Chin Heng <ayeowch@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "bittwistb.h"

char *program_name;

char ebuf[PCAP_ERRBUF_SIZE]; /* pcap error buffer */

int vflag = 0; /* 1 - print source and destination MAC for forwarded packets */

pcap_t *pd[PORT_MAX]; /* array of pcap descriptors for all interfaces */
int pd_count = 0; /* number of pcap descriptors created */

struct bridge_ht bridge_ht[HASH_SIZE]; /* hash table to store bridge_ht structures */
struct ether_addr *bridge_addr[PORT_MAX]; /* storage for local MAC */

/* Ethernet broadcast MAC address */
const u_char ether_bcast[ETHER_ADDR_LEN] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* stats */
static u_int pkts_sent = 0;
static u_int bytes_sent = 0;
static u_int failed = 0;
struct timeval start = {0,0};
struct timeval end = {0,0};

int main(int argc, char **argv)
{
    char *cp;
    int c;
    pcap_if_t *devptr;
    int i, j;
    char *devices = NULL;

    if ((cp = strrchr(argv[0], '/')) != NULL)
        program_name = cp + 1;
    else
        program_name = argv[0];

    /* process options */
    while ((c = getopt(argc, argv, "di:vh")) != -1) {
        switch (c) {
            case 'd':
                if (pcap_findalldevs(&devptr, ebuf) < 0)
                    error("%s", ebuf);
                else {
                    for (i = 0; devptr != 0; i++) {
                        (void)printf("%d. %s", i + 1, devptr->name);
                        if (devptr->description != NULL)
                            (void)printf(" (%s)", devptr->description);
                        (void)putchar('\n');
                        devptr = devptr->next;
                    }
                }
                exit(EXIT_SUCCESS);
            case 'i':
                devices = optarg;
            break;
            case 'v':
                ++vflag;
                break;
            case 'h':
            default:
                usage();
        }
    }

    if (devices == NULL)
        error("interfaces not specified");

    cp = (char *)strtok(devices, ",");
    i = 0;
    while (cp != NULL) {
        if (i >= PORT_MAX)
            error("invalid interfaces specification");

        /* empty error buffer to grab warning message (if exist) from pcap_open_live() below */
        *ebuf = '\0';

        /* create a pcap descriptor for each interface to capture packets */
        pd[i] = pcap_open_live(cp,
                               ETHER_MAX_LEN, /* portion of packet to capture */
                               1,             /* promiscuous mode is on */
                               1,             /* read timeout, in milliseconds */
                               ebuf);
        if (pd[i] == NULL)
            error("%s", ebuf);
        else if (*ebuf)
            notice("%s", ebuf); /* warning message from pcap_open_live() above */

        bridge_addr[i] = (struct ether_addr *)malloc(sizeof(struct ether_addr));
        if (bridge_addr[i] == NULL)
            error("malloc(): cannot allocate memory for bridge_addr[%d]", i);

        /* copy MAC address for cp into bridge_addr[i] */
        if (gethwaddr(bridge_addr[i], cp) == -1)
            error("gethwaddr(): cannot locate hardware address for %s", cp);

        if (vflag > 0) {
            (void)printf("%s=", cp);
            for (j = 0; j < ETHER_ADDR_LEN; j++) {
                (void)printf("%02x", bridge_addr[i]->octet[j]);
                j == (ETHER_ADDR_LEN - 1) ? (void)putchar(',') : (void)putchar(':');
            }
            (void)printf("port=%d\n", i + 1);
        }

        cp = (char *)strtok(NULL, ",");
        ++i;
    }
    if (i < PORT_MIN)
        error("invalid interfaces specification");
    pd_count = i; /* number of pcap descriptors that we have actually created */

    /* set signal handler for SIGINT (Control-C) */
    (void)signal(SIGINT, cleanup);

    if (gettimeofday(&start, NULL) == -1)
        notice("gettimeofday(): %s", strerror(errno));

    bridge_on(); /* run bridge */

    exit(EXIT_SUCCESS);
}

void bridge_on(void)
{
    /*
     * struct pollfd {
     *     int    fd;      -> file descriptor
     *     short  events;  -> events to look for
     *     short  revents; -> events returned
     * };
     */
    struct pollfd pcap_poll[pd_count]; /* to poll initialized pd */
    int poll_ret = 0; /* return code for poll() */
    int ret;
    int i;
    sigset_t block_sig;

    (void)sigemptyset(&block_sig);
    (void)sigaddset(&block_sig, SIGALRM);

    /*
     * we are interested only on these event bitmasks:
     * POLLIN = Data other than high priority data may be read without blocking
     * POLLPRI = High priority data may be read without blocking
     */
    for (i = 0; i < pd_count; i++) {
        pcap_poll[i].fd = pcap_fileno(pd[i]);
        pcap_poll[i].events = POLLIN | POLLPRI;
        pcap_poll[i].revents = 0;
    }

    /*
     * set signal handler for SIGALRM, generated by setitimer() in hash_alarm(),
     * to remove old hash entries from the hash table
     */
    (void)signal(SIGALRM, hash_alarm_handler);

    /* set alarm for the first time */
    if (hash_alarm(HASH_ALARM) == -1)
        error("setitimer(): %s", strerror(errno));

    while (1) { /* run infinitely until user Control-C */
        /* we do not want to interrupt the polling -> hold SIGALRM */
        (void)sigprocmask(SIG_BLOCK, &block_sig, NULL);

        /* poll for a packet on all the initialized pd */
        poll_ret = poll(pcap_poll,  /* array of pollfd structures */
                        pd_count,   /* size of pcap_poll array */
                        1);         /* timeout, in milliseconds */

        /* release SIGALRM */
        (void)sigprocmask(SIG_UNBLOCK, &block_sig, NULL);

        /* we have got packet(s) from one or more of the initialized pd */
        if (poll_ret > 0) {
            for (i = 0; i < pd_count; i++) {
                if (pcap_poll[i].revents > 0) {
                    ret = pcap_dispatch(pd[i],                    /* pcap descriptor */
                                        -1,                       /* -1 -> process all packets */
                                        (pcap_handler)bridge_fwd, /* callback function */
                                        (u_char *)(i + 1));       /* LAN segment or port (first port is 1 not 0) */
                    /* ret = number of packets read */
                    if (ret == 0)
                        /* we came in from poll(), timeout is unlikely */
                        notice("pcap_dispatch(): read timeout");
                    else if (ret == -1)
                        notice("%s", pcap_geterr(pd[i]));
                }
            }
        }
        else if (poll_ret == 0) {
            /* we do not want to print this during normal use! */
            /* notice("poll(): time limit expires"); */
        }
        else {
            notice("poll(): %s", strerror(errno));
        }

        /* reset events */
        for (i = 0; i < pd_count; i++)
            pcap_poll[i].revents = 0;
    }
}

void bridge_fwd(u_char *port, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
    /*
     * Ethernet header (14 bytes)
     * 1. destination MAC (6 bytes)
     * 2. source MAC (6 bytes)
     * 3. type (2 bytes)
     */
    struct ether_header *eth_hdr;
    int index; /* hash table index */
    int i;
    int new_hash_entry = 0; /* set to 1 if we have new hash entry */
    int outport = 0; /* port on which this packet will depart */
    /*
     * port on which the host with source MAC address for this packet resides on, this
     * value may be different with port (first argument of this function) if this packet
     * is departing from an interface when it was captured
     */
    int sport = 0;

    /* do nothing if Ethernet header is truncated */
    if (header->caplen < ETHER_HDR_LEN)
        return;

    eth_hdr = (struct ether_header *)malloc(ETHER_HDR_LEN);
    if (eth_hdr == NULL)
        error("malloc(): cannot allocate memory for eth_hdr");

    /* copy Ethernet header from pkt_data into eth_hdr */
    memcpy(eth_hdr, pkt_data, ETHER_HDR_LEN);

    /* BEGIN BRIDGE LEARNING SECTION */
    index = HASH_FUNC(eth_hdr->ether_shost); /* hash source MAC */

    /* found an entry in hash table @ index */
    if (bridge_ht[index].port != 0) {
        /* collision -> override previous bridge_ht structure @ index in hash table */
        if (!ETHER_ADDR_EQ(&bridge_ht[index].etheraddr, eth_hdr->ether_shost))
            new_hash_entry = 1;

        /* port on which the host with source MAC address for this packet resides on */
        sport = bridge_ht[index].port;
    }
    /* nothing @ index of hash table -> store our new bridge_ht structure there */
    else {
        new_hash_entry = 1;
    }

    /* store a new hash entry */
    if (new_hash_entry) {
        /* copy source MAC from eth_hdr into bridge_ht[index] */
        memcpy(&bridge_ht[index].etheraddr, eth_hdr->ether_shost, ETHER_ADDR_LEN);

        /* port on which source MAC resides on */
        bridge_ht[index].port = (int)port;

        /* set a timeout for this entry in units of HASH_ALARM in seconds */
        bridge_ht[index].timeout = HASH_TIMEOUT;

        if (vflag > 0) {
            (void)printf("[%06d]mac=", index);
            for (i = 0; i < ETHER_ADDR_LEN; i++) {
                (void)printf("%02x", eth_hdr->ether_shost[i]);
                i == (ETHER_ADDR_LEN - 1) ? (void)putchar(',') : (void)putchar(':');
            }
            (void)printf("port=%d\n", bridge_ht[index].port);
        }
    }
    /* END BRIDGE LEARNING SECTION */

    /* check if destination MAC is local MAC */
    for (i = 0; i < pd_count; i++) {
        if (ETHER_ADDR_EQ(bridge_addr[i]->octet, eth_hdr->ether_dhost)) {
            free(eth_hdr); eth_hdr = NULL;
            return;
        }
    }

    /* BEGIN BRIDGE FORWARDING SECTION */
    /* broadcast packet */
    if (ETHER_ADDR_EQ(eth_hdr->ether_dhost, ether_bcast)) {
        /* forward packet out through all ports except the port on which this packet arrives */
        outport = -(int)port;
    }
    /* multicast packet */
    else if (ETHER_IS_MULTICAST(eth_hdr->ether_dhost)) {
        /* forward packet out through all ports except the port on which this packet arrives */
        outport = -(int)port;
    }
    /* unicast packet */
    else {
    index = HASH_FUNC(eth_hdr->ether_dhost); /* hash destination MAC */

    /* nothing @ index in hash table -> we have no information on destination MAC */
    if (bridge_ht[index].port == 0) {
            /* forward packet out through all ports except the port on which this packet arrives */
            outport = -(int)port;
    }
    else {
            /* source MAC in hash table matches destination MAC */
            if (ETHER_ADDR_EQ(&bridge_ht[index].etheraddr, eth_hdr->ether_dhost)) {
            /* do not forward packet within the same LAN segment */
            if (bridge_ht[index].port == (int)port) {
                    outport = 0;
            }
            /* forward packet out through bridge_ht[index].port */
            else {
                    outport = bridge_ht[index].port;
            }
            }
            /* forward packet out through all ports except the port on which this packet arrives */
            else {
            outport = -(int)port;
            }
    }
    }

    if (outport != 0) {
        send_packets(outport,
                     sport,
                     (const struct ether_addr *)eth_hdr->ether_dhost,
                     (const struct ether_addr *)eth_hdr->ether_shost,
                     pkt_data, header->caplen);
    }
    /* END BRIDGE FORWARDING SECTION */

    free(eth_hdr); eth_hdr = NULL;
    return;
}

void send_packets(int outport,
                  int sport,
                  const struct ether_addr *ether_dhost,
                  const struct ether_addr *ether_shost,
                  const u_char *pkt_data,
                  int pkt_len)
{
    int start, end;
    int i, j;
    int ret;
    sigset_t block_sig;

    (void)sigemptyset(&block_sig);
    (void)sigaddset(&block_sig, SIGINT);

    /*
     * if port is a negative integer, the positive of it is the port
     * on which this packet arrives
     */
    if (outport < 0) { /* send out through all ports except the port on which this packet arrives */
        start = 0;
        end = pd_count;
    }
    else { /* send out through the specified port only */
        start = outport - 1;
        end = outport;
    }

    (void)sigprocmask(SIG_BLOCK, &block_sig, NULL); /* hold SIGINT */

    /* finish the injection before we give way to SIGINT */
    for (i = start; i < end; i++) {
        /*
         * if we are forwarding this packet out through all ports, do not forward
         * it back to the port on which this packet arrives
         */
        if ((outport < 0) && (i == (-outport - 1))) {
            /* do nothing */
        }
        /*
         * in addition to the port on which we received this packet, we also should not send this
         * packet to the port on which the host with this source MAC address resides on
         */
        else if ((sport != 0) && (i == (sport - 1))) {
            /* do nothing */
        }
        else {
            if ((ret = pcap_inject(pd[i], pkt_data, pkt_len)) == -1) {
                notice("%s", pcap_geterr(pd[i]));
                ++failed;
            }
            else {
                if (vflag > 0) {
                (void)printf("dmac=");
                for (j = 0; j < ETHER_ADDR_LEN; j++) {
                    (void)printf("%02x", ether_dhost->octet[j]);
                    j == (ETHER_ADDR_LEN - 1) ? (void)putchar(',') : (void)putchar(':');
                }
                (void)printf("smac=");
                for (j = 0; j < ETHER_ADDR_LEN; j++) {
                    (void)printf("%02x", ether_shost->octet[j]);
                    j == (ETHER_ADDR_LEN - 1) ? (void)putchar(',') : (void)putchar(':');
                }
                    (void)printf("outport=%d\n", i + 1);
                }
                ++pkts_sent;
                bytes_sent += ret;
            }
        }
    }

    (void)sigprocmask(SIG_UNBLOCK, &block_sig, NULL); /* release SIGINT */
}

void hash_alarm_handler(int signum)
{
    int i;

    for (i = 0; i < HASH_SIZE; i++) {
        if ((bridge_ht[i].port != 0) && (--bridge_ht[i].timeout < 0)) {
            bridge_ht[i].port = 0;
            if (vflag > 0)
                (void)printf("[%06d]deleted\n", i);
        }
    }

    /* reset alarm */
    if (hash_alarm(HASH_ALARM) == -1)
        error("setitimer(): %s", strerror(errno));
}

int hash_alarm(unsigned int seconds)
{
    struct itimerval old, new;

    new.it_interval.tv_usec = 0;
    new.it_interval.tv_sec = 0;
    new.it_value.tv_usec = 0;
    new.it_value.tv_sec = (long)seconds;

    return (setitimer(ITIMER_REAL, &new, &old));
}

/*
 * looks like BSD does not have SIOCGIFHWADDR,
 * let me know if you have a portable gethwaddr(), i.e. works for both BSD and Linux systems
 */
#ifndef SIOCGIFHWADDR
/*
 * Reference: getmac.c from bridged by Keisuke Uehara(kei@wide.ad.jp)
 *
 * Copyright(c) 1999
 * Keisuke UEHARA(kei@wide.ad.jp)
 * All rights reserved.
 *
 *
 * Copyright(c) 1998,1999
 * Masahiro ISHIYAMA(masahiro@isl.rdc.toshiba.co.jp)
 * All rights reserved.
 *
 */
#define ALLSET(flag, bits) (((flag) & (bits)) == (bits))
int gethwaddr(struct ether_addr *ether_addr, char *device)
{
    struct ifaddrs *ifa, *ifap;
    struct sockaddr_dl *sdl;

    if (getifaddrs(&ifap) < 0)
        return (-1);

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (strcmp(device, ifa->ifa_name) != 0)
            continue;
        if (!ALLSET(ifa->ifa_flags, IFF_UP | IFF_BROADCAST))
            continue;
        if (ifa->ifa_addr->sa_family != AF_LINK)
            continue;

        sdl = (struct sockaddr_dl *)ifa->ifa_addr;
        bcopy(LLADDR(sdl), ether_addr, ETHER_ADDR_LEN);
        freeifaddrs(ifap);
        return (0);
    }

    freeifaddrs(ifap);
    return (-1);
}
#else
/* works for Linux */
int gethwaddr(struct ether_addr *ether_addr, char *device)
{
    struct ifreq ifr;
    int fd;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) != -1) {
        strcpy(ifr.ifr_name, device);
        if (ioctl(fd, SIOCGIFHWADDR, &ifr) != -1) {
            bcopy(&ifr.ifr_hwaddr.sa_data, ether_addr, ETHER_ADDR_LEN);
            return (0);
        }
    }

    return (-1);
}
#endif

void info(void)
{
    struct timeval elapsed;
    float seconds;

    if (gettimeofday(&end, NULL) == -1)
        notice("gettimeofday(): %s", strerror(errno));
    timersub(&end, &start, &elapsed);
    seconds = elapsed.tv_sec + (float)elapsed.tv_usec / 1000000;

    (void)putchar('\n');
    notice("%u packets (%u bytes) sent", pkts_sent, bytes_sent);
    if (failed)
        notice("%u write attempts failed", failed);
    notice("Elapsed time = %f seconds", seconds);
}

void cleanup(int signum)
{
    if (signum == -1)
        exit(EXIT_FAILURE);
    else
        info();
    exit(EXIT_SUCCESS);
}

/*
 * Reference: tcpdump's util.c
 *
 * Copyright (c) 1990, 1991, 1993, 1994, 1995, 1996, 1997
 *      The Regents of the University of California.  All rights reserved.
 *
 */
void notice(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    (void)vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (*fmt) {
        fmt += strlen(fmt);
        if (fmt[-1] != '\n')
            (void)fputc('\n', stderr);
    }
}

/*
 * Reference: tcpdump's util.c
 *
 * Copyright (c) 1990, 1991, 1993, 1994, 1995, 1996, 1997
 *      The Regents of the University of California.  All rights reserved.
 *
 */
void error(const char *fmt, ...)
{
    va_list ap;
    (void)fprintf(stderr, "%s: ", program_name);
    va_start(ap, fmt);
    (void)vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (*fmt) {
        fmt += strlen(fmt);
        if (fmt[-1] != '\n')
            (void)fputc('\n', stderr);
    }
    cleanup(-1);
}

void usage(void)
{
    (void)fprintf(stderr, "%s version %s\n"
        "%s\n"
        "Usage: %s [-d] [-v] [-i interfaces] [-h]\n"
        "\nOptions:\n"
        " -d             Print a list of network interfaces available.\n"
        " -v             Print forwarding information including the destination\n"
        "                and source MAC addresses of forwarded packets.\n"
        " -i interfaces  List of comma separated Ethernet-type interfaces to bridge.\n"
        "                Example: -i vr0,fxp0\n"
        "                You can specify between %d to %d interfaces.\n"
        " -h             Print version information and usage.\n",
        program_name, BITTWISTB_VERSION, pcap_lib_version(), program_name, PORT_MIN, PORT_MAX);
    exit(EXIT_SUCCESS);
}
