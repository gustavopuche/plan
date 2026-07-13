/*
 * Create and destroy the print popup. It is installed when the
 * Print entry in the File pulldown in the main calendar window
 * is used.
 *
 *	create_print_popup()
 *	destroy_print_popup()
 *
 * Originally by Karl Bunch, extensively hacked 23.12.94 by thomas@bitrot
 * Only-my-appointments toggle added by Richard G. Hash (rgh@shell.com)
 * More hacking by thomas@bitrot 25.12.98, color
 */

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
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
#include <Xm/TextF.h>
#include <Xm/Protocols.h>
#include "cal.h"

#include <fcntl.h>
#include <errno.h>

#define PS_LIB		"plan_cal.ps"
#ifdef JAPAN
#define PS_LIB_SJ	"plan_calSJ.ps"
extern char		*localename;
#endif
#ifdef BSD
extern FILE		*popen();
#endif

static void mode_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void omit_appts_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void omit_priv_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void omit_others_callback(Widget, int, XmToggleButtonCallbackStruct *);
static void color_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void done_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void print_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void broken_pipe_handler	(int);
static int Print_Calendar	(FILE *, int, int, int, int);
static int Print_Day		(FILE *, int, int, int, int);
static void OutStr		(FILE *, char *);

extern int		curr_month, curr_year, curr_week;
extern struct holiday	holiday[];
extern struct holiday	sm_holiday[];
extern struct user	*user;			/* user list (from file_r.c) */

extern Display		*display;		/* everybody uses this server*/
extern struct config	config;			/* global configuration data */
extern struct plist	*mainlist;		/* list of all sched entries */
extern Pixel		color[NCOLS];		/* colors: COL_* */
extern XColor		xcolor[NCOLS];		/* for PostScript color */
extern char		Print_Spooler[100];	/* defined in file_r.c */

char			*Default_Spooler = PS_SPOOLER;
static BOOL		popup_created;		/* popup exists if TRUE */
static Widget		print_shell;		/* popup menu shell */
static Widget		spool_text;		/* Spool String Text Widget */



/*
 * create a print popup as a separate application shell. When the popup
 * already exists and was just popped down, pop it up and exit. This ensures
 * that it comes up with the same defaults.
 */

