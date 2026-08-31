/*
 * Copyright 1991 by OMRON Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name OMRON not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  OMRON makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * OMRON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OMRON BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 *      Authors: Chris Peterson MIT X Consortium
 *               Li Yuhong      OMRON Corporation
 *               Frank Sheeran  OMRON Corporation
 *
 * Much code taken from X11R3 String and Disk Sources.
 */

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

/*
  Copyright © 2026 David Flater
  X11 license (as per the historical licenses that the package inherits)
*/

/*
 * MultiSrc.c - MultiSrc object. (For use with the text widget).
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xfuncs.h>
#include <X11/Xmu/CharSet.h>
#include <X11/Xmu/Misc.h>
#include <X11/Xos.h>
#include "XawI18n.h"
#include <X11/Xaw3dXft/AnyStringP.h>
#include <X11/Xaw3dXft/MultiSrcP.h>
#include <X11/Xaw3dXft/Xaw3dP.h>
#include <X11/Xaw3dXft/XawImP.h>
#include <X11/Xaw3dXft/XawInit.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>

#ifdef O_CLOEXEC
#define FOPEN_CLOEXEC "e"
#else
#define FOPEN_CLOEXEC ""
#define O_CLOEXEC 0
#endif

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

/* Private Data */

static XawTextPosition Scan(Widget, XawTextPosition, XawTextScanType,
                            XawTextScanDirection, int, Boolean);
static XawTextPosition Search(Widget, XawTextPosition, XawTextScanDirection,
                              XawTextBlock *);
static XawTextPosition ReadText(Widget, XawTextPosition, XawTextBlock *, int);
static int ReplaceText(Widget, XawTextPosition, XawTextPosition, XawTextBlock *);
static MultiPiece * FindPiece(MultiSrcObject, XawTextPosition, XawTextPosition *);
static MultiPiece * AllocNewPiece(MultiSrcObject, MultiPiece *);
static FILE * InitStringOrFile(MultiSrcObject);
static void FreeAllPieces(MultiSrcObject);
static void RemovePiece(MultiSrcObject, MultiPiece *);
static void BreakPiece(MultiSrcObject, MultiPiece *);
static void LoadPieces(MultiSrcObject, FILE *, char *);
static void RemoveOldStringOrFile(MultiSrcObject, Boolean);
static void ClassInitialize(void);
static void Initialize(Widget, Widget, ArgList, Cardinal *);
static void Destroy(Widget);
static void GetValuesHook(Widget, ArgList, Cardinal *);
static void *StorePiecesInString(MultiSrcObject);
static Boolean SetValues(Widget, Widget, Widget, ArgList, Cardinal *);
static Boolean WriteToFile(XawTextEncoding, void*, String);

#define MyWStrncpy( t,s,wcnt ) (void) memmove( (t), (s), (wcnt)*sizeof(wchar_t))

#ifndef MyWStrncpy
static void (MyWStrncpy)();
#endif

extern wchar_t* _XawTextMBToWC(Display *, char *, int *);
extern char *_XawTextWCToMB(Display *, wchar_t *, int *);

#define superclass		(&textSrcClassRec)
MultiSrcClassRec multiSrcClassRec = {
  { /* object_class fields */
    /* superclass	  	*/	(WidgetClass) superclass,
    /* class_name	  	*/	"MultiSrc",
    /* widget_size	  	*/	sizeof(MultiSrcRec),
    /* class_initialize   	*/	ClassInitialize,
    /* class_part_initialize	*/	NULL,
    /* class_inited       	*/	FALSE,
    /* initialize	  	*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* pad		  	*/	NULL,
    /* pad		  	*/	NULL,
    /* pad		  	*/	0,
    /* resources	  	*/	NULL,
    /* num_resources	  	*/	0,
    /* xrm_class	  	*/	NULLQUARK,
    /* pad		  	*/	FALSE,
    /* pad		  	*/	FALSE,
    /* pad			*/	FALSE,
    /* pad		  	*/	FALSE,
    /* destroy		  	*/	Destroy,
    /* pad		  	*/	NULL,
    /* pad		  	*/	NULL,
    /* set_values	  	*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* pad			*/	NULL,
    /* get_values_hook		*/	GetValuesHook,
    /* pad		 	*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private   	*/	NULL,
    /* pad		   	*/	NULL,
    /* pad			*/	NULL,
    /* pad			*/	NULL,
    /* extension		*/	NULL
  },
  { /* textSrc_class fields */
    /* Read                     */      ReadText,
    /* Replace                  */      ReplaceText,
    /* Scan                     */      Scan,
    /* Search                   */      Search,
    /* SetSelection             */      XtInheritSetSelection,
    /* ConvertSelection         */      XtInheritConvertSelection
  },
  { /* multiSrc_class fields */
    /* Keep the compiler happy */       '\0'
  }
};

