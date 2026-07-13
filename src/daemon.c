/*
 * separate daemon program that reads the database and waits for events to
 * trigger, and pops up menus or whatever the user specified. When a cycling
 * event triggers, it is advanced in-core, but nothing is ever written back
 * to the database file. The daemon re-reads the database whenever it
 * receives a SIGHUP signal, which is sent by the main program after it
 * writes the database. The daemon writes its pid to /tmp/.pland<UID>, or
 * terminates if the file already exists. The purpose of this lockfile is
 * to prevent multiple daemons, and to tell the plan program who to send
 * the SIGHUP to.
 *
 * This program uses the $PATH environment variable to find programs.
 *
 * Author: thomas@bitrot.de (Thomas Driemeyer)
 */

			/* dummies for prototype declarations in config.h */
typedef void		*Widget;
typedef void		*XtPointer;
typedef void		*XPoint;
typedef unsigned long	XtInputId;
typedef unsigned short	Dimension;
typedef int		Region;
typedef void		*XEvent;		/* open Motif */
typedef int		Boolean;		/* open Motif */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include "cal.h"
#include "version.h"
#include <pwd.h>
#ifdef __EMX__
#include <io.h>
#else
#include <utmp.h>
#endif
#ifdef AIXV3
struct utmp *getutent();
#endif
#ifndef NOLIMITS
#include <limits.h>
#endif


#ifndef SIGCHLD			/* for BSD systems */
#define SIGCHLD SIGCLD
#endif
#ifndef PIPE_MAX		/* writing this many bytes never blocks */
#define PIPE_MAX 512
#endif
#if defined(BSD)||defined(MIPS)	/* detach forked process from terminal */
#define ORPHANIZE setpgrp(0,0)	/* session, and attach to pid 1 (init). */
#else				/* This means we'll get no ^C and other */
#define ORPHANIZE setsid()	/* signals. If -DBSD doesn't help, the */
#endif				/* daemon works without, sort of. */
#if defined(SUN)||defined(BSD)||defined(SVR4) /* use itimer instead of sleep */
#define ITIMER			/* systems, because not only may sleep be */
#endif				/* uninterruptible, it may lose SIGHUP. */
#ifdef ITIMER
#include <sys/time.h>
#endif
#ifdef MIPS
extern struct passwd *getpwuid();
extern struct utmp *getutent();
#endif
#ifdef PLANGROK
void *toplevel = 0;		/* dummy for formfile.c */
#endif

static void popup_window	(int, struct entry *);
static void send_mail		(int, struct entry *);
static void reaper		(int);
static void exec_program	(struct entry *, char *, BOOL);
static void write_script	(char *, char *);
#ifdef JAPAN
static int chkctype		(unsigned char, int);
static unsigned short e2j	(unsigned short);
static unsigned short sj2j	(unsigned short);
static unsigned char *mimeencode(unsigned char *, unsigned short *,
							unsigned short *);
static void sprintmime		(unsigned char *, unsigned char *);
#endif
static char *get_subject	(int, struct entry *, BOOL);
static char *get_subtitle	(struct entry *);
static char *get_icontitle	(int, struct entry *);
static void sighand		(int);
static void usage		(void);
static void alert		(struct entry *, int);
void create_error_popup		(void *, int, char *, ...);
time_t read_last_alarm          (void);
void update_last_alarm          (void);
static char *expand_percent	(struct entry *, char *);

char			*progname;	/* argv[0] */
extern char		lockpath[];	/* lockfile path (from lock.c) */
struct config		config;		/* global configuration data */
struct plist		*mainlist;	/* list of all schedule entries */
extern time_t		cutoff;		/* all triggers before this are done */
static BOOL		reread;		/* caught SIGHUP, re-read mainlist */
BOOL			debug;		/* print debugging information */
static BOOL		opt_t;		/* TRUE if -t (tty) */
static time_t		last_alarm;	/* Last time alarm was checked for */

int			curr_year;	/* dummy for DST calculating routines*/
Widget			mainwindow;	/* dummy for error popup in network.c*/
					/* dummy functions, never used: */
int	  write_one_entry    (UNUSED int (*a)(), UNUSED struct entry *e)
							{return(0);}
XtInputId register_X_input   (UNUSED int a)		{return(0);}
void	  unregister_X_input (UNUSED XtInputId a)	{}


