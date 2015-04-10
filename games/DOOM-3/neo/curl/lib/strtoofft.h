#ifndef _CURL_STRTOOFFT_H
#define _CURL_STRTOOFFT_H
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
 * $Id: strtoofft.h,v 1.9 2004/03/03 13:32:57 bagder Exp $
 ***************************************************************************/

/*
 * CAUTION: this header is designed to work when included by the app-side
 * as well as the library. Do not mix with library internals!
 */

#include "setup.h"
#include <stddef.h>
#include <curl/curl.h> /* for the curl_off_t type */

/* Determine what type of file offset conversion handling we wish to use.  For
 * systems with a 32-bit curl_off_t type, we should use strtol.  For systems
 * with a 64-bit curl_off_t type, we should use strtoll if it exists, and if
 * not, should try to emulate its functionality.  At any rate, we define
 * 'strtoofft' such that it can be used to work with curl_off_t's regardless.
 */
#if SIZEOF_CURL_OFF_T > 4
#if HAVE_STRTOLL
#define strtoofft strtoll
#else /* HAVE_STRTOLL */

/* For MSVC7 we can use _strtoi64() which seems to be a strtoll() clone */
#if defined(_MSC_VER) && (_MSC_VER >= 1300)
#define strtoofft _strtoi64
#else /* MSVC7 or later */
curl_off_t curlx_strtoll(const char *nptr, char **endptr, int base);
#define strtoofft curlx_strtoll
#define NEED_CURL_STRTOLL
#endif /* MSVC7 or later */

#endif /* HAVE_STRTOLL */
#else /* SIZEOF_CURL_OFF_T > 4 */
/* simply use strtol() to get 32bit numbers */
#define strtoofft strtol
#endif

#endif

