/*
 * write the master list to the ~/.dayplan file.
 *
 *	copyfile(source, target)		Copy a file
 *	write_mainlist(list, path, what)	Write appointments and config
 *						info to a database file.
 */

#include <stdio.h>
#include <time.h>
#include <Xm/Xm.h>
#ifdef MIPS
#include <sys/types.h>
#include <sys/fcntl.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif
#include <sys/stat.h>
#include <errno.h>
#include "cal.h"
#include "version.h"

static BOOL writefile(struct plist *, char *, int, int);

extern char		*progname;		/* argv[0] */
extern struct config	config;			/* global configuration data */
extern struct plist	*mainlist;		/* all schedule entries */
extern struct user	*user;			/* user list for week view */
extern int		nusers;			/* # of users in user list */
extern char		Print_Spooler[100];	/* print spooling command */
extern BOOL		reread;			/* need to re-read db file */


/*
 * copy a file to another. If the target file exists, truncate and overwrite,
 * don't recreate, to preserve its ownership and permissions. This is used
 * to make a backup of a database before rewriting it, and to restore the
 * original from the backup if the rewrite failed. Return FALSE on failure.
 */

BOOL copyfile(
	char		*source,
	char		*target)
{
	FILE		*sfp, *tfp;
	char		line[1024];	/* line buffer */
	BOOL		fail = FALSE;

	if (!(sfp = fopen(source, "r"))) {
		if (errno == ENOENT) {
			unlink(target);
			return(TRUE);
		} else {
			perror(source);
			return(FALSE);
		}
	}
	if (!(tfp = fopen(target, "w"))) {
		perror(target);
		fclose(sfp);
		return(FALSE);
	}
	while (!fail && !feof(sfp)) {
		if (!fgets(line, 1024, sfp))
			break;
		errno = 0;
		fputs(line, tfp);
		fail += !!errno;
	}
	fclose(sfp);
	fclose(tfp);
	return(!fail);
}


/*
 * write all files in which at least one entry has changed. For the own main
 * list and the private list, use the mainlist->modified flag; for other users
 * (whose entries are also in the mainlist) use user[].modified. User 0 is
 * special, it's the user himself. Don't write it twice, but always write to
 * the file, even in network mode.
 */

void write_mainlist(void)
{
	int		u;		/* user counter */
	char		path[1024];	/* user file name to write */

	if (!config.webplan &&
	    mainlist->modified &&
	    writefile(mainlist, DB_PUB_PATH,  0, WR_PUBLIC|WR_CONFIG) &&
	    writefile(mainlist, DB_PRIV_PATH, 0, WR_PRIVATE))
		mainlist->modified = FALSE;

	for (u=1; u < nusers; u++) {
		if (user[u].fromserver || !user[u].modified)
			continue;
		if (!user[u].path || !user[u].name) {
			create_error_popup(0, 0,
			 _("file of user #%d not written, no name or path\n"),
									u);
			continue;
		}
		strcpy(path, find_pub_path(user[u].path));
		user[u].readonly = !access(path, F_OK) &&
				  !!access(path, W_OK);
		user[u].time     = get_time();
		if (writefile(mainlist, path, u, WR_PUBLIC))
			user[u].modified = 0;
	}
}


/*
 * write a list to a file. The <what> bitmap determines what is written:
 * WR_CONFIG writes configuration info, WR_PUBLIC writes public appointments,
 * and WR_PRIVATE writes private appointments. Specifying WR_PRIVATE has the
 * side effect of making the file read-only. After calling this routine,
 * call resynchronize_daemon() to tell the daemon to re-read the databases.
 * If another user's file is written (name != 0), the beginning of the file
 * containing global configuration is preserved.
 *
 * To guard against disk errors, this algorithm is used:
 *   - copy the old file to a backup file with restrictive permissions
 *   - if failure, delete backup and exit
 *   - rewrite old file, preserving ownership and permissions
 *   - if failure
 *     - copy backup to file
 *     - if success, delete backup
 *     - exit
 *   - delete backup
 *
 * Returns FALSE for all types of failure
 */

static FILE *curr_fp;
static int write_to_file(line) char *line;
	{ errno = 0; fprintf(curr_fp, "%s", line);
	  return(errno && errno != ENOTTY); }

