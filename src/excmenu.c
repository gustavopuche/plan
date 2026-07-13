/*
 * Create and destroy the appt exception popup. It allows specification of
 * up to NEXC exception dates on which the appointment will not trigger.
 * The entry being edited is in edit.entry, as usual.
 *
 *	destroy_except_popup()
 *	create_except_popup()
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
#include <Xm/TextF.h>
#include <Xm/Protocols.h>
#include "cal.h"

static void done_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void text_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void clear_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void split_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void hide_m_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void hide_y_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void hide_w_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void hide_d_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void hide_o_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void notime_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void acol_callback	(Widget, int, XmToggleButtonCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern Widget		toplevel;	/* top-level shell for icon name */
extern struct edit	edit;		/* info about entry being edited */
extern struct config	config;		/* global configuration data */
extern XtAppContext	app;		/* application handle */
extern struct mainmenu	mainmenu;	/* all important main window widgets */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern struct plist	*mainlist;	/* list of all schedule entries */

static BOOL		have_shell;	/* message popup exists if TRUE */
static Widget		shell;		/* popup menu shell */
static Widget		text_w[NEXC];	/* exception date text widgets */
static Widget		acol_w;		/* appointment text color button */
static Widget		acol_sw[9];	/* choices in acol_w color popup */


/*
 * fill out the menu, by printing all the current exception dates. This
 * is done whenever anything changes.
 */

static void fillout_except_menu(void)
{
	Arg			args[2];
	int			i, col;

	for (i=0; i < NEXC; i++)
		print_text_button(text_w[i], edit.entry.except[i] ?
				mkdatestring(edit.entry.except[i]) : " ");

	print_pixmap(edit.menu->entry[edit.y][SC_EXCEPT], PIC_EXCEPT,
				show_except_pixmap(&edit.entry));

	col = edit.entry.acolor ? COL_HBLACK + edit.entry.acolor-1 : COL_STD;
#ifdef XmCSimpleOptionMenu
	XtSetArg(args[0], XmNforeground, color[col]);
	XtSetArg(args[1], XmNmenuHistory, acol_sw[(int)edit.entry.acolor]);
	XtSetValues(acol_w, args, 2);
#else
	XtSetArg(args[0], XmNforeground, color[col]);
	XtSetValues(acol_w, args, 1);
	print_button(acol_w, edit.entry.acolor ? "%d" : _("default"),
							edit.entry.acolor-1);
#endif
}


/*
 * retrieve texts from text widgets, store everything in the appointment.
 * Call fillout_except_menu after calling this routine, because cleared
 * dates are eliminated.
 */

static void readback_except_menu(void)
{
	char		*string, *p;	/* contents of text widget */
	int		i, j=0;		/* exception counter */

	for (i=0; i < NEXC; i++) {
		string = XmTextGetString(text_w[i]);
		for (p=string; *p == ' ' || *p == '\t'; p++);
		if (*p)
			edit.entry.except[j++] = parse_datestring(p, 0);
		XtFree(string);
	}
	while (j < NEXC)
		edit.entry.except[j++] = 0;
}


/*
 * destroy a popup. Remove it from the screen, and destroy its widgets.
 * It's too much trouble to keep them for next time.
 */

void destroy_except_popup(void)
{
	if (have_shell) {
		readback_except_menu();
		XtPopdown(shell);
		XTDESTROYWIDGET(shell);
		have_shell = FALSE;
		edit.changed = TRUE;
		edit.entry.suspended = FALSE;
		print_pixmap(edit.menu->entry[edit.y][SC_EXCEPT], PIC_EXCEPT,
					show_except_pixmap(&edit.entry));
	}
}


/*
 * create an exception popup as a separate application shell. The popup is
 * initialized with data from edit.entry.
 */

