/*
 * read and write database files.
 *
 *	write_dbase(form, path)		write dbase
 *	read_dbase(form, path)		read dbase into empty dbase struct
 */

#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <assert.h>
#include <signal.h>
#ifdef DIRECT
#include <sys/dir.h>
#define  dirent direct
#else
#include <dirent.h>
#endif
#include <errno.h>
#include <Xm/Xm.h>
#include "config.h"

#ifdef GROK
#include "proto.h"
#endif
#ifdef PLANGROK
#include "cal.h"
#endif
#if defined(GROK) || defined(PLANGROK)
#include "grok.h"
#include "form.h"

#define R_SEP	'\n'			/* row (card) separator */
#define ESC	'\\'			/* treat next char literally */

extern char	*progname;		/* argv[0] */
extern Widget	toplevel;		/* top-level shell for icon name */


/*
 * If the print spooler dies for some reason, print a message. Don't let
 * grok die because of the broken pipe signal.
 */

static void broken_pipe_handler(int sig)
{
	create_error_popup(toplevel, 0,
			"Procedural script aborted with signal %d\n", sig);
	signal(SIGPIPE, SIG_IGN);
}


/*
 * Write the database and all the items in it. The <path> is from the
 * forms <dbase> field. Both the database and the form will be stored
 * in a CARD structure; both are required to access cards. If the
 * database is procedural, open a pipe to the database process and add
 * -r option.
 * Returns FALSE if the file could not be written.
 */

static BOOL writefile(DBASE *, FORM *, int);

BOOL write_dbase(
	DBASE		*dbase,		/* form and items to write */
	FORM		*form,		/* contains column delimiter */
	BOOL		force)		/* write even if not modified*/
{
	int		s;		/* section index */
	BOOL		ok = TRUE;

	if (dbase->nsects == 1 && (dbase->modified || force))
		ok = writefile(dbase, form, 0);
	else
		for (s=0; s < dbase->nsects; s++)
			if (dbase->sect[s].modified &&
					(!dbase->sect[s].rdonly || force))
				ok &= writefile(dbase, form, s);
	return(ok);
}


static BOOL writefile(
	DBASE		*dbase,		/* form and items to write */
	FORM		*form,		/* contains column delimiter */
	int		nsect)		/* section to write */
{
	SECTION		*sect;		/* section to write */
	char		*path;		/* file to write list to */
	FILE		*fp;		/* open file */
	int		r, c;		/* row and column counters */
	int		hicol;		/* # of columns of card -1 */
	char		*value;		/* converted data to write */
	char		buf[40];	/* for date/time conversion */
	ITEM		*item = 0;	/* item describing value */
	int		i;		/* index of item */
	char		*p;		/* string copy pointer */

	sect = &dbase->sect[nsect];
	path = sect->path ? sect->path : form->dbase;
	if (!path || !*path) {
		create_error_popup(toplevel, 0,
			"Database has no name, cannot save to disk");
		return(FALSE);
	}
	path = resolve_tilde(path);
	if (form->proc) {
		char cmd[1024];
		sprintf(cmd, "%s -w %s", path, form->name);
		if (!(fp = popen(cmd, "w"))) {
			create_error_popup(toplevel, errno,
				"Failed to run shell script %s", cmd);
			return(FALSE);
		}
	} else
		if (!(fp = fopen(path, "w"))) {
			create_error_popup(toplevel, errno,
				"Failed to create database file %s", path);
			return(FALSE);
		}
	for (r=0; r < dbase->nrows; r++) {
		if (nsect != dbase->row[r]->section)
			continue;
		for (hicol=dbase->row[r]->ncolumns-1; hicol > 0; hicol--)
			if (dbase_get(dbase, r, hicol))
				break;
		for (c=0; c <= hicol; c++) {
			if ((value = dbase_get(dbase, r, c))) {
				for (i=form->nitems-1; i >= 0; i--) {
					item = form->items[i];
					if (item->column == c)
						break;
				}
				if (i >= 0 && item->type == IT_TIME) {
					long secs = atoi(value);
					switch(item->timefmt) {
					  case T_DATE:
						sprintf(value = buf, "%s",
							mkdatestring(secs));
						break;
					  case T_TIME:
						sprintf(value = buf, "%s",
							mktimestring(secs, 0));
						break;
					  case T_DATETIME:
						sprintf(value = buf, "%s %s",
							mkdatestring(secs),
							mktimestring(secs, 0));
						break;
					  case T_DURATION:
						sprintf(value = buf, "%s",
							mktimestring(secs, 1));
						break;
					}
				}
				for (p=value; *p; p++) {
					if (*p == R_SEP ||
					    *p == form->cdelim ||
					    *p == ESC)
						fputc(ESC, fp);
					fputc(*p, fp);
				}
			}
			if (c < hicol)
				fputc(form->cdelim, fp);
		}
		fputc(R_SEP, fp);
	}
	if (form->proc)
		if (pclose(fp)) {
			create_error_popup(toplevel, errno,
				"%s:\nfailed to create database", path);
			return(FALSE);
		} else
			return(TRUE);
	else
		fclose(fp);
	if (sect)
		sect->modified = FALSE;
	dbase->modified = FALSE;
	sect->mtime = time(0);
	print_info_line();
	return(TRUE);
}


