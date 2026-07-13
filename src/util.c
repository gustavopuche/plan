/*
 * various and sundry little helper functions, some of them to replace standard
 * library functions that should be but aren't available on all systems. Some
 * are stubs for the grok program; several grok source files (g_*.c) are linked
 * into plan to be able to read grok databases.
 *
 *	resolve_tilde(path)		return path with ~ replaced with home
 *					directory as found in $HOME
 *	find_file(buf, name, exec)	find file <name> and store the path
 *					in <buf>. Return FALSE if not found.
 *	startup_lock(fname, force)	make sure program runs only once 
 *					at any time
 *	lockfile(fp, lock)		lock or unlock the database to
 *					prevent simultaneous access
 *	fatal(fmt, ...)			print fatal error message and exit
 *	mystrdup(s)			local version of strdup
 *	mystrcasecmp(a,b)		local version of strcasecmp
 *	exit(ret)			local version of exit that writes back
 *	to_octal(n)			convert ascii to octal string (grok)
 *	to_ascii(str, def)		convert octal string to ascii (grok)
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <stdarg.h>
#if defined(IBM) || defined(ULTRIX)
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <pwd.h>
#if !defined(NOLOCK) && defined(LOCKF) || defined(linux)
#include <sys/file.h>
#endif
#if !defined(NOLOCK) && defined(LOCKF)
#include <sys/file.h>
#endif
#if defined(IBM) && !defined(NOLOCK)
#include <sys/lockf.h>
#endif
#include <sys/signal.h>
#include <sys/stat.h>
#include <Xm/Xm.h>
#include "config.h"
#include "cal.h"

#ifdef MIPS
extern struct passwd *getpwnam();
#endif

extern char		*progname;	/* argv[0] */
extern struct plist	*mainlist;	/* list of all schedule entries */
char			lockpath[256];	/* lockfile path */


/*
 * If <path> begins with a tilde, replace the tilde with $HOME. This is used
 * for the database files, and the holiday file (see holiday.c).
 */

char *resolve_tilde(
	char		*path)			/* path with ~ */
{
	struct passwd	*pw;			/* for searching home dirs */
	static char	pathbuf[512];		/* path with ~ expanded */
	char		*p, *q;			/* username copy pointers */
	char		*home = 0;		/* home dir (if ~ in path) */

	if (*path != '~')
		return(path);

	if (!path[1] || path[1] == '/') {
		*pathbuf = 0;
		if (!(home = getenv("HOME")) &&
		    !(home = getenv("home")))
			fprintf(stderr, _("%s: $HOME and $home not set,\n"),
								progname);
	} else {
		for (p=path+1, q=pathbuf; *p && *p != '/'; p++, q++)
			*q = *p;
		*q = 0;
		if ((pw = getpwnam(pathbuf))) {
			home = pw->pw_dir;
			path = p-1;
		}
	}
	if (!home) {
		fprintf(stderr, _("%s: can't evaluate ~%s in %s, using .\n"),
						progname, pathbuf, path);
		home = ".";
	}
	sprintf(pathbuf, "%s/%s", home, path+1);
	return(pathbuf);
}


/*
 * locate a program or file, and return its complete path. This is used by
 * the daemon to locate notifier and user programs, and by plan to locate
 * pland and plan.help. Assume that <buf> has space for 1024 chars. PATH
 * is a macro defined by the Makefile.
 */

#ifndef PATH
#define PATH 0
#endif

