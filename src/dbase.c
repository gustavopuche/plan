/*
 * list management: all entries are kept in lists. A list is a header
 * followed by an array of entries. List sizes are dynamic. Array space
 * is allocated in multiples of CHUNK; if space runs out the list is
 * re-allocated. Copying is done the simple way; let's hope lists don't
 * get too big. There is room for improvement here.
 *
 * Normally, there is one "master list" with everything in the calendar
 * file, from which sublists are created as needed, for example everything
 * on a particular day. Sublists are managed in sublist.c.
 *
 *	name_to_user(name)		convert user name to index into
 *					user[] array.
 *	create_list(list)		Start a new list.
 *	destroy_list(list)		Delete a list.
 *	add_entry(list, entry)		Add an entry to a list. This may
 *					require realloc'ing the list.
 *	delete_entry(list, n)		Delete an entry from the list.
 *	resort_entry(list, n)		Move a particular (newly added or
 *					changed) entry to its proper place.
 *	rebuild_repeat_chain(list)	chain all repeating entries in the
 *					list.
 *
 *	lookup_entry(lookup, list, time, exact, norep, which, usr)
 *					Find an entry in the list by date.
 *					Either finds first, or insert place.
 *	lookup_next_entry(lookup)		Find the next entry.
 *
 *	find_next_trigger(list, time, entryp, wait_time)
 *					Find next alarm or warn trigger time.
 *	clone_entry(new, old)		Copy one entry to another, and all
 *					malloced strings in it.
 *	destroy_entry(entry)		Free all malloced strings of an entry.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Xm/Xm.h>
#include "cal.h"

#define CHUNK		100		/* pointer list allocation unit */

extern struct user	*user;		/* user list (from file_r.c) */
extern int		nusers;		/* number of users in user list */
time_t			cutoff;		/* no alarms before this time */
					/* (used by daemon only) */
extern BOOL		debug;		/* print debugging information */


/*----------------------------------- user mgmt -----------------------------*/
/*
 * return user number for an entry. Call with entry->user if entry->user is
 * not null, and use result to index user[] array. Return -1 for errors
 * (shouldn't happen).
 */

int name_to_user(
	char		*name)		/* name to look up */
{
	int		u;		/* index into user array */

	if (!name)
		return(0);
	for (u=0; u < nusers; u++)
		if (user[u].name && !strcmp(user[u].name, name))
			return(u);
	return(0);
}


/*----------------------------------- list mgmt -----------------------------*/
/*
 * create an empty list, with CHUNK blank entries. When the blank entries
 * are all filled, add_entry will allocate more. It's usually enough though.
 */

void create_list(
	struct plist	**list)		/* pointer to list pointer */
{
	if (!(*list = (struct plist *)malloc(sizeof(struct plist) +
					    sizeof(struct entry) * CHUNK)))
		fatal(_("no memory"));
	(*list)->modified  = FALSE;
	(*list)->locked    = FALSE;
	(*list)->nentries  = 0;
	(*list)->size      = CHUNK;
	(*list)->repeating = -1;
}


/*
 * destroy a list, by freeing all its entries and then the list itself.
 * This is used by the daemon only, which re-reads the database when it
 * gets a HUP signal. Ignore the lint complaint.
 */

void destroy_list(
	struct plist	**list)		/* pointer to list pointer */
{
	int		i;		/* entry counter */

	if (!*list)
		return;
	(*list)->locked++;
	for (i=0; i < (*list)->nentries; i++)
		destroy_entry(&(*list)->entry[i]);
	(*list)->nentries = 0;
	free(*list);
	*list = 0;
}


/*
 * if an entry is being resorted, make sure it doesn't change its place in
 * the list relative to other entries that have the same time. Return the
 * modified index n.
 */

static int sort_same_time(
	struct entry	*entry,		/* changed entry */
	struct plist	*list,		/* list to adjust */
	int		n)		/* index to proposed new pos */
{
	while (n > 0 && list->entry[n-1].time == entry->time
		     && list->entry[n-1].seq > entry->seq)
		n--;
	while (n < list->nentries-1 && list->entry[n].time == entry->time
				    && list->entry[n].seq < entry->seq)
		n++;
	return(n);
}


