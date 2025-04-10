/* Copyright (c) 2000, 2001  Thorsten Kukuk
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

#include <unistd.h>
#include <syslog.h>
#include <getopt.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "log_msg.h"
#include "yp.h"
#include "ypxfr.h"
#include "ypxfrd.h"
#include <rpcsvc/ypclnt.h>

#if defined(HAVE_LIBGDBM)
#include <gdbm.h>

#define ypdb_store gdbm_store
#define YPDB_REPLACE GDBM_REPLACE
#define ypdb_close gdbm_close
#define ypdb_fetch gdbm_fetch
static GDBM_FILE dbm;
#elif defined (HAVE_NDBM)
#include <ndbm.h>

#define ypdb_store dbm_store
#define YPDB_REPLACE DBM_REPLACE
#define ypdb_close dbm_close
#define ypdb_fetch dbm_fetch
static DBM *dbm;
#endif

#ifndef YPMAPDIR
#define YPMAPDIR "/var/yp"
#endif

static char *path_ypdb = YPMAPDIR;

static void
Usage (int exit_code)
{
  fprintf (stderr, "usage: ypxfr [-f] [-c] [-d target domain] [-h source host] [-s source domain]\n");
  fprintf (stderr, "             [-C taskid program-number ipaddr port] [-p yp_path] mapname ...\n");
  fprintf (stderr, "       ypxfr --version\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "where\n");
  fprintf (stderr, "\t-f forces transfer even if the master's copy is not newer.\n");
  fprintf (stderr, "\thost may be either a name or an internet\n");
  fprintf (stderr, "\t     address of form ww.xx.yy.zz\n");
  fprintf (stderr, "\t-c inhibits sending a \"Clear map\" message to the local ypserv.\n");
  fprintf (stderr, "\t-C is used by ypserv to pass callback information.\n");
  exit (exit_code);
}


static char *
ypxfr_err_string (enum ypxfrstat error)
{
  switch (error)
    {
    case YPXFR_SUCC:
      return "Success";
    case YPXFR_AGE:
      return "Master's version not newer";
    case YPXFR_NOMAP:
      return "Can't find server for map";
    case YPXFR_NODOM:
      return "Domain not supported";
    case YPXFR_RSRC:
      return "Local resource alloc failure";
    case YPXFR_RPC:
      return "RPC failure talking to server";
    case YPXFR_MADDR:
      return "Can't get master address";
    case YPXFR_YPERR:
      return "YP server/map db error";
    case YPXFR_BADARGS:
      return "Request arguments bad";
    case YPXFR_DBM:
      return "Local dbm operation failed";
    case YPXFR_FILE:
      return "Local file I/O operation failed";
    case YPXFR_SKEW:
      return "Map version skew during transfer";
    case YPXFR_CLEAR:
      return "Can't send \"Clear\" req to local ypserv";
    case YPXFR_FORCE:
      return "No local order number in map  use -f flag.";
    case YPXFR_XFRERR:
      return "ypxfr error";
    case YPXFR_REFUSED:
      return "Transfer request refused by ypserv";
    }
  return "Unknown Error, should not happen";
}

static int ypxfrd_file = 0;

static bool_t
xdr_ypxfr_xfr (XDR *xdrs, xfr *objp)
{
  while (1)
    {
      if (!xdr_xfr (xdrs, objp))
        return (FALSE);
      if (objp->ok == TRUE)
        {
          if (write (ypxfrd_file, objp->xfr_u.xfrblock_buf.xfrblock_buf_val,
                     objp->xfr_u.xfrblock_buf.xfrblock_buf_len) == -1)
            {
	      log_msg ("write failed: %s", strerror (errno));
              return FALSE;
            }
        }
      xdr_free ((xdrproc_t) xdr_xfr, (char *) objp);
      if (objp->ok == FALSE)
        {
          switch (objp->xfr_u.xfrstat)
            {
            case XFR_DONE:
              return TRUE;
              break;
            case XFR_DENIED:
              log_msg ("access to map denied by rpc.ypxfrd");
              return FALSE;
              break;
            case XFR_NOFILE:
              log_msg ("reqested map does not exist");
              return FALSE;
              break;
            case XFR_ACCESS:
              log_msg ("rpc.ypxfrd couldn't access the map");
              return FALSE;
              break;
            case XFR_BADDB:
              log_msg ("file is not a database");
              return FALSE;
              break;
            case XFR_READ_OK:
	      if (debug_flag)
		log_msg ("block read successfully");
              return TRUE;
              break;
            case XFR_READ_ERR:
              log_msg ("got read error from rpc.ypxfrd");
              return FALSE;
              break;
            case XFR_DB_ENDIAN_MISMATCH:
	      log_msg ("rpc.ypxfrd databases have the wrong endian");
              return FALSE;
              break;
            case XFR_DB_TYPE_MISMATCH:
              log_msg ("rpc.ypxfrd doesn't support the needed database type");
              return FALSE;
              break;
            default:
              log_msg ("got unknown status from rpc.ypxfrd");
              return FALSE;
              break;
            }
        }
    }
}

int
ypxfrd_transfer (char *host, char *map, char *domain, char *tmpname)
{
  CLIENT *clnt;
  struct ypxfr_mapname req;
  struct xfr resp;
  struct timeval timeout = {25, 0};

  if (debug_flag)
    fprintf (stderr, "Trying ypxfrd ...");

  if (!getrpcport (host, YPXFRD_FREEBSD_PROG, YPXFRD_FREEBSD_VERS,
                   IPPROTO_TCP))
    {
      if (debug_flag)
        log_msg (" not running\n");
      return 1;
    }

  req.xfrmap = map;
  req.xfrdomain = domain;
  req.xfrmap_filename = map;
#if defined(HAVE_LIBGDBM)
#if SIZEOF_LONG == 8
  req.xfr_db_type = XFR_DB_GNU_GDBM64;
#else
  req.xfr_db_type = XFR_DB_GNU_GDBM;
#endif
#if defined(WORDS_BIGENDIAN)
  req.xfr_byte_order = XFR_ENDIAN_BIG;
#else
  req.xfr_byte_order = XFR_ENDIAN_LITTLE;
#endif
#elif defined (HAVE_NDBM)
#if defined(__sun__) || defined (sun)
  req.xfr_db_type = XFR_DB_NDBM;
#if defined(WORDS_BIGENDIAN)
  req.xfr_byte_order = XFR_ENDIAN_BIG;
#else
  req.xfr_byte_order = XFR_ENDIAN_LITTLE;
#endif
#else
  req.xfr_db_type = XFR_DB_BSD_NDBM;
  req.xfr_byte_order = XFR_ENDIAN_ANY;
#endif
#endif
  memset (&resp, 0, sizeof (resp));

  if ((clnt = clnt_create (host, YPXFRD_FREEBSD_PROG,
                           YPXFRD_FREEBSD_VERS, "tcp")) == NULL)
    goto error;

  if ((ypxfrd_file = open (tmpname, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
    {
      clnt_destroy (clnt);
      log_msg ("couldn't open %s: %s", tmpname, strerror (errno));
      goto error;
    }

  if (clnt_call (clnt, YPXFRD_GETMAP, (xdrproc_t) xdr_ypxfr_mapname,
                 (caddr_t) &req, (xdrproc_t) xdr_ypxfr_xfr,
                 (caddr_t) &resp, timeout) != RPC_SUCCESS)
    {
      log_msg ("%s", clnt_sperror (clnt, "call to rpc.ypxfrd failed"));
      unlink (tmpname);
      clnt_destroy (clnt);
      close (ypxfrd_file);
      goto error;
    }

  clnt_destroy (clnt);
  close (ypxfrd_file);

  if (debug_flag)
    log_msg (" success\n");

  return 0;

error:
  if (debug_flag)
    log_msg (" (failed, fallback to enumeration)\n");
  return 1;
}

/* NetBSD has a different prototype in struct ypall_callback */
#if defined(__NetBSD__)
static int
ypxfr_foreach (u_long status, char *key, int keylen,
               char *val, int vallen, void *data __attribute__ ((unused)))
