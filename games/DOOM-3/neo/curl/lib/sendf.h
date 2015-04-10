#ifndef __SENDF_H
#define __SENDF_H
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
 * $Id: sendf.h,v 1.24 2004/03/10 16:01:48 bagder Exp $
 ***************************************************************************/

CURLcode Curl_sendf(curl_socket_t sockfd, struct connectdata *,
                    const char *fmt, ...);
void Curl_infof(struct SessionHandle *, const char *fmt, ...);
void Curl_failf(struct SessionHandle *, const char *fmt, ...);

#define infof Curl_infof
#define failf Curl_failf

#define CLIENTWRITE_BODY   1
#define CLIENTWRITE_HEADER 2
#define CLIENTWRITE_BOTH   (CLIENTWRITE_BODY|CLIENTWRITE_HEADER)

CURLcode Curl_client_write(struct SessionHandle *data, int type, char *ptr,
                           size_t len);

/* internal read-function, does plain socket, SSL and krb4 */
int Curl_read(struct connectdata *conn, curl_socket_t sockfd,
              char *buf, size_t buffersize,
              ssize_t *n);
/* internal write-function, does plain socket, SSL and krb4 */
CURLcode Curl_write(struct connectdata *conn,
                    curl_socket_t sockfd,
                    void *mem, size_t len,
                    ssize_t *written);

/* the function used to output verbose information */
int Curl_debug(struct SessionHandle *handle, curl_infotype type,
               char *data, size_t size);


#endif
