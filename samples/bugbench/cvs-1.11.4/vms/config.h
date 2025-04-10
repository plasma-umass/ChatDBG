/* config.h - OpenVMS/AXP specific configuration
              June 1995 - <benjamin@cyclic.com> */

/* We only want to build the client */
#define CLIENT_SUPPORT 1
#undef SERVER_SUPPORT

/* Set up for other #if's which follow */
#ifndef __DECC_VER
#define __DECC_VER  0
#endif
#ifndef __VMS_VER
#define __VMS_VER   0
#endif

/* VMS is case insensitive */
/* #define FOLD_FN_CHAR(c) tolower(c) */

/* Temporary files named "#booger.3.6~" aren't legal under VMS,
   Define this if you want to use names which are legal for VMS */
#define USE_VMS_FILENAMES 1

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you have <dirent.h>.  */
/* #undef DIRENT */

/* Define if you have <sys/param.h> */
/* #undef HAVE_SYS_PARAM_H */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef gid_t */

/* Define if you support file names longer than 14 characters.  */
/* #undef HAVE_LONG_FILE_NAMES */

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
/* #define HAVE_SYS_WAIT_H 1 OpenVMS POSIX has it, but VMS does not. */
#undef POSIX

/* Define if utime(file, NULL) sets file's timestamp to the present.  */
/* #undef HAVE_UTIME_NULL */

/* Define if on MINIX.  */
/* #undef _MINIX */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef mode_t */

/* Define if you don't have <dirent.h>, but have <ndir.h>.  */
#define HAVE_NDIR_H 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
/* #undef _POSIX_1_SOURCE */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you don't have <dirent.h>, but have <sys/dir.h>.  */
/* #undef SYSDIR */

/* Define if you don't have <dirent.h>, but have <sys/ndir.h>.  */
/* #undef SYSNDIR */

/* Define if your <sys/time.h> declares struct tm.  */
/* #undef TM_IN_SYS_TIME */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef uid_t */

/* Define if the closedir function returns void instead of int.  */
/* #undef VOID_CLOSEDIR */

/* Define if you have MIT Kerberos version 4 available.  */
/* #undef HAVE_KERBEROS */

/* Define if you have the fchmod function.  */
/* #undef HAVE_FCHMOD */

/* Define if you have the fsync function.  */
/* #undef HAVE_FSYNC */

/* Define if you have the ftime function.  */
/* #undef HAVE_FTIME */

/* Define if you have the ftruncate function.  */
/* #undef HAVE_FTRUNCATE */

/* Define if you have the getpagesize function.  */
/* #undef HAVE_GETPAGESIZE */

/* Define if you have the krb_get_err_text function.  */
/* #undef HAVE_KRB_GET_ERR_TEXT */

/* Define if you have the mkdir function */
#define HAVE_MKDIR 1

/* Define if you have the rmdir function */
#define HAVE_RMDIR 1

/* Define if you have the rename function */
#define HAVE_RENAME 1

/* Define if you have the putenv function.  */
/* #undef HAVE_PUTENV */

/* Define if you have the timezone function.  */
/* #undef HAVE_TIMEZONE */

/* Define if you have the vfork function.  */
#define HAVE_VFORK

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF

/* Define if you have the <errno.h> header file.  */
/* #undef HAVE_ERRNO_H */

/* Define if you have the <fcntl.h> header file.  */
#if __DECC_VER >= 50700000
# define HAVE_FCNTL_H 1
#endif

/* Define if you have the <memory.h> header file.  */
/* #undef HAVE_MEMORY_H */

/* Define if you have the <ndbm.h> header file.  */
/* #undef HAVE_NDBM_H */

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define to force lib/regex.c to use malloc instead of alloca.  */
#define REGEX_MALLOC 1

/* Define to force lib/regex.c to define re_comp et al.  */
#define _REGEX_RE_COMP 1

/* Define if you have the <sys/select.h> header file.  */
/* #undef HAVE_SYS_SELECT_H */

/* Define this if your <sys/socket.h> defines select() */
#define SYS_SOCKET_H_DEFINES_SELECT 1

/* Define if you have the <sys/timeb.h> header file.  */
#define HAVE_SYS_TIMEB_H 1
#define HAVE_TIMEB_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the <utime.h> header file.  */
/* #undef HAVE_UTIME_H */

/* Define if you have the nsl library (-lnsl).  */
/* #undef HAVE_LIBNSL */

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

/* Under Windows NT, filenames are case-insensitive, and both / and \
   are path component separators.  */
#define FOLD_FN_CHAR(c) (VMS_filename_classes[(unsigned char) (c)])
extern unsigned char VMS_filename_classes[];
#define FILENAMES_CASE_INSENSITIVE 1

/* Like strcmp, but with the appropriate tweaks for file names.
   Under Windows NT, filenames are case-insensitive but case-preserving,
   and both \ and / are path element separators.  */
extern int fncmp (const char *n1, const char *n2);

/* Fold characters in FILENAME to their canonical forms.  
   If FOLD_FN_CHAR is not #defined, the system provides a default
   definition for this.  */
extern void fnfold (char *FILENAME);

#define RSH_NOT_TRANSPARENT 1
#define START_SERVER vms_start_server
#define NO_SOCKET_TO_FD 1
#define START_SERVER_RETURNS_SOCKET 1
#define SEND_NEVER_PARTIAL 1
#define SYSTEM_GETCALLER() getlogin ()
#define GETPWNAM_MISSING 1

