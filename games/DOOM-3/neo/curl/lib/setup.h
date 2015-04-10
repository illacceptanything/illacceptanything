#ifndef __SETUP_H
#define __SETUP_H
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
 * $Id: setup.h,v 1.59 2004/03/17 12:46:46 bagder Exp $
 ***************************************************************************/

#ifdef HTTP_ONLY
#define CURL_DISABLE_FTP
#define CURL_DISABLE_LDAP
#define CURL_DISABLE_TELNET
#define CURL_DISABLE_DICT
#define CURL_DISABLE_FILE
#define CURL_DISABLE_GOPHER
#endif

#if !defined(WIN32) && defined(__WIN32__)
/* This should be a good Borland fix. Alexander J. Oss told us! */
#define WIN32
#endif

#ifdef HAVE_CONFIG_H
#include "config.h" /* the configure script results */
#else
#ifdef WIN32
/* hand-modified win32 config.h! */
#include "config-win32.h"
#endif
#endif

#ifdef VMS
/* hand-modified VMS config.h! */
#include "config-vms.h"
#endif
#ifdef NETWARE
/* hand-modified NetWare config.h! */
#include "config-netware.h"
#endif
#ifdef macintosh
/* hand-modified MacOS config.h! */
#include "config-mac.h"
#endif
#ifdef AMIGA
/* hand-modified AmigaOS config.h! */
#include "amigaos.h"
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#if !defined(__cplusplus) && !defined(__BEOS__)
typedef unsigned char bool;
#define typedef_bool
#endif

#ifdef HAVE_LONGLONG
#define LONG_LONG long long
#define ENABLE_64BIT
#else
#ifdef _MSC_VER
#define LONG_LONG __int64
#define ENABLE_64BIT
#endif
#endif /* HAVE_LONGLONG */

#ifndef SIZEOF_CURL_OFF_T
/* If we don't know the size here, we assume a conservative size: 4. When
   building libcurl, the actual size of this variable should be define in the
   config*.h file. */
#define SIZEOF_CURL_OFF_T 4
#endif

/* We set up our internal prefered (CURL_)FORMAT_OFF_T here */
#if SIZEOF_CURL_OFF_T > 4
#define FORMAT_OFF_T "lld"
#else
#define FORMAT_OFF_T "ld"
#endif

#ifdef NEED_REENTRANT
/* Solaris machines needs _REENTRANT set for a few function prototypes and
   things to appear in the #include files. We need to #define it before all
   #include files */
#define _REENTRANT
#endif

#include <stdio.h>
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#include <errno.h>

#ifdef __TANDEM /* for nsr-tandem-nsk systems */
#include <floss.h>
#endif

#if defined(HAVE_X509_H) && defined(HAVE_SSL_H) && defined(HAVE_RSA_H) && \
defined(HAVE_PEM_H) && defined(HAVE_ERR_H) && defined(HAVE_CRYPTO_H) && \
defined(HAVE_LIBSSL) && defined(HAVE_LIBCRYPTO)
  /* the six important includes files all exist and so do both libs,
     defined SSLeay usage */
#define USE_SSLEAY 1
#endif
#if defined(HAVE_OPENSSL_X509_H) && defined(HAVE_OPENSSL_SSL_H) && \
defined(HAVE_OPENSSL_RSA_H) && defined(HAVE_OPENSSL_PEM_H) && \
defined(HAVE_OPENSSL_ERR_H) && defined(HAVE_OPENSSL_CRYPTO_H) && \
defined(HAVE_LIBSSL) && defined(HAVE_LIBCRYPTO)
  /* the six important includes files all exist and so do both libs,
     defined SSLeay usage */
#define USE_SSLEAY 1
#define USE_OPENSSL 1
#endif

#ifndef STDC_HEADERS /* no standard C headers! */
#ifdef	VMS
#include "../include/curl/stdcheaders.h"
#else
#include <curl/stdcheaders.h>
#endif
#endif

#if defined(CURLDEBUG) && defined(HAVE_ASSERT_H)
#define NDEBUG
#define curlassert(x) assert(x)
#else
/* does nothing without CURLDEBUG defined */
#define curlassert(x)
#endif

#ifdef MSG_NOSIGNAL
/* If we have the MSG_NOSIGNAL define, we make sure to use that in the forth
   argument to send() and recv() */
