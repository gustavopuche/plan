/*
 * Main calendar window widgets.
 *
 *	locate_in_month_calendar()	find out where we are in the calendar
 *					pop up a one-day list menu
 *	draw_month_year()		Draw the month and year into the
 *					main calendar window
 *	draw_calendar()			Draw day grid into the main calendar
 *					window
 *	draw_day(day)			Draw one day box into the day grid
 *					in the main calendar window
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <Xm/Xm.h>
#include "cal.h"

static void draw_day_notes	(int);
static BOOL set_day_bkg_color	(int);
static int day_to_pos		(int *, int *, int);
static int pos_to_day		(int *, int, int);
#ifdef JAPAN
static int chkctype		(char, int);
static int mixedstrtok		(char *, char *);
static void mixedstr_draw	(Window, int, int,char*, int, int, int);

extern char		*localename;	/* locale name */
extern unsigned short	(*kanji2jis)();	/* kanji2jis == e2j or sj2j */
#endif
extern Display		*display;	/* everybody uses the same server */
extern GC		jgc, gc;	/* graphic context, Japanese and std */
extern GC		chk_gc;		/* checkered gc for light background */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern struct mainmenu	mainmenu;	/* all important main window widgets */
extern struct config	config;		/* global configuration data */
extern struct user	*user;		/* user list (from file_r.c) */
extern int		curr_month;	/* month being displayed, 0..11 */
extern int		curr_year;	/* year being displayed, since 1900 */
extern time_t		curr_week;	/* week being displayed, time in sec */
extern time_t		curr_day;	/* day being displayed, time in sec */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern struct holiday	holiday[366];	/* info for each day, separate for */
extern struct holiday	sm_holiday[366];/* full-line texts under, and small */
extern short		monthbegin[12];	/* julian date each month begins with*/
extern short		monthlen[];	/* length of each month */
static int		lines_per_day;	/* max # of entries in a day box */
static int		line_height;	/* height of one entry line in pixels*/
static struct entry	**drawn;	/* 31*n entries drawn in month cal, */
					/* for locating entries for drag&drop*/
extern char *weekday_name[];
extern char *monthname[];


/*
 * for a given position on the canvas, identify the entry there and return
 * a pointer to it and which area it was hit in (one of the M_ constants).
 * Locate an event if epp!=0, and return exactly where the event was hit.
 * If none was found, return the time if possible for creating new events.
 * If epp is 0, we are dragging an event right now; just return a new time.
 * In month calendars, there are no multiday appointments and the trigger
 * always agrees with the time if set.
 *
 *   ret	*epp	*time	*trig	xp..ysp	event
 *
 *   M_ILLEGAL	0	0	-	-	no recognizable position
 *   M_OUTSIDE	0	time	-	-	no event but in a valid day
 *   M_INSIDE	entry	time	trigg	-	in event center, edit it
 *   M_LEFT	entry	time	trigg	pos/sz	near left edge, move start date
 *   M_RIGHT	entry	time	trigg	pos/sz	near right edge, move end date
 *   M_DAY_CAL	-	time	-	-	pop up a day calendar view
 *   M_WEEK_CAL	-	time	-	-	pop up a week calendar view
 */

