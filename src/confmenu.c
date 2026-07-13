/*
 * Create and destroy the calendar view config menu, which shows global,
 * month, and week options. It's all called "week range" for historical
 * reasons, guess what this file did originally. This is called from the
 * Config pulldown.
 *
 *	destroy_calconfig_popup()
 *	create_calconfig_popup()
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
#include <Xm/ScrolledW.h>
#include <Xm/Protocols.h>
#include "cal.h"

static void flag_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void done_callback	(Widget, int, XmToggleButtonCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern Widget		toplevel;	/* top-level shell for icon name */
extern struct config	config;		/* global configuration data */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern struct mainmenu	mainmenu;	/* all important main window widgets */

static BOOL		have_shell;	/* message popup exists if TRUE */
static Widget		shell;		/* popup menu shell */
static Widget		ndays;		/* # of days in week text widgets */
static Widget		defwarn;	/* default warning string */
static Widget		min, max;	/* min and max hour text widgets */


/*
 * destroy a popup. Remove it from the screen, and destroy its widgets.
 * It's too much trouble to keep them for next time.
 */

void destroy_calconfig_popup(void)
{
	char			*string;	/* contents of text widget */
	int			n, b, e;
	BOOL			dummy;

	if (have_shell) {
		string = XmTextFieldGetString(ndays);
		n = atoi(string);
		XtFree(string);

		string = XmTextFieldGetString(defwarn);
		parse_warnstring(string, &config.early_time,
					 &config.late_time, &dummy);
		XtFree(string);

		string = XmTextFieldGetString(min);
		b = atoi(string);
		XtFree(string);

		string = XmTextFieldGetString(max);
		e = atoi(string);
		XtFree(string);

		if (e == 0)	e = 24;
		if (e <= b)	e = e+1;
		if (e > 24)	e = 24;
		if (e <= b)	b = e-1;

		config.week_ndays = n < 1 ? 1 : n > NDAYS ? NDAYS : n;
		config.week_minhour  = b;
		config.week_maxhour  = e;

		XtPopdown(shell);
		XTDESTROYWIDGET(shell);
		have_shell = FALSE;
		mainlist->modified = TRUE;
		write_mainlist();
		draw_week_calendar(NULL);
		draw_day_calendar(NULL);
	}
}


/*
 * create a week view range popup as a separate application shell. The
 * popup is initialized with data from config.
 */

