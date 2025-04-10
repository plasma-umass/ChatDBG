#ifndef APR_PRIVATE_H
#define APR_PRIVATE_H

@TOP@

/* Various #defines we need to know about */
#undef USE_THREADS
#undef EGD_DEFAULT_SOCKET
#undef HAVE_isascii
#undef DIRENT_INODE
#undef DIRENT_TYPE

/* Cross process serialization techniques */
#undef USE_FLOCK_SERIALIZE
#undef USE_SYSVSEM_SERIALIZE
#undef USE_FCNTL_SERIALIZE
#undef USE_PROC_PTHREAD_SERIALIZE
#undef USE_PTHREAD_SERIALIZE

#undef POSIXSEM_IS_GLOBAL
#undef SYSVSEM_IS_GLOBAL
#undef FCNTL_IS_GLOBAL
#undef FLOCK_IS_GLOBAL

#undef HAVE_INT64_C

@BOTTOM@

/* Make sure we have ssize_t defined to be something */
#undef ssize_t

/* switch this on if we have a BeOS version below BONE */
#if BEOS && !HAVE_BONE_VERSION
#define BEOS_R5 1
#else
#define BEOS_BONE 1
#endif

#ifdef SIGWAIT_TAKES_ONE_ARG
#define apr_sigwait(a,b) ((*(b)=sigwait((a)))<0?-1:0)
#else
#define apr_sigwait(a,b) sigwait((a),(b))
#endif

/*
 * Include common private declarations.
 */
#include "../apr_private_common.h"

#endif /* APR_PRIVATE_H */
