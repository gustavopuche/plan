/*
 * read the master list from the ~/.dayplan file. This file is also
 * linked into the daemon program (file_w.c is not).
 *
 *	find_pub_path(path)	resolve ~ in path and make absolute
 *	read_mainlist()		read all files into mainlist.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#ifdef MIPS
#include <sys/types.h>
#include <sys/fcntl.h>
#endif
#ifdef _APOLLO_SOURCE
#include <sys/types.h>
#include <sys/fcntl.h>
#endif
#if defined(ULTRIX) || defined(__EMX__)
#include <sys/types.h>
#endif
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>
#include <errno.h>
#include <Xm/Xm.h>
#include "cal.h"
#include "version.h"
#ifdef PLANGROK
#include "grok.h"
#include "form.h"
#endif

#define NDAYS		28			/* must agree with cal.h */

static int  readfile		(struct plist **, char *, int, BOOL);
static void make_default_user	(char *);
static void convert_dbase	(struct plist **, struct user *, char *);

extern char		*progname;		/* argv[0] */
extern struct plist	*mainlist;		/* all schedule entries */
extern struct config	config;			/* global configuration data */
struct user		*user;			/* user list for week view */
int			maxusers;		/* max # of users in u list */
int			nusers;			/* # of users in user list */
char			Print_Spooler[100];	/* print spooling command */


/*
 * Returns (in a static string) the calendar path corresponding to path,
 * resolving ~ and appending DB_PUB_PATH if a directory.
 */

char *find_pub_path(
	char		*path)		/* resolve ~ and make absolute */
{
	struct stat	sbuf;		/* check for directories */
	static char	pathbuf[1024];
	char		db_pub_path[512];

	strncpy(pathbuf, resolve_tilde(path), sizeof(pathbuf));
	pathbuf[sizeof(pathbuf)-1] = 0;
	if (!stat(pathbuf, &sbuf) && S_ISDIR(sbuf.st_mode)) {
		strcpy(db_pub_path, DB_PUB_PATH);
		assert(DB_PUB_PATH[0] == '~' && DB_PUB_PATH[1] == '/');
		assert(strlen(pathbuf)+strlen(db_pub_path) < sizeof(pathbuf));
		strcat(pathbuf, db_pub_path+1);
	}
	return(pathbuf);
}


/*
 * read the main list. First, read the main ~/.dayplan file, which also
 * provides the configuration data, including the user list. Next, read
 * the private dayplan file and all user dayplan files. Return a text
 * buffer with error messages; if msg is 0 print themn to stderr. This
 * file can't use create_error_popup because it is used by the daemon too.
 *
 * Depending on the mode, server connections are established after the
 * config part of the .dayplan file (including the user list and server
 * mode) was read in. The actual appointment data is then read from the
 * files or the servers.
 */

