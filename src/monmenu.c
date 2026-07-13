/*
 * Create the widgets for a month calendar in the mainwindow. This code is
 * separate from calmenu.c because other views can also draw into mainwindow.
 * The calendar itself is drawn using Xlib calls. Attempts to do that with
 * Motif were spectacularly slow.
 *
 *	destroy_month_view()		Destroy month view, to make room for
 *					another view in the mainwindow
 *	create_month_menu(parent)	Create a form widget for the month
 *					view, and everything in it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/RowColumn.h>
#include <Xm/LabelP.h>
#include <Xm/ArrowBP.h>
#include <Xm/ArrowBG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/Text.h>
#include <Xm/ScrolledW.h>
#include <Xm/DrawingA.h>
#include "cal.h"

static void resize_mainwindow	(Widget, Dimension, Dimension);
static void month_down		(Widget, int, XmToggleButtonCallbackStruct *);
static void month_up		(Widget, int, XmToggleButtonCallbackStruct *);
static void year_down		(Widget, int, XmToggleButtonCallbackStruct *);
static void year_up		(Widget, int, XmToggleButtonCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern GC		gc;		/* everybody uses this context */
extern struct mainmenu	mainmenu;	/* all important main window widgets */
extern struct config	config;		/* global configuration data */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern int		curr_month;	/* month being displayed, 0..11 */
extern int		curr_year;	/* year being displayed, since 1900 */
extern Widget		mainwindow;	/* popup menus hang off main window */
extern Widget		menubar;	/* menu bar at the top of the window */
static Widget		all;		/* the form that contains everything */
static BOOL		have_view;	/* <all> is a valid widget */


/*
 * destroy the month view inside the mainwindow. Clear some widget pointers
 * to prevent print_button() from trying to print into them; for example,
 * the time widget gets the new time printed into it every minute otherwise.
 */

void destroy_month_view(void)
{
	if (have_view) {
		XTDESTROYWIDGET(all);
		mainmenu.month	= 0;
		mainmenu.year	= 0;
		mainmenu.time	= 0;
		mainmenu.cal	= 0;
		have_view = FALSE;
	}
}


/*
 * months are different from other views. Week and year views have two create
 * routines, for "menus" (an X window containing the view) and for just the
 * "view" (relying on someone else to create the window). The latter is used
 * when all views go into the mainwindow, the former is used when every view
 * has its own X window. Only months don't have their own window, they always
 * go into the mainwindow. Thus, we only have create/destroy view routines.
 */

void create_month_view(
	Widget		parent)
{
	Arg		args[15];
	int		n;
	Widget		arrow, scroll;
	int		sm = config.smallmonth;
	Dimension	draw_xs, draw_ys;
	Dimension	inset_xs, inset_ys;

	all = XtCreateWidget("all", xmFormWidgetClass, parent, NULL, 0);
	XtAddCallback(all, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"cal");

	draw_xs = 2 * config.calbox_marg [sm] +
		  1 * config.calbox_arrow[sm] +
		  7 * config.calbox_xs   [sm];
	draw_ys = 2 * config.calbox_marg [sm] +
		  1 * config.calbox_title[sm] +
		  6 * config.calbox_ys   [sm];
	inset_xs = draw_xs + (config.sgimode ? 6 : 4);
	inset_ys = draw_ys + (config.sgimode ? 6 : 4);
								/* time */
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	mainmenu.time = XtCreateManagedWidget("time", xmLabelWidgetClass,
			all, args, n);
	if (sm)
		print_button(mainmenu.time,
				mktimestring(get_time(), FALSE));
	else
		print_button(mainmenu.time, "%s   %s",
				mkdatestring(get_time()),
				mktimestring(get_time(), FALSE));

	n = 0;							/* year */
	XtSetArg(args[n], XmNarrowDirection,	XmARROW_RIGHT);		n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		10);			n++;
	XtSetArg(args[n], XmNrightOffset,	5);			n++;
	XtSetArg(args[n], XmNforeground,	color[COL_BACK]);	n++;
	XtSetArg(args[n], XmNshadowThickness,	config.sgimode?2:0);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	arrow = XtCreateManagedWidget("nexty", xmArrowButtonWidgetClass,
			all, args, n);
	XtAddCallback(arrow, XmNactivateCallback, (XtCallbackProc)
			year_up, (XtPointer)0);
	XtAddCallback(arrow, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"cal_year");

	n = 0;
	XtSetArg(args[n], XmNarrowDirection,	XmARROW_LEFT);		n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	arrow);			n++;
	XtSetArg(args[n], XmNtopOffset,		10);			n++;
	XtSetArg(args[n], XmNrightOffset,	5);			n++;
	XtSetArg(args[n], XmNshadowThickness,	config.sgimode?2:0);	n++;
	XtSetArg(args[n], XmNforeground,	color[COL_BACK]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	arrow = XtCreateManagedWidget("prevy", xmArrowButtonWidgetClass,
			all, args, n);
	XtAddCallback(arrow, XmNactivateCallback, (XtCallbackProc)
			year_down, (XtPointer)0);
	XtAddCallback(arrow, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"cal_year");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	arrow);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNrightOffset,	4);			n++;
	mainmenu.year = XtCreateManagedWidget("year", xmLabelWidgetClass,
			all, args, n);
	XtAddCallback(mainmenu.year, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"cal_year");

	n = 0;							/* month */
	XtSetArg(args[n], XmNarrowDirection,	XmARROW_RIGHT);		n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	mainmenu.year);		n++;
	XtSetArg(args[n], XmNtopOffset,		10);			n++;
	XtSetArg(args[n], XmNrightOffset,	5);			n++;
	XtSetArg(args[n], XmNforeground,	color[COL_BACK]);	n++;
	XtSetArg(args[n], XmNshadowThickness,	config.sgimode?2:0);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	arrow = XtCreateManagedWidget("nextm", xmArrowButtonWidgetClass,
			all, args, n);
	XtAddCallback(arrow, XmNactivateCallback, (XtCallbackProc)
			month_up, (XtPointer)0);
	XtAddCallback(arrow, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"cal_month");

	n = 0;
	XtSetArg(args[n], XmNarrowDirection,	XmARROW_LEFT);		n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	arrow);			n++;
	XtSetArg(args[n], XmNtopOffset,		10);			n++;
	XtSetArg(args[n], XmNrightOffset,	5);			n++;
	XtSetArg(args[n], XmNforeground,	color[COL_BACK]);	n++;
	XtSetArg(args[n], XmNshadowThickness,	config.sgimode?2:0);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	arrow = XtCreateManagedWidget("prevm", xmArrowButtonWidgetClass,
			all, args, n);
	XtAddCallback(arrow, XmNactivateCallback, (XtCallbackProc)
			month_down, (XtPointer)0);
	XtAddCallback(arrow, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"cal_month");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
