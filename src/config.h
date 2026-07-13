#ifndef _CONFIG_H
#define _CONFIG_H


/*
 * if defined, compile in xmbase-grok support, mainly grok database access
 * code. You need grok 1.4 and up to really use this. See my homepage to find
 * out about the grok database manager (http://www.bitrot.de/).
 */

#define PLANGROK


/*
 * definitions common to the main program and the daemon. It excludes all
 * X and motif stuff.
 */

#ifndef PLANLOCK			 /* defaults for cc -D options */
#define PLANLOCK	/tmp/.plan%d
#endif
#ifndef PLANDLOCK			 /* defaults for cc -D options */
#define PLANDLOCK	/tmp/.pland%d
#endif
#ifndef DOFORK				 /* is -f or -F default? */
#define DOFORK		TRUE
#endif

#ifdef PLANHOME
#define DB_PUB_PATH	"~/" PLANHOME "/dayplan"
#define DB_PRIV_PATH	"~/" PLANHOME "/dayplan.priv"
#define HOLIDAY_PATH	"~/" PLANHOME "/holiday"
#define PLAND_PATH	"~/" PLANHOME "/pland"
#else
#define DB_PUB_PATH	"~/.dayplan"	 /* public appts & config */
#define DB_PRIV_PATH	"~/.dayplan.priv"/* private appointments */
#define HOLIDAY_PATH	"~/.holiday"	 /* holiday database */
#define PLAND_PATH	"~/.pland"	 /* pland database */
#endif
#define HOLIDAY_NAME	"holiday"	 /* global holiday dbase (LIB)*/
#define HELP_FN		"plan.help"	 /* help text file */
#define DAEMON_FN	"pland"		 /* daemon program */
#define ALARM_FN	"notifier"	 /* program that shows alarm popup */
#define PS_SPOOLER	"lp"
#define DEFAULTPATH	"/usr/local/lib:/usr/local/bin:/bin:/usr/bin:/usr/sbin:/usr/ucb:/usr/bsd:/usr/bin/X11:/usr/freeware/bin:/usr/freeware/lib:.:../misc"


#if defined(XlibSpecificationRelease) && defined(sgi) || defined(linux)
#include <X11/Xmu/Editres.h>
#define EDITRES
#endif
#ifndef _XEditResCheckMessages
#undef  EDITRES
#endif

#if defined(PLANGROK)			/* for grok sources (g_*, all ANSI) */
#ifdef ultrix
#define MYCONST				/* use this if cc doesn't know const */
#else
#define MYCONST		const
#endif
#endif

#ifndef PID_T
#define PID_T		pid_t		/* from types.h, see -DPID_T */
#endif

#define BOOL		int
#define TRUE		1
#define FALSE		0

#define TIMER_PERIOD	10*1000		/* autosave and recycle period [ms] */
#define MAXT		0x7fffffff	/* highest possible time_t value */
#define ANCIENT		3*60		/* ignore triggers older than this */
#ifdef BSD
#define HIBERNATE	5*60		/* if no alarm in sight, sleep <5 min*/
#else
#define HIBERNATE	60*60		/* if no alarm in sight, sleep <1 h */
#endif
#define EON		631152000	/* ignore schedule entries before */
					/* this date (sometime early 1990) */
#define NEXC		4		/* max number of exceptions in appt */

#define TIME(t)		((t)%86400)	/* extract time-of-day from date/time*/
#define DATE(t)		((t)/86400)	/* extract date from date/time */

#define PR_MONTH	0		/* values for config.pr_mode */
#define PR_DAY		1
#define PR_WEEK_L	2
#define PR_WEEK_P	3
#define PR_YEAR		4
#define PR_YOV		5

/* gcc doesn't support turning off stupid warning messages selectively */
#if defined(__GNUC__)
#define UNUSED __attribute__((unused))	/* Flag variable as unused */
#else /* not __GNUC__ */
#define UNUSED
#endif


