/*
 * year menu widgets.
 *
 *	draw_year_calendar()		Draw day grid into the year calendar
 *					window
 *	draw_year_day(day, month, year)	Draw one day box into the day grid
 *					in the year calendar window
 */

#include <stdio.h>
#include <time.h>
#include <Xm/Xm.h>
#include "cal.h"

static void draw_year_month	(int, int, int, int);
static void draw_year_daybox	(int, int, int, int, int);
static void draw_year_day_notes	(int, int, int, BOOL);
static void set_year_day_bkg_color(int, int, int);
static BOOL day_to_yearpos	(int *, int *, int, int, int);
static BOOL yearpos_to_day_month(int *, int *, BOOL *, BOOL *, int, int);

static int		qx,qy,qxs,qys;	/* pos and size of quit button */

extern Display		*display;	/* everybody uses the same server */
extern GC		gc;		/* everybody uses this context */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern Pixmap		pixmap[NPICS];	/* common symbols */
extern Widget		mainwindow;	/* raise month shell if clicked */
extern struct mainmenu	mainmenu;	/* all important main window widgets */
extern struct config	config;		/* global configuration data */
extern int		curr_month;	/* month being displayed, 0..11 */
extern int		curr_year;	/* year being displayed in cal menu */
extern time_t		curr_week;	/* week being displayed, time in sec */
extern time_t		curr_day;	/* day being displayed, time in sec */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern struct holiday	holiday[366];	/* info for each day, separate for */
extern struct holiday	sm_holiday[366];/* full-line texts under, and small */
extern short		monthbegin[12];	/* julian date each month begins with*/

extern char *weekday_name[];
extern char *monthname[];
extern short monthlen[];


/*
 * got a click in the calendar area. If it's in a grid box with a valid
 * day init, "edit" it by storing its day number in the edit_* variables.
 * The previously edited day, and the new edited day are redrawn to move
 * the yellow highlight.
 * Also, create a day popup menu.
 */

int clicked_year_calendar(
	int		x,		/* pixel pos clicked */
	int		y)
{
	int		day, month;
	BOOL		callweek;	/* if TRUE, pop up week view */
	BOOL		callday;	/* if TRUE, pop up day view */
	struct tm	tm;		/* for pos->time_t conversion */
	time_t		time;		/* time of day box, in sec */

	if (x >= qx && y > qy && x < qx+qxs && y < qy+qys)
		return('Q');
	if (!yearpos_to_day_month(&day, &month, &callweek, &callday, x, y))
		return(0);
	tm.tm_hour = 0;
	tm.tm_min  = 0;
	tm.tm_sec  = 0;
	tm.tm_year = curr_year;
	tm.tm_mon  = month;
	tm.tm_mday = day;

	if (callweek) {					/* call week view? */
		curr_week = tm_to_time(&tm);
		return('w');
	}
	if (callday) {					/* call week view? */
		curr_day = tm_to_time(&tm);
		return('d');
	}
	if (day == 0) {					/* call month view? */
		curr_month = month;
		draw_month_year();
		draw_calendar(NULL);
		XRaiseWindow(display, XtWindow(mainwindow));
		return('m');
	}
	if ((time = date_to_time(day, month, curr_year, 0, 0, 0)))
		create_list_popup(mainlist, time,
			(time_t)0, (char *)0, (struct entry *)0, 0, 0);
	return(0);
}


/*-------------------------------------------------- drawing ----------------*/
/*
 * draw the entire year menu
 */

