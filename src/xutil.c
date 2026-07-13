/*
 * various useful X11-related functions used everywhere
 *
 *	init_pixmaps()		create pixmaps on startup
 *	print_icon_name()	change the name below the icon
 *	set_icon()		set icon for the application
 *
 *	strlen_in_pixels(string, sfont)
 *	truncate_string(string, len, sfont)
 *	print_button(w, fmt, ...)	Prints a string into a string Label
 *					or PushButton.
 *	print_text_button(w, fmt, ...)	Prints a string into a Text button.
 *	print_pixmap(w, n, b)		Prints a pixmap into a Pixmap label.
 *	set_color(col)			Sets the foreground color to one of
 *					the predefined colors (COL_*).
 *	set_cursor(w, n)		set new cursor shape for drawing area w
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>
#include <Xm/Xm.h>
#include <Xm/Text.h>
#include <X11/StringDefs.h>
#include <Xm/DrawingA.h>
#include <Xm/Protocols.h>
#include <X11/cursorfont.h>
#include "cal.h"

#include "bm_recycle.h"
#include "bm_message.h"
#include "bm_script.h"
#include "bm_except.h"
#include "bm_private.h"
#include "bm_todo.h"
#include "bm_icon.h"
#include "bm_iconsub.h"
#include "bm_circle.h"
#include "bm_chkcircle.h"
#include "bm_checker.h"

static void canvas_callback(Widget, XButtonEvent *, String *, int);

extern struct config	config;		/* global configuration data */
extern Display		*display;	/* everybody uses the same server */
extern XtAppContext	app;		/* application handle */
extern Widget		toplevel;	/* top-level shell for icon name */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern struct user	*user;		/* user list (from file_r.c) */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern time_t		curr_week;	/* week being displayed, time in sec */
extern time_t		curr_day;	/* day being displayed, time in sec */
extern GC		jgc, gc;	/* graphic context, Japanese and std */
extern GC		xor_gc;		/* XOR gc for rubberbanding */
extern GC		chk_gc;		/* checkered gc for light background */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern struct edit	edit;		/* info about entry being edited */
Pixmap			pixmap[NPICS];	/* common symbols */
Pixmap		    graypixmap[NPICS];	/* common symbols, grayed-out */
Pixmap			col_pixmap[8];	/* colored circles for file list */
Pixmap			chk_pixmap[8];	/* same but checkered (light color) */

static char blank_bits[] =
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

static char *pics[NPICS]     = {recycle_bits, message_bits, script_bits,
				except_bits, private_bits, todo_bits,
				blank_bits, checker_bits};
static int pix_width[NPICS]  = {recycle_width, message_width, script_width,
				except_width, private_width, todo_width,
				recycle_width, checker_width};
static int pix_height[NPICS] = {recycle_height, message_height, script_height,
				except_height, private_height, todo_height,
				recycle_height, checker_height};


/*
 * intialize pixmaps for mode buttons. The standard mode pixmaps are created
 * both as black and gray versions, for enabled and disabled flags. The
 * checker pixmap is used to stipple backgrounds in calendar drawing areas;
 * only create one. The circle and chkcircle pixmaps are used for the group
 * buttons in the file list; create each with all eight colors.
 */

