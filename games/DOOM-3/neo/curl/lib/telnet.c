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
 * $Id: telnet.c,v 1.57 2004/03/11 13:13:35 bagder Exp $
 ***************************************************************************/

#include "setup.h"

#ifndef CURL_DISABLE_TELNET
/* -- WIN32 approved -- */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>

#if defined(WIN32)
#include <time.h>
#include <io.h>
#else
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <netinet/in.h>
#include <sys/time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <netdb.h>
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#include <sys/ioctl.h>
#include <signal.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif


#endif

#include "urldata.h"
#include <curl/curl.h>
#include "transfer.h"
#include "sendf.h"
#include "telnet.h"

#define _MPRINTF_REPLACE /* use our functions only */
#include <curl/mprintf.h>

#define  TELOPTS
#define  TELCMDS

#include "arpa_telnet.h"

/* The last #include file should be: */
#ifdef CURLDEBUG
#include "memdebug.h"
#endif

#define SUBBUFSIZE 512

#define CURL_SB_CLEAR(x)  x->subpointer = x->subbuffer;
#define CURL_SB_TERM(x)   { x->subend = x->subpointer; CURL_SB_CLEAR(x); }
#define CURL_SB_ACCUM(x,c) \
  if (x->subpointer < (x->subbuffer+sizeof x->subbuffer)) { \
    *x->subpointer++ = (c); \
  }

#define  CURL_SB_GET(x) ((*x->subpointer++)&0xff)
#define  CURL_SB_PEEK(x)   ((*x->subpointer)&0xff)
#define  CURL_SB_EOF(x) (x->subpointer >= x->subend)
#define  CURL_SB_LEN(x) (x->subend - x->subpointer)

#ifdef WIN32
typedef FARPROC WSOCK2_FUNC;
static CURLcode check_wsock2 ( struct SessionHandle *data );
#endif

static
void telrcv(struct connectdata *,
	    unsigned char *inbuf,	/* Data received from socket */
	    int count);			/* Number of bytes received */

static void printoption(struct SessionHandle *data,
			const char *direction,
			int cmd, int option);

static void negotiate(struct connectdata *);
static void send_negotiation(struct connectdata *, int cmd, int option);
static void set_local_option(struct connectdata *, int cmd, int option);
static void set_remote_option(struct connectdata *, int cmd, int option);

static void printsub(struct SessionHandle *data,
		     int direction, unsigned char *pointer, int length);
static void suboption(struct connectdata *);

/* For negotiation compliant to RFC 1143 */
#define CURL_NO	         0
#define CURL_YES 	 1
#define CURL_WANTYES	 2
#define CURL_WANTNO	 3

#define CURL_EMPTY	 0
#define CURL_OPPOSITE    1

/*
 * Telnet receiver states for fsm
 */
typedef enum
{
   CURL_TS_DATA = 0,
   CURL_TS_IAC,
   CURL_TS_WILL,
   CURL_TS_WONT,
   CURL_TS_DO,
   CURL_TS_DONT,
   CURL_TS_CR,
   CURL_TS_SB,   /* sub-option collection */
   CURL_TS_SE   /* looking for sub-option end */
} TelnetReceive;

struct TELNET {
  int please_negotiate;
  int already_negotiated;
  int us[256]; 
  int usq[256]; 
  int us_preferred[256]; 
  int him[256]; 
  int himq[256]; 
  int him_preferred[256]; 
  char subopt_ttype[32];             /* Set with suboption TTYPE */
  char subopt_xdisploc[128];          /* Set with suboption XDISPLOC */
  struct curl_slist *telnet_vars; /* Environment variables */

  /* suboptions */
  char subbuffer[SUBBUFSIZE];
  char *subpointer, *subend;      /* buffer for sub-options */
  
  TelnetReceive telrcv_state;
};

