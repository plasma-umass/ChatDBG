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

/* Portions Copyright 1998-2002 The OpenLDAP Foundation
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.  A copy of this license is available at
 * http://www.OpenLDAP.org/license.html or in file LICENSE in the
 * top-level directory of the distribution.
 * 
 * OpenLDAP is a registered trademark of the OpenLDAP Foundation.
 * 
 * Individual files and/or contributed packages may be copyright by
 * other parties and subject to additional restrictions.
 * 
 * This work is derived from the University of Michigan LDAP v3.3
 * distribution.  Information concerning this software is available
 * at: http://www.umich.edu/~dirsvcs/ldap/
 * 
 * This work also contains materials derived from public sources.
 * 
 * Additional information about OpenLDAP can be obtained at:
 *     http://www.openldap.org/
 */

/* 
 * Portions Copyright (c) 1992-1996 Regents of the University of Michigan.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

/*  apr_ldap_url.c -- LDAP URL (RFC 2255) related routines
 *
 *  Win32 and perhaps other non-OpenLDAP based ldap libraries may be
 *  missing ldap_url_* APIs.  We focus here on the one significant
 *  aspect, which is parsing.  We have [for the time being] omitted
 *  the ldap_url_search APIs.
 *
 *  LDAP URLs look like this:
 *    ldap[is]://host:port[/[dn[?[attributes][?[scope][?[filter][?exts]]]]]]
 *
 *  where:
 *   attributes is a comma separated list
 *   scope is one of these three strings:  base one sub (default=base)
 *   filter is an string-represented filter as in RFC 2254
 *
 *  e.g.,  ldap://host:port/dc=com?o,cn?base?o=openldap?extension
 *
 *  Tolerates URLs that look like: <ldapurl> and <URL:ldapurl>
 */

#include "apr_ldap.h"

#if APR_HAS_LDAP

#if !APR_HAS_LDAP_URL_PARSE

#include "apr_general.h"
#include "apr_strings.h"

#ifndef LDAPS_PORT
#define LDAPS_PORT              636  /* ldaps:/// default LDAP over TLS port */
#endif

#define LDAP_URL_PREFIX         "ldap://"
#define LDAP_URL_PREFIX_LEN     (sizeof(LDAP_URL_PREFIX)-1)
#define LDAPS_URL_PREFIX        "ldaps://"
#define LDAPS_URL_PREFIX_LEN    (sizeof(LDAPS_URL_PREFIX)-1)
#define LDAPI_URL_PREFIX        "ldapi://"
#define LDAPI_URL_PREFIX_LEN    (sizeof(LDAPI_URL_PREFIX)-1)
#define LDAP_URL_URLCOLON       "URL:"
#define LDAP_URL_URLCOLON_LEN   (sizeof(LDAP_URL_URLCOLON)-1)

#define LDAP_STRDUP(x) strdup(x)
#define LDAP_CALLOC(n, s) calloc(n, s)
#define LDAP_MALLOC(n) malloc(n)
#define LDAP_REALLOC(x, n) realloc(x, n)
#define LDAP_FREE(x) free(x)
#define LDAP_VFREE(a) ldap_charray_free(a)

#define ldap_utf8_strchr(x, s) strchr(x, *s)
#define ldap_utf8_strtok(x, s, l) apr_strtok(x, s, l)

/* local functions */
static const char* skip_url_prefix(const char *url, int *enclosedp,
                                   const char **scheme);

static void ldap_pvt_hex_unescape(char *s);

static int ldap_pvt_unhex(int c);

static void ldap_charray_free(char **a);

static char **ldap_str2charray(const char *str, const char *brkstr);

APU_DECLARE(int) apr_ldap_is_ldap_url(const char *url)
{
    int enclosed;
    const char * scheme;

    if( url == NULL ) {
        return 0;
    }

    if( skip_url_prefix( url, &enclosed, &scheme ) == NULL ) {
        return 0;
    }

    return 1;
}

