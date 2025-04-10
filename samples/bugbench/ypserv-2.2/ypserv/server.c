/* Copyright (c) 2000, 2001  Thorsten Kukuk
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include "yp.h"
#include "yp_db.h"
#include "access.h"
#include "ypserv_conf.h"
#include "log_msg.h"

bool_t
ypproc_null_2_svc (void *argp __attribute__ ((unused)),
		   void *result __attribute__ ((unused)),
		   struct svc_req *rqstp)
{
  if (debug_flag)
    {
      struct sockaddr_in *rqhost = svc_getcaller (rqstp->rq_xprt);
      log_msg ("ypproc_null() [From: %s:%d]",
	       inet_ntoa (rqhost->sin_addr),
	       ntohs (rqhost->sin_port));
    }

  if (!is_valid (rqstp, NULL, NULL))
    return FALSE;

  return TRUE;
}


bool_t
ypproc_domain_2_svc (domainname *argp, bool_t *result,
		     struct svc_req *rqstp)
{
  if (debug_flag)
    {
      struct sockaddr_in *rqhost = svc_getcaller (rqstp->rq_xprt);
      log_msg ("ypproc_domain(\"%s\") [From: %s:%d]",
	       *argp, inet_ntoa (rqhost->sin_addr),
	       ntohs (rqhost->sin_port));
    }

  switch (is_valid (rqstp, NULL, *argp))
    {
    case -3:
    case -2: /* -2 should not happen */
      *result = FALSE;
      break;
    case -1:
      if (debug_flag)
        log_msg ("\t-> Ignored (not a valid source host)");
      *result = FALSE;
      break;
    default:
      *result = TRUE;
    }

  if (debug_flag)
    log_msg ("\t-> %s.",
	     (*result == TRUE ? "Ok" : "Not served by us"));

  return TRUE;
}


bool_t
ypproc_domain_nonack_2_svc (domainname *argp, bool_t *result,
			    struct svc_req *rqstp)
{
  if (debug_flag)
    {
      struct sockaddr_in *rqhost = svc_getcaller (rqstp->rq_xprt);
      log_msg ("ypproc_domain_nonack(\"%s\") [From: %s:%d]",
	       *argp, inet_ntoa (rqhost->sin_addr),
	       ntohs (rqhost->sin_port));
    }

  switch (is_valid (rqstp, NULL, *argp))
    {
    case -3:
      if (debug_flag)
        log_msg ("\t-> Ignored (not a valid domain)");
      /* Bail out and don't return any RPC value */
      return FALSE;
    case -2: /* Should not happen */
      return FALSE;
    case -1:
      if (debug_flag)
        log_msg ("\t-> Ignored (not a valid source host)");
      return FALSE;
    default:
      *result = TRUE;
      break;
    }

  if (debug_flag)
    log_msg ("\t-> OK.");

  return TRUE;
}


bool_t
ypproc_match_2_svc (ypreq_key *argp, ypresp_val *result,
		    struct svc_req *rqstp)
{
  int valid;

  if (debug_flag)
    {
      struct sockaddr_in *rqhost = svc_getcaller (rqstp->rq_xprt);

      log_msg ("ypproc_match(): [From: %s:%d]",
	       inet_ntoa (rqhost->sin_addr), ntohs (rqhost->sin_port));

      log_msg ("\t\tdomainname = \"%s\"", argp->domain);
      log_msg ("\t\tmapname = \"%s\"", argp->map);
      log_msg ("\t\tkeydat = \"%.*s\"", (int) argp->key.keydat_len,
	       argp->key.keydat_val);
    }

  memset (result, 0, sizeof (ypresp_val));

  valid = is_valid (rqstp, argp->map, argp->domain);
  if (valid < 1)
    {
      switch (valid)
	{
	case -1:
	  if (debug_flag)
	    log_msg ("\t-> Ignored (not a valid source host)");
	  result->stat = YP_YPERR;
	  break;
	case -2:
	  if (debug_flag)
	    log_msg ("\t-> Ignored (not a valid map name)");
	  result->stat = YP_BADARGS;
	  break;
	case -3:
	  if (debug_flag)
	    log_msg ("\t-> Ignored (not a valid domain)");
	  result->stat = YP_NODOM;
	  break;
	}
      return TRUE;
    }

