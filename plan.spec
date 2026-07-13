Name: plan
Summary: plan - an X/Motif day planner
Version: 1.16
Release: 20191122
License: GPL
Group: Applications/Productivity
BuildRoot: %{_tmppath}/build-root-%{name}
Packager: Thomas Driemeyer (old)
Url: http://www.bitrot.de/plan.html (old)
Packager: Joachim Wiedorn
Url: https://www.joonet.de/plan/

%description
Plan is a schedule planner based on X/Motif. It displays a month calendar similar to xcal, but every day box is large enough to show appointments in small print. By pressing on a day box, the appointments for that day can be listed and edited. Appointments are entered with the following information (everything except the time is optional):

- the date, time, and length (time and days) of the appointment,
- an optional text message to be printed,
- an optional script to be executed,
- early-warn and late-warn triggers that precede the alarm time
- repetitions: [n-th] weekdays, days-of-the-month, every n days, yearly
- optional fast command-line appointment entry
- flexible ways to specify holidays and vacations
- extensive context help
- multiuser capability using an IP server program (with access lists),
- grouping of appointments into files, per-user, private, and others

The action being taken when a warn or alarm time is reached is programmable; by default a window pops up. In addition, a program can be executed, or mail can be sent. Other methods of listing appointments (today, this week, next week, or a keyword search for regular expressions) are also available. Plan can be configured to display times in 12-hour or 24-hour formats, mmddyy and ddmmyy date formats, and can show either Monday or Sunday in the leftmost column. Four view modes are supported: month, year, week, day, and a 365-day overview. The day, week, and overview plot appointments as colored and labeled bars on a time chart. 

Plan homepage: http://www.bitrot.de/plan.html

%prep

%setup -q

%build
cd src; make -j 2 linux

%install
cd src; make install

%clean
cd src; make clean; rm plan pland netplan notifier

cd $RPM_BUILD_ROOT

find . -type d -fprint $RPM_BUILD_DIR/file.list.%{name}.dirs
find . -type f -fprint $RPM_BUILD_DIR/file.list.%{name}.files.tmp
sed '/\/man\//s/$/.gz/g' $RPM_BUILD_DIR/file.list.%{name}.files.tmp > $RPM_BUILD_DIR/file.list.%{name}.files
find . -type l -fprint $RPM_BUILD_DIR/file.list.%{name}.libs
sed '1,2d;s,^\.,\%attr(-\,root\,root) \%dir ,' $RPM_BUILD_DIR/file.list.%{name}.dirs > $RPM_BUILD_DIR/file.list.%{name}
sed 's,^\.,\%attr(-\,root\,root) ,' $RPM_BUILD_DIR/file.list.%{name}.files >> $RPM_BUILD_DIR/file.list.%{name}
sed 's,^\.,\%attr(-\,root\,root) ,' $RPM_BUILD_DIR/file.list.%{name}.libs >> $RPM_BUILD_DIR/file.list.%{name}

%files
%defattr(-,root,root)
%doc README HISTORY

/usr/bin/plan
/usr/bin/netplan
/usr/lib/pland
/usr/lib/notifier
/usr/man/man1/plan.1
/usr/man/man4/plan.4
/usr/man/man1/netplan.1

%changelog
* Mon Apr 16 2007 Thomas Driemeyer
FEATURES:
    * implemented limited read-only support for vCalendar/iCalendar (.ics)
      files (Apple, Zimbra, Lotus, etc), which can be specified in File ->
      File list like other files. Repetition information is ignored.
    * added "make linux64" target. Some 64-bit Linux installations, notably
      OpenSUSE 10.2, omit 32-bit libraries or links, causing compilation errors
      for -lXt and others.
    * ported to MacOS X with X11 and OpenMotif. Requires X11 from the MacOS X
      install DVD, and OpenMotif from www.ist-inc.com/DOWNLOADS/motif_files/
      openmotif-compat-2.1.31_IST.macosx10.3.dmg . The X server must run, and
      DISPLAY must be set properly (like, localhost:0) or plan won't start up.
      This is not a proper Aqua/Cocoa/Carbon port, although I made some feeble
      attempts to fix the color scheme.
    * new Polish language file by Jaroslaw Arlet <j.arlet@awf-gorzow.edu.pl>
    * new command-line option -Y allows entering annual (yearly) appointments,
      by Steffen Pietsch <Steffen.Pietsch@berlinonline.de>
    * when alarms trigger, '%' codes in the short note text, message, and the
      script are expanded. (See the help text in plan's Message/Script help.)
      Eg., you can write message texts like "remember to see %U on %D at %T".
	  %N    the short note text
	  %M    the message text
	  %S    the script text
	  %D    the final trigger date
	  %T    the final trigger time
	  %L    the length in hours:minutes
	  %F    the file the appointment is stored in, or "private"
	  %U    your login name
	  %%	a percent sign, '%'

BUG FIXES:
    * fixed a timezone bug that could make alarms go off exactly 24 hours late,
      Aaron Kaplan <kaplan@cs.rochester.edu>
    * manpage files were installed without the trailing ".1" and ".4"
    * fixed compiler errors under Cygwin because the symbol linux was not set.
    * added -m32 option for Linux; gcc 4 defaults to 64 bits but the required
      libraries are not usually installed.
    * entering a day of the month as a date will now correctly find the next
      date with that day. A bug caused it to switch to a random date in 2000.
    * fixed a language file read error that omitted a trailing null byte.
      By Julien Soula <jsoula@univ-lille2.fr>
    * an incomplete X resource file crashed plan. Now defaults are used; the
      resulting windows are not very usable but it doesn't crash.

For older versions see the HISTORY file.

%defattr(-,root,root,0755)