int main(
	int			argc,
	char			**argv)
{
	PID_T			pid;		/* pid in lockfile */
	int			reason=0;	/* what triggers next? */
	time_t			now, twait;	/* now and wait for trigger */
	time_t			total_wait;
	time_t			ancient=ANCIENT;/* initially accept past trig*/
	struct entry		*entry;		/* next entry that triggers */
	BOOL			opt_k = FALSE;	/* TRUE if -k (kill & run) */
	BOOL			opt_K = FALSE;	/* TRUE if -K (kill & exit) */
	BOOL			opt_l = TRUE;	/* TRUE if -l (watch logout) */
	BOOL			opt_s = FALSE;	/* TRUE if -s (silent) */
	BOOL			opt_p = FALSE;	/* TRUE if -p (missed alarms) */
	struct passwd		*pw = 0;	/* user's password entry */
	BOOL			got_login=FALSE;/* was user ever logged in? */
	BOOL			logged_in=TRUE;	/* found in utmp? */
#ifdef ITIMER
 	struct itimerval	itv;		/* for sleep() replacement */
#endif
#ifdef RABBITS
	opt_l = FALSE;
#endif

	progname = argv[0];
	while (argc-- > 1)
		if (**++argv == '-') {
			while (*++*argv)
				switch(**argv) {
				  case 'k': opt_k = TRUE;  break;
				  case 'K': opt_K = TRUE;  break;
				  case 'd': debug = TRUE;  break;
				  case 'l': opt_l = TRUE;  break;
				  case 'L': opt_l = FALSE; break;
				  case 's': opt_s = TRUE;  break;
				  case 't': opt_t = TRUE;  break;
				  case 'p': opt_p = TRUE;  break;
				  default:  usage();
				}
		} else
			usage();

	if ((opt_l || opt_s) && !(pw = getpwuid(getuid())))
		fprintf(stderr, _("%s: WARNING: can't read user name.\n%s"),
				progname, _("Try running with -L option.\n"));
	if (!debug) {
		(void)umask(0077);
		setuid(getuid());
		setgid(getgid());
		if ((pid = fork()) > 0) {
			sleep(1);
			exit(0);
		}
		if (pid == -1)
			fprintf(stderr, _("%s: Warning: cannot fork\n"),
								progname);
		else
			ORPHANIZE;
	}
	signal(SIGHUP,  sighand);
	signal(SIGINT,  sighand);
	signal(SIGQUIT, sighand);		/* comment out any of */
	signal(SIGILL,  sighand);		/* these if undefined */
	signal(SIGBUS,  sighand);		/* */
	signal(SIGSEGV, sighand);		/* */
	signal(SIGTERM, sighand);		/* */
	signal(SIGPIPE, SIG_IGN);		/* survive death of notifier */
#ifdef ITIMER
	signal(SIGALRM, sighand);
#endif
	if (!startup_lock(TRUE, opt_k || opt_K)) {
		if (opt_k || opt_K)
			fatal(_("cannot kill existing %s daemon"), progname);
		else
			fatal(_("another daemon exists, use %s -k"), progname);
	}
	if (opt_K) {
		(void)unlink(lockpath);
		exit(0);
	}
	if (opt_p)
		ancient = read_last_alarm();

	/*
	 * main loop
	 */

	tzset();
	set_tzone();
	now = get_time();
	create_list(&mainlist);
	read_mainlist();
	(void)recycle_all(mainlist, FALSE, 0);
#ifdef ITIMER
	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = 0;
	itv.it_value.tv_usec = 0;
#endif
	for (;;) {
	    if (reread) {
		if (debug)
		    printf(_("%s: re-reading database\n"), progname);
		destroy_list(&mainlist);
		create_list(&mainlist);
		read_mainlist();
		(void)recycle_all(mainlist, FALSE, 0);
		reread = FALSE;
	    }
	    (void)recycle_all(mainlist, FALSE, 0);

	    cutoff = now - ancient;

	    if (!(opt_s && !logged_in)) {
		set_tzone();
		last_alarm = now = get_time();
		reason = find_next_trigger(mainlist, now, &entry, &twait);
		if (reason && twait <= 0) {
		    alert(entry, reason);
		    entry->triggered = reason;
		    continue;
		}
	    }
	    ancient = 0;
	    if (!reason)
		twait = HIBERNATE;
	    total_wait = twait;
	    while (total_wait && !reread) {
		twait = total_wait > 1*60 ? 1*60 : total_wait;
		if (debug)
		    printf(_("%s: sleeping for %d:%02d:%02d\n"), progname,
			   (int)twait/3600, (int)(twait/60)%60, (int)twait%60);
#ifdef ITIMER
		itv.it_value.tv_sec = twait;
		setitimer(ITIMER_REAL, &itv, NULL);
#if defined(SVR4) || defined(SOLARIS2)
		sigpause(SIGALRM);
#else
		sigpause(0);
#endif
#else
		sleep(twait);
#endif
		refresh_lock(resolve_tilde(PLANDLOCK));
		logged_in = FALSE;
		if (pw) {
#if defined(SUN) || defined(BSD) || defined(__FreeBSD__)
		    struct utmp	ut;
		    int		fd;
		    int		l;

		    fd = open("/etc/utmp", 0);
		    if (fd >= 0) {
			while ((l = read(fd, &ut, sizeof(ut))) == sizeof(ut)) {
			    if (ut.ut_name[0]			&&
			       (!strncmp(ut.ut_line, "tty", 3)	&&
			        strchr("pqrs", ut.ut_line[3]) ||
				!strncmp(ut.ut_line, "console", 7) &&
				!strncmp(pw->pw_name, ut.ut_name, 8))){
				    logged_in = TRUE;
				    break;
			    }
			}
			close(fd);
		    }
#elif defined(ULTRIX) || defined(MACOSX)
		    logged_in = TRUE;
#else
		    short pid = getpid();
		    struct utmp *u;
		    setutent();
		    while ((u = getutent()))
			if (u->ut_type == USER_PROCESS &&
			    u->ut_pid != pid &&
			    !strncmp(pw->pw_name, u->ut_user, 8)) {
				logged_in = TRUE;
				break;
			}
#endif
		}
		if (got_login && !logged_in && !opt_s) {
		    printf(_("%s: you have logged out, exiting (try -L?)\n"),
							    progname);
		    (void)unlink(lockpath);
		    exit(0);
		}
		if (logged_in) {
		    got_login = TRUE;
		    if (debug)
			printf(_("%s: utmp works, watching for logout\n"),
							    progname);
		} else if (opt_s && debug)
			printf(_("%s: logged out, keeping quiet\n"), progname);
		total_wait -= twait;
	    }
	}
}


