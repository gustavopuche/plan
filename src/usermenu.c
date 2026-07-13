/*
 * Create and destroy user menus and all widgets in them. All widgets
 * are labels or pushbuttons; they are faster than text buttons. Whenever
 * the user presses a button with text in it, it is overlaid with a Text
 * button. For editing and input into the Text button, see useredit.c.
 *
 *	all_files_served()		all files read from server?
 *	can_edit_appts()		don't edit appts if user menu is up
 *	destroy_user_popup()		remove user popup
 *	create_user_popup()		create user popup
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/LabelP.h>
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/Protocols.h>
#include "config.h"
#include "cal.h"

enum Columns { C_EN_MONTH=0, C_EN_WEEK, C_EN_DAY, C_EN_YROV, C_EN_ALARM,
		C_GROUP, C_NAME, C_EN_SRV, C_PATH, C_SERVER, NCOLUMNS,
		COLPOPUP };

static char *default_server	(char *, int, int);
static void edit_user_button	(BOOL, int, int);
static void got_user_text	(int, int, char *);
static void copyuser		(struct user **, int *, struct user *, int);
static void delete_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void sort_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void browse_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void cancel_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void done_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void list_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void got_text		(Widget, int, XmToggleButtonCallbackStruct *);
static Widget call_popup_callback(Widget, XtPointer, XEvent *, Boolean *);

#ifdef MIPS
extern struct passwd *getpwnam();
#endif

extern Display		*display;	/* everybody uses the same server */
extern Widget		toplevel;	/* top-level shell for icon name */
extern GC		gc;		/* everybody uses this context */
extern GC		chk_gc;		/* checkered gc for light background */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern Pixmap		pixmap[NPICS];	/* common symbols */
extern Pixmap		col_pixmap[8];	/* colored circles for file list */
extern Pixmap		chk_pixmap[8];	/* same but checkered (light color) */
extern Widget		mainwindow;	/* popup menus hang off main window */
extern struct config	config;		/* global configuration data */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern struct mainmenu	mainmenu;	/* all important main window widgets */
extern struct user	*user;		/* user list (from file_r.c) */
extern int		nusers;		/* number of users in user list */

static struct user	*saved_user;	/* saved user[] while editing */
static int		saved_nusers;	/* saved nusers while editing */
static BOOL		have_shell;	/* message popup exists if TRUE */
static Widget		shell;		/* popup menu shell */
static Widget		delete;		/* delete button, for desensitizing */
static Widget		info;		/* info line for error messages */
static Widget		textwidget;	/* if editing, text widget; else 0 */
static int		have_nrows;	/* # of table widget rows allocated */
static int		xedit, yedit;	/* if editing, column/row */
static int		ycurr;		/* current row, 0=none, 1=1st user...*/
static Widget		ulist;		/* user list RowColumn widget */
static Widget	(*utable)[NCOLUMNS+2];	/* all widgets in user list table */
					/* [0][*] is title row, */
					/* [*][NCOLUMNS] is form widget */
					/* [*][COLPOPUP] is color popup */

/*
 * return TRUE id all files are read from servers. In this case, buttons like
 * Sync or only-owner-can-write have no function and can be grayed out.
 */

BOOL all_files_served(void)
{
	int u;

	for (u=nusers-1; u >= 0; u++)
		if (!user[u].fromserver)
			return(FALSE);
	return(TRUE);
}


/*
 * Return FALSE if the user menu is up. While it is up, plan is not connected
 * to network servers, so any appointment editing is perilous and will probably
 * be lost. Also put up an error popup and bring the usermenu to the front.
 */

BOOL can_edit_appts(void)
{
	if (have_shell) {
		create_error_popup(shell, 0,
			_("Please finish specifying the file list and the\n"\
			  "network connections before editing appointments."));
		return(FALSE);
	}
	return(TRUE);
}


/*
 * copy user list <src> to user list <tar>. If <tar> is non-null, it is deleted
 * first. If <src> is null, <tar> will end up empty. This is used for copying
 * user to save_user when editing begins, and to copy save_user to user when
 * Cancel is pressed. The sizes are also copied but are not otherwise checked.
 */