BOOL read_mainlist(void)
{
	struct stat	sbuf;		/* check for directories */
	char		msgbuf[4096];	/* default message buffer */
	char		*msg, *p;	/* where messages are stored */
	char		path[1024];	/* user file name to read */
	int		u;		/* user counter */
	int		error;
	char		prevserver[100];

	msg  = msgbuf;
	*msg = 0;
	destroy_list(&mainlist);			/* delete appt list */
	create_list(&mainlist);

	if (user) {					/* delete user list */
		for (u=0; u < nusers; u++) {
			if (user[u].name)
				free((char *)user[u].name);
			if (user[u].path)
				free((char *)user[u].path);
			if (user[u].server)
				free((char *)user[u].server);
		}
		free((char *)user);
		user = 0;
	}
	nusers = maxusers = 0;
	if (config.java) {
		free(config.java);
		config.java = 0;
	}
	if (config.webplan) {				/*webplan: dummy user*/
		make_default_user(resolve_tilde(DB_PUB_PATH));
		obtain_netplan_users(config.webplan_host ? config.webplan_host
							 : "localhost");
	} else {					/* main data */
		if ((error = readfile(&mainlist, DB_PUB_PATH, 0, TRUE)))
			sprintf(msg+strlen(msg),
				_("Failed to read master file %s, error %d\n"),
				DB_PUB_PATH, error);
		else if (!access(resolve_tilde(DB_PUB_PATH), F_OK) &&
			  access(resolve_tilde(DB_PUB_PATH), W_OK))
			sprintf(msg+strlen(msg), _("Warning: own config and "
				"data file %s is not writable\n"),DB_PUB_PATH);

							/* private data */
		if ((error = readfile(&mainlist, DB_PRIV_PATH, 0, FALSE)))
			sprintf(msg+strlen(msg),
				_("Failed to read private file %s, error %d\n"),
				DB_PRIV_PATH, error);
		else if (!access(resolve_tilde(DB_PRIV_PATH), F_OK) &&
			  access(resolve_tilde(DB_PRIV_PATH), W_OK))
			sprintf(msg+strlen(msg), _("Warning: own private data "
				"file %s is not writable\n"), DB_PRIV_PATH);

		make_default_user(resolve_tilde(DB_PUB_PATH));
	}
	if ((p = attach_to_network()))			/* network connection*/
		strcat(msg, p);
	read_from_server(&mainlist, 0, 0);
							/* user data */
	*prevserver = 0;
	for (u=1; u < nusers; u++) {			/* skip 0, 0 is own */
		if (user[u].fromserver) {
			if (*prevserver && strcmp(prevserver, user[u].server)){
				sprintf(msg+strlen(msg),
				  _("Warning: more than one server used: %s %s"
				  ". Appointments may be mixed between them."),
				  prevserver, user[u].server);
			}
			/* \\ Should check for host aliases */
			strcpy(prevserver, user[u].server);
			read_from_server(&mainlist, u, 0);
			continue;
		}
		if (!user[u].path || !user[u].name) {
			sprintf(msg+strlen(msg),
			     _("file of user #%d not read, no name or path\n"),
								u);
			continue;
		}
		strcpy(path, find_pub_path(user[u].path));
		user[u].readonly = !access(path, F_OK) && !!access(path, W_OK);
		user[u].modified = FALSE;
		user[u].time     = get_time();
		if (config.owner_only && !stat(path, &sbuf)
				      && sbuf.st_uid != getuid())
			user[u].readonly = TRUE;

		if ((error = readfile(&mainlist, path, u, FALSE)) &&
							error != ENOENT)
			sprintf(msg+strlen(msg),
			    _("Failed to read file %s of user %s, error %d\n"),
				path, user[u].name, error);
	}
	rebuild_repeat_chain(mainlist);

	if (*msg)
		create_error_popup(0, 0,
				_("Error while reading files:\n%s"), msg);
	mainlist->modified = FALSE;
	return(!*msg);
}


/*
 * read a list from a file. If <list> points to a non-null pointer, the
 * list in the file is merged into the list. As with add_entry(), <list>
 * is a pointer to a pointer to the list because the list might grow and
 * must be re-allocated. If the file is empty, no new list is allocated,
 * and <list> continues to point to a null pointer.
 *
 * If it turns out that appointment data is to be read from servers instead
 * of files after a 'n' (networking) line was read, reading is aborted when
 * the first appointment definition is found (line beginning with a digit):
 * network mode...
 *   1: always read from file
 *   2: only read own: from .dayplan (conf) or .dayplan.priv (!conf&&!name)
 *   3: never read from any file
 *
 * Always read config information if conf==TRUE, it never comes from a server.
 * Returns nonzero if the file could not be read.
 */

