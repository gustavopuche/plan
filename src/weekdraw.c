/*
 * draws the week view. PostScript printing is also here because it uses
 * the same high-level routines, only the low-level output is diverted.
 *
 *	locate_in_week_calendar()	find out where we are in the calendar
 *	draw_week_day(day, month, year)	The day day/month/year has changed,
 *					redraw the week view if it contains
 *					that day.
 *	draw_week_calendar()		If there is a week menu, resize, and
 *					redraw
 *	print_week_calendar(fp,l)	print PostScript week to fp
 */

#include <stdio.h>
#include <time.h>
#include <Xm/Xm.h>
#include "cal.h"

#define DBLTIME		1000		/* doubleclick time [ms] */
#define SLOPE		1/4		/* slope of arrow head / barheight */
#define MINLEN		8		/* no bar is shorter than MINLEN */
#define TOD(t)		((t)%86400)
#define BOUND(t,a,b)	((t)<(a) ? (a) : (t)>(b) ? (b) : (t));
#define XPOS(t)		(((t) - c->week_minhour*3600) * c->week_hourwidth/3600)

static void gen_week			(void);
static void draw_week_day_background	(int);
static void draw_week_day_foreground	(int);
static void draw_bar			(struct weeknode *, int, int, int);
static int  drawn_lines;

extern Display		*display;	/* everybody uses the same server */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern struct config	config;		/* global configuration data */
extern struct mainmenu	mainmenu;	/* all important main window widgets */
extern struct user	*user;		/* user list (from file_r.c) */
extern time_t		curr_week;	/* week being displayed, time in sec */
extern struct week	week;		/* info on week view */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern struct holiday	holiday[366];	/* info for each day, separate for */
extern struct holiday	sm_holiday[366];/* full-line texts under, and small */


/*
 * for a given position on the canvas, identify the entry there and return
 * a pointer to it and which area it was hit in (one of the M_ constants).
 * Locate an event if epp!=0, and return exactly where the event was hit.
 * If none was found, return the time if possible for creating new events.
 * If epp is 0, we are dragging an event right now; just return a new time.
 *
 *   ret	*epp	*time	*trig	xp..ysp	event
 *
 *   M_ILLEGAL	0	0	-	-	no recognizable position
 *   M_OUTSIDE	0	time	-	-	no event but in a valid day
 *   M_INSIDE	entry	time	trigg	-	in event center, edit it
 *   M_LEFT	entry	time	trigg	pos/sz	near left edge, move start date
 *   M_RIGHT	entry	time	trigg	pos/sz	near right edge, move length
 */