WidgetClass multiSrcObjectClass = (WidgetClass)&multiSrcClassRec;

/************************************************************
 *
 * Semi-Public Interfaces.
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
  MultiSrcObject src = (MultiSrcObject) new;
  FILE *file;

  /*
   * Set correct flags (override resources) depending upon widget class.
   */

  src->text_src.text_format = XawFmtWide;
  src->multi_src.changes = FALSE;
  src->multi_src.allocated_string = FALSE;

  file = InitStringOrFile(src);
  LoadPieces(src, file, NULL);
}

/*	Function Name: ReadText
 *	Description: This function reads the source.
 *	Arguments: w - the MultiSource widget.
 *                 pos - position of the text to retrieve.
 * RETURNED        text - text block that will contain returned text.
 *                length - maximum number of characters to read.
 *	Returns: text position immediately after the characters read from the
 *               text buffer
 */

static XawTextPosition
ReadText(Widget w, XawTextPosition pos, XawTextBlock *text, int length)
{
  MultiSrcObject src = (MultiSrcObject) w;
  XawTextPosition count, start = 0;
  MultiPiece * piece = FindPiece(src, pos, &start);

  text->format = XawFmtWide;
  text->firstPos = pos;
  text->ptr = (char *)(piece->text + (pos - start));
  count = piece->used - (pos - start);
  text->length = (length > count) ? count : length;
  return(pos + text->length);
}

/*	Function Name: ReplaceText.
 *	Description: Replaces a block of text with new text.
 *	Arguments: w - the MultiSource widget.
 *                 startPos, endPos - ends of text that will be removed.
 *                 text - new text to be inserted into buffer at startPos.
 *	Returns: XawEditError or XawEditDone.
 */

static int
ReplaceText(Widget w, XawTextPosition startPos, XawTextPosition endPos, XawTextBlock *u_text_p)
{
  MultiSrcObject src = (MultiSrcObject) w;
  MultiPiece *start_piece, *end_piece, *temp_piece;
  XawTextPosition start_first = 0, end_first;
  int length, firstPos;
  wchar_t *wptr;
  Boolean local_artificial_block = False;
  XawTextBlock text;

  /* STEP 1: The user handed me a text block called `u_text' that may be
   * in either XawFmtWide or XawFmt8Bit (ie MB.)  Later code needs the block
   * `text' to hold XawFmtWide.  So, this copies `u_text' to `text', and if
   * `u_text' was MB, I knock it up to wide. */

  if ( u_text_p->length == 0 )	/* if so, the block contents never ref'd. */
      text.length = 0;

  else if ( u_text_p->format == XawFmtWide) {
      local_artificial_block = False;		/* ie, don't have to free it ourselves*/
      text.firstPos = u_text_p->firstPos;
      text.length =   u_text_p->length;
      text.ptr =      u_text_p->ptr;
      /* text.format is unneeded */

  } else {
      /* WARNING! u_text->firstPos and length are in units of CHAR, not CHARACTERS! */

      local_artificial_block = True;	/* ie, have to free it ourselves */
      text.firstPos = 0;
      text.length = u_text_p->length; /* _XawTextMBToWC converts this to wchar len. */

      text.ptr = (char*)_XawTextMBToWC( XtDisplay(XtParent(w)),
			 &(u_text_p->ptr[u_text_p->firstPos]), &(text.length) );

      /* I assert the following assignment is not needed - since Step 4
      depends on length, it has no need of a terminating NULL.  I think
      the ASCII-version has the same needless NULL. */
      /*((wchar_t*)text.ptr)[ text.length ] = NULL;*/
  }


  /* STEP 2: some initialization... */

  if (src->text_src.edit_mode == XawtextRead)
    return(XawEditError);

  start_piece = FindPiece(src, startPos, &start_first);
  end_piece = FindPiece(src, endPos, &end_first);


  /* STEP 3: remove the empty pieces... */

  if (start_piece != end_piece) {
    temp_piece = start_piece->next;

  /* If empty and not the only piece then remove it. */

    if ( ((start_piece->used = startPos - start_first) == 0) &&
	 !((start_piece->next == NULL) && (start_piece->prev == NULL)) )
      RemovePiece(src, start_piece);

    while (temp_piece != end_piece) {
      temp_piece = temp_piece->next;
      RemovePiece(src, temp_piece->prev);
    }
    end_piece->used -= endPos - end_first;
    if (end_piece->used != 0)
      MyWStrncpy(end_piece->text, (end_piece->text + endPos - end_first),
		(int) end_piece->used);
  }
  else {			/* We are fully in one piece. */
    if ( (start_piece->used -= endPos - startPos) == 0) {
      if ( !((start_piece->next == NULL) && (start_piece->prev == NULL)) )
	RemovePiece(src, start_piece);
    }
    else {
      MyWStrncpy(start_piece->text + (startPos - start_first),
		start_piece->text + (endPos - start_first),
		(int) (start_piece->used - (startPos - start_first)) );
    }
  }

  src->multi_src.length += text.length -(endPos - startPos);
  /*((TextWidget)src->object.parent)->text.lastPos = src->multi_src.length;*/



  /* STEP 4: insert the new stuff */

  if ( text.length != 0) {

    start_piece = FindPiece(src, startPos, &start_first);

    length = text.length;
    firstPos = text.firstPos;

    while (length > 0) {
      wchar_t* ptr;
      int fill;

      if (start_piece->used == src->text_src.piece_size) {
	BreakPiece(src, start_piece);
	start_piece = FindPiece(src, startPos, &start_first);
      }

      fill = Min((int)(src->text_src.piece_size - start_piece->used), length);

      ptr = start_piece->text + (startPos - start_first);
      MyWStrncpy(ptr + fill, ptr,
		(int) start_piece->used - (startPos - start_first));
      wptr =(wchar_t *)text.ptr;
      (void)wcsncpy(ptr, wptr + firstPos, fill);

      startPos += fill;
      firstPos += fill;
      start_piece->used += fill;
      length -= fill;
    }
  }

  if ( local_artificial_block == True )

      /* In other words, text is not the u_text that the user handed me but
      one I made myself.  I only care, because I need to free the string. */

      XFree( text.ptr );

  src->multi_src.changes = TRUE;

  XtCallCallbacks(w, XtNcallback, NULL);

  return(XawEditDone);
}