static int readfile(
	struct plist	**list,		/* list to add to */
	char		*path,		/* file to read list from */
	int		u,		/* user number, 0=own */
	BOOL		conf)		/* if TRUE, config stuff too */
{
	FILE		*fp;		/* open file */
	BOOL		writable;	/* can we lock the file? */
	char		line[1024];	/* line buffer */
	int		lineno = 0;	/* first line == "grok"? */

	if (conf)				/* defaults for 't' line */
		guess_tzone();
	path = resolve_tilde(path);
	writable = !access(path, W_OK);
	if (!(fp = fopen(path, writable ? "r+" : "r")))
		return(errno);
	if (writable)
		lockfile(fp, TRUE);
	for (;;) {
		if (!fgets(line, 1024, fp))
			break;
		if (!lineno && !strncasecmp(line, "begin:vcalendar", 15)) {
			struct user *up = &user[u];
			if (!u)
				create_error_popup(0, 0, _("The first file in the file list\n(%s)\nmust not be a vCalendar file!\nTrying to continue, you may lose data."), path);
			up->vcalendar = TRUE;
			up->readonly  = TRUE;
			read_vcal_file(list, fp, u);
			break;
		}
		if (!lineno++ && !strncmp(line, "grok", 4)) {
#ifndef PLANGROK
			create_error_popup(0, 0,
			     _("%s is a\ngrok database, not supported"), path);
#else
			extern FORM *form_create();
			struct user *up = &user[u];
			if (!u)
				create_error_popup(0, 0, _("The first file in the file list\n(%s)\nmust not be a grok database!\nTrying to continue, you may lose data."), path);
			form_delete(up->form);
			dbase_delete(up->dbase);
			up->grok     = TRUE;
			up->readonly = TRUE;
			up->form     = form_create();
			up->dbase    = dbase_create();
			if (read_form(up->form, path)) {
				char *p = up->form->dbase?up->form->dbase:path;
				read_dbase(up->dbase, up->form, p);
				convert_dbase(list, up, p);
			}
#endif
			break;
		}
		if (!conf && strchr("OotelLaPpmUu", *line))
			continue;
		if (conf &&
		    *line >= '0' && *line <= '9' && user
						 && user[u].fromserver)
			break;
		parse_file_line(list, path, user ? user[u].name:0, line);
	}
	if (writable)
		lockfile(fp, FALSE);
	fclose(fp);
	return(0);
}


/*
 * parse a line read from a file or from a server. An entry consists of one
 * or more lines and calls to this routine; remember the entry currently
 * being read and keep adding to it until a new entry starts. If the line
 * begins with '@', it's the first appt line from the network with '@fid rid'
 * prepended to the usual numerical date/time string.
 */