BOOL find_file(
	char		*buf,		/* buffer for returned path */
	char		*name,		/* file name to locate */
	BOOL		exec)		/* must be executable? */
{
	int		method;		/* search path counter */
	char		*path;		/* $PATH or DEFAULTPATH */
	int		namelen;	/* len of tail of name */
	char		*p, *q;		/* string copy pointers */

	if (*name == '/') {				/* begins with / */
		strcpy(buf, name);
		return(TRUE);
	}
	if (*name == '~') { 				/* begins with ~ */
		strcpy(buf, resolve_tilde(name));
		return(TRUE);
	}
	namelen = strlen(name);
	for (method=0; ; method++) {	
		switch(method) {
		  case 0:   path = PATH;		break;
		  case 1:   path = getenv("PLAN_PATH");	break;
		  case 2:   path = getenv("PATH");	break;
		  case 3:   path = DEFAULTPATH;		break;
		  default:  return(FALSE);
		}
		if (!path)
			continue;
		do {
			q = buf;
			p = path;
			while (*p && *p != ':' && q < buf + 1021 - namelen)
				*q++ = *p++;
			*q++ = '/';
			strcpy(q, name);
			if (!access(buf, exec ? X_OK : R_OK)) {
				strcpy(q = buf + strlen(buf), "/..");
				if (access(buf, X_OK)) {
					*q = 0;
					return(TRUE);
				}
			}
			*buf = 0;
			path = p+1;
		} while (*p);
	}
	/*NOTREACHED*/
}


/*
 * Make sure that the program is running only once, by creating a special
 * lockfile that contains our pid. If such a lockfile already exists, see
 * if the process that created it exists; if no, ignore it. If <force> is
 * TRUE, try to kill it. Otherwise, return FALSE.
 */

static BOOL check_process(BOOL, PID_T);

BOOL startup_lock(
	BOOL			pland,		/* is this plan or pland? */
	BOOL			force)		/* kill competitor */
{
	char			*pathtmp;	/* name of lockfile */
	char			buf[2048];	/* lockfile contents */
	int			lockfd;		/* lock/pid file */
	PID_T			pid;		/* pid in lockfile */
	int			retry = 5;	/* try to kill daemon 5 times*/

	pathtmp = resolve_tilde(pland ? PLANDLOCK : PLANLOCK);
	sprintf(buf, pathtmp, (int)getuid());
	strncpy(lockpath, resolve_tilde(buf), sizeof(lockpath)-1);
	lockpath[sizeof(lockpath)-1] = 0;
	while ((lockfd = open(lockpath, O_WRONLY|O_EXCL|O_CREAT, 0644)) < 0) {
		if ((lockfd = open(lockpath, O_RDONLY)) < 0) {
			int err = errno;
			fprintf(stderr, _("%s: cannot open lockfile "),
								progname);
			errno = err;
			perror(lockpath);
			_exit(1);
		}
		if (read(lockfd, buf, 10) < 5) {
			int err = errno;
			fprintf(stderr, _("%s: cannot read lockfile "),
								progname);
			errno = err;
			perror(lockpath);
			_exit(1);
		}
		buf[10] = 0;
		pid = atoi(buf);
		close(lockfd);
		if (pid == getpid())
			break;
		if (!retry--)
			fprintf(stderr,
			_("%s: failed to kill process %d owning lockfile %s\n"),
						progname, pid, lockpath);
#		ifndef NOKILL0
		if (kill(pid, 0) && errno == ESRCH) {
			if (unlink(lockpath) && errno != ENOENT) {
				int err = errno;
				fprintf(stderr, _("%s: cannot unlink lockfile "
						), progname);
				errno = err;
				perror(lockpath);
				return(FALSE);
			}
			continue;
		}
#		endif
		if (!force)
			return(FALSE);

		/* if check_process is functional (works on current process),
		 * check whether the pid in the lockfile is really plan/pland.
		 * if yes (or if check_process is broken), kill the old pid. */
		if (check_process(pland, getpid()) &&
		   !check_process(pland, pid)) {
			fprintf(stderr, _("%s: process %d owning lockfile %s "
				"no longer exists, not killing"),
				progname, pid, pathtmp);
			unlink(pathtmp);
			break;
		}
		(void)kill(pid, retry == 4 ? SIGINT : SIGTERM);
		sleep(1);
	}
	sprintf(buf, _("%5d      (pid of lockfile for %s, for user %d)\n"),
				(int)getpid(), progname, (int)getuid());
	if (write(lockfd, buf, (int)strlen(buf)) != (int)strlen(buf)) {
		int err = errno;
		fprintf(stderr, _("%s: cannot write lockfile "), progname);
		errno = err;
		perror(lockpath);
		_exit(1);
	}
	close(lockfd);
	return(TRUE);
}