  if (argp->key.keydat_len == 0 || argp->key.keydat_val[0] == '\0')
    result->stat = YP_BADARGS;
  else
    {
      datum rdat, qdat;

      DB_FILE dbp = ypdb_open (argp->domain, argp->map);
      if (dbp == NULL)
        result->stat = YP_NOMAP;
      else
        {
          qdat.dsize = argp->key.keydat_len;
          qdat.dptr = argp->key.keydat_val;

          rdat = ypdb_fetch (dbp, qdat);

          if (rdat.dptr != NULL)
            {
              result->stat = YP_TRUE;
              result->val.valdat_len = rdat.dsize;
              result->val.valdat_val = rdat.dptr;
            }
          else
            result->stat = YP_NOKEY;

          ypdb_close (dbp);
        }
    }

  if (debug_flag)
    {
      if (result->stat == YP_TRUE)
        log_msg ("\t-> Value = \"%.*s\"",
		 (int) result->val.valdat_len, result->val.valdat_val);
      else
        log_msg ("\t-> Error #%d", result->stat);
    }

  return TRUE;
}


bool_t
ypproc_first_2_svc (ypreq_nokey *argp, ypresp_key_val *result,
		    struct svc_req *rqstp)
{
  DB_FILE dbp;
  int valid;

  if (debug_flag)
    {
      struct sockaddr_in *rqhost = svc_getcaller (rqstp->rq_xprt);
      log_msg ("ypproc_first(): [From: %s:%d]",
	       inet_ntoa (rqhost->sin_addr), ntohs (rqhost->sin_port));

      log_msg ("\tdomainname = \"%s\"", argp->domain);
      log_msg ("\tmapname = \"%s\"", argp->map);
    }

  memset (result, 0, sizeof (ypresp_key_val));

  valid = is_valid (rqstp, argp->map, argp->domain);
  if (valid < 1)
    {
      switch (valid)
	{
	case -1:
	  if (debug_flag)
	    log_msg ("\t-> Ignored (not a valid source host)");
	  result->stat = YP_YPERR;
	case -2:
          if (debug_flag)
            log_msg ("\t-> Ignored (not a valid map name)");
          result->stat = YP_BADARGS;
	  break;
	case -3:
          if (debug_flag)
            log_msg ("\t-> Ignored (not a valid domain)");
          result->stat = YP_NODOM;
	  break;
        }
      return TRUE;
    }


  dbp = ypdb_open (argp->domain, argp->map);
  if (dbp == NULL)
    result->stat = YP_NOMAP;
  else
    {
      datum dkey = ypdb_firstkey (dbp);

      while (dkey.dptr != NULL && dkey.dptr[0] == 'Y' &&
	     dkey.dptr[1] == 'P' && dkey.dptr[2] == '_')
	{
#if defined(HAVE_NDBM)
	  /* This is much more faster then ypdb_nextkey, but
	     it is terrible to port to other databases */
	  dkey = dbm_nextkey (dbp);
#else
	  datum tkey = dkey;
	  dkey = ypdb_nextkey (dbp, tkey);
	  ypdb_free (tkey.dptr);
#endif
	}

      if (dkey.dptr != NULL)
	{
	  datum dval = ypdb_fetch (dbp, dkey);
	  result->stat = YP_TRUE;

	  result->key.keydat_len = dkey.dsize;
	  result->key.keydat_val = dkey.dptr;

	  result->val.valdat_len = dval.dsize;
	  result->val.valdat_val = dval.dptr;
	}
      else
	result->stat = YP_NOKEY;
      ypdb_close (dbp);
    }

  if (debug_flag)
    {
      if (result->stat == YP_TRUE)
        log_msg ("\t-> Key = \"%.*s\", Value = \"%.*s\"",
		 (int) result->key.keydat_len, result->key.keydat_val,
		 (int) result->val.valdat_len, result->val.valdat_val);
      else if (result->stat == YP_NOMORE)
        log_msg ("\t-> No more entry's");
      else
        log_msg ("\t-> Error #%d", result->stat);
    }
  return TRUE;
}


bool_t
ypproc_next_2_svc (ypreq_key *argp, ypresp_key_val *result,
		   struct svc_req *rqstp)
{
  DB_FILE dbp;
  int valid;

