/*
 * Create and destroy entry list menus and all widgets in them. All widgets
 * are labels or pushbuttons; they are faster than text buttons. Whenever
 * the user presses a button with text in it, it is overlaid with a Text
 * button. For editing and input into the Text button, see dayedit.c.
 *
 *	destroy_all_listmenus()		Destroy all list menus. This is done
 *					when the warning column width changes.
 *	update_all_listmenus(reeval)	Redraw all list menus. Also rebuild if
 *					reeval==TRUE.
 *	create_entry_widgets(w, nentries)
 *					Creates all widgets in a list menu
 *					that make up the entry list array. If
 *					widgets exist already, make sure that
 *					there are enough and remove excess.
 *	prepare_for_modify(name)	check if other user's file is writable,
 *					mark modified if yes
 *	edit_list_button(doedit, w, x, y)
 *					Turn a text button in the entry list
 *					from Label to Text or vice versa.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/LabelP.h>
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/ArrowB.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/Protocols.h>
#include "cal.h"

#define NBLANK		3	/* add blank rows this many at a time */
				/* dimensions of text entry area */
#define X_MARGIN	4	/* width of margin at left/right edges */
#define X_ENABLE	30	/* width of enable checkbutton */
#define X_LABEL		48	/* width of label text before text field */
#define X_DATE		100	/* width of text field for dates */
#define X_TIME		65	/* width of text fields except date and note */
#define X_ARROW		13	/* width of up/down arrows, Y_ROW/2 */
#define X_NOTEGAP	10	/* width of blank gap before note text field */
#define X_OPTION	26	/* width of square option button */
#define X_NOTE		(X_LABEL+6*X_OPTION)
#define X_TOTAL		(X_MARGIN*2 + X_ENABLE + X_LABEL*3 + X_ARROW*3 +\
			 X_DATE + X_TIME*3 + X_NOTEGAP + X_NOTE)
#define Y_MARGIN	8	/* height of margin at top/bottom edges */
#define Y_ROW		30	/* height of a single row */
#define Y_GAP		6	/* height of gap between entry row pairs */

