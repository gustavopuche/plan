/*
 * draws the year overview. PostScript printing is also here because it uses
 * the same high-level routines, only the low-level output is diverted. At
 * first, this job seems very similar to week view drawing, but in fact they
 * have little in common: bars have different shapes, are drawn in different
 * locations and are chosen by different criteria.
 *
 *	locate_in_yov_calendar()	find out where we are in the calendar
 *	draw_yov_calendar(region)	If there is a yov menu, resize, and
 *					redraw
 *	print_yov_calendar(fp)		print PostScript yov to fp
 */

#include <stdio.h>
#include <time.h>
#include <Xm/Xm.h>
#include "cal.h"

#define DBLTIME		1000		/* doubleclick time [ms] */
#define SLOPE		1/4		/* slope of arrow head / barheight */
#define TOD(t)		((t)%86400)
#define XPOS(t)		(((t) - curr_yov) / 86400 * daysz)
#define BOUND(t,a,b)	((t)<(a) ? (a) : (t)>(b) ? (b) : (t));

static void gen_yov			(void);
static void draw_yov_strip_background	(int);
static void draw_yov_strip_foreground	(int);
static void draw_bar			(struct weeknode *, int, int, int);

extern Display		*display;	/* everybody uses the same server */
extern GC		jgc, gc;	/* graphic context, Japanese and std */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern struct config	config;		/* global configuration data */
extern struct mainmenu	mainmenu;	/* all important main window widgets */
extern struct user	*user;		/* user list (from file_r.c) */
extern time_t		curr_yov;	/* yov being displayed, time in sec */
extern struct week	yov;		/* info on yov view */
extern int		yov_nstrips;	/* number of strips (max nusers) */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern struct holiday	holiday[366];	/* info for each day, separate for */
extern struct holiday	sm_holiday[366];/* full-line texts under, and small */
extern short		monthlen[];	/* length of each month */
extern char		*monthname[];	/* name of each month */
extern char		*yovday_name[];
static int		ndays;		/* number of days in year <curr_yov> */



/*
 * for a given position on the canvas, identify the entry there and return
 * a pointer to it and which area it was hit in (one of the M_ constants).
 * Locate an event if epp!=0, and return exactly where the event was hit.
 * If none was found, return the time if possible for creating new events.
 * If epp is 0, we are dragging an event right now; just return a new time.
 * Note that the time is purely a position in the calendar that can be
 * different from the entry trigger time if the event is a multiday entry.
 *
 *   ret	*epp	*time	*trig	xp..ysp	event
 *
 *   M_ILLEGAL	0	0	-	-	no recognizable position
 *   M_OUTSIDE	0	time	-	-	no event but in a valid day
 *   M_INSIDE	entry	time	trigg	-	in event center, edit it
 *   M_LEFT	entry	time	trigg	pos/sz	near left edge, move start date
 *   M_RIGHT	entry	time	trigg	pos/sz	near right edge, move end date
 */

MOUSE locate_in_yov_calendar(
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
	int		daysz;		/* horz pixels per day */
	int		s;		/* strip counter */
	int		yd;		/* y start coord of day box */
	int		x, y;		/* upper left of day bk box */
	int		xend;		/* right margin of chart */
	int		ys;		/* height of bar line w/ gaps*/
	int		b, e;		/* X start end end pos of bar*/

	x     = c->week_margin + c->week_daywidth + c->week_gap;
	yd    = c->week_margin + 2*c->week_hour + c->week_gap + 2;
	ys    = c->week_barheight + 2*c->week_bargap;
	daysz = c->yov_daywidth / c->yov_nmonths;
	xend  = x + ndays * daysz;
	*time = curr_yov + (xc-x) / daysz * 86400;
	if (xc < x || xc > xend || yc < c->week_margin + c->week_title)
		return(M_ILLEGAL);
	if (!epp)
		return(M_OUTSIDE);
	if (warn)
		*warn = FALSE;
	*epp  = 0;

	for (s=0; s < yov_nstrips; s++) {
	    for (y=yd, vert=yov.tree[s]; vert; vert=vert->down, y+=ys) {
		if (yc < y || yc >= y+ys)
			continue;
		for (horz=vert; horz; horz=horz->next) {
			ep = *epp = horz->entry;
			b  = x + XPOS(horz->trigger);
			e  = horz->days_warn ||
			     ep->rep_every!=86400? b + daysz :
			     ep->rep_last	 ? x + XPOS(ep->rep_last+86400)
						 : xend;
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
			if (xc < e+1)
				return(M_RIGHT);
			if (!horz->textinside && xc < e + horz->textlen)
				return(M_INSIDE);
		}
	    }
	    yd += yov.nlines[s] * ys + c->week_gap;
	}
	*epp = 0;
	return(M_OUTSIDE);
}