void create_print_popup(void)
{
	Widget		print_form, w;
	Widget		mode_frame, mode_form, mode_radio;
	Widget		spool_frame, spool_form;
	Widget		action_area;
	Atom		closewindow;
	XmString	s[10];
	char		tmp[32];
	int		n;

	if (popup_created) {
		XtPopup(print_shell, XtGrabNone);
		return;
	}
	print_shell = XtVaAppCreateShell(_("Print"), "plan",
					applicationShellWidgetClass, display,
					XmNdeleteResponse, XmDO_NOTHING,
					XmNiconic, False,
					NULL);

	set_icon(print_shell, 1);

	print_form = XtCreateManagedWidget("printform", xmFormWidgetClass,
						print_shell, NULL, 0);

	XtAddCallback(print_form, XmNhelpCallback, (XtCallbackProc)
					help_callback, (XtPointer)"print");

	mode_frame = XtVaCreateManagedWidget("mode",
					xmFrameWidgetClass, print_form,
					XmNtopAttachment, XmATTACH_FORM,
					XmNtopOffset, 8,
					XmNrightAttachment, XmATTACH_FORM,
					XmNrightOffset, 8,
					XmNleftAttachment, XmATTACH_FORM,
					XmNleftOffset, 8,
					NULL, 0, NULL);

	mode_form = XtCreateManagedWidget("print_mode", xmFormWidgetClass,
						mode_frame, NULL, 0);

	/* Print Mode */
	w = XtVaCreateManagedWidget(_("Print mode:"),
					xmLabelWidgetClass, mode_form,
					XmNtopAttachment, XmATTACH_FORM,
					XmNtopOffset, 8,
					XmNleftAttachment, XmATTACH_FORM,
					XmNleftOffset, 8,
					NULL);

	s[0] = XmStringCreateSimple(_("Month"));
	s[1] = XmStringCreateSimple(_("Day"));
	s[2] = XmStringCreateSimple(_("Week (Landscape)"));
	s[3] = XmStringCreateSimple(_("Week (Portrait)"));
	s[4] = XmStringCreateSimple(_("Year"));
	s[5] = XmStringCreateSimple(_("Year Overview"));
	mode_radio = XmVaCreateSimpleRadioBox(mode_form, "radio",
					config.pr_mode,
					(XtCallbackProc)mode_callback,
					XmNtopAttachment, XmATTACH_FORM,
					XmNtopOffset, 8,
					XmNleftAttachment, XmATTACH_WIDGET,
					XmNleftWidget, w,
					XmNleftOffset, 8,
					XmNspacing, 4,
					XmVaRADIOBUTTON, s[0], 0, NULL, NULL,
					XmVaRADIOBUTTON, s[1], 0, NULL, NULL,
					XmVaRADIOBUTTON, s[2], 0, NULL, NULL,
					XmVaRADIOBUTTON, s[3], 0, NULL, NULL,
					XmVaRADIOBUTTON, s[4], 0, NULL, NULL,
					XmVaRADIOBUTTON, s[5], 0, NULL, NULL,
					NULL);
	XtManageChild(mode_radio);
	for (n=0; n < 6; n++) {
		XmStringFree(s[n]);
		sprintf(tmp, "button_%d", n);
		if ((w = XtNameToWidget(mode_radio, tmp)) != NULL) {
			XtVaSetValues(w, XmNselectColor, color[COL_TOGGLE], 0,
					NULL);
		}
	}

	/* omission flags */
	w = XtVaCreateManagedWidget(_("Omit all appointments"),
					xmToggleButtonWidgetClass, mode_form,
					XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, mode_radio,
					XmNtopOffset, 12,
					XmNleftAttachment, XmATTACH_FORM,
					XmNleftOffset, 8,
					XmNselectColor,	color[COL_TOGGLE],
					XmNhighlightThickness, 0,
					XmNset,	config.pr_omit_appts,
					NULL);

	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
					omit_appts_callback, 0);

	w = XtVaCreateManagedWidget(_("Omit private appointments"),
					xmToggleButtonWidgetClass, mode_form,
					XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, w,
					XmNtopOffset, 4,
					XmNleftAttachment, XmATTACH_FORM,
					XmNleftOffset, 8,
					XmNselectColor,	color[COL_TOGGLE],
					XmNhighlightThickness, 0,
					XmNset,	config.pr_omit_priv,
					NULL);

	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
					omit_priv_callback, 0);

	w = XtVaCreateManagedWidget(_("Omit everyone else's appointments"),
					xmToggleButtonWidgetClass, mode_form,
					XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, w,
					XmNtopOffset, 4,
					XmNleftAttachment, XmATTACH_FORM,
					XmNleftOffset, 8,
					XmNselectColor, color[COL_TOGGLE],
					XmNhighlightThickness, 0,
					XmNset, config.pr_omit_others,
					NULL);

	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
					omit_others_callback, 0);

	w = XtVaCreateManagedWidget(_("Color printout"),
					xmToggleButtonWidgetClass, mode_form,
					XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, w,
					XmNtopOffset, 4,
					XmNbottomAttachment, XmATTACH_FORM,
					XmNbottomOffset, 8,
					XmNleftAttachment, XmATTACH_FORM,
					XmNleftOffset, 8,
					XmNselectColor, color[COL_TOGGLE],
					XmNhighlightThickness, 0,
					XmNset, config.pr_color,
					NULL);

	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc)
					color_callback, 0);

	/* Spool String */
	spool_frame = XtVaCreateManagedWidget("spool",
					xmFrameWidgetClass, print_form,
					XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopOffset, 8,
					XmNtopWidget, mode_frame,
					XmNleftAttachment, XmATTACH_FORM,
					XmNleftOffset, 8,
					XmNrightAttachment, XmATTACH_FORM,
					XmNrightOffset, 8,
					NULL);

	spool_form = XtCreateManagedWidget("spool_form", xmFormWidgetClass,
						spool_frame, NULL, 0);

	w = XtVaCreateManagedWidget(_("Spooler string:"),
					xmLabelWidgetClass, spool_form,
					XmNtopAttachment, XmATTACH_FORM,
					XmNtopOffset, 8,
					XmNbottomAttachment, XmATTACH_FORM,
					XmNbottomOffset, 8,
					XmNleftAttachment, XmATTACH_FORM,
					XmNleftOffset, 8,
					NULL);

	if (!*Print_Spooler) {
		strcpy(Print_Spooler, Default_Spooler);
		mainlist->modified = TRUE;
	}
	spool_text = XtVaCreateManagedWidget("spooler",
					xmTextFieldWidgetClass, spool_form,
					XmNtopAttachment, XmATTACH_FORM,
					XmNtopOffset, 4,
					XmNbottomAttachment, XmATTACH_FORM,
					XmNbottomOffset, 4,
					XmNleftAttachment, XmATTACH_WIDGET,
					XmNleftOffset, 8,
					XmNleftWidget, w,
					XmNrightAttachment, XmATTACH_FORM,
					XmNrightOffset, 8,
					XmNwidth, 250,
					XmNbackground, color[COL_TEXTBACK],
					XmNvalue, Print_Spooler,
					NULL);

	/* Buttons */
	action_area = XtVaCreateManagedWidget("action",
					xmFormWidgetClass, print_form,
					XmNfractionBase, 10,
					XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopOffset, 16,
					XmNtopWidget, spool_frame,
					XmNbottomAttachment, XmATTACH_FORM,
					XmNbottomOffset, 8,
					XmNleftAttachment, XmATTACH_FORM,
					XmNleftOffset, 8,
					XmNrightAttachment, XmATTACH_FORM,
					XmNrightOffset, 8,
					NULL);

	w = XtVaCreateManagedWidget(_("Print"),
					xmPushButtonWidgetClass, action_area,
					XmNtopAttachment, XmATTACH_FORM,
					XmNbottomAttachment, XmATTACH_FORM,
					XmNrightAttachment, XmATTACH_FORM,
					XmNwidth, 80,
					NULL);

	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
					print_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)help_callback,
					(XtPointer) "print_print");

	w = XtVaCreateManagedWidget(_("Cancel"),
					xmPushButtonWidgetClass, action_area,
					XmNtopAttachment, XmATTACH_FORM,
					XmNbottomAttachment, XmATTACH_FORM,
					XmNrightAttachment, XmATTACH_WIDGET,
					XmNrightWidget, w,
					XmNrightOffset, 8,
					XmNwidth, 80,
					NULL);

	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)done_callback,
					(XtPointer)0);
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)help_callback,
					(XtPointer)"print_cancel");

	w = XtVaCreateManagedWidget(_("Help"),
					xmPushButtonWidgetClass, action_area,
					XmNtopAttachment, XmATTACH_FORM,
					XmNtopPosition, 0,
					XmNbottomAttachment, XmATTACH_FORM,
					XmNbottomPosition, 10,
					XmNrightAttachment, XmATTACH_WIDGET,
					XmNrightWidget, w,
					XmNrightOffset, 8,
					XmNwidth, 80,
					NULL);

	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)help_callback,
					(XtPointer) "print");
	XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)help_callback,
					(XtPointer)"print");
	XtPopup(print_shell, XtGrabNone);

	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);

	XmAddWMProtocolCallback(print_shell, closewindow, (XtCallbackProc)
					done_callback, (XtPointer)print_shell);

	popup_created = TRUE;
}


