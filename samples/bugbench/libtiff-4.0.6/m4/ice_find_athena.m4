dnl $Id: ice_find_athena.m4,v 1.1 2006-10-13 10:13:31 dron Exp $
dnl 
dnl ice_find_athena.m4
dnl 
dnl -----------------------------------------------------------
dnl 
dnl  * To: autoconf-collection@peti.gmd.de
dnl  * Subject: ice_find_athena.m4
dnl  * From: Andreas Zeller 
dnl  * Date: Sat, 29 Nov 1997 17:14:09 +0100 (MET)
dnl  * Sender: autoconf-collection-owner@peti.gmd.de
dnl 
dnl Modified by Andrey Kiselev <dron@ak4719.spb.edu>, 2006.
dnl
dnl -----------------------------------------------------------
dnl 
dnl ICE_FIND_ATHENA
dnl ---------------
dnl
dnl Find Athena libraries and headers.
dnl Put Athena include directory in athena_includes,
dnl put Athena library directory in athena_libraries,
dnl and set appropriate flags to XAW_CFLAGS and XAW_LIBS.
dnl
dnl
AC_DEFUN([ICE_FIND_ATHENA],
[
AC_REQUIRE([AC_PATH_XTRA])
athena_includes=
athena_libraries=
AC_ARG_WITH(athena,
            AS_HELP_STRING([--without-athena], [do not use Athena widgets]))
dnl Treat --without-athena like
dnl --without-athena-includes --without-athena-libraries.
if test "$with_athena" = "no"
then
athena_includes=no
athena_libraries=no
fi
AC_ARG_WITH(athena-includes,
            AS_HELP_STRING([--with-athena-includes=DIR],
	                   [Athena include files are in DIR]),
	    [athena_includes="$withval"])
AC_ARG_WITH(athena-libraries,
            AS_HELP_STRING([--with-athena-libraries=DIR],
	                   [Athena libraries are in DIR]),
	    [athena_libraries="$withval"])
AC_MSG_CHECKING(for Athena)
#
#
# Search the include files.
#
if test "$athena_includes" = ""; then
AC_CACHE_VAL(ice_cv_athena_includes,
[
ice_athena_save_LIBS="$LIBS"
ice_athena_save_CFLAGS="$CFLAGS"
ice_athena_save_CPPFLAGS="$CPPFLAGS"
ice_athena_save_LDFLAGS="$LDFLAGS"
#
LIBS="$X_PRE_LIBS -lXaw -lXmu -lXext -lXt -lX11 $X_EXTRA_LIBS $LIBS"
CFLAGS="$X_CFLAGS $CFLAGS"
CPPFLAGS="$X_CFLAGS $CPPFLAGS"
LDFLAGS="$X_LIBS $LDFLAGS"
#
AC_TRY_COMPILE([
#include <X11/Intrinsic.h>
#include <X11/Xaw/Text.h>
],[int a;],
[
# X11/Xaw/Text.h is in the standard search path.
ice_cv_athena_includes=
],
[
# X11/Xaw/Text.h is not in the standard search path.
# Locate it and put its directory in `athena_includes'
#
# /usr/include/Motif* are used on HP-UX (Motif).
# /usr/include/X11* are used on HP-UX (X and Athena).
# /usr/dt is used on Solaris (Motif).
# /usr/openwin is used on Solaris (X and Athena).
# Other directories are just guesses.
for dir in "$x_includes" "${prefix}/include" /usr/include /usr/local/include \
           /usr/include/Motif2.0 /usr/include/Motif1.2 /usr/include/Motif1.1 \
           /usr/include/X11R6 /usr/include/X11R5 /usr/include/X11R4 \
           /usr/dt/include /usr/openwin/include \
           /usr/dt/*/include /opt/*/include /usr/include/Motif* \
           "${prefix}"/*/include /usr/*/include /usr/local/*/include \
           "${prefix}"/include/* /usr/include/* /usr/local/include/*; do
if test -f "$dir/X11/Xaw/Text.h"; then
ice_cv_athena_includes="$dir"
break
fi
done
])
#
LIBS="$ice_athena_save_LIBS"
CFLAGS="$ice_athena_save_CFLAGS"
CPPFLAGS="$ice_athena_save_CPPFLAGS"
LDFLAGS="$ice_athena_save_LDFLAGS"
])
athena_includes="$ice_cv_athena_includes"
fi
#
#
# Now for the libraries.
#
if test "$athena_libraries" = ""; then
AC_CACHE_VAL(ice_cv_athena_libraries,
[
ice_athena_save_LIBS="$LIBS"
ice_athena_save_CFLAGS="$CFLAGS"
ice_athena_save_CPPFLAGS="$CPPFLAGS"
ice_athena_save_LDFLAGS="$LDFLAGS"
#
LIBS="$X_PRE_LIBS -lXaw -lXmu -lXext -lXt -lX11 $X_EXTRA_LIBS $LIBS"
CFLAGS="$X_CFLAGS $CFLAGS"
CPPFLAGS="$X_CFLAGS $CPPFLAGS"
LDFLAGS="$X_LIBS $LDFLAGS"
#
AC_TRY_LINK([
#include <X11/Intrinsic.h>
#include <X11/Xaw/Text.h>
],[XtToolkitInitialize();],
[
# libXaw.a is in the standard search path.
ice_cv_athena_libraries="yes"
],
[
ice_cv_athena_libraries="no"
])
#
LIBS="$ice_athena_save_LIBS"
CFLAGS="$ice_athena_save_CFLAGS"
CPPFLAGS="$ice_athena_save_CPPFLAGS"
LDFLAGS="$ice_athena_save_LDFLAGS"
])
#
  if test "${ice_cv_athena_libraries}" = "no" ; then
    athena_libraries="no"
  fi
fi

# Set Athena definitions
#
if test "$athena_includes" != "" \
	-a "$athena_includes" != "$x_includes" \
	-a "$athena_includes" != "no" ; then
  XAW_CFLAGS="-I$athena_includes"
else
  XAW_CFLAGS=""
fi

if test "$athena_libraries" != "" \
	-a "$athena_libraries" != "$x_libraries" \
	-a "$athena_libraries" != "no" ; then
  XAW_LIBS="-L$athena_libraries -lXaw"
dnl  case "$X_LIBS" in
dnl    *-R\ *) X_LIBS="-L$athena_libraries -R $athena_libraries $X_LIBS";;
dnl    *-R*)   X_LIBS="-L$athena_libraries -R$athena_libraries $X_LIBS";;
dnl    *)      X_LIBS="-L$athena_libraries $X_LIBS";;
dnl  esac
else
  XAW_LIBS="-lXaw"
fi
#

if test "x${athena_libraries}" = x"no" ; then
  no_xaw="yes"
fi

athena_libraries_result="$athena_libraries"
athena_includes_result="$athena_includes"
test "$athena_libraries_result" = "" &&
  athena_libraries_result="in default path"
test "$athena_includes_result" = "" &&
  athena_includes_result="in default path"
test "$athena_libraries_result" = "no" &&
  athena_libraries_result="(none)"
test "$athena_includes_result" = "no" &&
  athena_includes_result="(none)"
AC_MSG_RESULT(
  [libraries $athena_libraries_result, headers $athena_includes_result])
AC_SUBST([XAW_CFLAGS])
AC_SUBST([XAW_LIBS])
])dnl
nn
