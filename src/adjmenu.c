/*
 * Create and destroy the time adjustment popup. It specidies a time offset,
 * and timezone and daylight saving adjustment.
 *
 *	destroy_adjust_popup()
 *	create_adjust_popup()
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
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/Protocols.h>
#include "cal.h"

static void done_callback  (Widget, int, XmToggleButtonCallbackStruct *);
static void guess_callback (Widget, int, XmToggleButtonCallbackStruct *);
static void dst_callback   (Widget, int, XmToggleButtonCallbackStruct *);
static void adjust_callback(Widget, int, XmToggleButtonCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern Widget		toplevel;	/* top-level shell for icon name */
extern struct edit	edit;		/* info about entry being edited */
extern struct config	config;		/* global configuration data */
extern XtAppContext	app;		/* application handle */
extern struct mainmenu	mainmenu;	/* all important main window widgets */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern struct plist	*mainlist;	/* list of all schedule entries */

static BOOL		have_shell;	/* message popup exists if TRUE */
static Widget		shell;		/* popup menu shell */
static Widget		time_w;		/* current time display */
static Widget		adjust_w;	/* time adjustment text widget */
static Widget		tzone_w;	/* timezone text widget */
static Widget		toggle0_w;	/* radio button for dst on */
static Widget		toggle1_w;	/* radio button for dst off */
static Widget		toggle2_w;	/* radio button for auto dst */
static Widget		toggle3_w;	/* radio button for system dst */
static Widget		begin_w;	/* daylight saving begin text widget */
static Widget		end_w;		/* daylight saving end text widget */


/*
 * print current time, once a second and when a button is pressed
 */

static void print_times(void)
{
	int			hour, min, sec;

	sec = get_time();
	if (config.smallmonth)
		print_button(mainmenu.time,	mktimestring(sec, FALSE));
	else
		print_button(mainmenu.time, "%s   %s",
						mkdatestring(sec),
						mktimestring(sec, FALSE));
	hour = (sec / 3600) % 24;
	min  = (sec / 60) % 60;
	sec %= 60;
	set_tzone();
	print_icon_name();
	if (config.ampm)
		print_button(time_w, "%2d:%02d:%02d%c", hour%12 ? hour%12 : 12,
						min, sec, "ap"[hour >= 12]);
	else
		print_button(time_w, "%02d:%02d:%02d", hour, min, sec);
}

/*ARGSUSED*/
static void sec_timer_callback(
	UNUSED XtPointer	data,		/* not used */
	UNUSED XtIntervalId	*id)		/* not used */
{
	if (have_shell) {
		print_times();
		XtAppAddTimeOut(app, 1000, sec_timer_callback, 0);
	}
}


/*
 * print text into all text widgets
 */

static void print_text_buttons(void)
{
	Arg			args;
	time_t			t;

	t = config.adjust_time < 0 ? -config.adjust_time : config.adjust_time;
	print_text_button(adjust_w, "%c %d:%02d:%02d",
				config.adjust_time < 0 ? '-' : '+',
				t/3600, (t/60)%60, t%60);

	t = config.raw_tzone < 0 ? -config.raw_tzone : config.raw_tzone;
	print_text_button(tzone_w, "%c %d:%02d",
				config.raw_tzone < 0 ? '-' : '+',
				t/3600, (t/60)%60);

	if (config.dst_flag == 2) {
		print_text_button(begin_w, "%d", config.dst_begin);
		print_text_button(end_w,   "%d", config.dst_end);
		XtSetArg(args, XmNsensitive, TRUE);
	} else {
		print_text_button(begin_w, "");
		print_text_button(end_w,   "");
		XtSetArg(args, XmNsensitive, FALSE);
	}
	XtSetValues(begin_w, &args, 1);
	XtSetValues(end_w,   &args, 1);
}


/*
 * retrieve texts from text widgets, store everything in config
 */