MOUSE locate_in_month_calendar(
	struct entry	**epp,		/* returned entry or 0 */
	BOOL		*warn,		/* is *epp an n-days-ahead w?*/
	time_t		*time,		/* if *epp=0, clicked where */
	UNUSED time_t	*trigger,	/* if *epp=0, entry trigger */
	int		*xp,		/* if M_INSIDE, rubber box position */
	int		*yp,
	int		*xsp,		/* and size */
	int		*ysp,
	int		xc,		/* pixel pos clicked */
	int		yc)
{
	struct config	*c = &config;
	int		xd, yd;		/* top left of clicked day box */
	int		day, n;		/* clicked day number, note line */
	struct tm	tm;		/* for pos->time_t conversion */
	int		sm = c->smallmonth;

	tm.tm_year = curr_year;
	tm.tm_mon  = curr_month;
	tm.tm_mday = 1;
	tm.tm_hour = 0;
	tm.tm_min  = 0;
	tm.tm_sec  = 0;
							/* call week view? */
	if (xc < c->calbox_marg[sm] + c->calbox_arrow[sm]) {
		yc -= c->calbox_marg[sm] + c->calbox_title[sm];
		yc /= c->calbox_ys[sm];
		(void)day_to_pos(&xd, &yd, 1);
		tm.tm_mday = 7 * yc - xd +1;
		if (tm.tm_mday < 1) {
			if (!tm.tm_mon--) {
				tm.tm_mon = 11;
				tm.tm_year--;
			}
			tm.tm_mday += monthlen[tm.tm_mon] +
					(tm.tm_mon==1 && !(tm.tm_year&3));
		}
		*time = tm_to_time(&tm);
		return(*time != (time_t)-1 ? M_WEEK_CAL : M_ILLEGAL);
	}
							/* call day view? */
	if (!pos_to_day(&day, xc, yc))
		return(M_ILLEGAL);
	if (day_to_pos(&xd, &yd, day)) {
		yd = c->calbox_marg[sm] + c->calbox_title[sm] +
							yd * c->calbox_ys[sm];
		if (yc < yd + font[FONT_DAY+sm]->max_bounds.ascent) {
			tm.tm_mday = day;
			*time = tm_to_time(&tm);
			return(M_DAY_CAL);
		}
	}
							/* hit appointment */
	if (warn)
		*warn = FALSE;
	if (epp)
		*epp = 0;
	if ((*time = date_to_time(day, curr_month, curr_year, 0, 0, 0))) {
		int day_xs = c->calbox_xs[sm];
		int yo = font[FONT_DAY+sm]->max_bounds.ascent;
		xd  = c->calbox_marg[sm] + c->calbox_arrow[sm] + xd * day_xs;
		yd += yo + (c->calbox_ys[sm]-yo-lines_per_day*line_height)/2+3;
		n   = (yc - yd) / line_height;
		assert(drawn && day >= 1 && day <= 31);
		if (!epp || n < 0 || n > lines_per_day ||
				!(*epp = drawn[(day-1) * lines_per_day + n]))
			return(M_OUTSIDE);
		if (xp) {
			*xp   = xd + 1;
			*yp   = yd + n * line_height;
			*xsp  = day_xs - 5;
			*ysp  = line_height - 1;
		}
		return(xc < xd + day_xs/3   ? M_LEFT  : M_INSIDE);
	}
	return(M_ILLEGAL);
}


/*-------------------------------------------------- drawing ----------------*/
/*
 * draw the weekday names, depending on the Sunday-first flag
 */

void draw_month_year(void)
{
	print_button(mainmenu.month, _(monthname[curr_month]));
	print_button(mainmenu.year, "%d", curr_year+1900);
}


/*
 * draw all day boxes into a clip region, or all if the region is null
 */

