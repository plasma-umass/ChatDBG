/* Copyright (c) 1996, 1997, 1998, 1999, 2000, 2001 Thorsten Kukuk
   Author: Thorsten Kukuk <kukuk@suse.de>

   The YP Server is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

   The YP Server is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with the YP Server; see the file COPYING. If
   not, write to the Free Software Foundation, Inc., 675 Mass Ave,
   Cambridge, MA 02139, USA. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <rpc/rpc.h>
#include <time.h>
#include "yp.h"
#include <rpcsvc/ypclnt.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <ctype.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#if defined(HAVE_LIBGDBM)
#include <gdbm.h>
#elif defined(HAVE_NDBM)
#include <ndbm.h>
#include <fcntl.h>
#endif
#include <getopt.h>

#include "log_msg.h"

#ifndef HAVE_STRDUP
#include <compat/strdup.c>
#endif

#ifndef HAVE_GETOPT_LONG
#include <compat/getopt.c>
#include <compat/getopt1.c>
#endif

#ifndef HAVE_STRERROR
#include <compat/strerror.c>
#endif

#ifndef YPMAPDIR
#define YPMAPDIR "/var/yp"
#endif

struct hostlist {
  char *hostname;
  struct hostlist *next;
};

struct hostlist *hostliste = NULL;

static char *DomainName = NULL;
int verbose_flag = 0;
static char local_hostname[MAXHOSTNAMELEN + 2];
static char *current_map;
static u_int CallbackProg = 0;
static u_int timeout = 90;
static u_int MapOrderNum;
static u_int maxchildren = 1;
static u_int children = 0;

#if HAVE__RPC_DTABLESIZE
extern int _rpc_dtablesize (void);
#elif HAVE_GETDTABLESIZE

int
_rpc_dtablesize ()
{
  static int size;

  if (size == 0)
    size = getdtablesize ();

  return size;
}
#else

#include <sys/resource.h>

int
_rpc_dtablesize ()
{
  static int size = 0;
  struct rlimit rlb;


  if (size == 0)
    if (getrlimit (RLIMIT_NOFILE, &rlb) >= 0)
      size = rlb.rlim_cur;

  return size;
}
#endif

static char *
yppush_err_string (enum yppush_status status)
{
  switch (status)
    {
    case YPPUSH_SUCC:
      return "Success";
    case YPPUSH_AGE:
      return "Master's version not newer";
    case YPPUSH_NOMAP:
      return "Can't find server for map";
    case YPPUSH_NODOM:
      return "Domain not supported";
    case YPPUSH_RSRC:
      return "Local resource alloc failure";
    case YPPUSH_RPC:
      return "RPC failure talking to server";
    case YPPUSH_MADDR:
      return "Can't get master address";
    case YPPUSH_YPERR:
      return "YP server/map db error";
    case YPPUSH_BADARGS:
      return "Request arguments bad";
    case YPPUSH_DBM:
      return "Local dbm operation failed";
    case YPPUSH_FILE:
      return "Local file I/O operation failed";
    case YPPUSH_SKEW:
      return "Map version skew during transfer";
    case YPPUSH_CLEAR:
      return "Can't send \"Clear\" req to local ypserv";
    case YPPUSH_FORCE:
      return "No local order number in map  use -f flag.";
    case YPPUSH_XFRERR:
      return "ypxfr error";
    case YPPUSH_REFUSED:
      return "Transfer request refused by ypserv";
    }
  return "YPPUSH: Unknown Error, this should not happen!";
}

bool_t
yppushproc_null_1_svc (void *req __attribute__ ((unused)),
		       void *resp __attribute__ ((unused)),
		       struct svc_req *rqstp __attribute__ ((unused)))
{
  resp = NULL;

  if (verbose_flag > 1)
    log_msg ("yppushproc_null_1_svc");

  return TRUE;
}


bool_t
yppushproc_xfrresp_1_svc (yppushresp_xfr *req, void *resp, struct svc_req *rqstp)
{
  struct sockaddr_in *sin;
  char *h;
  struct hostent *hp;

  if (verbose_flag > 1)
    log_msg ("yppushproc_xfrresp_1_svc");

  sin = svc_getcaller (rqstp->rq_xprt);

  hp = gethostbyaddr ((char *) &sin->sin_addr.s_addr,
		      sizeof (sin->sin_addr.s_addr), AF_INET);
  h = (hp && hp->h_name) ? hp->h_name : inet_ntoa (sin->sin_addr);

  memcpy ((yppushresp_xfr *) resp, req, sizeof (yppushresp_xfr));
  if (verbose_flag)
    {
      log_msg ("Status received from ypxfr on %s:", h);
      log_msg ("\tTransfer %sdone: %s", req->status == YPPUSH_SUCC ? "" : "not ",
	      yppush_err_string (req->status));
    }
  else if (req->status != YPPUSH_SUCC)
    log_msg ("%s: %s", h, yppush_err_string (req->status));

  return TRUE;
}

static void
yppush_xfrrespprog_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
  union {
    yppushresp_xfr yppushproc_xfrresp_1_arg;
  } argument;
  union {
  } result;
  bool_t retval;
  xdrproc_t _xdr_argument, _xdr_result;
  bool_t (*local)(char *, void *, struct svc_req *);

  if (verbose_flag > 1)
    log_msg ("yppush_xfrrespprog_1");

  switch (rqstp->rq_proc) {
  case YPPUSHPROC_NULL:
    _xdr_argument = (xdrproc_t) xdr_void;
    _xdr_result = (xdrproc_t) xdr_void;
    local = (bool_t (*) (char *, void *,  struct svc_req *))yppushproc_null_1_svc;
    break;

  case YPPUSHPROC_XFRRESP:
    _xdr_argument = (xdrproc_t) xdr_yppushresp_xfr;
    _xdr_result = (xdrproc_t) xdr_void;
    local = (bool_t (*) (char *, void *,  struct svc_req *))yppushproc_xfrresp_1_svc;
    break;

  default:
    svcerr_noproc (transp);
    return;
  }
  memset ((char *)&argument, 0, sizeof (argument));
  if (!svc_getargs (transp, _xdr_argument, (caddr_t) &argument))
    {
      svcerr_decode (transp);
      return;
    }
  retval = (bool_t) (*local)((char *)&argument, (void *)&result, rqstp);
  if (retval > 0 && !svc_sendreply(transp, _xdr_result, (char *)&result))
    {
      svcerr_systemerr (transp);
    }
  if (!svc_freeargs (transp, _xdr_argument, (caddr_t) &argument)) {
    log_msg ("unable to free arguments");
    exit (1);
  }

#if 0
  /* XXX */
  if (!yppush_xfrrespprog_1_freeresult (transp, _xdr_result, (caddr_t) &result))
    log_msg ("unable to free results");
