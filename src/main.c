/*
 * Initializes everything and starts a calendar window for the current
 * month. The interval timer (for autosave and entry recycling) and a
 * few routines used by everyone are also here.
 *
 *	main(argc, argv)		Guess.
 *	resynchronize_daemon()		Called when the database on disk has
 *					changed; tell the daemon to re-read.
 *	register_X_input(fd)		network.c opened new server socket
 *	unregister_X_input(fd)		network.c closed server socket
 *
 * Author: thomas@bitrot.de (Thomas Driemeyer)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <Xm/Xm.h>
#include <Xm/Text.h>
#include <X11/StringDefs.h>
#include <sys/stat.h>				  /* umask() */
#include <assert.h>
#include "cal.h"
#include "version.h"

#define SIG_PERIOD 10

#ifdef JAPAN
#include <locale.h>
extern unsigned short e2j(), sj2j();
#endif

static void non_interactive	(int, char *[]);
static int  dumptoday		(char *, int, BOOL, BOOL, char *, BOOL);
static BOOL dumpfilelist	(void);
static void sighand		(int);
static void sig_check_timer	(int);
static void usage		(void);
static void init_resources	(void);
static void init_colors		(void);
static void init_fonts		(void);
static void init_daemon		(void);
static void timer_callback	(XtPointer, XtIntervalId *);
static void socket_callback	(XtPointer, int *, XtInputId *);
extern void run_daemon		();
#if !defined(SUN) && !defined(IBM)
static XtSignalId sig_id;
static void sig_callback	(XtPointer, XtSignalId *);
#endif
#ifdef MIPS
extern void exit		(int);
#endif
#ifdef __FreeBSD__
static void plan_exit		(int);
#endif

Display			*display;	/* everybody uses the same server */
GC			jgc, gc;	/* graphic context, Japanese and std */
GC			xor_gc;		/* XOR gc for rubberbanding */
GC			chk_gc;		/* checkered gc for light background */
XtAppContext		app;		/* application handle */
Widget			toplevel;	/* top-level shell for icon name */
struct mainmenu		mainmenu;	/* all important main window widgets */
struct config		config;		/* global configuration data */
char			*progname;	/* argv[0] */
BOOL			interactive;	/* interactive or fast appt entry? */
XFontStruct		*font[NFONTS];	/* fonts: FONT_* */
XColor			xcolor[NCOLS];	/* useful for PostScript color */
Pixel			color[NCOLS];	/* colors: COL_* */
int			curr_month;	/* month being displayed, 0..11 */
int			curr_year;	/* year being displayed, since 1900 */
time_t			last_x_timer;	/* when did last X timer happen? */
struct plist		*mainlist;	/* list of all schedule entries */
extern struct user	*user;		/* user list for week view */
extern Pixmap		pixmap[NPICS];	/* common symbols */
extern Pixmap	    graypixmap[NPICS];	/* common symbols, grayed-out */
static BOOL		nodaemon;	/* don't insist on a daemon (-s) */
static BOOL		rundaemon;	/* auto-run daemon if there is none */
BOOL			reread;		/* need to re-read database file */
extern char		lockpath[];	/* lockfile path (from lock.c) */
extern struct edit	edit;		/* info about entry being edited */
BOOL			debug;		/* print debugging information */

#ifdef JAPAN
char			*localename;			/* locale name */
unsigned short		(*kanji2jis)() = e2j;	/* or sj2j() */
#endif

static String fallbacks[] = {
#include "resources.h"
	NULL
};


/*
 * initialize everything and create the main calendar window
 */