/*
 * add an entry to a list. The list is kept sorted. Since the list can grow,
 * a pointer to a pointer is passed; the pointed-to pointer changes if the
 * list is re-allocated (or allocated for the first time). To start a new
 * list, pass a pointer to a null pointer; to get rid of a list, free() it.
 * Returns the index to the new entry in the list.
 * Don't forget to call rebuild_repeat_chain(), because this routine is
 * trashing the chain.
 */


int add_entry(
	struct plist		**list,		/* list to add to */
	struct entry		*entry)		/* entry to add */
{
	int			num = 0;	/* # of entries in old list */
	int			size = 0;	/* size of allocated old list*/
	struct plist		*new;		/* larger list if required */
	struct lookup		lookup;		/* result of entry lookup */
	int			n;		/* index of new entry in list*/
	struct entry		*p, *q;		/* entry copy pointers */
	int			i;		/* entry copy counter */
	static unsigned int	next_seq;	/* next entry->seq number */

	if (*list) {
		(*list)->locked++;
		num  = (*list)->nentries;
		size = (*list)->size;
	}
	if (num+1 > size) {				/* need larger list */
		size += CHUNK;
		if (!(new = (struct plist *)malloc(sizeof(struct plist) +
						  sizeof(struct entry) *size)))
			fatal(_("no memory"));
		if (!*list)
			new->nentries = 0;		/* start new list... */
		else {
			new->nentries = num;		/* ...or copy old */
			p = (*list)->entry;
			q = new->entry;
			for (i=0; i < num; i++)
				*q++ = *p++;
		}
		new->size = size;
		new->locked = 1;
		if (*list)				/* discard old list */
			free(*list);
		*list = new;
	}
	new = *list;					/* insert entry */
	n = lookup_entry(&lookup, new, entry->time, FALSE, TRUE, '*', -1)
				? lookup.index : new->nentries;
	n = sort_same_time(entry, new, n);
	p = &new->entry[n];
	q = &new->entry[new->nentries];
	for (i=new->nentries; i > n; i--, q--)
		q[0] = q[-1];
	clone_entry(p, entry);
	new->nentries++;
	new->repeating = 0;
	new->modified = TRUE;
	new->locked--;
	entry->seq = next_seq++;
	return(n);
}


/*
 * delete an entry from a list. The list is never re-allocated because lists
 * never shrink, so a simple pointer to a list is expected, and the index to
 * the entry to be deleted.
 * Call rebuild_repeat_chain() afterwards, this routine trashes the chain.
 */

void delete_entry(
	struct plist	*list,		/* list to delete from */
	int		n)		/* index to entry to delete */
{
	struct entry	*q;		/* entry copy pointer */
	int		i;		/* entry copy counter */

	list->locked++;
	if (n < 0 || n >= list->nentries) {
		list->locked--;
		return;
	}
	destroy_entry(q = &list->entry[n]);
	for (i=n; i < list->nentries; i++, q++)
		q[0] = q[1];
	list->nentries--;
	list->repeating = 0;
	list->modified = TRUE;
	list->locked--;
}


/*
 * delete an entry from a list, by file ID and row ID. This is called when
 * a network server sends a new appointment that is supposed to replace the
 * corresponding older appointment in the local database, because some other
 * plan has edited the appointment.
 * Call rebuild_repeat_chain() afterwards, this routine trashes the chain.
 */

void delete_entry_by_id(
	struct plist	*list,		/* list to delete from */
	unsigned int	file,		/* file ID from server */
	int		id)		/* row ID from server */
{
	struct entry	*ep;		/* entry search pointer */
	int		n;		/* entry search counter */

	list->locked++;
	for (ep=list->entry, n=0; n < list->nentries; n++, ep++)
		if (ep->file == file && ep->id == id) {
			delete_entry(list, n);
			list->locked--;
			return;
		}
	list->locked--;
}


/*
 * if the time of an entry changed, the list has to be re-sorted. Copy a
 * block if entries to fill the hole, and move the changed entry.
 * Returns the index to the moved entry.
 * Call rebuild_repeat_chain() afterwards, this routine trashes the chain.
 */