#endif
  return;
}

static void
yppush_svc_run (char *target)
{
  fd_set readfds;
  struct timeval tr, tb;

  tb.tv_sec = timeout;
  tb.tv_usec = 0;
  tr = tb;

  for (;;)
    {
      readfds = svc_fdset;

      switch (select (_rpc_dtablesize (), &readfds, (void *) 0, (void *) 0, &tr))
	{
	case -1:
	  if (errno == EINTR)
	    {
	      tr = tb;		/* Read the Linux select.2 manpage ! */
	      continue;
	    }
	  log_msg ("svc_run: - select failed (%s)", strerror (errno));
	  return;
	case 0:
	  log_msg ("%s->%s: Callback timed out", current_map, target);
	  exit (0);
	default:
	  svc_getreqset (&readfds);
	  break;
	}
    }
}

/*
 *    Compare 2 hostnames.
 */
static int
hostcmp (char *h1, char *h2)
{
  char buf1[MAXHOSTNAMELEN + 1], buf2[MAXHOSTNAMELEN + 1];
  char *p, *s;

  strncpy (buf1, h1, sizeof (buf1));
  strncpy (buf2, h2, sizeof (buf2));
  s = strchr (buf1, '.');
  p = strchr (buf2, '.');
  if (s && !p)
    *s = 0;
  if (p && !s)
    *p = 0;

  return strcasecmp (buf1, buf2);
}

