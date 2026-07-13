/*
 * deals with the holiday file. A yacc parser is used to parse the file.
 * All the holidays of the specified year are calculated at once and stored
 * in two arrays that have one entry for each day of the year. The day
 * drawing routines just use the julian date to index into these arrays.
 * There are two arrays because holidays can be printed either on a full
 * line under the day number, or as a small line to the right of the day
 * number. It's convenient to have both.
 *
 *	reset_all_hdays()		reset holidays, for "reset" keyword
 *	parse_holidays(year, force)	read the holiday file and evaluate
 *					all the holiday definitions for
 *					<year>. Sets holiday and sm_holiday
 *					arrays. If force is set, re-eval even
 *					if year is the same as last time.
 *	dump_holiday(year)		print holiday list for a year (webplan)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#if !defined NEWSOS4 && !defined STDLIBMALLOC && !defined MACOSX
#include <malloc.h>
#endif
#ifdef CPP_PATH
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <Xm/Xm.h>
#include "cal.h"


/*
 * Before you mail and complain that the following macro is incorrect,
 * please consider that this is one of the main battlegrounds of the
 * Annual Usenet Flame Wars. 2000 is a leap year. Just trust me on this :-)
 */

#define LEAPYEAR(y)	!((y)&3)
#define JULIAN(m,d)	(monthbegin[m] + (d)-1+((m)>1 && LEAPYEAR(parse_year)))
#define LAST		999
#define ANY		0
#define	BEFORE		-1
#define AFTER		-2

static void setliteraldate(int, int, int, int *);
static int calc_easter(int);
static int calc_pascha(int);
extern int yyparse(void);


#if defined(bsdi)||defined(linux)||defined(__NetBSD__)||defined(__FreeBSD__)||defined(HPGCC)||defined(__EMX__)||defined(__OpenBSD__)||defined(MACOSX)
extern int yylineno;
#else
extern int	 yylineno;		/* current line # being parsed */
#endif
extern char	*yytext;		/* current token being parsed */
extern FILE	*yyin;			/* the file the parser reads from */
extern BOOL	 yacc_small;		/* small string or on its own line? */
extern int	 yacc_stringcolor;	/* color of holiday name text, 1..8 */
extern char	*yacc_string;		/* holiday name text */
extern int	 yacc_daycolor;		/* color of day number, 1..8 */
extern char	*progname;		/* argv[0] */
int		 parse_year = -1;	/* year being parsed, 0=1970..99=2069*/
static char	*filename;		/* holiday filename */
static char	 errormsg[200];		/* error message if any, or "" */
static int	 easter_julian;		/* julian date of Easter Sunday */
static int	 pascha_julian;		/* julian date of Pascha Sunday */
static char	*holiday_name;		/* strdup'd yacc_string */

struct holiday	 holiday[366];		/* info for each day, separate for */
struct holiday	 sm_holiday[366];	/* full-line texts under, and small */
					/* texts next to day number */
extern short	monthlen[12];
extern short	monthbegin[12];


int yyerror(char *msg)
{
	fprintf(stderr, _("%s: %s in line %d of %s\n"), progname,
					msg, yylineno+1, filename);
	if (!*errormsg)
		sprintf(errormsg,
		      _("Problem with holiday file %s:\n%.80s in line %d"),
					filename, msg, yylineno+1);
	return(0);
}


/*
 * clear holidays. This is done before parsing starts, or if a "reset"
 * command is found in a holiday file. This lets users override system
 * holidays because system holidays are read first and can be reset.
 */

void reset_holidays(
	struct holiday	*hp)
{
	int		d;

	for (d=0; d < 366; d++, hp++)
		if (hp->string) {
			if (!hp->dup)
				free(hp->string);
			hp->string	= 0;
			hp->stringcolor	= 0;
			hp->daycolor	= 0;
			hp->dup		= FALSE;
		}
}


static BOOL did_reset;			/* if .holiday resets, skip sys file */

void reset_all_hdays(void)		/* this is for parser.y only */
{
	did_reset = TRUE;
	reset_holidays(holiday);
	reset_holidays(sm_holiday);
}


/*
 * parse the holiday text file, and set up the holiday arrays for a year.
 * First read the system file (HOLIDAY_PATH), the the personal file; the
 * latter may use the "reset" keyword to get rid of all previous settings.
 * If year is -1, re-parse the last year parsed (this is used when the
 * holiday file changes). If there is a CPP_PATH, check if the executable
 * really exists, and if so, pipe the holioday files through it.
 * Return an error message if an error occurred, 0 otherwise.
 */

