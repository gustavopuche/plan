/*
 * Create the main calendar window and everything in it. That isn't all that
 * much because the calendar itself is drawn using Xlib calls. Attempts to
 * do that with Motif were spectacularly slow.
 *
 *	create_cal_widgets(toplevel)	Create all widgets in main calendar
 *					window.
 *	create_view(mode)		show a new view (month, week, year,...)
 *	create_view_form(s,p,v,x,y,n)	callback, create shell for new view
 *	redraw_all_views()		redraw all views
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
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/Protocols.h>
#include <X11/cursorfont.h>
#include "cal.h"

void	    create_view		(int);
static void filemenu_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void delpast_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void configmenu_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void language_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void search_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void view_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void help_pulldown	(Widget, int, XmToggleButtonCallbackStruct *);
static void list_by_user	(int);

extern Display		*display;	/* everybody uses the same server */
extern Widget		toplevel;	/* top-level shell for icon name */
extern GC		gc;		/* everybody uses this context */
extern struct config	config;		/* global configuration data */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern struct mainmenu	mainmenu;	/* all important main window widgets */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern int		curr_month;	/* month being displayed, 0..11 */
extern int		curr_year;	/* year being displayed, since 1900 */
extern time_t		curr_week;	/* week being displayed, time in sec */
extern time_t		curr_day;	/* day being displayed, time in sec */
Widget			mainwindow;	/* popup menus hang off main window */
Widget			menubar;	/* menu bar at the top of main window*/
Widget			mainview;	/* area in main window below menubar */

#ifdef FIXMBAR
#define FIX_MENUBAR XmVaSEPARATOR,
#else
#define FIX_MENUBAR
#endif

#ifdef NOMSEP
#define XM_VA_SEPARATOR
#else
#define XM_VA_SEPARATOR XmVaSEPARATOR,
#endif

#define MAXLANGUAGES 20
static char *language_name[MAXLANGUAGES];


/*
 * create the main calendar window with the month view, including the menu
 * bar and all its pulldowns.
 */

