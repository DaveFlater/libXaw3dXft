/*
Copyright (c) 1989, 1994  X Consortium

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

/*
 * SimpleMenu.c - Source code file for SimpleMenu widget.
 *
 * Date:    April 3, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium
 *          kit@expo.lcs.mit.edu
 */

/*********************************************************************
Copyright © 2026 David Flater
X11 license (as per the historical licenses that the package inherits)
*********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xmu/CharSet.h>
#include <X11/Xmu/Initer.h>
#include <X11/Xaw3dXft/Cardinals.h>
#include <X11/Xaw3dXft/SimpleMenP.h>
#include <X11/Xaw3dXft/SmeBSBP.h>
#include <X11/Xaw3dXft/SmeLine.h>
#include <X11/Xaw3dXft/ThreeDP.h>
#include <X11/Xaw3dXft/Xaw3dP.h>
#include <X11/Xaw3dXft/Xaw3dXftP.h>
#include <X11/Xaw3dXft/XawInit.h>

#define offset(field) XtOffsetOf(SimpleMenuRec, simple_menu.field)

static XtResource resources[] = {

/*
 * Label Resources.
 */

  {XtNlabel,  XtCLabel, XtRString, sizeof(String),
     offset(label_string), XtRString, NULL},
  {XtNlabelClass,  XtCLabelClass, XtRPointer, sizeof(WidgetClass),
     offset(label_class), XtRImmediate, (XtPointer) NULL},

/*
 * Layout Resources.
 */

  {XtNmultiColumn,  XtCMultiColumn, XtRBoolean, sizeof(Boolean),
     offset(multiColumn), XtRImmediate, (XtPointer)True},
  {XtNrowHeight,  XtCRowHeight, XtRDimension, sizeof(Dimension),
     offset(row_height), XtRImmediate, (XtPointer) 0},
  {XtNtopMargin,  XtCVerticalMargins, XtRDimension, sizeof(Dimension),
     offset(top_margin), XtRImmediate, (XtPointer) 0},
  {XtNbottomMargin,  XtCVerticalMargins, XtRDimension, sizeof(Dimension),
     offset(bottom_margin), XtRImmediate, (XtPointer) 0},
  {XtNleftWhitespace,  XtCHorizontalWhitespace, XtRDimension, sizeof(Dimension),
     offset(left_whitespace), XtRImmediate, (XtPointer) 0},
  {XtNrightWhitespace,  XtCHorizontalWhitespace, XtRDimension, sizeof(Dimension),
     offset(right_whitespace), XtRImmediate, (XtPointer) 0},

/*
 * Misc. Resources
 */

  {XtNscrollArrowSize, XtCScrollArrowSize, XtRDimension, sizeof(Dimension),
      offset(arrow_size), XtRImmediate, (XtPointer)8},
  {XtNallowShellResize, XtCAllowShellResize, XtRBoolean, sizeof(Boolean),
      XtOffsetOf(SimpleMenuRec, shell.allow_shell_resize),
      XtRImmediate, (XtPointer) TRUE},
  {XtNcursor, XtCCursor, XtRCursor, sizeof(Cursor),
      offset(cursor), XtRImmediate, (XtPointer) None},
  {XtNmenuOnScreen,  XtCMenuOnScreen, XtRBoolean, sizeof(Boolean),
      offset(menu_on_screen), XtRImmediate, (XtPointer) TRUE},
  {XtNpopupOnEntry,  XtCPopupOnEntry, XtRWidget, sizeof(Widget),
      offset(popup_entry), XtRWidget, NULL},
  // X.h defines NotUseful, WhenMapped, and Always to be 0, 1, and 2
  // respectively.  The default value of 3 results in the backing store
  // attribute being left alone, which is probably what we want.
  {XtNbackingStore, XtCBackingStore, XtRBackingStore, sizeof (int),
      offset(backing_store),
      XtRImmediate, (XtPointer) (Always + WhenMapped + NotUseful)},
  {XtNjumpScroll,  XtCJumpScroll, XtRInt, sizeof(int),
      offset(jump_val), XtRImmediate, (XtPointer)1},
};
#undef offset

static char defaultTranslations[] =
    "<EnterWindow>:     highlight()             \n\
     <LeaveWindow>:     unhighlight()           \n\
     <BtnMotion>:       highlight()             \n\
     <BtnUp>:           notify() unhighlight() popdown()";

/*
 * Semi Public function definitions.
 */

static void Redisplay(Widget, XEvent *, Region);
static void Realize(Widget, XtValueMask *, XSetWindowAttributes *);
static void Resize(Widget);
static void ChangeManaged(Widget);
static void Initialize(Widget, Widget, ArgList, Cardinal *);
static void ClassInitialize(void);
static void ClassPartInitialize(WidgetClass);
static Boolean SetValues(Widget, Widget, Widget, ArgList, Cardinal *);
static Boolean SetValuesHook(Widget, ArgList, Cardinal *);
static XtGeometryResult GeometryManager(Widget, XtWidgetGeometry *, XtWidgetGeometry *);
static void PopupCB(Widget, XtPointer, XtPointer);
static void PopupSubMenu(SimpleMenuWidget);
static void PopdownSubMenu(SimpleMenuWidget);

/*
 * Action Routine Definitions
 */

static void Highlight(Widget, XEvent *, String *, Cardinal *);
static void Unhighlight(Widget, XEvent *, String *, Cardinal *);
static void Notify(Widget, XEvent *, String *, Cardinal *);
static void PositionMenuAction(Widget, XEvent *, String *, Cardinal *);
static void Popdown(Widget, XEvent *, String *, Cardinal *);

/*
 * Private Function Definitions.
 */

static void MakeSetValuesRequest(Widget, Dimension, Dimension);
static void CreateLabel(Widget);
static void Layout(Widget);
static void AddPositionAction(XtAppContext, XPointer);
static void PositionMenu(Widget, XPoint *);
static void ChangeCursorOnGrab(Widget, XtPointer, XtPointer);
static void SetMarginWidths(Widget);
static Dimension GetMenuWidth(Widget);
static Widget FindMenu(Widget, String);
static SmeObject GetEventEntry(Widget, XEvent *);
static void MoveMenu(Widget, Position, Position);

static XtActionsRec actionsList[] =
{
  {"notify",            Notify},
  {"highlight",         Highlight},
  {"unhighlight",       Unhighlight},
  {"popdown",           Popdown}
};

static CompositeClassExtensionRec extension_rec = {
    /* next_extension */  NULL,
    /* record_type */     NULLQUARK,
    /* version */         XtCompositeExtensionVersion,
    /* record_size */     sizeof(CompositeClassExtensionRec),
    /* accepts_objects */ TRUE,
    /* allows_change_managed_set */ True
};