static void destroy_list_menu	(struct listmenu *);
static void create_list_widgets	(struct listmenu *, int);
static void list_widget_pos	(int, int, int *, int *, int *, int *, int *);
static void list_confirm	(Widget, int, XmToggleButtonCallbackStruct *);
static void list_undo		(Widget, int, XmToggleButtonCallbackStruct *);
static void list_dup		(Widget, int, XmToggleButtonCallbackStruct *);
static void list_delete		(Widget, int, XmToggleButtonCallbackStruct *);
static void list_done		(Widget, int, XmToggleButtonCallbackStruct *);
static void list_pin		(Widget, int, XmToggleButtonCallbackStruct *);
static void list_own		(Widget, int, XmToggleButtonCallbackStruct *);
static void list_entry		(Widget, int, XmToggleButtonCallbackStruct *);
static void list_edit		(Widget, int, XmToggleButtonCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern Widget		toplevel;	/* top-level shell for icon name */
extern GC		gc;		/* everybody uses this context */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern Pixmap		pixmap[NPICS];	/* common symbols */
extern struct config	config;		/* global configuration data */
extern struct mainmenu	mainmenu;	/* all important main window widgets */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern struct user	*user;		/* user list (from file_r.c) */
extern struct edit	edit;		/* info about entry being edited */
extern struct holiday	holiday[366];	/* info for each day, separate for */
extern struct holiday	sm_holiday[366];/* full-line texts under, and small */
extern short		monthbegin[12];	/* julian date each month begins with*/
extern int		curr_month;	/* month being displayed, 0..11 */
extern int		curr_year;	/* year being displayed, since 1900 */

static struct listmenu	*listmenu;	/* all important list popup widgets */



/*-------------------------------------------------- popup management -------*/
/*
 * return a pointer to the widget struct for the menu containing widget <w>.
 * This is used to find out in which instantiation of the list popup menu a
 * button exists.
 */

static struct listmenu *find_widget_in_list_menu(
	Widget		w)		/* button to find */
{
	struct listmenu	*scan;		/* popup scan pointer */

	do
		for (scan=listmenu; scan; scan=scan->next)
			if (w == scan->shell)
				return(scan);
	while ((w = XtParent(w)));
	return(0);
}


/*
 * a new popup is requested. Make a listmenu struct, or re-use an old one
 * if there is one that is not pinned. Popups or the widgets they contain
 * are never destroyed; they are merely popped down or declared unpinned.
 * If a nonzero <time> is specified, return the popup for that day if there
 * is one.
 */

static struct listmenu *create_list_menu(
	time_t		time,		/* for what day? */
	time_t		period,		/* for how many days? */
	char		*key,		/* keyword lookup? */
	struct entry	*entry,		/* just this one entry? */
	int		user_plus_1,	/* which user file */
	BOOL		private)	/* only private */
{
	struct listmenu *scan;		/* popup scan pointer */

	confirm_new_entry();
	for (scan=listmenu; scan; scan=scan->next) {	/* similar menu? */
		if (!scan->valid || !scan->popped)
			continue;
		if ((time	 && scan->time == time
				 && scan->period == period)		||
		    (key	 && scan->key
		    		 && !strcmp(scan->key, key))		||
		    (entry	 && entry == scan->oneentry)		||
		    (user_plus_1 && user_plus_1 == scan->user_plus_1)	||
		    (private	 && scan->private))

			return(scan);
	}
	for (scan=listmenu; scan; scan=scan->next)	/* unused menu? */
		if (scan->popped && !scan->pinned)
			break;
	if (!scan)
		for (scan=listmenu; scan; scan=scan->next)
			if (!scan->popped)
				break;
							/* no, make new one */
	if (!scan) {
		if (!(scan=(struct listmenu *)malloc(sizeof(struct listmenu))))
			fatal(_("no memory"));
		scan->popped      = TRUE;
		scan->valid       = FALSE;
		scan->pinned      = FALSE;
		scan->own_only    = config.ownonly;
		scan->entry       = 0;
		scan->nentries    = 0;
		scan->text        = 0;
		scan->sublist     = 0;
		scan->prev        = 0;
		scan->next        = listmenu;
		if (listmenu)
			listmenu->prev = scan;
		listmenu = scan;
	}
	destroy_sublist(scan);
	scan->time     = time;
	scan->period   = period;
	scan->key      = key;
	scan->oneentry = entry;
	scan->private  = private;
	scan->user_plus_1 = user_plus_1;
	return(scan);
}


/*
 * destroy a popup. Remove it from the screen, but do not destroy its
 * widgets. They will be re-used when a new popup is created. Release
 * the sublist so it won't be regenerated if some entry somewhere
 * changes.
 */

static void destroy_list_menu(
	struct listmenu	*lw)
{
	edit_list_button(0, lw, 0, 0);
	XmToggleButtonSetState(lw->pin, lw->pinned = FALSE, FALSE);
	XtPopdown(lw->shell);
	lw->popped = FALSE;
	confirm_new_entry();
	destroy_sublist(lw);
}


/*
 * the main list changed, and all list menus must be re-evaluated and
 * redrawn. Reevaluation can be disabled by setting reeval to FALSE; this
 * is useful if an appointment got changed but we don't want it to bounce
 * to a different place in the list or disappear altogether. This routine
 * is here because this is the only file that knows about all list menus.
 */

void update_all_listmenus(
	BOOL		reeval)		/* just redraw or recreate too? */
{
	struct listmenu	*scan;		/* popup scan pointer */
	
	for (scan=listmenu; scan; scan=scan->next)
		if (scan->popped) {
			if (reeval)
				create_sublist(mainlist, scan);
			draw_list(scan);
		}
}


/*
 * destroy all list menus. This is done when the width of the advance-warning
 * column is changed to a text entry field, using the fast-warning-entry
 * option in the config pulldown.
 */

void destroy_all_listmenus(void)
{
	struct listmenu		*scan, *next;	/* popup scan pointer */

	confirm_new_entry();
	for (scan=listmenu; scan; scan=next) {
		next = scan->next;
		if (scan->popped)
			destroy_list_menu(scan);
		if (scan->valid)
			XTDESTROYWIDGET(scan->shell);
		free((char *)scan);
	}
	listmenu      = 0;
	edit.menu     = 0;
	edit.srcentry = 0;
}


/*-------------------------------------------------- menu creation ----------*/
/*
 * create a list popup as a separate application shell. Use the argument
 * list to decide how many buttons to create initially.
 */

void create_list_popup(
	struct plist	*list,
	time_t		time,		/* for what day? */
	time_t		period,		/* for how many days? */
	char		*key,		/* keyword lookup? */
	struct entry	*entry,		/* just this one entry? */
	int		user_plus_1,	/* which user file */
	BOOL		private)	/* only private */
{
	struct listmenu	*w;
	char		title[200];	/* menu title string */
	char		*p;		/* temp for holiday lookup */
	struct tm	*tm;		/* time to m/d/y conv */
	struct holiday	*hp, *shp;	/* to check for holidays */

	if (!can_edit_appts())
		return;
	time   -= time   % 86400;
	period -= period % 86400;
	w = create_list_menu(time, period, key, entry, user_plus_1, private);
	create_list_widgets(w, 0);
	if (time) {
		strcpy(title, mkdatestring(time));
		if (period) {
			strcat(title, " - ");
			strcat(title, mkdatestring(time + period - 1));
		} else {
			tm  = time_to_tm(time);
			if ((p = parse_holidays(tm->tm_year, FALSE)))
				create_error_popup(mainmenu.cal, 0, p);
			shp = &sm_holiday[tm->tm_yday];
			hp  =    &holiday[tm->tm_yday];
			if (shp->string || hp->string) {
				p = " (";
				if (shp->string) {
					strcat(title, p);
					strcat(title, shp->string);
					p = ", ";
				}
				if (hp->string) {
					strcat(title, p);
					strcat(title, hp->string);
				}
				strcat(title, ")");
			}
		}
	} else if (w->key) {
		sprintf(title, "/%.40s/", w->key);
	} else if (entry) {
		strcpy(title, _("Entry from week view"));
	} else if (user_plus_1) {
		strcpy(title, user_plus_1 == 1 ? _("Own")
					       : user[user_plus_1-1].name);
	} else if (private) {
		strcpy(title, _("Private"));
	} else
		strcpy(title, _("All Entries"));

	print_button(w->title, title);
	create_sublist(list, w);
	draw_list(w);
}


/*
 * create all widgets for a schedule list popup, including the popup window
 * itself. The number of initial entry rows in the list is set to <nentries>.
 */

static void create_list_widgets(
	struct listmenu	*w,		/* create which menu */
	int		nentries)	/* how many entries */
{
	Widget		form, scroll, help;
	Arg		args[15];
	int		n, width;
	Atom		closewindow;

	if (w->valid) {
		XtPopup(w->shell, XtGrabNone);
		w->popped = TRUE;
		XRaiseWindow(display, XtWindow(w->shell));
		return;
	}
	w->valid = TRUE;

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmUNMAP);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	w->shell = XtCreatePopupShell(_("Schedule"),
			applicationShellWidgetClass, toplevel, args, n);
	
#	ifdef EDITRES
	XtAddEventHandler(w->shell, (EventMask)0, TRUE, 
 			_XEditResCheckMessages, NULL);
#	endif
	set_icon(w->shell, 1);
	form = XtCreateWidget("dayform", xmFormWidgetClass,
			w->shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"day");

							/*-- title -- */
	n = 0;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	w->title = XtCreateManagedWidget("title", xmLabelGadgetClass,
			form, args, n);
							/*-- buttons --*/
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		72);			n++;
	XtSetArg(args[n], XmNsensitive,		False);			n++;
	w->confirm = XtCreateManagedWidget(_("Confirm"),
			xmPushButtonWidgetClass, form, args, n);
	XtAddCallback(w->confirm, XmNactivateCallback, (XtCallbackProc)
			list_confirm, (XtPointer)(size_t)0);
	XtAddCallback(w->confirm, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"day_confirm");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w->confirm);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		72);			n++;
	XtSetArg(args[n], XmNsensitive,		False);			n++;
	w->undo = XtCreateManagedWidget(_("Undo"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w->undo, XmNactivateCallback, (XtCallbackProc)
			list_undo, (XtPointer)(size_t)0);
	XtAddCallback(w->undo, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"day_undo");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w->undo);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		72);			n++;
	XtSetArg(args[n], XmNsensitive,		False);			n++;
	w->dup = XtCreateManagedWidget(_("Dup"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w->dup, XmNactivateCallback, (XtCallbackProc)
			list_dup, (XtPointer)(size_t)0);
	XtAddCallback(w->dup, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer)"day_dup");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w->dup);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		72);			n++;
	XtSetArg(args[n], XmNsensitive,		False);			n++;
	w->del = XtCreateManagedWidget(_("Delete"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w->del, XmNactivateCallback, (XtCallbackProc)
			list_delete, (XtPointer)(size_t)0);
	XtAddCallback(w->del, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer)"day_del");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w->del);		n++;
	XtSetArg(args[n], XmNleftOffset,	32);			n++;
	XtSetArg(args[n], XmNwidth,		72);			n++;
	help = XtCreateManagedWidget(_("Help"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(help, XmNactivateCallback, (XtCallbackProc)
			help_callback, (XtPointer)"day");
	XtAddCallback(help, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"day");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	help);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		72);			n++;
	w->done = XtCreateManagedWidget(_("Done"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w->done, XmNactivateCallback, (XtCallbackProc)
			list_done, (XtPointer)(size_t)0);
	XtAddCallback(w->done, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer)"day_done");

	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_RED]);	n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	w->pin = XtCreateManagedWidget(_("Pin"), xmToggleButtonWidgetClass,
			form, args, n);
	XtAddCallback(w->pin, XmNvalueChangedCallback, (XtCallbackProc)
			list_pin, (XtPointer)(size_t)0);
	XtAddCallback(w->pin, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"day_pin");

	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w->pin);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNset,		config.ownonly);	n++;
	w->own = XtCreateManagedWidget(_("Own Only"),
			xmToggleButtonWidgetClass, form, args, n);
	XtAddCallback(w->own, XmNvalueChangedCallback, (XtCallbackProc)
			list_own, (XtPointer)(size_t)0);
	XtAddCallback(w->own, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"day_own");

							/*-- scroll --*/
	n = 0;
	width = X_TOTAL + 34 +
		(config.notewidth > X_NOTE ? config.notewidth - X_NOTE : 0);
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w->title);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	w->done);		n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		width);			n++;
	XtSetArg(args[n], XmNheight,		240);			n++;
	XtSetArg(args[n], XmNscrollingPolicy,	XmAUTOMATIC);		n++;
	scroll = XtCreateWidget("lscroll", xmScrolledWindowWidgetClass,
			form, args, n);
	XtAddCallback(scroll, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"day");

	n = 0;
	w->list = XtCreateWidget("list", xmBulletinBoardWidgetClass,
			scroll, args, n);
	XtManageChild(w->list);

	create_entry_widgets(w, nentries);

	XtManageChild(scroll);
	XtManageChild(form);
	XtPopup(w->shell, XtGrabNone);

	XtVaSetValues(w->shell, XmNdeleteResponse, XmDO_NOTHING, NULL);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(w->shell, closewindow, (XtCallbackProc)
			list_done, (XtPointer)w->shell);
}


