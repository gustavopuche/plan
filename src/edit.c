/*
 * Handle all the more complicated user interactions with an entry list menu.
 * Whenever the user begins editing a row, the entry in that row is copied
 * to the edit struct. All editing takes place, until it is confirmed and
 * put back into the main list. The reason for this is that entering a new
 * date or time doesn't make the row disappear from the current list because
 * it now belongs to a different day before editing is finished.
 *
 *	got_entry_press(w,lw,x,y,on,b)	The user pressed on one of the
 *					buttons in the list menu button array.
 *	confirm_new_entry()		Puts the currently edited row back
 *					into the mainlist, and re-sorts.
 *	undo_new_entry()		Kill the edit buffer, undoing the
 *					changes to the current row.
 *	got_entry_text(lw, x, y, text)	The user entered text into the text
 *					button. There is never more than one.
 *	sensitize_edit_buttons()	Desensitize all buttons in the
 *					bottom button row that don't make
 *					sense at the moment.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/LabelP.h>
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/Protocols.h>
#include "cal.h"

extern Display		*display;	/* everybody uses the same server */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern struct config	config;		/* global configuration data */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern Widget		mainwindow;	/* popup menus hang off main window */
extern struct user	*user;		/* user list (from file_r.c) */
extern BOOL		interactive;	/* interactive or fast appt entry? */
extern int		curr_month;	/* month being displayed, 0..11 */
extern int		curr_year;	/* year being displayed, since 1900 */
extern time_t		last_x_timer;	/* when did last X timer happen? */

struct edit		edit;		/* info about entry being edited */


/*
 * user selection popup is up, and the user has made a selection. Store the
 * new user number in the edited appointment. The user selection popup had
 * been installed by got_entry_press.
 */

static void got_new_user(
	int	newuser)
{
	int	olduser;

	olduser = name_to_user(edit.entry.user);
	if (!prepare_for_modify(user[olduser].name) ||
	    !prepare_for_modify(user[newuser].name))
		return;
	if (edit.entry.user)
		free(edit.entry.user);
	edit.entry.user = newuser<0 ? 0 : mystrdup(user[newuser].name);
	edit.changed = TRUE;
	edit.editing = TRUE;
	confirm_new_entry();
	update_all_listmenus(TRUE);
	sensitize_edit_buttons();
}


/*
 * user clicked on one of the buttons in the appointment entry menu
 */