static BOOL writefile(
	struct plist	*list,		/* list to write */
	char		*path,		/* file to write list to */
	int		u,		/* user index, 0=own */
	int		what)		/* bitmap of WR_* macros */
{
	struct config	*c = &config;
	FILE		*fp;		/* open file */
	char		backpath[1024];	/* path of backup file */
	char		line[1024];	/* line buffer */
	char		*name;		/* name of user, 0=own */
	char		*p;		/* string copy pointers */
	struct entry	*ep;		/* list entry being written */
	int		i;		/* entry counter */
	BOOL		write_error = FALSE;

	if (user[u].readonly)
		return(TRUE);
	list->locked = TRUE;
	path = resolve_tilde(path);
	if (user[u].grok) {
#ifdef qPLANGROK
		FORM *form = form_create();
		if (read_form(form, path))
			readgrokfile(form, u, form->dbase ? form->dbase
							  : formname);
		form_delete(form);
		free(form);
		user[u].grok = TRUE;
		return(TRUE);
#else
		create_error_popup(0, 0,
			_("%s is an\nxmbase-grok database, cannot write file"),
			path);
		return(FALSE);
#endif
	}
							/* backup database */
	sprintf(backpath, "%s.bak.%d", path, getuid());
	unlink(backpath);
	if (!copyfile(path, backpath)) {
		create_error_popup(0, errno,
		      _("Failed to back up database %s\nto %s, aborted write"),
			path, backpath);
		unlink(backpath);
		list->locked = FALSE;
		return(FALSE);
	}
	if (!(fp = fopen(path, "w"))) {			/* rewrite database */
		create_error_popup(0, errno,
			_("Failed to create appointment\ndatabase %s"), path);
		list->locked = FALSE;
		return(FALSE);
	}
	if ((what & WR_PRIVATE) && chmod(path, 0600))
		create_error_popup(0, errno,
		      _("Warning: cannot chmod 0600 appointment\ndatabase %s"),
		     	path);
	lockfile(fp, TRUE);

	if (u && !(what & WR_CONFIG)) {		/* copy user config */
		FILE *rfp;
		if (!(rfp = fopen(backpath, "r")) && errno != ENOENT) {
			create_error_popup(0, errno,
				_("Failed to read copy of user\ndatabase %s"),
				backpath);
			write_error = TRUE;
		}
		errno = 0;
		if (rfp) {
			while (!write_error) {
				if (!fgets(line, 1024, rfp))
					break;
				if (*line >= '0' && *line <= '9')
					break;
				fputs(line, fp);
				write_error |= errno && errno != ENOTTY;
			}
			fclose(rfp);
		}
	}
							/* write temp db */
	if (what & WR_CONFIG) {
		for (p=c->ewarn_prog; p && *p; p++)
			if (*p == '\n') *p = ';';
		for (p=c->lwarn_prog; p && *p; p++)
			if (*p == '\n') *p = ';';
		for (p=c->alarm_prog; p && *p; p++)
			if (*p == '\n') *p = ';';
		errno = 0;
		fprintf(fp, "plan %s\n", VERSION JAPANVERSION);
		write_error |= errno && errno != ENOTTY;
		fprintf(fp,
			"o\t%c%c%c%c%c%c%c%c%c%c%c%c%c%c %d %d %d %d %d %d\n",
					c->sunday_first	  ? 's' : '-',
					c->ampm		  ? 'a' : '-',
					c->mmddyy	  ? 'm' : '-',
					c->autodel	  ? 'd' : '-',
					c->julian	  ? 'j' : '-',
					c->gpsweek	  ? 'g' :
					c->weeknum	  ? 'w' : '-',
					c->nopast	  ? 'n' : '-',
					c->insecure_script? 'i' : '-',
					c->weekwarn	  ? 'w' : '-',
					c->weekuser	  ? 'u' : '-',
					c->week_bignotime ? 's' : '-',
					c->thu_week	  ? 't' :
					c->fullweek	  ? 'w' : '-',
					c->colorcode_m	  ? 'c' : '-',
					c->ownonly	  ? 'o' : '-',
					(int)c->early_time,
					(int)c->late_time,
					(int)c->wintimeout,
					c->week_minhour,
					c->week_maxhour,
					c->week_ndays);

		write_error |= errno && errno != ENOTTY;
		fprintf(fp,
			"O\t%c%c%c----------------------\n",
					c->share_mainwin  ? 's' : '-',
					c->resize_mainwin ? 'r' : '-',
					c->moretimecols   ? 't' : '-');

		write_error |= errno && errno != ENOTTY;
		fprintf(fp, "t\t%d %d %d %d %d\n",
					(int)c->adjust_time,
					(int)c->raw_tzone,
					c->dst_flag,
					c->dst_begin,
					c->dst_end);

		write_error |= errno && errno != ENOTTY;
		fprintf(fp, "e\t%c%c%c %s\n",
					c->ewarn_window ? 'w' : '-',
					c->ewarn_mail   ? 'm' : '-',
					c->ewarn_exec   ? 'x' : '-',
					c->ewarn_prog   ? c->ewarn_prog
							      : "");
		write_error |= errno && errno != ENOTTY;
		fprintf(fp, "l\t%c%c%c %s\n",
					c->lwarn_window ? 'w' : '-',
					c->lwarn_mail   ? 'm' : '-',
					c->lwarn_exec   ? 'x' : '-',
					c->lwarn_prog   ? c->lwarn_prog
							      : "");
		write_error |= errno && errno != ENOTTY;
		fprintf(fp, "a\t%c%c%c %s\n",
					c->alarm_window ? 'w' : '-',
					c->alarm_mail   ? 'm' : '-',
					c->alarm_exec   ? 'x' : '-',
					c->alarm_prog   ? c->alarm_prog
							      : "");
		write_error |= errno && errno != ENOTTY;
		fprintf(fp, "y\t%c--------- %d %d %d\n",
					c->yov_single ? 's' : '-',
					c->yov_nmonths,
					c->yov_display,
					c->yov_user);
		write_error |= errno && errno != ENOTTY;
		fprintf(fp, "P\t%c%c%c%c------ %d\n",
					c->pr_omit_appts  ? 'a' : '-',
					c->pr_omit_priv   ? 'p' : '-',
					c->pr_omit_others ? 'o' : '-',
					c->pr_color	  ? 'c' : '-',
					c->pr_mode);
		if (*Print_Spooler) {
			fprintf(fp, "p\t%s\n", Print_Spooler);
			write_error |= errno && errno != ENOTTY;
		}
		if (c->mailer) {
			fprintf(fp, "m\t%s\n", c->mailer);
			write_error |= errno && errno != ENOTTY;
		}
		if (c->language) {
			fprintf(fp, "L\t%s\n", c->language);
			write_error |= errno && errno != ENOTTY;
		}
		if (c->java) {
			fprintf(fp, "%s", c->java);
			write_error |= errno && errno != ENOTTY;
		}
		write_error |= errno && errno != ENOTTY;
		for (i=0; i < nusers; i++) {
			fprintf(fp,"u\t%-15s %-20s %d %2d %d %d %d %d %s %d\n",
					user[i].name,
					user[i].path,
					user[i].suspend_w,
					user[i].color,
					user[i].suspend_m,
					user[i].alarm,
					user[i].suspend_o,
					user[i].fromserver,
					user[i].server ? user[i].server : "",
					user[i].suspend_d);
			write_error |= errno && errno != ENOTTY;
		}
	}
	name = user ? user[u].name : 0;
	for (ep=list->entry, i=0; i < list->nentries; i++, ep++) {
		if ((!ep->user &&  name) ||
		     (ep->user && !name) ||
		     (ep->user &&  name  && strcmp(ep->user, name)))
			continue;
		if ((ep->private && !(what & WR_PRIVATE)) ||
		   (!ep->private && !(what & WR_PUBLIC)))
			continue;

		curr_fp = fp;
		write_error |= write_one_entry(write_to_file, ep);
	}
	lockfile(fp, FALSE);
	errno = 0;
	fclose(fp);
	write_error |= errno && errno != ENOTTY;
	list->modified = FALSE;
	list->locked   = FALSE;

	if (write_error) {				/* restore database */
		if (copyfile(backpath, path)) {
			unlink(backpath);
			create_error_popup(0, errno,
_("Failed to write appointment database file\n\"%s\".\n\
Your recent changes have been discarded.\n\
Check disk space and permissions of the directory!"), path);
		} else
			create_error_popup(0, errno,
_("Failed to write appointment database file\n\"%s\".\n\
Your recent changes have been discarded.\n\
Check disk space and permissions of the directory!\n\n\
Could not restore original contents. Backup is in\n\
%s, requires manual restore"), path, backpath);
		reread = TRUE;
		return(FALSE);
	}
	unlink(backpath);
	return(TRUE);
}


