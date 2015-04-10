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
 * $Id: connect.c,v 1.82 2004/03/17 12:46:45 bagder Exp $
 ***************************************************************************/

#include "setup.h"

#ifndef WIN32
/* headers for non-win32 */
#include <sys/time.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <sys/ioctl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* required for free() prototype, without it, this crashes
                       on macos 68K */
#endif
#if (defined(HAVE_FIONBIO) && defined(__NOVELL_LIBC__))
#include <sys/filio.h>
#endif
#if (defined(NETWARE) && defined(__NOVELL_LIBC__))
#undef in_addr_t
#define in_addr_t unsigned long
#endif
#ifdef	VMS
#include <in.h>
#include <inet.h>
#endif

#endif
#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifdef WIN32
#include <windows.h>
#define EINPROGRESS WSAEINPROGRESS
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EISCONN     WSAEISCONN
#define ENOTSOCK    WSAENOTSOCK
#define ECONNREFUSED WSAECONNREFUSED
#endif

#include "urldata.h"
#include "sendf.h"
#include "if2ip.h"
#include "connect.h"

/* The last #include file should be: */
#ifdef CURLDEBUG
#include "memdebug.h"
#endif

static bool verifyconnect(curl_socket_t sockfd);

int Curl_ourerrno(void)
{
#ifdef WIN32
  return (int)GetLastError();
#else
  return errno;
#endif
}

/*************************************************************************
 * Curl_nonblock
 *
 * Description:
 *  Set the socket to either blocking or non-blocking mode.
 */

int Curl_nonblock(curl_socket_t sockfd,    /* operate on this */
                  int nonblock   /* TRUE or FALSE */)
{
#undef SETBLOCK
#ifdef HAVE_O_NONBLOCK
  /* most recent unix versions */
  int flags;

  flags = fcntl(sockfd, F_GETFL, 0);
  if (TRUE == nonblock)
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
  else
    return fcntl(sockfd, F_SETFL, flags & (~O_NONBLOCK));
#define SETBLOCK 1
#endif

#ifdef HAVE_FIONBIO
  /* older unix versions */
  int flags;

  flags = nonblock;
  return ioctl(sockfd, FIONBIO, &flags);
#define SETBLOCK 2
#endif

#ifdef HAVE_IOCTLSOCKET
  /* Windows? */
  int flags;
  flags = nonblock;
  return ioctlsocket(sockfd, FIONBIO, &flags);
#define SETBLOCK 3
#endif

#ifdef HAVE_IOCTLSOCKET_CASE
  /* presumably for Amiga */
  return IoctlSocket(sockfd, FIONBIO, (long)nonblock);
#define SETBLOCK 4
#endif

#ifdef HAVE_SO_NONBLOCK
  /* BeOS */
  long b = nonblock ? 1 : 0;
  return setsockopt(sockfd, SOL_SOCKET, SO_NONBLOCK, &b, sizeof(b));
#define SETBLOCK 5
#endif

#ifdef HAVE_DISABLED_NONBLOCKING
  return 0; /* returns success */
#define SETBLOCK 6
#endif

#ifndef SETBLOCK
#error "no non-blocking method was found/used/set"
#endif
}

/*
 * waitconnect() returns:
 * 0    fine connect
 * -1   select() error
 * 1    select() timeout
 * 2    select() returned with an error condition
 */
static
int waitconnect(curl_socket_t sockfd, /* socket */
                long timeout_msec)
{
  fd_set fd;
  fd_set errfd;
  struct timeval interval;
  int rc;
#ifdef mpeix
  /* Call this function once now, and ignore the results. We do this to
     "clear" the error state on the socket so that we can later read it
     reliably. This is reported necessary on the MPE/iX operating system. */
  verifyconnect(sockfd);
#endif

  /* now select() until we get connect or timeout */
  FD_ZERO(&fd);
  FD_SET(sockfd, &fd);

  FD_ZERO(&errfd);
  FD_SET(sockfd, &errfd);

  interval.tv_sec = timeout_msec/1000;
  timeout_msec -= interval.tv_sec*1000;

  interval.tv_usec = timeout_msec*1000;

  rc = select(sockfd+1, NULL, &fd, &errfd, &interval);
  if(-1 == rc)
    /* error, no connect here, try next */
    return -1;
  
  else if(0 == rc)
    /* timeout, no connect today */
    return 1;

  if(FD_ISSET(sockfd, &errfd))
    /* error condition caught */
    return 2;

  /* we have a connect! */
  return 0;
}

