/* 
 * Draw the list of entries and everything in it. The list is drawn into
 * an array of text and image buttons. The first row is reserved for the
 * title; it cannot be pressed. This means schedule entries 0..n are drawn
 * into rows 1..n+1.
 *
 *	draw_list(w)			Draw or redraw a list of entries
 *					into a list menu.
 *	mkbuttonstring(lw, x, y)	Return a string to draw into a text
 *					button in the list menu array.
 *	mknotestring(ep)		Return the string to draw into the
 *					note button in the menu list array,
 *					or into the day box in the calendar.
 */

#include <stdio.h>
#include <time.h>
#include <Xm/Xm.h>
#include "cal.h"

extern struct edit	edit;		/* info about entry being edited */
extern struct config	config;		/* global configuration data */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern struct user	*user;		/* user list (from file_r.c) */


/*
 * return TRUE if the appointment should show the PIC_EXCEPT icon in the
 * day button
 */

BOOL show_except_pixmap(
	struct entry	*ep)
{
	int		i;

	if (!ep)
		return(FALSE);
	for (i=0; i < NEXC; i++)
		if (ep->except[i])
			return(TRUE);
	return(ep->acolor || ep->hide_in_m
			  || ep->hide_in_y
			  || ep->hide_in_w
			  || ep->hide_in_o
			  || ep->hide_in_d);
}


/*
 * draw a list into a list popup. Always call create_sublist() for the
 * menu first, so that there is something to draw. Clear now-unused rows
 * under the current last row that may have been drawn earlier.
 */

void draw_list(
	struct listmenu	*w)		/* draw into which menu */
{
	Widget		(*wp)[SC_N];	/* button widget pointer */
	struct entry	**epp;		/* entry pointer pointer */
	struct entry	*ep;		/* entry pointer */
	int		n = 0;		/* entry counter */
	int		u;		/* user number */
	Arg		arg;		/* for toggle/user buttons */

	create_entry_widgets(w, w->sublist ? w->sublist->nentries : 0);
	wp = w->entry + 1;				/* [0] is header */
	if (w->sublist) {				/* draw entries */
	    epp = w->sublist->entry;
	    for (n=0; n <= w->sublist->nentries; n++, wp++, epp++) {
		if (edit.editing && edit.menu == w && edit.y-1 == n)
			ep = &edit.entry;
		else if (n < w->sublist->nentries)
			ep = *epp;
		else
			break;
		u = name_to_user(ep->user);
		XtSetArg(arg, XmNset, !ep->suspended);
		XtSetValues ((*wp)[SC_ENABLE], &arg, 1);
		XtSetArg(arg, XmNbackground, color[u == 0 ?
				COL_BACK : COL_WUSER_0 + (user[u].color & 7)]);
		XtSetValues ((*wp)[SC_USER],  &arg, 1);
		print_button((*wp)[SC_USER],  "%s",
					     mkbuttonstring(w,SC_USER,   n+1));
		print_button((*wp)[SC_DATE], mkbuttonstring(w,SC_DATE,   n+1));
		print_button((*wp)[SC_ENDDATE],
					     mkbuttonstring(w,SC_ENDDATE,n+1));
		print_button((*wp)[SC_TIME], mkbuttonstring(w,SC_TIME,   n+1));
		print_button((*wp)[SC_LENGTH],mkbuttonstring(w,SC_LENGTH,n+1));
		print_button((*wp)[SC_NOTE], "%s",
					     mkbuttonstring(w,SC_NOTE,   n+1));
		print_button((*wp)[SC_ADVANCE], 
					     mkbuttonstring(w,SC_ADVANCE,n+1));
		print_button((*wp)[SC_ADVDAYS], 
					     mkbuttonstring(w,SC_ADVDAYS,n+1));
		print_pixmap((*wp)[SC_RECYCLE], PIC_RECYCLE,
					ep->rep_every  || ep->rep_days ||
					ep->rep_yearly || ep->rep_weekdays);
		print_pixmap((*wp)[SC_MESSAGE], PIC_MESSAGE, !!ep->message);
		print_pixmap((*wp)[SC_SCRIPT], PIC_SCRIPT,
					!ep->notime && ep->script);
		print_pixmap((*wp)[SC_EXCEPT], PIC_EXCEPT,
					show_except_pixmap(ep));
		print_pixmap((*wp)[SC_PRIVATE], PIC_PRIVATE, ep->private);
		print_pixmap((*wp)[SC_TODO], PIC_TODO, ep->todo);
	    }
	}
	for (; n < w->nentries; n++, wp++) {		/* draw blank rows */
		XtSetArg(arg, XmNset, FALSE);
		XtSetValues ((*wp)[SC_ENABLE], &arg, 1);
		XtSetArg(arg, XmNbackground, color[COL_BACK]);
		XtSetValues ((*wp)[SC_USER],   &arg, 1);
		print_button((*wp)[SC_USER],    "");
		print_button((*wp)[SC_DATE],    "");
		print_button((*wp)[SC_ENDDATE], "");
		print_button((*wp)[SC_TIME],    "");
		print_button((*wp)[SC_LENGTH],  "");
		print_button((*wp)[SC_ADVANCE], "");
		print_button((*wp)[SC_ADVDAYS], "");
		print_button((*wp)[SC_NOTE],    "");
		print_pixmap((*wp)[SC_RECYCLE], PIC_BLANK, FALSE);
		print_pixmap((*wp)[SC_MESSAGE], PIC_BLANK, FALSE);
		print_pixmap((*wp)[SC_SCRIPT],  PIC_BLANK, FALSE);
		print_pixmap((*wp)[SC_EXCEPT],  PIC_BLANK, FALSE);
		print_pixmap((*wp)[SC_PRIVATE], PIC_BLANK, FALSE);
		print_pixmap((*wp)[SC_TODO],    PIC_BLANK, FALSE);
	}
}


