/*
   Copyright (c) 1996, 1997, 1998, 1999, 2001 Thorsten Kukuk, <kukuk@suse.de>
   Copyright (c) 1994, 1995, 1996 Olaf Kirch, <okir@monad.swb.de>

   This file is part of the NYS YP Server.

   The NYS YP Server is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The NYS YP Server is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with the NYS YP Server; see the file COPYING.  If
   not, write to the Free Software Foundation, Inc., 675 Mass Ave,
   Cambridge, MA 02139, USA. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#if defined(HAVE_RPC_SVC_SOC_H)
#include <rpc/svc_soc.h>
#endif
#include "yppasswd.h"
#if defined(HAVE_GETOPT_H) && defined(HAVE_GETOPT_LONG)
#include <getopt.h>
#else
#include <compat/getopt.h>
#endif
#ifndef HAVE_GETOPT_LONG
#include <compat/getopt.c>
#include <compat/getopt1.c>
#endif

#ifndef HAVE_STRERROR
#include <compat/strerror.c>
#endif

#include "log_msg.h"

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif
#ifndef _PATH_VARRUN
#define _PATH_VARRUN "/etc/"
#endif
#define _YPPASSWDD_PIDFILE _PATH_VARRUN"yppasswdd.pid"

int use_shadow = 0;
int allow_chsh = 0;
int allow_chfn = 0;
int solaris_mode = -1;
int x_flag = -1;

#define xprt_addr(xprt)	(svc_getcaller(xprt)->sin_addr)
#define xprt_port(xprt)	ntohs(svc_getcaller(xprt)->sin_port)
void yppasswdprog_1 (struct svc_req *rqstp, SVCXPRT * transp);
void reaper (int sig);

/*==============================================================*
 * RPC dispatch function
 *==============================================================*/
void
yppasswdprog_1 (struct svc_req *rqstp, SVCXPRT * transp)
{
  yppasswd argument;
  int *result;
  xdrproc_t xdr_argument, xdr_result;

  switch (rqstp->rq_proc)
    {
    case NULLPROC:
      svc_sendreply (transp, (xdrproc_t) xdr_void, (char *) NULL);
      return;

    case YPPASSWDPROC_UPDATE:
      xdr_argument = (xdrproc_t) xdr_yppasswd;
      xdr_result = (xdrproc_t) xdr_int;
      break;

    default:
      svcerr_noproc (transp);
      return;
    }
  memset ((char *) &argument, 0, sizeof (argument));
  if (!svc_getargs (transp, xdr_argument, (caddr_t) &argument))
    {
      svcerr_decode (transp);
      return;
    }
  result = yppasswdproc_pwupdate_1 (&argument, rqstp);
  if (result != NULL
      && !svc_sendreply (transp, (xdrproc_t) xdr_result, (char *)result))
    {
      svcerr_systemerr (transp);
    }
  if (!svc_freeargs (transp, xdr_argument, (caddr_t) &argument))
    {
      log_msg ("unable to free arguments\n");
      exit (1);
    }
}

