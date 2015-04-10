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
 * $Id: http_ntlm.c,v 1.28 2004/03/08 16:20:51 bagder Exp $
 ***************************************************************************/
#include "setup.h"

/* NTLM details:
   
   http://davenport.sourceforge.net/ntlm.html
   http://www.innovation.ch/java/ntlm.html

*/

#ifndef CURL_DISABLE_HTTP
#ifdef USE_SSLEAY
/* We need OpenSSL for the crypto lib to provide us with MD4 and DES */

/* -- WIN32 approved -- */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>

#include "urldata.h"
#include "sendf.h"
#include "strequal.h"
#include "base64.h"
#include "http_ntlm.h"
#include "url.h"
#include "http.h" /* for Curl_http_auth_stage() */

#define _MPRINTF_REPLACE /* use our functions only */
#include <curl/mprintf.h>

#include <openssl/des.h>
#include <openssl/md4.h>
#include <openssl/ssl.h>

#if OPENSSL_VERSION_NUMBER < 0x00907001L
#define DES_key_schedule des_key_schedule
#define DES_cblock des_cblock
#define DES_set_odd_parity des_set_odd_parity
#define DES_set_key des_set_key
#define DES_ecb_encrypt des_ecb_encrypt

/* This is how things were done in the old days */
#define DESKEY(x) x
#define DESKEYARG(x) x
#else
/* Modern version */
#define DESKEYARG(x) *x
#define DESKEY(x) &x
#endif

/* The last #include file should be: */
#ifdef CURLDEBUG
#include "memdebug.h"
#endif

/* Define this to make the type-3 message include the NT response message */
#undef USE_NTRESPONSES

/*
  (*) = A "security buffer" is a triplet consisting of two shorts and one
  long:

  1. a 'short' containing the length of the buffer in bytes
  2. a 'short' containing the allocated space for the buffer in bytes
  3. a 'long' containing the offset to the start of the buffer from the
     beginning of the NTLM message, in bytes.
*/


CURLntlm Curl_input_ntlm(struct connectdata *conn,
                         bool proxy,   /* if proxy or not */
                         char *header) /* rest of the www-authenticate:
                                          header */
{
  /* point to the correct struct with this */
  struct ntlmdata *ntlm;

  ntlm = proxy?&conn->proxyntlm:&conn->ntlm;

  /* skip initial whitespaces */
  while(*header && isspace((int)*header))
    header++;

  if(checkprefix("NTLM", header)) {
    unsigned char buffer[256];
    header += strlen("NTLM");

    while(*header && isspace((int)*header))
      header++;

    if(*header) {
      /* We got a type-2 message here:

         Index   Description         Content
         0       NTLMSSP Signature   Null-terminated ASCII "NTLMSSP"
                                     (0x4e544c4d53535000)
         8       NTLM Message Type   long (0x02000000)
         12      Target Name         security buffer(*)
         20      Flags               long
         24      Challenge           8 bytes
         (32)    Context (optional)  8 bytes (two consecutive longs)
         (40)    Target Information  (optional) security buffer(*)
         32 (48) start of data block
      */

      size_t size = Curl_base64_decode(header, (char *)buffer);

      ntlm->state = NTLMSTATE_TYPE2; /* we got a type-2 */

      if(size >= 48)
        /* the nonce of interest is index [24 .. 31], 8 bytes */
        memcpy(ntlm->nonce, &buffer[24], 8);

      /* at index decimal 20, there's a 32bit NTLM flag field */
      
    }
    else {
      if(ntlm->state >= NTLMSTATE_TYPE1)
        return CURLNTLM_BAD;

      ntlm->state = NTLMSTATE_TYPE1; /* we should sent away a type-1 */
    }
  }
  return CURLNTLM_FINE;
}

/*
 * Turns a 56 bit key into the 64 bit, odd parity key and sets the key.  The
 * key schedule ks is also set.
 */
static void setup_des_key(unsigned char *key_56,
                          DES_key_schedule DESKEYARG(ks))
{
  DES_cblock key;

  key[0] = key_56[0];
  key[1] = ((key_56[0] << 7) & 0xFF) | (key_56[1] >> 1);
  key[2] = ((key_56[1] << 6) & 0xFF) | (key_56[2] >> 2);
  key[3] = ((key_56[2] << 5) & 0xFF) | (key_56[3] >> 3);
  key[4] = ((key_56[3] << 4) & 0xFF) | (key_56[4] >> 4);
  key[5] = ((key_56[4] << 3) & 0xFF) | (key_56[5] >> 5);
  key[6] = ((key_56[5] << 2) & 0xFF) | (key_56[6] >> 6);
  key[7] =  (key_56[6] << 1) & 0xFF;

  DES_set_odd_parity(&key);
  DES_set_key(&key, ks);
}

 /*
  * takes a 21 byte array and treats it as 3 56-bit DES keys. The
  * 8 byte plaintext is encrypted with each key and the resulting 24
  * bytes are stored in the results array.
  */