APU_DECLARE(int) apr_ldap_is_ldaps_url(const char *url)
{
    int enclosed;
    const char * scheme;

    if( url == NULL ) {
        return 0;
    }

    if( skip_url_prefix( url, &enclosed, &scheme ) == NULL ) {
        return 0;
    }

    return strcmp(scheme, "ldaps") == 0;
}

APU_DECLARE(int) apr_ldap_is_ldapi_url(const char *url)
{
    int enclosed;
    const char * scheme;

    if( url == NULL ) {
        return 0;
    }

    if( skip_url_prefix( url, &enclosed, &scheme ) == NULL ) {
        return 0;
    }

    return strcmp(scheme, "ldapi") == 0;
}

static const char *skip_url_prefix(const char *url, int *enclosedp,
                                   const char **scheme)
{
    /*
     * return non-zero if this looks like a LDAP URL; zero if not
     * if non-zero returned, *urlp will be moved past "ldap://" part of URL
     */
    const char *p;

    if ( url == NULL ) {
        return( NULL );
    }

    p = url;

    /* skip leading '<' (if any) */
    if ( *p == '<' ) {
        *enclosedp = 1;
        ++p;
    } else {
        *enclosedp = 0;
    }

    /* skip leading "URL:" (if any) */
    if ( strncasecmp( p, LDAP_URL_URLCOLON, LDAP_URL_URLCOLON_LEN ) == 0 ) {
        p += LDAP_URL_URLCOLON_LEN;
    }

    /* check for "ldap://" prefix */
    if ( strncasecmp( p, LDAP_URL_PREFIX, LDAP_URL_PREFIX_LEN ) == 0 ) {
        /* skip over "ldap://" prefix and return success */
        p += LDAP_URL_PREFIX_LEN;
        *scheme = "ldap";
        return( p );
    }

    /* check for "ldaps://" prefix */
    if ( strncasecmp( p, LDAPS_URL_PREFIX, LDAPS_URL_PREFIX_LEN ) == 0 ) {
        /* skip over "ldaps://" prefix and return success */
        p += LDAPS_URL_PREFIX_LEN;
        *scheme = "ldaps";
        return( p );
    }

    /* check for "ldapi://" prefix */
    if ( strncasecmp( p, LDAPI_URL_PREFIX, LDAPI_URL_PREFIX_LEN ) == 0 ) {
        /* skip over "ldapi://" prefix and return success */
        p += LDAPI_URL_PREFIX_LEN;
        *scheme = "ldapi";
        return( p );
    }

    return( NULL );
}


static int str2scope(const char *p)
{
    if ( strcasecmp( p, "one" ) == 0 ) {
        return LDAP_SCOPE_ONELEVEL;

    } else if ( strcasecmp( p, "onetree" ) == 0 ) {
        return LDAP_SCOPE_ONELEVEL;

    } else if ( strcasecmp( p, "base" ) == 0 ) {
        return LDAP_SCOPE_BASE;

    } else if ( strcasecmp( p, "sub" ) == 0 ) {
        return LDAP_SCOPE_SUBTREE;

    } else if ( strcasecmp( p, "subtree" ) == 0 ) {
        return LDAP_SCOPE_SUBTREE;
    }

    return( -1 );
}


