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


Copyright © 2026 David Flater
X11 license (as per the historical licenses that the package inherits)

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/Xfuncs.h>
#include <X11/Xmu/Atoms.h>
#include <X11/Xmu/Misc.h>
#include <X11/Xmu/StdSel.h>	 /* for XmuConvertStandardSelection */
#include <X11/Xutil.h>
#include "XawI18n.h"
#include <X11/Xaw3dXft/AnyStringP.h>
#include <X11/Xaw3dXft/Encoding.h>
#include <X11/Xaw3dXft/MultiSrcP.h>
#include <X11/Xaw3dXft/TextP.h>
#include <X11/Xaw3dXft/TextSink.h>
#include <X11/Xaw3dXft/TextSrc.h>
#include <X11/Xaw3dXft/Xaw3dP.h>
#include <X11/Xaw3dXft/Xaw3dXftP.h>
#include <X11/Xaw3dXft/XawImP.h>

#define SrcScan                XawTextSourceScan
#define FindDist               XawTextSinkFindDistance
#define FindPos                XawTextSinkFindPosition

#define XawTextActionMaxHexChars 100

/*
 * These are defined in TextPop.c
 */

extern void _XawTextInsertFileAction(Widget, XEvent *, String *, Cardinal *);
extern void _XawTextInsertFile(Widget, XEvent *, String *, Cardinal *);
extern void _XawTextSearch(Widget, XEvent *, String *, Cardinal *);
extern void _XawTextDoSearchAction(Widget, XEvent *, String *, Cardinal *);
extern void _XawTextDoReplaceAction(Widget, XEvent *, String *, Cardinal *);
extern void _XawTextSetField(Widget, XEvent *, String *, Cardinal *);
extern void _XawTextPopdownSearchAction(Widget, XEvent *, String *, Cardinal *);

/*
 * These are defined in Text.c
 */

extern char * _XawTextGetText(TextWidget, XawTextPosition, XawTextPosition);
extern void _XawTextAlterSelection(TextWidget, XawTextSelectionMode,
                                   XawTextSelectionAction, String *, Cardinal *);
extern void _XawTextVScroll(TextWidget, int);
extern void _XawTextSetSelection(TextWidget, XawTextPosition, XawTextPosition,
                                 String *, Cardinal);
extern void _XawTextCheckResize(TextWidget);
extern void _XawTextExecuteUpdate(TextWidget);
extern void _XawTextSetScrollBars(TextWidget);
extern void _XawTextClearAndCenterDisplay(TextWidget);
extern Atom * _XawTextSelectionList(TextWidget, String *, Cardinal);
extern void _XawTextPrepareToUpdate(TextWidget);
extern int _XawTextReplace(TextWidget, XawTextPosition, XawTextPosition, XawTextBlock *);

/*
 * These are defined here
 */

static void GetSelection(Widget, Time, String *, Cardinal);
static void KillSelection(TextWidget, XEvent *);
void _XawTextZapSelection(TextWidget, XEvent *, Boolean);

static void
ParameterError(Widget w, String param)
{
    String params[2];
    Cardinal num_params = 2;
    params[0] = XtName(w);
    params[1] = param;

    XtAppWarningMsg( XtWidgetToApplicationContext(w),
	"parameterError", "textAction", "XawError",
	"Widget: %s Parameter: %s",
	params, &num_params);
    XBell( XtDisplay( w ), 50 );
}

static void
StartAction(TextWidget ctx, XEvent *event)
{
  _XawTextPrepareToUpdate(ctx);
  if (event != NULL) {
    switch (event->type) {
    case ButtonPress:
    case ButtonRelease:
      ctx->text.time = event->xbutton.time;
      break;
    case KeyPress:
      KillSelection(ctx, event);
    case KeyRelease:
      ctx->text.time = event->xkey.time;
      break;
    case MotionNotify:
      ctx->text.time = event->xmotion.time;
      break;
    case EnterNotify:
    case LeaveNotify:
      ctx->text.time = event->xcrossing.time;
    }
  }
}

static void
NotePosition(TextWidget ctx, XEvent* event)
{
  switch (event->type) {
  case ButtonPress:
  case ButtonRelease:
    ctx->text.ev_x = event->xbutton.x;
    ctx->text.ev_y = event->xbutton.y;
    break;
  case KeyPress:
  case KeyRelease:
    {
      XRectangle cursor;
      XawTextSinkGetCursorBounds(ctx->text.sink, &cursor);
      ctx->text.ev_x = cursor.x + cursor.width / 2;
      ctx->text.ev_y = cursor.y + cursor.height / 2;
    }
    break;
  case MotionNotify:
    ctx->text.ev_x = event->xmotion.x;
    ctx->text.ev_y = event->xmotion.y;
    break;
  case EnterNotify:
  case LeaveNotify:
    ctx->text.ev_x = event->xcrossing.x;
    ctx->text.ev_y = event->xcrossing.y;
  }
}

static void
EndAction(TextWidget ctx)
{
  _XawTextCheckResize(ctx);
  _XawTextExecuteUpdate(ctx);
  ctx->text.mult = 1;
}

struct _SelectionList {
    String* params;
    Cardinal count;
    Time time;
    uint8_t asked;   /* Which selection encoding currently has been asked for:
			0 = internal encoding, 1 = UTF8_STRING, 2 = STRING */
    Atom selection;
};