static void copyuser(
	struct user	**tar,		/* lists to copy */
	int		*ntar,		/* list sizes to copy */
	struct user	*src,
	int		nsrc)
{
	struct user	*u;		/* target scan pointer */
	int		i;		/* target counter */

	if (*tar) {
		for (u=*tar, i=*ntar; i; i--, u++) {
			if (u->name)	free(u->name);
			if (u->path)	free(u->path);
			if (u->server)	free(u->server);
#ifdef PLANGROK
			form_delete(u->form);
			dbase_delete(u->dbase);
			u->grok  = FALSE;
			u->form  = 0;
			u->dbase = 0;
#endif
		}
		free(*tar);
	}
	*tar = 0;
	if (src) {
		if (!(u = *tar = malloc(nsrc * sizeof(struct user))))
			fatal(_("no memory"));
		for (i=0; i < nsrc; i++, u++, src++) {
			*u = *src;
			u->name   = mystrdup(u->name);
			u->path   = mystrdup(u->path);
			u->server = mystrdup(u->server);
		}
	}
	*ntar = nsrc;
}


/*
 * destroy the popup. Remove it from the screen, and destroy its widgets.
 * Redraw the week menu if there is one. If reading fails, something may
 * be wrong with the servers; don't remove the popup in this case. Before
 * saving or reading, check if any file is served that wasn't before, and
 * do the attach and offer to copy the file to the server first.
 */

static BOOL accept_new_list(void)
{
	struct entry	*ep;
	struct user	*up;
	int		u, i, nlocal, nremote;
	char		*msg;

	for (u=nusers-1; u >= 0; u--)
		if (!user[u].prev_fromserver && user[u].fromserver)
			break;
	if (u >= 0) {
	    if ((msg = attach_to_network())) {
		create_error_popup(shell, 0, msg);
		free(msg);
		return(FALSE);
	    }
	    for (up=user, u=0; u < nusers; u++, up++) {
		if (!up->fromserver || up->prev_fromserver)
			continue;
		nremote = open_and_count_rows_on_server(u);
		nlocal  = 0;
		ep = mainlist->entry;
		for (i=mainlist->nentries-1; i >= 0; i--, ep++)
			nlocal += (!ep->user && !u) ||
				  (ep->user && !strcmp(ep->user, up->name));
		if (!nlocal)
			continue;
		if (!nremote) {
			ep = mainlist->entry;
			for (i=mainlist->nentries-1; i >= 0; i--, ep++)
				if ((!ep->user && !u) ||
				     !strcmp(ep->user, up->name)) {
					ep->id   = 0;
					ep->file = up->file_id;
					if (!server_entry_op(ep, 'w'))
						create_error_popup(0, 0,
							_("write error"));
				}
			create_error_popup(mainwindow, 0,
				_("File %s on server %s did not\nexist. "\
				  "Created one and initialized with\n"\
				  "currently loaded appointments."),
				up->name, up->server);
			continue;

		} else if (!u) {
			char source[512], target[518];
			strcpy(source, resolve_tilde(DB_PUB_PATH));
			sprintf(target, "%s.old", source);
			unlink(target);
			if (!copyfile(source, target)) {
				create_error_popup(shell, 0,
					_("Failed to back up %s,\ncannot "\
					  "write to %s"), source, target);
				return(FALSE);
			}
			create_error_popup(mainwindow, 0,
				_("You have %d appointments here and\n"\
				  "%d on server %s. Saved %s\n"\
				  "to %s and discarded old\n"\
				  "appointments in %s."),
				nlocal, nremote, up->server,
				source, target, source);
		} else if (nlocal != nremote)
			create_error_popup(mainwindow, 0,
				_("File %s has %d appointments in %s\n"\
				  "and %d on server %s. Switching to "\
				  "server and ignoring\nfile %s."),
				up->name, nlocal, up->path,
				nremote, up->server, up->path);
	    }
	}
	mainlist->modified = TRUE;
	write_mainlist();
	if (!read_mainlist())
		return(FALSE);
	update_all_listmenus(TRUE);
	redraw_all_views();
	resynchronize_daemon();
	return(TRUE);
}


