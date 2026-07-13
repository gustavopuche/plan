/*
 * year overview menu widgets.
 *
 *	destroy_yov_menu()		remove yov menu
 *	create_yov_menu()		Create all widgets in yov calendar
 *					window.
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
#include <Xm/ArrowBP.h>
#include <Xm/ArrowBG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/Scale.h>
#include <Xm/Text.h>
#include <Xm/ScrolledW.h>
#include <Xm/DrawingA.h>
#include <Xm/Protocols.h>
#include "cal.h"

static void done_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void edit_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void sync_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void next_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void other_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void zoom_callback	(Widget, int, XmDrawingAreaCallbackStruct  *);
static void disp_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void single_callback	(Widget, int, XmToggleButtonCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern struct config	config;		/* global configuration data */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern Widget		mainwindow;	/* popup menus hang off main window */
extern int		curr_year;	/* year being displayed, since 1900 */
extern time_t		curr_yov;	/* year being displayed, in seconds */
extern struct week	yov;		/* info on yov view */
extern struct user	*user;		/* user list (from file_r.c) */
extern int		nusers;		/* number of users in array */

static Widget		w_disp[4];	/* 0=def, 1=all, 2=own, 3=yov_user */
static Widget		parent_form;	/* if in mainwindow, parent form */
static Widget		shell;		/* view window, or 0 if deinstalled */

					/* conv yov_nmonth <-> slider value */
/*			0  1  2  3  4  5  6  7  8  9 10 11 12 */
static int nm2sl[13] = {0, 0, 1, 2, 3, 3, 4, 4, 5, 5, 5, 5, 6};
static int sl2nm[7]  = {1, 2, 3, 4, 6, 8, 12};


/*
 * destroy the yov menu. Remove it from the screen, and destroy its widgets.
 */

void destroy_yov_menu(void)
{
	if (shell) {
		if (!parent_form)
			XtPopdown(shell);
		XTDESTROYWIDGET(shell);
		destroy_yov();
		shell = 0;
		yov.canvas = 0;
		drag_destroy('o');
		if (mainlist->modified)
			write_mainlist();
	}
}


/*
 * create the yov menu for the year (begiining at time <curr_yov>)
 */

static char *disptext[4] = { "Default", "All", "Own only", "%s" };

