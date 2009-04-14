/*!
 * \file umodemd.c
 * \author jon shusta <jshusta@whoi.edu>
 * \date 2009.04
 * \brief a daemon for logged serial comms with the micro-modem.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <libnmea.h>
#include <errno.h>


#define MAX(a,b)    (((a)>(b))?(a):(b))
#define MIN(a,b)    (((a)<(b))?(a):(b))

#ifndef CNEW_RTSCTS
#  define CNEW_RTSCTS (CRTSCTS)
#endif

#define FPATHSZ     (0x1000)
#define FLINESZ     (0x1000)
#define TIMESSZ     (0x1000)
#define NMEASSZ     (0x1000)
#define ERRORSZ     (0x1000)
#define IO_TX       (0)
#define IO_RX       (1)

#define UMODEMD_INITIALIZER \
{\
  .dev   = NULL, \
  .log   = NULL, \
  .baud  = 19200, \
  .t_pol = 50, \
  .t_sel = 50, \
  .i_cli = stdin, \
  .o_cli = stdout, \
  .modem = NULL, \
}


/*! modem daemon state
 */
typedef struct
{
  char *  dev,
       *  log;
  int     baud,
          mfd,
          t_pol,
          t_sel;
  FILE *  i_cli,
       *  o_cli;

} umodemd_t;


/*! forward decl
 */
void uerror  (umodemd_t *state, int e);
int umodemd_write_client  (umodemd_t *state, const char *msg, const size_t len, const struct timeval *now);
int umodemd_write_modem   (umodemd_t *state, const char *msg, const size_t len, const struct timeval *now);
int umodemd_write_log     (umodemd_t *state, const char *msg, const size_t len, const struct timeval *now, const int io);
int umodemd_dispatch      (umodemd_t *state, const char *msg, const size_t len, const int io);
int umodemd_fetch_modem   (umodemd_t *state, char *wbuf, size_t *wlen);
int umodemd_fetch_client  (umodemd_t *state, char *wbuf, size_t *wlen);
int umodemd_open_modem    (umodemd_t *state);
int umodemd_open_client   (umodemd_t *state);


/*! message strings
 */
const char *g_io_strs[2] =
{
  "TX",
  "RX",
};


/*! global state
 */
volatile int g_running = 1;


/*! custom error handler with fallback to printf
 */
void uerror(umodemd_t *state, int e)
{
  char ebuf[ERRORSZ] = {'\0'};
  strerror_r(e, ebuf, ERRORSZ);
  if (state) umodemd_write_client(state, ebuf, strlen(ebuf), NULL);
  else printf("%s\n", ebuf);
}


/*!
 */
