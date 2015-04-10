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
 * $Id: homedir.c,v 1.4 2004/01/29 13:54:08 bagder Exp $
 ***************************************************************************/

#include "setup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef VMS
#include <unixlib.h>
#endif

#include "homedir.h"

#ifdef CURLDEBUG
#include "../lib/memdebug.h"
#endif

static
char *GetEnv(const char *variable, char do_expand)
{
  char *env = NULL;
#ifdef WIN32
  char  buf1[1024], buf2[1024];
  DWORD rc;

  /* Don't use getenv(); it doesn't find variable added after program was
   * started. Don't accept truncated results (i.e. rc >= sizeof(buf1)).  */
  
  rc = GetEnvironmentVariable(variable, buf1, sizeof(buf1));
  if (rc > 0 && rc < sizeof(buf1)) {
    env = buf1;
    variable = buf1;
  }
  if (do_expand && strchr(variable,'%')) {
    /* buf2 == variable if not expanded */
    rc = ExpandEnvironmentStrings (variable, buf2, sizeof(buf2));
    if (rc > 0 && rc < sizeof(buf2) &&
        !strchr(buf2,'%'))    /* no vars still unexpanded */
      env = buf2;
  }
#else
  (void)do_expand;
#ifdef	VMS
  env = getenv(variable);
  if (env && strcmp("HOME",variable) == 0) {
	env = decc$translate_vms(env);
  }
#else
  /* no length control */
  env = getenv(variable);
#endif
#endif
  return (env && env[0])?strdup(env):NULL;
}

/* return the home directory of the current user as an allocated string */
char *homedir(void)
{
  char *home = GetEnv("HOME", FALSE);
  if(home)
    return home;
  
#if defined(HAVE_GETPWUID) && defined(HAVE_GETEUID)
 {
   struct passwd *pw = getpwuid(geteuid());
  
   if (pw) {
#ifdef VMS
     home = decc$translate_vms(pw->pw_dir);
#else
     home = pw->pw_dir;
#endif
     if (home && home[0])
       home = strdup(home);
   }
 }
#endif /* PWD-stuff */
#ifdef WIN32
  home = GetEnv("APPDATA", TRUE);
  if(!home)
    home = GetEnv("%USERPROFILE%\\Application Data", TRUE); /* Normally only
                                                               on Win-2K/XP */
#endif /* WIN32 */
  return home;
}