#ifdef WIN32
static CURLcode
check_wsock2 ( struct SessionHandle *data )
{
  int err; 
  WORD wVersionRequested;  
  WSADATA wsaData; 

  curlassert(data);

  /* telnet requires at least WinSock 2.0 so ask for it. */
  wVersionRequested = MAKEWORD(2, 0);

  err = WSAStartup(wVersionRequested, &wsaData); 
  
  /* We must've called this once already, so this call */
  /* should always succeed.  But, just in case... */
  if (err != 0) {
    failf(data,"WSAStartup failed (%d)",err);
    return CURLE_FAILED_INIT; 
  }

  /* We have to have a WSACleanup call for every successful */
  /* WSAStartup call. */
  WSACleanup();

  /* Check that our version is supported */
  if (LOBYTE(wsaData.wVersion) != LOBYTE(wVersionRequested) ||
      HIBYTE(wsaData.wVersion) != HIBYTE(wVersionRequested)) {
      /* Our version isn't supported */
      failf(data,"insufficient winsock version to support "
	    "telnet");
      return CURLE_FAILED_INIT;
  }

  /* Our version is supported */
  return CURLE_OK;
}
#endif
static
CURLcode init_telnet(struct connectdata *conn)
{
  struct TELNET *tn;

  tn = (struct TELNET *)malloc(sizeof(struct TELNET));
  if(!tn)
    return CURLE_OUT_OF_MEMORY;
  
  conn->proto.telnet = (void *)tn; /* make us known */

  memset(tn, 0, sizeof(struct TELNET));

  tn->telrcv_state = CURL_TS_DATA;

  /* Init suboptions */
  CURL_SB_CLEAR(tn);

  /* Set all options to NO */
#if 0
  /* NO is zero => default fill pattern */
  memset(tn->us, CURL_NO, 256);
  memset(tn->usq, CURL_NO, 256);
  memset(tn->us_preferred, CURL_NO, 256);
  memset(tn->him, CURL_NO, 256);
  memset(tn->himq, CURL_NO, 256);
  memset(tn->him_preferred, CURL_NO, 256);
#endif
  /* Set the options we want by default */
  tn->us_preferred[CURL_TELOPT_BINARY] = CURL_YES;
  tn->us_preferred[CURL_TELOPT_SGA] = CURL_YES;
  tn->him_preferred[CURL_TELOPT_BINARY] = CURL_YES;
  tn->him_preferred[CURL_TELOPT_SGA] = CURL_YES;

  return CURLE_OK;
}

static void negotiate(struct connectdata *conn)
{
  int i;
  struct TELNET *tn = (struct TELNET *)conn->proto.telnet;
   
  for(i = 0;i < CURL_NTELOPTS;i++)
  {
    if(tn->us_preferred[i] == CURL_YES)
      set_local_option(conn, i, CURL_YES);
      
    if(tn->him_preferred[i] == CURL_YES)
      set_remote_option(conn, i, CURL_YES);
  }
}

static void printoption(struct SessionHandle *data,
			const char *direction, int cmd, int option)
{
  const char *fmt;
  const char *opt;
   
  if (data->set.verbose)
  {
    if (cmd == CURL_IAC)
    {
      if (CURL_TELCMD_OK(option))
        Curl_infof(data, "%s IAC %s\n", direction, CURL_TELCMD(option));
      else
        Curl_infof(data, "%s IAC %d\n", direction, option);
    }
    else
    {
      fmt = (cmd == CURL_WILL) ? "WILL" : (cmd == CURL_WONT) ? "WONT" :
        (cmd == CURL_DO) ? "DO" : (cmd == CURL_DONT) ? "DONT" : 0;
      if (fmt)
      {
        if (CURL_TELOPT_OK(option))
          opt = CURL_TELOPT(option);
        else if (option == CURL_TELOPT_EXOPL)
          opt = "EXOPL";
        else
          opt = NULL;

        if(opt)
          Curl_infof(data, "%s %s %s\n", direction, fmt, opt);
        else
          Curl_infof(data, "%s %s %d\n", direction, fmt, option);
      }
      else
        Curl_infof(data, "%s %d %d\n", direction, cmd, option);
    }
  }
}

static void send_negotiation(struct connectdata *conn, int cmd, int option)
{
   unsigned char buf[3];

   buf[0] = CURL_IAC;
   buf[1] = cmd;
   buf[2] = option;
   
   (void)swrite(conn->sock[FIRSTSOCKET], buf, 3);
   
   printoption(conn->data, "SENT", cmd, option);
}

static
void set_remote_option(struct connectdata *conn, int option, int newstate)
{
  struct TELNET *tn = (struct TELNET *)conn->proto.telnet;
  if(newstate == CURL_YES)
  {
    switch(tn->him[option])
    {
      case CURL_NO:
        tn->him[option] = CURL_WANTYES;
        send_negotiation(conn, CURL_DO, option);
        break;
	 
      case CURL_YES:
        /* Already enabled */
        break;
	 
      case CURL_WANTNO:
        switch(tn->himq[option])
        {
          case CURL_EMPTY:
            /* Already negotiating for CURL_YES, queue the request */
            tn->himq[option] = CURL_OPPOSITE;
            break;
          case CURL_OPPOSITE:
            /* Error: already queued an enable request */
            break;
        }
        break;
	 
      case CURL_WANTYES:
        switch(tn->himq[option])
        {
          case CURL_EMPTY:
            /* Error: already negotiating for enable */
            break;
          case CURL_OPPOSITE:
            tn->himq[option] = CURL_EMPTY;
            break;
        }
        break;
    }
  }
  else /* NO */
  {
    switch(tn->him[option])
    {
      case CURL_NO:
        /* Already disabled */
        break;
	 
      case CURL_YES:
        tn->him[option] = CURL_WANTNO;
        send_negotiation(conn, CURL_DONT, option);
        break;
	 
      case CURL_WANTNO:
        switch(tn->himq[option])
        {
          case CURL_EMPTY:
            /* Already negotiating for NO */
            break;
          case CURL_OPPOSITE:
            tn->himq[option] = CURL_EMPTY;
            break;
        }
        break;
	 
      case CURL_WANTYES:
        switch(tn->himq[option])
        {
          case CURL_EMPTY:
            tn->himq[option] = CURL_OPPOSITE;
            break;
          case CURL_OPPOSITE:
            break;
        }
        break;
    }
  }
}

