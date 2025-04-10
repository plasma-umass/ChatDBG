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

#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <rpc/rpc.h>
#ifdef NEED_SVCSOC_H
#include <rpc/svc_soc.h>
#endif
#include "log_msg.h"
#include "ypxfrd.h"
#include "access.h"

static int file = 0;

/* Read a block from a file and call xdr_xfr for sending */
static bool_t
xdr_ypxfrd_xfr (register XDR *xdrs, xfr *objp)
{
  static unsigned char buf[XFRBLOCKSIZE];
  long len;

  while (1)
    {
      if ((len = read (file, &buf, XFRBLOCKSIZE)) != -1)
	{
	  /* We could send the next data block */
	  objp->ok = TRUE;
	  objp->xfr_u.xfrblock_buf.xfrblock_buf_len = len;
	  objp->xfr_u.xfrblock_buf.xfrblock_buf_val = (char *) &buf;
	}
      else
	{
	  /* We could not read the next data block, so send an
	     error status */
	  objp->ok = FALSE;
	  objp->xfr_u.xfrstat = XFR_READ_ERR;
	  log_msg ("read error: %s", strerror (errno));
	}

      /* call the next function for sending the data or status */
      if (!xdr_xfr (xdrs, objp))
	return FALSE;

      /* We have send the status report successfully, so exit the function
	 with an OK message. This does not mean that there were no errors
	 when reading the map, it only means that there were no errors
	 while sending data or the status. */
      if (objp->ok == FALSE)
	return TRUE;

      /* Now, if we have send the last packet successfully, we could
	 send the XFR_DONE message and quit the function with the
	 return code of the xdr_xfr function */
      if (objp->xfr_u.xfrblock_buf.xfrblock_buf_len < XFRBLOCKSIZE)
	{
	  /* This was the last packet, send the XFR_DONE message */
	  objp->ok = FALSE;
	  objp->xfr_u.xfrstat = XFR_DONE;
	  return (xdr_xfr (xdrs, objp));
	}
    }
}

struct xfr *
ypxfrd_getmap_1_svc (ypxfr_mapname *argp, struct svc_req *rqstp)
{
  static struct xfr result;
  char buf[MAXPATHLEN];
  struct sockaddr_in *rqhost;
  int valid;

  if (debug_flag)
    {
      rqhost = svc_getcaller (rqstp->rq_xprt);
      log_msg ("ypproc_null() [From: %s:%d]",
	      inet_ntoa (rqhost->sin_addr),
	      ntohs (rqhost->sin_port));
      log_msg ("\txfrdomain=%s", argp->xfrdomain);
      log_msg ("\txfrmap=%s", argp->xfrmap);
      log_msg ("\txfrmap_filename=%s", argp->xfrmap_filename);
    }

  result.ok = FALSE;
  result.xfr_u.xfrstat = XFR_DENIED;

  if ((valid = is_valid (rqstp, argp->xfrmap, argp->xfrdomain)) < 1)
    {
      if (valid == 0)
	{
	  if (debug_flag)
	    log_msg ("\t-> Ignored (not a valid source host)");
	}
      else
	{
	  if (debug_flag)
	    log_msg ("\t-> Ignored (not a valid domain)");
	}
      return (&result);
    }

#if defined(HAVE_LIBGDBM)
#if SIZEOF_LONG == 8
  if ((argp->xfr_db_type != XFR_DB_GNU_GDBM64) &&
      (argp->xfr_db_type != XFR_DB_ANY))
#else
  if ((argp->xfr_db_type != XFR_DB_GNU_GDBM) &&
      (argp->xfr_db_type != XFR_DB_ANY))
#endif
#elif defined (HAVE_LIBNDBM)
#if defined(__sun__) || defined(sun)
    if ((argp->xfr_db_type != XFR_DB_NDBM) &&
	(argp->xfr_db_type != XFR_DB_ANY))
#else
    if ((argp->xfr_db_type != XFR_DB_BSD_NDBM) &&
	(argp->xfr_db_type != XFR_DB_ANY))
#endif /* sun */
#else
  if (argp->xfr_db_type != XFR_DB_ANY)
#endif
    {
      result.xfr_u.xfrstat = XFR_DB_TYPE_MISMATCH;
      return (&result);
    }

#if defined(WORDS_BIGENDIAN)
  if ((argp->xfr_byte_order != XFR_ENDIAN_BIG) &&
      (argp->xfr_byte_order != XFR_ENDIAN_ANY))
#else
  if ((argp->xfr_byte_order != XFR_ENDIAN_LITTLE) &&
      (argp->xfr_byte_order != XFR_ENDIAN_ANY))
#endif
    {
      result.xfr_u.xfrstat = XFR_DB_ENDIAN_MISMATCH;
      return &result;
    }

  /* check, if the xfrmap and xfrmap_filename means the same map,
     not that some bad boys tell us that they will have the mail.aliases map,
     but put "../../../etc/shadow" in the xfrmap_filename. */
  if (strchr (argp->xfrmap_filename, '/') != NULL)
    {
      /* We don't have files in other directorys */
      result.xfr_u.xfrstat = XFR_NOFILE;
      return &result;
    }

  if (strncmp (argp->xfrmap, argp->xfrmap_filename, strlen (argp->xfrmap))
      != 0)
    return &result;

  if (strlen (argp->xfrdomain) + strlen (argp->xfrmap_filename) + 2
      < sizeof (buf))
    sprintf (buf, "%s/%s", argp->xfrdomain, argp->xfrmap_filename);
  else
    {
      log_msg ("Buffer overflow! [%s|%d]", __FILE__, __LINE__);
      result.xfr_u.xfrstat = XFR_NOFILE;
      return &result;
    }

  if (access ((char *) &buf, R_OK) == -1)
    {
      result.xfr_u.xfrstat = XFR_ACCESS;
      return &result;
    }

  if ((file = open ((char *) &buf, O_RDONLY)) == -1)
    {
      result.xfr_u.xfrstat = XFR_READ_ERR;
      return (&result);
    }

  /* Start with sending the database file */
  svc_sendreply (rqstp->rq_xprt, (xdrproc_t) xdr_ypxfrd_xfr, (char *) &result);

  close (file);

  return NULL;
}