struct config {			/* global configuration data */
	time_t		tzone;		/* timezone with dst figured in */
	BOOL		showicontime;	/* print time into icontitle if nz */
	BOOL		showicondate;	/* print date+time into icontitle */
	BOOL		noiconclock;	/* use icon to display time in clock */
	BOOL		noicon;		/* don't draw anything into the icon */
	BOOL		frame_today;	/* draw black frame into today's box */
	BOOL		sgimode;	/* use SGI desktop style */
	BOOL		smallmonth;	/* large or small month menu? */
	BOOL		nomonthshadow;	/* month only shows days in the month*/
	BOOL		share_mainwin;	/* all views go into mainwindow */
	BOOL		resize_mainwin;	/* auto-resize mainwin on view chg */
	BOOL		webplan;	/* called from cgi-bin, no user */
	char		*webplan_host;	/* called from cgi-bin, netplan host */

	BOOL		sunday_first;	/* TRUE for US style, else European */
	BOOL		ampm;		/* TRUE for US style, else European */
	BOOL		mmddyy;		/* TRUE for m/d/y, FALSE for d.m.y */
	BOOL		autodel;	/* TRUE deletes old entries after 24h*/
	BOOL		julian;		/* TRUE prints julian dates */
	BOOL		weeknum;	/* TRUE prints week numbers */
	BOOL		gpsweek;	/* TRUE prints GPS week numbers */
	BOOL		nopast;		/* TRUE skips todays past daybox notes*/
	BOOL		insecure_script;/* TRUE lets pland run netplan scripts*/
	BOOL		weekwarn;	/* TRUE shows warnings in week view */
	BOOL		weekuser;	/* TRUE shows user names in week bars*/
	BOOL		fullweek;	/* TRUE if week #1 is the one w 7 days*/
	BOOL		thu_week;	/* TRUE if week #1 is the one w Thursd*/
	BOOL		colorcode_m;	/* TRUE shows user color in month view*/
	BOOL		owner_only;	/* TRUE is only owner can write file */
	BOOL		ownonly;	/* TRUE if only own appts in apptmenu*/
	BOOL		moretimecols;	/* TRUE adds time cols in day/weekvw */
	int		notewidth;	/* width of note entry button */
	int		net_port;	/* network daemon IP port */
	BOOL		net_autostart;	/* try to autostart netplan */
	time_t		adjust_time;	/* correction constant for sys clock */
	time_t		raw_tzone;	/* tzone dst NOT figured in */
	int		dst_flag;	/* 0=dst on, 1=dst off, 2=begin..end */
	int		dst_begin;	/* if dst_flag==2, first dst day */
	int		dst_end;	/* if dst_flag==2, last dst day */
	int		dst_begin_time;	/* time of day dst_begin */
	int		dst_end_time;	/* time of day dst_end */

	time_t		early_time;	/* default # of secs for early warn */
	time_t		late_time;	/* default # of secs for late warn */

	int		calbox_xs[2];	/* main window: width of one day */
	int		calbox_ys[2];	/* main window: height of one day */
	int		calbox_marg[2];	/* main window: margin all around */
	int		calbox_arrow[2];/* main window: arrow column width */
	int		calbox_title[2];/* main window: height of wkday row */

	int		year_margin;	/* year window: margin all around */
	int		year_gap;	/* year window: gap between months */
	int		year_title;	/* year window: title line height */
	int		yearbox_xs;	/* year window: day box width */
	int		yearbox_ys;	/* year window: day box height */

	int		day_ndays;	/* day window: # of days in the view */
	int		day_minhour;	/* day window: leftmost hour (8) */
	int		day_maxhour;	/* day window: rightmost hour (24) */
	int		day_margin;	/* day window: margin all around */
	int		day_gap;	/* day window: hgap between day boxes*/
	int		day_title;	/* day window: title line height */
	int		day_headline;	/* day window: date headline height */
	int		day_hourwidth;	/* day window: time column width */
	int		day_hourheight;	/* day window: height for one hour */
	int		day_barwidth;	/* day window: width of an entry bar */