static char *
get_dbm_entry (char *key)
{
  static char mappath[MAXPATHLEN + 2];
  char *val;
  datum dkey, dval;
#if defined(HAVE_LIBGDBM)
  GDBM_FILE dbm;
#elif defined (HAVE_NDBM)
  DBM *dbm;
#endif

  if (strlen (YPMAPDIR) + strlen (DomainName) + strlen (current_map) + 3 < MAXPATHLEN)
    sprintf (mappath, "%s/%s/%s", YPMAPDIR, DomainName, current_map);
  else
    {
      log_msg ("YPPUSH ERROR: Path to long: %s/%s/%s", YPMAPDIR, DomainName, current_map);
      exit (1);
    }

#if defined(HAVE_LIBGDBM)
  dbm = gdbm_open (mappath, 0, GDBM_READER, 0600, NULL);
#elif defined(HAVE_NDBM)
  dbm = dbm_open (mappath, O_CREAT | O_RDWR, 0600);
#endif
  if (dbm == NULL)
    {
      log_msg ("YPPUSH: Cannot open %s", mappath);
      exit (1);
    }

  dkey.dptr = key;
  dkey.dsize = strlen (dkey.dptr);
#if defined(HAVE_LIBGDBM)
  dval = gdbm_fetch (dbm, dkey);
#elif defined(HAVE_NDBM)
  dval = dbm_fetch (dbm, dkey);
#endif
  if (dval.dptr == NULL)
    val = NULL;
  else
    {
      val = malloc (dval.dsize + 1);
      strncpy (val, dval.dptr, dval.dsize);
      val[dval.dsize] = 0;
    }
#if defined(HAVE_LIBGDBM)
  gdbm_close (dbm);
#elif defined(HAVE_NDBM)
  dbm_close (dbm);
#endif
  return val;
}

static u_int
getordernum (void)
{
  char *val;
  u_int i;

  val = get_dbm_entry ("YP_LAST_MODIFIED");

  if (val == NULL)
    {
      if (verbose_flag > 1)
	log_msg ("YPPUSH ERROR: Cannot determine order number for %s", current_map);
      free (val);
      return 0;
    }

  for (i = 0; i < strlen (val); ++i)
    {
      if (!isdigit (val[i]))
	{
	  log_msg ("YPPUSH ERROR: Order number '%s' in map %s is invalid!",
		   current_map, val);
	  free (val);
	  return 0;
	}
    }

  i = atoi (val);
  free (val);
  return i;
}

/* Create with the ypservers or slaves.hostname map a list with all
   slave servers we should send the new map */

/* NetBSD has a different prototype in struct ypall_callback */
#if defined(__NetBSD__)
static int
add_slave_server (u_long status, char *key, int keylen,
		  char *val, int vallen, void *data __attribute__ ((unused)))
#else
static int
add_slave_server (int status, char *key, int keylen,
		  char *val, int vallen, char *data __attribute__ ((unused)))
#endif
{
  char host[YPMAXPEER + 2];
  struct hostlist *tmp;

  if (verbose_flag > 1)
    log_msg ("add_slave_server: Key=%.*s, Val=%.*s, status=%d", keylen, key,
	     vallen, val, status);

  if (status != YP_TRUE)
    return status;

  if (vallen < YPMAXPEER)
    sprintf (host, "%.*s", vallen, val);
  else
    {
      log_msg ("YPPUSH ERROR: add_slave_server: %.*s to long", vallen, val);
      exit (1);
    }

  /* Do not add ourself! */
  if (hostcmp (local_hostname, host) == 0)
    {
      if (verbose_flag > 1)
	log_msg ("YPPUSH INFO: skipping %s", host);
      return 0;
    }

  if ((tmp = (struct hostlist *) malloc (sizeof (struct hostlist))) == NULL)
    {
      log_msg ("malloc() failed: %s", strerror (errno));
      return -1;
    }
  tmp->hostname = strdup (host);
  tmp->next = hostliste;
  hostliste = tmp;

  return 0;
}

static void
child_sig_int (int sig __attribute__ ((unused)))
{
  if (CallbackProg != 0)
    svc_unregister (CallbackProg, 1);
  exit (1);
}

