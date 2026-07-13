#!/usr/bin/perl

require ("./cgi-lib.pl") || die "can\'t require cgi-lib.pl: $!";;
require ("./common.pl") || die "can\'t require common.pl: $!";;

@days=(0,31,28,31,30,31,30,31,31,30,31,30,31 );



#--------------------------------------------------------------------------
# Header Routine
#--------------------------------------------------------------------------
sub header {
   local($mo,$yr)=@_;

   print "Content-type: text/html


<HTML>
<HEAD>
<TITLE>WebPlan User List</TITLE>
</HEAD>
<BODY>
<CENTER><P ALIGN=\"CENTER\"> <IMG SRC=\"rtsban.jpg\"></P></CENTER>
";
} # header

#--------------------------------------------------------------------------
# Print the Footer
#--------------------------------------------------------------------------
sub footer {
   print <<"END";
END
   print "</BODY>";
   print "</HTML>";
} # footer

#--------------------------------------------------------------------------
# Main Function 
#--------------------------------------------------------------------------

   @PlanData=&obtain_user;

   &header;
   print "<CENTER>\n";
   &listUser;
   print "</CENTER>\n";
   &footer;

