2026-06-10

Renamed and repacked files:
- Add/rename top level directory
- Apply standard file permissions
- Set owner to 1000/100
- Update tar format and compression

---------------------------------------------------------------------

XConf93_Layout.tar.xz
Origin:  https://keithp.com/~keithp/talks/layout.tar.gz

The Layout Widget:  A hierarchical, constraint-based widget, or a TeX-style
constraint widget class

The X Toolkit geometry management process is extremely flexible and powerful;
however, the existing composite widget classes make it difficult for the
application developer both to simply design an application layout, and even
more important, to make the layout work in a wide variety of environments.

The Layout widget class is described which uses a stretch/shrink model
similar to TeX to constrain the layout of an application in a manner which
allows the geometry of the children to respect the desires of the application
designer, while adapting to its environment, both in terms of the changing
geometry allocated to the widget, and to the changing needs of the child
widgets.  The specification of the child layout is entirely contained in a
resource which is interpreted at run time.

Seventh Annual X Technical Conference.  The X Resource, O'Reilly &
Associates, Issue Five, 1993, pp. 67-76.

---------------------------------------------------------------------

R5contrib_Layout_examples.tar.xz
Origin:  https://ftp.gwdg.de/pub/x11/x.org/R5contrib/Layout.tar.Z

Believed to be an accurate mirror of the disappeared
ftp://ftp.x.org/R5contrib/ referred to in the Xaw3d README:  "The samples in
Layout.h are wrong and don't work.  Example programs written by Keith Packard
that use the Layout widget are available at ftp://ftp.x.org/R5contrib/."

The PostScript file Layout-xconf93-paper.ps.Z available from R5contrib is
binarily different but visually identical to the layout.ps included in
XConf93_Layout.tar.xz.