#define superclass (&overrideShellClassRec)

SimpleMenuClassRec simpleMenuClassRec = {
  {
    /* superclass         */    (WidgetClass) superclass,
    /* class_name         */    "SimpleMenu",
    /* size               */    sizeof(SimpleMenuRec),
    /* class_initialize   */	ClassInitialize,
    /* class_part_initialize*/	ClassPartInitialize,
    /* Class init'ed      */	FALSE,
    /* initialize         */    Initialize,
    /* initialize_hook    */	NULL,
    /* realize            */    Realize,
    /* actions            */    actionsList,
    /* num_actions        */    XtNumber(actionsList),
    /* resources          */    resources,
    /* resource_count     */	XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion    */    TRUE,
    /* compress_exposure  */    TRUE,
    /* compress_enterleave*/ 	TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    NULL,
    /* resize             */    Resize,
    /* expose             */    Redisplay,
    /* set_values         */    SetValues,
    /* set_values_hook    */	SetValuesHook,
    /* set_values_almost  */	XtInheritSetValuesAlmost,
    /* get_values_hook    */	NULL,
    /* accept_focus       */    NULL,
    /* intrinsics version */	XtVersion,
    /* callback offsets   */    NULL,
    /* tm_table		  */    defaultTranslations,
    /* query_geometry	  */    NULL,
    /* display_accelerator*/    NULL,
    /* extension	  */    NULL
  },{
    /* geometry_manager   */    GeometryManager,
    /* change_managed     */    ChangeManaged,
    /* insert_child	  */	XtInheritInsertChild,
    /* delete_child	  */	XtInheritDeleteChild,
    /* extension	  */    NULL
  },{
    /* Shell extension	  */    NULL
  },{
    /* Override extension */    NULL
  },{
    /* Simple Menu extension*/  NULL
  }
};

WidgetClass simpleMenuWidgetClass = (WidgetClass)&simpleMenuClassRec;

#define SMW_ARROW_SIZE	(smw->simple_menu.arrow_size)
#define	SMW_UNMAPPING	0x01
#define SMW_POPLEFT	0x02

// All menu entries are children.  Not all children are menu entries.
// The first child is always the ThreeD.  Hide its presence.  Previously,
// every loop was skipping over it with
//   if (!XtIsManaged((Widget)*entry)) continue;
// and it was wrongly included in the count when calculating heights.

#define FirstEntry(smw) ((SmeObject *)(smw)->composite.children + 1)
#define ForAllEntries(smw, childP) \
	for ((childP) = FirstEntry(smw); \
	     (childP) < (SmeObject *)((smw)->composite.children + \
			              (smw)->composite.num_children); \
	     (childP)++)
#define NumEntries(smw) ((smw)->composite.num_children > 1 ? \
    (smw)->composite.num_children - 1 : 0)
#define LastEntry(smw) ((smw)->composite.num_children > 1 ? \
    (SmeObject *)((smw)->composite.children + \
                  (smw)->composite.num_children - 1) : NULL)

// This y coordinate means invalid, not applicable, do not draw.
static constexpr Position dead_y = -10;

/************************************************************
 *
 * Semi-Public Functions.
 *
 ************************************************************/

/*      Function Name: ClassInitialize
 *      Description: Class Initialize routine, called only once.
 *      Arguments: none.
 *      Returns: none.
 */

static void
ClassInitialize(void)
{
  XawInitializeWidgetSet();
  XtAddConverter( XtRString, XtRBackingStore, XmuCvtStringToBackingStore,
		 (XtConvertArgList)NULL, (Cardinal)0 );
  XmuAddInitializer( AddPositionAction, NULL);
}

/*      Function Name: ClassInitialize
 *      Description: Class Part Initialize routine, called for every
 *                   subclass.  Makes sure that the subclasses pick up
 *                   the extension record.
 *      Arguments: wc - the widget class of the subclass.
 *      Returns: none.
 */

static void
ClassPartInitialize(WidgetClass wc)
{
    SimpleMenuWidgetClass smwc = (SimpleMenuWidgetClass) wc;

/*
 * Make sure that our subclass gets the extension rec too.
 */

    extension_rec.next_extension = smwc->composite_class.extension;
    smwc->composite_class.extension = (XtPointer) &extension_rec;
}

/*      Function Name: Initialize
 *      Description: Initializes the simple menu widget
 *      Arguments: request - the widget requested by the argument list.
 *                 new     - the new widget with both resource and non
 *                           resource values.
 *      Returns: none.
 */

static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
  SimpleMenuWidget smw = (SimpleMenuWidget) new;

  XmuCallInitializers(XtWidgetToApplicationContext(new));

  /*
    Notes 2026-06-19 DWF:

    In sources as incoming, SimpleMenu inherited the default border width of
    1.  That somehow resulted in blurry text.  It was blurry regardless
    whether the text rendering was done by Xft or by Xlib.  The problem
    reproduced with plain Xaw and its default font.

    The border_hack workaround that was added in Xaw3dXft 1.6.2h set the
    window's border width to 0 after the fact and then artificially drew a
    black border.  In Xaw3dXft v2 I removed that hack and set border width
    to 0 here.

    The only two widgets with border_hack code were Tip and SimpleMenu.
    Tip is the only widget directly creating a child of the root window
    using XCreateWindow.  SimpleMenu is the only widget inheriting from
    Shell.
  */
  smw->core.border_width = 0;

  smw->simple_menu.label = NULL;
  smw->simple_menu.entry_set = NULL;
  smw->simple_menu.recursive_set_values = False;
  smw->simple_menu.current_first = NULL; // children is NULL at this point
  smw->simple_menu.too_tall =
    smw->simple_menu.multicolumn =
    smw->simple_menu.didnt_fit = False;
  smw->simple_menu.sub_menu = NULL;
  smw->simple_menu.state = 0;

  XtAddCallback(new, XtNpopupCallback, PopupCB, NULL);

  if (smw->simple_menu.label_class == NULL)
      smw->simple_menu.label_class = smeBSBObjectClass;

  // First child is ThreeD (unmanaged).
  smw->simple_menu.threeD = XtVaCreateWidget("threeD",
      threeDWidgetClass, new,
      XtNx, 0, XtNy, 0, XtNwidth, /* dummy */ 10, XtNheight, /* dummy */ 10,
      NULL);
  // True flag means that a dimension was specified and will be used.
  // Otherwise, core.width and height get overwritten later.
  smw->simple_menu.menu_width = (smw->core.width != 0);
  smw->simple_menu.menu_height = (smw->core.height != 0);

  // Second child is optional label.
  if (smw->simple_menu.label_string != NULL)
      CreateLabel(new);

  /* add a popup_callback routine for changing the cursor */
  XtAddCallback(new, XtNpopupCallback, ChangeCursorOnGrab, (XtPointer)NULL);
}