void draw_year_calendar(
	Region		region)
{
	struct config	*c = &config;
	Window		window;
	char		buf[20];
	int		month;
	int		i;

	if (!mainmenu.yearcal) {
		if (region)
			XDestroyRegion(region);
		return;
	}
	if (region)
		XSetRegion(display, gc, region);

	window = XtWindow(mainmenu.yearcal);
	set_color(COL_YBACK);
	XFillRectangle(display, window, gc, 0, 0, 9999, 9999);

	set_color(COL_YGRID);				/* quit button */
	qx  = qy = 10;
	qxs = 10 + strlen_in_pixels(_("Done"), FONT_STD);
	qys =  4 + font[FONT_STD]->max_bounds.ascent;
	XDrawRectangle(display, window, gc, qx, qy, qxs, qys);
	XSetFont(display, gc, font[FONT_STD]->fid);
	XDrawString(display, window, gc,
		qx+5, qy+font[FONT_STD]->max_bounds.ascent, _("Done"), 4);

	sprintf(buf, "%d", 1900 + curr_year);		/* year number */
	set_color(COL_YTITLE);
	XSetFont(display, gc, font[FONT_YTITLE]->fid);
	i = 4 * CHARWIDTH(FONT_YTITLE, '0');
	XDrawString(display, window, gc,
		c->year_margin + (c->yearbox_xs*7*4 + c->year_gap*3 - i)/2,
		c->year_margin + c->year_title * 3/4,
		buf, strlen(buf));

	for (month=0; month < 12; month++)		/* months */
		draw_year_month(month, curr_year,
			c->year_margin +
				month%4 * (c->yearbox_xs*7 + c->year_gap),
			c->year_margin + c->year_title + c->year_gap +
				month/4 * (c->yearbox_ys*8 + c->year_gap));
	set_color(COL_STD);

      if (region) {
      	XSetClipMask(display, gc, None);
		XDestroyRegion(region);
	}
}


/*
 * draw entire month at the specified position
 */

static void draw_year_month(
	int		month,
	int		year,
	int		x,
	int		y)
{
	struct config	*c = &config;
	Window		window = XtWindow(mainmenu.yearcal);
	XPoint		point[4];
	int		i, j;
	char		*mn;

							/* days */
	for (i=1; i < 32; i++)
		draw_year_daybox(i, month, year, x, y+2*c->yearbox_ys);

							/* month name */
	set_color(COL_YMONTH);
	XSetFont(display, gc, font[FONT_YMONTH]->fid);
	mn = monthname[month];
	XDrawString(display, window, gc,
			x, y + c->yearbox_ys * 3/4, mn, strlen(mn));

							/* weekday names */
	j = c->sunday_first ? 6 : 0;
	set_color(COL_YWEEKDAY);
	XSetFont(display, gc, font[FONT_YDAY]->fid);
	for (i=0; i < 7; i++, j++) {
		char *wdn = _(weekday_name[j%7]);
		XDrawString(display, window, gc,
				x + i * c->yearbox_xs, y + c->yearbox_ys * 7/4,
				wdn, strlen(wdn));
	}
							/* month box */
	set_color(COL_YGRID);
	XDrawRectangle(display, window, gc, x-1, y-1 + 2*c->yearbox_ys,
					7*c->yearbox_xs+1, 6*c->yearbox_ys+1);

	point[3].x =					/* week call arrows */
	point[0].x =
	point[1].x = x-8;
	point[2].x = x-4;
	point[2].y = y-1 + c->yearbox_ys * 5/2;
	point[3].y =
	point[0].y = point[2].y - (point[2].x - point[0].x);
	point[1].y = point[2].y + (point[2].x - point[0].x);
	if (c->calbox_arrow[0] > 3)
		for (i=0; i < 6; i++) {
			set_color(COL_YBOXBACK);
			XFillPolygon(display, window, gc, point, 3, Convex,
							CoordModeOrigin);
			set_color(COL_YGRID);
			XDrawLines(display, window, gc, point, 4,
							CoordModeOrigin);
			point[0].y += c->yearbox_ys;
			point[1].y += c->yearbox_ys;
			point[2].y += c->yearbox_ys;
			point[3].y += c->yearbox_ys;
		}
	set_color(COL_STD);
}


/*
 * draw one day box. The first routine is an interface for outside routines;
 * to turn the next day green at midnight etc. The second routine is for
 * internal use; it requires a X/Y coordinate.
 */

