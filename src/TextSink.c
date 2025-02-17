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
 * Author:  Chris Peterson, MIT X Consortium.
 *
 * Much code taken from X11R3 AsciiSink.
 */

/*
 * TextSink.c - TextSink object. (For use with the text widget).
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <ctype.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw3dxft/XawInit.h>
#include <X11/Xaw3dxft/TextSinkP.h>
#include <X11/Xaw3dxft/TextP.h>

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

static void ClassPartInitialize(WidgetClass);
static void Initialize(Widget, Widget, ArgList, Cardinal *);
static void Destroy(Widget);
static Boolean SetValues(Widget, Widget, Widget, ArgList, Cardinal *);

static int MaxHeight(Widget, int);
static int MaxLines(Widget, Dimension);
static void DisplayText(Widget, Position, Position, XawTextPosition,
                        XawTextPosition, Boolean);
static void InsertCursor(Widget, Position, Position, XawTextInsertState);
static void ClearToBackground(Widget, Position, Position, Dimension, Dimension);
static void FindPosition(Widget, XawTextPosition, int, int, Boolean,
                         XawTextPosition *, int *, int *);
static void FindDistance(Widget, XawTextPosition, int, XawTextPosition,
                         int *, XawTextPosition *, int *);
static void Resolve(Widget, XawTextPosition, int, int, XawTextPosition *);
static void SetTabs(Widget, int, short *);
static void GetCursorBounds(Widget, XRectangle *);

#define offset(field) XtOffsetOf(TextSinkRec, text_sink.field)
static XtResource resources[] = {
  {XtNforeground, XtCForeground, XtRPixel, sizeof (Pixel),
     offset(foreground), XtRString, XtDefaultForeground},
  {XtNbackground, XtCBackground, XtRPixel, sizeof (Pixel),
     offset(background), XtRString, XtDefaultBackground},
};
#undef offset

#define SuperClass		(&objectClassRec)
TextSinkClassRec textSinkClassRec = {
  {
/* core_class fields */
    /* superclass	  	*/	(WidgetClass) SuperClass,
    /* class_name	  	*/	"TextSink",
    /* widget_size	  	*/	sizeof(TextSinkRec),
    /* class_initialize   	*/	XawInitializeWidgetSet,
    /* class_part_initialize	*/	ClassPartInitialize,
    /* class_inited       	*/	FALSE,
    /* initialize	  	*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* obj1		  	*/	NULL,
    /* obj2		  	*/	NULL,
    /* obj3	  		*/	0,
    /* resources	  	*/	resources,
    /* num_resources	  	*/	XtNumber(resources),
    /* xrm_class	  	*/	NULLQUARK,
    /* obj4		  	*/	FALSE,
    /* obj5	  		*/	FALSE,
    /* obj6			*/	FALSE,
    /* obj7	  	  	*/	FALSE,
    /* destroy		  	*/	Destroy,
    /* obj8		  	*/	NULL,
    /* obj9		  	*/	NULL,
    /* set_values	  	*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* obj10			*/	NULL,
    /* get_values_hook		*/	NULL,
    /* obj11		 	*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private   	*/	NULL,
    /* obj12		   	*/	NULL,
    /* obj13			*/	NULL,
    /* obj14			*/	NULL,
    /* extension		*/	NULL
  },
/* textSink_class fields */
  {
    /* DisplayText              */      DisplayText,
    /* InsertCursor             */      InsertCursor,
    /* ClearToBackground        */      ClearToBackground,
    /* FindPosition             */      FindPosition,
    /* FindDistance             */      FindDistance,
    /* Resolve                  */      Resolve,
    /* MaxLines                 */      MaxLines,
    /* MaxHeight                */      MaxHeight,
    /* SetTabs                  */      SetTabs,
    /* GetCursorBounds          */      GetCursorBounds,
  }
};

WidgetClass textSinkObjectClass = (WidgetClass)&textSinkClassRec;