MOUSE locate_in_week_calendar(
	struct entry	**epp,		/* returned entry or 0 */
	BOOL		*warn,		/* is *epp an n-days-ahead w?*/
	time_t		*time,		/* if *epp=0, clicked where */
	time_t		*trigger,	/* if *epp=0, entry trigger */
	int		*xp,		/* if M_INSIDE, rubber box */
	int		*yp,
	int		*xsp,		/* position and size */
	int		*ysp,
	int		xc,		/* pixel pos clicked */
	int		yc)
{
	struct config	*c = &config;
	struct weeknode	*vert, *horz;	/* yov node scan pointers */
	struct entry	*ep;		/* entry to test */
	int		d;		/* day strip counter */
	int		yd;		/* y start coord of day box */
	int		x, y;		/* upper left of day bk box */
	int		xend;		/* right margin of chart */
	int		ys;		/* height of bar line w/ gaps*/
	time_t		bt;		/* start time of bar */
	int		b, e;		/* X start end end pos of bar*/

	x  = c->week_margin + c->week_daywidth + c->week_gap;
	yd = 2*c->week_margin + c->week_title + c->week_hour + c->week_gap + 2;
	ys = c->week_barheight + 2*c->week_bargap;
	xend = x + c->week_hourwidth * (c->week_maxhour - c->week_minhour);
	if (xc > xend || yc < c->week_margin + c->week_title)
		return(M_ILLEGAL);
	if (warn)
		*warn = FALSE;
						/* calc time in calendar */
	*time = 0;
	for (y=yd, d=0; d < c->week_ndays; d++) {
		if (yc < y)
			break;
		y += week.nlines[d] * ys + c->week_gap;
		if (yc < y) {
			*time = curr_week + d * 86400 + c->week_minhour * 3600+
					(xc - x) * 3600 / c->week_hourwidth;
			break;
		}
	}
						/* find entry under mouse */
	if (epp)
	    for (d=0; d < c->week_ndays; d++) {
		for (y=yd, vert=week.tree[d]; vert; vert=vert->down, y+=ys) {
		    if (yc < y || yc >= y+ys)
			continue;
		    if (xc < x)
			return(M_OUTSIDE);
		    for (horz=vert; horz; horz=horz->next) {
			ep = *epp = horz->entry;
			bt = TOD(ep->time);
			e = x + XPOS(TOD(bt));
			if (config.weekwarn)
				bt -= ep->early_warn > ep->late_warn ?
					ep->early_warn : ep->late_warn;
			if (bt < c->week_minhour*3600)
				bt = c->week_minhour*3600;
			b = x + XPOS(TOD(bt));
			if (!ep->notime)
				e += ep->length * c->week_hourwidth/3600;
			if (e < b + MINLEN)
				e = b + MINLEN;
			if (trigger)
				*trigger = horz->trigger;
			if (warn)
				*warn = horz->days_warn;
			if (xp) {
				*xp   = b + 1;
				*yp   = y + c->week_gap/2 + 1;
				*xsp  = e - b - 1;
				*ysp  = c->week_barheight & ~1;
			}
			if (xc < b-1)
				continue;
			if (xc < b + (e-b)/3)
				return(M_LEFT);
			if (xc < b + (e-b)*2/3)
				return(M_INSIDE);
			if (xc < e + 1)
				return(M_RIGHT);
			if (!horz->textinside && xc < e + horz->textlen)
				return(M_INSIDE);
		    }
		}
		yd += week.nlines[d] * ys + c->week_gap;
	    }
	return(*time ? M_OUTSIDE : M_ILLEGAL);
}


/*
 * If the specified date is in the current week, redraw everything (something
 * changed on the specified day).
 */

void draw_week_day(
	int		day,
	int		month,
	int		year)
{
	struct tm	tm;
	time_t		date;

	tm.tm_year = year;
	tm.tm_mon  = month;
	tm.tm_mday = day;
	tm.tm_hour = 0;
	tm.tm_min  = 0;
	tm.tm_sec  = 0;
	date = tm_to_time(&tm);
	if (date >= curr_week && date < curr_week*config.week_ndays*86400) {
		build_week(FALSE, FALSE);
		if (week.canvas)
			create_view('w');
	}
}


/*---------------------------------- drawing front-ends ---------------------*/
/*
 * draw one week into the current window, in the specified region (all
 * if null).
 */

void draw_week_calendar(
	Region	region)
{
	if (!g_init_window(&week, region))
		return;
	g_setcolor(COL_WBACK);
	g_fillrect(0, 0, week.canvas_xs, week.canvas_ys);
	gen_week();
	g_exit_window();
}


/*
 * print PostScript week calendar to file fp.
 */

void print_week_calendar(
	FILE	*fp,
	BOOL	landscape)
{
	time_t time = get_time();
	struct tm *tm = time_to_tm(time);
	time -= (tm->tm_wday + 6 + config.sunday_first)%7 * 86400;
	if (!curr_week)
		curr_week = time;
	build_week(config.pr_omit_priv, config.pr_omit_appts);
	g_init_ps(&week, fp, landscape);
	gen_week();
	g_exit_ps();
}


/*---------------------------------- high-level drawing ---------------------*/
/*
 * draw the week, using low-level routines
 */