/*
 * write a single appointment by generating one or more lines, and calling
 * a callback function for each line. Returns FALSE for all types of failure.
 */

int write_one_entry(
	int		(*callback)(char*),/* function that writes line */
	struct entry	*ep)		/* list entry to write */
{
	char		line[10240];	/* line buffer */
	char		*p, *q;		/* string copy pointers */
	int		j;		/* exception counter */
	struct tm	*tm;		/* time to m/d/y h:m:s conv */
	BOOL		write_error = FALSE;

	tm = time_to_tm(ep->time);
	sprintf(line,
"%d/%d/%d  %d:%d:%d  %d:%d:%d  %d:%d:%d  %d:%d:%d  %c%c%c%c%c%c%c%c%c- %d %d\n",
		(int)tm->tm_mon+1,
		(int)tm->tm_mday,
		(int)tm->tm_year + 1900,
		(int)(ep->notime ? 99 : tm->tm_hour),
		(int)(ep->notime ? 99 : tm->tm_min),
		(int)(ep->notime ? 99 : tm->tm_sec),
		(int)ep->length     / 3600,
		(int)ep->length     / 60   % 60,
		(int)ep->length            % 60,
		(int)ep->early_warn / 3600,
		(int)ep->early_warn / 60   % 60,
		(int)ep->early_warn        % 60,
		(int)ep->late_warn  / 3600,
		(int)ep->late_warn  / 60   % 60,
		(int)ep->late_warn         % 60,
		(int)ep->suspended ? 'S' : '-',
		(int)ep->private   ? 'P' : '-',
		(int)ep->noalarm   ? 'N' : '-',
		(int)ep->hide_in_m ? 'M' : '-',
		(int)ep->hide_in_y ? 'Y' : '-',
		(int)ep->hide_in_w ? 'W' : '-',
		(int)ep->hide_in_o ? 'O' : '-',
		(int)ep->hide_in_d ? 'D' : '-',
		(int)ep->todo	   ? 't' : '-',
		(int)ep->acolor,
		(int)ep->days_warn / 86400);

