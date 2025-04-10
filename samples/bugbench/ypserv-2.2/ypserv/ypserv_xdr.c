
#include <rpc/rpc.h>
#include "yp.h"

xdr_ypall_cb_t xdr_ypall_cb;

bool_t
xdr_ypresp_all(XDR *xdrs, ypresp_all *objp)
{
  if (xdrs->x_op == XDR_ENCODE)
    {
      while (1)
	{
	  if (xdr_bool(xdrs, &objp->more) == FALSE ||
	      xdr_ypresp_key_val(xdrs, &objp->ypresp_all_u.val) == FALSE)
	    {
	      if (xdr_ypall_cb.u.close != NULL)
		(*(xdr_ypall_cb.u.close))(xdr_ypall_cb.data);
	      
	      xdr_ypall_cb.data = NULL;
	      
	      return FALSE;
	    }
	  
	  if ((objp->ypresp_all_u.val.stat != YP_TRUE) ||
	      (*xdr_ypall_cb.u.encode)(&objp->ypresp_all_u.val,
				       xdr_ypall_cb.data) != YP_TRUE)
	    {
	      objp->more = FALSE;
	      
	      if (xdr_ypall_cb.u.close != NULL)
		(*(xdr_ypall_cb.u.close))(xdr_ypall_cb.data);
	      
	      xdr_ypall_cb.data = NULL;
	      
	      if (!xdr_bool(xdrs, &objp->more))
		return FALSE;
	      
	      return TRUE;
	    }
	  
	}
    }
  
#ifdef NOTYET /* This code isn't needed in the server */
    else if (xdrs->x_op == XDR_DECODE)
    {
	int more = 0;


	while (1)
	{
	    if (!xdr_bool(xdrs, &objp->more))
		return FALSE;

	    switch (objp->more)
	    {
	      case TRUE:
		if (!xdr_ypresp_key_val(xdrs, &objp->ypresp_all_u.val))
		    return FALSE;

		if (more == 0)
		    more = (*xdr_ypall_callback->foreach.decoder)
			(&objp->ypresp_all_u.val, xdr_ypall_callback->data);
		break;

	      case FALSE:
		return TRUE;

	      default:
		return FALSE;
	    }
	}
	return FALSE;
    }
#endif

    return TRUE;
}