/*
 * Waste popup if it's been created
 */

void destroy_print_popup(void)
{
	if (popup_created)
		XtPopdown(print_shell);
	if (mainlist->modified)
		write_mainlist();
}


/*ARGSUSED*/
static void mode_callback(
	UNUSED Widget			widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	if (data->set) {
		config.pr_mode     = item;
		mainlist->modified = TRUE;
	}
}


/*ARGSUSED*/
static void omit_appts_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	XmToggleButtonCallbackStruct	*data)
{
	config.pr_omit_appts = data->set;
	mainlist->modified   = TRUE;
}


/*ARGSUSED*/
static void omit_priv_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	XmToggleButtonCallbackStruct	*data)
{
	config.pr_omit_priv  = data->set;
	mainlist->modified   = TRUE;
}


/*ARGSUSED*/
static void omit_others_callback(
	UNUSED Widget 			 widget,
	UNUSED int    			 item,
	XmToggleButtonCallbackStruct	*data)
{
	config.pr_omit_others  = data->set;
	mainlist->modified     = TRUE;
}


/*ARGSUSED*/
static void color_callback(
	UNUSED Widget 			 widget,
	UNUSED int    			 item,
	XmToggleButtonCallbackStruct	*data)
{
	config.pr_color    = data->set;
	mainlist->modified = TRUE;
}


