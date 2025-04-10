/* Copyright (c) 1996, 1997, 1999, 2001  Thorsten Kukuk
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
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <rpc/rpc.h>
#include "yp.h"

struct {
  union {
    int (*encoder) (char *, int, char **, int *, char **, int *);
    int (*decoder) (int, char *, int, char *, int, char *);
  }
  foreach;
  char *data;
} *xdr_ypall_callback;


bool_t
xdr_domainname (XDR *xdrs, domainname *objp)
{
  if (!xdr_string (xdrs, objp, YPMAXDOMAIN))
    return (FALSE);

  return (TRUE);
}

bool_t
xdr_mapname (XDR *xdrs, mapname *objp)
{
  if (!xdr_string (xdrs, objp, YPMAXMAP))
    return FALSE;

  return TRUE;
}

bool_t
xdr_peername (XDR *xdrs, peername *objp)
{
  if (!xdr_string (xdrs, objp, YPMAXPEER))
    return (FALSE);
  return (TRUE);
}

bool_t
xdr_keydat (XDR *xdrs, keydat *objp)
{
  if (!xdr_bytes (xdrs, (char **) &objp->keydat_val,
		  (u_int *) &objp->keydat_len, ~0))
    return FALSE;
  return TRUE;
}

bool_t
xdr_valdat (XDR *xdrs, valdat *objp)
{
  if (!xdr_bytes (xdrs, (char **) &objp->valdat_val,
		  (u_int *) &objp->valdat_len, ~0))
    return FALSE;
  return TRUE;
}

bool_t
xdr_ypresp_val (XDR *xdrs, ypresp_val *objp)
{
  if (!xdr_ypstat (xdrs, &objp->stat))
    return FALSE;
  if (!xdr_valdat (xdrs, &objp->val))
    return FALSE;
  return TRUE;
}

bool_t
xdr_ypresp_key_val (XDR *xdrs, ypresp_key_val *objp)
{
  if (!xdr_ypstat (xdrs, &objp->stat))
    return FALSE;

  if (!xdr_valdat (xdrs, &objp->val))
    return FALSE;

  if (!xdr_keydat (xdrs, &objp->key))
    return FALSE;

  return TRUE;
}

bool_t
xdr_ypresp_master (XDR *xdrs, ypresp_master *objp)
{
  if (!xdr_ypstat (xdrs, &objp->stat))
    return FALSE;
  if (!xdr_peername (xdrs, &objp->peer))
    return FALSE;
  return TRUE;
}

bool_t
xdr_ypresp_order (XDR *xdrs, ypresp_order *objp)
{
  if (!xdr_ypstat (xdrs, &objp->stat))
    return FALSE;
  if (!xdr_u_int (xdrs, &objp->ordernum))
    return FALSE;
  return TRUE;
}

bool_t
xdr_ypbind_binding (XDR *xdrs, ypbind_binding *objp)
{
  if (!xdr_opaque (xdrs, objp->ypbind_binding_addr, 4))
    return FALSE;
  if (!xdr_opaque (xdrs, objp->ypbind_binding_port, 2))
    return FALSE;
  return TRUE;
}

bool_t
xdr_ypbind_setdom (XDR *xdrs, ypbind_setdom *objp)
{
  if (!xdr_domainname (xdrs, &objp->ypsetdom_domain))
    return FALSE;

  if (!xdr_ypbind_binding (xdrs, &objp->ypsetdom_binding))
    return FALSE;

  if (!xdr_u_int (xdrs, &objp->ypsetdom_vers))
    return FALSE;

  return TRUE;
}

bool_t
xdr_ypreq_key (XDR *xdrs, ypreq_key *objp)
{
  if (!xdr_domainname (xdrs, &objp->domain))
    return FALSE;

  if (!xdr_mapname (xdrs, &objp->map))
    return FALSE;

  if (!xdr_keydat (xdrs, &objp->key))
    return FALSE;

  return TRUE;
}

bool_t
xdr_ypreq_nokey (XDR *xdrs, ypreq_nokey *objp)
{
  if (!xdr_domainname (xdrs, &objp->domain))
    return FALSE;

  if (!xdr_mapname (xdrs, &objp->map))
    return FALSE;

  return TRUE;
}

bool_t
xdr_ypstat (XDR *xdrs, ypstat *objp)
{
  if (!xdr_enum (xdrs, (enum_t *) objp))
    return FALSE;

  return TRUE;
}

bool_t
xdr_ypxfrstat (XDR *xdrs, ypxfrstat *objp)
{
  if (!xdr_enum (xdrs, (enum_t *) objp))
    return FALSE;

  return TRUE;
}

bool_t
xdr_ypresp_xfr (XDR * xdrs, ypresp_xfr * objp)
{
  if (!xdr_u_int (xdrs, &objp->transid))
    return FALSE;
  if (!xdr_ypxfrstat (xdrs, &objp->xfrstat))
    return FALSE;
  return TRUE;
}

bool_t
ypxfr_xdr_ypresp_all (XDR *xdrs, ypresp_all *objp)
{
  int CallAgain = 0;

  if (xdrs->x_op == XDR_DECODE)
    {
      while (1)
	{
	  int s = objp->ypresp_all_u.val.stat;
	  memset (objp, '\0', sizeof (*objp));
	  objp->ypresp_all_u.val.stat = s;
	  if (!xdr_bool (xdrs, &objp->more))
	    return FALSE;

	  switch (objp->more)
	    {
	    case TRUE:
	      if (!xdr_ypresp_key_val (xdrs, &objp->ypresp_all_u.val))
		{
		  printf ("xdr_ypresp_key_val failed\n");
		  return (FALSE);
		}

	      if (CallAgain == 0)
		{
		  CallAgain = (*(xdr_ypall_callback->foreach.decoder))
		    (objp->ypresp_all_u.val.stat,
		     objp->ypresp_all_u.val.key.keydat_val,
		     objp->ypresp_all_u.val.key.keydat_len,
		     objp->ypresp_all_u.val.val.valdat_val,
		     objp->ypresp_all_u.val.val.valdat_len,
		     xdr_ypall_callback->data);
		}
	      break;
	    case FALSE:
	      return TRUE;
	    }
	  xdr_free ((xdrproc_t) ypxfr_xdr_ypresp_all, (char *) objp);
	}
    }
  else if (xdrs->x_op == XDR_ENCODE)
    {
      while (1)
	{
	  if (!xdr_bool (xdrs, &(objp->more)))
	    return FALSE;

	  if (!xdr_ypresp_key_val (xdrs, &objp->ypresp_all_u.val))
	    {
	      printf ("xdr_ypresp_key_val failed\n");
	      return FALSE;
	    }
	  if (objp->ypresp_all_u.val.stat != YP_TRUE)
	    {
	      objp->more = FALSE;
	      if (!xdr_bool (xdrs, &(objp->more)))
		return FALSE;

	      return TRUE;
	    }
	  objp->ypresp_all_u.val.stat =
	    (enum ypstat) (*(xdr_ypall_callback->foreach.encoder))
	    (objp->ypresp_all_u.val.key.keydat_val,
	     objp->ypresp_all_u.val.key.keydat_len,
	     &(objp->ypresp_all_u.val.key.keydat_val),
	     &(objp->ypresp_all_u.val.key.keydat_len),
	     &(objp->ypresp_all_u.val.val.valdat_val),
	     &(objp->ypresp_all_u.val.val.valdat_len));
	}
    }
  else
    return TRUE;
}

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

enum clnt_stat
ypproc_all_2 (ypreq_nokey *argp, ypresp_all *clnt_res, CLIENT *clnt)
{
  return (clnt_call(clnt, YPPROC_ALL,
                    (xdrproc_t) xdr_ypreq_nokey, (caddr_t) argp,
                    (xdrproc_t) ypxfr_xdr_ypresp_all, (caddr_t) clnt_res,
                    TIMEOUT));
}
