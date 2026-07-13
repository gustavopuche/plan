/*
 * Arrange entries in the day view.
 *
 *	build_day(omit_priv,omit_all)	Build the day node trees for all days
 *					of the day beginning on curr_day.
 *	destroy_day()			Release the day node trees.
 */

#include <stdio.h>
#include <stdlib.h>
#if !defined NEWSOS4 && !defined STDLIBMALLOC && !defined MACOSX
#include <malloc.h>
#endif
#include <time.h>
#include <Xm/Xm.h>
#include "cal.h"

extern struct config	config;		/* global configuration data */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern struct user	*user;		/* user list (from file_r.c) */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern int		curr_year;	/* year being displayed, since 1900 */

time_t			curr_day;	/* day being displayed, time in sec */
struct week		day;		/* info on day view */

static BOOL add_day_entry(int, time_t, BOOL, struct entry *, int);


/*
 * given the day number, build the data structures that describe all entries
 * in the display. Before calling this routine, set day_curr to 0:00 of the
 * day. Here, nlines is actually ncolumns, up/down is horizontal, and next/
 * prev is vertical because the time-of-day axis is vertical in day views.
 */

void build_day(
	BOOL		omit_priv,	/* ignore private (padlocked) entries*/
	BOOL		omit_all)	/* ignore all ent (not very useful) */
{
	struct config	*c = &config;
	time_t		date;		/* midnight of calculated day */
	int		wday;		/* day counter, 0..NDAYS-1 */
	int		nlines=0;	/* total number of bar columns */
	struct entry	*ep;		/* ptr to entry in mainlist */
	BOOL		found;		/* TRUE if lookup succeeded */
	struct lookup	lookup;		/* result of entry lookup */
	struct weeknode	*wp;		/* for counting # of lines on a day */
	int		u;		/* user number, -1 == ourselves */
	int		i;		/* for checking exceptions */
	time_t		trigger;	/* trigger date/time of *ep */

	destroy_day();
	if (!c->day_ndays)	c->day_ndays   = c->week_ndays;
	if (!c->day_minhour)	c->day_minhour = c->week_minhour;
	if (!c->day_maxhour)	c->day_maxhour = c->week_maxhour;
	if (!c->day_title)	c->day_title   = c->week_title;

	if (!curr_day)
		curr_day = get_time();
	curr_day = date = curr_day - curr_day % 86400;
	if (!(day.tree   = malloc(NDAYS * sizeof(struct weeknode *))) ||
	    !(day.nlines = malloc(NDAYS * sizeof(int))))
		fatal(_("no memory for day view"));
	memset((void *)day.tree,   0, NDAYS * sizeof(struct weeknode *));
	memset((void *)day.nlines, 0, NDAYS * sizeof(int));

	for (wday=0; wday < c->day_ndays; wday++, date+=86400) {
		day.nlines[wday] = 0;
		day.tree[wday] = 0;
		found = lookup_entry(&lookup, mainlist, date, TRUE, FALSE,
								'd', -1);
		for (; found; found = lookup_next_entry(&lookup)) {
			if (omit_all || lookup.trigger >= date + 86400)
				break;
			ep = &mainlist->entry[lookup.index];
			if (ep->hide_in_d || (ep->private && omit_priv))
				continue;
			u = name_to_user(ep->user);
			if (user[u].suspend_d)
				continue;
			trigger = lookup.trigger - lookup.trigger % 86400;
			for (i=0; i < NEXC; i++)
				if (ep->except[i] == trigger)
					break;
			if (i < NEXC)
				continue;
			if (!add_day_entry(wday, lookup.trigger,
					lookup.days_warn,
					&mainlist->entry[lookup.index], u)) {
				destroy_day();
				return;
			}
		}
		for (wp=day.tree[wday]; wp; wp=wp->down)
			day.nlines[wday]++;
		if (!day.nlines[wday])
			day.nlines[wday] = 1;
		nlines += day.nlines[wday];
	}
	day.canvas_xs	= c->day_margin
			+ c->day_hourwidth
			+ c->day_gap
			+ nlines * (c->day_barwidth + 2*c->day_gap)
			+ c->day_ndays * c->day_margin
			+ (config.moretimecols ? (c->day_ndays - 1) *
						 (c->day_hourwidth + 2) : 0);

	day.canvas_ys	= c->day_margin
			+ c->day_title
			+ c->day_headline*3
			+ c->day_gap
			+ c->day_hourheight * (c->day_maxhour - c->day_minhour)
			+ c->day_gap
			+ c->day_margin;
}


