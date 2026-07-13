#
# installation directories. DIR contains the main interactive plan program
# and LIB contains everything else (pland, notifier, plan.help, PostScript
# skeletons). If you change any of these, remove util.o.
# INCDIR is for searching nonstandard include directories, eg. "-I/usr/X11R6".
# ALWAYS TERMINATE macro assignments with #. DO NOT put a blank between the
# path and the #, or you'll get compile errors. DO NOT put blanks before and
# after "=", this is parsed and rewritten by the configure script.
#
#				# user-callable executable (plan)
DIR=/usr/bin
#				# auxiliary programs (pland, notifier, PS)
LIB=/usr/lib/plan
#				# help file (plan.help)
SBIN=/usr/sbin
#				# system programs
HELP=/usr/share/plan
#				# unformatted nroff manpage (plan.[14])
MAN=/usr/share/man/man
#				# formatted manpage (plan.[14])
CATMAN=NONE
#				# subdirectory in home dir, or empty
PLANRULE=-DPLANHOME=\"".plan.dir"\"
#				# lockfile for plan, %d is uid
PLANLOCK="~/.plan.dir/lock.plan"
#				# lockfile for pland, %d is uid
PLANDLOCK="~/.plan.dir/lock.pland"
#				# extra cc options, e.g. "-I/var/include"
INCDIR=
#				# standard Unix lexer
LEX=lex
#				# standard Unix compiler generator
YACC=yacc
#				# numerical user ID of <nobody> account
NOB_UID=63434
#				# default numerical group ID of <nobody>
NOB_GID=63434
# 				# used by plan to pre-process holiday files,
#				# plan will check if this file really exists
CPP_PATH='/usr/bin/cpp -P -I$(HELP)'
#				# by default, fork on startup? TRUE or FALSE
DOFORK=FALSE
#				# switching to ISO8859-1
ESCTOISOLATIN=\"\\033\(B\"
#				# switching to JIS X0201-1976 roman
ESCTOJISLATIN=\"\\033\(J\"
#				# uncomment this for Japanese version of plan
#				# JAPAN requires X11R5 and a Japanese env.
JAPANDEFS='-DJAPAN -DESCTOASCII=$(ESCTOJISLATIN) -DLENTOASCII=3 -DLOCALE_SJIS=ja_JP.SJIS -DLOCALE_EUC=ja_JP.EUC'
JAPAN=
#				# default holiday locale
HOLIDAY=spain
#				# do NOT define -O2 on DEC OSF/1 machines!
#				# use -O instead of -O2 on HP and Solaris
DEBUG=-O2
DEBUG=-O2
#				# what to do without make arguments
DEFTARGET=linux
#------------------------- end of configuration section ----------------------

OPT=$(DEBUG) $(PLANRULE) -DPLANLOCK=\"$(PLANLOCK)\" -DPLANDLOCK=\"$(PLANDLOCK)\"

DESTDIR=

#
# compiler flags. Add these to *_C macro as needed:
#
#	-DDESKTOP	for SGIs running IRIX 5.2, desktop look&feel, 4Dwm only
#	-DBSD		use setpgrp(0, 0) and new wait(); also for OSF systems
#	-DSUN		for SunOS 4.1.x systems
#	-DIBM		for IBM AIX systems, includes sys/lockf.h
#	-DULTRIX	for Ultrix 4.3/Motif 1.1.3 systems, different font
#	-DPID_T=long	for IBM/AIX systems and other systems with pids>30000
#	-DFIXMBAR	use if pulldowns in the menu bar are empty (HP/UX 8.x)
#	-DNOMSEP	remove all XmVaSEPARATORs in pulldowns (some HP/UX's)
#	-DNOREGEX	if you don't have regcmp() and regex()
#	-DNOLIMIT	if there is no /usr/include/limits.h
#	-DNOKILL0	if kill(pid,0) works (sends no signal, pid check only)
#	-DFLOCK		use flock instead of lockf
#	-DNOLOCK	use if lockf() and flock() are both undefined
#	-DUSERAND	use rand/srand instead of random/srandom
#	-DBACKING	for window backing store if supported (untested)
#	-DRABBITS	daemon survives logout (if there is no utmp/getpwnam)
#	-DDESTROYBUG	fixes Linux XtDestroyWidget bug that kills plan when a
#			Done button is used. WARNING: introduces a memory leak
#	-DSTDLIBMALLOC	do not include <malloc.h>, use <stdlib.h> instead,
#			(FreeBSD)
#
# Note for HP systems: if you have the PROG-AUX option, remove -DNOREGEX
# from HP_C, and add -lPW to HP_L1. On Suns, try removing -DNOREGEX (and
# adding -lPW if necessary) to get regular expression searches. If you have
# problems with HP/UX and missing pulldowns, try adding/removing -DFIXMBAR.
# Sun users using Solaris 2.1 and gcc may have to add -DNOMSEP for pulldowns,
# and -DNOLOCK if "deadlock avoided" messages appear. There is at least one
# IBM that will compile plan only with -DNOLOCK if -DIBM is removed.
# Don't use a renamed copy of this Makefile, it uses itself recursively.
# If your linker can't find libraries such as libXm.a, setenv LD_LIBRARY_PATH.
# SGI is compiled with -32 because IRIX 6.2 doesn't install -n32 Motif libs
# by default, and the cvd debugger works better with -32 too.
#

