lib_LTLIBRARIES = libXaw3dxft.la

AM_YFLAGS = -d -p LayYY

AM_CFLAGS =		\
	$(CWARNFLAGS)	\
	$(X11_CFLAGS)	\
	$(XPM_CFLAGS)

AM_CPPFLAGS = \
	-I${top_srcdir}/include	\
	$(XAW3D_CPPFLAGS)

#
# This doesn't appear to be used on any
# current systems -- it requires SUNSHLIB and !SHAREDCODE,
# but only sunLib.rules defines SUNSHLIB and that file also
# always defines SHAREDCODE. Go figure
#
# SHAREDLIB_SOURCES = sharedlib.c

if XAW_INTERNATIONALIZATION
I18N_SRCS = MultiSrc.c MultiSink.c XawIm.c XawI18n.c XawI18n.h
endif

libXaw3dxft_la_SOURCES = \
	AllWidgets.c \
	AsciiSink.c \
	AsciiSrc.c \
	AsciiText.c \
	Box.c \
	Command.c \
	Dialog.c \
	Form.c \
	Grip.c \
	Label.c \
	Layout.c \
	LayYY.h \
	List.c \
	MenuButton.c \
	Paned.c \
	Panner.c \
	Porthole.c \
	Repeater.c \
	Scrollbar.c \
	Simple.c \
	SimpleMenu.c \
	SmeBSB.c \
	Sme.c \
	SmeLine.c \
	SmeThreeD.c \
	StripChart.c \
	TextAction.c \
	Text.c \
	TextPop.c \
	TextSink.c \
	TextSrc.c \
	TextTr.c \
	ThreeD.c \
	Tip.c \
	Toggle.c \
	Tree.c \
	Vendor.c \
	Viewport.c \
	Xaw3dP.c \
	Xaw3dXft.c \
	XawInit.c \
	laygram.y \
	laylex.l \
	$(I18N_SRCS)

libXaw3dxft_la_LIBADD = \
	$(X11_LIBS) $(XPM_LIBS)

libXaw3dxft_la_LDFLAGS = -version-info 8:5:0 -no-undefined

BUILT_SOURCES = laygram.h

dist_doc_DATA = README.XAW3D

EXTRA_DIST = sharedlib.c Template.c
