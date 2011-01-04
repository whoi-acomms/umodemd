/*!
 * \file umodemd.c
 * \author jon shusta <jshusta@whoi.edu>
 * \date 2009.04
 * \brief a daemon for tee-ing NMEA between stdio and a WHOI micro-modem to 
 * logs.
 */

/*!
 * \mainpage umodemd
 * \section intro introduction
 * umodemd will open a file descriptor with termios settings for a modem, then
 * redirect stdin and stdout to that descriptor. everything written and read
 * between stdio and the modem will also be logged to the specified directory.
 *
 * all runtime errors are handled by attempting to write them in NMEA-compliant
 * strings, first to the logfiles, then to the console. clients which cannot
 * handle the custom ID cannot use umodemd.
 *
 * \section usage usage
 * <pre>umodemd x.y <jshusta@whoi.edu>
 * usage: ./umodemd <serial-device> <baud-rate> <log-directory>
 *  supports rates 19200 and 115200.
 *  log-directory may not have a trailing slash.</pre>
 *
 * \section files files
 *  <ul><li>config.h</li>
 *  <li>umodemd.c</li></ul>
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>

#define MAX(a,b)    (((a)>(b))?(a):(b))
#define MIN(a,b)    (((a)<(b))?(a):(b))

#ifndef CNEW_RTSCTS
#  define CNEW_RTSCTS (CRTSCTS)
#endif

/*! filepath of debug log.
 */
#define DEBUG_LOGFILE "umodemd.log"

/*!
 * a talker ID to pass NMEA-compliant log messages through the console
 * when writing to the debug log fails.
 */
#define UMODEMD_TALKER_ID "$UMDMD"

#define UM_LOGW_MINUTE  (30)  /*!< divide logs on 30 min interval */
#define UM_SYNC_MINUTE  (10)  /*!< sync disks on 10 min interval */

#define BUFSZ       (0x40000)  /*!< IO buffers */
#define NMEASSZ     (0x40000)  /*!< largest possible NMEA sentence */
#define FPATHSZ     (0x1000)   /*!< file paths */
#define TIMESSZ     (0x1000)   /*!< string representation of time */
#define ERRORSZ     (0x1000)   /*!< error strings */
#define IO_TX       (0)        /*!< message from console, to modem, to log */
#define IO_RX       (1)        /*!< message from modem, to console, to log */


#define UERROR(s,e) uerror(s,e,__LINE__)


/*! modem daemon state
 */
typedef struct
{
  char *  dev,
       *  log;
  int     baud,
          t_pol,
          mfd,
          sync_chk;
  FILE *  i_cli,
       *  o_cli;
} umodemd_t;


/** forward decls *************************************************************/


void uerror  (umodemd_t *state, int e, const int line);

void umodemd_sync         (umodemd_t *state);
int umodemd_write_debug   (umodemd_t *state, const int line, char *msg);
int umodemd_write_client  (umodemd_t *state, const char *msg, const size_t len, const struct timeval *now);
int umodemd_write_modem   (umodemd_t *state, const char *msg, const size_t len, const struct timeval *now);
int umodemd_write_log     (umodemd_t *state, const char *msg, const size_t len, const struct timeval *now, const int io);
int umodemd_dispatch      (umodemd_t *state, char *msg,  size_t len, const int io);
int umodemd_fetch         (umodemd_t *state, const int fd, char *wbuf, const size_t maxlen, size_t *wlen);
int umodemd_open_modem    (umodemd_t *state);
int umodemd_open_client   (umodemd_t *state);
void umodemd_close_modem  (umodemd_t *state);
void umodemd_close_client (umodemd_t *state);
void umodemd_signal       (int s);
int umodemd               (umodemd_t *state);
int umodemd_scan          (umodemd_t *state, int argc, char ** argv);

/** in-tree libnmea module ****************************************************/

extern int nmea_cksum(const char *sentence, long *cksum);
extern size_t nmea_scan(char *wbuf, const size_t maxlen, char *rbuf, size_t *rlen);



/*! message strings for the log source ID
 */
const char *g_io_strs[2] =
{
  "TX", /* IO_TX */
  "RX", /* IO_RX */
};


/*! is-running flag; switched to zero by signal handler on SIGINT,
 * checked by main loop every pass in while().
 */
volatile int g_running = 1;