int resort_entry(
	struct plist	*list,		/* list to re-sort */
	int		n)		/* index to changed entry */
{
	struct lookup	lookup;		/* result of entry lookup */
	struct entry	*p;		/* entry copy pointer */
	int		i;		/* entry copy counter */
	struct entry	save;		/* to save changed entry */
	int		d;		/* new destination index */

	list->locked++;
	if (n < 0 || n >= list->nentries) {
		list->locked--;
		return(0);
	}
	p = &list->entry[n];
	d = lookup_entry(&lookup, list, p->time, FALSE, TRUE, '*', -1)
				? lookup.index : list->nentries;
	if (d == n) {					/* no change? */
		list->locked--;
		return(n);
	}
	save = *p;
	d = sort_same_time(p, list, d);
	if (d < n)					/* move back in time?*/
		for (i=d; i < n; i++, p--)
			p[0] = p[-1];
	else						/* move forward? */
		for (i= --d; i > n; i--, p++)
			p[0] = p[1];
	*p = save;
	list->repeating = 0;
	list->modified = TRUE;
	list->locked--;
	return(d);
}


/*
 * rebuild the chain of repeating entries. Repeating entries are chained. The
 * chain is anchored in the entry list struct. Repeating entries should appear
 * in all day boxes and entry lists they will appear in during their lifetime.
 * The idea is that there are probably relatively few repeating entries, and
 * they can be found by following the chain. All other entries can still be
 * found using binary searches.
 * n-days-ahead entries are also added to this list, even if they are not
 * repeating (which means that they are in the binary-sorted entry list also)
 * This avoids having to make copies, and n-days-ahead warnings for repeating
 * entries are difficult.
 * Rebuilding could be part of add_entry(), but that would be inefficient
 * when a file is read in, because then it's sufficient to build the chain
 * once at EOF, not every time an entry is added.
 */

void rebuild_repeat_chain(
	struct plist	*list)		/* list to delete from */
{
	int		*prev;		/* previous chain pointer */
	struct entry	*ep;		/* scan pointer */
	int		n;		/* entry counter */

	if (list) {
		list->locked++;
		prev = &list->repeating;
		for (n=0, ep=list->entry; n < list->nentries; n++, ep++)
			if (ep->rep_every || ep->rep_weekdays ||
			    ep->rep_days  || ep->rep_yearly || ep->days_warn) {
				*prev = n;
				prev  = &ep->nextrep;
			}
		*prev = -1;
		list->locked--;
	}
}


/*
 * find an entry with a given time. if <exact> is TRUE, return the index of
 * the first matching entry (this is a straight lookup). If <exact> is FALSE,
 * return the index of the first entry after the last matching entry (this is
 * for inserting, always append if there are multiple actors with the same
 * time). If there is no match, return the index of the first entry with a
 * larger time, regardless of <exact>.
 * If <norep> is TRUE, ignore later instances of repeating entries. This
 * is used when new entries are inserted into the list; when looking up
 * an entry for printing, it is FALSE. Setting <norep> to TRUE also prevents
 * list->repeating to be used before the chain exists.
 * The lookup is a two-step process: entries at a particular date are found
 * using a binary search; the list is sorted by time. Future instances of
 * repeating entries cannot be found that way; they are found by evaluating
 * all repeating entries. Repeating entries are found by following the
 * <nextrep> chain that was built by rebuild_repeat_chain().
 * Return FALSE if the lookup failed, and we fell off the end of the list.
 */

static int lookup_regular_entry  (struct plist *, time_t, BOOL, BOOL);
static int lookup_repeating_entry(struct plist *, time_t, BOOL, time_t *,
							char, int, BOOL *);
BOOL lookup_entry(
	struct lookup	*lookup,	/* return struct */
	struct plist	*list,		/* list of entries */
	time_t		time,		/* time to locate */
	BOOL		exact,		/* need entry, not insert pt */
	BOOL		norep,		/* ignore instances */
	char		which,		/* 'mywod': mon,yr,wk,yov,day*/
	int		usr)		/* user number or -1 */
{
	int		repindex, regindex;
	time_t		reptime = -1, regtime = -1;
	BOOL		warn = 0;

	list->locked++;
	lookup->regindex  = lookup->repindex = -1;
	lookup->regtime   = lookup->reptime  = 0;
	lookup->which	  = which;
	lookup->user	  = usr;
	lookup->days_warn = 0;
	regindex = lookup_regular_entry(list, time, exact, norep);
	repindex = norep ? -1
			 : lookup_repeating_entry(list, time, exact,
						  &reptime, which, usr, &warn);
	if (regindex >= 0)
		regtime = list->entry[regindex].time;
	list->locked--;

	if (repindex >= 0 && (regtime < 0 || reptime < regtime)) {
		lookup->index	  = lookup->repindex = repindex;
		lookup->trigger	  = lookup->reptime  = reptime;
		lookup->days_warn = warn;
	} else {
		lookup->index	  = lookup->regindex = regindex;
		lookup->trigger	  = lookup->regtime  = regtime;
	}
	lookup->list = list;
	return(lookup->index >= 0 && lookup->index < list->nentries);
}