  if (debug_flag)
    {
      struct sockaddr_in *rqhost = svc_getcaller (rqstp->rq_xprt);

      log_msg ("ypproc_next(): [From: %s:%d]",
	       inet_ntoa (rqhost->sin_addr), ntohs (rqhost->sin_port));

      log_msg ("\tdomainname = \"%s\"", argp->domain);
      log_msg ("\tmapname = \"%s\"", argp->map);
      log_msg ("\tkeydat = \"%.*s\"",
              (int) argp->key.keydat_len,
              argp->key.keydat_val);
    }

  memset (result, 0, sizeof (ypresp_key_val));

  valid = is_valid (rqstp, argp->map, argp->domain);
  if (valid < 1)
    {
      switch (valid)
	{
	case -1:
          if (debug_flag)
            log_msg ("\t-> Ignored (not a valid source host)");
          result->stat = YP_YPERR;
	  break;
	case -2:
          if (debug_flag)
            log_msg ("\t-> Ignored (not a valid map name)");
	  result->stat = YP_BADARGS;
	  break;
	case -3:
          if (debug_flag)
            log_msg ("\t-> Ignored (not a valid domain)");
          result->stat = YP_NODOM;
	  break;
        }
      return TRUE;
    }

  dbp = ypdb_open (argp->domain, argp->map);
  if (dbp == NULL)
    result->stat = YP_NOMAP;
  else
    {
      datum oldkey, dkey;

      oldkey.dsize = argp->key.keydat_len;
      oldkey.dptr = strndup (argp->key.keydat_val, oldkey.dsize);

      dkey = ypdb_nextkey (dbp, oldkey);
      while (dkey.dptr != NULL && dkey.dptr[0] == 'Y' &&
	     dkey.dptr[1] == 'P' && dkey.dptr[2] == '_')
	{
	  free (oldkey.dptr);
	  oldkey.dsize = dkey.dsize;
	  oldkey.dptr = strndup (dkey.dptr, dkey.dsize);
	  ypdb_free (dkey.dptr);
	  dkey = ypdb_nextkey (dbp, oldkey);
	}

      free (oldkey.dptr);

      if (dkey.dptr == NULL)
	result->stat = YP_NOMORE;
      else
	{
	  datum dval = ypdb_fetch (dbp, dkey);

	  result->stat = YP_TRUE;
	  result->key.keydat_len = dkey.dsize;
	  result->key.keydat_val = dkey.dptr;

	  result->val.valdat_len = dval.dsize;
	  result->val.valdat_val = dval.dptr;
	}
      ypdb_close (dbp);
    }

  if (debug_flag)
    {
      if (result->stat == YP_TRUE)
        log_msg ("\t-> Key = \"%.*s\", Value = \"%.*s\"",
		 (int) result->key.keydat_len, result->key.keydat_val,
		 (int) result->val.valdat_len, result->val.valdat_val);
      else if (result->stat == YP_NOMORE)
        log_msg ("\t-> No more entry's");
      else
        log_msg ("\t-> Error #%d", result->stat);
    }

  return TRUE;
}

bool_t
ypproc_xfr_2_svc (ypreq_xfr *argp, ypresp_xfr *result,
		  struct svc_req *rqstp)
{
  DB_FILE dbp;
  struct sockaddr_in *rqhost = svc_getcaller (rqstp->rq_xprt);
  int valid;

  if (debug_flag)
    {
      log_msg ("ypproc_xfr_2_svc(): [From: %s:%d]\n\tmap_parms:",
	       inet_ntoa (rqhost->sin_addr), ntohs (rqhost->sin_port));

      log_msg ("\t\tdomain   = \"%s\"", argp->map_parms.domain);
      log_msg ("\t\tmap      = \"%s\"", argp->map_parms.map);
      log_msg ("\t\tordernum = %u", argp->map_parms.ordernum);
      log_msg ("\t\tpeer     = \"%s\"", argp->map_parms.peer);
      log_msg ("\t\ttransid  = %u", argp->transid);
      log_msg ("\t\tprog     = %u", argp->prog);
      log_msg ("\t\tport     = %u", argp->port);
    }

  memset (result, 0, sizeof (ypresp_xfr));
  result->transid = argp->transid;

