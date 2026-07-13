/*
 * Create and destroy the recycle popup. Recycling an entry means to specify
 * days on which it recurs; whenever its trigger time has passed, it it
 * changed to the next trigger date (see recycle()). The popup is installed
 * when the Recycle icon (looks like two round arrows) is pressed.
 *
 *	destroy_recycle_popup()
 *	create_recycle_popup()
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
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/Protocols.h>
#include "cal.h"

static void done_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void wkday_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void nth_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void day_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void every_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void every_text_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void yearly_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void last_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void last_text_callback	(Widget, int, XmToggleButtonCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern Widget		toplevel;	/* top-level shell for icon name */
extern struct edit	edit;		/* info about entry being edited */
extern struct config	config;		/* global configuration data */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern char		*weekday_name[];/* "Mon", "Tue", ... */

static BOOL		have_shell;	/* message popup exists if TRUE */
static Widget		shell;		/* popup menu shell */
static Widget		nth_widget[7];	/* any, 1st..5th, last toggle buttons*/
static Widget		last;		/* last-on date text */
static Widget		last_toggle;	/* last-on date toggle button */
static Widget		every;		/* every n days text */
static Widget		every_toggle;	/* every n days toggle button */
static Widget		yearly_toggle;	/* every year toggöe button */


/*
 * destroy a popup. Remove it from the screen, and destroy its widgets.
 * It's too much trouble to keep them for next time.
 */

void destroy_recycle_popup(void)
{
	char			*string;	/* contents of text widget */

	if (have_shell && edit.editing) {
		if (edit.entry.rep_every) {
			string = XmTextGetString(every);
			edit.entry.rep_every = atoi(string) * 86400;
			if (edit.entry.rep_every < 0)
				edit.entry.rep_every = 0;
			XtFree(string);
		}
		if (edit.entry.rep_last) {
			string = XmTextGetString(last);
			edit.entry.rep_last =
				parse_datestring(string, edit.entry.time);
			XtFree(string);
		}
		edit.changed = TRUE;
		edit.entry.suspended = FALSE;
		print_pixmap(edit.menu->entry[edit.y][SC_RECYCLE], PIC_RECYCLE,
			     edit.entry.rep_yearly || edit.entry.rep_days ||
			     edit.entry.rep_every  || edit.entry.rep_weekdays);
	}
	if (have_shell) {
		XtPopdown(shell);
		XTDESTROYWIDGET(shell);
		have_shell = FALSE;
		confirm_new_entry();
	}
}


/*
 * create a recycle popup as a separate application shell. The popup is
 * initialized with data from edit.entry.
 */

static char *nth_name[] = { "every...", "first...", "second...",
			    "third...", "fourth...", "fifth...", "last..." };