static int lookup_regular_entry(
	struct plist	*list,		/* list of entries */
	time_t		time,		/* time to locate */
	BOOL		exact,		/* need entry, not insert pt */
	BOOL		norep)		/* treat repeaters as regular*/
{
	struct entry	*entry;		/* entry array */
	int		lo, hi, mid;	/* binary search indices */

	entry = list->entry;
	lo = 0;
	hi = list->nentries;
	while (lo < hi) {
		mid = lo + (hi - lo) / 2;
		if (mid == list->nentries || time > entry[mid].time)
			lo = mid+1;
		else
			hi = mid;
	}
	if (exact)
		while (lo && time == entry[lo-1].time)
			lo--;
	else
		while (lo < list->nentries && time == entry[lo].time)
			lo++;
	if (!norep)
		while (lo < list->nentries && (entry[lo].rep_every    ||
					       entry[lo].rep_yearly   ||
					       entry[lo].rep_weekdays ||
					       entry[lo].rep_days))
			lo++;
	return(lo < list->nentries ? lo : -1);
}


static int lookup_repeating_entry(
	struct plist	*list,		/* list of entries */
	time_t		time,		/* time to locate */
	BOOL		exact,		/* need entry, not insert pt */
	time_t		*reptime,	/* returned repeat time */
	char		which,		/* 'mywd': month,yr,week,day */
	int		usr,		/* user number or -1 */
	BOOL		*warn)		/* found a n-days-ahead warn?*/
{
	struct entry	*entry;		/* entry array */
	int		n, i;		/* next repeating entry */
	time_t		earliest = 0;	/* earliest future entry yet */
	int		earliest_n= -1;	/* index of that entry */
	BOOL		earliest_w = 0;	/* is it a n-days-ahead warn?*/
	int		prev_n = -1;	/* entry before n */
	int		earliest_prev_n=-1;/* entry before earliest_n*/
	time_t		test;		/* next repeater trigger */

	exact = exact != FALSE;
	for (n=list->repeating; n != -1; prev_n=n, n=entry->nextrep) {
		entry = &list->entry[n];
		if (which != '*') {
			int u = name_to_user(entry->user);
			if ((which == 'm' && user[u].suspend_m) ||
			    (which == 'w' && user[u].suspend_w) ||
			    (which == 'o' && user[u].suspend_o) ||
			    (which == 'd' && user[u].suspend_d))
				continue;
		}
		if ((which == 'm' && entry->hide_in_m) ||
		    (which == 'y' && entry->hide_in_y) ||
		    (which == 'w' && entry->hide_in_w) ||
		    (which == 'o' && entry->hide_in_o) ||
		    (which == 'd' && entry->hide_in_d))
			continue;

		if (usr >= 0 && ((usr && !entry->user) ||
				 strcmp(user[usr].name, entry->user)))
			continue;

		i = recycle(entry, time, &test);
		if (i == 1 || i == 3)
			if (!earliest || test + exact < earliest) {
				earliest   = test;
				earliest_n = n;
				earliest_w = i == 3;
				earliest_prev_n = prev_n;
			}
	}
	if (earliest_n >= 0 && earliest_prev_n >= 0) {
		list->entry[earliest_prev_n].nextrep =
					list->entry[earliest_n].nextrep;
		list->entry[earliest_n].nextrep = list->repeating;
		list->repeating = earliest_n;
	}
	*reptime = earliest;
	*warn	 = earliest_w;
	return(earliest_n);
}


/*
 * given an entry, find the entry that triggers next. If there is none,
 * return FALSE.
 * The trigger time of the returned entry is stored in <lookup.trigger>.
 * Never call this when the previous call or lookup_entry() returned FALSE.
 *
 * First, if the previously returned entry was repeating, go to the next
 * repeating entry; if the previously returned entry was regular, go to
 * the next regular entry. Then pick the earlier of the two.
 */