/*--------------------------------- warn/alarm action -----------------------*/
/*
 * An entry triggered, because its early-warn time, late-warn time, or alarm
 * time was reached (reason is 1, 2, or 3, respectively). Do whatever was
 * requested. Don't exec scripts that came from a server unless explicitly
 * allowed, too insecure.
 */

static void alert(
	struct entry	*entry,		/* entry that triggered */
	int		reason)		/* why: 1=early,2=late,3=time*/
{
	char		cmd[4096];	/* command to execute */

	signal(SIGPIPE, SIG_IGN);		/* survive death of notifier */
	switch(reason) {
	  case 1:
		if (config.ewarn_window)
			popup_window(reason, entry);
		if (config.ewarn_mail)
			send_mail(reason, entry);
		if (config.ewarn_exec) {
			sprintf(cmd, config.ewarn_prog, entry->note ?
				expand_percent(entry, entry->note) : "");
			exec_program(entry, cmd, FALSE);
		}
		break;

	  case 2:
		if (config.lwarn_window)
			popup_window(reason, entry);
		if (config.lwarn_mail)
			send_mail(reason, entry);
		if (config.lwarn_exec) {
			sprintf(cmd, config.lwarn_prog, entry->note ?
				expand_percent(entry, entry->note) : "");
			exec_program(entry, cmd, FALSE);
		}
		break;

	  case 3:
		if (config.alarm_window && !entry->noalarm)
			popup_window(reason, entry);
		if (config.alarm_mail && !entry->noalarm)
			send_mail(reason, entry);
		if (config.alarm_exec && !entry->noalarm) {
			sprintf(cmd, config.alarm_prog, entry->note ?
				expand_percent(entry, entry->note) : "");
			exec_program(entry, cmd, FALSE);
		}
		if (entry->script) {
			if (!config.insecure_script &&
						(!entry->user || !entry->id))
				exec_program(entry, expand_percent(entry,
							entry->script), TRUE);
			else
				fprintf(stderr,
					_("%s: ignoring insecure script"),
								progname);
		}
	}
}


/*
 * pop up a window with the note text in it. This is done in a separate
 * program that gets the text as standard input, to avoid having to drag
 * in X stuff into the daemon. It is large enough as it is. The reason
 * argument determines the color of the popup.
 */

static void popup_window(
	int		reason,		/* why: 1=early,2=late,3=time*/
	struct entry	*entry)		/* entry that triggered */
{
	char		prog[1024];	/* path of notifier program */
	char		cmd[2048];	/* command string */
	int		timeout = (int)config.wintimeout;

	if (opt_t) {
		printf("*** %s\n*** %s\n", get_subject(reason, entry, FALSE),
					   get_subtitle(entry));
		return;
	}
	if (!find_file(prog, ALARM_FN, TRUE)) {
		fprintf(stderr, _("%s: %s: not found\n"), progname, ALARM_FN);
		return;
	}
	/* kill early warn window when late warn window appears */
	if (reason == 1 && entry->late_warn
			&& (timeout < 60 ||
			    timeout > entry->early_warn - entry->late_warn))
		timeout = entry->early_warn - entry->late_warn;

	/* kill early warn window when main alarm appears */
	if (reason == 1 && !entry->noalarm
			&& (timeout < 60 || timeout > entry->early_warn))
		timeout = entry->early_warn;