/*      Function Name: Redisplay
 *      Description: Redisplays the contents of the widget.
 *      Arguments: w - the simple menu widget.
 *                 event - the X event that caused this redisplay.
 *                 region - the region the needs to be repainted.
 *      Returns: none.
 */

static void Redisplay (Widget w, XEvent * event, Region region) {
  SimpleMenuWidget smw = (SimpleMenuWidget)w;
  ThreeDWidget tdw = (ThreeDWidget)smw->simple_menu.threeD;

  if (!XtIsRealized((Widget)smw))
    return;

  // Region is never used again.
  if (region == NULL)
    XClearWindow(XtDisplay(w), XtWindow(w));

  // Draw shadows.
  _ShadowSurroundedBox((Widget)smw, tdw, 0, 0, smw->core.width,
    smw->core.height, tdw->threeD.relief, True);

  // Draw scroll arrows if applicable.
  if (smw->simple_menu.too_tall) {
    // SMW_ARROW_SIZE is the height.  The width will be off by one.
    XPoint point[3];
    const Position xcenter = smw->core.width / 2;
    point[0].x = xcenter;
    point[1].x = xcenter - SMW_ARROW_SIZE / 2;
    point[2].x = xcenter + SMW_ARROW_SIZE / 2;
    // Top arrow
    assert(smw->simple_menu.current_first);
    if (smw->simple_menu.current_first != FirstEntry(smw)) {
      const Position ymin = smw->simple_menu.top_margin +
	tdw->threeD.shadow_width;
      point[0].y = ymin;
      point[1].y = point[2].y = ymin + SMW_ARROW_SIZE - 1;
      XFillPolygon(XtDisplay(w), XtWindow(w), tdw->threeD.bot_shadow_GC,
	point, 3, Convex, CoordModeOrigin);
    }
    // Bottom arrow
    if (smw->simple_menu.didnt_fit) {
      point[0].y = smw->simple_menu.bottom_y + SMW_ARROW_SIZE - 1;
      point[1].y = point[2].y = smw->simple_menu.bottom_y;
      XFillPolygon(XtDisplay(w), XtWindow(w), tdw->threeD.bot_shadow_GC,
	point, 3, Convex, CoordModeOrigin);
    }
  }

  // Draw the entries that are drawable.
  SmeObject *entry;
  ForAllEntries(smw, entry) {
    if ((*entry)->rectangle.y != dead_y) {
      SmeObjectClass class = (SmeObjectClass)(*entry)->object.widget_class;
      if (class->rect_class.expose != NULL)
	(class->rect_class.expose)((Widget)*entry, NULL, NULL);
    }
  }
}

/*      Function Name: Realize
 *      Description: Realizes the widget.
 *      Arguments: w - the simple menu widget.
 *                 mask - value mask for the window to create.
 *                 attrs - attributes for the window to create.
 *      Returns: none
 */

static void
Realize(Widget w, XtValueMask * mask, XSetWindowAttributes * attrs)
{
    SimpleMenuWidget smw = (SimpleMenuWidget) w;

    attrs->cursor = smw->simple_menu.cursor;
    *mask |= CWCursor;
    if ((smw->simple_menu.backing_store == Always) ||
	(smw->simple_menu.backing_store == NotUseful) ||
	(smw->simple_menu.backing_store == WhenMapped) ) {
	*mask |= CWBackingStore;
	attrs->backing_store = smw->simple_menu.backing_store;
    }
    else
	*mask &= ~CWBackingStore;

    (*superclass->core_class.realize) (w, mask, attrs);
}

/*      Function Name: Resize
 *      Description: Handle the menu being resized.
 *      Arguments: w - the simple menu widget or any of its object children.
 *      Returns: none.
 */

static void Resize (Widget w) {
  // In the irregular geometry management protocol used by SimpleMenu and
  // SmeBSB, this function's only purpose is to trigger a relayout when it is
  // called from SmeBSB's SetValues.
  if (XtIsSubclass(w, smeBSBObjectClass)) {
    Widget parent = XtParent(w);
    if (!XtIsRealized(parent))
      XtRealizeWidget(parent);
    Layout(w);
  }
}

/*      Function Name: SetValues
 *      Description: Relayout the menu when one of the resources is changed.
 *      Arguments: current - current state of the widget.
 *                 request - what was requested.
 *                 new - what the widget will become.
 *      Returns: none
 */

static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    SimpleMenuWidget smw_old = (SimpleMenuWidget)current;
    SimpleMenuWidget smw_new = (SimpleMenuWidget)new;
    Boolean ret_val = FALSE, layout = FALSE;
    smw_new->core.border_width = 0; // See Notes 2026-06-19 DWF above

    if (!smw_new->simple_menu.recursive_set_values) {
	if (smw_new->core.width != smw_old->core.width) {
	    smw_new->simple_menu.menu_width = (smw_new->core.width != 0);
	    layout = TRUE;
	}
	if (smw_new->core.height != smw_old->core.height) {
	    smw_new->simple_menu.menu_height = (smw_new->core.height != 0);
	    layout = TRUE;
	}
    }

    if (smw_old->simple_menu.cursor != smw_new->simple_menu.cursor)
	XDefineCursor(XtDisplay(new),
		      XtWindow(new), smw_new->simple_menu.cursor);

    if (smw_old->simple_menu.label_string !=smw_new->simple_menu.label_string) {
	if (smw_new->simple_menu.label_string == NULL)         /* Destroy. */
	    XtDestroyWidget((Widget)smw_old->simple_menu.label);
	else if (smw_old->simple_menu.label_string == NULL)    /* Create. */
	    CreateLabel(new);
	else {                                                 /* Change. */
	    Arg arglist[1] = {{XtNlabel,
			       (XtArgVal)smw_new->simple_menu.label_string}};
	    XtSetValues((Widget)smw_new->simple_menu.label, arglist, ONE);
	}
	layout = ret_val = True;
    }

    if (smw_old->simple_menu.label_class != smw_new->simple_menu.label_class)
	XtAppWarning(XtWidgetToApplicationContext(new),
		     "No Dynamic class change of the SimpleMenu Label.");

    if ((smw_old->simple_menu.top_margin != smw_new->simple_menu.top_margin) ||
	(smw_old->simple_menu.bottom_margin !=
	 smw_new->simple_menu.bottom_margin) /* filler.................  */ ) {
	layout = TRUE;
	ret_val = TRUE;
    }

    if (smw_old->simple_menu.left_whitespace != smw_new->simple_menu.left_whitespace) {
	layout = TRUE;
	ret_val = TRUE;
    }

    if (smw_old->simple_menu.right_whitespace != smw_new->simple_menu.right_whitespace) {
	layout = TRUE;
	ret_val = TRUE;
    }

    if (layout)
	Layout(new);

    return(ret_val);
}

