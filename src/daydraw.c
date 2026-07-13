/*
 * draws the day view. PostScript printing is also here because it uses
 * the same high-level routines, only the low-level output is diverted.
 *
 *	locate_in_day_calendar()	find out where we are in the calendar
 *	draw_day_calendar(region)	If there is a day menu, resize, and
 *					redraw
 *	print_day_calendar(fp,l)	print PostScript day to fp
 */

#include <stdio.h>
#include <time.h>
#include <Xm/Xm.h>
#include "cal.h"

#define DBLTIME		1000		/* doubleclick time [ms] */

#define CUTOFF	(config.day_gap-1)	/* corner cut-off distance */
#define MINLEN		14		/* no bar is shorter than MINLEN */

#define TOD(t)		((t)%86400)
#define BOUND(t,a,b)	((t)<(a) ? (a) : (t)>(b) ? (b) : (t));
#define YPOS(t)		(((t) - c->day_minhour*3600) * c->day_hourheight/3600)

static void gen_day	(void);
static void draw_bar	(struct weeknode *, int, int, int);
static void draw_octagon(int, BOOL, int, int, int, int);
static void draw_boxtext(int, int, int, int, char *, BOOL);

#ifdef JAPAN
extern int truncate_strpack();
extern int mixedstrlen_in_pixels();
extern void g_draw_strpack();
#endif

extern Display		*display;	/* everybody uses the same server */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern struct config	config;		/* global configuration data */
extern struct mainmenu	mainmenu;	/* all important main window widgets */
extern struct user	*user;		/* user list (from file_r.c) */
extern time_t		curr_day;	/* day being displayed, time in sec */
extern struct week	day;		/* info on day view */
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
 *   M_LEFT	entry	time	trigg	pos/sz	near top edge, move start date
 *   M_RIGHT	entry	time	trigg	pos/sz	near bottom edge, move length
 */

MOUSE locate_in_day_calendar(
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
	int		x0, y0;		/* XY start coord of day box*/
	int		x, y;		/* upper left of day bk box */
	int		xs;		/* width of bar col w/ gaps */
	time_t		bt;		/* start time of bar */
	int		b, e;		/* Y start end end pos of bar*/
	int		yend;		/* bottom margin of chart */

	x0 = c->day_margin + c->day_hourwidth + 2;
	y0 = c->day_margin + c->day_title + 2 * c->day_headline + 2;
	xs = c->day_barwidth + c->day_gap;
	yend = y0 + c->day_hourheight * (c->day_maxhour - c->day_minhour);

	if (yc > yend || yc < y0 || xc < x0)
		return(M_ILLEGAL);
	if (warn)
		*warn = FALSE;
						/* calc time in calendar */
	*time = 0;
	for (x=x0, d=0; d < c->day_ndays; d++) {
		if (xc < x)
			break;
		x += day.nlines[d] * xs + c->day_margin;
		if (d && config.moretimecols)
			x += c->day_hourwidth + 2;
		if (xc < x) {
			*time = curr_day + d * 86400 + c->day_minhour * 3600 +
					(yc - y0) * 3600 / c->day_hourheight;
			break;
		}
	}
						/* find entry under mouse */
	if (epp)
	    for (d=0; d < c->day_ndays; d++) {
		for (x=x0,y=y0,vert=day.tree[d]; vert; vert=vert->down,x+=xs) {
		    if (xc < x || xc >= x+xs)
			continue;
		    if (yc < y)
			return(M_OUTSIDE);
		    for (horz=vert; horz; horz=horz->next) {
			ep = *epp = horz->entry;
			bt = BOUND(TOD(ep->time), c->day_minhour*3600,
						     c->day_maxhour*3600);
			e  = BOUND(y + YPOS(bt), y0, yend);
			if (config.weekwarn)
				bt -= ep->early_warn > ep->late_warn ?
					ep->early_warn : ep->late_warn;
			b = BOUND(y + YPOS(bt), y0, yend);
			if (ep->notime)
				e = config.week_bignotime
					? y + YPOS(c->day_maxhour*3600)
					: b + YPOS((c->day_minhour+1)*3600);
			else {
				int l = ep->length * c->day_hourheight/3600;
				e += BOUND(l, MINLEN, 9999);
			}
			if (trigger)
				*trigger = horz->trigger;
			if (warn)
				*warn = horz->days_warn;
			if (xp) {
				*xp   = x + 1;
				*yp   = b + 1;
				*xsp  = c->day_barwidth & ~1;
				*ysp  = e - b - 1;
			}
			if (yc < b - 1)
				continue;
			if (yc < b + (e-b)/3)
				return(M_LEFT);
			if (yc < b + (e-b)*2/3)
				return(M_INSIDE);
			if (yc < e + 1)
				return(M_RIGHT);
		    }
		}
		x0 += day.nlines[d] * xs + c->day_margin;
		if (config.moretimecols)
			x0 += c->day_hourwidth + 2;
	    }
	if (epp)
		*epp = 0;
	return(*time ? M_OUTSIDE : M_ILLEGAL);
}