/*	Function Name: Scan
 *	Description: Scans the text source for the number and type
 *                   of item specified.
 *	Arguments: w - the MultiSource widget.
 *                 position - the position to start scanning.
 *                 type - type of thing to scan for.
 *                 dir - direction to scan.
 *                 count - which occurrence of this thing to search for.
 *                 include - whether or not to include the character found in
 *                           the position that is returned.
 *	Returns: the position of the item found.
 *
 * Note: While there are only 'n' characters in the file there are n+1
 *       possible cursor positions (one before the first character and
 *       one after the last character.
 */

static
XawTextPosition
Scan(Widget w, XawTextPosition position, XawTextScanType type,
     XawTextScanDirection dir, int count, Boolean include)
{
  MultiSrcObject src = (MultiSrcObject) w;
  int inc;
  MultiPiece * piece;
  XawTextPosition first = 0, first_eol_position = 0;
  wchar_t * ptr;

  if (type == XawstAll) {	/* Optimize this common case. */
    if (dir == XawsdRight)
      return(src->multi_src.length);
    return(0);			/* else. */
  }


  /* STEP 1: basic sanity checks */

  if (position > src->multi_src.length)
    position = src->multi_src.length;


  if ( dir == XawsdRight ) {
    if (position == src->multi_src.length)
      return(src->multi_src.length);
    inc = 1;
  }
  else {
    if (position == 0)
      return(0);
    inc = -1;
    position--;
  }

  piece = FindPiece(src, position, &first);

  if ( piece->used == 0 ) return(0); /* i.e., buffer is empty. */

  ptr = (position - first) + piece->text;

  switch (type) {
  case XawstEOL:
  case XawstParagraph:
  case XawstWhiteSpace:
    for ( ; count > 0 ; count-- ) {
      Boolean non_space = FALSE, first_eol = TRUE;
      /* CONSTCOND */
      while (TRUE) {
        wchar_t c = *ptr;

	ptr += inc;
	position += inc;

	if (type == XawstWhiteSpace) {
	  if (iswspace(c)) {
	    if (non_space)
	      break;
	  }
	  else
	    non_space = TRUE;
	}
	else if (type == XawstEOL) {
          if (c == L'\n') break;
	}
	else { /* XawstParagraph */
	  if (first_eol) {
            if (c == L'\n') {
	      first_eol_position = position;
	      first_eol = FALSE;
	    }
	  }
	  else
            if ( c == L'\n')
              break;
            else if ( !iswspace(c) )
	      first_eol = TRUE;
	}


	if ( ptr < piece->text ) {
	  piece = piece->prev;
	  if (piece == NULL)	/* Beginning of text. */
	    return(0);
	  ptr = piece->text + piece->used - 1;
	}
	else if ( ptr >= (piece->text + piece->used) ) {
	  piece = piece->next;
	  if (piece == NULL)	/* End of text. */
	    return(src->multi_src.length);
	  ptr = piece->text;
	}
      }
    }
    if (!include) {
      if ( type == XawstParagraph)
	position = first_eol_position;
      position -= inc;
    }
    break;
  case XawstPositions:
    position += count * inc;
    break;
  case XawstAll:		/* handled in special code above */
  default:
    break;
  }

  if ( dir == XawsdLeft )
    position++;

  if (position >= src->multi_src.length)
    return(src->multi_src.length);
  if (position < 0)
    return(0);

  return(position);
}