char *parse_holidays(
	int		year,		/* year to parse for, 0=1970 */
	BOOL		force)		/* file has changed, re-read */
{
	int		n;
	BOOL		piped = FALSE;
	char		buf[200];
#ifdef CPP_PATH
	char		*p, cpp[200];
#endif

	if (year == parse_year && !force)
		return(0);
	if (year < 0)
		year = parse_year;
	parse_year = year;
	easter_julian = calc_easter(year + 1900);
	pascha_julian = calc_pascha(year + 1900);

	reset_holidays(holiday);
	reset_holidays(sm_holiday);

	did_reset = FALSE;
	for (n=0; n < 2; n++) {
		sprintf(buf, "%s/%s", LIB, HOLIDAY_NAME);
		filename = resolve_tilde(n ? buf : HOLIDAY_PATH);
		if (access(filename, R_OK))
			continue;
#ifdef CPP_PATH
		strncpy(cpp, CPP_PATH, sizeof(cpp));
		cpp[sizeof(cpp)-1] = 0;
		if ((p = strchr(cpp, ' ')))
			*p = 0;
		if ((piped = !access(cpp, X_OK))) {
			char cmd[200];
			struct stat status;
			if (!stat(filename, &status) && !status.st_size)
				continue;
			sprintf(cmd, "%s %s", CPP_PATH, filename);
			yyin = popen(cmd, "r");
		} else
#endif
			yyin = fopen(filename, "r");
		if (!yyin)
			continue;
		*errormsg = 0;
		yylineno = 0;
		yyparse();
		if (piped)
			pclose(yyin);
		else
			fclose(yyin);
		if (*errormsg)
			return(errormsg);
		if (did_reset)		/* found reset command in ~/.holiday,*/
			break;		/* don't read system-wide holidays */
	}
	return(0);
}


/*--------------------- yacc callbacks --------------------------------------*/
/*
 * set holiday by weekday (monday..sunday). The expression is
 * "every <num>-th <wday> of <month> plus <off> days". num and month
 * can be ANY or LAST.
 */

void setwday(
	int		num,		/* which, 1st..5th, ANY, or LAST */
	int		wday,		/* 1=monday..7=sunday, -1=workday */
	int		month,		/* month, 1..12, ANY, or LAST */
	int		off,		/* offset in days */
	int		length)		/* length in days */
{
	int		min_month = 0, max_month = 11;
	int		min_num   = 0, max_num   = 4;
	int		m, n, d, l, mlen, wday1;
	int		dup = 0;

	if (month != ANY)
		min_month = max_month = month-1;
	if (month == LAST)
		min_month = max_month = 11;
	if (num != ANY)
		min_num = max_num = num-1;

	holiday_name = yacc_string;
	for (m=min_month; m <= max_month; m++) {
		(void)date_to_time(1, m, parse_year, &wday1, 0, 0);
		d = (wday-1 - (wday1-1) +7) % 7 + 1;
		mlen = monthlen[m] + (m==1 && LEAPYEAR(parse_year));
		if (num == LAST)
			for (l=0; l < length; l++)
				setliteraldate(m, d+28<=mlen ? d+28 : d+21,
								off+l, &dup);
		else
			for (d+=min_num*7, n=min_num; n <= max_num; n++, d+=7)
				if (d >= 1 && d <= mlen)
					for (l=0; l < length; l++)
						setliteraldate(m,d,off+l,&dup);
	}
}


/*
 * set holiday by weekday (monday..sunday) date offset. The expression is
 * "every <wday> before/after <date> plus <off> days". 
 * (This routine contributed by Peter Littlefield <plittle@sofkin.ca>)
 */

void setdoff(
	int		wday,		/* 1=monday..7=sunday */
	int		rel,		/* -1=before, -2=after */
	int		month,		/* month, 1..12, ANY, or LAST */
	int		day,		/* 1..31, ANY, or LAST */
	int		year,		/* 1..2069, or ANY */
	int		off,		/* offset in days */
	int		length)		/* length in days */
{
	int		min_month = 0, max_month = 11;
	int		min_day   = 1, max_day   = 31;
	int		m, d, nd, l, wday1;
	int		dup = 0;

	if (year != ANY) {
		year %= 100;
		if (year < 70) year += 100;
		if (year != parse_year)
			return;
	}
	if (month != ANY)
		min_month = max_month = month-1;
	if (month == LAST)
		min_month = max_month = 11;
	if (day != ANY)
		min_day   = max_day   = day;

	holiday_name = yacc_string;
	for (m=min_month; m <= max_month; m++)
		if (day == LAST) {
			(void)date_to_time(monthlen[m], m, parse_year,
								&wday1, 0, 0);
			if (wday < 0) {
				if (wday1 > 0 && wday1 < 6)
					wday = wday1;
				else
					wday = (rel == BEFORE) ? 5 : 1;
			}
			nd = (((wday - wday1 + 7) % 7) -
						((rel == BEFORE) ? 7 : 0)) % 7;
			for (l=0; l < length; l++)
				setliteraldate(m,monthlen[m]+nd, off+l, &dup);
		} else
			for (d=min_day; d <= max_day; d++) {
				(void)date_to_time(d, m, parse_year,
								&wday1, 0, 0);
				if (wday < 0) {
					if (wday1 > 0 && wday1 < 6)
						wday = wday1;
					else
						wday = (rel == BEFORE) ? 5 : 1;
				}
				nd = (((wday - wday1 + 7) % 7) -
						((rel == BEFORE) ? 7 : 0)) % 7;
				for (l=0; l < length; l++)
					setliteraldate(m, d+nd, off+l, &dup);
			}
}