void destroy_user_popup(
	BOOL		accept)		/* true=Done, false=Cancel */
{
	if (!have_shell)
		return;
	edit_user_button(FALSE, 0, 0);
	destroy_serv_popup();
	if (accept) {
		if (!accept_new_list())
			return;
		copyuser(&saved_user, &saved_nusers, 0, 0);
		mainlist->modified = TRUE;
		write_mainlist();
	} else
		copyuser(&user, &nusers, saved_user, saved_nusers);

	XtPopdown(shell);
	XTDESTROYWIDGET(shell);
	have_shell = FALSE;
}


/*
 * create a user popup as a separate application shell.
 */

void create_user_popup(void)
{
	Widget			form, scroll, w;
	Arg			args[15];
	int			n;
	Atom			closewindow;

	if (have_shell)
		return;

	destroy_all_listmenus();/* appointment editing is now locked */
	write_mainlist();	/* last chance to write modified foreign apts*/

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmUNMAP);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtCreatePopupShell(_("File List"),
			applicationShellWidgetClass, toplevel, args, n);
#	ifdef EDITRES
	XtAddEventHandler(shell, (EventMask)0, TRUE, 
 			_XEditResCheckMessages, NULL);
#	endif
	set_icon(shell, 1);
	form = XtCreateWidget("userform", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"user");

							/*-- buttons --*/
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	XtSetArg(args[n], XmNsensitive,		False);			n++;
	delete = w = XtCreateManagedWidget(_("Delete"),
			xmPushButtonWidgetClass, form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			delete_callback, (XtPointer)(size_t)0);
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback,   (XtPointer) "user_delete");
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Sort"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			sort_callback, (XtPointer)(size_t)0);
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer) "user_sort");
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Browse"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			browse_callback, (XtPointer)(size_t)0);
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer) "user_browse");
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Done"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			done_callback, (XtPointer)(size_t)0);
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer) "user_done");
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
			help_callback, (XtPointer)"user");
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer)"user");
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Cancel"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			cancel_callback, (XtPointer)(size_t)0);
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer) "user_done");

							/*-- infotext -- */
	n = 0;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	w);			n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNalignment,         XmALIGNMENT_BEGINNING);	n++;
	info = XtCreateManagedWidget(" ", xmLabelGadgetClass,
			form, args, n);

							/*-- scroll --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	info);			n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		785);			n++;
	XtSetArg(args[n], XmNheight,		250);			n++;
	XtSetArg(args[n], XmNscrollingPolicy,	XmAUTOMATIC);		n++;
	scroll = XtCreateWidget("uscroll", xmScrolledWindowWidgetClass,
			form, args, n);
	XtAddCallback(scroll, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"user");

	n = 0;
	ulist = XtCreateManagedWidget("ulist", xmBulletinBoardWidgetClass,
			scroll, args, n);

	create_user_rows();	/* have_shell must be FALSE here */

	XtManageChild(scroll);
	XtManageChild(form);
	XtPopup(shell, XtGrabNone);

	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow, (XtCallbackProc)
			done_callback, (XtPointer)shell);
	have_shell = TRUE;
	for (n=0; n < nusers; n++)
		user[n].prev_fromserver = user[n].fromserver;

	copyuser(&saved_user, &saved_nusers, user, nusers);
}


/*
 * makes sure there are enough widget rows for schedule entries. Also makes
 * sure that there aren't too many, for speed reasons. Allocate one extra
 * widget row for the title at the top. All the text buttons are
 * label widgets. For performance reasons, they are overlaid by a text
 * widget when pressed.
 */

static short cell_x    [NCOLUMNS];
static short cell_xs   [NCOLUMNS];
static char *cell_name [NCOLUMNS] = { "Month", "Week", "Day", "YrOv", "Alarm",
				      "Group", "Name", "Server", "Local path",
				      "Server Host"};