SGI4_C	=
SGI5_C	= -DSYSV -DSVR4 -DDESKTOP
SGI6_C	= -n32 -mips3 -woff 1167 -DSYSV -DSVR4 -DDESKTOP -D_BSD_SIGNALS
HP8_C	= -Aa -I/usr/include/Motif1.1 -I/usr/include/X11R4 -I/usr/include\
	  -D_INCLUDE_HPUX_SOURCE -D_INCLUDE_POSIX_SOURCE\
	  -D_INCLUDE_XOPEN_SOURCE -DNOREGEX -DFIXMBAR -DRABBITS
HP9_C	= -Aa -I/usr/include/Motif1.2 -I/usr/include/X11R5 -I/usr/include\
	  -D_HPUX_SOURCE -DRABBITS -DNOMSEP -DNOREGEX -DUSERAND
HP_C    = -Ae -I/usr/include/Motif1.2_R6 -I/usr/include/X11R6 -I/usr/include\
          -D_HPUX_SOURCE -DRABBITS
HPG_C	= -ansi -I/usr/include/Motif1.2 -I/usr/include/X11R5 -DHPGCC\
	  -DRABBITS -DNOMSEP -DNOREGEX -DUSERAND
IBM_C	= -O -I/usr/include/X11 -D_ALL_SOURCE -DIBM -DAIXV3 -DNOMSEP -DRABBITS
PS2_C	= -O -I/usr/include/X11 -D_POSIX_SOURCE\
	  -DIBM -D_XOPEN_SOURCE -D_ANSI_C_SOURCE -DNOMSEP -DNOLOCK
OSF_C	= -DNOREGEX -DBSD -DOSF -DRABBITS -DNOMSEP -DFLOCK
SUN_C	= -DSUN -DRABBITS -DNOMSEP
SUNG_C	= -DSUN -DRABBITS -DNOMSEP -ansi -I/usr/5include -I/usr/include/X11\
	  -DNOREGEX
SPARC_C	= -DSUN -DRABBITS -DNOMSEP -ansi -I/usr/5include -I/usr/include/X11\
	  -DNOREGEX -Dsparc
SOL_C	= -DSUN -DSOLARIS2 -DNOMSEP -I/usr/local/X11R5/include\
	  -I/usr/local/X11R5/include/X11 -I/usr/ucbinclude
SOLG_C	= -DSUN -DSOLARIS2 -DNOMSEP -DUSERAND -I/usr/openwin/include\
	  -I/usr/dt/include -L/usr/dt/lib
ULTRIX_C= -DNOLOCK -DULTRIX -DRABBITS -DNOMSEP
CDC_C	= -DUSERAND
MIPS_C	= -DMIPS -systype sysv -Wf,-XNd4000 -DPID_T="int" -DMIPS\
	  -I/usr/include/bsd
DG_C	=
SNI_C	= -DSYSV -DNOREGEX -DUSERAND
ISC_C	= -DSYSV -DNOREGEX -D_POSIX_SOURCE -D_XOPEN_SOURCE -D_SYSV3
NCR_C	= -DNCR -DNOMSEP -DRABBITS
USL_C	= -DNOKILL0 -I/usr/X/include
BSDI_C	= -DBSD -DRABBITS -I/usr/X11/include -DFLOCK -DNOREGEX -Dbsdi
BSDIG_C	= -DBSD -DRABBITS -I/usr/X11/include -DFLOCK -DNOREGEX -Dbsdi -ansi 
NBSD_C	= -ansi -I/usr/X11R6/include -DBSD -DRABBITS -DNOLOCK -DNOREGEX -DNO_MOTIF
FBSD_C	= -ansi -I/usr/X11R6/include -DBSD -DNOLOCK -DNOREGEX -DRABBITS -DSTDLIBMALLOC
OBSD_C	= -ansi -I/usr/X11R6/include -DBSD -DRABBITS -DNOLOCK -DNOREGEX -DNO_MOTIF -DSTDLIBMALLOC
LINUX_C	= -W -Wall -Wcast-align -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes \
    -Wpointer-arith -Wreturn-type -Wcast-qual -I/usr/X11/include -DNOLOCK -DLINUX -Dlinux -DRABBITS
