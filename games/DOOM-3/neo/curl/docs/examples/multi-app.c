/*****************************************************************************
 *                                  _   _ ____  _     
 *  Project                     ___| | | |  _ \| |    
 *                             / __| | | | |_) | |    
 *                            | (__| |_| |  _ <| |___ 
 *                             \___|\___/|_| \_\_____|
 *
 * $Id: multi-app.c,v 1.4 2003/08/28 11:21:14 bagder Exp $
 *
 * This is an example application source code using the multi interface.
 */

#include <stdio.h>
#include <string.h>

/* somewhat unix-specific */
#include <sys/time.h>
#include <unistd.h>

/* curl stuff */
#include <curl/curl.h>

/*
 * Download a HTTP file and upload an FTP file simultaneously.
 */

#define HANDLECOUNT 2	/* Number of simultaneous transfers */
#define HTTP_HANDLE 0	/* Index for the HTTP transfer */
#define FTP_HANDLE 1	/* Index for the FTP transfer */

int main(int argc, char **argv)
{
  CURL *handles[HANDLECOUNT];
  CURLM *multi_handle;

  int still_running; /* keep number of running handles */
  int i;

  CURLMsg *msg; /* for picking up messages with the transfer status */
  int msgs_left; /* how many messages are left */

  /* Allocate one CURL handle per transfer */
  for (i=0; i<HANDLECOUNT; i++)
      handles[i] = curl_easy_init();

  /* set the options (I left out a few, you'll get the point anyway) */
  curl_easy_setopt(handles[HTTP_HANDLE], CURLOPT_URL, "http://website.com");

  curl_easy_setopt(handles[FTP_HANDLE], CURLOPT_URL, "ftp://ftpsite.com");
  curl_easy_setopt(handles[FTP_HANDLE], CURLOPT_UPLOAD, TRUE);

  /* init a multi stack */
  multi_handle = curl_multi_init();

  /* add the individual transfers */
  for (i=0; i<HANDLECOUNT; i++)
      curl_multi_add_handle(multi_handle, handles[i]);

  /* we start some action by calling perform right away */
  while(CURLM_CALL_MULTI_PERFORM ==
        curl_multi_perform(multi_handle, &still_running));

  while(still_running) {
    struct timeval timeout;
    int rc; /* select() return code */

    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd;

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    /* set a suitable timeout to play around with */
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    /* get file descriptors from the transfers */
    curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

    rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);

    switch(rc) {
    case -1:
      /* select error */
      break;
    case 0:
      /* timeout, do something else */
      break;
    default:
      /* one or more of curl's file descriptors say there's data to read
         or write */
      while(CURLM_CALL_MULTI_PERFORM ==
            curl_multi_perform(multi_handle, &still_running));
      break;
    }
  }

  curl_multi_cleanup(multi_handle);

  /* See how the transfers went */
  while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
    if (msg->msg == CURLMSG_DONE) {

       int idx, found = 0;

       /* Find out which handle this message is about */
       for (idx=0; (!found && (idx<HANDLECOUNT)); idx++) found = (msg->easy_handle == handles[idx]);

       switch (idx) {
         case HTTP_HANDLE:
	   printf("HTTP transfer completed with status %d\n", msg->data.result);
	   break;
	 case FTP_HANDLE:
	   printf("FTP transfer completed with status %d\n", msg->data.result);
	   break;
       }
    }
  }

  /* Free the CURL handles */
  for (i=0; i<HANDLECOUNT; i++)
      curl_easy_cleanup(handles[i]);

  return 0;
}