/*---------------------------------- drawing front-ends ---------------------*/
/*
 * draw one year overview into the current window, in the specified region
 * (all if null).
 */

void draw_yov_calendar(
	Region	region)
{
	if (!g_init_window(&yov, region))
		return;
	g_setcolor(COL_WBACK);
	g_fillrect(0, 0, yov.canvas_xs, yov.canvas_ys);
	gen_yov();
	g_exit_window();
}


/*
 * print PostScript yov calendar to file fp.
 */

void print_yov_calendar(
	FILE	*fp)
{
	time_t time = get_time();
	struct tm *tm = time_to_tm(time);
	time -= (tm->tm_wday + 6 + config.sunday_first)%7 * 86400;
	if (!curr_yov)
		curr_yov = time;
	if (config.pr_omit_appts)
		destroy_yov();
	else
		build_yov(config.pr_omit_priv);
	g_init_ps(&yov, fp, TRUE);
	gen_yov();
	g_exit_ps();
}


/*---------------------------------- high-level drawing ---------------------*/
/*
 * draw the year overview, using low-level routines
 */

static void gen_yov(void)
{
	struct config	*c = &config;
	struct tm	*tm;
	char		buf[40], *p;
	int		year;		/* current year, 1970..2038 */
	int		daysz;		/* horz pixels per day */
	int		wday0;		/* weekday (0=mon) of 1st day*/
	int		i, m, d, x, y;
	int		charwidth;	/* width of a day num digit */

	tm    = time_to_tm(curr_yov);
	year  = tm->tm_year;
	wday0 = tm->tm_wday;
	ndays = year & 3 ? 365 : 366;
	daysz = c->yov_daywidth / c->yov_nmonths;
	if (daysz < 1) daysz = 1;
	if ((p = parse_holidays(year, FALSE)))
		create_error_popup(mainmenu.cal, 0, p);

	for (i=0; i < yov_nstrips; i++)			/* strip backgrounds */
		draw_yov_strip_background(i);

	tm = time_to_tm(get_time());			/* make today green */
	if (tm->tm_year == year) {
		y = 2*c->week_hour + 3,
		x = c->week_margin + c->week_daywidth + c->week_gap +
							tm->tm_yday * daysz,
		g_setcolor(COL_CALTODAY);
		g_fillrect(x, y, daysz, yov.canvas_ys - c->week_margin - y +2);
	}
							/* thick lines */
	g_setcolor(COL_WGRID);
	g_fillrect(c->week_margin,
		   2*c->week_hour + 1,
		   yov.canvas_xs - 2*c->week_margin +1,
		   2);
	g_fillrect(c->week_margin,
		   yov.canvas_ys - c->week_margin +2,
		   yov.canvas_xs - 2*c->week_margin +1,
		   2);
							/* month names */
	x = c->week_margin + c->week_daywidth + c->week_gap;
	y = c->week_hour;
	set_color(COL_WEEKDAY);
	g_setfont(FONT_WHOUR);
	for (i=0; i < 12; i++) {
		char *mn = _(monthname[i]);
		g_drawtext(x, y, mn, strlen(mn));
		x += (monthlen[i] + (i == 1 && !(year & 3))) * daysz;
	}
							/* day numbers/lines */
	x = c->week_margin + c->week_daywidth + c->week_gap;
	y = 2*c->week_hour;
	charwidth = CHARWIDTH(FONT_WHOUR, '0');

	for (d=1,m=i=0; i < ndays; i++) {
		BOOL weekbegin = (i+wday0) % 7 == 1;
		if (daysz >= 2 * charwidth || weekbegin) {
			sprintf(buf, "%d", d);
			g_setcolor(
			    holiday[i].daycolor	   ? holiday[i].daycolor    :
			    sm_holiday[i].daycolor ? sm_holiday[i].daycolor :
			    (i+wday0+6) % 7 > 4	   ? COL_WEEKEND
						   : COL_WEEKDAY);
			g_drawtext(x, y, buf, strlen(buf));
		}
		if (weekbegin && daysz > 4) {
			g_setcolor(COL_WGRID);
			g_fillrect(x, y+3, 2, yov.canvas_ys-c->week_margin-y);
		} else if (weekbegin || i == 0 || daysz > 4) {
			g_setcolor(COL_WGRID);
			g_fillrect(x, y+3, 1, yov.canvas_ys-c->week_margin-y);
		}
		x += daysz;
		if (++d > (monthlen[m] + (m == 1 && !(year & 3)))) {
			d -= monthlen[m] + (m == 1 && !(year & 3));
			m++;
		}
	}
	g_fillrect(x, y+3, 1, yov.canvas_ys - c->week_margin - y);
	for (i=0; i < yov_nstrips; i++)			/* strip foreground */
		draw_yov_strip_foreground(i);

	sprintf(buf, "%d", 1900 + year);		/* centered title */
	g_setcolor(COL_WTITLE);
	g_setfont(FONT_WTITLE);
	g_drawtext(c->week_margin, c->week_margin, buf, strlen(buf));
	g_setcolor(COL_STD);
}