int main(
	int			argc,
	char			*argv[])
{
	int			n, i;
	time_t			now;
	struct tm		*tm;
	char			buf[80], *name;
	BOOL			dofork = DOFORK;
	BOOL			nolock = FALSE;
	BOOL			others = FALSE;
	BOOL			showid = FALSE;
	char			*ulist = 0;
	char			*errmsg;
	XGCValues		gcval;

#ifdef __FreeBSD__
	atexit(plan_exit);
#endif
	interactive = FALSE;
	if ((progname = strrchr(argv[0], '/')) && progname[1])
		progname++;
	else
		progname = argv[0];
	if (!strcmp(progname, "webplan"))
		config.webplan = TRUE;
	tzset();
	now = get_time();
	tm = time_to_tm(now);
	curr_month = tm->tm_mon;
	curr_year  = tm->tm_year;
	set_tzone();
#ifdef PLANHOME
	mkdir(resolve_tilde("~/" PLANHOME), 0755);
#endif
	if (argc > 1 && isdigit(*argv[1])) {		/* non-interactive? */
		non_interactive(argc, argv);
		_exit(0);
	}
	for (n=1; n < argc; n++)			/* options */
		if (*argv[n] != '-')
			usage();
				/* multicharacter options are Xt/Xm, skip */
		else if (argv[n][2] < 'a' || argv[n][2] > 'z')
			switch(argv[n][1]) {
			  case 'd':
				for (i=0; fallbacks[i]; i++)
					printf("%s%s\n",progname,fallbacks[i]);
				fflush(stdout);
				return(0);
			  case 'v':
				fprintf(stderr, "%s: %s\n", progname,
							VERSION JAPANVERSION);
				return(0);
			  case 's':
				nodaemon = TRUE;
				break;
			  case 'S':
				rundaemon = TRUE;
				break;
			  case 'f':
				dofork = FALSE;
				break;
			  case 'b':
				dofork = TRUE;
				break;
			  case 'k':
				nolock = TRUE;
				break;
			  case 'o':
			  	others = TRUE;
				break;
			  case 'u':
			  	ulist = argv[++n];
				break;
			  case 'i':
				showid = TRUE;
				break;
			  case 't':
			  case 'T':
				if (argv[n][2])
					_exit(dumptoday(argv[n]+2,
					     argv[n+1] ? atoi(argv[n+1]) : 1,
					     argv[n][1]=='t', others,
					     ulist, showid));
				_exit(dumptoday(argv[n+1],
					     argv[n+2] ? atoi(argv[n+2]) : 1,
					     argv[n][1]=='t', others,
					     ulist, showid));
			  case 'W':
				config.webplan = TRUE;
				if (argv[n+1] && *argv[n+1] != '-')
					config.webplan_host = argv[++n];
				break;
			  case 'V':
				create_list(&mainlist);
				read_mainlist();
				write_vcal_mainlist(argv[n+1] ? argv[n+1]
							      : "vcal");
				return(0);
			  case 'F':
				_exit(!dumpfilelist());
			  case 'H':
				if (argv[n+1])
					dump_holiday(atoi(argv[n+1]));
			  	_exit(0);
			  case 'X':
				fprintf(stderr, "-X not yet supported\n");
				break;	/*<<<*/
			  default:
				usage();
			}
		else
			break;

	(void)umask(0077);
	if (dofork) {					/* background */
		PID_T pid = fork();
		if (pid < 0)
			perror("can't fork");
		else if (pid > 0)
			_exit(0);
	}
	interactive = TRUE;				/* init interactive */
	config.owner_only   = TRUE;
	config.ewarn_window = TRUE;
	config.lwarn_window = TRUE;
	config.alarm_window = TRUE;
	config.week_ndays   = 7;
	config.week_minhour = 8;
	config.week_maxhour = 20;
	config.dst_flag	    = 3;
	name            = getenv("LOGNAME");
	if (!name) name = getenv("USER");
	if (!name) name = getenv("user");
	if (!name) name = "username";
	sprintf(buf, "Mail -s %%s %.65s", name);
	config.mailer = mystrdup(buf);

	// try to enable UTF-8
	/* XtSetLanguageProc(NULL, NULL, NULL); */

	toplevel = XtAppInitialize(&app, "Plan", NULL, 0,
#		ifndef XlibSpecificationRelease
				(Cardinal *)&argc, argv,
#		else
				&argc, argv,
#		endif
				fallbacks, NULL, 0);
#ifdef JAPAN
	if (strcmp((localename = setlocale(LC_CTYPE, NULL)), LOCALE_SJIS) == 0)
		kanji2jis = sj2j;
#endif
#	ifdef EDITRES
	XtAddEventHandler(toplevel, (EventMask)0, TRUE, 
				_XEditResCheckMessages, NULL);
#	endif
	display = XtDisplay(toplevel);
	jgc	= XCreateGC(display, DefaultRootWindow(display), 0, 0);
	gc	= XCreateGC(display, DefaultRootWindow(display), 0, 0);
	xor_gc	= XCreateGC(display, DefaultRootWindow(display), 0, 0);
	chk_gc	= XCreateGC(display, DefaultRootWindow(display), 0, 0);
	gcval.function = GXxor;
	(void)XChangeGC(display, xor_gc, GCFunction, &gcval);
	XSetForeground(display, xor_gc,
			WhitePixelOfScreen(DefaultScreenOfDisplay(display)));

	init_resources();
	init_colors();
	init_fonts();
	init_pixmaps();

	set_icon(toplevel, 0);
	XSetForeground(display, xor_gc, color[COL_WIREFRAME]);

	read_mainlist();	/* get config: net, tzone mode, language, ...*/
	set_tzone();
	create_list(&mainlist);
	read_mainlist();	/* actually read apppointments */
	create_cal_widgets(toplevel);

	(void)recycle_all(mainlist, config.autodel, 0);
	signal(SIGHUP, sighand);

	if (!startup_lock(FALSE, nolock))
		if (!all_files_served())
			create_multiple_popup(nolock);
							/* holidays ok? */
	if ((errmsg = parse_holidays(curr_year, TRUE)))
		create_error_popup(mainmenu.cal, 0, errmsg);
							/* start */
#if !defined(SUN) && !defined(IBM)
	sig_id = XtAppAddSignal(app, sig_callback, 0);
#endif
	signal(SIGALRM, sig_check_timer);
	alarm(SIG_PERIOD);
							/* lockfile ok? */
	XtAppAddTimeOut(app, 100, timer_callback, 0);
	last_x_timer = get_time();
	XtRealizeWidget(toplevel);
	resynchronize_daemon();
	XtAppMainLoop(app);
	return(0);
}


/*
 * if the first argument is numeric, we are in non-interactive mode. Add a
 * single appointment at the specified time, signal the plan program to
 * re-read the database, and terminate.
 */