static int ldap_url_parse_ext(const char *url_in, 
                              apr_ldap_url_desc_t **ludpp)
{
/*
 *  Pick apart the pieces of an LDAP URL.
 */
    apr_ldap_url_desc_t *ludp;
    char        *p, *q, *r;
    int         i, enclosed;
    const char  *scheme = NULL;
    const char  *url_tmp;
    char        *url;

    if( url_in == NULL || ludpp == NULL ) {
        return LDAP_URL_ERR_PARAM;
    }

    *ludpp = NULL;  /* pessimistic */

    url_tmp = skip_url_prefix( url_in, &enclosed, &scheme );

    if ( url_tmp == NULL ) {
        return LDAP_URL_ERR_BADSCHEME;
    }

    /* make working copy of the remainder of the URL */
    url = LDAP_STRDUP( url_tmp );
    if ( url == NULL ) {
        return LDAP_URL_ERR_MEM;
    }

    if ( enclosed ) {
        p = &url[strlen(url)-1];

        if( *p != '>' ) {
            LDAP_FREE( url );
            return LDAP_URL_ERR_BADENCLOSURE;
        }

        *p = '\0';
    }

    /* allocate return struct */
    ludp = (apr_ldap_url_desc_t *)LDAP_CALLOC( 1, sizeof( apr_ldap_url_desc_t ));

    if ( ludp == NULL ) {
        LDAP_FREE( url );
        return LDAP_URL_ERR_MEM;
    }

    ludp->lud_next = NULL;
    ludp->lud_host = NULL;
    ludp->lud_port = LDAP_PORT;
    ludp->lud_dn = NULL;
    ludp->lud_attrs = NULL;
    ludp->lud_filter = NULL;
    ludp->lud_scope = -1;
    ludp->lud_filter = NULL;
    ludp->lud_exts = NULL;

    ludp->lud_scheme = LDAP_STRDUP( scheme );

    if ( ludp->lud_scheme == NULL ) {
        LDAP_FREE( url );
        apr_ldap_free_urldesc( ludp );
        return LDAP_URL_ERR_MEM;
    }

    if( strcasecmp( ludp->lud_scheme, "ldaps" ) == 0 ) {
        ludp->lud_port = LDAPS_PORT;
    }

    /* scan forward for '/' that marks end of hostport and begin. of dn */
    p = strchr( url, '/' );

    if( p != NULL ) {
        /* terminate hostport; point to start of dn */
        *p++ = '\0';
    }

    /* IPv6 syntax with [ip address]:port */
    if ( *url == '[' ) {
        r = strchr( url, ']' );
        if ( r == NULL ) {
            LDAP_FREE( url );
            apr_ldap_free_urldesc( ludp );
            return LDAP_URL_ERR_BADURL;
        }
        *r++ = '\0';
        q = strchr( r, ':' );
    } else {
        q = strchr( url, ':' );
    }

    if ( q != NULL ) {
        *q++ = '\0';
        ldap_pvt_hex_unescape( q );

        if( *q == '\0' ) {
            LDAP_FREE( url );
            apr_ldap_free_urldesc( ludp );
            return LDAP_URL_ERR_BADURL;
        }

        ludp->lud_port = atoi( q );
    }

    ldap_pvt_hex_unescape( url );

    /* If [ip address]:port syntax, url is [ip and we skip the [ */
    ludp->lud_host = LDAP_STRDUP( url + ( *url == '[' ) );

    if( ludp->lud_host == NULL ) {
        LDAP_FREE( url );
        apr_ldap_free_urldesc( ludp );
        return LDAP_URL_ERR_MEM;
    }

    /*
     * Kludge.  ldap://111.222.333.444:389??cn=abc,o=company
     *
     * On early Novell releases, search references/referrals were returned
     * in this format, i.e., the dn was kind of in the scope position,
     * but the required slash is missing. The whole thing is illegal syntax,
     * but we need to account for it. Fortunately it can't be confused with
     * anything real.
     */
    if( (p == NULL) && (q != NULL) && ((q = strchr( q, '?')) != NULL)) {
        q++;
        /* ? immediately followed by question */
        if( *q == '?') {
            q++;
            if( *q != '\0' ) {
                /* parse dn part */
                ldap_pvt_hex_unescape( q );
                ludp->lud_dn = LDAP_STRDUP( q );
            } else {
                ludp->lud_dn = LDAP_STRDUP( "" );
            }

            if( ludp->lud_dn == NULL ) {
                LDAP_FREE( url );
                apr_ldap_free_urldesc( ludp );
                return LDAP_URL_ERR_MEM;
            }
        }
    }

    if( p == NULL ) {
        LDAP_FREE( url );
        *ludpp = ludp;
        return LDAP_URL_SUCCESS;
    }

    /* scan forward for '?' that may marks end of dn */
    q = strchr( p, '?' );

    if( q != NULL ) {
        /* terminate dn part */
        *q++ = '\0';
    }

    if( *p != '\0' ) {
        /* parse dn part */
        ldap_pvt_hex_unescape( p );
        ludp->lud_dn = LDAP_STRDUP( p );
    } else {
        ludp->lud_dn = LDAP_STRDUP( "" );
    }

    if( ludp->lud_dn == NULL ) {
        LDAP_FREE( url );
        apr_ldap_free_urldesc( ludp );
        return LDAP_URL_ERR_MEM;
    }

    if( q == NULL ) {
        /* no more */
        LDAP_FREE( url );
        *ludpp = ludp;
        return LDAP_URL_SUCCESS;
    }

    /* scan forward for '?' that may marks end of attributes */
    p = q;
    q = strchr( p, '?' );

    if( q != NULL ) {
        /* terminate attributes part */
        *q++ = '\0';
    }

    if( *p != '\0' ) {
        /* parse attributes */
        ldap_pvt_hex_unescape( p );
        ludp->lud_attrs = ldap_str2charray( p, "," );

        if( ludp->lud_attrs == NULL ) {
            LDAP_FREE( url );
            apr_ldap_free_urldesc( ludp );
            return LDAP_URL_ERR_BADATTRS;
        }
    }

    if ( q == NULL ) {
        /* no more */
        LDAP_FREE( url );
        *ludpp = ludp;
        return LDAP_URL_SUCCESS;
    }

    /* scan forward for '?' that may marks end of scope */
    p = q;
    q = strchr( p, '?' );

    if( q != NULL ) {
        /* terminate the scope part */
        *q++ = '\0';
    }

    if( *p != '\0' ) {
        /* parse the scope */
        ldap_pvt_hex_unescape( p );
        ludp->lud_scope = str2scope( p );

        if( ludp->lud_scope == -1 ) {
            LDAP_FREE( url );
            apr_ldap_free_urldesc( ludp );
            return LDAP_URL_ERR_BADSCOPE;
        }
    }

    if ( q == NULL ) {
        /* no more */
        LDAP_FREE( url );
        *ludpp = ludp;
        return LDAP_URL_SUCCESS;
    }

    /* scan forward for '?' that may marks end of filter */
    p = q;
    q = strchr( p, '?' );

    if( q != NULL ) {
        /* terminate the filter part */
        *q++ = '\0';
    }

    if( *p != '\0' ) {
        /* parse the filter */
        ldap_pvt_hex_unescape( p );

        if( ! *p ) {
            /* missing filter */
            LDAP_FREE( url );
            apr_ldap_free_urldesc( ludp );
            return LDAP_URL_ERR_BADFILTER;
        }

        LDAP_FREE( ludp->lud_filter );
        ludp->lud_filter = LDAP_STRDUP( p );

        if( ludp->lud_filter == NULL ) {
            LDAP_FREE( url );
            apr_ldap_free_urldesc( ludp );
            return LDAP_URL_ERR_MEM;
        }
    }

    if ( q == NULL ) {
        /* no more */
        LDAP_FREE( url );
        *ludpp = ludp;
        return LDAP_URL_SUCCESS;
    }

    /* scan forward for '?' that may marks end of extensions */
    p = q;
    q = strchr( p, '?' );

    if( q != NULL ) {
        /* extra '?' */
        LDAP_FREE( url );
        apr_ldap_free_urldesc( ludp );
        return LDAP_URL_ERR_BADURL;
    }

    /* parse the extensions */
    ludp->lud_exts = ldap_str2charray( p, "," );

    if( ludp->lud_exts == NULL ) {
        LDAP_FREE( url );
        apr_ldap_free_urldesc( ludp );
        return LDAP_URL_ERR_BADEXTS;
    }

    for( i=0; ludp->lud_exts[i] != NULL; i++ ) {
        ldap_pvt_hex_unescape( ludp->lud_exts[i] );

        if( *ludp->lud_exts[i] == '!' ) {
            /* count the number of critical extensions */
            ludp->lud_crit_exts++;
        }
    }

    if( i == 0 ) {
        /* must have 1 or more */
        LDAP_FREE( url );
        apr_ldap_free_urldesc( ludp );
        return LDAP_URL_ERR_BADEXTS;
    }

    /* no more */
    *ludpp = ludp;
    LDAP_FREE( url );
    return LDAP_URL_SUCCESS;
}

