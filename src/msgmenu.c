/*
 * Create and destroy a text popup. It is installed when the Message icon
 * (looks like a post-it note), the Script icon (a percent sign), or the
 * Meeting icon (four arrows) is pressed. The popup shows the appropriate
 * title, and string entered or removed by the user are stored back into
 * the proper string pointers (which point to members of the edit struct).
 *
 *	destroy_text_popup()
 *	create_text_popup(title, initial, x, readonly)
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/LabelP.h>
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/Protocols.h>
#include "cal.h"

static void done_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void delete_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void clear_callback	(Widget, int, XmToggleButtonCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern Widget		toplevel;	/* top-level shell for icon name */
extern struct edit	edit;		/* info about entry being edited */
extern struct plist	*mainlist;	/* list of all schedule entries */

static BOOL		have_shell;	/* message popup exists if TRUE */
static BOOL		readonly_msg;	/* just a displayer, not editable */
static char		**source;	/* ptr to string ptr in edit.entry */
static int		button_x;	/* which button was pressed, SC_* */
static Widget		shell;		/* popup menu shell */
static Widget		text;		/* text widget */


/*
 * destroy a popup. Remove it from the screen, and destroy its widgets.
 * It's too much trouble to keep them for next time.
 */

void destroy_text_popup(void)
{
	char		*string;	/* contents of text widget */

	if (have_shell && !readonly_msg && edit.editing) {
		if (*source)
			free(*source);
		string = XmTextGetString(text);
		*source = *string ? mystrdup(string) : 0;
		edit.changed = TRUE;
		edit.entry.suspended = FALSE;
		print_pixmap(edit.menu->entry[edit.y][button_x],
			button_x + PIC_MESSAGE - SC_MESSAGE, *string);
		print_button(edit.menu->entry[edit.y][SC_NOTE], "%s",
			mkbuttonstring(edit.menu, SC_NOTE, edit.y));
		XtFree(string);
	}
	if (have_shell) {
		XtPopdown(shell);
		XTDESTROYWIDGET(shell);
		have_shell = FALSE;
		confirm_new_entry();
	}
}


/*
 * create a message or script popup as a separate application shell. The
 * readonly flag allows displaying messages even though the appointment is
 * read-only; otherwise it would be impossible to display.
 */

void create_text_popup(
	char		*title,		/* menu title string */
	char		**initial,	/* initial default text */
	int		x,		/* button pressed, SC_* */
	BOOL		readonly)	/* just show, no editing */
{
	Widget		form, but, help;
	Arg		args[20];
	int		n;
	Atom		closewindow;

	destroy_text_popup();

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtCreatePopupShell(title, applicationShellWidgetClass,
			toplevel, args, n);

#	ifdef EDITRES
	XtAddEventHandler(shell, (EventMask)0, TRUE, 
 			_XEditResCheckMessages, NULL);
#	endif
	set_icon(shell, 1);
	form = XtCreateManagedWidget("msgform", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"msg");

							/*-- buttons --*/
	if (!readonly) {
	  n = 0;
	  XtSetArg(args[n], XmNbottomAttachment,XmATTACH_FORM);		n++;
	  XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	  XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	  XtSetArg(args[n], XmNleftOffset,	8);			n++;
	  XtSetArg(args[n], XmNleftOffset,	8);			n++;
	  XtSetArg(args[n], XmNwidth,		80);			n++;
	  XtSetArg(args[n], XmNtraversalOn,	False);			n++;
	  but = XtCreateManagedWidget(_("Delete"), xmPushButtonWidgetClass,
			form, args, n);
	  XtAddCallback(but, XmNactivateCallback, (XtCallbackProc)
			delete_callback, (XtPointer)0);
	  XtAddCallback(but, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer)"msg_delete");

	  n = 0;
	  XtSetArg(args[n], XmNbottomAttachment,XmATTACH_FORM);		n++;
	  XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	  XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	  XtSetArg(args[n], XmNleftWidget,	but);			n++;
	  XtSetArg(args[n], XmNleftOffset,	8);			n++;
	  XtSetArg(args[n], XmNwidth,		80);			n++;
	  XtSetArg(args[n], XmNtraversalOn,	False);			n++;
	  but = XtCreateManagedWidget(_("Clear"), xmPushButtonWidgetClass,
			form, args, n);
	  XtAddCallback(but, XmNactivateCallback, (XtCallbackProc)
			clear_callback, (XtPointer)0);
	  XtAddCallback(but, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer)"msg_clear");
	}
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	but = XtCreateManagedWidget(_("Done"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(but, XmNactivateCallback, (XtCallbackProc)
			done_callback, (XtPointer)0);
	XtAddCallback(but, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer)"msg_done");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	but);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	help = XtCreateManagedWidget(_("Help"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(help, XmNactivateCallback, (XtCallbackProc)
			help_callback, (XtPointer)"msg");
	XtAddCallback(help, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"msg");

							/*-- text --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	but);			n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNeditMode,		XmMULTI_LINE_EDIT);	n++;
	XtSetArg(args[n], XmNcolumns,		40);			n++;
	XtSetArg(args[n], XmNrows,		10);			n++;
	XtSetArg(args[n], XmNscrollVertical,	True);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNeditable,		!readonly);		n++;
	text = XmCreateScrolledText(form, "text", args, n);
	if (*initial) {
		XmTextSetString(text, *initial);
		XmTextSetInsertionPosition(text, strlen(*initial));
	}
	XtManageChild(text);
	XtAddCallback(text, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"msg_text");

	XtPopup(shell, XtGrabNone);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow, (XtCallbackProc)
			done_callback, (XtPointer)shell);
	have_shell   = TRUE;
	source       = initial;
	button_x     = x;
	readonly_msg = readonly;
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * All of these routines are direct X callbacks.
 */

/*ARGSUSED*/
static void done_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	destroy_text_popup();
}


/*ARGSUSED*/
static void delete_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	XmTextSetString(text, "");
	destroy_text_popup();
}


/*ARGSUSED*/
static void clear_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	XmTextSetString(text, "");
	XmTextSetInsertionPosition(text, 0);
}
