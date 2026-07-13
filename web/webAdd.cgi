#!/usr/bin/perl
# author: Michel Bourget, hacked for plan 1.8 by thomas@bitrot.de
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
sub add {
   local($dorep);

   $start="00000000"; 
	if ( $in{mo_start} <=9 ) {
   		substr($start,1,1)=$in{mo_start};
	} else {
		substr($start,0,2)=$in{mo_start};
	}
	if ( $in{jr_start} <=9 ) {
		substr($start,3,1)=$in{jr_start};
	} else {
		substr($start,2,2)=$in{jr_start};
	}
	if ( $in{type} eq "Appointment" ) { 
		if ( $in{hr_start} < 1000 ) {
			substr($start,5,3)=$in{hr_start};
		} else {
			substr($start,4,4)=$in{hr_start};
		}
	}
#  -l option
   $len="0000";
	if ( $in{type} eq "Appointment" ) {
		if ( $in{hr_end} < 10  ) {
			substr($len,3,1)=$in{hr_end};
		} 
		else {
			if ( $in{hr_end} < 100  ) {
				substr($len,2,2)=$in{hr_end};
			} 
			else {
				if ( $in{hr_end} < 1000 ) {
					substr($len,1,3)=$in{hr_end};
				} 
				else {
					substr($len,0,4)=$in{hr_end};
				}
			}
		}
	}
#  -u option
$user=$in{group};
#  Don't ask why ... user contains a \0 at the end ...
#  Must be a bug in cgi-lib in readparse
   chop $user;

# Testing if user=none is entered 
   if ( $in{group} eq "none" ) {
 	print "User None specified: nothing added \!<br>\n";
 	return;
   }

#  -l or -L
   if ( $in{type} eq "Appointment" ) {
	$long="-l $len";
   } else {
	$long="-L";
   }

# -r, -d, -D repeat options (hacked 21.12.98 thomas@bitrot.de)

   $dorep=0;
   $rep=" ";
   if ( $in{mday} ne "No" ) {			# on a day of the month
	$dorep=1;
	$rep=$rep."-d $in{mday} ";
   }
   if ( $in{wday} ne "No" ) {			# on a weekday
	$dorep=1;
	$rep=$rep."-D $in{wday} ";
   }
   if ( $in{ndays} ne "No" ) {			# every n days
	$dorep=1;
	$rep=$rep."-r $in{ndays} ";
   }
   if ( $dorep == 1 ) {				# end date
   	$till="00000000"; 
	if ( $in{mo_end} <=9 ) {
   		substr($till,1,1)=$in{mo_end};
	} else {
		substr($till,0,2)=$in{mo_end};
	}
	if ( $in{jr_end} <=9 ) {
		substr($till,3,1)=$in{jr_end};
	} else {
		substr($till,2,2)=$in{jr_end};
	}
	$rep=$rep."-e $till";
#	Testing if till > start 
	$nstart = substr($start,0,4);
	$ntill  = substr($till,0,4);
	if ( $ntill <= $nstart ) {
		print "Error: Stop Date MUST be at least one day after Start Date \!<br>\n";
		return;
	}
   }

   $data="$in{descr}";

#  print "Not ready yet : below are debug messages<br>";
#  user add a \0 appended to it ... that's why data was not passed
#  to the dam script with system or open() statement

   $cmd="/usr/local/bin/plan $start -W $long $rep -u $user \"$data\"";
#  system("$cmd");
        open ( IN , "$cmd |");
        @error=<IN>;
        close(IN);
        foreach $line ( @error ) {
                print "$line\n";
        }
   print "Done\! ... Please <U>\"Get Calendar\"</U> to update your Scheduler View.";
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

   &header;
   &add;
   &footer;

