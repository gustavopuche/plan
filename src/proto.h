/*
 * all prototypes
 */
/*---------------------------------------- main.c ------------*/

extern XtInputId register_X_input(	/* tell X about new server connection*/
	int		fd);		/* new connection */
extern void unregister_X_input(		/* tell X when server conn. is down */
	XtInputId	x_id);		/* closed connection */
extern void resynchronize_daemon(void);	/* tell daemon to re-read database */
extern void set_color(			/* set foregnd color to one of COL_* */
	int		col);
extern void print_pixmap(		/* prints pixmap into a Pixmap label */
	Widget		w,		/* button to draw into */
	int		n,		/* pixmap #, one of PIC_* */
	BOOL		b);		/* true: black, false: grayed*/
extern void do_get_rsrc(		/* read resource */
	XtPointer	ret,
	char		*res_name,
	char		*res_class_name,
	char		*res_type,
	int		minvalue);
extern char *mystrdup(			/* strdup replacement for Ultrix */
	char		*s);
extern void exit(			/* local version of exit w/ writeback*/
	int		ret);
extern void fatal(			/* print error message and exits */
	char		*fmt, ...);
extern void print_button(		/* print a string into label or but */
	Widget		w,
	char		*fmt, ...);
extern void print_text_button(		/* print a string into a Text button */
	Widget		w,
	char		*fmt, ...);
#ifdef MIPS
extern char *getenv(const char *);
extern char *malloc(size_t);
extern char *realloc(void *, size_t);
#endif

/*---------------------------------------- convert.c ------------*/

extern char *mkdatestring(		/* convert # of seconds to a date */
	time_t		time);		/* date in seconds */
extern char *mktimestring(		/* convert # of seconds to a t-o-d */
	time_t		time,		/* date in seconds */
	BOOL		dur);		/* duration, not time-of-day */
extern time_t parse_datestring(		/* convert date string to # of secs */
	char		*text,		/* input string */
	time_t		dtime);		/* appt start time, 0=today */
extern time_t parse_timestring(		/* convert time string to # of secs */
	char		*text,		/* input string */
	BOOL		duration);	/* time-of-day or duration? */
extern time_t parse_datetimestring(	/* convert date+time to # of secs */
	char		*text);		/* input string */
extern void parse_warnstring(		/* early/late warning time, noalarm */
	char		*text,		/* input string */
	time_t		*early,		/* early warning in minutes */
	time_t		*late,		/* late warning in minutes */
	BOOL		*noalarm);	/* final alarm too? */

/*---------------------------------------- cyccalc.c ------------*/

extern time_t length_of_month(		/* # of days in the month <time> */
	time_t		time);		/* some date in the month */
extern BOOL recycle_all(		/* recycle all entries in <list> */
	struct plist	*list,		/* list to scan */
	BOOL		del,		/* auto-remove obsolete? */
	int		days);		/* if del, keep if age < days*/
extern int  recycle(			/* get next repeating trigger time */
	struct entry	*entry,		/* entry to test */
	time_t		today,		/* today (time is ignored) */
	time_t		*ret);		/* returned next trigger time*/
extern BOOL move_todo_entry(		/* if it's a todo entry, move it */
	struct entry	*entry,		/* entry to test */
	time_t		today);		/* today (time is ignored) */

/*---------------------------------------- dayedit.c ------------*/

extern void got_entry_press(		/* pressed on list menu button array */
	struct listmenu	*lw,		/* which menu */
	int		x,		/* which column/row */
	int		y,
	int		on,		/* enable radio but: on/off */
	int		shift);		/* arrow but: 1=shift key */
extern void confirm_new_entry(void);	/* put edited row back into mainlist */
extern void undo_new_entry(void);	/* Kill the edit buffer, undo */
extern void got_entry_text(		/* user entered text into text button*/
	struct listmenu	*lw,		/* which menu */
	int		x,		/* which column/row */
	int		y,
	char		*text);		/* text input by user */
extern void sensitize_edit_buttons(void); /* change bottom row */

/*---------------------------------------- dbase.c ------------*/

extern int  name_to_user(		/* user name to index into user[] */
	char		*name);		/* name to look up */
extern int  add_entry(			/* add an entry to a list */
	struct plist	**list,		/* list to add to */
	struct entry	*entry);	/* entry to add */