/* Create a pidfile on startup */
static void
create_pidfile (void)
{
  int fd, left, written;
  pid_t pid;
  char pbuf[50], *ptr;
  struct flock lock;

  fd = open (_YPPASSWDD_PIDFILE, O_CREAT | O_RDWR,
	     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd < 0)
    {
      log_msg ("cannot create pidfile %s", _YPPASSWDD_PIDFILE);
      if (debug_flag)
	log_msg ("\n");
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
    pid = 0;	        /* false, region is not locked by another proc */
  else
    pid = lock.l_pid;	/* true, return pid of lock owner */

  if (0 != pid)
    {
      log_msg ("rpc.yppasswdd already running (pid %d) - exiting", pid);
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


static void
usage (FILE * fp, int n)
{
  fputs ("Usage: rpc.yppasswdd [--debug] [-s shadowfile] [-p passwdfile] [-e chsh|chfn]\n", fp);
  fputs ("       rpc.yppasswdd [--debug] [-D directory] [-e chsh|chfn]\n", fp);
  fputs ("       rpc.yppasswdd [--debug] [-x program |-E program] [-e chsh|chfn]\n", fp);
  fputs ("       rpc.yppasswdd --port number\n", fp);
  fputs ("       rpc.yppasswdd --version\n", fp);
  exit (n);
}

static void
sig_child (int sig __attribute__ ((unused)))
{
  wait (NULL);
}

/* Clean up if we quit the program. */
static void
sig_quit (int sig __attribute__ ((unused)))
{
  pmap_unset (YPPASSWDPROG, YPPASSWDVERS);
  unlink (_YPPASSWDD_PIDFILE);
  exit (0);
}


static void
install_sighandler (void)
{
  struct sigaction sa;

  sigaction (SIGPIPE, NULL, &sa);
  sa.sa_handler = SIG_IGN;
#if !defined(sun) || (defined(sun) && defined(__svr4__))
  sa.sa_flags |= SA_RESTART;
  /* The opposite to SA_ONESHOT, do  not  restore
     the  signal  action.  This provides behavior
     compatible with BSD signal semantics. */
#endif
  sigemptyset (&sa.sa_mask);
  sigaction (SIGPIPE, &sa, NULL);
  /* Clear up if child exists */
  sigaction (SIGCHLD, NULL, &sa);
#if !defined(sun) || (defined(sun) && defined(__svr4__))
  sa.sa_flags |= SA_RESTART;
#endif
  sa.sa_handler = sig_child;
  sigemptyset (&sa.sa_mask);
  sigaction (SIGCHLD, &sa, NULL);
  /* If program quits, give ports free. */
  sigaction (SIGTERM, NULL, &sa);
#if !defined(sun) || (defined(sun) && defined(__svr4__))
  sa.sa_flags |= SA_RESTART;
#endif
  sa.sa_handler = sig_quit;
  sigemptyset (&sa.sa_mask);
  sigaction (SIGTERM, &sa, NULL);

  sigaction (SIGINT, NULL, &sa);
#if !defined(sun) || (defined(sun) && defined(__svr4__))
  sa.sa_flags |= SA_RESTART;
#endif
  sa.sa_handler = sig_quit;
  sigemptyset (&sa.sa_mask);
  sigaction (SIGINT, &sa, NULL);
}


int
main (int argc, char **argv)
{
  SVCXPRT *transp;
  int my_port = -1, my_socket;
  int c;

  /* Initialize logging. */
  openlog ("rpc.yppasswdd", LOG_PID, LOG_AUTH);

  /* Parse the command line options and arguments. */
  while (1)
    {
      int option_index = 0;
      static struct option long_options[] =
      {
	{"version", no_argument, NULL, '\255'},
	{"usage", no_argument, NULL, 'h'},
	{"help", no_argument, NULL, 'h'},
	{"execute", required_argument, NULL, 'x'},
	{"debug", no_argument, NULL, '\254'},
	{"port", required_argument, NULL, '\253'},
	{NULL, 0, NULL, '\0'}
      };

      c=getopt_long (argc, argv, "e:p:s:uhvD:E:x:m", long_options,
		     &option_index);
      if (c == EOF)
	break;
      switch (c)
	{
	case 'e':
	  if (!strcmp (optarg, "chsh"))
	    allow_chsh = 1;
	  else if (!strcmp (optarg, "chfn"))
	    allow_chfn = 1;
	  else
	    usage (stderr, 1);
	  break;
	case 'p':
	  if (solaris_mode == 1)
	    usage (stderr, 1);
	  solaris_mode = 0;
	  path_passwd = optarg;
	  break;
	case 's':
	  if (solaris_mode == 1)
	    usage (stderr, 1);
	  solaris_mode = 0;
#ifdef HAVE_GETSPNAM
	  path_shadow = optarg;
#endif
	  break;
	case 'D':
	  if (solaris_mode == 0)
	    usage (stderr, 1);
	  solaris_mode = 1;
	  path_passwd = malloc (strlen (optarg) + 8);
	  sprintf (path_passwd, "%s/passwd", optarg);
#ifdef HAVE_GETSPNAM
	  path_shadow = malloc (strlen (optarg) + 8);
	  sprintf (path_shadow, "%s/shadow", optarg);
#endif
	  break;
	case 'E':
	  external_update_program = strdup(optarg);
	  x_flag = 0;
	  break;
	case 'x':
	  external_update_program = strdup(optarg);
	  x_flag = 1;
	  break;
	case 'm':
	  if (solaris_mode == 0)
	    usage (stderr, 1);
	  solaris_mode = 1;
	  /* do nothing for now. We always run make, and we uses the
	     fastest arguments */
	  break;
	case 'h':
	  usage (stdout, 0);
	  break;
	case '\253':
          my_port = atoi (optarg);
          if (debug_flag)
            log_msg ("Using port %d\n", my_port);
          break;
	case '\255':
#if CHECKROOT
	  fprintf (stdout, "rpc.yppasswdd - YP server version %s (with CHECKROOT)\n",
		   VERSION);
#else /* NO CHECKROOT */
	  fprintf (stdout, "rpc.yppasswdd - YP server version %s\n",
		   VERSION);
#endif /* CHECKROOT */
	  exit (0);
	case '\254': /* --debug */
	  debug_flag = 1;
	  break;
	default:
	  usage (stderr, 1);
	}
    }

  /* No more arguments allowed. */
  if (optind != argc)
    usage (stderr, 1);

  /* Create tmp and .OLD file names for "passwd" */
  path_passwd_tmp = malloc (strlen (path_passwd) + 5);
  if (path_passwd_tmp == NULL)
    {
      log_msg ("rpc.yppasswdd: out of memory\n");
      exit (-1);
    }
  sprintf (path_passwd_tmp, "%s.tmp", path_passwd);
  path_passwd_old = malloc (strlen (path_passwd) + 5);
  if (path_passwd_old == NULL)
    {
      log_msg ("rpc.yppasswdd: out of memory\n");
      exit (-1);
    }
  sprintf (path_passwd_old, "%s.OLD", path_passwd);
#ifdef HAVE_GETSPNAM
  /* Create tmp and .OLD file names for "shadow" */
  path_shadow_tmp = malloc (strlen (path_shadow) + 5);
  if (path_shadow_tmp == NULL)
    {
      log_msg ("rpc.yppasswdd: out of memory\n");
      exit (-1);
    }
  sprintf (path_shadow_tmp, "%s.tmp", path_shadow);
  path_shadow_old = malloc (strlen (path_shadow) + 5);
  if (path_shadow_old == NULL)
    {
      log_msg ("rpc.yppasswdd: out of memory\n");
      exit (-1);
    }
  sprintf (path_shadow_old, "%s.OLD", path_shadow);
#endif /* HAVE_GETSPNAM */

  if (debug_flag)
    {
#if CHECKROOT
      log_msg ("rpc.yppasswdd - NYS YP server version %s (with CHECKROOT)\n",
	      VERSION);
#else /* NO CHECKROOT */
      log_msg ("rpc.yppasswdd - NYS YP server version %s\n", VERSION);
#endif /* CHECKROOT */
    }
  else
    {
      int i;

      /* We first fork off a child. */
      if ((i = fork ()) > 0)
	exit (0);

      if (i < 0)
	{
	  log_msg ("rpc.yppasswdd: cannot fork: %s\n", strerror (errno));
	  exit (-1);
	}

      if (setsid() == -1)
	{
	  log_msg ("rpc.yppasswdd: cannot setsid: %s\n", strerror (errno));
	  exit (-1);
	}

      if ((i = fork ()) > 0)
	exit (0);

      if (i < 0)
	{
	  log_msg ("rpc.yppasswdd: cannot fork: %s\n", strerror (errno));
	  exit (-1);
	}

      for (i = 0; i < getdtablesize (); ++i)
        close (i);
      errno = 0;

      chdir ("/");
      umask(0);
      i = open("/dev/null", O_RDWR);
      dup(i);
      dup(i);
    }

  create_pidfile ();

  /* Register a signal handler to reap children after they terminated */
  install_sighandler ();

  /* Create the RPC server */
  pmap_unset (YPPASSWDPROG, YPPASSWDVERS);

  if (my_port >= 0)
    {
      struct sockaddr_in s_in;
      int result;

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
          log_msg ("rpc.yppasswdd: can not bind UDP: %s ", strerror (errno));
          exit (1);
        }
    }
  else
    my_socket = RPC_ANYSOCK;

  transp = svcudp_create (my_socket);
  if (transp == NULL)
    {
      log_msg ("cannot create udp service.\n");
      exit (1);
    }
  if (!svc_register (transp, YPPASSWDPROG, YPPASSWDVERS, yppasswdprog_1,
		     IPPROTO_UDP))
    {
      log_msg ("unable to register yppaswdd udp service.\n");
      exit (1);
    }

  /* Run the server */
  svc_run ();
  log_msg ("svc_run returned\n");
  unlink (_YPPASSWDD_PIDFILE);
  return 1;
}
