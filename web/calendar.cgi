#!/usr/bin/perl
# author: Michel Bourget, hacked for plan 1.8 by thomas@bitrot.de
#--------------------------------------------------------------------------

#--------------------------------------------------------------------------
require ("./cgi-lib.pl") || die "can\'t require cgi-lib.pl: $!";;
require ("./common.pl") || die "can\'t require common.pl: $!";;

#--------------------------------------------------------------------------
# Print the HTML document
#--------------------------------------------------------------------------
sub form {
   print "Content-type: text/html



<HTML>
<HEAD>
<TITLE>The Schedule Viewer </TITLE>
<FRAMESET ROWS=\"12%,82%,6%\">
<FRAME NAME=\"Menu\" SRC=\"menu.cgi?group=$group&show_private=$show_private&show_appt=$show_appt\"  SCROLLING=\"auto\"  >
<FRAME NAME=\"calendar\" SRC=\"webmonth.cgi?group=$group&show_private=$show_private&show_appt=$show_appt\"  SCROLLING=\"auto\"  >
<FRAME NAME=\"user-info\" SRC=\"bottom.html\"  SCROLLING=\"no\"  >
</FRAMESET>
";
}

#--------------------------------------------------------------------------
# Main Function
#--------------------------------------------------------------------------
   &ReadParse;
  
   if ( $in{group} eq "" ) {
	$group="none";
   }
   else {
	$group=$in{group}
   }

   if ( $in{show_private} eq "" ) {
	$show_private="no";
   }
   else {
	$show_private=$in{show_private}
   }

   if ( $in{show_appt} eq "" ) {
	$show_appt="no";
   }
   else {
	$show_appt=$in{show_appt}
   }

   &form;