void create_recycle_popup(void)
{
	Widget		frame, frame2, form, form2, rowcol, w;
	int		day;		/* weekdays and month days */
	char		buf[10];	/* month day names */
	struct entry	*ep = &edit.entry;
	Arg		args[20];
	int		n;
	Atom		closewindow;

	destroy_recycle_popup();

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtCreatePopupShell(_("Schedule Recycler"),
			applicationShellWidgetClass, toplevel, args, n);
#	ifdef EDITRES
	XtAddEventHandler(shell, (EventMask)0, TRUE, 
 			_XEditResCheckMessages, NULL);
#	endif
	set_icon(shell, 1);
	form = XtCreateManagedWidget("cycform1", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"cyc"); 
							/*-- buttons --*/
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNleftOffset,	32);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Done"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			done_callback, (XtPointer)(size_t)0);
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer)"cyc_done");

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
			help_callback, (XtPointer)"cyc");
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback, (XtPointer)"cyc"); 
							/*-- last on --*/
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	w);			n++;
	XtSetArg(args[n], XmNbottomOffset,	20);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNshadowType,	XmSHADOW_ETCHED_IN);	n++;
	frame = XtCreateManagedWidget("", xmFrameWidgetClass,
			form, args, n);
	form2 = XtCreateManagedWidget("cycform2", xmFormWidgetClass,
			frame, NULL, 0);
	XtAddCallback(form2, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"cyc_last"); 
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		ep->rep_last != 0);	n++;
	last_toggle = XtCreateManagedWidget(_("Last day: stop repeating on"),
			xmToggleButtonWidgetClass,
			form2, args, n);
	XtAddCallback(last_toggle, XmNvalueChangedCallback, (XtCallbackProc)
			last_callback, (XtPointer)(size_t)0);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	last_toggle);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		140);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	last = XtCreateManagedWidget("", xmTextWidgetClass,
			form2, args, n);
	XtAddCallback(last, XmNactivateCallback, (XtCallbackProc)
			last_text_callback, (XtPointer)NULL);

							/*-- repeat every --*/
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	frame);			n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNshadowType,	XmSHADOW_ETCHED_IN);	n++;
	frame = XtCreateManagedWidget("", xmFrameWidgetClass,
			form, args, n);
	form2 = XtCreateManagedWidget("cycform3", xmFormWidgetClass,
			frame, NULL, 0);
	XtAddCallback(form2, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"cyc_every");

	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		ep->rep_every != 0);	n++;
	every_toggle = XtCreateManagedWidget(_("Repeat every"),
			xmToggleButtonWidgetClass,
			form2, args, n);
	XtAddCallback(every_toggle, XmNvalueChangedCallback, (XtCallbackProc)
			every_callback, (XtPointer)(size_t)0);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	every_toggle);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		60);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	every = XtCreateManagedWidget("", xmTextWidgetClass,
			form2, args, n);
	XtAddCallback(every, XmNactivateCallback, (XtCallbackProc)
			every_text_callback, (XtPointer)NULL);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	every);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	w = XtCreateManagedWidget(_("days"), xmLabelWidgetClass,
			form2, args, n);

							/*-- yearly --*/
	n = 0;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	40);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		ep->rep_yearly);	n++;
	yearly_toggle = XtCreateManagedWidget(_("Repeat every year"),
			xmToggleButtonWidgetClass,
			form2, args, n);
	XtAddCallback(yearly_toggle, XmNvalueChangedCallback, (XtCallbackProc)
			yearly_callback, (XtPointer)(size_t)0);
	XtAddCallback(yearly_toggle, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"cyc_yearly");

							/*-- weekdays --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	frame);			n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNshadowType,	XmSHADOW_ETCHED_IN);	n++;
	frame2 = XtCreateManagedWidget("", xmFrameWidgetClass,
			form, args, n);
	form2 = XtCreateManagedWidget("cycform4", xmFormWidgetClass,
			frame2, NULL, 0);
	XtAddCallback(form2, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"cyc_weekdays");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	w = XtCreateManagedWidget(_("Repeat on"), xmLabelWidgetClass,
			form2, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNspacing,		4);			n++;
	rowcol = XtCreateManagedWidget("", xmRowColumnWidgetClass,
			form2, args, n);
	for (day=0; day < 7; day++) {
	   BOOL on = day==0 ? !(ep->rep_weekdays & 0x3f00)
			    :  (ep->rep_weekdays & 0x0080 << day) != 0;
	   n = 0;
	   XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	   XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	   XtSetArg(args[n], XmNset,		on);			n++;
	   nth_widget[day] = XtCreateManagedWidget(_(nth_name[day]),
	  		xmToggleButtonWidgetClass,
			rowcol, args, n);
	   XtAddCallback(nth_widget[day], XmNvalueChangedCallback,
			(XtCallbackProc)nth_callback, (XtPointer)(size_t)day);
	}
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	rowcol);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNspacing,		4);			n++;
	rowcol = XtCreateManagedWidget("", xmRowColumnWidgetClass,
			form2, args, n);
	for (day=0; day < 7; day++) {
	   int d = config.sunday_first ? day : (day+1)%7;
	   BOOL on = (ep->rep_weekdays & (1<<d)) != 0;
	   n = 0;
	   XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	   XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	   XtSetArg(args[n], XmNset,		on);			n++;
	   w = XtCreateManagedWidget(_(weekday_name[(d+6)%7]),
	  		xmToggleButtonWidgetClass,
			rowcol, args, n);
	   XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			wkday_callback, (XtPointer)(size_t)d);
	}

							/*-- month days --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	frame);			n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	frame2);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNshadowType,	XmSHADOW_ETCHED_IN);	n++;
	frame2 = XtCreateManagedWidget("", xmFrameWidgetClass,
			form, args, n);
	form2 = XtCreateManagedWidget("cycform5", xmFormWidgetClass,
			frame2, NULL, 0);
	XtAddCallback(form2, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"cyc_days");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	w = XtCreateManagedWidget(_("Repeat on the"), xmLabelWidgetClass,
			form2, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNpacking,		XmPACK_COLUMN);		n++;
	XtSetArg(args[n], XmNnumColumns,	4);			n++;
	rowcol = XtCreateManagedWidget("", xmRowColumnWidgetClass,
			form2, args, n);
	for (day=1; day <= 32; day++) {
	   BOOL on = (ep->rep_days & (1<<(day&31))) != 0;
	   if (day == 32)
		strcpy(buf, _("Last"));
	   else
		sprintf(buf, "%d%s", day, (day%10)==1 && day!=11 ? _("st") :
					  (day%10)==2 && day!=12 ? _("nd") :
					  (day%10)==3 && day!=13 ? _("rd") :
					 			   _("th"));
	   n = 0;
	   XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	   XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	   XtSetArg(args[n], XmNset,		on);			n++;
	   w = XtCreateManagedWidget(buf, xmToggleButtonWidgetClass,
			rowcol, args, n);
	   XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
	  		day_callback, (XtPointer)(size_t)day);
	}

	XtPopup(shell, XtGrabNone);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow, (XtCallbackProc)
			done_callback, (XtPointer)shell);

	if (ep->rep_last) {
		char *space, *text = mkdatestring(ep->rep_last);
		if ((space = strchr(text, ' ')))
			text = space+1;
		print_text_button(last, text);
	}
	if (ep->rep_every)
		print_text_button(every, "%d", ep->rep_every/86400);
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
	destroy_recycle_popup();
}


/*ARGSUSED*/
static void wkday_callback(
	UNUSED Widget			widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	if (data->set)
		edit.entry.rep_weekdays |= 1 << item;
	else
		edit.entry.rep_weekdays &= ~(1 << item);
	edit.changed = TRUE;
	sensitize_edit_buttons();
}