  valid = is_valid (rqstp, argp->map_parms.map, argp->map_parms.domain);
  if (valid < 1)
    {
      switch (valid)
	{
	case -1:
	  if (debug_flag)
	    log_msg ("\t-> Ignored (not a valid source host)");
	  else
	    log_msg ("refuse to transfer map from %s",
		     inet_ntoa (rqhost->sin_addr));
	  result->xfrstat = YPXFR_REFUSED;
	  break;
	case -2:
	  if (debug_flag)
	    log_msg ("\t-> Ignored (map contains \"/\"!)");
	  else
	    log_msg ("refuse to transfer map from %s, no valid mapname",
		     inet_ntoa (rqhost->sin_addr));
	  result->xfrstat = YPXFR_REFUSED;
	  break;
	case -3:
	  if (debug_flag)
	    log_msg ("\t-> Ignored (not a valid domain)");
	  else
	    log_msg ("refuse to transfer map from %s, no valid domain",
		     inet_ntoa (rqhost->sin_addr));
	  result->xfrstat = YPXFR_NODOM;
	  break;
	}
      return TRUE;
    }

  if (xfr_check_port)
    {
      if(ntohs(rqhost->sin_port) >= IPPORT_RESERVED)
        {
          if (debug_flag)
            log_msg ("\t-> Ignored (no reserved port!)");
          else
            log_msg ("refuse to transfer %s from %s, no valid port",
		     argp->map_parms.map, inet_ntoa (rqhost->sin_addr));

          result->xfrstat = YPXFR_REFUSED;
	  return TRUE;
        }
    }

  /* If we have the map, check, if the master name is the same as in
     the ypreq_xfr struct. If we doesn't have the map, refuse. */
  dbp = ypdb_open(argp->map_parms.domain, argp->map_parms.map);
  if (dbp != NULL)
    {
      datum key;

      key.dsize = sizeof ("YP_MASTER_NAME") - 1;
      key.dptr = "YP_MASTER_NAME";

      if(ypdb_exists (dbp, key))
        {
          datum val = ypdb_fetch (dbp, key);

          if ((size_t)val.dsize != strlen (argp->map_parms.peer) ||
              strncmp (val.dptr, argp->map_parms.peer, val.dsize) != 0)
            {
              if (debug_flag)
                log_msg ("\t->Ignored (%s is not the master!)",
			 argp->map_parms.peer);
              else
                log_msg ("refuse to transfer %s from %s, not master",
			 argp->map_parms.map, inet_ntoa (rqhost->sin_addr));

	      ypdb_close (dbp);
              result->xfrstat = YPXFR_NODOM;
              return TRUE;
            }
        }
      else
        {
          /* If we do not have a YP_MASTER_NAME key, we don't have a
             master/slave NIS system */
          if (debug_flag)
            log_msg ("\t->Ignored (no YP_MASTER_NAME key in local map)");

	  ypdb_close (dbp);
          result->xfrstat = YPXFR_REFUSED;
          return TRUE;
        }
      ypdb_close (dbp);
    }
  else if (trusted_master != NULL)
    {
      /* We have a new map. We only allow new maps from a NIS master
	 we trust (which means, the admin told us this master is ok. */
      if (strcasecmp (trusted_master, argp->map_parms.peer) != 0)
	{
	  if (debug_flag)
	    log_msg ("\t->Ignored (%s is not a trusted master!)",
		     argp->map_parms.peer);
	  else
	    log_msg ("refuse to transfer %s from %s, no trusted master",
		     argp->map_parms.map, inet_ntoa (rqhost->sin_addr));

	  ypdb_close (dbp);
	  result->xfrstat = YPXFR_NODOM;
	  return TRUE;
	}
    }
  /* If you wish to allow the transfer of new maps, change the next
     #if 1 statement to #if 0 */
#if 1
  else
    {
      /* We doesn't have the map, refuse the transfer */
      if (debug_flag)
        log_msg ("\t->Ignored (I don't have this map)");
      else
        log_msg ("refuse to transfer %s from %s, map doesn't exist",
		 argp->map_parms.map, inet_ntoa (rqhost->sin_addr));

      result->xfrstat = YPXFR_REFUSED;
      return TRUE;
    }
#endif