void create_cal_widgets(
	Widget		toplevel)
{
	Arg		args[4];
	Widget		menu, fmenu, w;
	XmString	s[MAXLANGUAGES+1];
	int		n;
	char		*p;

	XtSetArg(*args, XmNallowShellResize, TRUE);
	mainwindow = XtCreateManagedWidget("mainwindow",
			xmMainWindowWidgetClass, toplevel, args, 1);

	/*------------------------------------- menu bar -------------------*/
	s[0] = XmStringCreateSimple(_("File"));
	s[1] = XmStringCreateSimple(_("Config"));
	s[2] = XmStringCreateSimple(_("Search"));
	s[3] = XmStringCreateSimple(_("View"));
	s[4] = XmStringCreateSimple(_("Help"));
	menubar = XmVaCreateSimpleMenuBar(mainwindow, "menubar",
			FIX_MENUBAR
			XmVaCASCADEBUTTON, s[0], 'F',
			XmVaCASCADEBUTTON, s[1], 'C',
			XmVaCASCADEBUTTON, s[2], 'S',
			XmVaCASCADEBUTTON, s[3], 'V',
			XmVaCASCADEBUTTON, s[4], 'H',
			NULL);
	if ((w = XtNameToWidget(menubar, "button_4")))
		XtVaSetValues(menubar, XmNmenuHelpWidget, w, 0, NULL);
	for (n=0; n < 5; n++)
		XmStringFree(s[n]);

	s[0] = XmStringCreateSimple(_("File list..."));
	s[1] = XmStringCreateSimple(_("Reread files"));
	s[2] = XmStringCreateSimple(_("Delete past entries"));
	s[3] = XmStringCreateSimple(_("Print..."));
	s[4] = XmStringCreateSimple(_("About..."));
	s[5] = XmStringCreateSimple(_("Quit"));
	s[6] = XmStringCreateSimple("Ctrl-L");
	s[7] = XmStringCreateSimple("Ctrl-P");
	s[8] = XmStringCreateSimple("Ctrl-Q");
	fmenu = XmVaCreateSimplePulldownMenu(menubar, "file", 0,
			(XtCallbackProc)filemenu_callback,
			FIX_MENUBAR
			XmVaPUSHBUTTON,    s[0], 'F', "Ctrl<Key>L", s[6],
			XmVaPUSHBUTTON,    s[1], 'R', NULL, NULL,
			XmVaCASCADEBUTTON, s[2], 'D',
			XmVaPUSHBUTTON,    s[3], 'P', "Ctrl<Key>P", s[7],
			XmVaPUSHBUTTON,    s[4], 'A', NULL, NULL,
			XmVaPUSHBUTTON,    s[5], 'Q', "Ctrl<Key>Q", s[8],
			NULL);
	XtAddCallback(fmenu, XmNhelpCallback, (XtCallbackProc)help_callback,
			(XtPointer)"pd_file");
	for (n=0; n < 9; n++)
		XmStringFree(s[n]);

	s[0] = XmStringCreateSimple(_("older than one day"));
	s[1] = XmStringCreateSimple(_("older than one week"));
	s[2] = XmStringCreateSimple(_("older than one month"));
	s[3] = XmStringCreateSimple(_("older than 90 days"));
	s[4] = XmStringCreateSimple(_("older than one year"));
	fmenu = XmVaCreateSimplePulldownMenu(fmenu, "delpast", 2,
			(XtCallbackProc)delpast_callback,
			FIX_MENUBAR
			XmVaPUSHBUTTON,    s[0], 'd',  NULL, NULL,
			XmVaPUSHBUTTON,    s[1], 'w',  NULL, NULL,
			XmVaPUSHBUTTON,    s[2], 'm',  NULL, NULL,
			XmVaPUSHBUTTON,    s[3], '9',  NULL, NULL,
			XmVaPUSHBUTTON,    s[4], 'y',  NULL, NULL,
			NULL);
	XtAddCallback(fmenu, XmNhelpCallback, (XtCallbackProc)help_callback,
			(XtPointer)"pd_file");
	for (n=0; n < 5; n++)
		XmStringFree(s[n]);

	s[0] = XmStringCreateSimple(_("Calendar views..."));
	s[1] = XmStringCreateSimple(_("Adjust time..."));
	s[2] = XmStringCreateSimple(_("Alarm options..."));
	s[3] = XmStringCreateSimple(_("Define holidays..."));
	s[4] = XmStringCreateSimple(_("Language"));
	s[5] = XmStringCreateSimple("Ctrl-C");
	menu = XmVaCreateSimplePulldownMenu(menubar, "config", 1,
			(XtCallbackProc)configmenu_callback,
			FIX_MENUBAR
			XmVaPUSHBUTTON,    s[0], 'C', "Ctrl<Key>C", s[5],
			XmVaPUSHBUTTON,    s[1], 't', NULL, NULL,
			XmVaPUSHBUTTON,    s[2], 'o', NULL, NULL,
			XmVaPUSHBUTTON,    s[3], 'D', NULL, NULL,
			XmVaCASCADEBUTTON, s[4], 'L',
			NULL);
	XtAddCallback(menu, XmNhelpCallback, (XtCallbackProc)help_callback,
			(XtPointer)"pd_config");
	for (n=0; n < 6; n++)
		XmStringFree(s[n]);

	menu = XmVaCreateSimplePulldownMenu(menu, "langpd", 4,
			(XtCallbackProc)language_callback, FIX_MENUBAR NULL);
	XtAddCallback(menu, XmNhelpCallback, (XtCallbackProc)help_callback,
			(XtPointer)"pd_lang");
	for (n=0; n < MAXLANGUAGES; n++) {
		if (!(language_name[n] = mystrdup(p = get_language_name(n))))
			break;
		w = XtCreateManagedWidget(p, xmPushButtonWidgetClass,menu,0,0);
		XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			(XtCallbackProc)language_callback,
			(XtPointer)(size_t)n);
	}

	s[ 0] = XmStringCreateSimple(_("Today"));
	s[ 1] = XmStringCreateSimple(_("Tomorrow"));
	s[ 2] = XmStringCreateSimple(_("This week"));
	s[ 3] = XmStringCreateSimple(_("Next week"));
	s[ 4] = XmStringCreateSimple(_("This month"));
	s[ 5] = XmStringCreateSimple(_("All"));
	s[ 6] = XmStringCreateSimple(_("Private"));
	s[ 7] = XmStringCreateSimple(_("One file..."));
	s[ 8] = XmStringCreateSimple(_("Search keywords..."));
	s[ 9] = XmStringCreateSimple("Ctrl-E");
	s[10] = XmStringCreateSimple("Ctrl-I");
	s[11] = XmStringCreateSimple("Ctrl-J");
	s[12] = XmStringCreateSimple("Ctrl-S");
	menu = XmVaCreateSimplePulldownMenu(menubar, "search", 2,
			(XtCallbackProc)search_callback,
			FIX_MENUBAR
			XmVaPUSHBUTTON, s[0], 'T', "Ctrl<Key>E", s[9],
			XmVaPUSHBUTTON, s[1], 'o', "Ctrl<Key>I", s[10],
			XmVaPUSHBUTTON, s[2], 'w', "Ctrl<Key>J", s[11],
			XmVaPUSHBUTTON, s[3], 'N', NULL, NULL,
			XmVaPUSHBUTTON, s[4], 'm', NULL, NULL,
			XmVaPUSHBUTTON, s[5], 'A', NULL, NULL,
			XmVaPUSHBUTTON, s[6], 'P', NULL, NULL,
			XmVaPUSHBUTTON, s[7], 'f', NULL, NULL,
			XmVaPUSHBUTTON, s[8], 'S', "Ctrl<Key>S", s[12],
			NULL);
	XtAddCallback(menu, XmNhelpCallback, (XtCallbackProc)help_callback,
			(XtPointer)"pd_search");
	for (n=0; n < 13; n++)
		XmStringFree(s[n]);

	s[ 0] = XmStringCreateSimple(_("Day"));
	s[ 1] = XmStringCreateSimple(_("Week"));
	s[ 2] = XmStringCreateSimple(_("Small Month"));
	s[ 3] = XmStringCreateSimple(_("Large Month"));
	s[ 4] = XmStringCreateSimple(_("Year"));
	s[ 5] = XmStringCreateSimple(_("Year Overview"));
	s[ 6] = XmStringCreateSimple(_("Goto today"));
	s[ 7] = XmStringCreateSimple(_("Goto..."));
	s[ 8] = XmStringCreateSimple("Ctrl-D");
	s[ 9] = XmStringCreateSimple("Ctrl-W");
	s[10] = XmStringCreateSimple("Ctrl-N");
	s[11] = XmStringCreateSimple("Ctrl-M");
	s[12] = XmStringCreateSimple("Ctrl-Y");
	s[13] = XmStringCreateSimple("Ctrl-O");
	s[14] = XmStringCreateSimple("Ctrl-T");
	s[15] = XmStringCreateSimple("Ctrl-G");
	menu = XmVaCreateSimplePulldownMenu(menubar, "view", 3,
			(XtCallbackProc)view_callback,
			FIX_MENUBAR
			XmVaPUSHBUTTON, s[0], 'D', "Ctrl<Key>D", s[8],
			XmVaPUSHBUTTON, s[1], 'W', "Ctrl<Key>W", s[9],
			XmVaPUSHBUTTON, s[2], 'S', "Ctrl<Key>N", s[10],
			XmVaPUSHBUTTON, s[3], 'M', "Ctrl<Key>M", s[11],
			XmVaPUSHBUTTON, s[4], 'Y', "Ctrl<Key>Y", s[12],
			XmVaPUSHBUTTON, s[5], 'O', "Ctrl<Key>O", s[13],
			XM_VA_SEPARATOR
			XmVaPUSHBUTTON, s[6], 't', "Ctrl<Key>T", s[14],
			XmVaPUSHBUTTON, s[7], 'G', "Ctrl<Key>G", s[15],
			NULL);
	XtAddCallback(menu, XmNhelpCallback, (XtCallbackProc)help_callback,
			(XtPointer)"pd_view");
	for (n=0; n < 16; n++)
		XmStringFree(s[n]);

	s[0] = XmStringCreateSimple(_("On context"));
	s[1] = XmStringCreateSimple(_("Introduction"));
	s[2] = XmStringCreateSimple(_("Getting help"));
	s[3] = XmStringCreateSimple(_("Troubleshooting"));
	s[4] = XmStringCreateSimple(_("Files and programs"));
	s[5] = XmStringCreateSimple(_("Networking"));
	s[6] = XmStringCreateSimple(_("Database access"));
	s[7] = XmStringCreateSimple(_("X resources"));
	s[8] = XmStringCreateSimple(_("Languages"));
	s[9] = XmStringCreateSimple("Ctrl-H");
	(void)XmVaCreateSimplePulldownMenu(menubar, "help", 4,
			(XtCallbackProc)help_pulldown,
			FIX_MENUBAR
			XmVaPUSHBUTTON, s[0], 'c', "Ctrl<Key>H", s[9],
			XM_VA_SEPARATOR
			XmVaPUSHBUTTON, s[1], 'I', NULL, NULL,
			XmVaPUSHBUTTON, s[2], 'G', NULL, NULL,
			XmVaPUSHBUTTON, s[3], 'T', NULL, NULL,
			XmVaPUSHBUTTON, s[4], 'F', NULL, NULL,
			XmVaPUSHBUTTON, s[5], 'N', NULL, NULL,
			XmVaPUSHBUTTON, s[6], 'D', NULL, NULL,
			XmVaPUSHBUTTON, s[7], 'X', NULL, NULL,
			XmVaPUSHBUTTON, s[8], 'L', NULL, NULL,
			NULL);

	for (n=0; n < 9; n++)
		XmStringFree(s[n]);

	XtManageChild(menubar);
	mainview = XtCreateManagedWidget("all", xmFormWidgetClass,
							mainwindow, NULL, 0);
	create_view('m');
}


