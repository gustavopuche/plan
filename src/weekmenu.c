/*
 * week menu widgets.
 *
 *	destroy_week_menu()		remove week menu
 *	create_week_menu()		Create all widgets in week calendar
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
#include <Xm/Text.h>
#include <Xm/ScrolledW.h>
#include <Xm/DrawingA.h>
#include <Xm/Protocols.h>
#include "cal.h"

static void done_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void edit_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void sync_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void prev_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void next_callback	(Widget, int, XmToggleButtonCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern struct config	config;		/* global configuration data */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern Widget		mainwindow;	/* popup menus hang off main window */
extern time_t		curr_week;	/* week being displayed, time in sec */
extern struct week	week;		/* info on week view */

static Widget		parent_form;	/* if in mainwindow, parent form */
static Widget		shell;		/* week window, 0 if none */


/*
 * destroy the week menu. Remove it from the screen, and destroy its widgets.
 */

void destroy_week_menu(void)
{
	if (shell) {
		if (!parent_form)
			XtPopdown(shell);
		XTDESTROYWIDGET(shell);
		destroy_week();
		shell = 0;
		week.canvas = 0;
		drag_destroy('w');
	}
}


/*
 * create the week menu for the week beginning on curr_week.
 */

void create_week_menu(
	Widget		parent)
{
	Arg		args[15];
	int		n;
	Widget		form, w, w2;
	Atom		closewindow;

	build_week(FALSE, FALSE);

	form = create_view_form(&shell, parent_form = parent, 'w',
				week.canvas_xs, week.canvas_ys,_("Week View"));
	if (!form) {
		draw_week_calendar(NULL);
		return;
	}
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"week");

							/*-- buttons --*/
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		60);			n++;
	w = XtCreateManagedWidget(_("Prev"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			prev_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer) "week_prev");
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		60);			n++;
	w = XtCreateManagedWidget(_("Next"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			next_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer) "week_next");

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
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer) "week_done");
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Help"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			help_callback, (XtPointer)"week");
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer)"week");
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w2 = XtCreateManagedWidget(_("Edit"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			edit_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer) "week_edit");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	XtSetArg(args[n], XmNsensitive,		!all_files_served());	n++;
	w = XtCreateManagedWidget(_("Sync"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			sync_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer) "week_sync");

							/*--- infotext --- */
	n = 0;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	w2);			n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNalignment,         XmALIGNMENT_BEGINNING);	n++;
#ifdef JAPAN
	XtSetArg(args[n], XmNlabelString,	XmStringCreateSimple(" "));	n++;
	week.info = XtCreateManagedWidget("weekinfo", xmLabelWidgetClass,
			form, args, n);
#else
	week.info = XtCreateManagedWidget(" ", xmLabelWidgetClass,
			form, args, n);
#endif

							/*--- week view ---*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	week.info);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		week.canvas_xs +4);	n++;
	XtSetArg(args[n], XmNheight,		week.canvas_ys +4);	n++;
	XtSetArg(args[n], XmNscrollingPolicy,	XmAUTOMATIC);		n++;
	week.scroll = XtCreateWidget("scroll", xmScrolledWindowWidgetClass,
			form, args, n);

	n = 0;
	XtSetArg(args[n], XmNwidth,		week.canvas_xs);	n++;
	XtSetArg(args[n], XmNheight,		week.canvas_ys);	n++;
	week.canvas = XtCreateWidget("weekcal", xmDrawingAreaWidgetClass,
					week.scroll, args, n);
	drag_init('w', week.canvas, week.info, draw_week_calendar,
					locate_in_week_calendar);
	XtManageChild(week.canvas);
	XtManageChild(week.scroll);
	XtManageChild(form);

	if (!parent) {
		XtPopup(shell, XtGrabNone);
		closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
		XmAddWMProtocolCallback(shell, closewindow, (XtCallbackProc)
				done_callback, (XtPointer)shell);
	}
	/*
	resize_window(shell, week.scroll, week.canvas, 'w');
	*/
}


/*
 * resize a DrawingArea canvas, and resize the shell that contains it by
 * the same amount. Also, resize the scrolling widget around the canvas.
 * Make it a bit larger for the shadow.
 */

#if 0
static void resize_canvas(
	Widget		shell,		/* shell with the DrawingArea */
	Widget		scroll,		/* scrolling area around canvas */
	Widget		canvas,		/* DrawingArea */
	int		xs,		/* new DrawingArea size */
	int		ys)
{
	Dimension	sh_xs, sh_ys;	/* previous size of the shell */
	Dimension	can_xs, can_ys;	/* previous size of the canvas */

	if (parent_form)
		return;
	get_widget_size(shell,  &sh_xs,  &sh_ys);
	get_widget_size(canvas, &can_xs, &can_ys);
	set_widget_size(canvas, xs,      ys);
	set_widget_size(scroll, xs+4,    ys+4);
	set_widget_size(shell,  sh_xs - can_xs + xs,
				sh_ys - can_ys + ys);
}
#endif


/*-------------------------------------------------- callbacks --------------*/
/*
 * All of these routines are direct X callbacks.
 */

/*ARGSUSED*/
static void prev_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	curr_week -= 7*86400;
	build_week(FALSE, FALSE);
	draw_week_calendar(NULL);
}


/*ARGSUSED*/
static void next_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	curr_week += 7*86400;
	build_week(FALSE, FALSE);
	draw_week_calendar(NULL);
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
		destroy_week_menu();
}


/*ARGSUSED*/
static void edit_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	create_list_popup(mainlist, curr_week, config.week_ndays*86400,
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
	update_all_listmenus(TRUE);
	redraw_all_views();
}
