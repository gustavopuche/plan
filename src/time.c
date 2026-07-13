/*
 * Convert date and time data formats.
 *
 *	tm_to_time(tm)		convert tm struct to seconds since 1/1/70
 *	time_to_tm(tp)		convert seconds since 1/1/70 to tm struct
 *	get_time()		return current time in seconds since 1/1/70
 *	set_tzone()		figure out current timezone (config.tzone)
 *	date_to_time(day, month, year, wkday, yday, weeknum)
 *				Returns day/month/year as # of secs since
 *				1/1/70, the weekday (0..6), the julian date
 *				(0..365), and the week #.
 */

#include <stdio.h>
#include <stdlib.h>
#if defined(MIPS) || defined(NEWSOS4)
#include <sys/types.h>
#endif
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <Xm/Xm.h>
#include <X11/StringDefs.h>
#include "cal.h"

#undef DEBUG			/* define this to debug timezone calc */

extern struct config	config;		/* global configuration data */

short monthlen[12]   = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
short monthbegin[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};


/*
 * convert a tm structure (d.m.y h:m:s) to a time in seconds since 1/1/70.
 * The difference to mktime() is that the result modulo 86400 is always
 * the time, regardless of timezones and daylight saving time. This makes
 * it much easier to process days with different DST status.
 * This routine only works correctly for the range 1970..2069.
 */

time_t tm_to_time(
	struct tm	*tm)		/* time/date to convert */
{
	time_t		t;		/* return value */

	t  = monthbegin[tm->tm_mon]			/* full months */
	   + tm->tm_mday-1				/* full days */
	   + (!(tm->tm_year & 3) && tm->tm_mon > 1);	/* leap day this year*/
	tm->tm_yday = t;
	t += 365 * (tm->tm_year - 70)			/* full years */
	   + (tm->tm_year - 69)/4;			/* past leap days */
	tm->tm_wday = (t + 4) % 7;

	t = t*86400 + tm->tm_hour*3600 + tm->tm_min*60 + tm->tm_sec;
	if (tm->tm_mday > monthlen[tm->tm_mon] +
			  (!(tm->tm_year & 3) && tm->tm_mon == 1))
		return((time_t)-1);
	return(t);
}


/*
 * convert time in seconds since 1/1/70 to a tm structure, and also fill in
 * the week number, julian date, and weekday. The difference to localtime()
 * is that timezones and daylight saving time are ignored.
 * This routine only works correctly for the range 1970..2069.
 */

struct tm *time_to_tm(
	time_t		t)		/* time to convert */
{
	int			n, l;		/* temp */
	static struct tm	tm;		/* returned tm */

	tm.tm_sec  = t % 60;		t /= 60;
	tm.tm_min  = t % 60;		t /= 60;
	tm.tm_hour = t % 24;		t /= 24;
	tm.tm_wday = (t + 4) % 7;

	n = t / (365+365+366+365);
	tm.tm_year = 70 + n*4;		t -= n * (365+365+366+365);
	if (t > 364)	tm.tm_year++,	t -= 365;
	if (t > 364)	tm.tm_year++,	t -= 365;
	if (t > 365)	tm.tm_year++,	t -= 366;
	tm.tm_yday = t;

	for (n=0; n < 11; n++) {
		l = monthlen[n] + (n==1 && !(tm.tm_year&3));
		if (t < l)
			break;
		t -= l;
	}
	tm.tm_mon  = n;
	tm.tm_mday = t+1;
	return(&tm);
}


/*
 * return current time in seconds since 1/1/70. The timezone is applied
 * such that the return value modulo 86400 is the number of seconds since
 * midnight. Do not use localtime etc here!
 */

time_t get_time(void)
{
	return(time(0) - config.tzone + config.adjust_time);
}