/* Avoid name conflicts with VMS libraries.  */
#define getopt cvs_getopt
#define optind cvs_optind
#define optopt cvs_optopt
#define optarg cvs_optarg
#define opterr cvs_opterr

/* Avoid open/read/closedir name conflicts with DEC C 5.7 libraries,
   and fix the problem with readdir() retaining the trailing period.  */
#define CVS_OPENDIR  vms_opendir
#define CVS_READDIR  vms_readdir
#define CVS_CLOSEDIR vms_closedir

/* argv[0] in VMS is the full pathname which would look really ugly in error
   messages.  Even if we stripped out the directory and ".EXE;5", it would
   still be misleading, as if one has used "OLDCVS :== ...CVS-JULY.EXE",
   then argv[0] does not contain the name of the command which the user
   invokes CVS with.  If there is a way for VMS to find the latter, that
   might be worth messing with, but it also seems fine to just always call
   it "cvs".  */
#define ARGV0_NOT_PROGRAM_NAME

#define CVS_UNLINK vms_unlink

/* There is some pretty unixy code in src/commit.c which tries to
   prevent people from commiting changes as "root" (which would prevent
   CVS from making a log entry with the actual user).  On VMS, I suppose
   one could say that SYSTEM is equivalent, but I would think that it
   actually is not necessary; at least at the VMS sites I've worked at
   people just used their own accounts (turning privileges on and off
   as desired).  */
#undef	CVS_BADROOT

#define NO_SOCKET_TO_FD 1

/*
 * The following configuration options used to be defined in options.h.
 */

/*
 * For portability and heterogeneity reasons, CVS is shipped by default using
 * my own text-file version of the ndbm database library in the src/myndbm.c
 * file.  If you want better performance and are not concerned about
 * heterogeneous hosts accessing your modules file, turn this option off.
 */
#ifndef MY_NDBM
#define	MY_NDBM
#endif

/* Directory used for storing temporary files, if not overridden by
   environment variables or the -T global option.  There should be little
   need to change this (-T is a better mechanism if you need to use a
   different directory for temporary files).  */
#ifndef TMPDIR_DFLT
#define	TMPDIR_DFLT	"sys$scratch"
#endif

/*
 * The default editor to use, if one does not specify the "-e" option to cvs,
 * or does not have an EDITOR environment variable.  I set this to just "vi",
 * and use the shell to find where "vi" actually is.  This allows sites with
 * /usr/bin/vi or /usr/ucb/vi to work equally well (assuming that your PATH
 * is reasonable).
 */
#ifndef EDITOR_DFLT
#define	EDITOR_DFLT	""
#endif

/*
 * The default umask to use when creating or otherwise setting file or
 * directory permissions in the repository.  Must be a value in the
 * range of 0 through 0777.  For example, a value of 002 allows group
 * rwx access and world rx access; a value of 007 allows group rwx
 * access but no world access.  This value is overridden by the value
 * of the CVSUMASK environment variable, which is interpreted as an
 * octal number.
 */
#ifndef UMASK_DFLT
#define	UMASK_DFLT	002
#endif

/*
 * The cvs admin command is restricted to the members of the group
 * CVS_ADMIN_GROUP.  If this group does not exist, all users are
 * allowed to run cvs admin.  To disable the cvs admin for all users,
 * create an empty group CVS_ADMIN_GROUP.  To disable access control for
 * cvs admin, comment out the define below.
 */
#ifndef CVS_ADMIN_GROUP
/* #define CVS_ADMIN_GROUP "cvsadmin" */
#endif

/*
 * When committing or importing files, you must enter a log message.
 * Normally, you can do this either via the -m flag on the command line or an
 * editor will be started for you.  If you like to use logging templates (the
 * rcsinfo file within the $CVSROOT/CVSROOT directory), you might want to
 * force people to use the editor even if they specify a message with -m.
 * Enabling FORCE_USE_EDITOR will cause the -m message to be appended to the
 * temp file when the editor is started.
 */
#ifndef FORCE_USE_EDITOR
/* #define 	FORCE_USE_EDITOR */
#endif

/*
 * Yes, we can do the authenticated client.
 */
#define AUTH_CLIENT_SUPPORT 1

/*
 * If you are working with a large remote repository and a 'cvs checkout' is
 * swamping your network and memory, define these to enable flow control.
 * You will end up with even less guarantees of a consistant checkout,
 * but that may be better than no checkout at all.  The master server process
 * will monitor how far it is getting behind, if it reaches the high water
 * mark, it will signal the child process to stop generating data when
 * convenient (ie: no locks are held, currently at the beginning of a 
 * new directory).  Once the buffer has drained sufficiently to reach the
 * low water mark, it will be signalled to start again.
 * -- EXPERIMENTAL! --  A better solution may be in the works.
 * You may override the default hi/low watermarks here too.
 */
#ifndef SERVER_FLOWCONTROL
/* #define SERVER_FLOWCONTROL */
/* #define SERVER_HI_WATER (2 * 1024 * 1024) */
/* #define SERVER_LO_WATER (1 * 1024 * 1024) */
#endif

/* End of CVS options.h section */

#include "vms.h"