/*	Function Name: Search
 *	Description: Searches the text source for the text block passed
 *	Arguments: w - the MultiSource Widget.
 *                 position - the position to start scanning.
 *                 dir - direction to scan.
 *                 text - the text block to search for.
 *	Returns: the position of the item found.
 */

static XawTextPosition
Search(Widget w, XawTextPosition position, XawTextScanDirection dir, XawTextBlock *text)
{
  MultiSrcObject src = (MultiSrcObject) w;
  int inc, count = 0;
  wchar_t * ptr;
  wchar_t* wtarget;
  int wtarget_len;
  Display * d = XtDisplay(XtParent(w));
  MultiPiece * piece;
  wchar_t* buf;
  XawTextPosition first = 0;

  /* STEP 1: First, a brief sanity check. */

  if ( dir == XawsdRight )
    inc = 1;
  else {
    inc = -1;
    if (position == 0)
      return(XawTextSearchError);	/* scanning left from 0??? */
    position--;
  }


  /* STEP 2: Ensure I have a local wide string.. */

  /* Since this widget stores 32bit chars, I check here to see if
  I'm being passed a string claiming to be 8bit chars (ie, MB text.)
  If that is the case, naturally I convert to 32bit format. */

  /*if the block was XawFmt8Bit, length will convert to REAL wchar count
    below */
  wtarget_len = text->length;

  if ( text->format == XawFmtWide )
      wtarget = &( ((wchar_t*)text->ptr) [text->firstPos] );
  else
  {
      /* The following converts wtarget_len from byte len to wchar count */
      wtarget = _XawTextMBToWC( d, &text->ptr[ text->firstPos ], &wtarget_len );
  }

  /* OK, I can now assert that wtarget holds wide characters, wtarget_len
  holds an accurate count of those characters, and that firstPos has been
  effectively factored out of the following computations. */


  /* STEP 3: SEARCH! */

  buf = (wchar_t *)XtMalloc((unsigned)sizeof(wchar_t) * wtarget_len );
  (void)wcsncpy(buf, wtarget, wtarget_len );
  piece = FindPiece(src, position, &first);
  ptr = (position - first) + piece->text;

  /* CONSTCOND */
  while (TRUE) {
    if (*ptr == ((dir == XawsdRight) ? *(buf + count)
		                     : *(buf + wtarget_len - count - 1)) ) {
      if (count == (text->length - 1))
	break;
      else
	count++;
    }
    else {
      if (count != 0) {
	position -=inc * count;
	ptr -= inc * count;
      }
      count = 0;
    }

    ptr += inc;
    position += inc;

    while ( ptr < piece->text ) {
      piece = piece->prev;
      if (piece == NULL) {	/* Beginning of text. */
	XtFree((char *)buf);
	return(XawTextSearchError);
      }
      ptr = piece->text + piece->used - 1;
    }

    while ( ptr >= (piece->text + piece->used) ) {
      piece = piece->next;
      if (piece == NULL) {	/* End of text. */
	XtFree((char *)buf);
	return(XawTextSearchError);
      }
      ptr = piece->text;
    }
  }

  XtFree( (char *) buf );
  if (dir == XawsdLeft)
    return( position );
  return( position - ( wtarget_len - 1 ) );
}

/*	Function Name: SetValues
 *	Description: Sets the values for the MultiSource.
 *	Arguments: current - current state of the widget.
 *                 request - what was requested.
 *                 new - what the widget will become.
 *	Returns: True if redisplay is needed.
 */