/*ARGSUSED*/
static void done_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct *data)
{
	destroy_print_popup();
}


static void broken_pipe_handler(
	int				sig)
{
	create_error_popup(0, 0,
	 _("Print failed, aborted with signal %d\nCheck your spooler string."),
				sig);
	signal(SIGPIPE, SIG_IGN);
}


/*ARGSUSED*/
static void print_callback(
	UNUSED Widget			widget,
	UNUSED int			item,
	UNUSED XmToggleButtonCallbackStruct *data)
	{
	FILE	*pp;
	char	*Spooler = XmTextFieldGetString(spool_text);
	char	tmp[128];

	signal(SIGPIPE, broken_pipe_handler);

	sprintf(tmp, "exec %s", Spooler);

					/* Note any changes to spooler. */
	if (strcmp(Spooler, Print_Spooler)) {
		strcpy(Print_Spooler, Spooler);
		mainlist->modified = TRUE;
	}
	if ((pp = popen(tmp, "w")) == NULL)
		create_error_popup(print_shell, errno,
					_("Cannot open pipe to \")%s\""), tmp);
	else
		Print_Calendar(pp, config.pr_mode, curr_month, 1, curr_year);

	destroy_print_popup();
	signal(SIGPIPE, SIG_DFL);
}


static int Print_Calendar(
	FILE	*fp,
	int	Mode,
	int	Month,
	int	Day,
	int	Year)
{
	int	Pslib_Fd;
	char	*cp;
	char	tmp[512];
	char	PsLib[1024];
	int	nb;
	int	WeekDay, WeekNum, JulDay;
	int	CDay, StartDay, EndDay;
	int	CYear, StartYear, EndYear;
	int	CMonth, StartMonth, EndMonth;
	char	*Mode_Str;

	StartMonth = EndMonth = Month;
	StartDay = 1;
	EndDay = 31;
	StartYear = EndYear = Year;

	switch (Mode) {
	  case PR_MONTH:
		Mode_Str = "Month";
		break;

	  case PR_DAY:
		print_day_calendar(fp, FALSE);
		pclose(fp);
		return(0);

	  case PR_WEEK_L:
		print_week_calendar(fp, TRUE);
		pclose(fp);
		return(0);

	  case PR_WEEK_P:
		print_week_calendar(fp, FALSE);
		pclose(fp);
		return(0);

	  case PR_YEAR:
		Mode_Str = "Year";
		Month = 0;
		StartMonth = 0;
		EndMonth = 11;
		break;

	  case PR_YOV:
		print_yov_calendar(fp);
		pclose(fp);
		return(0);

	  default:
		return (-2);
	}
	WeekDay = -1;

#ifdef JAPAN
	if (strcmp(localename, LOCALE_SJIS) == 0 ||
	    strcmp(localename, LOCALE_EUC) == 0)
		find_file(PsLib, PS_LIB_SJ, FALSE);
	else
#endif
	find_file(PsLib, PS_LIB, FALSE);
	if ((Pslib_Fd = open(PsLib, O_RDONLY)) == -1) {
		create_error_popup(print_shell, errno,
		     _("Can't find PostScript skeleton\nfile \"%s\""), PS_LIB);
		pclose(fp);
		return (-1);
	}
	date_to_time(1, Month, Year, &WeekDay, &JulDay, &WeekNum);

	fprintf(fp, "%%!PS-Adobe\n");
	fprintf(fp, "%%\n");
	fprintf(fp, "%%%%Creator: plan\n");
	fprintf(fp, "%%%%Title: %s Calendar\n", Mode_Str);
	fprintf(fp, "%%%%EndComments\n");
	fprintf(fp, "%%\n");

	fprintf(fp, "%% Begin of PS_LIB (%s)\n", PsLib);

	while ((nb = read(Pslib_Fd, tmp, sizeof(tmp))) > 0)
		fwrite(tmp, sizeof(char), nb, fp);
	close(Pslib_Fd);

	fprintf(fp, "%% End of PS_LIB (%s)\n", PsLib);

	fflush(fp);

	fprintf(fp, "\n%% Plan Configuration values...\n");

#define BOOL_TO_PS(a,b) \
	fprintf(fp, "%s %s def\n", a, (b ? "true" : "false"))

	BOOL_TO_PS("/SundayFirst", config.sunday_first);
	BOOL_TO_PS("/MMDDYY", config.mmddyy);
	BOOL_TO_PS("/AMPM", config.ampm);
	BOOL_TO_PS("/Print_Julian", config.julian);
	BOOL_TO_PS("/Print_WeekNum", config.weeknum);

#undef BOOL_TO_PS

	fprintf(fp, "\n%% Print Information\n");

	fprintf(fp, "/Print_Mode (%s) def\n", Mode_Str);
	fprintf(fp, "/Month	%d def\n", Month+1);
	fprintf(fp, "/Day	%d def\n", Day);
	fprintf(fp, "/Year	%d def\n", Year+1900);
	fprintf(fp, "/WeekDay %d def\n", WeekDay);
	fprintf(fp, "/WeekNum %d def\n", WeekNum);
	fprintf(fp, "/JulDay	%d def\n", JulDay);

	fprintf(fp, "\n");

	fprintf(fp, "%% Month Range: %2d thru %2d\n",
						StartMonth+1, EndMonth+1);
	fprintf(fp, "%% Day Range	: %2d thru %2d\n", StartDay, EndDay);
	fprintf(fp, "%% Year Range : %2d thru %2d\n", StartYear, EndYear);

	fprintf(fp, "\nInitCal\n\n");
	fprintf(fp, "%% Data...\n\n");

	for (CYear = StartYear; CYear <= EndYear; CYear++) {
		if ((cp = parse_holidays(CYear, FALSE)) != NULL) {
			create_error_popup(print_shell, 0, cp);
			pclose(fp);
			return (-3);
		}
		for (CMonth = StartMonth; CMonth <= EndMonth; CMonth++)
			for (CDay = StartDay; CDay <= EndDay; CDay++)
				Print_Day(fp, CDay, CMonth, CYear, Mode);

		StartMonth = 1;
		EndMonth = 12;
	}
	fprintf(fp, "\n");
	fprintf(fp, "FinishCal\n");
	fflush(fp);
	pclose(fp);
	return (0);
}