static CURLcode bindlocal(struct connectdata *conn,
                          curl_socket_t sockfd)
{
#ifdef HAVE_INET_NTOA
  bool bindworked = FALSE;
  struct SessionHandle *data = conn->data;

  /*************************************************************
   * Select device to bind socket to
   *************************************************************/
  if (strlen(data->set.device)<255) {
    struct Curl_dns_entry *h=NULL;
    size_t size;
    char myhost[256] = "";
    in_addr_t in;
    int rc;
    bool was_iface = FALSE;

    /* First check if the given name is an IP address */
    in=inet_addr(data->set.device);
      
    if((in == CURL_INADDR_NONE) &&
       Curl_if2ip(data->set.device, myhost, sizeof(myhost))) {
      /*
       * We now have the numerical IPv4-style x.y.z.w in the 'myhost' buffer
       */
      rc = Curl_resolv(conn, myhost, 0, &h);
      if(rc == 1)
        (void)Curl_wait_for_resolv(conn, &h);

      if(h)
        was_iface = TRUE;
    }

    if(!was_iface) {
      /*
       * This was not an interface, resolve the name as a host name
       * or IP number
       */
      rc = Curl_resolv(conn, data->set.device, 0, &h);
      if(rc == 1)
        (void)Curl_wait_for_resolv(conn, &h);

      if(h)
        /* we know data->set.device is shorter than the myhost array */
        strcpy(myhost, data->set.device);
    }

    if(! *myhost) {
      /* need to fix this
         h=Curl_gethost(data,
         getmyhost(*myhost,sizeof(myhost)),
         hostent_buf,
         sizeof(hostent_buf));
      */
      failf(data, "Couldn't bind to '%s'", data->set.device);
      return CURLE_HTTP_PORT_FAILED;
    }

    infof(data, "We bind local end to %s\n", myhost);

#ifdef SO_BINDTODEVICE
    /* I am not sure any other OSs than Linux that provide this feature, and
     * at the least I cannot test. --Ben
     *
     * This feature allows one to tightly bind the local socket to a
     * particular interface.  This will force even requests to other local
     * interfaces to go out the external interface.
     *
     */
    if (was_iface) {
      /* Only bind to the interface when specified as interface, not just as a
       * hostname or ip address.
       */
      if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE,
                     data->set.device, strlen(data->set.device)+1) != 0) {
        /* printf("Failed to BINDTODEVICE, socket: %d  device: %s error: %s\n",
           sockfd, data->set.device, strerror(errno)); */
        infof(data, "SO_BINDTODEVICE %s failed\n",
              data->set.device);
        /* This is typiclally "errno 1, error: Operation not permitted" if
           you're not running as root or another suitable privileged user */
      }
    }
