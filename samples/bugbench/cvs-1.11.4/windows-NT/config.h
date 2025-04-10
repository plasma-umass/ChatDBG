/* config.h --- configuration file for Windows NT
   Jim Blandy <jimb@cyclic.com> --- July 1995  */

/* This file lives in the windows-NT subdirectory, which is only included
   in your header search path if you're working under Microsoft Visual C++,
   and use ../cvsnt.mak for your project.  Thus, this is the right place to
   put configuration information for Windows NT.  */

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#undef _ALL_SOURCE

/* Define to empty if the keyword does not work.  */
/* Const is working.  */
#undef const

/* Define to `int' if <sys/types.h> doesn't define.  */
/* Windows NT doesn't have gid_t.  It doesn't even really have group
   numbers, I think.  This will take more thought to get right, but
   let's get it running first.  */
#define gid_t int

/* Define if you support file names longer than 14 characters.  */
/* Yes.  Woo.  */
#define HAVE_LONG_FILE_NAMES 1

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
/* If POSIX.1 requires this, why doesn't WNT have it?  */
#undef HAVE_SYS_WAIT_H

/* Define if utime(file, NULL) sets file's timestamp to the present.  */
/* Experimentation says yes.  Wish I had the full documentation, but
   I have neither the CD-ROM nor a CD-ROM drive to put it in.  */
#define HAVE_UTIME_NULL 1

/* On Windows NT, when a file is being watched, utime expects a file
   to be writable */
#define UTIME_EXPECTS_WRITABLE

/* Define if on MINIX.  */
/* Hah.  */
#undef _MINIX

/* Define to `int' if <sys/types.h> doesn't define.  */
#define mode_t int

/* Define to `int' if <sys/types.h> doesn't define.  */
/* Under Windows NT, we use the process handle as the pid.
   We could #define pid_t to be HANDLE, but that would require
   us to #include <windows.h>, which I don't trust, and HANDLE
   is a pointer type anyway.  */
#define pid_t int

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
/* This string doesn't appear anywhere in the system header files,
   so I assume it's irrelevant.  */
#undef _POSIX_1_SOURCE

/* Define if you need to in order for stat and other things to work.  */
/* Same as for _POSIX_1_SOURCE, above.  */
#undef _POSIX_SOURCE

/* Define as the return type of signal handlers (int or void).  */
/* The manual says they return void.  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* sys/types.h doesn't define it, but stdio.h does, which cvs.h
   #includes, so things should be okay.  */
/* #undef size_t */

/* Define if the `S_IS*' macros in <sys/stat.h> do not work properly.  */
/* We don't seem to have them at all; let ../lib/system.h define them.  */
#define STAT_MACROS_BROKEN 1
 
/* Define if you have the ANSI C header files.  */
/* We'd damn well better.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
/* We don't have <sys/time.h> at all.  Why isn't there a definition
   for HAVE_SYS_TIME_H anywhere in config.h.in?  */
#undef TIME_WITH_SYS_TIME

/* Define to `int' if <sys/types.h> doesn't define.  */
#define uid_t int

/* Define if you have MIT Kerberos version 4 available.  */
/* We don't.  Cygnus says they've ported it to Windows 3.1, but
   I don't know if that means that it works under Windows NT as
   well.  */
#undef HAVE_KERBEROS

/* Define if you want CVS to be able to be a remote repository client.  */
/* We just want the client stuff.  */
#define CLIENT_SUPPORT

/* Define if you want CVS to be able to serve repositories to remote
   clients.  */
/* No server support yet.  Note that you don't have to define
   CLIENT_SUPPORT or SERVER_SUPPORT to enable the non-remote code;
   that's always there.  */
#undef SERVER_SUPPORT

/* Define if you have the connect function.  */
/* Not used?  */
#define HAVE_CONNECT

/* Define if you have the fchdir function.  */
#undef HAVE_FCHDIR

/* Define if you have the fchmod function.  */
#undef HAVE_FCHMOD

/* Define if you have the fsync function.  */
#undef HAVE_FSYNC

/* Define if you have the ftime function.  */
#define HAVE_FTIME 1

/* Define if you have the ftruncate function.  */
#undef HAVE_FTRUNCATE

/* Define if you have the getpagesize function.  */
#undef HAVE_GETPAGESIZE

/* Define if you have the krb_get_err_text function.  */
#undef HAVE_KRB_GET_ERR_TEXT

