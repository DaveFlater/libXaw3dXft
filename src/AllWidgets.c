/*

Copyright (c) 1991, 1994  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <X11/IntrinsicP.h>
#include <X11/Xaw3dXft/AllWidgets.h>

#include <X11/Composite.h>
#include <X11/Constraint.h>
#include <X11/Core.h>
#include <X11/Object.h>
#include <X11/RectObj.h>
#include <X11/Shell.h>
#include <X11/Vendor.h>
#include <X11/Xaw3dXft/AsciiSink.h>
#include <X11/Xaw3dXft/AsciiText.h>
#include <X11/Xaw3dXft/Box.h>
#include <X11/Xaw3dXft/Dialog.h>
#include <X11/Xaw3dXft/Form.h>
#include <X11/Xaw3dXft/Grip.h>
#include <X11/Xaw3dXft/Layout.h>
#include <X11/Xaw3dXft/List.h>
#include <X11/Xaw3dXft/MenuButton.h>
#include <X11/Xaw3dXft/MultiSink.h>
#include <X11/Xaw3dXft/MultiSrc.h>
#include <X11/Xaw3dXft/Paned.h>
#include <X11/Xaw3dXft/Panner.h>
#include <X11/Xaw3dXft/Porthole.h>
#include <X11/Xaw3dXft/Repeater.h>
#include <X11/Xaw3dXft/Scrollbar.h>
#include <X11/Xaw3dXft/SimpleMenu.h>
#include <X11/Xaw3dXft/Sme.h>
#include <X11/Xaw3dXft/SmeBSB.h>
#include <X11/Xaw3dXft/SmeLine.h>
#include <X11/Xaw3dXft/SmeThreeD.h>
#include <X11/Xaw3dXft/StripChart.h>
#include <X11/Xaw3dXft/ThreeD.h>
#include <X11/Xaw3dXft/Toggle.h>
#include <X11/Xaw3dXft/Tree.h>
#include <X11/Xaw3dXft/Viewport.h>

static_assert(Got_XAW_defines);
XmuWidgetNode XawWidgetArray[] = {
{ "applicationShell", &applicationShellWidgetClass },
{ "asciiSink", &asciiSinkObjectClass },
{ "asciiSrc", &asciiSrcObjectClass },
{ "asciiText", &asciiTextWidgetClass },
{ "box", &boxWidgetClass },
{ "command", &commandWidgetClass },
{ "composite", &compositeWidgetClass },
{ "constraint", &constraintWidgetClass },
{ "core", &coreWidgetClass },
{ "dialog", &dialogWidgetClass },
{ "form", &formWidgetClass },
{ "grip", &gripWidgetClass },
{ "label", &labelWidgetClass },
{ "layout", &layoutWidgetClass },
{ "list", &listWidgetClass },
{ "menuButton", &menuButtonWidgetClass },
#ifdef XAW_INTERNATIONALIZATION
{ "multiSink", &multiSinkObjectClass },
{ "multiSrc", &multiSrcObjectClass },
#endif
{ "object", &objectClass },
{ "overrideShell", &overrideShellWidgetClass },
{ "paned", &panedWidgetClass },
{ "panner", &pannerWidgetClass },
{ "porthole", &portholeWidgetClass },
{ "rect", &rectObjClass },
{ "repeater", &repeaterWidgetClass },
{ "scrollbar", &scrollbarWidgetClass },
{ "shell", &shellWidgetClass },
{ "simpleMenu", &simpleMenuWidgetClass },
{ "simple", &simpleWidgetClass },
{ "smeBSB", &smeBSBObjectClass },
{ "smeLine", &smeLineObjectClass },
{ "smeThreeD", &smeThreeDObjectClass },
{ "sme", &smeObjectClass },
{ "stripChart", &stripChartWidgetClass },
{ "textSink", &textSinkObjectClass },
{ "textSrc", &textSrcObjectClass },
{ "text", &textWidgetClass },
{ "threeD", &threeDWidgetClass },
{ "toggle", &toggleWidgetClass },
{ "topLevelShell", &topLevelShellWidgetClass },
{ "transientShell", &transientShellWidgetClass },
{ "tree", &treeWidgetClass },
{ "vendorShell", &vendorShellWidgetClass },
{ "viewport", &viewportWidgetClass },
{ "wmShell", &wmShellWidgetClass },
};

int XawWidgetCount = XtNumber(XawWidgetArray);
