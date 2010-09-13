/*!
 * \file libnmea.c
 * \author jon shusta <jshusta@whoi.edu>
 * \date 2008.08
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/** nmea scanner states
 */
#define S_NMARK (1)  /** in junk */
#define S_SMARK (2)  /** saw $, inside message */
#define S_EMARK (3)  /** saw \n, end message */


/*!
 */
int nmea_cksum(const char *  sentence,
               long *        cksum)
{
  char *end     = NULL,
       *start   = NULL;
  long calcsum  = 0,
       checksum = 0;

  end = strchr(sentence, '*');                /* end at asterisk */
  if (NULL == end) return -1;
  checksum = strtol(end+1, (char **)NULL, 16);/* get stated cksum */
  if (0 > checksum)
  {
    perror("strtol");
    return -1;
  }
  start = (char *)sentence + 1;               /* begin after $ */
  while (start < end) calcsum ^= *(start++);  /* calculate */
  if (NULL != cksum) *cksum = calcsum;        /* store if valid pointer */
  return (calcsum == checksum) ? 0 : -1;      /* test, return */
}


/*!
 */
size_t nmea_scan(char *        wbuf,
                 const size_t  maxlen,
                 char *        rbuf,
                 size_t *      rlen)
{
  size_t len = 0;
  int state = S_NMARK,
      pivot = 0,
      k     = 0,
      go    = 1;

  while (k < *rlen && go)
  {
    switch (state)
    {
    case S_NMARK:                               /* nothing state */
      while ((k < *rlen) && ('$' != rbuf[k]))   /* begin looking for $ */
        ++k;
      if (k < *rlen)                            /* found $, loop broke */
      {
        pivot = k;                              /* store pivot point */
        state = S_SMARK;                        /* transition to in-sentence state */
      }
      break;                                    /* fall through if no $ and k==rlen */

    case S_SMARK:                               /* in-sentence state */
      while ((k < *rlen) && ('\n' != rbuf[k]))  /* begin looking for \n */
      {
        if ('$' == rbuf[k])                     /* update pivot if new $ found; */
          pivot = k;                            /* quietly discard sentence between $'s */
        ++k;
      }
      if (k < *rlen)                            /* found \n, loop broke */
        state = S_EMARK;
      break;                                    /* fall through if no \n */

    case S_EMARK:                               /* scan state ready to extract a sentence */
      len = k - pivot + 1;                      /* get sentence length */
      if (len >= maxlen) len = maxlen-1;        /* truncate if too long */
      memcpy(wbuf, rbuf + pivot, len);          /* copy from rbuf to wbuf */
      memset(wbuf+len, '\0', sizeof(char));     /* scrub the rest of wbuf */
      pivot = k + 1;                            /* save new pivot value */
      go = 0;                                   /* terminate the scanner */
      break;
    }
  }

  *rlen -= pivot;                                /* update length to return real pivot to zero */
  if (*rlen) memmove(rbuf, rbuf + pivot, *rlen); /* erase all scanned-over input from rbuf */
  return len;
}
