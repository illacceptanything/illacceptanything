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
 * $Id: sendf.c,v 1.80 2004/03/10 09:50:12 bagder Exp $
 ***************************************************************************/

#include "setup.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>	/* required for send() & recv() prototypes */
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <curl/curl.h>
#include "urldata.h"
#include "sendf.h"
#include "connect.h" /* for the Curl_ourerrno() proto */

#define _MPRINTF_REPLACE /* use the internal *printf() functions */
#include <curl/mprintf.h>

#ifdef HAVE_KRB4
#include "security.h"
#endif
#include <string.h>
/* The last #include file should be: */
#ifdef CURLDEBUG
#include "memdebug.h"
#endif

/* returns last node in linked list */
static struct curl_slist *slist_get_last(struct curl_slist *list)
{
  struct curl_slist	*item;

  /* if caller passed us a NULL, return now */
  if (!list)
    return NULL;

  /* loop through to find the last item */
  item = list;
  while (item->next) {
    item = item->next;
  }
  return item;
}

/* append a struct to the linked list. It always retunrs the address of the
 * first record, so that you can sure this function as an initialization
 * function as well as an append function. If you find this bothersome,
 * then simply create a separate _init function and call it appropriately from
 * within the proram. */
struct curl_slist *curl_slist_append(struct curl_slist *list,
                                     const char *data)
{
  struct curl_slist	*last;
  struct curl_slist	*new_item;

  new_item = (struct curl_slist *) malloc(sizeof(struct curl_slist));
  if (new_item) {
    new_item->next = NULL;
    new_item->data = strdup(data);
  }
  if (new_item == NULL || new_item->data == NULL) {
    return NULL;
  }

  if (list) {
    last = slist_get_last(list);
    last->next = new_item;
    return list;
  }

  /* if this is the first item, then new_item *is* the list */
  return new_item;
}

/* be nice and clean up resources */
void curl_slist_free_all(struct curl_slist *list)
{
  struct curl_slist	*next;
  struct curl_slist	*item;

  if (!list)
    return;

  item = list;
  do {
    next = item->next;
		
    if (item->data) {
      free(item->data);
    }
    free(item);
    item = next;
  } while (next);
}

/* Curl_infof() is for info message along the way */

void Curl_infof(struct SessionHandle *data, const char *fmt, ...)
{
  if(data && data->set.verbose) {
    va_list ap;
    char print_buffer[1024 + 1];
    va_start(ap, fmt);
    vsnprintf(print_buffer, 1024, fmt, ap);
    va_end(ap);
    Curl_debug(data, CURLINFO_TEXT, print_buffer, strlen(print_buffer));
  }
}

/* Curl_failf() is for messages stating why we failed.
 * The message SHALL NOT include any LF or CR.
 */

void Curl_failf(struct SessionHandle *data, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  if(data->set.errorbuffer && !data->state.errorbuf) {
    vsnprintf(data->set.errorbuffer, CURL_ERROR_SIZE, fmt, ap);
    data->state.errorbuf = TRUE; /* wrote error string */

    if(data->set.verbose) {
      size_t len = strlen(data->set.errorbuffer);
      bool doneit=FALSE;
      if(len < CURL_ERROR_SIZE - 1) {
        doneit = TRUE;
        data->set.errorbuffer[len] = '\n';
        data->set.errorbuffer[++len] = '\0';
      }
      Curl_debug(data, CURLINFO_TEXT, data->set.errorbuffer, len);
      if(doneit)
        /* cut off the newline again */
        data->set.errorbuffer[--len]=0;
    }
  }
  va_end(ap);
}

/* Curl_sendf() sends formated data to the server */
CURLcode Curl_sendf(curl_socket_t sockfd, struct connectdata *conn,
                    const char *fmt, ...)
{
  struct SessionHandle *data = conn->data;
  ssize_t bytes_written;
  size_t write_len;
  CURLcode res;
  char *s;
  char *sptr;
  va_list ap;
  va_start(ap, fmt);
  s = vaprintf(fmt, ap); /* returns an allocated string */
  va_end(ap);
  if(!s)
    return CURLE_OUT_OF_MEMORY; /* failure */

  bytes_written=0;
  write_len = strlen(s);
  sptr = s;

  while (1) {
    /* Write the buffer to the socket */
    res = Curl_write(conn, sockfd, sptr, write_len, &bytes_written);

    if(CURLE_OK != res)
      break;

    if(data->set.verbose)
      Curl_debug(data, CURLINFO_DATA_OUT, sptr, bytes_written);

    if((size_t)bytes_written != write_len) {
      /* if not all was written at once, we must advance the pointer, decrease
         the size left and try again! */
      write_len -= bytes_written;
      sptr += bytes_written;
    }
    else
      break;
  }

  free(s); /* free the output string */

  return res;
}