/*-------------------------------------------------- view switching ---------*/
/*
 * put a view into the main or separate window: m=month, w=week, y=year,
 * o=year overview. If one-window mode is off, this is called only for the
 * regular and small month views. It is also called to redraw; if the new
 * view is the same as the old one don't destroy, just re-create. The create
 * calls first calculate the size they need and then call create_view_form
 * to set up windows, shells, and a toplevel form.
 */

void create_view(
	int		view)		/* m, w, y, o, d */
{
	static int	lastview = 'm';	/* previous view to destroy first */

	if (!config.share_mainwin) {
		switch(view) {
		  case 'm':	destroy_month_view();
				create_month_view(mainview);	break;
		  case 'w':	create_week_menu(0);		break;
		  case 'y':	create_year_menu(0);		break;
		  case 'o':	create_yov_menu(0);		break;
		  case 'd':	create_day_menu(0);		break;
		}
	} else {
		if (view != lastview) switch(lastview) {
		  case 'm':	destroy_month_view();		break;
		  case 'w':	destroy_week_menu();		break;
		  case 'y':	destroy_year_menu();		break;
		  case 'o':	destroy_yov_menu();		break;
		  case 'd':	destroy_day_menu();		break;
		}
		switch(lastview = view) {
		  case 'm':	create_month_view(mainview);	break;
		  case 'w':	create_week_menu(mainview);	break;
		  case 'y':	create_year_menu(mainview);	break;
		  case 'o':	create_yov_menu(mainview);	break;
		  case 'd':	create_day_menu(mainview);	break;
		}
	}
}


