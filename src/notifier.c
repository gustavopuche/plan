/*
 * Notify program. Pops up a green, yellow, or red menu with some text
 * in it. A title can be specified, default is "Notify". The text is
 * read from the argument file name, or from standard input.
 *
 * This program is intended to be used by the pland daemon. It must
 * reside in the same directory as pland; pland looks at its own path
 * in argv[0] to locate notifier.
 *
 * Author: thomas@bitrot.de (Thomas Driemeyer)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <Xm/Xm.h>
#include <X11/StringDefs.h>
#include "cal.h"
#undef COL_RED
#undef NCOLS
#include "notifier.h"
#include "version.h"
#include "bm_noticon.h"

#if defined(XlibSpecificationRelease) && (defined(sgi) || defined(_sgi))
#include <X11/Xmu/Editres.h>
#define EDITRES
#endif
#ifndef _XEditResCheckMessages
#undef  EDITRES
#endif

#ifdef USERAND
#define random rand
#define srandom srand
#endif

static void readfile		(char **, FILE *);
static void usage		(void);
static char *string_resource	(char *, char *);
static void init_colors		(void);

Display			*display;		/* common server */
GC			gc;			/* common graphics context */
char			*progname;		/* argv[0] */
Pixel			color[NCOLS];		/* colors: COL_* */
Pixel			bkcolor = COL_RED;	/* background color (COL_*) */
XtAppContext		app;			/* application handle */
static Widget		toplevel;		/* widget tree */


static String fallbacks[] = {
	"*background:		#a0a0a0",
	"*colStd:		#202020",
	"*colRedAlert:		#ff0000",
	"*colYellowAlert:	#c0a000",
	"*colGreenAlert: 	#20a020",
#if !defined(sgi) && !defined(_sgi)
	"*fontList:		-*-helvetica-*-r-*-*-17-*",
#else
	"*fontList:		-b&h-*-medium-r-*-*-17-*",
#endif
	"*title.fontList:	-*-times-*-*-*-*-34-*",
#ifdef JAPAN	/* For Japanese. */
#ifndef sun
#ifdef NEWSOS4
	"*subtitle.fontList:	-*-times-*-r-*-*-14-*,14x14kanji=JISX0208.1983-0",
	"*message.fontList:	-*-new century schoolbook-*-r-*-*-17-*,16x16kanji=JISX0208.1983-0",
#else	/* For NON-NEWSOS4 */
	"*subtitle.fontList:	-*-times-*-r-*-*-14-*;-*-fixed-medium-r-normal--14-*-*-*-*-*-jisx0208.1983-*;-*-fixed-medium-r-normal--14-*-*-*-*-*-jisx0201.1976-*:",
		/* This may be a wrong fontList in case of sgi.*/
#if !defined(sgi) && !defined(_sgi)
	"*message.fontList:	-*-new century schoolbook-*-r-*-*-17-*;-*-fixed-medium-r-normal--16-*-*-*-*-*-jisx0208.1983-*;-*-fixed-medium-r-normal--16-*-*-*-*-*-jisx0201.1976-*:",
#else
	"*message.fontList:	-*-palatino-*-r-*-*-17-*;-*-fixed-medium-r-normal--16-*-*-*-*-*-jisx0208.1983-*;-*-fixed-medium-r-normal--16-*-*-*-*-*-jisx0201.1976-*:",
#endif	/* !defined(sgi) && !defined(_sgi) */
#endif	/* The end of NEWSOS4 */
#else	/* For sun */
	"*subtitle*fontList:    -*-times-*-r-*-*-14-*;-sun-*-14-*-jisx0208.1983-0;-sun-*-14-*-jisx0201.1976-0:",
	"*message.fontList:     -*-new century schoolbook-*-r-*-*-17-*;-sun-minchou-*-18-*-jisx0208.1983-0;-sun-*-18-*-jisx0201.1976-0:",
#endif	/* The end of sun */
#else
	"*subtitle.fontList:	-*-times-*-r-*-*-14-*",
#if !defined(sgi) && !defined(_sgi)
	"*message.fontList:	-*-new century schoolbook-*-r-*-*-17-*",
#else
	"*message.fontList:	-*-palatino-*-r-*-*-17-*",
#endif
#endif
#ifdef DESKTOP
	"*sgiMode:		True",
	"*useSchemes:		True",
#endif
	NULL
};