/*ARGSUSED*/
static void nth_callback(
	UNUSED Widget			widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	struct entry			*ep = &edit.entry;
	int				i;
	Arg				args;

	if (item)
		if (data->set)
			ep->rep_weekdays |= 0x0080 << item;
		else
			ep->rep_weekdays &= ~(0x0080 << item);
	else
			ep->rep_weekdays &= ~0x3f00;
	for (i=0; i < 7; i++) {
		XtSetArg(args, XmNset, i ?  (ep->rep_weekdays & 0x0080<<i) != 0
					 : !(ep->rep_weekdays & 0x3f00));
		XtSetValues(nth_widget[i], &args, 1);
	}
	edit.changed = TRUE;
	sensitize_edit_buttons();
}


/*ARGSUSED*/
static void day_callback(
	UNUSED Widget			widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	if (data->set)
		edit.entry.rep_days |= 1 << (item&31);
	else
		edit.entry.rep_days &= ~(1 << (item&31));
	edit.changed = TRUE;
	sensitize_edit_buttons();
}


/*ARGSUSED*/
static void every_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	XmToggleButtonCallbackStruct	*data)
{
	if (data->set) {
		XmProcessTraversal(every, XmTRAVERSE_CURRENT);
		edit.entry.rep_every = 86400;
		print_text_button(every, "1");
	} else {
		edit.entry.rep_every = 0;
		print_text_button(every, "");
	}
	edit.changed = TRUE;
	sensitize_edit_buttons();
}


/*ARGSUSED*/
static void every_text_callback(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	char				*text = XmTextGetString(widget);
	Arg				args;

	edit.entry.rep_every = atoi(text) * 86400;
	if (edit.entry.rep_every < 0)
		edit.entry.rep_every = 0;
	XtSetArg(args, XmNset, edit.entry.rep_every > 0);
	XtSetValues(every_toggle, &args, 1);
	XtFree(text);
}


/*ARGSUSED*/
static void yearly_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	XmToggleButtonCallbackStruct	*data)
{
	edit.entry.rep_yearly = data->set;
	edit.changed = TRUE;
	sensitize_edit_buttons();
}


/*ARGSUSED*/
static void last_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	XmToggleButtonCallbackStruct	*data)
{
	if (data->set) {
		char *p, date[40];
		XmProcessTraversal(last, XmTRAVERSE_CURRENT);
		edit.entry.rep_last = edit.entry.time -
				      edit.entry.time % 86400 + 86400;
		strcpy(date, mkdatestring(edit.entry.rep_last));
		p = strchr(date, ' ');
		print_text_button(last, p ? p+1 : date);
	} else {
		edit.entry.rep_last = 0;
		print_text_button(last, "");
	}
	edit.changed = TRUE;
	sensitize_edit_buttons();
}


/*ARGSUSED*/
static void last_text_callback(
	Widget				widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	char				*text = XmTextGetString(widget);
	Arg				args;

	edit.entry.rep_last = parse_datestring(text, edit.entry.time);
	if (edit.entry.rep_last < edit.entry.time + 86400-1) {
		char *p, date[40];
		strcpy(date, mkdatestring(edit.entry.time));
		p = strchr(date, ' ');
		print_text_button(last, p ? p+1 : date);
		create_error_popup(widget, 0,
			_("The new last-trigger date is before the\n"\
			  "first-trigger date, %s. Ignored."), date);
		edit.entry.rep_last = edit.entry.time;
	}
	XtSetArg(args, XmNset, edit.entry.rep_last > 0);
	XtSetValues(last_toggle, &args, 1);
	XtFree(text);
}
