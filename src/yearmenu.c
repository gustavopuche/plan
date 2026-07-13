/*
 * year menu widgets.
 *
 *	destroy_year_menu()		Destroy the year window an its
 *					contents
 *	create_year_menu()		Create a year calendar for curr_year
 *					and put a year view into it
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
static void year_cal_callback	(Widget, int, XmDrawingAreaCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern GC		gc;		/* everybody uses this context */
extern Widget		toplevel;	/* top-level shell */
extern struct mainmenu	mainmenu;	/* all important window widgets */
extern Widget		mainwindow;	/* popup menus hang off main window */
extern struct config	config;		/* global configuration data */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern Pixel		color[NCOLS];	/* colors: COL_* */

static Widget		shell;		/* view window, or 0 if deinstalled */
static Widget		parent_form;	/* if in mainwindow, parent form */


/*
 * destroy the year menu. Remove it from the screen, and destroy its widgets.
 * It's too much trouble to keep them for next time.
 */

void destroy_year_menu(void)
{
	if (shell) {
		if (!parent_form)
			XtPopdown(shell);
		XTDESTROYWIDGET(shell);
		shell = 0;
		mainmenu.yearcal = 0;
	}
}


/*
 * create the year menu.
 * There should be buttons for Quit, next/prev year, next/prev 6 months.
 */

void create_year_menu(
	Widget		parent)
{
	Arg		args[15];
	int		n;
	Atom		closewindow;
	Widget		form, scroll;
	Dimension	xs, ys;

	xs = 2*config.year_margin+ 4*config.yearbox_xs*7+ 3*config.year_gap +4;
	ys = 2*config.year_margin+ 1*config.year_title +
				   3*config.yearbox_ys*8+ 3*config.year_gap +4;

	form = create_view_form(&shell, parent_form = parent, 'y',
						xs, ys, _("Year View"));
	if (!form) {
		draw_year_calendar(NULL);
		return;
	}
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"year");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		xs);			n++;
	XtSetArg(args[n], XmNheight,		ys);			n++;
	XtSetArg(args[n], XmNscrollingPolicy,	XmAUTOMATIC);		n++;
	scroll = XtCreateWidget("scroll", xmScrolledWindowWidgetClass,
			form, args, n);

	n = 0;
	XtSetArg(args[n], XmNwidth,		xs-4);			n++;
	XtSetArg(args[n], XmNheight,		ys-4);			n++;
	mainmenu.yearcal = XtCreateWidget("yearcal", xmDrawingAreaWidgetClass,
					scroll, args, n);
	XtAddCallback(mainmenu.yearcal, XmNinputCallback, (XtCallbackProc)
					year_cal_callback, (XtPointer)NULL);
	XtAddCallback(mainmenu.yearcal, XmNexposeCallback, (XtCallbackProc)
					year_cal_callback, (XtPointer)NULL);
	XtManageChild(mainmenu.yearcal);
	XtManageChild(scroll);
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
 * All of these routines are direct X callbacks.
 */

/*ARGSUSED*/
static void done_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	if (parent_form)
		create_view('m');
	else
		destroy_year_menu();
}


/*ARGSUSED*/
static void year_cal_callback(
	UNUSED Widget			w,
	UNUSED int			data,
	XmDrawingAreaCallbackStruct	*info)
{
	int				view;

	if (info->reason == XmCR_INPUT && info->event->xany.type==ButtonPress){
		if ((view = clicked_year_calendar(info->event->xbutton.x,
						  info->event->xbutton.y))) {
			if (parent_form)
				create_view(view == 'Q' ? 'm' : view);
			if (view == 'Q')
				destroy_year_menu();
		}
	} else if (info->reason == XmCR_EXPOSE) {
		XEvent dummy;
		Region region = XCreateRegion();
		XtAddExposureToRegion(info->event, region);
		while (XCheckWindowEvent(display, info->window, ExposureMask,
								&dummy))
			XtAddExposureToRegion(&dummy, region);
		draw_year_calendar(region);
	}
}
