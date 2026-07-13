/*
 * Create and destroy the server browse popup. It is installed when the
 * Browse button in the file list menu is pressed.
 *
 *	destroy_serv_popup()
 *	create_serv_popup()
 */

#include <stdio.h>
#include <time.h>
#ifndef MACOSX
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/LabelP.h>
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/TextF.h>
#include <Xm/List.h>
#include <Xm/Protocols.h>
#include "cal.h"

static void done_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void add_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void text_callback	(Widget, int, XmToggleButtonCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern Widget		toplevel;	/* top-level shell for icon name */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern struct config	config;		/* global configuration data */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern struct user	*user;		/* user list (from file_r.c) */
extern int		nusers;		/* number of users in user list */
extern int		maxusers;	/* max # of users in u list */

static BOOL		have_shell;	/* message popup exists if TRUE */
static Widget		shell;		/* popup menu shell */
static Widget		text;		/* server name */
static Widget		list;		/* file list */
static char		**filelist;	/* list of file names on server */
static XmString		*liststrings;	/* list of file names shown in list */
static int		nfiles;		/* # of entries in filelist */
static char		*hname;		/* server host name to add from */


/*
 * destroy a popup. Remove it from the screen, and destroy its widgets.
 * Detsroying the list displayed in the list widget is a separate function
 * because it's also used when a new host is selected.
 */

static void destroy_serv_list(void)
{
	int		i;

	if (nfiles) {
		for (i=0; i < nfiles; i++)
			free(filelist[i]);
		free(filelist);
		if (liststrings) {
			for (i=0; i < nfiles; i++)
				XmStringFree(liststrings[i]);
			free(liststrings);
			liststrings = 0;
		}
		nfiles = 0;
	}
}

void destroy_serv_popup(void)
{
	if (have_shell)
		XtPopdown(shell);
	have_shell = FALSE;
	destroy_serv_list();
}


/*
 * create a server popup as a separate application shell. When the popup
 * already exists and was just popped down, pop it up and exit. This
 * ensures that it comes up with the same text.
 */

void create_serv_popup(void)
{
	Widget			form, w, ww;
	Arg			args[20];
	int			n, m;
	Atom			closewindow;

	if (have_shell) {
		XtPopup(shell, XtGrabNone);
		return;
	}
	for (m=0; m < 2 && !hname; m++)
		for (n=0; n < nusers && !hname; n++)
			if (m || user[n].fromserver)
				hname = user[n].server;
	if (!hname)
		hname = "localhost";
	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	  XmDO_NOTHING);	n++;
	XtSetArg(args[n], XmNiconic,		  False);		n++;
	XtSetArg(args[n], XmNkeyboardFocusPolicy, XmEXPLICIT);		n++;
	shell = XtCreatePopupShell(_("Browse Server"),
			applicationShellWidgetClass, toplevel, args, n);
#	ifdef EDITRES
	XtAddEventHandler(shell, (EventMask)0, TRUE, 
 			_XEditResCheckMessages, NULL);
#	endif
	set_icon(shell, 1);
	form = XtCreateManagedWidget("servform", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"serv");

						/*--------- server ----------*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	w = XtCreateManagedWidget(_("netplan server to query:"),
			xmLabelWidgetClass, form, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	16);			n++;
	XtSetArg(args[n], XmNwidth,		300);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	text = XtCreateManagedWidget(" ",
			xmTextFieldWidgetClass, form, args, n);
	print_text_button(text, "%s", hname);
	XtAddCallback(text, XmNactivateCallback, (XtCallbackProc)
			text_callback, (XtPointer)NULL);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		text);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	ww = XtCreateManagedWidget(_("Available files, select and Add:"),
			xmLabelWidgetClass, form, args, n);

						/*--------- buttons ---------*/
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	16);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Done"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			done_callback, (XtPointer)0);
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Help"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			help_callback, (XtPointer)"serv");
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Add"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			add_callback, (XtPointer)0);
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Query"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			text_callback, (XtPointer)0);

						/*--------- list ------------*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		ww);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	w);			n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	16);			n++;
	XtSetArg(args[n], XmNselectionPolicy,	XmEXTENDED_SELECT);	n++;
	/* XtSetArg(args[n], XmNselectColor,	XmHIGHLIGHT_COLOR);	n++; */
	XtSetArg(args[n], XmNvisibleItemCount,	10);			n++;
	XtSetArg(args[n], XmNitemCount,		0);			n++;
	XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmSTATIC);		n++;
	list = XmCreateScrolledList(form, "servlist",
			args, n);
	XtAddCallback(list, XmNdefaultActionCallback,
				(XtCallbackProc)add_callback, (XtPointer)0);
	XmListSelectPos(list, 1, False);

	XtManageChild(list);
	XmProcessTraversal(text, XmTRAVERSE_CURRENT);
	XtPopup(shell, XtGrabNone);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow,
			(XtCallbackProc)done_callback, (XtPointer)shell);
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
	destroy_serv_popup();
}


/*ARGSUSED*/
static void text_callback(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	int				i;
	Arg				args[2];
	char				*string;

	string = XmTextFieldGetString(widget);
	if (!*string) {
		XtFree(string);
		return;
	}
	hname = string;
	destroy_serv_list();
	if (!get_netplan_filelist(string, &filelist, &nfiles))
		create_error_popup(shell, 0,
			_("Cannot connect to netplan\nserver host %s"),string);

	if (!(liststrings = malloc(nfiles * sizeof(XmString))))
		fatal("no memory");
	for (i=0; i < nfiles; i++)
		liststrings[i] = XmStringCreate(filelist[i],
						XmFONTLIST_DEFAULT_TAG);
	XtSetArg(args[0], XmNitemCount, nfiles);
	XtSetArg(args[1], XmNitems,     liststrings);
	XtSetValues(list, args, 2);
	XmListSelectPos(list, 1, True);
}


/*ARGSUSED*/
static void add_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	int				n, i, u, eq, *pos;
	char				buf[1024];
#ifdef XmNselectedPositionCount
	Arg				args[2];

	XtSetArg(args[0], XmNselectedPositionCount, &n);
	XtSetArg(args[1], XmNselectedPositions,     &pos);
	XtGetValues(list, args, 2);
#else
	XmListGetSelectedPos(list, &pos, &n);
#endif
	for (i=0; i < n; i++) {
		for (eq=u=0; u < nusers && !eq; u++)
			eq = user[u].fromserver			&&
			     user[u].server     		&&
			     !strcmp(user[u].server, hname)	&&
			     !strcmp(user[u].name,   filelist[pos[i]-1]);
		if (eq)
			continue;
		if (nusers >= maxusers) {
			u = nusers + 10;
			user = user ? realloc(user, u * sizeof(struct user))
				    : malloc(u * sizeof(struct user));
			if (!user)
				fatal(_("no memory"));
			memset(user+maxusers, 0,
					(u-maxusers) * sizeof(struct user));
			maxusers = u;
		}
		sprintf(buf, "~/.dayplan.%.1000s", filelist[pos[i]-1]);
		user[nusers].alarm	= TRUE;
		user[nusers].fromserver	= TRUE;
		user[nusers].name	= mystrdup(filelist[pos[i]-1]);
		user[nusers].path	= mystrdup(buf);
		user[nusers].server	= mystrdup(hname);
		user[nusers].color	= user[nusers-1].color + 1;
		user[nusers].file_id	= -1;
		user[nusers].modified	= TRUE;
		nusers++;
		draw_user_row(nusers);
		create_user_rows();
	}
}