void init_pixmaps(void)
{
	int			p;
	Colormap		cmap;
	XColor			rgb;
	char			*c = 0;
	BOOL			failed;

	cmap = DefaultColormap(display, DefaultScreen(display));
	do_get_rsrc((XtPointer)&c, "colBack", "ColBack", XtRString, 0);
	failed = !c || !XParseColor(display, cmap, c, &rgb)
		    || !XAllocColor(display, cmap, &rgb);
	for (p=0; p < NPICS; p++)
	    if (p != PIC_CHECKER)
		if (!(pixmap[p] = XCreatePixmapFromBitmapData(display,
				DefaultRootWindow(display),
				pics[p], pix_width[p], pix_height[p],
				color[COL_STD],
				failed ? color[COL_BACK] : rgb.pixel,
				DefaultDepth(display,DefaultScreen(display))))
		 || !(graypixmap[p] = XCreatePixmapFromBitmapData(display,
				DefaultRootWindow(display),
				pics[p], pix_width[p], pix_height[p],
				color[COL_GRAYICON],
				failed ? color[COL_BACK] : rgb.pixel,
				DefaultDepth(display,DefaultScreen(display)))))
			fatal(_("no memory for pixmaps"));

	for (p=0; p < 8; p++)
		if (!(col_pixmap[p] = XCreatePixmapFromBitmapData(display,
				DefaultRootWindow(display),
				circle_bits, circle_width, circle_height,
				color[COL_WUSER_0 + p], color[COL_CALBACK],
				DefaultDepth(display,DefaultScreen(display))))
		 || !(chk_pixmap[p] = XCreatePixmapFromBitmapData(display,
				DefaultRootWindow(display), chkcircle_bits,
				chkcircle_width, chkcircle_height,
				color[COL_WUSER_0 + p], color[COL_CALBACK],
				DefaultDepth(display,DefaultScreen(display)))))
			fatal(_("no memory for pixmaps"));

	if (!(pixmap[PIC_CHECKER] = XCreatePixmapFromBitmapData(display,
				DefaultRootWindow(display),
				checker_bits, checker_width, checker_height,
				~0, 0, 1)))
			fatal(_("no memory for pixmaps"));

	XSetForeground(display, chk_gc, color[COL_CALBACK]);
	XSetStipple   (display, chk_gc, pixmap[PIC_CHECKER]);
	XSetFillStyle (display, chk_gc, FillStippled);

}


/*
 * set icon for the application or a shell.
 */

void set_icon(
	Widget			shell,
	int			sub)		/* 0=main, 1=submenu */
{
	Pixmap			icon;

	if (!config.noicon && !sub)
		if ((icon = XCreatePixmapFromBitmapData(display,
				DefaultRootWindow(display),
				sub ? iconsub_bits : icon_bits,
				icon_width, icon_height,
				WhitePixelOfScreen(XtScreen(shell)),
				BlackPixelOfScreen(XtScreen(shell)),
				DefaultDepth(display,
				DefaultScreen(display)))))

			XtVaSetValues(shell, XmNiconPixmap, icon, NULL);
	print_icon_name();
}


/*
 * change the name below the icon. Relies on <toplevel> from main(). Changing
 * the icon title to the current time can bbe switched off because it prevents
 * the screen saver from kicking in.
 */

void print_icon_name(void)
{
	time_t		tod;		/* current time-of-day */
	char		buf[64];	/* icon label string */
	struct tm	*tm;		/* time to m/d/y conv */

	if (config.showicondate) {
		tod = get_time();
		tm  = time_to_tm(tod);
		if (config.mmddyy)
			sprintf(buf, "%d/%d", tm->tm_mon+1, tm->tm_mday);
		else
			sprintf(buf, "%d.%d.", tm->tm_mday, tm->tm_mon+1);
		sprintf(buf+strlen(buf), " %s", mktimestring(tod, FALSE));
		XtVaSetValues(toplevel, XmNiconName, buf, NULL);

	} else if (config.showicontime) {
		tod = get_time();
		XtVaSetValues(toplevel, XmNiconName,
					mktimestring(tod, FALSE), NULL);
	}
}


/*
 * return the length of <string> in pixels.
 */

int strlen_in_pixels(
	char		*string,	/* string to truncate */
	int		sfont)		/* FONT_* */
{
	int		len;		/* length in pixels */

	for (len=0; *string; string++)
		len += CHARWIDTH(sfont, *string);
	return(len);
}


/*
 * truncate <string> such that it is not longer than <len> pixels when
 * drawn with font <sfont>, by storing \0 somewhere in the string.
 */

void truncate_string(
	char		*string,	/* string to truncate */
	int		len,		/* max len in pixels */
	int		sfont)		/* font of string */
{
	while (*string) {
		len -= font[sfont]->per_char[((unsigned char) *string) -
					font[sfont]->min_char_or_byte2].width;
		if (len < 0)
			*string = 0;
		else
			string++;
	}
}


