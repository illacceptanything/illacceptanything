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
 * $Id: writeenv.c,v 1.7 2004/01/07 09:19:36 bagder Exp $
 ***************************************************************************/

#include "setup.h"

#ifdef USE_ENVIRONMENT

#include <curl/curl.h>

#ifdef __riscos__
#include <kernel.h>
#endif

struct
{
  const char * name;
  CURLINFO id;
  enum {
    writeenv_NONE,
    writeenv_DOUBLE,
    writeenv_LONG,
    writeenv_STRING
  } type;
} variables[14] =
{
  {"curl_url_effective", CURLINFO_EFFECTIVE_URL, writeenv_STRING},
  {"curl_http_code", CURLINFO_RESPONSE_CODE, writeenv_LONG},
  {"curl_time_total", CURLINFO_TOTAL_TIME, writeenv_DOUBLE},
  {"curl_time_namelookup", CURLINFO_NAMELOOKUP_TIME, writeenv_DOUBLE},
  {"curl_time_connect", CURLINFO_CONNECT_TIME, writeenv_DOUBLE},
  {"curl_time_pretransfer", CURLINFO_PRETRANSFER_TIME, writeenv_DOUBLE},
  {"curl_time_starttransfer", CURLINFO_STARTTRANSFER_TIME, writeenv_DOUBLE},
  {"curl_size_header", CURLINFO_HEADER_SIZE, writeenv_LONG},
  {"curl_size_request", CURLINFO_REQUEST_SIZE, writeenv_LONG},
  {"curl_size_download", CURLINFO_SIZE_DOWNLOAD, writeenv_DOUBLE},
  {"curl_size_upload", CURLINFO_SIZE_UPLOAD, writeenv_DOUBLE},
  {"curl_speed_download", CURLINFO_SPEED_DOWNLOAD, writeenv_DOUBLE},
  {"curl_speed_upload", CURLINFO_SPEED_UPLOAD, writeenv_DOUBLE},
  {NULL, 0, writeenv_NONE}
 };

static void internalSetEnv(const char * name, char * value)
{
  /* Add your OS-specific code here. */
#ifdef __riscos__
  _kernel_setenv(name, value);
#endif
  return;
}

void ourWriteEnv(CURL *curl)
{
  unsigned int i;
  char *string, numtext[10];
  long longinfo;
  double doubleinfo;
  
  for (i=0; variables[i].name; i++) {
    switch (variables[i].type) {
    case writeenv_STRING:
      if (curl_easy_getinfo(curl, variables[i].id, &string) == CURLE_OK)
        internalSetEnv(variables[i].name, string);
      else
        internalSetEnv(variables[i].name, NULL);
      break;

    case writeenv_LONG:
      if (curl_easy_getinfo(curl, variables[i].id, &longinfo) == CURLE_OK) {
        sprintf(numtext, "%5ld", longinfo);
        internalSetEnv(variables[i].name, numtext);
      }
      else
        internalSetEnv(variables[i].name, NULL);
      break;
    case writeenv_DOUBLE:
      if (curl_easy_getinfo(curl, variables[i].id, &doubleinfo) == CURLE_OK) {
        sprintf(numtext, "%6.2f", doubleinfo);
        internalSetEnv(variables[i].name, numtext);
      }
      else
        internalSetEnv(variables[i].name, NULL);
      break;
    }
  }

  return;
}

#endif
