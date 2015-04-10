#ifndef __HTTP_NTLM_H
#define __HTTP_NTLM_H
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
 * $Id: http_ntlm.h,v 1.6 2004/01/07 09:19:35 bagder Exp $
 ***************************************************************************/

typedef enum {
  CURLNTLM_NONE, /* not a ntlm */
  CURLNTLM_BAD,  /* an ntlm, but one we don't like */
  CURLNTLM_FIRST, /* the first 401-reply we got with NTLM */
  CURLNTLM_FINE, /* an ntlm we act on */

  CURLNTLM_LAST  /* last entry in this enum, don't use */
} CURLntlm;

/* this is for ntlm header input */
CURLntlm Curl_input_ntlm(struct connectdata *conn, bool proxy, char *header);

/* this is for creating ntlm header output */
CURLcode Curl_output_ntlm(struct connectdata *conn, bool proxy, bool *ready);

void Curl_ntlm_cleanup(struct SessionHandle *data);


/* Flag bits definitions based on http://davenport.sourceforge.net/ntlm.html */

#define NTLMFLAG_NEGOTIATE_UNICODE               (1<<0)
/* Indicates that Unicode strings are supported for use in security buffer
   data. */

#define NTLMFLAG_NEGOTIATE_OEM                   (1<<1)
/* Indicates that OEM strings are supported for use in security buffer data. */

#define NTLMFLAG_REQUEST_TARGET                  (1<<2)
/* Requests that the server's authentication realm be included in the Type 2
   message. */

/* unknown (1<<3) */
#define NTLMFLAG_NEGOTIATE_SIGN                  (1<<4)
/* Specifies that authenticated communication between the client and server
   should carry a digital signature (message integrity). */

#define NTLMFLAG_NEGOTIATE_SEAL                  (1<<5)
/* Specifies that authenticated communication between the client and server
   should be encrypted (message confidentiality). */

#define NTLMFLAG_NEGOTIATE_DATAGRAM_STYLE        (1<<6)
/* unknown purpose */

#define NTLMFLAG_NEGOTIATE_LM_KEY                (1<<7)
/* Indicates that the LAN Manager session key should be used for signing and
   sealing authenticated communications. */

#define NTLMFLAG_NEGOTIATE_NETWARE               (1<<8)
/* unknown purpose */

#define NTLMFLAG_NEGOTIATE_NTLM_KEY              (1<<9)
/* Indicates that NTLM authentication is being used. */

/* unknown (1<<10) */
/* unknown (1<<11) */

#define NTLMFLAG_NEGOTIATE_DOMAIN_SUPPLIED       (1<<12)
/* Sent by the client in the Type 1 message to indicate that a desired
   authentication realm is included in the message. */

#define NTLMFLAG_NEGOTIATE_WORKSTATION_SUPPLIED  (1<<13)
/* Sent by the client in the Type 1 message to indicate that the client
   workstation's name is included in the message. */

#define NTLMFLAG_NEGOTIATE_LOCAL_CALL            (1<<14)
/* Sent by the server to indicate that the server and client are on the same
   machine. Implies that the client may use a pre-established local security
   context rather than responding to the challenge. */

#define NTLMFLAG_NEGOTIATE_ALWAYS_SIGN           (1<<15)
/* Indicates that authenticated communication between the client and server
   should be signed with a "dummy" signature. */

#define NTLMFLAG_TARGET_TYPE_DOMAIN              (1<<16)
/* Sent by the server in the Type 2 message to indicate that the target
   authentication realm is a domain. */

#define NTLMFLAG_TARGET_TYPE_SERVER              (1<<17)
/* Sent by the server in the Type 2 message to indicate that the target
   authentication realm is a server. */

#define NTLMFLAG_TARGET_TYPE_SHARE               (1<<18)
/* Sent by the server in the Type 2 message to indicate that the target
   authentication realm is a share. Presumably, this is for share-level
   authentication. Usage is unclear. */

#define NTLMFLAG_NEGOTIATE_NTLM2_KEY             (1<<19)
/* Indicates that the NTLM2 signing and sealing scheme should be used for
   protecting authenticated communications. */

#define NTLMFLAG_REQUEST_INIT_RESPONSE           (1<<20)
/* unknown purpose */

#define NTLMFLAG_REQUEST_ACCEPT_RESPONSE         (1<<21)
/* unknown purpose */

#define NTLMFLAG_REQUEST_NONNT_SESSION_KEY       (1<<22)
/* unknown purpose */

#define NTLMFLAG_NEGOTIATE_TARGET_INFO           (1<<23)
/* Sent by the server in the Type 2 message to indicate that it is including a
   Target Information block in the message. */

/* unknown (1<24) */
/* unknown (1<25) */
/* unknown (1<26) */
/* unknown (1<27) */
/* unknown (1<28) */

#define NTLMFLAG_NEGOTIATE_128                   (1<<29)
/* Indicates that 128-bit encryption is supported. */

#define NTLMFLAG_NEGOTIATE_KEY_EXCHANGE          (1<<30)
/* unknown purpose */

#define NTLMFLAG_NEGOTIATE_56                    (1<<31)
/* Indicates that 56-bit encryption is supported. */
#endif
