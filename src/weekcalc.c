/*
 * Arrange entries in the week view.
 *
 *	build_week(omit_priv,omit_all)	Build the week node trees for all days
 *					of the week beginning on curr_week.
 *	destroy_week()			Release the week node trees.
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

time_t			curr_week;	/* week being displayed, time in sec */
struct week		week;		/* info on week view */

static BOOL add_week_entry(int, time_t, BOOL, struct entry *, int);


/*
 * given the first day of a week, build the data structures that describe
 * all entries in the display. Before calling this routine, set week_curr
 * to 0:00 of the first day of the week. The code is basically a sequence
 * of reasons NOT to show an entry...
 */

void build_week(
	BOOL		omit_priv,	/* ignore private (padlocked) entries*/
	BOOL		omit_all)	/* ignore all ent (not very useful) */
{
	struct config	*c = &config;
	time_t		date;		/* midnight of calculated day */
	int		wday;		/* day counter, 0..NDAYS-1 */
	int		nlines=0;	/* total number of bar lines */
	struct entry	*ep;		/* ptr to entry in mainlist */
	BOOL		found;		/* TRUE if lookup succeeded */
	struct lookup	lookup;		/* result of entry lookup */
	struct weeknode	*wp;		/* for counting # of lines on a day */
	int		u;		/* user number, -1 == ourselves */
	int		i;		/* for checking exceptions */
	time_t		trigger;	/* trigger date/time of *ep */

	if (!c->week_maxhour) c->week_maxhour = 20;

	destroy_week();
	curr_week = date = curr_week - curr_week % 86400;
	if (!(week.tree   = malloc(NDAYS * sizeof(struct weeknode *))) ||
	    !(week.nlines = malloc(NDAYS * sizeof(int))))
		fatal(_("no memory for week view"));
	memset((void *)week.tree,   0, NDAYS * sizeof(struct weeknode *));
	memset((void *)week.nlines, 0, NDAYS * sizeof(int));

	for (wday=0; wday < c->week_ndays; wday++, date+=86400) {
		week.nlines[wday] = 0;
		week.tree[wday] = 0;
		found = lookup_entry(&lookup, mainlist, date, TRUE, FALSE,
								'w', -1);
		for (; found; found = lookup_next_entry(&lookup)) {
			if (omit_all || lookup.trigger >= date + 86400)
				break;
			ep = &mainlist->entry[lookup.index];
			if (ep->hide_in_w || (ep->private && omit_priv))
				continue;
			u = name_to_user(ep->user);
			if (user[u].suspend_w)
				continue;
			trigger = lookup.trigger - lookup.trigger % 86400;
			for (i=0; i < NEXC; i++)
				if (ep->except[i] == trigger)
					break;
			if (i < NEXC)
				continue;
			if (!add_week_entry(wday, lookup.trigger,
					lookup.days_warn,
					&mainlist->entry[lookup.index], u)) {
				destroy_week();
				return;
			}
		}
		for (wp=week.tree[wday]; wp; wp=wp->down)
			week.nlines[wday]++;
		if (!week.nlines[wday])
			week.nlines[wday] = 1;
		nlines += week.nlines[wday];
	}
	week.canvas_xs	= c->week_margin
			+ c->week_daywidth
			+ c->week_gap
			+ c->week_hourwidth *
					(c->week_maxhour - c->week_minhour)
			+ c->week_margin;

	week.canvas_ys	= c->week_margin
			+ c->week_title
			+ c->week_margin
			+ c->week_hour
			+ c->week_ndays * c->week_gap
			+ nlines * (c->week_barheight + 2*c->week_bargap)
			+ c->week_gap
			+ c->week_margin;
}


/*
 * insert an entry into the tree for week day <wday>. Try to find a hole
 * in an existing line, or append a new line if no hole can be found. This
 * routine is used by build_week() only.
 */

#define GET_EXTENT(node, ep, begin, end)				   \
{									   \
	begin = !config.weekwarn ? node->trigger			   \
				 : ep->early_warn > ep->late_warn	   \
					 ? node->trigger - ep->early_warn  \
					 : node->trigger - ep->late_warn;  \
	end = ep->notime ? config.week_bignotime			   \
			   ? node->trigger + 86400			   \
			   : node->trigger + config.week_minhour * 3600	   \
			 : node->trigger + ep->length;			   \
	if (!node->textinside)						   \
		end += (node->textlen+6) * 3600/config.week_hourwidth;	   \
}

