/*
 * PostScript/window drawing routines shared by the week and year overview
 * charts. Both call these functions to draw and don't care whether the
 * graphivs go to the screen or to the PostScript printer.
 *
 *	g_init_window	(week, region)
 *	g_exit_window	(void)
 *	g_init_ps	(week, fp, landscape)
 *	g_exit_ps	(void)
 *
 *	g_setcolor	(color)
 *	g_setfont	(fonti)
 *	g_fillrect	(x, y, xs, ys)
 *	g_drawtext	(x, y, text, len)
 *	g_drawtext16	(x, y, text, len)
 *	g_drawpoly	(point, num, checker)
 *	g_drawlines	(point, num)
 */

#include <stdio.h>
#include <time.h>
#include <Xm/Xm.h>
#include "cal.h"

extern Display		*display;	/* everybody uses the same server */
extern GC		jgc, gc;	/* graphic context, Japanese and std */
extern GC		chk_gc;		/* checkered gc for light background */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern struct config	config;		/* global configuration data */
extern XColor		xcolor[NCOLS];	/* useful for PostScript color */

static FILE		*fp;		/* PostScript file, or 0 if window */
static BOOL		ps_landscape;	/* if PostScript, rotate 90 degrees */
static double		ps_xsize, ps_ysize;	/* PS paper size in inches */
static double		ps_xoff,  ps_yoff;	/* PS paper size in points */
static Window		window;		/* if fp==0, X window to draw into */
static Region		region;		/* if fp==0, X clip region */


/*---------------------------------- init/exit ------------------------------*/
/*
 * init graphics routines for printing into window. The week structure has
 * info on the current window size and IDs and is also used by the year
 * overview. Return FALSE if drawing is not possible.
 */

BOOL g_init_window(
	struct week	*week,		/* either &week or &yov */
	Region		newregion)	/* draw only here */
{
	Arg		args[2];

	region = newregion;
	if (!week->canvas) {
		if (region)
			XDestroyRegion(region);
		return(FALSE);
	}
	XtSetArg(args[0], XmNwidth,  week->canvas_xs);
	XtSetArg(args[1], XmNheight, week->canvas_ys);
	XtSetValues(week->canvas, args, 2);
	if (region) {
		XSetRegion(display, gc,     region);
		XSetRegion(display, chk_gc, region);
	}
	fp = 0;
	window = XtWindow(week->canvas);
	return(TRUE);
}

void g_exit_window(void)
{
	if (region) {
		XSetClipMask(display, gc,     None);
		XSetClipMask(display, chk_gc, None);
		XDestroyRegion(region);
		region = 0;
	}
}


/*
 * print PostScript week calendar to file fp. Paper sizes are chosen so
 * everything fits on both A4 (297x210 mm) and letter (11x8.5 in) sizes.
 * 36 points (1/2 in) is reserved for a margin around all edges. Landscape
 * is done by "rotating the paper", not swapping X/Y coords.
 * ISO code suggested by Axel Stegner <stegner@fb3-s7.math.tu-berlin.de>.
 */

void g_init_ps(
	struct week	*week,		/* either &week or &yov */
	FILE		*newfp,
	BOOL		landscape)
{
#ifdef JAPAN
	fprintf(fp = newfp,
"%%!PS-Adobe\n\
%%%%Creator: plan\n\
%%%%Title: Week Calendar\n\
%%%%EndComments\n\
0.5 setlinewidth\n\
/DefNormFont /Helvetica findfont def\n\
/DefJapaneseFont /GothicBBB-Medium-H findfont def\n");
/* /DefJapaneseFont /Ryumin-Light-H findfont def\n"); */
#else
	fprintf(fp = newfp,
"%%!PS-Adobe\n\
%%%%Creator: plan\n\
%%%%Title: Week Calendar\n\
%%%%EndComments\n\
0.5 setlinewidth\n\
%% Set up ISO Latin 1 character encoding\n\
/reencodeISO {\n\
dup dup findfont dup length dict begin\n\
	{ 1 index /FID ne { def }{ pop pop } ifelse\n\
	} forall\n\
	/Encoding ISOLatin1Encoding def\n\
	currentdict end definefont\n\
	} def\n\
/Helvetica-Bold reencodeISO def\n\
/Helvetica reencodeISO def\n\
/Courier reencodeISO def\n\
/Courier-Bold reencodeISO def\n\
/DefNormFont /Helvetica findfont def\n");
#endif
	if ((ps_landscape = landscape)) {
		fprintf(fp,
"newpath clippath pathbbox pop exch pop exch add 0 translate 90 rotate\n");
		ps_xsize = 10.0 * 72;
		ps_ysize =  7.2 * 72;
		ps_xoff  = week->canvas_xs;
		ps_yoff  = week->canvas_ys > 800 ? week->canvas_ys : 800;
	} else {
		ps_xsize =  7.2 * 72;
		ps_ysize = 10.0 * 72;
		ps_xoff  = week->canvas_xs;
		ps_yoff  = week->canvas_ys > 1200 ? week->canvas_ys : 1200;
	}
}

void g_exit_ps(void)
{
	fprintf(fp, "showpage\n");
}


/*---------------------------------- drawing --------------------------------*/