/* Define if you have the putenv function.  */
#define HAVE_PUTENV 1

/* Define if you have the sigaction function.  */
#undef HAVE_SIGACTION

/* Define if you have the sigblock function.  */
#undef HAVE_SIGBLOCK

/* Define if you have the sigprocmask function.  */
#undef HAVE_SIGPROCMASK

/* Define if you have the sigsetmask function.  */
#undef HAVE_SIGSETMASK

/* Define if you have the sigvec function.  */
#undef HAVE_SIGVEC

/* Define if you have the timezone function.  */
/* Hmm, I actually rather think it's an extern long
   variable; that message was mechanically generated
   by autoconf.  And I don't see any actual uses of
   this function in the code anyway, hmm.  */
#undef HAVE_TIMEZONE

/* Define if you have the usleep function.  */
#define HAVE_USLEEP 1

/* Define if you have the vfork function.  */
#undef HAVE_VFORK

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define if you have the <direct.h> header file.  */
/* Windows NT wants this for mkdir and friends.  */
#define HAVE_DIRECT_H 1

/* Define if you have the <dirent.h> header file.  */
/* No, but we have the <direct.h> header file...  */
#undef HAVE_DIRENT_H

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <io.h> header file.  */
/* Apparently this is where Windows NT declares all the low-level
   Unix I/O routines like open and creat and stuff.  */
#define HAVE_IO_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <ndbm.h> header file.  */
#undef HAVE_NDBM_H

/* Define if you have the <ndir.h> header file.  */
#define HAVE_NDIR_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/bsdtypes.h> header file.  */
#undef HAVE_SYS_BSDTYPES_H

/* Define if you have the <sys/dir.h> header file.  */
#undef HAVE_SYS_DIR_H

/* Define if you have the <sys/ndir.h> header file.  */
#undef HAVE_SYS_NDIR_H

/* Define if you have the <sys/param.h> header file.  */	
#undef HAVE_SYS_PARAM_H

/* Define if you have the <sys/select.h> header file.  */
#undef HAVE_SYS_SELECT_H

/* Define if you have the <sys/time.h> header file.  */
#undef HAVE_SYS_TIME_H

/* Define if you have the <sys/timeb.h> header file.  */
#define HAVE_SYS_TIMEB_H 1

/* Define if you have the <unistd.h> header file.  */
#undef HAVE_UNISTD_H

/* Define if you have the <utime.h> header file.  */
#undef HAVE_UTIME_H

/* Define if you have the inet library (-linet).  */
#undef HAVE_LIBINET

/* Define if you have the nsl library (-lnsl).  */
/* This is not used anywhere in the source code.  */
#undef HAVE_LIBNSL

/* Define if you have the nsl_s library (-lnsl_s).  */
#undef HAVE_LIBNSL_S

/* Define if you have the socket library (-lsocket).  */
/* This isn't ever used either.  */
#undef HAVE_LIBSOCKET

/* Under Windows NT, mkdir only takes one argument.  */
#define CVS_MKDIR wnt_mkdir
extern int wnt_mkdir (const char *PATH, int MODE);
#define CVS_STAT wnt_stat
extern int wnt_stat ();
#define CVS_LSTAT wnt_lstat
extern int wnt_lstat ();

#define CVS_RENAME wnt_rename
extern int wnt_rename (const char *, const char *);

/* This function doesn't exist under Windows NT; we
   provide a stub.  */
extern int readlink (char *path, char *buf, int buf_size);

/* This is just a call to GetCurrentProcessID.  */
extern pid_t getpid (void);

/* We definitely have prototypes.  */
#define USE_PROTOTYPES 1

/* This is just a call to the Win32 Sleep function.  */
unsigned int sleep (unsigned int);
/* So is this */
int usleep (unsigned long);

/* Don't worry, Microsoft, it's okay for these functions to
   be in our namespace.  */
#define popen _popen
#define pclose _pclose

/* When writing binary data to stdout, we better set
   stdout to binary mode using setmode.  */
#define USE_SETMODE_STDOUT 1

/* Diff also has an ifdef for setmode, and it is HAVE_SETMODE.  */
#define HAVE_SETMODE 1

/* Diff needs us to define this.  I think it could always be
   -1 for CVS, because we pass temporary files to diff, but
   config.h seems like the easiest place to put this, so for
   now we put it here.  */