static void non_interactive(
	int		argc,			/* arguments from main */
	char		*argv[])		/* arguments from main */
{
	time_t		now;
	struct tm	*tm;			/* current date and time */
	struct tm	tm1;			/* for time conversion */
	time_t		trigger;		/* trigger date and time */
	char		msg[1024];		/* note from command line */
	int		n, i, j;		/* char and arg counters */
	long long	l;
	char		lockfile[80];		/* plan lockfile, for sighup */
	char		*flags;			/* where to hide reminder */
	FILE		*fp;			/* for reading lockfile */
	PID_T		pid = 0;		/* interactive plan PID */
	assert(sizeof(l) >= 6);

	create_list(&mainlist);
	read_mainlist();
	set_tzone();

	n = strlen(argv[1]);
	for (j=0; j < n; j++)
		if (argv[1][j] < '0' || argv[1][j] > '9')
			break;
	if (j == n) {				/* digits only: [yyyymmdd][mmdd]hhmm */
		if (n != 4 && n != 8 && n != 12 && j == n)
			usage();
		l = atoll(argv[1]);
		now = get_time();
		tm = time_to_tm(now);
		tm1.tm_sec   = 0;
		tm1.tm_min   = l % 100;
		tm1.tm_hour  = (l/100) % 100;
		tm1.tm_mday  = (n == 8 || n == 12) ? (l/10000)%100     : tm->tm_mday;
		tm1.tm_mon   = (n == 8 || n == 12) ? (l/1000000)%100-1 : tm->tm_mon;
		tm1.tm_year  = n == 12 ? ((l/100000000)%10000)-1900 : curr_year + (tm1.tm_mon < tm->tm_mon);
		trigger = tm_to_time(&tm1);
	} else {				/* try free-form date+time */
		trigger = parse_datetimestring(argv[1]);
		if (!trigger)
			usage();
	}
	*msg = 0;
	memset(&edit, 0, sizeof(struct edit));
	for (i=2; i+1 < argc && *argv[i] == '-'; i+=2)
		switch(argv[i][1]) {
		  case 'l':
			edit.entry.length = parse_timestring(argv[i+1], TRUE);
			break;
		  case 'n':
			edit.entry.notime = TRUE;
			edit.entry.length = 0;
			i--;
			break;
		  case 'r':
			edit.entry.rep_every = atoi(argv[i+1]) * 86400;
			break;
		  case 'Y':
			edit.entry.rep_yearly = TRUE; 
			--i;			/* No argument used for -Y */
			break;
		  case 'd':
			edit.entry.rep_days |= 1 << atoi(argv[i+1]);
			break;
		  case 'D':
			edit.entry.rep_weekdays |= 1 << atoi(argv[i+1]);
			break;
		  case 'O':
			edit.entry.rep_weekdays |= 1 << (atoi(argv[i+1]) + 7);
			break;
		  case 'e':
        		edit.entry.rep_last = parse_datestring(argv[i+1],
								trigger);
			break;
		  case 'w':
			edit.entry.early_warn = atoi(argv[i+1]) * 60;
			break;
		  case 'W':
			edit.entry.late_warn = atoi(argv[i+1]) * 60;
			break;
		  case 'u':
			edit.entry.user = mystrdup(argv[i+1]);
			break;
		  case 'c':
			edit.entry.acolor = atoi(argv[i+1]) * 60;
			break;
		  case 'N':
			++edit.entry.noalarm;
			--i;			/* No argument used for -N */
			break;
		  case 'h':
			flags = argv[i+1];
			while (*flags) {
				switch(*flags++) {
				  case 'm':	/* don't show in month view */
					++edit.entry.hide_in_m;
					break;
				  case 'y':	/* don't show in year view */
					++edit.entry.hide_in_y;
					break;
				  case 'w':	/* don't show in week view */
					++edit.entry.hide_in_w;
					break;
				  case 'o':	/* don't show in year overview */
					++edit.entry.hide_in_o;
					break;
				  case 'd':	/* don't show in day view */
					++edit.entry.hide_in_d;
					break;
				  default:
					usage();
				}
			}
			break;
		  default:
			usage();
		}
	for (; i < argc; i++) {
		if (strlen(msg) + strlen(argv[i]) > 1021)
			break;
		strcat(msg, argv[i]);
		if (i < argc-1)
			strcat(msg, " ");
	}
	if (user && !edit.entry.user)
		edit.entry.user = mystrdup(user[0].name);

	edit.editing	= TRUE;
	edit.changed	= TRUE;
	edit.entry.note = mystrdup(msg);
	edit.entry.time = trigger;
	edit.entry.file = user[name_to_user(edit.entry.user)].file_id;

	confirm_new_entry();
	(void)write_mainlist();
	resynchronize_daemon();
	printf("%s %s \"%.57s\"\n", mkdatestring(trigger),
				    mktimestring(trigger, FALSE), msg);
	fflush(stdout);
	sprintf(lockfile, resolve_tilde(PLANLOCK), (int)getuid());
	if ((fp = fopen(lockfile, "r"))) {
		fscanf(fp, "%d", &pid);
		if (pid > 1)
			(void)kill(pid, SIGHUP);
	}
}


/*
 * -t option, prints an ascii summary of all appointments on a day. This
 * is useful for cronjobs that send mail every morning. Return 1 if there
 * are no entries; this will be used as exit code (to suppress mail). The
 * <others> option (-o) prints not only own but also all other lists. The
 * <ulist> option (-u) prints all lists named in the comma-separated ulist.
 */