void draw_calendar(
	Region		region)
{
	struct config	*c = &config;
	Window		window;
	XRectangle	rects[7+8], *r = rects;
	XPoint		point[4];
	int		i, j, x, y;
	int		sm = c->smallmonth;

	if (!mainmenu.cal) {
		if (region)
			XDestroyRegion(region);
		return;
	}
	if (region) {
		XSetRegion(display, gc,     region);
		XSetRegion(display, chk_gc, region);
	}
	window = XtWindow(mainmenu.cal);
	set_color(COL_CALBACK);				/* clear */
	XFillRectangle(display, window, gc, 0, 0,
		2*c->calbox_marg[sm] + c->calbox_xs[sm] * 7
				     + c->calbox_arrow[sm],
		2*c->calbox_marg[sm] + c->calbox_ys[sm] * 6
				     + c->calbox_title[sm]);

							/* vert lines */
	for (i=0; i < 8; i++, r++) {
		r->x	  = c->calbox_marg[sm] + i * c->calbox_xs[sm]
					       + c->calbox_arrow[sm];
		r->y	  = c->calbox_marg[sm] + c->calbox_title[sm];
		r->width  = 3;
		r->height = c->calbox_ys[sm] * 6;
	}
							/* horz lines */
	for (i=0; i < 7; i++, r++) {
		r->x	  = c->calbox_marg[sm] + c->calbox_arrow[sm];
		r->y	  = c->calbox_marg[sm] + c->calbox_title[sm]
					       + c->calbox_ys[sm] * i;
		r->width  = c->calbox_xs[sm] * 7 + 3;
		r->height = 3;
	}
	set_color(COL_GRID);
	XFillRectangles(display, window, gc, rects, 7+8);
	set_color(COL_CALFRAME);
	XDrawRectangle(display, window, gc,
			c->calbox_marg[sm] + c->calbox_arrow[sm] -1,
			c->calbox_marg[sm] + c->calbox_title[sm] -1,
			c->calbox_xs[sm] * 7 + 4,
			c->calbox_ys[sm] * 6 + 4);

	point[3].x =					/* week call arrows */
	point[0].x =
	point[1].x = c->calbox_marg[sm]/2 +1;
	point[2].x = point[0].x + c->calbox_arrow[sm] -1;
	point[2].y = c->calbox_marg[sm] + c->calbox_title[sm]
					+ c->calbox_ys[sm] / 2;
	point[3].y =
	point[0].y = point[2].y - (point[2].x - point[0].x);
	point[1].y = point[2].y + (point[2].x - point[0].x);
	if (c->calbox_arrow[sm] > 3)
		for (i=0; i < 6; i++) {
			set_color(COL_CALSHADE);
			XFillPolygon(display, window, gc, point, 3, Convex,
							CoordModeOrigin);
			set_color(COL_STD);
			XDrawLines(display, window, gc, point, 4,
							CoordModeOrigin);
			point[0].y += c->calbox_ys[sm];
			point[1].y += c->calbox_ys[sm];
			point[2].y += c->calbox_ys[sm];
			point[3].y += c->calbox_ys[sm];
		}
	set_color(COL_STD);

							/* weekday names */
	j = c->sunday_first ? 6 : 0;
	XSetFont(display, gc, font[sm ? FONT_NOTE : FONT_STD]->fid);
	for (i=0; i < 7; i++, j++) {
		char *wdn = _(weekday_name[j%7]);
		XDrawString(display, window, gc,
			c->calbox_marg[sm] + c->calbox_arrow[sm]
					   + c->calbox_xs[sm] * i,
			c->calbox_marg[sm] + c->calbox_title[sm] * 3/4,
			wdn, strlen(wdn));
	}
	day_to_pos(&x, &y, 1);				/* day boxes */
	for (i=1-x; i < 43-x; i++)
		draw_day(i);

	if (region) {
		XSetClipMask(display, gc,     None);
		XSetClipMask(display, chk_gc, None);
		XDestroyRegion(region);
	}
}


/*
 * draw one day box.
 */