/*
 * given a pid (read from the lockfile), make sure it really belongs to a
 * process called "plan" or "pland". The expected process might have died
 * and the pid might have been reused by some other process, which we do
 * not want to kill. Return TRUE if the process is still plan or pland.
 */

static BOOL check_process(
	BOOL			pland,		/* false: plan, true: pland */
	PID_T			pid)		/* check this process ID */
{
#ifdef LINUX
	struct stat		statbuf;	/* for checking ownership */
	char			buf[1024];	/* lockfile contents */
	int			fd;		/* /proc/%d/... file */
	char			*p;		/* for mangling the prog name*/
	int			i;

	sprintf(buf, "/proc/%u/cmdline", pid);
	if (stat(buf, &statbuf) < 0)			/* read file owner */
		return(FALSE);
	if (statbuf.st_uid != getuid())			/* is it us? */
		return(FALSE);
	while ((fd = open(buf, O_RDONLY)) < 0)		/* open /proc file */
		return(FALSE);
	i = read(fd, buf, sizeof(buf)-1);		/* get cmdline */
	close(fd);
	if (i < 1)					/* none found? */
		return(FALSE);
	buf[i] = 0;
	if ((p = strchr(buf, ' ')))			/* isolate first word*/
		*p = 0;
	p = strrchr(buf, '/');				/* cut off /path/ */
	p = p ? p+1 : buf;
	return(!strcmp(p, pland ? "pland" : "plan"));	/* compare prog name */
#else
	(void)pland;
	(void)pid;
	return(FALSE);
#endif
}


/*
 * Create new lock file when it has been deleted (after cleanup of /tmp)
 */

BOOL refresh_lock(
	char		*pathtmp)	/* PLANDLOCK or PLANLOCK */
{
	char		buf[80];	/* lockfile contents */
	int		lockfd;		/* lock/pid file */

	sprintf(lockpath, pathtmp, (int)getuid());
	if ((lockfd = open(lockpath, O_RDONLY)) >= 0) {
		close(lockfd);
		return(TRUE);
	}
	lockfd = creat(lockpath,0644);
	if (lockfd < 0) {
		int err = errno;
		fprintf(stderr, _("%s: cannot create lockfile "), progname);
		errno = err;
		perror(lockpath);
		_exit(1);
	}
	sprintf(buf, _("%5d      (pid of lockfile for %s, for user %d)\n"),
				(int)getpid(), progname, (int)getuid());
	if (write(lockfd, buf, (int)strlen(buf)) != (int)strlen(buf)) {
		int err = errno;
		fprintf(stderr, _("%s: cannot write lockfile "), progname);
		errno = err;
		perror(lockpath);
		_exit(1);
	}
	close(lockfd);
	return(TRUE);
}


/*
 * try to lock or unlock the database file. There is actually little risk
 * that two plan programs try to access simultaneously, unless someone adds
 * an appointment graphically, and then another one on the command line
 * within 10 seconds. If people add two at the same time, locking doesn't
 * help at all. As a side effect, the file is rewound.
 */

#ifndef NOLOCK
static BOOL got_alarm;
static void (*old_alarm)();

/*ARGSUSED*/
static void alarmhand(
	int sig)
{
	got_alarm = TRUE;
	signal(SIGALRM, old_alarm);
}
#endif