/*
 * insert an entry into the tree for day day <wday>. Try to find a hole
 * in an existing line, or append a new line if no hole can be found. This
 * routine is used by build_day() only. Entries without time go to the
 * beginning of the list, with time 0.
 */

#define GET_EXTENT(node, ep, begin, end)				   \
{									   \
	begin = ep->notime	 ? node->trigger + config.day_minhour*3600 \
	      : !config.weekwarn ? node->trigger			   \
				 : ep->early_warn > ep->late_warn	   \
					 ? node->trigger - ep->early_warn  \
					 : node->trigger - ep->late_warn;  \
	end = ep->notime ? config.week_bignotime			   \
			   ? node->trigger + 86400			   \
			   : begin + 3600				   \
			 : node->trigger + ep->length;			   \
}

static BOOL add_day_entry(
	int		wday,			/* day counter, 0..NDAYS-1 */
	time_t		time,			/* appropriate trigger time */
	BOOL		warn,			/* it's a n-days-ahead warng */
	struct entry	*new,			/* entry to insert */
	int		u)			/* owner user, 0=own */
{
	struct weeknode	*node;			/* new day struct */
	struct weeknode	*vert;			/* for finding free line */
	struct weeknode	*horz;			/* for finding free slot */
	struct entry	*old;			/* already inserted entry */
	time_t		new_begin, new_end;	/* extent of entry to insert */
	time_t		old_begin, old_end;	/* extent of existing entry */

	if (!(node = (struct weeknode *)malloc(sizeof(struct weeknode))))
		return(FALSE);
	node->next	= node->prev = node->up = node->down = 0;
	node->entry	= new;
	node->trigger	= time;
	node->days_warn	= warn;
	node->text[0]	= 0;
	if (config.weekuser && u && user[u].name)
		sprintf(node->text, "%s: ", user[u].name);
	strncat(node->text, mknotestring(new),
				sizeof(node->text)-1-strlen(node->text));
	node->text[sizeof(node->text)-1] = 0;
	node->textlen = strlen_in_pixels(node->text, FONT_WNOTE);
	node->textinside = TRUE;
	if (!day.tree[wday]) {
		day.tree[wday] = node;
		return(TRUE);
	}

	GET_EXTENT(node, new, new_begin, new_end);

	for (vert=day.tree[wday]; vert; vert=vert->down) {
		old = vert->entry;
		GET_EXTENT(vert, old, old_begin, old_end);
		if (new_end <= old_begin) {		/* first in column? */
			node->up   = vert->up;
			node->down = vert->down;
			node->next = vert;
			vert->prev = node;
			day.tree[wday] = node;
			return(TRUE);
		}
		for (horz=vert; horz; horz=horz->next) {
			if (new_begin < old_end)
				break;
			if (!horz->next) {		/* last in column? */
				node->prev = horz;
				horz->next = node;
				return(TRUE);
			}
			old = horz->next->entry;
			GET_EXTENT(horz->next, old, old_begin, old_end);
			if (new_end <= old_begin) {	/* fits in hole? */
				node->next = horz->next;
				node->prev = horz;
				horz->next->prev = node;
				horz->next = node;
				return(TRUE);
			}
		}
		if (!vert->down) {			/* append new column?*/
			vert->down = node;
			node->up   = vert;
			return(TRUE);
		}
	}
	return(FALSE);	/* we never get here */
}


/*
 * destroy the day trees for all days.
 */

void destroy_day(void)
{
	int		wday;			/* day counter */
	struct weeknode	*vert, *nextvert;	/* line pointer */
	struct weeknode	*horz, *nexthorz;	/* node pointer */

	if (!day.tree)
		return;
	for (wday=0; wday < NDAYS; wday++) {
		for (vert=day.tree[wday]; vert; vert=nextvert) {
			nextvert = vert->down;
			for (horz=vert; horz; horz=nexthorz) {
				nexthorz = horz->next;
				free((void *)horz);
			}
		}
		day.tree[wday] = 0;
	}
	free(day.tree);
	free(day.nlines);
	day.tree = 0;
	day.nlines = 0;
}