/*! global state. accessed directly only in main() and signal handler.
 */
umodemd_t g_state =
{
  .dev      =  NULL,
  .log      =  NULL,
  .baud     =  19200,
  .t_pol    =  50,
  .mfd      =  -1,
  .sync_chk =  1,
};


// definitions


/*!
void printd(const char *msg, int len)
{
  int k;
  for (k = 0; k < len; k++)
    printf(isprint(msg[k]) ? "%c" : "\\x%02hhx" , msg[k]);
}
 */


/*! sync wrapper
 */
void umodemd_sync(umodemd_t *state)
{
  struct tm stamp;
  time_t now = time(NULL);
  if (((time_t)-1) == now) return;
  if (NULL == gmtime_r(&now, &stamp)) return;

  if (0 == (stamp.tm_min % UM_SYNC_MINUTE))
  {
    if (state->sync_chk)
    {
      state->sync_chk = 0;
      sync();
    }
  }
  else state->sync_chk = 1;
}


/*!
 * custom error handler.
 * first attempts to write to debug log.
 * on failure, a state-less attempt is made (see umodemd_write_debug).
 *
 * NMEA-friendly errno indicator is sent to stdout
 * this avoids confusing the client when state->o_cli == stdout (default)
 * since uerror falls through, the cascading failure will log two errors
 * the first one will be the uerror(NULL, errno) call inside umodemd_write_debug
 * the second one will be the "real" error that triggered the first uerror() call.
 *
 */
void uerror(umodemd_t *state, int e, const int line)
{
  char ebuf[ERRORSZ] = {'\0'},
       cbuf[ERRORSZ] = {'\0'};
  long cksum = 0;
  int  uerr = 1;

  strerror_r(e, ebuf, ERRORSZ);

  if (state)
  {
    uerr = umodemd_write_debug(state, line, ebuf);
  }

  if (uerr)
  {
    snprintf(cbuf, ERRORSZ, UMODEMD_TALKER_ID ",%s:%d,%d,%s*", __FILE__, line, e, ebuf);
    nmea_cksum(cbuf, &cksum);
    printf("%s%02hhx\n", cbuf, (int)cksum);
  }
}


/*!
 */
int umodemd_write_debug(umodemd_t *state, const int line, char *msg)
{
  int wlen;
  FILE *df;
  df = fopen(DEBUG_LOGFILE, "a");
  if (NULL == df)
  {
    fprintf(stderr, "FATAL: could not open " DEBUG_LOGFILE "\n");
    return -1;
  }
  wlen = fprintf(df, "%ld %s:%d %s\n", time(NULL), __FILE__, line, msg);
  fclose(df);
  if (0 > wlen)
  {
    UERROR(NULL, errno); /* UERROR STATE MUST BE NULL HERE. */
    return -1;
  }
  return 0;
}


/*!
 */
int umodemd_write_client(umodemd_t *state, const char *msg, const size_t len, const struct timeval *now)
{
  int wlen = fprintf(state->o_cli, "%s\n", msg);
  if (0 > wlen)
  {
    UERROR(state, errno);
    return -1;
  }
  return 0;
}


/*!
 */
int umodemd_write_modem(umodemd_t *state, const char *msg, const size_t len, const struct timeval *now)
{
  char nmsg[NMEASSZ] = {'\0'};
  int  nlen = 0,
       wsz  = 0,
       wr   = 0;

  nlen = snprintf(nmsg, NMEASSZ, "%s\r\n", msg);
  if (0 > nlen)
  {
    UERROR(state, errno);
    return -1;
  }
  while (wsz < nlen)
  {
    wr = write(state->mfd, nmsg+wsz, nlen-wsz);
    if (0 == wr) break;
    if (0 > wr)
    {
      UERROR(state, errno);
      return -1;
    }
    wsz += (size_t)wr;
  }
  return 0;
}


/*!
 */