void draw_day(
	int		day)		/* 1..31 : current month */
					/* < 1 : previous month */
					/* > monthlen : next month */
{
	Window		window;
	char		buf[200];	/* julian, weeknum, holiday */
	int		x, y;		/* position in calendar */
	int		jul;		/* julian date of daybox */
	int		daynum_xs;	/* width of day# in pixels */
	int		tmp_curr_month;	/* tmp storage for curr_month*/
	int		tmp_curr_year;	/* tmp storage for curr_year */
	int		orig_day;	/* original value of day */
	int		last_day;	/* last day of current month */
	BOOL		leftedge;	/* in leftmost daybox? */
	BOOL		weekend;	/* saturday or sunday? */
	BOOL		today;		/* is this today's box? */
	BOOL		othermonth;	/* not current month ? */
	struct holiday	*hp, *shp;	/* to check for holidays */
	char		*errmsg;	/* holiday parser error */
	int		sm = config.smallmonth;

	if (!mainmenu.cal)
		return;
	orig_day = day;
	tmp_curr_month = curr_month;
	tmp_curr_year  = curr_year;
	last_day = monthlen[curr_month] + (curr_month==1 && !(curr_year&3));
	if (day > last_day) {
		day_to_pos(&x, &y, last_day);
		othermonth = TRUE;
		day = day - last_day;
		curr_month = (curr_month+1) % 12;
		if (curr_month == 0)
			curr_year++;
		y = (y*7 + x + day) / 7;
		x = (x + day) % 7;
	} else {
		day_to_pos(&x, &y, day);
		if (day < 1) {
			othermonth = TRUE;
			curr_month = curr_month == 0 ? 11 : curr_month-1;
			if (curr_month == 11)
				curr_year--;
			last_day = monthlen[curr_month] +
					(curr_month==1 && !(curr_year&3));
			day = last_day + day;
		} else
			othermonth = FALSE;
	}
	if (config.nomonthshadow && othermonth) {
		curr_month = tmp_curr_month;
		curr_year  = tmp_curr_year;
		return;
	}
	window = XtWindow(mainmenu.cal);
	weekend = x == 6 || x == (config.sunday_first ? 0 : 5);
	leftedge = x == 0;
	daynum_xs = 5 + 2 * CHARWIDTH(FONT_DAY+sm, '0');
	x = config.calbox_marg[sm] + x * config.calbox_xs[sm] + 3 +
					 config.calbox_arrow[sm];
	y = config.calbox_marg[sm] + y * config.calbox_ys[sm] + 3 +
					 config.calbox_title[sm];
	today = set_day_bkg_color(day);
	XFillRectangle(display, window, gc, x, y, config.calbox_xs[sm]-3,
						  config.calbox_ys[sm]-3);
	sprintf(buf, "%d", day);
	XSetFont(display, gc, font[FONT_DAY+sm]->fid);

	if ((errmsg = parse_holidays(curr_year, FALSE)))
		create_error_popup(mainmenu.cal, 0, errmsg);
	jul = monthbegin[curr_month] + day-1 +(curr_month>1 && !(curr_year&3));
	hp  =    &holiday[jul];
	shp = &sm_holiday[jul];

							/* day number */
	set_color(othermonth    ? COL_NOTEOFF   :
		  hp->daycolor	?  hp->daycolor :
		  shp->daycolor	? shp->daycolor :
		  weekend	? COL_WEEKEND	: COL_WEEKDAY);

	XDrawString(display, window, gc,
		x+2, y+font[FONT_DAY+sm]->max_bounds.ascent, buf, strlen(buf));

							/* julian & week num */
	if (config.julian || config.weeknum) {
		int wkday, weeknum;
		(void)date_to_time(day, curr_month, curr_year,
						&wkday, &jul, &weeknum);
		*buf = 0;
		if (config.weeknum && leftedge) {
			if (config.gpsweek)
				sprintf(buf, "[%d]", ((curr_year * 365
						+ (curr_year+3)/4
						+ jul) / 7 + 945) & 1023);
			else
				sprintf(buf, "(%d)", weeknum+1);
		}
		if (config.julian)
			sprintf(buf+strlen(buf), "%d", jul+1);
		truncate_string(buf, config.calbox_xs[sm] - 3 - daynum_xs,
								FONT_NOTE);
		XSetFont(display, gc, font[FONT_NOTE]->fid);
		XDrawString(display, window, gc,
			x + daynum_xs,
			y + font[FONT_NOTE]->max_bounds.ascent,
							buf, strlen(buf));

	}
							/* small holiday */
	if (shp->string && ((!config.julian && !config.weeknum) ||
				font[FONT_DAY+sm]->max_bounds.ascent >=
				font[FONT_NOTE]->max_bounds.ascent * 2 - 1)) {
		strncpy(buf, shp->string, sizeof(buf)-1);
		buf[sizeof(buf)-1] = 0;
#ifndef JAPAN
		truncate_string(buf, config.calbox_xs[sm] - 3 - daynum_xs,
								FONT_NOTE);
#endif
		if (othermonth == TRUE)
			set_color(COL_NOTEOFF);
		else if (shp->stringcolor)
			set_color(shp->stringcolor);
		else if (shp->daycolor)
			set_color(shp->daycolor);
		XSetFont(display, gc, font[FONT_NOTE]->fid);
#ifdef JAPAN
		XSetFont(display, jgc, font[FONT_JNOTE]->fid);
		mixedstr_draw(window, x + daynum_xs,
			      y + font[FONT_DAY+sm]->max_bounds.ascent, buf,
			      config.calbox_xs[sm] - 3 - daynum_xs,
			      FONT_NOTE, FONT_JNOTE);
#else
		XDrawString(display, window, gc,
			x + daynum_xs,
			y + font[FONT_DAY+sm]->max_bounds.ascent,
							buf, strlen(buf));
#endif
	}
							/* entry notes */
	set_color(COL_STD);
	curr_month = tmp_curr_month;
	curr_year  = tmp_curr_year;
	draw_day_notes(orig_day);
							/* opt. today frame */
	if (today && config.frame_today) {
		XDrawRectangle(display, window, gc, x+1, y+1,
			       config.calbox_xs[sm]-6, config.calbox_ys[sm]-6);
		XDrawRectangle(display, window, gc, x+2, y+2,
			       config.calbox_xs[sm]-8, config.calbox_ys[sm]-8);
	}
}


/*
 * draw (or undraw) the notes in a day box. Looks up the day in the database.
 * Remember which entry was drawn where; this simplifies drag&drop because
 * it's easy to find the entry under the cursor.
 */