LINUX64_C = $(LINUX_C)
SVR4_C	= -DSVR4 -DSYSV -DOLDTZ -DUSERAND -DPID_T=long -DNOMSEP
NEWS_C	= -DNEWSOS4 -DNOMSEP -DSUN -DBSD -DRABBITS -DNGROUPS_MAX=16 -DPID_T="int" -Dconst=""
NEWSG_C	= -DNEWSOS4 -DNOMSEP -DSUN -DBSD -DRABBITS -DNGROUPS_MAX=16 -DPID_T="int"
NEWS6_C	= -DNOMSEP -DSVR4 -DRABBITS -DNOREGEX
NEWS6G_C= -DNOMSEP -DSVR4 -DRABBITS -DNOREGEX
SQNT2_C	= -I/usr/include  -DUSERAND -DBACKING -DSYSV -D_NO_PROTO -DUSG
SQNT4_C	= -I/usr/include  -DBACKING -DUSERAND -DSETREUID
CONV_C	= -Dconvex -DBSD -DRABBITS -I/usr/X11/include -DNOLOCK -DNOREGEX -Dbsdi
UW_C	= -I/usr/X/include -DOLDTZ -DUSERAND \
	  -DSVR4 -D_INCLUDE_POSIX_SOURCE -D_INCLUDE_XOPEN_SOURCE
APOL_C	= -I/usr/include/X11R5 -I/usr/include -D_APOLLO_SOURCE -DRABBITS -DNOMSEP -DNOREGEX \
      -DUSERAND -DNOLOCK
OS2_C	= -Zmt -Wimplicit -Wno-unused -I$(X11ROOT)/XFree86/include -DNOMSEP -DNOLOCK
LYNX_C  = -DNOREGEX -D__NO_INCLUDE_WARN__ -DBSD -mthreads
MACOSX_C= -W -Wall -I/usr/X11R6/include -I/usr/OpenMotif/include -DNOLOCK -DMACOSX -DNOREGEX


#
# loader flags. Requires X11, Xt, and Motif libraries. PW is for regcmp/regex.
#

SGI_L1	= -lXm_s -lXt_s -lXmu -lX11_s -lPW -lsun
SGI_L2	= -lsun -lc_s
SGI5_L1	= -lXm -lXt -lX11 -lPW
SGI5_L2	= -lc
SGI6_L1	= -lXm -lXt -lX11 -lPW
SGI6_L2	= -L/usr/local/lib -lc
HP8_L1	= -L/usr/lib/X11R4 -L/usr/lib/Motif1.1 -lXm -lXt -lX11 -lc
HP8_L2	=
HP9_L1	= -L/usr/lib/X11R5 -L/usr/lib/Motif1.2 -lXm -lXt -lX11 -lc
HP9_L2	=
HP_L1	= -L/usr/lib/X11R6 -L/usr/lib/Motif1.2_R6 -lXm -lXt -lX11 -lc -lPW
HP_L2	= -lXm -lXt -lX11 -lc -lPW
IBM_L1	= -lXm -lXt -lX11 -lPW
IBM_L2	=
PS2_L1	= -lXm -lXt -lX11 -lPW
PS2_L2	=
OSF_L1	= -lXm -lXt -lX11
OSF_L2	=
SUN_L1	= -lXm -lXt -lX11
SUN_L2	= -lc
SUNG_L1	= -L/usr/5lib -lXm -lXt -lX11
SUNG_L2	= -L/usr/5lib -lc
SPARC_L1= -L/usr/5lib -lXm -lXt -lX11 -lm
SPARC_L2= -L/usr/5lib -lc
SOL_L1	= -L$(OPENWINHOME)/lib -L$(MOTIFHOME)/lib -lXm -lXt -lX11 -lelf -lgen -lsocket -lnsl
SOL_L2	= -lc -lsocket -lnsl
SOLG_L1	= -L$(OPENWINHOME)/lib -L$(MOTIFHOME)/lib -lXm -lXt -lX11 -lelf -lgen -lsocket -lnsl
SOLG_L2	= -lc -lsocket -lnsl
ULTRIX_L1= -lXm -lXt -lX11
ULTRIX_L2=
CDC_L1	= -lXm -lXt -lX11 -lgen -lnsl -lsocket
CDC_L2	= -lgen -lnsl -lsocket
MIPS_L1	= -lXm -lXt -lX11 -lbsd -lsun -lc -lPW
#MIPS_L1= -L/usr/X11/lib -lXm -lXt -lX11 -lbsd -lc -lPW
MIPS_L2	=
DG_L1	= -lXm -lXt -lX11 -lPW
DG_L2	=
SNI_L1	= -lXm -lXt -lX11 -lsocket -lnsl -lXmu -lXext -lgen 
SNI_L2	= -lsocket -lnsl
ISC_L1	= -lXm -lXt -lX11 -lXmu -linet -lnsl_s
ISC_L2	=
NCR_L1	= -lXm -lXt -lX11 -lXext -lnsl -lgen
NCR_L2	= -lgen
USL_L1	= -B static -lXm -B dynamic -lXaw -lXext -lXmu -lXt -lX11 -lm -lnsl -lgen
USL_L1m	= -lXm -lXaw -lXext -lXmu -lXt -lX11 -lm -lnsl -lgen
USL_L2	= -lc
BSDI_L1	= -lXm -lXt -lX11
BSDI_L2	= -lc
BSDIG_L1= -L/usr/X11/lib -lXm -lXt -lX11
BSDIG_L2= -L/usr/X11/lib -lc
NBSD_L1	= -L/usr/X11R6/lib -lXm -lXt -lX11
NBSD_L2	= -L/usr/X11R6/lib -lc
FBSD_L1	= -L/usr/X11R6/lib -lXm -lXt -lX11 -lXext -lSM -lICE -lXpm
FBSD_L2	= -L/usr/X11R6/lib -lc
OBSD_L1	= -L/usr/local/lib -lXm -L/usr/X11R6/lib -lXt -lX11
OBSD_L2	= -lc
LINUX_L1= -L/usr/X11R6/lib -L/usr/lib/X11R6/lib -lXm -lXt -lX11 -lXpm -lc -lXext -lICE -lX11
LINUX_L2= -lc
SLINUX_L1= -L/usr/X11R6/lib -L/usr/lib/X11R6/lib -lSM /usr/X11R6/lib/libXm.a -lXt -lX11 /usr/X11R6/lib/libXpm.a -lc -lXext -lSM -lICE -lX11
SLINUX_L2= -lc
SVR4_L1	= -lXm -lXt -lX11 -lgen -lsocket -lnsl
SVR4_LK2=
NEWS_L1	= -lXm -lXt -lX11 -lXlc
NEWS_L2	=
NEWS6_L1= -lXm -lXt -lX11 -lsocket -lnsl
NEWS6_L2= -lsocket -lnsl
SQNT2_L1= -Xo -lXm -lXaw -lXmu -lXt -lXext -lX11 -lm -lPW -leli -linet -lnsl -lseq
SQNT2_L2= 
SQNT4_L1= -lXm -lXaw -lXmu -lXt -lXext -lX11 -lm -lPW
SQNT4_L2= -lsocket -lnsl -lseq
CONV_L1	= -lXm -lXt -lX11
CONV_L2	= -lc
UW_L1	= -lXm -lXt -lX11 -lgen -lnsl
UW_L2	= -lgen
APOL_L1	= -L/usr/lib/X11R5 -L/usr/lib/Motif1.2 -lXm -lXt -lX11 -lc
APOL_L2	=
OS2_L1	= -s -Zexe -Zmtd -L$(X11ROOT)/XFree86/lib -lXm -lXt -lX11 -lsocket -lbsd -lregexp -Zsysv-signals
OS2_L2	= -s -Zexe -Zmtd -lsocket -lbsd -Zsysv-signals
LYNX_L1	= -mthreads -lXm_s -lXt_s -lSM -lICE -lXpm -lXext -lXmu -lX11_s -lbsd
LYNX_L2	= -mthreads -lbsd
MACOSX_L1= -L/usr/X11R6/lib -L/usr/OpenMotif/lib -lSM -lXm -lXt -lX11 -lXpm -lc -lXext -lSM -lICE -lX11
MACOSX_L2= -lc