	/* kill late warn window when main alarm appears */
	if (reason == 2 && !entry->noalarm
			&& (timeout < 60 || timeout > entry->late_warn))
		timeout = entry->late_warn;

	if (timeout > 59)
		sprintf(cmd, "%s -%d -e%d -t\1%s\2 -s\1%s\2 -i\1%s\2",
					prog, reason,
					timeout/60,
					get_subject(reason, entry, FALSE),
					get_subtitle(entry),
					get_icontitle(reason, entry));
	else
		sprintf(cmd, "%s -%d -t\1%s\2 -s\1%s\2 -i\1%s\2",
					prog, reason,
					get_subject(reason, entry, FALSE),
					get_subtitle(entry),
					get_icontitle(reason, entry));
	exec_program(entry, cmd, FALSE);
}


/*
 * send the entry's text to some user, as a mail message. The reason field
 * is used for composing the Subject. If the entry came from a server, don't
 * send mail because that would be a security hole.
 */

static void send_mail(
	int		reason,		/* why: 1=early,2=late,3=time*/
	struct entry	*entry)		/* entry that triggered */
{
	char		cmd[2048];	/* command string */
	char		subject[512];	/* quoted subject */

	if (!config.mailer || !*config.mailer) {
		fprintf(stderr, _("%s: no mailer defined\n"), progname);
		return;
	}
	strcpy(subject+1, get_subject(reason, entry, TRUE));
	subject[0] = 1;
	strcat(subject, "\2");
	sprintf(cmd, config.mailer, subject);
	exec_program(entry, cmd, FALSE);
}


/*
 * DateBk4 (a PalmPilot application) synced messages may have extra info.
 * Skip it. By Johan Vromans <jvromans@squirrel.nl>
 */

static char *get_message(
	struct entry	*entry)
{
	if (entry->message) {
		char *msg = expand_percent(entry, entry->message);
		int l = strlen(msg);
		if (l >= 20 && msg[0] == '#' && msg[1] == '#' && msg[19]=='\n')
			if (l > 20)
				return(msg + 20);
			else
				return(NULL);
		else
			return(msg);
	}
	return(NULL);
}


/*
 * execute a program, and pass the entry's message or note text as standard
 * input. The program is executed as a child process, and reads the message
 * from a pipe. The pipe is written to by a second child (unless the second
 * fork fails, in which case the parent writes to the pipe) to avoid blocking
 * the parent if the message is large and the child isn't really interested
 * in the message. Blocking would freeze the daemon and could lose triggers.
 * If the message is short (fits into one pipe write), don't use a 2nd child.
 */

/*ARGSUSED*/
static void reaper(
	UNUSED int		sig)
{
	char			path[40];	/* script to delete */
#if (defined BSD && !defined OSF && !defined __FreeBSD__)
	union wait		dummy;
#else
	int			dummy;
#endif
	sprintf(path, "/tmp/pland%d", wait(&dummy));
	if (debug)
		printf(_("%s: deleting script \"%s\"\n"), progname, path);
	(void)unlink(path);
	signal(SIGCHLD, reaper);
}