/*      Function Name: SetValuesHook
 *      Description: To handle a special case, this is passed the
 *                   actual arguments.
 *      Arguments: w - the menu widget.
 *                 arglist - the argument list passed to XtSetValues.
 *                 num_args - the number of args.
 *      Returns: none
 */

/*
 * If the user actually passed a width and height to the widget
 * then this MUST be used, rather than our newly calculated width and
 * height.
 */

static Boolean
SetValuesHook(Widget w, ArgList arglist, Cardinal *num_args)
{
    Cardinal i;
    Dimension width, height;

    width = w->core.width;
    height = w->core.height;

    for ( i = 0 ; i < *num_args ; i++) {
	if (!strcmp(arglist[i].name, XtNwidth))
	    width = (Dimension) arglist[i].value;
	if (!strcmp(arglist[i].name, XtNheight))
	    height = (Dimension) arglist[i].value;
    }

    if ((width != w->core.width) || (height != w->core.height))
	MakeSetValuesRequest(w, width, height);
    return(FALSE);
}

/************************************************************
 *
 * Geometry Management routines.
 *
 ************************************************************/

/*	Function Name: GeometryManager
 *	Description: This is the SimpleMenu Widget's Geometry Manager.
 *	Arguments: w - the Menu Entry making the request.
 *                 request - requested new geometry.
 *                 reply - the allowed geometry.
 *	Returns: XtGeometry{Yes, No, Almost, Done}.
 */

// The job of this geometry manager is to stay out of the way.
static XtGeometryResult GeometryManager (Widget w, XtWidgetGeometry *request,
XtWidgetGeometry *reply) {
  XtGeometryMask mode = request->request_mode;
  if (mode & (CWBorderWidth | CWSibling | CWStackMode))
    return XtGeometryNo;
  SmeObject entry = (SmeObject)w;
  reply->x      = (mode & CWX      ? request->x      : entry->rectangle.x);
  reply->y      = (mode & CWY      ? request->y      : entry->rectangle.y);
  reply->width  = (mode & CWWidth  ? request->width  : entry->rectangle.width);
  reply->height = (mode & CWHeight ? request->height : entry->rectangle.height);
  if (!(mode & XtCWQueryOnly)) {
    if (mode & CWX)
      entry->rectangle.x = request->x;
    if (mode & CWY)
      entry->rectangle.y = request->y;
    if (mode & CWWidth)
      entry->rectangle.width = request->width;
    if (mode & CWHeight)
      entry->rectangle.height = request->height;
  }
  return XtGeometryDone;
}

/*	Function Name: ChangeManaged
 *	Description: called whenever a new child is managed.
 *	Arguments: w - the simple menu widget.
 *	Returns: none.
 */

static void ChangeManaged (Widget w) {
  Layout(w);
}

/************************************************************
 *
 * Global Action Routines.
 *
 * These actions routines will be added to the application's
 * global action list.
 *
 ************************************************************/

/*      Function Name: PositionMenuAction
 *      Description: Positions the simple menu widget.
 *      Arguments: w - a widget (no the simple menu widget.)
 *                 event - the event that caused this action.
 *                 params, num_params - parameters passed to the routine.
 *                                      we expect the name of the menu here.
 *      Returns: none
 */

static void
PositionMenuAction(Widget w, XEvent * event, String * params, Cardinal * num_params)
{
  Widget menu;
  XPoint loc;

  if (*num_params != 1) {
    char error_buf[BUFSIZ];
    (void) sprintf(error_buf, "%s %s",
	    "Xaw - SimpleMenuWidget: position menu action expects only one",
	    "parameter which is the name of the menu.");
    XtAppWarning(XtWidgetToApplicationContext(w), error_buf);
    return;
  }

  if ( (menu = FindMenu(w, params[0])) == NULL) {
    char error_buf[BUFSIZ];
    (void) sprintf(error_buf, "%s '%s'",
	    "Xaw - SimpleMenuWidget: could not find menu named: ", params[0]);
    XtAppWarning(XtWidgetToApplicationContext(w), error_buf);
    return;
  }

  switch (event->type) {
  case ButtonPress:
  case ButtonRelease:
    loc.x = event->xbutton.x_root;
    loc.y = event->xbutton.y_root;
    PositionMenu(menu, &loc);
    break;
  case EnterNotify:
  case LeaveNotify:
    loc.x = event->xcrossing.x_root;
    loc.y = event->xcrossing.y_root;
    PositionMenu(menu, &loc);
    break;
  case MotionNotify:
    loc.x = event->xmotion.x_root;
    loc.y = event->xmotion.y_root;
    PositionMenu(menu, &loc);
    break;
  default:
    PositionMenu(menu, (XPoint *)NULL);
    break;
  }
}

/************************************************************
 *
 * Widget Action Routines.
 *
 ************************************************************/

/*      Function Name: Unhighlight
 *      Description: Unhighlights current entry.
 *      Arguments: w - the simple menu widget.
 *                 event - the event that caused this action.
 *                 params, num_params - ** NOT USED **
 *      Returns: none
 */

static void
Unhighlight(Widget w, XEvent * event, String * params, Cardinal * num_params)
{
    SimpleMenuWidget smw = (SimpleMenuWidget) w;
    SimpleMenuWidget sub = (SimpleMenuWidget) smw->simple_menu.sub_menu;
    SmeObject entry = smw->simple_menu.entry_set;
    SmeObjectClass class;

    if (entry == NULL || entry == GetEventEntry(w, event)) {
	smw->simple_menu.entry_set = NULL;
	PopdownSubMenu(smw);
	return;
    }

    if (event->xcrossing.y < 0 || event->xcrossing.y >= (int)smw->core.height)
	PopdownSubMenu(smw);
    else if (sub &&
		((event->xcrossing.x < 0 &&
			!(sub->simple_menu.state & SMW_POPLEFT)) ||
		(event->xcrossing.x >= (int)smw->core.width &&
			(sub->simple_menu.state & SMW_POPLEFT))))
	PopdownSubMenu(smw);

    smw->simple_menu.entry_set = NULL;
    class = (SmeObjectClass)entry->object.widget_class;
    (class->sme_class.unhighlight) ((Widget)entry);
}

/*      Function Name: Highlight
 *      Description: Highlights current entry.
 *      Arguments: w - the simple menu widget.
 *                 event - the event that caused this action.
 *                 params, num_params - ** NOT USED **
 *      Returns: none
 */