void got_entry_press(
	struct listmenu	*lw,		/* which menu */
	int		x,		/* which column/row */
	int		y,
	int		on,		/* enable radio but: on/off */
	int		shift)		/* arrow but: 1=shift key */
{
	struct entry	*ep;		/* affected entry */
	int		num;		/* number of entries so far */
	time_t		now, temp;	/* current time */
	int		inc;		/* if arrow button, +1 or -1 */
	int		xp = 0;		/* after inc/dec, reprint x */

	destroy_user_sel_popup();
	num = lw->sublist ? lw->sublist->nentries : 0;
	if (y-1 > num)					/* illegal entry? */
		return;
	if (y-1 < num) {
		ep = lw->sublist->entry[y-1];
		if (user[name_to_user(ep->user)].readonly)
			switch(x) {			/* readonly: show */
			  case SC_MESSAGE:
				create_text_popup(_("Message"),
						&ep->message, x, TRUE);
				return;

			  case SC_SCRIPT:
				if (!ep->notime)
					create_text_popup(_("Shell Script"),
						&ep->script, x, TRUE);
				return;
			}
		if (!prepare_for_modify(lw->sublist->entry[y-1]->user)) {
			if (x == SC_ENABLE)
				XtVaSetValues(lw->entry[y][x], XmNset, !on, 0,
						NULL);
			return;
		}
	}
	if (edit.editing && (edit.y != y || edit.menu != lw))
		confirm_new_entry();
	num = lw->sublist ? lw->sublist->nentries : 0;
	inc = x < SC_D ? 1 : -1;
	if (!edit.editing) {
		if (y-1 < num) {
			clone_entry(&edit.entry, lw->sublist->entry[y-1]);
			if (!server_entry_op(&edit.entry, 'l')) {
				create_error_popup(lw->shell, 0, "%s\n%s\n",
					_("Somebody else is editing this "\
					  "entry. Please try again later."));
				update_all_listmenus(FALSE);
				return;
			}
		} else {
			if (x != SC_DATE && x != SC_TIME && x != SC_NOTE) {
				create_error_popup(lw->shell, 0,
					_("Please enter time or date first"));
				update_all_listmenus(FALSE);
				return;
			}
			clone_entry(&edit.entry, (struct entry *)0);
			now = get_time();
			edit.entry.time = lw->time ? lw->time : now - now%60;
			edit.entry.notime = x == SC_NOTE;
			edit.entry.late_warn  = config.late_time;
			edit.entry.early_warn = config.early_time;
		}
		edit.editing	= TRUE;
		edit.changed	= FALSE;
		edit.y		= y;
		edit.menu	= lw;
		edit.srcentry	= 0;
		sensitize_edit_buttons();
	}
	ep = &edit.entry;
	switch(x) {
	  case SC_ENABLE:
		ep->suspended = !on;
		edit.changed = TRUE;
		sensitize_edit_buttons();
		update_all_listmenus(FALSE);
		break;

	  case SC_USER:
		create_user_sel_popup(lw->entry[y][x], name_to_user(ep->user),
						(BOOL (*)())got_new_user);
		break;

	  case SC_TIME:
		if (lw->time && !lw->period &&
				(!lw->sublist || y > lw->sublist->nentries))
			print_button(lw->entry[y][SC_DATE],
				mkdatestring(lw->time ? lw->time : now));
		edit_list_button(1, lw, x, y);
		break;

	  case SC_LENGTH:
		if (ep->notime)
			break;
	  case SC_ENDDATE:
	  case SC_DATE:
	  case SC_ADVDAYS:
	  case SC_NOTE:
		if (x == SC_ENDDATE)
			destroy_recycle_popup();
		edit_list_button(1, lw, x, y);
		break;

	  case SC_ADVANCE:
		if (ep->notime)
			break;
		edit_list_button(1, lw, x, y);
		break;

	  case SC_RECYCLE:
		create_recycle_popup();
		break;

	  case SC_MESSAGE:
		create_text_popup(_("Message"), &edit.entry.message, x, FALSE);
		break;

	  case SC_SCRIPT:
		if (ep->notime)
			break;
		create_text_popup(_("Shell Script"), &edit.entry.script, x,
									FALSE);
		break;

	  case SC_EXCEPT:
		create_except_popup();
		break;

	  case SC_PRIVATE:
		ep->private ^= TRUE;
		got_new_user(0);
		break;

	  case SC_TODO:
		print_pixmap(lw->entry[y][SC_TODO], PIC_TODO, ep->todo^=TRUE);
		edit.changed = TRUE;
		edit.entry.suspended = FALSE;
		sensitize_edit_buttons();
		break;

	  case SC_I_DATE:
	  case SC_D_DATE:
		temp = inc * (shift ? 7 : 1) * 86400;
		ep->time += temp;
		if (ep->rep_last) {
			ep->rep_last += temp;
			print_button(lw->entry[y][SC_ENDDATE],
					mkbuttonstring(lw, SC_ENDDATE, y));
		}
		xp = SC_DATE;
		break;

	  case SC_D_ENDDATE:
	  	if (ep->rep_every != 86400 ||
		    ep->rep_last < ep->time - ep->time % 86400 + 86400)
			break;
	  case SC_I_ENDDATE:
		destroy_recycle_popup();
		if (ep->rep_every != 86400 || !ep->rep_last)
			ep->rep_last = ep->time - ep->time % 86400;
		ep->rep_last += inc * (shift ? 7 : 1) * 86400;
		ep->rep_every = 86400;
	  	if (ep->rep_last < ep->time - ep->time % 86400 + 86400) {
			ep->rep_last  = 0;
			ep->rep_every = 0;
		}
		print_pixmap(lw->entry[y][SC_RECYCLE], PIC_RECYCLE,
					ep->rep_every  || ep->rep_days ||
					ep->rep_yearly || ep->rep_weekdays);
		xp = SC_ENDDATE;
		break;

	  case SC_I_TIME:
	  case SC_D_TIME:
		temp = ep->time % 86400;
		temp += inc * (shift ? 60 : 5) * 60;
		ep->time += temp % 86400 - ep->time % 86400;
		xp = SC_TIME;
		break;

	  case SC_I_LENGTH:
	  case SC_D_LENGTH:
		if (ep->notime)
			break;
	  	ep->length += inc * (shift ? 60 : 5) * 60;
		if (ep->length < 0)
			ep->length = 0;
		if (ep->length > 86400 - ep->time % 86400)
			ep->length = 86400 - ep->time % 86400;
		xp = SC_LENGTH;
		break;

	  case SC_I_ADVANCE:
	  case SC_D_ADVANCE: {
		time_t *warn = shift ? &ep->early_warn : &ep->late_warn;
		*warn += inc * 5 * 60;
		if (*warn < 0)
			*warn = 0;
		if (*warn > ep->time % 86400)
			*warn = ep->time % 86400;
		xp = SC_ADVANCE;
		break; }

	  case SC_I_ADVDAYS:
	  case SC_D_ADVDAYS:
		ep->days_warn += inc * (shift ? 7 : 1) * 86400;
		if (ep->days_warn < 0)
			ep->days_warn = 0;
		xp = SC_ADVDAYS;
		break;
	}
	if (xp) {
		print_button(lw->entry[y][xp], mkbuttonstring(lw, xp, y));
		edit.changed = TRUE;
		edit.entry.suspended = FALSE;
		sensitize_edit_buttons();
	}
}


