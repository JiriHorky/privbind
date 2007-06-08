#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([privbind],[0.2])
AM_INIT_AUTOMAKE
AC_COPYRIGHT([Copyright (C) 2006-2007 Shachar Shemesh])

# Checks for programs.
AC_PROG_CC

AC_DISABLE_STATIC # don't build static libraries by default
AC_PROG_LIBTOOL

# Checks for libraries.
# Checks for header files.
# Checks for typedefs, structures, and compiler characteristics.
# Checks for library functions.

sock_fd_num=497
AC_MSG_CHECKING([what default fd to use for parent-child communication])
AC_ARG_WITH([comm-socket],AC_HELP_STRING([--with-comm-socket=num],[Set the communication socket number]),
[
[if [ "$(($withval))" -eq "$withval" ]]
then
    sock_fd_num="$withval"
else
    AC_MSG_ERROR([Invalid comm-socket value passed])
fi])
AC_MSG_RESULT($sock_fd_num)
AC_DEFINE_UNQUOTED(COMM_SOCKET, $sock_fd_num, [Communication socket between parent and child])

AC_MSG_CHECKING([whether to enable internal test-only features])
AC_ARG_ENABLE([testing],AC_HELP_STRING([--enable-testing],
   [Enable certain self test features]),
   [
      AC_MSG_RESULT([yes])
      AC_DEFINE(DEBUG_TESTING,1,[Options for internal sanity testing. Do not enable for distributed builds])
   ],
   [AC_MSG_RESULT([no])] )

AC_DEFINE(PRELOADLIBNAME, "libprivbind-0.so", [Name for the preload library])
AC_SUBST(PRELOADLIBNAME, "libprivbind-0.so")

AC_CONFIG_FILES(Makefile)
AM_CONFIG_HEADER(config.h)
AC_OUTPUT
