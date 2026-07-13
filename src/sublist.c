/*
 * sublist management: a sublist is an array of pointer pointing to
 * specific entries in the master list (see dbase.c), for example all
 * entries on the same day. These extractions are done here.
 * Returns pointer to sublist, or 0 if there are no entries.
 *
 *	create_sublist(list, w)
 *	destroy_sublist(w)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <Xm/Xm.h>
#include "cal.h"

#define CHUNK		100			/* sublist allocation unit */

#if defined(SUN) && !defined(SOLARIS2)
#define regcmp re_comp
#define regex re_exec
#endif
#if defined(linux) || defined(__EMX__) || defined(__FreeBSD_kernel__)
#define regcmp regcomp
#define regex regexec
#endif
#ifndef NOREGEX
extern char		*regcmp(), *regex(), *__loc1;
#endif
extern int		search_mode;		/* 0=case, 1=lit, 2=regex */
extern struct user	*user;			/* user list (from file_r.c) */
static void		append_entry(struct sublist **, struct entry *);
static BOOL		keymatch(char *, char *);


/*
 * create a sublist that describes the contents of a list menu. A sublist
 * is an array of pointers to schedule entries (which live in the main list).
 * The sublist is put into the rows of the list menu in order, but there
 * are usually more menu rows than sublist entry pointers (so we have a
 * few blank rows for new input).
 * This routine is called whenever a list menu is created, and whenever an
 * entry changes in any way. That isn't very efficient if the main list has
 * hundreds or thousands of entries, so this may change in future versions.
 */

void create_sublist(
	struct plist	*list,		/* main list with all entries*/
	struct listmenu	*w)		/* menu we are doing this for*/
{
	char		*key = w->key;	/* keyword to look for */
	BOOL		found;		/* TRUE if lookup succeeded */
	struct lookup	lookup;		/* result of entry lookup */
	struct entry	*ep;		/* next entry to check */
	int		i;		/* entry counter */

	list->locked++;
	destroy_sublist(w);				/* zap old list */
	ep = list->entry;
	if (w->time) {					/*--- time range ---*/
		time_t	begin = w->time;
		time_t	end   = w->period ? begin+w->period : begin+86400;
		found = lookup_entry(&lookup, list, begin, TRUE, FALSE,
								'*', -1);
		while (found && lookup.trigger >= begin
			     && lookup.trigger <  end) {
			if (!w->own_only || !list->entry[lookup.index].user
					 || !strcmp(user[0].name,
					       list->entry[lookup.index].user))
				append_entry(&w->sublist,
						&list->entry[lookup.index]);
			found = lookup_next_entry(&lookup);
		}
	} else if (w->key) {				/*--- keywords ---*/
		if (search_mode == 1) {
			char *k;
			if (!(key = mystrdup(key))) {
				list->locked--;
				return;
			}
			for (k=key; *k; k++)
				if (isupper(*k)) *k = tolower(*k);
		}
#ifndef NOREGEX
		if (search_mode == 2)
			if (!(key = regcmp(key, 0))) {
				list->locked--;
				return;
			}
#endif
		for (ep=list->entry, i=0; i < list->nentries; i++, ep++)
			if (((ep->message && keymatch(key, ep->message)) ||
			     (ep->script  && keymatch(key, ep->script )) ||
			     (ep->note    && keymatch(key, ep->note   ))) &&
					      (!w->own_only || !ep->user ||
					      !strcmp(user[0].name, ep->user)))
				append_entry(&w->sublist, ep);
#ifndef NOREGEX
		if (search_mode == 2)
			free(key);
#endif
		if (search_mode == 1)
			free(key);

	} else if (w->oneentry) {			/*--- one entry --- */
		for (i=0; i < list->nentries; i++, ep++)
			if (w->oneentry == ep &&
					     (!w->own_only || !ep->user ||
					     !strcmp(user[0].name, ep->user)))
				append_entry(&w->sublist, w->oneentry);

	} else if (w->private) {			/*--- private --- */
		for (i=0; i < list->nentries; i++, ep++)
			if (ep->private)
				append_entry(&w->sublist, ep);

	} else if (w->user_plus_1) {			/*--- one file --- */
		for (i=0; i < list->nentries; i++, ep++)
			if ((w->user_plus_1 == 1 && !ep->user) ||
			     w->user_plus_1 == name_to_user(ep->user)+1)
				append_entry(&w->sublist, ep);

	} else {					/*--- all ---*/
		for (i=0; i < list->nentries; i++, ep++)
			if (!w->own_only || !ep->user ||
			    !strcmp(user[0].name, ep->user))
				append_entry(&w->sublist, ep);
	}
	list->locked--;
}


