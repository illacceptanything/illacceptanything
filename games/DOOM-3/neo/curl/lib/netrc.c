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
 * $Id: netrc.c,v 1.30 2004/02/19 09:22:00 bagder Exp $
 ***************************************************************************/

#include "setup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef VMS
#include <unixlib.h>
#endif

#include <curl/curl.h>
#include "netrc.h"

#include "strequal.h"
#include "strtok.h"

#define _MPRINTF_REPLACE /* use our functions only */
#include <curl/mprintf.h>

/* The last #include file should be: */
#ifdef CURLDEBUG
#include "memdebug.h"
#endif

/* Debug this single source file with:
   'make netrc' then run './netrc'!

   Oh, make sure you have a .netrc file too ;-)
 */

/* Get user and password from .netrc when given a machine name */

enum {
  NOTHING,
  HOSTFOUND,    /* the 'machine' keyword was found */
  HOSTCOMPLETE, /* the machine name following the keyword was found too */
  HOSTVALID,    /* this is "our" machine! */

  HOSTEND /* LAST enum */
};

/* make sure we have room for at least this size: */
#define LOGINSIZE 64
#define PASSWORDSIZE 64

/* returns -1 on failure, 0 if the host is found, 1 is the host isn't found */
int Curl_parsenetrc(char *host,
                    char *login,
                    char *password,
                    char *netrcfile)
{
  FILE *file;
  int retcode=1;
  int specific_login = (login[0] != 0);
  char *home = NULL; 
  bool home_alloc = FALSE;
  bool netrc_alloc = FALSE;
  int state=NOTHING;

  char state_login=0;      /* Found a login keyword */
  char state_password=0;   /* Found a password keyword */
  char state_our_login=0;  /* With specific_login, found *our* login name */

#define NETRC DOT_CHAR "netrc"

#ifdef CURLDEBUG
  {
    /* This is a hack to allow testing.
     * If compiled with --enable-debug and CURL_DEBUG_NETRC is defined,
     * then it's the path to a substitute .netrc for testing purposes *only* */

    char *override = curl_getenv("CURL_DEBUG_NETRC");

    if (override) {
      printf("NETRC: overridden " NETRC " file: %s\n", home);
      netrcfile = override;
      netrc_alloc = TRUE;
    }
  }
#endif /* CURLDEBUG */
  if(!netrcfile) {
    home = curl_getenv("HOME"); /* portable environment reader */
    if(home) {
      home_alloc = TRUE;
#if defined(HAVE_GETPWUID) && defined(HAVE_GETEUID)
    }
    else {
      struct passwd *pw;
      pw= getpwuid(geteuid());
      if (pw) {
#ifdef	VMS
        home = decc$translate_vms(pw->pw_dir);
#else
        home = pw->pw_dir;
#endif
      }
#endif
    }

    if(!home)
      return -1;

    netrcfile = curl_maprintf("%s%s%s", home, DIR_CHAR, NETRC);
    if(!netrcfile) {
      if(home_alloc)
        free(home);
      return -1;
    }
    netrc_alloc = TRUE;
  }

  file = fopen(netrcfile, "r");
  if(file) {
    char *tok;
    char *tok_buf;
    bool done=FALSE;
    char netrcbuffer[256];

    while(!done && fgets(netrcbuffer, sizeof(netrcbuffer), file)) {
      tok=strtok_r(netrcbuffer, " \t\n", &tok_buf);
      while(!done && tok) {

        if (login[0] && password[0]) {
          done=TRUE;
          break;
        }

	switch(state) {
	case NOTHING:
	  if(strequal("machine", tok)) {
	    /* the next tok is the machine name, this is in itself the
	       delimiter that starts the stuff entered for this machine,
	       after this we need to search for 'login' and
	       'password'. */
	    state=HOSTFOUND;
	  }
	  break;
	case HOSTFOUND:
	  if(strequal(host, tok)) {
	    /* and yes, this is our host! */
	    state=HOSTVALID;
#ifdef _NETRC_DEBUG
	    printf("HOST: %s\n", tok);
#endif
	    retcode=0; /* we did find our host */
	  }
	  else
	    /* not our host */
	    state=NOTHING;
	  break;
	case HOSTVALID:
	  /* we are now parsing sub-keywords concerning "our" host */
	  if(state_login) {
            if (specific_login) {
              state_our_login = strequal(login, tok);
            }
            else {
              strncpy(login, tok, LOGINSIZE-1);
#ifdef _NETRC_DEBUG
	      printf("LOGIN: %s\n", login);
#endif
            }
	    state_login=0;
	  }
	  else if(state_password) {
            if (state_our_login || !specific_login) {
              strncpy(password, tok, PASSWORDSIZE-1);
#ifdef _NETRC_DEBUG
              printf("PASSWORD: %s\n", password);
#endif
            }
	    state_password=0;
	  }
	  else if(strequal("login", tok))
	    state_login=1;
	  else if(strequal("password", tok))
	    state_password=1;
	  else if(strequal("machine", tok)) {
	    /* ok, there's machine here go => */
	    state = HOSTFOUND;
            state_our_login = 0;
	  }
	  break;
	} /* switch (state) */

	tok = strtok_r(NULL, " \t\n", &tok_buf);
      } /* while (tok) */
    } /* while fgets() */

    fclose(file);
  }

  if(home_alloc)
    free(home);
  if(netrc_alloc)
    free(netrcfile);

  return retcode;
}

#ifdef _NETRC_DEBUG
int main(int argc, char **argv)
{
  char login[64]="";
  char password[64]="";

  if(argc<2)
    return -1;

  if(0 == ParseNetrc(argv[1], login, password)) {
    printf("HOST: %s LOGIN: %s PASSWORD: %s\n",
	   argv[1], login, password);
  }
}

#endif
