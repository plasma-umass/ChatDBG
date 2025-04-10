/* Copyright (c) 1996, 1997, 1998, 1999, 2001 Thorsten Kukuk
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

/* ypxfrd - ypxfrd main routines.  */

#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <syslog.h>
#ifndef LOG_DAEMON
#include <sys/syslog.h>
#endif
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <rpc/rpc.h>
#ifdef NEED_SVCSOC_H
#include <rpc/svc_soc.h>
#endif
#include <rpc/pmap_clnt.h>
#include <getopt.h>
#include "ypxfrd.h"
#include "access.h"

#ifndef SA_RESTART
#define SA_RESTART 0
#endif

#include "log_msg.h"

extern void ypxfrd_freebsd_prog_1(struct svc_req *, SVCXPRT *);

int _rpcpmstart = 0;
int _rpcfdtype = 0;
int _rpcsvcdirty = 0;

#ifndef _RPCSVC_CLOSEDOWN
#define _RPCSVC_CLOSEDOWN       120
#endif

#ifndef YPMAPDIR
#define YPMAPDIR   "/var/yp"
#endif
char *path_ypdb = YPMAPDIR;

char *progname;

#if HAVE__RPC_DTABLESIZE
extern int _rpc_dtablesize(void);
#elif HAVE_GETDTABLESIZE
static int
_rpc_dtablesize()
{
  static int size;

  if (size == 0)
    {
      size = getdtablesize();
    }
  return (size);
}
#else

#include <sys/resource.h>

static int
_rpc_dtablesize()
{
  static int size = 0;
  struct rlimit rlb;

  if (size == 0)
    {
      if (getrlimit(RLIMIT_NOFILE, &rlb) >= 0)
	size = rlb.rlim_cur;
    }

  return size;
}
#endif

/*
** Needed, if we start rpc.ypxfrd from inetd
*/
static void
closedown (int sig)
{
  signal(sig, closedown);
  if (_rpcsvcdirty == 0)
    {
      static int size;
      int i, openfd;

      if (_rpcfdtype == SOCK_DGRAM)
	exit(0);
      if (size == 0)
	size = _rpc_dtablesize();

      for (i = 0, openfd = 0; i < size && openfd < 2; ++i)
	if (FD_ISSET(i, &svc_fdset))
	  openfd++;
      if (openfd <= 1)
	exit(0);
    }
  alarm(_RPCSVC_CLOSEDOWN);
}

/* Clean up after child processes signal their termination.  */
static void
sig_child (int sig __attribute__ ((unused)))
{
   int st;

   /* Clear all childs.  */
   while (waitpid(-1, &st, WNOHANG) > 0)
     ;
}

/* Clean up if we quit the program.  */
static void
sig_quit (int sig __attribute__ ((unused)))
{
  pmap_unset (YPXFRD_FREEBSD_PROG, YPXFRD_FREEBSD_VERS);
  exit (0);
}

/*
** Reload securenets and config file
*/
static void
sig_hup (int sig __attribute__ ((unused)))
{
  load_securenets();
  load_config();
}

static void
Usage (int exitcode)
{
  fputs ("usage: rpc.ypxfrd [--debug] [-d path] [-p port]\n", stderr);
  fputs ("       rpc.ypxfrd --version\n", stderr);

  exit (exitcode);
}