static BOOL add_week_entry(
	int		wday,			/* day counter, 0..NDAYS-1 */
	time_t		time,			/* appropriate trigger time */
	BOOL		warn,			/* it's a n-days-ahead warng */
	struct entry	*new,			/* entry to insert */
	int		u)			/* owner user, 0=own */
{
	struct weeknode	*node;			/* new week struct */
	struct weeknode	*vert;			/* for finding free line */
	struct weeknode	*horz;			/* for finding free slot */
	struct entry	*old;			/* already inserted entry */
	time_t		new_begin, new_end;	/* extent of entry to insert */
	time_t		old_begin, old_end;	/* extent of existing entry */
	int		space_avail;		/* width of space for note */
#ifdef JAPAN
	int		i, clen = 0, plen = 0;
	strpack		partialstr[MAXPARTIALSTRING];
	unsigned char	strpool[MAXPARTIALCHAR];
#endif

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
	space_avail = new->length * config.week_hourwidth / 3600 -
							config.week_barheight;
#ifdef JAPAN
	partialstr->strptr = strpool;
	if ((node->textlen = mixedstrlen_in_pixels(node->text, partialstr,
						   FONT_WNOTE, FONT_JNOTE)) >
			config.week_maxnote) {
		for (i=0; partialstr[i].strptr != NULL &&
			  i < MAXPARTIALSTRING ||
					((node->textlen = plen), 0); i++) {
			if (plen + partialstr[i].pixlen > config.week_maxnote){
								/* Truncating*/
				partialstr[i].length *=
					(double)(config.week_maxnote-plen) /
						(double)partialstr[i].pixlen;
				node->textlen = config.week_maxnote;
				if (partialstr[i].length == 0 ||
				    partialstr[i].asciistr == False &&
				   (partialstr[i].length &= ~1) == 0)
					partialstr[i].strptr = NULL;
				node->text[clen += partialstr[i].length] ='\0';
				break;
			} else {
				clen += partialstr[i].length;
				plen += partialstr[i].pixlen;
			}
		}
	}
#else
	truncate_string(node->text, space_avail > config.week_maxnote ?
				space_avail : config.week_maxnote, FONT_WNOTE);
	node->textlen = strlen_in_pixels(node->text, FONT_WNOTE);
#endif
	node->textinside = (new->notime && config.week_bignotime) ||
	                   space_avail >= node->textlen;

	if (!week.tree[wday]) {
		week.tree[wday] = node;
		return(TRUE);
	}

	GET_EXTENT(node, new, new_begin, new_end);

	for (vert=week.tree[wday]; vert; vert=vert->down) {
		old = vert->entry;
		GET_EXTENT(vert, old, old_begin, old_end);
		if (new_end <= old_begin) {		/* first in line? */
			node->up   = vert->up;
			node->down = vert->down;
			node->next = vert;
			vert->prev = node;
			week.tree[wday] = node;
			return(TRUE);
		}
		for (horz=vert; horz; horz=horz->next) {
			if (new_begin < old_end)
				break;
			if (!horz->next) {		/* last in line? */
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
		if (!vert->down) {			/* append new line? */
			vert->down = node;
			node->up   = vert;
			return(TRUE);
		}
	}
	return(FALSE);	/* we never get here */
}


/*
 * destroy the week trees for all days.
 */

void destroy_week(void)
{
	int		wday;			/* day counter */
	struct weeknode	*vert, *nextvert;	/* line pointer */
	struct weeknode	*horz, *nexthorz;	/* node pointer */

	if (!week.tree)
		return;
	for (wday=0; wday < NDAYS; wday++) {
		for (vert=week.tree[wday]; vert; vert=nextvert) {
			nextvert = vert->down;
			for (horz=vert; horz; horz=nexthorz) {
				nexthorz = horz->next;
				free((void *)horz);
			}
		}
		week.tree[wday] = 0;
	}
	free(week.tree);
	free(week.nlines);
	week.tree = 0;
	week.nlines = 0;
}