void parse_file_line(
	struct plist		**list,		/* list to add to */
	char			*path,		/* file/server to read from */
	char			*name,		/* user name, 0=not yet known*/
	char			*line)		/* line to be parsed */
{
	struct config		*c = &config;
	int			fid, rid;	/* network file ID and row ID*/
	char			*instring;	/* string at line[2] or [3] */
	char			*p;		/* line scan pointer */
	static struct entry	entry;		/* temp entry before adding */
	static struct entry	*ep = 0;	/* list entry being read */
	int			n, m;		/* tmp entry # of ep in list */
	struct tm		tm;		/* m/d/y h:m:s to time conv */
	int			tmp[9];		/* temps for sscanf */
	int			acol, ndays;	/* more temps for sscanf */
	char			tflag, sflag, pflag, nflag;
	char			nmflag, nyflag, nwflag, noflag, ndflag;
	char			**string;	/* string to add to */
	int			len;		/* new length of string */
	char			buf1[256], buf2[1024], buf3[1024];

	if (*line == '@') {
		sscanf(line+1, "%d %d", &fid, &rid);
		while (*++line != ' ');
		while (*++line == ' ');
		while (*++line != ' ');
		while (*++line == ' ');
		delete_entry_by_id(*list, fid, rid);
	} else
		fid = rid = 0;
	instring = line+2;
	*buf1 = *buf2 = *buf3 = 0;
	switch(*line) {
	  case '\n':
	  case '#':
		return;

	  case 'V':
		line[strlen(line)-1] = 0;
		sprintf(buf1, "plan %s", VERSION JAPANVERSION);
		if (strcmp(line, buf1))
			fprintf(stderr, _("%s: WARNING: file %s is version "\
					  "%s, program version is %s\n"),
						progname, path, line, buf1);
		break;

	  case 'o':
		tmp[0] = tmp[1] = tmp[2] = tmp[3] = tmp[4] = 0;
		tmp[5] = 7;
		sscanf(line+16, "%d %d %d %d %d %d",
				 tmp+0, tmp+1, tmp+2, tmp+3, tmp+4, tmp+5);
		c->early_time	  = tmp[0];
		c->late_time	  = tmp[1];
		c->wintimeout	  = tmp[2];
		c->week_minhour	  = tmp[3];
		c->week_maxhour	  = tmp[4];
		c->week_ndays	  = tmp[5];
		c->sunday_first   = line[2]  != '-';
		c->ampm           = line[3]  != '-';
		c->mmddyy         = line[4]  != '-';
		c->autodel        = line[5]  != '-';
		c->julian         = line[6]  != '-';
		c->gpsweek        = line[7]  == 'g';
		c->weeknum        = line[7]  != '-';
		c->nopast         = line[8]  != '-';
		c->insecure_script= line[9]  != '-';
		c->weekwarn       = line[10] != '-';
		c->weekuser       = line[11] != '-';
		c->week_bignotime = line[12] != '-';
		c->fullweek       = line[13] == 'w';
		c->thu_week       = line[13] == 't';
		c->colorcode_m    = line[14] != '-';
		c->ownonly	  = line[15] != '-';
		if (c->week_minhour >= c->week_maxhour) {
			c->week_minhour = 8;
			c->week_maxhour = 20;
		}
		if (c->week_ndays < 1)
			c->week_ndays = 7;
		if (c->week_ndays > NDAYS)
			c->week_ndays = NDAYS;
		break;

	  case 'O':
		c->share_mainwin  = line[2]  != '-';
		c->resize_mainwin = line[3]  != '-';
		c->moretimecols   = line[4]  != '-';
		break;

	  case 't':
		tmp[0] = tmp[1] = tmp[2] = tmp[3] = tmp[4] = 0;
		sscanf(line+2, "%d %d %d %d %d",
				 tmp+0, tmp+1, tmp+2, tmp+3, tmp+4);
		c->adjust_time	= tmp[0];
		c->raw_tzone	= tmp[1];
		c->dst_flag	= tmp[2];
		c->dst_begin	= tmp[3];
		c->dst_end	= tmp[4];
		set_tzone();
		break;

	  case 'e':
		c->ewarn_window = line[2] != '-';
		c->ewarn_mail   = line[3] != '-';
		c->ewarn_exec   = line[4] != '-';
		line[strlen(line)-1] = 0;
		if (c->ewarn_prog)
			free(c->ewarn_prog);
		c->ewarn_prog = 0;
		if (line[5] && line[6])
			c->ewarn_prog = mystrdup(line+6);
		break;

	  case 'l':
		c->lwarn_window = line[2] != '-';
		c->lwarn_mail   = line[3] != '-';
		c->lwarn_exec   = line[4] != '-';
		line[strlen(line)-1] = 0;
		if (c->lwarn_prog)
			free(c->lwarn_prog);
		c->lwarn_prog = 0;
		if (line[5] && line[6])
			c->lwarn_prog = mystrdup(line+6);
		break;

	  case 'a':
		c->alarm_window = line[2] != '-';
		c->alarm_mail   = line[3] != '-';
		c->alarm_exec   = line[4] != '-';
		line[strlen(line)-1] = 0;
		if (c->alarm_prog)
			free(c->alarm_prog);
		c->alarm_prog = 0;
		if (line[5] && line[6])
			c->alarm_prog = mystrdup(line+6);
		break;

	  case 'y':
		c->yov_single = line[2] != '-';
		for (p=line+2; *p && *p != ' '; p++);
		sscanf(p, "%d %d %d", &c->yov_nmonths,
				      &c->yov_display,
				      &c->yov_user);
		break;

	  case 'P':
		c->pr_omit_appts  = line[2] != '-';
		c->pr_omit_priv   = line[3] != '-';
		c->pr_omit_others = line[4] != '-';
		c->pr_color	  = line[5] != '-';
		c->pr_mode        = atoi(line+13);
		break;

	  case 'p':
		strncpy(Print_Spooler, (const char *)line + 2,
					sizeof(Print_Spooler));
		Print_Spooler[sizeof(Print_Spooler)-1] = 0;
		Print_Spooler[strlen(Print_Spooler)-1] = 0;
		break;

	  case 'm':
		line[strlen(line)-1] = 0;
		if (line[1] && line[2])
			c->mailer = mystrdup(line+2);
		break;

	  case 'U':
		make_default_user(path);
		break;

	  case 'u':
		if (nusers >= maxusers) {
			n = nusers + 10;
			user = user ? realloc(user, n * sizeof(struct user))
				    : malloc(n * sizeof(struct user));
			if (!user)
				fatal(_("no memory"));
			memset(user+maxusers, 0,
					(n-maxusers) * sizeof(struct user));
			maxusers = n;
		}
		sscanf(line+2, "%s %s %d %d %d %d %d %d %s %d\n", buf1, buf2,
				&user[nusers].suspend_w,
				&user[nusers].color,
				&user[nusers].suspend_m,
				&user[nusers].alarm,
				&user[nusers].suspend_o,
				&user[nusers].fromserver, buf3,
				&user[nusers].suspend_d);
		if (!*buf3 && gethostname(buf3, sizeof(buf3)))
			strcpy(buf3, "localhost");
		m = 0;
		do for (n=nusers-1; n >= 0; n--)
			if (!strcmp(user[n].name, buf1)) {
				strcat(buf1, "x");
				break;
			}
		while (n >= 0 && m++ < nusers);
		user[nusers].name    = mystrdup(buf1);
		user[nusers].path    = mystrdup(buf2);
		user[nusers].server  = mystrdup(buf3);
		user[nusers].file_id = -1;
		nusers++;
		break;

	  case 'L':
		if (c->language)
			free(c->language);
		c->language = mystrdup(line+2);
		for (p=c->language; *p > ' '; p++);
		*p = 0;
		break;

	  case 'j':
		if (c->java) {
			c->java = realloc(c->java, strlen(c->java) +
						   strlen(line) + 1);
			strcat(c->java, line);
		} else {
			c->java = malloc(strlen(line) + 1);
			strcpy(c->java, line);
		}
		break;

	  case '0':
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	  case '8':
	  case '9':
		make_default_user(path);
		clone_entry(&entry, (struct entry *)0);
		sflag = pflag = nflag =
		nmflag = nyflag = nwflag = noflag = ndflag = 0;
		acol = ndays = 0;
		sscanf(line,
      "%d/%d/%d %d:%d:%d %d:%d:%d %d:%d:%d %d:%d:%d %c%c%c%c%c%c%c%c%c- %d %d",
			&tm.tm_mon, &tm.tm_mday, &tm.tm_year,
			&tm.tm_hour, &tm.tm_min, &tm.tm_sec,
			tmp+0, tmp+1, tmp+2,
			tmp+3, tmp+4, tmp+5,
			tmp+6, tmp+7, tmp+8,
			&sflag, &pflag, &nflag,
			&nmflag, &nyflag, &nwflag, &noflag, &ndflag, &tflag,
			&acol, &ndays);
		tm.tm_mon--;
		if (tm.tm_year < 70)
			tm.tm_year += 100;
		if (tm.tm_year > 1900)
			tm.tm_year -= 1900;
		if ((entry.notime = tm.tm_hour > 23))
			tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
		entry.time         = tm_to_time(&tm);
		entry.length       = tmp[0]*3600 + tmp[1]*60 + tmp[2];
		entry.early_warn   = tmp[3]*3600 + tmp[4]*60 + tmp[5];
		entry.late_warn    = tmp[6]*3600 + tmp[7]*60 + tmp[8];
		entry.days_warn    = ndays * 86400;
		entry.suspended    = sflag  == 'S';
		entry.private      = pflag  == 'P';
		entry.noalarm      = nflag  == 'N';
		entry.hide_in_m    = nmflag == 'M';
		entry.hide_in_y    = nyflag == 'Y';
		entry.hide_in_w    = nwflag == 'W';
		entry.hide_in_o    = noflag == 'O';
		entry.hide_in_d    = ndflag == 'D';
		entry.todo	   = tflag  == 't';
		entry.acolor       = acol;
		entry.user	   = name ? mystrdup(name) : 0;
		entry.file	   = fid;
		entry.id	   = rid;
		n = add_entry(list, &entry);	/* sets <list> */
		ep = &(*list)->entry[n];
		break;

	  case 'R':
		tmp[0] = tmp[1] = tmp[2] = tmp[3] = tmp[4] = 0;
		sscanf(line+2, "%d %d %d %d %d",
				 tmp+0, tmp+1, tmp+2, tmp+3, tmp+4);
		ep->rep_every	= tmp[0];
		ep->rep_last	= tmp[1];
		ep->rep_weekdays= tmp[2];
		ep->rep_days	= tmp[3];
		ep->rep_yearly	= tmp[4];

		if (ep->time && ep->rep_last && ep->time > ep->rep_last)
			ep->rep_last = 0;
		break;

	  case 'E':
		for (n=0; n < NEXC; n++)
			if (!ep->except[n])
				break;
		if (n < NEXC) {
			sscanf(line+2, "%d/%d/%d",
				&tm.tm_mon, &tm.tm_mday, &tm.tm_year);
			tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
			tm.tm_mon--;
			if (tm.tm_year < 70)
				tm.tm_year += 100;
			if (tm.tm_year > 1900)
				tm.tm_year -= 1900;
			ep->except[n] = tm_to_time(&tm);
		} else
			fprintf(stderr, "%s",
				_("too many exception dates for appt"));
		break;

	  case 'N':
		string = &ep->note;
		ep->hide_in_w |= line[2] == '-';
		if (line[2] == '-' || line[2] == '=') {
			ep->hide_in_m = ep->hide_in_y = TRUE;
			instring++;
		}
		goto add;

	  case 'M':
		string = &ep->message;
		goto add;

	  case 'S':
		string = &ep->script;
add:			len  = *string ? strlen(*string) : 0;
		len += strlen(instring) + 2;
		if (!(p = malloc(len))) {
			fprintf(stderr, _("%s: no memory"), progname);
			exit(1);
		}
		if (*string) {
			strcpy(p, *string);
			strcat(p, instring);
			free(*string);
		} else
			strcpy(p, instring);
		*string = p;
		break;

	  case 'J':
		if (ep->java) {
			ep->java = realloc(ep->java, strlen(ep->java) +
						   strlen(line) + 1);
			strcat(ep->java, line);
		} else {
			ep->java = malloc(strlen(line) + 1);
			strcpy(ep->java, line);
		}
		break;

	  default:
		fprintf(stderr, _("%s: %s: illegal line:\n%s"),
						progname, path, line);
	}
}