/*
 * makes sure there are enough widget rows for schedule entries. Also makes
 * sure that there aren't too many, for speed reasons. As usual, allocate
 * one extra widget row for the title at the top. Each row consists of
 * SC_N different widgets. This can be called again later with a larger
 * nentries argument, to make the list longer. All the text buttons are
 * label widgets. For performance reasons, they are overlaid by a text
 * widget when pressed. This doesn't make much difference on an Indigo,
 * but it might on other platforms and I'd feel bad in the morning otherwise.
 * No text is printed into the buttons yet, this is done later by draw_list().
 */

static char *help_name[SC_N] = {
	"day_enable", "day_user", "day_date", "day_enddate", "day_time",
	"day_length", "day_advance", "day_advdays", "day_note", "day_recycle",
	"day_message", "day_script", "day_except", "day_private", "day_todo",
	"day_date", "day_enddate", "day_time", "day_length", "day_advance",
	"day_advdays",
	"day_date", "day_enddate", "day_time", "day_length", "day_advance",
	"day_advdays",
	"day_date", "day_enddate", "day_time", "day_length", "day_advance",
	"day_advdays", "day_flags"
};

void create_entry_widgets(
	struct listmenu	*w,
	int		nentries)
{
	int		x, y;
	Arg		args[15];
	int		n;
	int		xpos, ypos;
	int		width, height;
	int		align;
	char		*name;
	WidgetClass	class;

	nentries = nentries+NBLANK - nentries%NBLANK;
	/* right number of rows? */ 
	if (nentries == w->nentries)
		return;

	/* make the list size fit the new number of rows */
	n = 0;
	list_widget_pos(SC_NOTE, nentries+1,
					&xpos, &ypos, &width, &height, &align);
	XtSetArg(args[n], XmNheight, ypos); n++;
	XtSetValues(w->list, args, n);

	/* have too many? */
	if (nentries < w->nentries) {
		for (y=nentries+1; y <= w->nentries; y++)
			for (x=0; x < SC_N; x++)
				XtDestroyWidget(w->entry[y][x]);
		w->nentries = nentries;
		return;
	}

	/* need more! */
	n = (nentries+1) * SC_N * sizeof(Widget *);
	if ((w->entry && !(w->entry = (Widget (*)[])realloc(w->entry, n))) ||
	   (!w->entry && !(w->entry = (Widget (*)[])malloc(n))))
		fatal(_("no memory"));

	XtUnmanageChild(w->list);
	for (x=0; x < SC_N; x++) {
	    for (y=w->nentries; y <= nentries; y++) {
		list_widget_pos(x, y, &xpos, &ypos, &width, &height, &align);
		class = xmPushButtonWidgetClass;
		name  = "";
		n = 0;
		switch(x) {
		  case SC_ENABLE:
			class  = xmToggleButtonWidgetClass;
			XtSetArg(args[n], XmNselectColor, color[COL_TOGGLE]);
			n++;
			break;

		  case SC_USER:     name = _("Group"); break;
		  case SC_DATE:     name = _("Date");  break;
		  case SC_TIME:     name = _("Time");  break;
		  case SC_ADVANCE:  name = _("Warn");  break;
		  case SC_NOTE:     name = _("Note");  break;

		  case SC_RECYCLE:
		  case SC_MESSAGE:
		  case SC_SCRIPT:
		  case SC_EXCEPT:
		  case SC_PRIVATE:
		  case SC_TODO:
			XtSetArg(args[n], XmNlabelType, XmPIXMAP); n++;
			break;

		  case SC_I_DATE:
		  case SC_I_ENDDATE:
		  case SC_I_TIME:
		  case SC_I_LENGTH:
		  case SC_I_ADVANCE:
		  case SC_I_ADVDAYS:
			XtSetArg(args[n], XmNarrowDirection, XmARROW_UP); n++;
			class = xmArrowButtonWidgetClass;
			break;

		  case SC_D_DATE:
		  case SC_D_ENDDATE:
		  case SC_D_TIME:
		  case SC_D_LENGTH:
		  case SC_D_ADVANCE:
		  case SC_D_ADVDAYS:
			XtSetArg(args[n], XmNarrowDirection, XmARROW_DOWN);n++;
			class = xmArrowButtonWidgetClass;
			break;

		  case SC_L_DATE:    name = _("beg:");  break;
		  case SC_L_ENDDATE: name = _("end:");  break;
		  case SC_L_TIME:    name = _("beg:");  break;
		  case SC_L_LENGTH:  name = _("len:");  break;
		  case SC_L_ADVANCE: name = _("min:");  break;
		  case SC_L_ADVDAYS: name = _("days:"); break;
		  case SC_L_FLAGS:   name = _("opt:");  break;
		}
		if (!y || x >= SC_L)
			class = xmLabelWidgetClass;
		if ((x && y && x < SC_L) || (!y && x >= SC_L))
			name  = " ";
		if (!y && class != xmLabelWidgetClass)
			continue;

		XtSetArg(args[n], XmNx,			xpos);		n++;
		XtSetArg(args[n], XmNy,			ypos);		n++;
		XtSetArg(args[n], XmNwidth,		width);		n++;
		XtSetArg(args[n], XmNheight,		height);	n++;
		XtSetArg(args[n], XmNalignment,         align);		n++;
		XtSetArg(args[n], XmNrecomputeSize,	False);		n++;
		XtSetArg(args[n], XmNtraversalOn,	True);		n++;
		XtSetArg(args[n], XmNhighlightThickness,0);		n++;
		XtSetArg(args[n], XmNshadowThickness,	x&&y&&x<SC_N);	n++;
		w->entry[y][x] = XtCreateManagedWidget(name, class,
				w->list, args, n);
		if (x < SC_L) {
		    if (y && x)
			XtAddCallback(w->entry[y][x], XmNactivateCallback,
					(XtCallbackProc)list_entry,
					(XtPointer)(size_t)(x + y * SC_N));
		    if (y && !x)
			XtAddCallback(w->entry[y][x], XmNvalueChangedCallback,
					(XtCallbackProc)list_entry,
					(XtPointer)(size_t)(x + y * SC_N));
		}
		XtAddCallback(w->entry[y][x], XmNhelpCallback,
					(XtCallbackProc)help_callback,
					(XtPointer)help_name[x]);
	    }
	}
	w->nentries = nentries;
	XtManageChild(w->list);
}


