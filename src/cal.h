/*
 * everything the main program needs but the daemon doesn't. This mainly
 * means X stuff to keep the size of the daemon down.
 */

#include "config.h"

struct mainmenu {		/* all important main window widgets */
	Widget	month;			/* main window: month name label */
	Widget	year;			/* main window: year number label */
	Widget	time;			/* main window: time display label */
	Widget	cal;			/* main window: calendar drawing area*/
	Widget	yearcal;		/* year window: calendar drawing area*/
	Widget	weekcal;		/* week window: calendar drawing area*/
};


				/* when writing database files, write what? */
#define WR_CONFIG	1		/* configuration & preferences */
#define WR_PUBLIC	2		/* entries readable by everybody */
#define WR_PRIVATE	4		/* entries readable by owner only */

#define CHARWIDTH(f,c) \
	(font[(f)]->per_char[(c) < (int)font[(f)]->min_char_or_byte2 || \
			     (c) > (int)font[(f)]->max_char_or_byte2 ? '0' : \
			     (c) - (int)font[(f)]->min_char_or_byte2].width)



/*
 * everything about a list popup. time, period, and key describe what is
 * listed in the menu:
 *
 * time=0, period=0, key=0:		the entire list
 * time=0, period=0, key=string:	all entries that contain <key>
 * time>0, period=0, key=X:		one day only
 * time>0, period>1, key=X:		<period/86400> days
 *
 * The sublist field contains an array of pointers being displayed in the
 * menu. It is created by create_sublist(). listmenu.sublist->nentries is
 * the number of entries being displayed in the menu, which is always
 * less than listmenu.nentries, which is the number of button rows.
 */

				/* button columns in day schedule list */
#define SC_ENABLE	0		/* radio button to enable */
#define SC_USER		1		/* user name or own */
#define SC_DATE		2		/* date */
#define SC_ENDDATE	3		/* end date (last day of repeat) */
#define SC_TIME		4		/* time */
#define SC_LENGTH	5		/* length */
#define SC_ADVANCE	6		/* advance-warning times */
#define SC_ADVDAYS	7		/* advance warning n days ahead */
#define SC_NOTE		8		/* short note for calendar */
#define SC_FLAGS	9		/* first flag (same order as PIC_*) */
#define SC_RECYCLE	9		/*   repeat flag */
#define SC_MESSAGE	10		/*   show-message flag */
#define SC_SCRIPT	11		/*   execute-script flag */
#define SC_EXCEPT	12		/*   exception flag */
#define SC_PRIVATE	13		/*   keep-private flag */
#define SC_TODO		14		/*   keep-private flag */
#define SC_I		15		/* first inc arrow */
#define SC_I_DATE	15		/*   ++date */
#define SC_I_ENDDATE	16		/*   ++enddate */
#define SC_I_TIME	17		/*   ++time */
#define SC_I_LENGTH	18		/*   ++length */
#define SC_I_ADVANCE	19		/*   ++warn */
#define SC_I_ADVDAYS	20		/*   ++warndays */
#define SC_D		21		/* first dec arrow */
#define SC_D_DATE	21		/*   --date */
#define SC_D_ENDDATE	22		/*   --enddate */
#define SC_D_TIME	23		/*   --time */
#define SC_D_LENGTH	24		/*   --length */
#define SC_D_ADVANCE	25		/*   --warn */
#define SC_D_ADVDAYS	26		/*   --warndays */
#define SC_L		27		/* first label */
#define SC_L_DATE	27		/*   label for date: "beg" */
#define SC_L_ENDDATE	28		/*   label for end date: "end" */
#define SC_L_TIME	29		/*   label for time: "beg" */
#define SC_L_LENGTH	30		/*   label for length: "len" */
#define SC_L_ADVANCE	31		/*   label for warning: "time" */
#define SC_L_ADVDAYS	32		/*   label for warn days: "days" */
#define SC_L_FLAGS	33		/*   label for options: "opt" */
#define SC_N		34		/* # of columns */


struct listmenu {		/* all important list popup widgets */
	struct listmenu *next, *prev;	/* if there are multiple list popups */
	BOOL	popped;			/* TRUE if the menu is accessible */
	BOOL	valid;			/* TRUE if the widgets exist */
	BOOL	pinned;			/* TRUE if popup can't be re-used */
	BOOL	own_only;		/* TRUE if other users' appts omitted*/
	struct	sublist *sublist;	/* sublist with live entry ptrs */
	time_t	time;			/* if nonzero, first day in list */
	time_t	period;			/* if nonzero, how many days */
	char	*key;			/* if nonzero, this is a keyword list*/
	struct	entry *oneentry;	/* if nonzero, this one entry only */
	int	user_plus_1;		/* if nonzero, user file to list +1 */
	BOOL	private;		/* if nonzero, list only private */
	Widget	shell;			/* the popup menu itself */
	Widget	title;			/* title string */
	Widget	confirm, undo, dup;	/* buttons */
	Widget	del, done, pin, own;	/* more buttons */
	Widget	list;			/* RowColumn widget for entry list */
	Widget	(*entry)[SC_N];		/* each entry has SC_N columns */
	Widget	text;			/* if in text input mode, Text widget*/
	int	nentries;		/* how many rows, multiple of 5 */
	int	xedit, yedit;		/* row/col being edited if .text!=0 */
};


