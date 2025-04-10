/* Copyright (c) 1996,1997, 1998, 1999, 2000 Thorsten Kukuk
   Author: Thorsten Kukuk <kukuk@suse.de>

   The YP Server is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

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

#include <alloca.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <ctype.h>
#include <netdb.h>
#include <rpc/rpc.h>

#include "yp.h"

#if defined (__NetBSD__) || (defined(__GLIBC__) && (__GLIBC__ == 2 && __GLIBC_MINOR__ == 0))
/* <rpc/rpc.h> is missing the prototype */
int callrpc (char *host, u_long prognum, u_long versnum, u_long procnum,
             xdrproc_t inproc, char *in, xdrproc_t outproc, char *out);
#endif
#include <sys/param.h>
#include <sys/time.h>

#if defined(HAVE_LIBGDBM)

#include <gdbm.h>

#define ypdb_store gdbm_store
#define YPDB_REPLACE GDBM_REPLACE
#define ypdb_close gdbm_close
static GDBM_FILE dbm;

#elif defined (HAVE_NDBM)

#include <ndbm.h>

#define ypdb_store dbm_store
#define YPDB_REPLACE DBM_REPLACE
#define ypdb_close dbm_close
static DBM *dbm;

#else

#error "No database found or selected!"

#endif

static int lower = 0;

static inline void
write_data (datum key, datum data)
{
  if (ypdb_store (dbm, key, data, YPDB_REPLACE) != 0)
    {
      perror ("makedbm: dbm_store");
      ypdb_close (dbm);
      exit (1);
    }
}

#ifdef HAVE_NDBM
static char *
strapp (const char *str1, const char *str2)
{
  char *buffer = alloca(strlen (str1) + strlen (str2) + 1);

  strcpy (buffer, str1);
  strcat (buffer, str2);

  return strdup (buffer);
}
#endif

