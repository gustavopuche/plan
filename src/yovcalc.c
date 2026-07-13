/*
 * Arrange entries in the yov view.
 *
 *	build_yov(omit_priv)		Build the yov node trees for all days
 *					of the yov beginning on curr_yov.
 *	destroy_yov()			Release the yov node trees.
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
extern int		nusers;		/* number of users in user list */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */

time_t			curr_yov;	/* yov being displayed, time in sec */
struct week		yov;		/* info on yov view */
int			yov_nstrips;	/* number of strips (max nusers+1) */
static unsigned short	ignore_id;	/* identifies current run */

static BOOL add_yov_entry(int, time_t, BOOL, struct entry *);


/*
 * given the first day of a yov, build the data structures that describe
 * all entries in the display. Before calling this routine, set yov_curr
 * to 0:00 of the first day of the year. This code uses the entry->ignore
 * number. It is incremented every time this is called, and assigned the
 * current number when found. When an appointment is to be added that
 * already has the current <ignore_id> number, *and* if it is a multiday
 * bar (rep_every == 1day), ignore it because the single multiday bar covers
 * all the days it triggers on. Using a counter is easier than having to
 * clear traversal flags in entries.
 * Collect the users 0..nusers (0=own) into pre-allocated strips, then
 * eliminate the strips that have remained empty. A strip is a shaded gray
 * box containing entries from a single user.
 */

void build_yov(
	BOOL		omit_priv)	/* ignore private (padlocked) entries*/
{
	struct config	*c = &config;
	time_t		date;		/* 0:00 on January 1 of displ'd year */
	int		day, ndays;	/* day counter 0..365 or 366 */
	int		nlines=0;	/* total number of bar lines */
	struct entry	*ep;		/* ptr to entry in mainlist */
	BOOL		found;		/* TRUE if lookup succeeded */
	struct lookup	lookup;		/* result of entry lookup */
	struct weeknode	*wp;		/* for counting # of lines on a day */
	int		u;		/* user number, -1 == ourselves */
	int		i;		/* for checking exceptions */
	time_t		trigger;

	if (c->yov_nmonths < 1)
		c->yov_nmonths = 1;
	if (c->yov_nmonths > 12)
		c->yov_nmonths = 12;
	if (c->yov_display < 0 || c->yov_display > 3)
		c->yov_display = 0;
	if (c->yov_user < 0 || c->yov_user > nusers)
		c->yov_user = 0;
	if (c->yov_wwidth < 10)
		c->yov_wwidth = 870;
	if (c->yov_wheight < 10)
		c->yov_wheight = 400;
	if (c->yov_daywidth < 1)
		c->yov_daywidth = 24;

	ignore_id++;
	destroy_yov();
	curr_yov = date = curr_yov - curr_yov % 86400;
	if (!(yov.tree   = malloc(nusers * sizeof(struct weeknode *))) ||
	    !(yov.nlines = malloc(nusers * sizeof(int))))
		fatal(_("no memory for year overview"));
	ndays = time_to_tm(date)->tm_year & 3 ? 365 : 366;

	for (u=0; u < nusers; u++)
		yov.tree[u] = 0;

	for (day=0; day < ndays; day++) {
		found = lookup_entry(&lookup, mainlist, date, TRUE, FALSE,
								'o', -1);
		for (; found; found = lookup_next_entry(&lookup)) {
			if (lookup.trigger >= date + 86400)
				break;
			ep = &mainlist->entry[lookup.index];
			if (ep->ignore == ignore_id && ep->rep_every == 86400)
				continue;
			if (!lookup.days_warn)
				ep->ignore = ignore_id;
			if (ep->hide_in_o || (ep->private && omit_priv))
				continue;
			if (!c->yov_single && ep->rep_every != 86400)
				continue;
			u = name_to_user(ep->user);
			if ((c->yov_display == 0 && user[u].suspend_o)	||
			    (c->yov_display == 2 && u != 0)		||
			    (c->yov_display == 3 && u != c->yov_user))
				continue;
			trigger = lookup.trigger - lookup.trigger % 86400;
			if (ep->rep_every != 86400) {
				for (i=0; i < NEXC; i++)
					if (ep->except[i] == trigger)
						break;
				if (i < NEXC)
					continue;
			}
			if (!add_yov_entry(u, lookup.trigger,
					   lookup.days_warn,
					   &mainlist->entry[lookup.index])) {
				destroy_yov();
				return;
			}
		}
		date += 86400;
	}
	for (yov_nstrips=u=0; u < nusers; u++)
		if (yov.tree[u]) {
			yov.tree  [yov_nstrips] = yov.tree  [u];
			yov.nlines[yov_nstrips] = 0;
			for (wp=yov.tree[yov_nstrips]; wp; wp=wp->down)
				yov.nlines[yov_nstrips]++;
			if (!yov.nlines[yov_nstrips])
				yov.nlines[yov_nstrips] = 1;
			nlines += yov.nlines[yov_nstrips];
			yov_nstrips++;
		}
	yov.canvas_xs	= c->week_margin
			+ c->week_daywidth		/* user name */
			+ c->week_gap
			+ c->yov_daywidth / c->yov_nmonths * ndays
			+ c->week_margin;

	yov.canvas_ys	= c->week_margin
			+ c->week_hour			/* month name */
			+ c->week_hour			/* day number */
			+ yov_nstrips * c->week_gap
			+ nlines * (c->week_barheight + 2*c->week_bargap)
			+ c->week_gap
			+ c->week_margin;
}