/*
  The signature here is as per XtSelectionCallbackPro in Xt docs.

  value:  Specifies a pointer to the selection value. The requesting client
  owns this storage and is responsible for freeing it by calling XtFree when
  it is done with it.

  client_data:  This is the struct _SelectionList pointer from GetSelection.
  We pass it on when applicable and free it at final disposition.
*/
static void
_SelectionReceived(Widget w, XtPointer client_data, Atom *selection, Atom *type,
                   XtPointer value, unsigned long *length, int* format) {
  Display *d = XtDisplay(w);
  TextWidget ctx = (TextWidget)w;
  XawTextBlock text = {0};

  /*
    "The special symbolic constant XT_CONVERT_FAIL is used to indicate that
    the selection conversion failed because the selection owner did not
    respond within the Intrinsics selection timeout interval."
    XT_CONVERT_FAIL is #define XT_CONVERT_FAIL (Atom)0x80000001

    "If the SelectionNotify event returns a property of None, meaning the
    conversion has been refused because there is no owner for the specified
    selection or the owner cannot convert the selection to the requested
    target for any reason, the procedure is called with a value of NULL and a
    length of zero."

    That leaves the possibility of a valid zero-length selection (value is
    not NULL but length is zero).
  */

  assert(type);
  #ifdef TEXT_TRACE
  if (*type == 0)
    printf("SelectionReceived:  received type == 0\n");
  else if (*type == XT_CONVERT_FAIL)
    printf("SelectionReceived:  received type == XT_CONVERT_FAIL\n");
  else {
    char *temp = XGetAtomName(d, *type);
    printf("SelectionReceived:  received type %s\n", temp);
    XFree(temp);
  }
  #endif

  // Fail block
  if (!value || *type == XT_CONVERT_FAIL) {
    struct _SelectionList* list = (struct _SelectionList*)client_data;
    if (list != NULL) {
      #ifdef TEXT_TRACE
      printf("SelectionReceived:  ask %u failed\n", list->asked);
      #endif
      if (list->asked < 2) {
	// Internal encoding > UTF8_STRING > STRING
	XtGetSelectionValue(w, list->selection, (list->asked++ ? XA_STRING :
	  XA_UTF8_STRING(d)), _SelectionReceived, (XtPointer)list, list->time);
	client_data = NULL; // Was freed at final disposition
      } else {
	// All supported encodings failed.  Fall back to the next param.
	if (list->count > 1) {
	  #ifdef TEXT_TRACE
	  printf("SelectionReceived:  proceeding to next param\n");
	  #endif
	  GetSelection(w, list->time, list->params+1, list->count-1);
	} else {
	  // Out of options.
	  #ifdef TEXT_TRACE
	  printf("SelectionReceived:  params exhausted\n");
	  #endif
	}
      }
    } else {
      #ifdef TEXT_TRACE
      printf("SelectionReceived:  fail block entered with null list\n");
      #endif
    }
    goto finish;
  }

  // We got the selection, but its length is 0.
  assert(length);
  if (*length == 0) {
    XtWarning("libXaw3dXft: ignoring received selection of length 0");
    goto finish;
  }

  // format Specifies the size in bits of the data in each element of value.
  // Nobody else uses format != 8.
  assert(format && (*format == 8 ||
                    *format == 32 && *type == XAWA_UTF32_STRING(d)));

  // We got the selection.  Its encoding is in *type.  Get it ready for
  // ReplaceText.
  if (*type != _XawTextInternalEncoding(d, ctx) &&
      *type != XA_UTF8_STRING(d) &&
      *type != XA_STRING) {
    XtWarning("libXaw3dXft: ignoring received selection of unexpected type");
    goto finish;
  }

  StartAction(ctx, NULL);

  if (*type == _XawTextInternalEncoding(d, ctx)) {
    // Length should already be in characters rather than bytes.
    text = (XawTextBlock){0, *length, value, _XawTextFormat(ctx)};
  } else if (_XawTextFormat(ctx) == XawFmtWide) {
    const XawTextEncoding receivedEncoding = (*type == XA_STRING ?
      XawTextEncoding8bit : XawTextEncodingUTF8);
    Cardinal num_bytes = strlen(value);
    void *wcs = Xaw3dXftAnyToWcN(receivedEncoding, value, &num_bytes);
    text = (XawTextBlock){0, num_bytes/sizeof(wchar_t), wcs, XawFmtWide};
  } else {
    if (*type == XA_STRING)
      text = (XawTextBlock){0, *length, value, XawFmt8Bit}; // No conversion
    else {
      Cardinal num_bytes = strlen(value);
      char *cs = Xaw3dXftUTF8To8bitN(value, &num_bytes);
      text = (XawTextBlock){0, num_bytes, cs, XawFmt8Bit};
    }
  }

  if (_XawTextReplace(ctx, ctx->text.insertPos, ctx->text.insertPos, &text)) {
    XBell(d, 0);
    EndAction(ctx);
    goto finish;
  }
  ctx->text.insertPos = SrcScan(ctx->text.source, ctx->text.old_insert,
				XawstPositions, XawsdRight, text.length, True);

  _XawTextSetScrollBars(ctx);
  EndAction(ctx);

  finish:
  // Free all the things
  if (text.ptr && text.ptr != value)
    free(text.ptr);
  if (value)
    XtFree(value);
  if (client_data)
    XtFree(client_data);
}

static void GetSelection (Widget w, Time timev, String *params,
Cardinal num_params) {
  Display *d = XtDisplay(w);
  TextWidget ctx = (TextWidget)w;
  Atom selection;
  int buffer;

  assert(params && num_params);
  #ifdef TEXT_TRACE
  printf("GetSelection num_params = %u\n", num_params);
  for (unsigned i=0; i<num_params; ++i)
    printf("  %u:  %s\n", i, params[i]);
  #endif

  selection = XInternAtom(d, *params, False);
  switch (selection) {
    case XA_CUT_BUFFER0: buffer = 0; break;
    case XA_CUT_BUFFER1: buffer = 1; break;
    case XA_CUT_BUFFER2: buffer = 2; break;
    case XA_CUT_BUFFER3: buffer = 3; break;
    case XA_CUT_BUFFER4: buffer = 4; break;
    case XA_CUT_BUFFER5: buffer = 5; break;
    case XA_CUT_BUFFER6: buffer = 6; break;
    case XA_CUT_BUFFER7: buffer = 7; break;
    default:             buffer = -1;
  }
  if (buffer >= 0) {
    // A cut buffer
    int nbytes;
    char *line = XFetchBuffer(d, &nbytes, buffer);
    if (line) {
      unsigned long length = nbytes;
      int fmt8 = 8;
      Atom type = XA_STRING;
      _SelectionReceived(w, NULL, &selection, &type, line, &length, &fmt8);
    } else if (num_params > 1) {
      #ifdef TEXT_TRACE
      printf("XFetchBuffer %d failed.  Proceeding to next param.\n");
      #endif
      GetSelection(w, timev, params+1, num_params-1);
    } else {
      #ifdef TEXT_TRACE
      printf("XFetchBuffer %d failed and there are no more params.\n");
      #endif
    }
  } else {
    // A selection, not a cut buffer
    struct _SelectionList *list = XtNew(struct _SelectionList);
    *list = (struct _SelectionList){params, num_params, timev, 0, selection};
    XtGetSelectionValue(w, selection, _XawTextInternalEncoding(d, ctx),
      _SelectionReceived, (XtPointer)list, timev);
  }
}

static void
InsertSelection(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  StartAction((TextWidget)w, event); /* Get Time. */
  GetSelection(w, ((TextWidget)w)->text.time, params, *num_params);
  EndAction((TextWidget)w);
}

/************************************************************
 *
 * Routines for Moving Around.
 *
 ************************************************************/

static void
Move(TextWidget ctx, XEvent *event, XawTextScanDirection dir,
     XawTextScanType type, Boolean include)
{
  StartAction(ctx, event);
  ctx->text.insertPos = SrcScan(ctx->text.source, ctx->text.insertPos,
				type, dir, ctx->text.mult, include);
  EndAction(ctx);
}

static void
MoveForwardChar(Widget w, XEvent *event, String *p, Cardinal *n)
{
   Move((TextWidget) w, event, XawsdRight, XawstPositions, TRUE);
}

static void
MoveBackwardChar(Widget w, XEvent *event, String *p, Cardinal *n)
{
  Move((TextWidget) w, event, XawsdLeft, XawstPositions, TRUE);
}

static void
MoveForwardWord(Widget w, XEvent *event, String *p, Cardinal *n)
{
  Move((TextWidget) w, event, XawsdRight, XawstWhiteSpace, FALSE);
}

static void
MoveBackwardWord(Widget w, XEvent *event, String *p, Cardinal *n)
{
  Move((TextWidget) w, event, XawsdLeft, XawstWhiteSpace, FALSE);
}

static void
MoveForwardParagraph(Widget w, XEvent *event, String *p, Cardinal *n)
{
  Move((TextWidget) w, event, XawsdRight, XawstParagraph, FALSE);
}

static void
MoveBackwardParagraph(Widget w, XEvent *event, String *p, Cardinal *n)
{
  Move((TextWidget) w, event, XawsdLeft, XawstParagraph, FALSE);
}

static void
MoveToLineEnd(Widget w, XEvent *event, String *p, Cardinal *n)
{
  Move((TextWidget) w, event, XawsdRight, XawstEOL, FALSE);
}

static void
MoveToLineStart(Widget w, XEvent *event, String *p, Cardinal *n)
{
  Move((TextWidget) w, event, XawsdLeft, XawstEOL, FALSE);
}


