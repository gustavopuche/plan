/*
 * Create and destroy the alarm configuration popup. It contains possible
 * actions to be taken when a warn or alarm time triggers. The information
 * is saved at the beginning of the .dayplan file; the daemon needs it.
 *
 *	destroy_config_popup()
 *	create_config_popup()
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/RowColumn.h>
#include <Xm/LabelP.h>
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/TextF.h>
#include <Xm/Protocols.h>
#include "cal.h"

static void done_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void toggle_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void ewarn_prog_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void lwarn_prog_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void alarm_prog_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void mailer_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void timeout_callback	(Widget, int, XmToggleButtonCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern Widget		toplevel;	/* top-level shell for icon name */
extern struct edit	edit;		/* info about entry being edited */
extern struct config	config;		/* global configuration data */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern struct plist	*mainlist;	/* list of all schedule entries */

static BOOL		have_shell;	/* message popup exists if TRUE */
static Widget		shell;		/* popup menu shell */
static Widget		ewarn_prog;	/* early-warn program text */
static Widget		lwarn_prog;	/* late-warn program text */
static Widget		alarm_prog;	/* alarm time program text */
static Widget		mailer;		/* mailer command line text */
static Widget		timeout;	/* max lifetime of notifiers */


/*
 * destroy a popup. Remove it from the screen, and destroy its widgets.
 * It's too much trouble to keep them for next time.
 */

void destroy_config_popup(void)
{
	char		*string;	/* contents of text widget */
	char		*p;		/* for stripping leading blanks */

	if (have_shell) {
		string = XmTextFieldGetString(ewarn_prog);
		if (config.ewarn_prog)
			free(config.ewarn_prog);
		for (p=string; *p == ' ' || *p == '\t'; p++);
		config.ewarn_prog = mystrdup(p);
		XtFree(string);

		string = XmTextFieldGetString(lwarn_prog);
		if (config.lwarn_prog)
			free(config.lwarn_prog);
		for (p=string; *p == ' ' || *p == '\t'; p++);
		config.lwarn_prog = mystrdup(p);
		XtFree(string);

		string = XmTextFieldGetString(alarm_prog);
		if (config.alarm_prog)
			free(config.alarm_prog);
		for (p=string; *p == ' ' || *p == '\t'; p++);
		config.alarm_prog = mystrdup(p);
		XtFree(string);

		string = XmTextFieldGetString(mailer);
		if (config.mailer)
			free(config.mailer);
		for (p=string; *p == ' ' || *p == '\t'; p++);
		config.mailer = mystrdup(p);
		XtFree(string);

		string = XmTextFieldGetString(timeout);
		config.wintimeout = parse_timestring(string, TRUE);
		XtFree(string);

		XtPopdown(shell);
		XTDESTROYWIDGET(shell);
		have_shell = FALSE;
		mainlist->modified = TRUE;
		write_mainlist();
	}
}


/*
 * create a config popup as a separate application shell. The popup is
 * initialized with data from config.
 */

