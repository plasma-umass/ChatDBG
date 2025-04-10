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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <getopt.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>

#include "yp.h"
#include "access.h"
#include "log_msg.h"

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif
#ifndef _PATH_VARRUN
#define _PATH_VARRUN "/etc/"
#endif
#define _YPSERV_PIDFILE _PATH_VARRUN"ypserv.pid"

#ifndef YPOLDVERS
#define YPOLDVERS 1
#endif

static char *path_ypdb = YPMAPDIR;

static void
ypprog_2 (struct svc_req *rqstp, register SVCXPRT * transp)
{
  union {
    domainname ypproc_domain_2_arg;
    domainname ypproc_domain_nonack_2_arg;
    ypreq_key ypproc_match_2_arg;
    ypreq_nokey ypproc_first_2_arg;
    ypreq_key ypproc_next_2_arg;
    ypreq_xfr ypproc_xfr_2_arg;
    ypreq_nokey ypproc_all_2_arg;
    ypreq_nokey ypproc_master_2_arg;
    ypreq_nokey ypproc_order_2_arg;
    domainname ypproc_maplist_2_arg;
  } argument;
  union {
    bool_t ypproc_domain_2_res;
    bool_t ypproc_domain_nonack_2_res;
    ypresp_val ypproc_match_2_res;
    ypresp_key_val ypproc_first_2_res;
    ypresp_key_val ypproc_next_2_res;
    ypresp_xfr ypproc_xfr_2_res;
    ypresp_all ypproc_all_2_res;
    ypresp_master ypproc_master_2_res;
    ypresp_order ypproc_order_2_res;
    ypresp_maplist ypproc_maplist_2_res;
  } result;
  bool_t retval;
  xdrproc_t _xdr_argument, _xdr_result;
  bool_t (*local) (char *, void *, struct svc_req *);

  switch (rqstp->rq_proc)
    {
    case YPPROC_NULL:
      _xdr_argument = (xdrproc_t) xdr_void;
      _xdr_result = (xdrproc_t) xdr_void;
      local =
	(bool_t (*)(char *, void *, struct svc_req *)) ypproc_null_2_svc;
      break;

    case YPPROC_DOMAIN:
      _xdr_argument = (xdrproc_t) xdr_domainname;
      _xdr_result = (xdrproc_t) xdr_bool;
      local =
	(bool_t (*)(char *, void *, struct svc_req *)) ypproc_domain_2_svc;
      break;

    case YPPROC_DOMAIN_NONACK:
      _xdr_argument = (xdrproc_t) xdr_domainname;
      _xdr_result = (xdrproc_t) xdr_bool;
      local =
	(bool_t (*)(char *, void *, struct svc_req *))
	ypproc_domain_nonack_2_svc;
      break;

    case YPPROC_MATCH:
      _xdr_argument = (xdrproc_t) xdr_ypreq_key;
      _xdr_result = (xdrproc_t) xdr_ypresp_val;
      local =
	(bool_t (*)(char *, void *, struct svc_req *)) ypproc_match_2_svc;
      break;

    case YPPROC_FIRST:
      _xdr_argument = (xdrproc_t) xdr_ypreq_nokey;
      _xdr_result = (xdrproc_t) xdr_ypresp_key_val;
      local =
	(bool_t (*)(char *, void *, struct svc_req *)) ypproc_first_2_svc;
      break;

    case YPPROC_NEXT:
      _xdr_argument = (xdrproc_t) xdr_ypreq_key;
      _xdr_result = (xdrproc_t) xdr_ypresp_key_val;
      local =
	(bool_t (*)(char *, void *, struct svc_req *)) ypproc_next_2_svc;
      break;

    case YPPROC_XFR:
      _xdr_argument = (xdrproc_t) xdr_ypreq_xfr;
      _xdr_result = (xdrproc_t) xdr_ypresp_xfr;
      local = (bool_t (*)(char *, void *, struct svc_req *)) ypproc_xfr_2_svc;
      break;

    case YPPROC_CLEAR:
      _xdr_argument = (xdrproc_t) xdr_void;
      _xdr_result = (xdrproc_t) xdr_void;
      local =
	(bool_t (*)(char *, void *, struct svc_req *)) ypproc_clear_2_svc;
      break;

    case YPPROC_ALL:
      _xdr_argument = (xdrproc_t) xdr_ypreq_nokey;
      _xdr_result = (xdrproc_t) xdr_ypresp_all;
      local = (bool_t (*)(char *, void *, struct svc_req *)) ypproc_all_2_svc;
      break;

    case YPPROC_MASTER:
      _xdr_argument = (xdrproc_t) xdr_ypreq_nokey;
      _xdr_result = (xdrproc_t) xdr_ypresp_master;
      local =
	(bool_t (*)(char *, void *, struct svc_req *)) ypproc_master_2_svc;
      break;

    case YPPROC_ORDER:
      _xdr_argument = (xdrproc_t) xdr_ypreq_nokey;
      _xdr_result = (xdrproc_t) xdr_ypresp_order;
      local =
	(bool_t (*)(char *, void *, struct svc_req *)) ypproc_order_2_svc;
      break;

    case YPPROC_MAPLIST:
      _xdr_argument = (xdrproc_t) xdr_domainname;
      _xdr_result = (xdrproc_t) xdr_ypresp_maplist;
      local =
	(bool_t (*)(char *, void *, struct svc_req *)) ypproc_maplist_2_svc;
      break;

    default:
      svcerr_noproc (transp);
      return;
    }

  memset ((char *) &argument, 0, sizeof (argument));
  if (!svc_getargs (transp, _xdr_argument, (caddr_t) &argument))
    {
      svcerr_decode (transp);
      return;
    }

  retval = (bool_t) (*local) ((char *) &argument, (void *) &result, rqstp);
  if (retval > 0 && !svc_sendreply (transp, _xdr_result, (char *) &result))
      svcerr_systemerr (transp);

  if (!svc_freeargs (transp, _xdr_argument, (caddr_t) &argument))
    {
      fprintf (stderr, "unable to free arguments");
      exit (1);
    }

  if (!ypprog_2_freeresult (transp, _xdr_result, (caddr_t) &result))
    fprintf (stderr, "unable to free results");

  return;
}


