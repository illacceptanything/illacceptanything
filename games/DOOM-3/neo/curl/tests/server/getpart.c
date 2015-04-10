/***************************************************************************
 *                                  _   _ ____  _     
 *  Project                     ___| | | |  _ \| |    
 *                             / __| | | | |_) | |    
 *                            | (__| |_| |  _ <| |___ 
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2004, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 * 
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * $Id: getpart.c,v 1.10 2004/03/10 08:12:09 bagder Exp $
 ***************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "getpart.h"

#define EAT_SPACE(ptr) while( ptr && *ptr && isspace((int)*ptr) ) ptr++
#define EAT_WORD(ptr) while( ptr && *ptr && !isspace((int)*ptr) && ('>' != *ptr)) ptr++

#ifdef DEBUG
#define show(x) printf x
#else
#define show(x)
#endif

static
char *appendstring(char *string, /* original string */
                   char *buffer, /* to append */
                   size_t *stringlen, /* length of string */
                   size_t *stralloc)  /* allocated size */
{
  size_t len = strlen(buffer);
  size_t needed_len = len + *stringlen;

  if(needed_len >= *stralloc) {
    char *newptr;
    size_t newsize = needed_len*2; /* get twice the needed size */

    newptr = realloc(string, newsize);
    if(newptr) {
      string = newptr;
      *stralloc = newsize;
    }
    else
      return NULL;
  }
  strcpy(&string[*stringlen], buffer);
  *stringlen += len;

  return string;
}

const char *spitout(FILE *stream,
                    const char *main,
                    const char *sub, int *size)
{
  char buffer[8192]; /* big enough for anything */
  char cmain[128]=""; /* current main section */
  char csub[128]="";  /* current sub section */
  char *ptr;
  char *end;
  char display = 0;

  char *string;
  size_t stringlen=0;
  size_t stralloc=256;

  enum {
    STATE_OUTSIDE,
    STATE_INMAIN,
    STATE_INSUB,
    STATE_ILLEGAL
  } state = STATE_OUTSIDE;

  string = (char *)malloc(stralloc);
  if(!string)
    return NULL;

  string[0] = 0; /* zero first byte in case of no data */
  
  while(fgets(buffer, sizeof(buffer), stream)) {

    ptr = buffer;

    /* pass white spaces */
    EAT_SPACE(ptr);

    if('<' != *ptr) {
      if(display) {
        show(("=> %s", buffer));
        string = appendstring(string, buffer, &stringlen, &stralloc);
        show(("* %s\n", buffer));
      }
      continue;
    }

    ptr++;
    EAT_SPACE(ptr);

    if('/' == *ptr) {
      /* end of a section */
      ptr++;
      EAT_SPACE(ptr);

      end = ptr;
      EAT_WORD(end);
      *end = 0;

      if((state == STATE_INSUB) &&
         !strcmp(csub, ptr)) {
        /* this is the end of the currently read sub section */
        state--;
        csub[0]=0; /* no sub anymore */
        display=0;
      }
      else if((state == STATE_INMAIN) &&
              !strcmp(cmain, ptr)) {
        /* this is the end of the currently read main section */
        state--;
        cmain[0]=0; /* no main anymore */
        display=0;
      }
    }
    else if(!display) {
      /* this is the beginning of a section */
      end = ptr;
      EAT_WORD(end);
      
      *end = 0;
      switch(state) {
      case STATE_OUTSIDE:
        strcpy(cmain, ptr);
        state = STATE_INMAIN;
        break;
      case STATE_INMAIN:
        strcpy(csub, ptr);
        state = STATE_INSUB;
        break;
      default:
        break;
      }
    }
    if(display) {
      string = appendstring(string, buffer, &stringlen, &stralloc);
      show(("* %s\n", buffer));
    }

    if((STATE_INSUB == state) &&
       !strcmp(cmain, main) &&
       !strcmp(csub, sub)) {
      show(("* (%d bytes) %s\n", stringlen, buffer));
      display = 1; /* start displaying */
    }
    else {
      show(("%d (%s/%s): %s\n", state, cmain, csub, buffer));
      display = 0; /* no display */
    }
  }

  *size = stringlen;
  return string;
}

#ifdef TEST
int main(int argc, char **argv)
{
  if(argc< 3) {
    printf("./moo main sub\n");
  }
  else {
    int size;
    char *buffer = spitout(stdin, argv[1], argv[2], &size);
  }
  return 0;
}
#endif
