/*
 * yppasswdd
 * Copyright 1994, 1995, 1996 Olaf Kirch, <okir@monad.swb.de>
 *
 * This program is covered by the GNU General Public License, version 2.
 * It is provided in the hope that it is useful. However, the author
 * disclaims ALL WARRANTIES, expressed or implied. See the GPL for details.
 *
 * This file was generated automatically by rpcgen from yppasswd.x, and
 * editied manually.
 */

#ifndef _YPPASSWD_H_
#define _YPPASSWD_H_

#define YPPASSWDPROG ((u_long)100009)
#define YPPASSWDVERS ((u_long)1)
#define YPPASSWDPROC_UPDATE ((u_long)1)

/*
 * The password struct passed by the update call. I renamed it to
 * xpasswd to avoid a type clash with the one defined in <pwd.h>.
 */
#ifndef __sgi
typedef struct xpasswd {
	char *		pw_name;
	char *		pw_passwd;
	int		pw_uid;
	int		pw_gid;
	char *		pw_gecos;
	char *		pw_dir;
	char *		pw_shell;
} xpasswd;
#else
# include <pwd.h>
typedef struct xpasswd	xpasswd;
#endif

/* The updated password information, plus the old password.
 */
typedef struct yppasswd {
	char *oldpass;
	xpasswd newpw;
} yppasswd;

/* XDR encoding/decoding routines */
bool_t xdr_xpasswd (XDR *xdrs, xpasswd *objp);
bool_t xdr_yppasswd(XDR *xdrs, yppasswd *objp);

/* The server procedure invoked by the main loop. */
void   yppasswdprog_1(struct svc_req *rqstp, SVCXPRT *transp);

/* Handlers for the update RPC call, one for normal passwd files, and
 * one for shadow passwords.
 */
int *  yppasswdproc_pwupdate_1(yppasswd *yppw, struct svc_req *rqstp);
int *  yppasswdproc_spwupdate_1(yppasswd *yppw, struct svc_req *rqstp);

/*
 * Command-line options to yppasswdd.
 */
extern int	allow_chsh;
extern int	allow_chfn;
extern int	use_shadow;
extern int      x_flag;
extern char	*path_passwd;
extern char 	*path_passwd_tmp;
extern char 	*path_passwd_old;
extern char	*path_shadow;
extern char 	*path_shadow_tmp;
extern char 	*path_shadow_old;
extern char     *external_update_program;

/* This command is forked to rebuild the NIS maps after a successful
 * update. MAP_UPDATE0 is used as argv[0].
 */
#define MAP_UPDATE		"pwupdate"
#define MAP_UPDATE_PATH		YPBINDIR "/" MAP_UPDATE

#endif _YPPASSWD_H_