static void
ClassPartInitialize(WidgetClass wc)
{
  TextSinkObjectClass t_src, superC;

  t_src = (TextSinkObjectClass) wc;
  superC = (TextSinkObjectClass) t_src->object_class.superclass;

/*
 * We don't need to check for null super since we'll get to TextSink
 * eventually.
 */

    if (t_src->text_sink_class.DisplayText == XtInheritDisplayText)
      t_src->text_sink_class.DisplayText = superC->text_sink_class.DisplayText;

    if (t_src->text_sink_class.InsertCursor == XtInheritInsertCursor)
      t_src->text_sink_class.InsertCursor =
                  	                  superC->text_sink_class.InsertCursor;

    if (t_src->text_sink_class.ClearToBackground== XtInheritClearToBackground)
      t_src->text_sink_class.ClearToBackground =
	                             superC->text_sink_class.ClearToBackground;

    if (t_src->text_sink_class.FindPosition == XtInheritFindPosition)
      t_src->text_sink_class.FindPosition =
	                                  superC->text_sink_class.FindPosition;

    if (t_src->text_sink_class.FindDistance == XtInheritFindDistance)
      t_src->text_sink_class.FindDistance =
	                                 superC->text_sink_class.FindDistance;

    if (t_src->text_sink_class.Resolve == XtInheritResolve)
      t_src->text_sink_class.Resolve = superC->text_sink_class.Resolve;

    if (t_src->text_sink_class.MaxLines == XtInheritMaxLines)
      t_src->text_sink_class.MaxLines = superC->text_sink_class.MaxLines;

    if (t_src->text_sink_class.MaxHeight == XtInheritMaxHeight)
      t_src->text_sink_class.MaxHeight = superC->text_sink_class.MaxHeight;

    if (t_src->text_sink_class.SetTabs == XtInheritSetTabs)
      t_src->text_sink_class.SetTabs = superC->text_sink_class.SetTabs;

    if (t_src->text_sink_class.GetCursorBounds == XtInheritGetCursorBounds)
      t_src->text_sink_class.GetCursorBounds =
                                       superC->text_sink_class.GetCursorBounds;
}

/*	Function Name: Initialize
 *	Description: Initializes the TextSink Object.
 *	Arguments: request, new - the requested and new values for the object
 *                                instance.
 *	Returns: none.
 *
 */

/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
  TextSinkObject sink = (TextSinkObject) new;

  sink->text_sink.tab_count = 0; /* Initialize the tab stops. */
  sink->text_sink.tabs = NULL;
  sink->text_sink.char_tabs = NULL;
}

/*	Function Name: Destroy
 *	Description: This function cleans up when the object is
 *                   destroyed.
 *	Arguments: w - the TextSink Object.
 *	Returns: none.
 */

static void
Destroy(Widget w)
{
  TextSinkObject sink = (TextSinkObject) w;

  XtFree((char *) sink->text_sink.tabs);
  XtFree((char *) sink->text_sink.char_tabs);
}

/*	Function Name: SetValues
 *	Description: Sets the values for the TextSink
 *	Arguments: current - current state of the object.
 *                 request - what was requested.
 *                 new - what the object will become.
 *	Returns: True if redisplay is needed.
 */

/* ARGSUSED */
static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
  TextSinkObject w = (TextSinkObject) new;
  TextSinkObject old_w = (TextSinkObject) current;

  if (w->text_sink.foreground != old_w->text_sink.foreground)
     ((TextWidget)XtParent(new))->text.redisplay_needed = True;

  return FALSE;
}

/************************************************************
 *
 * Class specific methods.
 *
 ************************************************************/

/*	Function Name: DisplayText
 *	Description: Stub function that in subclasses will display text.
 *	Arguments: w - the TextSink Object.
 *                 x, y - location to start drawing text.
 *                 pos1, pos2 - location of starting and ending points
 *                              in the text buffer.
 *                 highlight - hightlight this text?
 *	Returns: none.
 *
 * This function doesn't actually display anything, it is only a place
 * holder.
 */