void create_yov_menu(
	Widget		parent)
{
	Arg		args[15];
	int		n, i;
	Widget		form, below, v, w;
	Atom		closewindow;
	char		text[256];
	struct tm	tm;

	tm.tm_year = curr_year;
	tm.tm_mon  = 0;
	tm.tm_mday = 1;
	tm.tm_hour = 0;
	tm.tm_min  = 0;
	tm.tm_sec  = 0;
	curr_yov = tm_to_time(&tm);

	build_yov(FALSE);

	form = create_view_form(&shell, parent_form = parent, 'o',
			yov.canvas_xs, yov.canvas_ys, _("Year Overview"));
	if (!form) {
		draw_yov_calendar(NULL);
		return;
	}
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"yov");

						/*---------- button form --*/
							/*-- next/prev --*/
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		60);			n++;
	v = XtCreateManagedWidget(_("Prev"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(v, XmNactivateCallback, (XtCallbackProc)
			next_callback, (XtPointer)(size_t)-1);
	XtAddCallback(v, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"yov_prev");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	v);			n++;
	XtSetArg(args[n], XmNleftOffset,	0);			n++;
	XtSetArg(args[n], XmNwidth,		60);			n++;
	v = XtCreateManagedWidget(_("Next"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(v, XmNactivateCallback, (XtCallbackProc)
			next_callback, (XtPointer)(size_t)1);
	XtAddCallback(v, XmNhelpCallback, (XtCallbackProc)
			help_callback,(XtPointer)"yov_next"); 

							/*-- sy,ed,hlp,done -*/
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		72);			n++;
	w = XtCreateManagedWidget(_("Done"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			done_callback, (XtPointer)(size_t)0);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback,(XtPointer)"yov_done"); 
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	0);			n++;
	XtSetArg(args[n], XmNwidth,		72);			n++;
	w = XtCreateManagedWidget(_("Help"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			help_callback, (XtPointer)"yov");
	XtAddCallback(w, XmNhelpCallback,  (XtCallbackProc)
			help_callback, (XtPointer)"yov");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	0);			n++;
	XtSetArg(args[n], XmNwidth,		72);			n++;
	w = XtCreateManagedWidget(_("Edit"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			edit_callback, (XtPointer)(size_t)0);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback,(XtPointer)"yov_edit");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	0);			n++;
	XtSetArg(args[n], XmNwidth,		72);			n++;
	XtSetArg(args[n], XmNsensitive,		!all_files_served());	n++;
	w = XtCreateManagedWidget(_("Sync"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			sync_callback, (XtPointer)(size_t)0);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"yov_sync");
	below = w;
							/*-- zoom slider --*/
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	10);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	v);			n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	v = XtCreateManagedWidget(_("Zoom in"), xmLabelWidgetClass,
			form, args, n);
	XtAddCallback(v, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"yov_zoom");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	10);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	16);			n++;
	w = XtCreateManagedWidget("out", xmLabelWidgetClass, form, args, n);
	XtAddCallback(v, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"yov_zoom");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	12);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	v);			n++;
	XtSetArg(args[n], XmNleftOffset,	4);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	4);			n++;
	XtSetArg(args[n], XmNminimum,		0);			n++;
	XtSetArg(args[n], XmNmaximum,		6);			n++;
	XtSetArg(args[n], XmNorientation,	XmHORIZONTAL);		n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNvalue,	nm2sl[config.yov_nmonths]);	n++;
	w = XmCreateScale(form, "zoom", args, n);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"yov_zoom");
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			zoom_callback, (XtPointer)(size_t)0);
	XtManageChild(w);

						/*---------- infotext form --*/
	n = 0;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	w);			n++;
	XtSetArg(args[n], XmNbottomOffset,	10);			n++;
	XtSetArg(args[n], XmNalignment,         XmALIGNMENT_BEGINNING);	n++;
#ifdef JAPAN
	XtSetArg(args[n], XmNlabelString,	XmStringCreateSimple(" "));	n++;
	yov.info = XtCreateManagedWidget("yovinfo", xmLabelWidgetClass,
			form, args, n);
#else
	yov.info = XtCreateManagedWidget(" ", xmLabelWidgetClass, form,args,n);
#endif

						/*---------- mode form --*/
							/*-- which --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		10);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	w = XtCreateManagedWidget(_("Display:"), xmLabelWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"yov_disp");

	for (i=0; i < 4; i++) {
	  sprintf(text, i < 3 ? _(disptext[i]) : "%s",
			 config.yov_user < nusers ? user[config.yov_user].name
						  : _("None"));
	  n = 0;
	  XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	  XtSetArg(args[n], XmNtopOffset,	10);			n++;
	  XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	  XtSetArg(args[n], XmNleftWidget,	w);			n++;
	  XtSetArg(args[n], XmNleftOffset,	8);			n++;
	  XtSetArg(args[n], XmNindicatorType,	XmONE_OF_MANY);		n++;
	  XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	  XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	  XtSetArg(args[n], XmNset,		i==config.yov_display);	n++;
	  w = w_disp[i] = XtCreateManagedWidget(text,
				xmToggleButtonWidgetClass, form, args, n);
	  XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
	  		disp_callback, (XtPointer)(size_t)i);
	  XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
	  		help_callback, (XtPointer)"yov_disp");
	}

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		64);			n++;
	w = XtCreateManagedWidget(_("Other"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			other_callback, (XtPointer)(size_t)0);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"yov_disp");
	below = w;
							/*-- single-day --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		10);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNset,		config.yov_single);	n++;
	w = XtCreateManagedWidget(_("Show single-day appointments"),
			xmToggleButtonWidgetClass, form, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			single_callback, (XtPointer)(size_t)0);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"day_single");

						/*---------- year overview --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		below);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	yov.info);		n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		config.yov_wwidth+4);	n++;
	XtSetArg(args[n], XmNheight,		config.yov_wheight+4);	n++;
	XtSetArg(args[n], XmNscrollingPolicy,	XmAUTOMATIC);		n++;
	yov.scroll = XtCreateWidget("scroll", xmScrolledWindowWidgetClass,
			form, args, n);

	n = 0;
	XtSetArg(args[n], XmNwidth,		yov.canvas_xs);	n++;
	XtSetArg(args[n], XmNheight,		yov.canvas_ys);	n++;
	yov.canvas = XtCreateWidget("yovcal", xmDrawingAreaWidgetClass,
							yov.scroll, args, n);
	drag_init('o', yov.canvas, yov.info, draw_yov_calendar,
					locate_in_yov_calendar);
	XtManageChild(yov.canvas);
	XtManageChild(yov.scroll);
	XtManageChild(form);

	if (!parent) {
		XtPopup(shell, XtGrabNone);
		closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
		XmAddWMProtocolCallback(shell, closewindow, (XtCallbackProc)
					done_callback, (XtPointer)shell);
	}
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * callbacks for bottom button row (callback for drawing area is handled by
 * xutil.c, and installed by drag_init()).
 */