/*
 * return some sensible string representation of a button. This doesn't work
 * for pixmap labels.
 */

char *mkbuttonstring(
	struct listmenu	*lw,		/* menu the button is in */
	int		x,		/* column and row (y >= 1) */
	int		y)
{
	static char	buf[40];	/* string representation */
	struct entry	*ep;		/* entry pointer */
	int		u;		/* user number */

	if (edit.editing && edit.menu == lw && edit.y == y)
		ep = &edit.entry;
	else if (!lw->sublist || y > lw->sublist->nentries)
		return("");
	else
		ep = lw->sublist->entry[y-1];

	switch(x) {
	  case SC_USER:
		u = name_to_user(ep->user);
		return(u == 0 ? "" : user[u].name);

	  case SC_DATE:
		return(mkdatestring(ep->time));

	  case SC_ENDDATE:
		return(ep->rep_every == 86400 &&
		       ep->rep_last >= ep->time - ep->time % 86400 + 86400 ?
		     			mkdatestring(ep->rep_last) : " ");

	  case SC_TIME:
		return(ep->notime ? "-" : mktimestring(ep->time, FALSE));

	  case SC_LENGTH:
		return(ep->length > 0 && !ep->notime ?
					mktimestring(ep->length, TRUE) : "");
	  case SC_ADVANCE:
		*buf = 0;
		if (ep->notime)
			break;
		if (ep->early_warn)
			sprintf(buf, "%d", (int)ep->early_warn/60);
		if (ep->late_warn) {
			if (ep->early_warn)
				strcat(buf, ",");
			sprintf(buf+strlen(buf), "%d", (int)ep->late_warn/60);
		}
		if (ep->noalarm) {
			if (ep->early_warn || ep->late_warn)
				strcat(buf, ",");
			strcat(buf, "-");
		}
		break;

	  case SC_ADVDAYS:
		sprintf(buf, "%d", (int)(ep->days_warn / 86400));
		return(ep->days_warn > 0 ? buf : "");

	  case SC_NOTE:
		return(mknotestring(ep));
	}
	return(buf);
}


/*
 * return the entries's note string. This is separate from mkbuttonstring
 * because it is also used to print notes into the calendar day boxes.
 */

char *mknotestring(
	struct entry	*ep)		/* entry pointer */
{
	static char	buf[256];	/* string representation */
	char		*note;		/* note to print */
	int		i;		/* string index */

	if ((note = ep->note)    == 0 &&
	    (note = ep->message) == 0 &&
	    (note = ep->script)  != 0)
		if (note[0] == '#' && note[1] == '!') {
			while (*note && *note++ != '\n');
			if (!*note)
				note = ep->script;
		}
	i = 0;
	if (note)
		while (i < 255 && *note && *note != '\n')
			buf[i++] = *note++;
	buf[i] = 0;
	return(buf);
}