static void draw_day_notes(
	int		day)		/* 1..31 */
{
	BOOL		today;		/* TRUE: it's today's daybox */
	struct tm	*tm;		/* for daylight-saving tzone */
	time_t		tod;		/* current time-of-day */
	time_t		daytime;	/* time of day box, in sec */
	BOOL		found;		/* TRUE if lookup succeeded */
	BOOL		othermonth;	/* not current month ? */
	int		tmp_curr_month;	/* tmp storage for curr_month*/
	int		tmp_curr_year;	/* tmp storage for curr_year */
	int		last_day;	/* last day of current month */
	struct lookup	lookup;		/* result of entry lookup */
	struct entry	*ep;		/* ptr to entry in mainlist */
	int		u;		/* if other user, user[] indx*/
	int		x, y;		/* position in calendar */
	int		xo, i;		/* for aligning ':' in time */
	int		ye;		/* bottom limit of day box */
	int		ys;		/* height of a text line */
	char		buf[100];	/* string buffer (time/note) */
	int		jul;		/* julian date of daybox */
	int		nlines = 0;	/* # printed, <lines_per_day */
	struct holiday	*hp;		/* holiday info for this day */
	char		*errmsg;	/* parser error */
	Window		window;
	int		sm = config.smallmonth;
	int		ascent, descent;
	struct entry	**drawn_entry = 0;

	if (!mainmenu.cal)
		return;
							/* which day box? */
	tmp_curr_month = curr_month;
	tmp_curr_year = curr_year;
	last_day = monthlen[curr_month] + (curr_month==1 && !(curr_year&3));
	if (day > last_day) {
		day_to_pos(&x, &y, last_day);
		othermonth = TRUE;
		day = day - last_day;
		curr_month = (curr_month+1) % 12;
		if (curr_month == 0)
			curr_year++;
		y = (y*7 + x + day) / 7;
		x = (x + day) % 7;
	} else {
		day_to_pos(&x, &y, day);
		if (day < 1) {
			othermonth = TRUE;
			curr_month = curr_month == 0 ? 11 : curr_month-1;
			if (curr_month == 11)
				curr_year--;
			last_day = monthlen[curr_month] +
					(curr_month==1 && !(curr_year&3));
			day = last_day + day;
		} else
			othermonth = FALSE;
	}
	daytime = date_to_time(day, curr_month, curr_year, 0, 0, 0);

	window = XtWindow(mainmenu.cal);
	tod    = get_time();
	tm     = time_to_tm(tod);
	today  = day        == tm->tm_mday &&
		 curr_month == tm->tm_mon  &&
		 curr_year  == tm->tm_year;
							/* day box geometry */
	x  = config.calbox_marg[sm] + x * config.calbox_xs[sm] + 3 +
						config.calbox_arrow[sm];
	y  = config.calbox_marg[sm] + y * config.calbox_ys[sm] + 3 +
						config.calbox_title[sm];
	ye = y + config.calbox_ys[sm] - 3;

	ascent  = font[FONT_NOTE]->max_bounds.ascent;
	descent = font[FONT_NOTE]->max_bounds.descent/2;
	XSetFont(display, gc,  font[FONT_NOTE]->fid);
#ifdef JAPAN
	XSetFont(display, jgc, font[FONT_JNOTE]->fid);
	if (ascent + descent < font[FONT_JNOTE]->max_bounds.ascent +
			       font[FONT_JNOTE]->max_bounds.descent/2) {
		ascent  = font[FONT_JNOTE]->max_bounds.ascent;
		descent = font[FONT_JNOTE]->max_bounds.descent/2;
	}
#endif
	ys = ascent + descent;
	y += font[FONT_DAY+sm]->max_bounds.ascent + (ye - y) % ys / 2;

	if (!othermonth) {				/* drawn entry buffer*/
		i = (ye - y) / ys;
		if (i < 0) i = 0;
		if (!drawn || i > lines_per_day) {
			if (drawn)
				free(drawn);
			drawn = (struct entry **)malloc(31 * i *
							sizeof(struct entry *));
			memset(drawn, 0, 31 * i * sizeof(struct entry *));
			lines_per_day = i;
			line_height   = ys;
		}
		drawn_entry = drawn + (day - 1) * i;
		memset(drawn_entry, 0, i * sizeof(struct entry *));
	}
							/* background */
	(void)set_day_bkg_color(day);
	XFillRectangle(display, window, gc, x,y, config.calbox_xs[sm]-3, ye-y);

							/* large holiday? */
	if ((errmsg = parse_holidays(curr_year, FALSE)))
		create_error_popup(mainmenu.cal, 0, errmsg);
	jul = monthbegin[curr_month] +day-1 + (curr_month>1 && !(curr_year&3));
	hp  = &holiday[jul];
	if (hp->string && *hp->string && y + descent*2 <= ye) {
		y += ascent;
		set_color(othermonth ? COL_NOTEOFF :
			   hp->stringcolor ? hp->stringcolor : COL_NOTE);
#ifdef JAPAN
		mixedstr_draw(window, x+2, y, hp->string,
				config.calbox_xs[sm]-3, FONT_NOTE, FONT_JNOTE);
#else
		strncpy(buf, hp->string, sizeof(buf)-1);
		truncate_string(buf, config.calbox_xs[sm] - 3, FONT_NOTE);
		XDrawString(display, window, gc, x+2, y, buf, strlen(buf));
#endif
		y += descent/2;
		if (drawn_entry) {
			nlines++;
			drawn_entry++;
		}
	}
							/* entry loop */
	found = lookup_entry(&lookup, mainlist, daytime, TRUE, FALSE, 'm', -1);
	for (; found; found = lookup_next_entry(&lookup)) {

		ep = &mainlist->entry[lookup.index];
		u = name_to_user(ep->user);
		if (ep->hide_in_m || user[u].suspend_m)
			continue;
		for (i=0; i < NEXC; i++)
			if (ep->except[i] == daytime)
				break;
		if (i < NEXC)
			continue;
		if (lookup.index >= mainlist->nentries	||
		    lookup.trigger <  daytime		||
		    lookup.trigger >= daytime + 86400)
			break;
		if (today && config.nopast &&
		    lookup.trigger < tod   &&
		    !ep->notime)
			continue;

		if (y + font[FONT_NOTE]->max_bounds.descent
		      + font[FONT_NOTE]->max_bounds.descent > ye) {
			int xe = x + config.calbox_xs[sm] -3;
			set_color(othermonth ? COL_NOTEOFF : COL_NOTE);
			XFillRectangle(display, window, gc, xe-4,  ye-3, 2, 2);
			XFillRectangle(display, window, gc, xe-8,  ye-3, 2, 2);
			XFillRectangle(display, window, gc, xe-12, ye-3, 2, 2);
			break;
		}
		if (config.colorcode_m && u > 0 && !othermonth) {
			set_color(COL_WUSER_0 + (user[u].color & 7));
			XFillRectangle(display, window, gc,
					x, y+1, config.calbox_xs[sm] - 3,
					font[FONT_NOTE]->max_bounds.ascent+1);
			if (user[u].color > 7)
				XFillRectangle(display, window, chk_gc,
					x, y+1, config.calbox_xs[sm] - 3,
					font[FONT_NOTE]->max_bounds.ascent+1);
		}
		if (ep->notime)
			sprintf(buf, "%s%.90s", lookup.days_warn ? "W: " : "",
						mknotestring(ep));
		else
			sprintf(buf, "%s %s%.90s", mktimestring(lookup.trigger,
								FALSE),
						lookup.days_warn ? "W: " : "",
						mknotestring(ep));
#ifdef JAPAN
		y += (font[FONT_NOTE ]->max_bounds.ascent <
		      font[FONT_JNOTE]->max_bounds.ascent) ?
		      font[FONT_JNOTE]->max_bounds.ascent :
		      font[FONT_NOTE ]->max_bounds.ascent;

		set_color(othermonth    ? COL_NOTEOFF :
			  ep->suspended ? COL_NOTEOFF :
			  ep->acolor    ? COL_HBLACK + ep->acolor-1
			  		: COL_NOTE);
		mixedstr_draw(window, x+2, y, buf, config.calbox_xs[sm] - 3,
			      FONT_NOTE, FONT_JNOTE);
#else
		truncate_string(buf, config.calbox_xs[sm] - 5, FONT_NOTE);
		y += font[FONT_NOTE]->max_bounds.ascent;
		set_color(othermonth    ? COL_NOTEOFF :
			  ep->suspended ? COL_NOTEOFF :
			  ep->acolor    ? COL_HBLACK + ep->acolor-1
			  		: COL_NOTE);
		xo = CHARWIDTH(FONT_NOTE, '0') - CHARWIDTH(FONT_NOTE, *buf);
		if (xo < 0) xo = 0;
		XDrawString(display, window, gc, x+2+xo, y, buf, strlen(buf));
#endif
		y += font[FONT_NOTE]->max_bounds.descent/2;
		if (drawn_entry && nlines++ < lines_per_day)
			*drawn_entry++ = ep;
	}
	set_color(COL_STD);
	curr_month = tmp_curr_month;
	curr_year  = tmp_curr_year;
}