start: $(DEFTARGET)

#---------------------------------------------------------------
# to add another system here, define XXX_C, XXX_L1, and XXX_L2 by picking
# available options from the list at the top of this Makefile, add an echo
# here, and add a target in the next section. Please mail patches to me.

help:
	@echo "Usage:"
	@echo "  make sgi4      4.x SGI systems"
	@echo "  make sgi5      5.x SGI systems, desktop look&feel, 4Dwm only"
	@echo "  make sgi       6.x SGI systems, desktop look&feel, 4Dwm only"
	@echo "  make hp8       HP/UX 8 with X11R4 and Motif 1.1.5"
	@echo "  make hp9       HP/UX 9 with X11R5 and Motif 1.2"
	@echo "  make hp        HP/UX 10/11 with X11R5 and Motif 1.2"
	@echo "  make hpg       HP/UX 10/11 with X11R5, Motif 1.2, gcc"
	@echo "  make ibm       IBM RS6000 running AIX"
	@echo "  make ps2       IBM PS/2 Model 8577, AIX 1.3 with mpp level 0013 patches, gcc"
	@echo "  make osf       DEC running OSF/1"
	@echo "  make sun       Sun with Motif (won't work with OpenLook)"
	@echo "  make sung      Sun 4.1.3/Solaris 2.2 with Motif, using gcc"
	@echo "  make sparc     Sun 4.1.3 on SUN 4C, gcc, adds -Dsparc option"
	@echo "  make sol       Solaris 2.5 with Motif, using cc"
	@echo "  make solg      Solaris 2.5 with Motif, using gcc"
	@echo "  make ultrix    Ultrix 4.3 with Motif 1.1.3"
	@echo "  make cdc       CDC/Mips running EP/IX 2.1.1"
	@echo "  make mips      Mips Magnum RISCos 4.52 with RISCwindows 4.0"
	@echo "  make dg        Data General DG/UX, with gcc"
	@echo "  make sni       SNI SINIX 5.41"
	@echo "  make isc       SunSoft Interactive Unix v4.0"
	@echo "  make ncr       NCR 3000 systems"
	@echo "  make usl       USL SVR4.2 V1 with mdev package"
	@echo "  make uslm      ...same with Motif run-time libraries"
	@echo "  make linux     Linux with Motif"
	@echo "  make slinux    Linux with statically linked Motif"
	@echo "  make bsdi      BSDI/386"
	@echo "  make bsdig     BSDI/386 with gcc"
	@echo "  make netbsd    NetBSD/i386 1.0A (maybe others too)"
	@echo "  make freebsd   FreeBSD 2.x"
	@echo "  make openbsd	OpenBSD"
	@echo "  make svr4      SVR4 Using gcc"
	@echo "  make news4     Sony NEWS-OS4.x"
	@echo "  make news4g    Sony NEWS-OS4.x with gcc"
	@echo "  make news6     Sony NEWS-OS6.x"
	@echo "  make news6g    Sony NEWS-OS6.x with gcc"
	@echo "  make sqnt2     Sequent Dynix/ptx 2.0 or 2.1, using cc"
	@echo "  make sqnt4     Sequent Dynix/ptx 4.0 or 4.1, using cc"
	@echo "  make convex    Convex C220 running BSD, gcc"
	@echo "  make uw        UnixWare 1.1 and 2.0"
	@echo "  make apollo    HP/Apollo DomainOS"
	@echo "  make os2       OS/2, XFree86"
	@echo "  make lynx      LynxOS mag 2.5.1 092297-D i386"
	@echo "  make macosx    MacOS X with OpenMotif 2.1 (www.ist-inc.com)"
	@echo ""
	@echo "  make install   install compiled programs in"
	@echo "                 $(DIR), $(SBIN), and $(LIB)."
	@echo "  make clean     remove everything in this dir except sources"
	@echo ""
	@echo "If you have problems, first read this Makefile. You may also"
	@echo "want to change the installation directory if you are not root."
	@echo "The installation directory is set at the top of this Makefile."
	@echo "For a Japanese version, edit the JAPAN line in the Makefile."