void create_except_popup(void)
{
	Widget			form, frame, form2, line, popup, w;
	Arg			args[20];
	int			n, i;
	Atom			closewindow;
	XmString		str;

	destroy_except_popup();

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtCreatePopupShell(_("Exception Dates"),
			applicationShellWidgetClass, toplevel, args, n);
#	ifdef EDITRES
	XtAddEventHandler(shell, (EventMask)0, TRUE, 
 			_XEditResCheckMessages, NULL);
#	endif
	set_icon(shell, 1);
	form = XtCreateManagedWidget("excform1", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"except");

							/*--- exceptions ---*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	w = XtCreateManagedWidget(_("Exception Dates:"),
			xmLabelWidgetClass, form, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNshadowType,	XmSHADOW_ETCHED_IN);	n++;
	frame = XtCreateManagedWidget("", xmFrameWidgetClass,
			form, args, n);
	form2 = XtCreateManagedWidget("excform2", xmFormWidgetClass,
			frame, NULL, 0);
	XtAddCallback(form2, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"except");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	  XmATTACH_FORM);	n++;
	XtSetArg(args[n], XmNtopOffset,		  8);			n++;

	for (i=0; i < NEXC; i++) {
	   XtSetArg(args[n], XmNleftAttachment,	   XmATTACH_FORM);	n++;
	   XtSetArg(args[n], XmNleftOffset,	   8);			n++;
	   XtSetArg(args[n], XmNrightAttachment,   XmATTACH_FORM);	n++;
	   XtSetArg(args[n], XmNrightOffset,	   8);			n++;
	   line = XtCreateManagedWidget("lineform", xmFormWidgetClass,
			form2, args, n);
	   n = 0;
	   XtSetArg(args[n], XmNtopAttachment,	   XmATTACH_FORM);	n++;
	   XtSetArg(args[n], XmNbottomAttachment,  XmATTACH_FORM);	n++;
	   XtSetArg(args[n], XmNleftAttachment,	   XmATTACH_FORM);	n++;
	   w = XtCreateManagedWidget(_("No alarm on"), xmLabelWidgetClass,
			line, args, n);
	   n = 0;
	   XtSetArg(args[n], XmNtopAttachment,	   XmATTACH_FORM);	n++;
	   XtSetArg(args[n], XmNbottomAttachment,  XmATTACH_FORM);	n++;
	   XtSetArg(args[n], XmNleftAttachment,	   XmATTACH_WIDGET);	n++;
	   XtSetArg(args[n], XmNleftWidget,	   w);			n++;
	   XtSetArg(args[n], XmNleftOffset,	   16);			n++;
	   XtSetArg(args[n], XmNwidth,		   140);		n++;
	   XtSetArg(args[n], XmNrecomputeSize,	   False);		n++;
	   XtSetArg(args[n], XmNpendingDelete,	   True);		n++;
	   XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	   XtSetArg(args[n], XmNbackground,	   color[COL_TEXTBACK]);n++;
	   text_w[i] = XtCreateManagedWidget(" ", xmTextFieldWidgetClass,
			line, args, n);
	   XtAddCallback(text_w[i], XmNactivateCallback, (XtCallbackProc)
	   		text_callback, (XtPointer)(size_t)i);
	   n = 0;
	   XtSetArg(args[n], XmNtopAttachment,	   XmATTACH_FORM);	n++;
	   XtSetArg(args[n], XmNbottomAttachment,  XmATTACH_FORM);	n++;
	   XtSetArg(args[n], XmNleftAttachment,	   XmATTACH_WIDGET);	n++;
	   XtSetArg(args[n], XmNleftWidget,	   text_w[i]);		n++;
	   XtSetArg(args[n], XmNleftOffset,	   16);			n++;
	   XtSetArg(args[n], XmNwidth,		   80);			n++;
	   w = XtCreateManagedWidget(_("Split"), xmPushButtonWidgetClass,
			line, args, n);
	   XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
	   		split_callback, (XtPointer)(size_t)i);
	   XtAddCallback(form2, XmNhelpCallback, (XtCallbackProc)
	   		help_callback, (XtPointer)"exc_split");
	   n = 0;
	   XtSetArg(args[n], XmNtopAttachment,	   XmATTACH_FORM);	n++;
	   XtSetArg(args[n], XmNbottomAttachment,  XmATTACH_FORM);	n++;
	   XtSetArg(args[n], XmNleftAttachment,	   XmATTACH_WIDGET);	n++;
	   XtSetArg(args[n], XmNleftWidget,	   w);			n++;
	   XtSetArg(args[n], XmNleftOffset,	   8);			n++;
	   XtSetArg(args[n], XmNrightAttachment,   XmATTACH_FORM);	n++;
	   XtSetArg(args[n], XmNwidth,		   80);			n++;
	   w = XtCreateManagedWidget(_("Clear"), xmPushButtonWidgetClass,
			line, args, n);
	   XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
	   		clear_callback, (XtPointer)(size_t)i);
	   XtAddCallback(form2, XmNhelpCallback, (XtCallbackProc)
	   		help_callback, (XtPointer)"exc_clear");

	   n = 0;
	   XtSetArg(args[n], XmNtopAttachment,	  XmATTACH_WIDGET);	n++;
	   XtSetArg(args[n], XmNtopWidget,	  line);		n++;
	   XtSetArg(args[n], XmNtopOffset,	  8);			n++;
	   if (i == NEXC-2) {
	      XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);	n++;
	      XtSetArg(args[n], XmNbottomOffset,  8);			n++;
	   }
	}
							/*--- flags ---*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		frame);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	w = XtCreateManagedWidget(_("Options:"),
			xmLabelWidgetClass, form, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNshadowType,	XmSHADOW_ETCHED_IN);	n++;
	frame = XtCreateManagedWidget("", xmFrameWidgetClass,
			form, args, n);
	form2 = XtCreateManagedWidget("excform3", xmFormWidgetClass,
			frame, NULL, 0);
	XtAddCallback(form2, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"exc_flags");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		edit.entry.hide_in_m);	n++;
	w = XtCreateManagedWidget(_("Don't show appointment in month view"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			hide_m_callback, (XtPointer)(size_t)0);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		edit.entry.hide_in_y);	n++;
	w = XtCreateManagedWidget(_("Don't show appointment in year view"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			hide_y_callback, (XtPointer)(size_t)0);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		edit.entry.hide_in_w);	n++;
	w = XtCreateManagedWidget(_("Don't show appointment in week view"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			hide_w_callback, (XtPointer)(size_t)0);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		edit.entry.hide_in_d);	n++;
	w = XtCreateManagedWidget(_("Don't show appointment in day view"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			hide_d_callback, (XtPointer)(size_t)0);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		edit.entry.hide_in_o);	n++;
	w = XtCreateManagedWidget(_("Don't show appointment in year overview"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			hide_o_callback, (XtPointer)(size_t)0);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNset,		edit.entry.notime);	n++;
	w = XtCreateManagedWidget(_("Reminder only: no alarms, time not shown"),
			xmToggleButtonWidgetClass, form2, args, n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
			notime_callback, (XtPointer)(size_t)0);

							/*-- appt colors --*/
#ifdef XmCSimpleOptionMenu
	popup = XmCreatePulldownMenu(form, "pulldown", NULL, 0);
	for (i=0; i <= 8; i++) {
		char name[10];
		sprintf(name, i ? "%d" : _("default"), i-1);
		XtSetArg(args[0], XmNalignment, XmALIGNMENT_CENTER);
		XtSetArg(args[1], XmNforeground,
					color[i ? COL_HBLACK+i-1 : COL_STD]);
		acol_sw[i] = XtCreateManagedWidget(name,
				xmPushButtonWidgetClass, popup, args, 2);
		XtAddCallback(acol_sw[i], XmNactivateCallback,
				(XtCallbackProc)acol_callback,
				(XtPointer)(size_t)i);
	}
	str = XmStringCreateSimple(_("Appointment text color:"));
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNmarginHeight,	0);			n++;
	XtSetArg(args[n], XmNwidth,		60);			n++;
	XtSetArg(args[n], XmNhighlightThickness,1);			n++;
	XtSetArg(args[n], XmNsubMenuId,		popup);			n++;
	XtSetArg(args[n], XmNlabelString,	str);			n++;
	acol_w = w = XmCreateOptionMenu(form2, "exc_acolor", args, n);
	XtAddCallback(acol_w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"exc_acolor");
	XmStringFree(str);
	XtManageChild(acol_w);