extern int  resort_entry(		/* move an entry to its proper place */
	struct plist	*list,		/* list to re-sort */
	int		n);		/* index to changed entry */
extern int  find_next_trigger( 		/* find next alarm/warn trigger time */
	struct plist	*list,		/* list of entries */
	time_t		time,		/* time to locate */
	struct entry	**entryp,	/* ret: entry that was found */
	time_t		*wait_time);	/* ret: how long until triggr*/
extern void create_list(		/* start a new list */
	struct plist	**list);	/* pointer to list pointer */
extern void destroy_list(		/* delete a list. */
	struct plist	**list);	/* pointer to list pointer */
extern void delete_entry(		/* delete an entry from the list. */
	struct plist	*list,		/* list to delete from */
	int		n);		/* index to entry to delete */
extern void clone_entry(		/* copy one entry to another */
	struct entry	*new,		/* entry to fill in */
	struct entry	*old);		/* entry to clone, or 0 */
extern void rebuild_repeat_chain(	/* chain all repeating entries */
	struct plist	*list);		/* list to delete from */
extern void destroy_entry(		/* free all strings of an entry */
	struct entry	*entry);	/* entry to destroy */
extern BOOL lookup_entry( 		/* find an entry in the list by date */
	struct lookup	*lookup,	/* return struct */
	struct plist	*list,		/* list of entries */
	time_t		time,		/* time to locate */
	BOOL		exact,		/* need entry, not insert pt */
	BOOL		norep,		/* ignore instances */
	char		which,		/* 'mywod': mon,yr,wk,yov,day*/
	int		usr);		/* user number or -1 */
extern BOOL lookup_next_entry(		/* find the next entry */
	struct lookup	*lookup);	/* previously found entry */
extern void delete_entry_by_id(
	struct plist	*list,		/* list to delete from */
	unsigned int	file,		/* file ID from server */
	int		id);		/* row ID from server */

/*---------------------------------------- file_r.c ------------*/

extern char *find_pub_path(		/* return the calendar path */
        char            *path);		/* resolve ~ and make absolute */
extern BOOL read_mainlist(void);	/* read all files into mainlist */
extern void parse_file_line(		/* parse one line from appt file */
	struct plist	**list,		/* list to add to */
	char		*path,		/* file/server to read from */
	char		*name,		/* user name, 0=not yet known*/
	char		*line);		/* line to be parsed */

/*---------------------------------------- file_w.c ------------*/

extern BOOL copyfile(			/* copy one file to another */
	char		*source,
	char		*target);
extern void write_mainlist(void);	/* write all modified files */
extern int  write_one_entry(		/* write one entry to file or server */
	int		(*callback)(char*),	/* function that writes line */
	struct entry	*ep);		/* list entry to write */

/*---------------------------------------- vcalendar_r.c ------------*/

extern void read_vcal_file(
	struct plist	**list,		/* list to add to */
	FILE		*fp,		/* file to read list from */
	int		u);		/* user number */

/*---------------------------------------- vcalendar_w.c ------------*/

extern void write_vcal_mainlist(
	char		*path);		/* write vCalendar data to this file */

/*---------------------------------------- help.c ------------*/

extern void help_callback(		/* print help for a topic */
	Widget		parent,
	char		*topic);

/*---------------------------------------- holiday.c ------------*/

extern char *parse_holidays(		/* eval holiday defs for <year> */
	int		year,		/* year to parse for, 0=1970 */
	BOOL		force);		/* file has changed, re-read */
extern void reset_all_hdays(void);	/* delete all known holidays */
extern void seteaster(			/* set a holiday relative to Easter */
	int		off,		/* offset in days */
	int		length,		/* length in days */
	int		pascha);	/* 0=Easter, 1=Christian Orthodox */
extern void setdate(			/* set holiday by date */
	int		month,		/* 1..12, ANY, or LAST */
	int		day,		/* 1..31, ANY, or LAST */
	int		year,		/* 1..2069, or ANY */
	int		off,		/* offset in days */
	int		length);	/* length in days */
extern void setwday(			/* set holiday by weekday */
	int		num,		/* which, 1st..5th, ANY, or LAST */
	int		wday,		/* 1=monday..7=sunday */
	int		month,		/* month, 1..12, ANY, or LAST */
	int		off,		/* offset in days */
	int		length);	/* length in days */