static void
create_file (char *fileName, char *dbmName, char *masterName,
	     char *domainName, char *inputName,
	     char *outputName, int aliases, int shortlines,
	     int b_flag, int s_flag, int remove_comments,
	     int check_limit)
{
  datum kdat, vdat;
  char *key = NULL;
  size_t keylen = 0;
  char *filename = NULL;
  FILE *input;
  char orderNum[12];
  struct timeval tv;
  struct timezone tz;

  input = strcmp (fileName, "-") ? fopen (fileName, "r") : stdin;
  if (input == NULL)
    {
      fprintf (stderr, "makedbm: Cannot open %s\n", fileName);
      exit (1);
    }

  filename = calloc (1, strlen (dbmName) + 3);
  sprintf (filename, "%s~", dbmName);
#if defined(HAVE_LIBGDBM)
  dbm = gdbm_open (filename, 0, GDBM_NEWDB | GDBM_FAST, 0600, NULL);
#elif defined(HAVE_NDBM)
  dbm = dbm_open (filename, O_CREAT | O_RDWR, 0600);
#endif
  if (dbm == NULL)
    {
      fprintf (stderr, "makedbm: Cannot open %s\n", filename);
      exit (1);
    }

  if (masterName && *masterName)
    {
      kdat.dptr = "YP_MASTER_NAME";
      kdat.dsize = strlen (kdat.dptr);
      vdat.dptr = masterName;
      vdat.dsize = strlen (vdat.dptr);
      write_data (kdat, vdat);
    }

  if (domainName && *domainName)
    {
      kdat.dptr = "YP_DOMAIN_NAME";
      kdat.dsize = strlen (kdat.dptr);
      vdat.dptr = domainName;
      vdat.dsize = strlen (vdat.dptr);
      write_data (kdat, vdat);
    }

  if (inputName && *inputName)
    {
      kdat.dptr = "YP_INPUT_NAME";
      kdat.dsize = strlen (kdat.dptr);
      vdat.dptr = inputName;
      vdat.dsize = strlen (vdat.dptr);
      write_data (kdat, vdat);
    }

  if (outputName && *outputName)
    {
      kdat.dptr = "YP_OUTPUT_NAME";
      kdat.dsize = strlen (kdat.dptr);
      vdat.dptr = outputName;
      vdat.dsize = strlen (vdat.dptr);
      write_data (kdat, vdat);
    }

  if (b_flag)
    {
      kdat.dptr = "YP_INTERDOMAIN";
      kdat.dsize = strlen (kdat.dptr);
      vdat.dptr = "";
      vdat.dsize = strlen (vdat.dptr);
      write_data (kdat, vdat);
    }

  if (s_flag)
    {
      kdat.dptr = "YP_SECURE";
      kdat.dsize = strlen (kdat.dptr);
      vdat.dptr = "";
      vdat.dsize = strlen (vdat.dptr);
      write_data (kdat, vdat);
    }

  if (aliases)
    {
      kdat.dptr = "@";
      kdat.dsize = strlen (kdat.dptr);
      vdat.dptr = "@";
      vdat.dsize = strlen (vdat.dptr);
      write_data (kdat, vdat);
    }

  gettimeofday (&tv, &tz);
  sprintf (orderNum, "%ld", (long) tv.tv_sec);
  kdat.dptr = "YP_LAST_MODIFIED";
  kdat.dsize = strlen (kdat.dptr);
  vdat.dptr = orderNum;
  vdat.dsize = strlen (vdat.dptr);
  write_data (kdat, vdat);

  while (!feof (input))
    {
      char *cptr;

#ifdef HAVE_GETLINE
      ssize_t n = getline (&key, &keylen, input);
#elif HAVE_GETDELIM
      ssize_t n = getdelim (&key, &keylen, '\n', input);
#else
      ssize_t n;

      if (key == NULL)
	{
	  keylen = 8096;
	  key = malloc (keylen);
	}
      key[0] = '\0';
      fgets (key, keylen - 1, input);
      if (key != NULL)
	n = strlen (key);
      else
	n = 0;
#endif
      if (n < 1)
	break;
      if (key[n - 1] == '\n' || key[n - 1] == '\r')
	key[n - 1] = '\0';
      if (n > 1 && (key[n - 2] == '\n' || key[n - 2] == '\r'))
	key[n - 2] = '\0';

      if (remove_comments)
	if ((cptr = strchr (key, '#')) != NULL)
	  {
	    *cptr = '\0';
	    --cptr;
	    while (*cptr == ' ' || *cptr == '\t')
	      {
		*cptr = '\0';
		--cptr;
	      }
	  }

      if (strlen (key) == 0)
	continue;

      if (aliases)
	{
	  int len;

	  len = strlen (key);
	  while (key[len - 1] == ' ' || key[len - 1] == '\t')
	    {
	      key[len - 1] = '\0';
	      --len;
	    }

	  while (key[len - 1] == ',')
	    {
	      char *nkey = NULL;
	      size_t nkeylen = 0;
#ifdef HAVE_GETLINE
	      getline (&nkey, &nkeylen, input);
#elif HAVE_GETDELIM
	      getdelim (&nkey, &nkeylen, '\n', input);
#else
	      nkeylen = 8096;
	      nkey = malloc (nkeylen);
	      nkey[0] = '\0';
	      fgets (nkey, nkeylen - 1, input);
#endif

	      cptr = nkey;
	      while ((*cptr == ' ') || (*cptr == '\t'))
		++cptr;
	      if (strlen (key) + strlen (cptr) < keylen)
		strcat (key, cptr);
	      else
		{
		  keylen += nkeylen;
		  key = realloc (key, keylen);
		  if (key == NULL)
		    abort ();
		  strcat (key, cptr);
		}

	      free (nkey);

	      if ((cptr = strchr (key, '\n')) != NULL)
		*cptr = '\0';
	      len = strlen (key);
	      while (key[len - 1] == ' ' || key[len - 1] == '\t')
		{
		  key[len - 1] = '\0';
		  len--;
		}
	    }
	  if ((cptr = strchr (key, ':')) != NULL)
	    *cptr = ' ';
	}
      else
	while (key[strlen (key) - 1] == '\\')
	  {
	    char *nkey;
	    size_t nkeylen = 0;
#ifdef HAVE_GETLINE
	    ssize_t n = getline (&nkey, &nkeylen, input);
#elif HAVE_GETDELIM
	    ssize_t n = getdelim (&nkey, &nkeylen, '\n', input);
#else
	    ssize_t n;

	    nkeylen = 8096;
	    nkey = malloc (nkeylen);
	    nkey[0] = '\0';
	    fgets (nkey, nkeylen - 1, input);
	    if (nkey != NULL)
	      n = strlen (nkey);
	    else
	      n = 0;
#endif
	    if (n < 1)
	      break;
	    if (nkey[n - 1] == '\n' || nkey[n - 1] == '\r')
	      nkey[n - 1] = '\0';
	    if (n > 1 && (nkey[n - 2] == '\n' || nkey[n - 2] == '\r'))
	      nkey[n - 2] = '\0';

	    key[strlen (key) - 1] = '\0';

	    if (shortlines)
	      {
		int len;

		len = strlen (key);
		key[len - 1] = '\0';
		len--;
		if ((key[len - 1] != ' ') && (key[len - 1] != '\t'))
		  strcat (key, " ");
		cptr = nkey;
		while ((*cptr == ' ') || (*cptr == '\t'))
		  ++cptr;
		if (len + 1 + strlen (cptr) < keylen)
		  strcat (key, cptr);
		else
		  {
		    keylen += nkeylen;
		    key = realloc (key, keylen);
		    if (key == NULL)
		      abort ();
		    strcat (key, nkey);
		  }
	      }
	    else
	      {
		keylen += nkeylen;
		key = realloc (key, keylen);
		if (key == NULL)
		  abort ();
		strcat (key, nkey);
	      }
	    free (nkey);

	    if ((cptr = strchr (key, '\n')) != NULL)
	      *cptr = '\0';
	  }

      cptr = key;

      while (*cptr && *cptr != '\t' && *cptr != ' ')
	++cptr;

      *cptr++ = '\0';

      while (*cptr == '\t' || *cptr == ' ')
	++cptr;

      if (strlen (key) == 0)
	{
	  if (strlen (cptr) != 0)
	    fprintf (stderr,
		     "makedbm: warning: malformed input data (ignored)\n");
	}
      else
	{
	  int i;

	  if (check_limit && strlen (key) > YPMAXRECORD)
	    {
	      fprintf (stderr, "makedbm: warning: key too long: %s\n", key);
	      continue;
	    }
	  kdat.dsize = strlen (key);
	  kdat.dptr = key;

	  if (check_limit && strlen (cptr) > YPMAXRECORD)
	    {
	      fprintf (stderr, "makedbm: warning: data too long: %s\n", cptr);
	      continue;
	    }
	  vdat.dsize = strlen (cptr);
	  vdat.dptr = cptr;

	  if (lower)
	    for (i = 0; i < kdat.dsize; i++)
	      kdat.dptr[i] = tolower (kdat.dptr[i]);

	  write_data (kdat, vdat);
	}
    }

  ypdb_close (dbm);
#if defined(HAVE_NDBM)
#if defined(__GLIBC__) && __GLIBC__ >= 2
  {
    char *dbm_db = strapp (dbmName, ".db");
    char *filedb = strapp (filename, ".db");

    unlink (dbm_db);
    rename (filedb, dbm_db);
  }
#else
  {
    char *dbm_pag = strapp (dbmName, ".pag");
    char *dbm_dir = strapp (dbmName, ".dir");
    char *filepag = strapp (filename, ".pag");
    char *filedir = strapp (filename, ".dir");

    unlink (dbm_pag);
    unlink (dbm_dir);
    rename (filepag, dbm_pag);
    rename (filedir, dbm_dir);
  }
#endif
#else
  unlink (dbmName);
  rename (filename, dbmName);
#endif
  free (filename);
}