static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
  MultiSrcObject src =      (MultiSrcObject) new;
  MultiSrcObject old_src = (MultiSrcObject) current;
  XtAppContext app_con = XtWidgetToApplicationContext(new);
  Boolean total_reset = FALSE, string_set = FALSE;
  FILE * file;

  if ( old_src->text_src.use_string_in_place !=
       src->text_src.use_string_in_place ) {
      XtAppWarning( app_con,
	"libXaw3dXft MultiSrc: the useStringInPlace resource may not be changed.");
       src->text_src.use_string_in_place =
	   old_src->text_src.use_string_in_place;
  }

  for (Cardinal i = 0; i < *num_args ; i++ )
      if (streq(args[i].name, XtNstring)) {
	  string_set = TRUE;
	  break;
      }

  if (string_set || (old_src->text_src.type != src->text_src.type)) {
    RemoveOldStringOrFile(old_src, string_set);
    src->multi_src.allocated_string = old_src->multi_src.allocated_string;
    file = InitStringOrFile(src);
    LoadPieces(src, file, NULL);
    XawTextSetSource(XtParent(new), new, 0);   /* Tell text widget
						  what happened. */
    total_reset = True;
  }

  if ( old_src->text_src.string_length != src->text_src.string_length )
      src->text_src.piece_size = src->text_src.string_length;

  if ( !total_reset && (old_src->text_src.piece_size
      != src->text_src.piece_size) ) {
      String mb_string = StorePiecesInString( old_src );

      if ( mb_string != 0 ) {
          FreeAllPieces( old_src );
          LoadPieces( src, NULL, mb_string );
          XtFree( mb_string );
      } else {
          /* If the buffer holds bad chars, don't touch it... */
          XtAppWarningMsg( app_con,
		"convertError", "multiSource", "XawError",
                 XtName( XtParent( (Widget) old_src ) ), NULL, NULL );
          XtAppWarningMsg( app_con,
		"convertError", "multiSource", "XawError",
                 "Non-character code(s) in buffer.", NULL, NULL );
      }
  }

  return(FALSE);
}

/*	Function Name: GetValuesHook
 *	Description: This is a get values hook routine that sets the
 *                   values specific to the multi source.
 *	Arguments: w - the MultiSource Widget.
 *                 args - the argument list.
 *                 num_args - the number of args.
 *	Returns: none.
 */

static void
GetValuesHook(Widget w, ArgList args, Cardinal *num_args)
{
  MultiSrcObject src = (MultiSrcObject) w;
  if (src->text_src.type == XawAsciiString) {
    for (Cardinal i = 0; i < *num_args ; i++ )
      if (streq(args[i].name, XtNstring)) {
	if (_XawMultiSave(w))	/* If save successful. */
	  *((char **) args[i].value) = src->text_src.string;
	break;
      }
  }
}

/*	Function Name: Destroy
 *	Description: Destroys an multi source (frees all data)
 *	Arguments: src - the Multi source Widget to free.
 *	Returns: none.
 */

static void
Destroy(Widget w)
{
    RemoveOldStringOrFile((MultiSrcObject) w, True);
}

/************************************************************
 *
 * Public routines
 *
 ************************************************************/

/*	Function Name: XawMultiSourceFreeString
 *	Description: Frees the string returned by a get values call
 *                   on the string when the source is of type string.
 *	Arguments: w - the MultiSrc widget.
 *	Returns: none.
 *
 * The public interface is XawAsciiSourceFreeString!
 */

void
_XawMultiSourceFreeString(
    Widget w)
{
  MultiSrcObject src = (MultiSrcObject) w;

/*if (src->multi_src.allocated_string&& src->text_src.type != XawAsciiFile) {*/
  /* ASSERT: src->multi_src.allocated_string -> we MUST free .string! */
  if ( src->multi_src.allocated_string ) {
    XtFree(src->text_src.string);
    src->multi_src.allocated_string = FALSE;
    src->text_src.string = NULL;
  }
}

/*	Function Name: _XawMultiSave
 *	Description: Saves all the pieces into a file or string as required.
 *	Arguments: w - the multiSrc Widget.
 *	Returns: TRUE if the save was successful.
 *
 * The public interface is XawAsciiSave(w)!
 */

