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
 * $Id: urlglob.c,v 1.31 2004/03/08 12:51:13 bagder Exp $
 ***************************************************************************/

/* client-local setup.h */
#include "setup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>

#define _MPRINTF_REPLACE /* we want curl-functions instead of native ones */
#include <curl/mprintf.h>

#include "urlglob.h"


#ifdef CURLDEBUG
#include "../lib/memdebug.h"
#endif

typedef enum {
  GLOB_OK,
  GLOB_ERROR
} GlobCode;

/*
 * glob_word()
 *
 * Input a full globbed string, set the forth argument to the amount of
 * strings we get out of this. Return GlobCode.
 */
static GlobCode glob_word(URLGlob *, /* object anchor */
                          char *,    /* globbed string */
                          int,       /* position */
                          int *);    /* returned number of strings */

static GlobCode glob_set(URLGlob *glob, char *pattern, int pos, int *amount)
{
  /* processes a set expression with the point behind the opening '{'
     ','-separated elements are collected until the next closing '}'
  */
  char* buf = glob->glob_buffer;
  URLPattern *pat;

  pat = (URLPattern*)&glob->pattern[glob->size / 2];
  /* patterns 0,1,2,... correspond to size=1,3,5,... */
  pat->type = UPTSet;
  pat->content.Set.size = 0;
  pat->content.Set.ptr_s = 0;
  pat->content.Set.elements = (char**)malloc(0);
  ++glob->size;

  while (1) {
    switch (*pattern) {
    case '\0':			/* URL ended while set was still open */
      snprintf(glob->errormsg, sizeof(glob->errormsg),
               "unmatched brace at pos %d\n", pos);
      return GLOB_ERROR;

    case '{':
    case '[':			/* no nested expressions at this time */
      snprintf(glob->errormsg, sizeof(glob->errormsg),
               "nested braces not supported at pos %d\n", pos);
      return GLOB_ERROR;

    case ',':
    case '}':				/* set element completed */
      *buf = '\0';
      pat->content.Set.elements =
        realloc(pat->content.Set.elements,
                (pat->content.Set.size + 1) * sizeof(char*));
      if (!pat->content.Set.elements) {
        snprintf(glob->errormsg, sizeof(glob->errormsg), "out of memory");
        return GLOB_ERROR;
      }
      pat->content.Set.elements[pat->content.Set.size] =
        strdup(glob->glob_buffer);
      ++pat->content.Set.size;

      if (*pattern == '}') {
        /* entire set pattern completed */
        int wordamount;

	/* always check for a literal (may be "") between patterns */
        if(GLOB_ERROR == glob_word(glob, ++pattern, ++pos, &wordamount))
          wordamount=1;
	*amount = pat->content.Set.size * wordamount;

        return GLOB_OK;
      }

      buf = glob->glob_buffer;
      ++pattern;
      ++pos;
      break;

    case ']':				/* illegal closing bracket */
      snprintf(glob->errormsg, sizeof(glob->errormsg), 
               "illegal pattern at pos %d\n", pos);
      return GLOB_ERROR;

    case '\\':				/* escaped character, skip '\' */
      if (*(buf+1) == '\0') {		/* but no escaping of '\0'! */
        snprintf(glob->errormsg, sizeof(glob->errormsg), 
                 "illegal pattern at pos %d\n", pos);
	return GLOB_ERROR;
      }
      ++pattern;
      ++pos;				/* intentional fallthrough */

    default:
      *buf++ = *pattern++;		/* copy character to set element */
      ++pos;
    }
  }
  /* we never reach this point */
}

