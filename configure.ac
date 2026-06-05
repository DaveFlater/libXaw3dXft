AC_PREREQ([2.64])

# Update these numbers on every release
m4_define([LVT_VERSION_MAJOR], [1])
m4_define([LVT_VERSION_MINOR], [6])
m4_define([LVT_VERSION_PATCH], [4])

# -------
m4_define([LVT_VERSION], LVT_VERSION_MAJOR.LVT_VERSION_MINOR.LVT_VERSION_PATCH)
m4_define([LVT_VERSION_STR], \"LVT_VERSION\")

AC_INIT([libXaw3dXft], m4_defn([LVT_VERSION]), [dave@flaterco.com])
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

AC_ARG_ENABLE([internationalization],
  AS_HELP_STRING([--enable-internationalization], [enable utf8 strings (default: yes)]),
  [],
  [enable_internationalization=yes])
AS_IF([test "x$enable_internationalization" != xno],
  [AS_VAR_SET([CONFIG_INTERNATIONALIZATION], ["#define XAW_INTERNATIONALIZATION 1"])],
  [AS_VAR_SET([CONFIG_INTERNATIONALIZATION], ["#undef XAW_INTERNATIONALIZATION"])])
AM_CONDITIONAL(XAW_INTERNATIONALIZATION, [test "x$enable_internationalization" != xno])

AC_ARG_ENABLE([multiplane-bitmaps],
  [AS_HELP_STRING([--enable-multiplane-bitmaps], [enable XPM support])],
  [],
  [enable_multiplane_bitmaps=no])
AS_IF([test "x$enable_multiplane_bitmaps" != xno],
  [AS_VAR_SET([CONFIG_MULTIPLANE_PIXMAPS], ["#define XAW_MULTIPLANE_PIXMAPS 1"])
   PKG_CHECK_MODULES(XPM, xpm)],
  [AS_VAR_SET([CONFIG_MULTIPLANE_PIXMAPS], ["#undef XAW_MULTIPLANE_PIXMAPS"])])

AC_ARG_ENABLE([arrow-scrollbars],
  [AS_HELP_STRING([--enable-arrow-scrollbars], [enable arrow scrollbars])],
  [],
  [enable_arrow_scrollbars=no])
AS_IF([test "x$enable_arrow_scrollbars" != xno],
  [AS_VAR_SET([CONFIG_ARROW_SCROLLBARS], ["#define XAW_ARROW_SCROLLBARS 1"])],
  [AS_VAR_SET([CONFIG_ARROW_SCROLLBARS], ["#undef XAW_ARROW_SCROLLBARS"])])

AC_ARG_ENABLE([gray-stipples],
  [AS_HELP_STRING([--enable-gray-stipples], [enable gray stipples])],
  [],
  [enable_gray_stipples=no])
AS_IF([test "x$enable_gray_stipples" != xno],
  [AS_VAR_SET([CONFIG_GRAY_BLKWHT_STIPPLES], ["#define XAW_GRAY_BLKWHT_STIPPLES 1"])],
  [AS_VAR_SET([CONFIG_GRAY_BLKWHT_STIPPLES], ["#undef XAW_GRAY_BLKWHT_STIPPLES"])])

AC_SUBST([LIBXAW3DXFT_VERSION_MAJOR], LVT_VERSION_MAJOR)
AC_SUBST([LIBXAW3DXFT_VERSION_MINOR], LVT_VERSION_MINOR)
AC_SUBST([LIBXAW3DXFT_VERSION_PATCH], LVT_VERSION_PATCH)
AC_SUBST([LIBXAW3DXFT_VERSION],       LVT_VERSION)
AC_SUBST([LIBXAW3DXFT_VERSION_STR],   LVT_VERSION_STR)

AC_SUBST([LIBXAW3DXFT_VERSION_MAJOR_FUNNAME],
  m4_format('libXaw3dXft_version_major_%s', LVT_VERSION_MAJOR))
AC_SUBST([LIBXAW3DXFT_VERSION_MINOR_FUNNAME],
  m4_format('libXaw3dXft_version_minor_%s', LVT_VERSION_MINOR))
AC_SUBST([LIBXAW3DXFT_VERSION_PATCH_FUNNAME],
  m4_format('libXaw3dXft_version_patch_%s', LVT_VERSION_PATCH))

AC_SUBST([MACRO_INTERNATIONALIZATION], $CONFIG_INTERNATIONALIZATION)
AC_SUBST([MACRO_MULTIPLANE_PIXMAPS],   $CONFIG_MULTIPLANE_PIXMAPS)
AC_SUBST([MACRO_ARROW_SCROLLBARS],     $CONFIG_ARROW_SCROLLBARS)
AC_SUBST([MACRO_GRAY_BLKWHT_STIPPLES], $CONFIG_GRAY_BLKWHT_STIPPLES)

AC_CONFIG_FILES([
  Makefile
  xaw3dxft.pc
  include/X11/Xaw3dxft/Xaw3dXft.h
  include/X11/Xaw3dxft/Xaw3d.h
  src/Xaw3dXft.c
])
AC_OUTPUT