void draw_year_day(
	int		day,		/* 1..31 */
	int		month,		/* 0..11 */
	int		year)		/* year since 1900 */
{
	struct config	*c = &config;

	if (!mainmenu.yearcal || year != curr_year)
		return;
	draw_year_daybox(day, month, year,
			c->year_margin +
				month%4 * (c->yearbox_xs*7 + c->year_gap),
			c->year_margin + c->year_title + c->year_gap +
				month/4 * (c->yearbox_ys*8 + c->year_gap)+
				2*c->yearbox_ys);
}

static void draw_year_daybox(
	int			day,		/* 1..31 */
	int			month,
	int			year,
	int			x,		/* upper right of first day */
	int			y)
{
	Window			window = XtWindow(mainmenu.yearcal);
	int			dx, dy;		/* position in calendar */
	char			buf[20];
	BOOL			weekend;	/* saturday or sunday? */
	int			jul;		/* julian date of daybox */
	struct holiday		*hp, *shp;	/* holiday info for this day */
	char			*errmsg;	/* holiday parser error */

	if ((errmsg = parse_holidays(year, FALSE)))
		create_error_popup(mainmenu.cal, 0, errmsg);
	jul = monthbegin[month] + day-1 + (month>1 && !(year&3));
	hp  = &holiday[jul];
	shp = &sm_holiday[jul];

	if (!day_to_yearpos(&dx, &dy, day, month, year))
		return;
	weekend = dx == 6 || dx == (config.sunday_first ? 0 : 5);
	x += dx * config.yearbox_xs;
	y += dy * config.yearbox_ys;

	set_year_day_bkg_color(day, month, year);
	XFillRectangle(display, window, gc, x+1, y+1, config.yearbox_xs-2,
						      config.yearbox_ys-2);
	sprintf(buf, "%d", day);
	XSetFont(display, gc, font[FONT_YNUM]->fid);
	set_color( hp->daycolor	?  hp->daycolor :
		  shp->daycolor ? shp->daycolor :
		  weekend	? COL_WEEKEND	: COL_WEEKDAY);
	XDrawString(display, window, gc,
		x+2, y+font[FONT_YNUM]->max_bounds.ascent, buf, strlen(buf));

	draw_year_day_notes(day, month, year, TRUE);
}


/*
 * draw (or undraw) the notes in a day box. Looks up the day in the database.
 * This is very inefficient, it should cache the previously found entry. At
 * the moment, it's O(n^2).
 */

static void draw_year_day_notes(
	int		day,		/* 1..31 */
	int		month,		/* 0..11 */
	int		year,		/* year since 1900 */
	BOOL		ifyes)		/* only draw, never undraw */
{
	struct config	*c = &config;
	struct lookup	lookup;		/* result of entry lookup */
	time_t		time;		/* time of day box, in sec */
	int		x, y;		/* position in calendar */
	struct entry	*ep;		/* ptr to entry in mainlist */
	BOOL		found;		/* TRUE if lookup succeeded */
	int		i;		/* for checking exceptions */

	if (!mainmenu.yearcal || year != curr_year)
		return;
	if (!day_to_yearpos(&x, &y, day, month, year) ||
	    !(time = date_to_time(day, month, year, 0, 0, 0)))
		return;

	x  = c->year_margin +	month%4 * (c->yearbox_xs*7 + c->year_gap) +
				c->yearbox_xs - 8 - 2 +
				x * c->yearbox_xs;
	y  = c->year_margin + c->year_title + c->year_gap +
				month/4 * (c->yearbox_ys*8 + c->year_gap) +
				3*c->yearbox_ys -10 +
				y * c->yearbox_ys;

	found = lookup_entry(&lookup, mainlist, time, TRUE, FALSE, 'y', -1);
	for (; found; found = lookup_next_entry(&lookup)) {
		ep = &mainlist->entry[lookup.index];
		if (lookup.trigger >= time + 86400) {
			found = FALSE;
			break;
		}
		if (ep->hide_in_y)
			continue;
		for (i=0; i < NEXC; i++)
			if (ep->except[i] == time)
				break;
		if (i < NEXC)
			continue;
		if (lookup.index   <  mainlist->nentries &&
		    lookup.trigger >= time		 &&
		    lookup.trigger <  time + 86400)
			break;
	}
	if (found)
		set_color(COL_YNUMBER);
	else
		if (ifyes)
			set_year_day_bkg_color(day, month, year);
		else
			return;

	XFillRectangle(display, XtWindow(mainmenu.yearcal), gc, x, y, 3, 3);
	set_color(COL_STD);
}