#define XSCALE(x) (36.0 +	     (x)  * ps_xsize / ps_xoff)
#define YSCALE(y) (36.0 + (ps_yoff - (y)) * ps_ysize / ps_yoff)

/*
 * The g_setcolor routine uses white for COL_CALTODAY, so today is not
 * highlighted.
 */

void g_setcolor(
	int	color)
{
	if (!fp)					/* to window */
		set_color(color);

	else if (config.pr_color)			/* color PostScript */
		fprintf(fp, "%g %g %g setrgbcolor ",
			xcolor[color].red   / 65536.,
			xcolor[color].green / 65536.,
			xcolor[color].blue  / 65536.);
	else {						/* b/w PostScript */
		double col;
		if (color >= COL_WUSER_1 && color <= COL_WUSER_7)
			col = (color - COL_WUSER_1) / 7.0 * 0.15 + 0.8;
		else if (color >= COL_HBLACK && color <= COL_HWHITE)
			col = (color - COL_HBLACK) / 7.0 * 0.3;
		else switch(color) {
		  case COL_WUSER_0:	col = 1.0;	break;
		  case COL_CALTODAY:	col = 1.0;	break;
		  case COL_STD:		col = 0;	break;
		  case COL_WBACK:	col = 1.0;	break;
		  case COL_WBOXBACK:	col = 0.97;	break;
		  case COL_WDAY:	col = 0;	break;
		  case COL_WFRAME:	col = 0;	break;
		  case COL_WGRID:	col = 0;	break;
		  case COL_WNOTE:	col = 0;	break;
		  case COL_WTITLE:	col = 0;	break;
		  case COL_WWARN:	col = 0.95;	break;
		  case COL_WEEKDAY:	col = 0;	break;
		  case COL_WEEKEND:	col = 0;	break;
		  default:		col = 0;	break;
		}
		fprintf(fp, "%g setgray ", col);
	}
}

void g_setfont(
	int	fonti)
{
	if (fp) {
		fprintf(fp, "DefNormFont %d scalefont setfont\n",
			ps_landscape ? fonti == FONT_WTITLE ? 20 : 8
				     : fonti == FONT_WTITLE ? 14 : 6);
	}
#ifdef JAPAN
	else if (fonti == FONT_JNOTE)
		XSetFont(display, jgc, font[fonti]->fid);
#endif
	else
		XSetFont(display, gc, font[fonti]->fid);
}


void g_fillrect(
	int	x,
	int	y,
	int	xs,
	int	ys)
{
	if (fp) {
		fprintf(fp,
     "%g %g moveto %g %g lineto %g %g lineto %g %g lineto %g %g lineto fill\n",
   			XSCALE(x),    YSCALE(y),
   			XSCALE(x+xs), YSCALE(y),
			XSCALE(x+xs), YSCALE(y+ys),
			XSCALE(x),    YSCALE(y+ys),
   			XSCALE(x),    YSCALE(y));
	} else
		XFillRectangle(display, window, gc, x, y, xs, ys);
}


void g_drawtext(
	int	x,
	int	y,
	char	*text,
	int	len)
{
	if (fp) {
		fprintf(fp, "%g %g moveto (", XSCALE(x), YSCALE(y));
		while (*text && len--) {
			if (*text == '(' || *text == ')')
				fputc('\\', fp);
			fputc(*text++, fp);
		}
		fprintf(fp, ") show\n");
	} else
		XDrawString(display, window, gc, x, y, text, len);
}


#ifdef JAPAN
void g_drawtext16(
        int             x,
        int             y,
        char            *text,
        int             len)
{
	if (fp) {
		fprintf(fp, "gsave DefJapaneseFont %d scalefont setfont\n"
				"%g %g moveto <",
				ps_landscape ? 9 : 7, XSCALE(x), YSCALE(y));
		while (*text && len--)
			fprintf(fp, "%02X", *text++);
		fprintf(fp, "> show grestore\n");
	} else
		XDrawString16(display, window, jgc, x, y, (XChar2b *)text, len);
}
#endif


void g_drawpoly(
	XPoint	*point,
	int	num,
	BOOL	checker)
{
	if (fp) {
		int n;
		fprintf(fp, "%g %g moveto ",
   			XSCALE(point[0].x), YSCALE(point[0].y));
		for (n=1; n < num; n++)
			fprintf(fp, "%g %g lineto ",
				XSCALE(point[n].x), YSCALE(point[n].y));
		fprintf(fp, "fill\n");
	} else {
		XFillPolygon(display, window, gc, point, num,
						Convex, CoordModeOrigin);
		if (checker)
			XFillPolygon(display, window, chk_gc, point, num,
						Convex, CoordModeOrigin);
	}
}


void g_drawlines(
	XPoint	*point,
	int	num)
{
	if (fp) {
		int n;
		fprintf(fp, "%g %g moveto ",
   			XSCALE(point[0].x), YSCALE(point[0].y));
		for (n=1; n < num; n++)
			fprintf(fp, "%g %g lineto ",
				XSCALE(point[n].x), YSCALE(point[n].y));
		fprintf(fp, "stroke\n");
	} else
		XDrawLines(display, window, gc, point, num, CoordModeOrigin);
}