static void calc_resp(unsigned char *keys,
                      unsigned char *plaintext,
                      unsigned char *results)
{
  DES_key_schedule ks;

  setup_des_key(keys, DESKEY(ks));
  DES_ecb_encrypt((DES_cblock*) plaintext, (DES_cblock*) results,
                  DESKEY(ks), DES_ENCRYPT);

  setup_des_key(keys+7, DESKEY(ks));
  DES_ecb_encrypt((DES_cblock*) plaintext, (DES_cblock*) (results+8),
                  DESKEY(ks), DES_ENCRYPT);

  setup_des_key(keys+14, DESKEY(ks));
  DES_ecb_encrypt((DES_cblock*) plaintext, (DES_cblock*) (results+16),
                  DESKEY(ks), DES_ENCRYPT);
}

/*
 * Set up lanmanager and nt hashed passwords
 */
static void mkhash(char *password,
                   unsigned char *nonce,  /* 8 bytes */
                   unsigned char *lmresp  /* must fit 0x18 bytes */
#ifdef USE_NTRESPONSES
                   , unsigned char *ntresp  /* must fit 0x18 bytes */
#endif
  )
{
  unsigned char lmbuffer[21];
#ifdef USE_NTRESPONSES
  unsigned char ntbuffer[21];
#endif
  unsigned char *pw;
  static const unsigned char magic[] = {
    0x4B, 0x47, 0x53, 0x21, 0x40, 0x23, 0x24, 0x25
  };
  unsigned int i;
  size_t len = strlen(password);

  /* make it fit at least 14 bytes */
  pw = malloc(len<7?14:len*2);
  if(!pw)
    return; /* this will lead to a badly generated package */

  if (len > 14)
    len = 14;
  
  for (i=0; i<len; i++)
    pw[i] = toupper(password[i]);

  for (; i<14; i++)
    pw[i] = 0;

  {
    /* create LanManager hashed password */
    DES_key_schedule ks;

    setup_des_key(pw, DESKEY(ks));
    DES_ecb_encrypt((DES_cblock *)magic, (DES_cblock *)lmbuffer,
                    DESKEY(ks), DES_ENCRYPT);
  
    setup_des_key(pw+7, DESKEY(ks));
    DES_ecb_encrypt((DES_cblock *)magic, (DES_cblock *)(lmbuffer+8),
                    DESKEY(ks), DES_ENCRYPT);

    memset(lmbuffer+16, 0, 5);
  }
  /* create LM responses */
  calc_resp(lmbuffer, nonce, lmresp);

#ifdef USE_NTRESPONSES
  {
    /* create NT hashed password */
    MD4_CTX MD4;

    len = strlen(password);

    for (i=0; i<len; i++) {
      pw[2*i]   = password[i];
      pw[2*i+1] = 0;
    }

    MD4_Init(&MD4);
    MD4_Update(&MD4, pw, 2*len);
    MD4_Final(ntbuffer, &MD4);

    memset(ntbuffer+16, 0, 8);
  }

  calc_resp(ntbuffer, nonce, ntresp);
#endif

  free(pw);
}

#define SHORTPAIR(x) ((x) & 0xff), ((x) >> 8)
#define LONGQUARTET(x) ((x) & 0xff), (((x) >> 8)&0xff), \
  (((x) >>16)&0xff), ((x)>>24)