/*
 * When a 'U', 'u', or begin-appointment line is found, user[0] (own) must
 * exist. If not, this routine makes one, and also ensures that <user> is
 * not a null pointer because everybody relies on that.
 */

static void make_default_user(
	char		*path)		/* file path for own .dayplan */
{
	struct passwd	*pw;
	char		host[1024], *name;

	if (user && nusers && user[0].name)
		return;
	if (config.webplan)
		name  = "webplan";
	else {
		pw = getpwuid(getuid());
		name = pw ? pw->pw_name : getenv("LOGNAME");
		if (!name)
			name = _("unknown");
	}
	if (!user) {
		maxusers = 10;
		if (!(user = malloc(maxusers * sizeof(struct user))))
			fatal(_("no memory"));
		memset(user, 0, maxusers * sizeof(struct user));
	}
	if (gethostname(host, sizeof(host)))
		strcpy(host, "localhost");
	user[0].name    = mystrdup(name);
	user[0].path    = mystrdup(path);
	user[0].server  = mystrdup(host);
	user[0].alarm   = !config.webplan;
	user[0].file_id = -1;
	nusers = 1;
}


/*
 * convert a grok database to a list of appointment entries
 */

#ifdef PLANGROK
static void convert_dbase(
	struct plist	**list,		/* list to add to */
	struct user	*up,		/* user with form/dbase */
	char		*db)		/* db name for error messages*/
{
	struct entry	*elist;		/* temporary list of entries */
	struct entry	*ep;		/* list entry being read */
	ITEM		*item;		/* current field description */
	ROW		*row;		/* curr database row */
	char		*data;		/* curr database contents */
	int		en, ei;		/* # of entries; curr entry */
	int		fn, fi;		/* # of fields; curr field */
	BOOL		tagged=FALSE;	/* dbase has plan interface? */
	BOOL		have_d, have_n;	/* dbase has date & note? */
	int		def_d, def_n;	/* default date & note fields*/
	int		n;

	if ((en = up->dbase->nrows) < 1 || (fn = up->form->nitems) < 1)
		return;
	if (!(elist = (struct entry *)malloc(en * sizeof(struct entry))))
		fatal(_("no memory for appointment list"));
	for (ep=elist, ei=0; ei < en; ei++, ep++) {
		clone_entry(ep, (struct entry *)0);
		ep->grok_row = ei;
	}
	have_d = have_n = FALSE;			/* have date & note? */
	def_d  = def_n  = -1;
	for (fi=fn-1; fi >= 0; fi--) {
		item = up->form->items[fi];
		tagged |= item->plan_if != 0;
		have_d |= item->plan_if == 't';
		have_n |= item->plan_if == 'n';
		if (item->type == IT_TIME &&
		   (item->timefmt == T_DATE || item->timefmt == T_DATETIME))
			def_d = fi;
		if (item->type == IT_INPUT || item->type == IT_NOTE)
			def_n = fi;
	}
	if (!have_d && def_d < 0) {
		create_error_popup(0, 0, _("xmbase-grok database %s\ncannot be read because the calendar\ninterface is %s, and there are no\ndate fields to automatically generate one."), db, tagged ? _("incomplete") : _("missing"));
		return;
	}
	if (!have_d)
		up->form->items[def_d]->plan_if = 't';
	if (!have_n)
		up->form->items[def_n]->plan_if = 'n';

	for (fi=0; fi < fn; fi++) {			/* ok, convert dbase */
		item = up->form->items[fi];
		for (ep=elist, ei=0; ei < en; ei++, ep++) {
			row = up->dbase->row[ei];
			if (row->ncolumns <= item->column)
				continue;
			if (!(data = row->data[item->column]))
				continue;
			switch(item->plan_if) {
			  case 't': if (item->timefmt == T_DATE) {
					ep->notime = TRUE;
					ep->time = atoi(data);
				    } else if (item->timefmt == T_DATETIME)
					ep->time = atoi(data);
				    break;
			  case 'l': ep->length	   = atoi(data);	break;
			  case 'w': ep->early_warn = atoi(data);	break;
			  case 'W': ep->late_warn  = atoi(data);	break;
			  case 'r': ep->rep_every  = atoi(data)*86400;	break;
			  case 'e': ep->rep_last   = atoi(data);	break;
			  case 'c': ep->acolor	   = atoi(data) & 7;	break;
			  case 'n': ep->note	   = mystrdup(data);	break;
			  case 'm': ep->message	   = mystrdup(data);	break;
			  case 's': ep->script	   = mystrdup(data);	break;
			  case 'S': ep->suspended  = !!atoi(data);	break;
			  case 'T': ep->notime	   = !!atoi(data);	break;
			  case 'A': ep->noalarm	   = !!atoi(data);	break;
			}
		}
	}
	for (ep=elist, n=ei=0; ei < en; ei++, ep++)	/* sanity checks */
		if (ep->time) {
			n++;
			if (ep->rep_last) {
				if (!ep->rep_every)
					ep->rep_every = 86400;
				if (ep->rep_last < ep->time)
					ep->rep_last = ep->time;
			}
			ep->user = mystrdup(up->name);
			add_entry(list, ep);
		} else
			destroy_entry(ep);
	if (!n)
		create_error_popup(0, 0, _("%d of %d appointments from grok database\n%s discarded\nbecause of invalid date/time."), en-n, en, db);

	free((void *)elist);
}
#endif
