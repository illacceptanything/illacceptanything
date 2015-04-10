/*****************************************************************************
 *                                  _   _ ____  _     
 *  Project                     ___| | | |  _ \| |    
 *                             / __| | | | |_) | |    
 *                            | (__| |_| |  _ <| |___ 
 *                             \___|\___/|_| \_\_____|
 *
 * $Id: persistant.c,v 1.2 2003/11/19 08:20:13 bagder Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <curl/curl.h>

int main(int argc, char **argv)
{
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_ALL);

  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1);

    /* get the first document */
    curl_easy_setopt(curl, CURLOPT_URL, "http://curl.haxx.se/");
    res = curl_easy_perform(curl);

    /* get another document from the same server using the same
       connection */
    curl_easy_setopt(curl, CURLOPT_URL, "http://curl.haxx.se/docs/");
    res = curl_easy_perform(curl);

    /* always cleanup */
    curl_easy_cleanup(curl);
  }

  return 0;
}