static GlobCode glob_range(URLGlob *glob, char *pattern, int pos, int *amount)
{
  /* processes a range expression with the point behind the opening '['
     - char range: e.g. "a-z]", "B-Q]"
     - num range: e.g. "0-9]", "17-2000]"
     - num range with leading zeros: e.g. "001-999]"
     expression is checked for well-formedness and collected until the next ']'
  */
  URLPattern *pat;
  char *c;
  int wordamount=1;
  
  pat = (URLPattern*)&glob->pattern[glob->size / 2];
  /* patterns 0,1,2,... correspond to size=1,3,5,... */
  ++glob->size;

  if (isalpha((int)*pattern)) {		/* character range detected */
    pat->type = UPTCharRange;
    if (sscanf(pattern, "%c-%c]", &pat->content.CharRange.min_c,
               &pat->content.CharRange.max_c) != 2 ||
	pat->content.CharRange.min_c >= pat->content.CharRange.max_c ||
	pat->content.CharRange.max_c - pat->content.CharRange.min_c > 'z' - 'a') {
      /* the pattern is not well-formed */ 
      snprintf(glob->errormsg, sizeof(glob->errormsg),
               "illegal pattern or range specification after pos %d\n", pos);
      return GLOB_ERROR;
    }
    pat->content.CharRange.ptr_c = pat->content.CharRange.min_c;
    /* always check for a literal (may be "") between patterns */

    if(GLOB_ERROR == glob_word(glob, pattern + 4, pos + 4, &wordamount))
      wordamount=1;

    *amount = (pat->content.CharRange.max_c -
               pat->content.CharRange.min_c + 1) *
      wordamount;

    return GLOB_OK;
  }

  if (isdigit((int)*pattern)) { /* numeric range detected */

    pat->type = UPTNumRange;
    pat->content.NumRange.padlength = 0;
    if (sscanf(pattern, "%d-%d]",
               &pat->content.NumRange.min_n,
               &pat->content.NumRange.max_n) != 2 ||
	pat->content.NumRange.min_n >= pat->content.NumRange.max_n) {
      /* the pattern is not well-formed */ 
      snprintf(glob->errormsg, sizeof(glob->errormsg), 
               "error: illegal pattern or range specification after pos %d\n",
               pos);
      return GLOB_ERROR;
    }
    if (*pattern == '0') {		/* leading zero specified */
      c = pattern;  
      while (isdigit((int)*c++))
	++pat->content.NumRange.padlength; /* padding length is set for all
                                              instances of this pattern */
    }
    pat->content.NumRange.ptr_n = pat->content.NumRange.min_n;
    c = (char*)strchr(pattern, ']'); /* continue after next ']' */
    if(c)
      c++;
    else {
      snprintf(glob->errormsg, sizeof(glob->errormsg), "missing ']'");
      return GLOB_ERROR; /* missing ']' */
    }

    /* always check for a literal (may be "") between patterns */

    if(GLOB_ERROR == glob_word(glob, c, pos + (c - pattern), &wordamount))
      wordamount = 1;
    
    *amount = (pat->content.NumRange.max_n - 
               pat->content.NumRange.min_n + 1) *
      wordamount;

    return GLOB_OK;
  }
  snprintf(glob->errormsg, sizeof(glob->errormsg), 
           "illegal character in range specification at pos %d\n", pos);
  return GLOB_ERROR;
}