/*
 * Read a database from a file. The database must be empty (just created
 * with dbase_create()). Returns FALSE if the file could not be read.
 * Print a warning if the database is supposed to be writable but isn't.
 *
 * This works by first reading in fields, allocating one string for each.
 * The strings are stored in a long list. Escaped separators are resolved.
 * Row separators are stored as null pointers in the list. When the end
 * of the file is reached, the number of rows and columns is known, and
 * the pointers in the list are put in the right places of the dbase array.
 */

#define LCHUNK	4096		/* alloc this many new list ptrs */
#define BCHUNK	1024		/* alloc this many new chars for item */

static BOOL find_db_file     (DBASE *, FORM *, char *);
static BOOL read_dir_or_file (DBASE *, FORM *, char *);
static BOOL read_file	     (DBASE *, FORM *, char *, time_t);

BOOL read_dbase(
	DBASE			*dbase,		/* form and items to write */
	FORM			*form,		/* contains column delimiter */
	char			*path)		/* file to read list from */
{
	char			pathbuf[1024];	/* file name with path */
	BOOL			ret;		/* return code, FALSE=error */

	if (!path || !*path) {
		create_error_popup(toplevel, 0,
			"Database has no name, cannot read from disk");
		return(FALSE);
	}
	dbase_delete(dbase);
#ifdef GROK
	init_variables();
#endif
	if (*path != '/' && *path != '~' && form->path) {
		char *p;
		strcpy(pathbuf, form->path);
		if ((p = strrchr(pathbuf, '/'))) {
			strcpy(p+1, path);
			path = pathbuf;
		}
	}
	if (!(ret = find_db_file(dbase, form, path)))
		create_error_popup(toplevel, 0, "Failed to read %s", path);

	dbase->currsect = -1;
	dbase->modified = FALSE;
	print_info_line();
	return(ret);
}


/*
 * read both path.db and path, in that order. Return FALSE if both failed.
 */

static BOOL find_db_file(
	DBASE		*dbase,		/* form and items to write */
	FORM		*form,		/* contains column delimiter */
	char		*path)		/* file to read list from */
{
	char		pathbuf[1024];	/* file name with path */
	int		nread = 0;	/* # of successful reads */

	sprintf(pathbuf, "%s.db", path);
	if (!access(pathbuf, F_OK))
		nread += read_dir_or_file(dbase, form, pathbuf);
	if (!access(path, F_OK))
		nread += read_dir_or_file(dbase, form, path);
	return(nread > 0);
}


/*
 * read path. If it's a directory, recurse. Return FALSE if nothing was found.
 */

#define MAXSC  1000		/* no more than 200 sections */

static int compare_name(
	MYCONST void	*u,
	MYCONST void	*v)
{
	return(strcmp(*(char **)u, *(char **)v));
}

static BOOL read_dir_or_file(
	DBASE		*dbase,		/* form and items to write */
	FORM		*form,		/* contains column delimiter */
	char		*path)		/* file to read list from */
{
	char		*name[MAXSC];	/* directory listing */
	char		pathbuf[1024];	/* file name with path */
	struct stat	statbuf;	/* is path a directory? */
	DIR		*dir;		/* open directory file */
	struct dirent	*dp;		/* one directory entry */
	int		nfiles = 0;	/* # of files in directory */
	int		i, n=0;

	if (stat(path, &statbuf))
		return(FALSE);
	if (!S_ISDIR(statbuf.st_mode))
		return(read_file(dbase, form, path, statbuf.st_mtime));
	if (!(dir = opendir(path)))
		return(FALSE);
	while ((dp = readdir(dir)) && n < MAXSC)
		if (*dp->d_name != '.')
			name[n++] = mystrdup(dp->d_name);
	(void)closedir(dir);
	if (n)
		qsort(name, n, sizeof(char *), compare_name);

	for (i=0; i < n; i++) {
		sprintf(pathbuf, "%s/%s", path, name[i]);
		free(name[i]);
		nfiles += read_dir_or_file(dbase, form, pathbuf);
	}
	dbase->havesects = TRUE;
	return(nfiles > 0);
}


