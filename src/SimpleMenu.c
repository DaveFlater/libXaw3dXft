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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <limits.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <X11/Xaw3dxft/Xaw3dP.h>
#include <X11/Xaw3dxft/XawInit.h>
#include <X11/Xaw3dxft/Xaw3dXftP.h>
#include <X11/Xaw3dxft/SimpleMenP.h>
#include <X11/Xaw3dxft/SmeBSBP.h>
#include <X11/Xaw3dxft/SmeLine.h>
#include <X11/Xaw3dxft/Cardinals.h>
#include <X11/Xaw3dxft/ThreeDP.h>

#include <X11/Xmu/Initer.h>
#include <X11/Xmu/CharSet.h>

#define streq(a, b)        ( strcmp((a), (b)) == 0 )

#define offset(field) XtOffsetOf(SimpleMenuRec, simple_menu.field)
#define MULTICOLUMN 255

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

  { XtNallowShellResize, XtCAllowShellResize, XtRBoolean, sizeof(Boolean),
      XtOffsetOf(SimpleMenuRec, shell.allow_shell_resize),
      XtRImmediate, (XtPointer) TRUE },
  {XtNcursor, XtCCursor, XtRCursor, sizeof(Cursor),
      offset(cursor), XtRImmediate, (XtPointer) None},
  {XtNmenuOnScreen,  XtCMenuOnScreen, XtRBoolean, sizeof(Boolean),
      offset(menu_on_screen), XtRImmediate, (XtPointer) TRUE},
  {XtNpopupOnEntry,  XtCPopupOnEntry, XtRWidget, sizeof(Widget),
      offset(popup_entry), XtRWidget, NULL},
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
static void Layout(Widget, Dimension *, Dimension *);
static void AddPositionAction(XtAppContext, XPointer);
static void PositionMenu(Widget, XPoint *);
static void ChangeCursorOnGrab(Widget, XtPointer, XtPointer);
static void SetMarginWidths(Widget);
static Dimension GetMenuWidth(Widget, Widget);
static Dimension GetMenuHeight(Widget);
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

#define SMW_ARROW_SIZE	8
#define	SMW_UNMAPPING	0x01
#define SMW_POPLEFT	0x02

#define ForAllChildren(smw, childP) \
	for ((childP) = (SmeObject *) (smw)->composite.children; \
	     (childP) < (SmeObject *) ((smw)->composite.children + \
			(smw)->composite.num_children); \
	     (childP)++)

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

/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
  SimpleMenuWidget smw = (SimpleMenuWidget) new;

  XmuCallInitializers(XtWidgetToApplicationContext(new));

  smw->simple_menu.label = NULL;
  smw->simple_menu.entry_set = NULL;
  smw->simple_menu.recursive_set_values = FALSE;
  smw->simple_menu.first_entry = NULL;
  smw->simple_menu.current_first = NULL;
  smw->simple_menu.first_y = 0;
  smw->simple_menu.too_tall = FALSE;
  smw->simple_menu.multicolumn = FALSE;
  smw->simple_menu.sub_menu = NULL;
  smw->simple_menu.state = 0;

  XtAddCallback(new, XtNpopupCallback, PopupCB, NULL);

  if (smw->simple_menu.label_class == NULL)
      smw->simple_menu.label_class = smeBSBObjectClass;

  if (smw->simple_menu.label_string != NULL)
      CreateLabel(new);

  /* GetMenuHeight() needs this */
  smw->simple_menu.threeD = XtVaCreateWidget("threeD", threeDWidgetClass,
      new,
      XtNx, 0, XtNy, 0, XtNwidth, /* dummy */ 10, XtNheight, /* dummy */ 10,
      NULL);

  smw->simple_menu.menu_width = TRUE;

  if (smw->core.width == 0) {
      smw->simple_menu.menu_width = FALSE;
      smw->core.width = GetMenuWidth(new, (Widget)NULL);
  }

  smw->simple_menu.menu_height = TRUE;

  if (smw->core.height == 0) {
      smw->simple_menu.menu_height = FALSE;
      smw->core.height = GetMenuHeight(new);
  }

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