/* ARGSUSED */
static void
DisplayText(Widget w, Position x, Position y, XawTextPosition pos1,
            XawTextPosition pos2, Boolean highlight)
{
  return;
}

/*	Function Name: InsertCursor
 *	Description: Places the InsertCursor.
 *	Arguments: w - the TextSink Object.
 *                 x, y - location for the cursor.
 *                 staye - whether to turn the cursor on, or off.
 *	Returns: none.
 *
 * This function doesn't actually display anything, it is only a place
 * holder.
 */

/* ARGSUSED */
static void
InsertCursor(Widget w, Position x, Position y, XawTextInsertState state)
{
  return;
}

/*	Function Name: ClearToBackground
 *	Description: Clears a region of the sink to the background color.
 *	Arguments: w - the TextSink Object.
 *                 x, y  - location of area to clear.
 *                 width, height - size of area to clear
 *	Returns: void.
 *
 */

/* ARGSUSED */
static void
ClearToBackground(Widget w, Position x, Position y, Dimension width, Dimension height)
{
/*
 * Don't clear in height or width are zero.
 * XClearArea() has special semantic for these values.
 */

    if ( (height == 0) || (width == 0) ) return;
    XClearArea(XtDisplayOfObject(w), XtWindowOfObject(w),
	       x, y, width, height, False);
}

/*	Function Name: FindPosition
 *	Description: Finds a position in the text.
 *	Arguments: w - the TextSink Object.
 *                 fromPos - reference position.
 *                 fromX   - reference location.
 *                 width,  - width of section to paint text.
 *                 stopAtWordBreak - returned position is a word break?
 *                 resPos - Position to return.      *** RETURNED ***
 *                 resWidth - Width actually used.   *** RETURNED ***
 *                 resHeight - Height actually used. *** RETURNED ***
 *	Returns: none (see above).
 */

/* ARGSUSED */
static void
FindPosition(Widget w, XawTextPosition fromPos, int fromx, int width, Boolean stopAtWordBreak,
             XawTextPosition *resPos, int *resWidth, int *resHeight)
{
  *resPos = fromPos;
  *resHeight = *resWidth = 0;
}

/*	Function Name: FindDistance
 *	Description: Find the Pixel Distance between two text Positions.
 *	Arguments: w - the TextSink Object.
 *                 fromPos - starting Position.
 *                 fromX   - x location of starting Position.
 *                 toPos   - end Position.
 *                 resWidth - Distance between fromPos and toPos.
 *                 resPos   - Acutal toPos used.
 *                 resHeight - Height required by this text.
 *	Returns: none.
 */

/* ARGSUSED */
static void
FindDistance(Widget w, XawTextPosition fromPos, int fromx, XawTextPosition toPos,
             int *resWidth, XawTextPosition *resPos, int *resHeight)
{
  *resWidth = *resHeight = 0;
  *resPos = fromPos;
}

/*	Function Name: Resolve
 *	Description: Resloves a location to a position.
 *	Arguments: w - the TextSink Object.
 *                 pos - a reference Position.
 *                 fromx - a reference Location.
 *                 width - width to move.
 *                 resPos - the resulting position.
 *	Returns: none
 */

/* ARGSUSED */
static void
Resolve(Widget w, XawTextPosition pos, int fromx, int width, XawTextPosition *resPos)
{
  *resPos = pos;
}

/*	Function Name: MaxLines
 *	Description: Finds the Maximum number of lines that will fit in
 *                   a given height.
 *	Arguments: w - the TextSink Object.
 *                 height - height to fit lines into.
 *	Returns: the number of lines that will fit.
 */

/* ARGSUSED */
static int
MaxLines(Widget w, Dimension height)
{
  /*
   * The fontset has gone down to descent Sink Widget, so
   * the functions such MaxLines, SetTabs... are bound to the descent.
   *
   * by Li Yuhong, Jan. 15, 1991
   */
  return 0;
}

/*	Function Name: MaxHeight
 *	Description: Finds the Minium height that will contain a given number
 *                   lines.
 *	Arguments: w - the TextSink Object.
 *                 lines - the number of lines.
 *	Returns: the height.
 */