#endif

    in=inet_addr(myhost);
    if (CURL_INADDR_NONE != in) {

      if ( h ) {
        Curl_addrinfo *addr = h->addr;

        Curl_resolv_unlock(data, h);
        /* we don't need it anymore after this function has returned */

#ifdef ENABLE_IPV6
        if( bind(sockfd, addr->ai_addr, addr->ai_addrlen) >= 0) {
          /* we succeeded to bind */
          struct sockaddr_in6 add;

          bindworked = TRUE;
	
          size = sizeof(add);
          if(getsockname(sockfd, (struct sockaddr *) &add,
                         (socklen_t *)&size)<0) {
            failf(data, "getsockname() failed");
            return CURLE_HTTP_PORT_FAILED;
          }
        }
#else
        {
          struct sockaddr_in sa;

          memset((char *)&sa, 0, sizeof(sa));
          memcpy((char *)&sa.sin_addr, addr->h_addr, addr->h_length);
          sa.sin_family = AF_INET;
          sa.sin_addr.s_addr = in;
          sa.sin_port = 0; /* get any port */
	
          if( bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) >= 0) {
            /* we succeeded to bind */
            struct sockaddr_in add;
	
            bindworked = TRUE;
            
            size = sizeof(add);
            if(getsockname(sockfd, (struct sockaddr *) &add,
                           (socklen_t *)&size)<0) {
              failf(data, "getsockname() failed");
              return CURLE_HTTP_PORT_FAILED;
            }
          }
        }
#endif
        if(!bindworked) {
          switch(errno) {
          case EBADF:
            failf(data, "Invalid descriptor: %d", errno);
            break;
          case EINVAL:
            failf(data, "Invalid request: %d", errno);
            break;
          case EACCES:
            failf(data, "Address is protected, user not superuser: %d", errno);
            break;
          case ENOTSOCK:
            failf(data,
                  "Argument is a descriptor for a file, not a socket: %d",
                  errno);
            break;
          case EFAULT:
            failf(data, "Inaccessable memory error: %d", errno);
            break;
          case ENAMETOOLONG:
            failf(data, "Address too long: %d", errno);
            break;
          case ENOMEM:
            failf(data, "Insufficient kernel memory was available: %d", errno);
            break;
          default:
            failf(data, "errno %d", errno);
            break;
          } /* end of switch(errno) */
	
          return CURLE_HTTP_PORT_FAILED;
        } /* end of else */
	
      } /* end of if  h */
      else {
	failf(data,"could't find my own IP address (%s)", myhost);
	return CURLE_HTTP_PORT_FAILED;
      }
    } /* end of inet_addr */

    else {
      failf(data, "could't find my own IP address (%s)", myhost);
      return CURLE_HTTP_PORT_FAILED;
    }

    return CURLE_OK;

  } /* end of device selection support */
#endif /* end of HAVE_INET_NTOA */

  return CURLE_HTTP_PORT_FAILED;
}

/*
 * verifyconnect() returns TRUE if the connect really has happened.
 */
static bool verifyconnect(curl_socket_t sockfd)
{
#if defined(SO_ERROR) && !defined(WIN32)
  int err = 0;
  socklen_t errSize = sizeof(err);
  if( -1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR,
                       (void *)&err, &errSize))
    err = Curl_ourerrno();

  if ((0 == err) || (EISCONN == err))
    /* we are connected, awesome! */
    return TRUE;
  
  /* This wasn't a successful connect */
  return FALSE;
#else
  (void)sockfd;
  return TRUE;
#endif
}

/*
 * Curl_is_connected() is used from the multi interface to check if the
 * firstsocket has connected.
 */