	int		week_ndays;	/* week window: # of days in a week */
	int		week_minhour;	/* week window: leftmost hour (8) */
	int		week_maxhour;	/* week window: rightmost hour (24) */
	int		week_margin;	/* week window: margin all around */
	int		week_gap;	/* week window: gap around day boxes */
	int		week_title;	/* week window: title line height */
	int		week_hour;	/* week window: hour column height */
	int		week_daywidth;	/* week window: weekday title width */
	int		week_hourwidth;	/* week window: weekday title width */
	int		week_barheight;	/* week window: height of appt bar */
	int		week_bargap;	/* week window: gap above/below bar */
	int		week_maxnote;	/* week window: max length of note */
	BOOL		week_bignotime;	/* week window: large notime bars */

	int		yov_nmonths;	/* year ov: 1..12 months in view */
	int		yov_display;	/* year ov: 0=def,1=all,2=own,3=user */
	int		yov_user;	/* year ov: u for "disp user" choice */
	BOOL		yov_single;	/* year ov: show single-day entries */
	int		yov_wwidth;	/* year ov: width of scroll area */
	int		yov_wheight;	/* year ov: height of scroll area */
	int		yov_daywidth;	/* year ov: width of one day */

	BOOL		ewarn_window;	/* TRUE: popup window if early warn */
	BOOL		ewarn_mail;	/* TRUE: send mail if early warn */
	BOOL		ewarn_exec;	/* TRUE: exec program if early warn */
	BOOL		lwarn_window;	/* TRUE: popup window if late warn */
	BOOL		lwarn_mail;	/* TRUE: send mail if late warn */
	BOOL		lwarn_exec;	/* TRUE: exec program if late warn */
	BOOL		alarm_window;	/* TRUE: popup window if alarm time */
	BOOL		alarm_mail;	/* TRUE: send mail if alarm time */
	BOOL		alarm_exec;	/* TRUE: exec program if alarm time */
	char		*ewarn_prog;	/* program to exec at early warn time*/
	char		*lwarn_prog;	/* program to exec at late warn time */
	char		*alarm_prog;	/* program to exec at alarm time */
	char		*mailer;	/* mail command, %s is predef subject*/
	time_t		wintimeout;	/* max lifetime of notifiers */

	int		pr_mode;	/* month, year, week, day: PR_* */
	BOOL		pr_omit_appts;	/* TRUE: omit all apts from printout */
	BOOL		pr_omit_priv;	/* TRUE: omit private appts */
	BOOL		pr_omit_others; /* FALSE: omit everyone else's appts */ 
	int		pr_color;	/* TRUE: generate color PostScript */
	char		*language;	/* 0=English, or "german", "french"..*/
	char		*java;		/* don't lose plan 2.0 configs */
};


/*
 * A single schedule entry. All fields except those after "rest is for internal
 * use" are written to the schedule file.
 * When the server sends an entry to a client, it stamps the entry with <file>
 * and <id> info so it can identify it when the client sends a modified entry.
 */

struct entry {			/* one day-schedule entry */
	char		*message;	/* message text, 0 if none */
	char		*script;	/* shell script, 0 if none */
	char		*note;		/* explicit short note, 0 if none */
	char		*java;		/* don't lose plan 2.0 configs */
	time_t		time;		/* time and date in seconds */
	time_t		length;		/* length in seconds */
	time_t		early_warn;	/* warn this many seconds in advance */
	time_t		late_warn;	/* and this many seconds, too */
	time_t		days_warn;	/* and this many days/86400 before */
	time_t		except[NEXC];	/* don't trigger on these dates */
	time_t		rep_every;	/* repeat every N days (in seconds) */
	time_t		rep_last;	/* stop repeating on this date (secs)*/
	int		rep_weekdays;	/* weekday bitmap, bit 0=mon..6=sun */
					/* bit 8=1st..12=5th, 13=last week */
	int		rep_days;	/* day bitmap, bit 0=last, 1..31=days*/
	BOOL		rep_yearly;	/* repeat every year on the same date*/
	BOOL		suspended;	/* not active, green radio button off*/
	BOOL		private;	/* not accessible for others */
	BOOL		noalarm;	/* no alarm, just warnings if enabled*/
	BOOL		notime;		/* no time is printed in day lists */
	BOOL		hide_in_m;	/* don't show in month view */
	BOOL		hide_in_y;	/* don't show in year view */
	BOOL		hide_in_w;	/* don't show in week view */
	BOOL		hide_in_o;	/* don't show in year overview */
	BOOL		hide_in_d;	/* don't show in day view */
	BOOL		todo;		/* todo item, auto-advance */
	char		acolor;		/* text col: 0=std, 1..8=HBLACK.. */
					/* --- rest is for internal use --- */
	char		triggered;	/* 0=none, 1=early, 2=late, 3=time */
	int		nextrep;	/* nxt repeating entry in chain or -1*/
	char		*user;		/* user name, 0 if from own .dayplan */
	unsigned int	file;		/* unique file ID code for server */
	int		id;		/* unique entry ID code (0=unknown) */
	unsigned short	client;		/* server: if nz, client lock */
	unsigned short	ignore;		/* multiday bar & already in yov */
	int		seq;		/* arbitrary # to ensure stable sort */
	int		grok_row;	/* if grok file, source row number */
};