static void
MoveLine(TextWidget ctx, XEvent *event, XawTextScanDirection dir)
{
  XawTextPosition new, next_line, junk;
  int from_left, garbage;

  StartAction(ctx, event);

  if (dir == XawsdLeft)
    ctx->text.mult++;

  new = SrcScan(ctx->text.source, ctx->text.insertPos,
		XawstEOL, XawsdLeft, 1, FALSE);

  const Position adjLeft = ctx->text.margins.left -
                           (Position)ctx->text.hscroll_offset;
  FindDist(ctx->text.sink, new, adjLeft, ctx->text.insertPos,
	   &from_left, &junk, &garbage);

  new = SrcScan(ctx->text.source, ctx->text.insertPos, XawstEOL, dir,
		ctx->text.mult, (dir == XawsdRight));

  next_line = SrcScan(ctx->text.source, new, XawstEOL, XawsdRight, 1, FALSE);

  FindPos(ctx->text.sink, new, adjLeft, from_left, FALSE,
	  &(ctx->text.insertPos), &garbage, &garbage);

  if (ctx->text.insertPos > next_line)
    ctx->text.insertPos = next_line;

  EndAction(ctx);
}

static void
MoveNextLine(Widget w, XEvent *event, String *p, Cardinal *n)
{
  MoveLine( (TextWidget) w, event, XawsdRight);
}

static void
MovePreviousLine(Widget w, XEvent *event, String *p, Cardinal *n)
{
  MoveLine( (TextWidget) w, event, XawsdLeft);
}

static void
MoveBeginningOfFile(Widget w, XEvent *event, String *p, Cardinal *n)
{
  Move((TextWidget) w, event, XawsdLeft, XawstAll, TRUE);
}

static void
MoveEndOfFile(Widget w, XEvent *event, String *p, Cardinal *n)
{
  Move((TextWidget) w, event, XawsdRight, XawstAll, TRUE);
}

static void
Scroll(TextWidget ctx, XEvent *event, XawTextScanDirection dir)
{
  StartAction(ctx, event);

  if (dir == XawsdLeft)
    _XawTextVScroll(ctx, ctx->text.mult);
  else
    _XawTextVScroll(ctx, -ctx->text.mult);

  EndAction(ctx);
}

static void
ScrollOneLineUp(Widget w, XEvent *event, String *p, Cardinal *n)
{
  Scroll( (TextWidget) w, event, XawsdLeft);
}

static void
ScrollOneLineDown(Widget w, XEvent *event, String *p, Cardinal *n)
{
  Scroll( (TextWidget) w, event, XawsdRight);
}

static void
MovePage(TextWidget ctx, XEvent *event, XawTextScanDirection dir)
{
  int scroll_val = Max(1, ctx->text.lt.lines - 2);

  if (dir == XawsdLeft)
    scroll_val = -scroll_val;

  StartAction(ctx, event);
  _XawTextVScroll(ctx, scroll_val);
  ctx->text.insertPos = ctx->text.lt.top;
  EndAction(ctx);
}

static void
MoveNextPage(Widget w, XEvent *event, String *p, Cardinal *n)
{
  MovePage((TextWidget) w, event, XawsdRight);
}

static void
MovePreviousPage(Widget w, XEvent *event, String *p, Cardinal *n)
{
  MovePage((TextWidget) w, event, XawsdLeft);
}

/************************************************************
 *
 * Delete Routines.
 *
 ************************************************************/

// Wrapper to route the XtOwnSelection callback to merged code
static Boolean
ConvertSelection(Widget w, Atom *selection, Atom *target, Atom *type,
                 XtPointer* value, unsigned long *length, int *format) {
  return _XawTextConvertSelection(w, selection, target, type, value, length,
    format, False);
}

static void
LoseSelection(Widget w, Atom *selection)
{
  TextWidget ctx = (TextWidget) w;
  Atom* atomP;
  int i;
  XawTextSelectionSalt	*salt, *prevSalt, *nextSalt;

    prevSalt = 0;
    for (salt = ctx->text.salt2; salt; salt = nextSalt)
    {
    	atomP = salt->s.selections;
	nextSalt = salt->next;
    	for (i = 0 ; i < salt->s.atom_count; i++, atomP++)
	    if (*selection == *atomP)
		*atomP = (Atom)0;

    	while (salt->s.atom_count &&
	       salt->s.selections[salt->s.atom_count-1] == 0)
	{
	    salt->s.atom_count--;
	}

    	/*
    	 * Must walk the selection list in opposite order from UnsetSelection.
    	 */

    	atomP = salt->s.selections;
    	for (i = 0 ; i < salt->s.atom_count; i++, atomP++)
    	    if (*atomP == (Atom)0)
 	    {
      	      *atomP = salt->s.selections[--salt->s.atom_count];
      	      while (salt->s.atom_count &&
	     	     salt->s.selections[salt->s.atom_count-1] == 0)
    	    	salt->s.atom_count--;
    	    }
	if (salt->s.atom_count == 0)
	{
	    XtFree ((char *) salt->s.selections);

            /* WARNING: the next line frees memory not allocated in Xaw. */
            /* Could be a serious bug.  Someone look into it. */
	    XtFree (salt->contents);
	    if (prevSalt)
		prevSalt->next = nextSalt;
	    else
		ctx->text.salt2 = nextSalt;
	    XtFree ((char *) salt);
	}
	else
	    prevSalt = salt;
    }
}

static void _DeleteOrKill (TextWidget ctx, XawTextPosition from,
XawTextPosition to, Boolean kill) {
  XawTextBlock text;

  if (kill && from < to) {
    XawTextSelectionSalt    *salt;
    Atom selection = XInternAtom(XtDisplay(ctx), "SECONDARY", False);

    LoseSelection ((Widget) ctx, &selection);
    salt = (XawTextSelectionSalt *) XtMalloc (sizeof (XawTextSelectionSalt));
    if (!salt)
	return;
    salt->s.selections = (Atom *) XtMalloc (sizeof (Atom));
    if (!salt->s.selections)
    {
	XtFree ((char *) salt);
	return;
    }
    #ifdef TEXT_TRACE
    printf("DeleteOrKill:  putting text into salt->contents\n");
    #endif
    salt->s.left = from;
    salt->s.right = to;
    salt->s.type = XawselectNull; // irrelevant I guess
    salt->contents = _XawTextGetText(ctx, from, to);
    salt->next = ctx->text.salt2;
    ctx->text.salt2 = salt;
    salt->s.selections[0] = selection;
    XtOwnSelection ((Widget) ctx, selection, ctx->text.time,
		    ConvertSelection, LoseSelection, NULL);
    salt->s.atom_count = 1;
/*
    XStoreBuffer(XtDisplay(ctx), ptr, strlen(ptr), 1);
    XtFree(ptr);
*/
  }
  text.length = 0;
  text.firstPos = 0;

  text.format = _XawTextFormat(ctx);
  text.ptr = "";  /* These two lines needed to make legal TextBlock */

  if (_XawTextReplace(ctx, from, to, &text)) {
    XBell(XtDisplay(ctx), 50);
    return;
  }
  ctx->text.insertPos = from;
  ctx->text.showposition = TRUE;
}

static void
DeleteOrKill(TextWidget ctx, XEvent *event, XawTextScanDirection dir,
             XawTextScanType type, Boolean include, Boolean kill)
{
  XawTextPosition from, to;
  XawTextBlock text;

  StartAction(ctx, event);
  to = SrcScan(ctx->text.source, ctx->text.insertPos,
	       type, dir, ctx->text.mult, include);

/*
 * If no movement actually happened, then bump the count and try again.
 * This causes the character position at the very beginning and end of
 * a boundary to act correctly.
 */

  if (to == ctx->text.insertPos)
      to = SrcScan(ctx->text.source, ctx->text.insertPos,
		   type, dir, ctx->text.mult + 1, include);

  if (dir == XawsdLeft) {
    from = to;
    to = ctx->text.insertPos;
  }
  else { /* dir == XawsdRight */
    from = ctx->text.insertPos;
  }

  _DeleteOrKill(ctx, from, to, kill);
  _XawTextSetScrollBars(ctx);
  EndAction(ctx);
}