BOOL lookup_next_entry(
	struct lookup	*lookup)	/* previously found entry */
{
	struct entry	*entry;		/* entry array */
	int		n, i;		/* next repeating entry */
	int		repindex = -1, regindex = -1;
	time_t		reptime, regtime = 0;
	struct plist	*list = lookup->list;
	time_t		time = 0;
	int		prev_n, earliest_prev_n = -1;
	time_t		earliest = 0;
	int		earliest_n = -1;
	BOOL		earliest_w = 0;
	int		u;

							/* next repeating */
	prev_n = lookup->repindex >= 0 ? lookup->repindex
				       : list->repeating;
	n = prev_n == -1 ? -1 : list->entry[prev_n].nextrep;
	if (lookup->repindex >= 0) {
		prev_n = lookup->repindex;
		n      = list->entry[prev_n].nextrep;
	} else {
		prev_n = -1;
		n      = list->repeating;
	}
	for (; n != -1; prev_n=n, n=entry->nextrep) {
		entry = &list->entry[n];
		u = name_to_user(entry->user);
		if ( lookup->which != '*' &&
		   ((lookup->which == 'm' && user[u].suspend_m) ||
		    (lookup->which == 'w' && user[u].suspend_w) ||
		    (lookup->which == 'o' && user[u].suspend_o) ||
		    (lookup->which == 'd' && user[u].suspend_d)))
			continue;

		if ((lookup->which == 'm' && entry->hide_in_m) ||
		    (lookup->which == 'y' && entry->hide_in_y) ||
		    (lookup->which == 'w' && entry->hide_in_w) ||
		    (lookup->which == 'o' && entry->hide_in_o) ||
		    (lookup->which == 'd' && entry->hide_in_d))
			continue;

		if (lookup->user >= 0 && lookup->user != u)
			continue;

		i = recycle(entry, lookup->trigger, &time);
		if (i == 1 || i == 3)
			if (!earliest || time < earliest) {
				earliest   = time;
				earliest_n = n;
				earliest_w = i == 3;
				earliest_prev_n = prev_n;
			}
	}
	repindex = earliest_n;
	reptime  = earliest;
	entry = &list->entry[n = earliest_n];
	if (n >= 0 && earliest_prev_n >= 0) {
		if (earliest_prev_n == -1)
			list->repeating = entry->nextrep;
		else
			list->entry[earliest_prev_n].nextrep
					= entry->nextrep;
		if (lookup->repindex == -1) {
			entry->nextrep = list->repeating;
			list->repeating = n;
		} else {
			entry->nextrep =
				list->entry[lookup->repindex].nextrep;
			list->entry[lookup->repindex].nextrep = n;
		}
	}

							/* next regular */
	if (lookup->regindex < 0) {
		if ((regindex = lookup_regular_entry(list,
					lookup->trigger, TRUE, FALSE)) >= 0)
			regtime = list->entry[regindex].time;
	} else {
		n = lookup->regindex + 1;
		entry = &list->entry[n];
		for (; n < list->nentries; n++, entry++)
			if (!entry->rep_every && !entry->rep_weekdays &&
			    !entry->rep_days  && !entry->rep_yearly) {
				regindex = n;
				regtime  = entry->time;
				break;
			}
	}
							/* use rep or reg */
	if (repindex >= 0 && (regindex < 0 || reptime < regtime)) {
		lookup->index     = lookup->repindex = repindex;
		lookup->trigger   = lookup->reptime  = reptime;
		lookup->days_warn = earliest_w;
	} else {
		lookup->index     = lookup->regindex = regindex;
		lookup->trigger   = lookup->regtime  = regtime;
		lookup->days_warn = 0;
	}
	return(lookup->index >= 0 && lookup->index < list->nentries);
}


/*
 * scan main list, and return the next entry. The next entry is the one
 * triggers next for one of three reasons (returned value):
 *  0: no trigger for at least 24 hours after <time>,
 *  1: early_warn of *entryp is nonzero, and early warn time is reached,
 *  2: late_warn of *entryp is nonzero, and late warn time is reached,
 *  3: the alarm time of *entryp is reached.
 * Reaching the n-days-ahead warn time is the same as reaching the main
 * alarm time; the warning is treated like a repetition except that the
 * early and late warning times are ignored.
 *
 * The algorithm uses the fact that no warning can come more than 24 hours
 * before the trigger time. The first entry that is at most 24 hours early
 * is looked up, then the next 72 hours are searched linearly. Events that
 * <triggered> before are ignored. The caller must store the returned reason
 * in (*entryp)->triggered to avoid double triggers, iff the reason is > 0.
 * wait can be negative, up to -ANCIENT, to catch events that were missed.
 *
 * This routine is used by the daemon only. Ignore the lint complaint.
 */