static void
Highlight(Widget w, XEvent * event, String * params, Cardinal * num_params)
{
    SimpleMenuWidget smw = (SimpleMenuWidget) w;
    SmeObject entry;
    SmeObjectClass class;

    if (!XtIsSensitive(w)) return;

    entry = GetEventEntry(w, event);
    if (entry == smw->simple_menu.entry_set)
	return;

    PopdownSubMenu(smw);
    Unhighlight(w, event, params, num_params);

    if (entry == NULL)
	return;
    if (!XtIsSensitive((Widget) entry)) {
	smw->simple_menu.entry_set = NULL;
	return;
    }

    if (!(smw->simple_menu.state & SMW_UNMAPPING)) {
	smw->simple_menu.entry_set = entry;
	class = (SmeObjectClass)entry->object.widget_class;
	(class->sme_class.highlight) ((Widget)entry);
	if (XtIsSubclass((Widget)entry, smeBSBObjectClass))
	    PopupSubMenu(smw);
    }
}

/*      Function Name: Notify
 *      Description: Notify user of current entry.
 *      Arguments: w - the simple menu widget.
 *                 event - the event that caused this action.
 *                 params, num_params - ** NOT USED **
 *      Returns: none
 */

static void
Notify(Widget w, XEvent * event, String * params, Cardinal * num_params)
{
    SimpleMenuWidget smw = (SimpleMenuWidget) w;
    SmeObject entry = smw->simple_menu.entry_set;
    SmeObjectClass class;

    if ( (entry == NULL) || !XtIsSensitive((Widget) entry ) ) return;

    class = (SmeObjectClass) entry->object.widget_class;
    (class->sme_class.notify)( (Widget) entry );
}

/************************************************************
 *
 * Public Functions.
 *
 ************************************************************/

/*	Function Name: XawSimpleMenuAddGlobalActions
 *	Description: adds the global actions to the simple menu widget.
 *	Arguments: app_con - the appcontext.
 *	Returns: none.
 */

void
XawSimpleMenuAddGlobalActions(XtAppContext app_con)
{
    XtInitializeWidgetClass(simpleMenuWidgetClass);
    XmuCallInitializers( app_con );
}


/*	Function Name: XawSimpleMenuGetActiveEntry
 *	Description: Gets the currently active (set) entry.
 *	Arguments: w - the smw widget.
 *	Returns: the currently set entry or NULL if none is set.
 */

Widget
XawSimpleMenuGetActiveEntry(Widget w)
{
    SimpleMenuWidget smw = (SimpleMenuWidget) w;

    return( (Widget) smw->simple_menu.entry_set);
}

/*	Function Name: XawSimpleMenuClearActiveEntry
 *	Description: Unsets the currently active (set) entry.
 *	Arguments: w - the smw widget.
 *	Returns: none.
 */

void
XawSimpleMenuClearActiveEntry(Widget w)
{
    SimpleMenuWidget smw = (SimpleMenuWidget) w;

    smw->simple_menu.entry_set = NULL;
}

/************************************************************
 *
 * Private Functions.
 *
 ************************************************************/

/*	Function Name: CreateLabel
 *	Description: Creates the menu label.
 *	Arguments: w - the smw widget.
 *	Returns: none.
 *
 * Creates the label object and makes sure it is the second child in
 * in the list.
 */

static void CreateLabel (Widget w) {
  SimpleMenuWidget smw = (SimpleMenuWidget)w;
  Arg args[2];

  if (!smw->simple_menu.label_string || smw->simple_menu.label) {
    char error_buf[BUFSIZ];
    (void) sprintf(error_buf, "Xaw Simple Menu Widget: %s or %s, %s",
      "label string is NULL", "label already exists",
      "no label is being created.");
    XtAppWarning(XtWidgetToApplicationContext(w), error_buf);
    return;
  }

  XtSetArg(args[0], XtNlabel, smw->simple_menu.label_string);
  XtSetArg(args[1], XtNjustify, XtJustifyCenter);
  smw->simple_menu.label = (SmeObject)
    XtCreateManagedWidget("menuLabel",
      smw->simple_menu.label_class, w,
      args, TWO);
  if (!XtIsSubclass((Widget)smw->simple_menu.label, smeBSBObjectClass))
    XtError("libXaw3dXft: labelClass is not a subclass of SmeBSB");

  /*
    We need the ThreeD and the label to be first and second on the list of
    children.  If a label is added after the menu entries, it will appear at
    the end of the list and needs to be moved up.

    Another way to control placement is provided by the Xt Composite
    insert_position procedure, which is intended for use by the application.
    If the application actually uses that, we'll get an assertion failure
    here.
  */
  const Cardinal nc = smw->composite.num_children;
  assert(nc >= 2);
  assert(smw->composite.children[0] != (Widget)smw->simple_menu.label);
  if (smw->composite.children[1] != (Widget)smw->simple_menu.label) {
    assert(smw->composite.children[nc-1] == (Widget)smw->simple_menu.label);
    (void)memmove(smw->composite.children+2, smw->composite.children+1,
      (nc-2)*sizeof(Widget));
    smw->composite.children[1] = (Widget)smw->simple_menu.label;
  }
}

/* Function Name: Layout
 * Description: Determines geometry; sets positions and sizes of entries.
 * Arguments: w - The SimpleMenu or any of its SmeBSB children.
 */