extern void setdoff(			/* set holiday by weekday date offset*/
	int		rel,		/* -1=before, -2=after */
	int		wday,		/* 1=monday..7=sunday */
	int		month,		/* month, 1..12, ANY, or LAST */
	int		day,		/* 1..31, ANY, or LAST */
	int		year,		/* 1..2069, or ANY */
	int		off,		/* offset in days */
	int		length);	/* length in days */
extern int day_from_name(
	char		*str);
extern int day_from_easter(void);
extern int day_from_monthday(
	int		m,
	int		d);
extern void monthday_from_day(
	int		day,
	int		*m,
	int		*d,
	int		*y);
extern int day_from_wday(
	int		day,
	int		wday,
	int		num);
extern void dump_holiday(
	int		year);

/*---------------------------------------- util.c ------------*/

extern char *resolve_tilde(		/* return path with ~ replaced */
	char		*path);		/* path with ~ */
extern BOOL find_file(			/* find file <name> and store path */
	char		*buf,		/* buffer for returned path */
	char		*name,		/* file name to locate */
	BOOL		exec);		/* must be executable? */
extern BOOL startup_lock(		/* make sure program runs only once */
	BOOL		pland,		/* is this plan or pland? */
	BOOL		force);		/* kill competitor */
extern BOOL refresh_lock(
	char		*pathtmp);	/* PLANDLOCK or PLANLOCK */
extern void lockfile(
	FILE		*fp,		/* file to lock */
	BOOL		lock);		/* TRUE=lock, FALSE=unlock */
extern int mystrcasecmp(		/* local version of strcasecmp */
	char		*a,
	char		*b);
void mybzero(
	void		*p,
	int		n);
void print_info_line(void);
char to_ascii(
	char		*str,		/* string to convert to ascii */
	int		def);		/* default if string is empty */

/*---------------------------------------- xutil.c ------------*/

extern void init_pixmaps(void);		/* intialize pixmaps for mode buttons*/
extern void print_icon_name(void);	/* change the name below the icon */
extern void set_icon(			/* set icon for the application */
	Widget		shell,
	int		sub);		/* 0=main, 1=submenu */
extern int strlen_in_pixels(		/* return length of string in pixels */
	char		*string,	/* string to truncate */
	int		sfont);		/* FONT_* */
extern void truncate_string(		/* truncate string to fit into n pixs*/
	char		*string,	/* string to truncate */
	int		len,		/* max len in pixels */
	int		sfont);		/* font of string */
extern void drag_init(
	int		view,		/* 'm', 'd', 'w', or 'o' */
	Widget		canvas,		/* drawing area to initialize */
	Widget		info,		/* label widget for info messages */
	void		(*rcb)(Region),	/* redraw callback */
	MOUSE		(*lcb)(struct entry **,
			       BOOL, time_t,
			       time_t, int,
			       int, int,
			       int, int, int));	/* locate position in canvas */
extern void drag_destroy(
	int		view);		/* 'm', 'd', 'w', or 'o' */

/*---------------------------------------- popup.c ------------*/

extern void create_about_popup(void);	/* create About info popup */
extern void create_nodaemon_popup(void);/* create no-daemon warning popup */
extern void create_multiple_popup(	/* create multiple-plan's warning */
	BOOL		nolock);	/* tried to kill competitor */
extern void multiple_writer_warning_popup( /* create write warning popup */
	Widget		widget);	/* install near this widget */
extern void new_language_popup(		/* selected new language */
	Widget		widget);	/* install near this widget */
extern void create_error_popup(
	Widget		widget,
	int		error,
	char		*fmt, ...);

/*---------------------------------------- print.c ------------*/

extern void create_print_popup(void);	/* create a print popup */

/*---------------------------------------- sublist.c ------------*/

extern void create_sublist(		/* create a sublist for list menu */
	struct plist	*list,		/* main list with all entries*/
	struct listmenu	*w);		/* menu we are doing this for*/
extern void destroy_sublist(		/* destroy the sublist of a list menu*/
	struct listmenu	*w);		/* menu we are doing this for*/

/*---------------------------------------- time.c ------------*/