/*
 * when create_view calls one of the create functions, the create funtion
 * calculates the size it needs and calls the following function. It creates
 * a window if necessary (share_mainwin is off and it's not a month view),
 * and resizes if enabled. Returns a form to draw into, or 0 if a shell
 * exists and it's sufficient to redraw.
 */

Widget create_view_form(
	Widget		*shell,		/* if shell exists, widget, else &0 */
	Widget		parent,		/* parent widget (mainview) if share */
	UNUSED int	view,		/* m, w, y, o, d */
	UNUSED int	xs,		/* requested form width */
	UNUSED int	ys,		/* requested form height */
	char		*name)		/* window name */
{
	Widget		form;		/* form to create to draw view into */
	Dimension	x, y;
	Arg		args[4];

	if (*shell) {
		if (!parent) {
			/*
			resize_canvas(shell, week.scroll, week.canvas, xs, ys);
			*/
			XtPopup(*shell, XtGrabNone);
			/*
			XRaiseWindow(display, XtWindow(*shell));
			*/
		}
		return(0);
	}
	if (parent) {
		XtSetArg(args[0], XmNtopAttachment,	XmATTACH_FORM);
		XtSetArg(args[1], XmNbottomAttachment,	XmATTACH_FORM);
		XtSetArg(args[2], XmNleftAttachment,	XmATTACH_FORM);
		XtSetArg(args[3], XmNrightAttachment,	XmATTACH_FORM);
		*shell = form = XtCreateWidget("viewform", xmFormWidgetClass,
							parent, args, 4);
		get_widget_size(parent, &x, &y);
		set_widget_size(*shell,  x,  y);
	} else {
		XtSetArg(args[0], XmNdeleteResponse,	XmDO_NOTHING);
		XtSetArg(args[1], XmNiconic,		False);
		*shell = XtCreatePopupShell(name, applicationShellWidgetClass,
					    toplevel, args, 2);
#		ifdef EDITRES
		XtAddEventHandler(*shell, (EventMask)0, TRUE, 
				_XEditResCheckMessages, NULL);
#		endif
		set_icon(*shell, 0);
		form = XtCreateWidget("viewform", xmFormWidgetClass,
				*shell, NULL, 0);
	}
	return(form);
}