/*
 * set holiday by date. Ignore holidays in the wrong year. The code is
 * complicated by expressions such as "any/last/any" (every last day of
 * the month).
 */

void setdate(
	int		month,		/* 1..12, ANY, or LAST */
	int		day,		/* 1..31, ANY, or LAST */
	int		year,		/* 1..2069, or ANY */
	int		off,		/* offset in days */
	int		length)		/* length in days */
{
	int		min_month = 0, max_month = 11;
	int		min_day   = 1, max_day   = 31;
	int		m, d, l;
	int		dup = 0;

	if (year != ANY) {
		year %= 100;
		if (year < 70) year += 100;
		if (year != parse_year)
			return;
	}
	if (month != ANY)
		min_month = max_month = month-1;
	if (month == LAST)
		min_month = max_month = 11;
	if (day != ANY)
		min_day   = max_day   = day;

	holiday_name = yacc_string;
	for (m=min_month; m <= max_month; m++)
		if (day == LAST)
			for (l=0; l < length; l++)
				setliteraldate(m, monthlen[m], off+l, &dup);
		else
			for (d=min_day; d <= max_day; d++)
				for (l=0; l < length; l++)
					setliteraldate(m, d, off+l, &dup);
}


/*
 * After the two routines above have removed ambiguities (ANY) and resolved
 * weekday specifications, this routine registers the holiday in the holiday
 * array. There are two of these, for full-line holidays (they take away one
 * appointment line in the month calendar daybox) and "small" holidays, which
 * appear next to the day number. If the day is already some other holiday,
 * ignore the new one. <dup> is information stored for parse_holidays(), it
 * will free() the holiday name only if its dup field is 0 (because many
 * string fields can point to the same string, which was allocated only once
 * by the lexer, and should therefore only be freed once).
 */

static int colormap[10] = {
	0, COL_HBLACK, COL_HRED, COL_HGREEN, COL_HYELLOW,
	COL_HBLUE, COL_HMAGENTA, COL_HCYAN, COL_HWHITE, COL_WEEKEND };

static void setliteraldate(
	int		month,		/* 0..11 */
	int		day,		/* 1..31 */
	int		off,		/* offset in days */
	int		*dup)		/* flag for later free() */
{
	int julian = JULIAN(month, day) + off;
	struct holiday *hp = yacc_small ? &sm_holiday[julian]
					: &holiday[julian];

	if (julian >= 0 && julian <= 365 && !hp->string) {
		if (!*dup)
			holiday_name = mystrdup(holiday_name);
		hp->string	= holiday_name;
		hp->stringcolor	= colormap[yacc_stringcolor];
		hp->daycolor	= colormap[yacc_daycolor];
		hp->dup		= (*dup)++;
	}
}


/*
 * set a holiday relative to Easter
 */

void seteaster(
	int		off,		/* offset in days */
	int		length,		/* length in days */
	int		pascha)		/* 0=Easter, 1=Christian Orthodox */
{
	int		dup = 0;	/* flag for later free() */
	int julian = (pascha ? pascha_julian : easter_julian) + off;
	struct holiday *hp = yacc_small ? &sm_holiday[julian]
					: &holiday[julian];

	holiday_name = yacc_string;
	while (length-- > 0) {
		if (julian >= 0 && julian <= 365 && !hp->string) {
			if (!dup)
				holiday_name = mystrdup(holiday_name);
			hp->string	= holiday_name;
			hp->stringcolor	= colormap[yacc_stringcolor];
			hp->daycolor	= colormap[yacc_daycolor];
			hp->dup		= dup++;
		}
		julian++, hp++;
	}
}


/*
 * calculate Easter Sunday as a julian date. I got this from Armin Liebl
 * <liebla@informatik.tu-muenchen.de>, who got it from Knuth. I hope I got
 * all this right...
 */