#else
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	line = XtCreateManagedWidget("excform4", xmFormWidgetClass,
			form2, args, n);
	XtAddCallback(line, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"exc_acolor");
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	acol_w = XtCreateManagedWidget(" ", xmPushButtonWidgetClass,
			line, args, n);
	XtAddCallback(acol_w, XmNactivateCallback, (XtCallbackProc)
			acol_callback,(XtPointer)(size_t)0);
	XtAddCallback(acol_w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"exc_acolor");
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	acol_w);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	w = XtCreateManagedWidget(_("Appointment text color"),
			xmLabelWidgetClass, line, args, n);
#endif

							/*-- buttons --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		frame);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Done"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			done_callback, (XtPointer)(size_t)0);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)
			help_callback, (XtPointer)"exc_done");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		frame);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(_("Help"), xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
			help_callback,(XtPointer)"except");
	XtAddCallback(w, XmNhelpCallback,     (XtCallbackProc)
			help_callback,(XtPointer)"except");

	XtPopup(shell, XtGrabNone);
	fillout_except_menu();

	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow, (XtCallbackProc)
			done_callback, (XtPointer)shell);
	have_shell = TRUE;
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * buttons
 */

/*ARGSUSED*/
static void done_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	destroy_except_popup();
}


/*
 * exceptions
 */