#define same_file(s,t) (-1)

/* This is where old bits go to die under Windows NT.  */
#define DEVNULL "nul"

/* Don't use an rsh subprocess to connect to the server, because
   the rsh does inappropriate translations on the data (CR-LF/LF).  */
#define RSH_NOT_TRANSPARENT 1
extern void wnt_start_server (int *tofd, int *fromfd,
			      char *client_user,
			      char *server_user,
			      char *server_host,
			      char *server_cvsroot);
extern void wnt_shutdown_server (int fd);
#define START_SERVER wnt_start_server
#define SHUTDOWN_SERVER wnt_shutdown_server

#define SYSTEM_INITIALIZE(pargc,pargv) init_winsock()
extern void init_winsock();
#define SYSTEM_CLEANUP() wnt_cleanup()
extern void wnt_cleanup (void);

#define HAVE_WINSOCK_H

/* This tells the client that it must use send()/recv() to talk to the
   server if it is connected to the server via a socket; Win95 needs
   it because _open_osfhandle doesn't work.  */
#define NO_SOCKET_TO_FD 1

/* This tells the client that, in addition to needing to use
   send()/recv() to do socket I/O, the error codes for send()/recv()
   and other socket operations are not available through errno.
   Instead, this macro should be used to obtain an error code. */
#define SOCK_ERRNO (WSAGetLastError ())

/* This tells the client that, in addition to needing to use
   send()/recv() to do socket I/O, the error codes for send()/recv()
   and other socket operations are not known to strerror.  Instead,
   this macro should be used to convert the error codes to strings. */
#define SOCK_STRERROR sock_strerror
extern char *sock_strerror (int errnum);

/* The internal rsh client uses sockets not file descriptors.  Note
   that as the code stands now, it often takes values from a SOCKET and
   puts them in an int.  This is ugly but it seems like sizeof
   (SOCKET) <= sizeof (int) on win32, even the 64-bit variants.  */
#define START_SERVER_RETURNS_SOCKET 1

/* Is this true on NT?  Seems like I remember reports that NT 3.51 has
   problems with 200K writes (of course, the issue of large writes is
   moot since the use of buffer.c ensures that writes will only be as big
   as the buffers).  */
#define SEND_NEVER_PARTIAL 1

/* Force lib/regex.c to use malloc instead of messing around with alloca
   and define the old re_comp routines that we use.  */
#define REGEX_MALLOC 1
#define _REGEX_RE_COMP 1

/* ssize_t not available under Windows */
typedef int ssize_t;

/*
 * When committing a permanent change, CVS and RCS make a log entry of
 * who committed the change.  If you are committing the change logged in
 * as "root" (not under "su" or other root-priv giving program), CVS/RCS
 * cannot determine who is actually making the change.
 *
 * As such, by default, CVS disallows changes to be committed by users
 * logged in as "root".  You can disable this option by commenting
 * out the lines below.
 *
 * Under Windows NT, privileges are associated with groups, not users,
 * so the case in which someone has logged in as root does not occur.
 * Thus, there is no need for this hack.
 *
 * Are we sure this doesn't happen with Administrator? -DRP
 */
#undef CVS_BADROOT

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
#define	TMPDIR_DFLT	"c:\\temp"
#endif

/*
 * The default editor to use, if one does not specify the "-e" option to cvs,
 * or does not have an EDITOR environment variable.  I set this to just "vi",
 * and use the shell to find where "vi" actually is.  This allows sites with
 * /usr/bin/vi or /usr/ucb/vi to work equally well (assuming that your PATH
 * is reasonable).
 *
 * The notepad program seems to be Windows NT's bare-bones text editor.
 */
#ifndef EDITOR_DFLT
#define	EDITOR_DFLT	"notepad"
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
 *
 * Under Windows NT, this must not be used because it tries to include
 * <grp.h>
 */
#ifdef CVS_ADMIN_GROUP
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

/* End of CVS options.h section */

/* The following macros are usually defined by running ./configure under
 * UNIX OSs.
 *
 * FIXME:
 * This should probably be autogenerated somehow when configure is run on other
 * platforms, like some of the Makefiles are.  That way, there is only one
 * place the version string needs to be updated by hand for a new release.
 */
#define PACKAGE_STRING "Concurrent Versions System (CVS) 1.11.4"