static void
KillSelection(TextWidget ctx, XEvent *event)
{
  char strbuf[BUFSIZ];
  KeySym keysym;
  /* Option to delete selection, if any */
  if ( (_Xaw3dXft->edit_delete_alternative >= 2) &&
    XLookupString((XKeyEvent*)event, strbuf, BUFSIZ, &keysym, NULL))
  {
    if (keysym != XK_BackSpace && keysym != XK_Delete &&
	(ctx->text.s.left != ctx->text.s.right))
        _DeleteOrKill(ctx, ctx->text.s.left, ctx->text.s.right, 1);
  }
}

static void
DeleteForwardChar(Widget w, XEvent *event, String *p, Cardinal *n)
{
  DeleteOrKill((TextWidget) w, event, XawsdRight, XawstPositions, TRUE, FALSE);
}

static void
DeleteBackwardChar(Widget w, XEvent *event, String *p, Cardinal *n)
{
  TextWidget ctx = (TextWidget) w;
  if ( _Xaw3dXft->edit_delete_alternative &&
      (ctx->text.s.left != ctx->text.s.right))
  	_XawTextZapSelection( (TextWidget) w, event, TRUE);
  else
  	DeleteOrKill((TextWidget) w, event, XawsdLeft, XawstPositions, TRUE, FALSE);
}

static void
DeleteBackwardCharOnly(Widget w, XEvent *event, String *p, Cardinal *n)
{
  DeleteOrKill((TextWidget) w, event, XawsdLeft, XawstPositions, TRUE, FALSE);
}

static void
DeleteForwardWord(Widget w, XEvent *event, String *p, Cardinal *n)
{
  DeleteOrKill((TextWidget) w, event,
	       XawsdRight, XawstWhiteSpace, FALSE, FALSE);
}

static void
DeleteBackwardWord(Widget w, XEvent *event, String *p, Cardinal *n)
{
  DeleteOrKill((TextWidget) w, event,
	       XawsdLeft, XawstWhiteSpace, FALSE, FALSE);
}

static void
KillForwardWord(Widget w, XEvent *event, String *p, Cardinal *n)
{
  DeleteOrKill((TextWidget) w, event,
	       XawsdRight, XawstWhiteSpace, FALSE, TRUE);
}

static void
KillBackwardWord(Widget w, XEvent *event, String *p, Cardinal *n)
{
  DeleteOrKill((TextWidget) w, event,
	       XawsdLeft, XawstWhiteSpace, FALSE, TRUE);
}

static void
KillToEndOfLine(Widget w, XEvent *event, String *p, Cardinal *n)
{
  TextWidget ctx = (TextWidget) w;
  XawTextPosition end_of_line;

  StartAction(ctx, event);
  end_of_line = SrcScan(ctx->text.source, ctx->text.insertPos, XawstEOL,
			XawsdRight, ctx->text.mult, FALSE);
  if (end_of_line == ctx->text.insertPos)
    end_of_line = SrcScan(ctx->text.source, ctx->text.insertPos, XawstEOL,
			  XawsdRight, ctx->text.mult, TRUE);

  _DeleteOrKill(ctx, ctx->text.insertPos, end_of_line, TRUE);
  _XawTextSetScrollBars(ctx);
  EndAction(ctx);
}

static void
KillToEndOfParagraph(Widget w, XEvent *event, String *p, Cardinal *n)
{
  DeleteOrKill((TextWidget) w, event, XawsdRight, XawstParagraph, FALSE, TRUE);
}

void
_XawTextZapSelection(TextWidget ctx, XEvent *event, Boolean kill)
{
   StartAction(ctx, event);
   _DeleteOrKill(ctx, ctx->text.s.left, ctx->text.s.right, kill);
  _XawTextSetScrollBars(ctx);
   EndAction(ctx);
}

static void
KillCurrentSelection(Widget w, XEvent *event, String *p, Cardinal *n)
{
  _XawTextZapSelection( (TextWidget) w, event, TRUE);
}

static void
KillCurrentSelectionOrBackwardChar(Widget w, XEvent *event, String *p, Cardinal *n)
{
  TextWidget ctx = (TextWidget) w;
  if (ctx->text.s.left != ctx->text.s.right)
  	_XawTextZapSelection( (TextWidget) w, event, TRUE);
  else
  	DeleteOrKill((TextWidget) w, event, XawsdLeft, XawstPositions, TRUE, FALSE);
}

static void
DeleteCurrentSelection(Widget w, XEvent *event, String *p, Cardinal *n)
{
  _XawTextZapSelection( (TextWidget) w, event, FALSE);
}

/************************************************************
 *
 * Insertion Routines.
 *
 ************************************************************/

static int
InsertNewLineAndBackupInternal(TextWidget ctx)
{
  int count, error = XawEditDone;
  XawTextBlock text;

  text.format = _XawTextFormat(ctx);
  text.length = ctx->text.mult;
  text.firstPos = 0;

  if ( text.format == XawFmtWide ) {
      wchar_t* wptr;
      text.ptr =  XtMalloc(sizeof(wchar_t) * ctx->text.mult);
      wptr = (wchar_t *)text.ptr;
      for (count = 0; count < ctx->text.mult; count++ )
          wptr[count] = L'\n';
  }
  else
  {
      text.ptr = XtMalloc(sizeof(char) * ctx->text.mult);
      for (count = 0; count < ctx->text.mult; count++ )
          text.ptr[count] = '\n';
  }

  if (_XawTextReplace(ctx, ctx->text.insertPos, ctx->text.insertPos, &text)) {
    XBell( XtDisplay(ctx), 50);
    error = XawEditError;
  }
  else
    ctx->text.showposition = TRUE;

  XtFree( text.ptr );
  return( error );
}

static void
InsertNewLineAndBackup(Widget w, XEvent *event, String *p, Cardinal *n)
{
  StartAction( (TextWidget) w, event );
  (void) InsertNewLineAndBackupInternal( (TextWidget) w );
  _XawTextSetScrollBars( (TextWidget) w);
  EndAction( (TextWidget) w );
}

static int
LocalInsertNewLine(TextWidget ctx, XEvent *event)
{
  StartAction(ctx, event);
  if (InsertNewLineAndBackupInternal(ctx) == XawEditError)
    return(XawEditError);
  ctx->text.insertPos = SrcScan(ctx->text.source, ctx->text.insertPos,
			     XawstPositions, XawsdRight, ctx->text.mult, TRUE);
  _XawTextSetScrollBars(ctx);
  EndAction(ctx);
  return(XawEditDone);
}

static void
InsertNewLine(Widget w, XEvent *event, String *p, Cardinal *n)
{
  (void) LocalInsertNewLine( (TextWidget) w, event);
}

