/*
 * write the master list to a vCalendar file. Most of the code here was
 * derived from http://www.kanzaki.com/docs/ical/.
 *
 *	write_vcal_mainlist(list, path, what)	Write appointments to vCalendar
 */

#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <Xm/Xm.h>
#include "cal.h"
#include "version.h"

extern struct plist	*mainlist;		/* all schedule entries */
extern struct user	*user;			/* user list for week view */
extern int		nusers;			/* # of users in user list */

static char *vcal_time(time_t, BOOL);
static void vcal_puts(FILE *, char *);


/*
 * write all files.
 */

void write_vcal_mainlist(
	char		*path)		/* write vCalendar data to this file */
{
	FILE		*fp;		/* vCalendar file to write */
	struct entry	*ep;		/* iterates over all entries */
	int		i, u, j, w;	/* entry and user counter, misc */
	char		message[2048];	/* message printed on alarm */
	char		*prefix;	/* prefix string for printing lists */

	path = resolve_tilde(path);
	if (!(fp = fopen(path, "w"))) {
		create_error_popup(0, 0, _("Cannot write vCalendar file\n%s: "
						"%\n"), path, strerror(errno));
		return;
	}
	tzset();
	fprintf(fp, "BEGIN:VCALENDAR\r\n"
		    "VERSION:2.0\r\n"
		    "X-WR-TIMEZONE;VALUE=TEXT:%s\r\n"
		    "PRODID:plan-%s\r\n"
		    "CALSCALE:GREGORIAN\r\n", tzname[0], VERSION);

	mainlist->locked = TRUE;
	for (ep=mainlist->entry, i=0; i < mainlist->nentries; i++, ep++) {
		if (ep->suspended)
			continue;
		fprintf(fp, "BEGIN:VEVENT\r\n");
							/* start date/time */
		fprintf(fp, "DTSTART;TZID=%s:%s\r\n", tzname[0],
					vcal_time(ep->time, ep->notime));
		*message = 0;
		if (ep->note || ep->message) {
			if (ep->note)
				strncat(message, ep->note, sizeof(message-4));
			if (ep->note && ep->message)
				strcat(message, "; ");
			if (ep->message)
				strncat(message, ep->message,
					sizeof(message-1)-strlen(message));
		}
							/* end date/time */
		if (ep->rep_every == 86400) {
			if (ep->rep_last)
				fprintf(fp, "DTEND;TZID=%s:%s\r\n", tzname[0],
					vcal_time(ep->rep_last, ep->notime));
			else
				fprintf(fp, "DTEND;TZID=%s:"
					"99990101T000000\r\n", tzname[0]);
		}
							/* duration */
		if (ep->length >= 60)
			fprintf(fp, "DURATION:PT%dH%dM\r\n",
				(int)ep->length/3600, (int)ep->length/60%60);

							/* note & message */
		if (ep->note || *message) {
			vcal_puts(fp, "SUMMARY:");
			vcal_puts(fp, ep->note ? ep->note : message);
			fputs("\r\n", fp);
		}
							/* early warn window */
		if (ep->early_warn >= 60) {
			fprintf(fp, "BEGIN:VALARM\r\n"
				    "ACTION:DISPLAY\r\n"
				    "TRIGGER:-P%dM\r\n"
				    "DESCRIPTION:Early warning: ",
				(int)ep->early_warn / 60);
			vcal_puts(fp, message);
			fputs("\r\nEND:VALARM\r\n", fp);
		}
							/* late warn window */
		if (ep->late_warn >= 60) {
			fprintf(fp, "BEGIN:VALARM\r\n"
				    "ACTION:DISPLAY\r\n"
				    "TRIGGER:-P%dM\r\n"
				    "DESCRIPTION:Late warning: ",
				(int)ep->late_warn / 60);
			vcal_puts(fp, message);
			fputs("\r\nEND:VALARM\r\n", fp);
		}
							/* show alarm window */
		if (!ep->noalarm) {
			fprintf(fp, "BEGIN:VALARM\r\n"
				    "ACTION:DISPLAY\r\n"
				    "DESCRIPTION:Alarm: ");
			vcal_puts(fp, message);
			fputs("\r\nEND:VALARM\r\n", fp);
		}
							/* every n-th day */
		if (ep->rep_every > 86400) {
			fprintf(fp, "RRULE:FREQ=DAILY;");
			if (ep->rep_last)
				fprintf(fp, "UNTIL=%s;",
					vcal_time(ep->rep_last, FALSE));
			fprintf(fp, "INTERVAL=%d\r\n",
				(int)ep->rep_every / 86400);
		}
							/* every weekday */
		if (ep->rep_weekdays && !(ep->rep_weekdays & 0xff00)) {
			fprintf(fp, "RRULE:FREQ=WEEKLY;");
			if (ep->rep_last)
				fprintf(fp, "UNTIL=%s;",
					vcal_time(ep->rep_last, FALSE));
			prefix = "WKST=MO;BYDAY=";
			for (j=0; j < 7; j++) {
				fprintf(fp, "%s%c%c", prefix,
					"MTWTFSS"[j], "OUEHRAU"[j]);
				prefix = ",";
			}
			fputs("\r\n", fp);
							/* every n-th weekday*/
		} else if (ep->rep_weekdays) {
			fprintf(fp, "RRULE:FREQ=MONTHLY;");
			if (ep->rep_last)
				fprintf(fp, "UNTIL=%s;",
					vcal_time(ep->rep_last, FALSE));
			prefix = "BYDAY=";
			for (w=0; w < 6; w++)
				for (j=0; j < 7; j++) {
					fprintf(fp, "%s%d%c%c", prefix,
						w == 5 ? -1 : w+1,
						"MTWTFSS"[j], "OUEHRAU"[j]);
					prefix = ",";
				}
			fputs("\r\n", fp);
		}
							/* month day repeats */
		if (ep->rep_days) {
			fprintf(fp, "RRULE:FREQ=MONTHLY;");
			if (ep->rep_last)
				fprintf(fp, "UNTIL=%s;",
					vcal_time(ep->rep_last, FALSE));
			prefix = "BYMONTHDAY=";
			for (j=0; j < 32; j++) {
				fprintf(fp, "%s%d", prefix, j ? j : -1);
				prefix = ",";
			}
			fputs("\r\n", fp);
		}
							/* exception dates */
		prefix = "EXDATE:";
		for (j=0; j < NEXC; j++)
			if (ep->except[j]) {
				fprintf(fp, "%s%s", prefix,
					vcal_time(ep->time, ep->notime));
				prefix = ",";
			}
		if (*prefix == ',')
			fputs("\r\n", fp);
							/* user ID (file ID) */
		if (!ep->user)
			fprintf(fp, "UID:%d\r\n", ep->private ? 2 : 1);
		else {
			for (u=nusers-1; u; u--)
				if (!strcmp(ep->user, user[u].name))
					break;
			fprintf(fp, "UID:%d\r\n", u ? u+1 : 0);
		}
							/* finish */
		fprintf(fp, "SEQUENCE:0\r\n");
		fprintf(fp, "END:VEVENT\r\n");
	}
	mainlist->locked = FALSE;

	fprintf(fp, "END:VCALENDAR\r\n");
	fclose(fp);
}


