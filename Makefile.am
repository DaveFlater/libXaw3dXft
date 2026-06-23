# X11_CFLAGS, XPM_CFLAGS, X11_LIBS, and XPM_LIBS come from the two
# invocations of PKG_CHECK_MODULES in configure.ac.  All of the flags for all
# of the modules listed in an invocation are piled into the _CFLAGS and _LIBS
# of the prefix specified as the first argument.
AM_YFLAGS = -d -p LayYY
AM_CFLAGS = $(X11_CFLAGS) $(XPM_CFLAGS)
AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include
ACLOCAL_AMFLAGS = -I m4
lib_LTLIBRARIES = libXaw3dXft.la
libXaw3dXft_la_LIBADD = $(X11_LIBS) $(XPM_LIBS)
docdir = $(datarootdir)/doc/libXaw3dXft

if XAW_INTERNATIONALIZATION
I18N_SRCS = src/MultiSrc.c src/MultiSink.c src/XawIm.c src/XawI18n.c \
  src/XawI18n.h
I18N_HDRS = \
  include/X11/Xaw3dXft/MultiSrc.h \
  include/X11/Xaw3dXft/MultiSrcP.h \
  include/X11/Xaw3dXft/MultiSink.h \
  include/X11/Xaw3dXft/MultiSinkP.h \
  include/X11/Xaw3dXft/VendorEP.h \
  include/X11/Xaw3dXft/XawImP.h
endif

libXaw3dXft_la_SOURCES = \
  src/AllWidgets.c \
  src/AsciiSink.c \
  src/AsciiSrc.c \
  src/AsciiText.c \
  src/Box.c \
  src/Command.c \
  src/Dialog.c \
  src/Form.c \
  src/Grip.c \
  src/Label.c \
  src/Layout.c \
  src/LayYY.h \
  src/List.c \
  src/MenuButton.c \
  src/Paned.c \
  src/Panner.c \
  src/Porthole.c \
  src/Repeater.c \
  src/Scrollbar.c \
  src/Simple.c \
  src/SimpleMenu.c \
  src/SmeBSB.c \
  src/Sme.c \
  src/SmeLine.c \
  src/SmeThreeD.c \
  src/StripChart.c \
  src/TextAction.c \
  src/Text.c \
  src/TextPop.c \
  src/TextSink.c \
  src/TextSrc.c \
  src/TextTr.c \
  src/ThreeD.c \
  src/Tip.c \
  src/Toggle.c \
  src/Tree.c \
  src/Vendor.c \
  src/Viewport.c \
  src/Xaw3dP.c \
  src/XawInit.c \
  src/laygram.y \
  src/laylex.l \
  $(I18N_SRCS)
nodist_libXaw3dXft_la_SOURCES = src/Xaw3dXft.c config.h

xaw3dincludedir=$(includedir)/X11/Xaw3dXft
xaw3dinclude_HEADERS = \
  include/X11/Xaw3dXft/AllWidgets.h \
  include/X11/Xaw3dXft/AsciiSink.h \
  include/X11/Xaw3dXft/AsciiSinkP.h \
  include/X11/Xaw3dXft/AsciiSrc.h \
  include/X11/Xaw3dXft/AsciiSrcP.h \
  include/X11/Xaw3dXft/AsciiText.h \
  include/X11/Xaw3dXft/AsciiTextP.h \
  include/X11/Xaw3dXft/Box.h \
  include/X11/Xaw3dXft/BoxP.h \
  include/X11/Xaw3dXft/Cardinals.h \
  include/X11/Xaw3dXft/Command.h \
  include/X11/Xaw3dXft/CommandP.h \
  include/X11/Xaw3dXft/Dialog.h \
  include/X11/Xaw3dXft/DialogP.h \
  include/X11/Xaw3dXft/Form.h \
  include/X11/Xaw3dXft/FormP.h \
  include/X11/Xaw3dXft/Grip.h \
  include/X11/Xaw3dXft/GripP.h \
  include/X11/Xaw3dXft/Label.h \
  include/X11/Xaw3dXft/LabelP.h \
  include/X11/Xaw3dXft/Layout.h \
  include/X11/Xaw3dXft/LayoutP.h \
  include/X11/Xaw3dXft/List.h \
  include/X11/Xaw3dXft/ListP.h \
  include/X11/Xaw3dXft/MenuButton.h \
  include/X11/Xaw3dXft/MenuButtoP.h \
  include/X11/Xaw3dXft/Paned.h \
  include/X11/Xaw3dXft/PanedP.h \
  include/X11/Xaw3dXft/Panner.h \
  include/X11/Xaw3dXft/PannerP.h \
  include/X11/Xaw3dXft/Porthole.h \
  include/X11/Xaw3dXft/PortholeP.h \
  include/X11/Xaw3dXft/Repeater.h \
  include/X11/Xaw3dXft/RepeaterP.h \
  include/X11/Xaw3dXft/Reports.h \
  include/X11/Xaw3dXft/Scrollbar.h \
  include/X11/Xaw3dXft/ScrollbarP.h \
  include/X11/Xaw3dXft/Simple.h \
  include/X11/Xaw3dXft/SimpleMenP.h \
  include/X11/Xaw3dXft/SimpleMenu.h \
  include/X11/Xaw3dXft/SimpleP.h \
  include/X11/Xaw3dXft/SmeBSB.h \
  include/X11/Xaw3dXft/SmeBSBP.h \
  include/X11/Xaw3dXft/Sme.h \
  include/X11/Xaw3dXft/SmeLine.h \
  include/X11/Xaw3dXft/SmeLineP.h \
  include/X11/Xaw3dXft/SmeP.h \
  include/X11/Xaw3dXft/SmeThreeD.h \
  include/X11/Xaw3dXft/SmeThreeDP.h \
  include/X11/Xaw3dXft/StripCharP.h \
  include/X11/Xaw3dXft/StripChart.h \
  include/X11/Xaw3dXft/Text.h \
  include/X11/Xaw3dXft/TextP.h \
  include/X11/Xaw3dXft/TextSink.h \
  include/X11/Xaw3dXft/TextSinkP.h \
  include/X11/Xaw3dXft/TextSrc.h \
  include/X11/Xaw3dXft/TextSrcP.h \
  include/X11/Xaw3dXft/ThreeD.h \
  include/X11/Xaw3dXft/ThreeDP.h \
  include/X11/Xaw3dXft/Tip.h \
  include/X11/Xaw3dXft/TipP.h \
  include/X11/Xaw3dXft/Toggle.h \
  include/X11/Xaw3dXft/ToggleP.h \
  include/X11/Xaw3dXft/Tree.h \
  include/X11/Xaw3dXft/TreeP.h \
  include/X11/Xaw3dXft/Viewport.h \
  include/X11/Xaw3dXft/ViewportP.h \
  include/X11/Xaw3dXft/Xaw3dP.h \
  include/X11/Xaw3dXft/Xaw3dXftP.h \
  include/X11/Xaw3dXft/XawInit.h \
  $(I18N_HDRS)