/*ARGSUSED*/
static void text_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	readback_except_menu();
	fillout_except_menu();
}


/*ARGSUSED*/
static void clear_callback(
	UNUSED Widget			widget,
	int				item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	print_text_button(text_w[item], "");
	readback_except_menu();
	fillout_except_menu();
}


/*ARGSUSED*/
static void split_callback(
	UNUSED Widget			widget,
	int				item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	char		*string, *p;	/* contents of text widget */
	time_t		time;		/* exception time to make appt for */
	struct entry	new;		/* cloned entry, old is edit.entry */
	int		i;		/* for clearing exceptions of new */

	string = XmTextGetString(text_w[item]);
	for (p=string; *p == ' ' || *p == '\t'; p++);
	if (!*p) {
		XtFree (string);
		return;
	}
	time = parse_datestring(p, 0);
	XtFree(string);
	clone_entry(&new, &edit.entry);
	for (i=0; i < NEXC; i++)
		new.except[i] = 0;
	new.time	 = time + edit.entry.time % 86400;
	new.rep_every	 = 0;
	new.rep_last	 = 0;
	new.rep_weekdays = 0;
	new.rep_days	 = 0;
	new.rep_yearly	 = FALSE;
	new.file	 = 0;
	new.id		 = 0;
	confirm_new_entry();
	edit.entry	 = new;
	edit.editing	 = TRUE;
	edit.changed	 = TRUE;
	edit.y		 = edit.menu->sublist ?
					edit.menu->sublist->nentries+1 : 1;
	draw_list(edit.menu);
	XtPopup(edit.menu->shell, XtGrabNone);
	sensitize_edit_buttons();
}


/*
 * flags
 */

/*ARGSUSED*/
static void hide_m_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	edit.entry.hide_in_m ^= TRUE;
	edit.changed = TRUE;
	print_pixmap(edit.menu->entry[edit.y][SC_EXCEPT], PIC_EXCEPT,
					show_except_pixmap(&edit.entry));
}


/*ARGSUSED*/
static void hide_y_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	edit.entry.hide_in_y ^= TRUE;
	edit.changed = TRUE;
	print_pixmap(edit.menu->entry[edit.y][SC_EXCEPT], PIC_EXCEPT,
					show_except_pixmap(&edit.entry));
}


/*ARGSUSED*/
static void hide_w_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	edit.entry.hide_in_w ^= TRUE;
	edit.changed = TRUE;
	print_pixmap(edit.menu->entry[edit.y][SC_EXCEPT], PIC_EXCEPT,
					show_except_pixmap(&edit.entry));
}


/*ARGSUSED*/
static void hide_d_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	edit.entry.hide_in_d ^= TRUE;
	edit.changed = TRUE;
	print_pixmap(edit.menu->entry[edit.y][SC_EXCEPT], PIC_EXCEPT,
					show_except_pixmap(&edit.entry));
}


/*ARGSUSED*/
static void hide_o_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	edit.entry.hide_in_o ^= TRUE;
	edit.changed = TRUE;
	print_pixmap(edit.menu->entry[edit.y][SC_EXCEPT], PIC_EXCEPT,
					show_except_pixmap(&edit.entry));
}


/*ARGSUSED*/
static void notime_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	edit.entry.notime ^= TRUE;
	edit.changed = TRUE;
	draw_list(edit.menu);
	print_pixmap(edit.menu->entry[edit.y][SC_EXCEPT], PIC_EXCEPT,
					show_except_pixmap(&edit.entry));
}


/*
 * colors
 */

/*ARGSUSED*/
static void acol_callback(
	UNUSED Widget			widget,
	int				item,
	UNUSED XmToggleButtonCallbackStruct*data)
{
	edit.changed = TRUE;
#ifdef XmCSimpleOptionMenu
	edit.entry.acolor = item;
#else
	edit.entry.acolor = (edit.entry.acolor + 1) % 9;
#endif
	fillout_except_menu();
}