static void Layout (Widget w) {
  SimpleMenuWidget smw;
  SmeObject *entry;

  if (XtIsSubclass(w, simpleMenuWidgetClass))
    smw = (SimpleMenuWidget)w;
  else
    smw = (SimpleMenuWidget)XtParent(w);
  ThreeDWidget tdw = (ThreeDWidget)smw->simple_menu.threeD;

  const Dimension shadowWidth = tdw->threeD.shadow_width;
  const Boolean canResize = !XtIsRealized((Widget)smw) ||
                              smw->shell.allow_shell_resize,
              fixedHeight = smw->simple_menu.menu_height || !canResize,
               fixedWidth = smw->simple_menu.menu_width || !canResize;
  const Dimension heightLimit = (fixedHeight ? smw->core.height :
    HeightOfScreen(XtScreen(smw)) - 2*smw->core.border_width);
  const SmeObject *lastEntry = LastEntry(smw);
  const Position y_cutoff = heightLimit - smw->simple_menu.bottom_margin -
    shadowWidth;

  smw->simple_menu.too_tall = False;    // Is scrolling needed?
  smw->simple_menu.didnt_fit = False;   // Is there a *bottom* scroll arrow?
  smw->simple_menu.multicolumn = False; // Is there more than one column?

  Boolean repeat = True;
  Dimension height;
  Cardinal column;
  while (repeat) {
    repeat = False;
    column = 0;
    height = smw->simple_menu.top_margin + shadowWidth;
    Dimension height_max = height;
    Boolean header = smw->simple_menu.too_tall, trailer = False;

    // Include top arrow when applicable.
    if (smw->simple_menu.too_tall) {
      if (smw->simple_menu.current_first) {
	if (smw->simple_menu.current_first != FirstEntry(smw))
	  height += SMW_ARROW_SIZE;
      } else
	smw->simple_menu.current_first = FirstEntry(smw);
    }

    ForAllEntries(smw, entry) {
      assert(XtIsManaged((Widget)*entry));

      // If row height was set, apply it to everything except the label.
      if (smw->simple_menu.row_height != 0 &&
	*entry != smw->simple_menu.label)
	(*entry)->rectangle.height = smw->simple_menu.row_height;

      if (entry == smw->simple_menu.current_first)
	header = False;
      if (header || trailer) {
	// This entry will not be displayed.
	(*entry)->rectangle.x = column;
	(*entry)->rectangle.y = dead_y;
	continue;
      }

      // There is an edge case when we are on the last entry, and it'll
      // fit, but there's no extra room for an arrow.  (There's also a
      // pathological case being ignored when SMW_ARROW_SIZE is taller than
      // the rectangles.)
      if (height + (*entry)->rectangle.height +
	(entry == lastEntry ? 0 : SMW_ARROW_SIZE) >= y_cutoff) {
	// Vertical space exceeded.
	if (smw->simple_menu.multiColumn) {
	  // Start a new column.
	  smw->simple_menu.multicolumn = True;
	  column += 1;
	  height = smw->simple_menu.top_margin + shadowWidth;
	} else if (smw->simple_menu.too_tall) {
	  smw->simple_menu.didnt_fit = trailer = True;
	  (*entry)->rectangle.x = column;
	  (*entry)->rectangle.y = dead_y;
	  // Mark where the bottom scrolling zone begins.
	  smw->simple_menu.bottom_y = height;
	  // Add the bottom arrow so that height is computed correctly
	  // after the loop.
	  height_max = height + SMW_ARROW_SIZE;
	  continue;
	} else {
	  // Restart with scroll arrows.
	  smw->simple_menu.too_tall = True;
	  repeat = True;
	  break;
	}
      }
      (*entry)->rectangle.x = column;
      (*entry)->rectangle.y = height;
      height += (*entry)->rectangle.height;
      if (height > height_max)
	height_max = height;
    }
    if (!repeat) {
      // If we finished the all entries loop without finding current_first,
      // it is invalid and must be reset.  This could happen if somebody
      // deletes a menu entry.
      if (header) {
	smw->simple_menu.current_first = FirstEntry(smw);
	repeat = True;
      } else
        // Finalize height.
        height = (fixedHeight ? smw->core.height :
	  height_max + smw->simple_menu.bottom_margin + shadowWidth);
    }
  }

  // Reset current_first if we aren't scrolling anymore.
  if (!smw->simple_menu.too_tall)
    smw->simple_menu.current_first = FirstEntry(smw);

  SetMarginWidths((Widget)smw);
  Dimension width, columnWidth;
  if (fixedWidth) {
    width = smw->core.width;
    columnWidth = (width - 2*shadowWidth) / (column+1);
  } else {
    // GetMenuWidth does not result in calling Xaw3dXftSizeAnyString on every
    // entry label again.  That happens only if and when the SmeBSB gets a
    // relevant SetValue.
    columnWidth = GetMenuWidth((Widget)smw);
    width = columnWidth * (column+1) + 2*shadowWidth;
    const Dimension maxWidth = WidthOfScreen(XtScreen(w)) -
                               2 * w->core.border_width;
    if (width > maxWidth) {
      width = maxWidth;
      columnWidth = (width - 2*shadowWidth) / (column+1);
    }
  }
  // All the x were set to column number.  Finalize their values.
  ForAllEntries(smw, entry) {
    (*entry)->rectangle.x = (*entry)->rectangle.x * columnWidth
      + shadowWidth;
    (*entry)->rectangle.width = columnWidth;
  }

  // Maybe resize, maybe move.
  assert(!fixedHeight || height == smw->core.height);
  assert(!fixedWidth || width == smw->core.width);
  if (width != smw->core.width || height != smw->core.height) {
    MakeSetValuesRequest((Widget)smw, width, height);
    if (smw->simple_menu.menu_on_screen)
      MoveMenu((Widget)smw, smw->core.x, smw->core.y);
  }
}

/*	Function Name: AddPositionAction
 *	Description: Adds the XawPositionSimpleMenu action to the global
 *                   action list for this appcon.
 *	Arguments: app_con - the application context for this app.
 *                 data - NOT USED.
 *	Returns: none.
 */

static void
AddPositionAction(XtAppContext app_con, XPointer data)
{
    static XtActionsRec pos_action[] = {
        { "XawPositionSimpleMenu", PositionMenuAction },
    };

    XtAppAddActions(app_con, pos_action, XtNumber(pos_action));
}

/*	Function Name: FindMenu
 *	Description: Find the menu give a name and reference widget.
 *	Arguments: widget - reference widget.
 *                 name   - the menu widget's name.
 *	Returns: the menu widget or NULL.
 */

static Widget
FindMenu(Widget widget, String name)
{
    Widget w, menu;

    for ( w = widget ; w != NULL ; w = XtParent(w) )
	if ( (menu = XtNameToWidget(w, name)) != NULL )
	    return(menu);
    return(NULL);
}

/*	Function Name: PositionMenu
 *	Description: Places the menu
 *	Arguments: w - the simple menu widget.
 *                 location - a pointer to the position or NULL.
 *	Returns: none.
 */

static void PositionMenu (Widget w, XPoint *location) {
    SimpleMenuWidget smw = (SimpleMenuWidget) w;
    SmeObject entry;
    XPoint t_point;

    if (location == NULL) {
	Window junk1, junk2;
	int root_x, root_y, junkX, junkY;
	unsigned int junkM;

	location = &t_point;
	if (XQueryPointer(XtDisplay(w), XtWindow(w), &junk1, &junk2,
			  &root_x, &root_y, &junkX, &junkY, &junkM) == FALSE) {
	    char error_buf[BUFSIZ];
	    (void) sprintf(error_buf, "%s %s", "Xaw Simple Menu Widget:",
		    "Could not find location of mouse pointer");
	    XtAppWarning(XtWidgetToApplicationContext(w), error_buf);
	    return;
	}
	location->x = (short) root_x;
	location->y = (short) root_y;
    }

    /*
     * The width will not be correct unless it is realized.
     */

    XtRealizeWidget(w);

    location->x -= (Position) w->core.width/2;

    if (smw->simple_menu.popup_entry == NULL)
	entry = smw->simple_menu.label;
    else
	entry = smw->simple_menu.popup_entry;

    if (entry != NULL)
	location->y -= entry->rectangle.y + entry->rectangle.height/2;

    MoveMenu(w, (Position) location->x, (Position) location->y);
}