/*---------------------------------- drawing front-ends ---------------------*/
/*
 * draw one day into the current window, in the specified region (all if null).
 */

void draw_day_calendar(
	Region	region)
{
	if (!g_init_window(&day, region))
		return;
	g_setcolor(COL_WBACK);
	g_fillrect(0, 0, day.canvas_xs, day.canvas_ys);
	gen_day();
	g_exit_window();
}


/*
 * print PostScript day calendar to file fp.
 */

void print_day_calendar(
	FILE	*fp,
	BOOL	landscape)
{
	time_t time = get_time();
	struct tm *tm = time_to_tm(time);
	time -= (tm->tm_wday + 6 + config.sunday_first)%7 * 86400;
	if (!curr_day)
		curr_day = time;
	build_day(config.pr_omit_priv, config.pr_omit_appts);
	g_init_ps(&day, fp, landscape);
	gen_day();
	g_exit_ps();
}


/*---------------------------------- high-level drawing ---------------------*/
/*
 * draw the day, using low-level routines
 */

static void gen_day(void)
{
	struct config	*c = &config;
	time_t		today;		/* today's date */
	char		buf[40], *p;	/* text to print */
	int		d, h, l;	/* day, hour, text length */
	int		x, xs;		/* appt bar pos, col width */
	int		x0, y0;		/* top left inside line box */
	int		dx, dy;		/* size inside day line box */
	int		cy_d;		/* character height */
	int		datecolor;	/* color of date heading for day */
	struct weeknode	*vert, *horz;	/* day node scan pointers */
	struct holiday	*hp, *shp, *holp;/* to check for holidays */
	struct tm	*tm;		/* today's date as m/d/y */
#ifdef JAPAN
	strpack		partialstr[MAXPARTIALSTRING];
	unsigned char	strpool[MAXPARTIALCHAR];
#endif

	x0 = c->day_margin;
	y0 = c->day_margin + c->day_title + 3*c->day_headline + 2;
	xs = c->day_barwidth + c->day_gap;
	dy = c->day_hourheight * (c->day_maxhour - c->day_minhour);
	cy_d = font[FONT_WDAY]->max_bounds.ascent;

	strcpy(buf, mkdatestring(curr_day));			/* title */
	strcat(buf, " - ");
	strcat(buf, mkdatestring(curr_day + (c->day_ndays-1)*86400));
	g_setcolor(COL_WTITLE);
	g_setfont(FONT_WTITLE);
	g_drawtext(c->day_margin + c->day_hourwidth + 2,
		   c->day_margin + c->day_title * 3/4,
		   buf, strlen(buf));
								/* holidays */
	tm = time_to_tm(curr_day);
	if ((p = parse_holidays(tm->tm_year, FALSE)))
		create_error_popup(mainmenu.cal, 0, p);
	shp = &sm_holiday[tm->tm_yday];
	hp  =    &holiday[tm->tm_yday];

	for (d=0; d < c->day_ndays; d++, hp++, shp++) {		/* day loop */

		if (!d || config.moretimecols) {		/* hour col */
			g_setcolor(COL_WDAY);
			g_setfont(FONT_WHOUR);
			for (h=0; h <= c->day_maxhour-c->day_minhour; h++) {
				p = mktimestring((h + c->day_minhour) * 3600,
									FALSE);
				l = strlen_in_pixels(p, FONT_WDAY);
				g_drawtext(x0 + (c->day_hourwidth - l) / 2,
					   y0 + c->day_hourheight*h + cy_d/2-2,
					   p, strlen(p));
			}
			x0 += c->day_hourwidth + 2;
		}
		tm = time_to_tm(curr_day + d*86400);
		if (tm->tm_yday == 0) {				/* new year? */
			if ((p = parse_holidays(tm->tm_year, FALSE)))
				create_error_popup(mainmenu.cal, 0, p);
			shp = &sm_holiday[0];
			hp  =    &holiday[0];
		}
		if (!(dx = day.nlines[d] * xs))
			continue;

		today = get_time() - (curr_day + d*86400);	/* green? */
		if (today >= 0 && today < 86400) {
			g_setcolor(COL_CALTODAY);
			g_fillrect(x0-2, y0-7-2*c->day_headline,
				   dx+4, 2*c->day_headline+5);
		}
								/* day color */
		datecolor = tm->tm_wday == 0 ||
			    tm->tm_wday == 6 ? COL_WEEKEND : COL_WDAY;

								/* holiday */
		if ((holp = hp->string ? hp : shp)->string) {
			if (holp->daycolor)
				datecolor = holp->daycolor;
			g_setcolor(holp->stringcolor ? holp->stringcolor :
				   holp->daycolor    ? holp->daycolor
						     : COL_WDAY);
#ifdef JAPAN
			partialstr->strptr = strpool;
			if (mixedstrlen_in_pixels(holp->string,
				      partialstr, FONT_WNOTE, FONT_JNOTE) > dx)
				truncate_strpack(partialstr, dx, FONT_WNOTE);
			g_draw_strpack(x0, y0-3, partialstr);
#else
			strncpy(buf, holp->string, sizeof(buf)-1);
			truncate_string(buf, dx, FONT_WDAY);
			g_drawtext(x0, y0-3, buf, strlen(buf));
#endif
		}
		g_setfont(FONT_WHOUR);				/* date */
		g_setcolor(datecolor);
		p = mkdatestring(curr_day + d * 86400);
		truncate_string(p, dx, FONT_WHOUR);
		g_drawtext(x0, y0-5-c->day_headline, p, strlen(p));

		g_setcolor(COL_WBOXBACK);			/* box bgnd */
		g_fillrect(x0,    y0,    dx,   dy);

		g_setcolor(COL_WGRID);				/* box lines */
		g_fillrect(x0-2,  y0-2,  dx+4, 2);
		g_fillrect(x0-2,  y0+dy, dx+4, 2);
		g_fillrect(x0-2,  y0,    2,    dy);
		g_fillrect(x0+dx, y0,    2,    dy);
		for (h=1; h <= c->day_maxhour-c->day_minhour; h++)
			g_fillrect(x0, y0 + c->day_hourheight * h, dx, 1);

		x = x0 + c->day_gap/2;				/* appt bars */
		for (vert=day.tree[d]; vert; vert=vert->down) {
			for (horz=vert; horz; horz=horz->next) {
				int u = name_to_user(horz->entry->user);
				draw_bar(horz, u>0 ? user[u].color : 0, x, y0);
			}
			x += xs;
		}
		x0 += dx + c->day_margin;
	}
	g_setcolor(COL_STD);
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
	time_t		tod;		/* clipped appt time of day */
	int		yend;		/* right margin of chart */
	int		w[2], b, e;	/* early/late, begin, end */
	int		i;		/* warning counter, 0..1 */
	char		msg[256];

	tod  = BOUND(TOD(ep->time), c->day_minhour*3600, c->day_maxhour*3600);
	yend = y + c->day_hourheight * (c->day_maxhour - c->day_minhour);
	i = ep->early_warn > ep->late_warn;
	w[!i] = tod - ep->early_warn;
	w[ i] = tod - ep->late_warn;
	b     = tod;
	e     = tod + ep->length;
	w[0]  = y + YPOS(w[0]);
	w[1]  = y + YPOS(w[1]);
	b     = y + YPOS(b);
	e     = y + YPOS(e);
	if (ep->notime) {
		b = y + YPOS(c->day_minhour*3600);
		e = config.week_bignotime ? y + YPOS(c->day_maxhour*3600)
					  : b + YPOS((c->day_minhour+1)*3600);
	}
	if (config.weekwarn && !ep->notime && !node->days_warn)
		for (i=0; i < 2; i++)
			draw_octagon(COL_WWARN, FALSE, x, w[i], e, yend);

	draw_octagon(ep->suspended || node->days_warn ? COL_WWARN :
			COL_WUSER_0 + (color & 7), color > 7, x, b, e, yend);
	*msg = 0;
	if (!ep->notime) {
		strcpy(msg, mktimestring(node->trigger, FALSE));
		if (ep->length)
			sprintf(msg+strlen(msg), "-%s", mktimestring(
					node->trigger + ep->length, FALSE));
		if (node->days_warn)
			strcat(msg, _(" (warn)"));
	}
	strcat(msg, " ");
	strcat(msg, node->text);

	draw_boxtext(x, b, c->day_barwidth, e-b < MINLEN ? MINLEN : e-b,
						msg, FALSE);
}