#---------------------------------------------------------------

sgi: sgi6

sgi4:
	env CF="$(SGI4_C)" L1F="$(SGI_L1)" L2F="$(SGI_L2)" MYCC=cc make all

sgi5:
	env CF="$(SGI5_C)" L1F="$(SGI5_L1)" L2F="$(SGI5_L2)" MYCC=cc make all

sgi6:
	env CF="$(SGI6_C)" L1F="$(SGI6_L1)" L2F="$(SGI6_L2)" MYCC=cc make all

sgi6g:
	env CF="$(SGI6_C)" L1F="$(SGI6_L1)" L2F="$(SGI6_L2)" MYCC=gcc make all

hp8:
	env CF="$(HP8_C)" L1F="$(HP8_L1)" L2F="$(HP8_L2)" MYCC=cc make all

hp9:
	env CF="$(HP9_C)" L1F="$(HP9_L1)" L2F="$(HP9_L2)" MYCC=cc make all

hp:
	env CF="$(HP_C)" L1F="$(HP_L1)" L2F="$(HP_L2)" MYCC=cc make all

hpg:
	bison -y -d parser.y
	-mv y.tab.c parser.c
	env CF="$(HPG_C)" L1F="$(HP_L1)" L2F="$(HP_L2)" MYCC=gcc make all

ibm:
	env CF="$(IBM_C)" L1F="$(IBM_L1)" L2F="$(IBM_L2)" MYCC=cc make all

ps2:
	env CF="$(PS2_C)" L1F="$(PS2_L1)" L2F="$(PS2_L2)" MYCC=gcc make all

osf:
	env CF="$(OSF_C)" L1F="$(OSF_L1)" L2F="$(OSF_L2)" MYCC=cc make all

sun:
	env CF="$(SUN_C)" L1F="$(SUN_L1)" L2F="$(SUN_L2)" MYCC=acc make all

sung:
	env CF="$(SUNG_C)" L1F="$(SUNG_L1)" L2F="$(SUNG_L2)" MYCC=gcc make all

sparc:
	env CF="$(SPARC_C)" L1F="$(SPARC_L1)" L2F="$(SPARC_L2)" MYCC=gcc \
		make NOB_UID=65534 NOB_GID=65534 all

sol:
	env CF="$(SOL_C)" L1F="$(SOL_L1)" L2F="$(SOL_L2)" MYCC=cc make all

solg:
	env CF="$(SOLG_C)" L1F="$(SOLG_L1)" L2F="$(SOLG_L2)" MYCC="LD_RUN_PATH=/usr/dt/lib gcc" make all

ultrix:
	env CF="$(ULTRIX_C)" L1F="$(ULTRIX_L1)" L2F="$(ULTRIX_L2)" MYCC=cc make all

cdc:
	env CF="$(CDC_C)" L1F="$(CDC_L1)" L2F="$(CDC_L2)" MYCC=cc make all

mips:
	env CF="$(MIPS_C)" L1F="$(MIPS_L1)" L2F="$(MIPS_L2)" MYCC=cc make all

