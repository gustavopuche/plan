/*
 * user selection popup. This is used in the appointment entry when the
 * group column buttons are pressed, and when the <other> button in the
 * year overview menu is pressed. The popup contains one button for every
 * user defined in the user definition menu (usermenu.c).
 *
 *	destroy_user_sel_popup()
 *	create_user_sel_popup(widget, curr, callback)
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/LabelP.h>
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/Protocols.h>
#include "cal.h"


static void user_sel_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void abort_callback	(Widget, int, XmToggleButtonCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern struct config	config;		/* global configuration data */
extern struct user	*user;		/* user list (from file_r.c) */
extern int		nusers;		/* number of users in array */
static BOOL		(*callback)(int); /* callback when user makes selection*/
static int		curr_user;	/* previous (default) user ID */


/*
 * destroy user selection popup. Remove it from the screen, and destroy
 * its widgets.
 */

static Widget		shell;			/* popup menu shell */

void destroy_user_sel_popup(void)
{
	if (shell) {
		XtPopdown(shell);
		XTDESTROYWIDGET(shell);
		shell = 0;
	}
}


/*
 * create a user selection poup over button <widget>, with one selection
 * button per writable user in it. When the user presses one, reclassify
 * the appointment.
 */

void create_user_sel_popup(
	UNUSED Widget	widget,		/* install popup near this button */
	int		curr,		/* current user ID (default) */
	BOOL		(*callb)())	/* function to call with new user ID */
{
	Widget		form, radio, w, done;
	Arg		args[20];
	int		n;
	Atom		closewindow;
	int		u;
	char		buf[1024];

	destroy_user_sel_popup();
	curr_user = curr;
	callback  = callb;

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtAppCreateShell(_("Group Selection"), "plan",
			applicationShellWidgetClass, display, args, n);
#	ifdef EDITRES
	XtAddEventHandler(shell, (EventMask)0, TRUE, 
 			_XEditResCheckMessages, NULL);
#	endif
	set_icon(shell, 1);
	form = XtCreateManagedWidget("selform", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"day_user");
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	6);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	6);			n++;
	w = XtCreateManagedWidget(_("Available files:"), xmLabelWidgetClass,
			form, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	6);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	6);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNspacing,		0);			n++;
	XtSetArg(args[n], XmNnumColumns,	nusers/20+1);		n++;
	radio = XmCreateRadioBox(form, "radio", args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		radio);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	6);			n++;
	XtSetArg(args[n], XmNleftOffset,	32);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	done = XtCreateManagedWidget(_("Done"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(done, XmNactivateCallback, (XtCallbackProc)
			abort_callback, (XtPointer)(size_t)0);

	for (u=0; u < nusers; u++) {
	   sprintf(buf, "%s%s%s", user[u].name,
				  u == 0 ? _(" (own)") : "",
				  user[u].readonly ? _(" (readonly)") : "");
	   n = 0;
	   XtSetArg(args[n], XmNindicatorType,	   XmONE_OF_MANY);	n++;
	   XtSetArg(args[n], XmNselectColor,	   color[COL_TOGGLE]);	n++;
	   XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	   XtSetArg(args[n], XmNset,		   u == curr_user);	n++;
	   w = XtCreateManagedWidget(buf,
			   xmToggleButtonWidgetClass, radio, args, n);
	   XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
	   		user_sel_callback, (XtPointer)(size_t)u);
	}
	XtManageChild(radio);
	XtPopup(shell, XtGrabNone);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow, (XtCallbackProc)
			abort_callback, (XtPointer)shell);
}


/*
 * user pressed one of the buttons in the user selection popup. This
 * routine is always called twice by the Motif toolkit: once for deselecting
 * the old choice, and once for the new choice. This is true even if both
 * are the same.
 */

/*ARGSUSED*/
static void user_sel_callback(
	UNUSED Widget			widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	if (!data->set)
		return;
	if (item != curr_user)
		(*callback)(item);
	destroy_user_sel_popup();
}

/*ARGSUSED*/
static void abort_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	destroy_user_sel_popup();
}