static
void rec_will(struct connectdata *conn, int option)
{
  struct TELNET *tn = (struct TELNET *)conn->proto.telnet;
  switch(tn->him[option])
  {
    case CURL_NO:
      if(tn->him_preferred[option] == CURL_YES)
      {
        tn->him[option] = CURL_YES;
        send_negotiation(conn, CURL_DO, option);
      }
      else
      {
        send_negotiation(conn, CURL_DONT, option);
      }
      break;
	 
    case CURL_YES:
      /* Already enabled */
      break;
	 
    case CURL_WANTNO:
      switch(tn->himq[option])
      {
        case CURL_EMPTY:
          /* Error: DONT answered by WILL */
          tn->him[option] = CURL_NO;
          break;
        case CURL_OPPOSITE:
          /* Error: DONT answered by WILL */
          tn->him[option] = CURL_YES;
          tn->himq[option] = CURL_EMPTY;
          break;
      }
      break;
	 
    case CURL_WANTYES:
      switch(tn->himq[option])
      {
        case CURL_EMPTY:
          tn->him[option] = CURL_YES;
          break;
        case CURL_OPPOSITE:
          tn->him[option] = CURL_WANTNO;
          tn->himq[option] = CURL_EMPTY;
          send_negotiation(conn, CURL_DONT, option);
          break;
      }
      break;
  }
}
   
static
void rec_wont(struct connectdata *conn, int option)
{
  struct TELNET *tn = (struct TELNET *)conn->proto.telnet;
  switch(tn->him[option])
  {
    case CURL_NO:
      /* Already disabled */
      break;
	 
    case CURL_YES:
      tn->him[option] = CURL_NO;
      send_negotiation(conn, CURL_DONT, option);
      break;
	 
    case CURL_WANTNO:
      switch(tn->himq[option])
      {
        case CURL_EMPTY:
          tn->him[option] = CURL_NO;
          break;
	 
        case CURL_OPPOSITE:
          tn->him[option] = CURL_WANTYES;
          tn->himq[option] = CURL_EMPTY;
          send_negotiation(conn, CURL_DO, option);
          break;
      }
      break;
	 
    case CURL_WANTYES:
      switch(tn->himq[option])
      {
        case CURL_EMPTY:
          tn->him[option] = CURL_NO;
          break;
        case CURL_OPPOSITE:
          tn->him[option] = CURL_NO;
          tn->himq[option] = CURL_EMPTY;
          break;
      }
      break;
  }
}
   
static void
set_local_option(struct connectdata *conn, int option, int newstate)
{
  struct TELNET *tn = (struct TELNET *)conn->proto.telnet;
  if(newstate == CURL_YES)
  {
    switch(tn->us[option])
    {
      case CURL_NO:
        tn->us[option] = CURL_WANTYES;
        send_negotiation(conn, CURL_WILL, option);
        break;
	 
      case CURL_YES:
        /* Already enabled */
        break;
	 
      case CURL_WANTNO:
        switch(tn->usq[option])
        {
          case CURL_EMPTY:
            /* Already negotiating for CURL_YES, queue the request */
            tn->usq[option] = CURL_OPPOSITE;
            break;
          case CURL_OPPOSITE:
            /* Error: already queued an enable request */
            break;
        }
        break;
	 
      case CURL_WANTYES:
        switch(tn->usq[option])
        {
          case CURL_EMPTY:
            /* Error: already negotiating for enable */
            break;
          case CURL_OPPOSITE:
            tn->usq[option] = CURL_EMPTY;
            break;
        }
        break;
    }
  }
  else /* NO */
  {
    switch(tn->us[option])
    {
      case CURL_NO:
        /* Already disabled */
        break;
	 
      case CURL_YES:
        tn->us[option] = CURL_WANTNO;
        send_negotiation(conn, CURL_WONT, option);
        break;
	 
      case CURL_WANTNO:
        switch(tn->usq[option])
        {
          case CURL_EMPTY:
            /* Already negotiating for NO */
            break;
          case CURL_OPPOSITE:
            tn->usq[option] = CURL_EMPTY;
            break;
        }
        break;
	 
      case CURL_WANTYES:
        switch(tn->usq[option])
        {
          case CURL_EMPTY:
            tn->usq[option] = CURL_OPPOSITE;
            break;
          case CURL_OPPOSITE:
            break;
        }
        break;
    }
  }
}