/*
 * return the current time zone as the number of seconds as difference
 * to GMT (UTC) in seconds. Since all the tricks with tm_isdst, gmtoff,
 * _timezone etc turned out to be nonportable, I am now parsing the TZ
 * environment variable directly. There is nothing standard about standard
 * libraries when it comes to time calculation!
 */

void set_tzone(void)
{
	struct config	*c = &config;
	time_t		now;			/* current local time */
	time_t		tod;			/* time-of-day, 0..86399 */
	struct tm	*tm;			/* julian date of <now> */
	BOOL		b, e;

	now = time(0) - c->raw_tzone;
	tod = now % 86400;
	tm = time_to_tm(now);
	switch(c->dst_flag) {
	  case 0:					/* dst on */
		c->tzone = c->raw_tzone - 3600;
		break;
	  case 1:					/* dst off */
		c->tzone = c->raw_tzone;
		break;
	  case 2:					/* auto dst */
		b = tm->tm_yday >  c->dst_begin ||
		   (tm->tm_yday == c->dst_begin && tod >= c->dst_begin_time);
		e = tm->tm_yday <  c->dst_end   ||
		   (tm->tm_yday == c->dst_end   && tod <  c->dst_end_time);
		if (c->dst_begin <= c->dst_end ? b && e : b || e)
			c->tzone = c->raw_tzone - 3600;
		else
			c->tzone = c->raw_tzone;
	  	break;
	  case 3:
		guess_tzone();
		c->tzone = c->raw_tzone;
		c->dst_flag = 3;
	}
}


/*
 * Figure out reasonable values for the config fields raw_tzone, dst_flag,
 * dst_begin, and dst_end. This is used for defaults if the .dayplan file
 * does not specify the adjustment, or when the user presses Guess in the
 * Adjust Time popup.
 *
 * DANGER DANGER DANGER - do not try to use system functions like gmtime to
 * get the timezone from the system beyond what is done here. There are WAY
 * too many systems out there where these functions are broken, and I will
 * not accept patches that try to use them because it's a support nightmare.
 * Believe me, I have been there (plan 1.0) and I won't do it again. -Thomas
 */

#define DST_BEGIN	64			/* default begin of DST */
#define DST_END		220			/* default end of DST */

#ifdef DEBUG
static char *zone_time_string();
#endif
static int get_zone_time();
static int julian_year_date();