static char *cell_help [NCOLUMNS] = { "user_enable", "user_enable",
				      "user_enable", "user_enable",
				      "user_enable", "user_color", "user_name",
				      "user_home", "user_home", "user_home" };

void create_user_rows(void)
{
	Widget			w, *urow;
	int			nrows = nusers+5 - nusers%5;
	int			x, y, xo;
	Arg			args[20];
	int			n;
	int			align = XmALIGNMENT_CENTER;
	BOOL			shadow = FALSE;
	String			callbk = 0;
	char			*name;
	WidgetClass		class = 0;

	if (!cell_x[0])
		for (x=4, n=0; n < NCOLUMNS; x+=cell_xs[n++]) {
			cell_x[n]  = x;
			cell_xs[n] = n == C_NAME || n == C_SERVER ? 100 :
				     n == C_PATH		  ? 220 : 45;
		}
	if (!have_shell)				/* check # of rows: */
		have_nrows = 0;
	if (nrows <= have_nrows)
		return;

	n = (nrows+1) * (NCOLUMNS+2) * sizeof(Widget *);
	if ((utable && !(utable = (Widget (*)[])realloc(utable, n))) ||
	   (!utable && !(utable = (Widget (*)[])malloc(n))))
		fatal(_("no memory"));
							/* build list */
	XtUnmanageChild(ulist);
	for (y=have_nrows; y <= nrows; y++) {
	    urow = utable[y];
	    n = 0;
	    XtSetArg(args[n], XmNtopAttachment,		XmATTACH_FORM);   n++;
	    XtSetArg(args[n], XmNheight,		30);		  n++;
	    XtSetArg(args[n], XmNy,			10 + 30 * y);	  n++;
	    XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);   n++;
	    urow[NCOLUMNS] = XtCreateWidget("userrowform",
	   				 xmFormWidgetClass, ulist, args, n);
	    for (x=0; x < NCOLUMNS; x++) {
		xo = n = 0;
		if (y) {
			switch(x) {
			  case C_EN_MONTH:
			  case C_EN_WEEK:
			  case C_EN_DAY:
			  case C_EN_YROV:
			  case C_EN_ALARM:
			  case C_EN_SRV:
				shadow = FALSE;
				class  = xmToggleButtonWidgetClass;
				callbk = XmNvalueChangedCallback;
				align  = XmALIGNMENT_CENTER;
				xo     = 12;
				XtSetArg(args[n], XmNselectColor,
							color[COL_TOGGLE]);n++;
				break;
			  case C_GROUP:
				shadow = TRUE;
				class  = xmPushButtonWidgetClass;
				callbk = XmNactivateCallback;
				align  = XmALIGNMENT_CENTER;
				if (y > 1)
					XtSetArg(args[n], XmNlabelType,
							XmPIXMAP); n++;
				break;
			  case C_NAME:
			  case C_PATH:
			  case C_SERVER:
				shadow = TRUE;
				class  = xmPushButtonWidgetClass;
				callbk = XmNactivateCallback;
				align  = XmALIGNMENT_BEGINNING;
			}
			name   = " ";
		} else {
			shadow = FALSE;
			class  = xmLabelWidgetClass;
			align  = XmALIGNMENT_CENTER;
			name   = _(cell_name[x]);
		}
		XtSetArg(args[n], XmNx,			cell_x[x]+xo);  n++;
		XtSetArg(args[n], XmNwidth,		cell_xs[x]-xo);	n++;
		XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM); n++;
		XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);	n++;
#if LESSTIF_VERSION != 0
		XtSetArg(args[n], XmNleftAttachment,	XmATTACH_SELF); n++;
		XtSetArg(args[n], XmNrightAttachment,	XmATTACH_SELF); n++;
#endif
		XtSetArg(args[n], XmNalignment,         align);		n++;
		XtSetArg(args[n], XmNrecomputeSize,	False);		n++;
		XtSetArg(args[n], XmNtraversalOn,	True);		n++;
		XtSetArg(args[n], XmNhighlightThickness,0);		n++;
		XtSetArg(args[n], XmNshadowThickness,	shadow);	n++;
		urow[x] = XtCreateManagedWidget(name, class,
						urow[NCOLUMNS], args, n);
