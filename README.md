# libXaw3dXft: Athena Widgets + 3D + FreeType font support

## Overview and usage

For now, please refer to
[READMEs-old/libXaw3dXft-1.6.2h.txt](READMEs-old/libXaw3dXft-1.6.2h.txt).

Example configure.ac:

    AC_INIT([hello], [1.0])
    AC_CONFIG_SRCDIR([Makefile.am])
    AM_INIT_AUTOMAKE([foreign dist-xz no-dist-gzip])
    AC_LANG([C])
    AC_PROG_CC
    AC_REQUIRE_CPP
    PKG_CHECK_MODULES(XAW3DXFT, [libxaw3dxft])
    AC_CONFIG_FILES([Makefile])
    AC_OUTPUT

Example Makefile.am:

    AM_CFLAGS     = $(XAW3DXFT_CFLAGS)
    AM_LDFLAGS    = $(XAW3DXFT_LIBS)
    bin_PROGRAMS  = hello
    hello_SOURCES = hello.c

## Building a release

    ./configure --enable-internationalization --enable-multiplane-bitmaps --enable-gray-stipples --enable-arrow-scrollbars
    make -j 4
    make install

See the INSTALL file for general help on using configure.

## Building git sources

First,

    autoreconf --install

Then proceed as for building a release.

## History

Kaleb Keithley originated libXaw3d in 1992 as a general replacement for the
[Athena Widgets (Xaw)](https://gitlab.freedesktop.org/xorg/lib/libxaw) of
X11.  libXaw3d 1.5, released 1998-05-14, was "based on the R6.1/R6.3/R6.4
Athena Widget set."

D. J. Hawkey Jr. took over as maintainer for libXaw3d 1.5E, released
2003-03-08.  "This release of Xaw3d is based on X.Org's X11R6.3 Athena
toolkit, with bits and pieces thrown in from other sources."  "There were no
public releases of 1.5A through 1.5D."

[X.Org took over maintenance for libXaw3d
1.6](https://gitlab.freedesktop.org/xorg/lib/libxaw3d), released 2012-01-21.
At that point, libXaw3d was effectively forked from libXaw R6.3 (ish).

Meanwhile, [Jean-Pierre Demailly originated
libXaw3dXft](https://sourceforge.net/projects/sf-xpaint/files/libxaw3dxft/)
in 2009-09 as a general replacement for libXaw3d.  libXaw3dXft 1.6.2,
released 2012-03-04, was based on some libXaw3d development snapshot between
2012-02-15 and 2012-03-04.  At that point, libXaw3dXft was effectively forked
from libXaw3d.  The libXaw3d 1.6.2 release was not until 2012-03-29.

Jean-Pierre Demailly released libXaw3dXft-1.6.2h on 2020-07-02.  [He passed
away on 2022-03-17](https://en.wikipedia.org/wiki/Jean-Pierre_Demailly).

[Dave Flater took over maintenance of
libXaw3dXft](https://github.com/DaveFlater/libXaw3dXft) on 2025-02-16.  The
previous [SourceForge
repo](https://sourceforge.net/projects/sf-xpaint/files/libxaw3dxft/), which
has libXaw3dXft as a subdirectory of the XPaint project, appears to be
[abandoned and
unrecoverable](https://sourceforge.net/p/forge/documentation/Abandoned%20Projects/).
The new repo is at
[https://github.com/DaveFlater/libXaw3dXft](https://github.com/DaveFlater/libXaw3dXft).

## To do

The following changes have been deferred so that the 1.6.3 release will
maintain backward compatibility.

Build in support for HiDPI / 4k displays so that heavy-handed upscaling by
the desktop environment is unnecessary.

Make the naming consistent.  The name of the lib has been spelled Xaw3dXft,
Xaw3dxft, and xaw3dxft in different places:

- Name in configure.ac is libXaw3dXft
- Name in pkgconfig file is Xaw3dxft
- Identifier in source code is Xaw3dXft
- Include dir is Xaw3dxft
- Header file is Xaw3dXft.h
- Shared library is libXaw3dxft.so
- pkgconfig file is libxaw3dxft.pc (but it correctly links -lXaw3dxft)

Make --enable-internationalization --enable-multiplane-bitmaps
--enable-gray-stipples --enable-arrow-scrollbars the default configuration.
(--enable-internationalization is already already on by default: see
libXaw3d 2012-02-01 a17b2984.)

Set the beNiceToColormap resource to False on everything by default.