static BOOL set_day_bkg_color(
	int			day)		/* 1..31 */
{
	struct tm		*tm;
	BOOL			ret;

	tm = time_to_tm(get_time());
	ret = day == tm->tm_mday && curr_month == tm->tm_mon
				 && curr_year  == tm->tm_year;
	set_color(ret ? COL_CALTODAY : COL_CALSHADE);
	return(ret);
}


/*-------------------------------------------------- low-level --------------*/
/*
 * convert day numbers 1..31 to a pixel position, and vice versa.
 */

static int day_to_pos(
	int			*x,		/* returned box, 0..6/0..5 */
	int			*y,
	int			day)		/* 1..31 */
{
	int			wkday;

	if (!date_to_time(day, curr_month, curr_year, &wkday, 0, 0))
		return(FALSE);
	*x = config.sunday_first ? wkday : (wkday + 6) % 7;
	*y = (day-1 - *x + 6) / 7;
	return(TRUE);
}


static int pos_to_day(
	int			*day,		/* returned day #, 1..31 */
	int			x,		/* pixel pos in drawable */
	int			y)
{
	int			xtest, ytest;
	int			sm = config.smallmonth;

	(void)day_to_pos(&xtest, &ytest, 1);
	x -= config.calbox_marg[sm] + config.calbox_arrow[sm];
	y -= config.calbox_marg[sm] + config.calbox_title[sm];
	if (x < 0 || y < 0)
		return(FALSE);
	x /= config.calbox_xs[sm];
	y /= config.calbox_ys[sm];
	*day = y * 7 + x - xtest + 1;
	if (*day < 1)
		return(FALSE);
	return(day_to_pos(&xtest, &ytest, *day));
}