/* Create a pidfile on startup */
static void
create_pidfile (void)
{
  int fd, left, written, flags;
  pid_t pid;
  char pbuf[10], *ptr;
  struct flock lock;

  fd = open (_YPSERV_PIDFILE, O_CREAT | O_RDWR,
	     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd < 0)
    {
      log_msg ("cannot create pidfile %s", _YPSERV_PIDFILE);
      if (debug_flag)
	log_msg ("\n");
    }

  /* Make sure file gets correctly closed when process finished.  */
  flags = fcntl (fd, F_GETFD, 0);
  if (flags == -1)
    {
      /* Cannot get file flags.  */
      close (fd);
      return;
    }
  flags |= FD_CLOEXEC;		/* Close on exit.  */
  if (fcntl (fd, F_SETFD, flags) < 0)
    {
      /* Cannot set new flags.  */
      close (fd);
      return;
    }

  lock.l_type = F_WRLCK;
  lock.l_start = 0;
  lock.l_whence = SEEK_SET;
  lock.l_len = 0;

  /* Is the pidfile locked by another ypserv ? */
  if (fcntl (fd, F_GETLK, &lock) < 0)
    {
      log_msg ("fcntl error");
      if (debug_flag)
	log_msg ("\n");
    }
  if (lock.l_type == F_UNLCK)
    pid = 0;			/* false, not locked by another proc */
  else
    pid = lock.l_pid;		/* true, return pid of lock owner */

  if (0 != pid)
    {
      log_msg ("ypserv already running (pid %d) - exiting", pid);
      if (debug_flag)
	log_msg ("\n");
      exit (1);
    }

  /* write lock */
  lock.l_type = F_WRLCK;
  lock.l_start = 0;
  lock.l_whence = SEEK_SET;
  lock.l_len = 0;
  if (0 != fcntl (fd, F_SETLK, &lock))
    log_msg ("cannot lock pidfile");
  sprintf (pbuf, "%ld\n", (long) getpid ());
  left = strlen (pbuf);
  ptr = pbuf;
  while (left > 0)
    {
      if ((written = write (fd, ptr, left)) <= 0)
	return;			/* error */
      left -= written;
      ptr += written;
    }

  return;
}