/*
 * draw some text into a button. This is here because it's used by many
 * routines. There are two versions, stdarg and varargs, because Sun has
 * the gall to ship a pre-ansi compiler unless you pay extra...
 */

void print_button(
	Widget		w,
	char		*fmt, ...)
{
	va_list		parm;
	Arg		args;
	XmString	string;
	char		buf[1024];

	if (fmt && w) {
		va_start(parm, fmt);
		vsprintf(buf, fmt, parm);
		va_end(parm);
		string = XmStringCreateSimple(*buf ? buf : " ");
		XtSetArg(args, XmNlabelString, string);
		XtSetValues(w, &args, 1);
		XmStringFree(string);
	}
}

void print_text_button(
	Widget		w,
	char		*fmt, ...)
{
	va_list		parm;
	char		buf[1024];

	if (w) {
		va_start(parm, fmt);
		vsprintf(buf, fmt, parm);
		va_end(parm);
		XmTextSetString(w, *buf ? buf : " ");
		XmTextSetInsertionPosition(w, strlen(buf));
	}
}


/*
 * draw a pixmap into a button, or remove the pixmap. The button must have
 * been a pixmap button all along, this routine can't mix text and pixmaps.
 * If <n> is -1, draw a blank pixmap.
 */

void print_pixmap(
	Widget		w,		/* button to draw into */
	int		n,		/* pixmap #, one of PIC_* */
	BOOL		b)		/* true: black, false: grayed*/
{
	Arg		arg;

	if (w) {
		XtSetArg(arg, XmNlabelPixmap, b ? pixmap[n] : graypixmap[n]);
		XtSetValues(w, &arg, 1);
	}
}


/*
 * set a color into the regular (non-xor) graphical context (GC) for drawing.
 */

void set_color(
	int		col)
{
	XSetForeground(display, jgc, color[col]);
	XSetForeground(display, gc,  color[col]);
}


/*
 * set mouse cursor to one of the predefined shapes. This is used in the
 * calendar view drawing areas. The M_ macro values are used to select a
 * cursor; they are mapped to X11 XC_ glyph codes here.
 */

static int cursorglyph[] = {
	XC_X_cursor,		/* M_ILLEGAL */
	XC_pencil,		/* M_OUTSIDE */
	XC_arrow,		/* M_ARROW */
	XC_pencil,		/* M_INSIDE */
	XC_fleur,		/* M_LEFT */
	XC_right_side,		/* M_RIGHT */
	XC_hand2,		/* M_DAY_CAL */
	XC_hand2,		/* M_WEEK_CAL */
	XC_hand2		/* M_MONTH_CAL */
};

void set_cursor(
	Widget		w,		/* in which widget */
	int		n)		/* which cursor, one of M_* */
{
	static Cursor	cursor[M_NCURSORS];
	Window		win = XtWindow(w);

	assert(sizeof(cursorglyph) == sizeof(int) * M_NCURSORS);
	assert(n >= 0 && n < M_NCURSORS);
	if (!cursor[n])
		cursor[n] = XCreateFontCursor(display, cursorglyph[n]);
	if (win)
		XDefineCursor(display, win, cursor[n]);
}


/*
 * drag-and-drop: change a new drawing area used for a calendar view such
 * that it generates events when the mouse is dragged. Exposing is handled
 * here, only a callback to call to redraw a region is needed. The main
 * event callback is also handled here.
 */

#define NVIEWS	4
static char	drag_views[NVIEWS] = "mdwo";
static struct dragc_s {
	int	view;			/* [0]='m', [1]='d', [2]='w', [3]='o'*/
	Widget	canvas;			/* drawing area */
	Widget	info;			/* text widget for info msgs, or 0 */
	void	(*expose_cb)();		/* expose callback */
	MOUSE	(*locate_cb)();		/* locate callback */
}		drag_context[NVIEWS];

static void expose_callback();