  switch (fork ())
    {
    case 0:
      {
        char *ypxfr_command = alloca (sizeof (YPBINDIR) + 8);
        char g[30], t[30], p[30];
        int i;

        umask (0);
        i = open ("/dev/null", O_RDWR);
        dup (i);
        dup (i);

        sprintf (ypxfr_command, "%s/ypxfr", YPBINDIR);
        sprintf (t, "%u", argp->transid);
        sprintf (g, "%u", argp->prog);
        sprintf (p, "%u", argp->port);
        if (debug_flag)
          execl (ypxfr_command, "ypxfr", "--debug", "-d",
                 argp->map_parms.domain, "-h", argp->map_parms.peer,
		 "-C", t, g,
                 inet_ntoa (rqhost->sin_addr), p, argp->map_parms.map, NULL);
        else
          execl (ypxfr_command, "ypxfr", "-d", argp->map_parms.domain, "-h",
                 argp->map_parms.peer, "-C", t, g,
                 inet_ntoa (rqhost->sin_addr), p, argp->map_parms.map, NULL);

        log_msg ("ypxfr execl(): %s", strerror (errno));
        exit (0);
      }
    case -1:
      log_msg ("Cannot fork: %s", strerror (errno));
      result->xfrstat = YPXFR_XFRERR;
    default:
      result->xfrstat = YPXFR_SUCC;
      break;
    }

  return TRUE;
}

bool_t ypproc_clear_2_svc (void *argp __attribute__ ((unused)),
			   void *result __attribute__ ((unused)),
			   struct svc_req *rqstp)
{
  if (debug_flag)
    {
      struct sockaddr_in *rqhost = svc_getcaller (rqstp->rq_xprt);
      log_msg ("ypproc_clear_2_svc() [From: %s:%d]",
	       inet_ntoa (rqhost->sin_addr), ntohs (rqhost->sin_port));
    }

  if (is_valid (rqstp, NULL, NULL) < 1)
    {
      if (debug_flag)
        log_msg ("\t-> Ignored (not a valid source host)");
    }
  else
    ypdb_close_all ();

  return TRUE;
}

/* We need the struct for giving ypall_encode the DB_FILE handle */
typedef struct ypall_data {
  DB_FILE dbm;
  datum dkey;
  datum dval;
} *ypall_data_t;

static int
ypall_close (void *data)
{
  if (data == NULL)
    {
      log_msg ("ypall_close() called with NULL pointer.");
      return 0;
    }

  ypdb_close (((ypall_data_t) data)->dbm);
  if (((ypall_data_t) data)->dkey.dptr)
    ypdb_free (((ypall_data_t) data)->dkey.dptr);
  if (((ypall_data_t) data)->dval.dptr)
    ypdb_free (((ypall_data_t) data)->dval.dptr);
  free (data);
  return 0;
}

static int
ypall_encode (ypresp_key_val *val, void *data)
{
  datum oldkey;

  oldkey.dsize = val->key.keydat_len;
  oldkey.dptr = strndup (val->key.keydat_val, oldkey.dsize);
  ypdb_free (((ypall_data_t) data)->dkey.dptr);
  ((ypall_data_t) data)->dkey.dptr = NULL;
  ypdb_free (((ypall_data_t) data)->dval.dptr);
  ((ypall_data_t) data)->dval.dptr = NULL;

  ((ypall_data_t) data)->dkey = ypdb_nextkey (((ypall_data_t) data)->dbm,
					      oldkey);

  while (((ypall_data_t) data)->dkey.dptr != NULL &&
	 ((ypall_data_t) data)->dkey.dptr[0] == 'Y' &&
	 ((ypall_data_t) data)->dkey.dptr[1] == 'P' &&
	 ((ypall_data_t) data)->dkey.dptr[2] == '_')
    {
      free (oldkey.dptr);
      oldkey.dsize = ((ypall_data_t) data)->dkey.dsize;
      oldkey.dptr = strndup (((ypall_data_t) data)->dkey.dptr,
			     ((ypall_data_t) data)->dkey.dsize);
      ypdb_free (((ypall_data_t) data)->dkey.dptr);
      ((ypall_data_t) data)->dkey.dptr = NULL;

      ((ypall_data_t) data)->dkey = ypdb_nextkey (((ypall_data_t) data)->dbm,
						  oldkey);
    }

  free (oldkey.dptr);

  if (((ypall_data_t) data)->dkey.dptr == NULL)
    val->stat = YP_NOMORE;
  else
    {
      ((ypall_data_t) data)->dval =
	ypdb_fetch (((ypall_data_t) data)->dbm, ((ypall_data_t) data)->dkey);

      val->stat = YP_TRUE;

      val->key.keydat_val = ((ypall_data_t) data)->dkey.dptr;
      val->key.keydat_len = ((ypall_data_t) data)->dkey.dsize;

      val->val.valdat_val = ((ypall_data_t) data)->dval.dptr;
      val->val.valdat_len = ((ypall_data_t) data)->dval.dsize;
    }
  return val->stat;
}