static void
dump_file (char *dbmName)
{
  datum key, data;
#if defined(HAVE_LIBGDBM)
  dbm = gdbm_open (dbmName, 0, GDBM_READER, 0600, NULL);
#elif defined(HAVE_NDBM)
  dbm = dbm_open (dbmName, O_RDONLY, 0600);
#endif
  if (dbm == NULL)
    {
      fprintf (stderr, "makedbm: Cannot open %s\n", dbmName);
      exit (1);
    }
#if defined(HAVE_LIBGDBM)
  for (key = gdbm_firstkey (dbm); key.dptr; key = gdbm_nextkey (dbm, key))
    {
      data = gdbm_fetch (dbm, key);
      if (!data.dptr)
	{
	  fprintf (stderr, "Error:\n");
	  perror (dbmName);
	  exit (1);
	}
      printf ("%.*s %.*s\n",
	      key.dsize, key.dptr,
	      data.dsize, data.dptr);
      free (data.dptr);
    }
#elif defined(HAVE_NDBM)
  key = dbm_firstkey (dbm);
  while (key.dptr)
    {
      data = dbm_fetch (dbm, key);
      if (!data.dptr)
	{
	  fprintf (stderr, "Error:\n");
	  perror (dbmName);
	  exit (1);
	}
      printf ("%.*s %.*s\n",
	      key.dsize, key.dptr,
	      data.dsize, data.dptr);
      key = dbm_nextkey (dbm);
    }
#endif
  ypdb_close (dbm);
}