APU_DECLARE(int) apr_ldap_url_parse(const char *url_in, 
                                    apr_ldap_url_desc_t **ludpp)
{
    int rc = ldap_url_parse_ext( url_in, ludpp );

    if( rc != LDAP_URL_SUCCESS ) {
        return rc;
    }

    if ((*ludpp)->lud_scope == -1) {
        (*ludpp)->lud_scope = LDAP_SCOPE_BASE;
    }

    if ((*ludpp)->lud_host != NULL && *(*ludpp)->lud_host == '\0') {
        LDAP_FREE( (*ludpp)->lud_host );
        (*ludpp)->lud_host = NULL;
    }

    return rc;
}

APU_DECLARE(void) apr_ldap_free_urldesc(apr_ldap_url_desc_t *ludp)
{
    if ( ludp == NULL ) {
        return;
    }
    
    if ( ludp->lud_scheme != NULL ) {
        LDAP_FREE( ludp->lud_scheme );
    }

    if ( ludp->lud_host != NULL ) {
        LDAP_FREE( ludp->lud_host );
    }

    if ( ludp->lud_dn != NULL ) {
        LDAP_FREE( ludp->lud_dn );
    }

    if ( ludp->lud_filter != NULL ) {
        LDAP_FREE( ludp->lud_filter);
    }

    if ( ludp->lud_attrs != NULL ) {
        LDAP_VFREE( ludp->lud_attrs );
    }

    if ( ludp->lud_exts != NULL ) {
        LDAP_VFREE( ludp->lud_exts );
    }

    LDAP_FREE( ludp );
}