/*
 * resize window such that the given shell fits exactly.
 * The shell is the form that all window contents are drawn into. In the main
 * window, its parent is a slightly bigger form that also contains the menubar,
 * and that one's parent is <toplevel>. Resizing <toplevel> resizes the window.
 * Windows other than the main window don't have extra forms for menubars etc,
 * <shell> is the window widget directly.
 * The trick is to resize the scroll area to accomodate the form (the scrolled
 * white drawing area) and also resize the shell and the toplevel, but leaving
 * the deltas from scroll to shell and toplevel unchanged because we don't know
 * the space requirements of the buttons that live in the delta.
 */

#if 0
static Dimension	shell_dx[10];	/* overhead required by shell (extra */
static Dimension	shell_dy[10];	/* buttons etc) and the window (menu */
static Dimension	window_dx[10];	/* bar) in addition to the scrolling */
static Dimension	window_dy[10];	/* area. Set up when first used. */

resize_window(
	Widget		shell,		/* window shell to resize */
	Widget		scroll,		/* scrolling area, will fit form */
	Widget		form,		/* form inside scrolling area */
	int		view)		/* m, w, y, o, d */
{
	int		i;		/* shell/window_dx/y index */
	Widget		window, scrollp;
	Dimension	xc, yc, xp, yp, xf, yf, xw, yw, xh, yh;

	for (window=shell; XtParent(window); window=XtParent(window));
	scrollp = XtParent(scroll);
	if (view == 'm' && config.smallmonth)
		view = 's';
	for (i=0; i < 6; i++)
		if (view == "smwyod"[i])
			break;
	get_widget_size(form,    &xf, &yf);
	get_widget_size(scroll,  &xc, &yc);
	get_widget_size(scrollp, &xp, &yp);
	get_widget_size(shell,   &xh, &yh);
	get_widget_size(window,  &xw, &yw);

	printf("---- %c ----\n", view);
	printf("window: %d %d\n", xw, yw);
	printf("shell:  %d %d\n", xh, yh);
	printf("sc_par: %d %d\n", xp, yp);
	printf("scroll: %d %d\n", xc, yc);
	printf("form:   %d %d\n\n", xf, yf);

	if (!xp)
		return;
	if (!shell_dx[i]) {
		shell_dx[i] = xh - xc;
		shell_dy[i] = yh - yc;
	}
	if (!window_dx[i]) {
		window_dx[i] = xw - xh;
		window_dy[i] = yw - yh;
	}
	printf("d_win:  %d %d\n", window_dx[i], window_dy[i]);
	printf("d_sh:   %d %d\n", shell_dx[i], shell_dy[i]);
	set_widget_size(scroll,  xc=xf+4, xc=yf+4);
	set_widget_size(scrollp, xp=xc+shell_dx[i], yp=xc+shell_dy[i]);
	set_widget_size(shell,   xh=xp, yh=yp);
	set_widget_size(window,  xw=xh+window_dx[i], yw=yh+window_dy[i]);
	printf("window: %d %d\n", xw, yw);
	printf("shell:  %d %d\n", xh, yh);
	printf("sc_par: %d %d\n\n", xp, yp);
	printf("scroll: %d %d\n", xc, yc);

	get_widget_size(form,    &xf, &yf);
	get_widget_size(scroll,  &xc, &yc);
	get_widget_size(scrollp, &xp, &yp);
	get_widget_size(shell,   &xh, &yh);
	get_widget_size(window,  &xw, &yw);
	printf("window: %d %d\n", xw, yw);
	printf("shell:  %d %d\n", xh, yh);
	printf("sc_par: %d %d\n", xp, yp);
	printf("scroll: %d %d\n", xc, yc);
	printf("form:   %d %d\n\n", xf, yf);
}
#endif


/*
 * resize the main window. When this is called for the first time, just
 * record the window size. Any future call applies the change to the size
 * of the calendar to the window based on the original size. Not used.
 */

