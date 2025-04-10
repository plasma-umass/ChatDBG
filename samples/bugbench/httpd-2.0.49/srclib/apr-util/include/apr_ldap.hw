/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr.h"
#include "apu.h"

/*
 * apr_ldap.h is generated from apr_ldap.h.in by configure -- do not edit apr_ldap.h
 */
/**
 * @file apr_ldap.h
 * @brief  APR-UTIL LDAP 
 */
#ifndef APU_LDAP_H
#define APU_LDAP_H

/**
 * @defgroup APR_Util_LDAP LDAP
 * @ingroup APR_Util
 * @{
 */


/*
 * This switches LDAP support on or off.
 */

/* this will be defined if LDAP support was compiled into apr-util */
#define APR_HAS_LDAP		  1

/* this whole thing disappears if LDAP is not enabled */
#if !APR_HAS_LDAP

#define APR_HAS_NETSCAPE_LDAPSDK    0
#define APR_HAS_NOVELL_LDAPSDK      0
#define APR_HAS_OPENLDAP_LDAPSDK    0
#define APR_HAS_MICROSOFT_LDAPSDK   0
#define APR_HAS_OTHER_LDAPSDK       0

#define APR_HAS_LDAP_SSL            0
#define APR_HAS_LDAP_URL_PARSE    0


#else /* ldap support available */


   /* There a several LDAPv3 SDKs available on various platforms
    * define which LDAP SDK is used 
   */
#define APR_HAS_NETSCAPE_LDAPSDK    0
#define APR_HAS_NOVELL_LDAPSDK      0
#define APR_HAS_OPENLDAP_LDAPSDK    0
#define APR_HAS_MICROSOFT_LDAPSDK   1
#define APR_HAS_OTHER_LDAPSDK       0

   /* define if LDAP SSL support is available 
   */
#define APR_HAS_LDAP_SSL            1

   /* If no APR_HAS_xxx_LDAPSDK is defined error out
    * Define if the SDK supports the ldap_url_parse function 
   */
#if APR_HAS_NETSCAPE_LDAPSDK 
   #define APR_HAS_LDAP_URL_PARSE      1
#elif APR_HAS_NOVELL_LDAPSDK 
   #define APR_HAS_LDAP_URL_PARSE      1
#elif APR_HAS_OPENLDAP_LDAPSDK
   #define APR_HAS_LDAP_URL_PARSE      1
#elif APR_HAS_MICROSOFT_LDAPSDK
   #define APR_HAS_LDAP_URL_PARSE      0
#elif APR_HAS_OTHER_LDAPSDK
   #define APR_HAS_LDAP_URL_PARSE      0
#else
   #define APR_HAS_LDAP_URL_PARSE      0
   #error "ERROR no LDAP SDK defined!"
#endif

/* These are garbage, our public macros are always APR_HAS_ prefixed,
 * and use 0/1 values, not defined/undef semantics.  
 *
 * Will be deprecated in APR 1.0
 */
#if APR_HAS_LDAP
#define APU_HAS_LDAP
#endif


/* LDAP header files */

#if APR_HAS_NETSCAPE_LDAPSDK
#include <ldap.h>
#include <lber.h>
#if APR_HAS_LDAP_SSL 
#include <ldap_ssl.h>
#endif
#endif

#if APR_HAS_NOVELL_LDAPSDK
#include <ldap.h>
#include <lber.h>
#if APR_HAS_LDAP_SSL 
#include <ldap_ssl.h>
#endif
#endif

#if APR_HAS_OPENLDAP_LDAPSDK
#include <ldap.h>
#include <lber.h>
#endif

/* Included in Windows 2000 and later, earlier 9x/NT 4.0 clients
 * will need to obtain the Active Directory Client Extensions.
 */
#if APR_HAS_MICROSOFT_LDAPSDK
#include <winldap.h>
#define LDAPS_PORT LDAP_SSL_PORT
#endif


/* LDAPv2 SDKs don't use const parameters in their prototypes.  
 * LDAPv3 SDKs do use const.  When compiling with LDAPv2 SDKs, const_cast 
 * casts away the constness, but won't under LDAPv3 
 */
#if LDAP_VERSION_MAX <= 2
#define const_cast(x) ((char *)(x))
#else
#define const_cast(x) (x)
#endif
   

#include "apr_ldap_url.h"

/* Define some errors that are mysteriously gone from OpenLDAP 2.x */
#ifndef LDAP_URL_ERR_NOTLDAP
#define LDAP_URL_ERR_NOTLDAP LDAP_URL_ERR_BADSCHEME
#endif

#ifndef LDAP_URL_ERR_NODN
#define LDAP_URL_ERR_NODN LDAP_URL_ERR_BADURL
#endif

/** @} */
#endif /* APR_HAS_LDAP */
#endif /* APU_LDAP_H */
