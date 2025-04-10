/* Copyright (c) 1996, 1997, 1998, 1999, 2000, 2001 Thorsten Kukuk
   Author: Thorsten Kukuk <kukuk@suse.de>

   The YP Server is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

   The YP Server is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with the YP Server; see the file COPYING. If
   not, write to the Free Software Foundation, Inc., 675 Mass Ave,
   Cambridge, MA 02139, USA. */

#define _GNU_SOURCE

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(HAVE_GETOPT_H) && defined(HAVE_GETOPT_LONG)
#include <getopt.h>
#endif

#include "hash.h"

#define PARSE_FOR_USERS 0
#define PARSE_FOR_HOSTS 1

extern void rev_setnetgrent (const char *);
extern void rev_endnetgrent (void);
extern int rev_getnetgrent (char **, char **, char **);

static int insert_netgroup (hash_t ** liste, const char *key, const char *val);
static void usage (int exit_code);

hash_t *input = NULL;
static hash_t **output = NULL;
static hash_t **empty = NULL;

int
main (int argc, char **argv)
{
#define BUFSIZE 8192
  char *host, *user, *domain, *key;
  char buffer[BUFSIZE + 1];
  int hosts;
  hash_t *work
   ;
#ifdef DEBUG_NETGROUP
  FILE *debug_file = NULL;
#endif

  /* Only -u or -h are allowed, --version will exit the program */
  /* --users = -u, --hosts = -h */
  hosts = -1;

  if (argc < 2)
    usage (1);

  while (1)
    {
      int c;
      int option_index = 0;
      static struct option long_options[] =
      {
	{"version", no_argument, NULL, '\255'},
	{"hosts", no_argument, NULL, 'h'},
	{"users", no_argument, NULL, 'u'},
	{"help", no_argument, NULL, '\254'},
	{"usage", no_argument, NULL, '\254'},
	{NULL, 0, NULL, '\0'}
      };

      c = getopt_long (argc, argv, "uh", long_options, &option_index);
      if (c == EOF)
	break;
      switch (c)
	{
	case 'u':
	  hosts = PARSE_FOR_USERS;
	  break;
	case 'h':
	  hosts = PARSE_FOR_HOSTS;
	  break;
	case '\255':
          printf ("revnetgroup (%s) %s", PACKAGE, VERSION);
	  exit (0);
	case '\254':
	  usage (0);
	  break;
	default:
	  usage (1);
	  break;
	}
    }

#ifdef DEBUG_NETGROUP
  debug_file = fopen ("/etc/netgroup", "rt");
  if (debug_file == NULL)
    {
      perror ("Error: could not open /etc/netgroup:");
      exit (0);
    }
#endif

  /* Put the netgroup names in a list: */

#ifdef DEBUG_NETGROUP
  while (fgets (buffer, BUFSIZE, debug_file))
#else
  while (fgets (buffer, BUFSIZE, stdin))
#endif
    {
      char *val;
      char *cptr;

      if (buffer[0] == '#')
	continue;

      if (strlen (buffer) == 0)
	continue;

      /* Replace first '\n' with '\0' */
      if ((cptr = strchr (buffer, '\n')) != NULL)
	*cptr = '\0';

      while (buffer[strlen (buffer) - 1] == '\\')
	{
#ifdef DEBUG_NETGROUP
	  fgets (&buffer[strlen (buffer) - 1],
		 BUFSIZE - strlen (buffer), debug_file);
#else
	  fgets (&buffer[strlen (buffer) - 1],
		 BUFSIZE - strlen (buffer), stdin);
#endif
	  if ((cptr = strchr (buffer, '\n')) != NULL)
	    *cptr = '\0';
	}

      val = (char *) (strpbrk (buffer, " \t"));
      if (val == NULL)
	continue;
      key = (char *) &buffer;
      *val = '\0';
      val++;
      insert_netgroup (&input, key, val);
#ifdef DEBUG_NETGROUP
      fprintf (stderr, "KEY: [%s]\n", key);
#endif
    }

#ifdef DEBUG_NETGROUP
  fclose (debug_file);
#endif


#ifdef DEBUG_NETGROUP
  fprintf (stderr, "About to enter while loop...\n");
#endif

  /*
   * Find all members of each netgroup and keep track of which
   * group they belong to.
   */
  empty = hash_malloc ();
  output = hash_malloc ();
  work = input;
  while (work != NULL)
    {
      rev_setnetgrent (work->key);

#ifdef DEBUG_NETGROUP
      fprintf (stderr, "Processing: [%s]\n", work->key);
#endif

      while (rev_getnetgrent (&host, &user, &domain) != 0)
	{
	  static char star[] = "*";
	  char *key = NULL;
	  char *dat = NULL;

	  /* what are we processing for? */
	  if (hosts == PARSE_FOR_HOSTS)
	    dat = host;
	  else
	    dat = user;

	  /* empty fields are wildcard fields = use '-'
	   * to prevent entries
	   */
	  if (dat == NULL)
	    dat = star;

	  /* if we have an entry with data... */
	  if (dat[0] != '-')
	    {
	      /* create the dat/domain key */
	      if (domain == NULL)
		{
		  key = malloc (strlen (dat) + 3);
		  sprintf (key, "%s.*", dat);
		}
	      else
		{
		  key = malloc (strlen (dat) + strlen (domain) + 2);
		  sprintf (key, "%s.%s", dat, domain);
		}

	      /* if we have a wildcard search */
	      if (*dat == '*')
		{
		  char *val = hash_search (empty, key);

		  if (val != NULL)
		    {
		      char *buf = NULL;
		      char *buf2 = NULL;
		      int found = 0;

		      buf2 = malloc (strlen (val) + 2);
		      sprintf (buf2, "%s,", val);
		      buf = strtok (buf2, ",");

		      while ((buf != NULL) && (found == 0))
			{
			  found = (strcmp (buf, work->key) == 0);
			  buf = strtok (NULL, ",");
			}

		      free (buf2);

		      if (!found)
			{
			  buf = malloc (strlen (work->key) + strlen (val) + 2);
			  sprintf (buf, "%s,%s", val, work->key);
			  hash_delkey (empty, key);
			  hash_insert (empty, key, buf);
			  free (buf);
			}
		    }
		  else
		    {
		      hash_insert (empty, key, work->key);
		    }
		}
	      else
		{
		  /* non-wild card search */
		  char *val = hash_search (output, key);

		  if (val != NULL)
		    {
		      char *buf = NULL;
		      char *buf2 = NULL;
		      int found = 0;

		      buf2 = malloc (strlen (val) + 2);
		      sprintf (buf2, "%s,", val);
		      buf = strtok (buf2, ",");

		      while ((buf != NULL) && (found == 0))
			{
			  found = (strcmp (buf, work->key) == 0);
			  buf = strtok (NULL, ",");
			}

		      free (buf2);

		      if (!found)
			{
			  buf = malloc (strlen (work->key) + strlen (val) + 2);
			  sprintf (buf, "%s,%s", val, work->key);
			  hash_delkey (output, key);

			  hash_insert (output, key, buf);
			  free (buf);
			}
		    }
		  else
		    {
		      hash_insert (output, key, work->key);
		    }
		}

	      free (key);
	    }
	}
      work = work->next;
      /* Release resources used by the getnetgrent code. */
      rev_endnetgrent ();
    }

#ifdef DEBUG_NETGROUP
  fprintf (stderr, "About to print results...\n");
#endif

  /* Print the results. */
  work = hash_first (output);
  while (work != NULL)
    {
      printf ("%s\t%s\n", work->key, work->val);
      work = hash_next (output, work->key);
    }

  work = hash_first (empty);
  while (work != NULL)
    {
      printf ("%s\t%s\n", work->key, work->val);
      work = hash_next (empty, work->key);
    }

#if 0
  remove_netgroup (&input);
  remove_netgroup (&output);
  remove_netgroup (&empty);
#endif

  return 0;
}

static void
usage (int exit_code)
{
  fprintf (stderr, "usage: revnetgroup -u|-h\n");
  fprintf (stderr, "       revnetgroup --version\n");
  exit (exit_code);
}

int
insert_netgroup (hash_t ** liste, const char *key, const char *val)
{
  hash_t *work;

  work = malloc (sizeof (hash_t));
  work->next = *liste;
  work->key = malloc (strlen (key) + 1);
  work->val = malloc (strlen (val) + 1);
  strcpy (work->key, key);
  strcpy (work->val, val);
  *liste = work;

  return 0;
}