#else
static int
ypxfr_foreach (int status, char *key, int keylen,
               char *val, int vallen, char *data __attribute__ ((unused)))
#endif
{
  datum outKey, outData;

  if (debug_flag > 1)
    {
      if (keylen != 0 && vallen != 0)
        log_msg ("ypxfr_foreach: key=%.*s, val=%.*s", keylen, key,
		 vallen, val);
      else
        log_msg ("ypxfr_foreach: empty key/value pair!");
    }

  if (status == YP_NOMORE)
    return 0;

  if (status != YP_TRUE)
    {
      int s = ypprot_err (status);
      log_msg ("%s", yperr_string (s));
      return 1;
    }

  if (keylen != 0)
    {
      /* Hack for broken maps: If keylen/vallen includes the trailing 0,
         decrement it. */
      if (keylen > 0 && key[keylen - 1] == '\0')
        --keylen;
      if (vallen > 0 && val[vallen - 1] == '\0')
        --vallen;
      /* Now save it */
      outKey.dptr = keylen ? key : "";
      outKey.dsize = keylen;
      outData.dptr = vallen ? val : "";
      outData.dsize = vallen;
      if (ypdb_store (dbm, outKey, outData, YPDB_REPLACE) != 0)
        return 1;
    }

  return 0;
}