static int dumptoday(
	char		*date,		/* dump what day, "" is today */
	int		ndays,		/* number of days to dump */
	BOOL		lengthmode,	/* replace end time with length? */
	BOOL		others,		/* everybody else's appts too? */
	char		*ulist,		/* comma-separated list of users */
	BOOL		showid)		/* machine-readable format for WWW */
{
	struct lookup	lookup;		/* for finding entries */
	time_t		start;		/* 0:00 of the day to dump */
	BOOL		found;		/* TRUE if there is another entry */
	int		numfound = 0;	/* # of entries dumped */
	char		timestr[40];	/* buffer for time string */
	char		userstr[200];	/* buffer for user name */
	int		i;		/* for checking exceptions */
	time_t		trigger;
	char		*dash, *p;

	create_list(&mainlist);
	read_mainlist();

	start = parse_datestring(date && *date ? date : "today", 0);
	do {
	    for (found=lookup_entry(&lookup,mainlist,start,TRUE,FALSE,'*',-1);
			found && lookup.trigger < start+86400;
			found = lookup_next_entry(&lookup)) {
		struct entry *ep = &mainlist->entry[lookup.index];
		trigger = lookup.trigger - lookup.trigger % 86400;
		for (i=0; i < NEXC; i++)
			if (ep->except[i] == trigger)
				break;
		if (i < NEXC || ep->suspended || (!ulist && !others &&
				(ep->user && strcmp(ep->user, user[0].name))))
			continue;
		if (ulist) {
			char *ul = mystrdup(ulist), *u;
			for (u=strtok(ul, ","); u && strcmp(u, ep->user);
							     u=strtok(0, ","));
			free(ul);
			if (!u)
				continue;
		}
		strcpy(timestr, mktimestring(lookup.trigger, FALSE));
		sprintf(userstr, ep->private ? "private: " :
				 ep->user    ? "%s: "      : "", ep->user);
		if (showid)
			printf("%d;", ep->id);
		dash = showid ? "-" : "-    ";
		printf(showid ? "%s;%s;%s;%s;%s%s"
			      : "%-13s   %-6s  %5s   %s%s%s",
			mkdatestring(lookup.trigger),
			ep->notime ? dash : timestr,
			ep->length == 0 ? dash :
			lengthmode ? mktimestring(ep->length, TRUE)
				   : mktimestring(lookup.trigger +
						  ep->length, FALSE),
			userstr,
			lookup.days_warn ? "Warn: " : "",
			mknotestring(ep));
		if (ep->note && ep->message) {
			printf(", ");
			for (p=ep->message; *p; p++)
				putchar(*p == '\n' ? ' ' : *p);
		}
		putchar('\n');
		numfound++;
	    }
	    start += 86400;
	} while (--ndays > 0);
	fflush(stdout);
	return(numfound == 0);
}


/*
 * -F option: read file list from server and print it to stdout. Should be
 * used with -W option to select a server.
 */

static BOOL dumpfilelist(void)
{
	char		**list;		/* returned list */
	int		num, i;		/* returned # of entries in list */

	if (!get_netplan_filelist(config.webplan_host ?
			config.webplan_host : "localhost", &list, &num))
		return(FALSE);
	for (i=0; i < num; i++) {
		puts(list[i]);
		free(list[i]);
	}
	fflush(stdout);
	free(list);
	return(TRUE);
}


/*
 * network.c has opened a new connection fd to a server. Register the file
 * descriptor with the X server so the event loop gets interrupted when
 * the server wants to tell us something, such as that another plan has
 * modified an appointment and this plan ought to update its database.
 */

XtInputId register_X_input(
	int		fd)		/* new connection */
{
	return(app ? XtAppAddInput(app, fd, (XtPointer)XtInputReadMask,
					socket_callback, (XtPointer)0) : 0);
}


/*
 * network.c has closed a network connection. Tell the X server we are no
 * longer interested in events on its file descriptor. This is here because
 * <app> is here, and because network.c is also used by the daemon which
 * does not have XtRemoveInput.
 */

void unregister_X_input(
	XtInputId	x_id)		/* closed connection */
{
	XtRemoveInput(x_id);
}


/*
 * signal handler. SIGHUP re-reads the database. It is sent when another
 * plan program is started in non-interactive mode, by specifying an
 * appointment on the command line. That entry should appear in our menus.
 */

static void sighand(
	int			sig)		/* signal type */
{
	if (sig == SIGHUP) {				/* re-read database */
		signal(SIGHUP, sighand);
		reread = TRUE;
	} else {					/* die */
		(void)unlink(lockpath);
		fprintf(stderr, _("%s: killed with signal %d\n"),
							progname, sig);
		exit(0);
	}
}


/*
 * usage information
 */