static
void rec_do(struct connectdata *conn, int option)
{
  struct TELNET *tn = (struct TELNET *)conn->proto.telnet;
  switch(tn->us[option])
  {
    case CURL_NO:
      if(tn->us_preferred[option] == CURL_YES)
      {
        tn->us[option] = CURL_YES;
        send_negotiation(conn, CURL_WILL, option);
      }
      else
      {
        send_negotiation(conn, CURL_WONT, option);
      }
      break;
	 
    case CURL_YES:
      /* Already enabled */
      break;
	 
    case CURL_WANTNO:
      switch(tn->usq[option])
      {
        case CURL_EMPTY:
          /* Error: DONT answered by WILL */
          tn->us[option] = CURL_NO;
          break;
        case CURL_OPPOSITE:
          /* Error: DONT answered by WILL */
          tn->us[option] = CURL_YES;
          tn->usq[option] = CURL_EMPTY;
          break;
      }
      break;
	 
    case CURL_WANTYES:
      switch(tn->usq[option])
      {
        case CURL_EMPTY:
          tn->us[option] = CURL_YES;
          break;
        case CURL_OPPOSITE:
          tn->us[option] = CURL_WANTNO;
          tn->himq[option] = CURL_EMPTY;
          send_negotiation(conn, CURL_WONT, option);
          break;
      }
      break;
  }
}

static   
void rec_dont(struct connectdata *conn, int option)
{
  struct TELNET *tn = (struct TELNET *)conn->proto.telnet;
  switch(tn->us[option])
  {
    case CURL_NO:
      /* Already disabled */
      break;
	 
    case CURL_YES:
      tn->us[option] = CURL_NO;
      send_negotiation(conn, CURL_WONT, option);
      break;
	 
    case CURL_WANTNO:
      switch(tn->usq[option])
      {
        case CURL_EMPTY:
          tn->us[option] = CURL_NO;
          break;
	 
        case CURL_OPPOSITE:
          tn->us[option] = CURL_WANTYES;
          tn->usq[option] = CURL_EMPTY;
          send_negotiation(conn, CURL_WILL, option);
          break;
      }
      break;
	 
    case CURL_WANTYES:
      switch(tn->usq[option])
      {
        case CURL_EMPTY:
          tn->us[option] = CURL_NO;
          break;
        case CURL_OPPOSITE:
          tn->us[option] = CURL_NO;
          tn->usq[option] = CURL_EMPTY;
          break;
      }
      break;
  }
}


static void printsub(struct SessionHandle *data,
		     int direction,		/* '<' or '>' */
		     unsigned char *pointer,	/* where suboption data is */
		     int length)		/* length of suboption data */
{
  int i = 0;

  if (data->set.verbose)
  {
    if (direction)
    {
      Curl_infof(data, "%s IAC SB ", (direction == '<')? "RCVD":"SENT");
      if (length >= 3)
      {
        int j;

        i = pointer[length-2];
        j = pointer[length-1];

        if (i != CURL_IAC || j != CURL_SE)
        {
          Curl_infof(data, "(terminated by ");
          if (CURL_TELOPT_OK(i))
            Curl_infof(data, "%s ", CURL_TELOPT(i));
          else if (CURL_TELCMD_OK(i))
            Curl_infof(data, "%s ", CURL_TELCMD(i));
          else
            Curl_infof(data, "%d ", i);
          if (CURL_TELOPT_OK(j))
            Curl_infof(data, "%s", CURL_TELOPT(j));
          else if (CURL_TELCMD_OK(j))
            Curl_infof(data, "%s", CURL_TELCMD(j));
          else
            Curl_infof(data, "%d", j);
          Curl_infof(data, ", not IAC SE!) ");
        }
      }
      length -= 2;
    }
    if (length < 1)
    {
      Curl_infof(data, "(Empty suboption?)");
      return;
    }

    if (CURL_TELOPT_OK(pointer[0])) {
      switch(pointer[0]) {
        case CURL_TELOPT_TTYPE:
        case CURL_TELOPT_XDISPLOC:
        case CURL_TELOPT_NEW_ENVIRON:
          Curl_infof(data, "%s", CURL_TELOPT(pointer[0]));
          break;
        default:
          Curl_infof(data, "%s (unsupported)", CURL_TELOPT(pointer[0]));
          break;
      }
    }
    else
      Curl_infof(data, "%d (unknown)", pointer[i]);

    switch(pointer[1]) {
      case CURL_TELQUAL_IS:
        Curl_infof(data, " IS");
        break;
      case CURL_TELQUAL_SEND:
        Curl_infof(data, " SEND");
        break;
      case CURL_TELQUAL_INFO:
        Curl_infof(data, " INFO/REPLY");
        break;
      case CURL_TELQUAL_NAME:
        Curl_infof(data, " NAME");
        break;
    }
      
    switch(pointer[0]) {
      case CURL_TELOPT_TTYPE:
      case CURL_TELOPT_XDISPLOC:
        pointer[length] = 0;
        Curl_infof(data, " \"%s\"", &pointer[2]);
        break;
      case CURL_TELOPT_NEW_ENVIRON:
        if(pointer[1] == CURL_TELQUAL_IS) {
          Curl_infof(data, " ");
          for(i = 3;i < length;i++) {
            switch(pointer[i]) {
              case CURL_NEW_ENV_VAR:
                Curl_infof(data, ", ");
                break;
              case CURL_NEW_ENV_VALUE:
                Curl_infof(data, " = ");
                break;
              default:
                Curl_infof(data, "%c", pointer[i]);
                break;
            }
          }
        }
        break;
      default:
        for (i = 2; i < length; i++)
          Curl_infof(data, " %.2x", pointer[i]);
        break;
    }
      
    if (direction)
    {
      Curl_infof(data, "\n");
    }
  }
}