extern struct ypall_callback *xdr_ypall_callback;

static enum ypxfrstat
ypxfr (char *map, char *source_host, char *source_domain, char *target_domain,
       int noclear, int force)
{
  struct ypall_callback callback;
  char dbName_orig[MAXPATHLEN + 1];
  char dbName_temp[MAXPATHLEN + 1];
  char *master_host = NULL;
  struct ypreq_key req_key;
  struct ypresp_val resp_val;
  struct ypresp_all resp_all;
  datum outKey, outData;
  struct timeval TIMEOUT = {25, 0};
  CLIENT *clnt_udp;
  struct sockaddr_in sockaddr, sockaddr_udp;
  struct ypresp_order resp_order;
  struct ypreq_nokey req_nokey;
  uint32_t masterOrderNum;
  struct hostent *h;
  int sock, result;

  /* Name of the map file */
  if (strlen (path_ypdb) + strlen (target_domain) + strlen (map) + 3 < MAXPATHLEN)
    sprintf (dbName_orig, "%s/%s/%s", path_ypdb, target_domain, map);
  else
    {
      log_msg ("ERROR: Path to long: %s/%s/%s", path_ypdb, target_domain, map);
      return YPXFR_RSRC;
    }

  /* Name of the temporary map file */
  if (strlen (path_ypdb) + strlen (target_domain) + strlen (map) + 4 < MAXPATHLEN)
    sprintf (dbName_temp, "%s/%s/%s~", path_ypdb, target_domain, map);
  else
    {
      log_msg ("ERROR: Path to long: %s/%s/%s~", path_ypdb, target_domain, map);
      return YPXFR_RSRC;
    }

  /* Build a connection to the host with the master map, not to
     the nearest ypserv, because this map could be out of date. */
  memset (&sockaddr, '\0', sizeof (sockaddr));
  sockaddr.sin_family = AF_INET;
  if (source_host)
    {
      h = gethostbyname (source_host);
      if (!h)
	return YPXFR_RSRC;
    }
  else
    {
      /* Get the Master hostname for the map, if no explicit
	 sourcename is given. */
      char *master_name = NULL;

      if (yp_master (source_domain, map, &master_name))
        return YPXFR_MADDR;
      h = gethostbyname (master_name);
      if (!h)
	return YPXFR_RSRC;
    }
  memcpy (&sockaddr.sin_addr, h->h_addr, sizeof sockaddr.sin_addr);
  memcpy (&sockaddr_udp, &sockaddr, sizeof (sockaddr));
  master_host = alloca (strlen (h->h_name) + 1);
  strcpy (master_host, h->h_name);

  /* Create a udp socket to the server */
  sock = RPC_ANYSOCK;
  clnt_udp = clntudp_create (&sockaddr_udp, YPPROG, YPVERS, TIMEOUT, &sock);
  if (clnt_udp == NULL)
    {
      log_msg (clnt_spcreateerror ("YPXFR"));
      return YPXFR_RPC;
    }

  /* We cannot use the libc functions since we don't know which host
     they use. So query the host we must use to get the order number
     for the map on the master host. */
  req_nokey.domain = source_domain;
  req_nokey.map = map;
  if (ypproc_order_2 (&req_nokey, &resp_order, clnt_udp) != RPC_SUCCESS)
    {
      log_msg (clnt_sperror (clnt_udp, "masterOrderNum"));
      masterOrderNum = time (NULL); /* We set it to the current time.
                                       So a new map will be always newer. */
    }
  else if (resp_order.stat != YP_TRUE)
    {
      switch (resp_order.stat)
	{
	case YP_NOMAP:
	  return YPXFR_NOMAP;
	case YP_NODOM:
	  return YPXFR_NODOM;
	case YP_BADDB:
	  return YPXFR_DBM;
	case YP_YPERR:
	  return YPXFR_YPERR;
	case YP_BADARGS:
	  return YPXFR_BADARGS;
	default:
	  log_msg ("ERROR: not expected value: %s", resp_order.stat);
	  return YPXFR_XFRERR;
	}
    }
  else
    {
      masterOrderNum = resp_order.ordernum;
      xdr_free ((xdrproc_t) xdr_ypresp_order, (char *) &resp_order);
    }

  /* If we doesn't force the map, look, if the new map is really newer */
  if (!force)
    {
      uint32_t localOrderNum = 0;
      datum inKey, inVal;

#if defined(HAVE_LIBGDBM)
      dbm = gdbm_open (dbName_orig, 0, GDBM_READER, 0600, NULL);
#elif defined(HAVE_NDBM)
      dbm = dbm_open (dbName_orig, O_CREAT|O_RDWR, 0600);
#endif
      if (dbm == NULL)
        {
	  if (debug_flag)
	    log_msg ("Cannot open old %s - ignored.", dbName_orig);
          localOrderNum = 0;
        }
      else
        {
          inKey.dptr = "YP_LAST_MODIFIED";
          inKey.dsize = strlen (inKey.dptr);
          inVal = ypdb_fetch (dbm, inKey);
          if (inVal.dptr)
            {
              int i;
              char *d = inVal.dptr;

              for (i = 0; i < inVal.dsize; i++, d++)
                {
                  if (!isdigit (*d))
                    {
		      log_msg ("YP_LAST_MODIFIED entry \"%.*s\" in \"%s\" is not valid",
			       inVal.dsize, inVal.dptr, dbName_orig);
		      clnt_destroy (clnt_udp);
                      ypdb_close (dbm);
                      return YPXFR_SKEW;
                    }
                }
	      d = alloca (inVal.dsize + 2);
	      strncpy (d, inVal.dptr, inVal.dsize);
	      d[inVal.dsize] = '\0';
              localOrderNum = atoi (d);
            }
          ypdb_close (dbm);
        }

      if (debug_flag > 1)
        log_msg ("masterOrderNum=%d, localOrderNum=%d",
		 masterOrderNum, localOrderNum);

      if (localOrderNum >= masterOrderNum)
	{
	  if (debug_flag)
	    log_msg("Map on Master \"%s\" is not newer", master_host);
	  clnt_destroy (clnt_udp);
	  return YPXFR_AGE;
	}
    }

  /* Try to use ypxfrd for getting the new map. If it fails, use the old
     method. */
  if ((result = ypxfrd_transfer (master_host, map, target_domain, dbName_temp)) != 0)
    {
      /* No success with ypxfrd, get the map entry by entry */
      char orderNum[255];
      CLIENT *clnt_tcp;

#if defined(HAVE_LIBGDBM)
      dbm = gdbm_open (dbName_temp, 0, GDBM_NEWDB, 0600, NULL);
#elif defined(HAVE_NDBM)
      dbm = dbm_open (dbName_temp, O_CREAT|O_RDWR, 0600);
#endif
      if (dbm == NULL)
        {
          log_msg ("Cannot open %s", dbName_temp);
	  clnt_destroy (clnt_udp);
          return YPXFR_DBM;
        }

      outKey.dptr = "YP_MASTER_NAME";
      outKey.dsize = strlen (outKey.dptr);
      outData.dptr = master_host;
      outData.dsize = strlen (outData.dptr);
      if (ypdb_store (dbm, outKey, outData, YPDB_REPLACE) != 0)
        {
          ypdb_close (dbm);
          unlink (dbName_temp);
          return YPXFR_DBM;
        }
      snprintf (orderNum, sizeof (orderNum), "%d", masterOrderNum);
      outKey.dptr = "YP_LAST_MODIFIED";
      outKey.dsize = strlen (outKey.dptr);
      outData.dptr = orderNum;
      outData.dsize = strlen (outData.dptr);
      if (ypdb_store (dbm, outKey, outData, YPDB_REPLACE) != 0)
        {
          ypdb_close (dbm);
          unlink (dbName_temp);
          return YPXFR_DBM;
        }

      /* Get the YP_INTERDOMAIN field. This is needed from the SunOS ypserv.
	 We ignore this, since we have the "dns" option. But a Sun could be
	 a NIS slave server and request the map from us. */
      req_key.domain = source_domain;
      req_key.map = map;
      req_key.key.keydat_val = "YP_INTERDOMAIN";
      req_key.key.keydat_len = strlen ("YP_INTERDOMAIN");
      if (ypproc_match_2 (&req_key, &resp_val, clnt_udp) != RPC_SUCCESS)
        {
          clnt_perror (clnt_udp, "ypproc_match(YP_INTERDOMAIN)");
	  clnt_destroy (clnt_udp);
	  ypdb_close (dbm);
	  unlink (dbName_temp);
	  return YPXFR_RPC;
        }
      else
        {
          if (resp_val.stat == YP_TRUE)
            {
              outKey.dptr = "YP_INTERDOMAIN";
              outKey.dsize = strlen (outKey.dptr);
              outData.dptr = "";
              outData.dsize = 0;
              if (ypdb_store (dbm, outKey, outData, YPDB_REPLACE) != 0)
                {
		  clnt_destroy (clnt_udp);
                  ypdb_close (dbm);
                  unlink (dbName_temp);
                  return YPXFR_DBM;
                }
            }
          xdr_free ((xdrproc_t) xdr_ypresp_val, (char *) &resp_val);
        }

      /* Get the YP_SECURE field. */
      req_key.domain = source_domain;
      req_key.map = map;
      req_key.key.keydat_val = "YP_SECURE";
      req_key.key.keydat_len = strlen ("YP_SECURE");
      if (ypproc_match_2 (&req_key, &resp_val, clnt_udp) != RPC_SUCCESS)
        {
          clnt_perror (clnt_udp, "yproc_match");
	  clnt_destroy (clnt_udp);
	  ypdb_close (dbm);
	  unlink (dbName_temp);
	  return YPXFR_RPC;
        }
      else
        {
          if (resp_val.stat == YP_TRUE)
            {
              outKey.dptr = "YP_SECURE";
              outKey.dsize = strlen (outKey.dptr);
              outData.dptr = "";
              outData.dsize = 0;
              if (ypdb_store (dbm, outKey, outData, YPDB_REPLACE) != 0)
		{
		  clnt_destroy (clnt_udp);
		  ypdb_close (dbm);
                  unlink (dbName_temp);
                  return YPXFR_DBM;
                }
            }
          xdr_free ((xdrproc_t) xdr_ypresp_val, (char *) &resp_val);
        }

      /* We don't need clnt_udp any longer, give it free */
      clnt_destroy (clnt_udp);
      /* Create a tcp socket to the server */
      sock = RPC_ANYSOCK;
      clnt_tcp = clnttcp_create (&sockaddr, YPPROG, YPVERS, &sock, 0, 0);
      if (clnt_tcp == NULL)
	{
	  clnt_pcreateerror ("YPXFR");
	  return YPXFR_RPC;
	}

      callback.foreach = ypxfr_foreach;
      callback.data = NULL;
      {
        req_nokey.domain = source_domain;
        req_nokey.map = map;
        xdr_ypall_callback = &callback;
        if (ypproc_all_2 (&req_nokey, &resp_all, clnt_tcp) != RPC_SUCCESS)
          {
            clnt_perror (clnt_tcp, "ypall");
	    clnt_destroy (clnt_tcp);
	    ypdb_close (dbm);
	    return YPXFR_RPC;
          }
        else
          {
            switch (resp_all.ypresp_all_u.val.stat)
              {
              case YP_TRUE:
              case YP_NOMORE:
                result = 0;
                break;
              default:
                result = ypprot_err (resp_all.ypresp_all_u.val.stat);
              }
            clnt_freeres (clnt_tcp, (xdrproc_t) ypxfr_xdr_ypresp_all,
			  (caddr_t) &resp_all);
          }
      }

      clnt_destroy (clnt_tcp);
      ypdb_close (dbm);
    }

  if (result == 0)
    {
      rename (dbName_temp, dbName_orig);
    }
  else
    unlink(dbName_temp);

  if (!noclear) /* send the clear independing if an error occurs before */
    {
      char in = 0, *out = NULL;
      int stat;

      if ((stat = callrpc ("localhost", YPPROG, YPVERS, YPPROC_CLEAR,
                           (xdrproc_t) xdr_void, &in,
                           (xdrproc_t) xdr_void, out)) != RPC_SUCCESS)
        {
          log_msg ("failed to send 'clear' to local ypserv: %s",
		   clnt_sperrno ((enum clnt_stat) stat));
          return YPXFR_CLEAR;
        }
    }

  return result == 0 ? YPXFR_SUCC : YPXFR_YPERR;
}

