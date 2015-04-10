/*****************************************************************************
 *                                  _   _ ____  _     
 *  Project                     ___| | | |  _ \| |    
 *                             / __| | | | |_) | |    
 *                            | (__| |_| |  _ <| |___ 
 *                             \___|\___/|_| \_\_____|
 *
 * $Id: post-callback.c,v 1.3 2003/12/08 14:14:26 bagder Exp $
 *
 * An example source code that issues a HTTP POST and we provide the actual
 * data through a read callback.
 *
 */
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

char data[]="this is what we post to the silly web server";

struct WriteThis {
  char *readptr;
  int sizeleft;
};

size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
  struct WriteThis *pooh = (struct WriteThis *)userp;

  if(size*nmemb < 1)
    return 0;

  if(pooh->sizeleft) {
    *(char *)ptr = pooh->readptr[0]; /* copy one single byte */
    pooh->readptr++;                 /* advance pointer */
    pooh->sizeleft--;                /* less data left */
    return 1;                        /* we return 1 byte at a time! */
  }

  return -1;                         /* no more data left to deliver */
}

int main(void)
{
  CURL *curl;
  CURLcode res;

  struct WriteThis pooh;

  pooh.readptr = data;
  pooh.sizeleft = strlen(data);

  curl = curl_easy_init();
  if(curl) {
    /* First set the URL that is about to receive our POST. */
    curl_easy_setopt(curl, CURLOPT_URL,
                     "http://receivingsite.com.pooh/index.cgi");
    /* Now specify we want to POST data */
    curl_easy_setopt(curl, CURLOPT_POST, TRUE);

    /* Set the expected POST size */
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, pooh.sizeleft);

    /* we want to use our own read function */
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

    /* pointer to pass to our read function */
    curl_easy_setopt(curl, CURLOPT_READDATA, &pooh);

    /* get verbose debug output please */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);

    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  return 0;
}