static CURLcode check_telnet_options(struct connectdata *conn)
{
  struct curl_slist *head;
  char option_keyword[128];
  char option_arg[256];
  char *buf;
  struct SessionHandle *data = conn->data;
  struct TELNET *tn = (struct TELNET *)conn->proto.telnet;

  /* Add the user name as an environment variable if it
     was given on the command line */
  if(conn->bits.user_passwd)
  {
    snprintf(option_arg, sizeof(option_arg), "USER,%s", conn->user);
    tn->telnet_vars = curl_slist_append(tn->telnet_vars, option_arg);

    tn->us_preferred[CURL_TELOPT_NEW_ENVIRON] = CURL_YES;
  }

  for(head = data->set.telnet_options; head; head=head->next) {
    if(sscanf(head->data, "%127[^= ]%*[ =]%255s",
              option_keyword, option_arg) == 2) {

      /* Terminal type */
      if(curl_strequal(option_keyword, "TTYPE")) {
        strncpy(tn->subopt_ttype, option_arg, 31);
        tn->subopt_ttype[31] = 0; /* String termination */
        tn->us_preferred[CURL_TELOPT_TTYPE] = CURL_YES;
        continue;
      }

      /* Display variable */
      if(curl_strequal(option_keyword, "XDISPLOC")) {
        strncpy(tn->subopt_xdisploc, option_arg, 127);
        tn->subopt_xdisploc[127] = 0; /* String termination */
        tn->us_preferred[CURL_TELOPT_XDISPLOC] = CURL_YES;
        continue;
      }

      /* Environment variable */
      if(curl_strequal(option_keyword, "NEW_ENV")) {
        buf = strdup(option_arg);
        if(!buf)
          return CURLE_OUT_OF_MEMORY;
        tn->telnet_vars = curl_slist_append(tn->telnet_vars, buf);
        tn->us_preferred[CURL_TELOPT_NEW_ENVIRON] = CURL_YES;
        continue;
      }

      failf(data, "Unknown telnet option %s", head->data);
      return CURLE_UNKNOWN_TELNET_OPTION;
    } else {
      failf(data, "Syntax error in telnet option: %s", head->data);
      return CURLE_TELNET_OPTION_SYNTAX;
    }
  }

  return CURLE_OK;
}

/*
 * suboption()
 *
 * Look at the sub-option buffer, and try to be helpful to the other
 * side.
 */

static void suboption(struct connectdata *conn)
{
  struct curl_slist *v;
  unsigned char temp[2048];
  int len;
  int tmplen;
  char varname[128];
  char varval[128];
  struct SessionHandle *data = conn->data;
  struct TELNET *tn = (struct TELNET *)conn->proto.telnet;

  printsub(data, '<', (unsigned char *)tn->subbuffer, CURL_SB_LEN(tn)+2);
  switch (CURL_SB_GET(tn)) {
    case CURL_TELOPT_TTYPE:
      len = strlen(tn->subopt_ttype) + 4 + 2;
      snprintf((char *)temp, sizeof(temp),
               "%c%c%c%c%s%c%c", CURL_IAC, CURL_SB, CURL_TELOPT_TTYPE,
               CURL_TELQUAL_IS, tn->subopt_ttype, CURL_IAC, CURL_SE);
      (void)swrite(conn->sock[FIRSTSOCKET], temp, len);
      printsub(data, '>', &temp[2], len-2);
      break;
    case CURL_TELOPT_XDISPLOC:
      len = strlen(tn->subopt_xdisploc) + 4 + 2;
      snprintf((char *)temp, sizeof(temp),
               "%c%c%c%c%s%c%c", CURL_IAC, CURL_SB, CURL_TELOPT_XDISPLOC,
               CURL_TELQUAL_IS, tn->subopt_xdisploc, CURL_IAC, CURL_SE);
      (void)swrite(conn->sock[FIRSTSOCKET], temp, len);
      printsub(data, '>', &temp[2], len-2);
      break;
    case CURL_TELOPT_NEW_ENVIRON:
      snprintf((char *)temp, sizeof(temp),
               "%c%c%c%c", CURL_IAC, CURL_SB, CURL_TELOPT_NEW_ENVIRON,
               CURL_TELQUAL_IS);
      len = 4;

      for(v = tn->telnet_vars;v;v = v->next) {
        tmplen = (strlen(v->data) + 1);
        /* Add the variable only if it fits */
        if(len + tmplen < (int)sizeof(temp)-6) {
          sscanf(v->data, "%127[^,],%s", varname, varval);
          snprintf((char *)&temp[len], sizeof(temp) - len,
                   "%c%s%c%s", CURL_NEW_ENV_VAR, varname,
                   CURL_NEW_ENV_VALUE, varval);
          len += tmplen;
        }
      }
      snprintf((char *)&temp[len], sizeof(temp) - len,
               "%c%c", CURL_IAC, CURL_SE);
      len += 2;
      (void)swrite(conn->sock[FIRSTSOCKET], temp, len);
      printsub(data, '>', &temp[2], len-2);
      break;
  }
  return;
}