dg:
	env CF="$(DG_C)" L1F="$(DG_L1)" L2F="$(DG_L2)" MYCC=gcc make all

sni:
	env CF="$(SNI_C)" L1F="$(SNI_L1)" L2F="$(SNI_L2)" MYCC=cc make all

isc:
	env CF="$(ISC_C)" L1F="$(ISC_L1)" L2F="$(ISC_L2)" MYCC=gcc make all

ncr:
	env CF="$(NCR_C)" L1F="$(NCR_L1)" L2F="$(NCR_L2)" MYCC=cc make all

usl:
	env CF="$(USL_C)" L1F="$(USL_L1)" L2F="$(USL_L2)" MYCC=cc make all

uslm:
	env CF="$(USL_C)" L1F="$(USL_L1m)" L2F="$(USL_L2)" MYCC=cc make all

bsdi:
	env CF="$(BSDI_C)" L1F="$(BSDI_L1)" L2F="$(BSDI_L2)" MYCC=acc make all

bsdig:
	env CF="$(BSDIG_C)" L1F="$(BSDIG_L1)" L2F="$(BSDIG_L2)" MYCC=gcc make all

netbsd:
	env CF="$(NBSD_C)" L1F="$(NBSD_L1)" L2F="$(NBSD_L2)" MYCC=gcc $(MAKE) all

freebsd:
	env CF="$(FBSD_C)" L1F="$(FBSD_L1)" L2F="$(FBSD_L2)" MYCC=${CC} make all

openbsd:
	env CF="$(OBSD_C)" L1F="$(OBSD_L1)" L2F="$(OBSD_L2)" MYCC=${CC} make all

svr4:
	env CF="$(SVR4_C)" L1F="$(SVR4_L1)" L2F="$(SVR4_L2)" MYCC=gcc make all

linux:
	-env CF="$(LINUX_C)" L1F="$(LINUX_L1)" L2F="$(LINUX_L2)" MYCC=gcc make all
	@echo --------------------
	@echo 'NOTE: if compilation failed with "cannot find -lXt", try "make clean; make linux64"'

linux64:
	env CF="$(LINUX64_C)" L1F="$(LINUX_L1)" L2F="$(LINUX_L2)" MYCC=gcc make all

slinux:
	env CF="$(LINUX_C)" L1F="$(SLINUX_L1)" L2F="$(SLINUX_L2)" MYCC=gcc make all

news4:
	make CF="$(NEWS_C)" L1F="$(NEWS_L1)" L2F="$(NEWS_L2)" MYCC=cc all

news4g:
	make CF="$(NEWSG_C)" L1F="$(NEWS_L1)" L2F="$(NEWS_L2)" MYCC=gcc all

news6:
	env CF="$(NEWS6_C)" L1F="$(NEWS6_L1)" L2F="$(NEWS6_L2)" MYCC=/usr/bin/cc make all

news6g:
	env CF="$(NEWS6G_C)" L1F="$(NEWS6_L1)" L2F="$(NEWS6_L2)" MYCC=gcc make all

sqnt2:
	env CF="$(SQNT2_C)"  L1F="$(SQNT2_L1)" L2F="$(SQNT2_L2)" MYCC=cc make all

sqnt4:
	env CF="$(SQNT4_C)"  L1F="$(SQNT4_L1)" L2F="$(SQNT4_L2)" MYCC=cc make all

convex:
	env CF="$(CONV_C)" L1F="$(CONV_L1)" L2F="$(CONV_L2)" MYCC=gcc make all

uw:
	env CF="$(UW_C)" L1F="$(UW_L1)" L2F="$(UW_L2)" MYCC=cc make all

apollo:
	env CF="$(APOL_C)" L1F="$(APOL_L1)" L2F="$(APOL_L2)" MYCC=cc make all

os2:
	make all CF="$(OS2_C)" L1F="$(OS2_L1)" L2F="$(OS2_L2)" MYCC=gcc

lynx:
	make CF="$(LYNX_C)" L1F="$(LYNX_L1)" L2F="$(LYNX_L2)" MYCC=gcc \
	                LEX="flex -l" YACC="bison -y" all

macosx:
	env CF="$(MACOSX_C)" L1F="$(MACOSX_L1)" L2F="$(MACOSX_L2)" MYCC=gcc make all

#------------------------------ don't change anything below ------------------

INC	= bm_blank.h bm_except.h bm_icon.h bm_iconsub.h bm_checker.h\
	  bm_private.h bm_message.h bm_noticon.h bm_recycle.h bm_script.h\
	  bm_circle.h bm_chkcircle.h bm_tri.h bm_todo.h cal.h config.h proto.h\
	  notifier.h version.h resources.h grok.h form.h

