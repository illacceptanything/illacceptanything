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
 * $Id: http_negotiate.c,v 1.7 2004/01/07 09:19:35 bagder Exp $
 ***************************************************************************/
#include "setup.h"

#ifdef HAVE_GSSAPI
#ifdef HAVE_GSSMIT
#define GSS_C_NT_HOSTBASED_SERVICE gss_nt_service_name
#endif

#ifndef CURL_DISABLE_HTTP
/* -- WIN32 approved -- */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "urldata.h"
#include "sendf.h"
#include "strequal.h"
#include "base64.h"
#include "http_negotiate.h"

#define _MPRINTF_REPLACE /* use our functions only */
#include <curl/mprintf.h>

/* The last #include file should be: */
#ifdef CURLDEBUG
#include "memdebug.h"
#endif

static int
get_gss_name(struct connectdata *conn, gss_name_t *server)
{
  struct negotiatedata *neg_ctx = &conn->data->state.negotiate;
  OM_uint32 major_status, minor_status;
  gss_buffer_desc token = GSS_C_EMPTY_BUFFER;
  char name[2048];
  const char* service;

  /* GSSAPI implementation by Globus (known as GSI) requires the name to be
     of form "<service>/<fqdn>" instead of <service>@<fqdn> (ie. slash instead
     of at-sign). Also GSI servers are often identified as 'host' not 'khttp'.
     Change following lines if you want to use GSI */

  /* IIS uses the <service>@<fqdn> form but uses 'http' as the service name */
  
  if (neg_ctx->gss) 
    service = "khttp";
  else
    service = "http";

  token.length = strlen(service) + 1 + strlen(conn->hostname) + 1;
  if (token.length + 1 > sizeof(name))
    return EMSGSIZE;
  sprintf(name, "%s@%s", service, conn->hostname);

  token.value = (void *) name;
  major_status = gss_import_name(&minor_status,
                                 &token,
                                 GSS_C_NT_HOSTBASED_SERVICE,
                                 server);

  return GSS_ERROR(major_status) ? -1 : 0;
}

static void
log_gss_error(struct connectdata *conn, OM_uint32 error_status, char *prefix)
{
  OM_uint32 maj_stat, min_stat;
  OM_uint32 msg_ctx = 0;
  gss_buffer_desc status_string;
  char buf[1024];
  size_t len;

  snprintf(buf, sizeof(buf), "%s", prefix);
  len = strlen(buf);
  do {
    maj_stat = gss_display_status (&min_stat,
                                   error_status,
                                   GSS_C_MECH_CODE,
                                   GSS_C_NO_OID,
                                   &msg_ctx,
                                   &status_string);
    if (sizeof(buf) > len + status_string.length + 1) {
      sprintf(buf + len, ": %s", (char*) status_string.value);
      len += status_string.length;
    }
    gss_release_buffer(&min_stat, &status_string);
  } while (!GSS_ERROR(maj_stat) && msg_ctx != 0);

  infof(conn->data, buf);
}

int Curl_input_negotiate(struct connectdata *conn, char *header)
{ 
  struct negotiatedata *neg_ctx = &conn->data->state.negotiate;
  OM_uint32 major_status, minor_status, minor_status2;
  gss_buffer_desc input_token = GSS_C_EMPTY_BUFFER;
  gss_buffer_desc output_token = GSS_C_EMPTY_BUFFER;
  int ret;
  size_t len;
  bool gss;
  const char* protocol;

  while(*header && isspace((int)*header))
    header++;
  if(checkprefix("GSS-Negotiate", header)) {
    protocol = "GSS-Negotiate";
    gss = TRUE;
  }
  else if (checkprefix("Negotiate", header)) {
    protocol = "Negotiate";
    gss = FALSE;
  }
  else
    return -1;

  if (neg_ctx->context) {
    if (neg_ctx->gss != gss) {
      return -1;
    }
  }
  else {
    neg_ctx->protocol = protocol;
    neg_ctx->gss = gss;
  }
    
  if (neg_ctx->context && neg_ctx->status == GSS_S_COMPLETE) {
    /* We finished succesfully our part of authentication, but server
     * rejected it (since we're again here). Exit with an error since we
     * can't invent anything better */
    Curl_cleanup_negotiate(conn->data);
    return -1;
  }

  if (neg_ctx->server_name == NULL &&
      (ret = get_gss_name(conn, &neg_ctx->server_name)))
    return ret;

  header += strlen(neg_ctx->protocol);
  while(*header && isspace((int)*header))
    header++;

  len = strlen(header);
  if (len > 0) {
    int rawlen;
    input_token.length = (len+3)/4 * 3;
    input_token.value = malloc(input_token.length);
    if (input_token.value == NULL)
      return ENOMEM;
    rawlen = Curl_base64_decode(header, input_token.value);
    if (rawlen < 0)
      return -1;
    input_token.length = rawlen;

#ifdef HAVE_SPNEGO /* Handle SPNEGO */
    if (checkprefix("Negotiate", header)) {
        ASN1_OBJECT *   object            = NULL;
        int             rc                = 1;
        unsigned char * spnegoToken       = NULL;
        size_t          spnegoTokenLength = 0;
        unsigned char * mechToken         = NULL;
        size_t          mechTokenLength   = 0;

        spnegoToken = malloc(input_token.length);
        if (input_token.value == NULL)
          return ENOMEM;
        spnegoTokenLength = input_token.length;

        object = OBJ_txt2obj ("1.2.840.113554.1.2.2", 1);
        if (!parseSpnegoTargetToken(spnegoToken,
                                    spnegoTokenLength,
                                    NULL,
                                    NULL,
                                    &mechToken,
                                    &mechTokenLength,
                                    NULL,
                                    NULL)) {
          free(spnegoToken);
          spnegoToken = NULL;
          infof(conn->data, "Parse SPNEGO Target Token failed\n");
        }
        else {
          free(input_token.value);
          input_token.value = NULL;
          input_token.value = malloc(mechTokenLength);
          memcpy(input_token.value, mechToken,mechTokenLength);
          input_token.length = mechTokenLength;
          free(mechToken);
          mechToken = NULL;
          infof(conn->data, "Parse SPNEGO Target Token succeded\n");
        }
    }
#endif
  }

  major_status = gss_init_sec_context(&minor_status,
                                      GSS_C_NO_CREDENTIAL,
                                      &neg_ctx->context,
                                      neg_ctx->server_name,
                                      GSS_C_NO_OID,
                                      GSS_C_DELEG_FLAG,
                                      0,
                                      GSS_C_NO_CHANNEL_BINDINGS,
                                      &input_token,
                                      NULL,
                                      &output_token,
                                      NULL,
                                      NULL);
  if (input_token.length > 0)
    gss_release_buffer(&minor_status2, &input_token);
  neg_ctx->status = major_status;
  if (GSS_ERROR(major_status)) {
    /* Curl_cleanup_negotiate(conn->data) ??? */
    log_gss_error(conn, minor_status,
                  (char *)"gss_init_sec_context() failed: ");
    return -1;
  }

  if (output_token.length == 0) {
    return -1;
  }

  neg_ctx->output_token = output_token;
  /* conn->bits.close = FALSE; */

  return 0;
}
   

