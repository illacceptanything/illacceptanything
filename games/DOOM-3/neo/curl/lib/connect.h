#ifndef __CONNECT_H
#define __CONNECT_H
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
 * $Id: connect.h,v 1.16 2004/03/09 22:52:50 bagder Exp $
 ***************************************************************************/

int Curl_nonblock(curl_socket_t sockfd,    /* operate on this */
                  int nonblock   /* TRUE or FALSE */);

CURLcode Curl_is_connected(struct connectdata *conn,
                           curl_socket_t sockfd,
                           bool *connected);

CURLcode Curl_connecthost(struct connectdata *conn,
                          struct Curl_dns_entry *host, /* connect to this */
                          int port,       /* connect to this port number */
                          curl_socket_t *sockconn, /* not set if error */
                          Curl_ipconnect **addr, /* the one we used */
                          bool *connected /* truly connected? */
                          );

int Curl_ourerrno(void);
#endif