nodist_xaw3dinclude_HEADERS = \
  include/X11/Xaw3dXft/Xaw3dXft.h \
  include/X11/Xaw3dXft/Xaw3d.h

EXTRA_DIST = \
  src/Template.c \
  include/X11/Xaw3dXft/Template.h \
  include/X11/Xaw3dXft/TemplateP.h

BUILT_SOURCES = src/laygram.h
dist_doc_DATA = README.md Xresources
# Bunch of extra directory definitions because Automake doesn't want to
# preserve relative directory structure
readmepicsdir = $(docdir)/README_pics
dist_readmepics_DATA = \
  README_pics/arrow_disabled.png \
  README_pics/arrow_enabled.png  \
  README_pics/gray_disabled.png  \
  README_pics/gray_enabled.png
olddocslayoutdir = $(docdir)/Docs_old/Layout
dist_olddocslayout_DATA = \
  Docs_old/Layout/R5contrib_Layout_examples.tar.xz \
  Docs_old/Layout/README.txt                       \
  Docs_old/Layout/XConf93_Layout.tar.xz
olddocsreadmesdir = $(docdir)/Docs_old/READMEs
dist_olddocsreadmes_DATA = \
  Docs_old/READMEs/libXaw3d-1.5.txt                \
  Docs_old/READMEs/libXaw3d-1.5E.txt               \
  Docs_old/READMEs/libXaw3d-1.6.2_brief.txt        \
  Docs_old/READMEs/libXaw3d-1.6.2.txt              \
  Docs_old/READMEs/libXaw3dXft-1.3.x.txt           \
  Docs_old/READMEs/libXaw3dXft-1.6.2h.txt
olddocsxawdir = $(docdir)/Docs_old/Xaw_R6.3
dist_olddocsxaw_DATA = \
  Docs_old/Xaw_R6.3/widg.idx.ps                    \
  Docs_old/Xaw_R6.3/widgets.ps

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xaw3dxft.pc

# Backward compatibility
install-data-hook:
	if [ -e "$(DESTDIR)$(xaw3dincludedir)" ]; then ln -s Xaw3dXft "$(DESTDIR)$(includedir)/X11/Xaw3dxft"; fi
uninstall-hook:
	rm -f "$(DESTDIR)$(includedir)/X11/Xaw3dxft"

# -version-info current:revision:age

# Shared lib versions are almost, but not quite, completely unlike
# release versions.  They are sufficiently unlike that if the two
# happen to be the same it will cause people to jump to the conclusion
# that you don't know what you are doing.

# If the library source code has changed at all since the last update,
# then increment revision (c:r:a becomes c:r+1:a).

# If any interfaces have been added, removed, or changed since the
# last update, increment current, and set revision to 0.

# If any interfaces have been added since the last public release,
# then increment age.

# If any interfaces have been removed since the last public release,
# then set age to 0.

# FIXME: Update libtool version info on each release
libXaw3dXft_la_LDFLAGS = -version-info 8:7:0 -no-undefined