static void
InsertNewLineAndIndent(Widget w, XEvent *event, String *p, Cardinal *n)
{
  XawTextBlock text;
  XawTextPosition pos1;
  int length;
  TextWidget ctx = (TextWidget) w;
  String line_to_ip;

  StartAction(ctx, event);
  pos1 = SrcScan(ctx->text.source, ctx->text.insertPos,
		 XawstEOL, XawsdLeft, 1, FALSE);

  line_to_ip = _XawTextGetText(ctx, pos1, ctx->text.insertPos);

  text.format = _XawTextFormat(ctx);
  text.firstPos = 0;

  if ( text.format == XawFmtWide ) {
     wchar_t* ptr;
     text.ptr = XtMalloc( ( 2 + wcslen((wchar_t*)line_to_ip) ) * sizeof(wchar_t) );

     ptr = (wchar_t*)text.ptr;
     ptr[0] = L'\n';
     wcscpy( (wchar_t*) ++ptr, (wchar_t*) line_to_ip );

     length = wcslen((wchar_t*)text.ptr);
     while ( length && ( iswspace(*ptr) || ( *ptr == L'\t' ) ) )
         ptr++, length--;
     *ptr = (wchar_t)0;
     text.length = wcslen((wchar_t*)text.ptr);

  } else {
     char *ptr;
     length = strlen(line_to_ip);
     /* The current line + \0 and LF will be copied to this
	buffer. Before my fix, only length + 1 bytes were
	allocated, causing on machine with non-wasteful
	malloc implementation segmentation violations by
	overwriting the bypte after the allocated area

	-gustaf neumann
      */
     text.ptr = XtMalloc( ( 2 + length ) * sizeof( char ) );

     ptr = text.ptr;
     ptr[0] = '\n';
     strcpy( ++ptr, line_to_ip );

     length++;
     while ( length && ( isspace(*ptr) || ( *ptr == '\t' ) ) )
         ptr++, length--;
     *ptr = '\0';
     text.length = strlen(text.ptr);
  }
  XtFree( line_to_ip );

  if (_XawTextReplace(ctx,ctx->text.insertPos, ctx->text.insertPos, &text)) {
    XBell(XtDisplay(ctx), 50);
    XtFree(text.ptr);
    EndAction(ctx);
    return;
  }
  XtFree(text.ptr);
  ctx->text.insertPos = SrcScan(ctx->text.source, ctx->text.insertPos,
				XawstPositions, XawsdRight, text.length, TRUE);
  _XawTextSetScrollBars(ctx);
  EndAction(ctx);
}

/************************************************************
 *
 * Selection Routines.
 *
 *************************************************************/

static void
SelectWord(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  TextWidget ctx = (TextWidget) w;
  XawTextPosition l, r;

  StartAction(ctx, event);
  l = SrcScan(ctx->text.source, ctx->text.insertPos,
	      XawstWhiteSpace, XawsdLeft, 1, FALSE);
  r = SrcScan(ctx->text.source, l, XawstWhiteSpace, XawsdRight, 1, FALSE);
  _XawTextSetSelection(ctx, l, r, params, *num_params);
  EndAction(ctx);
}

static void
SelectAll(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  TextWidget ctx = (TextWidget) w;

  StartAction(ctx, event);
  _XawTextSetSelection(ctx,zeroPosition,ctx->text.lastPos,params,*num_params);
  EndAction(ctx);
}

static void
ModifySelection(TextWidget ctx, XEvent *event, XawTextSelectionMode mode,
                XawTextSelectionAction action, String *params, Cardinal *num_params)
{
  StartAction(ctx, event);
  NotePosition(ctx, event);
  _XawTextAlterSelection(ctx, mode, action, params, num_params);
  EndAction(ctx);
}

static void
SelectStart(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  ModifySelection((TextWidget) w, event,
		  XawsmTextSelect, XawactionStart, params, num_params);
}

static void
SelectAdjust(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  ModifySelection((TextWidget) w, event,
		  XawsmTextSelect, XawactionAdjust, params, num_params);
}

static void
SelectEnd(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  ModifySelection((TextWidget) w, event,
		  XawsmTextSelect, XawactionEnd, params, num_params);
}

static void
ExtendStart(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  ModifySelection((TextWidget) w, event,
		  XawsmTextExtend, XawactionStart, params, num_params);
}

static void
ExtendAdjust(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  ModifySelection((TextWidget) w, event,
		  XawsmTextExtend, XawactionAdjust, params, num_params);
}

static void
ExtendEnd(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  ModifySelection((TextWidget) w, event,
		  XawsmTextExtend, XawactionEnd, params, num_params);
}

static void
SelectSave(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    int	    num_atoms;
    Display* dpy = XtDisplay(w);
    Atom    selections[256];

    StartAction(  (TextWidget) w, event );
    num_atoms = *num_params;
    if (num_atoms > 256)
	num_atoms = 256;
    XInternAtoms(dpy, (char **)params, num_atoms, False, selections);
    _XawTextSaltAwaySelection( (TextWidget) w, selections, num_atoms );
    EndAction(  (TextWidget) w );
}

/************************************************************
 *
 * Misc. Routines.
 *
 ************************************************************/

static void
RedrawDisplay(Widget w, XEvent *event, String *p, Cardinal *n)
{
  StartAction( (TextWidget) w, event);
  _XawTextClearAndCenterDisplay((TextWidget) w);
  EndAction( (TextWidget) w);
}

static void
TextFocusIn (Widget w, XEvent *event, String *p, Cardinal *n)
{
  TextWidget ctx = (TextWidget) w;

  /* Let the input method know focus has arrived. */
  _XawImSetFocusValues (w, NULL, 0);
  if ( event->xfocus.detail == NotifyPointer ) return;

  ctx->text.hasfocus = TRUE;
}

static void
TextFocusOut(Widget w, XEvent *event, String *p, Cardinal *n)
{
  TextWidget ctx = (TextWidget) w;

  /* Let the input method know focus has left.*/
  _XawImUnsetFocus(w);
  if ( event->xfocus.detail == NotifyPointer ) return;
  ctx->text.hasfocus = FALSE;
}

static void
TextEnterWindow(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  TextWidget ctx = (TextWidget) w;

  if ((event->xcrossing.detail != NotifyInferior) && event->xcrossing.focus &&
      !ctx->text.hasfocus) {
	_XawImSetFocusValues(w, NULL, 0);
  }
}

static void
TextLeaveWindow(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  TextWidget ctx = (TextWidget) w;

  if ((event->xcrossing.detail != NotifyInferior) && event->xcrossing.focus &&
      !ctx->text.hasfocus) {
	_XawImUnsetFocus(w);
  }
}

/*	Function Name: AutoFill
 *	Description: Breaks the line at the previous word boundary when
 *                   called inside InsertChar.
 *	Arguments: ctx - The text widget.
 *	Returns: none
 */

static void
AutoFill(TextWidget ctx)
{
  int width, height, x, line_num, max_width;
  XawTextPosition ret_pos;
  XawTextBlock text;

  if ( !((ctx->text.auto_fill) && (ctx->text.mult == 1)) )
    return;

  for ( line_num = 0; line_num < ctx->text.lt.lines ; line_num++)
    if ( ctx->text.lt.info[line_num].position >= ctx->text.insertPos )
      break;
  line_num--;			/* backup a line. */

  max_width = Max(0, (int)(ctx->core.width - HMarginsOffset(ctx)));

  x = ctx->text.margins.left - (int)ctx->text.hscroll_offset;
  XawTextSinkFindPosition( ctx->text.sink,ctx->text.lt.info[line_num].position,
			  x, max_width, TRUE, &ret_pos, &width, &height);

  if ( ret_pos >= ctx->text.insertPos )
    return;

  text.format = XawFmt8Bit;
  if (_XawTextFormat(ctx) == XawFmtWide) {
    text.format = XawFmtWide;
    text.ptr =  (char *)XtMalloc(sizeof(wchar_t) * 2);
    ((wchar_t*)text.ptr)[0] = L'\n';
    ((wchar_t*)text.ptr)[1] = 0;
  } else
    text.ptr = "\n";
  text.length = 1;
  text.firstPos = 0;

  if (_XawTextReplace(ctx, ret_pos - 1, ret_pos, &text))
    XBell(XtDisplay((Widget) ctx), 0);	/* Unable to edit, complain. */
}

