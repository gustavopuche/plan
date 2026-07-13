/*
 * Convert date and time data formats.
 *
 *	mkdatestring(time)		Convert # of seconds since 1/1/70
 *					to a date string
 *	mktimestring(time, dur)		Convert # of seconds since 1/1/70
 *					to a time-of-day or duration string
 *	parse_datestring(text,dtime)	Interpret the date string, and
 *					return the # of seconds since 1/1/70
 *	parse_timestring(text,dur)	Interpret the time or duration string,
 *					and return the # of seconds since 0:00
 *	parse_datetimestring(text)	Interpret the date + time string,
 *					and return the # of seconds since 0:00
 *	parse_warnstring(text,e,l,n)	return early and late warning time,
 *					and the noalarm flag
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include "cal.h"

extern struct config	config;		/* global configuration data */

#ifdef FRENCH
char *weekday_name[7] =     { "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"};
char *alt_weekday_name[7] = { "Lu", "Ma", "Me", "Je", "Ve", "Sa", "Di" };
char *monthname[12] =       { "Janvier", "Février", "Mars", "Avril", "Mai", "Juin",
				"Juillet", "Août", "Septembre", "Octobre", "Novembre", "Décembre"};
#elif GERMAN
char *weekday_name[7] =     { "Mon", "Die", "Mit", "Don", "Fre", "Sam", "Son"};
char *alt_weekday_name[7] = { "Mo", "Di", "Mi", "Do", "Fr", "Sa", "So" };
char *monthname[12] =       { "Januar", "Februar", "März", "April", "Mai", "Juni",
				"Juli", "August", "September", "Oktober", "November", "Dezember"};