void create_config_popup(void)
{
	Widget			form, frame, form2, rowcol, w;
	Arg			args[20];
	int			n;
	Atom			closewindow;
	int			len;

	destroy_config_popup();
	len = XTextWidth(font[FONT_STD], _("Early warning:"), 14);

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtCreatePopupShell(_("Alarm Options"),
			applicationShellWidgetClass, toplevel, args, n);
#	ifdef EDITRES
	XtAddEventHandler(shell, (EventMask)0, TRUE, 
 			_XEditResCheckMessages, NULL);
#	endif
	set_icon(shell, 1);
	form = XtCreateManagedWidget("cnfform1", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)help_callback,
			(XtPointer)"opt");

							/*-- early warn --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM)	;	n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNshadowType,	XmSHADOW_ETCHED_IN);	n++;
	frame = XtCreateManagedWidget("", xmFrameWidgetClass,
			form, args, n);
	form2 = XtCreateManagedWidget("cnfform2", xmFormWidgetClass,
			frame, NULL, 0);
	XtAddCallback(form2, XmNhelpCallback, (XtCallbackProc)help_callback,
			(XtPointer)"opt_early");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		len);			n++;
	w = XtCreateManagedWidget(_("Early warning:"), xmLabelWidgetClass,
			form2, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNspacing,		4);			n++;
	rowcol = XtCreateManagedWidget("", xmRowColumnWidgetClass,
			form2, args, n);
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.ewarn_window);	n++;
	w = XtCreateManagedWidget(_("Green window"), xmToggleButtonWidgetClass,
			rowcol, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			toggle_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_early_w");

	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.ewarn_mail);	n++;
	w = XtCreateManagedWidget(_("Send mail"), xmToggleButtonWidgetClass,
			rowcol, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			toggle_callback, (XtPointer)1);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_early_m");

	form2 = XtCreateManagedWidget("cnfform3", xmFormWidgetClass,
			rowcol, NULL, 0);
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.ewarn_exec);	n++;
	w = XtCreateManagedWidget(_("Execute"), xmToggleButtonWidgetClass,
			form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			toggle_callback, (XtPointer)2);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_early_x");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		350);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	ewarn_prog = XtCreateManagedWidget("", xmTextFieldWidgetClass,
			form2, args, n);
	XtAddCallback(ewarn_prog, XmNactivateCallback, (XtCallbackProc)
			ewarn_prog_callback, (XtPointer)NULL);
	XtAddCallback(ewarn_prog, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_early_x");
	if (config.ewarn_prog)
		print_text_button(ewarn_prog, "%s", config.ewarn_prog);

							/*-- late warn --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		frame);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNshadowType,	XmSHADOW_ETCHED_IN);	n++;
	frame = XtCreateManagedWidget("", xmFrameWidgetClass,
			form, args, n);
	form2 = XtCreateManagedWidget("cnfform4", xmFormWidgetClass,
			frame, NULL, 0);
	XtAddCallback(form2, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_late");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		len);			n++;
	w = XtCreateManagedWidget(_("Late warning:"), xmLabelWidgetClass,
			form2, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNspacing,		4);			n++;
	rowcol = XtCreateManagedWidget("", xmRowColumnWidgetClass,
			form2, args, n);
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.lwarn_window);	n++;
	w = XtCreateManagedWidget(_("Yellow window"),
			xmToggleButtonWidgetClass, rowcol, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			toggle_callback, (XtPointer)3);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_late_w");

	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.lwarn_mail);	n++;
	w = XtCreateManagedWidget(_("Send mail"), xmToggleButtonWidgetClass,
			rowcol, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			toggle_callback, (XtPointer)4);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_late_m");

	form2 = XtCreateManagedWidget("cnfform5", xmFormWidgetClass,
			rowcol, NULL, 0);
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.lwarn_exec);	n++;
	w = XtCreateManagedWidget(_("Execute"), xmToggleButtonWidgetClass,
			form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			toggle_callback, (XtPointer)5);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_late_x");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		350);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	lwarn_prog = XtCreateManagedWidget("", xmTextFieldWidgetClass,
			form2, args, n);
	XtAddCallback(lwarn_prog, XmNactivateCallback, (XtCallbackProc)
			lwarn_prog_callback, (XtPointer)NULL);
	XtAddCallback(lwarn_prog, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_late_x");
	if (config.lwarn_prog)
		print_text_button(lwarn_prog, "%s", config.lwarn_prog);

							/*-- alarm --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		frame);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNshadowType,	XmSHADOW_ETCHED_IN);	n++;
	frame = XtCreateManagedWidget("", xmFrameWidgetClass,
			form, args, n);
	form2 = XtCreateManagedWidget("cnfform6", xmFormWidgetClass,
			frame, NULL, 0);
	XtAddCallback(form2, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_alarm");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		len);			n++;
	w = XtCreateManagedWidget(_("Alarm:"), xmLabelWidgetClass,
			form2, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNspacing,		4);			n++;
	rowcol = XtCreateManagedWidget("", xmRowColumnWidgetClass,
			form2, args, n);
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.alarm_window);	n++;
	w = XtCreateManagedWidget(_("Red window"), xmToggleButtonWidgetClass,
			rowcol, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			toggle_callback, (XtPointer)6);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_alarm_a");

	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.alarm_mail);	n++;
	w = XtCreateManagedWidget(_("Send mail"), xmToggleButtonWidgetClass,
			rowcol, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			toggle_callback, (XtPointer)7);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_alarm_m");

	form2 = XtCreateManagedWidget("cnfform7", xmFormWidgetClass,
			rowcol, NULL, 0);
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.alarm_exec);	n++;
	w = XtCreateManagedWidget(_("Execute"), xmToggleButtonWidgetClass,
			form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			toggle_callback, (XtPointer)8);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_alarm_x");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		350);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	alarm_prog = XtCreateManagedWidget("", xmTextFieldWidgetClass,
			form2, args, n);
	XtAddCallback(alarm_prog, XmNactivateCallback, (XtCallbackProc)
			alarm_prog_callback, (XtPointer)NULL);
	XtAddCallback(alarm_prog, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_alarm_x");
	if (config.alarm_prog)
		print_text_button(alarm_prog, "%s", config.alarm_prog);

							/*-- mailer --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		frame);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	w = XtCreateManagedWidget(_("Mailer:"), xmLabelWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_mailer");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		frame);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		250);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	mailer = XtCreateManagedWidget("", xmTextFieldWidgetClass,
			form, args, n);
	XtAddCallback(mailer, XmNactivateCallback, (XtCallbackProc)
			mailer_callback, (XtPointer)NULL);
	XtAddCallback(mailer, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_mailer");
	if (config.mailer)
		print_text_button(mailer, "%s", config.mailer);

							/*-- timeout --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		mailer);		n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	w = XtCreateManagedWidget(_("Windows time out after"),
			xmLabelWidgetClass, form, args, n);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_timeout");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		mailer);		n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		120);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	timeout = XtCreateManagedWidget("", xmTextFieldWidgetClass,
			form, args, n);
	XtAddCallback(timeout, XmNactivateCallback, (XtCallbackProc)
			timeout_callback, (XtPointer)NULL);
	XtAddCallback(timeout, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_timeout");
	if (config.wintimeout > 59)
		print_text_button(timeout, "%s",
					mktimestring(config.wintimeout, TRUE));

							/*-- buttons --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		timeout);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Done"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			done_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"opt_done");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		timeout);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Help"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)help_callback,
			(XtPointer)"opt");
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)help_callback,
			(XtPointer)"opt");

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
static void done_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	destroy_config_popup();
}


/*ARGSUSED*/
static void toggle_callback(
	UNUSED Widget			widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	switch(item) {
	  case 0:	config.ewarn_window = data->set;	break;
	  case 1:	config.ewarn_mail   = data->set;	break;
	  case 2:	config.ewarn_exec   = data->set;	break;
	  case 3:	config.lwarn_window = data->set;	break;
	  case 4:	config.lwarn_mail   = data->set;	break;
	  case 5:	config.lwarn_exec   = data->set;	break;
	  case 6:	config.alarm_window = data->set;	break;
	  case 7:	config.alarm_mail   = data->set;	break;
	  case 8:	config.alarm_exec   = data->set;	break;
	}
}