static void
InsertChar(Widget w, XEvent *event, String *p, Cardinal *n)
{
  TextWidget ctx = (TextWidget) w;
  char *ptr, strbuf[BUFSIZ];
  int count, error;
  KeySym keysym;
  XawTextBlock text;
  Status status;

  if (XtIsSubclass (ctx->text.source, (WidgetClass) multiSrcObjectClass))
    text.length = _XawImWcLookupString (w, &event->xkey,
		(wchar_t*) strbuf, BUFSIZ, &keysym);
  else
    text.length = XLookupString ((XKeyEvent*)event, strbuf, BUFSIZ, &keysym, NULL);

  if (text.length == 0)
      return;

  text.format = _XawTextFormat( ctx );
  if ( text.format == XawFmtWide ) {
      text.ptr = ptr = XtMalloc(sizeof(wchar_t) * text.length * ctx->text.mult );
      for (count = 0; count < ctx->text.mult; count++ ) {
          memcpy((char*) ptr, (char *)strbuf, sizeof(wchar_t) * text.length );
          ptr += sizeof(wchar_t) * text.length;
      }

  } else { /* == XawFmt8Bit */
      text.ptr = ptr = XtMalloc( sizeof(char) * text.length * ctx->text.mult );
      for ( count = 0; count < ctx->text.mult; count++ ) {
          strncpy( ptr, strbuf, text.length );
          ptr += text.length;
      }
  }

  text.length = text.length * ctx->text.mult;
  text.firstPos = 0;

  StartAction(ctx, event);

  error = _XawTextReplace(ctx, ctx->text.insertPos,ctx->text.insertPos, &text);

  if (error == XawEditDone) {
      ctx->text.insertPos = SrcScan(ctx->text.source, ctx->text.insertPos,
	      XawstPositions, XawsdRight, text.length, TRUE);
      AutoFill(ctx);
  }
  else
      XBell(XtDisplay(ctx), 50);

  XtFree(text.ptr);
  _XawTextSetScrollBars(ctx);
  EndAction(ctx);
}


/* IfHexConvertHexElseReturnParam() - called by InsertString
 *
 * i18n requires the ability to specify multiple characters in a hexa-
 * decimal string at once.  Since Insert was already too long, I made
 * this a separate routine.
 *
 * A legal hex string in MBNF: '0' 'x' ( HEX-DIGIT HEX-DIGIT )+ '\0'
 *
 * WHEN:    the passed param is a legal hex string
 * RETURNS: a pointer to that converted, null terminated hex string;
 *          len_return holds the character count of conversion result
 *
 * WHEN:    the passed param is not a legal hex string:
 * RETURNS: the parameter passed;
 *          len_return holds the char count of param.
 *
 * NOTE:    In neither case will there be strings to free. */

static char*
IfHexConvertHexElseReturnParam(char *param, int *len_return)
{
  char *p;                     /* steps through param char by char */
  char c;                      /* holds the character pointed to by p */

  int ind;		       /* steps through hexval buffer char by char */
  static char hexval[ XawTextActionMaxHexChars ];
  Boolean first_digit;

  /* reject if it doesn't begin with 0x and at least one more character. */

  if ( ( param[0] != '0' ) || ( param[1] != 'x' ) || ( param[2] == '\0' ) ) {
      *len_return = strlen( param );
      return( param );
  }

  /* Skip the 0x; go character by character shifting and adding. */

  first_digit = True;
  ind = 0;
  hexval[ ind ] = '\0';

  for ( p = param+2; ( c = *p ); p++ ) {
      hexval[ ind ] *= 16;
      if (c >= '0' && c <= '9')
          hexval[ ind ] += c - '0';
      else if (c >= 'a' && c <= 'f')
          hexval[ ind ] += c - 'a' + 10;
      else if (c >= 'A' && c <= 'F')
          hexval[ ind ] += c - 'A' + 10;
      else break;

      /* If we didn't break in preceding line, it was a good hex char. */

      if ( first_digit )
          first_digit = False;
      else {
          first_digit = True;
          if ( ++ind < XawTextActionMaxHexChars )
              hexval[ ind ] = '\0';
          else {
              *len_return = strlen( param );
              return( param );
          }
      }
  }

  /* We quit the above loop because we hit a non hex.  If that char is \0... */

  if ( ( c == '\0' ) && first_digit ) {
      *len_return = strlen( hexval );
      return( hexval );       /* ...it was a legal hex string, so return it.*/
  }

  /* Else, there were non-hex chars or odd digit count, so... */

  *len_return = strlen( param );
  return( param );			   /* ...return the verbatim string. */
}


/* InsertString() - action
 *
 * Mostly rewritten for R6 i18n.
 *
 * Each parameter, in turn, will be insert at the inputPos
 * and the inputPos advances to the insertion's end.
 *
 * The exception is that parameters composed of the two
 * characters 0x, followed only by an even number of
 * hexadecimal digits will be converted to characters. */

static void InsertString (Widget w, XEvent *event, String *params,
Cardinal *num_params) {
  TextWidget ctx = (TextWidget) w;
  XtAppContext app_con = XtWidgetToApplicationContext(w);
  XawTextBlock text;
  int	   i;

  text.firstPos = 0;
  text.format = _XawTextFormat(ctx);

  StartAction(ctx, event);
  for (i = *num_params; i; i--, params++) {
    /*
      I did not debug the hex encoding option.  0xAABBCCDDEE becomes the
      bytes AA, BB, CC, DD, EE in sequence, and so on for any even number of
      hex chars after the 0x.  The docs say "When the international resource
      is true, a hexadecimal string is intrepeted as being in a multi-byte
      encoding."  Now it gets interpreted as UTF-8.  It would be better to
      have a sequence of Unicode code points, each one introduced by 0x so
      that you get the byte order correct, and then slam it into 8bit or Wc
      with no further translation.
    */
    text.ptr = IfHexConvertHexElseReturnParam(*params, &text.length);
    if (text.length == 0) continue;

    // If Text is not 8bit, assume that the parameters are UTF-8.
    Boolean ptrIsTemp = False;
    if (text.format == XawFmtWide) {
      ptrIsTemp = True;
      Cardinal num_bytes = text.length;
      text.ptr = (char *)Xaw3dXftAnyToWcN(XawTextEncodingUTF8, text.ptr,
	&num_bytes);
      text.length = num_bytes / sizeof(wchar_t);
    }

    if (_XawTextReplace(ctx, ctx->text.insertPos,
			ctx->text.insertPos, &text)) {
      XBell(XtDisplay(ctx), 50);
      EndAction(ctx);
      if (ptrIsTemp)
	free(text.ptr);
      return;
    }

    /* Advance insertPos to the end of the string we just inserted. */
    ctx->text.insertPos = SrcScan(ctx->text.source, ctx->text.insertPos,
			  XawstPositions, XawsdRight, text.length, True);
    if (ptrIsTemp)
      free(text.ptr);
  }
  EndAction(ctx);
}


/* DisplayCaret() - action
 *
 * The parameter list should contain one boolean value.  If the
 * argument is true, the cursor will be displayed.  If false, not.
 *
 * The exception is that EnterNotify and LeaveNotify events may
 * have a second argument, "always".  If they do not, the cursor
 * is only affected if the focus member of the event is true.	*/