static void gen_week(void)
{
	struct config	*c = &config;
	char		buf[40], *p;
	int		i, l, x, y;

	drawn_lines = 0;
	for (i=0; i < c->week_ndays; i++)		/* day background */
		draw_week_day_background(i);
							/* thick lines */
	g_setcolor(COL_WGRID);
	g_fillrect(c->week_margin,
		   2*c->week_margin + c->week_title + c->week_hour + 1,
		   week.canvas_xs - 2*c->week_margin +1,
		   2);
	g_fillrect(c->week_margin,
		   week.canvas_ys - c->week_margin +2,
		   week.canvas_xs - 2*c->week_margin +1,
		   2);
							/* thin lines */
	x = c->week_margin + c->week_daywidth + c->week_gap;
	y = 2*c->week_margin + c->week_title + c->week_hour + 3;
	for (i=0; i <= c->week_maxhour-c->week_minhour; i++)
		g_fillrect(x + i * c->week_hourwidth, y, 1,
				week.canvas_ys - c->week_margin - y + 3);

	for (i=0; i < c->week_ndays; i++)		/* day foreground */
		draw_week_day_foreground(i);
							/* hour labels */
	g_setcolor(COL_WDAY);
	g_setfont(FONT_WHOUR);
	for (i=0; i <= c->week_maxhour-c->week_minhour; i++) {
		p = mktimestring((i + c->week_minhour) * 3600, FALSE);
		l = strlen_in_pixels(p, FONT_WHOUR);
		g_drawtext(x + i * c->week_hourwidth - l/2,
			   2*c->week_margin + c->week_title + c->week_hour -2,
			   p, strlen(p));
	}
	strcpy(buf, mkdatestring(curr_week));		/* centered title */
	strcat(buf, " - ");
	strcat(buf, mkdatestring(curr_week + (c->week_ndays-1)*86400));
	g_setcolor(COL_WTITLE);
	g_setfont(FONT_WTITLE);
	g_drawtext((week.canvas_xs - strlen_in_pixels(buf,FONT_WTITLE))/2, 
		   c->week_margin + c->week_title * 3/4,
		   buf, strlen(buf));
	g_setcolor(COL_STD);
}


/*
 * draw the background of one day of the week at its position. This includes
 * the day number and the box, but not the bars themselves. The bars are
 * printed after the thin hour lines are drawn by the caller, by calling the
 * following routine.
 */
#ifdef JAPAN
void g_draw_strpack();
#endif