static int
yppush_foreach (const char *host)
{
  SVCXPRT *CallbackXprt;
  CLIENT *PushClient = NULL;
  struct ypreq_xfr req;
  struct timeval tv = {10, 0};
  u_int transid;
  char server[YPMAXPEER + 2];
  int sock;
  struct sigaction sa;

  if (verbose_flag > 1)
    log_msg ("yppush_foreach: host=%s", host);

  sa.sa_handler = child_sig_int;
  sigemptyset (&sa.sa_mask);
#if defined(linux) || (defined(sun) && defined(__srv4__))
  sa.sa_flags = SA_NOMASK;
  /* Do  not  prevent  the  signal   from   being
     received from within its own signal handler. */
#endif
  sigaction (SIGINT, &sa, NULL);

  if (strlen (host) < YPMAXPEER)
    sprintf (server, "%s", host);
  else
    {
      log_msg ("YPPUSH ERROR: yppush_foreach: %.*s to long", host);
      exit (1);
    }

  PushClient = clnt_create (server, YPPROG, YPVERS, "udp");
  if (PushClient == NULL)
    {
      log_msg ("%s", host);
      clnt_pcreateerror ("");
      return 1;
    }

  sock = RPC_ANYSOCK;
  CallbackXprt = svcudp_create (sock);
  if (CallbackXprt == NULL)
    {
      log_msg ("YPPUSH: Cannot create callback transport to host \"%s\".", server);
      return 1;
    }
  for (CallbackProg = 0x40000000; CallbackProg < 0x5fffffff; CallbackProg++)
    {
      if (svc_register (CallbackXprt, CallbackProg, 1,
			yppush_xfrrespprog_1, IPPROTO_UDP))
	break;
    }

  switch (transid = fork ())
    {
    case -1:
      perror ("Cannot fork");
      exit (-1);
    case 0:
      yppush_svc_run (server);
      exit (0);
    default:
      close (CallbackXprt->xp_sock);
      req.map_parms.domain = (char *) DomainName;
      req.map_parms.map = (char *) current_map;
      /* local_hostname is correct since we have compared it with YP_MASTER_NAME */
      req.map_parms.peer = local_hostname;
      req.map_parms.ordernum = MapOrderNum;
      req.transid = transid;
      req.prog = CallbackProg;
      req.port = CallbackXprt->xp_port;

      if (verbose_flag)
	{
	  log_msg ("%s has been called.", server);
	  if (verbose_flag > 1)
	    {
	      log_msg ("\t->target: %s", server);
	      log_msg ("\t->domain: %s", req.map_parms.domain);
	      log_msg ("\t->map: %s", req.map_parms.map);
	      log_msg ("\t->tarnsid: %d", req.transid);
	      log_msg ("\t->prog: %d", req.prog);
	      log_msg ("\t->master: %s", req.map_parms.peer);
	      log_msg ("\t->ordernum: %d", req.map_parms.ordernum);
	    }
	}

      if (clnt_call (PushClient, YPPROC_XFR, (xdrproc_t) xdr_ypreq_xfr,
		     (caddr_t) &req, (xdrproc_t) xdr_void, NULL,
		     tv) != RPC_SUCCESS)
	{
	  log_msg ("YPPUSH: Cannot call YPPROC_XFR on host \"%s\"%s", server,
		   clnt_sperror (PushClient, ""));
	  kill (transid, SIGTERM);
	}

      waitpid (transid, &sock, 0);
      svc_unregister (CallbackProg, 1);
      CallbackProg = 0;
      if (PushClient != NULL)
	{
	  clnt_destroy (PushClient);
	  PushClient = NULL;
	}
    }

  return 0;
}

static void
sig_child (int sig __attribute__ ((unused)))
{
  int status;

  while (waitpid (-1, &status, WNOHANG) > 0)
    {
      if (verbose_flag > 1)
	log_msg ("Child %d exists", WEXITSTATUS (status));
      children--;
    }
}

static inline void
Usage (int exit_code)
{
  log_msg ("Usage: yppush [-d domain] [-t timeout] [-p #] [-h host] [-v] mapname ...");
  log_msg ("       yppush --version");
  exit (exit_code);
}