extern void set_tzone(void);		/* figure out current timezone */
extern void guess_tzone(void);		/* guess timezone from $TZ */
extern time_t tm_to_time(		/* convert tm struct to seconds */
	struct tm	*tm);		/* time/date to convert */
extern time_t get_time(void);		/* current time in seconds */
extern time_t date_to_time(		/* day/month/year as # of secs */
	int		day,
	int		month,
	int		year,
	int		*wkday,
	int		*julian,
	int		*weeknum);
extern struct tm *time_to_tm(		/* seconds to tm struct */
	time_t		t);		/* time to convert */

/*---------------------------------------- weekcalc.c ------------*/

extern void build_week(			/* week node tree for curr_week */
	BOOL		omit_priv,	/* ignore private (padlocked) entries*/
	BOOL		omit_all);	/* ignore all ent (not very useful) */
extern void destroy_week(void);		/* release the week node trees. */

/*---------------------------------------- yovcalc.c ------------*/

extern void build_yov(			/* week node tree for curr_week */
	BOOL		omit_priv);	/* ignore private (padlocked) entries*/
extern void destroy_yov(void);		/* release the week node trees. */

/*---------------------------------------- daycalc.c ------------*/

extern void build_day(			/* week node tree for curr_day */
	BOOL		omit_priv,	/* ignore private (padlocked) entries*/
	BOOL		omit_all);	/* ignore all ent (not very useful) */
extern void destroy_day(void);		/* release the day node trees. */

/*---------------------------------------- adjmenu.c ------------*/

extern void create_adjust_popup(void);	/* create an adjust popup */

/*---------------------------------------- calmenu.c ------------*/

extern void create_cal_widgets(		/* create the main calendar window */
	Widget		toplevel);
extern void create_view(		/* create a new month/week/... view */
	int		view);		/* m, w, y, o, d */
extern Widget create_view_form(		/* make a window or form for a view */
	Widget		*shell,		/* if shell exists, widget, else &0 */
	Widget		parent,		/* parent widget (mainview) if share */
	int		view,		/* m, w, y, o, d */
	int		xs,		/* requested form width */
	int		ys,		/* requested form height */
	char		*name);		/* window name */
extern void redraw_all_views(void);	/* refraw all existing views */
extern void get_widget_size(		/* get the size of a widget */
	Widget		w,
	Dimension	*xs,
	Dimension	*ys);
extern void set_widget_size(		/* set the size of a widget */
	Widget		w,
	Dimension	xs,
	Dimension	ys);

/*---------------------------------------- cnfmenu.c ------------*/

extern void create_config_popup(void);	/* create a config popup */

/*---------------------------------------- cycmenu.c ------------*/

extern void destroy_recycle_popup(void);/* destroy a popup */
extern void create_recycle_popup(void);	/* create a recycle popup */

/*---------------------------------------- daymenu.c ------------*/

extern void destroy_all_listmenus(void);/* destroy all list menus */
extern void update_all_listmenus(BOOL);	/* redraw (opt.reeval) all list menus*/
extern void create_entry_widgets(	/* creates all widgets in a list menu*/
	struct listmenu	*w,
	int		nentries);
extern void edit_list_button(		/* turn text button to label or back */
	int		doedit,		/* 1=edit, 0=unedit, 2=del */
	struct listmenu	*w,		/* create which menu */
	int		x,		/* column, SC_* */
	int		y);		/* row, y=0: title */
extern void create_list_popup(		/* create a list popup */
	struct plist	*list,
	time_t		time,		/* for what day? */
	time_t		period,		/* for how many days? */
	char		*key,		/* keyword lookup? */
	struct entry	*entry,		/* just this one entry? */
	int		user_plus_1,	/* which user file */
	BOOL		private);	/* only private */
extern BOOL prepare_for_modify(		/* check if user file is writable */
	char		*name);		/* user name or 0 */

/*---------------------------------------- excmenu.c ------------*/

extern void destroy_except_popup(void);	/* destroy a popup */
extern void create_except_popup(void);	/* create an exception */

/*---------------------------------------- gotomenu.c ------------*/

extern void create_goto_popup(void);	/* create a goto popup */

/*---------------------------------------- holmenu.c ------------*/

extern void create_holiday_popup(void);	/* create a holiday popup */

