## ---------------------------------- ##
## Check if --with-efence was given.  ##
## ---------------------------------- ##

# serial 1

AC_DEFUN(AM_WITH_EFENCE,
[AC_MSG_CHECKING(if malloc debugging with efence is wanted)
AC_ARG_WITH(efence,
[  --with-efence           use ElectricFence],
[if test "$withval" = yes; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(WITH_EFENCE,1,
            [Define if using the ElectricFence debugging malloc package])
  LIBS="$LIBS -lefence"
  LDFLAGS="$LDFLAGS -g"
  CFLAGS="-g"
else
  AC_MSG_RESULT(no)
fi], [AC_MSG_RESULT(no)])
])