static
void telrcv(struct connectdata *conn,
            unsigned char *inbuf,	/* Data received from socket */
            int count)			/* Number of bytes received */
{
  unsigned char c;
  int in = 0;
  struct SessionHandle *data = conn->data;
  struct TELNET *tn = (struct TELNET *)conn->proto.telnet;

  while(count--)
  {
    c = inbuf[in++];

    switch (tn->telrcv_state)
    {
      case CURL_TS_CR:
        tn->telrcv_state = CURL_TS_DATA;
        if (c == '\0')
        {
          break;   /* Ignore \0 after CR */
        }

        Curl_client_write(data, CLIENTWRITE_BODY, (char *)&c, 1);
        continue;

      case CURL_TS_DATA:
        if (c == CURL_IAC)
        {
          tn->telrcv_state = CURL_TS_IAC;
          break;
        }
        else if(c == '\r')
        {
          tn->telrcv_state = CURL_TS_CR;
        }

        Curl_client_write(data, CLIENTWRITE_BODY, (char *)&c, 1);
        continue;

      case CURL_TS_IAC:
      process_iac:
      switch (c)
      {
        case CURL_WILL:
          tn->telrcv_state = CURL_TS_WILL;
          continue;
        case CURL_WONT:
          tn->telrcv_state = CURL_TS_WONT;
          continue;
        case CURL_DO:
          tn->telrcv_state = CURL_TS_DO;
          continue;
        case CURL_DONT:
          tn->telrcv_state = CURL_TS_DONT;
          continue;
        case CURL_SB:
          CURL_SB_CLEAR(tn);
          tn->telrcv_state = CURL_TS_SB;
          continue;
        case CURL_IAC:
          Curl_client_write(data, CLIENTWRITE_BODY, (char *)&c, 1);
          break;
        case CURL_DM:
        case CURL_NOP:
        case CURL_GA:
        default:
          printoption(data, "RCVD", CURL_IAC, c);
          break;
      }
      tn->telrcv_state = CURL_TS_DATA;
      continue;

      case CURL_TS_WILL:
        printoption(data, "RCVD", CURL_WILL, c);
        tn->please_negotiate = 1;
        rec_will(conn, c);
        tn->telrcv_state = CURL_TS_DATA;
        continue;
      
      case CURL_TS_WONT:
        printoption(data, "RCVD", CURL_WONT, c);
        tn->please_negotiate = 1;
        rec_wont(conn, c);
        tn->telrcv_state = CURL_TS_DATA;
        continue;
      
      case CURL_TS_DO:
        printoption(data, "RCVD", CURL_DO, c);
        tn->please_negotiate = 1;
        rec_do(conn, c);
        tn->telrcv_state = CURL_TS_DATA;
        continue;
      
      case CURL_TS_DONT:
        printoption(data, "RCVD", CURL_DONT, c);
        tn->please_negotiate = 1;
        rec_dont(conn, c);
        tn->telrcv_state = CURL_TS_DATA;
        continue;

      case CURL_TS_SB:
        if (c == CURL_IAC)
        {
          tn->telrcv_state = CURL_TS_SE;
        }
        else
        {
          CURL_SB_ACCUM(tn,c);
        }
        continue;

      case CURL_TS_SE:
        if (c != CURL_SE)
        {
          if (c != CURL_IAC)
          {
            /*
             * This is an error.  We only expect to get
             * "IAC IAC" or "IAC SE".  Several things may
             * have happend.  An IAC was not doubled, the
             * IAC SE was left off, or another option got
             * inserted into the suboption are all possibilities.
             * If we assume that the IAC was not doubled,
             * and really the IAC SE was left off, we could
             * get into an infinate loop here.  So, instead,
             * we terminate the suboption, and process the
             * partial suboption if we can.
             */
            CURL_SB_ACCUM(tn, (unsigned char)CURL_IAC);
            CURL_SB_ACCUM(tn, c);
            tn->subpointer -= 2;
            CURL_SB_TERM(tn);
	    
            printoption(data, "In SUBOPTION processing, RCVD", CURL_IAC, c);
            suboption(conn);   /* handle sub-option */
            tn->telrcv_state = CURL_TS_IAC;
            goto process_iac;
          }
          CURL_SB_ACCUM(tn,c);
          tn->telrcv_state = CURL_TS_SB;
        }
        else
        {
          CURL_SB_ACCUM(tn, (unsigned char)CURL_IAC);
          CURL_SB_ACCUM(tn, (unsigned char)CURL_SE);
          tn->subpointer -= 2;
          CURL_SB_TERM(tn);
          suboption(conn);   /* handle sub-option */
          tn->telrcv_state = CURL_TS_DATA;
        }
        break;
    }
  }
}