static void eval_menu_texts(void)
{
	char		*string;	/* contents of text widget */

	string = XmTextGetString(adjust_w);
	config.adjust_time = *string=='+' ?  parse_timestring(string+1, TRUE) :
			     *string=='-' ? -parse_timestring(string+1, TRUE)
					  :  parse_timestring(string,   TRUE);
	XtFree(string);

	string = XmTextGetString(tzone_w);
	config.raw_tzone   = *string=='+' ?  parse_timestring(string+1, TRUE) :
			     *string=='-' ? -parse_timestring(string+1, TRUE)
					  :  parse_timestring(string,   TRUE);
	XtFree(string);

	string = XmTextGetString(begin_w);
	config.dst_begin = atoi(string);
	XtFree(string);

	string = XmTextGetString(end_w);
	config.dst_end = atoi(string);
	XtFree(string);

	set_tzone();
}


/*
 * destroy a popup. Remove it from the screen, and destroy its widgets.
 * It's too much trouble to keep them for next time.
 */

void destroy_adjust_popup(void)
{
	if (have_shell) {
		eval_menu_texts();
		XtPopdown(shell);
		XTDESTROYWIDGET(shell);
		have_shell = FALSE;
		mainlist->modified = TRUE;
		write_mainlist();
		resynchronize_daemon();
	}
}


/*
 * create an adjust popup as a separate application shell. The popup is
 * initialized with data from config.
 */

