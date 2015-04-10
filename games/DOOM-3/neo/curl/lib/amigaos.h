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
 * $Id: amigaos.h,v 1.3 2004/01/13 08:35:57 bagder Exp $
 ***************************************************************************/

#ifndef LIBCURL_AMIGAOS_H
#define LIBCURL_AMIGAOS_H

#ifndef __ixemul__

#include <exec/types.h>
#include <exec/execbase.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <bsdsocket.h>

#include "config-amigaos.h"

#define select(args...)	WaitSelect( args, NULL)
#define inet_ntoa(x)	Inet_NtoA( x ## .s_addr)
#define ioctl(a,b,c,d)	IoctlSocket( (LONG)a, (ULONG)b, (char*)c)
#define _AMIGASF	1

extern void amiga_cleanup();
extern BOOL amiga_init();

#else /* __ixemul__ */

#warning compiling with ixemul...

#endif /* __ixemul__ */
#endif /* LIBCURL_AMIGAOS_H */
