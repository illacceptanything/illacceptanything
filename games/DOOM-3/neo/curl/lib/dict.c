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
 * $Id: dict.c,v 1.34 2004/03/09 22:52:50 bagder Exp $
 ***************************************************************************/

#include "setup.h"

/* -- WIN32 approved -- */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>

#if defined(WIN32) && !defined(__GNUC__) || defined(__MINGW32__)
#include <time.h>
#include <io.h>
#else
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <netinet/in.h>
#include <sys/time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <netdb.h>
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#include <sys/ioctl.h>
#include <signal.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif


#endif

#include "urldata.h"
#include <curl/curl.h>
#include "transfer.h"
#include "sendf.h"

#include "progress.h"
#include "strequal.h"
#include "dict.h"

#define _MPRINTF_REPLACE /* use our functions only */
#include <curl/mprintf.h>

CURLcode Curl_dict(struct connectdata *conn)
{
  char *word;
  char *ppath;
  char *database = NULL;
  char *strategy = NULL;
  char *nthdef = NULL; /* This is not part of the protocol, but required
                          by RFC 2229 */
  CURLcode result=CURLE_OK;
  struct SessionHandle *data=conn->data;
  curl_socket_t sockfd = conn->sock[FIRSTSOCKET];

  char *path = conn->path;
  curl_off_t *bytecount = &conn->bytecount;

  if(conn->bits.user_passwd) {
    /* AUTH is missing */
  }

  if (strnequal(path, DICT_MATCH, sizeof(DICT_MATCH)-1) ||
      strnequal(path, DICT_MATCH2, sizeof(DICT_MATCH2)-1) ||
      strnequal(path, DICT_MATCH3, sizeof(DICT_MATCH3)-1)) {
      
    word = strchr(path, ':');
    if (word) {
      word++;
      database = strchr(word, ':');
      if (database) {
        *database++ = (char)0;
        strategy = strchr(database, ':');
        if (strategy) {
          *strategy++ = (char)0;
          nthdef = strchr(strategy, ':');
          if (nthdef) {
            *nthdef++ = (char)0;
          }
        }
      }
    }
      
    if ((word == NULL) || (*word == (char)0)) {
      failf(data, "lookup word is missing");
    }
    if ((database == NULL) || (*database == (char)0)) {
      database = (char *)"!";
    }
    if ((strategy == NULL) || (*strategy == (char)0)) {
      strategy = (char *)".";
    }
      
    result = Curl_sendf(sockfd, conn,
                        "CLIENT " LIBCURL_NAME " " LIBCURL_VERSION "\n"
                        "MATCH "
                        "%s "    /* database */
                        "%s "    /* strategy */
                        "%s\n"   /* word */
                        "QUIT\n",
                        
                        database,
                        strategy,
                        word
                        );
    if(result)
      failf(data, "Failed sending DICT request");
    else
      result = Curl_Transfer(conn, FIRSTSOCKET, -1, FALSE, bytecount,
                             -1, NULL); /* no upload */      
    if(result)
      return result;
  }
  else if (strnequal(path, DICT_DEFINE, sizeof(DICT_DEFINE)-1) ||
           strnequal(path, DICT_DEFINE2, sizeof(DICT_DEFINE2)-1) ||
           strnequal(path, DICT_DEFINE3, sizeof(DICT_DEFINE3)-1)) {
    
    word = strchr(path, ':');
    if (word) {
      word++;
      database = strchr(word, ':');
      if (database) {
        *database++ = (char)0;
        nthdef = strchr(database, ':');
        if (nthdef) {
          *nthdef++ = (char)0;
        }
      }
    }
      
    if ((word == NULL) || (*word == (char)0)) {
      failf(data, "lookup word is missing");
    }
    if ((database == NULL) || (*database == (char)0)) {
      database = (char *)"!";
    }
      
    result = Curl_sendf(sockfd, conn,
                        "CLIENT " LIBCURL_NAME " " LIBCURL_VERSION "\n"
                        "DEFINE "
                        "%s "     /* database */
                        "%s\n"    /* word */
                        "QUIT\n",
                        database,
                        word);
    if(result)
      failf(data, "Failed sending DICT request");
    else
      result = Curl_Transfer(conn, FIRSTSOCKET, -1, FALSE, bytecount,
                             -1, NULL); /* no upload */
    
    if(result)
      return result;
      
  }
  else {
      
    ppath = strchr(path, '/');
    if (ppath) {
      int i;
	
      ppath++;
      for (i = 0; ppath[i]; i++) {
        if (ppath[i] == ':')
          ppath[i] = ' ';
      }
      result = Curl_sendf(sockfd, conn,
                          "CLIENT " LIBCURL_NAME " " LIBCURL_VERSION "\n"
                          "%s\n"
                          "QUIT\n", ppath);
      if(result)
        failf(data, "Failed sending DICT request");
      else
        result = Curl_Transfer(conn, FIRSTSOCKET, -1, FALSE, bytecount,
                               -1, NULL);
      if(result)
        return result;
    }
  }

  return CURLE_OK;
}