static GlobCode glob_word(URLGlob *glob, char *pattern, int pos, int *amount)
{
  /* processes a literal string component of a URL
     special characters '{' and '[' branch to set/range processing functions
   */ 
  char* buf = glob->glob_buffer;
  int litindex;
  GlobCode res = GLOB_OK;

  *amount = 1; /* default is one single string */

  while (*pattern != '\0' && *pattern != '{' && *pattern != '[') {
    if (*pattern == '}' || *pattern == ']')
      return GLOB_ERROR;

    /* only allow \ to escape known "special letters" */
    if (*pattern == '\\' &&
        (*(pattern+1) == '{' || *(pattern+1) == '[' ||
         *(pattern+1) == '}' || *(pattern+1) == ']') ) {

      /* escape character, skip '\' */
      ++pattern;
      ++pos;
      if (*pattern == '\0')		/* but no escaping of '\0'! */
	return GLOB_ERROR;
    }
    *buf++ = *pattern++;		/* copy character to literal */
    ++pos;
  }
  *buf = '\0';
  litindex = glob->size / 2;
  /* literals 0,1,2,... correspond to size=0,2,4,... */
  glob->literal[litindex] = strdup(glob->glob_buffer);
  if(!glob->literal[litindex])
    return GLOB_ERROR;
  ++glob->size;

  switch (*pattern) {
  case '\0':
    break;			/* singular URL processed  */

  case '{':
    /* process set pattern */
    res = glob_set(glob, ++pattern, ++pos, amount);
    break;

  case '[':
    /* process range pattern */
    res= glob_range(glob, ++pattern, ++pos, amount);
    break;
  }

  if(GLOB_OK != res)
    /* free that strdup'ed string again */
    free(glob->literal[litindex]);

  return res; /* something got wrong */
}

int glob_url(URLGlob** glob, char* url, int *urlnum, FILE *error)
{
  /*
   * We can deal with any-size, just make a buffer with the same length
   * as the specified URL!
   */
  URLGlob *glob_expand;
  int amount;
  char *glob_buffer=(char *)malloc(strlen(url)+1);

  *glob = NULL;
  if(NULL == glob_buffer)
    return CURLE_OUT_OF_MEMORY;

  glob_expand = (URLGlob*)malloc(sizeof(URLGlob));
  if(NULL == glob_expand) {
    free(glob_buffer);
    return CURLE_OUT_OF_MEMORY;
  }
  glob_expand->size = 0;
  glob_expand->urllen = strlen(url);
  glob_expand->glob_buffer = glob_buffer;
  glob_expand->beenhere=0;
  if(GLOB_OK == glob_word(glob_expand, url, 1, &amount))
    *urlnum = amount;
  else {
    if(error && glob_expand->errormsg[0]) {
      /* send error description to the error-stream */
      fprintf(error, "curl: (%d) [globbing] %s\n",
              CURLE_URL_MALFORMAT, glob_expand->errormsg);
    }
    /* it failed, we cleanup */
    free(glob_buffer);
    free(glob_expand);
    glob_expand = NULL;
    *urlnum = 1;
    return CURLE_URL_MALFORMAT;
  }

  *glob = glob_expand;
  return CURLE_OK;
}

void glob_cleanup(URLGlob* glob)
{
  int i, elem;

  for (i = glob->size - 1; i >= 0; --i) {
    if (!(i & 1)) {	/* even indexes contain literals */
      free(glob->literal[i/2]);
    }
    else {		/* odd indexes contain sets or ranges */
      if (glob->pattern[i/2].type == UPTSet) {
	for (elem = glob->pattern[i/2].content.Set.size - 1;
             elem >= 0;
             --elem) {
	  free(glob->pattern[i/2].content.Set.elements[elem]);
	}
	free(glob->pattern[i/2].content.Set.elements);
      }
    }
  }
  free(glob->glob_buffer);
  free(glob);
}