int umodemd_write_client(umodemd_t *state, const char *msg, const size_t len, const struct timeval *now)
{
  int wlen = fprintf(state->o_cli, "%s", msg);
  if (0 > wlen)
  {
    uerror(NULL, errno);
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
    uerror(state, errno);
    return -1;
  }
  while (wsz < nlen)
  {
    wr = write(state->mfd, nmsg+wsz, nlen-wsz);
    if (0 == wr) break;
    if (0 > wr)
    {
      uerror(state, errno);
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
         pbuf[FPATHSZ] = {'\0'},
         lbuf[FLINESZ] = {'\0'};
  int    wlen,
         plen,
         llen;
  FILE * fp;

  struct tm stamp;

  if (NULL == gmtime_r(&(now->tv_sec), &stamp))
  {
    uerror(state, errno);
    return -1;
  }

  /* get filename
   *   "%s/nmea-%s.csv"
   *   state->log
   *   strftime "%F-%R"
   */
  wlen = strftime(tbuf, TIMESSZ-1, "%F-%R", &stamp);
  if (0 == wlen)
  {
    uerror(state, errno);
    return -1;
  }
  plen = snprintf(pbuf, FPATHSZ, "%s/nmea-%s.csv", state->log, tbuf);
  if (0 > plen)
  {
    uerror(state, errno);
    return -1;
  }

  /* open + append
   */
  fp = fopen(pbuf, "a");
  if (NULL == fp)
  {
    uerror(state, errno);
    return -1;
  }

  /* get log line
   *   "%s,NMEA.%s,%s"
   *   strftime "%F-%T%^Z"
   *   direction: {TX, RX}
   *   smsg
   */
  strftime(tbuf, TIMESSZ-1, "%F-%T%^Z", &stamp);
  {
    uerror(state, errno);
    if (fclose(fp)) uerror(state, errno);
    return -1;
  }
  llen = snprintf(lbuf, FLINESZ, "%s,NMEA.%s,%s", tbuf, g_io_strs[io], msg);
  if (0 > llen)
  {
    uerror(state, errno);
    if (fclose(fp)) uerror(state, errno);
    return -1;
  }

  /* write
   */
  wlen = fprintf(fp, "%s\n", lbuf);
  if (0 > llen) uerror(state, errno);
  if (fclose(fp)) uerror(state, errno);
  return 0;
}


/*!
 */
int umodemd_dispatch(umodemd_t *state, const char *msg, const size_t len, const int io)
{
  char  smsg[NMEASSZ] = {'\0'};
  int   slen,
        wlen,
        uerr = 0;

  struct timeval now;

  gettimeofday(&now, NULL);

  /* sanitize the string so that all handlers work from the same content
   * fit into sentence buffer with zero-term
   * remove trailing whitespace
   */
  slen = snprintf(smsg, MIN(len, NMEASSZ), "%s", msg);
  if (0 > slen)
  {
    uerror(state, errno);
    return -1;
  }
  wlen = strcspn(smsg, "\t\r\n ");
  if (wlen)
  {
    smsg[wlen] = 0;
    slen = wlen;
  }

  /* dispatch sentence
   */
  if (io == IO_TX) uerr |= umodemd_write_modem(state, smsg, slen, &now);
  if (io == IO_RX) uerr |= umodemd_write_client(state, smsg, slen, &now);
  uerr |= umodemd_write_log(state, smsg, slen, &now, io);
  return uerr;
}


/*!
 */
int umodemd_fetch_modem(umodemd_t *state, char *wbuf, size_t *wlen)
{
  struct pollfd pfd =
  {
    .revents = 0,
    .events  = POLLIN,
    .fd      = state->mfd,
  };
  int ret = 0,
      len = 0;

  ret = poll(&pfd, 1, state->t_pol);
  if (0 > ret)
  {
    uerror(state, errno);
    return -1;
  }

  if (0 == ret)
    return 0;

  if (0 == (pfd.revents & POLLIN))
    return 0;

  len = read(state->mfd, wbuf + *wlen, NMEASSZ - *wlen);
  if (0 > len)
  {
    uerror(state, errno);
    return -1;
  }

  *wlen += len;
  return 1;
}


/*!
 */
int umodemd_fetch_client(umodemd_t *state, char *wbuf, size_t *wlen)
{
  struct timeval tv;
  fd_set fds;
  int ret = 0,
      fd  = fileno(state->i_cli),
      c   = 0;

  tv.tv_sec  = 0;
  tv.tv_usec = 1000 * state->t_sel;

  FD_ZERO(&fds);
  FD_SET(fd, &fds);

  ret = select(fd+1, &fds, NULL, NULL, &tv);
  if (0 > ret)
  {
    uerror(state, errno);
    return -1;
  }

  if (FD_ISSET(fd, &fds))
  {
    c = fgetc(state->i_cli);

    switch(c)
    {
      case EOF:
      case '\r':
      case '\n': return 1;
    }
    wbuf[(*wlen)++] = c;
  }

  return *wlen == NMEASSZ;
}


/*!
 */
int umodemd_open_modem(umodemd_t *state)
{
  struct termios opt;
  int rate = 19200;

  state->mfd = open(state->dev, O_RDWR|O_NOCTTY);
  if (0 > state->mfd)
  {
    uerror(state, errno);
    return -1;
  }

  if (tcgetattr(state->mfd, &opt))
  {
    uerror(state, errno);
    return -1;
  }

  switch (state->baud)
  {
    case 19200:  rate = B19200;  break;
    case 115200: rate = B115200; break;
  }
  if (cfsetispeed(&opt, rate))
  {
    uerror(state, errno);
    return -1;
  }
  if (cfsetospeed(&opt, rate))
  {
    uerror(state, errno);
    return -1;
  }

  opt.c_iflag     = (IGNBRK | IGNPAR) & ~(IXON | IXOFF | IXANY);
  opt.c_oflag     = 0;
  opt.c_lflag     = 0;
  opt.c_cflag    |= (CLOCAL | CREAD | CS8);
  opt.c_cflag    &= ~PARENB;
  opt.c_cflag    &= ~CSTOPB;
  opt.c_cflag    &= ~CSIZE;
  opt.c_cflag    &= ~CNEW_RTSCTS;
  opt.c_cc[VMIN]  = 0;
  opt.c_cc[VTIME] = 0;

  if (tcsetattr(state->mfd, TCSAFLUSH, &opt))
  {
    uerror(state, errno);
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
int umodemd(umodemd_t *state)
{
  char    txbuf[NMEASSZ*8] = {'\0'};
  char    rxbuf[NMEASSZ*8] = {'\0'},
          msg[NMEASSZ]     = {'\0'};
  size_t  txlen = 0,
          rxlen = 0,
          len   = 0,
          ret   = 0;

  if (umodemd_open_client(state))
    return 1;

  if (umodemd_open_modem(state))
    return 1;

  while (g_running)
  {
    if (0 < umodemd_fetch_modem(state, rxbuf, &rxlen))
    while (1)
    {
      len = nmea_scan(msg, NMEASSZ, rxbuf, &rxlen);
      if (!len) break;
      ret = umodemd_dispatch(state, msg, len, IO_RX);
    }

    if (0 < umodemd_fetch_client(state, txbuf, &txlen))
    {
      len = snprintf(msg, NMEASSZ, "%s", txbuf);
      txlen = 0;
      ret = umodemd_dispatch(state, msg, len, IO_TX);
    }
  }

  /* slam modem fd closed
   */
  ioctl(state->mfd, TCFLSH, 0);
  close(state->mfd);
  return 0;
}


/*!
 */
int umodemd_scan(umodemd_t *state,
                 int        argc,
                 char **    argv)
{
  DIR *d;

  if (3 < argc) return 1;

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
  printf("usage: %s <serial-device> <baud-rate> <log-directory>\n", img);
  printf("  log-directory may not have a trailing slash.\n");
  return 1;
}


/*!
 */
int main(int     argc,
         char ** argv)
{
  umodemd_t state;
  return umodemd_scan(&state, argc-1, argv+1)
       ? umodemd_usage(argv[0])
       : umodemd(&state);
}