extern xdr_ypall_cb_t xdr_ypall_cb;

bool_t
ypproc_all_2_svc (ypreq_nokey *argp, ypresp_all *result, struct svc_req *rqstp)
{
  ypall_data_t data;
  int valid;

  if (debug_flag)
    {
      struct sockaddr_in *rqhost;

      rqhost = svc_getcaller (rqstp->rq_xprt);
      log_msg ("ypproc_all_2_svc(): [From: %s:%d]",
	       inet_ntoa (rqhost->sin_addr), ntohs (rqhost->sin_port));

      log_msg ("\t\tdomain = \"%s\"", argp->domain);
      log_msg ("\t\tmap = \"%s\"", argp->map);
    }

  memset (result, 0, sizeof (ypresp_all));
  xdr_ypall_cb.u.encode = NULL;
  xdr_ypall_cb.u.close = NULL;
  xdr_ypall_cb.data = NULL;

  valid = is_valid (rqstp, argp->map, argp->domain);
  if (valid < 1)
    {
      switch (valid)
	{
	case -1:
	  if (debug_flag)
	    log_msg ("\t-> Ignored (not a valid source host)");
	  result->ypresp_all_u.val.stat = YP_YPERR;
	  break;
	case -2:
	  if (debug_flag)
	    log_msg ("\t-> Ignored (not a valid map name)");
	  result->ypresp_all_u.val.stat = YP_BADARGS;
	  break;
	case -3:
	  if (debug_flag)
	    log_msg ("\t-> Ignored (not a valid domain)");
	  result->ypresp_all_u.val.stat = YP_NODOM;
	}
      return TRUE;
    }

  result->more = TRUE;

  if ((data = calloc (1, sizeof (struct ypall_data))) == NULL)
    {
      log_msg ("ERROR: could not allocate enough memory! [%s|%d]",
	       __FILE__, __LINE__);
      result->ypresp_all_u.val.stat = YP_YPERR;
      return TRUE;
    }

  data->dbm = ypdb_open (argp->domain, argp->map);

  if (data->dbm == NULL)
    result->ypresp_all_u.val.stat = YP_NOMAP;
  else
    {
      data->dkey = ypdb_firstkey (data->dbm);

      while (data->dkey.dptr != NULL && data->dkey.dptr[0] == 'Y'
	     && data->dkey.dptr[1] == 'P' && data->dkey.dptr[2] == '_')
	{
	  datum tkey = data->dkey;
	  data->dkey = ypdb_nextkey (data->dbm, tkey);
	  ypdb_free (tkey.dptr);
	}

      if (data->dkey.dptr != NULL)
	{
	  data->dval = ypdb_fetch (data->dbm, data->dkey);

	  result->ypresp_all_u.val.stat = YP_TRUE;

	  result->ypresp_all_u.val.key.keydat_len = data->dkey.dsize;
	  result->ypresp_all_u.val.key.keydat_val = data->dkey.dptr;

	  result->ypresp_all_u.val.val.valdat_len = data->dval.dsize;
	  result->ypresp_all_u.val.val.valdat_val = data->dval.dptr;

	  xdr_ypall_cb.u.encode = ypall_encode;
	  xdr_ypall_cb.u.close = ypall_close;
	  xdr_ypall_cb.data = (void *) data;

	  if (debug_flag)
	    log_msg ("\t -> First value returned.");

	  if (result->ypresp_all_u.val.stat == YP_TRUE)
	    return TRUE; /* We return to commit the data.
			    This also means, we don't give
			    data free here */
	}
      else
	result->ypresp_all_u.val.stat = YP_NOMORE;

      ypdb_close (data->dbm);
    }

  free (data);

  if (debug_flag)
    log_msg ("\t -> Exit from ypproc_all without sending data.");

  return TRUE;
}

bool_t
ypproc_master_2_svc (ypreq_nokey *argp, ypresp_master *result,
		     struct svc_req *rqstp)
{
  DB_FILE dbp;
  int valid;