int
main (int argc, char **argv)
{
  struct hostlist *tmp;
  enum ypstat y;
  struct sigaction sig;

  debug_flag = 1;

  sig.sa_handler = sig_child;
  sigemptyset (&sig.sa_mask);
#if defined(linux) || (defined(sun) && defined(__srv4__))
  sig.sa_flags = SA_NOMASK;
  /* Do  not  prevent  the  signal   from   being
     received from within its own signal handler. */
#endif
  sigaction (SIGCHLD, &sig, NULL);

  while (1)
    {
      int c;
      int option_index = 0;
      static struct option long_options[] =
      {
	{"version", no_argument, NULL, '\255'},
	{"verbose", no_argument, NULL, 'v'},
	{"host", required_argument, NULL, 'h'},
	{"help", no_argument, NULL, 'u'},
	{"usage", no_argument, NULL, 'u'},
	{"parallel", required_argument, NULL, 'p'},
	{"timeout", required_argument, NULL, 't'},
	{NULL, 0, NULL, '\0'}
      };

      c = getopt_long (argc, argv, "d:vh:ut:p:j:", long_options, &option_index);
      if (c == EOF)
	break;
      switch (c)
	{
	case 'd':
	  DomainName = optarg;
	  break;
	case 'v':
	  verbose_flag++;
	  break;
	case 't':
	  timeout = atoi (optarg);
	  break;
	case 'j':
	case 'p':
	  maxchildren = atoi (optarg);
	  break;
	case 'h':
	  /* we can handle multiple hosts */
	  tmp = (struct hostlist *) malloc (sizeof (struct hostlist));
	  if (tmp == NULL)
	    {
	      log_msg ("malloc() failed: %s", strerror (errno));
	      return 1;
	    }
	  tmp->hostname = strdup (optarg);
	  tmp->next = hostliste;
	  hostliste = tmp;
	  break;
	case 'u':
	  Usage (0);
	  break;
	case '\255':
          log_msg ("yppush (%s) %s", PACKAGE, VERSION);
          return 0;
	default:
	  Usage (1);
	}
    }

  argc -= optind;
  argv += optind;

  if (argc < 1)
    Usage (1);

  if (DomainName == NULL)
    {
      if (yp_get_default_domain (&DomainName) != 0)
	{
	  log_msg ("YPPUSH: Cannot get default domain");
	  return 1;
	}
      if (strlen(DomainName) == 0)
	{
	  log_msg ("YPPUSH: Domainname not set");
	  return 1;
	}
    }

  if (gethostname (local_hostname, MAXHOSTNAMELEN) != 0)
    {
      perror ("YPPUSH: gethostname");
      log_msg ("YPPUSH: Cannot determine local hostname");
      return 1;
    }
#if USE_FQDN
  else
    {
      struct hostent *hp;

      if (!(hp = gethostbyname (local_hostname)))
	{
	  perror ("YPPUSH: gethostbyname()");
	  log_msg ("YPPUSH: using not FQDN name");
	}
      else
	{
	  strncpy (local_hostname, hp->h_name, MAXHOSTNAMELEN);
	  local_hostname[MAXHOSTNAMELEN] = '\0';
	}
    }
#endif

  if (hostliste == NULL)
    {
      struct ypall_callback f;

      memset (&f, 0, sizeof f);
      f.foreach = add_slave_server;
#ifdef OSF_KLUDGE
      y = yp_all (DomainName, "ypservers", f);
#else
      y = yp_all (DomainName, "ypservers", &f);
#endif
      if (y && y != YP_NOMORE)
	{
	  log_msg ("Could not read ypservers map: %d %s", y, yperr_string (y));
	}
    }

  while (*argv)
    {
      char *val;

      current_map = *argv++;
      val = get_dbm_entry ("YP_MASTER_NAME");
      if (val && strcasecmp (val, local_hostname) != 0)
	{
	  log_msg ("YPPUSH: %s is not the master for %s, try it from %s.",
		  local_hostname, current_map, val);
	  free (val);
	  continue;
	}
      else if (val)
	free (val);

      MapOrderNum = getordernum ();
#if 0
      if (MapOrderNum == 0xffffffff)
	continue;
#endif
      tmp = hostliste;
      while (tmp != NULL)
	{
	  while (children >= maxchildren)
	    sleep (1);
	  children++;
	  switch (fork ())
	    {
	    case -1:
	      perror ("YPPUSH: Cannot fork");
	      exit (1);
	    case 0:
	      yppush_foreach (tmp->hostname);
	      exit (children);
	    default:
	      if (verbose_flag > 1)
		log_msg ("Start new child (%d)", children);
	      break;
	    }
	  tmp = tmp->next;
	}
      while (children != 0)
	{
	  sleep (10);
	  if (verbose_flag > 1)
	    log_msg ("Running Children: %d", children);
	}
    }

  if (verbose_flag > 1)
    log_msg ("all done (%d running childs)", children);

  return 0;
}
