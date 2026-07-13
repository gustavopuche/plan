/*
 * Various popups, such as the About... dialog.
 *
 *	create_about_popup()
 *					Create About info popup
 *	create_error_popup(widget, error, fmt, ...)
 *					Create error popup with Unix error
 *	create_nodaemon_popup()
 *					Create no-daemon warning popup
 *	create_multiple_popup()
 *					Create multiple-plan's warning popup
 *	multiple_writer_warning_popup()
 *					Create popup with write warning
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#if defined(__EMX__) || defined(LINUX) || defined(MACOSX)
#include <sys/wait.h>
#endif
#include <Xm/Xm.h>
#include <Xm/MessageB.h>
#include <Xm/Protocols.h>
#include "cal.h"
#include "version.h"

#define NODAEMON_ONCE		/* if defined, the error popup that offers */
				/* to start pland comes up only once. */
static Boolean suppress_daemon_warning = False;

#ifndef __DATE__
#define __DATE__ ""
#endif

extern char		*progname;	/* argv[0] */
extern Display		*display;	/* everybody uses the same server */
extern Widget		mainwindow;	/* popup menus hang off main window */
extern XtAppContext	app;		/* application handle */
extern BOOL		interactive;	/* interactive or fast appt entry? */
extern BOOL		startup_lock(BOOL, BOOL);


/*---------------------------------------------------------- about ----------*/
static char about_message[] = "\
\n\
Day Planner version %s\n\
Compiled %s\n\n\
Author: Thomas Driemeyer\n\
<thomas@bitrot.de>\n\
http://www.bitrot.de\n\n\
The sources are available at\n\
ftp://ftp.bitrot.de and\n\
ftp://plan.ftp.fu-berlin.de\
\n";


void create_about_popup(void)
{
	char		msg[512];
	Widget		dialog;
	XmString	s;
	Arg		args[10];
	int		n;

	sprintf(msg, _(about_message), VERSION JAPANVERSION, __DATE__);
	s = XmStringCreateLtoR(msg, XmSTRING_DEFAULT_CHARSET);
	n = 0;
	XtSetArg(args[n], XmNmessageString,	s);			n++;
	dialog = XmCreateInformationDialog(mainwindow, _("About"), args, n);
	XmStringFree(s);
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
	(void)XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XtManageChild(dialog);
}


/*---------------------------------------------------------- errors ---------*/
void create_error_popup(
	Widget		widget,
	UNUSED int	error,
	char		*fmt, ...)
{
	va_list		parm;
	char		msg[10240];
	Widget		dialog;
	XmString	string;
	Arg		args;

	if (!widget)
		widget = mainwindow;
	strcpy(msg, _("ERROR:\n\n"));

	va_start(parm, fmt);
	vsprintf(msg + strlen(msg), fmt, parm);
	va_end(parm);
#	if defined(sgi) || defined(_sgi)
	if (error) {
		strcat(msg, "\n");
		strcat(msg, sys_errlist[error]);
	}
#	endif
	if (!interactive || !widget) {
		fputs(msg, stderr);
		return;
	}
	string = XmStringCreateLtoR(msg, XmSTRING_DEFAULT_CHARSET);
	XtSetArg(args, XmNmessageString, string);
	dialog = XmCreateWarningDialog(widget, _("Error"), &args, 1);
	XmStringFree(string);
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
	(void)XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XtManageChild(dialog);
}


/*---------------------------------------------------------- no daemon ------*/
static char nodaemon_message[] = "\
Warning:\n\
There is no Scheduler daemon. Without a daemon,\n\
no action will be taken when an alarm triggers.\n\
Please start \"%s\" in your ~/.xsession or\n\
~/.sgisession file, or run plan with the -S option.";

void run_daemon(Widget dialog);
static void cancel_callback(Widget dialog);
static BOOL running;

void create_nodaemon_popup(void)
{
	char		msg[512];
	Widget		dialog;
	XmString	s1, s2, s3;
	Arg		args[10];
	int		n;

	if (running)
		return;
	running = TRUE;
#ifdef NODAEMON_ONCE
	if (suppress_daemon_warning)
		return;
#endif
	sprintf(msg, _(nodaemon_message), DAEMON_FN);
	s1 = XmStringCreateLtoR(msg, XmSTRING_DEFAULT_CHARSET);
	s2 = XmStringCreateSimple(_("Start daemon now"));
	s3 = XmStringCreateSimple(_("Continue without daemon"));
	n = 0;
	XtSetArg(args[n], XmNmessageString,	s1);		n++;
	XtSetArg(args[n], XmNokLabelString,	s2);		n++;
	XtSetArg(args[n], XmNcancelLabelString,	s3);		n++;
	dialog = XmCreateWarningDialog(mainwindow, _("Warning"), args, n);
	XmStringFree(s1);
	XmStringFree(s2);
	XmStringFree(s3);
	XtAddCallback(dialog, XmNokCallback, (XtCallbackProc)
			run_daemon, (XtPointer)NULL);
	XtAddCallback(dialog, XmNcancelCallback, (XtCallbackProc)
			cancel_callback, (XtPointer)NULL);
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
	(void)XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XtManageChild(dialog);
}


/*
 * start the daemon. Nohup the daemon process so that it survives the death
 * of its parent. Also used by the -S autostart option. pland immediately
 * backgrounds itself, so the main process must call wait() to avoid zombies.
 */

