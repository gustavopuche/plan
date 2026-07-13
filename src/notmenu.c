/*
 * Create the notifier window and everything in it. Only the Dismiss button
 * has a callback, which terminates everything. The <msg> argument points
 * to the text to fill the scrollable text window with; it is read-only.
 */

#include <stdio.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/Form.h>
#include <Xm/LabelP.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/Text.h>
#include "notifier.h"

#define SNOOZE_PERIOD	5*60*1000	/* snooze time in milliseconds */

/* gcc soesn't support turning off stupid warning messages selectively */
#if defined(__GNUC__)
#define UNUSED __attribute__((unused))	/* Flag variable as unused */
#else /* not __GNUC__ */
#define UNUSED
#endif

static void snooze_callback	(Widget, int, XmToggleButtonCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern XtAppContext	app;		/* application handle for timer */
extern GC		gc;		/* everybody uses this context */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern Pixel		bkcolor;	/* background color (COL_*) */
static Widget		mainwindow;	/* for popdown/popup by snooze */


void create_widgets(
	Widget		toplevel,
	char		*title,		/* title string */
	char		*subtitle,	/* subtitle string */
	char		*msg)		/* message text */
{
	Widget		form, w, snooze, dismiss;
	XmString	string;
	Arg		args[20];
	int		n;
	int		nlines=0, nchars=0;
	char		*p;

	for (n=0, p=msg; *p; p++)		/* count x/y size of text */
		if (*p != '\n')
			n++;
		else {
			nlines++;
			if (n > nchars)
				nchars = n;
			n = 0;
		}
	if (n) {
		nlines++;
		if (n > nchars)
			nchars = n;
	}
	if (nchars > 80)
		nchars = 80;
	if (nlines > 20)
		nlines = 20;

	mainwindow = XtCreateManagedWidget("mainwindow",
			xmMainWindowWidgetClass, toplevel, NULL, 0);
	n = 0;
	XtSetArg(args[n], XmNbackground,	color[bkcolor]);	n++;
	form = XtCreateWidget("form", xmFormWidgetClass,
			mainwindow, args, n);

	n = 0;							/* title */
	string = XmStringCreateSimple(title);
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		12);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNbackground,	color[bkcolor]);	n++;
	XtSetArg(args[n], XmNlabelString,	string);		n++;
	w = XtCreateManagedWidget("title", xmLabelWidgetClass,
			form, args, n);
	XmStringFree(string);

	if (subtitle && *subtitle) {				/* subtitle */
	  n = 0;
	  string = XmStringCreateSimple(subtitle);
	  XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	  XtSetArg(args[n], XmNtopWidget,	w);			n++;
	  XtSetArg(args[n], XmNtopOffset,	0);			n++;
	  XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	  XtSetArg(args[n], XmNleftOffset,	8);			n++;
	  XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	  XtSetArg(args[n], XmNrightOffset,	8);			n++;
	  XtSetArg(args[n], XmNbackground,	color[bkcolor]);	n++;
	  XtSetArg(args[n], XmNlabelString,	string);		n++;
	  w = XtCreateManagedWidget("subtitle", xmLabelWidgetClass,
			form, args, n);
	  XmStringFree(string);
	}

	n = 0;							/* snooze */
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	12);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	12);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[bkcolor]);	n++;
	snooze = XtCreateManagedWidget("Snooze 5 min", xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(snooze, XmNactivateCallback, (XtCallbackProc)
			snooze_callback, (XtPointer)0);

	n = 0;							/* dismiss */
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	12);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	snooze);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	12);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[bkcolor]);	n++;
	dismiss = XtCreateManagedWidget("Dismiss", xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(dismiss, XmNactivateCallback, (XtCallbackProc)
			exit, (XtPointer)0);

	n = 0;							/* message */
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		12);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	snooze);		n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	12);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	12);			n++;
	XtSetArg(args[n], XmNrows,		nlines);		n++;
	XtSetArg(args[n], XmNcolumns,		nchars+1);		n++;
	XtSetArg(args[n], XmNbackground,	color[bkcolor]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNeditable,		False);			n++;
	XtSetArg(args[n], XmNeditMode,		XmMULTI_LINE_EDIT);	n++;
	XtSetArg(args[n], XmNvalue,		msg);			n++;
	if (nlines) {
		w = XmCreateScrolledText(form, "message", args, n);
		XtManageChild(w);
	}

	XtManageChild(form);
}


/*
 * snooze was pressed. Pop the window down for five minutes.
 */

/*ARGSUSED*/
static void timer_callback(
	UNUSED XtPointer	data,		/* not used */
	UNUSED XtIntervalId	*id)		/* not used */
{
	XtPopup(XtParent(mainwindow), XtGrabNone);
	XBell(display, 0);
	XBell(display, 0);
}


/*ARGSUSED*/
static void snooze_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	XtPopdown(XtParent(mainwindow));
	XtUnmapWidget(XtParent(mainwindow));
	XtAppAddTimeOut(app, SNOOZE_PERIOD, timer_callback, 0);
}