char *glob_next_url(URLGlob *glob)
{
  char *buf = glob->glob_buffer;
  URLPattern *pat;
  char *lit;
  signed int i;
  int carry;

  if (!glob->beenhere)
    glob->beenhere = 1;
  else {
    carry = 1;

    /* implement a counter over the index ranges of all patterns,
       starting with the rightmost pattern */
    for (i = glob->size / 2 - 1; carry && i >= 0; --i) {
      carry = 0;
      pat = &glob->pattern[i];
      switch (pat->type) {
      case UPTSet:
	if (++pat->content.Set.ptr_s == pat->content.Set.size) {
	  pat->content.Set.ptr_s = 0;
	  carry = 1;
	}
	break;
      case UPTCharRange:
	if (++pat->content.CharRange.ptr_c > pat->content.CharRange.max_c) {
	  pat->content.CharRange.ptr_c = pat->content.CharRange.min_c;
	  carry = 1;
	}
	break;
      case UPTNumRange:
	if (++pat->content.NumRange.ptr_n > pat->content.NumRange.max_n) {
	  pat->content.NumRange.ptr_n = pat->content.NumRange.min_n;
	  carry = 1;
	}
	break;
      default:
	printf("internal error: invalid pattern type (%d)\n", pat->type);
	exit (CURLE_FAILED_INIT);
      }
    }
    if (carry)		/* first pattern ptr has run into overflow, done! */
      return NULL;
  }

  for (i = 0; i < glob->size; ++i) {
    if (!(i % 2)) {            /* every other term (i even) is a literal */
      lit = glob->literal[i/2];
      strcpy(buf, lit);
      buf += strlen(lit);
    }
    else {				/* the rest (i odd) are patterns */
      pat = &glob->pattern[i/2];
      switch(pat->type) {
      case UPTSet:
	strcpy(buf, pat->content.Set.elements[pat->content.Set.ptr_s]);
	buf += strlen(pat->content.Set.elements[pat->content.Set.ptr_s]);
	break;
      case UPTCharRange:
	*buf++ = pat->content.CharRange.ptr_c;
	break;
      case UPTNumRange:
	sprintf(buf, "%0*d",
                pat->content.NumRange.padlength, pat->content.NumRange.ptr_n); 
        buf += strlen(buf); /* make no sprint() return code assumptions */
	break;
      default:
	printf("internal error: invalid pattern type (%d)\n", pat->type);
	exit (CURLE_FAILED_INIT);
      }
    }
  }
  *buf = '\0';
  return strdup(glob->glob_buffer);
}

char *glob_match_url(char *filename, URLGlob *glob)
{
  char *target;
  int allocsize;
  int stringlen=0;
  char numbuf[18];
  char *appendthis = NULL;
  int appendlen = 0;

  /* We cannot use the glob_buffer for storage here since the filename may
   * be longer than the URL we use. We allocate a good start size, then
   * we need to realloc in case of need.
   */
  allocsize=strlen(filename);
  target = malloc(allocsize);
  if(NULL == target)
    return NULL; /* major failure */

  while (*filename) {
    if (*filename == '#' && isdigit((int)filename[1])) {
      /* only '#1' ... '#9' allowed */
      int i;
      unsigned long num = strtoul(&filename[1], &filename, 10);

      i = num-1;

      if (num && (i <= glob->size / 2)) {
        URLPattern pat = glob->pattern[i];
        switch (pat.type) {
        case UPTSet:
          appendthis = pat.content.Set.elements[pat.content.Set.ptr_s];
          appendlen =
            (int)strlen(pat.content.Set.elements[pat.content.Set.ptr_s]);
          break;
        case UPTCharRange:
          numbuf[0]=pat.content.CharRange.ptr_c;
          numbuf[1]=0;
          appendthis=numbuf;
          appendlen=1;
          break;
        case UPTNumRange:
          sprintf(numbuf, "%0*d",
                  pat.content.NumRange.padlength,
                  pat.content.NumRange.ptr_n);
          appendthis = numbuf;
          appendlen = (int)strlen(numbuf);
          break;
        default:
          printf("internal error: invalid pattern type (%d)\n",
                 (int)pat.type);
          free(target);
          return NULL;
        }
      }
    }
    else {
      appendthis=filename++;
      appendlen=1;
    }
    if(appendlen + stringlen >= allocsize) {
      char *newstr;
      allocsize = (appendlen + stringlen)*2;
      newstr=realloc(target, allocsize);
      if(NULL ==newstr) {
        free(target);
        return NULL;
      }
      target=newstr;
    }
    memcpy(&target[stringlen], appendthis, appendlen);
    stringlen += appendlen;
  }
  target[stringlen]= '\0';
  return target;
}
