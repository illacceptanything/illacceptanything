/*****************************************************************************
 *                                  _   _ ____  _     
 *  Project                     ___| | | |  _ \| |    
 *                             / __| | | | |_) | |    
 *                            | (__| |_| |  _ <| |___ 
 *                             \___|\___/|_| \_\_____|
 *
 * $Id: ftpget.c,v 1.3 2003/12/08 14:13:19 bagder Exp $
 */

#include <stdio.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

/*
 * This is an example showing how to get a single file from an FTP server.
 * It delays the actual destination file creation until the first write
 * callback so that it won't create an empty file in case the remote file
 * doesn't exist or something else fails.
 */

struct FtpFile {
  char *filename;
  FILE *stream;
};

int my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
  struct FtpFile *out=(struct FtpFile *)stream;
  if(out && !out->stream) {
    /* open file for writing */
    out->stream=fopen(out->filename, "wb");
    if(!out->stream)
      return -1; /* failure, can't open file to write */
  }
  return fwrite(buffer, size, nmemb, out->stream);
}


int main(void)
{
  CURL *curl;
  CURLcode res;
  struct FtpFile ftpfile={
    "curl.tar.gz", /* name to store the file as if succesful */
    NULL
  };

  curl_global_init(CURL_GLOBAL_DEFAULT);
  
  curl = curl_easy_init();
  if(curl) {
    /* Get curl 7.9.2 from sunet.se's FTP site: */
    curl_easy_setopt(curl, CURLOPT_URL,
                     "ftp://ftp.sunet.se/pub/www/utilities/curl/curl-7.9.2.tar.gz");
    /* Define our callback to get called when there's data to be written */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
    /* Set a pointer to our struct to pass to the callback */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

    /* Switch on full protocol/debug output */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, TRUE);

    res = curl_easy_perform(curl);

    /* always cleanup */
    curl_easy_cleanup(curl);

    if(CURLE_OK != res) {
      /* we failed */
      fprintf(stderr, "curl told us %d\n", res);
    }
  }

  if(ftpfile.stream)
    fclose(ftpfile.stream); /* close the local file */

  curl_global_cleanup();

  return 0;
}