static void exec_program(
	struct entry	*entry,		/* entry that triggered */
	char		*program,	/* program or script to exec */
	BOOL		script)		/* run program in a subshell */
{
	int		fd[2];		/* pipe to child process */
	PID_T		pid;		/* child process id */
	char		*p = program;	/* source command line */
	char		*q = program;	/* target command line */
	char		*argv[100];	/* argument vector */
	int		argc = 0;	/* argument counter */
	char		scriptname[40];	/* if script, temp file name */
	int		quote;		/* argument quote flags */
	char		*msg;		/* msg to print */
	long		msgsize;	/* strlen(msg) */
	char		progpath[1024];	/* path name of executable */

	if (!script) {				/* build arguments */
		if (!p) return;
		while (argc < 99) {
			argv[argc++] = q;
			quote = 0;
			while (*p && (quote | *p) != ' ') {
				switch(*p) {
				  case '\'':
					 if (!(quote & 0x600))
						quote ^= 0x100;
					 break;
				  case '"':
					 if (!(quote & 0x500))
						quote ^= 0x200;
					 break;
				  case 1:
					 quote |= 0x400;
					 break;
				  case 2:
					 quote &= ~0x400;
					 break;
				  case '\\':
					if (!quote && p[1])
						p++;
				  default:
					*q++ = *p;
				}
				p++;
			}
			if (!*p++)
				break;
			*q++ = 0;
			while (*p == ' ')
				p++;
		}
		*q = 0;
		argv[argc] = 0;

		if (!find_file(progpath, argv[0], TRUE)) {
			FILE *err;
			if ((err = fopen("/dev/console", "w"))) {
				fprintf(err, _("%s: %s: not found\n"),
							progname, argv[0]);
				fclose(err);
			}
			return;
		}
		if (debug) {
			int i;
			printf(_("%s: running \"%s\", args"), progname,
								progpath);
			for (i=0; i < argc; i++)
				printf(" \"%s\"", argv[i]);
			printf("\n");
		}
	}
	if (pipe(fd)) {				/* pipe from child 2 to ch 1 */
		perror(progname);
		return;
	}
	signal(SIGCHLD, reaper);
	if ((pid = fork()) < 0) {
		perror(progname);
		return;
	}
	if (!pid) {				/* child 1, execs program */
		close(0);
		dup(fd[0]);
		close(fd[0]);
		close(fd[1]);
		ORPHANIZE;
		if (script) {
			write_script(scriptname, program);
			strcpy(progpath, scriptname);
			argv[0] = scriptname;
			argv[1] = 0;
		}
		execv(progpath, argv);
		perror(argv[0]);
		exit(0);
	}
	pid = 1;

	/* DateBk4 (a PalmPilot application) synced messages may have rather */
	/* uninteresting messages. From Johan Vromans <jvromans@squirrel.nl> */
	if (!(msg = get_message(entry)))
		msg = entry->note ? expand_percent(entry, entry->note) : "";
	msgsize = strlen(msg);

	if (msgsize <= PIPE_MAX ||		/* child 2, feeds child 1 */
	    (pid = fork()) <= 0) {
		close(fd[0]);
		(void)write(fd[1], msg, msgsize);
		close(fd[1]);
		if (!pid)
			exit(0);
	} else {				/* parent, do nothing */
		close(fd[0]);
		close(fd[1]);
	}
}

/*
 * write script to a file. The script name is generated from the process ID,
 * so that reaper() knows which script to delete after this process terminates.
 * The buffer for the script name is in the calling routine; it has to know
 * because it is going to exec it. If something goes wrong, exit; we are in
 * a child here. (Actually, this is what makes the whole thing look difficult:
 * the script can't be written by the daemon, it must be written by the
 * child that execs, because the script file name contains the child's PID,
 * which isn't known before the fork. Putting the PID into the script file
 * name makes it easy to delete the script file when the child dies, without
 * keeping track of which child runs which script.)
 * If the first line does not begin with "#!", "#!/bin/sh\n" is inserted.
 */

static void write_script(
	char			*scriptname,	/* script name buffer */
	char			*program)	/* script text */
{
	int			len;		/* size of program */
	int			fd;		/* script file */

	sprintf(scriptname, "/tmp/pland%d", getpid());
	if (debug)
		printf(_("%s: generating script \"%s\"\n"), progname,
								scriptname);
	if ((fd = open(scriptname, O_WRONLY | O_CREAT | O_TRUNC, 0777)) < 0) {
		int err = errno;
		fprintf(stderr, _("%s: can't create %s: "), progname,
								scriptname);
		errno = err;
		perror("");
		exit(1);
	}
	len = strlen(program);
	if ((program[0] != '#' && program[1] != '!' &&
	    write(fd, "#!/bin/sh\n", 10) != 10) ||
	    write(fd, program, len) != len) {
		int err = errno;
		fprintf(stderr, _("%s: can't write to %s: "), progname,
								scriptname);
		errno = err;
		perror("");
		close(fd);
		(void)unlink(scriptname);
		exit(1);
	}
	(void)write(fd, "\n", 1);
	close(fd);
}

#ifdef JAPAN

#ifndef ESCTOASCII
#define ESCTOASCII	"\033(J"	/* Switching to JIS X0201-1976 roman. */
#endif
#ifndef LENTOASCII
#define LENTOASCII	3		/* Length of the escape-sequence. */
#endif

#define ESCTOKANJI	"\033$B"	/* Switching to Kanji. */
#define LENTOKANJI	3		/* Length of the escape-sequence. */
#define MIMELEAD	"=?ISO-2022-JP?B?"
#define LENMIMELEAD	16
#define MIMETRAIL	"?="
#define LENMIMETRAIL	2

#define CT_ASC	0	/* ASCII character */
#define CT_EUC1	1	/* The 1st byte of EUC character */
#define CT_EUC2	2	/* The 2nd byte of EUC character */
#define CT_SJ1	3	/* The 1st byte of SJIS character */
#define CT_SJ2	4	/* The 2nd byte of SJIS character */
#define CT_KJ1	5	/* The 1st byte of EUC or SJIS character */
#define CT_KJ2	6	/* The 2nd byte of EUC or SJIS character */
#define CT_UHOH	7

/* Conversion table for MIME base64 (RFC1521) */
static unsigned char base64[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
	'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', '+', '/'
};