int find_next_trigger(
	struct plist	*list,		/* list of entries */
	time_t		time,		/* time to locate */
	struct entry	**entryp,	/* ret: entry that was found */
	time_t		*wait_time)	/* ret: how long until triggr*/
{
	BOOL		found;		/* TRUE if lookup succeeded */
	struct lookup	lookup;		/* result of entry lookup */
	struct entry	*ep;		/* current entry */
	time_t		trigger;	/* next trigger time of *ep */
	time_t		dist, t;	/* time distance to event */
	time_t		mindist = MAXT;	/* smallest distance so far */
	int		i;		/* for checking exceptions */
	int		u;		/* user[] index */
	int		reason = 0;	/* why is it earliest: 1,2,3 */

	if (!list)
		return(0);

	/*
	t = time % 86400;
	t = t < 3*3600 ? time - t : time - 3*3600;
	*/
	t = cutoff;
	if (debug)
		printf(_("searching for next trigger from: %ld\n"),t);

	for (found = lookup_entry(&lookup, list, t, TRUE, FALSE, '*', -1);
	     found;
	     found = lookup_next_entry(&lookup)) {

		trigger = lookup.trigger;
		if (trigger > time+2*86400)
			break;
		ep = list->entry + lookup.index;
		if (trigger + ep->length < cutoff)
			continue;
		if (!ep->triggered && trigger > cutoff
				   && trigger < time
				   && trigger + ep->length > cutoff)
			ep->triggered = 2;
		for (i=0; i < NEXC; i++)
			if (ep->except[i] == trigger - trigger % 86400)
				break;
		u = name_to_user(ep->user);
		if (!ep->suspended && !ep->notime && i==NEXC && user[u].alarm
				   && (ep->triggered==2 || !lookup.days_warn))
			switch (ep->triggered) {
			  case 0:
				if (ep->early_warn &&
				    trigger - ep->early_warn > cutoff) {
					dist = trigger - ep->early_warn - time;
					if (dist > -ANCIENT && dist < mindist){
						mindist = dist;
						*entryp = ep;
						reason  = 1;
					}
				}
			  case 1:
				if (ep->late_warn &&
				    trigger - ep->late_warn > cutoff) {
					dist = trigger - ep->late_warn - time;
					if (dist > -ANCIENT && dist < mindist){
						mindist = dist;
						*entryp = ep;
						reason  = 2;
					}
				}
			  case 2:
				if (trigger > cutoff) {
					dist = trigger - time;
					if (dist < mindist) {
						mindist = dist;
						*entryp = ep;
						reason  = 3;
					}
				}
			}
	}
	*wait_time = mindist;
	return(reason);
}


/*----------------------------------- entry mgmt ----------------------------*/
/*
 * duplicate an entry. This is done when someone begins to edit an entry;
 * the old entry is preserved and stays in the list until the new entry is
 * confirmed (see confirm_new_entry()). If <old> is 0, the new entry is
 * zeroed.
 */

void clone_entry(
	struct entry	*new,		/* entry to fill in */
	struct entry	*old)		/* entry to clone, or 0 */
{
	if (!old) {					/* zero new entry */
		unsigned int i;
		for (i=0; i < sizeof(struct entry); i++)
			((char *)new)[i] = 0;
		return;
	}
	*new = *old;					/* copy old to new */
	if (old->user)
		new->user    = mystrdup(old->user);
	if (old->message)
		new->message = mystrdup(old->message);
	if (old->script)
		new->script  = mystrdup(old->script);
	if (old->note)
		new->note    = mystrdup(old->note);
}


/*
 * destroy an entry. De-allocate all memory hanging off it. Don't release
 * the entry itself, it's probably some static buffer.
 */

void destroy_entry(
	struct entry	*entry)		/* entry to destroy */
{
	if (entry->user)
		free(entry->user);
	if (entry->message)
		free(entry->message);
	if (entry->script)
		free(entry->script);
	if (entry->note)
		free(entry->note);
	clone_entry(entry, (struct entry *)0); /* paranoid */
}