#if LesstifVersion >= 88
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	mainmenu.time);		n++;
#endif
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	arrow);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNrightOffset,	5);			n++;
	XtSetArg(args[n], XmNalignment,		XmALIGNMENT_END);	n++;
	mainmenu.month = XtCreateManagedWidget("month", xmLabelWidgetClass,
			all, args, n);
	XtAddCallback(mainmenu.month, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"cal_month");

	n = 0;							/* calendar */
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		mainmenu.month);	n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		inset_xs);		n++;
	XtSetArg(args[n], XmNheight,		inset_ys);		n++;
	XtSetArg(args[n], XmNscrollingPolicy,	XmAUTOMATIC);		n++;
	scroll = XtCreateWidget("scroll", xmScrolledWindowWidgetClass,
			all, args, n);

	n = 0;
	XtSetArg(args[n], XmNwidth,		draw_xs);		n++;
	XtSetArg(args[n], XmNheight,		draw_ys);		n++;
	mainmenu.cal = XtCreateWidget("cal", xmDrawingAreaWidgetClass,
			scroll, args, n);
	XtAddCallback(mainmenu.cal, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"cal");
	drag_init('m', mainmenu.cal, 0, draw_calendar,
					locate_in_month_calendar);
	XtManageChild(mainmenu.cal);
	XtManageChild(scroll);
	XtManageChild(all);

	draw_month_year();	/* fill in current month and year */
	resize_mainwindow(parent, inset_xs, inset_ys);

	have_view = TRUE;
}


/*
 * resize the main window. When this is called for the first time, just
 * record the window size. Any future call applies the change to the size
 * of the calendar to the window based on the original size.
 */

static void resize_mainwindow(
	Widget		 win,		/* mainwindow */
	Dimension	 xs,		/* space needed for calendar */
	Dimension	 ys)
{
	Widget		 window;
	Dimension	 win_xs,  win_ys;
	static Dimension last_xs, last_ys;

	for (window=win; XtParent(window); window=XtParent(window));
	if (last_xs && xs != last_xs) {
		get_widget_size(XtParent(win), &win_xs, &win_ys);
		set_widget_size(XtParent(win), win_xs - last_xs + xs,
					       win_ys - last_ys + ys);
		set_widget_size(window, win_xs - last_xs + xs,
					win_ys - last_ys + ys);
	}
	last_xs = xs;
	last_ys = ys;
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * the previous or next month or year arrow button was pressed. Switch the
 * calendar, and make sure we stay in the range 1900..2037. All of these
 * routines are direct X callbacks.
 */

/*ARGSUSED*/
static void month_down(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	if (curr_month <= 0) {
		if (curr_year <= 0)
			return;
		curr_month = 12;
		curr_year--;
	}
	curr_month--;
	draw_month_year();
	draw_calendar(NULL);
	if (curr_month == 11)
		draw_year_calendar(NULL);
}

/*ARGSUSED*/
static void month_up(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	if (curr_month >= 11) {
		if (curr_year >= 2037)
			return;
		curr_month = -1;
		curr_year++;
	}
	curr_month++;
	draw_month_year();
	draw_calendar(NULL);
	if (curr_month == 0)
		draw_year_calendar(NULL);
}

/*ARGSUSED*/
static void year_down(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	if (curr_year <= 70)
		return;
	curr_year--;
	draw_month_year();
	draw_calendar(NULL);
	draw_year_calendar(NULL);
}

/*ARGSUSED*/
static void year_up(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	if (curr_year >= 137)
		return;
	curr_year++;
	draw_month_year();
	draw_calendar(NULL);
	draw_year_calendar(NULL);
}