#if 0
static void resize_mainwindow(
	Widget		 win,		/* mainwindow */
	Dimension	 xs,		/* space needed for calendar */
	Dimension	 ys)
{
	Arg		 args[2];
	Dimension	 win_xs,  win_ys;
	static Dimension last_xs, last_ys;

	if (last_xs && xs != last_xs) {
		XtSetArg(args[0], XmNwidth,  &win_xs);
		XtSetArg(args[1], XmNheight, &win_ys);
		XtGetValues(XtParent(win), args, 2);
		XtSetArg(args[0], XmNwidth,  win_xs - last_xs + xs);
		XtSetArg(args[1], XmNheight, win_ys - last_ys + ys);
		XtSetValues(XtParent(win), args, 2);
	}
	last_xs = xs;
	last_ys = ys;
}
#endif


/*
 * lots of places change the database and need to redraw all views to show
 * the changed data. It's easier to do it all in one place.
 */

void redraw_all_views(void)
{
	build_day(FALSE, FALSE);
	build_week(FALSE, FALSE);
	build_yov(FALSE);
	draw_calendar(NULL);
	draw_yov_calendar(NULL);
	draw_day_calendar(NULL);
	draw_week_calendar(NULL);
	draw_year_calendar(NULL);
}


/*
 * get and set widget sizes
 */

void get_widget_size(
	Widget		w,
	Dimension	*xs,
	Dimension	*ys)
{
	Arg		args[2];

	XtSetArg(args[0], XmNwidth,  xs);
	XtSetArg(args[1], XmNheight, ys);
	XtGetValues(w, args, 2);
}


