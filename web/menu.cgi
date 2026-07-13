#!/usr/bin/perl
#--------------------------------------------------------------------------

#--------------------------------------------------------------------------
require ("./cgi-lib.pl") || die "can\'t require cgi-lib.pl: $!";;
require ("./common.pl") || die "can\'t require common.pl: $!";;




#--------------------------------------------------------------------------
# User-definable variables
#--------------------------------------------------------------------------
$myurl="http:webmonth.cgi";

@month=( "", "January ", "February", "March", "April", "May", "June", 
         "July", "August", "September", "October", "November", "December" );

&main(@ARGV);

#--------------------------------------------------------------------------
#--------------------------------------------------------------------------
sub header {
   local($mo,$yr)=@_;

   print "Content-type: text/html


<HTML>
<HEAD>
<TITLE>January 1999</TITLE>
</HEAD>
<BODY>
";
} # header

#--------------------------------------------------------------------------
# Print the content of the FORM
#--------------------------------------------------------------------------
sub form {
   
   print "<FORM method=post action=$myurl TARGET=\"calendar\">\n";


   @group_list=&get_group;
   
   print "<FONT size=2>";
   print "<CENTER>";
  
   print "<SELECT NAME=\"group\">\n";
   print "<OPTION ";
   print "SELECTED " if ( $group eq "none" ) ;
   print "VALUE=\"none\"> (All Users) \n"; 
   foreach (@group_list) {
       $group_elem=$_; 
       print "<OPTION ";
       chop $group_elem;
       print "SELECTED " if ( $group eq "$group_elem" ) ;
       print "VALUE=\"$group_elem\">", $group_elem, "\n";
   }
   print "</SELECT>\n";

   print "<SELECT NAME=\"years\">\n";
   for ($i=1998;$i<=2034;$i++) {
       print "<OPTION ";
       print "SELECTED " if ($i == $cur_year );
       print "VALUE=\"$i\">", $i, "\n";
   }
   print "</SELECT>\n";

   print "<SELECT NAME=\"month\">\n";
   for ($i=1;$i<=12;$i++) {
       print "<OPTION ";
       print "SELECTED " if ($i == $cur_month );
       print "VALUE=\"$i\">", $month[$i], "\n";
   }
   print "</SELECT>\n";

   print "<SELECT NAME=\"jour\">\n";
   print "<OPTION VALUE=0>(All Days)\n";
   for ($i=1;$i<=31;$i++) {
       print "<OPTION ";
       print "SELECTED " if ( $i == $cur_jour );
       print "VALUE=\"$i\">", $i, "\n";
   }
   print "</SELECT>\n";

   print "<SELECT NAME=\"show_private\">\n";
   print "<OPTION ";
   print "SELECTED " if ( $show_private eq "no" );
   print "VALUE=\"no\">Private: no\n";
   print "<OPTION ";
   print "SELECTED " if ( $show_private eq "yes" );
   print "VALUE=\"yes\">Private: yes\n";
   print "</SELECT>\n";

   print "<SELECT NAME=\"show_appt\">\n";
   print "<OPTION ";
   print "SELECTED " if ( $show_appt eq "no" );
   print "VALUE=\"no\">Appointment: no\n";
   print "<OPTION ";
   print "SELECTED " if ( $show_appt eq "yes" );
   print "VALUE=\"yes\">Appointment: yes\n";
   print "</SELECT>\n";


   print "<INPUT TYPE=SUBMIT VALUE=\"Get Calendar\"> \n" ;
   print "<A HREF=\"help.html\" target=\"calendar\"><I><U><B>Help?</I></U></B></A>";
   print "</FONT>";
   print "</CENTER>";
   print "</form>\n" ;

} # form

#--------------------------------------------------------------------------
# Print the Footer
#--------------------------------------------------------------------------
sub footer {
   print <<"END";
END
   print "</HTML>";
} # footer

#--------------------------------------------------------------------------
# Main Function 
#--------------------------------------------------------------------------
sub main {

   ($sec,$min,$hour,$mday,$cur_month,$cur_year)=localtime(time);
   $cur_jour=$mday;
   $cur_month++;
   $cur_year+=1900;

   &ReadParse;

   if( $in{group} eq "" ) {
	$group="none";
   }
   else {
	$group=$in{group};
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
        $show_appt=$in{show_appt};
   }

   &header;
   &form;
   &footer;

} # main