int umodemd_write_log(umodemd_t *state, const char *msg, const size_t len, const struct timeval *now, const int io)
{
  char   tbuf[TIMESSZ] = {'\0'},
         filetimebuf[TIMESSZ] = {'\0'},
         pbuf[FPATHSZ] = {'\0'};
  int    wlen,
         plen;
  FILE * fp;
  void * result;
  struct tm stamp;
  struct tm filename_time;

  if (NULL == gmtime_r(&(now->tv_sec), &stamp))
  {
    UERROR(state, errno);
    return -1;
  }

  /* get filename
   * round minutes off to tens
   *   "%s/nmea-%s.csv"
   *   state->log
   *   strftime "%F-%H-%M"
   */
  /* Make a copy of the timestamp, and round it
   * to the nearest 10 minutes.
   */
  result = memcpy(&filename_time, &stamp, sizeof(struct tm));
  if (NULL == result)
  {
    UERROR(state, errno);
    return -1;
  }
  filename_time.tm_min = (stamp.tm_min/UM_LOGW_MINUTE)*UM_LOGW_MINUTE;

  /* Turn the rounded timestamp into a string we can use in the file name.
   */
  wlen = strftime(filetimebuf, TIMESSZ-1, "%F-%H-%M", &filename_time);
  if (0 == wlen)
  {
    UERROR(state, errno);
    return -1;
  }

  /* Generate the file pa
   */
  plen = snprintf(pbuf, FPATHSZ, "%s/nmea-%s.csv", state->log, filetimebuf);
  if (0 > plen)
  {
    UERROR(state, errno);
    return -1;
  }

  /* open + append
   */
  fp = fopen(pbuf, "a");
  if (NULL == fp)
  {
    UERROR(state, errno);
    return -1;
  }

  /* get log line
   *   "%s,NMEA.%s,%s"
   *   strftime "%F %T"
   *   direction: {TX, RX}
   *   smsg
   */
  wlen = strftime(tbuf, TIMESSZ-1, "%F %T", &stamp);
  if (0 == wlen)
  {
    UERROR(state, errno);
    memset(tbuf, '\0', TIMESSZ);
    tbuf[0]=' ';
  }

  /* write
   */
  wlen = fprintf(fp, "%sZ,NMEA.%s,%s\n", tbuf, g_io_strs[io], msg);
  if (0 > wlen) UERROR(state, errno);
  if (fclose(fp)) UERROR(state, errno);
  return 0;
}


/*!
 */
int umodemd_dispatch(umodemd_t *state, char *msg, size_t len, const int io)
{
  int uerr = 0;
  struct timeval now;

  /* sanitize
   */
  len = strcspn(msg, "\r\n");
  msg[len] = 0;

  /* dispatch sentence
   */
  gettimeofday(&now, NULL);
  if (io == IO_TX) uerr |= umodemd_write_modem(state, msg, len, &now);
  if (io == IO_RX) uerr |= umodemd_write_client(state, msg, len, &now);
  uerr |= umodemd_write_log(state, msg, len, &now, io);
  return uerr;
}


/*!
 */
int umodemd_fetch(umodemd_t *state, const int fd, char *wbuf, const size_t maxlen, size_t *wlen)
{
  struct pollfd pfd =
  {
    .revents = 0,
    .events  = POLLIN,
    .fd      = fd,
  };
  int ret = 0,
      len = 0;

  ret = poll(&pfd, 1, state->t_pol);
  if (0 > ret)
  {
    if (EINTR == errno) return 0;
    UERROR(state, errno);
    return -1;
  }

  if (0 == ret)
    return 0;

  if (0 == (pfd.revents & POLLIN))
    return 0;

  len = read(pfd.fd, wbuf + *wlen, maxlen - *wlen);
  if (0 > len)
  {
    if (EINTR == errno) return 0;
    UERROR(state, errno);
    return -1;
  }

  *wlen += len;
  return 1;
}


/*!
 */
