/*
 * Create and destroy the holiday popup. It is used for editing the
 * ~/.holiday file. It also has an informative help text that explains
 * the syntax. When the popup is removed, the holiday file is re-parsed.
 *
 *	destroy_holiday_popup()
 *	create_holiday_popup()
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
static void cancel_callback	(Widget, int, XmToggleButtonCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern Widget		toplevel;	/* top-level shell for icon name */
extern struct edit	edit;		/* info about entry being edited */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern int		curr_year;	/* year being displayed, since 1900 */
extern struct mainmenu	mainmenu;	/* all important main window widgets */

static BOOL		have_shell;	/* message popup exists if TRUE */
static Widget		shell;		/* popup menu shell */
static Widget		text;		/* text widget */


/*
 * destroy a popup. Remove it from the screen, and destroy its widgets.
 * It's too much trouble to keep them for next time.
 */

void destroy_holiday_popup(void)
{
	char		*string;	/* contents of text widget */
	char		*errmsg;	/* holiday parser error */
	FILE		*fp;		/* holiday file */

	if (have_shell) {
		if (!(fp = fopen(resolve_tilde(HOLIDAY_PATH), "w")))
			perror(HOLIDAY_PATH);
		else {
			string = XmTextGetString(text);
			if (fwrite(string, strlen(string), 1, fp) != 1)
				perror(HOLIDAY_PATH);
			XtFree(string);
			fclose(fp);
			if ((errmsg = parse_holidays(-1, TRUE)))
				create_error_popup(mainmenu.cal, 0, errmsg);
			update_all_listmenus(TRUE);
			redraw_all_views();
		}
		XtPopdown(shell);
		XTDESTROYWIDGET(shell);
		have_shell = FALSE;
		confirm_new_entry();
	}
}


/*
 * create a holiday popup as a separate application shell.
 */

void create_holiday_popup(void)
{
	Widget			form, but, help;
	Arg			args[20];
	int			n;
	Atom			closewindow;
	FILE			*fp;

	destroy_holiday_popup();

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtCreatePopupShell(_("Holidays"),
			applicationShellWidgetClass, toplevel, args, n);
#	ifdef EDITRES
	XtAddEventHandler(shell, (EventMask)0, TRUE, 
 			_XEditResCheckMessages, NULL);
#	endif
	set_icon(shell, 1);
	form = XtCreateManagedWidget("dayform", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"holiday");

							/*-- buttons --*/
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
			help_callback, (XtPointer)"holiday_done");

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
			help_callback, (XtPointer)"holiday");
	XtAddCallback(help, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"holiday");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	help);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	but = XtCreateManagedWidget(_("Cancel"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(but, XmNactivateCallback, (XtCallbackProc)
			cancel_callback,(XtPointer)0);
	XtAddCallback(but, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer)"holiday_cancel");

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
	XtSetArg(args[n], XmNcolumns,		80);			n++;
	XtSetArg(args[n], XmNrows,		20);			n++;
	XtSetArg(args[n], XmNscrollVertical,	True);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	text = XmCreateScrolledText(form, "holiday", args, n);

	if ((fp = fopen(resolve_tilde(HOLIDAY_PATH), "r"))) {
		fseek(fp, 0, 2);
		if ((n = ftell(fp))) {
			char *buf;
			rewind(fp);
			if ((buf = malloc(n+1))) {
				if (fread(buf, n, 1, fp) == 1) {
					buf[n] = 0;
					XmTextSetString(text, buf);
				}
				free(buf);
			}
		}
		fclose(fp);
	}
	XmTextSetInsertionPosition(text, 0);
	XtManageChild(text);
	XtAddCallback(text, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"holiday");

	XtPopup(shell, XtGrabNone);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow, (XtCallbackProc)
			done_callback, (XtPointer)shell);

	have_shell = TRUE;
}


/*-------------------------------------------------- callbacks --------------*/
/*ARGSUSED*/
static void done_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	destroy_holiday_popup();
}

/*ARGSUSED*/
static void cancel_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	XtPopdown(shell);
	XTDESTROYWIDGET(shell);
	have_shell = FALSE;
}