/*	Function Name: MoveMenu
 *	Description: Actually moves the menu, may force it to
 *                   to be fully visible if menu_on_screen is TRUE.
 *	Arguments: w - the simple menu widget.
 *                 x, y - the current location of the widget.
 *	Returns: none
 */

static void
MoveMenu(Widget w, Position x, Position y)
{
    SimpleMenuWidget smw = (SimpleMenuWidget) w;
    Arg arglist[2];

    if (smw->simple_menu.menu_on_screen) {
	int width = w->core.width + 2 * w->core.border_width;
	int height = w->core.height + 2 * w->core.border_width;

	if (x >= 0) {
	    int scr_width = WidthOfScreen(XtScreen(w));
	    if (x + width > scr_width)
		x = scr_width - width;
	}
	if (x < 0)
	    x = 0;

	if (y >= 0) {
	    int scr_height = HeightOfScreen(XtScreen(w));
	    if (y + height > scr_height)
		y = scr_height - height;
	}
	if (y < 0)
	    y = 0;
    }

    XtSetArg(arglist[0], XtNx, x);
    XtSetArg(arglist[1], XtNy, y);
    XtSetValues(w, arglist, TWO);
}

/*	Function Name: ChangeCursorOnGrab
 *	Description: Changes the cursor on the active grab to the one
 *                   specified in our resource list.
 *	Arguments: w - the widget.
 *                 junk, garbage - ** NOT USED **.
 *	Returns: None.
 */

static void
ChangeCursorOnGrab(Widget w, XtPointer junk, XtPointer garbage)
{
    SimpleMenuWidget smw = (SimpleMenuWidget) w;

    /*
     * The event mask here is what is currently in the MIT implementation.
     * There really needs to be a way to get the value of the mask out
     * of the toolkit (CDP 5/26/89).
     */

    XChangeActivePointerGrab(XtDisplay(w), ButtonPressMask|ButtonReleaseMask,
			     smw->simple_menu.cursor,
			     XtLastTimestampProcessed(XtDisplay(w)));
}

/*      Function Name: MakeSetValuesRequest
 *      Description: Makes a (possibly recursive) call to SetValues,
 *                   I take great pains to not go into an infinite loop.
 *      Arguments: w - the simple menu widget.
 *                 width, height - the size of the ask for.
 *      Returns: none
 */

static void
MakeSetValuesRequest(Widget w, Dimension width, Dimension height)
{
    SimpleMenuWidget smw = (SimpleMenuWidget) w;

    if ( !smw->simple_menu.recursive_set_values ) {
	if ( (smw->core.width != width) || (smw->core.height != height) ) {
	    Arg arglist[2];

	    smw->simple_menu.recursive_set_values = TRUE;
	    XtSetArg(arglist[0], XtNwidth, width);
	    XtSetArg(arglist[1], XtNheight, height);
	    XtSetValues(w, arglist, TWO);
	}
	else if (XtIsRealized( (Widget) smw))
	    Redisplay((Widget) smw, (XEvent *) NULL, (Region) NULL);
    }
    smw->simple_menu.recursive_set_values = FALSE;
}

/*
 * Function Name: SetMarginWidths()
 * Description:   Set new margin values for all menu children.
 * Arguments:     w - the simple menu widget
 *                w_ent - the current menu entry
 */
static void
SetMarginWidths(Widget w)
{
    SimpleMenuWidget smw = (SimpleMenuWidget)w;
    SmeObject *entry;
    SmeBSBObject bsb_entry;
    Dimension l_mrgn = 0, l_bmw, r_mrgn = 0, r_bmw;

    if (smw->simple_menu.left_whitespace || smw->simple_menu.right_whitespace)
    {
	/* determine the widest bitmaps */
	l_bmw = r_bmw = (Dimension)0;
	ForAllEntries(smw, entry)
	{
	    assert(XtIsManaged((Widget)*entry));
	    if (*entry == smw->simple_menu.label) continue;
	    if (XtIsSubclass((Widget)*entry, smeLineObjectClass))
		continue;

	    bsb_entry = (SmeBSBObject)&((*entry)->object);
	    if (bsb_entry->sme_bsb.left_bitmap_width > l_bmw)
		l_bmw = bsb_entry->sme_bsb.left_bitmap_width;
	    if (bsb_entry->sme_bsb.right_bitmap_width > r_bmw)
		r_bmw = bsb_entry->sme_bsb.right_bitmap_width;
	}

	/* set the margin values */
	if (smw->simple_menu.left_whitespace)
	    l_mrgn = l_bmw +
			(smw->simple_menu.left_whitespace * ((l_bmw) ? 2 : 1));
	if (smw->simple_menu.right_whitespace)
	    r_mrgn = r_bmw +
			(smw->simple_menu.right_whitespace * ((r_bmw) ? 2 : 1));

	/* make all the margins uniform */
	ForAllEntries(smw, entry)
	{
	    if (*entry == smw->simple_menu.label) continue;
	    if (XtIsSubclass((Widget)*entry, smeLineObjectClass))
		continue;

	    bsb_entry = (SmeBSBObject)&((*entry)->object);
	    if (smw->simple_menu.left_whitespace)
		bsb_entry->sme_bsb.left_margin = l_mrgn;
	    if (smw->simple_menu.right_whitespace)
		bsb_entry->sme_bsb.right_margin = r_mrgn;
	}
    }
}

/*      Function Name: GetMenuWidth
 *      Description: Sets the width to the widest entry in pixels.
 *      Arguments: w - the simple menu widget.
 *      Returns: width of menu.  [Single column logic]
 */

static Dimension
GetMenuWidth(Widget w)
{
    SimpleMenuWidget smw = (SimpleMenuWidget) w;
    Dimension width, widest = (Dimension) 0;
    SmeObject * entry;

    if ( smw->simple_menu.menu_width )
	return(smw->core.width);

    ForAllEntries(smw, entry) {
	XtWidgetGeometry preferred;
	assert(XtIsManaged((Widget)*entry));
	XtQueryGeometry((Widget) *entry, (XtWidgetGeometry *)NULL, &preferred);
	if (preferred.request_mode & CWWidth)
	    width = preferred.width;
	else
	    width = (*entry)->rectangle.width;
	if ( width > widest )
	    widest = width;
    }
    return(widest);
}

/*      Function Name: GetEventEntry
 *      Description: Gets an entry given an event that has X and Y coords.
 *      Arguments: w - the simple menu widget.
 *                 event - the event.
 *      Returns: the entry that this point is in.
 */