#else
char *weekday_name[7] =     { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
char *alt_weekday_name[7] = { "Mo", "Tu", "We", "Th", "Fr", "Sa", "Su" };
char *monthname[12] =       { "January", "February", "March", "April", "May", "June",
				"July", "August", "September", "October", "November", "December"};
#endif



/*
 * return some sensible string representation of a date (no time)
 */

char *mkdatestring(
	time_t		time)		/* date in seconds */
{
	static char	buf[40];	/* string representation */
	struct tm	*tm;		/* time to m/d/y conv */

	tm = time_to_tm(time);
	if (config.mmddyy)
		sprintf(buf, "%s, %d/%d/%02d",
					_(weekday_name[(tm->tm_wday+6)%7]),
					tm->tm_mon+1, tm->tm_mday,
					tm->tm_year % 100);
	else
		sprintf(buf, "%s, %d.%d.%02d",
					_(weekday_name[(tm->tm_wday+6)%7]),
					tm->tm_mday, tm->tm_mon+1,
					tm->tm_year % 100);
	return(buf);
}


/*
 * return some sensible string representation of a time (no date)
 */

char *mktimestring(
	time_t		time,		/* date in seconds */
	BOOL		dur)		/* duration, not time-of-day */
{
	static char	buf[40];	/* string representation */
	int		hour, min;	/* time components */

	if (!dur)
		time %= 86400;
	time /= 60;
	hour  = time / 60;
	min   = time % 60;
	if (config.ampm && !dur)
		sprintf(buf, "%2d:%02d%c", hour%12 ? hour%12 : 12,
					   min,
					   hour < 12 ? 'a' : 'p');
	else
		sprintf(buf, "%02d:%02d",   hour, min);
	return(buf);
}


/*
 * parse the date string, and return the number of seconds. The default
 * time argument is for the +x notation, it's typically the trigger date
 * of the appointment. Use 0 if it's not available; today is default.
 * Doesn't know about alpha month names yet.
 */

time_t parse_datestring(
	char		*text,		/* input string */
	time_t		dtime)		/* appt start time, 0=today */
{
	time_t		today;		/* current date in seconds */
	struct tm	*tm;		/* today's date */
	long		num[3];		/* m,d,y or d,m,y */
	int		nnum;		/* how many numbers in text */
	long		i;		/* tmp counter */
	char		*p;		/* text scan pointer */
	char		buf[10];	/* lowercase weekday name */
	int		noswap;

	today  = get_time();				/* today's date */
	today -= today % 86400;
	tm = time_to_tm(today);
	for (p=text; *p; p++)				/* ignore "wkday," */
		if (*p == ',') {
			text = p+1;
			break;
		}
	while (*text == ' ' || *text == '\t')		/* skip blanks */
		text++;
	for (p=text; *p; p++)				/* -> lowercase */
		if (*p >= 'A' && *p <= 'Z')
			*p += 'a' - 'A';
							/* today, tomorrow? */
	if (!strncmp(text, "tod", 3) ||
	    !strncmp(text, "heu", 3) ||
	    !strncmp(text, "auj", 3) || !*text)
		return(today);

	if (!strncmp(text, "tom", 3) ||
	    !strncmp(text, "mor", 3) ||
	    !strncmp(text, "dem", 3))
		return(today + 86400);

	if (!strncmp(text, "yes", 3) ||
	    !strncmp(text, "ges", 3) ||
	    !strncmp(text, "hie", 3))
		return(today - 86400);

	if (!strncmp(text, "ueb", 3))
		return(today + 2*86400);

	if (*text == '+')
		return((dtime ? dtime-dtime%86400 : today)
				+ atoi(text+1) * 86400);
	if (*text == '-')
		return((dtime ? dtime-dtime%86400 : today)
				- atoi(text+1) * 86400);
							/* weekday name? */
	for (i=0; i < 7; i++) {
		strcpy(buf, _(weekday_name[i]));
		*buf += 'a' - 'A';
		if (!strncmp(buf, text, strlen(buf)))
			break;
		strcpy(buf, alt_weekday_name[i]);
		*buf += 'a' - 'A';
		if (!strncmp(buf, text, strlen(buf)))
			break;
	}
	if (i < 7) {
		i = (i - tm->tm_wday + 8) % 7;
		return(today + i*86400);
	}
							/* d/m/y numbers? */
	num[0] = num[1] = num[2] = 0;
	p = text;
	for (nnum=0; nnum < 3; nnum++) {
		if (!*p || *p == ' ' || *p == '\t')
			break;
		while (*p >= '0' && *p <= '9')
			num[nnum] = num[nnum]*10 + *p++ - '0';
		while (*p && !(*p >= '0' && *p <= '9') && *p!=' ' && *p!='\t')
			p++;
	}
	if (nnum == 0)					/* ... no numbers */
		return(today);
	noswap = FALSE;
	if (nnum == 3) {				/* ... have year? */
		if (num[0] > 999) {			/*     (yyyy/mm/dd) */
			i = num[0];
			num[0] = num[2];
			num[2] = i;
			noswap = TRUE;
		}
		if (num[2] < 70)
			num[2] += 100;
		else if (num[2] > 100)
			num[2] -= 1900;
		if (num[2] < 70 || num[2] > 137)
			num[2]  = tm->tm_year;
		tm->tm_year = num[2];
	}
	if (nnum == 1) {				/* ... day only */
		if (num[0] < tm->tm_mday)
			if (++tm->tm_mon == 12) {
				tm->tm_mon = 0;
				tm->tm_year++;
			}
		tm->tm_mday = num[0];
	} else {					/* ... d/m or m/d */
		if (config.mmddyy && !noswap) {
			i      = num[0];
			num[0] = num[1];
			num[1] = i;
		}
		if (nnum < 3 && num[1]*100+num[0] <
						(tm->tm_mon+1)*100+tm->tm_mday)
			tm->tm_year++;
		tm->tm_mday = num[0];
		tm->tm_mon  = num[1]-1;
	}
	return(tm_to_time(tm));
}


/*
 * parse the time string, and return the number of seconds. If a time and not a
 * duration is expected, interpret "0"..."23" as 0:00...23:00, instead of 0:23
 */

time_t parse_timestring(
	char		*text,		/* input string */
	BOOL		duration)	/* time-of-day or duration? */
{
	long		num[3];		/* h,m,s */
	int		nnum;		/* how many numbers in text */
	int		ndigits;	/* digit counter */
	char		*p = text;	/* text pointer */
	int		i;		/* text index, backwards*/
	char		ampm = 0;	/* 0, 'a', or 'p' */
	char		sign = 0;	/* 0, '+', or '-' */
	int		h=0, m=0, s=0;	/* hours, minutes, seconds */

	while (*p == ' ' || *p == '\t')
		p++;
	if (!strncmp(p, "noon", 4))
		return(12*3600);
	if (!strncmp(p, "midn", 4))
		return(0);
	if (*p == '+' || *p == '-')
		sign = *p++;
	while (*p == ' ' || *p == '\t')
		p++;
	i = strlen(p)-1;
	while (i && (p[i] == ' ' || p[i] == '\t'))
		i--;
	if (i && p[i] == 'm')
		i--;
	if (i && (p[i] == 'a' || p[i] == 'p'))
		ampm = p[i--];
	while (i && (p[i] == ' ' || p[i] == '\t'))
		i--;
	num[0] = num[1] = num[2] = 0;
	for (nnum=0; i >= 0 && nnum < 3; nnum++) {
		ndigits = 0;
		while (i >= 0 && (p[i] < '0' || p[i] > '9'))
			i--;
		while (i >= 0 && p[i] >= '0' && p[i] <= '9' && ndigits++ < 2)
			num[nnum] += (p[i--] - '0') * (ndigits==1 ? 1 : 10);
	}
	if (ampm && nnum == 1) {
		nnum = 2;
		num[1] = num[0];
		num[0] = 0;
	}
	switch(nnum) {
	  case 0:	return(get_time() % 86400);
	  case 1:
		if (duration || num[0] >= 24) {
	  		h = 0;		m = num[0];	s = 0;
		} else {
	  		h = num[0];	m = 0;		s = 0;
		}							break;
	  case 2:	h = num[1];	m = num[0];	s = 0;		break;
	  case 3:	h = num[2];	m = num[1];	s = num[0];	break;
	}
	if (config.ampm) {
		if (ampm == 'a' && h == 12)
			h = 0;
		if (ampm == 'p' && h < 12)
			h += 12;
	}
	/* Input < 100 : input = nr of minutes (90 = 1h30) */
	/* Input > 100 : input = hhmm, max value for mm = 59 */
	if (h > 0 && m > 60)
		m = 59;
	s += h * 3600 + m * 60;
	return(((sign == '+' ? get_time() + s :
		 sign == '-' ? get_time() - s : s) + 86400) % 86400);
}


/*
 * parse string with both date and time
 */

time_t parse_datetimestring(
	char		*text)		/* input string */
{
	struct tm	tm1, tm2;	/* for adding date and time */
	time_t		t;		/* parsed time */

	while (*text == ' ' || *text == '\t')
		text++;
	t = parse_datestring(text, 0);
	tm1 = *time_to_tm(t);
	while (*text && *text != ' ' && *text != '\t')
		text++;
	while (*text == ' ' || *text == '\t')
		text++;
	t = parse_timestring(text, FALSE);
	tm2 = *time_to_tm(t);
	tm1.tm_hour = tm2.tm_hour;
	tm1.tm_min  = tm2.tm_min;
	tm1.tm_sec  = tm2.tm_sec;
	return(tm_to_time(&tm1));
}


/*
 * parse the fast advance-warning string. This mode can be enabled with the
 * config pulldown, and allows numeric entries like "<early>,<late>,-" that
 * replace the usual popup. Not as pretty, but faster.
 */

void parse_warnstring(
	char		*text,		/* input string */
	time_t		*early,		/* early warning in minutes */
	time_t		*late,		/* late warning in minutes */
	BOOL		*noalarm)	/* final alarm too? */
{
	long		num[2], tmp;	/* early, late, swap temp */
	int		nnum;		/* how many numbers in text */
	char		*p;		/* text scan pointer */

	if (*text == '=') {
		*late    = config.late_time;
		*early   = config.early_time;
		*noalarm = FALSE;
		return;
	}
	num[0] = num[1] = 0;
	p = text;
	while (*p == ' ' || *p == '\t')
		p++;
	for (nnum=0; nnum < 2; nnum++) {
		while (*p >= '0' && *p <= '9')
			num[nnum] = num[nnum]*10 + *p++ - '0';
		while (*p && !strchr("0123456789-", *p))
			p++;
	}
	*noalarm = *p == '-';
	if (nnum > 1 && num[0] && num[0] < num[1]) {
		tmp    = num[0];
		num[0] = num[1];
		num[1] = tmp;
	}
	*late  = num[1] * 60;
	*early = num[0] * 60;
}