void guess_tzone(void)
{
	char		*tz=0;			/* timezone string (TZ) */
	int		zone=0, dst= -3600;	/* difference from GMT */
	int		begin=0, begintime=0;	/* dst begin: julian day/time*/
	int		end=0,   endtime=0;	/* dst end: julian day/time */

	if (config.dst_flag != 3 && (((tz = getenv("PLAN_TZ")) && *tz) ||
				     ((tz = getenv("TZ"))      && *tz))
				 && strrchr("0123456789-",tz[3])) {
		do {
			if (!*tz++ || !*tz++ || !*tz++)		/* MET */
				break;
			zone = get_zone_time(&tz, 0);		/* time */
			dst  = zone - 3600;
			if (!*tz++ || !*tz++ || !*tz++)		/* MST */
				break;
			if (*tz>='A' && *tz<='Z') tz++;	     /* allow MESZ */
			dst = get_zone_time(&tz, dst);		/* time */
			if (*tz == ';') {			/* ;begin */
				tz++;
				while (*tz >= '0' && *tz <= '9')
					begin = begin*10 + *tz++ - '0';
				if (*tz == '/') {		/*    /time */
					tz++;
					begintime = get_zone_time(&tz, 0);
				}
				if (*tz++ != ',')		/* ,end */
					break;
				while (*tz >= '0' && *tz <= '9')
					end = end*10 + *tz++ - '0';
				if (*tz++ == '/')		/*    /time */
					endtime = get_zone_time(&tz, 0);
			} else if (*tz == ',') {		/* ,begin */
				tz++;
				if (*tz != 'M') break;
				begin = julian_year_date(&tz);
				if (*tz == '/') {
					tz++;			/*   /time */
					begintime = get_zone_time(&tz, 0);
				} else begintime= 2*3600;	/* at 2 AM */
				if (*tz == ',') {		/* ,end */
					tz++;
					if (*tz != 'M') break;
					end = julian_year_date(&tz);
					if (*tz == '/') {	/*   /time */
						tz++;
						endtime = get_zone_time(&tz,0);
					} else
						endtime = 2*3600;/* at 2 AM */
				}
			} else break;
		} while (0);
	} else {
		/*------------------------------------------------------------
		 * this code is largely untested and available only for a few
		 * platforms. If you have corrections or extensions for other
		 * platforms, implement it here and tell me (thomas@bitrot.de).
		 */
#if defined(SVR4) || defined(SOLARIS2) || defined(linux) || defined(sgi) || defined(OSF)
		time_t now = time(0);
		struct tm *tm = localtime(&now);
		time_t h = tm->tm_hour;
		struct tm *gmt = gmtime(&now);
		zone = gmt->tm_hour - h;
		if (zone < -13)
			zone += 24;
		if (zone > 10)
			zone -= 24;
		zone *= 3600;
#else
#if defined(bsdi) || defined(SUN) || defined(SVR4) || defined(convex)
		time_t now;
		struct tm *tm;
		tzset();
		now = time(0);
		tm = localtime(&now);
		zone = -tm->tm_gmtoff;
#else
#if defined(__FreeBSD__) /* by Stefan `Sec` Zehl <sec@42.org> */
		time_t now = time(0);
		struct tm *tm = localtime(&now);
		config.tzone = tm->tm_gmtoff;
		zone = - tm->tm_gmtoff + tm->tm_isdst * 3600;
#else
		static int once;
		if (config.dst_flag != 3 && !once++)
			create_error_popup(0, 0, "automatic timezone not suppo"
					"rted for this machine, choose Adjust "
					"Time from the Config pulldown\n");
		zone = 0;
#endif
#endif
#endif
		/*-----------------------------------------------------------*/
	}


#ifdef DEBUG
	if (tz = getenv("PLAN_TZ"))
		printf("PLAN_TZ env variable: %s\n", tz);
	if (tz = getenv("TZ"))
		printf("TZ env variable:      %s\n", tz);
	printf("standard timezone:    GMT%s\n", zone_time_string(zone));
	printf("DST timezone:         GMT%s\n", zone_time_string(dst));
	printf("DST start:            %s on julian day %d\n",
					zone_time_string(begintime)+1, begin);
	printf("DST end:              %s on julian day %d\n",
					zone_time_string(endtime)+1, end);
#endif
	if (!begin) {						/* defaults */
		begin     = DST_BEGIN;
		begintime = 2*3600;
	}
	if (!end) {
		end       = DST_END;
		endtime   = 2*3600;
	}
	config.raw_tzone      = zone;
	config.dst_flag       = 2;
	config.dst_begin      = begin;
	config.dst_end        = end;
	config.dst_begin_time = begintime;
	config.dst_end_time   = endtime;
#ifdef DEBUG
	printf("current timezone:  GMT%s\n", zone_time_string(zone));
#endif
}


/*
 * parse a time string of the form [hours[:minutes[:seconds]]], return seconds
 */

static int get_zone_time(
	char		**tzp,
	int		zdefault)
{
	char		*tz = *tzp;
	int		i, num, zone = 0;
	int		sign = 1;

	if (*tz == '-') tz++, sign = -1;
	if (*tz == '+') tz++;
	for (i=0; i < 3; i++) {
		zone *= 60;
		num = 0;
		while (*tz >= '0' && *tz <= '9')
			num = num*10 + *tz++ - '0';
		zone += num;
		tz += *tz == ':';
	}
	zone *= sign;
	if (tz == *tzp)
		zone = zdefault;
	*tzp = tz;
	return(zone);
}