static void ldap_pvt_hex_unescape(char *s)
{
    /*
     * Remove URL hex escapes from s... done in place.  The basic concept for
     * this routine is borrowed from the WWW library HTUnEscape() routine.
     */
    char    *p;

    for ( p = s; *s != '\0'; ++s ) {
        if ( *s == '%' ) {
            if ( *++s == '\0' ) {
                break;
            }
            *p = ldap_pvt_unhex( *s ) << 4;
            if ( *++s == '\0' ) {
                break;
            }
            *p++ += ldap_pvt_unhex( *s );
        } else {
            *p++ = *s;
        }
    }

    *p = '\0';
}


static int ldap_pvt_unhex(int c)
{
    return( c >= '0' && c <= '9' ? c - '0'
        : c >= 'A' && c <= 'F' ? c - 'A' + 10
        : c - 'a' + 10 );
}


static void ldap_charray_free(char **a)
{
    char    **p;

    if ( a == NULL ) {
        return;
    }

    for ( p = a; *p != NULL; p++ ) {
        if ( *p != NULL ) {
            LDAP_FREE( *p );
        }
    }

    LDAP_FREE( (char *) a );
}

static char **ldap_str2charray(const char *str_in, const char *brkstr)
{
    char    **res;
    char    *str, *s;
    char    *lasts;
    int i;

    /* protect the input string from strtok */
    str = LDAP_STRDUP( str_in );
    if( str == NULL ) {
        return NULL;
    }

    i = 1;
    for ( s = str; *s; s++ ) {
        if ( ldap_utf8_strchr( brkstr, s ) != NULL ) {
            i++;
        }
    }

    res = (char **) LDAP_MALLOC( (i + 1) * sizeof(char *) );

    if( res == NULL ) {
        LDAP_FREE( str );
        return NULL;
    }

    i = 0;

    for ( s = ldap_utf8_strtok( str, brkstr, &lasts );
        s != NULL;
        s = ldap_utf8_strtok( NULL, brkstr, &lasts ) )
    {
        res[i] = LDAP_STRDUP( s );

        if(res[i] == NULL) {
            for( --i ; i >= 0 ; i-- ) {
                LDAP_FREE( res[i] );
            }
            LDAP_FREE( res );
            LDAP_FREE( str );
            return NULL;
        }

        i++;
    }

    res[i] = NULL;

    LDAP_FREE( str );
    return( res );
}

#endif /* !APR_HAS_LDAP_URL_PARSE */

#endif /* APR_HAS_LDAP */