static void draw_week_day_background(
	int		wday)		/* day of the week, < NDAYS */
{
	struct config	*c = &config;
	time_t		today;		/* today's date */
	struct holiday	*hp, *shp;	/* to check for holidays */
	char		*errmsg;	/* holiday parser error */
	char		buf[20];	/* holiday text buffer */
	struct tm	*tm;		/* today's date as m/d/y */
	char		*p;		/* temp for day name */
	int		x, y;		/* upper left of day bk box */
	int		ys;		/* height of bar line w/ gaps*/
	int		yt;		/* text y position */
	int		yts;		/* vert space for text */
	int		i;
#ifdef JAPAN
	int		plen;		/* pixel-length of partialstr */
	strpack		partialstr[MAXPARTIALSTRING];
	unsigned char	strpool[MAXPARTIALCHAR];
#endif

	if (week.nlines[wday] == 0)
		return;
	x  = c->week_margin + c->week_daywidth + c->week_gap;
	y  = 2*c->week_margin + c->week_title + c->week_hour + c->week_gap + 2;
	ys = c->week_barheight + 2*c->week_bargap;
	for (i=0; i < wday; i++)
		y += week.nlines[i] * ys + c->week_gap;

							/* clear weekday name*/
	yts = week.nlines[wday] * ys + c->week_gap -1;
	today = get_time() - (curr_week + wday*86400);
	g_setcolor(today < 0 || today >= 86400 ? COL_WBACK : COL_CALTODAY);
	g_fillrect(c->week_margin, y, c->week_daywidth, yts - c->week_gap);

							/* weekday name */
	yt = y + font[FONT_WDAY]->max_bounds.ascent;
	i = (wday + 7 - c->sunday_first) % 7;
	p = mkdatestring(curr_week + wday * 86400);
	truncate_string(p, c->week_daywidth-2, FONT_WDAY);
	g_setcolor(COL_WDAY);
	g_setfont(FONT_WDAY);
	g_drawtext(c->week_margin, yt, p, strlen(p));
	yt += font[FONT_WDAY]->max_bounds.ascent +
	      font[FONT_WDAY]->max_bounds.descent;
							/* holidays */
	tm = time_to_tm(curr_week + wday*86400);
	if ((errmsg = parse_holidays(tm->tm_year, FALSE)))
		create_error_popup(mainmenu.cal, 0, errmsg);
	shp = &sm_holiday[tm->tm_yday];
	hp  =    &holiday[tm->tm_yday];
	for (i=0; i < 2; i++, shp=hp)
		if (shp->string && yt < y + yts) {
			g_setcolor(shp->stringcolor ? shp->stringcolor :
				   shp->daycolor    ? shp->daycolor
						    : COL_WDAY);
#ifdef JAPAN
			partialstr->strptr = strpool;
			if ((plen = mixedstrlen_in_pixels(shp->string,
					partialstr, FONT_WNOTE, FONT_JNOTE))
				> c->week_daywidth-2)
				plen = truncate_strpack(partialstr,
							c->week_daywidth-2,
							FONT_WNOTE);

			g_draw_strpack(c->week_margin +
				c->week_daywidth-2 - plen, yt, partialstr);
			yt += font[FONT_WDAY]->max_bounds.ascent +
			      font[FONT_WDAY]->max_bounds.descent >
			      font[FONT_JNOTE]->max_bounds.ascent +
			      font[FONT_JNOTE]->max_bounds.descent ?
				font[FONT_WDAY]->max_bounds.ascent +
				font[FONT_WDAY]->max_bounds.descent :
				font[FONT_JNOTE]->max_bounds.ascent +
				font[FONT_JNOTE]->max_bounds.descent;
#else
			strncpy(buf, shp->string, sizeof(buf)-1);
			truncate_string(buf, c->week_daywidth-2, FONT_WDAY);
			g_drawtext(c->week_margin + c->week_daywidth-2 -
					strlen_in_pixels(buf, FONT_WDAY),
				   yt, buf, strlen(buf));
			yt += font[FONT_WDAY]->max_bounds.ascent +
			      font[FONT_WDAY]->max_bounds.descent;
#endif
		}
							/* appointment box */
	g_setcolor(COL_WBOXBACK);
	g_fillrect(x, y, week.canvas_xs - c->week_margin - x,
			 week.nlines[wday] * ys);
							/* repeated times */
	if (wday && c->week_gap > font[FONT_WHOUR]->max_bounds.ascent-2
		 && drawn_lines > 10 && config.moretimecols) {
		drawn_lines -= 10;
		g_setcolor(COL_WDAY);
		g_setfont(FONT_WHOUR);
		for (i=0; i < c->week_maxhour-c->week_minhour; i++) {
			char buf[10];
			sprintf(buf, "%d", i + c->week_minhour);
			g_drawtext(x + i * c->week_hourwidth + 2, y - 1,
						buf, strlen(buf));
		}
	}
	drawn_lines += week.nlines[wday];
}


/*
 * draw the foreground of one day of the week at its position. This includes
 * all the bars. The background and the thin vertical lines have already been
 * drawn by the caller.
 */

