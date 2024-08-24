#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([tex3ds], [2.0.1], [https://github.com/devkitPro/tex3ds/issues])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([source/tex3ds.cpp])

AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CXX

AC_LANG_PUSH([C++])
AX_APPEND_COMPILE_FLAGS([-Wall -pthread -flto -pipe])
AX_CXX_COMPILE_STDCXX_11(noext, mandatory)
AC_LANG_POP()

AC_CHECK_PROGS([DOXYGEN], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([Doxyfile])])

AC_CHECK_PROGS([DOT], [dot])
AM_CONDITIONAL([HAVE_DOT], [test -n "$DOT"])
AM_COND_IF([HAVE_DOT], AC_SUBST(HAVE_DOT, ["YES"], AC_SUBST(HAVE_DOT, ["NO"])))

AC_DEFUN([PKG_CHECK_MODULES_STATIC],
[AC_REQUIRE([PKG_PROG_PKG_CONFIG])dnl
_save_PKG_CONFIG=$PKG_CONFIG
PKG_CONFIG="$PKG_CONFIG --static"
PKG_CHECK_MODULES([$1], [$2], [$3], [$4], [$5])
PKG_CONFIG=$_save_PKG_CONFIG[]dnl
])

# Checks for libraries.
PKG_CHECK_MODULES_STATIC(FreeType, [freetype2])
PKG_CHECK_MODULES_STATIC(ImageMagick, [Magick++ >= 6.0.0])

# Checks for header files.
AC_CHECK_HEADERS([unistd.h])

# Checks for library functions.
AC_CHECK_FUNCS([strcasecmp])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