/* ARGSUSED */
static int
MaxHeight(Widget w, int lines)
{
  return 0;
}

/*	Function Name: SetTabs
 *	Description: Sets the Tab stops.
 *	Arguments: w - the TextSink Object.
 *                 tab_count - the number of tabs in the list.
 *                 tabs - the text positions of the tabs.
 *	Returns: none
 */

/*ARGSUSED*/
static void
SetTabs(Widget w, int tab_count, short *tabs)
{
  return;
}

/*	Function Name: GetCursorBounds
 *	Description: Finds the bounding box for the insert curor (caret).
 *	Arguments: w - the TextSinkObject.
 *                 rect - an X rectance containing the cursor bounds.
 *	Returns: none (fills in rect).
 */

/* ARGSUSED */
static void
GetCursorBounds(Widget w, XRectangle * rect)
{
  rect->x = rect->y = rect->width = rect->height = 0;
}
/************************************************************
 *
 * Public Functions.
 *
 ************************************************************/


/*	Function Name: XawTextSinkDisplayText
 *	Description: Stub function that in subclasses will display text.
 *	Arguments: w - the TextSink Object.
 *                 x, y - location to start drawing text.
 *                 pos1, pos2 - location of starting and ending points
 *                              in the text buffer.
 *                 highlight - hightlight this text?
 *	Returns: none.
 *
 * This function doesn't actually display anything, it is only a place
 * holder.
 */

/* ARGSUSED */
void
XawTextSinkDisplayText(Widget w,
#if NeedWidePrototypes
		       /* Position */ int x, /* Position */ int y,
#else
		       Position x, Position y,
#endif
		       XawTextPosition pos1, XawTextPosition pos2,
#if NeedWidePrototypes
		       /* Boolean */ int highlight)
#else
		       Boolean highlight)
#endif
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  (*class->text_sink_class.DisplayText)(w, x, y, pos1, pos2, highlight);
}

/*	Function Name: XawTextSinkInsertCursor
 *	Description: Places the InsertCursor.
 *	Arguments: w - the TextSink Object.
 *                 x, y - location for the cursor.
 *                 staye - whether to turn the cursor on, or off.
 *	Returns: none.
 *
 * This function doesn't actually display anything, it is only a place
 * holder.
 */

/* ARGSUSED */
void
XawTextSinkInsertCursor(Widget w,
#if NeedWidePrototypes
			int x, int y, int state)
#else
			Position x, Position y, XawTextInsertState state)
#endif
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  (*class->text_sink_class.InsertCursor)(w, x, y, state);
}


/*	Function Name: XawTextSinkClearToBackground
 *	Description: Clears a region of the sink to the background color.
 *	Arguments: w - the TextSink Object.
 *                 x, y  - location of area to clear.
 *                 width, height - size of area to clear
 *	Returns: void.
 *
 * This function doesn't actually display anything, it is only a place
 * holder.
 */

/* ARGSUSED */
void
XawTextSinkClearToBackground (Widget w,
#if NeedWidePrototypes
			      int x, int y, int width, int height)
#else
			      Position x, Position y,
			      Dimension width, Dimension height)
#endif
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  (*class->text_sink_class.ClearToBackground)(w, x, y, width, height);
}

/*	Function Name: XawTextSinkFindPosition
 *	Description: Finds a position in the text.
 *	Arguments: w - the TextSink Object.
 *                 fromPos - reference position.
 *                 fromX   - reference location.
 *                 width,  - width of section to paint text.
 *                 stopAtWordBreak - returned position is a word break?
 *                 resPos - Position to return.      *** RETURNED ***
 *                 resWidth - Width actually used.   *** RETURNED ***
 *                 resHeight - Height actually used. *** RETURNED ***
 *	Returns: none (see above).
 */