/*
 * called for each day of month and year views. Prints data into plan_cal.ps.
 */

static int Print_Day(
	FILE		*fp,
	int		Day,
	int		Month,
	int		Year,
	int		Mode)
{
	int		DayStart, DayEnd, WeekDay, WeekNum, JulDay, found;
	time_t		trigger;
	struct entry	*entry;
	struct holiday	*hp, *hp2;
	struct lookup	lookup;
	struct tm	*tm;
	int		i, j, u, daycolor;

	if (!(DayStart = date_to_time(Day, Month, Year,
					&WeekDay, &JulDay, &WeekNum)))
		return (-1);

	daycolor = WeekDay == 6 || WeekDay == 0 ? COL_WEEKEND : COL_WEEKDAY;
	hp  = &sm_holiday[JulDay];
	hp2 =    &holiday[JulDay];
	for (j=0; j < 2; j++)
		if (!daycolor && hp->daycolor)
			daycolor = (j ? holiday : sm_holiday)[JulDay].daycolor;
	DayEnd = DayStart + 86399;
#if 0
	if (config.pr_color && daycolor)
		fprintf(fp, "%g %g %g setrgbcolor ",
				xcolor[daycolor].red   / 65536.,
				xcolor[daycolor].green / 65536.,
				xcolor[daycolor].blue  / 65536.);
#endif
	fprintf(fp, "%d %d %d %d %d NewDay\n",
				Month+1, Day, Year, WeekNum+1, JulDay+1);

	if ((hp->string && *hp->string) || (hp2->string && *hp2->string)) {
		fprintf(fp, "[ ");
		for (j=0; j < 2; j++, hp=hp2) {
			if (!hp->string || !*hp->string)
				continue;
			i = hp->stringcolor ? hp->stringcolor : daycolor;
			if (config.pr_color && i)
				fprintf(fp, "%g %g %g setrgbcolor ",
					xcolor[i].red   / 65536.,
					xcolor[i].green / 65536.,
					xcolor[i].blue  / 65536.);
			OutStr(fp, hp->string);
		}
		fprintf(fp, " ] PaintHolidays\n");
	}
	if (config.pr_omit_appts) {
		fflush(fp);
		return (0);
	}

	found = lookup_entry(&lookup, mainlist, DayStart, TRUE, FALSE,
				config.pr_mode == PR_MONTH ? 'm' :
				config.pr_mode == PR_YEAR  ? 'y' : '*', -1);

	for (; found; found = lookup_next_entry(&lookup)) {
		entry = &mainlist->entry[lookup.index];

		if ((config.pr_mode == PR_MONTH && entry->hide_in_m) ||
		    (config.pr_mode == PR_YEAR  && entry->hide_in_y))
			continue;
		if ((Mode == PR_MONTH && entry->hide_in_m) ||
		    (Mode == PR_YEAR  && entry->hide_in_y))
			continue;
		u = name_to_user(entry->user);
		if (user[u].suspend_m)
			continue;
		if (config.pr_omit_priv && entry->private)
			continue;
		if (config.pr_omit_others && u)
			continue;

		if (lookup.index >= mainlist->nentries
				|| lookup.trigger < DayStart
				|| lookup.trigger > DayEnd)
			break;

		trigger = lookup.trigger - lookup.trigger % 86400;
		for (i=0; i < NEXC; i++)
			if (entry->except[i] == trigger)
				break;
		if (i < NEXC)
			continue;

		tm = time_to_tm(entry->time);

		fprintf(fp, "	");

		if (config.pr_color) {
			int color;

			if (u > 0) {
				double r, g, b;
				color = COL_WUSER_0 + (user[u].color & 7);
				r = xcolor[color].red   / 65536.;
				g = xcolor[color].green / 65536.;
				b = xcolor[color].blue  / 65536.;
				if(user[u].color > 7) {
					r = (r + 1) / 2;
					g = (g + 1) / 2;
					b = (b + 1) / 2;
				}
				fprintf(fp, "%g %g %g setrgbcolor ", r, g, b);
				fprintf(fp, "PaintBGRect\n	");
			}
			color = entry->acolor ? COL_HBLACK+entry->acolor-1
					      : COL_WNOTE;
			fprintf(fp, "%g %g %g setrgbcolor ",
				xcolor[color].red   / 65536.,
				xcolor[color].green / 65536.,
				xcolor[color].blue  / 65536.);
		}
		if (config.mmddyy)
			fprintf(fp, " (%d/%d/%d)\t",
				tm->tm_mon+1, tm->tm_mday, tm->tm_year + 1900);
		else
			fprintf(fp, " (%d.%d.%d)\t",
				tm->tm_mday, tm->tm_mon+1, tm->tm_year + 1900);

		if (entry->notime)
			fprintf(fp, "()");
		else {
			if (config.ampm)
				fprintf(fp, "(%2d:%02d%c)\t",
					tm->tm_hour % 12 ? tm->tm_hour%12 : 12,
					tm->tm_min,
					tm->tm_hour < 12 ? 'a' : 'p');
			else
				fprintf(fp, "(%02d:%02d)\t", tm->tm_hour,
								tm->tm_min);
		}
		OutStr(fp, mknotestring(entry));
		if (entry->message && *entry->message) {
			fprintf(fp, "\n	");
			OutStr(fp, entry->message);
			fprintf(fp, "\n PaintEntry\n");
		} else
			fprintf(fp, "\t() PaintEntry\n");

		fflush(fp);
	}
	fflush(fp);
	return (0);
}


/*
 * Output a string in a manner compatible with PostScript..
 */

static void OutStr(
	FILE		*fp,
	char		*Str)
{
	putc('(', fp);
	for (; Str && *Str; Str++)
		switch (*Str) {
		  case '\n':
			/* String trailing New Lines */
			if (!Str[1])
				break;

			/* Replace Newlines we spaces... */
			/* In the "PostScript" we output an escaped newline */
			/* to improve readability */
			putc(' ', fp);

		  case '\\':
		  case '(':
		  case ')':
			fprintf(fp, "\\%c", *Str);
			break;

		  case '\f':
			fprintf(fp, "\\f");
			break;

		  case '\r':
			fprintf(fp, "\\r");
			break;

		default:
			if (*(unsigned char *)Str >= 160)
				fprintf(fp, "\\%o", (unsigned int)*Str & 255);
			else
				putc(*Str, fp);
			break;
		}
	putc(')', fp);
}