#define SEND_4TH_ARG MSG_NOSIGNAL
#define HAVE_MSG_NOSIGNAL 1 /* we have MSG_NOSIGNAL */
#else
#define SEND_4TH_ARG 0
#endif


/* Below we define four functions. They should
   1. close a socket
   2. read from a socket
   3. write to a socket

   4. set the SIGALRM signal timeout
   5. set dir/file naming defines
   */

#ifdef WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  /* Prevent including <winsock*.h> in <windows.h> */
#endif

#include <winsock2.h>        /* required by telnet.c */

#if defined(ENABLE_IPV6) || defined(USE_SSLEAY)
#include <ws2tcpip.h>
#endif

#if !defined(__GNUC__) || defined(__MINGW32__)
#define sclose(x) closesocket(x)
#define sread(x,y,z) recv(x,y,z, SEND_4TH_ARG)
#define swrite(x,y,z) (size_t)send(x,y,z, SEND_4TH_ARG)
#undef HAVE_ALARM
#else
     /* gcc-for-win is still good :) */
#define sclose(x) close(x)
#define sread(x,y,z) recv(x,y,z, SEND_4TH_ARG)
#define swrite(x,y,z) send(x,y,z, SEND_4TH_ARG)
#define HAVE_ALARM
#endif

#define DIR_CHAR      "\\"
#define DOT_CHAR      "_"

#else

#ifdef DJGPP
#define sclose(x)         close_s(x)
#define sread(x,y,z)      read_s(x,y,z)
#define swrite(x,y,z)     write_s(x,y,z)
#define select(n,r,w,x,t) select_s(n,r,w,x,t)
#define ioctl(x,y,z) ioctlsocket(x,y,(char *)(z))
#define IOCTL_3_ARGS
#include <tcp.h>
#ifdef word
#undef word
#endif

#else

#ifdef __BEOS__
#define sclose(x) closesocket(x)
#define sread(x,y,z) (ssize_t)recv(x,y,z, SEND_4TH_ARG)
#define swrite(x,y,z) (ssize_t)send(x,y,z, SEND_4TH_ARG)
#else
#define sclose(x) close(x)
#define sread(x,y,z) recv(x,y,z, SEND_4TH_ARG)
#define swrite(x,y,z) send(x,y,z, SEND_4TH_ARG)
#endif

#define HAVE_ALARM

#endif

#ifdef _AMIGASF
#undef HAVE_ALARM
#undef sclose
#define sclose(x) CloseSocket(x)
#endif

#define DIR_CHAR      "/"
#define DOT_CHAR      "."

#ifdef DJGPP
#undef DOT_CHAR
#define DOT_CHAR      "_"
#endif

#ifndef fileno /* sunos 4 have this as a macro! */
int fileno( FILE *stream);
#endif

#endif

/* now typedef our socket type */
#ifdef WIN32
typedef SOCKET curl_socket_t;
#define CURL_SOCKET_BAD INVALID_SOCKET
#else
typedef int curl_socket_t;
#define CURL_SOCKET_BAD -1
#endif

#if defined(ENABLE_IPV6) && defined(USE_ARES)
#error "ares does not yet support IPv6. Disable IPv6 or ares and rebuild"
#endif

#if defined(WIN32) && !defined(__CYGWIN32__) && !defined(USE_ARES) && !defined(ENABLE_IPV6)
#define USE_THREADING_GETHOSTBYNAME  /* Cygwin uses alarm() function */
#endif

/*
 * Curl_addrinfo MUST be used for name resolving information.
 * Information regarding a single IP witin a Curl_addrinfo MUST be stored in
 * a Curl_ipconnect struct.
 */
#ifdef ENABLE_IPV6
typedef struct addrinfo Curl_addrinfo;
typedef struct addrinfo Curl_ipconnect;
#else
typedef struct hostent Curl_addrinfo;
typedef struct in_addr Curl_ipconnect;
#endif

#ifdef VMS
#define IOCTL_3_ARGS
#endif

#ifdef mpeix
#define IOCTL_3_ARGS
#endif

#ifndef ECONNRESET
#ifdef WSAECONNRESET
#define ECONNRESET WSAECONNRESET
#else
/* This will effectively prevent the code from working in this particular
   aspect, but it still compile fine! */
#define ECONNRESET 10000
#endif
#endif

#ifdef NETWARE
#undef HAVE_ALARM
#endif

#endif /* __CONFIG_H */
