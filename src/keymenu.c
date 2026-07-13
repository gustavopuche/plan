/*
 * Create and destroy the keyword search popup. It is installed when the
 * Keyword Search entry in the Seach pulldown in the main calendar window
 * is used.
 *
 *	destroy_keyword_popup()
 *	create_keyword_popup()
 */

#include <stdio.h>
#include <time.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/LabelP.h>
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/Protocols.h>
#include "cal.h"

static void mode_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void text_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void search_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void done_callback	(Widget, int, XmToggleButtonCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern Widget		toplevel;	/* top-level shell for icon name */
extern struct config	config;		/* global configuration data */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern Pixel		color[NCOLS];	/* colors: COL_* */

static BOOL		have_shell;	/* message popup exists if TRUE */
static Widget		shell;		/* popup menu shell */
static Widget		text;		/* search string */
static char		*search_string;	/* string to search for */
int			search_mode=1;	/* 0=case sensitive, 1=ins., 2=regex */


/*
 * destroy a popup. Remove it from the screen, and destroy its widgets.
 * It's too much trouble to keep them for next time.
 */

static void destroy_keyword_popup(void)
{
	if (have_shell) {
		if (search_string)
			XtFree(search_string);
		search_string = XmTextGetString(text);
		XtPopdown(shell);
		have_shell = FALSE;
	}
}


/*
 * create a keyword popup as a separate application shell. When the popup
 * already exists and was just popped down, pop it up and exit. This ensures
 * that it comes up with the same defaults.
 */

void create_keyword_popup(void)
{
	Widget			form, radio, w;
	Arg			args[20];
	int			n;
	Atom			closewindow;

	if (have_shell) {
		XtPopup(shell, XtGrabNone);
		return;
	}
	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtCreatePopupShell(_("Keyword Search"),
			applicationShellWidgetClass, toplevel, args, n);
#	ifdef EDITRES
	XtAddEventHandler(shell, (EventMask)0, TRUE, 
 			_XEditResCheckMessages, NULL);
#	endif
	set_icon(shell, 1);
	form = XtCreateManagedWidget("keyform", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"key");

							/*-- mode --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	w = XtCreateManagedWidget(_("Search mode:"), xmLabelWidgetClass,
			form, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNspacing,		4);			n++;
	radio = XmCreateRadioBox(form, "radio", args, n);

	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNindicatorType,	XmONE_OF_MANY);		n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		False);			n++;
	w = XtCreateManagedWidget(_("Literal case sensitive"),
			xmToggleButtonWidgetClass,
			radio, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			mode_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"key_literal");

	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNindicatorType,	XmONE_OF_MANY);		n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		True);			n++;
	w = XtCreateManagedWidget(_("Literal case insensitive"),
			xmToggleButtonWidgetClass,
			radio, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			mode_callback, (XtPointer)1);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"key_uncase");

#ifndef NOREGEX
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNindicatorType,	XmONE_OF_MANY);		n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		False);			n++;
	w = XtCreateManagedWidget(_("Regular expression"),
			xmToggleButtonWidgetClass,
			radio, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			mode_callback, (XtPointer)2);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"key_regular");
#endif

	XtManageChild(radio);
							/*-- search string --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		radio);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	w = XtCreateManagedWidget(_("Search for:"), xmLabelWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"key_string");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		5);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	16);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	text = XtCreateManagedWidget("", xmTextWidgetClass,
			form, args, n);
	XtAddCallback(text, XmNactivateCallback, (XtCallbackProc)
			text_callback, (XtPointer)NULL);
	XtAddCallback(text, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"key_string");

							/*-- buttons --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		text);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	16);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Search"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			search_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"key_search");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		text);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Cancel"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			done_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"key_cancel");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		text);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Help"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			help_callback, (XtPointer)"key");
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer)"key");

	XtPopup(shell, XtGrabNone);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow, (XtCallbackProc)
			done_callback, (XtPointer)shell);

	have_shell = TRUE;
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * All of these routines are direct X callbacks.
 */

/*ARGSUSED*/
static void mode_callback(
	UNUSED Widget			widget,
	int				item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	search_mode = item;
}


/*ARGSUSED*/
static void done_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	destroy_keyword_popup();
}


/*ARGSUSED*/
static void search_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	if (search_string)
		XtFree(search_string);
	search_string = XmTextGetString(text);
	if (*search_string)
		create_list_popup(mainlist, (time_t)0, (time_t)0,
			search_string, (struct entry *)0, 0, 0);
}


/*ARGSUSED*/
static void text_callback(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	if (search_string)
		XtFree(search_string);
	search_string = XmTextGetString(widget);
	if (*search_string)
		create_list_popup(mainlist, (time_t)0, (time_t)0,
			search_string, (struct entry *)0, 0, 0);
}