static void
DisplayCaret(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  TextWidget ctx = (TextWidget)w;
  Boolean display_caret = True;

  if  ( ( event->type == EnterNotify || event->type == LeaveNotify ) &&
        ( ( *num_params >= 2 ) && ( strcmp( params[1], "always" ) == 0 ) ) &&
        ( !event->xcrossing.focus ) )
      return;

  if (*num_params > 0) {	/* default arg is "True" */
      XrmValue from, to;
      from.size = strlen(from.addr = params[0]);
      XtConvert( w, XtRString, &from, XtRBoolean, &to );

      if ( to.addr != NULL )
          display_caret = *(Boolean*)to.addr;
      if ( ctx->text.display_caret == display_caret )
          return;
  }
  StartAction(ctx, event);
  ctx->text.display_caret = display_caret;
  EndAction(ctx);
}


/* Multiply() - action
 *
 * The parameter list may contain either a number or the string 'Reset'.
 *
 * A number will multiply the current multiplication factor by that number.
 * Many of the text widget actions will will perform n actions, where n is
 * the multiplication factor.
 *
 * The string reset will reset the mutiplication factor to 1. */

static void
Multiply(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  TextWidget ctx = (TextWidget) w;
  int mult;

  if (*num_params != 1) {
      XtAppError( XtWidgetToApplicationContext( w ),
	       "Xaw Text Widget: multiply() takes exactly one argument.");
      XBell( XtDisplay( w ), 0 );
      return;
  }

  if ( ( params[0][0] == 'r' ) || ( params[0][0] == 'R' ) ) {
      XBell( XtDisplay( w ), 0 );
      ctx->text.mult = 1;
      return;
  }

  if ( ( mult = atoi( params[0] ) ) == 0 ) {
      char buf[ BUFSIZ ];
      sprintf(buf, "%s %s", "Xaw Text Widget: multiply() argument",
	    "must be a number greater than zero, or 'Reset'." );
      XtAppError( XtWidgetToApplicationContext( w ), buf );
      XBell( XtDisplay( w ), 50 );
      return;
  }

  ctx->text.mult *= mult;
}


/* StripOutOldCRs() - called from FormRegion
 *
 * removes CRs in widget ctx, from from to to.
 *
 * RETURNS: the new ending location (we may add some characters),
 * or XawReplaceError if the widget can't be written to. */

static XawTextPosition
StripOutOldCRs(TextWidget ctx, XawTextPosition from, XawTextPosition to)
{
  XawTextPosition startPos, endPos, eop_begin, eop_end, temp;
  Widget src = ctx->text.source;
  XawTextBlock text;
  char *buf;

  /* Initialize our TextBlock with two spaces. */

  text.firstPos = 0;
  text.format = _XawTextFormat(ctx);
  if ( text.format == XawFmt8Bit )
      text.ptr= "  ";
  else {
      static wchar_t wc_two_spaces[ 3 ];
      wc_two_spaces[0] = L' ';
      wc_two_spaces[1] = L' ';
      wc_two_spaces[2] = 0;
      text.ptr = (char*) wc_two_spaces;
  }

  /* Strip out CR's. */

  eop_begin = eop_end = startPos = endPos = from;
  /* CONSTCOND */
  while (TRUE) {
      endPos=SrcScan(src, startPos, XawstEOL, XawsdRight, 1, FALSE);

      temp = SrcScan(src, endPos, XawstWhiteSpace, XawsdLeft, 1, FALSE);
      temp = SrcScan(src, temp,   XawstWhiteSpace, XawsdRight,1, FALSE);

      if (temp > startPos)
          endPos = temp;

      if (endPos >= to)
          break;

      if (endPos >= eop_begin) {
          startPos = eop_end;
          eop_begin=SrcScan(src, startPos, XawstParagraph, XawsdRight, 1,FALSE);
          eop_end = SrcScan(src, startPos, XawstParagraph, XawsdRight, 1, TRUE);
      }
    else {
      XawTextPosition periodPos, next_word;
      int i, len;

      periodPos= SrcScan(src, endPos, XawstPositions, XawsdLeft, 1, TRUE);
      next_word = SrcScan(src, endPos, XawstWhiteSpace, XawsdRight, 1, FALSE);

      len = next_word - periodPos;

      text.length = 1;
      buf = _XawTextGetText(ctx, periodPos, next_word);
      if (text.format == XawFmtWide) {
        if ( (periodPos < endPos) && (((wchar_t*)buf)[0] == L'.'))
          text.length++;
      } else
        if ( (periodPos < endPos) && (buf[0] == '.') )
	  text.length++;	/* Put in two spaces. */

      /*
       * Remove all extra spaces.
       */

      for (i = 1 ; i < len; i++)
        if (text.format ==  XawFmtWide) {
          if ( !iswspace(((wchar_t*)buf)[i]) || ((periodPos + i) >= to) ) {
             break;
          }
        } else
	  if ( !isspace(buf[i]) || ((periodPos + i) >= to) ) {
	      break;
	  }

      XtFree(buf);

      to -= (i - text.length - 1);
      startPos = SrcScan(src, periodPos, XawstPositions, XawsdRight, i, TRUE);
      if (_XawTextReplace(ctx, endPos, startPos, &text) != XawEditDone)
	  return XawReplaceError;
      startPos -= i - text.length;
    }
  }
  return(to);
}


/* InsertNewCRs() - called from FormRegion
 *
 * inserts new CRs for FormRegion, thus for FormParagraph action */

static void
InsertNewCRs(TextWidget ctx, XawTextPosition from, XawTextPosition to)
{
  XawTextPosition startPos, endPos, space, eol;
  XawTextBlock text;
  int i, width, height, len;
  char * buf;

  text.firstPos = 0;
  text.length = 1;
  text.format = _XawTextFormat( ctx );

  if ( text.format == XawFmt8Bit )
      text.ptr = "\n";
  else {
      static wchar_t wide_CR[ 2 ];
      wide_CR[0] = L'\n';
      wide_CR[1] = 0;
      text.ptr = (char*) wide_CR;
  }

  startPos = from;
  /* CONSTCOND */
  while (TRUE) {
      XawTextSinkFindPosition( ctx->text.sink, startPos,
	(int)ctx->text.margins.left - (int)ctx->text.hscroll_offset,
			    (int) (ctx->core.width - HMarginsOffset(ctx)),
			    TRUE, &eol, &width, &height);
      if (eol >= to)
          break;

      eol  = SrcScan(ctx->text.source, eol, XawstPositions, XawsdLeft, 1, TRUE);
      space= SrcScan(ctx->text.source, eol, XawstWhiteSpace,XawsdRight,1, TRUE);

      startPos = endPos = eol;
      if (eol == space)
          return;

      len = (int) (space - eol);
      buf = _XawTextGetText(ctx, eol, space);
      for ( i = 0 ; i < len ; i++)
      if (text.format == XawFmtWide) {
          if (!iswspace(((wchar_t*)buf)[i]))
              break;
      } else
          if (!isspace(buf[i]))
              break;

      to -= (i - 1);
      endPos = SrcScan(ctx->text.source, endPos,
		     XawstPositions, XawsdRight, i, TRUE);
      XtFree(buf);

      if (_XawTextReplace(ctx, startPos, endPos, &text))
          return;

      startPos = SrcScan(ctx->text.source, startPos,
		       XawstPositions, XawsdRight, 1, TRUE);
  }
}


/* FormRegion() - called by FormParagraph
 *
 * oversees the work of paragraph-forming a region
 *
 * RETURNS: XawEditDone if successful, or XawReplaceError. */