/* this is for creating ntlm header output */
CURLcode Curl_output_ntlm(struct connectdata *conn,
                          bool proxy,
                          bool *ready)
{
  const char *domain=""; /* empty */
  const char *host=""; /* empty */
  int domlen=(int)strlen(domain);
  int hostlen = (int)strlen(host);
  int hostoff; /* host name offset */
  int domoff;  /* domain name offset */
  size_t size;
  char *base64=NULL;
  unsigned char ntlmbuf[256]; /* enough, unless the host/domain is very long */

  /* point to the address of the pointer that holds the string to sent to the
     server, which is for a plain host or for a HTTP proxy */
  char **allocuserpwd;

  /* point to the name and password for this */
  char *userp;
  char *passwdp;
  /* point to the correct struct with this */
  struct ntlmdata *ntlm;

  *ready = FALSE;

  if(proxy) {
    allocuserpwd = &conn->allocptr.proxyuserpwd;
    userp = conn->proxyuser;
    passwdp = conn->proxypasswd;
    ntlm = &conn->proxyntlm;
  }
  else {
    allocuserpwd = &conn->allocptr.userpwd;
    userp = conn->user;
    passwdp = conn->passwd;
    ntlm = &conn->ntlm;
  }

  /* not set means empty */
  if(!userp)
    userp=(char *)"";

  if(!passwdp)
    passwdp=(char *)"";
  
  switch(ntlm->state) {
  case NTLMSTATE_TYPE1:
  default: /* for the weird cases we (re)start here */
    hostoff = 32;
    domoff = hostoff + hostlen;
    
    /* Create and send a type-1 message:

    Index Description          Content
    0     NTLMSSP Signature    Null-terminated ASCII "NTLMSSP"
                               (0x4e544c4d53535000)
    8     NTLM Message Type    long (0x01000000)
    12    Flags                long
    16    Supplied Domain      security buffer(*)
    24    Supplied Workstation security buffer(*)
    32    start of data block

    */

    snprintf((char *)ntlmbuf, sizeof(ntlmbuf), "NTLMSSP%c"
             "\x01%c%c%c" /* 32-bit type = 1 */
             "%c%c%c%c"   /* 32-bit NTLM flag field */
             "%c%c"  /* domain length */
             "%c%c"  /* domain allocated space */
             "%c%c"  /* domain name offset */
             "%c%c"  /* 2 zeroes */
             "%c%c"  /* host length */
             "%c%c"  /* host allocated space */
             "%c%c"  /* host name offset */
             "%c%c"  /* 2 zeroes */
             "%s"   /* host name */
             "%s",  /* domain string */
             0,     /* trailing zero */
             0,0,0, /* part of type-1 long */

             LONGQUARTET(
               NTLMFLAG_NEGOTIATE_OEM|      /*   2 */
               NTLMFLAG_NEGOTIATE_NTLM_KEY  /* 200 */
               /* equals 0x0202 */
               ),
             SHORTPAIR(domlen),
             SHORTPAIR(domlen),
             SHORTPAIR(domoff),
             0,0,
             SHORTPAIR(hostlen),
             SHORTPAIR(hostlen),
             SHORTPAIR(hostoff),
             0,0,
             host, domain);

    /* initial packet length */
    size = 32 + hostlen + domlen;

    /* now keeper of the base64 encoded package size */
    size = Curl_base64_encode((char *)ntlmbuf, size, &base64);

    if(size >0 ) {
      Curl_safefree(*allocuserpwd);
      *allocuserpwd = aprintf("%sAuthorization: NTLM %s\r\n",
                              proxy?"Proxy-":"",
                              base64);
      free(base64);
    }
    else
      return CURLE_OUT_OF_MEMORY; /* FIX TODO */

    break;
    
  case NTLMSTATE_TYPE2:
    /* We received the type-2 already, create a type-3 message:

    Index   Description            Content
    0       NTLMSSP Signature      Null-terminated ASCII "NTLMSSP"
                                   (0x4e544c4d53535000)
    8       NTLM Message Type      long (0x03000000)
    12      LM/LMv2 Response       security buffer(*)
    20      NTLM/NTLMv2 Response   security buffer(*)
    28      Domain Name            security buffer(*)
    36      User Name              security buffer(*)
    44      Workstation Name       security buffer(*)
    (52)    Session Key (optional) security buffer(*)
    (60)    Flags (optional)       long
    52 (64) start of data block

    */
  
  {
    int lmrespoff;
    int ntrespoff;
    int useroff;
    unsigned char lmresp[0x18]; /* fixed-size */
#ifdef USE_NTRESPONSES
    unsigned char ntresp[0x18]; /* fixed-size */
#endif
    const char *user;
    int userlen;

    user = strchr(userp, '\\');
    if(!user)
      user = strchr(userp, '/');

    if (user) {
      domain = userp;
      domlen = user - domain;
      user++;
    }
    else
      user = userp;
    userlen = strlen(user);

    mkhash(passwdp, &ntlm->nonce[0], lmresp
#ifdef USE_NTRESPONSES
           , ntresp
#endif
      );

    domoff = 64; /* always */
    useroff = domoff + domlen;
    hostoff = useroff + userlen;
    lmrespoff = hostoff + hostlen;
    ntrespoff = lmrespoff + 0x18;

    /* Create the big type-3 message binary blob */
    size = snprintf((char *)ntlmbuf, sizeof(ntlmbuf),
                    "NTLMSSP%c"
                    "\x03%c%c%c" /* type-3, 32 bits */

                    "%c%c%c%c" /* LanManager length + allocated space */
                    "%c%c" /* LanManager offset */
                    "%c%c" /* 2 zeroes */

                    "%c%c" /* NT-response length */
                    "%c%c" /* NT-response allocated space */
                    "%c%c" /* NT-response offset */
                    "%c%c" /* 2 zeroes */
                    
                    "%c%c"  /* domain length */
                    "%c%c"  /* domain allocated space */
                    "%c%c"  /* domain name offset */
                    "%c%c"  /* 2 zeroes */
                    
                    "%c%c"  /* user length */
                    "%c%c"  /* user allocated space */
                    "%c%c"  /* user offset */
                    "%c%c"  /* 2 zeroes */
                    
                    "%c%c"  /* host length */
                    "%c%c"  /* host allocated space */
                    "%c%c"  /* host offset */
                    "%c%c%c%c%c%c"  /* 6 zeroes */
                    
                    "\xff\xff"  /* message length */
                    "%c%c"  /* 2 zeroes */
                    
                    "\x01\x82" /* flags */
                    "%c%c"  /* 2 zeroes */

                    /* domain string */
                    /* user string */
                    /* host string */
                    /* LanManager response */
                    /* NT response */
                    ,
                    0, /* zero termination */
                    0,0,0, /* type-3 long, the 24 upper bits */

                    SHORTPAIR(0x18),  /* LanManager response length, twice */
                    SHORTPAIR(0x18),
                    SHORTPAIR(lmrespoff),
                    0x0, 0x0,
                    
#ifdef USE_NTRESPONSES
                    SHORTPAIR(0x18),  /* NT-response length, twice */
                    SHORTPAIR(0x18),
#else
                    0x0, 0x0,
                    0x0, 0x0,
#endif
                    SHORTPAIR(ntrespoff),
                    0x0, 0x0,

                    SHORTPAIR(domlen),
                    SHORTPAIR(domlen),
                    SHORTPAIR(domoff),
                    0x0, 0x0,

                    SHORTPAIR(userlen),
                    SHORTPAIR(userlen),
                    SHORTPAIR(useroff),
                    0x0, 0x0,
                    
                    SHORTPAIR(hostlen),
                    SHORTPAIR(hostlen),
                    SHORTPAIR(hostoff),
                    0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
             
                    0x0, 0x0,

                    0x0, 0x0);

    /* size is now 64 */
    size=64;
    ntlmbuf[62]=ntlmbuf[63]=0;

    memcpy(&ntlmbuf[size], domain, domlen);
    size += domlen;

    memcpy(&ntlmbuf[size], user, userlen);
    size += userlen;

    /* we append the binary hashes to the end of the blob */
    if(size < ((int)sizeof(ntlmbuf) - 0x18)) {
      memcpy(&ntlmbuf[size], lmresp, 0x18);
      size += 0x18;
    }

#ifdef USE_NTRESPONSES
    if(size < ((int)sizeof(ntlmbuf) - 0x18)) {      
      memcpy(&ntlmbuf[size], ntresp, 0x18);
      size += 0x18;
    }
#endif

    ntlmbuf[56] = size & 0xff;
    ntlmbuf[57] = size >> 8;

    /* convert the binary blob into base64 */
    size = Curl_base64_encode((char *)ntlmbuf, size, &base64);

    if(size >0 ) {
      Curl_safefree(*allocuserpwd);
      *allocuserpwd = aprintf("%sAuthorization: NTLM %s\r\n",
                              proxy?"Proxy-":"",
                              base64);
      free(base64);
    }
    else
      return CURLE_OUT_OF_MEMORY; /* FIX TODO */

    ntlm->state = NTLMSTATE_TYPE3; /* we sent a type-3 */
    *ready = TRUE;

    /* Switch to web authentication after proxy authentication is done */
    if (proxy)
      Curl_http_auth_stage(conn->data, 401);
  }
  break;

  case NTLMSTATE_TYPE3:
    /* connection is already authenticated,
     * don't send a header in future requests */
    if(*allocuserpwd) {
      free(*allocuserpwd);
      *allocuserpwd=NULL;
    }
    *ready = TRUE;
    break;
  }

  return CURLE_OK;
}
#endif /* USE_SSLEAY */
#endif /* !CURL_DISABLE_HTTP */