/*
 * the rest of this file is for Japanese version only. Written by Ogura
 * Yoshito, like everything else enclosed in #ifdef JAPAN and #endif.
 */

#ifdef JAPAN

#define CT_ASC 0
#define CT_KJ1 1
#define CT_KJ2 2
#define CT_UHOH 3

/*
 *	chkctype() and e2j() is originally coded by
 * NISHIJIMA, Takanori (racsho@cpdc.canon.co.jp).
 * Ogura Yoshito changed a few lines of chkctype();
 */

static int chkctype(
	char	ch,
	int 	stat)
{
	switch (stat) {
	  case CT_ASC:
	  case CT_KJ2:
		if (ch & 0x80)
			return CT_KJ1;
		else
			return CT_ASC;
		break;
	  case CT_KJ1:
		if (ch)
			return CT_KJ2;
		else
			return CT_UHOH;
		break;
	  default:
		return CT_UHOH;
	}
}

unsigned short e2j(
	unsigned short wc)
{
	return (wc & 0x7F7F);
}


/*
 *  sj2j() is coded with referring to nkf. This function is called
 * very frequently. So you can obtain better response with modification.
 * nkf, Network Kanji Filter
 * AUTHOR: Itaru ICHIKAWA (ichikawa@flab.fujitsu.co.jp)
 */

unsigned short sj2j(
	unsigned short wc)
{
	return (((((wc >> 8) << 1) - (((wc >> 8) > 0x9f) << 7) - 0xe1) << 8) +
		((wc & 0x00ff) >= 0x9f ? (wc & 0x00ff) - 0x7e | 0x100
				       : (wc & 0x00ff) - 0x1f
						       - ((wc & 0x00ff) >> 7)));
}


static int mixedstrtok(
	char			*jisstr,
	char			*sjstr)
{
	unsigned char		*dptr = (unsigned char *)jisstr,
				*sptr = (unsigned char *)sjstr; 
	unsigned short		wc;	/* works for a 2-byte character */
	int			prev_ct = CT_ASC, stat;

	if (sptr == NULL) {
		*dptr = '\0';
		return (-1);	/* Illegal pointer. */
	}
	stat = chkctype(*(char *)sptr, CT_ASC);
	while (*sptr != '\0') {
		switch (prev_ct = chkctype(*sptr, prev_ct)) {
		  case CT_ASC:
			if (stat != CT_ASC)
				goto STATCHG;
			*dptr++ = *sptr++;
			break;
		  case CT_KJ1:
			if (stat == CT_ASC)
				goto STATCHG;
			wc = *sptr++ << 8;
			break;
		  case CT_KJ2:
			if (stat == CT_ASC)
				goto STATCHG;
			wc |= *sptr++;
			wc = (*kanji2jis)(wc);	/* kanji2jis == e2j or sj2j */
			*dptr++ = wc >> 8;
			*dptr++ = wc;
			break;
		  default:
			*dptr = '\0';
			return (-1);		/* Parse error. */
		}
	}
STATCHG:
	*dptr = '\0';
	if (jisstr != (char *)dptr)		/* 1: ASCII, 2: SJIS string */
		return(stat == CT_ASC ? 1 : 2);
	return(0);
}