static int calc_easter(
	int		year)		/* Easter in which year? */
{
	int golden, cent, grcor, clcor, extra, epact, easter;

	golden = (year/19)*(-19);
	golden += year+1;
	cent = year/100+1;
	grcor = (cent*3)/(-4)+12;
	clcor = ((cent-18)/(-25)+cent-16)/3;
	extra = (year*5)/4+grcor-10;
	epact = golden*11+20+clcor+grcor;
	epact += (epact/30)*(-30);
	if (epact<=0)
		epact += 30;
	if (epact==25) {
		if (golden>11)
			epact += 1;
	} else {
		if (epact==24)
			epact += 1;
	}
	easter = epact*(-1)+44;
	if (easter<21)
		easter += 30;
	extra += easter;
	extra += (extra/7)*(-7);
	extra *= -1;
	easter += extra+7;
	easter += 31+28+!(year&3)-1;
	return(easter);
}


/*
 * set a holiday relative to Pascha, which is the Christian Orthodox Easter.
 * Algorithm provided by Efthimios Mavrogeorgiadis <emav@enl.auth.gr>.
 * Changed 12.9.99 by Efthimios Mavrogeorgiadis <emav@enl.auth.gr>.
 */

static int calc_pascha(
	int		year)		/* Pascha in which year? */
{
	int a = year % 19;
	int b = (19 * a + 15) % 30;
	int c = (year + (year - (year % 4))/4 + b) % 7;
	int d = b - c;
	int e = d-3 - (2 - (year-(year%100))/100 + (year-(year%400))/400);
	int f = (e - (e % 31))/31;
	int day = e - 30 * f;
	return(31 + 28+!(year&3) + 31 + (f ? 30 : 0) + day-1);
}


/*
 * functions used for [] syntax: (Erwin Hugo Achermann <acherman@inf.ethz.ch>)
 *
 * day_from_name (str)			gets day from symbolic name
 * day_from_easter ()			gets day as easter sunday
 * day_from_monthday (m, d)		gets <day> from <month/day>
 * day_from_wday (day, wday, num)	gets num-th day (wday) after <day> day
 * monthday_from_day (day, *m, *d, *y)	gets month/day/cur_year from <day>
 */

int day_from_name(
	char	*str)
{
	int	i;
	char	*name;

	for (i=0; i < 366; i++) {
		name = holiday[i].string;
		if (name && !strcmp(str, name))
			return(i);
	}
	return(-1);
}


int day_from_easter(void)
{
	return(easter_julian);
}


int day_from_monthday(
	int	m,
	int	d)
{
	if (m == 13)
		return(365 + LEAPYEAR(parse_year));
	return(JULIAN(m - 1, d));
}


void monthday_from_day(
	int	day,
	int	*m,
	int	*d,
	int	*y)
{
	int	i, len;

	*y = parse_year;
	*m = 0;
	*d = 0;
	if (day < 0)
		return;
	for (i=0; i < 12; i++) {
		len = monthlen[i] + (i == 1 && LEAPYEAR(parse_year));
		if (day < len) {
			*m = i + 1;
			*d = day + 1;
			break;
		}
		day -= len;
	}
}


int day_from_wday(
	int	day,
	int	wday,
	int	num)
{
	int	wkday, yday, weeknum;

	(void)date_to_time(1, 0, parse_year, &wkday, &yday, &weeknum);
	day += (wday - wkday - day + 1001) % 7;
	day += num * 7;
	return (day);
}


/*
 * This function implements the -H option. Its main purpose is for the web
 * interface. Written by Michel Bourget, extended by thomas@bitrot.de.
 */

static char *julian_to_date(
	int	julian,
	int	year)
{
	static char str[32];
	int 	total, i, d, m;
	int	nbdays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	total=0;
	for (i=1; i <= 12; i++) {
		if (i==2 && (year%4) == 0)
			nbdays[i-1] = 29;
		total += nbdays[i-1];
		if (total < julian)
			continue;
		m = i;
		d = julian - total + nbdays[i-1];
		sprintf(str, "%d.%d.%d", d, m, year+1900);
		return(str);
	}
	return("ERROR");
}


void dump_holiday(
	int	year)
{
	int	i;

	parse_holidays(year -= 1900, TRUE);
	for (i=0; i < 366; i++) {
		if (holiday[i].string)
			printf("%d;%s;%s\n", i+1, julian_to_date(i+1, year),
						holiday[i].string);
		if (sm_holiday[i].string)
			printf("%d;%s;%s\n", i+1, julian_to_date(i+1, year),
						sm_holiday[i].string);
	}
	fflush(stdout);
}