static int
FormRegion(TextWidget ctx, XawTextPosition from, XawTextPosition to)
{
  if ( from >= to ) return XawEditDone;

  if ( ( to = StripOutOldCRs( ctx, from, to ) ) == XawReplaceError )
      return XawReplaceError;

  /* insure that the insertion point is within legal bounds */
  if ( ctx->text.insertPos > SrcScan( ctx->text.source, 0,
				       XawstAll, XawsdRight, 1, TRUE ) )
      ctx->text.insertPos = to;

  InsertNewCRs(ctx, from, to);
  _XawTextBuildLineTable(ctx, ctx->text.lt.top, TRUE);
  return XawEditDone;
}


/* FormParagraph() - action
 *
 * removes and reinserts CRs to maximize line length without clipping */

static void
FormParagraph(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  TextWidget ctx = (TextWidget) w;
  XawTextPosition from, to;

  StartAction(ctx, event);

  from =  SrcScan( ctx->text.source, ctx->text.insertPos,
		  XawstParagraph, XawsdLeft, 1, FALSE );
  to  =  SrcScan( ctx->text.source, from,
		 XawstParagraph, XawsdRight, 1, FALSE );

  if ( FormRegion( ctx, from, to ) == XawReplaceError )
      XBell( XtDisplay( w ), 0 );
  _XawTextSetScrollBars( ctx );
  EndAction( ctx );
}


/* TransposeCharacters() - action
 *
 * Swaps the character to the left of the mark
 * with the character to the right of the mark. */

static void
TransposeCharacters(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  TextWidget ctx = (TextWidget) w;
  XawTextPosition start, end;
  XawTextBlock text;
  char* buf;
  int i;

  StartAction(ctx, event);

  /* Get bounds. */

  start = SrcScan( ctx->text.source, ctx->text.insertPos, XawstPositions,
		  XawsdLeft, 1, TRUE );
  end = SrcScan( ctx->text.source, ctx->text.insertPos, XawstPositions,
		XawsdRight, ctx->text.mult, TRUE );

  /* Make sure we aren't at the very beginning or end of the buffer. */

  if ( ( start == ctx->text.insertPos ) || ( end == ctx->text.insertPos ) ) {
      XBell( XtDisplay( w ), 0 );	/* complain. */
      EndAction( ctx );
      return;
  }

  ctx->text.insertPos = end;

  text.firstPos = 0;
  text.format = _XawTextFormat(ctx);

  /* Retrieve text and swap the characters. */

  if ( text.format == XawFmtWide) {
      wchar_t wc;
      wchar_t* wbuf;

      wbuf = (wchar_t*) _XawTextGetText(ctx, start, end);
      text.length = wcslen( wbuf );
      wc = wbuf[ 0 ];
      for ( i = 1; i < text.length; i++ )
          wbuf[ i-1 ] = wbuf[ i ];
      wbuf[ i-1 ] = wc;
      buf = (char*) wbuf; /* so that it gets assigned and freed */

  } else { /* thus text.format == XawFmt8Bit */
      char c;
      buf = _XawTextGetText( ctx, start, end );
      text.length = strlen( buf );
      c = buf[ 0 ];
      for ( i = 1; i < text.length; i++ )
          buf[ i-1 ] = buf[ i ];
      buf[ i-1 ] = c;
  }

  text.ptr = buf;

  /* Store new text in source. */

  if (_XawTextReplace (ctx, start, end, &text))	/* Unable to edit, complain. */
      XBell(XtDisplay(w), 0);
  XtFree((char *) buf);
  EndAction(ctx);
}


/* NoOp() - action
 * This action performs no action, and allows the user or
 * application programmer to unbind a translation.
 *
 * Note: If the parameter list contains the string "RingBell" then
 *       this action will ring the bell.
 */

static void
NoOp(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    if (*num_params != 1)
	return;

    switch(params[0][0]) {
    case 'R':
    case 'r':
	XBell(XtDisplay(w), 0);
    default:			/* Fall Through */
	break;
    }
}

/* Reconnect() - action
 * This reconnects to the input method.  The user will typically call
 * this action if/when connection has been severed, or when the app
 * was started up before an IM was started up.
 */

static void
Reconnect(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    _XawImReconnect( w );
}


XtActionsRec _XawTextActionsTable[] = {

/* motion bindings */

  {"forward-character", 	MoveForwardChar},
  {"backward-character", 	MoveBackwardChar},
  {"forward-word", 		MoveForwardWord},
  {"backward-word", 		MoveBackwardWord},
  {"forward-paragraph", 	MoveForwardParagraph},
  {"backward-paragraph", 	MoveBackwardParagraph},
  {"beginning-of-line", 	MoveToLineStart},
  {"end-of-line", 		MoveToLineEnd},
  {"next-line", 		MoveNextLine},
  {"previous-line", 		MovePreviousLine},
  {"next-page", 		MoveNextPage},
  {"previous-page", 		MovePreviousPage},
  {"beginning-of-file", 	MoveBeginningOfFile},
  {"end-of-file", 		MoveEndOfFile},
  {"scroll-one-line-up", 	ScrollOneLineUp},
  {"scroll-one-line-down", 	ScrollOneLineDown},

/* delete bindings */

  {"delete-next-character", 	DeleteForwardChar},
  {"delete-previous-character", DeleteBackwardChar},
  {"delete-previous-char-only", DeleteBackwardCharOnly},
  {"delete-next-word", 		DeleteForwardWord},
  {"delete-previous-word", 	DeleteBackwardWord},
  {"delete-selection", 		DeleteCurrentSelection},

/* kill bindings */

  {"kill-word", 		KillForwardWord},
  {"backward-kill-word", 	KillBackwardWord},
  {"kill-selection", 		KillCurrentSelection},
  {"kill-selection-or-char", 	KillCurrentSelectionOrBackwardChar},
  {"kill-to-end-of-line", 	KillToEndOfLine},
  {"kill-to-end-of-paragraph", 	KillToEndOfParagraph},

/* new line stuff */

  {"newline-and-indent", 	InsertNewLineAndIndent},
  {"newline-and-backup", 	InsertNewLineAndBackup},
  {"newline", 			InsertNewLine},

/* Selection stuff */

  {"select-word", 		SelectWord},
  {"select-all", 		SelectAll},
  {"select-start", 		SelectStart},
  {"select-adjust", 		SelectAdjust},
  {"select-end", 		SelectEnd},
  {"select-save",		SelectSave},
  {"extend-start", 		ExtendStart},
  {"extend-adjust", 		ExtendAdjust},
  {"extend-end", 		ExtendEnd},
  {"insert-selection",		InsertSelection},

/* Miscellaneous */

  {"redraw-display", 		RedrawDisplay},
  {"insert-file", 		_XawTextInsertFile},
  {"search",		        _XawTextSearch},
  {"insert-char", 		InsertChar},
  {"insert-string",		InsertString},
  {"focus-in", 	 	        TextFocusIn},
  {"focus-out", 		TextFocusOut},
  {"enter-window", 	 	TextEnterWindow},
  {"leave-window", 		TextLeaveWindow},
  {"display-caret",		DisplayCaret},
  {"multiply",		        Multiply},
  {"form-paragraph",            FormParagraph},
  {"transpose-characters",      TransposeCharacters},
  {"no-op",                     NoOp},

/* Action to bind special translations for text Dialogs. */

  {"InsertFileAction",          _XawTextInsertFileAction},
  {"DoSearchAction",            _XawTextDoSearchAction},
  {"DoReplaceAction",           _XawTextDoReplaceAction},
  {"SetField",                  _XawTextSetField},
  {"PopdownSearchAction",       _XawTextPopdownSearchAction},

/* Reconnect to Input Method */
  {"reconnect-im",       Reconnect} /* Li Yuhong, Omron KK, 1991 */
};

Cardinal _XawTextActionsTableCount = XtNumber(_XawTextActionsTable);