/*ARGSUSED*/
static void next_callback(
	UNUSED Widget			widget,
	int				item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	struct tm			*tm;

	tm = time_to_tm(curr_yov);
	tm->tm_year += item;
	curr_yov = tm_to_time(tm);
	build_yov(FALSE);
	draw_yov_calendar(NULL);
}


/*ARGSUSED*/
static void done_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	if (parent_form)
		create_view('m');
	else
		destroy_yov_menu();
}


/*ARGSUSED*/
static void edit_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	struct tm *tm = time_to_tm(curr_yov);
	create_list_popup(mainlist, curr_yov,
				(tm->tm_year & 3 ? 365 : 366) * 86400,
				0, (struct entry *)0, 0, 0);
}


/*ARGSUSED*/
static void sync_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	if (!can_edit_appts())
		return;
	if (mainlist->modified) {
		write_mainlist();
		resynchronize_daemon();
	}
	read_mainlist();
	build_yov(FALSE);
	update_all_listmenus(TRUE);
	redraw_all_views();
}


/*ARGSUSED*/
static void zoom_callback(
	Widget				w,
	UNUSED int			data,
	UNUSED XmDrawingAreaCallbackStruct*info)
{
	XmScaleGetValue(w, &config.yov_nmonths);
	config.yov_nmonths = sl2nm[config.yov_nmonths];
	mainlist->modified = TRUE;
	build_yov(FALSE);
	draw_yov_calendar(NULL);
}


/*
 * callbacks for top button row
 */

/*ARGSUSED*/
static void single_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	XmToggleButtonCallbackStruct	*data)
{
	config.yov_single = data->set;
	mainlist->modified = TRUE;
	build_yov(FALSE);
	draw_yov_calendar(NULL);
}


/*ARGSUSED*/
static void disp_callback(
	UNUSED Widget			widget,
	int				item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	Arg				args;
	int				i;

	config.yov_display = item;
	mainlist->modified = TRUE;
	build_yov(FALSE);
	draw_yov_calendar(NULL);
	for (i=0; i < 4; i++) {
		XtSetArg(args, XmNset, i == config.yov_display);
		XtSetValues(w_disp[i], &args, 1);
	}
}


static void callback(
	int		newuser)
{
	if (newuser < 0)
		disp_callback(0, 2, 0);
	else {
		config.yov_user = newuser;
		print_button(w_disp[3], "%s",
			newuser < nusers ? user[newuser].name : _("None"));
		disp_callback(0, 3, 0);
	}
}


/*ARGSUSED*/
static void other_callback(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	create_user_sel_popup(widget, config.yov_user, (BOOL (*)())callback);
}