P_SRC	= main.c popup.c print.c gotomenu.c network.c\
	  calmenu.c monmenu.c mondraw.c editmenu.c editdraw.c edit.c\
	  msgmenu.c cycmenu.c cyccalc.c keymenu.c excmenu.c\
	  cnfmenu.c yearmenu.c yeardraw.c holmenu.c adjmenu.c\
	  weekmenu.c weekdraw.c weekcalc.c servmenu.c usermenu.c confmenu.c\
	  yovmenu.c yovdraw.c yovcalc.c daymenu.c daydraw.c daycalc.c\
	  usersel.c psdraw.c xutil.c help.c dbase.c file_r.c file_w.c\
	  convert.c util.c time.c sublist.c parser.y parser.lex holiday.c\
	  language.c g_dbase.c g_dbfile.c g_formfile.c g_formop.c \
	  vcalendar_r.c vcalendar_w.c

P_OBJ	= main.o popup.o print.o gotomenu.o network.o\
	  calmenu.o monmenu.o mondraw.o editmenu.o editdraw.o edit.o\
	  msgmenu.o cycmenu.o cyccalc.o keymenu.o excmenu.o\
	  cnfmenu.o yearmenu.o yeardraw.o holmenu.o adjmenu.o\
	  weekmenu.o weekdraw.o weekcalc.o servmenu.o usermenu.o confmenu.o\
	  yovmenu.o yovdraw.o yovcalc.o daymenu.o daydraw.o daycalc.o\
	  usersel.o psdraw.o xutil.o help.o dbase.o file_r.o file_w.o\
	  convert.o util.o time.o sublist.o parser.o lex.yy.o holiday.o\
	  language.o g_dbase.o g_dbfile.o g_formfile.o g_formop.o \
	  vcalendar_r.o vcalendar_w.o

D_SRC	= daemon.c dbase.c file_r.c util.c time.c cyccalc.c network.c\
	  convert.c language.c g_dbase.c g_dbfile.c g_formfile.c g_formop.c\
	  vcalendar_r.c
D_OBJ	= daemon.o dbase.o file_r.o util.o time.o cyccalc.o network.o\
	  convert.o language.o g_dbase.o g_dbfile.o g_formfile.o g_formop.o\
	  vcalendar_r.o

N_SRC	= notifier.c notmenu.c
N_OBJ	= notifier.o notmenu.o

CCLINE	= $(MYCC) $(OPT) $(CF) $(JAPAN) $(INCDIR) $(CPPFLAGS) $(CFLAGS)

#---------------------------------------------------------------
all:	plan pland notifier

plan:	$(P_OBJ)
	$(CCLINE) -o plan $(P_OBJ) $(LDFLAGS) $(L1F)

pland:	$(D_OBJ)
	$(CCLINE) -o pland $(D_OBJ) $(LDFLAGS) $(L2F)

notifier: $(N_OBJ)
	$(CCLINE) -o notifier $(N_OBJ) $(LDFLAGS) $(L1F)

.c.o:
	$(CCLINE) -c $<

print.o: print.c $(MAKEFILE)
	$(CCLINE) -c -DLIB=\"$(LIB)\" $<

network.o: network.c netplan_if.h $(MAKEFILE)
	$(CCLINE) -c -DLIB=\"$(LIB)\" $<

usermenu.o: usermenu.c $(MAKEFILE)
	$(CCLINE) -c -DLIB=\"$(LIB)\" $<

holiday.o: holiday.c $(MAKEFILE)
	$(CCLINE) -DCPP_PATH=\"$(CPP_PATH)\" -c -DLIB=\"$(LIB)\" $<

util.o: util.c $(MAKEFILE)
	$(CCLINE) -c -DPATH=\"$(LIB):$(DIR):$(HELP)\" $<

daemon.c: $(Makefile)

parser.c: parser.y
	$(YACC) -d parser.y
	-mv y.tab.c parser.c

lex.yy.o: lex.yy.c parser.c
	$(CCLINE) -c lex.yy.c

lex.yy.c: parser.lex
	$(LEX) parser.lex

$(P_OBJ): config.h cal.h proto.h
$(D_OBJ): config.h proto.h
$(N_OBJ): notifier.h
$(S_OBJ): $(S_INC)
main.o popup.o file_r.o file_w.o daemon.o notifier.o: version.h
main.o:	resources.h
g_dbase.o g_dbfile.o g_formfile.o g_formop.o: grok.h form.h