  if (debug_flag)
    {
      struct sockaddr_in *rqhost = svc_getcaller (rqstp->rq_xprt);
      log_msg ("ypproc_master_2_svc(): [From: %s:%d]",
	       inet_ntoa (rqhost->sin_addr), ntohs (rqhost->sin_port));

      log_msg ("\t\tdomain = \"%s\"", argp->domain);
      log_msg ("\t\tmap = \"%s\"", argp->map);
    }

  memset (result, 0, sizeof (ypresp_master));

  valid = is_valid (rqstp, argp->map, argp->domain);
  if (valid < 1)
    {
      switch (valid)
	{
	case -1:
          if (debug_flag)
            log_msg ("\t-> Ignored (not a valid source host)");
	  result->stat = YP_YPERR;
	  break;
	case -2:
          if (debug_flag)
            log_msg ("\t-> Ignored (not a valid map name)");
	  result->stat = YP_BADARGS;
	  break;
	case -3:
          if (debug_flag)
            log_msg ("\t-> Ignored (not a domain)");
          result->stat = YP_NODOM;
        }
      result->peer = strdup ("");
      return TRUE;
    }

  dbp = ypdb_open (argp->domain, argp->map);
  if (dbp == NULL)
    result->stat = YP_NOMAP;
  else
    {
      datum key, val;

      key.dsize = sizeof ("YP_MASTER_NAME") - 1;
      key.dptr = "YP_MASTER_NAME";

      val = ypdb_fetch (dbp, key);
      if (val.dptr == NULL)
	{
	  /* No YP_MASTER_NAME record in map? There is someting wrong */
	  result->stat = YP_BADDB;
	}
      else
	{
	  int i;
	  char *hostbuf = alloca (val.dsize + 1);

	  /* put the eof string mark at the end of the string */
	  for (i = 0; i < val.dsize; ++i)
	    hostbuf[i] = val.dptr[i];
	  hostbuf[val.dsize] = '\0';
	  ypdb_free (val.dptr);

	  if ((result->peer = strdup (hostbuf)) == NULL)
	    result->stat = YP_YPERR;
	  else
	    result->stat = YP_TRUE;
	}

      ypdb_close (dbp);
    }

  if (result->peer == NULL)
    result->peer = strdup ("");

  if (debug_flag)
    log_msg ("\t-> Peer = \"%s\"", result->peer);

  return TRUE;
}


/* Get the DateTimeModified value for a certain map database */
static inline unsigned long
get_dtm (const char *domain, const char *map)
{
  struct stat sbuf;
  char *buf = alloca (strlen (domain) + strlen (map) + 3);
  char *cp;

  cp = stpcpy (buf, domain);
  *cp++ = '/';
  strcpy (cp, map);

  if (stat (buf, &sbuf) < 0)
    return time (NULL); /* We set it to the current time. */
  else
    return (unsigned long) sbuf.st_mtime;
}

bool_t
ypproc_order_2_svc (ypreq_nokey *argp, ypresp_order *result,
		    struct svc_req *rqstp)
{
  DB_FILE dbp;
  int valid;

  if (debug_flag)
    {
      struct sockaddr_in *rqhost;

      rqhost = svc_getcaller (rqstp->rq_xprt);

      log_msg ("ypproc_order_2_svc(): [From: %s:%d]",
	       inet_ntoa (rqhost->sin_addr), ntohs (rqhost->sin_port));

      log_msg ("\t\tdomain = \"%s\"", argp->domain);
      log_msg ("\t\tmap = \"%s\"", argp->map);
    }

  memset (result, 0, sizeof (ypresp_order));

  valid = is_valid (rqstp, argp->map, argp->domain);
  if (valid < 1)
    {
      switch (valid)
	{
	case -1:
          if (debug_flag)
            log_msg ("\t-> Ignored (not a valid source host)");
          result->stat = YP_YPERR;
	  break;
	case -2:
          if (debug_flag)
            log_msg ("\t-> Ignored (not a valid map name)");
          result->stat = YP_BADARGS;
	  break;
	case -3:
          if (debug_flag)
            log_msg ("\t-> Ignored (not a valid domain)");
          result->stat = YP_NODOM;
	  break;
        }
      return TRUE;
    }

  dbp = ypdb_open (argp->domain, argp->map);

