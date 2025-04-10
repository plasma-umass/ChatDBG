/* Copyright (C) 1995 Free Software Foundation, Inc.
 * This file is part of the GNU C Library.
 *
 * The GNU C Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The GNU C Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the GNU C Library; see the file COPYING.LIB.  If
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA.
 *
 * Author:  Swen Thuemmler <swen@uni-paderborn.de>
 *
 * Changes for the use with revnetgroup:
 *      Thorsten Kukuk <kukuk@suse.de>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "hash.h"

extern hash_t *input;

void rev_setnetgrent (const char *);
void rev_endnetgrent (void);
int rev_getnetgrent (char **, char **, char **);

struct netgrentry
  {
    char *host;
    char *user;
    char *domain;
  };


struct netgrlist
  {
    int maxmembers;
    int members;
    struct netgrentry *list;
  };


static void rev_expand_netgroupentry (const char *, struct netgrlist *);
static void rev_parse_entry (char *, char *, struct netgrlist *);
static void rev_netgr_free (struct netgrlist *);
static struct netgrlist list = {0, 0, NULL};
static int first = 1;
static char *netgroup = NULL;

static char *
search_netgroup (hash_t ** liste, const char *key)
{
  hash_t *work;

  work = *liste;

  while ((work != NULL) && (0 != strcmp (work->key, key)))
    work = work->next;

  if (work != NULL)
    return work->val;
  else
    return NULL;
}

void
rev_setnetgrent (const char *netgr)
{
  if (NULL == netgroup || 0 != strcmp (netgroup, netgr))
    {
      rev_endnetgrent ();
      netgroup = strdup (netgr);
      rev_expand_netgroupentry (netgr, &list);
    }
  first = 1;
}

void
rev_endnetgrent (void)
{
  if (NULL != netgroup)
    {
      free (netgroup);
      netgroup = NULL;
    }

  if (NULL != list.list)
    rev_netgr_free (&list);
  first = 1;
}

int
rev_getnetgrent (char **machinep, char **userp, char **domainp)
{
  static int current = 0;
  struct netgrentry *entry;

  if (1 == first)
    current = first = 0;
  else
    current++;

  if (current < list.members)
    {
      entry = &list.list[current];
      *machinep = entry->host;
      *userp = entry->user;
      *domainp = entry->domain;
      return 1;
    }
  return 0;
}

static void
rev_netgr_free (struct netgrlist *list)
{
  int i;
  for (i = 0; i < list->members; i++)
    {
      free (list->list[i].host);
      free (list->list[i].user);
      free (list->list[i].domain);
    }
  free (list->list);
  list->maxmembers = 0;
  list->members = 0;
  list->list = NULL;
}

static void
rev_expand_netgroupentry (const char *netgr, struct netgrlist *list)
{
  char *outval = NULL;
  char *outptr = NULL;
  char *start = NULL;
  char *end = NULL;
  char *realend = NULL;

  if (*netgr == '\0')
    return;

  outptr = search_netgroup (&input, netgr);
  if (outptr == NULL)
    return;

  /* make a copy to work with */
  outval = strdup (outptr);
  if (outval == NULL)
    {
      fprintf (stderr, "ERROR: could not allocate enough memory! [%s|%d]\n",
	       __FILE__, __LINE__);
      exit (1);
    }

  /* outval enthaelt den Eintrag. Zuerst Leerzeichen ueberlesen */
  start = outval;
  realend = start + strlen (outval);
  while (isspace (*start) && start < realend)
    start++;

  while (start < realend)
    {
      if ('(' == *start)	/* Eintrag gefunden */
	{
	  /* this a tuple... */
	  end = strchr (start, ')');
	  if (NULL == end)
	    {
	      free (outval);
	      return;
	    }

	  /* add the entry to the list? */
	  rev_parse_entry (start + 1, end, list);
	}
      else
	{
	  /* okay, this should be a group (ie. not
	     a tuple... */
	  end = start + 1;

	  while ((*end != '\0') && (!isspace (*end)))
	    end++;

	  *end = '\0';

	  /* recursion */
	  rev_expand_netgroupentry (start, list);
	}

      /* skip to the next entry */
      start = end + 1;

      if (end == realend)
	break;

      assert (start <= realend);

      while ((start < realend) && (isspace (*start)))
	start++;
    }

  /* free the copy */
  free (outval);
}

static void
rev_parse_entry (char *start, char *end, struct netgrlist *list)
{
  char *host, *user, *domain;
  struct netgrentry *entry;
  /* First split entry into fields. Return, when finding malformed entry */
  host = start;
  start = strchr (host, ',');
  if (NULL == start || start >= end)
    return;
  *start = '\0';
  user = start + 1;
  start = strchr (user, ',');
  if (NULL == start || start >= end)
    return;
  *start = '\0';
  domain = start + 1;
  if (start > end)
    return;
  *end = '\0';
  /* Entry is correctly formed, put it into the list */
  if (0 == list->maxmembers)
    {
      list->list = malloc (10 * sizeof (struct netgrentry));
      if (NULL != list->list)
	list->maxmembers = 10;
    }

  if (list->members == list->maxmembers)
    {
      list->list = realloc (list->list,
		      (list->maxmembers + 10) * sizeof (struct netgrentry));
      if (NULL == list->list)
	{
	  list->maxmembers = 0;
	  list->members = 0;
	  return;
	}
      list->maxmembers += 10;
    }
  /*
   * FIXME: this will not handle entries of the form ( asdf, sdfa , asdf )
   * (note the spaces). This should be handled better!
   */
  entry = &list->list[list->members];
  entry->user = ('\0' == *user) ? NULL : strdup (user);
  entry->host = ('\0' == *host) ? NULL : strdup (host);
  entry->domain = ('\0' == *domain) ? NULL : strdup (domain);
  list->members++;
  return;
}