/*---------------------------------------- keymenu.c ------------*/

extern void create_keyword_popup(void);	/* create a keyword popup */

/*---------------------------------------- monmenu.c ------------*/

extern void destroy_month_view(void);	/* destroy month view */
extern void create_month_view(		/* create month view */
	Widget		parent);

/*---------------------------------------- msgmenu.c ------------*/

extern void destroy_text_popup(void);	/* destroy a popup */
extern void create_text_popup(		/* create a message or script popup */
	char		*title,		/* menu title string */
	char		**initial,	/* initial default text */
	int		x,		/* button pressed, SC_* */
	BOOL		readonly);	/* just show, no editing */

/*---------------------------------------- notmenu.c ------------*/

extern void create_widgets(
	Widget		toplevel,
	char		*title,		/* title string */
	char		*subtitle,	/* subtitle string */
	char		*msg);		/* message text */

/*---------------------------------------- rangemenu.c ------------*/

extern void create_calconfig_popup(void);/* create a week view range popup */

/*---------------------------------------- usermenu.c ------------*/

extern BOOL all_files_served(void);	/* all files read from IP servers? */
extern void create_user_popup(void);	/* create user popup */
extern void create_user_rows(void);	/* make enough rows in user menu list*/
extern void draw_user_row(int);		/* redraw a row in the user menu list*/

/*---------------------------------------- servmenu.c ------------*/

extern void destroy_serv_popup(void);
extern void create_serv_popup(void);

/*---------------------------------------- weekmenu.c ------------*/

extern void destroy_week_menu(void);
extern void create_week_menu(		/* create the week menu for curr_week*/
	Widget		parent);

/*---------------------------------------- yearmenu.c ------------*/

extern void destroy_year_menu(void);	/* destroy the year window */
extern void create_year_menu(		/* create year calendar for curr_year*/
	Widget		parent);

/*---------------------------------------- yearmenu.c ------------*/

extern void destroy_yov_menu(void);
extern void create_yov_menu(
	Widget		parent);

/*---------------------------------------- daymenu.c ------------*/

extern void destroy_day_menu(void);
extern void create_day_menu(		/* create the day menu for curr_day */
	Widget		parent);

/*---------------------------------------- editdraw.c ------------*/

extern void draw_list(			/* draw or redraw a list of entries */
	struct listmenu	*w);		/* draw into which menu */
extern char *mkbuttonstring(		/* string to draw into a text button */
	struct listmenu	*lw,		/* menu the button is in */
	int		x,		/* column and row (y >= 1) */
	int		y);
extern char *mknotestring(		/* return string to draw into note */
	struct entry	*ep);		/* entry pointer */
extern BOOL show_except_pixmap(		/* should appt show PIC_EXCEPT? */
	struct entry	*ep);

/*---------------------------------------- mondraw.c ------------*/

extern MOUSE locate_in_month_calendar(	/* press in calendar day box */
	struct entry	**epp,		/* returned entry or 0 */
	BOOL		*warn,		/* is *epp an n-days-ahead w?*/
	time_t		*time,		/* if *epp=0, clicked where */
	time_t		*trigger,	/* if *epp=0, entry trigger */
	int		*xp,		/* if M_INSIDE, rubber box position */
	int		*yp,
	int		*xsp,		/* and size */
	int		*ysp,
	int		xc,		/* pixel pos clicked */
	int		yc);
extern void draw_month_year(void);	/* draw month and year into calendar */
extern void draw_calendar(		/* draw day grid into main calendar */
	Region		region);
extern void draw_day(			/* draw one day box into the day grid*/
	int		day);		/* 1..31 : current month */
					/* < 1 : previous month */
					/* > monthlen : next month */

/*---------------------------------------- weekdraw.c ------------*/

extern MOUSE locate_in_week_calendar(	/* press on the week display detected*/
	struct entry	**epp,		/* returned entry or 0 */
	BOOL		*warn,		/* is *epp an n-days-ahead w?*/
	time_t		*time,		/* if *epp=0, clicked where */
	time_t		*trigger,	/* if *epp=0, entry trigger */
	int		*xp,		/* if M_INSIDE, rubber box */
	int		*yp,
	int		*xsp,		/* position and size */
	int		*ysp,
	int		xc,		/* pixel pos clicked */
	int		yc);