int umodemd_open_modem(umodemd_t *state)
{
  struct termios opt;
  int rate = 0;

  state->mfd = open(state->dev, O_RDWR|O_NOCTTY);
  if (0 > state->mfd)
  {
    UERROR(state, errno);
    return -1;
  }

  memset(&opt, 0, sizeof(struct termios));
  if (tcgetattr(state->mfd, &opt))
  {
    UERROR(state, errno);
    return -1;
  }

  opt.c_iflag    &= ~IXON;
  opt.c_iflag    &= ~IXOFF;
  opt.c_iflag    &= ~IXANY;
  opt.c_iflag    |= IGNBRK | IGNPAR;
  opt.c_oflag     = 0;
  opt.c_lflag     = 0;
  opt.c_cflag    &= ~CSIZE;
  opt.c_cflag    &= ~PARENB;
  opt.c_cflag    &= ~CSTOPB;
  opt.c_cflag    &= ~CNEW_RTSCTS;
  opt.c_cflag    |= CLOCAL | CREAD | CS8;
  opt.c_cc[VMIN]  = 0;
  opt.c_cc[VTIME] = 0;


  switch (state->baud)
  {
    case   1200: rate = B1200;   break;
    case   1800: rate = B1800;   break;
    case   2400: rate = B2400;   break;
    case   4800: rate = B4800;   break;
    case   9600: rate = B9600;   break;
    default:
    case  19200: rate = B19200;  break;
    case  38400: rate = B38400;  break;
    case  57600: rate = B57600;  break;
    case 115200: rate = B115200; break;
    case 230400: rate = B230400; break;
  }
  if (cfsetispeed(&opt, rate))
  {
    UERROR(state, errno);
    return -1;
  }
  if (cfsetospeed(&opt, rate))
  {
    UERROR(state, errno);
    return -1;
  }
  if (tcsetattr(state->mfd, TCSANOW, &opt))
  {
    UERROR(state, errno);
    return -1;
  }

  return 0;
}


/*!
 */
int umodemd_open_client(umodemd_t *state)
{
  return 0;
}


/*!
 */
void umodemd_close_modem(umodemd_t *state)
{
#ifdef HAVE_SYS_IOCTL_H
  ioctl(state->mfd, TCFLSH, 0);
#endif
  close(state->mfd);
}


/*!
 */
void umodemd_close_client(umodemd_t *state)
{
}


/*!
 */
void umodemd_signal(int s)
{
  g_running = 0;
}


/*!
 */
int umodemd(umodemd_t *state)
{
  char    txbuf[BUFSZ] = {'\0'};
  char    rxbuf[BUFSZ] = {'\0'},
          msg[NMEASSZ] = {'\0'};
  size_t  txlen = 0,
          rxlen = 0,
          len   = 0,
          ret   = 0;

  if (umodemd_open_client(state)) return 1;
  if (umodemd_open_modem(state))  return 1;

  signal(SIGINT, umodemd_signal);

  while (g_running)
  {
    umodemd_sync(state);

    if (0 < umodemd_fetch(state, state->mfd, rxbuf, BUFSZ, &rxlen))
    while (1)
    {
      memset(msg, '\0', sizeof(char) * NMEASSZ);
      len = nmea_scan(msg, NMEASSZ, rxbuf, &rxlen);
      if (0 == len) break;
      ret = umodemd_dispatch(state, msg, len, IO_RX);
    }

    if (0 > fileno(state->i_cli))
    {
      perror("fileno");
      break;
    }

    if (0 < umodemd_fetch(state, fileno(state->i_cli), txbuf, BUFSZ, &txlen))
    {
      memset(msg, '\0', sizeof(char) * NMEASSZ);
      len = nmea_scan(msg, NMEASSZ, txbuf, &txlen);
      if (len)
        ret = umodemd_dispatch(state, msg, len, IO_TX);
    }
  }

  sync();
  umodemd_close_client(state);
  umodemd_close_modem(state);
  return 0;
}


/*!
 */
int umodemd_scan(umodemd_t *state,
                 int        argc,
                 char **    argv)
{
  DIR *d;

  if (3 > argc) return 1;

  if (access(argv[0], F_OK|R_OK|W_OK))
  {
    perror("access");
    return 1;
  }

  if (NULL == (d = opendir(argv[2])))
  {
    perror("opendir");
    return 1;
  }
  closedir(d);

  state->dev  = argv[0];
  state->baud = atoi(argv[1]);
  state->log  = argv[2];
  return 0;
}


/*!
 */
int umodemd_usage(char *img)
{
  printf(PACKAGE_STRING " <" PACKAGE_BUGREPORT ">\n");
  printf("usage: %s <serial-device> <baud-rate> <log-directory>\n", img);
  printf("  log-directory may not have a trailing slash.\n");
  return 1;
}


/*!
 */
int main(int     argc,
         char ** argv)
{
  g_state.i_cli = stdin;
  g_state.o_cli = stdout;
  return umodemd_scan(&g_state, argc-1, argv+1)
       ? umodemd_usage(argv[0])
       : umodemd(&g_state);
}