void create_calconfig_popup(void)
{
	Widget			root, scroll, form, frame, form2, w, w2, help;
	Arg			args[20];
	int			n, a, b;
	Atom			closewindow;
	Dimension		xs=0, ys=0;

	destroy_calconfig_popup();

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtCreatePopupShell(_("Calendar view configuration"),
			applicationShellWidgetClass, toplevel, args, n);
#	ifdef EDITRES
	XtAddEventHandler(shell, (EventMask)0, TRUE, 
 			_XEditResCheckMessages, NULL);
#	endif
	set_icon(shell, 1);
	root = XtCreateManagedWidget("rangeform0", xmFormWidgetClass,
			shell, NULL, 0);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNshadowThickness,	0);			n++;
	XtSetArg(args[n], XmNscrollingPolicy,	XmVARIABLE);		n++;
	scroll = XtCreateManagedWidget("scroll", xmScrolledWindowWidgetClass,
			root, args, n);
	XtAddCallback(scroll, XmNhelpCallback, (XtCallbackProc)help_callback,
			(XtPointer)"calconfig");
	form = XtCreateManagedWidget("rangeform0", xmFormWidgetClass,
			scroll, NULL, 0);

							/*-- buttons --*/
	n = 0;
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
			help_callback, (XtPointer) "range_done");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	help = XtCreateManagedWidget(_("Help"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(help, XmNactivateCallback, (XtCallbackProc)
			help_callback, (XtPointer)"calconfig");
	XtAddCallback(help, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"calconfig");

						/*------------- global ------*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	w = XtCreateManagedWidget(_("Global options:"),
			xmLabelWidgetClass, form, args, n);

							/*-- frame --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNshadowType,	XmSHADOW_ETCHED_IN);	n++;
	frame = XtCreateManagedWidget("", xmFrameWidgetClass,
			form, args, n);
	form2 = XtCreateManagedWidget("rangeform2", xmFormWidgetClass,
			frame, NULL, 0);
							/*-- share mainwin --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.share_mainwin);	n++;
	w = XtCreateManagedWidget(_("Use main window for all views"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback, (XtPointer)14);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"global_flags");

							/*-- sunday 1st --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.sunday_first);	n++;
	w = XtCreateManagedWidget(_("Week begins with Sunday"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"global_flags");

							/*-- m/d/y --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.mmddyy);		n++;
	w = XtCreateManagedWidget(_("Month/day/year date format"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback, (XtPointer)1);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"global_flags");

							/*-- am/pm --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.ampm);		n++;
	w = XtCreateManagedWidget(_("12-hour am/pm time format"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback, (XtPointer)2);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"global_flags");

							/*-- autodelete --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.autodel);	n++;
	w = XtCreateManagedWidget(_("Auto-delete past entries"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback, (XtPointer)3);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"global_flags");

							/*-- insecure script */
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.insecure_script);n++;
	w = XtCreateManagedWidget(_("Run netplan scripts (security risk!)"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback, (XtPointer)15);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"global_flags");

							/*-- owner only --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.owner_only);	n++;
	XtSetArg(args[n], XmNsensitive,		!all_files_served());	n++;
	w = w2 = XtCreateManagedWidget(_("Only owner can write appt files"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback,(XtPointer)11);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"global_flags");

							/*-- default warn --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w2);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	w = XtCreateManagedWidget(_("Default warnings:"),
			xmLabelWidgetClass, form2, args, n);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"range_defwarn");
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w2);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	4);			n++;
	XtSetArg(args[n], XmNwidth,		120);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	defwarn = XtCreateManagedWidget(" ", xmTextFieldWidgetClass,
			form2, args, n);
	XtAddCallback(defwarn, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"range_defwarn");

						/*------------- month -------*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		frame);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	w = XtCreateManagedWidget(_("Month view options:"),
			xmLabelWidgetClass, form, args, n);

							/*-- frame --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNshadowType,	XmSHADOW_ETCHED_IN);	n++;
	frame = XtCreateManagedWidget("", xmFrameWidgetClass,
			form, args, n);
	form2 = XtCreateManagedWidget("rangeform2", xmFormWidgetClass,
			frame, NULL, 0);
							/*-- julian --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.julian);		n++;
	w = XtCreateManagedWidget(_("Show Julian dates"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback, (XtPointer)4);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"month_flags");

							/*-- week num --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.weeknum);	n++;
	w = XtCreateManagedWidget(_("Show week numbers"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback, (XtPointer)5);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"month_flags");

							/*-- gps week --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.gpsweek);	n++;
	w = XtCreateManagedWidget(_("Use GPS week numbers"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback,(XtPointer)13);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"month_flags");

							/*-- full week --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.fullweek);	n++;
	w = XtCreateManagedWidget(_("First week is first full week"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback, (XtPointer)6);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"month_flags");

							/*-- Thu week --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.thu_week);	n++;
	w = XtCreateManagedWidget(_("First week is first week with Thursday"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback,(XtPointer)16);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"month_flags");

							/*-- color code --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.colorcode_m);	n++;
	w = XtCreateManagedWidget(_("Colored background for other files"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback, (XtPointer)7);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"month_flags");

							/*-- no past --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.nopast);		n++;
	w = XtCreateManagedWidget(_("Don't show past entries in today's box"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback, (XtPointer)8);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"month_flags");


						/*------------- week --------*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		frame);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	w = XtCreateManagedWidget(_("Week and Day view options:"),
			xmLabelWidgetClass, form, args, n);

							/*-- frame --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	help);			n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNshadowType,	XmSHADOW_ETCHED_IN);	n++;
	w = XtCreateManagedWidget("", xmFrameWidgetClass,
			form, args, n);
	form2 = XtCreateManagedWidget("rangeform2", xmFormWidgetClass,
			w, NULL, 0);
							/*-- # of days --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		10);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	w = XtCreateManagedWidget(_("Week view contains"),
			xmLabelWidgetClass, form2, args, n);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"range_ndays");
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	ndays = XtCreateManagedWidget(" ", xmTextFieldWidgetClass,
			form2, args, n);
	XtAddCallback(ndays, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"range_ndays");
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		10);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	ndays);			n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtCreateManagedWidget(_("days"), xmLabelWidgetClass,
			form2, args, n);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"range_ndays");

							/*-- min --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		ndays);			n++;
	XtSetArg(args[n], XmNtopOffset,		6);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	w = XtCreateManagedWidget(_("Begin view at"),
			xmLabelWidgetClass, form2, args, n);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"range_min");
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		ndays);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	ndays);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	min = XtCreateManagedWidget(" ", xmTextFieldWidgetClass,
			form2, args, n);
	XtAddCallback(min, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"range_min");
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		ndays);			n++;
	XtSetArg(args[n], XmNtopOffset,		6);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	min);			n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtCreateManagedWidget(_("o'clock"), xmLabelWidgetClass,
			form2, args, n);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"range_min");

							/*-- max --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		min);			n++;
	XtSetArg(args[n], XmNtopOffset,		6);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	w = XtCreateManagedWidget(_("End view at"),
			xmLabelWidgetClass, form2, args, n);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"range_max");
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		min);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	ndays);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	max = XtCreateManagedWidget("", xmTextFieldWidgetClass,
			form2, args, n);
	XtAddCallback(max, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"range_max");
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		min);			n++;
	XtSetArg(args[n], XmNtopOffset,		6);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	min);			n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtCreateManagedWidget(_("o'clock"), xmLabelWidgetClass,
			form2, args, n);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"range_max");

							/*-- show warnings --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		max);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.weekwarn);	n++;
	w = XtCreateManagedWidget(_("Show advance-warning times"),
			xmToggleButtonWidgetClass,
			form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback,(XtPointer)9);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"range_warn");

							/*-- show filename --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.weekuser);	n++;
	w = XtCreateManagedWidget(_("Show file names in bar labels"),
			xmToggleButtonWidgetClass,
			form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback,(XtPointer)10);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"range_user");

							/*-- notime bars --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.week_bignotime);	n++;
	w = XtCreateManagedWidget(_("Large bars for week entries without time"),
			xmToggleButtonWidgetClass,
			form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback, (XtPointer)12);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"range_user");

							/*-- extra times --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.moretimecols);	n++;
	w = XtCreateManagedWidget(_("Insert extra time columns and rows"),
			xmToggleButtonWidgetClass,
			form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			flag_callback, (XtPointer)17);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"range_user");

	XtRealizeWidget(shell);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow, (XtCallbackProc)
			done_callback, (XtPointer)shell);
	XtSetArg(args[0], XmNwidth,  &xs);
	XtSetArg(args[1], XmNheight, &ys);
	XtGetValues(form, args, 2);
	XtSetArg(args[0], XmNwidth,  xs+8);
	XtSetArg(args[1], XmNheight, ys+8);
	XtSetValues(shell, args, 2);
	XtPopup(shell, XtGrabNone);

	a = config.early_time / 60;
	b = config.late_time  / 60;
	print_text_button(defwarn, a&&b ? "%d, %d" : a ? "%d" : "", a?a:b, b);
	print_text_button(ndays,   "%d", config.week_ndays);
	print_text_button(min,     "%d", config.week_minhour);
	print_text_button(max,     "%d", config.week_maxhour);

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
	destroy_calconfig_popup();
}


static BOOL *flagptr[] = {
	&config.sunday_first, &config.mmddyy, &config.ampm, &config.autodel,
	&config.julian, &config.weeknum, &config.fullweek, &config.colorcode_m,
	&config.nopast, &config.weekwarn,&config.weekuser, &config.owner_only,
	&config.week_bignotime, &config.gpsweek, &config.share_mainwin,
	&config.insecure_script, &config.thu_week, &config.moretimecols
};

/*ARGSUSED*/
static void flag_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	*flagptr[item] = data->set;
	if (flagptr[item] == &config.owner_only) {
		read_mainlist();
		if (!config.owner_only)
			multiple_writer_warning_popup(widget);
	}
	if (flagptr[item] == &config.share_mainwin && !config.share_mainwin) {
		destroy_year_menu();
		destroy_week_menu();
		destroy_yov_menu();
		destroy_day_menu();
		create_view('m');
	}
	mainlist->modified = TRUE;
	update_all_listmenus(TRUE);
	redraw_all_views();
	print_icon_name();
	if (config.smallmonth)
		print_button(mainmenu.time, mktimestring(get_time(), FALSE));
	else
		print_button(mainmenu.time, "%s   %s",
					    mkdatestring(get_time()),
					    mktimestring(get_time(), FALSE));
}