static String translations =
		"<Btn1Down>:	canvas(down)	ManagerGadgetArm()\n\
		 <Btn1Up>:	canvas(up)	ManagerGadgetActivate()\n\
		 <Motion>:	canvas(motion)	ManagerGadgetButtonMotion()\n\
		 <Btn1Motion>:	canvas(motion)	ManagerGadgetButtonMotion()";

void drag_init(
	int		view,		/* 'm', 'd', 'w', or 'o' */
	Widget		canvas,		/* drawing area to initialize */
	Widget		info,		/* label widget for info messages */
	void		(*rcb)(),	/* redraw callback */
	MOUSE		(*lcb)())	/* locate position in canvas */
{
	static BOOL	did_init = FALSE;
	XtActionsRec	action;
	Arg		arg;
	char		*p;
	struct dragc_s	*dc;

	if (!did_init) {
		action.string = "canvas";
		action.proc   = (XtActionProc)canvas_callback;
		XtAppAddActions(app, &action, 1);
		did_init = TRUE;
	}
	p = strchr(drag_views, view);
	assert(p);
	dc = &drag_context[p - drag_views];
	dc->view      = view;
	dc->canvas    = canvas;
	dc->info      = info;
	dc->expose_cb = rcb;
	dc->locate_cb = lcb;

	XtAddCallback(canvas, XmNexposeCallback, (XtCallbackProc)
			expose_callback, (XtPointer)(p - drag_views));
	XtSetArg(arg, XmNtranslations, XtParseTranslationTable(translations));
	XtSetValues(canvas, &arg, 1);
	set_cursor(canvas, M_OUTSIDE);
}

void drag_destroy(
	int		view)		/* 'm', 'd', 'w', or 'o' */
{
	char		*p;
	struct dragc_s	*dc;

	p = strchr(drag_views, view);
	assert(p);
	dc = &drag_context[p - drag_views];
	mybzero(dc, sizeof(struct dragc_s));
}


/*ARGSUSED*/
static void expose_callback(
	UNUSED Widget			canvas,		/* drawing area */
	XtPointer			data,		/* view: 0..NVIEWS-1 */
	XmDrawingAreaCallbackStruct	*info)		/* what & where */
{
	XEvent dummy;
	Region region = XCreateRegion();
	XtAddExposureToRegion(info->event, region);
	while (XCheckWindowEvent(display, info->window, ExposureMask, &dummy))
		XtAddExposureToRegion(&dummy, region);
	assert((long)data >= 0 && (long)data < NVIEWS);
	(*drag_context[(long)data].expose_cb)(region);
}


/*
 * this routine is called directly from the drawing area's translation table,
 * with mouse up/down/motion events. It is installed by the previous function
 * (drag_init). It handles all calendar views.
 */

static void drag_entry		(int, MOUSE, time_t, time_t);
static void draw_rubberband	(Widget, BOOL, int, int, int, int);

