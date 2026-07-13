/*
 * foreign language support. Every string to translate is enclosed in _().
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>
#include <Xm/Xm.h>
#include "cal.h"

static char **read_language_file(char *);

extern struct config	config;		/* global configuration data */
extern Widget		mainwindow;	/* popup menus hang off main window */

static char *targetlang;	/* target language name */
static int  nstrings;		/* number of strings in english/target lists */
static char **english;		/* English strings */
static char **target;		/* target language strings */


/*
 * All translatable strings in this program are passed through this function.
 * If the configured language is English, just return that string. Otherwise
 * read the appropriate language file. Then look up the English string in the
 * English string file. If it is found to be the n-th string in that file,
 * the n-th string from the non-English language file is returned. Just to
 * be safe, all messages in this file are always English.
 */

char *_(
	char		*eng)		/* English string to substitute */
{
	static BOOL	busy;
	int		l, m, h, d;

	if (busy || !config.language)
		return(eng);
	if (!strcmp(config.language, "english")) {
		free(config.language);
		config.language = 0;
		return(eng);
	}
						/* new language: free old */
	busy = TRUE;
	if (!targetlang)
		targetlang = mystrdup(config.language);

	else if (strcmp(targetlang, config.language)) {
		free(targetlang);
		targetlang = mystrdup(config.language);
		if (target) {
			free(target);
			target = 0;
		}
	}
	if ((!english && !(english = read_language_file("english"))) ||
	    (!target  && !(target  = read_language_file(targetlang)))) {
		if (mainwindow) {
			free(config.language);
			free(targetlang);
			config.language = targetlang = 0;
		}
		busy = FALSE;
		return(eng);
	}
	for (l=0, h=nstrings-1; l <= h; ) {
		m = l + (h - l)/2;
		if (!(d = strncmp(eng, english[m], 32))) {
			busy = FALSE;
			return(target[m]);
		}
		if (d < 0)
			h = m - 1;
		else
			l = m + 1;
	}
	fprintf(stderr, "error in plan.lang.english: missing string: \"%s\"\n",
									eng);
	busy = FALSE;
	return(eng);
}


/*
 * read a language file into a message list. First read the file, then count
 * the lines and build a memory block consisting of a pointer table followed
 * by the strings. It's easier than messing with fgets.
 */

static char **read_language_file(
	char		*lang)		/* name of language */
{
	FILE		*fp;		/* open language file */
	char		name[256];	/* language file name */
	char		path[1024];	/* language file path */
	int		fsize, n, nline;/* for message size calculation */
	char		*file, *p, *q;	/* file in memory */
	char		**list, **pp;	/* returned message list */

	sprintf(name, "plan.lang.%.240s", lang);
	if (name[10] >= 'A' && name[10] <= 'Z')
		name[10] += 'a'-'A';
	if (!find_file(path, name, FALSE)) {
		create_error_popup(0, 0,
			"Cannot find language file \"%s\"", name);
		return(0);
	}
	if (!(fp = fopen(path, "r"))) {
		create_error_popup(0, errno,
			"Cannot open language file\n\"%s\"", path);
		return(0);
	}
	fseek(fp, 0, 2);				/* read file into mem*/
	fsize = ftell(fp);
	rewind(fp);
	if (!(file = (char *)malloc(fsize))) 
		fatal("no memory for language file");
	if ((int)fread(file, 1, fsize, fp) != fsize) {
		fclose(fp);
		create_error_popup(0, errno,
			"Cannot read language file\n\"%s\"", path);
		return(0);
	}
	fclose(fp);
	for (nline=n=0; n < fsize; n++)			/* count lines */
		nline += file[n] == '\n';
	if ((nstrings && nstrings != nline) || nline < 300 || nline > 500) {
		create_error_popup(0, errno,
			"Language file \"%s\"\nhas unexpected number of lines",
									path);
		return(0);
	}
	if (!nstrings)
		nstrings = nline;
							/* create list */
	if (!(list = (char **)malloc(nline * sizeof(char *) + fsize+1)))
		fatal("no memory for language file");
	memcpy(list[0] = p = (char *)&list[nline], file, fsize);
	p[fsize] = 0;
							/* eval "\n", eoln=0 */
	for (q=p; *p; p++, q++)
		if      (*p   == '\n')	*q = 0;
		else if (*p   != '\\')	*q = *p;
		else if (*++p == 'n')	*q = '\n';
		else if (*p   == 't')	*q = '\t';
		else			*q = *p;
							/* find lines */
	for (p=list[0], pp=list+1, n=0; n < nline-1; n++) {
		while (*p) p++;
		*pp++ = ++p;
	}
	free(file);
	return(list);
}


/*
 * return the name of the n-th available language. Language names are taken
 * from file names in $LIB whose names are "plan.lang_" followed by the
 * language name. The first language is always "english". If there is no
 * n-th language, return 0.
 */

char *get_language_name(
	int		n)		/* number of language */
{
	char		path[1024], *p;	/* language file path */
	static DIR	*dir;		/* open directory file */
	struct dirent	*dp;		/* one directory entry */
	static char	name[64];	/* returned name */

	if (!n) {
		if (dir)
			closedir(dir);
		dir = 0;
		if (find_file(path, "plan.lang.english", FALSE)) {
			if ((p = strrchr(path, '/')))
				*p = 0;
			dir = opendir(p ? path : ".");
		}
		return("English");
	}
	if (!dir)
		return(0);
	for (;;) {
		if (!(dp = readdir(dir))) {
			closedir(dir);
			dir = 0;
			return(0);
		}
		if (!strncmp(dp->d_name, "plan.lang.", 10) &&
				strcmp(dp->d_name, "plan.lang.english")) {
			strncpy(name, dp->d_name+10, sizeof(name));
			name[sizeof(name)-1] = 0;
			if (*name >= 'a' && *name <= 'z')
				*name -= 'a'-'A';
			return(name);
		}
	}
}
