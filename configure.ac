AC_PREREQ([2.64])
# FIXME: Update version info on each release
AC_INIT([libXaw3dXft], [1.6.3], [dave@flaterco.com])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([dist-xz no-dist-gzip subdir-objects -Wall -Werror])

AC_LANG([C])
AC_PROG_CC
m4_version_prereq([2.70], [AC_PROG_LEX(noyywrap)], [AC_PROG_LEX])
AC_PROG_YACC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_REQUIRE_CPP
LT_INIT

# [Unhelpful X.Org boilerplate deleted]
# https://www.x.org/wiki/NewModuleGuidelines/ recommends some boilerplate
# involving XORG_MACROS_VERSION followed by XORG_DEFAULT_OPTIONS.  It messes
# with the compiler flags and defines a lot of macros that are never used.
# Slackware package x/util-macros
# /usr/share/pkgconfig/xorg-macros.pc
# /usr/share/aclocal/xorg-macros.m4

# PKG_CHECK_MODULES is a pkg-config thing.  Testable modules are just
# whatever has a .pc file in PKG_CONFIG_PATH.  See man pkg-config.
PKG_CHECK_MODULES(X11, [xproto x11 xt xmu xext xft])
AC_CHECK_HEADERS([wctype.h wchar.h widec.h])
AC_CHECK_FUNCS([iswspace])
AC_TYPE_INTPTR_T

XAW3D_CPPFLAGS=

AC_ARG_ENABLE([internationalization],
    AS_HELP_STRING([--enable-internationalization], dnl
    [enable utf8 strings (default: yes)]),
    [XAW_I18N=$enableval], [XAW_I18N=yes])
if test "x$XAW_I18N" = xyes; then
    XAW3D_CPPFLAGS="${XAW3D_CPPFLAGS} -DXAW_INTERNATIONALIZATION"
fi
AM_CONDITIONAL(XAW_INTERNATIONALIZATION, [test "x$XAW_I18N" = xyes])

AC_ARG_ENABLE([multiplane-bitmaps],
    [AS_HELP_STRING([--enable-multiplane-bitmaps], [enable XPM support])],
    [],
    [enable_multiplane_bitmaps=no])

AS_IF([test "x$enable_multiplane_bitmaps" != xno],
     [XAW3D_CPPFLAGS="${XAW3D_CPPFLAGS} -DXAW_MULTIPLANE_PIXMAPS" dnl
      PKG_CHECK_MODULES(XPM, xpm)])

AC_ARG_ENABLE([gray-stipples],
    [AS_HELP_STRING([--enable-gray-stipples], [enable gray stipples])],
    [],
    [enable_gray_stipples=no])

AS_IF([test "x$enable_gray_stipples" != xno],
    [XAW3D_CPPFLAGS="${XAW3D_CPPFLAGS} -DXAW_GRAY_BLKWHT_STIPPLES"])

AC_ARG_ENABLE([arrow-scrollbars],
    [AS_HELP_STRING([--enable-arrow-scrollbars], [enable arrow scrollbars])],
    [],
    [enable_arrow_scrollbars=no])

AS_IF([test "x$enable_arrow_scrollbars" != xno],
    [XAW3D_CPPFLAGS="${XAW3D_CPPFLAGS} -DXAW_ARROW_SCROLLBARS"])

AC_SUBST(XAW3D_CPPFLAGS)
AC_CONFIG_FILES([Makefile libxaw3dxft.pc])
AC_OUTPUT