/*
 * chkctype() and e2j() is originally coded by
 * NISHIJIMA, Takanori (racsho@cpdc.canon.co.jp).
 * Ogura Yoshito changed chkctype().
 */

static int chkctype(
	unsigned char	ch,
	int		stat)
{
	switch (stat) {
	  case CT_ASC:
	  case CT_SJ2:
	  case CT_KJ2:
	  case CT_EUC2:
		if (ch <= 0x7f)
			return CT_ASC;
		else if (ch >= 0x81 && ch <= 0x9f)
			return CT_SJ1;
		else if (ch >= 0xa1 && ch < 0xe0 || ch == 0xfd || ch == 0xfe)
			return CT_EUC1;
		else if (ch >= 0xe0 && ch <= 0xfc)
			return CT_KJ1;
		else
			return CT_UHOH;
		break;
	  case CT_KJ1:
		if (ch >= 0x40 && ch <= 0x7e || ch >= 0x80 && ch < 0xa1)
			return CT_SJ2;
		else if (ch == 0xfd || ch == 0xfe)
			return CT_EUC2;
		else if (ch >= 0xa1 && ch <= 0xfc)
			return CT_KJ2;
		else
			return CT_UHOH;
		break;
	  case CT_SJ1:
		if (ch >= 0x40 && ch <= 0x7e || ch >= 0x80 && ch <= 0xfc)
			return CT_SJ2;
		else
			return CT_UHOH;
		break;
	  case CT_EUC1:
		if (ch >= 0xa1 && ch <= 0xfe)
			return CT_EUC2;
		else
			return CT_UHOH;
		break;
	  default:
		return CT_UHOH;
	}
}


static unsigned short e2j(
	unsigned short	wc)
{
	return(wc & 0x7F7F);
}


/*
 *  sj2j() is coded with referring to nkf.
 * nkf, Network Kanji Filter
 * AUTHOR: Itaru ICHIKAWA (ichikawa@flab.fujitsu.co.jp)
 */

static unsigned short sj2j(
	unsigned short	wc)
{
	return (((((wc >> 8) << 1) - (((wc >> 8) > 0x9f) << 7) - 0xe1) << 8) +
		((wc & 0x00ff) >= 0x9f ? (wc & 0x00ff) - 0x7e | 0x100
				       : (wc & 0x00ff) - 0x1f
						       - ((wc & 0x00ff) >> 7)));
}


static unsigned char *mimeencode(
	unsigned char	*p,
	unsigned short	*jis,
	unsigned short	*end)
{
	unsigned char	*tmp, *begin, buf[80];
	int		i;

	strcpy(p, MIMELEAD);
	begin = p += LENMIMELEAD;
	strcpy(tmp = buf, ESCTOKANJI);
	tmp += LENTOKANJI;
	while (jis < end) {
		*tmp++ = *jis >> 8;
		*tmp++ = *jis++;
	}
	strcpy(tmp, ESCTOASCII);
	tmp += LENTOASCII;
	for (i=0; i < tmp - buf; i++) {
		switch (i % 3) {
		  case 0:
			*p++ = base64[buf[i] >> 2];
			break;
		  case 1:
			*p++ = base64[buf[i] >> 4 | (buf[i-1] & 0x03) << 4];
			break;
		  case 2:
			*p++ = base64[buf[i] >> 6 | (buf[i-1] & 0x0f) << 2];
			*p++ = base64[buf[i] & 0x3f];
			break;
		}
	}
	switch (i % 3) {
	  case 1:
		*p++ = base64[(buf[i-1] & 0x03) << 4];
		break;
	  case 2:
		*p++ = base64[(buf[i-1] & 0x0f) << 2];
		break;
	}
	while (p - begin & 0x03)
		*p++ = '=';	/* padding for base64 */
	strcpy(p, MIMETRAIL);
	p += LENMIMETRAIL;
	return (p);
}