int
main (int argc, char **argv)
{
  SVCXPRT *main_transp;
  int my_port = -1;
  int my_socket;
  struct sockaddr_in socket_address;
  int result;
  struct sigaction sa;
  int socket_size;

  progname = strrchr (argv[0], '/');
  if (progname == (char *) NULL)
    progname = argv[0];
  else
    progname++;

  openlog(progname, LOG_PID, LOG_DAEMON);

  while(1)
    {
      int c;
      int option_index = 0;
      static struct option long_options[] =
      {
        {"version", no_argument, NULL, '\255'},
        {"debug", no_argument, NULL, '\254'},
        {"port", required_argument, NULL, 'p'},
	{"path", required_argument, NULL, 'd'},
	{"dir", required_argument, NULL, 'd'},
        {"usage", no_argument, NULL, 'u'},
	{"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, '\0'}
      };

      c=getopt_long(argc, argv, "p:d:uh",long_options, &option_index);
      if (c==EOF) break;
      switch (c)
        {
	case '\255':
	  debug_flag = 1;
	  log_msg("rpc.ypxfrd (%s) %s\n", PACKAGE, VERSION);
	  exit(0);
	case '\254':
	  debug_flag++;
	  break;
	case 'd':
	  path_ypdb = optarg;
	  if (debug_flag)
	    log_msg("Using database directory: %s\n", path_ypdb);
	  break;
	case 'p':
	  my_port = atoi(optarg);
	  if (debug_flag)
	    log_msg("Using port %d\n", my_port);
	  break;
	case 'u':
        case 'h':
          Usage(0);
          break;
        case '?':
          Usage(1);
          break;
        }
    }

  argc-=optind;
  argv+=optind;

  if (debug_flag)
    log_msg("[Welcome to the rpc.ypxfrd Daemon, version %s]\n", VERSION);
  else
    if(!_rpcpmstart)
      {
	int i;

	if ((i = fork()) > 0)
	  exit(0);

	if (i < 0)
	  {
	    log_msg ("Cannot fork: %s\n", strerror (errno));
	    exit (-1);
	  }

	if (setsid() == -1)
	  {
	    log_msg ("Cannot setsid: %s\n", strerror (errno));
	    exit (-1);
	  }

	if ((i = fork()) > 0)
	  exit(0);

	if (i < 0)
	  {
	    log_msg ("Cannot fork: %s\n", strerror (errno));
	    exit (-1);
	  }

	for (i = 0; i < getdtablesize(); ++i)
	  close(i);
	errno = 0;

	umask(0);
	i = open("/dev/null", O_RDWR);
	dup(i);
	dup(i);
      }

  /* Change current directory to database location */
  if (chdir(path_ypdb) < 0)
    {
      log_msg("%s: chdir: %", argv[0], strerror(errno));
      exit(1);
    }

  load_securenets();
  load_config();

  /*
   * Ignore SIGPIPEs. They can hurt us if someone does a ypcat
   * and then hits CTRL-C before it terminates.
   */
  sigaction(SIGPIPE, NULL, &sa);
  sa.sa_handler = SIG_IGN;
#if !defined(sun) || (defined(sun) && defined(__svr4__))
  sa.sa_flags |= SA_RESTART;
  /*
   * The opposite to SA_ONESHOT, do  not  restore
   * the  signal  action.  This provides behavior
   * compatible with BSD signal semantics.
   */
#endif
  sigemptyset(&sa.sa_mask);
  sigaction(SIGPIPE, &sa, NULL);
  /*
   * Clear up if child exists
   */
  sigaction(SIGCHLD, NULL, &sa);
#if !defined(sun) || (defined(sun) && defined(__svr4__))
  sa.sa_flags |= SA_RESTART;
#endif
  sa.sa_handler = sig_child;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGCHLD, &sa, NULL);
  /*
   * If program quits, give ports free.
   */
  sigaction(SIGTERM, NULL, &sa);
#if !defined(sun) || (defined(sun) && defined(__svr4__))
  sa.sa_flags |= SA_RESTART;
#endif
  sa.sa_handler = sig_quit;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGTERM, &sa, NULL);

  sigaction(SIGINT, NULL, &sa);
#if !defined(sun) || (defined(sun) && defined(__svr4__))
  sa.sa_flags |= SA_RESTART;
#endif
  sa.sa_handler = sig_quit;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);

  /*
   * If we get a SIGHUP, reload the securenets and config file.
   */
  sigaction(SIGHUP, NULL, &sa);
#if !defined(sun) || (defined(sun) && defined(__svr4__))
  sa.sa_flags |= SA_RESTART;
