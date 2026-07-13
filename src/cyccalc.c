/*
 * Move the date of entries that have recycling information to the next
 * trigger date.
 *
 *	length_of_month(time)		Return the # of days in the month
 *					<time> [#secs since 1/1/70] is in
 *	recycle_all(list, del, days)	Calls recycle() for all entries
 *					in <list>, and optionally deletes
 *					obsolete (expired) entries.
 *	recycle(entry, date, ret)	If <entry> cycles, return its next
 *					trigger time after or on <date>,
 *					and find out if it is now obsolete.
 */

#include <stdio.h>
#include <time.h>
#include <Xm/Xm.h>
#include "cal.h"

extern struct config	config;		/* global configuration data */
extern struct user	*user;		/* user list (from file_r.c) */
extern short		monthlen[12];


/*
 * return the number of seconds in the month that <time> falls into. This
 * routine overwrites the static tm buffer.
 */

time_t length_of_month(
	time_t		time)		/* some date in the month */
{
	struct tm	*tm;		/* time to m/d/y conv */

	tm = time_to_tm(time);
	time -= (tm->tm_mday-1) * 86400;
	tm->tm_mday = 1;
	if (tm->tm_mon++ == 11) {
		tm->tm_mon = 0;
		tm->tm_year++;
	}
	return(tm_to_time(tm) - time);
}


/*
 * scan the entire list and remove obsolete entries, and recycle repeating
 * entries (provided they don't belong to another user and are read-only).
 * This is called regularly. Return TRUE if something changed. If TRUE is
 * returned, the caller must call draw_calendar() and update_all_listmenus().
 */

BOOL recycle_all(
	struct plist	*list,		/* list to scan */
	BOOL		del,		/* auto-remove obsolete? */
	int		days)		/* if del, keep if age < days*/
{
	struct entry	*ep;		/* current entry considered */
	int		n;		/* entry counter */
	int		u;		/* user[] index */
	time_t		now;		/* current time */
	time_t		time;		/* next trigger time */
	BOOL		chg = FALSE;	/* TRUE if list was changed */

	now = get_time();
	for (n=0; n < list->nentries; n++) {
		ep = &list->entry[n];
		u = name_to_user(ep->user);
		if (!user[u].readonly) {
			BOOL need_resort = move_todo_entry(ep, now);
			switch(recycle(ep, now, &time)) {
			  case 2:
				if (del && now - time > days * 86400) {
					server_entry_op(ep, 'd');
					delete_entry(list, n--);
					need_resort = FALSE;
					chg = TRUE;
				}
				break;

			  case 1:
				if (del && time > ep->time) {
					ep->time = time;
					need_resort = TRUE;
				}
				break;
			}
			if (need_resort) {
				resort_entry(list, n--);
				chg = TRUE;
			}
		}
	}
	if (chg)
		rebuild_repeat_chain(list);
	return(chg);
}


/*
 * If the entry has recycling info, move it up to the next date it triggers.
 * Searching for the next trigger date if multiple repeat restrictions are
 * set is very messy, I am using a trial-and-error approach for now.
 * Set *ret to the time the entry will trigger next after <date>, and return
 *
 *  0	if the entry didn't change and doesn't trigger,
 *  1	if the entry is triggering on or after <today>; trigger time is in ret,
 *  2	if the entry has become obsolete (24 hours after last trigger time),
 *  3	if the n-days-ahead warn time of the entry is triggering on or after
 *	date; trigger time is in *ret. Same as 1 but gray out bars in views.
 */

#define INC_TM(T) {							\
	mlen = monthlen[T.tm_mon] + (T.tm_mon==1 && !(T.tm_year&3));	\
	if (++T.tm_mday > mlen) {					\
		T.tm_mday = 1;						\
		if (++T.tm_mon == 12) {					\
			T.tm_mon = 0;					\
			T.tm_year++;					\
		}							\
	}}