	write_error |= (*callback)(line);
	if (ep->rep_every || ep->rep_weekdays || ep->rep_days
					      || ep->rep_yearly) {
		sprintf(line, "R\t%d %d %d %d %d\n",
			(int)ep->rep_every,
			(int)ep->rep_last,
			(int)ep->rep_weekdays,
			(int)ep->rep_days,
			(int)ep->rep_yearly);
		write_error |= (*callback)(line);
	}
	for (j=0; j < NEXC; j++)
		if (ep->except[j]) {
			tm = time_to_tm(ep->except[j]);
			sprintf(line, "E\t%d/%d/%d\n",
					(int)tm->tm_mon+1,
					(int)tm->tm_mday,
					(int)tm->tm_year + 1900);
			write_error |= (*callback)(line);
		}

	for (j=0; j < 4; j++) {
		switch(j) {
		  case 0:  p = ep->note;	break;
		  case 1:  p = ep->message;	break;
		  case 2:  p = ep->script;	break;
		  case 3:  p = ep->java;	break;
		  default: p = 0;
		}
		if (p)
			while (*p) {
				line[0] = "NMSJ"[j];
				line[1] = '\t';
				for (q=line+2; *p && *p != '\n'; )
					*q++ = *p++;
				*q++ = '\n';
				*q = 0;
				write_error |= (*callback)(line);
				if (*p) p++;
			}
	}
	return(write_error);
}