struct plist {			/* list of entries */
	BOOL		modified;	/* TRUE if modified */
	BOOL		locked;		/* prevents writefile while changing */
	int		nentries;	/* # of valid entries in list */
	int		size;		/* # of entries allocated */
	int		repeating;	/* first repeating entry, -1 if none */
	struct entry	entry[1];	/* first entry. The list is allocated*/
};					/* with space for <size> entries.*/

				/* subset of list, points into struct plist */
struct sublist {			/* list of entries */
	long		nentries;	/* # of valid entries in list */
	long		size;		/* # of entries allocated */
	struct entry	*entry[1];	/* first entry. The list is allocated*/
};					/* with space for <size> entries.*/


/*
 * when we look up an entry, an entry and its trigger time are returned in
 * this struct, which the caller must provide. lookup_entry() also stores
 * other information in this struct that enables lookup_next_entry() to
 * continue the search exactly where lookup_entry() left off.
 */

struct lookup {
					/* the next two are return values */
	long		index;		/* the entry found when looking up */
	time_t		trigger;	/* ... that entry's trigger time */
	BOOL		days_warn;	/* this is an n-days-ahead warning */
					/* the rest is for the next lookup */
	struct plist	*list;		/* in which list were we looking */
	long		regindex;	/* last regular entry found */
	time_t		regtime;	/* ... that entry's trigger time */
	long		repindex;	/* last repeating entry found */
	time_t		reptime;	/* ... that entry's trigger time */
	char		which;		/* 'm,y,w': for month,year,week view */
	int		user;		/* user index in user[], -1=any */
};


/*
 * Information stored about each user in the user list. The user list is
 * an array, because it doesn't change often and it facilitates sorting.
 * Only name, home, suspended, and color are stored in the database file.
 */

struct user {
	char		*name;		/* user name */
	char		*path;		/* home directory, file, or host */
	char		*server;	/* if net.mode > 1, server host */
	BOOL		fromserver;	/* read from server or path? */
	BOOL		prev_fromserver;/* same before editing file list */
	BOOL		grok;		/* <path> is an xmbase-grok form */
	BOOL		vcalendar;	/* <path> is a vCalendar (.ics) file */
	BOOL		modified;	/* an entry needs saving to disk */
	BOOL		readonly;	/* cannot be modified, file is r/o */
	BOOL		suspend_m;	/* ignored in month view */
	BOOL		suspend_w;	/* ignored in week view */
	BOOL		suspend_o;	/* ignored in year overview */
	BOOL		suspend_d;	/* ignored in day view */
	BOOL		alarm;		/* appts of user cause alarms */
	int		color;		/* color 0..7 in the week view */
	time_t		time;		/* time when database was read */
	struct host	*host;		/* server socket connection, 0=none */
	int		file_id;	/* open server file handle, or -1 */
#if defined(PLANGROK)
	struct form	*form;		/* describes structure of grok db */
	struct dbase	*dbase;		/* 2D string data array of grok db */
#endif
};


/*
 * info about a connected server, maintained in network.c.
 */

struct host {
	struct host	*next;		/* next in host chain */
	char		*name;		/* host name */
	BOOL		open;		/* true if server has opened file */
	int		fd;		/* socket connection to server */
	XtInputId	x_id;		/* registered X event */
};
#endif /* _CONFIG_H */