static void usage(void)
{
	fprintf(stderr,
	    "GUI usage: %s [options]\n\tOptions:\n%s%s%s%s%s\n", progname,
	    "\t-s\t\tstandalone, don't offer to start daemon\n",
	    "\t-S\t\tautomatically start daemon if it doesn't exist\n",
	    "\t-f\t\tdon't fork on startup\n",
	    "\t-b\t\tdo fork on startup\n",
	    "\t-k\t\tignore lock file and start up in any case\n");
	fprintf(stderr,
	    "append usage: %s %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n", progname,
	    "[[yyyy]mmdd]hhmm [options] [message]*\n",
	    "\tAdd appointment at mm/dd hh:mm (default is today).\n",
	    "\tOptions:\n",
	    "\t-u U\t\tapply to user U instead of own appointments\n",
	    "\t-l T\t\tappointment length is T (eg., 1:00 is one hour)\n",
	    "\t-n\t\tappointment has no time-of-day\n",
	    "\t-N\t\tno alarms (reminder only)\n",
	    "\t-r N\t\trepeat every N days\n",
	    "\t-d N\t\trepeat every day N (1..31) of the month\n",
	    "\t-D N\t\trepeat every weekday N: 0=mon, 1=tue, ... 6=sun\n",
	    "\t-Y\t\trepeat every year (birthdays etc.)\n",
	    "\t-e D\t\tend date, stop repeating on date D\n",
	    "\t-w N\t\tearly warning is N minutes before hhmm time\n",
	    "\t-W N\t\tlate warning is N minutes before hhmm time\n",
	    "\t-c N\t\tcolor code for appointment\n",
	    "\t-h [m|y|w|o|d]*\thide in month, year, week, overview or day views\n");
	fprintf(stderr,
	    "batch usage: %s options\n\tOptions:\n%s%s%s%s%s%s%s%s%s%s%s%s%s",
	    progname,
	    "\t-h\t\tprint this help text\n",
	    "\t-d\t\tdump fallback app-defaults and exit\n",
	    "\t-v\t\tprint version string\n",
	    "\t-W [H]\t\tcalled from cgi-bin, get files from server host H\n",
	    "\t-H N\t\tdump all holidays in year N (1970..2037)\n",
	    "\t-F\t\tdump all files found on netplan server set with -W\n",
	    "\t-t [D [n]]\ttext dump of n days, starting with date D\n",
	    "\t-T [D [n]]\tsame as -t, print end time instead of length\n",
	    "\t\t\teg: -t, -t +3, -T tomorrow, -o -t 24.12. 3, -o -T wed 7\n",
	    "\t-o\t\tmodifies -t/-T output to show other users\n",
	    "\t-u N\t\tmodifies -t/-T to show comma-separated user list N\n",
	    "\t-i\t\tmake -t/-T machine-readable (used by WWW frontend)\n",
	    "\t-V file\t\texperimental vCalendar export\n");
	exit(1);
}


/*
 * exit is redefined here so it the database is written back before the
 * program dies for any reason - including Motif-Quit.
 * How is this for a hack :-) That's what you get for downloading sources
 * from alt.sources... This trick doesn't work on FreeBSD, but atexit is
 * not available everywhere so it's #ifdefed.
 */