/*
 * destroy the sublist of a list menu. De-allocate it, and decrement the
 * entry reference counts.
 */

void destroy_sublist(
	struct listmenu		*w)		/* menu we are doing this for*/
{
	if (w->sublist) {
		free(w->sublist);
		w->sublist = 0;
	}
}


/*
 * for generating sublist by keyword: return TRUE if the keyword is in the
 * string. This routine is time-critical, it is moderately optimized. It
 * assumes that search strings are short; it could stop scanning earlier.
 * In case insensitive mode, <key> is expected to contain lower case only.
 * In regexp mode, <key> is expected to be a compiled regular expression.
 */

static BOOL keymatch(
	char		*key,		/* keyword to look for */
	char		*string)	/* text to look in */
{
	char		*p;		/* fast string scan ptr */
	char		key0 = key[0];	/* first char of key */
	char		c;		/* temp for case conversion */
	int		i;		/* scan index */
	int		len = strlen(key);

	switch(search_mode) {
	  default:
	  case 0:
		for (p=string; *p; p++)
			if (p[0] == key0 &&
			    !strncmp(key+1, p+1, len-1))
				return(TRUE);
		return(FALSE);

	  case 1:
		for (p=string; *p; p++) {
			c = p[0];
			if (isupper(c)) c = tolower(c);
			if (c == key0) {
				for (i=1; i < len; i++) {
					c = p[i];
					if (isupper(c)) c = tolower(c);
					if (c != key[i])
						break;
				}
				if (i == len)
					return(TRUE);
			}
		}
		return(FALSE);

#ifndef NOREGEX
	  case 2: {
		char dummy[1024];
		return(regex(key, string,
				dummy, dummy, dummy, dummy, dummy,
				dummy, dummy, dummy, dummy, dummy) != 0);
		}
#endif
	}
}


/*----------------------------- sublist management for use by above routines */
/*
 * append an entry to a sublist. There is no sorting because the main list
 * is sorted and will be scanned in order when a sublist is generated from it.
 * A pointer to a pointer is passed; the pointed-to pointer changes if the
 * list is re-allocated (or allocated for the first time). To start a new
 * list, pass a pointer to a null pointer; to get rid of a list, free() it.
 * Nothing is ever removed from a sublist; it is remade every time.
 */

static void append_entry(
	struct sublist	**sub,		/* list to append to */
	struct entry	*entry)		/* entry to append */
{
	int		num;		/* # of entries in old list */
	int		size;		/* size of allocated old list*/
	struct sublist	*new;		/* larger list if required */
	struct entry	**p, **q;	/* entry copy pointers */
	int		i;		/* entry copy counter */

	num  = *sub ? (*sub)->nentries : 0;
	size = *sub ? (*sub)->size     : 0;
	if (num+1 > size) {				/* need larger list */
		size += CHUNK;
		if (!(new = (struct sublist *)malloc(sizeof(struct sublist) +
					  sizeof(struct entry *) * size)))
			fatal(_("no memory"));
		if (!*sub)
			new->nentries = 0;		/* start new list... */
		else {
			new->nentries = num;		/* ...or copy old */
			p = (*sub)->entry;
			q = new->entry;
			for (i=0; i < num; i++)
				*q++ = *p++;
		}
		new->size = size;
		if (*sub)				/* discard old list */
			free(*sub);
		*sub = new;
	}
	(*sub)->entry[num] = entry;			/* append entry */
	(*sub)->nentries++;
}