/* Clean up if we quit the program. */
static void
sig_quit (int sig __attribute__ ((unused)))
{
  pmap_unset (YPPROG, YPVERS);
  pmap_unset (YPPROG, YPOLDVERS);
  unlink (_YPSERV_PIDFILE);

  exit (0);
}

/* Reload securenets and config file */
static void
sig_hup (int sig __attribute__ ((unused)))
{
  load_securenets ();
  load_config ();
}

static void
sig_child (int sig __attribute__ ((unused)))
{
  int st;

  /* Clear all childs */
  while (waitpid(-1, &st, WNOHANG) > 0)
    ;
}


static void
Usage (int exitcode)
{
  fputs ("usage: ypserv [-d [path]] [-p port]\n", stderr);
  fputs ("       ypserv --version\n", stderr);

  exit (exitcode);
}

int
main (int argc, char **argv)
{
  SVCXPRT *transp;
  int my_port = -1, my_socket, result;
  struct sockaddr_in s_in;
  struct sigaction sa;

  openlog ("ypserv", LOG_PID, LOG_DAEMON);

  while (1)
    {
      int c;
      int option_index = 0;
      static struct option long_options[] = {
	{"version", no_argument, NULL, 'v'},
	{"debug", no_argument, NULL, 'd'},
	{"port", required_argument, NULL, 'p'},
	{"usage", no_argument, NULL, 'u'},
	{"help", no_argument, NULL, 'h'},
	{NULL, 0, NULL, '\0'}
      };

      c = getopt_long (argc, argv, "vdp:buh", long_options, &option_index);
      if (c == EOF)
	break;
      switch (c)
	{
	case 'v':
	  debug_flag = 1;
	  log_msg ("ypserv (%s) %s\n", PACKAGE, VERSION);
	  return 0;
	case 'd':
	  ++debug_flag;
	  break;
	case 'p':
	  my_port = atoi (optarg);
	  if (debug_flag)
	    log_msg ("Using port %d\n", my_port);
	  break;
	case 'u':
	case 'h':
	  Usage (0);
	  break;
	default:
	  Usage (1);
	  break;
	}
    }

  argc -= optind;
  argv += optind;

  if (debug_flag)
    log_msg ("[ypserv (%s) %s]\n", PACKAGE, VERSION);
  else
    {
      int i;

      if ((i = fork ()) > 0)
	exit (0);

      if (i < 0)
	{
	  log_msg ("Cannot fork: %s\n", strerror (errno));
	  exit (-1);
	}

      if (setsid () == -1)
	{
	  log_msg ("Cannot setsid: %s\n", strerror (errno));
	  exit (-1);
	}

      if ((i = fork ()) > 0)
	exit (0);

      if (i < 0)
	{
	  log_msg ("Cannot fork: %s\n", strerror (errno));
	  exit (-1);
	}

      for (i = 0; i < getdtablesize (); ++i)
	close (i);
      errno = 0;

      umask (0);
      i = open ("/dev/null", O_RDWR);
      dup (i);
      dup (i);
    }

  if (argc > 0 && debug_flag)
    {
      path_ypdb = argv[0];
      log_msg ("Using database directory: %s\n", path_ypdb);
    }

  /* Change current directory to database location */
  if (chdir (path_ypdb) < 0)
    {
      log_msg ("ypserv: chdir: %s", strerror (errno));
      exit (1);
    }

  create_pidfile ();

  load_securenets ();
  load_config ();

  /*
   * Ignore SIGPIPEs. They can hurt us if someone does a ypcat
   * and then hits CTRL-C before it terminates.
   */
  sigaction (SIGPIPE, NULL, &sa);
  sa.sa_handler = SIG_IGN;
  sigemptyset (&sa.sa_mask);
  sigaction (SIGPIPE, &sa, NULL);
  /*
   * If program quits, give ports free.
   */
  sigaction (SIGTERM, NULL, &sa);
  sa.sa_handler = sig_quit;
  sigemptyset (&sa.sa_mask);
  sigaction (SIGTERM, &sa, NULL);

  sigaction (SIGINT, NULL, &sa);
  sa.sa_handler = sig_quit;
  sigemptyset (&sa.sa_mask);
  sigaction (SIGINT, &sa, NULL);

  /*
   * If we get a SIGHUP, reload the securenets and config file.
   */
  sigaction (SIGHUP, NULL, &sa);
  sa.sa_handler = sig_hup;
  sigemptyset (&sa.sa_mask);
  sigaction (SIGHUP, &sa, NULL);

  /*
   * On SIGCHLD wait for the child process, so it can give free all
   * resources.
   */
  sigaction (SIGCHLD, NULL, &sa);
  sa.sa_handler = sig_child;
  sigemptyset (&sa.sa_mask);
  sigaction (SIGCHLD, &sa, NULL);

  pmap_unset (YPPROG, YPVERS);
  pmap_unset (YPPROG, YPOLDVERS);

  if (my_port >= 0)
    {
      my_socket = socket (AF_INET, SOCK_DGRAM, 0);
      if (my_socket < 0)
	{
	  log_msg ("can not create UDP: %s", strerror (errno));
	  exit (1);
	}

      memset ((char *) &s_in, 0, sizeof (s_in));
      s_in.sin_family = AF_INET;
      s_in.sin_addr.s_addr = htonl (INADDR_ANY);
      s_in.sin_port = htons (my_port);

      result = bind (my_socket, (struct sockaddr *) &s_in,
		     sizeof (s_in));
      if (result < 0)
	{
	  log_msg ("ypserv: can not bind UDP: %s ", strerror (errno));
	  exit (1);
	}
    }
  else
    my_socket = RPC_ANYSOCK;

  transp = svcudp_create (my_socket);
  if (transp == NULL)
    {
      log_msg ("cannot create udp service.");
      exit (1);
    }

  if (!svc_register (transp, YPPROG, YPVERS, ypprog_2, IPPROTO_UDP))
    {
      log_msg ("unable to register (YPPROG, YPVERS, udp).");
      exit (1);
    }

  /* XXX ypprog_2 should be ypprog_1 */
  if (!svc_register (transp, YPPROG, YPOLDVERS, ypprog_2, IPPROTO_UDP))
    {
      log_msg ("unable to register (YPPROG, YPOLDVERS, udp).");
      exit (1);
    }

  if (my_port >= 0)
    {
      my_socket = socket (AF_INET, SOCK_STREAM, 0);
      if (my_socket < 0)
	{
	  log_msg ("ypserv: can not create TCP: %s", strerror (errno));
	  exit (1);
	}

      memset (&s_in, 0, sizeof (s_in));
      s_in.sin_family = AF_INET;
      s_in.sin_addr.s_addr = htonl (INADDR_ANY);
      s_in.sin_port = htons (my_port);

      result = bind (my_socket, (struct sockaddr *) &s_in,
		     sizeof (s_in));
      if (result < 0)
	{
	  log_msg ("ypserv: can not bind TCP ", strerror (errno));
	  exit (1);
	}
    }
  else
    my_socket = RPC_ANYSOCK;

  transp = svctcp_create (my_socket, 0, 0);
  if (transp == NULL)
    {
      log_msg ("ypserv: cannot create tcp service\n");
      exit (1);
    }

  if (!svc_register (transp, YPPROG, YPVERS, ypprog_2, IPPROTO_TCP))
    {
      log_msg ("ypserv: unable to register (YPPROG, YPVERS, tcp)\n");
      exit (1);
    }

  /* XXX ypprog_2 should be ypprog_1 */
  if (!svc_register (transp, YPPROG, YPOLDVERS, ypprog_2, IPPROTO_TCP))
    {
      log_msg ("ypserv: unable to register (YPPROG, YPOLDVERS, tcp)\n");
      exit (1);
    }

  svc_run ();
  log_msg ("svc_run returned");
  unlink (_YPSERV_PIDFILE);
  exit (1);
  /* NOTREACHED */
}