extern void draw_week_day(		/* day d/m/y has changed, redraw week*/
	int		day,
	int		month,
	int		year);
extern void draw_week_calendar(		/* resize and redraw week menu */
	Region		region);
extern void print_week_calendar(	/* print PostScript week to fp */
	FILE		*fp,
	BOOL		landscape);

/*---------------------------------------- daydraw.c ------------*/

extern MOUSE locate_in_day_calendar(	/* press on the day display detected */
	struct entry	**epp,		/* returned entry or 0 */
	BOOL		*warn,		/* is *epp an n-days-ahead w?*/
	time_t		*time,		/* if *epp=0, clicked where */
	time_t		*trigger,	/* if *epp=0, entry trigger */
	int		*xp,		/* if M_INSIDE, rubber box */
	int		*yp,
	int		*xsp,		/* position and size */
	int		*ysp,
	int		xc,		/* pixel pos clicked */
	int		yc);
extern void draw_day_calendar(		/* resize and redraw day menu */
	Region		region);
extern void print_day_calendar(		/* print PostScript day to fp */
	FILE		*fp,
	BOOL		landscape);

/*---------------------------------------- yovdraw.c ------------*/

extern MOUSE locate_in_yov_calendar(	/* press on the yov display detected */
	struct entry	**epp,		/* returned entry or 0 */
	BOOL		*warn,		/* is *epp an n-days-ahead w?*/
	time_t		*time,		/* if *epp=0, clicked where */
	time_t		*trigger,	/* if *epp=0, entry trigger */
	int		*xp,		/* if M_INSIDE, rubber box */
	int		*yp,
	int		*xsp,		/* position and size */
	int		*ysp,
	int		xc,		/* pixel pos clicked */
	int		yc);
extern void draw_yov_calendar(		/* resize and redraw yov menu */
	Region		region);
extern void print_yov_calendar(		/* print PostScript yov to fp */
	FILE		*fp);

/*---------------------------------------- yeardraw.c ------------*/

extern void draw_year_calendar(		/* draw day grid into year cal */
	Region		region);
extern void draw_year_day(		/* draw one day box into year cal */
	int		day,		/* 1..31 */
	int		month,		/* 0..11 */
	int		year);		/* year since 1900 */
extern int clicked_year_calendar(	/* got a click in the calendar area */
	int		x,		/* pixel pos clicked */
	int		y);

/*---------------------------------------- psdraw.c ------------*/

extern BOOL g_init_window(		/* init week/yov window drawing */
	struct week	*week,		/* either &week or &yov */
	Region		newregion);	/* draw only here */
extern void g_exit_window(void);	/* finish week/yov window drawing */
extern void g_init_ps(			/* init week/yov PostScript drawing */
	struct week	*week,		/* either &week or &yov */
	FILE		*newfp,
	BOOL		landscape);
extern void g_exit_ps(void);		/* finish week/yov PostScript drawing*/
extern void g_setcolor(			/* select COL_* color */
	int		color);
extern void g_setfont(			/* select FONT_* font */
	int		fonti);
extern void g_fillrect(			/* draw filled rectangle */
	int		x,
	int		y,
	int		xs,
	int		ys);
extern void g_drawtext(			/* draw latin text string */
	int		x,
	int		y,
	char		*text,
	int		len);
extern void g_drawtext16(		/* draw Japanese text string */
	int		x,
	int		y,
	char		*text,
	int		len);
extern void g_drawpoly(			/* draw filled polygon */
	XPoint		*point,
	int		num,
	BOOL		checker);
extern void g_drawlines(		/* draw connected lines */
	XPoint		*point,
	int		num);

/*---------------------------------------- usersel.c ------------*/

extern void destroy_user_sel_popup(void);
extern void create_user_sel_popup(
	Widget		widget,		/* install popup near this button */
	int		curr,		/* current user ID (default) */
	BOOL		(*callb)(int));	/* function to call with new user ID */

/*---------------------------------------- network.c ------------*/

extern BOOL can_edit_appts(void);	/* ok to edit appointments? */
extern int connect_netplan_server(
	char		*uhost,		/* requested server host name */
	char		*uname,		/* user file name req'd from server */
	char		*msg);		/* for appending error message */