static void set_year_day_bkg_color(
	int		day,		/* 1..31 */
	int		month,
	int		year)
{
	struct tm *tm = time_to_tm(get_time());
	set_color(day   == tm->tm_mday &&
		  month == tm->tm_mon  &&
		  year  == tm->tm_year ? COL_CALTODAY : COL_YBOXBACK);
}


/*-------------------------------------------------- low-level --------------*/
static BOOL day_to_yearpos(
	int		*x,		/* returned box, 0..6/0..5 */
	int		*y,
	int		day,		/* 1..31 */
	int		month,
	int		year)
{
	int		wkday;

	if (!date_to_time(day, month, year, &wkday, 0, 0))
		return(FALSE);
	*x = config.sunday_first ?  wkday : (wkday + 6) % 7;
	*y = (day-1 - *x + 6) / 7;
	return(TRUE);
}


/*
 * check where on the year calendar the user pressed, and return the month
 * and day. If the returned day is 0, the user clicked on the month name.
 * If the user clicked on one of the week call arrows to the left of each
 * week row, set *callweek and return the first day of that week.
 */

static BOOL yearpos_to_day_month(
	int		*day,		/* returned day#, 1..31 or 0*/
	int		*month,		/* returned month #, 0..11 */
	BOOL		*callweek,	/* if TRUE, pop up week view*/
	BOOL		*callday,	/* if TRUE, pop up day view */
	int		x,		/* pixel pos in drawable */
	int		y)
{
	struct config	*c = &config;
	int		dx, dy;		/* month box size, with gap */
	int		xx, yy;		/* month box no, max 6*7 */
	int		xtest, ytest;	/* temps */

	*callweek = FALSE;
	*callday  = FALSE;
	dx = c->year_gap + 7*c->yearbox_xs;
	x -= c->year_margin - c->year_gap;
	xtest = x / dx;
	x -= xtest * dx;

	dy = c->year_gap + 8*c->yearbox_ys;
	y -= c->year_margin + c->year_title + c->year_gap - c->yearbox_ys;
	ytest = y / dy;
	y -= ytest * dy;

	if (xtest < 0 || xtest > 3 || ytest < 0 || ytest > 2)
		return(FALSE);
	*month = 4*ytest + xtest;

	if (x < c->year_gap) {
		y = y / c->yearbox_ys -3;
		if (y < 0 || y > 5)
			return(FALSE);
		(void)day_to_yearpos(&xtest, &ytest, 1, *month, curr_year);
		*day = 7 * y - xtest +1;
		if (*day < 1) {
			if (--*month < 0)
				return(FALSE);
			*day += monthlen[*month] +
					(*month==1 && !(curr_year&3));
		}
		*callweek = TRUE;
		return(*day <= monthlen[*month]);
	}

	(void)day_to_yearpos(&xtest, &ytest, 1, *month, curr_year);
	if (x < 0 || y < 0)
		return(FALSE);
	x -= c->year_gap;
	xx = x  / c->yearbox_xs;
	yy = y  / c->yearbox_ys;
	x -= xx * c->yearbox_xs;
	y -= yy * c->yearbox_ys;
	if (yy < 0)
		return(FALSE);
	if (yy < 3) {
		*day = 0;
		return(TRUE);
	}
	*day = (yy-3) * 7 + xx - xtest + 1;
	if (*day <= 0 || !day_to_yearpos(&xtest,&ytest,*day,*month,curr_year))
		return(FALSE);
	*callday = x < y;
	return(TRUE);
}