/*
 * draw the background of one day of the yov at its position. This includes
 * the day number and the box, but not the bars themselves. The bars are
 * printed after the thin hour lines are drawn by the caller, by calling the
 * following routine.
 */

static void draw_yov_strip_background(
	int		strip)		/* day of the yov, < NDAYS */
{
	struct config	*c = &config;
	char		buf[64];	/* holiday/user text buffer */
	char		*p;		/* temp for day name */
	int		x, y;		/* upper left of day bk box */
	int		ys;		/* height of bar line w/ gaps*/
	int		yts;		/* vert space for text */
	int		i;

	if (yov.nlines[strip] == 0)
		return;
	x  = c->week_margin + c->week_daywidth + c->week_gap;
	y  = c->week_margin + 2*c->week_hour + c->week_gap+2;
	ys = c->week_barheight + 2*c->week_bargap;
	for (i=0; i < strip; i++)
		y += yov.nlines[i] * ys + c->week_gap;

							/* clear user name*/
	yts = yov.nlines[strip] * ys + c->week_gap -1;
	g_setcolor(COL_WBACK);
	g_fillrect(c->week_margin, y, c->yov_daywidth, yts);

							/* user name */
	i = (strip + 7 - c->sunday_first) % 7;
	if ((p = yov.tree[strip]->entry->user)) {
		strncpy(buf, p, sizeof(buf));
		buf[sizeof(buf)-1] = 0;
	} else
		strcpy(buf, _("Own"));
	truncate_string(buf, c->week_daywidth-2, FONT_WDAY);
	g_setcolor(COL_WDAY);
	g_setfont(FONT_WDAY);
	g_drawtext(c->week_margin, y + font[FONT_WDAY]->max_bounds.ascent,
			buf, strlen(buf));
							/* strip background */
	g_setcolor(COL_WBOXBACK);
	g_fillrect(x, y, yov.canvas_xs - c->week_margin - x,
			 yov.nlines[strip] * ys);
}


/*
 * draw the foreground of one day of the yov at its position. This includes
 * all the bars. The background and the thin vertical lines have already been
 * drawn by the caller.
 */

static void draw_yov_strip_foreground(
	int		strip)		/* user strip number, 0=top */
{
	struct config	*c = &config;
	struct weeknode	*vert, *horz;	/* yov node scan pointers */
	int		x, y;		/* upper left of day bk box */
	int		ys;		/* height of bar line w/ gaps*/
	int		i, u;

	x  = c->week_margin + c->week_daywidth + c->week_gap;
	y  = c->week_margin + 2*c->week_hour + c->week_gap + 2;
	ys = c->week_barheight + 2*c->week_bargap;
	for (i=0; i < strip; i++)
		y += yov.nlines[i] * ys + c->week_gap;

	for (vert=yov.tree[strip]; vert; vert=vert->down, y+=ys)
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
	int		b, e;		/* begin/end pixel position */
	int		daysz;		/* horz pixels per day */
	int		slope;		/* delta-x of the arrowheads */
	XPoint		point[7];	/* polygon vertex list */
#ifdef JAPAN
	int		plen;		/* pixel-length of partialstr */
	strpack		partialstr[MAXPARTIALSTRING];
	unsigned char	strpool[MAXPARTIALCHAR];
#endif

	daysz = c->yov_daywidth / c->yov_nmonths;
	slope = daysz < c->week_barheight ? 0 : c->week_barheight * SLOPE;
	xend  = x + ndays * daysz;
	b     = x + XPOS(node->trigger);
	e     = node->days_warn ||
		ep->rep_every != 86400	? b + daysz :
		ep->rep_last		? x + XPOS(ep->rep_last + 86400)
					: xend;
	point[6].x =
	point[0].x =
	point[2].x = BOUND(b+slope, x, xend);
	point[1].x = BOUND(b, x, xend);
	point[3].x = BOUND(e-slope, point[0].x, xend);
	point[3].x =
	point[5].x = BOUND(point[3].x, point[1].x, xend);
	point[4].x = BOUND(e, point[3].x, xend);
	point[6].y =
	point[0].y =
	point[5].y = y;
	point[1].y =
	point[4].y = y + c->week_barheight/2+1;
	point[2].y =
	point[3].y = y + (c->week_barheight&~1)+1;

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