Boolean _XawMultiSave (Widget w) {
  MultiSrcObject src = (MultiSrcObject)w;
  void *mb_string;
  if (src->text_src.type == XawAsciiFile) {
    if (!src->multi_src.changes) /* No changes to save. */
      return True;
    mb_string = StorePiecesInString(src);
    assert(mb_string);
    if (!WriteToFile(src->text_src.encoding, mb_string, src->text_src.string)) {
      XtFree(mb_string);
      return False;
    }
    XtFree(mb_string);
  } else {
    // This is used in GetValuesHook.
    mb_string = StorePiecesInString(src);
    assert(mb_string);
    if (src->multi_src.allocated_string)
      XtFree(src->text_src.string);
    else
      src->multi_src.allocated_string = True;
    src->text_src.string = mb_string;
  }
  src->multi_src.changes = False;
  return True;
}

/*	Function Name: XawMultiSaveAsFile
 *	Description: Save the current buffer as a file.
 *	Arguments: w - the MultiSrc widget.
 *                 name - name of the file to save this file into.
 *	Returns: True if the save was successful.
 *
 * The public interface is XawAsciiSaveAsFile!
 */

Boolean
_XawMultiSaveAsFile(
    Widget w,
    _Xconst char* name)
{
  MultiSrcObject src = (MultiSrcObject) w;
  String mb_string = StorePiecesInString( src );
  assert(mb_string);
  const XawTextEncoding encoding = src->text_src.encoding;
  Boolean ret = WriteToFile(encoding, mb_string, (char *)name);
  XtFree(mb_string);
  return ret;
}

/************************************************************
 *
 * Private Functions.
 *
 ************************************************************/

static void RemoveOldStringOrFile (MultiSrcObject src, Boolean checkString) {
  FreeAllPieces(src);
  if (checkString && src->multi_src.allocated_string) {
    XtFree(src->text_src.string);
    src->multi_src.allocated_string = False;
    src->text_src.string = NULL;
  }
}

/*	Function Name: WriteToFile
 *	Description: Write the string specified to the beginning of the file
 *                   specified.
 *	Arguments: string - string to write.
 *                 name - the name of the file
 *	Returns: returns TRUE if successful, FALSE otherwise.
 */

static Boolean WriteToFile (XawTextEncoding encoding, void *string, String name)
{
  int fd;
  Bool result = True;
  const Cardinal num_bytes = Xaw3dXftAnyStrlen(encoding, string);
  if ((fd = open(name, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666)) == -1) {
    perror(name);
    XtWarning("libXaw3dXft: error on file open");
    return False;
  }
  if (write(fd, string, num_bytes) == -1) {
    perror(name);
    XtWarning("libXaw3dXft: error writing to file");
    result = False;
  }
  if (close(fd) == -1) {
    perror(name);
    XtWarning("libXaw3dXft: error on file close");
    result = False;
  }
  return result;
}


/*	Function Name: StorePiecesInString
 *	Description:   store the pieces in memory into a string in the
 *                     external encoding.
 *	Arguments:     src - the multiSrc to gather data from
 *	Returns:       char *mb_string.     Caller must free.
 */

static void *StorePiecesInString (MultiSrcObject src) {
  if (src->multi_src.length > Xaw3dXftAnyStringLengthLimit)
    XtError("libXaw3dXft: character string too long in Text widget");
  const Cardinal num_chars = src->multi_src.length;

  // Assemble the complete wide string from the piece table.
  XawTextPosition first;
  MultiPiece *piece;
  wchar_t *wc_string = (wchar_t*)XtMalloc((num_chars + 1) * sizeof(wchar_t));
  for (first = 0, piece = src->multi_src.first_piece; piece != NULL;
       first += piece->used, piece = piece->next)
    (void) wcsncpy(wc_string + first, piece->text, piece->used);
  wc_string[num_chars] = 0;

  // Make the external string.
  Cardinal num_bytes = num_chars * sizeof(wchar_t);
  void *external_string = Xaw3dXftWcToAnyN(wc_string, &num_bytes,
    src->text_src.encoding);

  // Optionally rebuild the piece table.  The round trip to external encoding
  // might be lossy, but the loss is inevitable if that's how we're saving
  // it.
  if (src->text_src.data_compression) {
    FreeAllPieces(src);
    LoadPieces(src, NULL, external_string);
  }

  XtFree((char*)wc_string);
  return external_string;
}


/*	Function Name: InitStringOrFile.
 *	Description: Initializes the string or file before LoadPieces.
 *	Arguments: src - the MultiSource.
 *	Returns: none - May exit though.
 */