#ifdef XmCSimpleOptionMenu
		if (y && x == C_GROUP) {
			XtSetArg(args[0], XmNbackground, color[COL_CALBACK]);
			urow[COLPOPUP] = XmCreatePopupMenu(urow[x],
							"colorpopup", args, 1);
			for (n=0; n < 16; n++) {
				XtSetArg(args[0], XmNlabelType, XmPIXMAP);
				XtSetArg(args[1], XmNbackground,
					color[COL_CALBACK]);
				XtSetArg(args[2], XmNlabelPixmap,
					(n>7 ? chk_pixmap : col_pixmap)[n&7]);
				w = XtCreateManagedWidget("pcolor",
					xmPushButtonWidgetClass,
					urow[COLPOPUP], args, 3);
				XtAddCallback(w, XmNactivateCallback,
					(XtCallbackProc)list_callback,
					(XtPointer)(size_t)(C_GROUP
							+ y * NCOLUMNS
							+ (n+1) * 65536));
			}
			XtAddEventHandler(urow[x], ButtonPressMask, False,
					(XtEventHandler)call_popup_callback,
					(XtPointer)(size_t)y);
		} else
#endif
		if (y)
			XtAddCallback(urow[x], callbk,
					(XtCallbackProc)list_callback,
			   		(XtPointer)(size_t)(x + y * NCOLUMNS));
		XtAddCallback(urow[x], XmNhelpCallback,
					(XtCallbackProc)help_callback,
					(XtPointer)cell_help[x]);
	    }
	    XtManageChild(urow[NCOLUMNS]);
	}
	for (y=have_nrows; y <= nrows; y++)
		draw_user_row(y);
	have_nrows = nrows;
	XtManageChild(ulist);
}


/*-------------------------------------------------- editing ----------------*/
/*
 * return a default host name for row y (1=own)
 */

static char *default_server(
	char		*buf,		/* name buffer, static wastes space */
	int		bufsz,		/* size of buffer */
	int		y)		/* for which row, 1=own */
{
	return(y == 1 ? gethostname(buf, bufsz)	? "localhost"
						: buf
		      : user[0].server		? user[0].server
						: "localhost");
}


/*
 * turn a text label into a Text button, to allow user input. This is done
 * by simply installing a text widget on top of the label widget. The proper
 * user name or home dir is put into the text widget. The previously edited
 * button is un-edited.
 */

static void edit_user_button(
	BOOL		doedit,		/* TRUE=edit, FALSE=unedit */
	int		x,		/* column, 0..NCOLUMNS-1* */
	int		y)		/* row, y=0: title */
{
	struct user	*u;
	Arg		args[15];
	int		n;
	char		*text, buf[1024];

	if (textwidget) {
		BOOL free_string = TRUE;
		char *string = XmTextGetString(textwidget);
		while (*string == ' ' || *string == '\t') string++;
		if (!*string) {
			free_string = FALSE;
			switch(xedit) {
			  case C_NAME: {
				struct passwd *pw = getpwuid(getuid());
				string = pw ? pw->pw_name : getenv("LOGNAME");
				break; }
			  case C_PATH:
			  	break;
			  case C_SERVER:
				string = default_server(buf, sizeof(buf), y);
			}
		}
		got_user_text(xedit, yedit,
				string && *string ? string : "unknown");
		if (free_string)
			XtFree(string);
		XtDestroyWidget(textwidget);
		if (yedit && yedit <= nusers)
			user[yedit-1].suspend_m =
			user[yedit-1].suspend_w =
			user[yedit-1].suspend_o =
			user[yedit-1].suspend_d = 0;
		draw_user_row(yedit);
		create_user_rows();
	}
	textwidget = 0;
	if (!doedit)
		return;

	if (y > nusers+1)
		y = nusers+1;
	u = &user[y-1];
	n = 0;
	XtSetArg(args[n], XmNx,			10 + cell_x[x]);	n++;
	XtSetArg(args[n], XmNy,			10 + 30*y);		n++;
	XtSetArg(args[n], XmNwidth,		cell_xs[x]);		n++;
	XtSetArg(args[n], XmNheight,		30);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNshadowThickness,	1);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	textwidget = XtCreateManagedWidget("text", xmTextWidgetClass,
						ulist, args, n);
	XtAddCallback(textwidget, XmNactivateCallback, (XtCallbackProc)
			got_text, (XtPointer)(size_t)(x + y * NCOLUMNS));
	XtAddCallback(textwidget, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)cell_help[x]);
	XmProcessTraversal(textwidget, XmTRAVERSE_CURRENT);

	text =  y > nusers    ? ""	  :
		x == C_NAME   ? u->name   :
		x == C_PATH   ? u->path   :
		x == C_SERVER ? u->server : "";
	print_text_button(textwidget, "%s", text);
	xedit = x;
	yedit = y;
}


