/* -*- c++ -*- */
#ifndef _SNMP_COMPAT_H_
#define _SNMP_COMPAT_H_

/***************************************************************************
 *
 *           Copyright 1997 by Carnegie Mellon University
 * 
 *                       All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of CMU not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 * 
 * CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * 
 * Author: Ryan Troll <ryan+@andrew.cmu.edu>
 * 
 * $Id: snmp_compat.h,v 1.3 1998/04/04 01:43:45 kostas Exp $
 * 
 ***************************************************************************/

/* SMI Types */
#ifndef INTEGER
#define INTEGER     ASN_INTEGER
#define STRING      ASN_OCTET_STR
#define OBJID       ASN_OBJECT_ID
#define NULLOBJ     ASN_NULL
#endif
/* PDU Types */

#define GET_REQ_MSG	    (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x0)
#define GETNEXT_REQ_MSG     (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x1)
#define GET_RSP_MSG         (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x2)
#define SET_REQ_MSG         (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x3)
#define TRP_REQ_MSG	    (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x4)	/*Obsolete */

#define INFORM_REQ_MSG      (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x6)

#endif /* _SNMP_COMPAT_H_ */