/*
 * return position, size, and alignment of a widget in the scroll list
 */

static void list_widget_pos(
	int		x,		/* row/column */
	int		y,
	int		*xpos,		/* returned pixel pos */
	int		*ypos,
	int		*width,		/* returned width */
	int		*height,	/* returned height */
	int		*align)		/* ret. text alignment: l,c,r*/
{
	*width  = X_LABEL;
	*height = Y_ROW;
	*xpos   = X_MARGIN + X_ENABLE;
	*ypos   = Y_MARGIN + (y ? y * (Y_ROW*2 + Y_GAP) - Y_ROW : 0);
	*align  = x < SC_L ? XmALIGNMENT_CENTER : XmALIGNMENT_END;
	if (x >= SC_I && x < SC_L) {
		*width  = X_ARROW;
		*height = Y_ROW/2;
		if (x >= SC_D)
			*ypos += *height;
	}
	switch(x) {
	  case SC_ENABLE:
		*xpos  = X_MARGIN;
		*width = X_ENABLE;
		break;
	  case SC_USER:
		*width = X_TIME;
		break;
	  case SC_DATE:
		*xpos += X_LABEL + X_TIME;
		*width = X_DATE;
		break;
	  case SC_ENDDATE:
		*ypos += Y_ROW;
		*xpos += X_LABEL + X_TIME;
		*width = X_DATE;
		break;
	  case SC_TIME:
		*xpos += X_LABEL*2 + X_TIME + X_DATE + X_ARROW;
		*width = X_TIME;
		break;
	  case SC_LENGTH:
		*ypos += Y_ROW;
		*xpos += X_LABEL*2 + X_TIME + X_DATE + X_ARROW;
		*width = X_TIME;
		break;
	  case SC_ADVANCE:
		*xpos += X_LABEL*3 + X_TIME*2 + X_DATE + X_ARROW*2;
		*width = X_TIME;
		break;
	  case SC_ADVDAYS:
		*ypos += Y_ROW;
		*xpos += X_LABEL*3 + X_TIME*2 + X_DATE + X_ARROW*2;
		*width = X_TIME;
		break;
	  case SC_NOTE:
		*xpos += X_LABEL*3 + X_TIME*3 + X_DATE + X_ARROW*3 + X_NOTEGAP;
		*width = config.notewidth > X_NOTE ? config.notewidth : X_NOTE;
		*align = XmALIGNMENT_BEGINNING;
		break;

	  case SC_RECYCLE:
	  case SC_MESSAGE:
	  case SC_SCRIPT:
	  case SC_EXCEPT:
	  case SC_PRIVATE:
	  case SC_TODO:
		*ypos += Y_ROW;
		*xpos += X_LABEL*4 + X_TIME*3 + X_DATE + X_ARROW*3 +
					X_NOTEGAP + X_OPTION * (x-SC_FLAGS);
		*width = X_OPTION;
		break;

	  case SC_I_ENDDATE:
	  case SC_D_ENDDATE:
		*ypos += Y_ROW;
	  case SC_I_DATE:
	  case SC_D_DATE:
		*xpos += X_LABEL + X_TIME + X_DATE;
		break;
	  case SC_I_LENGTH:
	  case SC_D_LENGTH:
		*ypos += Y_ROW;
	  case SC_I_TIME:
	  case SC_D_TIME:
		*xpos += X_LABEL*2 + X_TIME*2 + X_DATE + X_ARROW;
		break;
	  case SC_I_ADVDAYS:
	  case SC_D_ADVDAYS:
		*ypos += Y_ROW;
	  case SC_I_ADVANCE:
	  case SC_D_ADVANCE:
		*xpos += X_LABEL*3 + X_TIME*3 + X_DATE + X_ARROW*2;
		break;

	  case SC_L_ENDDATE:
		*ypos += Y_ROW;
	  case SC_L_DATE:
		*xpos += X_TIME;
		break;
	  case SC_L_LENGTH:
		*ypos += Y_ROW;
	  case SC_L_TIME:
		*xpos += X_LABEL + X_TIME + X_DATE + X_ARROW;
		break;
	  case SC_L_ADVDAYS:
		*ypos += Y_ROW;
	  case SC_L_ADVANCE:
		*xpos += X_LABEL*2 + X_TIME*2 + X_DATE + X_ARROW*2;
		break;
	  case SC_L_FLAGS:
		*ypos += Y_ROW;
		*xpos += X_LABEL*3 + X_TIME*3 + X_DATE + X_ARROW*3 + X_NOTEGAP;
		break;
	}
	if (y == 0)
		*align = XmALIGNMENT_CENTER;
	--*width; --*height;
}


