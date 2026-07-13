#!/usr/bin/perl
#--------------------------------------------------------------------------

#--------------------------------------------------------------------------
require ("./cgi-lib.pl") || die "can\'t require cgi-lib.pl: $!";;
require ("./common.pl") || die "can\'t require common.pl: $!";;

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
sub delete {

   print "<FONT COLOR=\"#0000ff\">Attempt to delete ... </FONT>\n";

   if ( $Qui ne $Userid ) { 
	print "<FONT COLOR=\"#ff0000\">Can not delete: please use \"Get Calendar\" to re-sync</FONT>\n";
        return;
   }
   else {
   	open ( IN , "/usr/local/bin/plan -W -X $Row $Qui |" );
   	@error=<IN>;
   	close(IN);
	foreach $line ( @error ) { 
		print "$line\n";
	}
	print "Done. Please use \"Get Calendar\" to re-sync\n";
   }
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
   $Qui=$in{qui};
   $Userid=$in{userid};


   &header;
   &delete;
   &footer;