/*
 * Curl_write() is an internal write function that sends plain (binary) data
 * to the server. Works with plain sockets, SSL or kerberos.
 */
CURLcode Curl_write(struct connectdata *conn,
                    curl_socket_t sockfd,
                    void *mem,
                    size_t len,
                    ssize_t *written)
{
  ssize_t bytes_written;
  CURLcode retcode;

#ifdef USE_SSLEAY
  /* Set 'num' to 0 or 1, depending on which socket that has been sent here.
     If it is the second socket, we set num to 1. Otherwise to 0. This lets
     us use the correct ssl handle. */
  int num = (sockfd == conn->sock[SECONDARYSOCKET]);
  /* SSL_write() is said to return 'int' while write() and send() returns
     'size_t' */
  if (conn->ssl[num].use) {
    int err;
    char error_buffer[120]; /* OpenSSL documents that this must be at least
                               120 bytes long. */
    int sslerror;
    int rc = SSL_write(conn->ssl[num].handle, mem, len);

    if(rc < 0) {
      err = SSL_get_error(conn->ssl[num].handle, rc);
    
      switch(err) {
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        /* The operation did not complete; the same TLS/SSL I/O function
           should be called again later. This is basicly an EWOULDBLOCK
           equivalent. */
        *written = 0;
        return CURLE_OK;
      case SSL_ERROR_SYSCALL:
        failf(conn->data, "SSL_write() returned SYSCALL, errno = %d\n",
              Curl_ourerrno());
        return CURLE_SEND_ERROR;
      case SSL_ERROR_SSL:
        /*  A failure in the SSL library occurred, usually a protocol error.
            The OpenSSL error queue contains more information on the error. */
        sslerror = ERR_get_error();
        failf(conn->data, "SSL_write() error: %s\n",
              ERR_error_string(sslerror, error_buffer));
        return CURLE_SEND_ERROR;
      }
      /* a true error */
      failf(conn->data, "SSL_write() return error %d\n", err);
      return CURLE_SEND_ERROR;
    }
    bytes_written = rc;
  }
  else {
#else
  (void)conn;
#endif
#ifdef HAVE_KRB4
    if(conn->sec_complete) {
      bytes_written = Curl_sec_write(conn, sockfd, mem, len);
    }
    else
#endif /* HAVE_KRB4 */
    {
      bytes_written = (ssize_t)swrite(sockfd, mem, len);
    }
    if(-1 == bytes_written) {
      int err = Curl_ourerrno();

      if(
#ifdef WSAEWOULDBLOCK
        /* This is how Windows does it */
        (WSAEWOULDBLOCK == err)
#else
        /* As pointed out by Christophe Demory on March 11 2003, errno
           may be EWOULDBLOCK or on some systems EAGAIN when it returned
           due to its inability to send off data without blocking. We
           therefor treat both error codes the same here */
        (EWOULDBLOCK == err) || (EAGAIN == err) || (EINTR == err)
#endif
        )
        /* this is just a case of EWOULDBLOCK */
        bytes_written=0;
    }
#ifdef USE_SSLEAY
  }
#endif

  *written = bytes_written;
  retcode = (-1 != bytes_written)?CURLE_OK:CURLE_SEND_ERROR;

  return retcode;
}

/* client_write() sends data to the write callback(s)

   The bit pattern defines to what "streams" to write to. Body and/or header.
   The defines are in sendf.h of course.
 */