/*
 * confirm new entry, put it in the list. If the user has changed, mark
 * the old and the new user modified (if non-null), or reclassify as own
 * if old or new user is read-only. If it ends up with a different user,
 * tell the old server to delete and the new server to add.
 * If it's more than 30 seconds since the last timer event, assume it's
 * dead (bug or incompatibility in the XFree server as of June 99) and
 * write synchronously from now on.
 */

void confirm_new_entry(void)
{
	BOOL		changed;
	unsigned int	new_file;
	unsigned int	old_file = edit.entry.file;
	struct user	*u;
	BOOL            addentry = TRUE;

	static char werror_msg[] = "The netplan server reported an error\n"
				   "when this record was committed. Check\n"
				   "access permissions.\n";
	destroy_user_sel_popup();
	edit_list_button(0, edit.menu, 0, 0);
	destroy_text_popup();
	destroy_recycle_popup();
	destroy_except_popup();
	changed = edit.editing && edit.changed;
	if (changed) {
		struct entry *ep = 0;
		mainlist->locked++;
		if (!edit.menu)
			ep = edit.srcentry;
		else if (edit.menu->sublist &&
				edit.y-1 < edit.menu->sublist->nentries)
			ep = edit.menu->sublist->entry[edit.y-1];
		if (!edit.entry.user && user[0].name)
			edit.entry.user = mystrdup(user[0].name);
		u = &user[name_to_user(edit.entry.user)];
		new_file = u->file_id;
		if (edit.entry.private && ep && !ep->private) {	/* private */
			server_entry_op(&edit.entry, 'u');
			server_entry_op(&edit.entry, 'd');
			edit.entry.file = 0;
			edit.entry.id   = 0;

		} else if (!edit.entry.id) {			/* new */
			edit.entry.file = new_file;
			if (u == &user[0] && user[0].readonly) {
				prepare_for_modify(0);
				addentry = FALSE;
			} else if (!server_entry_op(&edit.entry, 'w')) {
				create_error_popup(0, 0, _(werror_msg));
				edit.entry.file = old_file;
			}
		} else if (new_file == edit.entry.file) {	/* same file */
			if (!server_entry_op(&edit.entry, 'w')) {
				create_error_popup(0, 0, _(werror_msg));
				addentry = FALSE;
			}
			server_entry_op(&edit.entry, 'u');
		} else {					/* diff file */
			unsigned int old_id = edit.entry.id;
			server_entry_op(&edit.entry, 'u');
			server_entry_op(&edit.entry, 'd');
			edit.entry.file = new_file;
			edit.entry.id   = 0;
			if (!server_entry_op(&edit.entry, 'w')) {
				create_error_popup(0, 0, _(werror_msg));
				edit.entry.file = old_file;
				edit.entry.id   = old_id;
			}
		}
		if (ep)
			delete_entry(mainlist, ep - mainlist->entry);
		if (addentry) {
			(void)add_entry(&mainlist, &edit.entry);
			rebuild_repeat_chain(mainlist);
		}
		mainlist->locked--;
	}
	destroy_entry(&edit.entry);
	edit.editing = FALSE;
	edit.changed = FALSE;
	if (interactive) {
		sensitize_edit_buttons();
		if (changed) {
			/* new entry vanishes from edit menu if arg is FALSE */
			update_all_listmenus(TRUE);
			redraw_all_views();
			mainlist->modified = TRUE;
			if (last_x_timer != (time_t)-1 &&
			    last_x_timer < get_time()-30) {
				fprintf(stderr, "plan: X server fails to send "
						"timer events, switching to sy"
						"nchronous file writing. Time "
						"displays will be wrong.\n");
				last_x_timer = (time_t)-1;
			}
			if (last_x_timer == (time_t)-1)
				write_mainlist();
		}
	}
}


