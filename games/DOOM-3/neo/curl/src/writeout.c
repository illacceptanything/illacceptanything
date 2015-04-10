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
 * $Id: writeout.c,v 1.21 2004/02/25 15:41:36 bagder Exp $
 ***************************************************************************/

#include "setup.h"

#include <stdio.h>
#include <string.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <curl/curl.h>

#define _MPRINTF_REPLACE /* we want curl-functions instead of native ones */
#include <curl/mprintf.h>

#include "writeout.h"

typedef enum {
  VAR_NONE,       /* must be the first */
  VAR_TOTAL_TIME,
  VAR_NAMELOOKUP_TIME,
  VAR_CONNECT_TIME,
  VAR_PRETRANSFER_TIME,
  VAR_STARTTRANSFER_TIME,
  VAR_SIZE_DOWNLOAD,
  VAR_SIZE_UPLOAD,
  VAR_SPEED_DOWNLOAD,
  VAR_SPEED_UPLOAD,
  VAR_HTTP_CODE,
  VAR_HEADER_SIZE,
  VAR_REQUEST_SIZE,
  VAR_EFFECTIVE_URL,
  VAR_CONTENT_TYPE,
  VAR_NUM_OF_VARS /* must be the last */
} replaceid;

struct variable {
  const char *name;
  replaceid id;
};


static struct variable replacements[]={
  {"url_effective", VAR_EFFECTIVE_URL},
  {"http_code", VAR_HTTP_CODE},
  {"time_total", VAR_TOTAL_TIME},
  {"time_namelookup", VAR_NAMELOOKUP_TIME},
  {"time_connect", VAR_CONNECT_TIME},
  {"time_pretransfer", VAR_PRETRANSFER_TIME},
  {"time_starttransfer", VAR_STARTTRANSFER_TIME},
  {"size_header", VAR_HEADER_SIZE},
  {"size_request", VAR_REQUEST_SIZE},
  {"size_download", VAR_SIZE_DOWNLOAD},
  {"size_upload", VAR_SIZE_UPLOAD},
  {"speed_download", VAR_SPEED_DOWNLOAD},
  {"speed_upload", VAR_SPEED_UPLOAD},
  {"content_type", VAR_CONTENT_TYPE},
  {NULL, VAR_NONE}
};

void ourWriteOut(CURL *curl, char *writeinfo)
{
  FILE *stream = stdout;
  char *ptr=writeinfo;
  char *stringp;
  long longinfo;
  double doubleinfo;

  while(*ptr) {
    if('%' == *ptr) {
      if('%' == ptr[1]) {
        /* an escaped %-letter */
        fputc('%', stream);
        ptr+=2;
      }
      else {
        /* this is meant as a variable to output */
        char *end;
        char keepit;
        int i;
        if(('{' == ptr[1]) && (end=strchr(ptr, '}'))) {
          ptr+=2; /* pass the % and the { */
          keepit=*end;
          *end=0; /* zero terminate */
          for(i=0; replacements[i].name; i++) {
            if(curl_strequal(ptr, replacements[i].name)) {
              switch(replacements[i].id) {
              case VAR_EFFECTIVE_URL:
                if((CURLE_OK ==
                    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &stringp))
                   && stringp)
                  fputs(stringp, stream);
                break;
              case VAR_HTTP_CODE:
                if(CURLE_OK ==
                   curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &longinfo))
                  fprintf(stream, "%03ld", longinfo);
                break;
              case VAR_HEADER_SIZE:
                if(CURLE_OK ==
                   curl_easy_getinfo(curl, CURLINFO_HEADER_SIZE, &longinfo))
                  fprintf(stream, "%ld", longinfo);
                break;
              case VAR_REQUEST_SIZE:
                if(CURLE_OK ==
                   curl_easy_getinfo(curl, CURLINFO_REQUEST_SIZE, &longinfo))
                  fprintf(stream, "%ld", longinfo);
                break;
              case VAR_TOTAL_TIME:
                if(CURLE_OK ==
                   curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &doubleinfo))
                  fprintf(stream, "%.3f", doubleinfo);
                break;
              case VAR_NAMELOOKUP_TIME:
                if(CURLE_OK ==
                   curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME,
                                     &doubleinfo))
                  fprintf(stream, "%.3f", doubleinfo);
                break;
              case VAR_CONNECT_TIME:
                if(CURLE_OK ==
                   curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &doubleinfo))
                  fprintf(stream, "%.3f", doubleinfo);
                break;
              case VAR_PRETRANSFER_TIME:
                if(CURLE_OK ==
                   curl_easy_getinfo(curl, CURLINFO_PRETRANSFER_TIME, &doubleinfo))
                  fprintf(stream, "%.3f", doubleinfo);
                break;
              case VAR_STARTTRANSFER_TIME:
                if(CURLE_OK ==
                   curl_easy_getinfo(curl, CURLINFO_STARTTRANSFER_TIME, &doubleinfo))
                  fprintf(stream, "%.3f", doubleinfo);
                break;
              case VAR_SIZE_UPLOAD:
                if(CURLE_OK ==
                   curl_easy_getinfo(curl, CURLINFO_SIZE_UPLOAD, &doubleinfo))
                  fprintf(stream, "%.0f", doubleinfo);
                break;
              case VAR_SIZE_DOWNLOAD:
                if(CURLE_OK ==
                   curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &doubleinfo))
                  fprintf(stream, "%.0f", doubleinfo);
                break;
              case VAR_SPEED_DOWNLOAD:
                if(CURLE_OK ==
                   curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &doubleinfo))
                  fprintf(stream, "%.3f", doubleinfo);
                break;
              case VAR_SPEED_UPLOAD:
                if(CURLE_OK ==
                   curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &doubleinfo))
                  fprintf(stream, "%.3f", doubleinfo);
                break;
              case VAR_CONTENT_TYPE:
                if((CURLE_OK ==
                    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &stringp))
                   && stringp)
                  fputs(stringp, stream);
                break;
              default:
                /* -Wunreachable-code wrongly complains on this */
                break;
              }
              break;
            }
          }
          ptr=end+1; /* pass the end */
          *end = keepit;
        }
        else {
          /* illegal syntax, then just output the characters that are used */
          fputc('%', stream);
          fputc(ptr[1], stream);
          ptr+=2;
        }
      }
    }
    else if('\\' == *ptr) {
      switch(ptr[1]) {
      case 'r':
        fputc('\r', stream);
        break;
      case 'n':
        fputc('\n', stream);
        break;
      case 't':
        fputc('\t', stream);
        break;
      default:
        /* unknown, just output this */
        fputc(*ptr, stream);
        fputc(ptr[1], stream);
        break;
      }
      ptr+=2;
    }
    else {
      fputc(*ptr, stream);
      ptr++;
    }
  }
  
}