void create_adjust_popup(void)
{
	Widget			form, frame, form2, form3, radio, w, w2;
	Arg			args[20];
	int			n;
	Atom			closewindow;

	destroy_adjust_popup();

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtCreatePopupShell(_("Adjust Time"),
			applicationShellWidgetClass, toplevel, args, n);
#	ifdef EDITRES
	XtAddEventHandler(shell, (EventMask)0, TRUE, 
 			_XEditResCheckMessages, NULL);
#	endif
	set_icon(shell, 1);
	form = XtCreateManagedWidget("adjform1", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)help_callback,
			(XtPointer)"adj");

							/*-- current time --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	time_w = XtCreateManagedWidget(" ", xmLabelWidgetClass,
			form, args, n);
							/*-- adjust time --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		time_w);		n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNshadowType,	XmSHADOW_ETCHED_IN);	n++;
	frame = XtCreateManagedWidget("", xmFrameWidgetClass,
			form, args, n);
	form2 = XtCreateManagedWidget("adjform2", xmFormWidgetClass,
			frame, NULL, 0);
	XtAddCallback(form2, XmNhelpCallback, (XtCallbackProc)help_callback,
			(XtPointer)"adj_time");
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		12);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	w = XtCreateManagedWidget(_("Add"), xmLabelWidgetClass,
			form2, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNwidth,		140);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	adjust_w = XtCreateManagedWidget("", xmTextFieldWidgetClass,
			form2, args, n);
	XtAddCallback(adjust_w, XmNactivateCallback, (XtCallbackProc)
			adjust_callback, (XtPointer)NULL);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		12);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	adjust_w);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	w = XtCreateManagedWidget(_("(h:m:s) to system time"),
			xmLabelWidgetClass, form2, args, n);
							/*-- adjust tzone --*/
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
	form2 = XtCreateManagedWidget("adjform3", xmFormWidgetClass,
			frame, NULL, 0);
	XtAddCallback(form2, XmNhelpCallback, (XtCallbackProc)help_callback,
			(XtPointer)"adj_dst");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	w2 = XtCreateManagedWidget(_("Timezone is GMT"), xmLabelWidgetClass,
			form2, args, n);
	XtAddCallback(w2, XmNhelpCallback, (XtCallbackProc)help_callback,
			(XtPointer)"adj_zone");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w2);			n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNwidth,		100);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	tzone_w = XtCreateManagedWidget("", xmTextFieldWidgetClass,
			form2, args, n);
	XtAddCallback(tzone_w, XmNactivateCallback, (XtCallbackProc)
			adjust_callback, (XtPointer)NULL);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	tzone_w);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	w = XtCreateManagedWidget(_("(h:m)"), xmLabelWidgetClass,
			form2, args, n);
							/*-- dst radio --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w2);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	form3 = XtCreateManagedWidget("adjform2", xmFormWidgetClass,
			form2, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		24);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	w = XtCreateManagedWidget(_("Daylight Saving is"), xmLabelWidgetClass,
			form3, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		20);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNspacing,		4);			n++;
	radio = XmCreateRadioBox(form3, "radio", args, n);

	n = 0;
	XtSetArg(args[n], XmNindicatorType,	XmONE_OF_MANY);		n++;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.dst_flag==3);	n++;
	toggle3_w = XtCreateManagedWidget(_("use system time"),
			xmToggleButtonWidgetClass,
			radio, args, n);
	XtAddCallback(toggle3_w, XmNvalueChangedCallback, (XtCallbackProc)
			dst_callback, (XtPointer)3);
	n = 0;
	XtSetArg(args[n], XmNindicatorType,	XmONE_OF_MANY);		n++;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.dst_flag==0);	n++;
	toggle0_w = XtCreateManagedWidget(_("always on"),
			xmToggleButtonWidgetClass,
			radio, args, n);
	XtAddCallback(toggle0_w, XmNvalueChangedCallback, (XtCallbackProc)
			dst_callback, (XtPointer)0);
	n = 0;
	XtSetArg(args[n], XmNindicatorType,	XmONE_OF_MANY);		n++;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.dst_flag==1);	n++;
	toggle1_w = XtCreateManagedWidget(_("always off"),
			xmToggleButtonWidgetClass,
			radio, args, n);
	XtAddCallback(toggle1_w, XmNvalueChangedCallback, (XtCallbackProc)
			dst_callback, (XtPointer)1);
	n = 0;
	XtSetArg(args[n], XmNindicatorType,	XmONE_OF_MANY);		n++;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		config.dst_flag==2);	n++;
	toggle2_w = XtCreateManagedWidget(_("automatic"),
			xmToggleButtonWidgetClass,
			radio, args, n);
	XtAddCallback(toggle2_w, XmNvalueChangedCallback, (XtCallbackProc)
			dst_callback, (XtPointer)2);

							/*-- dst day range --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		form3);			n++;
	XtSetArg(args[n], XmNtopOffset,		24);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	w = XtCreateManagedWidget(_("Automatic DST from Julian day"),
			xmLabelWidgetClass,
			form2, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		form3);			n++;
	XtSetArg(args[n], XmNtopOffset,		20);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNwidth,		60);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	begin_w = XtCreateManagedWidget("", xmTextFieldWidgetClass,
			form2, args, n);
	XtAddCallback(begin_w, XmNactivateCallback, (XtCallbackProc)
			adjust_callback, (XtPointer)NULL);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		form3);			n++;
	XtSetArg(args[n], XmNtopOffset,		24);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	begin_w);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	w = XtCreateManagedWidget(_("to"), xmLabelWidgetClass,
			form2, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		form3);			n++;
	XtSetArg(args[n], XmNtopOffset,		20);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		60);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	end_w = XtCreateManagedWidget("", xmTextFieldWidgetClass,
			form2, args, n);
	XtAddCallback(end_w, XmNactivateCallback, (XtCallbackProc)
			adjust_callback, (XtPointer)NULL);

							/*-- buttons --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		frame);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
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
			help_callback, (XtPointer)"adj_done");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		frame);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Help"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			help_callback, (XtPointer)"adj");
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer)"adj");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		frame);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Guess"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			guess_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"adj_guess");

	XtManageChild(radio);
	XtPopup(shell, XtGrabNone);
	print_text_buttons();
	print_times();

	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow, (XtCallbackProc)
			done_callback, (XtPointer)shell);
	have_shell = TRUE;
	XtAppAddTimeOut(app, 1000, sec_timer_callback, 0);
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
	destroy_adjust_popup();
}


/*ARGSUSED*/
static void guess_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	Arg				args;

	guess_tzone();
	set_tzone();
	XtSetArg(args, XmNset, FALSE);
	XtSetValues(toggle0_w, &args, 1);
	XtSetValues(toggle1_w, &args, 1);
	XtSetValues(toggle3_w, &args, 1);
	XtSetArg(args, XmNset, TRUE);
	XtSetValues(toggle2_w, &args, 1);
	print_text_buttons();
	print_times();
}


/*ARGSUSED*/
static void dst_callback(
	UNUSED Widget			widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	if (data->set) {
		config.dst_flag = item;
		print_text_buttons();
		print_times();
	}
}


/*ARGSUSED*/
static void adjust_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	eval_menu_texts();
	print_text_buttons();
	print_times();
}