static void sprintmime(
	unsigned char	*dptr,
	unsigned char	*sptr)
{
	unsigned char	*p = sptr;
	unsigned short	*wcp, wc, buf[40];	/* works for a 2-byte char */
	int		kanjicode = CT_EUC1, prev_ct = CT_ASC, stat = CT_ASC;

	if (sptr == NULL) {
		*dptr = '\0';
		return;
	}
	while (*sptr != '\0') {
		stat = chkctype(*sptr++, stat);
		if (stat == CT_SJ1 || stat == CT_SJ2) {
			kanjicode = CT_SJ1;
			break;
		} else if (stat == CT_EUC1 || stat == CT_EUC2) {
			kanjicode = CT_EUC1;
			break;
		}
	}
	stat = chkctype(*(sptr = p), CT_ASC);
	wcp = buf;
	while (*sptr != '\0') {
		switch (prev_ct = chkctype(*sptr, prev_ct)) {
		  case CT_ASC:
			if (stat != CT_ASC) {
				stat = prev_ct;
				dptr = mimeencode(dptr, buf, wcp);
				wcp = buf;
			}
			*dptr++ = *sptr++;
			break;
		  case CT_SJ1:
		  case CT_KJ1:
		  case CT_EUC1:
			if (stat == CT_ASC)
				stat = prev_ct;
			wc = *sptr++ << 8;
			break;
		  case CT_SJ2:
		  case CT_KJ2:
		  case CT_EUC2:
			wc |= *sptr++;
			*wcp++ = (kanjicode == CT_SJ1) ? sj2j(wc)
						       : e2j(wc);
			break;
		  default:
			*dptr = '\0';
			return;		/* Illegal pointer. */
		}
	}
	if (wcp != buf)
		dptr = mimeencode(dptr, buf, wcp);
	*dptr = '\0';
}
#endif


/*
 * Return some descriptive message, to be used as a Subject or header text
 * to tell the user what is happening. At most 79 characters are returned.
 */

static char *get_subject(
	int		reason,		/* why: 1=early,2=late,3=time*/
	struct entry	*entry,		/* entry that triggered */
	BOOL		withnote)	/* append note string too */
{
	static char	msg[512];
	time_t		time = (entry->time % 86400) / 60;
	char		ampm = 0;

	if (config.ampm) {
		ampm = time < 12*60 ? 'a' : 'p';
		time %= 12*60;
		if (time < 60)
			time = 12*60;
	}
	sprintf(msg, reason==1 || reason==2 ? _("Warning: alarm at %d:%02d%c")
					    : _("ALARM at %d:%02d%c"),
	 					       time/60, time%60, ampm);
	if (withnote && entry->note != 0) {
		int len = strlen(msg);
#ifdef JAPAN
		msg[len++] = ' ';
		msg[len++] = '(';
		sprintmime(msg+len, expand_percent(entry, entry->note));
#else
		sprintf(msg+len, " (%.50s", expand_percent(entry,entry->note));
#endif
		len = strlen(msg);
		if (msg[len-1] == '\n')
			len--;
		msg[len] = ')';
		msg[len+1] = 0;
	}
	return(msg);
}



/*
 * Return a subtitle string with extra info for notifier window
 */

static char *get_subtitle(
	struct entry	*entry)		/* entry that triggered */
{
	static char	msg[160];
	time_t		time;
	char		ampm = 0;

	*msg = 0;
	if (entry->user)
		sprintf(msg, _("user %s"), entry->user);

	if (entry->length) {
		time = ((entry->time + entry->length) % 86400) / 60;
		if (config.ampm) {
			ampm = time < 12*60 ? 'a' : 'p';
			time %= 12*60;
			if (time < 60)
				time = 12*60;
		}
		if (*msg)
			strcat(msg, ": ");
		sprintf(msg+strlen(msg), _("length %d:%02d, until %d:%02d%c"),
				entry->length/3600, (entry->length/60)%60,
				time/60, time%60, ampm);
	}
	if (entry->note && get_message(entry)) {
		if (*msg)
			strcat(msg, " - ");
		strncpy(msg+strlen(msg), expand_percent(entry,entry->note),80);
	}
	return(msg);
}



/*
 * Return a short title string for the notifier icon title. It only
 * contains the time of the trigger, and "early" or "late" flags.
 */

static char *get_icontitle(
	int		reason,		/* why: 1=early,2=late,3=time*/
	struct entry	*entry)		/* entry that triggered */
{
	static char	msg[20];
	time_t		time = (entry->time % 86400) / 60;
	char		ampm = 0;

	if (config.ampm) {
		ampm = time < 12*60 ? 'a' : 'p';
		time %= 12*60;
		if (time < 60)
			time = 12*60;
	}
	sprintf(msg, "%d:%02d%c", (int)(time/60), (int)(time%60), ampm);
	if (reason == 1)
		strcat(msg, " (E)");
	if (reason == 2)
		strcat(msg, " (L)");
	return(msg);
}


/*
 * given a string, expand %X codes with strings from the entry. This is used
 * for messages and scripts, so that they can contain times and lengths etc.
 */