/*ARGSUSED*/
static void ewarn_prog_callback(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	char				*text = XmTextFieldGetString(widget);

	if (config.ewarn_prog)
		free(config.ewarn_prog);
	config.ewarn_prog = mystrdup(text);
	XtFree(text);
}


/*ARGSUSED*/
static void lwarn_prog_callback(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	char				*text = XmTextFieldGetString(widget);

	if (config.lwarn_prog)
		free(config.lwarn_prog);
	config.lwarn_prog = mystrdup(text);
	XtFree(text);
}


/*ARGSUSED*/
static void alarm_prog_callback(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	char				*text = XmTextFieldGetString(widget);

	if (config.alarm_prog)
		free(config.alarm_prog);
	config.alarm_prog = mystrdup(text);
	XtFree(text);
}


/*ARGSUSED*/
static void mailer_callback(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	char				*text = XmTextFieldGetString(widget);

	if (config.mailer)
		free(config.mailer);
	config.mailer = mystrdup(text);
	XtFree(text);
}


/*ARGSUSED*/
static void timeout_callback(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	char				*text = XmTextFieldGetString(widget);

	config.wintimeout = parse_timestring(text, TRUE);
	XtFree(text);
	print_text_button(timeout, config.wintimeout > 59 ? "%s" : "",
					mktimestring(config.wintimeout, TRUE));
}