/*
 * draw one octagon (rectangle with the corners cut off, to make it easier
 * to see where one box ends and the next begins). Allow a bottom limit to
 * allow clipping, and to show flat bottoms where an end time isn't known.
 * The 9th polygon vertex closes the polygon. It runs counter-clockwise
 * from the topmost vertex at the left edge (10-o'clock position)
 */

static void draw_octagon(
	int	color,		/* color, 0..7 */
	BOOL	checker,	/* add checkerboard pattern */
	int	x,		/* top left XY pos in canvas */
	int	y,
	int	ye,		/* Y bottom in canvas */
	int	yend)		/* bottom Y clip limit */
{
	XPoint	point[9];	/* polygon vertex list */

	if (ye < y + MINLEN) {		/* too narrow: draw flat bottom */
		ye = y + 9999;
		yend = y + MINLEN;
	}
	point[8].x =
	point[0].x =
	point[1].x = x;
	point[7].x =
	point[2].x = x + CUTOFF;
	point[6].x =
	point[3].x = x + config.day_barwidth - CUTOFF;
	point[5].x =
	point[4].x = x + config.day_barwidth;

	point[7].y =
	point[6].y = y;
	point[8].y =
	point[0].y =
	point[5].y = y + CUTOFF;
	point[1].y =
	point[4].y = BOUND(ye - CUTOFF, y, yend);
	point[2].y =
	point[3].y = BOUND(ye, y, yend);

	g_setcolor(color);
	g_drawpoly(point, 8, checker);
	g_setcolor(COL_WFRAME);
	g_drawlines(point, 9);
}