/*
 * the user has entered text into a name, path, or server text button.
 * Check & store.
 */

static void got_user_text(
	int		x,		/* column, 0..NCOLUMNS-1* */
	int		y,		/* row, y=0: title */
	char		*string)	/* text entered by user */
{
	struct user	*u;
	char		buf[100], *p;
	int		i;

	if (!y--)
		return;
	while (*string == ' ' || *string == '\t')
		string++;
	while ((i = strlen(string)) && string[i-1] == ' ')
		string[i-1] = 0;
	if (!*string)
		return;
	for (i=0; i < nusers; i++) {
		if (x == C_NAME && i != y && !strcmp(string, user[i].name)) {
			create_error_popup(shell, 0,
				_("Name is not unique, rejected"));
			return;
		}
		if (x == C_PATH && i != y && !strcmp(string, user[i].path)) {
			create_error_popup(shell, 0,
				_("Path is not unique, rejected"));
			return;
		}
	}
	if (y >= nusers) {
		int n = ++nusers * sizeof(struct user);
		if ((user && !(user = (struct user *)realloc(user, n))) ||
		   (!user && !(user = (struct user *)malloc(n))))
			fatal(_("no memory"));
		y = nusers-1;
		u = user + y;
		memset(u, 0, sizeof(struct user));
		u->file_id = -1;
		u->fromserver = user[nusers > 2].fromserver;
	}
	u = user+y;
	u->modified = TRUE;
	switch(x) {
	  case C_NAME:
		if (u->name)
			free(u->name);
		u->name = mystrdup(string);
		for (p=u->name; *p; p++)	/* netplan can't mkdir */
			if (*p == '/')
				*p = '_';
		break;

	  case C_PATH:
		if (u->path)
			free(u->path);
		u->path = mystrdup(string);
		u->fromserver = FALSE;
		if (*u->path && access(u->path, F_OK))
			print_button(info, "%s: %s", u->path, errno==ENOENT ?
					_("new file") : _("cannot access"));
		break;

	  case C_SERVER:
		if (u->server)
			free(u->server);
		u->server = mystrdup(string);
		u->fromserver = TRUE;
	}
							/* default path? */
	if (u->name && *u->name && !u->path) {
		if (getpwnam(u->name)) {
			sprintf(buf, "~%s", u->name);
			u->path = mystrdup(buf);
		} else {
			sprintf(buf, "%s/%s", LIB, u->name);
			print_button(info, _("no user \"%s\""), u->name);
			u->path = mystrdup(buf);
		}
	}
							/* default server? */
	if (u->name && *u->name && !u->server)
		u->server = mystrdup(default_server(buf, sizeof(buf), y));
}


/*
 * draw all buttons of row y. y must be > 0 because 0 is the title row.
 * If y is > nusers, the row is blanked.
 */