/* ARGSUSED */
void
XawTextSinkFindPosition(Widget w, XawTextPosition fromPos, int fromx,
			int width,
#if NeedWidePrototypes
			/* Boolean */ int stopAtWordBreak,
#else
			Boolean stopAtWordBreak,
#endif
			XawTextPosition *resPos, int *resWidth, int *resHeight)
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  (*class->text_sink_class.FindPosition)(w, fromPos, fromx, width,
					 stopAtWordBreak,
					 resPos, resWidth, resHeight);
}

/*	Function Name: XawTextSinkFindDistance
 *	Description: Find the Pixel Distance between two text Positions.
 *	Arguments: w - the TextSink Object.
 *                 fromPos - starting Position.
 *                 fromX   - x location of starting Position.
 *                 toPos   - end Position.
 *                 resWidth - Distance between fromPos and toPos.
 *                 resPos   - Acutal toPos used.
 *                 resHeight - Height required by this text.
 *	Returns: none.
 */

/* ARGSUSED */
void
XawTextSinkFindDistance (Widget w, XawTextPosition fromPos, int fromx,
			 XawTextPosition toPos, int *resWidth,
			 XawTextPosition *resPos, int *resHeight)
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  (*class->text_sink_class.FindDistance)(w, fromPos, fromx, toPos,
					 resWidth, resPos, resHeight);
}

/*	Function Name: XawTextSinkResolve
 *	Description: Resloves a location to a position.
 *	Arguments: w - the TextSink Object.
 *                 pos - a reference Position.
 *                 fromx - a reference Location.
 *                 width - width to move.
 *                 resPos - the resulting position.
 *	Returns: none
 */

/* ARGSUSED */
void
XawTextSinkResolve(Widget w, XawTextPosition pos, int fromx, int width,
		   XawTextPosition *resPos)
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  (*class->text_sink_class.Resolve)(w, pos, fromx, width, resPos);
}

/*	Function Name: XawTextSinkMaxLines
 *	Description: Finds the Maximum number of lines that will fit in
 *                   a given height.
 *	Arguments: w - the TextSink Object.
 *                 height - height to fit lines into.
 *	Returns: the number of lines that will fit.
 */

/* ARGSUSED */
int
XawTextSinkMaxLines(Widget w,
#if NeedWidePrototypes
		    /* Dimension */ int height)
#else
		    Dimension height)
#endif
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  return((*class->text_sink_class.MaxLines)(w, height));
}

/*	Function Name: XawTextSinkMaxHeight
 *	Description: Finds the Minimum height that will contain a given number
 *                   lines.
 *	Arguments: w - the TextSink Object.
 *                 lines - the number of lines.
 *	Returns: the height.
 */

/* ARGSUSED */
int
XawTextSinkMaxHeight(Widget w, int lines)
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  return((*class->text_sink_class.MaxHeight)(w, lines));
}

/*	Function Name: XawTextSinkSetTabs
 *	Description: Sets the Tab stops.
 *	Arguments: w - the TextSink Object.
 *                 tab_count - the number of tabs in the list.
 *                 tabs - the text positions of the tabs.
 *	Returns: none
 */

void
XawTextSinkSetTabs(Widget w, int tab_count, int *tabs)
{
  if (tab_count > 0) {
    TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;
    short *char_tabs = (short*)XtMalloc( (unsigned)tab_count*sizeof(short) );
    short *tab;
    int i;

    for (i = tab_count, tab = char_tabs; i; i--) *tab++ = (short)*tabs++;

    (*class->text_sink_class.SetTabs)(w, tab_count, char_tabs);
    XtFree((char *)char_tabs);
  }
}

/*	Function Name: XawTextSinkGetCursorBounds
 *	Description: Finds the bounding box for the insert curor (caret).
 *	Arguments: w - the TextSinkObject.
 *                 rect - an X rectance containing the cursor bounds.
 *	Returns: none (fills in rect).
 */

/* ARGSUSED */
void
XawTextSinkGetCursorBounds(Widget w, XRectangle *rect)
{
  TextSinkObjectClass class = (TextSinkObjectClass) w->core.widget_class;

  (*class->text_sink_class.GetCursorBounds)(w, rect);
}