  if (dbp == NULL)
    result->stat = YP_NOMAP;
  else
    {
      datum key, val;

      key.dsize = sizeof ("YP_LAST_MODIFIED") - 1;
      key.dptr = "YP_LAST_MODIFIED";

      val = ypdb_fetch (dbp, key);
      if (val.dptr == NULL)
	{
	  /* No YP_LAST_MODIFIED record in map? Use DTM timestamp.. */
	  result->ordernum = get_dtm (argp->domain, argp->map);
	}
      else
	{
	  char *buf = alloca (val.dsize + 1);

	  memcpy (buf, val.dptr, val.dsize);
	  buf[val.dsize] = '\0';
	  result->ordernum = atoi (buf);
	  ypdb_free (val.dptr);
	}

      result->stat = YP_TRUE;
      ypdb_close (dbp);
    }

  if (debug_flag)
    log_msg ("-> Order # %u", result->ordernum);

  return TRUE;
}


static int
add_maplist (ypmaplist **mlhp, char *map)
{
  ypmaplist *mlp;
#if defined(HAVE_NDBM)
#if defined(sun) || defined(__sun__)
  int len = strlen (map);

  /* We have all maps twice: with .dir and with .pag. Ignore .pag */
  if (len > 3 && map[len - 4] == '.' && map[len - 3] == 'p' &&
      map[len - 2] == 'a' && map[len - 1] == 'g')
    return 0;

  if (len > 3 && map[len - 4] == '.' && map[len - 3] == 'd' &&
      map[len - 2] == 'i' && map[len - 1] == 'r')
    map[len - 4] = '\0';
#else
  int len = strlen (map);

  if (len > 2 && map[len - 3] == '.' && map[len - 2] == 'd' &&
      map[len - 1] == 'b')
    map[len - 3] = '\0';
#endif
#endif

  if ((mlp = malloc (sizeof (*mlp))) == NULL)
    return -1;

  if ((mlp->map = strdup (map)) == NULL)
    {
      free (mlp);
      return -1;
    }

  mlp->next = *mlhp;
  *mlhp = mlp;

  return 0;
}

bool_t
ypproc_maplist_2_svc (domainname *argp, ypresp_maplist *result,
		      struct svc_req *rqstp)
{
  DIR *dp;
  int valid;

  if (debug_flag)
    {
      struct sockaddr_in *rqhost = svc_getcaller (rqstp->rq_xprt);

      log_msg ("ypproc_maplist_2_svc(): [From: %s:%d]",
	       inet_ntoa (rqhost->sin_addr), ntohs (rqhost->sin_port));

      log_msg ("\t\tdomain = \"%s\"", *argp);
    }

  memset (result, 0, sizeof (ypresp_maplist));

  valid = is_valid (rqstp, NULL, *argp);
  if (valid < 1)
    {
      switch (valid)
	{
	case -1:
          if (debug_flag)
            log_msg ("\t-> Ignored (not a valid source host)");
          result->stat = YP_YPERR;
	  break;
	case -2: /* should never happen */
	case -3:
          if (debug_flag)
            log_msg ("\t-> Ignored (not a valid domain)");
          result->stat = YP_NODOM;
	  break;
        }
      return TRUE;
    }

  /* open domain directory */
  dp = opendir (*argp);
  if (dp == NULL)
    {
      if (debug_flag)
	log_msg ("opendir: %s", strerror (errno));

      result->stat = YP_BADDB;
    }
  else
    {
      struct dirent *dep;

      while ((dep = readdir (dp)) != NULL)
	{
	  /* ignore files starting with . */
	  if (dep->d_name[0] == '.')
	    continue;
	  if (add_maplist (&result->maps, dep->d_name) < 0)
	    {
	      result->stat = YP_YPERR;
	      break;
	    }
	}
      closedir (dp);
      result->stat = YP_TRUE;
    }

  if (debug_flag)
    {
      if (result->stat == YP_TRUE)
        {
          ypmaplist *p;

          p = result->maps;
          log_msg ("-> ");
          while (p)
            {
              if (p->next)
		log_msg ("   %s,", p->map);
	      else
		log_msg ("   %s", p->map);
              p = p->next;
            }
        }
      else
        log_msg ("\t-> Error #%d", result->stat);
    }

  return TRUE;
}

int
ypprog_2_freeresult (SVCXPRT *transp __attribute__ ((unused)),
		     xdrproc_t xdr_result, caddr_t result)
{
  xdr_free (xdr_result, result);

  return 1;
}