int mixedstrlen_in_pixels(
	char			*sjstr,
	strpack			*stp,
	int			ascfont,
        int                     jfont)
{
	int			jlen, plen = 0, i = 0, j = 0, k;
	char			*sjptr = sjstr;
	unsigned char		buf[100], *cpyptr, *strpool = stp->strptr;

	k = mixedstrtok(cpyptr = buf, sjptr);
	do {
		if (k <= 0) {
			if (k)
				fprintf(stderr,_("String conversion error.\n"));
			stp[i].strptr = NULL;
			break;
		}
		if (j+1+(jlen = stp[i].length = strlen(buf))>=MAXPARTIALCHAR) {
			if (jlen = stp[i].length = (MAXPARTIALCHAR-j-1 & ~1)) {
				for (stp[i].strptr = strpool + j;
						j < MAXPARTIALCHAR-1; j++)
					strpool[j] = *cpyptr++;
				strpool[j] = '\0';
				if (k == 1) {
					stp[i].asciistr = True;
					plen += stp[i].pixlen = XTextWidth(
						    font[ascfont], buf, jlen);
				} else {
					stp[i].asciistr = False;
					plen += stp[i].pixlen = XTextWidth16(
						    font[jfont],
						    (XChar2b *)buf, jlen/2);
				}
				if (++i < MAXPARTIALSTRING)
					stp[i].strptr = NULL;
			} else
				stp[i].strptr = NULL;
			break;
		}
		sjptr += jlen;
		stp[i].strptr = strpool + j;
		while (*cpyptr != '\0')
			strpool[j++] = *cpyptr++;
		strpool[j++] = '\0';
		if (k == 1) {
			stp[i].asciistr = True;
			plen += stp[i].pixlen = XTextWidth(font[ascfont],
							buf, jlen);
		} else {
			stp[i].asciistr = False;
			plen += stp[i].pixlen = XTextWidth16(font[jfont],
						(XChar2b *)buf, jlen/2);
		}
	} while (++i < MAXPARTIALSTRING	&&
				((k = mixedstrtok(cpyptr = buf, sjptr)) ||
						((stp[i].strptr = NULL), 0)));
	return plen;
}


static void mixedstr_draw(
			  Window window,
			  int x,/* coordinate */
			  int y,
			  char *mxstr,/* string to draw */
			  int maxpix,        /* Max width in pixels. */
			  int ascfont,/* fonts of string */
			  int jfont )
{
	int		i, len, deltax = 0;
	char		buf[100];

	i = mixedstrtok(buf, mxstr);
	do {
		if (i <= 0) {
			if (i)
				fprintf(stderr,_("String conversion error.\n"));
			break;
		}
		mxstr += (len = strlen(buf));
		if (i == 1) {
			i = XTextWidth(font[ascfont], buf, len);
			if (maxpix < i) {
				/*
				 * It makes me happy 'truncate_string'
				 * to return length of truncated string.
				XDrawString(display, window, gc,
					x+deltax, y, buf,
					truncate_string(buf, maxpix, ascfont);
				 */
				(void)truncate_string(buf, maxpix, ascfont);
				XDrawString(display, window, gc, x+deltax, y,
					    buf, strlen(buf));
				break;
			} else
				XDrawString(display, window, gc, x+deltax, y,
					    buf, len);
		}
		else {
			i = XTextWidth16(font[jfont], (XChar2b *)buf, len/2);
			if(maxpix < i)
				len *= (double)maxpix/(double)i;
			XDrawString16(display, window, jgc, x+deltax, y,
				      (XChar2b *)buf, len/2);
		}
		deltax += i;
		maxpix -= i;
	} while (maxpix > 0 && (i = mixedstrtok(buf, mxstr)));
}
#endif