/* ARGSUSED */
static void
Redisplay(Widget w, XEvent * event, Region region)
{
    SimpleMenuWidget smw = (SimpleMenuWidget)w;
    SmeObject *entry;
    SmeObjectClass class;
    ThreeDWidget tdw = (ThreeDWidget)smw->simple_menu.threeD;
    RectObjPart old_pos;
    int y, max_y, new_y, dy, s = tdw->threeD.shadow_width;
    Boolean can_paint;
    XPoint point[3];
    static int bw = -1;

    memset(&old_pos, 0, sizeof(RectObjPart));

    if (region == NULL)
	XClearWindow(XtDisplay(w), XtWindow(w));

    if (XtIsRealized((Widget)smw)) {
	_ShadowSurroundedBox((Widget)smw, tdw,
			0, 0, smw->core.width, smw->core.height,
			tdw->threeD.relief, True);
        if (_Xaw3dXft->border_hack) {
	    /* work around composition/Xft related bug on some X servers... */
	    if (bw == -1)
	        bw = XtBorderWidth(w);
            if (bw)
                XSetWindowBorderWidth(XtDisplayOfObject(w),
                                      XtWindowOfObject(w), 0);
            for (y=0; y<bw; y++) {
	        dy = 2*y+1;
                XDrawRectangle(XtDisplayOfObject(w), XtWindowOfObject(w),
                               XtGetGC(w, 0, 0), y, y,
                               XtWidth(w)-dy, XtHeight(w)-dy);
	    }
        }
    }

    smw->simple_menu.didnt_fit = False;
    y = 0;
    max_y = HeightOfScreen(XtScreen(w)) - s;
    new_y = -(*(SmeObject *)(smw)->composite.children)->rectangle.y;
    can_paint = False;

    /* check and paint each of the entries - including the label */
    ForAllChildren(smw, entry)
    {
	if (!XtIsManaged((Widget)*entry)) continue;

	if (smw->simple_menu.first_entry == NULL)
        {
	    smw->simple_menu.first_entry = entry;
	    smw->simple_menu.current_first = entry;
	}

	if (smw->simple_menu.too_tall)
	{
	    dy = 0;

	    if (entry == (smw->simple_menu.current_first))
	    {
		new_y = (*entry)->rectangle.y - 1;

		if (smw->simple_menu.current_first != smw->simple_menu.first_entry)
		{
		    point[0].x = (*entry)->rectangle.x + (*entry)->rectangle.width / 2;
		    point[0].y = s + 1;
		    point[1].x = (*entry)->rectangle.x + (*entry)->rectangle.width / 2 - SMW_ARROW_SIZE / 2;
		    point[1].y = s + SMW_ARROW_SIZE;
		    point[2].x = (*entry)->rectangle.x + (*entry)->rectangle.width / 2 + SMW_ARROW_SIZE / 2;
		    point[2].y = s + SMW_ARROW_SIZE;
		    XFillPolygon(XtDisplay(w), smw->core.window,
			    tdw->threeD.bot_shadow_GC, point, 3, Convex,
			    CoordModeOrigin);

		    new_y -= SMW_ARROW_SIZE;
		    dy = SMW_ARROW_SIZE;
		}

		smw->simple_menu.first_y = new_y;
		can_paint = True;
	    }
	    else if (!can_paint)
		continue;

	    old_pos = (*entry)->rectangle;
	    (*entry)->rectangle.y -= new_y;

	    if ((*entry)->rectangle.y + (*entry)->rectangle.height + dy > max_y)
	    {
		smw->simple_menu.last_y = (*entry)->rectangle.y;
		point[0].x = (*entry)->rectangle.x + (*entry)->rectangle.width / 2;
		point[0].y = max_y - 1;
		point[1].x = (*entry)->rectangle.x + (*entry)->rectangle.width / 2 - SMW_ARROW_SIZE / 2;
		point[1].y = max_y - SMW_ARROW_SIZE;
		point[2].x = (*entry)->rectangle.x + (*entry)->rectangle.width / 2 + SMW_ARROW_SIZE / 2;
		point[2].y = max_y - SMW_ARROW_SIZE;
		XFillPolygon(XtDisplay(w), smw->core.window,
			tdw->threeD.bot_shadow_GC, point, 3, Convex,
			CoordModeOrigin);

		smw->simple_menu.didnt_fit = True;
		(*entry)->rectangle = old_pos;
		break;
	    }
	}

	/*
	if (region != NULL)
	    switch (XRectInRegion(region,
		    (int)(*entry)->rectangle.x, (int)(*entry)->rectangle.y,
		    (unsigned int)(*entry)->rectangle.width,
		    (unsigned int)(*entry)->rectangle.height))
	    {
		case RectangleIn:
		case RectanglePart:
		    break;
		default:
		    continue;
	    }
	*/

	class = (SmeObjectClass)(*entry)->object.widget_class;

	if (class->rect_class.expose != NULL)
	    (class->rect_class.expose)((Widget)*entry, NULL, NULL);

	if (smw->simple_menu.too_tall) (*entry)->rectangle = old_pos;

	y += (*entry)->rectangle.height;
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

     /* check if the menu is too big */
     if (smw->core.height >= HeightOfScreen(XtScreen(w))) {
         smw->simple_menu.too_tall = TRUE;
         smw->core.height = HeightOfScreen(XtScreen(w));
     }

    (*superclass->core_class.realize) (w, mask, attrs);
}

/*      Function Name: Resize
 *      Description: Handle the menu being resized.
 *      Arguments: w - the simple menu widget or any of its object children.
 *      Returns: none.
 */

static void
Resize(Widget w)
{
    /*
     * The sole purpose of this function is to force an initial
     * layout by handling a call from some child widget. Ick.
     */

    if (XtIsSubclass(w, smeBSBObjectClass))
    {
	Widget parent = XtParent(w);

	if (!XtIsRealized(parent))
	    XtRealizeWidget(parent);

	Layout(w, (Dimension *)NULL, (Dimension *)NULL);
    }
}

/*      Function Name: SetValues
 *      Description: Relayout the menu when one of the resources is changed.
 *      Arguments: current - current state of the widget.
 *                 request - what was requested.
 *                 new - what the widget will become.
 *      Returns: none
 */

/* ARGSUSED */
static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    SimpleMenuWidget smw_old = (SimpleMenuWidget) current;
    SimpleMenuWidget smw_new = (SimpleMenuWidget) new;
    Boolean ret_val = FALSE, layout = FALSE;

    if (!XtIsRealized(current)) return(FALSE);

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
	    XtDestroyWidget((Widget) smw_old->simple_menu.label);
	else if (smw_old->simple_menu.label_string == NULL)    /* Create. */
	    CreateLabel(new);
	else {                                                 /* Change. */
	    Arg arglist[1];

	    XtSetArg(arglist[0], XtNlabel, smw_new->simple_menu.label_string);
	    XtSetValues((Widget) smw_new->simple_menu.label, arglist, ONE);
	}
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
	Layout(new, (Dimension *)NULL, (Dimension *)NULL);

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
	if ( streq(arglist[i].name, XtNwidth) )
	    width = (Dimension) arglist[i].value;
	if ( streq(arglist[i].name, XtNheight) )
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
 *	Returns: XtGeometry{Yes, No, Almost}.
 */

static XtGeometryResult
GeometryManager(Widget w, XtWidgetGeometry * request, XtWidgetGeometry * reply)
{
    SimpleMenuWidget smw = (SimpleMenuWidget) XtParent(w);
    SmeObject entry = (SmeObject) w;
    XtGeometryMask mode = request->request_mode;
    XtGeometryResult answer;
    Dimension old_height, old_width;

    if ( !(mode & CWWidth) && !(mode & CWHeight) )
	return(XtGeometryNo);

    reply->width = request->width;
    reply->height = request->height;

    old_width = entry->rectangle.width;
    old_height = entry->rectangle.height;

    Layout(w, &(reply->width), &(reply->height) );

/*
 * Since we are an override shell and have no parent there is no one to
 * ask to see if this geom change is okay, so I am just going to assume
 * we can do whatever we want.  If you subclass be very careful with this
 * assumption, it could bite you.
 *
 * Chris D. Peterson - Sept. 1989.
 */

    if ( (reply->width == request->width) &&
	 (reply->height == request->height) ) {

	if ( mode & XtCWQueryOnly ) {
	    entry->rectangle.width = old_width;
	    entry->rectangle.height = old_height;
	}
	else {
	    /* Actually perform the layout */
	    Layout(( Widget) smw, (Dimension *)NULL, (Dimension *)NULL);
	}
	answer = XtGeometryDone;
    }
    else {
	entry->rectangle.width = old_width;
	entry->rectangle.height = old_height;

	if ( ((reply->width == request->width) && !(mode & CWHeight)) ||
	      ((reply->height == request->height) && !(mode & CWWidth)) ||
	      ((reply->width == request->width) &&
	       (reply->height == request->height)) )
	    answer = XtGeometryNo;
	else {
	    answer = XtGeometryAlmost;
	    reply->request_mode = 0;
	    if (reply->width != request->width)
		reply->request_mode |= CWWidth;
	    if (reply->height != request->height)
		reply->request_mode |= CWHeight;
	}
    }
    return(answer);
}

/*	Function Name: ChangeManaged
 *	Description: called whenever a new child is managed.
 *	Arguments: w - the simple menu widget.
 *	Returns: none.
 */

static void
ChangeManaged(Widget w)
{
    Layout(w, (Dimension *)NULL, (Dimension *)NULL);
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

/* ARGSUSED */
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

/* ARGSUSED */
static void
Unhighlight(Widget w, XEvent * event, String * params, Cardinal * num_params)
{
    SimpleMenuWidget smw = (SimpleMenuWidget) w;
    SimpleMenuWidget sub = (SimpleMenuWidget) smw->simple_menu.sub_menu;
    SmeObject entry = smw->simple_menu.entry_set;
    SmeObjectClass class;
    int old_pos;

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
    class = (SmeObjectClass) entry->object.widget_class;

    /* backup, then restore, original entry position */
    old_pos = entry->rectangle.y;
    entry->rectangle.y -= smw->simple_menu.first_y;

    (class->sme_class.unhighlight) ((Widget) entry);

    entry->rectangle.y = old_pos;
}

/*      Function Name: Highlight
 *      Description: Highlights current entry.
 *      Arguments: w - the simple menu widget.
 *                 event - the event that caused this action.
 *                 params, num_params - ** NOT USED **
 *      Returns: none
 */

/* ARGSUSED */
static void
Highlight(Widget w, XEvent * event, String * params, Cardinal * num_params)
{
    SimpleMenuWidget smw = (SimpleMenuWidget) w;
    SmeObject entry;
    SmeObjectClass class;
    int old_pos;

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
	class = (SmeObjectClass) entry->object.widget_class;

	/* backup, then restore, original entry position */
	old_pos = entry->rectangle.y;
	entry->rectangle.y -= smw->simple_menu.first_y;

	(class->sme_class.highlight) ((Widget) entry);
	if (XtIsSubclass((Widget)entry, smeBSBObjectClass))
	    PopupSubMenu(smw);

	entry->rectangle.y = old_pos;
    }
}

/*      Function Name: Notify
 *      Description: Notify user of current entry.
 *      Arguments: w - the simple menu widget.
 *                 event - the event that caused this action.
 *                 params, num_params - ** NOT USED **
 *      Returns: none
 */

/* ARGSUSED */
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
 *	Description: Creates a the menu label.
 *	Arguments: w - the smw widget.
 *	Returns: none.
 *
 * Creates the label object and makes sure it is the first child in
 * in the list.
 */

static void
CreateLabel(Widget w)
{
    SimpleMenuWidget smw = (SimpleMenuWidget) w;
    Widget * child, * next_child;
    int i;
    Arg args[2];

    if ( (smw->simple_menu.label_string == NULL) ||
	 (smw->simple_menu.label != NULL) ) {
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

    next_child = NULL;
    for (child = smw->composite.children + smw->composite.num_children,
	 i = smw->composite.num_children ; i > 0 ; i--, child--) {
	if (next_child != NULL)
	    *next_child = *child;
	next_child = child;
    }
    *child = (Widget) smw->simple_menu.label;
}

/*	Function Name: Layout
 *	Description: lays the menu entries out all nice and neat.
 *	Arguments: w - See below (+++)
 *                 width_ret, height_ret - The returned width and
 *                                         height values.
 *	Returns: none.
 *
 * if width == NULL || height == NULL then it assumes the you do not care
 * about the return values, and just want a relayout.
 *
 * if this is not the case then it will set width_ret and height_ret
 * to be width and height that the child would get if it were laid out
 * at this time.
 *
 * +++ "w" can be the simple menu widget or any of its object children.
 */

static void
Layout(Widget w, Dimension *width_ret, Dimension *height_ret)
{
    SmeObject current_entry, *entry;
    SimpleMenuWidget smw;
    ThreeDWidget tdw;
    Dimension width, height, height_max = 0, scr_height_max, column = 0;
    Boolean do_layout = (height_ret == NULL || width_ret == NULL);
    Boolean allow_change_size, allow_multi_column;

    height = 0;

    if (XtIsSubclass(w, simpleMenuWidgetClass))
    {
	smw = (SimpleMenuWidget)w;
	current_entry = NULL;
    }
    else
    {
	smw = (SimpleMenuWidget)XtParent(w);
	current_entry = (SmeObject)w;
    }
    tdw = (ThreeDWidget)smw->simple_menu.threeD;

    do_layout |= (current_entry != NULL);
    allow_change_size =
		(!XtIsRealized((Widget)smw) || smw->shell.allow_shell_resize);

    if (smw->simple_menu.menu_height)
	height = smw->core.height;
    else if (do_layout)
    {
        /* width still unset, using it as place holder */
	width = smw->simple_menu.top_margin + tdw->threeD.shadow_width;
	height = width;
        scr_height_max =  HeightOfScreen(XtScreen(smw))
	  - (smw->simple_menu.top_margin + smw->simple_menu.bottom_margin +
	     2 * tdw->threeD.shadow_width);
        /* MultiColumnMenu works only when NoHilitReverse is also set */
	allow_multi_column = allow_change_size &&
			     _Xaw3dXft->multi_column_menu &&
                             _Xaw3dXft->no_hilit_reverse;

	ForAllChildren(smw, entry)
	{
	    if (!XtIsManaged((Widget)*entry)) continue;

	    if (smw->simple_menu.row_height != 0 &&
			*entry != smw->simple_menu.label)
		(*entry)->rectangle.height = smw->simple_menu.row_height;
            if (allow_multi_column &&
		height > scr_height_max - (*entry)->rectangle.height) {
                column += 1;
                height = width;
		smw->simple_menu.multicolumn = TRUE;
	    }
	    (*entry)->rectangle.y = height;
	    (*entry)->rectangle.x = column;
	    height += (*entry)->rectangle.height;
	    if (height > height_max)
	        height_max = height;
	}

	height = height_max + smw->simple_menu.bottom_margin + tdw->threeD.shadow_width;
    }
    else if (smw->simple_menu.row_height != 0 &&
		current_entry != smw->simple_menu.label)
    {
	height = smw->simple_menu.row_height * smw->composite.num_children;
	height += tdw->threeD.shadow_width * 2;
    }

    if (smw->simple_menu.menu_width)
	width = smw->core.width;
    else if (allow_change_size)
    {
	SetMarginWidths((Widget)smw);
	width = GetMenuWidth((Widget)smw, (Widget)NULL);
    }
    else
	width = smw->core.width;

    if (do_layout)
    {
        ForAllChildren(smw, entry) {
	    (*entry)->rectangle.x = (*entry)->rectangle.x * width;
	    if (XtIsManaged((Widget)*entry))
		(*entry)->rectangle.width = width;
	}
	if (allow_change_size) {
            width *= (column+1);
	    MakeSetValuesRequest((Widget) smw, width, height);
	}
    }
    else
    {
        *width_ret = width;
        if (height != 0) *height_ret = height;
    }
}

/*	Function Name: AddPositionAction
 *	Description: Adds the XawPositionSimpleMenu action to the global
 *                   action list for this appcon.
 *	Arguments: app_con - the application context for this app.
 *                 data - NOT USED.
 *	Returns: none.
 */

/* ARGSUSED */
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
 *                 location - a pointer the the position or NULL.
 *	Returns: none.
 */

static void
PositionMenu(Widget w, XPoint * location)
{
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
 *                   specified in out resource list.
 *	Arguments: w - the widget.
 *                 junk, garbage - ** NOT USED **.
 *	Returns: None.
 */

/* ARGSUSED */
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
	ForAllChildren(smw, entry)
	{
	    if (!XtIsManaged((Widget)*entry)) continue;
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
	ForAllChildren(smw, entry)
	{
	    if (!XtIsManaged((Widget)*entry)) continue;
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
 *                 w_ent - the current menu entry.
 *      Returns: width of menu.
 */

static Dimension
GetMenuWidth(Widget w, Widget w_ent)
{
    SmeObject cur_entry = (SmeObject) w_ent;
    SimpleMenuWidget smw = (SimpleMenuWidget) w;
    Dimension width, widest = (Dimension) 0;
    SmeObject * entry;

    if ( smw->simple_menu.menu_width )
	return(smw->core.width);

    ForAllChildren(smw, entry) {
	XtWidgetGeometry preferred;

	if (!XtIsManaged( (Widget) *entry)) continue;

	if (*entry != cur_entry) {
	    XtQueryGeometry((Widget) *entry, (XtWidgetGeometry *)NULL, &preferred);

	    if (preferred.request_mode & CWWidth)
		width = preferred.width;
	    else
		width = (*entry)->rectangle.width;
	}
	else
	    width = (*entry)->rectangle.width;

	if ( width > widest )
	    widest = width;
    }

    return(widest);
}

/*      Function Name: GetMenuHeight
 *      Description: Sets the height to all the entries in pixels.
 *      Arguments: w - the simple menu widget.
 *      Returns: height of menu.
 */

static Dimension
GetMenuHeight(Widget w)
{
    SimpleMenuWidget smw = (SimpleMenuWidget) w;
    ThreeDWidget tdw = (ThreeDWidget) smw->simple_menu.threeD;
    SmeObject * entry;
    Dimension height;

    if (smw->simple_menu.menu_height)
	return(smw->core.height);

    height = smw->simple_menu.top_margin + smw->simple_menu.bottom_margin;
    height += tdw->threeD.shadow_width * 2;

    if (smw->simple_menu.row_height == 0) {
	ForAllChildren(smw, entry)
	    if (XtIsManaged ((Widget) *entry))
		height += (*entry)->rectangle.height;
    } else
	height += smw->simple_menu.row_height * smw->composite.num_children;

    return(height);
}

/*      Function Name: GetEventEntry
 *      Description: Gets an entry given an event that has X and Y coords.
 *      Arguments: w - the simple menu widget.
 *                 event - the event.
 *      Returns: the entry that this point is in.
 */

static SmeObject
GetEventEntry(Widget w, XEvent * event)
{
    Position x_loc = 0, y_loc = 0;
    SimpleMenuWidget smw = (SimpleMenuWidget)w;
    SmeObject *entry;
    static XPoint last_pos;
    XPoint pos;
    int s = ((ThreeDWidget)smw->simple_menu.threeD)->threeD.shadow_width;

    switch (event->type) {
	case MotionNotify:
	    x_loc = event->xmotion.x;
	    y_loc = event->xmotion.y;
	    pos.y = event->xmotion.y_root;
	    break;
	case EnterNotify:
	case LeaveNotify:
	    x_loc = event->xcrossing.x;
	    y_loc = event->xcrossing.y;
	    pos.y = event->xcrossing.y_root;
	    break;
	case ButtonPress:
	case ButtonRelease:
	    x_loc = event->xbutton.x;
	    y_loc = event->xbutton.y;
	    pos.y = event->xbutton.y_root;
	    break;
	default:
	    XtAppError(XtWidgetToApplicationContext(w),
		       "Unknown event type in GetEventEntry().");
	    pos.y = 0;
	    break;
    }

    if (x_loc < 0 || x_loc >= (int)smw->core.width)
	return NULL;
    else if (smw->simple_menu.too_tall) {
	if (pos.y >= smw->simple_menu.last_y && smw->simple_menu.didnt_fit) {
	    if (last_pos.y && pos.y < last_pos.y) {
		last_pos.y = pos.y;
		return NULL;
	    }
	    smw->simple_menu.current_first += smw->simple_menu.jump_val;
	    Redisplay(w, (XEvent *)NULL, (Region)NULL);
	    last_pos.y = pos.y;
	    return NULL;
	} else if (pos.y <= s + SMW_ARROW_SIZE &&
		smw->simple_menu.first_entry != smw->simple_menu.current_first)
	{
	    if (pos.y && (!last_pos.y || pos.y > last_pos.y)) {
		last_pos.y = pos.y;
		return NULL;
	    }
	    smw->simple_menu.current_first -= smw->simple_menu.jump_val;
	    Redisplay(w, (XEvent *)NULL, (Region)NULL);
	    last_pos.y = pos.y;
	    return NULL;
	}
	else
	    last_pos.y = 0;
    } else if (y_loc < 0 || y_loc >= (int)smw->core.height)
	return NULL;

    ForAllChildren(smw, entry) {
	int tmp_y;

	if (!XtIsManaged((Widget)*entry)) continue;

	tmp_y = (*entry)->rectangle.y - smw->simple_menu.first_y;

	if (tmp_y < y_loc && tmp_y + (int)(*entry)->rectangle.height > y_loc) {
	    if (*entry == smw->simple_menu.label)
		return NULL;	/* cannot select the label */
	    else
	    if (!smw->simple_menu.multicolumn ||
		(x_loc >= (*entry)->rectangle.x &&
		 x_loc <= (*entry)->rectangle.x + (*entry)->rectangle.width))
		return *entry;
	}
    }

    return NULL;
}

/*ARGSUSED*/
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
