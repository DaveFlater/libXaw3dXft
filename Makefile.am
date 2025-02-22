lib_LTLIBRARIES = libXaw3dxft.la
AM_YFLAGS = -d -p LayYY
AM_CFLAGS = $(X11_CFLAGS) $(XPM_CFLAGS)
AM_CPPFLAGS = -I${top_srcdir}/include $(XAW3D_CPPFLAGS)
ACLOCAL_AMFLAGS = -I m4

if XAW_INTERNATIONALIZATION
I18N_SRCS = src/MultiSrc.c src/MultiSink.c src/XawIm.c src/XawI18n.c \
  src/XawI18n.h
I18N_HDRS = \
  include/X11/Xaw3dxft/MultiSrc.h \
  include/X11/Xaw3dxft/MultiSrcP.h \
  include/X11/Xaw3dxft/MultiSink.h \
  include/X11/Xaw3dxft/MultiSinkP.h \
  include/X11/Xaw3dxft/VendorEP.h \
  include/X11/Xaw3dxft/XawImP.h
endif

libXaw3dxft_la_SOURCES = \
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
  src/Xaw3dXft.c \
  src/XawInit.c \
  src/laygram.y \
  src/laylex.l \
  $(I18N_SRCS)

xaw3dincludedir=${includedir}/X11/Xaw3dxft
xaw3dinclude_HEADERS = \
  include/X11/Xaw3dxft/AllWidgets.h \
  include/X11/Xaw3dxft/AsciiSink.h \
  include/X11/Xaw3dxft/AsciiSinkP.h \
  include/X11/Xaw3dxft/AsciiSrc.h \
  include/X11/Xaw3dxft/AsciiSrcP.h \
  include/X11/Xaw3dxft/AsciiText.h \
  include/X11/Xaw3dxft/AsciiTextP.h \
  include/X11/Xaw3dxft/Box.h \
  include/X11/Xaw3dxft/BoxP.h \
  include/X11/Xaw3dxft/Cardinals.h \
  include/X11/Xaw3dxft/Command.h \
  include/X11/Xaw3dxft/CommandP.h \
  include/X11/Xaw3dxft/Dialog.h \
  include/X11/Xaw3dxft/DialogP.h \
  include/X11/Xaw3dxft/Form.h \
  include/X11/Xaw3dxft/FormP.h \
  include/X11/Xaw3dxft/Grip.h \
  include/X11/Xaw3dxft/GripP.h \
  include/X11/Xaw3dxft/Label.h \
  include/X11/Xaw3dxft/LabelP.h \
  include/X11/Xaw3dxft/Layout.h \
  include/X11/Xaw3dxft/LayoutP.h \
  include/X11/Xaw3dxft/List.h \
  include/X11/Xaw3dxft/ListP.h \
  include/X11/Xaw3dxft/MenuButton.h \
  include/X11/Xaw3dxft/MenuButtoP.h \
  include/X11/Xaw3dxft/Paned.h \
  include/X11/Xaw3dxft/PanedP.h \
  include/X11/Xaw3dxft/Panner.h \
  include/X11/Xaw3dxft/PannerP.h \
  include/X11/Xaw3dxft/Porthole.h \
  include/X11/Xaw3dxft/PortholeP.h \
  include/X11/Xaw3dxft/Repeater.h \
  include/X11/Xaw3dxft/RepeaterP.h \
  include/X11/Xaw3dxft/Reports.h \
  include/X11/Xaw3dxft/Scrollbar.h \
  include/X11/Xaw3dxft/ScrollbarP.h \
  include/X11/Xaw3dxft/Simple.h \
  include/X11/Xaw3dxft/SimpleMenP.h \
  include/X11/Xaw3dxft/SimpleMenu.h \
  include/X11/Xaw3dxft/SimpleP.h \
  include/X11/Xaw3dxft/SmeBSB.h \
  include/X11/Xaw3dxft/SmeBSBP.h \
  include/X11/Xaw3dxft/Sme.h \
  include/X11/Xaw3dxft/SmeLine.h \
  include/X11/Xaw3dxft/SmeLineP.h \
  include/X11/Xaw3dxft/SmeP.h \
  include/X11/Xaw3dxft/SmeThreeD.h \
  include/X11/Xaw3dxft/SmeThreeDP.h \
  include/X11/Xaw3dxft/StripCharP.h \
  include/X11/Xaw3dxft/StripChart.h \
  include/X11/Xaw3dxft/Text.h \
  include/X11/Xaw3dxft/TextP.h \
  include/X11/Xaw3dxft/TextSink.h \
  include/X11/Xaw3dxft/TextSinkP.h \
  include/X11/Xaw3dxft/TextSrc.h \
  include/X11/Xaw3dxft/TextSrcP.h \
  include/X11/Xaw3dxft/ThreeD.h \
  include/X11/Xaw3dxft/ThreeDP.h \
  include/X11/Xaw3dxft/Tip.h \
  include/X11/Xaw3dxft/TipP.h \
  include/X11/Xaw3dxft/Toggle.h \
  include/X11/Xaw3dxft/ToggleP.h \
  include/X11/Xaw3dxft/Tree.h \
  include/X11/Xaw3dxft/TreeP.h \
  include/X11/Xaw3dxft/Viewport.h \
  include/X11/Xaw3dxft/ViewportP.h \
  include/X11/Xaw3dxft/Xaw3dP.h \
  include/X11/Xaw3dxft/Xaw3dXft.h \
  include/X11/Xaw3dxft/Xaw3dXftP.h \
  include/X11/Xaw3dxft/XawInit.h \
  $(I18N_HDRS)

EXTRA_DIST = \
  src/Template.c \
  include/X11/Xaw3dxft/Template.h \
  include/X11/Xaw3dxft/TemplateP.h

BUILT_SOURCES = src/laygram.h
dist_doc_DATA = README.md Xdefault
oldreadmesdir = $(docdir)/READMEs-old
dist_oldreadmes_DATA = \
  READMEs-old/libXaw3d-1.6.2_brief.txt \
  READMEs-old/libXaw3dXft-1.3.x.txt \
  READMEs-old/libXaw3dXft-1.6.2h.txt \
  READMEs-old/libXaw3d-1.5.txt \
  READMEs-old/libXaw3d-1.5E.txt \
  READMEs-old/libXaw3d-1.6.2.txt

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxaw3dxft.pc

libXaw3dxft_la_LIBADD = $(X11_LIBS) $(XPM_LIBS)

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
libXaw3dxft_la_LDFLAGS = -version-info 8:6:0 -no-undefined