static void
send_clear (void)
{
  char in = 0;
  char *out = NULL;
  int stat;
  if ((stat = callrpc ("localhost", YPPROG, YPVERS, YPPROC_CLEAR,
		       (xdrproc_t) xdr_void, &in,
		       (xdrproc_t) xdr_void, out)) != RPC_SUCCESS)
    {
      fprintf (stderr, "failed to send 'clear' to local ypserv: %s",
	       clnt_sperrno ((enum clnt_stat) stat));
    }
}

static void
Usage (int exit_code)
{
  fprintf (stderr, "usage: makedbm -u dbname\n");
  fprintf (stderr, "       makedbm [-a|-r] [-b] [-c] [-s] [-l] [-i YP_INPUT_NAME]\n");
  fprintf (stderr, "               [-o YP_OUTPUT_NAME] [-m YP_MASTER_NAME] inputfile dbname\n");
  fprintf (stderr, "       makedbm -c\n");
  fprintf (stderr, "       makedbm --version\n");
  exit (exit_code);
}

int
main (int argc, char *argv[])
{
  char *domainName = NULL;
  char *inputName = NULL;
  char *outputName = NULL;
  char masterName[MAXHOSTNAMELEN + 1] = "";
  int dump = 0;
  int aliases = 0;
  int shortline = 0;
  int clear = 0;
  int b_flag = 0;
  int s_flag = 0;
  int remove_comments = 0;
  int check_limit = 1;

  while (1)
    {
      int c;
      int option_index = 0;
      static struct option long_options[] =
      {
	{"version", no_argument, NULL, '\255'},
	{"dump", no_argument, NULL, 'u'},
	{"help", no_argument, NULL, 'h'},
	{"usage", no_argument, NULL, 'h'},
	{"secure", no_argument, NULL, 's'},
	{"aliases", no_argument, NULL, 'a'},
	{"send_clear", no_argument, NULL, 'c'},
	{"remove-spaces", no_argument, NULL, '\254'},
	{"remove-comments", no_argument, NULL, 'r'},
	{"no-limit-check", no_argument, NULL, '\253'},
	{NULL, 0, NULL, '\0'}
      };

      c = getopt_long (argc, argv, "abcd:hi:lm:o:rsu", long_options, &option_index);
      if (c == EOF)
	break;
      switch (c)
	{
	case 'a':
	  aliases++;
	  shortline++;
	  break;
	case 'b':
	  b_flag++;
	  break;
	case 'c':
	  clear++;
	  break;
	case 'l':
	  lower++;
	  break;
	case 'u':
	  dump++;
	  break;
	case '\254':
	  shortline++;
	  break;
	case 'r':
	  remove_comments++;
	  break;
	case 's':
	  s_flag++;
	  break;
	case 'd':
	  domainName = optarg;
	  break;
	case 'i':
	  inputName = optarg;
	  break;
	case 'o':
	  outputName = optarg;
	  break;
	case 'm':
	  if (strlen (optarg) <= MAXHOSTNAMELEN)
	    strcpy (masterName, optarg);
	  else
	    fprintf (stderr, "hostname to long: %s\n", optarg);
	  break;
	case '\253':
	  check_limit = 0;
	  break;
	case '\255':
	  fprintf  (stdout, "makedbm (%s) %s", PACKAGE, VERSION);
	  return 0;
	case 'h':
	  Usage (0);
	  break;
	case '?':
	  Usage (1);
	  break;
	}
    }

  argc -= optind;
  argv += optind;

  if (dump)
    {
      if (argc < 1)
	Usage (1);
      else
	dump_file (argv[0]);
    }
  else
    {
      if (clear && argc == 0)
	{
	  send_clear ();
	  return 0;
	}

      if (argc < 2)
	Usage (1);
      else
	{
	  if (strlen (masterName) == 0)
	    {
	      if (gethostname (masterName, sizeof (masterName)) < 0)
		perror ("gethostname");
#if USE_FQDN
	      else
		{
		  struct hostent *hp;

		  if (!(hp = gethostbyname (masterName)))
		    perror ("gethostbyname()");
		  else
		    {
		      strncpy (masterName, hp->h_name, MAXHOSTNAMELEN);
		      masterName[MAXHOSTNAMELEN] = '\0';
		    }
		}
#endif
	    }
	  create_file (argv[0], argv[1], masterName, domainName,
		       inputName, outputName, aliases, shortline,
		       b_flag, s_flag, remove_comments, check_limit);

	  if (clear)
	    send_clear ();
	}
    }

  return 0;
}