/*
 * print a time in vCalendar format: YYYYMMDD T HHMMSS, where HHMMSS==000000
 * for "no time". This means that midnight must be printed as 000001 because
 * otherwise midnight == no time.
 */

static char *vcal_time(
	time_t		time,		/* convert this time (seconds) */
	BOOL		notime)		/* no time, all day */
{
	static char buf[32];
	struct tm	*tm;		/* time to m/d/y h:m:s conv */

	tm = time_to_tm(time);
	if (notime)
		tm->tm_hour = tm->tm_min = tm->tm_sec = 0;
	else if (!(tm->tm_hour | tm->tm_min | tm->tm_sec))
		tm->tm_sec = 1;

	sprintf(buf, "%04d%02d%02dT%02d%02d%02d",
		(int)tm->tm_year + 1900,
		(int)tm->tm_mon+1,
		(int)tm->tm_mday,
		(int)tm->tm_hour,
		(int)tm->tm_min,
		(int)tm->tm_sec);
	return(buf);
}


/*
 * print a string. The trailing newline is ignored. Internal newlines are
 * replaced with "\r\n ", which vCalendar accepts as a continuation line.
 */

static void vcal_puts(
	FILE		*fp,		/* vCalendar file to write */
	char		*string)	/* string to print */
{
	for (; *string; string++)
		if (*string != '\n')
			fputc(*string, fp);
		else if (string[1])
			fputs("\r\n ", fp);
}