static char *expand_percent(
	struct entry	*entry,		/* entry that triggered */
	char		*text)		/* string with %X to replace */
{
	static char	out[4096];	/* put result with substitutions here*/
	char		*p, *q;		/* source and ragte copy pointers */

	if (!strchr(text, '%'))		/* no '%': nothing to do */
		return(text);
	for (p=text, q=out; *p; p++) {
		char *replace = 0;
		if (*p !='%') {
			if (q < out + sizeof(out)-1)
				*q++ = *p;
			continue;
		}
		switch(*++p) {
		  case '%': replace = "%";				break;
		  case 'M': replace = entry->message;			break;
		  case 'S': replace = entry->script;			break;
		  case 'N': replace = entry->note;			break;
		  case 'D': replace = mkdatestring(entry->time);	break;
		  case 'T': replace = mktimestring(entry->time, FALSE);	break;
		  case 'L': replace = mktimestring(entry->length,TRUE);	break;
		  case 'U': replace = getenv("USER");			break;
		  case 'F': replace = entry->private ? "private"   :
				      entry->user    ? entry->user
						     : getenv("USER");	break;
		}
		/* got a nonempty %X expansion: append to out */
		if (replace) {
			int len = strlen(replace);
			if (q + len >= out + sizeof(out)-1)
				len = sizeof(out)-1 - (q - out);
			if (len > 0) {
				strncpy(q, replace, len);
				q += len;
			}
		}
	}
	*q = 0;
	return out;
}


/*--------------------------------- misc ------------------------------------*/
/*
 * signal handler. SIGHUP re-reads the database by interrupting the sleep();
 * all others terminate the program and delete the lockfile.
 */

static void sighand(
	int			sig)		/* signal type */
{
#ifdef ITIMER
	if (sig == SIGALRM)
		signal(SIGALRM, sighand);
	else
#endif
	if (sig == SIGHUP) {				/* re-read database */
		signal(SIGHUP, sighand);
		reread = TRUE;
	} else {					/* die */
		(void)unlink(lockpath);
		fprintf(stderr, _("%s: killed with signal %d\n"),progname,sig);
		update_last_alarm();
		exit(0);
	}
}


/*
 * an error occurred, probably a file or network error. No popups here,
 * print to stderr.
 */

void create_error_popup(
	UNUSED void	*widget,
	UNUSED int	error,
	char		*fmt, ...)
{
	va_list		parm;
	fprintf(stderr, "%s: ", progname);
	va_start(parm, fmt);
	vfprintf(stderr, fmt, parm);
	va_end(parm);
#	if defined(sgi) || defined(_sgi)
	if (error) {
		fprintf(stderr, "\n");
		fprintf(stderr, sys_errlist[error]);
	}
#	endif
}


/*
 * record the last time an alarm was check for in the ~/.pland file so it can
 * be referenced on the next run, by Brian L. Shaver <shaker.lxxv@verizon.net>
 */

void update_last_alarm(void)
{
	FILE		*fp;

	if ((fp = fopen(resolve_tilde(PLAND_PATH), "w"))) {
		fprintf(fp, "%ld", last_alarm);
		if (debug)
			printf(_("recorded last alarm: %ld\n"), last_alarm);
		fclose(fp);
	} else
		if (debug)
			printf("%s",_("unable to update last alarm\n"));
}


/*
 * read the last alarm check time so that alarms which may have been
 * missed can be picked up when pland is restarted.
 */

time_t read_last_alarm(void)
{
	time_t		result;
	FILE		*fp;

	if ((fp = fopen(resolve_tilde(PLAND_PATH), "r"))) {
		fscanf(fp,"%ld",&result);
		if (debug)
			printf(_("reading last alarm: %ld\n"), result);
		result = get_time() - result;	/* compute cutoff difference */
		if (debug)
			printf(_("initial alarm cutoff: %ld\n"), result);
		fclose(fp);
	} else {
		result = ANCIENT;
		if (debug)
			printf("%s", _("last alarm not present\n"));
	}
	return(result);
}


/*
 * usage message if incorrect options are given
 */

static void usage(void)
{
	fprintf(stderr, _("%s: plan calendar daemon, version %s.\n"),
					progname, VERSION JAPANVERSION);
	fprintf(stderr, "%s", _("Usage: pland [-dkKtslL]\n"));
	fprintf(stderr, "%s", _("-d\tdebug: stay in foreground, print actions\n"));
	fprintf(stderr, "%s", _("-k\tkill existing daemon, then continue\n"));
	fprintf(stderr, "%s", _("-K\tkill existing daemon only\n"));
	fprintf(stderr, "%s", _("-t\tdon't pop up notifier windows, print to stdout\n"));
	fprintf(stderr, "%s", _("-s\twatch utmp and sleep while user is logged out\n"));
	fprintf(stderr, "%s", _("-l\twatch utmp and exit when user logs out\n"));
	fprintf(stderr, "%s", _("-L\tdon't watch utmp (utmp is often buggy)\n"));
	fprintf(stderr, "%s", _("-p\treport missed alarms when starting.\n"));
	fprintf(stderr, "%s",
#	ifdef RABBITS
		_("-L is the default. Try -l if you get too many pland's.\n"));
#	else
		_("-l is the default. Try -d or -L if pland exits early.\n"));
#	endif
	exit(1);
}