/*
 * when the user is entering a new entry or changing an existing one, all
 * the new data goes into this struct only. This struct "overlays" the
 * real entry until editing is undone or confirmed.
 */

struct edit {
	BOOL		editing;	/* new_entry is being edited */
	BOOL		changed;	/* new entry is valid, needs confirm */
	struct entry	entry;		/* buffer for entry being edited */
	int		y;		/* row# of new entry, > 0 */
	struct listmenu	*menu;		/* ptr to listmenu with new entry */
	struct entry	*srcentry;	/* if menu==0, orig entry (dragging) */
};


/*
 * holiday struct. There are several of these, for holidays, vacations, and
 * birthdays. To keep it simple, all three lists use the same struct. There
 * is one struct for each day; if its "string" pointer is nonzero, the
 * holiday or whatever exists and must be put in the day box. If <dup> is
 * true, the struct is a duplicate of some other struct; its <string> must
 * not be free()d (this happens for vacations, which are typically longer
 * than one day, even in the US).
 */

struct holiday {
	char		*string;	/* name of holiday, 0=not a holiday */
	int		stringcolor;	/* 0=default, 1..8=black..white */
	int		daycolor;	/* 0=default, 1..8=black..white */
	BOOL		dup;		/* this is a clone of 1st vacatn day */
};


/*
 * Node of the trees that control the arrangement of entries in the week,
 * year overview, and day views. There is one tree per day. Consecutive
 * nonoverlapping entries are chained with next/prev; lines are chained by
 * up/down in the first node. In week view mode, one strip (tree) describes
 * one day; in year overview mode, one strip describes one user. Day views
 * have only one tree.
 */

#define NDAYS		28		/* max # of days in week or day view */

struct weeknode {
	struct weeknode	*next;		/* next entry to the right */
	struct weeknode	*prev;		/* next entry to the left */
	struct weeknode	*up;		/* if !*prev, first in prev line */
	struct weeknode	*down;		/* if !*prev, first in next line */
	struct entry	*entry;		/* entry being described */
	time_t		trigger;	/* real trigger time for this bar */
	BOOL		days_warn;	/* this is a n-days-ahead warning */
	char		text[80];	/* note text */
	int		textlen;	/* length of text in pixels */
	BOOL		textinside;	/* TRUE if text fits in bar */
};

struct week {
	struct weeknode	**tree;		/* anchors for all days/users */
	int		*nlines;	/* # of lines, min 1, 0=not in year */
	int		canvas_xs;	/* drawing area width */
	int		canvas_ys;	/* drawing area height */
	Widget		canvas;		/* drawing area widget */
	Widget		scroll;		/* scrolling widget around canvas */
	Widget		info;		/* info text line */
};


/*
 * X stuff
 */

				/* fonts */
#define FONT_STD	0		/* standard font: menus, text */
#define FONT_HELP	1		/* pretty font for help popups */
#define FONT_DAY	2		/* month view: large day #s */
#define FONT_SMDAY	3		/* small month view: large day #s */
#define FONT_NOTE	4		/* month view: small notes */
#define FONT_YTITLE	5		/* year view: title string */
#define FONT_YMONTH	6		/* year view: month names */
#define FONT_YDAY	7		/* year view: weekday names */
#define FONT_YNUM	8		/* year view: day numbers */
#define FONT_WTITLE	9		/* week view: title string */
#define FONT_WDAY	10		/* week view: weekday column */
#define FONT_WHOUR	11		/* week view: hour row */
#define FONT_WNOTE	12		/* week view: small notes */
#define FONT_JNOTE	13		/* Japanese font for all the notes */
#define NFONTS		14

				/* colors */