/*
 * read one plain file and append to dbase section list. Return FALSE on error.
 */

static BOOL read_file(
	DBASE		*dbase,		/* form and items to write */
	FORM		*form,		/* contains column delimiter */
	char		*path,		/* file to read list from */
	time_t		mtime)		/* file modification time */
{
	SECTION		*sect;		/* new section */
	FILE		*fp;		/* open file */
	int		nc = 0;		/* size of curr line */
	int		col = 0;	/* current column being added*/
	int		row = -1;	/* current row being added */
	char		*buf, *p;	/* buffer for one item */
	int		bindex = 0;	/* next free byte in buf */
	int		bsize;		/* size of buf in bytes */
	char		c, c0;		/* next char from file */
	BOOL		error;		/* in TRUE, abort */
	ITEM		*item=0;	/* for time/date conversion */
	int		i;

							/* step 1: open file */
	signal(SIGPIPE, broken_pipe_handler);
	if (form->proc) {
		char cmd[1024];
		path = resolve_tilde(path);
		sprintf(cmd, "%s -r %s", path, form->name);
		if (!(fp = popen(cmd, "r")))
			return(FALSE);
	} else {
		path = resolve_tilde(path);
		if (!(fp = fopen(path, "r")))
			return(FALSE);
	}
							/* step 2: new sectn */
	i = (dbase->nsects+1) * sizeof(SECTION);
	if (!(sect = dbase->sect ? realloc(dbase->sect, i) : malloc(i))) {
		create_error_popup(toplevel, errno,
			"No memory for section %s", path);
		form->proc ? pclose(fp) : fclose(fp);
		return(FALSE);
	}
	dbase->sect = sect;
	mybzero(sect = &dbase->sect[dbase->nsects], sizeof(SECTION));
	dbase->currsect = dbase->nsects++;
	sect->mtime = mtime;
	sect->path  = mystrdup(path);
							/* step 3: read list */
	buf = (char  *)malloc((bsize = BCHUNK) * sizeof(char));
	error = !buf;
	while (!error) {					/* read file:*/
		c = c0 = fgetc(fp);
		if (!feof(fp) && c == ESC)
			c = fgetc(fp);
								/* end of str*/
		if (feof(fp) || c0 == form->cdelim || c0 == R_SEP) {
			if (!nc) {
				if (c0 == form->cdelim) col++;
				if (feof(fp))	break;
				else		continue;
			}
			buf[bindex] = 0;			/* next col */
								/* .. date? */
			for (i=-1, p=buf; *p; p++)
				if (*p=='.' || *p==':' || *p=='/') {
					for (i=form->nitems-1; i >= 0; i--) {
						item = form->items[i];
						if (item->column == col)
							break;
					}
					break;
				}
			if (i >= 0 && item->type == IT_TIME)	/* .. -> int */
				switch(item->timefmt) {
				  case T_DATE:
					sprintf(buf, "%d", (int)
						parse_datestring(buf, 0));
					break;
				  case T_TIME:
					sprintf(buf, "%d", (int)
						parse_timestring(buf, FALSE));
					break;
				  case T_DATETIME:
					sprintf(buf, "%d", (int)
						parse_datetimestring(buf));
					break;
				  case T_DURATION:
					sprintf(buf, "%d", (int)
						parse_timestring(buf, TRUE));
				}
								/* store str */
			if (row < 0)
				if (error |= !dbase_addrow(&row, dbase))
					break;
			dbase_put(dbase, row, col++, buf);
			bindex = 0;
			if (feof(fp) || c0 == R_SEP) {		/* next row */
				col = nc = 0;
				row = -1;
				if (feof(fp))
					break;
			}
		} else {					/* store char*/
			if (bindex+1 >= bsize) {
				char *new = (char *)realloc(buf,
						(bsize += BCHUNK));
				if (error |= !new)
					break;
				buf = new;
			}
			buf[bindex++] = c;
			nc++;
		}
	}
								/* done. */
	free(buf);
	error |= form->proc ? !!pclose(fp) : !!fclose(fp);
	sect->modified = FALSE;
	sect->rdonly   =
	dbase->rdonly  = form->proc ? FALSE : !!access(path, W_OK);
	if (error)
		create_error_popup(toplevel, errno,
			"Failed to allocate memory for\ndatabase %s", path);
	return(!error);
}
#endif /* GROK || PLANGROK */