extern char *attach_to_network(void);	/* (re)connect to net, reread appts */
extern void detach_from_network(void);	/* disconnect from all servers */
extern int  name_to_file(		/* convert file/user name to file ID */
	char		*name);		/* name to locate */
extern BOOL server_entry_op(		/* link, unlink, delete, send entry */
	struct entry	*entry,		/* entry to lock/unlock/delete */
	char		op);		/* l=lock, u=unlock, d=del, w=write */
extern void read_from_server(		/* read one or all appointments */
	struct plist	**list,		/* list to add to */
	int		uid,		/* user[] index or -1 */
	int		rid);		/* entry ID, or 0 for all */
extern BOOL puts_server(		/* send message to a server */
	int		fd,		/* socket descriptor to send to */
	char		*msg);		/* message string to send, 0-term */
extern BOOL gets_server(		/* receive message from server */
	int		fd,		/* socket descriptor to send to */
	char		*msg,		/* buffer for incoming message */
	int		num,		/* size of msg buffer (paranoia...) */
	BOOL		wait,		/* need reply, not just '?' and 'R' */
	BOOL		cont);		/* expecting a continuation line? */
extern int open_and_count_rows_on_server(
	int		uid);		/* user[] index */
extern BOOL get_netplan_filelist(
	char		*uhost,		/* server host to request from */
	char		***list,	/* returned list */
	int		*num);		/* returned # of entries in list */
extern void obtain_netplan_users(
	char		*uhost);	/* server host to request from */

/*---------------------------------------- language.c ------------*/

extern char *_(char *);			/* translate a string to current lang*/
extern char *get_language_name(		/* return name of n-th language */
	int		n);		/* number of language */


/*
 * grok prototypes
 */

/*---------------------------------------- g_dbase.c ------------*/

extern struct dbase *dbase_create(void);
extern void dbase_delete(
	struct dbase	*dbase);	/* dbase to delete */
extern BOOL dbase_addrow(
	int		*rowp,		/* ptr to returned row number */
	struct dbase	*dbase);	/* database to add row to */
extern void dbase_delrow(
	int		nrow,		/* row to delete */
	struct dbase	*dbase);	/* database to delete row in */
extern char *dbase_get(
	struct dbase	*dbase,		/* database to get string from */
	int		nrow,		/* row to get */
	int		ncolumn);	/* column to get */
extern BOOL dbase_put(
	struct dbase	*dbase,		/* database to put into */
	int		nrow,		/* row to put into */
	int		ncolumn,	/* column to put into */
	char		*data);		/* string to store */

/*---------------------------------------- g_dbfile.c ------------*/

extern BOOL write_dbase(
	struct dbase	*dbase,		/* form and items to write */
	struct form	*form,		/* contains column delimiter */
	BOOL		force);		/* write even if not modified*/
extern BOOL read_dbase(
	struct dbase	*dbase,		/* form and items to write */
	struct form	*form,		/* contains column delimiter */
	char		*path);		/* file to read list from */

/*---------------------------------------- g_formfile.c ------------*/

extern BOOL write_form(
	struct form	*form);		/* form and items to write */
extern BOOL read_form(
	struct form	*form,		/* form and items to write */
	char		*path);		/* file to read list from */

/*---------------------------------------- g_formop.c ------------*/

extern struct form *form_create(void);
extern struct form *form_clone(
	struct form	*parent);	/* old form */
extern void form_delete(
	struct form	*form);		/* form to delete */
extern BOOL verify_form(
	struct form	*form,		/* form to verify */
	int		*bug,		/* retuirned buggy item # */
	Widget		shell);		/* error popup parent */
extern void form_edit_script(
	struct form	*form,		/* form to edit */
	Widget		shell,		/* error popup parent */
	char		*fname);	/* file name of script (dbase name) */
extern void form_sort(
	struct form	*form);		/* form to sort */
extern void item_deselect(
	struct form	*form);		/* describes form and all items in it*/
extern BOOL item_create(
	struct form	*form,		/* describes form and all items in it*/
	int		nitem);		/* the current item, insert point */
extern void item_delete(
	struct form	*form,		/* describes form and all items in it*/
	int		nitem);		/* the current item, insert point */
extern struct item *item_clone(
	struct item	*parent);	/* item to clone */
