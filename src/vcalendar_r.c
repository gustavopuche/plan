/*
 * read a calendar file in vCalendar format. vCalendar files are used by many
 * calendar programs such as Apple iCal, Zimbra, Google Calendar, Lotus, and
 * many others. The format is defined in RFC 2445, but only a small subset of
 * that monster of a standard is implemented here.
 *
 *	read_vcal_file()	read an vCalendar files into an entry list.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <Xm/Xm.h>
#include "cal.h"
#include "config.h"
#include "proto.h"

static void   parse_vcal_line(struct plist **, char *, char *);
static char  *parse_vcal_command_arg(char *);
static time_t parse_vcal_datetime(char *, BOOL *);
static time_t parse_vcal_duration(char *);

static BOOL in_entry;			/* between begin..end entry */
extern struct user *user;		/* user list (from file_r.c) */
extern struct config config;		/* global configuration data */


/*
 * read a list from an vCalendar file. If <list> points to a non-null pointer,
 * the list in the file is merged into the list. As with add_entry(), <list>
 * is a pointer to a pointer to the list because the list might grow and
 * must be re-allocated. If the file is empty, no new list is allocated,
 * and <list> continues to point to a null pointer.
 */

void read_vcal_file(
	struct plist	**list,		/* list to add to */
	FILE		*fp,		/* file to read list from */
	int		u)		/* user number */
{
	char		line[1024], *p;	/* line buffer */

	in_entry = FALSE;
	rewind(fp);
	while (fgets(line, 1024, fp)) {
		if ((p = strchr(line, '\r'))) *p = 0;
		if ((p = strchr(line, '\n'))) *p = 0;
		parse_vcal_line(list, user ? user[u].name : 0, line);
	}
}


/*
 * parse a line read from the vCalendar file. Parsing is rather simple: just
 * waand ignore everything not known.
 */

static void parse_vcal_line(
	struct plist		**list,		/* list to add to */
	char			*name,		/* user name, 0=not yet known*/
	char			*line)		/* line to be parsed */
{
	static struct entry	entry;		/* temp entry before adding */

	char *arg = parse_vcal_command_arg(line);
	if (!arg)
		return;

	if (!strcasecmp(line, "begin") && !strcasecmp(arg, "vevent")) {
		clone_entry(&entry, (struct entry *)0);
		entry.user = name ? mystrdup(name) : 0;
		in_entry = TRUE;

	} else if (!strcasecmp(line, "begin") && !strcasecmp(arg, "vtodo")) {
		clone_entry(&entry, (struct entry *)0);
		entry.user = name ? mystrdup(name) : 0;
		in_entry = TRUE;
	}
	if (!in_entry)		/* ignore any entries other than the above */
		return;

	if (!strcasecmp(line, "dtstart")) {
		entry.time = parse_vcal_datetime(arg, &entry.notime);
		entry.noalarm = TRUE;

	} else if (!strcasecmp(line, "dtend")) {
		time_t end = parse_vcal_datetime(arg, 0);
		if (end - entry.time < 86400)
			entry.length = end - entry.time;
		else {
			entry.rep_every = 86400;
			entry.rep_last  = end;
		}

	} else if (!strcasecmp(line, "duration")) {
		time_t dur = parse_vcal_duration(arg);
		if (dur < 86400)
			entry.length = dur;
		else {
			entry.rep_every = 86400;
			entry.rep_last  = entry.time + dur;
		}

	} else if (!strcasecmp(line, "summary")) {
		entry.note = mystrdup(arg);

	} else if (!strcasecmp(line, "trigger")) {
		entry.time = parse_vcal_datetime(arg, &entry.notime);

	} else if (!strcasecmp(line, "organizer")	||
		   !strcasecmp(line, "attendee")	||
		   !strcasecmp(line, "due")		||
		   !strcasecmp(line, "status")		||
		   !strcasecmp(line, "class")		||
		   !strcasecmp(line, "category")	||
		   !strcasecmp(line, "description")) {

		char *msg, *p;
		for (p=line+1; *p; p++)
			*p = tolower(*p);
		if (entry.message) {
			msg = malloc(strlen(entry.message) + strlen(line)+2 +
					strlen(arg) + 1);
			sprintf(msg, "%s%s: %s\n", entry.message, line, arg);
			free(entry.message);
		} else {
			msg = malloc(strlen(line)+2 + strlen(arg) + 1);
			sprintf(msg, "%s: %s\n", line, arg);
		}
		entry.message = msg;

	} else if (!strcasecmp(line, "end") && (!strcasecmp(arg, "vevent") ||
						!strcasecmp(arg, "vtodo"))) {
		if (!entry.note)
			entry.note = mystrdup("(none)");
		add_entry(list, &entry);
		in_entry = FALSE;
	}
}


/*
 * put a \0 at the end of the command, and return a pointer to the argument.
 * Lines have the form "command[;junk]*:arg". Return 0 for lines that don't
 * have this form; they will be ignored.
 */

static char *parse_vcal_command_arg(
	char		*line)		/* input line to break up */
{
	char *p;
	for (p=line; *p; p++)
		if (*p == ';')
			*p++ = 0;
		else if (*p == ':') {
			*p = 0;
			return p+1;
		}
	return 0;
}


/*
 * parse an ICS date/time string of the form dateTtime, with date=YYYYMMDD and
 * time=HHMMSS. If a 'Z' follows, the date is UTC and must be converted to
 * local time.
 */

static time_t parse_vcal_datetime(
	char		*str,		/* parse this ICS date/time string */
	BOOL		*notime)	/* set to true if Ttime is missing */
{
	struct tm tm;
	memset(&tm, 0, sizeof(tm));
	sscanf(str, "%04d%02d%02d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
	tm.tm_year -= 1900;
	tm.tm_mon--;
	if (notime)
		*notime = str[8] != 'T';
	if (str[8] == 'T')
		sscanf(str+9, "%02d%02d%02d",
					&tm.tm_hour, &tm.tm_min, &tm.tm_sec);
	if (str[15] == 'Z')
		return(tm_to_time(&tm) + config.tzone);
	else
		return(tm_to_time(&tm));
}


/*
 * parse an ICS suration string of the form [+|-]PdatesTtimes, where dates is
 * a sequence of nW (weeks) or nD; and times is a sequence of nH, nM, or nS.
 * For example, P15DT5H30M0S means 15 days and 5:30:00.
 */

static time_t parse_vcal_duration(
	char		*str)		/* parse this ICS duration string */
{
	int dur = 0, n = 0, sign = 1;
	for (; *str; str++)
		switch (*str | 0x20) {
		  case '+': sign = 1;				break;
		  case '-': sign = -1;				break;
		  case 'p':					break;
		  case 't':					break;
		  case 'w': dur += n * 7 * 86400; n = 0;	break;
		  case 'd': dur += n * 86400;	  n = 0;	break;
		  case 'h': dur += n * 3600;	  n = 0;	break;
		  case 'm': dur += n * 60;	  n = 0;	break;
		  case 's': dur += n;		  n = 0;	break;
		  case '0':
		  case '1':
		  case '2':
		  case '3':
		  case '4':
		  case '5':
		  case '6':
		  case '7':
		  case '9': n = n * 10 + *str - '0';		break;
		}
	return((time_t)(dur * sign));
}