/*ARGSUSED*/
static void canvas_callback(
	Widget		canvas,		/* canvas, == calendar drawing area */
	XButtonEvent	*event,		/* X event, contains position */
	String		*args,		/* what happened, up/down/motion */
	UNUSED int	nargs)		/* # of args, must be 1 */
{
	static struct entry *entry;	/* entry being dragged */
	static BOOL	warn;		/* just the n-days-ahead copy? */
	static MOUSE	mode;		/* what's being moved: M_* */
	static time_t	time0;		/* mouse time when first pressed */
	static time_t	trigger;	/* event trigger when first pressed */
	static int	down_x, down_y;	/* pos where pen was pressed down */
	static int	xp,yp,xsp,ysp;	/* initial entry rubber band box */
	static BOOL	moving;		/* this is not a selection, move box */
	MOUSE		m;		/* target area being moved to */
	time_t		time;		/* current mouse time */
	struct dragc_s	*dc;		/* drag context for callbacks */
	int		x=0, y=0;	/* new item start */
	int		xs=0, ys=0;	/* new item size */
	int		dx, dy;		/* movement since initial press */
	char		buf[1024];	/* for date string */
	int		i;

	for (i=0, dc=drag_context; i < NVIEWS; i++, dc++)
		if (canvas == dc->canvas)
			break;
	if (i >= NVIEWS)
		return;
							/* switch views */
	if (!strcmp(args[0], "down")) {
		mode = (*dc->locate_cb)(&entry, &warn, &time0, &trigger,
				&xp, &yp, &xsp, &ysp, event->x, event->y);
		switch(mode) {
		  case M_DAY_CAL:
			curr_day = time0;
			create_view('d');
			break;
		  case M_WEEK_CAL:
			curr_week = time0;
			create_view('w');
			break;
		  default:
			down_x = event->x;
			down_y = event->y;
			moving = FALSE;
			set_cursor(canvas, mode);
		}
		return;
	}
	if (!strcmp(args[0], "motion") && (event->state & Button1Mask)
				       && mode != M_OUTSIDE) {
		if (dc->view == 'o' && mode == M_RIGHT && entry->rep_every
						&& entry->rep_every != 86400)
			print_button(dc->info,
				_("Cannot resize entry with nondaily repeat"));
		else if (!moving) {
			x = abs(event->x - down_x);
			y = abs(event->y - down_y);
			moving |= x > 3 || y > 3;
			if (moving && mode == M_INSIDE)
				mode = M_LEFT;
			if (moving) {
				confirm_new_entry();
				if (!can_edit_appts())
					return;
				clone_entry(&edit.entry, entry);
				if (!server_entry_op(&edit.entry, 'l')) {
					create_error_popup(0, 0,
					  _("Somebody else is editing this "\
					    "entry. Please try again later."));
					return;
				}
				edit.editing  = TRUE;
				edit.changed  = FALSE;
				edit.y	      = y;
				edit.menu     = 0;
				edit.srcentry = entry;
			}
		}
	}
	if (moving) {
		x  = xp;
		y  = yp;
		xs = xsp;
		ys = ysp;
		dx = event->x - down_x;
		dy = event->y - down_y;
		if (mode == M_LEFT) {
			x += dx;
			if (dc->view != 'o')
				y += dy;
		}
		if (mode == M_RIGHT)
			switch(dc->view) {
			  case 'w':
			  case 'o': xs = xs > -dx ? xs + dx : 0; break;
			  case 'd': ys = ys > -dy ? ys + dy : 0; break;
			}
	}
	if (!strcmp(args[0], "motion")) {			/* dragging */
		if (moving) {
			struct entry save = edit.entry;
			(*dc->locate_cb)(0, 0, &time, 0,
					 0, 0, 0, 0, event->x, event->y);
			drag_entry(dc->view, mode, time0, time);
			if (mode != M_RIGHT)
				sprintf(buf, edit.entry.notime ? "%s":"%s %s",
					mkdatestring(edit.entry.time),
					mktimestring(edit.entry.time, FALSE));
			else if (dc->view == 'm' || dc->view == 'o')
				strcpy(buf, mkdatestring(edit.entry.rep_last &&
						 edit.entry.rep_every == 86400
						 ? edit.entry.rep_last
						 : edit.entry.time));
			else
				strcpy(buf, mktimestring(edit.entry.time,
							 FALSE));
			print_button(dc->info, buf);
			edit.entry = save;
			draw_rubberband(dc->canvas, TRUE, x, y, xs, ys);
		}
		if (event->state & Button1Mask)
			return;
		entry = 0;					/* mouse move*/
		warn  = 0;
		mode  = (*dc->locate_cb)(&entry, &warn, &time, &trigger,
					 0, 0, 0, 0, event->x, event->y);
		set_cursor(canvas, mode);
		if (!dc->info)
			return;
		if (!entry) {
			print_button(dc->info, mode != M_OUTSIDE ? " " :
				     dc->view == 'd' || dc->view == 'w' ?
				     "%s %s":"%s", mkdatestring(time),
						   mktimestring(time, FALSE));
			return;
		}
		sprintf(buf, warn ? _("Advance warning: %s") : "%s",
						mkdatestring(trigger));
		if (entry->rep_last > entry->time && !warn)
			sprintf(buf+strlen(buf), " - %s",
						mkdatestring(entry->rep_last));
		if (!entry->notime)
			sprintf(buf+strlen(buf), ", %s", mktimestring(trigger,
								      FALSE));
		if (entry->user)
			sprintf(buf+strlen(buf), ": %s", entry->user);
		if (entry->note)
			sprintf(buf+strlen(buf), ": %s", entry->note);
		print_button(dc->info, "%s", buf);

	}
	if (!strcmp(args[0], "up")) {				/* drag end */
		draw_rubberband(dc->canvas, FALSE, 0, 0, 0, 0);
		set_cursor(canvas, mode);
		if (moving) {
			m = (*dc->locate_cb)(0, 0, &time, 0, 0, 0, 0, 0,
							event->x, event->y);
			if (m != M_OUTSIDE && m != M_LEFT && m != M_RIGHT)
				print_button(dc->info,
					_("Cannot move there, aborted."));
			else {
				drag_entry(dc->view, mode, time0, time);
				confirm_new_entry();
				update_all_listmenus(TRUE);
				redraw_all_views();
			}
			moving = FALSE;
		} else {					/* editing */
			if (mode == M_INSIDE && dc->view != 'm')
				create_list_popup(mainlist,
						  (time_t)0, (time_t)0,
						  (char *)0, entry, 0, 0);

			else if (mode == M_OUTSIDE || mode == M_INSIDE)
				create_list_popup(mainlist, time0-time0%86400,
						  (time_t)0, (char *)0,
						  (struct entry *)0, 0, 0);
		}
	}
}