/*
 * undo all editing, restore the entry before we started editing
 */

void undo_new_entry(void)
{
	if (edit.editing) {
		server_entry_op(&edit.entry, 'u');
		destroy_text_popup();
		destroy_recycle_popup();
		destroy_entry(&edit.entry);
		edit.editing = FALSE;
		edit.changed = FALSE;
		sensitize_edit_buttons();
		draw_list(edit.menu);
	}
}


/*
 * user entered text into one of the buttons in the entry list
 */

void got_entry_text(
	struct listmenu	*lw,		/* which menu */
	int		x,		/* which column/row */
	int		y,
	char		*text)		/* text input by user */
{
	struct entry	*ep;		/* affected entry */
	char		*p;		/* to remove trailing \n's */
	time_t		temp;

	while (*text == ' ' || *text == '\t')
		text++;
	ep = &edit.entry;
	switch(x) {
	  case SC_DATE:
		if (*text && (temp = parse_datestring(text, 0)) > EON)
			ep->time = ep->time % 86400 + temp;

		edit_list_button(1, lw, SC_ENDDATE, y);
		print_button(lw->entry[y][SC_DATE],
					mkbuttonstring(lw, SC_DATE, y));
		break;

	  case SC_ENDDATE:
		if (*text && (temp = parse_datestring(text, 0)) > EON) {
			ep->rep_last  = temp;
			ep->rep_every = 86400;

		} else if (ep->rep_last && ep->rep_every == 86400) {
			ep->rep_last  = 0;
			ep->rep_every = 0;
		}
		edit_list_button(1, lw, SC_TIME, y);
		print_button(lw->entry[y][SC_ENDDATE],
					mkbuttonstring(lw, SC_ENDDATE, y));
		break;

	  case SC_TIME:
		if (*text == '-') {
			ep->notime = TRUE;
			ep->time -= ep->time % 86400;
		} else if (*text) {
			ep->notime = FALSE;
			ep->time -= ep->time % 86400;
			ep->time += parse_timestring(text, FALSE);
		}
		edit_list_button(1,lw, ep->notime ? SC_NOTE : SC_LENGTH, y);
		print_button(lw->entry[y][SC_TIME],
					mkbuttonstring(lw, SC_TIME, y));
		print_button(lw->entry[y][SC_LENGTH],
					mkbuttonstring(lw, SC_LENGTH, y));
		print_button(lw->entry[y][SC_ADVANCE],
					mkbuttonstring(lw, SC_ADVANCE, y));
		break;

	  case SC_LENGTH: {
		time_t length;
		length = *text==0   ? 0 :
			 *text=='-' ? parse_timestring(text+1, FALSE) -
								 ep->time%86400
				    : parse_timestring(text,   FALSE);
		if (length < 0)
			length = 0;
		if (length > 86400)
			length = 86400;
		ep->length = length;
		edit_list_button(1, lw, ep->notime ? SC_NOTE:SC_ADVANCE, y);
		print_button(lw->entry[y][SC_LENGTH],
					mkbuttonstring(lw, SC_LENGTH, y));
		break; }

	  case SC_ADVANCE:
		parse_warnstring(text, &ep->early_warn, &ep->late_warn,
							&ep->noalarm);
		edit_list_button(1, lw, SC_ADVDAYS, y);
		print_button(lw->entry[y][SC_ADVANCE],
					mkbuttonstring(lw, SC_ADVANCE, y));
		break;

	  case SC_ADVDAYS:
		ep->days_warn = atoi(text) * 86400;
		if (ep->days_warn < 0)
			ep->days_warn = 0;
		edit_list_button(1, lw, SC_NOTE, y);
		print_button(lw->entry[y][SC_ADVDAYS],
					mkbuttonstring(lw, SC_ADVDAYS, y));
		break;

	  case SC_NOTE:
		if (ep->note)
			free(ep->note);
		ep->note = 0;
		for (p=text+strlen(text)-1; p != text &&
			(*p == '\n' || *p == '\t' || *p == ' '); *p-- = 0);
		for (p=text; *p; p++)
			if (*p == '\n') *p = ' ';
		if (*text)
			ep->note = mystrdup(text);
		edit_list_button(0, lw, 0, 0);
		print_button(lw->entry[y][SC_NOTE], "%s",
					mkbuttonstring(lw, SC_NOTE, y));
		XmProcessTraversal(lw->done, XmTRAVERSE_CURRENT);
	}
	if (x == SC_DATE || x == SC_ENDDATE || x == SC_TIME)
		if (ep->rep_last && ep->rep_last < ep->time) {
			create_error_popup(lw->shell, 0,
				_("The new date is after the last-trigger "\
				  "date,\n %s. The last-trigger date has been"\
				  " disabled."), mkdatestring(ep->rep_last));
			ep->rep_every = 0;
			ep->rep_last  = 0;
		}
	edit.changed = TRUE;
	edit.entry.suspended = FALSE;
#if 1						/* 1=Return in note confirms */
	if (x == SC_NOTE) /* eoln */
		confirm_new_entry();
#endif
	sensitize_edit_buttons();
}


/*
 * while a new entry is being edited, it is kept in the <edit> struct,
 * which overrides the appropriate entry (if any) in the mainlist. If
 * it is being edited, the Confirm button should be sensitive. If it is
 * being edited and has been changed, the Undo button should be sensitive.
 */

void sensitize_edit_buttons(void)
{
	Arg			args;

	if (edit.menu) {
		XtSetArg(args, XmNsensitive, edit.changed);
		XtSetValues(edit.menu->confirm, &args, 1);
		XtSetArg(args, XmNsensitive, edit.changed);
		XtSetValues(edit.menu->undo,    &args, 1);
		XtSetArg(args, XmNsensitive, edit.editing);
		XtSetValues(edit.menu->dup,     &args, 1);
		XtSetArg(args, XmNsensitive, edit.editing);
		XtSetValues(edit.menu->del,     &args, 1);
	}
}