/*
 * parse a date in format 'Mmm.ww.dd', where
 * ww is a week number, mm a month number, and dd a day code starting from
 * Sunday (0); week 5 denotes always the last week of a month.
 * (leading zeros may be omitted in all cases)
 *
 * The dates may optionally be followed by a time designation after a slash.
 * The default for the time is 2 AM.
 *
 * interface: the scanning pointer tzp must be incremented
 *
 * This routine by Klaus Guntermann <guntermann@iti.informatik.th-darmstadt.de>
 */

extern int	curr_year;		/* year being displayed, since 1900 */
time_t		date_to_time();

static int julian_year_date(
	char		**tzp)
{
	char		*tz = *tzp;
	int		day, month, week;
	int		wkday, julian;

	if (*tz == 'M') {	/* scan the format */
		tz++;
		month = 0;
		while (*tz >= '0' && *tz <= '9')
                        month = month*10 + *tz++ - '0';
		if (*tz != '.') {
			*tzp = tz;
			return(0);
		}
		tz++;
		week = 0;
		while (*tz >= '0' && *tz <= '9')
                        week = week*10 + *tz++ - '0';
		if (*tz != '.') {
			*tzp = tz;
			return(0);
		}
		tz++;
		day=0;
		while (*tz >= '0' && *tz <= '9')
                        day = day*10 + *tz++ - '0';

		/* compute the corresponding Julian date in this year */
		if (week<5) {
			/* did not ask for a day in the last week of a month */
			(void)date_to_time(1, month-1, curr_year,
						&wkday, &julian, NULL);
			julian += 1+day-wkday;	/* advance to proper day */
			julian += (week - (wkday <= day ? 1 : 0)) * 7;
			/* and correct for the number of weeks wanted */
		} else {
			/* compute backwards from the 1st day of the next month,
			   which may be the first month of the next year */
			(void)date_to_time(1, (month < 11 ? month : 0),
					  (month<11 ? curr_year : curr_year+1),
						&wkday, &julian, NULL);
			/* now go back to the proper day... */
			julian += 1 + day - wkday - (day >= wkday ? 7 : 0);
		}
	}
	*tzp = tz;
	return(julian);
}

/*
 * return a number of seconds as a string "+/-hours:minutes:seconds"
 */

#ifdef DEBUG
static char *zone_time_string(
	int		t)
{
	static char buf[12];

	t %= 86400;
	buf[0] = t < 0 ? '-' : '+';
	if (t < 0)
		t = -t;
	sprintf(buf+1, "%02d:%02d:%02d", t/3600, (t/60)%60, t%60);
	return(buf);
}
#endif


/*
 * day/month/year to time. Return 0 if something is wrong.
 * Also return the weekday, julian date, and week number of that date.
 * Note that *wkday counts from 0=sunday to 6=saturday.
 */

time_t date_to_time(
	int		day,
	int		month,
	int		year,
	int		*wkday,
	int		*julian,
	int		*weeknum)
{
	struct tm	tm;
	time_t		time;

	tm.tm_sec   = 0;
	tm.tm_min   = 0;
	tm.tm_hour  = 0;
	tm.tm_mday  = day;
	tm.tm_mon   = month;
	tm.tm_year  = year;
	time = tm_to_time(&tm);
	if (wkday)
		*wkday   = tm.tm_wday;
	if (julian)
		*julian  = tm.tm_yday;
	if (weeknum) {
		int x = (tm.tm_wday + 700 - tm.tm_yday) % 7;
		*weeknum = config.thu_week
			 ? (tm.tm_yday + x - (x<5 ? 1 : 8)) / 7
			 : config.fullweek
				? tm.tm_yday / 7
				: tm.tm_yday > 0 ? (tm.tm_yday-1) / 7 + 1 : 0;
	}
	return(time == -1 || day != tm.tm_mday ? 0 : time);
}