#ifdef __FreeBSD__
void plan_exit(
#else
void exit(
#endif
	int	ret)
{
	static int exiting = 0;
	if (!exiting++) {
		if (*lockpath)
			(void)unlink(lockpath);
		if (mainlist && mainlist->modified) {
			write_mainlist();
			resynchronize_daemon();
		}
	}
	_exit(ret);
#ifndef __FreeBSD__
	exit(ret);
#endif
}


/*---------------------------------------------------------------------------*/
/*
 * read resources and put them into the config struct. This routine is used
 * for getting three types of resources: res_type={XtRInt,XtRBool,XtRString}.
 */

#define get_rsrc(A,B,C,D,E) do_get_rsrc((XtPointer)A, B, C, D, E)

void do_get_rsrc(
	XtPointer	ret,
	char		*res_name,
	char		*res_class_name,
	char		*res_type,
	int		minvalue)
{
	XtResource	res_list[1];

	res_list->resource_name	  = res_name;
	res_list->resource_class  = res_class_name;
	res_list->resource_type	  = res_type;
	res_list->resource_size	  = sizeof(res_type);
	res_list->resource_offset = 0;
	res_list->default_type	  = res_type;
	res_list->default_addr	  = 0;

	XtGetApplicationResources(toplevel, ret, res_list, 1, NULL, 0);

	if (minvalue >= 0 && res_type == XtRInt && *(int *)ret < minvalue) {
		fprintf(stderr, "plan: configuration error in X11 resources: "
				"%s (class %s) has value %d, minimum is %d.\n"
				"Run plan -d to see defaults.\n", res_name,
				res_class_name, *(int *)ret, minvalue);
		*(int *)ret = minvalue;
	}
}


static void init_resources(void)
{
    struct config	*c = &config;
    char		*startup_as = 0;

    get_rsrc(&c->calbox_xs[0],   "calBoxWidth",    "CalBoxWidth",  XtRInt, 16);
    get_rsrc(&c->calbox_ys[0],   "calBoxHeight",   "CalBoxHeight", XtRInt, 16);
    get_rsrc(&c->calbox_marg[0], "calBoxMargin",   "CalBoxMargin", XtRInt, 0);
    get_rsrc(&c->calbox_arrow[0],"calArrowWidth",  "CalArrowWidth",XtRInt, 4);
    get_rsrc(&c->calbox_xs[1],   "calBoxWidthSm",  "CalBoxWidth",  XtRInt, 16);
    get_rsrc(&c->calbox_ys[1],   "calBoxHeightSm", "CalBoxHeight", XtRInt, 16);
    get_rsrc(&c->calbox_marg[1], "calBoxMarginSm", "CalBoxMargin", XtRInt, 0);
    get_rsrc(&c->calbox_arrow[1],"calArrowWidthSm","CalArrowWidth",XtRInt, 4);
    get_rsrc(&c->notewidth,      "noteWidth",      "NoteWidth",    XtRInt, 16);
    get_rsrc(&c->year_margin,    "yearMargin",     "YearMargin",   XtRInt, 0);
    get_rsrc(&c->year_gap,	 "yearGap",        "YearGap",      XtRInt, 0);
    get_rsrc(&c->year_title,     "yearTitle",      "YearTitle",    XtRInt, 0);
    get_rsrc(&c->yearbox_xs,     "yearBoxWidth",   "YearBoxWidth", XtRInt, 80);
    get_rsrc(&c->yearbox_ys,     "yearBoxHeight",  "YearBoxHeight",XtRInt, 40);
    get_rsrc(&c->day_margin,     "dayMargin",      "dayMargin",    XtRInt, 0);
    get_rsrc(&c->day_gap,        "dayGap",         "dayGap",       XtRInt, 0);
    get_rsrc(&c->day_headline,   "dayHeadline",    "dayHeadline",  XtRInt, 0);
    get_rsrc(&c->day_hourwidth,  "dayHourWidth",   "dayHourWidth", XtRInt, 20);
    get_rsrc(&c->day_hourheight, "dayHourHeight",  "dayHourHeight",XtRInt, 8);
    get_rsrc(&c->day_barwidth,   "dayBarWidth",    "dayBarWidth",  XtRInt, 16);
    get_rsrc(&c->week_margin,    "weekMargin",     "WeekMargin",   XtRInt, 0);
    get_rsrc(&c->week_gap,	 "weekGap",        "WeekGap",      XtRInt, 0);
    get_rsrc(&c->week_daywidth,  "weekDayWidth",   "WeekDayWidth", XtRInt, 16);
    get_rsrc(&c->week_hourwidth, "weekHourWidth",  "WeekHourWidth",XtRInt, 16);
    get_rsrc(&c->week_barheight, "weekBarHeight",  "WeekBarHeight",XtRInt, 8);
    get_rsrc(&c->week_bargap,    "weekBarGap",     "WeekBarGap",   XtRInt, 2);
    get_rsrc(&c->week_maxnote,   "weekMaxNote",    "WeekMaxNote",  XtRInt, 0);
    get_rsrc(&c->yov_wwidth,     "yovWWidth",      "YovWWidth",    XtRInt, 16);
    get_rsrc(&c->yov_wheight,    "yovWHeight",     "YovWHeight",   XtRInt, 8);
    get_rsrc(&c->yov_daywidth,   "yovDayWidth",    "YovDayWidth",  XtRInt, 16);
    get_rsrc(&c->showicontime,   "showIconTime",   "ShowIconTime",XtRBool, 0);
    get_rsrc(&c->showicondate,   "showIconDate",   "ShowIconDate",XtRBool, 0);
    get_rsrc(&c->noiconclock,    "noIconClock",    "NoIconClock", XtRBool, 0);
    get_rsrc(&c->frame_today,    "frameToday",     "FrameToday",  XtRBool, 0);
    get_rsrc(&c->noicon,	 "noIcon",         "NoIcon",      XtRBool, 0);
    get_rsrc(&c->sgimode,        "sgiMode",        "SgiMode",     XtRBool, 0);
    get_rsrc(&c->nomonthshadow,  "noMonthShadow", "NoMonthShadow",XtRBool, 0);
    get_rsrc(&startup_as,        "startupAs",     "StartupAs",  XtRString, 0);
    config.smallmonth = startup_as && *startup_as == 's';
}


/*
 * determine all colors, and allocate them. They can then be used by a call
 * to set_color(COL_XXX).
 */

static void init_colors(void)
{
	Screen			*screen = DefaultScreenOfDisplay(display);
	Colormap		cmap;
	int			i, d;
	char			*c, *n, class_name[256];

	cmap = DefaultColormap(display, DefaultScreen(display));
	for (i=0; i < NCOLS; i++) {
		switch (i) {
		  default:
		  case COL_STD:		n = "colStd";		d=1;	break;
		  case COL_BACK:	n = "colBack";		d=0;	break;
		  case COL_CALBACK:	n = "colCalBack";	d=0;	break;
		  case COL_CALSHADE:	n = "colCalShade";	d=0;	break;
		  case COL_CALTODAY:	n = "colCalToday";	d=0;	break;
		  case COL_CALFRAME:	n = "colCalFrame";	d=0;	break;
		  case COL_GRID:	n = "colGrid";		d=1;	break;
		  case COL_WEEKDAY:	n = "colWeekday";	d=1;	break;
		  case COL_WEEKEND:	n = "colWeekend";	d=1;	break;
		  case COL_NOTE:	n = "colNote";		d=1;	break;
		  case COL_NOTEOFF:	n = "colNoteOff";	d=1;	break;
		  case COL_TOGGLE:	n = "colToggle";	d=1;	break;
		  case COL_RED:		n = "colRed";		d=1;	break;
		  case COL_TEXTBACK:	n = "colTextBack";	d=0;	break;
		  case COL_YBACK:	n = "colYearBack";	d=0;	break;
		  case COL_YBOXBACK:	n = "colYearBoxBack";	d=0;	break;
		  case COL_YNUMBER:	n = "colYearNumber";	d=1;	break;
		  case COL_YWEEKDAY:	n = "colYearWeekday";	d=1;	break;
		  case COL_YMONTH:	n = "colYearMonth";	d=1;	break;
		  case COL_YTITLE:	n = "colYearTitle";	d=1;	break;
		  case COL_YGRID:	n = "colYearGrid";	d=1;	break;
		  case COL_HBLACK:	n = "colHolidayBlack";	d=1;	break;
		  case COL_HRED:	n = "colHolidayRed";	d=1;	break;
		  case COL_HGREEN:	n = "colHolidayGreen";	d=1;	break;
		  case COL_HYELLOW:	n = "colHolidayYellow";	d=1;	break;
		  case COL_HBLUE:	n = "colHolidayBlue";	d=1;	break;
		  case COL_HMAGENTA:	n = "colHolidayMagenta";d=1;	break;
		  case COL_HCYAN:	n = "colHolidayCyan";	d=1;	break;
		  case COL_HWHITE:	n = "colHolidayWhite";	d=0;	break;
		  case COL_WBACK:	n = "colWeekBack";	d=0;	break;
		  case COL_WBOXBACK:	n = "colWeekBoxback";	d=0;	break;
		  case COL_WTITLE:	n = "colWeekTitle";	d=1;	break;
		  case COL_WGRID:	n = "colWeekGrid";	d=1;	break;
		  case COL_WDAY:	n = "colWeekDay";	d=1;	break;
		  case COL_WNOTE:	n = "colWeekNote";	d=1;	break;
		  case COL_WFRAME:	n = "colWeekFrame";	d=1;	break;
		  case COL_WWARN:	n = "colWeekWarn";	d=0;	break;
		  case COL_WUSER_0:	n = "colWeekUser_0";	d=0;	break;
		  case COL_WUSER_1:	n = "colWeekUser_1";	d=0;	break;
		  case COL_WUSER_2:	n = "colWeekUser_2";	d=0;	break;
		  case COL_WUSER_3:	n = "colWeekUser_3";	d=0;	break;
		  case COL_WUSER_4:	n = "colWeekUser_4";	d=0;	break;
		  case COL_WUSER_5:	n = "colWeekUser_5";	d=0;	break;
		  case COL_WUSER_6:	n = "colWeekUser_6";	d=0;	break;
		  case COL_WUSER_7:	n = "colWeekUser_7";	d=0;	break;
		  case COL_GRAYICON:	n = "colGrayIcon";	d=0;	break;
		  case COL_WIREFRAME:	n = "colWireFrame";	d=1;	break;
		}
		strcpy(class_name, n);
		class_name[0] &= ~('a'^'A');
		get_rsrc(&c, n, class_name, XtRString, 0);
		if (!XParseColor(display, cmap, c, &xcolor[i]))
			fprintf(stderr, _("%s: unknown color for %s\n"),
							progname, n);
		else if (!XAllocColor(display, cmap, &xcolor[i]))
			fprintf(stderr, _("%s: can't alloc color for %s\n"),
							progname, n);
		else {
			color[i] = xcolor[i].pixel;
			continue;
		}
		if (d) {
			color[i] = BlackPixelOfScreen(screen);
			xcolor[i].red = xcolor[i].green = xcolor[i].blue = 0;
		} else {
			color[i] = WhitePixelOfScreen(screen);
			xcolor[i].red = xcolor[i].green = xcolor[i].blue = ~0;
		}
	}
}


/*
 * load all fonts and make them available in the "fonts" struct. They are
 * loaded into the GC as necessary.
 */

static void init_fonts(void)
{
	int			i;
	char			*f, *nf, class_name[256];

	for (i=0; i < NFONTS; i++) {
		switch (i) {
		  default:
  		  case FONT_STD:	f = "fontList";		break;
		  case FONT_HELP:	f = "helpFont";		break;
		  case FONT_DAY:	f = "calNumberFont";	break;
		  case FONT_SMDAY:	f = "calNumberFontSm";	break;
		  case FONT_NOTE:	f = "calNoteFont";	break;
		  case FONT_YTITLE:	f = "yearTitleFont";	break;
		  case FONT_YMONTH:	f = "yearMonthFont";	break;
		  case FONT_YDAY:	f = "yearWeekdayFont";	break;
		  case FONT_YNUM:	f = "yearNumberFont";	break;
		  case FONT_WTITLE:	f = "weekTitleFont";	break;
		  case FONT_WDAY:	f = "weekDayFont";	break;
		  case FONT_WHOUR:	f = "weekHourFont";	break;
		  case FONT_WNOTE:	f = "weekNoteFont";	break;
#ifdef JAPAN
		  case FONT_JNOTE:	f = "jNoteFont";	break;
#endif
		}
		strcpy(class_name, f);
		class_name[0] &= ~('a'^'A');
		get_rsrc(&nf, f, class_name, XtRString, 0);
		if (!(font[i] = XLoadQueryFont(display, nf))) {
			fprintf(stderr, _("plan: warning: bad font for %s\n"),
									f);
			if (!(font[i] = XLoadQueryFont(display, "variable")) &&
			    !(font[i] = XLoadQueryFont(display, "fixed")))
				fatal(_("can't load font \"variable\"\n"), f);
		}
	}
	config.calbox_title[0]	= font[FONT_STD]   ->max_bounds.ascent +
				  font[FONT_STD]   ->max_bounds.descent;
	config.calbox_title[1]	= font[FONT_SMDAY] ->max_bounds.ascent +
				  font[FONT_SMDAY] ->max_bounds.descent;
	config.week_title	= font[FONT_WTITLE]->max_bounds.ascent +
				  font[FONT_WTITLE]->max_bounds.descent;
	config.week_hour	= font[FONT_WHOUR] ->max_bounds.ascent +
				  font[FONT_WHOUR] ->max_bounds.descent + 6;
}


/*
 * get the process ID of the daemon. This pid gets a SIGHUP signal whenever
 * the database has been written back to disk. The daemon then re-reads it.
 * If there is no daemon, set the PID to 0.
 */

static PID_T		daemon_pid;	/* process ID of daemon (gets SIGHUP)*/

static void init_daemon(void)
{
	int			i, lockfd;	/* lockfile descriptor */
	char			path[256];	/* lockfile path */
	char			buf[12];	/* contents of file (pid) */

	daemon_pid = 0;
	sprintf(path, resolve_tilde(PLANDLOCK), (int)getuid());
	if ((lockfd = open(path, O_RDONLY)) < 0) {
		if (rundaemon) {
			run_daemon(0);
			for (i=0; i < 5; i++) {
				if ((lockfd = open(path, O_RDONLY)) >= 0)
					break;
				sleep(1);
			}
		}
		if (lockfd < 0) {
			if (!nodaemon) {
				int e = errno;
				fprintf(stderr, _("%s: WARNING - no daemon, "),
							progname);
				errno = e;
				perror(path);
			}
			return;
		}
	}
	if (read(lockfd, buf, 10) < 5) {
		fprintf(stderr,
			_("%s: WARNING: daemon lockfile %s unreadable: "),
							progname, path);
		perror("");
		return;
	}
	close(lockfd);
	buf[10] = 0;
	daemon_pid = atoi(buf);
}


void resynchronize_daemon(void)
{
	if (!config.webplan && (!daemon_pid || kill(daemon_pid, SIGHUP))) {
		init_daemon();
		if (!nodaemon && (!daemon_pid || kill(daemon_pid, SIGHUP))) {
			fprintf(stderr, _("%s: WARNING: can't signal daemon: "),
								progname);
			perror("");
			if (interactive)
				create_nodaemon_popup();
			else
				fprintf(stderr,
				       _("%s: WARNING: no daemon, run pland\n"),
								progname);
		}
	}
}


/*
 * this routine gets called every 10 seconds. Redraw the main calendar when
 * midnight passes, to move the green highlight. Write the main list if it
 * has changed. Also, see if any old entries need to be recycled or deleted.
 * If we got a SIGHUP signal, re-read the database when it is safe. Keep the
 * last X timer time in last_x_timer - recent XFree servers (as of June 99)
 * appear to have a bug or incompatibility that prevents X timers from
 * happening. This way other code can find out about the failure and write
 * back changes immediately. The <busy> flag prevents this code from being
 * reentered while it is running, as the result of an alarm signal. This
 * looks like a race condition but is safe because there is only one thread.
 */

/*ARGSUSED*/
static void timer_callback(
	UNUSED XtPointer    data,	/* not used */
	UNUSED XtIntervalId *id)	/* not used */
{
	static int	busy;		/* this code is not reentrant */
	static time_t	last_tod;	/* previous time-of-day */
	time_t		tod;		/* current time-of-day */

	if (busy++) {
		busy--;
		return;
	}
	set_tzone();
	last_x_timer = get_time();
	tod = last_x_timer % 86400;
	if (tod < last_tod)
		redraw_all_views();

	if (tod/60 != last_tod/60) {
		print_icon_name();
		if (config.smallmonth)
			print_button(mainmenu.time,
					mktimestring(get_time(), FALSE));
		else
			print_button(mainmenu.time, "%s   %s",
					mkdatestring(get_time()),
					mktimestring(get_time(), FALSE));
	}
	last_tod = tod;
	if (mainlist->modified && !mainlist->locked) {
		write_mainlist();
		resynchronize_daemon();
	}
	if (!mainlist->modified && !mainlist->locked && reread) {
		reread = FALSE;
		destroy_list(&mainlist);
		read_mainlist();
		update_all_listmenus(TRUE);
		redraw_all_views();
	}
	if (recycle_all(mainlist, config.autodel, 7)) {
		draw_calendar(NULL);
		update_all_listmenus(TRUE);
	}
	XtAppAddTimeOut(app, TIMER_PERIOD, timer_callback, 0);
	busy--;
}


/*
 * It seems that current XFree implementations are broken and fail to send
 * X timer events. This causes plan to fail to write modified data back to
 * disks. These functions, written by Francis Montagnac <Francis.Montagnac@
 * sophia.inria.fr>, use periodic SysV alarm signals to prod the X timer.
 */

static void sig_check_timer (
	int		sig)		/* signal type */
{
	if (sig == SIGALRM)
		alarm(SIG_PERIOD);
	signal(sig, sig_check_timer);
#if !defined(SUN) && !defined(IBM)
	XtNoticeSignal(sig_id);
#endif
}


#if !defined(SUN) && !defined(IBM)
static void sig_callback (
	UNUSED XtPointer  data,		/* closure */
	UNUSED XtSignalId *id)
{
	if (last_x_timer < get_time()-30)	/* Even if last_x_timer == -1*/
		timer_callback(0, 0);
}
#endif


/*
 * all sockets are registered with the X server (by calling register_X_input)
 * to cause events when data arrives from the socket when this code isn't
 * expecting it. This happens when another plan modifies an appointment in a
 * file that this plan has open; it is necessary to update the local database.
 * This callback is called when such an event occurs. Read the message on
 * socked <fd> and let gets_server handle asynchronous events.
 */

static void socket_callback(
	UNUSED XtPointer data,
	int		 *fd,
	UNUSED XtInputId *x_id)
{
	char		 buf[1024];	/* message buffer for gets_server */
	BOOL		 save_mod;	/* auto-update isn't a modification */

	save_mod = mainlist->modified;
	(void)gets_server(*fd, buf, sizeof(buf), FALSE, FALSE);
	rebuild_repeat_chain(mainlist);
	mainlist->modified = save_mod;
	update_all_listmenus(TRUE);
	redraw_all_views();
}