/*
 * draw a text into the given space, optionally splitting the text into
 * multiple lines. Use the small note font.
 */

#define BOXFONT FONT_NOTE
#define ISSPACE(c) ((c)==' ' || (c)=='\t')
#define MAXSPLIT 6

static void draw_boxtext(
	int		x,		/* top left corner of free space */
	int		y,
	int		xs,		/* size of free space */
	int		ys,
	char		*text,		/* text to print */
	BOOL		center)		/* center text? */
{
	char		*beg, *p, *p_s;	/* soln, curr char, since last space */
	int		len, len_s;	/* curr text length in pixels */
	int		cys, lys;	/* character height, line height */
	int		i;
#ifdef JAPAN
	int		n, plen;	/* pixel-length of partialstr */
	strpack		partialstr[MAXPARTIALSTRING], save, saveasc;
	unsigned char	strpool[MAXPARTIALCHAR];
#endif

	g_setfont(BOXFONT);
	cys = font[BOXFONT]->max_bounds.ascent;
#ifdef JAPAN
	if (cys < font[FONT_JNOTE]->max_bounds.ascent)
	  cys = font[FONT_JNOTE]->max_bounds.ascent;
#endif
	lys = cys * 5/4;
	i   = (ys-cys) / lys;
	y  += (ys - cys - i*lys) / 2 + cys;
	x  += 3;
	xs -= 5;

#ifdef JAPAN
	partialstr->strptr = strpool;
	(void) mixedstrlen_in_pixels(text, partialstr, FONT_NOTE, FONT_JNOTE);
	for (n = plen = 0; partialstr[n].strptr != NULL && n < MAXPARTIALSTRING; n++) {
	  if (partialstr[n].asciistr == False) {	/* case of Japanese font */
	    if (n + 1 < MAXPARTIALSTRING)
	      saveasc = partialstr[n + 1];
	    while (plen + partialstr[n].pixlen > xs) {
	      save = partialstr[n];
	      (void) truncate_strpack(partialstr + n, xs - plen, FONT_NOTE);
	      if (partialstr[n].strptr == NULL) {
		partialstr[n] = save;
	      } else {
		i = center ? x + (xs - partialstr[n].length) / 2 : x + plen;
		g_drawtext16(i, y, partialstr[n].strptr, partialstr[n].length / 2);
		partialstr[n].strptr += partialstr[n].length;
		partialstr[n].pixlen *= partialstr[n].length =
		  save.length - partialstr[n].length;
		partialstr[n].pixlen /= save.length;
	      }
	      plen = 0;
	      y += lys;
	      if ((ys -= lys) < cys)
		goto NO_MORE_SPACE;
	    }
	    i = center ? x + (xs - partialstr[n].pixlen) / 2 : x + plen;
	    g_drawtext16(i, y, partialstr[n].strptr, partialstr[n].length / 2);
	    if (center) {
	      plen = 0;
	      y += lys;
	      if ((ys -= lys) < cys)
		break;
	    } else
	      plen += partialstr[n].pixlen;
	    if (n + 1 < MAXPARTIALSTRING)
	      partialstr[n + 1] = saveasc;
	  } else {					/* case of ASCII font */
	    for (len_s = len = plen, beg = p_s = p = (char *) partialstr[n].strptr; *p; p++) {
	      if (*p == ' ' || *p == '\t') {
		p_s   = p;
		len_s = len;
	      }
	      len += CHARWIDTH(FONT_NOTE, *p);
	      if (!p[1])
		p++;
	      if (len > xs || !*p) {
		if (len > xs && p_s != beg
		    && p_s >= p-MAXSPLIT && ys >= lys+cys) {
		  p   = p_s;
		  len = len_s;
		}
		i = center ? x + (xs - len)/2 : x + plen;
		g_drawtext(i, y, beg, p - beg);
		if (*p) {
		  while (ISSPACE(*p)) p++;
		  beg = p_s = p--;
		  plen = len = len_s = 0;
		  y += lys;
		  if ((ys -= lys) < cys)
		    goto NO_MORE_SPACE;
		} else {
		  if (center) {
		    plen = 0;
		    y += lys;
		    if ((ys -= lys) < cys)
		      goto NO_MORE_SPACE;
		  } else
		    plen = len;
		  break;
		}
	      }
	    }
	  }
	}
NO_MORE_SPACE:;		/* It seems that some compilers need a SEMICOLON. */
#else
	for (len_s=len=0, beg=p_s=p=text; *p; p++) {
		if (*p == ' ' || *p == '\t') {
			p_s   = p;
			len_s = len;
		}
		len += CHARWIDTH(BOXFONT, *p);
		if (!p[1])
			p++;
		if (len > xs || !*p) {
			if (len > xs && p_s != beg
				     && p_s >= p-MAXSPLIT && ys >= lys+cys) {
				p   = p_s;
				len = len_s;
			}
			i = center ? x + (xs - len)/2 : x;
			g_drawtext(i, y, beg, p-beg);
			while (ISSPACE(*p)) p++;
			beg = p_s   = p--;
			len = len_s = 0;
			y += lys;
			if ((ys -= lys) < cys)
				break;
		}
	}
#endif
}
