/* 
 * All configurable options are enabled by using --enable-....
 * when running configure. See configure --help for a list
 * of all available options.
 *
 * You are free to edit this file, but it will be overwritten
 * each time you run configure. You may need to edit this file
 * if configure falsely picks up a library function or structure
 * that doesn't really work on your system.
 *
 * Another way to block a function that should not be detected
 * is to
 * setenv ac_cv_func_<functionname> no
 * before running configure, as in
 * setenv ac_cv_func_setresuid no
 *
 * It is possible to enable some of the configurable options
 * by editing this file alone, but some of them requires changes
 * in the Makefiles, wich is done automatically by configure.
 *
 */
@ TOP @
/* $Id: acconfig.h,v 1.39.2.7 1999/10/05 19:58:44 wessels Exp $ */

/*********************************
 * START OF CONFIGURABLE OPTIONS *
 *********************************/
/*
 * If you are upset that the cachemgr.cgi form comes up with the hostname
 * field blank, then define this to getfullhostname()
 */
#undef CACHEMGR_HOSTNAME

/* Define to do simple malloc debugging */
#undef XMALLOC_DEBUG

/* Define for log file trace of mem alloc/free */
#undef MEM_GEN_TRACE

/* Define to have malloc statistics */
#undef XMALLOC_STATISTICS

/* Define to have a detailed trace of memory allocations */
#undef XMALLOC_TRACE

#undef FORW_VIA_DB

/* Define to use async disk I/O operations */
#undef USE_ASYNC_IO

/* Defines how many threads to use for async I/O */
#undef NUMTHREADS

/*
 * If you want to use Squid's ICMP features (highly recommended!) then
 * define this.  When USE_ICMP is defined, Squid will send ICMP pings
 * to origin server sites.  This information is used in numerous ways:
 *         - Sent in ICP replies so neighbor caches know how close
 *           you are to the source.
 *         - For finding the closest instance of a URN.
 *         - With the 'test_reachability' option.  Squid will return
 *           ICP_OP_MISS_NOFETCH for sites which it cannot ping.
 */
#undef USE_ICMP

/*
 * Traffic management via "delay pools".
 */
#undef DELAY_POOLS

/*
 * If you want to log User-Agent request header values, define this.
 * By default, they are written to useragent.log in the Squid log
 * directory.
 */
#undef USE_USERAGENT_LOG

/*
 * A dangerous feature which causes Squid to kill its parent process
 * (presumably the RunCache script) upon receipt of SIGTERM or SIGINT.
 * Use with caution.
 */
#undef KILL_PARENT_OPT

/* Define to enable SNMP monitoring of Squid */
#undef SQUID_SNMP

/*
 * Define to enable WCCP
 */
#define USE_WCCP 1

/*
 * Squid frequently calls gettimeofday() for accurate timestamping.
 * If you are concerned that gettimeofday() is called too often, and
 * could be causing performance degradation, then you can define
 * ALARM_UPDATES_TIME and cause Squid's clock to be updated at regular
 * intervals (one second) with ALARM signals.
 */
#undef ALARM_UPDATES_TIME

/*
 * Define this to include code which lets you specify access control
 * elements based on ethernet hardware addresses.  This code uses
 * functions found in 4.4 BSD derviations (e.g. FreeBSD, ?).
 */
#undef USE_ARP_ACL

/*
 * Define this to include code for the Hypertext Cache Protocol (HTCP)
 */
#undef USE_HTCP

/*
 * Use Cache Digests for locating objects in neighbor caches.  This
 * code is still semi-experimental. 
 */
#undef USE_CACHE_DIGESTS

/*
 * Cache Array Routing Protocol
 */
#undef USE_CARP

/********************************
 *  END OF CONFIGURABLE OPTIONS *
 ********************************/

/* Define if struct tm has tm_gmtoff member */
#undef HAVE_TM_GMTOFF

/* Define if struct mallinfo has mxfast member */
#undef HAVE_EXT_MALLINFO

/* Default FD_SETSIZE value */
#undef DEFAULT_FD_SETSIZE

/* Maximum number of open filedescriptors */
#undef SQUID_MAXFD

/* UDP send buffer size */
#undef SQUID_UDP_SO_SNDBUF

/* UDP receive buffer size */
#undef SQUID_UDP_SO_RCVBUF

/* TCP send buffer size */
#undef SQUID_TCP_SO_SNDBUF

/* TCP receive buffer size */
#undef SQUID_TCP_SO_RCVBUF

/* Host type from configure */
#undef CONFIG_HOST_TYPE

/* If we need to declare sys_errlist[] as external */
#undef NEED_SYS_ERRLIST

/* If gettimeofday is known to take only one argument */
#undef GETTIMEOFDAY_NO_TZP

/* If libresolv.a has been hacked to export _dns_ttl_ */
#undef LIBRESOLV_DNS_TTL_HACK

/* Define if struct ip has ip_hl member */
#undef HAVE_IP_HL

/* Define if your compiler supports prototyping */
#undef HAVE_ANSI_PROTOTYPES

/* Define if we should use GNU regex */
#undef USE_GNUREGEX

/* signed size_t, grr */
#undef ssize_t

/*
 * Yay! Another Linux brokenness.  Its not good enough to know that
 * setresuid() exists, because RedHat 5.0 declare setresuid() but
 * doesn't implement it.
 */
#undef HAVE_SETRESUID

/* Define if you have struct rusage */
#undef HAVE_STRUCT_RUSAGE

/*
 * This makes warnings go away.  If you have socklen_t defined in your
 * /usr/include files, then this should remain undef'd.  Otherwise it
 * should be defined to int.
 */
#undef socklen_t

/*
 * By default (for now anyway) Squid includes options which allows
 * the cache administrator to violate the HTTP protocol specification
 * in terms of cache behaviour.  Setting this to '0' will disable
 * such code.
 */
#define HTTP_VIOLATIONS 1

/*
 * Enable support for Transparent Proxy on systems using IP-Filter
 * address redirection. This provides "masquerading" support for non
 *  Linux system.
 */
#undef IPF_TRANSPARENT

/*
 * Enable code for assiting in finding memory leaks.  Hacker stuff only.
 */
#undef USE_LEAKFINDER

/*
 * type of fd_set array
 */
#undef fd_mask

/*
 * If _res structure has nsaddr_list member
 */
#undef HAVE_RES_NSADDR_LIST

/*
 * If _res structure has ns_list member
 */
#undef HAVE_RES_NS_LIST

/*
 * Compile in support for Ident (RFC 931) lookups?  Enabled by default.
 */
#define USE_IDENT 1

/*
 * If your system has statvfs(), and if it actually works!
 */
#undef HAVE_STATVFS

/*
 * If --disable-internal-dns was given to configure, then we'll use
 * the dnsserver processes instead.
 */
#undef USE_DNSSERVERS

/*
 * we check for the existance of struct mallinfo
 */
#undef HAVE_STRUCT_MALLINFO

/*
 * Do we want to use truncate(2) or unlink(2)?
 */
#undef USE_TRUNCATE

/*
 * Allow underscores in host names
 */
#undef ALLOW_HOSTNAME_UNDERSCORES

/*
 * Use the heap-based replacement techniques
 */
#undef HEAP_REPLACEMENT