void draw_user_row(
	int		y)
{
	Widget		*urow = utable[y];
	Arg		args[3];
	struct user	*u = &user[y-1];

	if (y < 1)
		return;
	if (y <= nusers) {					/* valid row */
		XtSetArg(args[0], XmNset, !u->suspend_m);
		XtSetValues (urow[C_EN_MONTH], args, 1);
		XtSetArg(args[0], XmNset, !u->suspend_w);
		XtSetValues (urow[C_EN_WEEK],  args, 1);
		XtSetArg(args[0], XmNset, !u->suspend_d);
		XtSetValues (urow[C_EN_DAY],   args, 1);
		XtSetArg(args[0], XmNset, !u->suspend_o);
		XtSetValues (urow[C_EN_YROV],  args, 1);
		XtSetArg(args[0], XmNset,  u->alarm);
		XtSetValues (urow[C_EN_ALARM], args, 1);
		XtSetArg(args[0], XmNset,  u->fromserver);
		XtSetValues (urow[C_EN_SRV],   args, 1);
		XtSetArg(args[0], XmNsensitive, !u->fromserver);
		XtSetValues(urow[C_PATH],      args, 1);
		XtSetArg(args[0], XmNsensitive, u->fromserver);
		XtSetValues(urow[C_SERVER],    args, 1);
		if (y == 1) {
			print_button(urow[C_GROUP], _("own"));
		} else {
#ifdef XmCSimpleOptionMenu
			XtSetArg(args[0], XmNbackground,  color[COL_CALBACK]);
			XtSetArg(args[1], XmNlabelPixmap, (u->color > 7 ?
					chk_pixmap : col_pixmap)[u->color&7]);
			XtSetValues(urow[C_GROUP], args, 2);
#endif
		}
		print_button(urow[C_NAME],   "%s", u->name   ? u->name : "?");
		print_button(urow[C_PATH],   "%s", u->path   ? u->path : "?");
		print_button(urow[C_SERVER], "%s", u->server ? u->server:"?");
	} else {
		XtSetArg(args[0], XmNset, 0);			/* blank row */
		XtSetValues (urow[C_EN_MONTH], args, 1);
		XtSetValues (urow[C_EN_WEEK],  args, 1);
		XtSetValues (urow[C_EN_DAY],   args, 1);
		XtSetValues (urow[C_EN_YROV],  args, 1);
		XtSetValues (urow[C_EN_ALARM], args, 1);
		XtSetValues (urow[C_EN_SRV],   args, 1);
		XtSetArg(args[0], XmNbackground,  color[COL_BACK]);
		XtSetArg(args[1], XmNlabelPixmap, pixmap[PIC_BLANK]);
		XtSetValues (urow[C_GROUP], args, 2);
		print_button(urow[C_GROUP],  " ");
		print_button(urow[C_NAME],   " ");
		print_button(urow[C_PATH],   " ");
		print_button(urow[C_SERVER], " ");
	}
}



/*-------------------------------------------------- callbacks --------------*/
/*
 * Delete, Add-all, Sort, and Done buttons
 * All of these routines are direct X callbacks.
 */

/*ARGSUSED*/
static void delete_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	int				n;
	Arg				args;

	if (ycurr == 1)
		print_button(info, _("Cannot delete own file"));

	else if (ycurr && ycurr <= nusers) {
		edit_user_button(FALSE, 0, 0);
		for (n=ycurr-1; n < nusers-1; n++)
			user[n] = user[n+1];
		n = --nusers * sizeof(struct user);
		if (n && !(user = (struct user *)realloc(user, n)))
			fatal(_("no memory"));
		for (n=1; n <= have_nrows; n++)
			draw_user_row(n);
	}
	if (ycurr <= 1) {
		XtSetArg(args, XmNsensitive, 0);
		XtSetValues(delete, &args, 1);
	}
}


#if defined(ULTRIX) || defined(MIPS)
		/* this means Ultrix 4.2A. If Ultrix 4.3 complains about */
		/* a missing const, change the following definition. */
#define CONST
#else
#define CONST const
#endif

static int compare(CONST void *u, CONST void *v) {
	return(  ((struct user *)u)->color == ((struct user *)v)->color
	? strcmp(((struct user *)u)->name,    ((struct user *)v)->name)
	:        ((struct user *)u)->color -  ((struct user *)v)->color); }

