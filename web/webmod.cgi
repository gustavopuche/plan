#!/usr/bin/perl
#--------------------------------------------------------------------------

#--------------------------------------------------------------------------
require ("./cgi-lib.pl") || die "can\'t require cgi-lib.pl: $!";;
require ("./common.pl") || die "can\'t require common.pl: $!";;

@days=(0,31,28,31,30,31,30,31,31,30,31,30,31 );



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
   

#  print "<FONT COLOR=\"#ff0000\">Function disabled until further notice.</FONT>\n<br>";
#  print "<FONT COLOR=\"#ff0000\">Please use Day view for details.</FONT>\n<br>";
#  print "Sorry for inconvenience. This is 1st P1 bug :)\n<br>";
#  return;

   print "<FORM method=post action=webmodDel.cgi TARGET=\"user-info\">\n";


   @PlanData=&get_plan_data(0,$Month,$Year,none);

   $found=0;
   foreach $line ( @PlanData ) {
	   @info=split(/;/,$line);
	   if ( $Row == $info[0] ) {
		   $found=1;
		   last;
	   }
   }
   if ( $found == 0 ) {
	print "<FONT COLOR=\"#ff0000\">Entry not found; please use \"Get Calendar\" to re-sync</FONT>\n";
   }
   else {

   	print "<font size=3>";
   	print "<FONT COLOR=\"#0000ff\">$info[4]</FONT> ";
	print "$info[1] ";
        if ( "$info[2]" ne "-" ) {
	  print "<U>From:</U> $info[2] ";
	  print "<U>Length:</U> $info[3] ";
        } else {
          print "<U>Day event</U> ";
        }
	print "<I>$info[5]</I> ";

	
   	print "<INPUT TYPE=\"HIDDEN\" NAME=\"row\" VALUE=\"$Row\">";
   	print "<INPUT TYPE=\"HIDDEN\" NAME=\"qui\" VALUE=\"$info[4]\">";
   	print "<INPUT TYPE=\"PASSWORD\" NAME=\"userid\" SIZE=\"12\" >";
   	print "<INPUT TYPE=SUBMIT VALUE=\"Delete It\"> \n" ;
   }
   print "</FORM>\n" ;

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
   &ReadParse;
   $Row=$in{row};
   $Month=$in{month};
   $Year=$in{year};

   &header;
   &form;
   &footer;