CURLcode Curl_is_connected(struct connectdata *conn,
                           curl_socket_t sockfd,
                           bool *connected)
{
  int rc;
  struct SessionHandle *data = conn->data;

  *connected = FALSE; /* a very negative world view is best */

  if(data->set.timeout || data->set.connecttimeout) {
    /* there is a timeout set */

    /* Evaluate in milliseconds how much time that has passed */
    long has_passed = Curl_tvdiff(Curl_tvnow(), data->progress.start);

    /* subtract the most strict timeout of the ones */
    if(data->set.timeout && data->set.connecttimeout) {
      if (data->set.timeout < data->set.connecttimeout)
        has_passed -= data->set.timeout*1000;
      else 
        has_passed -= data->set.connecttimeout*1000;
    }
    else if(data->set.timeout)
      has_passed -= data->set.timeout*1000;
    else
      has_passed -= data->set.connecttimeout*1000;

    if(has_passed > 0 ) {
      /* time-out, bail out, go home */
      failf(data, "Connection time-out");
      return CURLE_OPERATION_TIMEOUTED;
    }
  }
  if(conn->bits.tcpconnect) {
    /* we are connected already! */
    *connected = TRUE;
    return CURLE_OK;
  }

  /* check for connect without timeout as we want to return immediately */
  rc = waitconnect(sockfd, 0);

  if(0 == rc) {
    if (verifyconnect(sockfd)) {
      /* we are connected, awesome! */
      *connected = TRUE;
      return CURLE_OK;
    }
    /* nope, not connected for real */
    failf(data, "Connection failed");
    return CURLE_COULDNT_CONNECT;
  }
  else if(1 != rc) {
    int error = Curl_ourerrno();
    failf(data, "Failed connect to %s:%d, errno: %d",
          conn->hostname, conn->port, error);
    return CURLE_COULDNT_CONNECT;
  }
  /*
   * If the connection phase is "done" here, we should attempt to connect
   * to the "next address" in the Curl_hostaddr structure that we resolved
   * before. But we don't have that struct around anymore and we can't just
   * keep a pointer since the cache might in fact have gotten pruned by the
   * time we want to read this... Alas, we don't do this yet.
   */

  return CURLE_OK;
}


/*
 * TCP connect to the given host with timeout, proxy or remote doesn't matter.
 * There might be more than one IP address to try out. Fill in the passed
 * pointer with the connected socket.
 */