/*ARGSUSED*/
void run_daemon(
	UNUSED Widget	dialog)		/* not used */
{
	char		path[1024];
	PID_T		pid;

	running = FALSE;
	if (!find_file(path, DAEMON_FN, TRUE)) {
		fprintf(stderr, _("%s: WARNING: can't find %s\n"),
							progname, DAEMON_FN);
		create_nodaemon_popup();
		return;
	}
	pid = fork();
	if (pid > 0) {
		wait(0);
		return;
	}
	if (pid == 0) {
		fprintf(stderr, _("%s: starting %s\n"), progname, DAEMON_FN);
#if defined(BSD) || defined(MIPS)
		setpgrp(0, 0);
#else
#ifdef __EMX__

#else
		setsid();
#endif
#endif
		detach_from_network();
		execl(path, DAEMON_FN, (char *)0);
	}
	fprintf(stderr, _("%s: WARNING: can't start %s: "), progname,
								DAEMON_FN);
	perror("");
	create_nodaemon_popup();
}


/*
 * don't start the daemon, and set the flag that prevents the popup in the
 * future.
 */

/*ARGSUSED*/
static void cancel_callback(
	UNUSED Widget	dialog)
{
	running = FALSE;
	suppress_daemon_warning = True;
}


/*---------------------------------------------------------- multiple plan's */
static char multiple_message[] = "\
Warning:\n\
Another %s program is running.\n\
%s\n\
Press Continue to start up anyway, or Kill to\n\
attempt to kill the other program. Continuing\n\
may cause loss of new appointment entries, and\n\
command-line entry of appointments will not work.";

static void kill_callback(Widget, XtPointer, XtPointer);
static void cont_callback(Widget, XtPointer, XtPointer);
static BOOL cont;

void create_multiple_popup(
	BOOL		nolock)		/* tried to kill competitor */
{
	char		msg[512];
	Widget		dialog;
	XmString	s1, s2, s3;
	Arg		args[10];
	int		n;

	cont = FALSE;
	sprintf(msg, _(multiple_message), progname,
		nolock ? _("The other program could not be killed.\n") : "");
	s1 = XmStringCreateLtoR(msg, XmSTRING_DEFAULT_CHARSET);
	s2 = XmStringCreateSimple(_("Kill"));
	s3 = XmStringCreateSimple(_("Continue"));
	n = 0;
	XtSetArg(args[n], XmNmessageString,	s1);		n++;
	XtSetArg(args[n], XmNokLabelString,	s2);		n++;
	XtSetArg(args[n], XmNcancelLabelString,	s3);		n++;
	XtSetArg(args[n], XmNdialogStyle,
			  XmDIALOG_FULL_APPLICATION_MODAL);	n++;
	dialog = XmCreateWarningDialog(mainwindow, _("Warning"), args, n);
	XmStringFree(s1);
	XmStringFree(s2);
	XmStringFree(s3);
	XtAddCallback(dialog, XmNokCallback, (XtCallbackProc)
			kill_callback, (XtPointer)NULL);
	XtAddCallback(dialog, XmNcancelCallback, (XtCallbackProc)
			cont_callback, (XtPointer)NULL);
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
	(void)XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XtManageChild(dialog);
	while (!cont) {
		XEvent event;
		XtAppNextEvent(app, &event);
		XtDispatchEvent(&event);
	}
}


/*
 * The user pressed Kill. Try to acquire a lock, then let
 * create_multiple_popup continue so that the main plan window can come up.
 */

/*ARGSUSED*/
static void kill_callback(
	UNUSED Widget	 dialog,
	UNUSED XtPointer a,
	UNUSED XtPointer b)
{
	if (!startup_lock(FALSE, TRUE))
		create_multiple_popup(TRUE);
	cont = TRUE;
}


/*
 * The user pressed Continue. Let create_multiple_popup continue so that the
 * main plan window can come up.
 */

/*ARGSUSED*/
static void cont_callback(
	UNUSED Widget	 dialog,
	UNUSED XtPointer a,
	UNUSED XtPointer b)
{
	cont = TRUE;
}


/*---------------------------------------------------------- multiple wr ----*/
void multiple_writer_warning_popup(
	Widget		widget)		/* install near this widget */
{
	Widget		dialog;
	XmString	string;
	Arg		args[2];

	string = XmStringCreateLtoR(
_("WARNING:\n\n\
You can now edit other users' appointments.\n\
plan will normally notice multiple writes at the same\n\
time (except over NFS), but will not notice if another\n\
user has modified the same appointment file while you\n\
were changing it. The older changes will get lost in\n\
this case. Watch out!)"), XmSTRING_DEFAULT_CHARSET);
	XtSetArg(args[0], XmNmessageString, string);
	XtSetArg(args[1], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
	dialog = XmCreateWarningDialog(widget, _("Error"), args, 2);
	XmStringFree(string);
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
	(void)XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XtManageChild(dialog);
}


/*---------------------------------------------------------- multiple wr ----*/
void new_language_popup(
	Widget		widget)		/* install near this widget */
{
	Widget		dialog;
	XmString	string;
	Arg		args[2];

	string = XmStringCreateLtoR(_("The new language will be effective\n"
			"after plan is restarted."), XmSTRING_DEFAULT_CHARSET);
	XtSetArg(args[0], XmNmessageString, string);
	XtSetArg(args[1], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
	dialog = XmCreateWarningDialog(widget, _("Warning"), args, 2);
	XmStringFree(string);
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
	(void)XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XtManageChild(dialog);
}
