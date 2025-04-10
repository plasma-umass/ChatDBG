/*
** hash.c - functions for a hash table.
**
** Copyright (c) 1996, 1997, 1999 Thorsten Kukuk
**
** This file is part of the NYS YP Server.
**
** The NYS YP Server is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License as
** published by the Free Software Foundation; either version 2 of the
** License, or (at your option) any later version.
**
** The NYS YP Server is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public
** License along with the NYS YP Server; see the file COPYING.  If
** not, write to the Free Software Foundation, Inc., 675 Mass Ave,
** Cambridge, MA 02139, USA.
**
** Author: Thorsten Kukuk <kukuk@suse.de>
*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include <assert.h>

#define TABLESIZE 997		/*Should be a prime */

#ifndef HAVE_STRDUP
#include "compat/strdup.c"
#endif

/*
 * hash_malloc(void)
 *
 *   Initialize a new hash table.
 */
hash_t **
hash_malloc (void)
{
  hash_t **work = NULL;
  int i = 0;

  work = malloc (sizeof (hash_t *) * TABLESIZE);
  if (work == NULL)
    {
      fprintf (stderr, "Out of memory.\n");
      exit (1);
    }

  for (i = 0; i < TABLESIZE; i++)
    work[i] = NULL;

  return work;
}

/*
 * hash_calc_key(const char* key)
 *
 *   Calculates the key, returns it.
 */
static inline long
hash_calc_key (const char *key)
{
  long hkey = 0;
  int length = strlen (key);
  int i = -1;

  for (i = 0; i < length; i++)
    hkey = (256 * hkey + key[i]) % TABLESIZE;

  assert (hkey < TABLESIZE);
  return hkey;
}


/*
 * hash_insert(hash_t **table, const char*, const char*)
 *
 *   Complete re-write, to insert item into head of list
 *   at it's entry in the table.
 */
int
hash_insert (hash_t **table, const char *key, const char *val)
{
  long hkey = -1;
  hash_t *work = NULL;

  assert (table != NULL);
  assert (key != NULL);
  assert (val != NULL);

  hkey = hash_calc_key (key);

  /* look for the item */
  work = table[hkey];
  while (work != NULL)
    {
      if (strcmp (work->key, key) == 0)
	{
	  return -1;
	}
      work = work->next;
    }

  /* insert into head of list */
  work = malloc (sizeof (hash_t));
  if (work == NULL)
    {
      fprintf (stderr, "Out of Memory.\n");
      exit (1);
    }

  /* setup the new node */
  work->key = strdup (key);
  work->val = strdup (val);
  work->next = NULL;

  if (table[hkey] != NULL)
    {
      work->next = table[hkey];
    }

  table[hkey] = work;

  return 0;
}


/*
 * hash_free(hash_t**)
 *
 *   Deallocates all the structures.
 *
 */
int
hash_free (hash_t **table __attribute__ ((unused)))
{
  /* XXX Not implementet yet! */

  return 0;
}


/*
 * hash_search(hash_t**, const char*)
 *
 *   Looks for specified key, returns value if found,
 *   and NULL if not.
 *
 */
char *
hash_search (hash_t **table, const char *key)
{
  hash_t *work = NULL;
  long hkey = -1;

  assert (table != NULL);
  assert (key != NULL);

  hkey = hash_calc_key (key);

  /* look for the key in the list */
  work = table[hkey];
  while (work != NULL)
    {
      if (strcmp (work->key, key) == 0)
	return work->val;

      work = work->next;
    }

  return NULL;
}


/*
 * hash_delkey(hash_t**, const char* )
 *
 *   Delete the item from the table.
 */
int
hash_delkey (hash_t **table, const char *key)
{
  hash_t *work = NULL;
  hash_t *prev = NULL;
  long hkey = -1;

  assert (table != NULL);
  assert (key != NULL);

  hkey = hash_calc_key (key);

  work = table[hkey];
  prev = table[hkey];

  while (work != NULL)
    {
      if (strcmp (work->key, key) == 0)
	{

	  /* delete this node, and return? */
	  if (work == table[hkey])
	    table[hkey] = work->next;
	  else
	    prev->next = work->next;

	  free (work->key);
	  free (work->val);
	  free (work);
	  break;
	}

      prev = work;
      work = work->next;
    }

  return 0;
}


/*
 * hash_first(hash_t**)
 *
 *   Returns the first item in the hash table.
 */
hash_t *
hash_first (hash_t **table)
{
  unsigned long i = 0;

  for (i = 0; i < TABLESIZE; i++)
    {
      if (table[i] != NULL)
	return table[i];
    }

  return NULL;
}

/*
 * hash_next(hash_t**, const char*)
 *
 *   Returns the next item in the cache.
 */
hash_t *
hash_next (hash_t **table, const char *key)
{
  hash_t *work = NULL;
  long hkey = -1;

  assert (table != NULL);
  assert (key != NULL);

  hkey = hash_calc_key (key);

  /* look for the item */
  work = table[hkey];
  while (work != NULL)
    {
      if (strcmp (work->key, key) == 0)
	{
	  work = work->next;
	  break;
	}
      work = work->next;
    }

  /* at this point, we have seen the key:
   * starting from here, return the first
   * valid pointer we find
   */
  if (work != NULL)
    return work;

  /* work is NULL, increment to next list. */
  hkey++;
  while (hkey < TABLESIZE)
    {
      if (table[hkey] != NULL)
	return table[hkey];

      hkey++;
    }

  return NULL;
}