/*
 * initialize everything and create the main calendar window
 */

int main(
	int			argc,
	char			*argv[])
{
	Pixmap			icon;
	int			n, i;
	String			*p;
	char			*msg = 0;	/* points to ascii message */
	char			*title = "Notifier";
	char			*subtitle = "";
	char			*icontitle = "Notify";
	long			timeout = 0;
	char			geometry[40], *q;

	if ((progname = strrchr(argv[0], '/')) && progname[1])
		progname++;
	else
		progname = argv[0];
	n = time((void *)0);
	for (i=0; i < argc; i++)
		for (q=argv[i]; *q; q++)
			n += n + *q;
	srandom(n);
	sprintf(geometry, "*geometry: +%d+%d",	(int)(random()*16&240),
						(int)(random()*16&240));
	fallbacks[0] = geometry;
#ifdef JAPAN	/* For Japanese. */
	XtSetLanguageProc(NULL, NULL, NULL);
#endif
	toplevel = XtAppInitialize(&app, title, NULL, 0,
#		ifndef XlibSpecificationRelease
				(Cardinal *)&argc, argv,
#		else
				/* X11R5 introduced XlibSpecificationRelease
				   and made cast unnecessary */
				&argc, argv,
#		endif
				fallbacks, NULL, 0);
#	ifdef EDITRES
	XtAddEventHandler(toplevel, (EventMask)0, TRUE, 
				_XEditResCheckMessages, NULL);
#	endif
	display = XtDisplay(toplevel);
	gc = XCreateGC(display, DefaultRootWindow(display), 0, 0);
	init_colors();
	if ((icon = XCreatePixmapFromBitmapData(display,
				DefaultRootWindow(display),
				icon_bits, icon_width, icon_height,
				WhitePixelOfScreen(XtScreen(toplevel)),
				BlackPixelOfScreen(XtScreen(toplevel)),
				DefaultDepth(display, 0))))

		XtVaSetValues(toplevel, XmNiconPixmap, icon, NULL);

	for (n=1; n < argc; n++)
		if (*argv[n] == '-')
			for (i=1; i && argv[n][i]; i++)
				switch(argv[n][i]) {
				  case 'd':
					for (p=fallbacks; *p; p++)
						printf("%s%s\n", progname, *p);
					fflush(stdout);
					exit(0);
				  case 'v':
					fprintf(stderr, "%s: %s\n", progname,
							VERSION JAPANVERSION);
					exit(0);
				  case 'e':
					timeout = atol(&argv[n][i+1]);
					i = -1;
					break;
				  case 't':
					title = &argv[n][i+1];
					i = -1;
					break;
				  case 's':
					subtitle = &argv[n][i+1];
					i = -1;
					break;
				  case 'i':
					icontitle = &argv[n][i+1];
					i = -1;
					break;
				  case '1':
					bkcolor = COL_GREEN;
					break;
				  case '2':
					bkcolor = COL_YELLOW;
					break;
				  case '3':
					bkcolor = COL_RED;
					break;
				  default:
					usage();
				}
		else {
			FILE *fp;
			if (msg)
				usage();
			if (!(fp = fopen(argv[n], "r"))) {
				fprintf(stderr, "%s: ", progname);
				perror(argv[n]);
				exit(1);
			}
			readfile(&msg, fp);
			fclose(fp);
		}
	if (!msg)
		readfile(&msg, stdin);

	XtVaSetValues(toplevel, XmNiconName, icontitle, NULL);
	create_widgets(toplevel, title, subtitle, msg);
	XtRealizeWidget(toplevel);
	XBell(display, 0);
	XBell(display, 0);
	if (timeout)
		(void)XtAppAddTimeOut(app, timeout*60*1000,
						(XtTimerCallbackProc)exit, 0);
	XtAppMainLoop(app);
	return(0);
}


