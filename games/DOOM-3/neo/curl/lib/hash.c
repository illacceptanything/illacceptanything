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
 * $Id: hash.c,v 1.20 2004/01/07 09:19:35 bagder Exp $
 ***************************************************************************/

#include "setup.h"

#include <string.h>
#include <stdlib.h>

#include "hash.h"
#include "llist.h"

#ifdef CURLDEBUG
/* this must be the last include file */
#include "memdebug.h"
#endif


static unsigned long
hash_str(const char *key, size_t key_length)
{
  char *end = (char *) key + key_length;
  unsigned long h = 5381;

  while (key < end) {
    h += h << 5;
    h ^= (unsigned long) *key++;
  }

  return h;
}

static void 
hash_element_dtor(void *user, void *element)
{
  curl_hash         *h = (curl_hash *) user;
  curl_hash_element *e = (curl_hash_element *) element;

  if (e->key) {
    free(e->key);
  }

  h->dtor(e->ptr);

  free(e);
}

/* return 1 on error, 0 is fine */
int
Curl_hash_init(curl_hash *h, int slots, curl_hash_dtor dtor)
{
  int i;

  h->dtor = dtor;
  h->size = 0;
  h->slots = slots;  

  h->table = (curl_llist **) malloc(slots * sizeof(curl_llist *));
  if(h->table) {
    for (i = 0; i < slots; ++i) {
      h->table[i] = Curl_llist_alloc((curl_llist_dtor) hash_element_dtor);
      if(!h->table[i]) {
        while(i--)
          Curl_llist_destroy(h->table[i], NULL);
        free(h->table);
        return 1; /* failure */
      }
    }
    return 0; /* fine */
  }
  else
    return 1; /* failure */
}

curl_hash *
Curl_hash_alloc(int slots, curl_hash_dtor dtor)
{
  curl_hash *h;

  h = (curl_hash *) malloc(sizeof(curl_hash));
  if (h) {
    if(Curl_hash_init(h, slots, dtor)) {
      /* failure */
      free(h);
      h = NULL;
    }
  }

  return h;
}

static int 
hash_key_compare(char *key1, size_t key1_len, char *key2, size_t key2_len)
{
  if (key1_len == key2_len && 
      *key1 == *key2 &&
      memcmp(key1, key2, key1_len) == 0) {
    return 1;
  }

  return 0;
}

static curl_hash_element *
mk_hash_element(char *key, size_t key_len, const void *p)
{
  curl_hash_element *he =
    (curl_hash_element *) malloc(sizeof(curl_hash_element));

  if(he) {
    he->key = strdup(key);
    he->key_len = key_len;
    he->ptr = (void *) p;
  }
  return he;
}

#define find_slot(__h, __k, __k_len) (hash_str(__k, __k_len) % (__h)->slots)

#define FETCH_LIST(x,y,z) x->table[find_slot(x, y, z)]

/* Return the data in the hash. If there already was a match in the hash,
   that data is returned. */
void *
Curl_hash_add(curl_hash *h, char *key, size_t key_len, void *p)
{
  curl_hash_element  *he;
  curl_llist_element *le;
  curl_llist *l = FETCH_LIST(h, key, key_len);

  for (le = l->head; le; le = le->next) {
    he = (curl_hash_element *) le->ptr;
    if (hash_key_compare(he->key, he->key_len, key, key_len)) {
      h->dtor(p);     /* remove the NEW entry */
      return he->ptr; /* return the EXISTING entry */
    }
  }

  he = mk_hash_element(key, key_len, p);
  if (he) {
    if(Curl_llist_insert_next(l, l->tail, he)) {
      ++h->size;
      return p; /* return the new entry */
    }
    /* couldn't insert it, destroy the 'he' element again */
    hash_element_dtor(h, he);
  }
  h->dtor(p); /* remove the NEW entry */
  return NULL; /* failure */
}

#if 0
int 
Curl_hash_delete(curl_hash *h, char *key, size_t key_len)
{
  curl_hash_element  *he;
  curl_llist_element *le;
  curl_llist *l = FETCH_LIST(h, key, key_len);

  for (le = l->head;
       le;
       le = le->next) {
    he = le->ptr;
    if (hash_key_compare(he->key, he->key_len, key, key_len)) {
      Curl_llist_remove(l, le, (void *) h);
      --h->size;
      return 1;
    }
  }

  return 0;
}
#endif

void *
Curl_hash_pick(curl_hash *h, char *key, size_t key_len)
{
  curl_llist_element *le;
  curl_hash_element  *he;
  curl_llist *l = FETCH_LIST(h, key, key_len);

  for (le = l->head;
       le;
       le = le->next) {
    he = le->ptr;
    if (hash_key_compare(he->key, he->key_len, key, key_len)) {
      return he->ptr;
    }
  }

  return NULL;
}

#if defined(CURLDEBUG) && defined(AGGRESIVE_TEST)
void 
Curl_hash_apply(curl_hash *h, void *user,
                void (*cb)(void *user, void *ptr))
{
  curl_llist_element  *le;
  int                  i;

  for (i = 0; i < h->slots; ++i) {
    for (le = (h->table[i])->head;
         le;
         le = le->next) {
      curl_hash_element *el = le->ptr;
      cb(user, el->ptr);
    }
  }
}
#endif

void
Curl_hash_clean(curl_hash *h)
{
  int i;

  for (i = 0; i < h->slots; ++i) {
    Curl_llist_destroy(h->table[i], (void *) h);
  }

  free(h->table);
}

void
Curl_hash_clean_with_criterium(curl_hash *h, void *user,
                               int (*comp)(void *, void *))
{
  curl_llist_element *le;
  curl_llist_element *lnext;
  curl_llist *list;
  int i;

  for (i = 0; i < h->slots; ++i) {
    list = h->table[i];
    le = list->head; /* get first list entry */
    while(le) {
      curl_hash_element *he = le->ptr;
      lnext = le->next;
      /* ask the callback function if we shall remove this entry or not */
      if (comp(user, he->ptr)) {
        Curl_llist_remove(list, le, (void *) h);
        --h->size; /* one less entry in the hash now */
      }
      le = lnext;
    }
  }
}

#if 0
int
Curl_hash_count(curl_hash *h)
{
  return h->size;
}
#endif

void 
Curl_hash_destroy(curl_hash *h)
{
  if (!h)
    return;

  Curl_hash_clean(h);
  free(h);
}