CURLcode Curl_telnet_done(struct connectdata *conn)
{
  struct TELNET *tn = (struct TELNET *)conn->proto.telnet;
  curl_slist_free_all(tn->telnet_vars);

  free(conn->proto.telnet);
  conn->proto.telnet = NULL;

  return CURLE_OK;
}

CURLcode Curl_telnet(struct connectdata *conn)
{
  CURLcode code;
  struct SessionHandle *data = conn->data;
  curl_socket_t sockfd = conn->sock[FIRSTSOCKET];
#ifdef WIN32
  HMODULE wsock2;
  WSOCK2_FUNC close_event_func;
  WSOCK2_FUNC create_event_func;
  WSOCK2_FUNC event_select_func;
  WSOCK2_FUNC enum_netevents_func;
  WSAEVENT event_handle;
  WSANETWORKEVENTS events;
  HANDLE stdin_handle;
  HANDLE objs[2];
  DWORD waitret;
  DWORD readfile_read;
#else
  fd_set readfd;
  fd_set keepfd;
#endif
  ssize_t nread;  
  bool keepon = TRUE;
  char *buf = data->state.buffer;
  struct TELNET *tn;

  code = init_telnet(conn);
  if(code)
    return code;

  tn = (struct TELNET *)conn->proto.telnet;

  code = check_telnet_options(conn);
  if(code)
    return code;

#ifdef WIN32
  /*
  ** This functionality only works with WinSock >= 2.0.  So,
  ** make sure have it.
  */
  code = check_wsock2(data);
  if (code)
    return code;

  /* OK, so we have WinSock 2.0.  We need to dynamically */
  /* load ws2_32.dll and get the function pointers we need. */
  wsock2 = LoadLibrary("WS2_32.DLL");
  if (wsock2 == NULL) {
    failf(data,"failed to load WS2_32.DLL (%d)",GetLastError());
    return CURLE_FAILED_INIT;
  }

  /* Grab a pointer to WSACreateEvent */
  create_event_func = GetProcAddress(wsock2,"WSACreateEvent");
  if (create_event_func == NULL) {
    failf(data,"failed to find WSACreateEvent function (%d)",
	  GetLastError());
    FreeLibrary(wsock2);
    return CURLE_FAILED_INIT;
  }

  /* And WSACloseEvent */
  close_event_func = GetProcAddress(wsock2,"WSACloseEvent");
  if (create_event_func == NULL) {
    failf(data,"failed to find WSACloseEvent function (%d)",
	  GetLastError());
    FreeLibrary(wsock2);
    return CURLE_FAILED_INIT;
  }

  /* And WSAEventSelect */
  event_select_func = GetProcAddress(wsock2,"WSAEventSelect");
  if (event_select_func == NULL) {
    failf(data,"failed to find WSAEventSelect function (%d)",
	  GetLastError());
    FreeLibrary(wsock2);
    return CURLE_FAILED_INIT;
  }

  /* And WSAEnumNetworkEvents */
  enum_netevents_func = GetProcAddress(wsock2,"WSAEnumNetworkEvents");
  if (enum_netevents_func == NULL) {
    failf(data,"failed to find WSAEnumNetworkEvents function (%d)",
	  GetLastError());
    FreeLibrary(wsock2);
    return CURLE_FAILED_INIT;
  }

  /* We want to wait for both stdin and the socket. Since
  ** the select() function in winsock only works on sockets
  ** we have to use the WaitForMultipleObjects() call.
  */

  /* First, create a sockets event object */
  event_handle = (WSAEVENT)create_event_func();
  if (event_handle == WSA_INVALID_EVENT) {
    failf(data,"WSACreateEvent failed (%d)",WSAGetLastError());
    FreeLibrary(wsock2);
    return CURLE_FAILED_INIT;
  }

  /* The get the Windows file handle for stdin */
  stdin_handle = GetStdHandle(STD_INPUT_HANDLE);

  /* Create the list of objects to wait for */
  objs[0] = stdin_handle;
  objs[1] = event_handle;

  /* Tell winsock what events we want to listen to */
  if(event_select_func(sockfd, event_handle, FD_READ|FD_CLOSE) == SOCKET_ERROR) {
    close_event_func(event_handle);
    FreeLibrary(wsock2);
    return 0;
  }

  /* Keep on listening and act on events */
  while(keepon) {
    waitret = WaitForMultipleObjects(2, objs, FALSE, INFINITE);
    switch(waitret - WAIT_OBJECT_0) {
    case 0:
    {
      unsigned char outbuf[2];
      int out_count = 0;
      ssize_t bytes_written;
      char *buffer = buf;
              
      if(!ReadFile(stdin_handle, buf, sizeof(data->state.buffer),
                   &readfile_read, NULL)) {
        keepon = FALSE;
        break;
      }
      nread = readfile_read;
        
      while(nread--) {
        outbuf[0] = *buffer++;
        out_count = 1;
        if(outbuf[0] == CURL_IAC)
          outbuf[out_count++] = CURL_IAC;
          
        Curl_write(conn, conn->sock[FIRSTSOCKET], outbuf,
                   out_count, &bytes_written);
      }
    }
    break;
      
    case 1:
      if(enum_netevents_func(sockfd, event_handle, &events)
         != SOCKET_ERROR) {
        if(events.lNetworkEvents & FD_READ) {
          /* This reallu OUGHT to check its return code. */
          (void)Curl_read(conn, sockfd, buf, BUFSIZE - 1, &nread);
            
          telrcv(conn, (unsigned char *)buf, nread);
          
          fflush(stdout);
            
          /* Negotiate if the peer has started negotiating,
             otherwise don't. We don't want to speak telnet with
             non-telnet servers, like POP or SMTP. */
          if(tn->please_negotiate && !tn->already_negotiated) {
            negotiate(conn);
            tn->already_negotiated = 1;
          }
        }
          
        if(events.lNetworkEvents & FD_CLOSE) {
          keepon = FALSE;
        }
      }
      break;
    }
  }

  /* We called WSACreateEvent, so call WSACloseEvent */
  if (close_event_func(event_handle) == FALSE) {
    infof(data,"WSACloseEvent failed (%d)",WSAGetLastError());
  }

  /* "Forget" pointers into the library we're about to free */
  create_event_func = NULL;
  close_event_func = NULL;
  event_select_func = NULL;
  enum_netevents_func = NULL;

  /* We called LoadLibrary, so call FreeLibrary */
  if (!FreeLibrary(wsock2))
    infof(data,"FreeLibrary(wsock2) failed (%d)",GetLastError());
#else
  FD_ZERO (&readfd);		/* clear it */
  FD_SET (sockfd, &readfd);
  FD_SET (0, &readfd);

  keepfd = readfd;

  while (keepon) {
    struct timeval interval;

    readfd = keepfd;		/* set this every lap in the loop */
    interval.tv_sec = 1;
    interval.tv_usec = 0;

    switch (select (sockfd + 1, &readfd, NULL, NULL, &interval)) {
    case -1:			/* error, stop reading */
      keepon = FALSE;
      continue;
    case 0:			/* timeout */
      break;
    default:			/* read! */
      if(FD_ISSET(0, &readfd)) { /* read from stdin */
        unsigned char outbuf[2];
        int out_count = 0;
        ssize_t bytes_written;
        char *buffer = buf;
        
        nread = read(0, buf, 255);

        while(nread--) {
          outbuf[0] = *buffer++;
          out_count = 1;
          if(outbuf[0] == CURL_IAC)
            outbuf[out_count++] = CURL_IAC;
      
          Curl_write(conn, conn->sock[FIRSTSOCKET], outbuf,
                     out_count, &bytes_written);
        }
      }

      if(FD_ISSET(sockfd, &readfd)) {
        /* This OUGHT to check the return code... */
        (void)Curl_read(conn, sockfd, buf, BUFSIZE - 1, &nread);

        /* if we receive 0 or less here, the server closed the connection and
           we bail out from this! */
        if (nread <= 0) {
          keepon = FALSE;
          break;
        }

        telrcv(conn, (unsigned char *)buf, nread);

        /* Negotiate if the peer has started negotiating,
           otherwise don't. We don't want to speak telnet with
           non-telnet servers, like POP or SMTP. */
        if(tn->please_negotiate && !tn->already_negotiated) {
          negotiate(conn);
          tn->already_negotiated = 1;
        }
      }
    }
    if(data->set.timeout) {
      struct timeval now;           /* current time */
      now = Curl_tvnow();
      if(Curl_tvdiff(now, conn->created)/1000 >= data->set.timeout) {
        failf(data, "Time-out");
        code = CURLE_OPERATION_TIMEOUTED;
        keepon = FALSE;
      }
    }
  }
#endif
  /* mark this as "no further transfer wanted" */
  Curl_Transfer(conn, -1, -1, FALSE, NULL, -1, NULL);

  return code;
}
#endif