int
main (int argc, char **argv)
{
  char *source_host = NULL, *target_domain = NULL, *source_domain = NULL;
  struct in_addr remote_addr;
  unsigned int transid = 0;
  unsigned short int remote_port = 0;
  uint32_t program_number = 0;
  int force = 0;
  int noclear = 0;

  if (argc < 2)
    Usage (1);

  if (!isatty (fileno (stderr)))
    openlog ("ypxfr", LOG_PID, LOG_DAEMON);
  else
    debug_flag = 1;

  while (1)
    {
      int c;
      int option_index = 0;
      static struct option long_options[] =
      {
	{"version", no_argument, NULL, '\255'},
	{"debug", no_argument, NULL, '\254'},
	{"help", no_argument, NULL, 'u'},
	{"usage", no_argument, NULL, 'u'},
	{"path", required_argument, NULL, 'p'},
	{NULL, 0, NULL, '\0'}
      };

      c = getopt_long (argc, argv, "ufcd:h:p:s:C:S", long_options, &option_index);
      if (c == EOF)
	break;
      switch (c)
	{
	case 'p':
	  path_ypdb = optarg;
	  if (debug_flag)
	    log_msg ("Using database directory: %s", path_ypdb);
	  break;
	case 'f':
	  force++;
	  break;
	case 'c':
	  noclear++;
	  break;
	case 'd':
	  if (strchr (optarg, '/'))
	    {
	      Usage (1);
	      break;
	    }
	  target_domain = optarg;
	  break;
	case 'h':
	  source_host = optarg;
	  break;
	case 's':
	  if (strchr (optarg, '/'))
	    {
	      Usage (1);
	      break;
	    }
	  source_domain = optarg;
	  break;
	case 'C':
	  if (optind + 3 > argc)
	    {
	      Usage (1);
	      break;
	    }
	  transid = atoi (optarg);
	  program_number = atoi (argv[optind++]);
	  remote_addr.s_addr = inet_addr (argv[optind++]);
	  remote_port = atoi (argv[optind++]);
	  break;
	case 'u':
	  Usage (0);
	  break;
	case '\254':
	  debug_flag = 2;
	  break;
	case '\255':
	  log_msg ("ypxfr (%s) %s", PACKAGE, VERSION);
	  return 0;
	default:
	  Usage (1);
	  break;
	}
    }
  argc -= optind;
  argv += optind;

  if (target_domain == NULL)
    yp_get_default_domain (&target_domain);

  if (source_domain == NULL)
    source_domain = target_domain;

  for (; *argv; argv++)
    {
      enum ypxfrstat res;

      /* Start the map transfer */
      if ((res = ypxfr (*argv, source_host, source_domain, target_domain,
			noclear, force)) != YPXFR_SUCC)
        {
          /* Don't syslog "Master's version not newer" as that is
	     the common case. */
          if (debug_flag || res != YPXFR_AGE)
            log_msg ("ypxfr: %s", ypxfr_err_string (res));
        }

      /* Now send the status to the yppush program, so it can display a
	 message for the sysop and do not timeout. */
      if (transid)
        {
          struct sockaddr_in addr;
          CLIENT *clnt;
          int s;
          ypresp_xfr resp;
          static struct timeval tv = {10, 0};

          memset (&addr, '\0', sizeof addr);
          addr.sin_addr = remote_addr;
          addr.sin_port = htons (remote_port);
          addr.sin_family = AF_INET;
          s = RPC_ANYSOCK;

          clnt = clntudp_create (&addr, program_number, 1, tv, &s);
          if (!clnt)
            {
              clnt_pcreateerror ("ypxfr_callback");
              continue;
            }
	  resp.transid = transid;
          resp.xfrstat = res;

          if (clnt_call (clnt, 1, (xdrproc_t) xdr_ypresp_xfr, (caddr_t) &resp,
                         (xdrproc_t) xdr_void, (caddr_t)&res, tv)
	      != RPC_SUCCESS)
            clnt_perror (clnt, "ypxfr_callback");

          clnt_destroy (clnt);

	}
    }

  return 0;
}