#define COL_BACK	0		/* standard background */
#define COL_STD		1		/* standard foreground */
#define COL_CALBACK	2		/* calendar background */
#define COL_CALACT	3		/* no longer used */
#define COL_CALTODAY	4		/* calendar background for today */
#define COL_CALSHADE	5		/* calendar daybox background */
#define COL_CALFRAME	6		/* calendar frame around all days */
#define COL_GRID	7		/* calendar grid lines */
#define COL_WEEKDAY	8		/* calendar weekday number */
#define COL_WEEKEND	9		/* calendar holiday number */
#define COL_NOTE	10		/* calendar weekday note text */
#define COL_NOTEOFF	11		/* calendar weekday note if suspended*/
#define COL_TOGGLE	12		/* schedule enable toggle button */
#define COL_RED		13		/* schedule pin toggle button */
#define COL_TEXTBACK	14		/* standard bkground of text widgets */
#define COL_YBACK	15		/* year view background */
#define COL_YBOXBACK	16		/* year view bkgd of month boxes */
#define COL_YNUMBER	17		/* year view day numbers */
#define COL_YWEEKDAY	18		/* year view weekday names */
#define COL_YMONTH	19		/* year view month names */
#define COL_YTITLE	20		/* year view title */
#define COL_YGRID	21		/* year view month box lines */
#define COL_HBLACK	22		/* holiday day or text */
#define COL_HRED	23		/* holiday day or text */
#define COL_HGREEN	24		/* holiday day or text */
#define COL_HYELLOW	25		/* holiday day or text */
#define COL_HBLUE	26		/* holiday day or text */
#define COL_HMAGENTA	27		/* holiday day or text */
#define COL_HCYAN	28		/* holiday day or text */
#define COL_HWHITE	29		/* holiday day or text */
#define COL_WBACK	30		/* week view background */
#define COL_WBOXBACK	31		/* week view bar box background */
#define COL_WTITLE	32		/* week view title string */
#define COL_WGRID	33		/* week view fat and fine grid */
#define COL_WDAY	34		/* week view week day and hour texts */
#define COL_WNOTE	35		/* week view note text in bars */
#define COL_WFRAME	36		/* week view frame around bars */
#define COL_WWARN	37		/* week view shading of warning bars */
#define COL_WUSER_0	38		/* week view shading of user 0 bars */
#define COL_WUSER_1	39		/* week view shading of user 1 bars */
#define COL_WUSER_2	40		/* week view shading of user 2 bars */
#define COL_WUSER_3	41		/* week view shading of user 3 bars */
#define COL_WUSER_4	42		/* week view shading of user 4 bars */
#define COL_WUSER_5	43		/* week view shading of user 5 bars */
#define COL_WUSER_6	44		/* week view shading of user 6 bars */
#define COL_WUSER_7	45		/* week view shading of user 7 bars */
#define COL_GRAYICON	46		/* color of flag icons that are off */
#define COL_WIREFRAME	47		/* exclusive-or color */
#define NCOLS		48

				/* pixmaps. Don't re-order the first six */
#define PIC_RECYCLE	0		/* schedule flag: repetitive */
#define PIC_MESSAGE	1		/* schedule flag: has a message text */
#define PIC_SCRIPT	2		/* schedule flag: has a shell script */
#define PIC_EXCEPT	3		/* schedule flag: has exceptions */
#define PIC_PRIVATE	4		/* schedule flag: private appt */
#define PIC_TODO	5		/* schedule flag: todo item */
#define PIC_BLANK	6		/* blank space (for inactive labels) */
#define PIC_CHECKER	7		/* tiny checkerboard for graying out */
#define NPICS		8


/*
 * drag-and-drop: possible areas where the mouse can be. Also used to set
 * the cursor glyph.
 */

typedef enum {
	M_ILLEGAL = 0,		/* outside the clickable area */
	M_OUTSIDE,		/* not near any item */
	M_ARROW,		/* some clickable area */
	M_INSIDE,		/* inside an item, but not near an edge */
	M_LEFT,			/* near left edge */
	M_RIGHT,		/* near right edge */
	M_DAY_CAL,		/* request switch to day calendar view */
	M_WEEK_CAL,		/* request switch to week calendar view */
	M_MONTH_CAL,		/* request switch to month calendar view */
	M_NCURSORS		/* number of valid mouse cursors */
} MOUSE;


/*
 * fix a Linux problem: most Done buttons crash plan, in XtDestroyWidget.
 * Until somebody figures out why, this fixes it by introducing a huge
 * memory leak.
 */

#ifdef DESTROYBUG
#define XTDESTROYWIDGET(w)
#else
#define XTDESTROYWIDGET(w) XtDestroyWidget(w)
#endif


/*
 * Definition of some constants regarding Japanese, and
 * Structure for partial strings of a mixed string, ascii and kanji
 * characters.
 */

#ifdef JAPAN

#ifndef LOCALE_SJIS
#define LOCALE_SJIS	"ja_JP.SJIS"
#endif
#ifndef LOCALE_EUC
#define LOCALE_EUC	"ja_JP.EUC"
#endif

#define MAXPARTIALSTRING	16
#define MAXPARTIALCHAR		1024

typedef struct {
	unsigned char	*strptr;	/* A pointer to a partial string. */
	BOOL		asciistr;	/* True: a string is ascii. */
	int		pixlen;		/* A length of a string in pixels. */
	int		length;		/* A length of a string in bytes. */
} strpack;
#endif

#include "proto.h"
