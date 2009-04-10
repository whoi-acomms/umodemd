/*!
 * \file umodemd.c
 * \author jon shusta <jshusta@whoi.edu>
 * \date 2009.04
 * \brief a daemon for logged serial comms with the micro-modem.
 */

#include <stdlib.h>
#include <stdio.h>
#include <libnmea.h>

#define NMEAMSGSZ   (0x1000)
#define MIN(a,b)    (((a)<(b))?(a):(b))
#define IO_TX       (0)
#define IO_RX       (1)

volatile int g_running = 1;


/*!
 */
void umodemd_dispatch(umodemd_t *state, const char *msg, const size_t len)
{
  char    smsg[NMEAMSGSZ+1]={'\0'};
  size_t  slen;

  slen = snprintf(smsg, MIN(len-1,NMEAMSGSZ+1), "%s", msg);

  /*
   * filename:
   *   "%s/nmea-%s.csv"
   *   state->log
   *   strftime "%F-%R"
   *
   * logline:
   *   "%s,%s,%s,%s"
   *   strftime "%F-%T"
   *   source:
   *   direction: {TX, RX}
   *   smsg
   */

  /*
   * fp = fopen(filename, create|write|append)
   * fprintf(fp, "%s\n", logline)
   * fclose(fp)
   */

  printf("%s\n", smsg);
}


/*!
 */
int umodemd(umodemd_t *state)
{
  char    rxbuf[NMEAMSGSZ*8] = {'\0'},
          msg[NMEAMSGSZ]     = {'\0'};
  size_t  rxlen = 0,
          len   = 0;

  while (g_running)
  {
    while (1)
    {
      len = nmea_scan(msg, NMEAMSGSZ, rxbuf, &rxlen);
      if (len) umodemd_dispatch(state, msg, len);
      else     break;
    }

    
  }
  return 0;
}


/*!
 */
int umodemd_scan(umodemd_t *state,
                 int        argc,
                 char **    argv)
{
  if (3 < argc) return 1;

  if (access(argv[0], F_OK|R_OK|W_OK))
  {
    perror("access");
    return 1;
  }

  if (NULL == opendir(argv[2]))
  {
    perror("opendir");
    return 1;
  }
  closedir(argv[2]);

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