int recycle(
	struct entry	*entry,		/* entry to test */
	time_t		today,		/* today (time is ignored) */
	time_t		*ret)		/* returned next trigger time*/
{
	struct tm	tm1;		/* first trigger m/d/y ever */
	struct tm	tm1w;		/* same for n-days-ahead warn*/
	struct tm	tm;		/* time to m/d/y h:m:s conv */
	struct tm	tmw;		/* same for n-days-ahead warn*/
	time_t		day;		/* <today> with no time info */
	long		tday;		/* entry date with no time */
	long		wday;		/* entry warn with no time */
	long		end;		/* don't search too far */
	long		every;		/* period in days */
	BOOL		simple;		/* no daily,monthly,yrly,warn*/
	int		mlen  = 0;	/* # of days in test month */
	int		mlenw = 0;	/* same for n-days-ahead warn*/
	time_t		warn;		/* n-days-ahead warn period */
	long		w = entry->rep_weekdays;
	long		d = entry->rep_days;
	time_t		last = entry->rep_last;

	memset(&tm1,  0, sizeof(tm1));
	memset(&tm1w, 0, sizeof(tm1w));
	memset(&tm,   0, sizeof(tm));
	memset(&tmw,  0, sizeof(tmw));

	*ret = entry->time;
	warn = entry->days_warn ? entry->time - entry->days_warn : 0;
	tday = *ret / 86400;
	wday = warn / 86400;
	day  = today / 86400;
	end  = today + 86400 * 32;
					/* have not reached warn or time? */
	if (day < tday && (!warn || day < wday))
		return(0);
					/* expired nonrepeating appt? */
	if (day > tday+1 && !entry->rep_yearly && !entry->rep_days
			 && !entry->rep_every  && !entry->rep_weekdays)
		return(2);
					/* have passed last-trigger date? */
	if (last && today > last + 86400-1)
		return(2);
					/* hit simple n-days-ahead warning? */
	if (warn && !w && !d && !entry->rep_yearly
		       && (!entry->rep_every || entry->rep_every == 86400)
		       && warn >= today && warn < today + 86400) {
		*ret = warn;
		return(3);
	}
					/* hit an every-nth-day repeat day? */
	if (day >= tday && entry->rep_every
			&& !((day - tday) % (entry->rep_every/86400))) {
		*ret %= 86400;
		*ret += today - today % 86400;
		return(last && *ret > last + 86400-1 ? 2 : 1);
	}
					/* search for weekdays and month days*/
	if (entry->rep_yearly) {
		tm1 = *time_to_tm(entry->time);
		if (warn)
			tm1w = *time_to_tm(entry->time - entry->days_warn);
	}
	simple = !entry->rep_yearly && !w && !d && !warn;
	every  = entry->rep_every && simple ? entry->rep_every / 86400 : 1;
	*ret  += ((day - tday + every-1) / every) * every * 86400;
	every *= 86400;
	if (!simple) {
		tm = *time_to_tm(*ret);
		if (warn)
			tmw = *time_to_tm(*ret + entry->days_warn);
	}
	while (*ret < end) {
					/* search passed end date: stop, fail*/
		if (last && *ret > last + 86400-1)
			return(2);
					/* yearly repeat, month/day match? */
		if (entry->rep_yearly) {
			if (day >= tday && tm.tm_mon  == tm1.tm_mon
					&& tm.tm_mday == tm1.tm_mday)
				return(1);
			if (warn &&
			    tm.tm_mon==tm1w.tm_mon && tm.tm_mday==tm1w.tm_mday)
				return(3);
		}
					/* search passed today: stop, fail */
		if (entry->rep_every && *ret > today + entry->rep_every +86400)
			return(0);
					/* prepare for weekly/monthly... */
		if (!simple) {
			if ((d & 1) || (w & 0x2000)) {
				mlen = length_of_month(*ret) / 86400;
				if (warn)
					mlenw = length_of_month(*ret -
						entry->days_warn) / 86400;
			}
					/* got a match of a weekly repeat? */
			if (day >= tday && (w & 1 << tm.tm_wday)) {
				if (!(w & 0x3f00))
					return(1);
				if ((w & 0x0100 << (tm.tm_mday-1) / 7) ||
				   ((w & 0x2000) && tm.tm_mday+7 > mlen))
					return(1);
			}
			if (warn && (w & 1 << tmw.tm_wday)) {
				if (!(w & 0x3f00))
					return(3);
				if ((w & 0x0100 << (tmw.tm_mday-1) / 7) ||
				   ((w & 0x2000) && tmw.tm_mday+7 > mlenw))
					return(3);
			}
					/* got a match of a monthly repeat? */
			if (day >= tday && ((d & 1 << tm.tm_mday) ||
					   ((d & 1) && tm.tm_mday == mlen)))
				return(1);
			if (warn && ((d & 1 << tmw.tm_mday) ||
				    ((d & 1) && tmw.tm_mday == mlenw)))
				return(3);
					/* no match, proceed with next day */
			INC_TM(tm);
			if (warn)
				INC_TM(tmw);
		}
		*ret += every;
	}
	return(*ret >= today && *ret < today + 86400 ? 1 : 0);
}


/*
 * if an entry is a todo entry, move either the end date (if it has one) or
 * the start date (otherwise) to today.
 */

BOOL move_todo_entry(
	struct entry	*entry,		/* entry to test */
	time_t		today)		/* today (time is ignored) */
{
	BOOL		ret = FALSE;

	if (entry->todo) {
		today -= today % 86400;
		if (entry->rep_last) {
			if ((ret = entry->rep_last < today))
				entry->rep_last = today;
		} else if ((ret = entry->time < today))
			entry->time = today + entry->time % 86400;
	}
	return(ret);
}