#endif
  sa.sa_handler = sig_hup;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGHUP, &sa, NULL);

  pmap_unset(YPXFRD_FREEBSD_PROG, YPXFRD_FREEBSD_VERS);

  socket_size = sizeof(socket_address);
  _rpcfdtype = 0;
  if (getsockname(0, (struct sockaddr *)&socket_address, &socket_size) == 0)
    {
      int  int_size = sizeof (int);
      if (socket_address.sin_family != AF_INET)
	return 1;
      if (getsockopt(0, SOL_SOCKET, SO_TYPE, (void*)&_rpcfdtype,
		     &int_size) == -1)
	return 1;
      _rpcpmstart = 1;
      my_socket = 0;
    }
  else
    my_socket = RPC_ANYSOCK;

  if ((_rpcfdtype == 0) || (_rpcfdtype == SOCK_DGRAM))
    {
      if (_rpcfdtype == 0 && my_port > 0)
	{
	  my_socket = socket (AF_INET, SOCK_DGRAM, 0);
	  if (my_socket < 0)
	    {
	      log_msg("can not create UDP: %s",strerror(errno));
	      return 1;
	    }

	  memset((char *) &socket_address, 0, sizeof(socket_address));
	  socket_address.sin_family = AF_INET;
	  socket_address.sin_addr.s_addr = htonl (INADDR_ANY);
	  socket_address.sin_port = htons (my_port);

	  result = bind (my_socket, (struct sockaddr *) &socket_address,
			 sizeof (socket_address));
	  if (result < 0)
	    {
	      log_msg("%s: can not bind UDP: %s ", progname,strerror(errno));
	      return 1;
	    }
	}

      main_transp = svcudp_create(my_socket);
      if (main_transp == NULL)
	{
	  log_msg("cannot create udp service.");
	  return 1;
	}
      if (!svc_register(main_transp, YPXFRD_FREEBSD_PROG, YPXFRD_FREEBSD_VERS,
			ypxfrd_freebsd_prog_1, IPPROTO_UDP))
	{
	  log_msg("unable to register (YPXFRD_FREEBSD_PROG, YPXFRD_FREEBSD_VERS, udp).");
	  return 1;
	}
    }

  if ((_rpcfdtype == 0) || (_rpcfdtype == SOCK_STREAM))
    {
      if (_rpcfdtype == 0 && my_port >= 0)
	{
	  my_socket = socket (AF_INET, SOCK_STREAM, 0);
	  if (my_socket < 0)
	    {
	      log_msg ("%s: can not create TCP ",progname);
	      return 1;
	    }

	  memset((char *) &socket_address, 0, sizeof(socket_address));
	  socket_address.sin_family = AF_INET;
	  socket_address.sin_addr.s_addr = htonl (INADDR_ANY);
	  socket_address.sin_port = htons (my_port);

	  result = bind (my_socket, (struct sockaddr *) &socket_address,
			 sizeof (socket_address));
	  if (result < 0)
	    {
	      log_msg("%s: can not bind TCP ",progname);
	      return 1;
	    }
	}
      main_transp = svctcp_create(my_socket, 0, 0);
      if (main_transp == NULL)
	{
	  log_msg("%s: cannot create tcp service\n", progname);
	  exit(1);
	}
      if (!svc_register(main_transp, YPXFRD_FREEBSD_PROG, YPXFRD_FREEBSD_VERS,
			ypxfrd_freebsd_prog_1, IPPROTO_TCP))
	{
	  log_msg("%s: unable to register (YPXFRD_FREEBSD_PROG, YPXFRD_FREEBSD_VERS, tcp)\n",
		 progname);
	  exit(1);
	}
    }

  if (_rpcpmstart)
    {
      signal (SIGALRM, closedown);
      alarm (_RPCSVC_CLOSEDOWN);
    }

  svc_run();
  log_msg("svc_run returned");
  exit(1);
  /* NOTREACHED */
}