static void draw_week_day_foreground(
	int		wday)		/* day of the week, < NDAYS */
{
	struct config	*c = &config;
	struct weeknode	*vert, *horz;	/* week node scan pointers */
	int		x, y;		/* upper left of day bk box */
	int		ys;		/* height of bar line w/ gaps*/
	int		i, u;

	x  = c->week_margin + c->week_daywidth + c->week_gap;
	y  = 2*c->week_margin + c->week_title + c->week_hour + c->week_gap + 2;
	ys = c->week_barheight + 2*c->week_bargap;
	for (i=0; i < wday; i++)
		y += week.nlines[i] * ys + c->week_gap;

	for (vert=week.tree[wday]; vert; vert=vert->down, y+=ys)
		for (horz=vert; horz; horz=horz->next) {
			u = name_to_user(horz->entry->user);
			draw_bar(horz, u > 0 ? user[u].color : 0,
				 x, y + c->week_bargap);
		}
}


/*
 * draw one entry bar.
 */

static void draw_bar(
	struct weeknode	*node,		/* entry node to print */
	int		color,		/* color, 0..7 */
	int		x,		/* top left pos in canvas */
	int		y)
{
	struct entry	*ep = node->entry;
	struct config	*c = &config;
	int		xend;		/* right margin of chart */
	int		w[2], b, e;	/* early/late, begin, end */
	int		ee;		/* e with min length applied */
	int		i;		/* warning counter, 0..1 */
	int		slope;		/* delta-x of the arrowheads */
	XPoint		point[7];	/* polygon vertex list */
#ifdef JAPAN
	int		plen;		/* pixel-length of partialstr */
	strpack		partialstr[MAXPARTIALSTRING];
	unsigned char	strpool[MAXPARTIALCHAR];
#endif

	slope = c->week_barheight * SLOPE;
	xend = x + c->week_hourwidth * (c->week_maxhour - c->week_minhour);
	i = ep->early_warn > ep->late_warn;
	w[!i] = TOD(ep->time) - ep->early_warn;
	w[ i] = TOD(ep->time) - ep->late_warn;
	b     = TOD(ep->time);
	e     = TOD(ep->time) + (ep->notime ? 1200 : ep->length);
	w[0]  = x + XPOS(w[0]);
	w[1]  = x + XPOS(w[1]);
	b     = x + XPOS(b);
	e     = x + XPOS(e);
	ee    = e < b+MINLEN ? b+MINLEN : e;
	if (ep->notime && config.week_bignotime) {
		b = 0;
		e = ee = 9999;
	}
	if (config.weekwarn && !ep->notime && !node->days_warn)
	    for (i=0; i < 2; i++)
		if (w[i] < b && w[i] <= w[1] && b+slope > 0) {
			point[5].x =
			point[0].x =
			point[2].x = BOUND(w[i]+slope, x, xend);
			point[1].x = BOUND(w[i], x, xend);
			point[3].x =
			point[4].x = BOUND(b+slope, point[0].x, xend);
			point[5].y =
			point[0].y =
			point[4].y = y;
			point[1].y = y + c->week_barheight/2+1;
			point[2].y =
			point[3].y = y + (c->week_barheight&~1)+1;
			g_setcolor(COL_WWARN);
			g_drawpoly(point, 5, FALSE);
			g_setcolor(COL_WFRAME);
			g_drawlines(point, 6);
		}

	point[6].x =
	point[0].x =
	point[2].x = BOUND(b+slope, x, xend);
	point[1].x = BOUND(b, x, xend-MINLEN);
	point[3].x = BOUND(ee-slope, point[0].x, xend);
	point[3].x =
	point[5].x = BOUND(point[3].x, point[1].x+MINLEN, xend);
	point[4].x = BOUND(e, point[3].x, xend);
	point[6].y =
	point[0].y =
	point[5].y = y;
	point[1].y =
	point[4].y = y + c->week_barheight/2+1;
	point[2].y =
	point[3].y = y + (c->week_barheight&~1)+1;
	if (ep->notime && !config.week_bignotime)
		point[3].x = point[5].x = point[0].x;
	g_setcolor(ep->suspended || node->days_warn ?
					COL_WWARN : COL_WUSER_0 + (color & 7));
	g_drawpoly(point, 6, color > 7);
	g_setcolor(COL_WFRAME);
	g_drawlines(point, 7);

	if (*node->text) {
		char buf[100];
		strcpy(buf, node->text);
#ifdef JAPAN
		partialstr->strptr = strpool;
		plen = mixedstrlen_in_pixels(node->text, partialstr,
					     FONT_WNOTE, FONT_JNOTE);
  		if (node->textinside) {
			if (plen > xend - (x = point[0].x))
				plen = truncate_strpack(partialstr, xend - x,
							FONT_WNOTE);
			x += (point[3].x-point[0].x - plen) / 2;
			if (x + plen > xend-1)
				x = xend-1 - plen;
		} else
			if (plen > xend - (x = point[4].x + 3))
				(void)truncate_strpack(partialstr, xend - x,
						       FONT_WNOTE);
#else
		if (node->textinside) {
			int l;
			x = point[0].x;
			truncate_string(buf, xend - x, FONT_WNOTE);
			l = strlen_in_pixels(buf, FONT_WNOTE);
			x += (point[3].x-point[0].x - l) / 2;
			if (x + l > xend-1)
				x = xend-1 - l;
		} else {
			x = point[4].x + 3;
			truncate_string(buf, xend - x, FONT_WNOTE);
		}
#endif
		g_setcolor(ep->acolor ? COL_HBLACK + ep->acolor-1 : COL_WNOTE);
		g_setfont(FONT_WNOTE);
#ifdef JAPAN
		g_setfont(FONT_JNOTE);
		g_draw_strpack(x, y + c->week_barheight/2
				    + font[FONT_WDAY]->max_bounds.ascent/2,
			       partialstr);
#else
		g_drawtext(x, y + c->week_barheight/2
			     		+ font[FONT_WDAY]->max_bounds.ascent/2,
			   buf, strlen(buf));
#endif
	}
}