static SmeObject GetEventEntry (Widget w, XEvent * event) {
  Position x_loc = 0, y_loc = 0;
  SimpleMenuWidget smw = (SimpleMenuWidget)w;
  SmeObject *entry;
  static Position last_y = dead_y;
  int s = ((ThreeDWidget)smw->simple_menu.threeD)->threeD.shadow_width;

  switch (event->type) {
  case MotionNotify:
    x_loc = event->xmotion.x;
    y_loc = event->xmotion.y;
    break;
  case EnterNotify:
  case LeaveNotify:
    x_loc = event->xcrossing.x;
    y_loc = event->xcrossing.y;
    break;
  case ButtonPress:
  case ButtonRelease:
    x_loc = event->xbutton.x;
    y_loc = event->xbutton.y;
    break;
  default:
    XtAppError(XtWidgetToApplicationContext(w),
      "Unknown event type in GetEventEntry().");
  }

  // Impossible coordinates are impossible
  if (x_loc < 0 || x_loc >= smw->core.width ||
      y_loc < 0 || y_loc >= smw->core.height)
    return NULL;

  // Scrolling is triggered by trying to get an entry that does not exist.
  if (smw->simple_menu.too_tall) {
    assert(smw->simple_menu.current_first);
    assert(NumEntries(smw) > 0);
    // Bottom scrolling zone
    if (y_loc >= smw->simple_menu.bottom_y && smw->simple_menu.didnt_fit) {
      // If pointer is moving upward, don't scroll.
      if (last_y == dead_y || y_loc >= last_y) {
	smw->simple_menu.current_first += smw->simple_menu.jump_val;
	if (smw->simple_menu.current_first > LastEntry(smw))
	  smw->simple_menu.current_first = LastEntry(smw);
	Layout(w);
	Redisplay(w, NULL, NULL);
      }
      last_y = y_loc;
      return NULL;
    // Top scrolling zone
    } else if (y_loc < smw->simple_menu.top_margin + s + SMW_ARROW_SIZE &&
    smw->simple_menu.current_first != FirstEntry(smw)) {
      // If pointer is moving downward, don't scroll.
      if (last_y == dead_y || y_loc <= last_y) {
	smw->simple_menu.current_first -= smw->simple_menu.jump_val;
	if (smw->simple_menu.current_first < FirstEntry(smw))
	  smw->simple_menu.current_first = FirstEntry(smw);
	Layout(w);
	Redisplay(w, NULL, NULL);
      }
      last_y = y_loc;
      return NULL;
    } else
      last_y = dead_y; // Reset latch
  }

  // The normal case:  figure out which entry the pointer is at.
  ForAllEntries(smw, entry) {
    if ((*entry)->rectangle.y != dead_y &&
	y_loc >= (*entry)->rectangle.y &&
        y_loc <  (*entry)->rectangle.y + (*entry)->rectangle.height &&
        (!smw->simple_menu.multicolumn ||
	  x_loc >= (*entry)->rectangle.x &&
	  x_loc <  (*entry)->rectangle.x + (*entry)->rectangle.width))
      return (*entry != smw->simple_menu.label ? *entry : NULL);
  }

  return NULL;
}

static void
PopupCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    SimpleMenuWidget smw = (SimpleMenuWidget)w;

    smw->simple_menu.state &= ~SMW_UNMAPPING;
}

static void
PopupSubMenu(SimpleMenuWidget smw)
{
    Widget menu;
    SmeBSBObject entry = (SmeBSBObject)smw->simple_menu.entry_set;
    Position menu_x, menu_y;
    Bool popleft;
    Arg args[2];

    if (entry->sme_bsb.menu_name == NULL)
	return;

    if ((menu = FindMenu((Widget)smw, entry->sme_bsb.menu_name)) == NULL)
	return;

    smw->simple_menu.sub_menu = menu;

    if (!XtIsRealized(menu))
	XtRealizeWidget(menu);

    popleft = (smw->simple_menu.state & SMW_POPLEFT) != 0;

    if (popleft)
	XtTranslateCoords((Widget)smw, -(int)XtWidth(menu),
			  XtY(entry) - XtBorderWidth(menu), &menu_x, &menu_y);
    else
	XtTranslateCoords((Widget)smw, XtWidth(smw), XtY(entry)
			  - XtBorderWidth(menu), &menu_x, &menu_y);

    if (!popleft && menu_x >= 0) {
	int scr_width = WidthOfScreen(XtScreen(menu));

	if (menu_x + XtWidth(menu) > scr_width) {
	    menu_x -= XtWidth(menu) + XtWidth(smw);
	    popleft = True;
	}
    }
    else if (popleft && menu_x < 0) {
	menu_x = 0;
	popleft = False;
    }

    if (menu_y >= 0) {
	ThreeDWidget tdw =
		(ThreeDWidget)((SimpleMenuWidget)menu)->simple_menu.threeD;
	int scr_height = HeightOfScreen(XtScreen(menu));

	if (menu_y + XtHeight(menu) > scr_height)
	    menu_y = scr_height - XtHeight(menu) - XtBorderWidth(menu);

	menu_y -= tdw->threeD.shadow_width;
    }
    if (menu_y < 0)
	menu_y = 0;

    XtSetArg(args[0], XtNx, menu_x);
    XtSetArg(args[1], XtNy, menu_y);
    XtSetValues(menu, args, TWO);

    if (popleft)
	((SimpleMenuWidget)menu)->simple_menu.state |= SMW_POPLEFT;
    else
	((SimpleMenuWidget)menu)->simple_menu.state &= ~SMW_POPLEFT;

    XtPopup(menu, XtGrabNone);
}

static void
Popdown(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    SimpleMenuWidget smw = (SimpleMenuWidget)w;

    while (XtParent(w) &&
	   XtIsSubclass(XtParent(w), simpleMenuWidgetClass)) {
	if (((SimpleMenuWidget)XtParent(w))->simple_menu.sub_menu == (Widget)w)
	{
	    w = XtParent(w);
	    smw = (SimpleMenuWidget)w;
	    smw->simple_menu.entry_set = NULL;
	}
	else
	    break;
    }

    smw->simple_menu.state |= SMW_UNMAPPING;
    PopdownSubMenu(smw);

    XtCallActionProc(w, "XtMenuPopdown", event, params, *num_params);
}

static void
PopdownSubMenu(SimpleMenuWidget smw)
{
    SimpleMenuWidget menu = (SimpleMenuWidget)smw->simple_menu.sub_menu;

    if (!menu) return;

    menu->simple_menu.state &= ~SMW_POPLEFT;
    menu->simple_menu.state |= SMW_UNMAPPING;
    PopdownSubMenu(menu);

    XtPopdown((Widget)menu);

    smw->simple_menu.sub_menu = NULL;
}
