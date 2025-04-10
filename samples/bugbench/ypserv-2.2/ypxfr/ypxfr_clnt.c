/* Copyright (c) 2000  Thorsten Kukuk
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE

#include <time.h>
#include <string.h>
#include <memory.h>

#include "yp.h"
#include "ypxfr.h"

static struct timeval TIMEOUT = { 25, 0 };

enum clnt_stat
ypproc_order_2 (ypreq_nokey *argp, ypresp_order *clnt_res, CLIENT *clnt)
{
  return (clnt_call(clnt, YPPROC_ORDER,
		    (xdrproc_t) xdr_ypreq_nokey, (caddr_t) argp,
		    (xdrproc_t) xdr_ypresp_order, (caddr_t) clnt_res,
		    TIMEOUT));
}