static FILE *InitStringOrFile (MultiSrcObject src) {
  char *open_mode = NULL;

  assert(!src->text_src.use_string_in_place);
  if (src->text_src.type == XawAsciiString) return NULL;

  src->multi_src.is_tempfile = False;

  switch (src->text_src.edit_mode) {
  case XawtextRead:
    if (src->text_src.string == NULL)
      XtErrorMsg("NoFile", "multiSourceCreate", "XawError",
	"Creating a read only disk widget and no file specified.",
	NULL, 0);
    open_mode = "r" FOPEN_CLOEXEC;
    break;
  case XawtextAppend:
  case XawtextEdit:
    if (src->text_src.string == NULL) {
      // Put a temp file name in text_src.string for XawAsciiSave
      if (src->multi_src.allocated_string)
	XtFree(src->text_src.string);
      char fileName[TMPSIZ];
      (void) tmpnam(fileName);
      src->text_src.string = strdup(fileName);
      src->multi_src.allocated_string = True;
      src->multi_src.is_tempfile = True;
      {
	char buf[sizeof fileName + 80];
	sprintf(buf,
	  "libXaw3dXft: Text widget will save to temp file %s",
	  fileName);
	XtWarning(buf);
      }
      // File will not be opened here
      // open_mode = "w" FOPEN_CLOEXEC;
    } else
      open_mode = "r+" FOPEN_CLOEXEC;
    break;
  default:
    XtErrorMsg("badMode", "multiSourceCreate", "XawError",
      "Bad editMode for multi source; must be Read, Append or Edit.",
      NULL, NULL);
  }

  if (!src->multi_src.is_tempfile) {
    FILE *file;
    if ((file = fopen(src->text_src.string, open_mode)) != 0)
      return file;
    else {
      String params[2];
      Cardinal num_params = 2;
      params[0] = src->text_src.string;
      params[1] = strerror(errno);
      XtAppWarningMsg(XtWidgetToApplicationContext((Widget)src),
	"openError", "multiSourceCreate", "XawWarning",
	"Cannot open file %s; %s", params, &num_params);
    }
  }
  return NULL;
}


/*
  LoadPieces

  Input:  One of three sources, all in external encoding
  1. text_src.string (initial input when text_src.type == XawAsciiString)
  2. The file parameter (initial input when text_src.type == XawAsciiFile)
  3. The string parameter (when recycling the piece table)

  multi_src.length is ignored on input.  string or text_src.string must be
  NUL-terminated.

  Result:
  Piece table is filled up with the input converted to wc.
  multi_src.length is the number of wide characters.
  If file was provided, it is closed.
*/
static void LoadPieces (MultiSrcObject src, FILE *file, char *string) {
  wchar_t *local_str = NULL;
  Cardinal local_num_wchars = 0;
  const XawTextEncoding encoding = src->text_src.encoding;

  // Following InitStringOrFile, string parameter is always null.
  // When recycling the piece table, file parameter is always null.
  // Sometimes they're both null, but they can't both be non-null.
  assert(!file || !string);

  if (!string && !file) {
    // Either our input is in text_src.string or there is none.
    // If we're in "file mode," text_src.string is a file name.
    if (src->text_src.type == XawAsciiString && src->text_src.string) {
      Cardinal num_bytes = Xaw3dXftAnyStrlen(encoding, src->text_src.string);
      local_str = Xaw3dXftAnyToWcN(encoding, src->text_src.string, &num_bytes);
      assert(num_bytes % sizeof(wchar_t) == 0);
      local_num_wchars = num_bytes / sizeof(wchar_t);
    }
  } else if (string) {
    // Our input is in parameter string, regardless of mode.  File is null.
    Cardinal num_bytes = Xaw3dXftAnyStrlen(encoding, string);
    local_str = Xaw3dXftAnyToWcN(encoding, string, &num_bytes);
    assert(num_bytes % sizeof(wchar_t) == 0);
    local_num_wchars = num_bytes / sizeof(wchar_t);
  } else {
    // Our input is in the file.
    assert(src->text_src.type == XawAsciiFile);
    if (fseek(file, 0, SEEK_END) == -1) {
      perror("fseek");
      XtError("libXaw3dXft: fseek failed in Text widget");
    }
    const long biglen = ftell(file);
    if (biglen > Xaw3dXftAnyStringLengthLimit)
      XtError("libXaw3dXft: file too long in Text widget");
    Cardinal num_bytes = biglen;
    rewind(file);
    void *slurp = XtMalloc(num_bytes+4);
    size_t ret = fread(slurp, 1, num_bytes, file);
    if (ret < num_bytes) {
      XtWarning("libXaw3dXft: short file read in Text widget");
      num_bytes = ret;
    }
    if (fclose(file)) {
      perror("fclose");
      XtWarning("libXaw3dXft: fclose failed in Text widget");
    }
    (void) memset(slurp+num_bytes, 0, 4);
    // A short read above could lead to a Rule 2 assert fail in
    // Xaw3dXftAnyToWcN here:
    local_str = Xaw3dXftAnyToWcN(encoding, slurp, &num_bytes);
    assert(num_bytes % sizeof(wchar_t) == 0);
    local_num_wchars = num_bytes / sizeof(wchar_t);
    free(slurp);
  }

  // Build the piece table from the wide string.
  wchar_t *ptr  = local_str;        // Maybe null
  uint64_t left = local_num_wchars; // Maybe 0
  MultiPiece *piece = NULL;
  do {
    piece = AllocNewPiece(src, piece);
    piece->text = (wchar_t*)XtMalloc(src->text_src.piece_size * sizeof(wchar_t));
    piece->used = Min(left, src->text_src.piece_size);
    if (piece->used != 0)
      (void) wcsncpy(piece->text, ptr, piece->used); // possibly unterminated
    left -= piece->used;
    ptr  += piece->used;
  } while (left > 0);

  src->multi_src.length = local_num_wchars;
  if (local_str)
    free(local_str);
}