/*-------------------------------------------------- editing ----------------*/
/*
 * turn a text label into a Text button, to allow user input. This is done
 * by simply installing a text widget on top of the label widget. <text>
 * is put into the text widget. The previously edited button is un-edited.
 * doedit==2 is like 0 (unedit) but with the intention to delete, so don't
 * store the new text and don't cause got_entry_text on the note to confirm
 * because that would overwrite edit.entry, including the ID for netplan.
 */

void edit_list_button(
	int		doedit,		/* 1=edit, 0=unedit, 2=del */
	struct listmenu	*w,		/* create which menu */
	int		x,		/* column, SC_* */
	int		y)		/* row, y=0: title */
{
	Arg		args[15];
	int		n;
	int		xpos, ypos;
	int		width, height;
	int		align;
	char		*text = "";
	char		*space;
	static BOOL	busy;		/* not re-entrant */
	/* first click sets position in text widget, second selects all */
	XmTextScanType	scantype[2] = {XmSELECT_POSITION, XmSELECT_ALL};

	if (busy || !w)				/* not editing */
		return;
	if (w->text) {
		if (doedit != 2) {
			char *string = XmTextGetString(w->text);
			busy = TRUE;
			got_entry_text(w, w->xedit, w->yedit, string);
			busy = FALSE;
			XtFree(string);
		}
		XmProcessTraversal(w->list, XmTRAVERSE_CURRENT);
		XtDestroyWidget(w->text);
		if (x && y)
			print_button(w->entry[y][x], "%s",
					text = mkbuttonstring(w, x, y));
	}
	w->text = 0;
	if (doedit != 1)
		return;

	list_widget_pos(x, y, &xpos, &ypos, &width, &height, &align);
	n = 0;
	XtSetArg(args[n], XmNx,			xpos);		n++;
	XtSetArg(args[n], XmNy,			ypos);		n++;
	XtSetArg(args[n], XmNwidth,		width);		n++;
	XtSetArg(args[n], XmNheight,		height);	n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);		n++;
	XtSetArg(args[n], XmNpendingDelete,	True);		n++;
	XtSetArg(args[n], XmNselectionArray,	scantype);	n++;
	XtSetArg(args[n], XmNselectionArrayCount, 2);		n++;
	XtSetArg(args[n], XmNhighlightThickness,  0);		n++;
	XtSetArg(args[n], XmNshadowThickness,	  1);		n++;
	XtSetArg(args[n], XmNbackground, color[COL_TEXTBACK]);	n++;
	w->text = XtCreateManagedWidget("text", xmTextWidgetClass,
			w->list, args, n);
	XtAddCallback(w->text, XmNactivateCallback, (XtCallbackProc)
			list_edit, (XtPointer)NULL);
	XtAddCallback(w->text, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)help_name[x]);
	XmProcessTraversal(w->text, XmTRAVERSE_CURRENT);

	if (w->sublist && w->sublist->nentries >= y-1 &&
			!(w->sublist->nentries == y-1 && x == SC_TIME))
		text = mkbuttonstring(w, x, y);
	if ((x == SC_DATE || x == SC_ENDDATE) &&
					(space = strchr(text, ' ')))
		text = space+1;
	print_text_button(w->text, "%s", text);
	w->xedit = x;
	w->yedit = y;
}