/* the two UNUSED's below are wrong but they shut up stupid gcc warnings */
void lockfile(
	UNUSED FILE	*fp,		/* file to lock */
	UNUSED BOOL	lock)		/* TRUE=lock, FALSE=unlock */
{
#ifndef NOLOCK
	if (lock) {
		got_alarm = FALSE;
		old_alarm = signal(SIGALRM, alarmhand);
		alarm(3);
		(void)rewind(fp);
		(void)lseek(fileno(fp), 0, 0);
#ifdef FLOCK
		if (flock(fileno(fp), LOCK_EX | LOCK_NB) || got_alarm) {
#else
		if (lockf(fileno(fp), F_LOCK, 0) || got_alarm) {
#endif
			perror(progname);
			fprintf(stderr,
_("%s: failed to lock database after 3 seconds, accessing anyway\n"), progname);
		}
		alarm(0);
	} else {
		(void)rewind(fp);
		(void)lseek(fileno(fp), 0, 0);
#ifdef FLOCK
		(void)flock(fileno(fp), LOCK_UN);
#else
		(void)lockf(fileno(fp), F_ULOCK, 0);
#endif
		return;
	}
#endif /* NOLOCK */
}


/*
 * whenever something goes seriously wrong, this routine is called. It makes
 * code easier to read. fatal() never returns.
 */

void fatal(
	char		*fmt, ...)
{
	va_list		parm;

	va_start(parm, fmt);
	fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, fmt, parm);
	va_end(parm);
	putc('\n', stderr);
	exit(1);
}


/*
 * Ultrix doesn't have strdup, so we'll need to define one locally.
 */

char *mystrdup(
	char		*s)
{
	char		*p = NULL;

	if (s && (p = (char *)malloc(strlen(s)+1)))
		strcpy(p, s);
	return(p);
}


/*
 * Sinix doesn't have strcasecmp, so here is my own. Not as efficient as
 * the canonical implementation, but short, and it's not time-critical.
 */

int mystrcasecmp(
	char		*a,
	char		*b)
{
	char		ac, bc;

	for (;;) {
		ac = *a++;
		bc = *b++;
		if (!ac || !bc)
			break;
		if (ac <= 'Z' && ac >= 'A')	ac += 'a' - 'A';
		if (bc <= 'Z' && bc >= 'A')	bc += 'a' - 'A';
		if (ac != bc)
			break;
	}
	return(ac - bc);
}


#ifdef PLANGROK

/*
 * convert ascii code to and from a string representation. This is used for
 * form files in grok format. Only compiled into plan versions with grok
 * support (the real name is xmbase-grok; you need version 1.4 or up).
 */

char *to_octal(
	int		n)		/* ascii to convert to string */
{
	static char	buf[8];

	if (n == '\t')			return("\\t");
	if (n == '\n')			return("\\n");
	if (n <= 0x20 || n >= 0x7f)	{ sprintf(buf, "\\%03o", n); }
	else				{ buf[0] = n; buf[1] = 0;    }
	return(buf);
}

char to_ascii(
	char		*str,		/* string to convert to ascii */
	int		def)		/* default if string is empty */
{
	int		n;
	char		*p = str;

	if (!p)
		return(def);
	while (*p == ' ' || *p == '\t') p++;
	if (!*p)
		return(def);
	if (*p == '\\') {
		if (p[1] == 't')	return('\t');
		if (p[1] == 'n')	return('\n');
		if (p[1] >= '0' &&
		    p[1] <= '7')	{ sscanf(p+1, "%o", &n); return(n); }
		if (p[1] == 'x')	{ sscanf(p+2, "%x", &n); return(n); }
		return('\\');
	} else
		return(*p);
}


/*
 * some systems use mybzero (BSD), others memset (SysV), I'll roll my own...
 */

void mybzero(
	void		*p,
	int		n)
{
	char		*q = p;
	while (n--) *q++ = 0;
}


/*
 * dummies for grok, called by g_*.c.
 */

void print_info_line(void) {}

#endif