void set_widget_size(
	Widget		w,
	Dimension	xs,
	Dimension	ys)
{
	Arg		args[2];

	XtSetArg(args[0], XmNwidth,  xs);
	XtSetArg(args[1], XmNheight, ys);
	XtSetValues(w, args, 2);
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * some item in one of the menu bar pulldowns was pressed. All of these
 * routines are direct X callbacks.
 */

/*ARGSUSED*/
static void filemenu_callback(
	UNUSED Widget			widget,
	int				item,
	UNUSED XmToggleButtonCallbackStruct *data)
{
	switch (item) {
	  case 0:						/* file list */
		create_user_popup();
		break;

	  case 1: {						/* reread */
		char *errmsg;
		if ((errmsg = parse_holidays(curr_year, FALSE)))
			create_error_popup(mainmenu.cal, 0, errmsg);
		if (can_edit_appts()) {
			read_mainlist();
			update_all_listmenus(TRUE);
			redraw_all_views();
		}
		break; }

	  case 3:						/* print */
		if (can_edit_appts())
			create_print_popup();
		break;

	  case 4:						/* about */
		create_about_popup();
		break;

	  case 5:						/* quit */
		exit(0);
	}
}

static int period[5] = {1, 7, 31, 90, 366};
/*ARGSUSED*/
static void delpast_callback(
	UNUSED Widget			widget,
	int				item,
	UNUSED XmToggleButtonCallbackStruct *data)
{
	if (can_edit_appts())
		if (item < 5 && recycle_all(mainlist, TRUE, period[item])) {
			update_all_listmenus(TRUE);
			redraw_all_views();
		}
}

/*ARGSUSED*/
static void configmenu_callback(
	UNUSED Widget			widget,
	int				item,
	UNUSED XmToggleButtonCallbackStruct *data)
{
	if (!can_edit_appts())
		return;
	switch (item) {
	  case 0:						/* calendars */
		create_calconfig_popup();
		break;
	  case 1:						/* time adj */
		create_adjust_popup();
		break;
	  case 2:						/* alarms */
		create_config_popup();
		break;
	  case 3:						/* def hlday */
		create_holiday_popup();
		break;
	}
}

/*ARGSUSED*/
static void language_callback(
	Widget				widget,
	int				item,
	UNUSED XmToggleButtonCallbackStruct *data)
{
	if (config.language)
		free(config.language);
	config.language = !strcmp(language_name[item], "english") ? 0 :
		mystrdup(language_name[item]);
	mainlist->modified = TRUE;
	write_mainlist();
	new_language_popup(widget);
}

/*ARGSUSED*/
static void search_callback(
	UNUSED Widget			widget,
	int				item,
	UNUSED XmToggleButtonCallbackStruct *data)
{
	time_t				now;		/* curr date, 0:00 */
	struct tm			*tm;

	if (!can_edit_appts())
		return;
	now  = get_time();
	now -= now % 86400;
	tm   = time_to_tm(now);
	switch (item) {
	  case 0:						/* Today */
		create_list_popup(mainlist, now,
				(time_t)0, (char *)0, (struct entry *)0, 0, 0);
		break;

	  case 1:						/* Tomorrow */
		create_list_popup(mainlist, now+86400,
				(time_t)0, (char *)0, (struct entry *)0, 0, 0);
		break;

	  case 2:						/* This week */
		now -= (config.sunday_first ?  tm->tm_wday
					    : (tm->tm_wday+6)%7) * 86400;
		create_list_popup(mainlist, now, 7*86400,
				(char *)0, (struct entry *)0, 0, 0);
		break;

	  case 3:						/* Next week */
		now -= (config.sunday_first ?  tm->tm_wday
					    : (tm->tm_wday+6)%7) * 86400;
		create_list_popup(mainlist, now+7*86400, 7*86400,
				(char *)0, (struct entry *)0, 0, 0);
		break;

	  case 4:						/* This month*/
		now -= (tm->tm_mday-1) * 86400;
		create_list_popup(mainlist, now, length_of_month(now),
				(char*)0, (struct entry *)0, 0, 0);
		break;

	  case 5:						/* All */
		create_list_popup(mainlist, (time_t)0,
				(time_t)0, (char *)0, (struct entry *)0, 0, 0);
		break;

	  case 6:						/* Private */
		create_list_popup(mainlist, (time_t)0,
				(time_t)0, (char *)0, (struct entry *)0, 0, 1);
		break;

	  case 7:						/* One file */
		create_user_sel_popup(menubar, -1, (BOOL (*)())list_by_user);
		break;

	  case 8:						/* Search */
		create_keyword_popup();
		break;
	}
}


static void list_by_user(
	int		u)		/* user index to list */
{
	create_list_popup(mainlist, (time_t)0,
			(time_t)0, (char *)0, (struct entry *)0, u+1, 0);
}


/*ARGSUSED*/
static void view_callback(
	UNUSED Widget			widget,
	int				item,
	UNUSED XmToggleButtonCallbackStruct *data)
{
	time_t time = get_time();
	struct tm *tm = time_to_tm(time);
	switch (item) {
	  case 0:						/* day */
		create_view('d');
		break;

	  case 1:						/* week */
		time -= (tm->tm_wday + 6 + config.sunday_first)%7 * 86400;
		curr_week = time;
		create_view('w');
		break;

	  case 2:						/* sm month */
	  case 3:						/* lg month */
		config.smallmonth = item == 2;
		create_view('m');
		draw_calendar(0);
		break;

	  case 4:						/* year */
		create_view('y');
		break;

	  case 5:						/* year overv*/
		create_view('o');
		break;

	  case 6:						/* goto today*/
		curr_month = tm->tm_mon;
		curr_year  = tm->tm_year;
		curr_week  =
		curr_day   = time - 86400 *
				(tm->tm_wday - !config.sunday_first);
		draw_month_year();
		redraw_all_views();
		break;

	  case 7:						/* goto */
		create_goto_popup();
		break;
	}
}


/*ARGSUSED*/
static void help_pulldown(
	UNUSED Widget			widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	Cursor				cursor;
	Widget				w;

	switch (item) {
	  case 0:						/* context */
		cursor = XCreateFontCursor(display, XC_question_arrow);
		if ((w = XmTrackingLocate(mainwindow, cursor, False))) {
			data->reason = XmCR_HELP;
			XtCallCallbacks(w, XmNhelpCallback, &data);
		}
		XFreeCursor(display, cursor);
		break;

	  case 1:						/* intro */
		help_callback(mainwindow, "intro");
		break;

	  case 2:						/* help */
		help_callback(mainwindow, "help");
		break;

	  case 3:						/* trouble */
		help_callback(mainwindow, "trouble");
		break;

	  case 4:						/* files */
		help_callback(mainwindow, "files");
		break;

	  case 5:						/* network */
		help_callback(mainwindow, "network");
		break;

	  case 6:						/* database */
		help_callback(mainwindow, "grok");
		break;

	  case 7:						/* widgets */
		help_callback(mainwindow, "widgets");
		break;

	  case 8:						/* languages */
		help_callback(mainwindow, "languages");
		break;
	}
}