/*
 * insert an entry into the tree for yov day <strip>. Try to find a hole
 * in an existing line, or append a new line if no hole can be found. This
 * routine is used by build_yov() only. It works with day numbers, not
 * seconds, and a bar always fills entire days.
 */

#define GET_EXTENT(node, ep, begin, end)				\
{									\
	begin = node->trigger / 86400;					\
	end   = node->days_warn ||					\
		ep->rep_every != 86400 ? begin + 1 :			\
		ep->rep_last           ? ep->rep_last / 86400 + 1	\
				       : 999999;			\
	if (!node->textinside)						\
		end += (node->textlen + daysz + 5) / daysz;		\
}

static BOOL add_yov_entry(
	int		strip,			/* strip (user) number */
	time_t		time,			/* appropriate trigger time */
	BOOL		warn,			/* it's a n-days-ahead warng */
	struct entry	*new)			/* entry to insert */
{
	struct weeknode	*node;			/* new yov struct */
	struct weeknode	*vert;			/* for finding free line */
	struct weeknode	*horz;			/* for finding free slot */
	struct entry	*old;			/* already inserted entry */
	time_t		new_begin, new_end;	/* extent of entry to insert */
	time_t		old_begin, old_end;	/* extent of existing entry */
	int		space_avail;		/* width of space for note */
	int		daysz;			/* zoomed day width in pixels*/
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
	strncpy(node->text, mknotestring(new), sizeof(node->text)-1);
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
	daysz = config.yov_daywidth / config.yov_nmonths;
	node->textinside = TRUE;
	GET_EXTENT(node, new, new_begin, new_end);
	node->textinside = node->textlen + config.week_barheight <=
						(new_end - new_begin) * daysz;
	if (!yov.tree[strip]) {
		yov.tree[strip] = node;
		return(TRUE);
	}

	GET_EXTENT(node, new, new_begin, new_end);

	for (vert=yov.tree[strip]; vert; vert=vert->down) {
		old = vert->entry;
		GET_EXTENT(vert, old, old_begin, old_end);
		if (new_end <= old_begin) {		/* first in line? */
			node->up   = vert->up;
			node->down = vert->down;
			node->next = vert;
			vert->prev = node;
			yov.tree[strip] = node;
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
 * destroy the yov trees for all days.
 */

void destroy_yov(void)
{
	int		strip;			/* day counter */
	struct weeknode	*vert, *nextvert;	/* line pointer */
	struct weeknode	*horz, *nexthorz;	/* node pointer */

	if (!yov.tree)
		return;
	for (strip=0; strip < yov_nstrips; strip++) {
		for (vert=yov.tree[strip]; vert; vert=nextvert) {
			nextvert = vert->down;
			for (horz=vert; horz; horz=nexthorz) {
				nexthorz = horz->next;
				free((void *)horz);
			}
		}
		yov.tree[strip] = 0;
	}
	free(yov.tree);
	free(yov.nlines);
	yov.tree = 0;
	yov.nlines = 0;
}