#ifdef JAPAN
int truncate_strpack(
	strpack		*partialstr,
	int		maxpix,		/* Maximum width in pixels. */
	int		ascfont)	/* ascii font to draw */
{
	int		plen = 0;
	strpack		*upperpartstr;

	for (upperpartstr = partialstr + MAXPARTIALSTRING;
	     partialstr < upperpartstr && partialstr->strptr != NULL;
	     partialstr++) {
		if (plen + partialstr->pixlen > maxpix) {	/* Truncating*/
			if (partialstr->asciistr == False) {	/* NON ascii */
				partialstr->length *= (double)(maxpix - plen) /
						    (double)partialstr->pixlen;
				if (partialstr->length &= ~1)
					if (++partialstr < upperpartstr)
						partialstr->strptr = NULL;
				else
					partialstr->strptr = NULL;
			} else {				/* ascii */
				(void)truncate_string(partialstr->strptr,
						      maxpix-plen, ascfont);
				if (partialstr->length =
						strlen(partialstr->strptr))
					if (++partialstr < upperpartstr)
						partialstr->strptr = NULL;
				else
					partialstr->strptr = NULL;
			}
			plen = maxpix;
			break;
		} else
			plen += partialstr->pixlen;
	}
	return plen;
}

void g_draw_strpack(
	int			x,
	int			y,
	strpack			*partialstr)
{
	strpack			*upperpartstr;

	for (upperpartstr = partialstr + MAXPARTIALSTRING;
	     partialstr < upperpartstr && partialstr->strptr != NULL;
	     partialstr++) {
		if (partialstr->asciistr == False)
			g_drawtext16(x, y, partialstr->strptr,
				      partialstr->length/2);
		else
			g_drawtext(x, y, partialstr->strptr,
					 partialstr->length);
		x += partialstr->pixlen;
	}
}
#endif