/*
 * a subroutine of the provious: the user has moved the mouse, or released
 * the left mouse button, while editing (dragging) an appointment. Apply the
 * time change reported by the initial and current locate function to the
 * appointment.
 */

static void drag_entry(
	int		view,		/* current view: 'm'=month, ... */
	MOUSE		mode,		/* what's being moved: M_LEFT? */
	time_t		time0,		/* mouse time when first pressed */
	time_t		time)		/* current mouse time */
{
	time_t		mod;		/* unit of movement, 5 min or 1 day */
	time_t		d_t;		/* time+date difference */
	time_t		d_d;		/* date difference, multiple of days */
	struct entry	*ep = &edit.entry;

	mod = view == 'm' || view == 'o' ? 86400 : 300;
	d_t = time - time % mod   - (time0 - time0 % mod);
	d_d = time - time % 86400 - (time0 - time0 % 86400);
	if (mode == M_LEFT) {			/* new begin */
		if (ep->rep_last > ep->time)
			ep->rep_last += d_d;
		ep->time += d_t;

	} else if (view != 'o') {		/* new length*/
		ep->length += d_t;
		if (ep->length < 0)
			ep->length = 0;
	} else {				/* or new end*/
		if (!ep->rep_last)
			ep->rep_last = ep->time - ep->time % 86400;
		ep->rep_last += d_d;
		ep->rep_every = 86400;
		if (ep->rep_last <= ep->time) {
			ep->rep_last  = 0;
			ep->rep_every = 0;
		}
	}
	edit.changed = TRUE;
}


/*
 * draw or undraw a rubber band box into the canvas, using XOR drawing. This
 * must be called in draw-undraw pairs.
 */

static void draw_rubberband(
	Widget		canvas,		/* drawing area to draw into */
	BOOL		draw,		/* draw or undraw */
	int		x,		/* position of box */
	int		y,
	int		xs,		/* size of box */
	int		ys)
{
	static BOOL	is_drawn;	/* TRUE if rubber band exists */
	static int	lx,ly,lxs,lys;	/* drawn rubberband */

	if (is_drawn) {
		is_drawn = FALSE;
		XDrawRectangle(display, XtWindow(canvas), xor_gc,
					lx, ly, lxs, lys);
	}
	if (draw) {
		is_drawn = TRUE;
		XDrawRectangle(display, XtWindow(canvas), xor_gc,
					lx=x-1, ly=y-1, lxs=xs+1, lys=ys+1);
	}
}