/*
 * whenever something goes seriously wrong, this routine is called. It makes
 * code easier to read. fatal() never returns. This may fail horribly if
 * VARARGS is defined, but at least it's going to do *something*.
 */

#ifndef VARARGS
/*VARARGS*/
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

#else
/*VARARGS*/
void fatal(
	char	*fmt,
	int	a,
	int	b,
	int	c,
	int	d)
{
	fprintf(stderr, fmt, a, b, c, d);
	putc('\n', stderr);
	exit(1);
}
#endif


/*
 * read the message to be displayed in the window from a file (or stdin).
 * Allocate enough storage.
 * This routine avoids feof() because it seems to be broken on some Unix PCs.
 */

static void readfile(
	char			**msg,
	FILE			*fp)
{
	long			size = 4098;	/* msg size, two more for \0 */
	long			nread = 0;	/* how many read so far */
	int			ncs;		/* how many added this time */

	if (!(*msg = malloc(size)))
		fatal("can't malloc %d bytes for message", size);
	for (;;) {
		ncs = fread(*msg + nread, 1, 4096, fp);
		(*msg)[nread += ncs] = 0;
		if (ncs < 4096)
			break;
		if (!(*msg = realloc(*msg, size += 4096)))
			fatal("can't realloc %d bytes for message", size);
	}
}


/*
 * usage information
 */

static void usage(void)
{
	fprintf(stderr,
		"Usage: %s [options] [file]\nOptions:\n%s%s%s%s%s%s%s%s%s%s",
			progname,
			"\t-h\t\tprint this help text\n",
			"\t-d\t\tdump fallback app-defaults and exit\n",
			"\t-tstring\tset title string (quote blanks)\n",
			"\t-sstring\tset subtitle string (quote blanks)\n",
			"\t-istring\tset icon title string (quote blanks)\n",
			"\t-v\t\tprint version string\n",
			"\t-eN\t\texpire after N minutes\n",
			"\t-1\t\tprint green window\n",
			"\t-2\t\tprint yellow window\n",
			"\t-3\t\tprint red window\n");
	exit(1);
}


/*
 * read real application resources
 */

static char *string_resource(
	char		*res_name,
	char		*res_class_name)
{
	char		*res;
	XtResource	res_list[1];

	res_list->resource_name   = res_name;
	res_list->resource_class  = res_class_name;
	res_list->resource_type   = XtRString;
	res_list->resource_size   = sizeof(XtRString);
	res_list->resource_offset = 0;
	res_list->default_type    = XtRString;
	res_list->default_addr    = NULL;

	XtGetApplicationResources(toplevel, &res, res_list, 1, NULL, 0);
	return(res);
}


/*
 * determine all colors, and allocate them.
 */

static void init_colors(void)
{
	Screen			*screen = DefaultScreenOfDisplay(display);
	Colormap		cmap;
	XColor			rgb;
	int			i, d;
	char			*c, *n, class_name[256];

	cmap = DefaultColormap(display, DefaultScreen(display));
	for (i=0; i < NCOLS; i++) {
		switch (i) {
		  default:
		  case COL_STD:		n = "colStd";		d = 1;	break;
		  case COL_RED:		n = "colRedAlert";	d = 0;	break;
		  case COL_YELLOW:	n = "colYellowAlert";	d = 0;	break;
		  case COL_GREEN:	n = "colGreenAlert";	d = 0;	break;
		}
		strcpy(class_name, n);
		class_name[0] &= ~('a'^'A');
		c = string_resource(n, class_name);
		if (!XParseColor(display, cmap, c, &rgb))
			fprintf(stderr, "%s: unknown color for %s\n",
							progname, n);
		else if (!XAllocColor(display, cmap, &rgb))
			fprintf(stderr, "%s: can't alloc color for %s\n",
							progname, n);
		else {
			color[i] = rgb.pixel;
			continue;
		}
		color[i] = d ? BlackPixelOfScreen(screen)
			     : WhitePixelOfScreen(screen);
	}
}