CURLcode Curl_output_negotiate(struct connectdata *conn)
{ 
  struct negotiatedata *neg_ctx = &conn->data->state.negotiate;
  OM_uint32 minor_status;
  char *encoded = NULL;
  int len;

#ifdef HAVE_SPNEGO /* Handle SPNEGO */
  if (checkprefix("Negotiate",neg_ctx->protocol)) {
    ASN1_OBJECT *   object            = NULL;
    int             rc                = 1;
    unsigned char * spnegoToken       = NULL;
    size_t          spnegoTokenLength = 0;
    unsigned char * responseToken       = NULL;
    size_t          responseTokenLength = 0;
    
    responseToken = malloc(neg_ctx->output_token.length);
    if ( responseToken == NULL)
      return CURLE_OUT_OF_MEMORY;
    memcpy(responseToken, neg_ctx->output_token.value,
           neg_ctx->output_token.length);
    responseTokenLength = neg_ctx->output_token.length;

    object=OBJ_txt2obj ("1.2.840.113554.1.2.2", 1);
    if (!makeSpnegoInitialToken (object,
                                 responseToken,
                                 responseTokenLength,
                                 &spnegoToken,
                                 &spnegoTokenLength)) {
      free(responseToken);
      responseToken = NULL;
      infof(conn->data, "Make SPNEGO Initial Token failed\n");
    }
    else {
      free(neg_ctx->output_token.value);
      responseToken = NULL;
      neg_ctx->output_token.value = malloc(spnegoTokenLength);
      memcpy(neg_ctx->output_token.value, spnegoToken,spnegoTokenLength);
      neg_ctx->output_token.length = spnegoTokenLength;
      free(spnegoToken);
      spnegoToken = NULL;
      infof(conn->data, "Make SPNEGO Initial Token succeded\n");
    }
  }
#endif
  len = Curl_base64_encode(neg_ctx->output_token.value,
                           neg_ctx->output_token.length,
                           &encoded);

  if (len < 0)
    return CURLE_OUT_OF_MEMORY;

  conn->allocptr.userpwd =
    aprintf("Authorization: %s %s\r\n", neg_ctx->protocol, encoded);
  free(encoded);
  gss_release_buffer(&minor_status, &neg_ctx->output_token);
  return (conn->allocptr.userpwd == NULL) ? CURLE_OUT_OF_MEMORY : CURLE_OK;
}

void Curl_cleanup_negotiate(struct SessionHandle *data)
{ 
  OM_uint32 minor_status;
  struct negotiatedata *neg_ctx = &data->state.negotiate;

  if (neg_ctx->context != GSS_C_NO_CONTEXT)
    gss_delete_sec_context(&minor_status, &neg_ctx->context, GSS_C_NO_BUFFER);

  if (neg_ctx->output_token.length != 0)
    gss_release_buffer(&minor_status, &neg_ctx->output_token);

  if (neg_ctx->server_name != GSS_C_NO_NAME)
    gss_release_name(&minor_status, &neg_ctx->server_name);
  
  memset(neg_ctx, 0, sizeof(*neg_ctx));
}


#endif
#endif