#---------------------------------------------------------------
install:
	-rm -f $(DESTDIR)$(DIR)/pland $(DESTDIR)$(DIR)/notifier
	mkdir -p $(DESTDIR)$(DIR)
	cp plan $(DESTDIR)$(DIR)
	mkdir -p $(DESTDIR)$(LIB)
	cp pland notifier ../misc/plan_cal.ps ../language/plan* $(DESTDIR)$(LIB)
	-(test x'$(JAPAN)' != x && cp ../misc/plan_calSJ.ps $(DESTDIR)$(LIB)) || exit 0
	mkdir -p $(DESTDIR)$(HELP)
	cp ../language/plan.help $(DESTDIR)$(HELP)
	-strip $(DESTDIR)$(DIR)/plan $(DESTDIR)$(LIB)/pland $(DESTDIR)$(LIB)/notifier
	-chmod 755 $(DESTDIR)$(DIR)/plan $(DESTDIR)$(LIB)/pland $(DESTDIR)$(LIB)/notifier
	-chmod 644 $(DESTDIR)$(HELP)/plan.help $(DESTDIR)$(LIB)/plan_cal.ps $(DESTDIR)$(LIB)/plan.lang.*
	-(test x'$(JAPAN)' != x && chmod 644 $(DESTDIR)$(LIB)/plan_calSJ.ps) || exit 0

	-(test x'$(CATMAN)' != xNONE -a -x "`which nroff`" && nroff -man ../misc/plan.1 >../misc/plan.1cat) || exit 0
	-(test x'$(CATMAN)' != xNONE -a -x "`which nroff`" && nroff -man ../misc/plan.4 >../misc/plan.4cat) || exit 0
	-(test x'$(MAN)'    != xNONE -a -d $(DESTDIR)$(MAN)1 && cp ../misc/plan.1 $(DESTDIR)$(MAN)1/plan.1) || exit 0
	-(test x'$(MAN)'    != xNONE -a -d $(DESTDIR)$(MAN)4 && cp ../misc/plan.4 $(DESTDIR)$(MAN)4/plan.4) || exit 0
	-(test x'$(CATMAN)' != xNONE -a -d $(DESTDIR)$(CATMAN)1 && cp ../misc/plan.1cat $(DESTDIR)$(CATMAN)1/plan.1) || exit 0
	-(test x'$(CATMAN)' != xNONE -a -d $(DESTDIR)$(CATMAN)4 && cp ../misc/plan.4cat $(DESTDIR)$(CATMAN)4/plan.4) || exit 0
	-(test -f ${HOME}/.holiday || cp ../holiday/holiday_$(HOLIDAY) ${HOME}/.holiday) || exit 0
	-(test `uname` = IRIX -o `uname` = IRIX64 && mkdir $$HOME/.icons) || exit 0
	-(test `uname` = IRIX -o `uname` = IRIX64 && cp ../misc/Plan.icon $$HOME/.icons/Plan.icon) || exit 0
	@echo ""
	@echo "use ../holiday/* as templates for your ~/.holiday file"

clean:
	-rm -f *.o
	-rm -f y.tab.h y.tab.c parser.c lex.yy.c
	-rm -f y.output core plan pland notifier tags

#---------------------------------------------------------------
# for my own use, for making releases etc.

VERSION	= `egrep define.VERSION     version.h | sed 's/"$$//;s/.*"//;s/[V ]//g'`
SVERSION= `egrep define.VERSION src/version.h | sed 's/"$$//;s/.*"//;s/[V ]//g'`
LIST	= README HISTORY plan.spec holiday/* language/* misc/* src/resources.h.bigfonts web/*
SRC	= Makefile $(P_SRC) daemon.c $(N_SRC) $(S_SRC) $(INC) $(S_INC) configure

LDIR	= /usr/X11R6/lib
BINDIR	= plan-$(VERSION)
gMYCC	= gcc
gCF	= -m32 -I/usr/X11R6/include -D_POSIX_SOURCE -DNOLOCK -DLINUX -Dlinux
gL1F	= -L$(LDIR) $(LDIR)/libXm.a -lXt -lX11 $(LDIR)/libXpm.a -lc -lXext -lICE -lXp -lX11
gL2F	= -lc

linuxtar: $(P_OBJ) $(D_OBJ) $(N_OBJ) $(S_OBJ) $(MAKEFILE) README.linuxbin
	-rm -rf $(BINDIR)
	mkdir $(BINDIR) $(BINDIR)/src
	cp README.linuxbin $(BINDIR)
	$(gMYCC) $(OPT) $(gCF) $(JAPAN) $(INCDIR) -o $(BINDIR)/src/plan     $(P_OBJ) $(gL1F)
	$(gMYCC) $(OPT) $(gCF) $(JAPAN) $(INCDIR) -o $(BINDIR)/src/pland    $(D_OBJ) $(gL2F)
	$(gMYCC) $(OPT) $(gCF) $(JAPAN) $(INCDIR) -o $(BINDIR)/src/notifier $(N_OBJ) $(gL1F)
	tar czvf ../plan-$(VERSION).linuxbin.tar.gz $(BINDIR)
	rm -rf $(BINDIR)

tar:
	mv Makefile Makefile.save
	cp Makefile.orig Makefile
	cd ..; rm -rf plan-$(SVERSION).tar.gz plan-$(SVERSION)
	cd ..; mkdir plan-$(SVERSION)
	cd ..; tar chf - $(LIST) `echo " $(SRC)" | sed 's= = src/=g'` | (cd plan-$(SVERSION); tar xvf -)
	cd ..; tar zhcvf plan-$(SVERSION).tar.gz plan-$(SVERSION)
	cd ..; rm -rf plan-$(SVERSION)
	mv Makefile.save Makefile

rpm: tar
	cd ..; rpmbuild -vv -ba plan-1.10-20070416.spec