CURLcode Curl_client_write(struct SessionHandle *data,
                           int type,
                           char *ptr,
                           size_t len)
{
  size_t wrote;

  if(0 == len)
    len = strlen(ptr);

  if(type & CLIENTWRITE_BODY) {
    wrote = data->set.fwrite(ptr, 1, len, data->set.out);
    if(wrote != len) {
      failf (data, "Failed writing body");
      return CURLE_WRITE_ERROR;
    }
  }
  if((type & CLIENTWRITE_HEADER) &&
     (data->set.fwrite_header || data->set.writeheader) ) {
    /*
     * Write headers to the same callback or to the especially setup
     * header callback function (added after version 7.7.1).
     */
    curl_write_callback writeit=
      data->set.fwrite_header?data->set.fwrite_header:data->set.fwrite;

    wrote = writeit(ptr, 1, len, data->set.writeheader);
    if(wrote != len) {
      failf (data, "Failed writing header");
      return CURLE_WRITE_ERROR;
    }
  }
  
  return CURLE_OK;
}

/*
 * Internal read-from-socket function. This is meant to deal with plain
 * sockets, SSL sockets and kerberos sockets.
 *
 * If the read would block (EWOULDBLOCK) we return -1. Otherwise we return
 * a regular CURLcode value.
 */
int Curl_read(struct connectdata *conn, /* connection data */
              curl_socket_t sockfd,     /* read from this socket */
              char *buf,                /* store read data here */
              size_t buffersize,        /* max amount to read */
              ssize_t *n)               /* amount bytes read */
{
  ssize_t nread;
#ifdef USE_SSLEAY
  /* Set 'num' to 0 or 1, depending on which socket that has been sent here.
     If it is the second socket, we set num to 1. Otherwise to 0. This lets
     us use the correct ssl handle. */
  int num = (sockfd == conn->sock[SECONDARYSOCKET]);

  *n=0; /* reset amount to zero */

  if (conn->ssl[num].use) {
    nread = SSL_read(conn->ssl[num].handle, buf, buffersize);

    if(nread < 0) {
      /* failed SSL_read */
      int err = SSL_get_error(conn->ssl[num].handle, nread);

      switch(err) {
      case SSL_ERROR_NONE: /* this is not an error */
      case SSL_ERROR_ZERO_RETURN: /* no more data */
        break;
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        /* there's data pending, re-invoke SSL_read() */
        return -1; /* basicly EWOULDBLOCK */
      default:
        /* openssl/ssl.h says "look at error stack/return value/errno" */
        {
          char error_buffer[120]; /* OpenSSL documents that this must be at
                                     least 120 bytes long. */
          int sslerror = ERR_get_error();
          failf(conn->data, "SSL read: %s, errno %d",
                ERR_error_string(sslerror, error_buffer),
                Curl_ourerrno() );
        }
        return CURLE_RECV_ERROR;
      }
    }
  }
  else {
#else
    (void)conn;
#endif
    *n=0; /* reset amount to zero */
#ifdef HAVE_KRB4
    if(conn->sec_complete)
      nread = Curl_sec_read(conn, sockfd, buf, buffersize);
    else
#endif
      nread = sread(sockfd, buf, buffersize);

    if(-1 == nread) {
      int err = Curl_ourerrno();
      conn->sockerror = err;
#ifdef WIN32
      if(WSAEWOULDBLOCK == err)
#else
      if((EWOULDBLOCK == err) || (EAGAIN == err) || (EINTR == err))
#endif
        return -1;
    }
    else
      conn->sockerror = 0; /* no error */

#ifdef USE_SSLEAY
  }
#endif /* USE_SSLEAY */
  *n = nread;
  return CURLE_OK;
}

/* return 0 on success */
int Curl_debug(struct SessionHandle *data, curl_infotype type,
               char *ptr, size_t size)
{
  static const char * const s_infotype[CURLINFO_END] = {
    "* ", "< ", "> ", "{ ", "} " };

  if(data->set.fdebug)
    return (*data->set.fdebug)(data, type, ptr, size,
                               data->set.debugdata);

  switch(type) {
  case CURLINFO_TEXT:
  case CURLINFO_HEADER_OUT:
  case CURLINFO_HEADER_IN:
    fwrite(s_infotype[type], 2, 1, data->set.err);
    fwrite(ptr, size, 1, data->set.err);
    break;
  default: /* nada */
    break;
  }
  return 0;
}
