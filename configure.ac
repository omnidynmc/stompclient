#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([stomp], [1.0.0.1], [gcarter@openaprs.net])
AC_CONFIG_SRCDIR([src/StompPeer.cpp])
AC_CONFIG_HEADERS([include/stomp/config.h])
AM_INIT_AUTOMAKE([stomp], [1.0.0.1])
LT_INIT
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_LIBTOOL

AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_LANG([C++])

prefix="`pwd`"
bindir="`pwd`"
libdir="`pwd`/lib"
includedir="`pwd`/include"
ofincludedir="`pwd`/libraries/openframe/include"
oflibdir="`pwd`/libraries/openframe/lib"

osincludedir="`pwd`/libraries/openstats/include"
oslibdir="`pwd`/libraries/openstats/lib"

CPPFLAGS="$CPPFLAGS -I/usr/include"
CXXFLAGS="-Wall -pipe -g -I$includedir -I$ofincludedir -I$osincludedir"
CXX="g++"
LIBS="$LIBS"

# Checks for libraries.

AC_CHECK_LIB([pthread], [pthread_create], [], [
               echo "pthread library is required for this program"
               exit 1
             ])

AC_CHECK_LIB([ossp-uuid++], [uuid_version], [], [
               echo "ossp-uuid++ library is required for this program"
               exit 1
             ])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_SUBST(bindir)
AC_SUBST(LIBTOOL_DEPS)

LIBS="$LIBS -L`pwd`/lib $DEPS_LIBS -L$oflibdir -L$oslibdir -lopenframe -lopenstats"

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile
               ])
AC_OUTPUT