/*
 * prepare for changing an entry of user <name>. Set the modified flag
 * for that user (so write_mainlist will write it later), or print an
 * error popup if the user's file is not writable. Writing to files
 * will start only when mainlist->modified is set, which has the side
 * effect of *always* writing out own files, but it's good enough for
 * now. Return TRUE if writing can proceed, or FALSE on error.
 */

BOOL prepare_for_modify(
	char		*name)		/* user name or 0 */
{
	int		u;		/* index into user array */

	if (name) {
		u = name_to_user(name);
	} else {
		name = user[u=0].name;
	}
	if (user[u].readonly) {
		if (user[u].fromserver)
			create_error_popup(0, 0,
_("Cannot modify appointments of %s,\n\
server on %s denies write permission.\n\
Re-check the permissions with File->Reread database.\n\
You can not disable individual appointments, but you\n\
can disable all with the File->File list popup."),
					name, user[u].server);
		else if (user[u].grok)
			create_error_popup(0, 0,
_("Cannot modify appointments of %s because they\n\
were read from a grok database. Saving is not\n\
implemented for grok databases. The file is\n%s"),
					name, user[u].path);
		else
			create_error_popup(0, 0,
_("Cannot modify appointments of %s,\n\
no write permission for %s\n\
or %s/.dayplan.\n\n\
Re-check the permissions with File->Reread database.\n\
You can not disable individual appointments, but you\n\
can disable all with the File->File list popup."),
					name, user[u].path, user[u].path);
		return(FALSE);
	}
	user[u].modified = TRUE;
	return(TRUE);
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * dup, Delete, and Done buttons
 * All of these routines are direct X callbacks.
 */

/*ARGSUSED*/
static void list_confirm(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	if (find_widget_in_list_menu(widget) == edit.menu)
		confirm_new_entry();
}


/*ARGSUSED*/
static void list_undo(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	struct listmenu *lw = find_widget_in_list_menu(widget);
	if (lw == edit.menu && prepare_for_modify(edit.entry.user)) {
		undo_new_entry();
		edit_list_button(0, lw, 0, 0);
	}
}


/*ARGSUSED*/
static void list_dup(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	struct entry tmp;
	struct listmenu *lw = find_widget_in_list_menu(widget);
	if (lw) {
		int num = lw->sublist ? lw->sublist->nentries : 0;
		if (edit.editing && lw == edit.menu && edit.y-1 < num
				 && prepare_for_modify(edit.entry.user)) {
			clone_entry(&tmp, &edit.entry);
			confirm_new_entry();
			tmp.file     = 0;
			tmp.id       = 0;
			edit.entry   = tmp;
			edit.y       = num+1;
			edit.editing = TRUE;
			draw_list(lw);
			edit.changed = TRUE;
			sensitize_edit_buttons();
			got_entry_press(lw, SC_DATE, edit.y, FALSE, 0);
		}
	}
}

/*ARGSUSED*/
static void list_delete(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	struct listmenu *lw = find_widget_in_list_menu(widget);
	if (!edit.editing || lw != edit.menu
			  || !edit.menu->sublist
			  || edit.y-1 >= edit.menu->sublist->nentries
			  || !prepare_for_modify(edit.entry.user))
		return;

	destroy_recycle_popup();
	destroy_text_popup();
	edit_list_button(2, lw, 0, 0);
	edit.editing = FALSE;
	edit.changed = FALSE;
	sensitize_edit_buttons();

	server_entry_op(&edit.entry, 'd');
	delete_entry(mainlist, lw->sublist->entry[edit.y-1] - mainlist->entry);

	rebuild_repeat_chain(mainlist);
	/* draw_list(edit.menu); 1.8.4 */
	update_all_listmenus(TRUE);
	redraw_all_views();
}

/*ARGSUSED*/
static void list_done(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	struct listmenu *lw = find_widget_in_list_menu(widget);
	if (lw)
		destroy_list_menu(lw);
}

/*ARGSUSED*/
static void list_pin(
	Widget				widget,
	UNUSED int			item,
	XmToggleButtonCallbackStruct	*data)
{
	struct listmenu *lw = find_widget_in_list_menu(widget);
	if (lw)
		lw->pinned = data->set;
}

/*ARGSUSED*/
static void list_own(
	Widget				widget,
	UNUSED int			item,
	XmToggleButtonCallbackStruct	*data)
{
	struct listmenu *lw = find_widget_in_list_menu(widget);
	if (lw) {
		lw->own_only = config.ownonly = data->set;
		create_sublist(mainlist, lw);
		draw_list(lw);
	}
}

/*ARGSUSED*/
static void list_entry(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	struct listmenu			*lw = find_widget_in_list_menu(widget);
	int				x = item % SC_N;
	int				y = item / SC_N;

	if (lw)
		got_entry_press(lw, x, y, data->set,
					data->event->xbutton.state & 1);
}

/*ARGSUSED*/
static void list_edit(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	struct listmenu			*lw = find_widget_in_list_menu(widget);
	char				*text = XmTextGetString(widget);

	if (lw)
		got_entry_text(lw, lw->xedit, lw->yedit, text);
	XtFree(text);
}