CURLcode Curl_connecthost(struct connectdata *conn,  /* context */
                          struct Curl_dns_entry *remotehost, /* use this one */
                          int port,                  /* connect to this */
                          curl_socket_t *sockconn,   /* the connected socket */
                          Curl_ipconnect **addr,     /* the one we used */
                          bool *connected)           /* really connected? */
{
  struct SessionHandle *data = conn->data;
  int rc;
  curl_socket_t sockfd= CURL_SOCKET_BAD;
  int aliasindex=0;
  char *hostname;

  struct timeval after;
  struct timeval before = Curl_tvnow();

#ifdef ENABLE_IPV6
  struct addrinfo *ai;
#endif  

  /*************************************************************
   * Figure out what maximum time we have left
   *************************************************************/
  long timeout_ms=300000; /* milliseconds, default to five minutes */

  *connected = FALSE; /* default to not connected */

  if(data->set.timeout || data->set.connecttimeout) {
    double has_passed;

    /* Evaluate in milliseconds how much time that has passed */
    has_passed = Curl_tvdiff(Curl_tvnow(), data->progress.start);

#ifndef min
#define min(a, b)   ((a) < (b) ? (a) : (b))
#endif

    /* get the most strict timeout of the ones converted to milliseconds */
    if(data->set.timeout && data->set.connecttimeout) {
      if (data->set.timeout < data->set.connecttimeout)
        timeout_ms = data->set.timeout*1000;
      else 
        timeout_ms = data->set.connecttimeout*1000;
    }
    else if(data->set.timeout)
      timeout_ms = data->set.timeout*1000;
    else
      timeout_ms = data->set.connecttimeout*1000;

    /* subtract the passed time */
    timeout_ms -= (long)has_passed;

    if(timeout_ms < 0) {
      /* a precaution, no need to continue if time already is up */
      failf(data, "Connection time-out");
      return CURLE_OPERATION_TIMEOUTED;
    }
  }

  hostname = data->change.proxy?conn->proxyhost:conn->hostname;
  infof(data, "About to connect() to %s port %d\n",
        hostname, port);

#ifdef ENABLE_IPV6
  /*
   * Connecting with a getaddrinfo chain
   */
  for (ai = remotehost->addr; ai; ai = ai->ai_next, aliasindex++) {
    sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sockfd == CURL_SOCKET_BAD)
      continue;
#else
  /*
   * Connecting with old style IPv4-only support
   */

  /* This is the loop that attempts to connect to all IP-addresses we
     know for the given host. One by one. */
  for(rc=-1, aliasindex=0;
      rc && (struct in_addr *)remotehost->addr->h_addr_list[aliasindex];
      aliasindex++) {
    struct sockaddr_in serv_addr;

    /* create an IPv4 TCP socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(CURL_SOCKET_BAD == sockfd) {
      failf(data, "couldn't create socket");
      return CURLE_COULDNT_CONNECT; /* big time error */
    }

    /* nasty address work before connect can be made */
    memset((char *) &serv_addr, '\0', sizeof(serv_addr));
    memcpy((char *)&(serv_addr.sin_addr),
           (struct in_addr *)remotehost->addr->h_addr_list[aliasindex],
           sizeof(struct in_addr));
    serv_addr.sin_family = remotehost->addr->h_addrtype;
    serv_addr.sin_port = htons((unsigned short)port);
#endif

    if(conn->data->set.device) {
      /* user selected to bind the outgoing socket to a specified "device"
         before doing connect */
      CURLcode res = bindlocal(conn, sockfd);
      if(res)
        return res;
    }

    /* set socket non-blocking */
    Curl_nonblock(sockfd, TRUE);

    /* do not use #ifdef within the function arguments below, as connect() is
       a defined macro on some platforms and some compilers don't like to mix
       #ifdefs with macro usage! (AmigaOS is one such platform) */
#ifdef ENABLE_IPV6
    rc = connect(sockfd, ai->ai_addr, ai->ai_addrlen);
#else
    rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
#endif

    if(-1 == rc) {
      int error=Curl_ourerrno();

      switch (error) {
      case EINPROGRESS:
      case EWOULDBLOCK:
#if defined(EAGAIN) && EAGAIN != EWOULDBLOCK
        /* On some platforms EAGAIN and EWOULDBLOCK are the
         * same value, and on others they are different, hence
         * the odd #if
         */
      case EAGAIN:
#endif
        /* asynchronous connect, wait for connect or timeout */
        if(data->state.used_interface == Curl_if_multi)
          /* don't hang when doing multi */
          timeout_ms = 0;
        
        rc = waitconnect(sockfd, timeout_ms);
        break;
      default:
        /* unknown error, fallthrough and try another address! */
        failf(data, "Failed to connect to %s IP number %d: %d",
              hostname, aliasindex+1, error);
        break;
      }
    }

    /* The '1 == rc' comes from the waitconnect(), and not from connect().
       We can be sure of this since connect() cannot return 1. */
    if((1 == rc) && (data->state.used_interface == Curl_if_multi)) {
      /* Timeout when running the multi interface, we return here with a
         CURLE_OK return code. */
      rc = 0;
      break;
    }
      
    if(0 == rc) {
      if (verifyconnect(sockfd)) {
        /* we are connected, awesome! */
        *connected = TRUE; /* this is a true connect */
        break;
      }
      /* nope, not connected for real */
      rc = -1;
    }

    /* connect failed or timed out */
    sclose(sockfd);
    sockfd = -1;

    /* get a new timeout for next attempt */
    after = Curl_tvnow();
    timeout_ms -= Curl_tvdiff(after, before);
    if(timeout_ms < 0) {
      failf(data, "connect() timed out!");
      return CURLE_OPERATION_TIMEOUTED;
    }
    before = after;
  }
  if (sockfd == CURL_SOCKET_BAD) {
    /* no good connect was made */
    *sockconn = -1;
    failf(data, "Connect failed");
    return CURLE_COULDNT_CONNECT;
  }

  /* leave the socket in non-blocking mode */

  /* store the address we use */
  if(addr) {
#ifdef ENABLE_IPV6
    *addr = ai;
#else
    *addr = (struct in_addr *)remotehost->addr->h_addr_list[aliasindex];
#endif
  }

  /* allow NULL-pointers to get passed in */
  if(sockconn)
    *sockconn = sockfd;    /* the socket descriptor we've connected */

  return CURLE_OK;
}