/*	Function Name: AllocNewPiece
 *	Description: Allocates a new piece of memory.
 *	Arguments: src - The MultiSrc Widget.
 *                 prev - the piece just before this one, or NULL.
 *	Returns: the allocated piece.
 */

static MultiPiece *
AllocNewPiece(MultiSrcObject src, MultiPiece *prev)
{
  MultiPiece * piece = XtNew(MultiPiece);

  if (prev == NULL) {
    src->multi_src.first_piece = piece;
    piece->next = NULL;
  }
  else {
    if (prev->next != NULL)
      (prev->next)->prev = piece;
    piece->next = prev->next;
    prev->next = piece;
  }

  piece->prev = prev;

  return(piece);
}

/*	Function Name: FreeAllPieces
 *	Description: Frees all the pieces
 *	Arguments: src - The MultiSrc Widget.
 *	Returns: none.
 */

static void
FreeAllPieces(MultiSrcObject src)
{
  MultiPiece * next, * first = src->multi_src.first_piece;

  if (first->prev != NULL)
    printf("Xaw MultiSrc Object: possible memory leak in FreeAllPieces().\n");

  for ( ; first != NULL ; first = next ) {
    next = first->next;
    RemovePiece(src, first);
  }
}

/*	Function Name: RemovePiece
 *	Description: Removes a piece from the list.
 *	Arguments:
 *                 piece - the piece to remove.
 *	Returns: none.
 */

static void
RemovePiece(MultiSrcObject src, MultiPiece *piece)
{
  if (piece->prev == NULL)
    src->multi_src.first_piece = piece->next;
  else
    (piece->prev)->next = piece->next;

  if (piece->next != NULL)
    (piece->next)->prev = piece->prev;

  XtFree((char *)piece->text);
  XtFree((char *)piece);
}

/*	Function Name: FindPiece
 *	Description: Finds the piece containing the position indicated.
 *	Arguments: src - The MultiSrc Widget.
 *                 position - the position that we are searching for.
 * RETURNED        first - the position of the first character in this piece.
 *	Returns: piece - the piece that contains this position.
 */

static MultiPiece *
FindPiece(MultiSrcObject src, XawTextPosition position, XawTextPosition *first)
{
  MultiPiece * old_piece = NULL, * piece = src->multi_src.first_piece;
  XawTextPosition temp;

  for ( temp = 0 ; piece != NULL ; temp += piece->used, piece = piece->next ) {
    *first = temp;
    old_piece = piece;

    if ((temp + piece->used) > position)
      return(piece);
  }
  return(old_piece);	  /* if we run off the end the return the last piece */
}

/*	Function Name: BreakPiece
 *	Description: Breaks a full piece into two new pieces.
 *	Arguments: src - The MultiSrc Widget.
 *                 piece - the piece to break.
 *	Returns: none.
 */

#define HALF_PIECE (src->text_src.piece_size/2)

static void
BreakPiece(MultiSrcObject src, MultiPiece *piece)
{
  MultiPiece * new = AllocNewPiece(src, piece);

  new->text = (wchar_t*)XtMalloc(src->text_src.piece_size * sizeof(wchar_t));
  (void) wcsncpy(new->text, piece->text + HALF_PIECE,
          src->text_src.piece_size - HALF_PIECE);
  piece->used = HALF_PIECE;
  new->used = src->text_src.piece_size - HALF_PIECE;
}