/*ARGSUSED*/
static void sort_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	Arg				args;
	int				n;

	if (nusers > 1) {
		edit_user_button(FALSE, 0, 0);
		XtSetArg(args, XmNsensitive, 0);
		XtSetValues(delete, &args, 1);
		qsort(user+1, nusers-1, sizeof(struct user), compare);
		for (n=1; n <= nusers; n++)
			draw_user_row(n);
	}
}


/*ARGSUSED*/
static void browse_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	edit_user_button(FALSE, 0, 0);
	create_serv_popup();
}


/*ARGSUSED*/
static void cancel_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	destroy_user_popup(FALSE);
}


/*ARGSUSED*/
static void done_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	destroy_user_popup(TRUE);
}


/*
 * the color column was pressed. Install a color popup. I couldn't figure out
 * how to post the menu with the left mouse button, and positioning it - the
 * XmNmenuPost resource probably doesn't work as expected. Fix later. This is
 * done only if Motif 1.2 or later is available. (Everybody is using 2.0 now.)
 */

#ifdef XmCSimpleOptionMenu
static Widget call_popup_callback(
	UNUSED Widget	widget,
	XtPointer	y,
        XEvent		*event,
	UNUSED Boolean	*cont)
{
	Widget popup = utable[(long)y][COLPOPUP];
	event->xbutton.x += 19;
	event->xbutton.y += 30 * user[(long)y-1].color + 4;
	XmMenuPosition(popup, (XButtonPressedEvent *)event);
	XtManageChild(popup);
	if (event->xbutton.button != 3)
		print_button(info, _("Use right mouse button to change color"));
	return(0);
}
#endif



/*
 * one of the buttons in the list was pressed. Remember that the line 0 is
 * the title, and line 1 is reserved for the user's own appointment file.
 * In the C_GROUP case, increment if the Motif version is older than 1.2,
 * or accept the result from the color popup otherwise.
 */

/*ARGSUSED*/
static void list_callback(
	UNUSED Widget			widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	int				x = (item & 65535) % NCOLUMNS;
	int				y = (item & 65535) / NCOLUMNS;
	struct user			*u = &user[y-1];
	Arg				arg;

	if (y > nusers) {					/* new entry */
		if (x != C_NAME) {
			print_button(info, _("Enter a name first"));
			return;
		}
		ycurr = 0;
		edit_user_button(TRUE, x, nusers+1);
	} else {						/* old entry */
		ycurr = y;
		switch(x) {
		  case C_EN_MONTH:
			u->suspend_m = !data->set;
			draw_calendar(NULL);
			break;

		  case C_EN_WEEK:
			u->suspend_w = !data->set;
			draw_week_calendar(NULL);
			break;

		  case C_EN_DAY:
			u->suspend_d = !data->set;
			draw_day_calendar(NULL);
			break;

		  case C_EN_YROV:
			u->suspend_o = !data->set;
			draw_yov_calendar(NULL);
			break;

		  case C_EN_ALARM:
			u->alarm = data->set;
			break;

		  case C_EN_SRV:
			u->fromserver = data->set;
			draw_user_row(y);
			break;

		  case C_GROUP:
			if (y > 1) {
				u->color = item < 65536 ? (u->color + 1) & 15
							: (item >> 16) - 1;
				XtSetArg(arg, XmNlabelPixmap, (u->color > 7 ?
					chk_pixmap : col_pixmap)[u->color&7]);
				XtSetValues(utable[y][x], &arg, 1);
			} else
				print_button(info,
						_("Cannot change own group"));
			break;

		  case C_PATH:
			if (y == 1) {
				print_button(info,_("Cannot change own path"));
				return;
			}
		  case C_NAME:
		  case C_SERVER:
			edit_user_button(TRUE, x, y);
		}
	}
	XtSetArg(arg, XmNsensitive, ycurr > 0);
	XtSetValues(delete, &arg, 1);
	print_button(info, " ");
}


/*
 * the user pressed Return in a text entry button
 */

/*ARGSUSED*/
static void got_text(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	edit_user_button(FALSE, 0, 0);
}
