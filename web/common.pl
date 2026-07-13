#--------------------------------------------------------------------------
#
# Author: Michel Bourget <michel@montreal.sgi.com>
# ported and slightly extended bu thomas@bitrot.de
#
#--------------------------------------------------------------------------
# This function returns the day of the week of the first day of the
# specified month. Rewritten 21.12.98 thomas@bitrot.de
#--------------------------------------------------------------------------
sub firstdom {
   local($mo, $yr)=@_;
   local($da,     # the day Jan 1 of this year is on
         $offset, # difference between last year and this year
         $i,      # loop index
         ); 

   @days=(0,31,28,31,30,31,30,31,31,30,31,30,31 );

   $da=(5+$yr+int(($yr+3)/4))%7;
   $days[2]=29 if ($yr%4 == 0);

   for ($i=1; $i<$mo; $i++) {
      $da+=$days[$i];
   }
   $da%7;
 
} # firstdom


#--------------------------------------------------------------------------
# This routine print Day label and day content for Month and Day view
#--------------------------------------------------------------------------
sub printday {
   local($day, $mo, $yr,$dayweek,$mode,$url)=@_;
   local($date,  # Date to examine
         $descr, # description of the event
         $flags, # how to sort out the various types of data
         $filter,# gets matched against flags
         $count, # number of events encountered so far
        );
    local($i,$PrtRows);
    
   $numsize=4;
   $datasize=2;
   print "<TD VALIGN=\"top\"><font size=$numsize>";
    
   if (!$day) {
      print "$day</font><br><br></TD>\n";
   } else {

      if ($mode eq short) {
      	if ( $height < 2 ) { $height = 2 ; } 
      	print "<A HREF=\"$url?jour=$day&month=$mo&years=$yr&group=$Group\" TARGET=\"calendar\">";
	$Today_Flag="";
        if ( $Today_Year == $yr && $Today_Month == $mo &&
		$Today_Day  == $day ) {
		$Today_Flag="<FONT COLOR=\"#ff0000\">";
	}
   	$holiday_text="";
   	foreach $line ( @PlanHoliday ) {
		@tmp=split(/;/,$line);
		@hol=split(/\./,$tmp[1]);
		if ( $hol[0] eq $day && $hol[1] eq $mo ) {
			$holiday_text=$tmp[2];
			last;
		}
   	}

      	print "$Today_Flag$day</A></font> <font COLOR =\"#ff0000\" size=1>$holiday_text</font><br>\n<font size=$datasize>";
      }
      else {
      	print "</font><br>\n<font size=$numsize>";
      }

      $PrtRows=0;
    

      foreach $line (@PlanData) {
      	if ( $line =~ /, $day\./ ) {
           @printpart= split(/;/,$line);
	   # Filter User you want to see : Team never ignored
	   if ( $Group eq "none" || $Group =~ $printpart[4] || $printpart[4] =~ "Team" ) {
		if ( "$printpart[4]" ne "Team" ) {
			# Filter Private Appt
			if ( $show_private eq "no" && substr($printpart[5],0,1) eq "." ) {
				next;
			}
			# Filter Appointment
			if ( $show_appt eq "no" && $printpart[2] ne "-" ) {
				next;
			}
		}
		if ( $mode eq short ) {
	   		$qui=substr($printpart[4],0,4);
	   		$note=substr($printpart[5],0,10);
			print "<A HREF=\"webmod.cgi?row=$printpart[0]&month=$mo&year=$yr\" TARGET=\"user-info\">";
	   		print "$qui ";
			print "</A>";
			print "$note</font><br>\n";
			print "<font size=$datasize>";
		}
		else {
			$id=$printpart[0];
			$debut=$printpart[2];
			$fin=$printpart[3];
	   		$qui=$printpart[4];
	   		$note=$printpart[5];
			print "<A HREF=\"webmod.cgi?row=$id&month=$mo&year=$yr\" TARGET=\"user-info\">";
			print "<CENTER>Del</CENTER></A></TD><TD>";
#			print "$id</TD><TD>";
			print "<CENTER>$debut</CENTER></TD><TD>";
			print "<CENTER>$fin</CENTER></TD><TD>";
			print "&nbsp $qui</TD><TD>";
			print "&nbsp $note</TD><TR><TD>";
	   		# print "$id\t$debut\t$fin\t$qui\t$note</font><br>\n";
			print "<font size=$numsize>";
		}
           	$PrtRows++;
	   }
         }
      }
   

      if ( $mode eq short ) {
		#for ($i=$PrtRows;$i<=$height;$i++) {
				#print ("<BR>\n");
		#}
		print ("<BR>\n");
      }
  
   }
} # printday


#--------------------------------------------------------------------------
# Print out the month's calendar
#--------------------------------------------------------------------------
sub month {
   local($mo,$yr,$url)=@_;
   local($da,$i);

   $da=&firstdom($mo,$yr);

   $pmo=$mo-1;
   $pyr=$yr;
   if ( $pmo < 1 ) {
	$pmo=12;
	$pyr=$yr-1;
   }
   $nmo=$mo+1;
   $nyr=$yr;
   if ( $nmo > 12 ) {
	$nmo=1;
	$nyr=$yr+1;
   }
   $Ppyr=$yr-1;
   $Pnyr=$yr+1;

   print "
<CENTER>
<TABLE BORDER=2  CELLPADDING=0 CELLSPACING=0>

<!-- print calendar title -->
<TR>
<TH COLSPAN=7>
<TABLE border=0 CELLPADDING=0 CELLSPACING=0>
	<TR>
		<TH WIDTH=65  align=Left><A HREF=\"$url?group=$Group&jour=0&month=$mo&years=$Ppyr&show_appt=$show_appt&show_private=$show_private\">$Ppyr</A></TH>
		<TH WIDTH=650 align=center><A HREF=\"$url?group=$Group&jour=0&month=$pmo&years=$pyr&show_appt=$show_appt&show_private=$show_private\">$month[$pmo]</A> $month[$Month] $Year <A HREF=\"$url?group=$Group&jour=0&month=$nmo&years=$nyr&show_appt=$show_appt&show_private=$show_private\">$month[$nmo]</A></TH>
		<TH WIDTH=65  align=right><A HREF=\"$url?group=$Group&jour=0&month=$mo&years=$Pnyr&show_appt=$show_appt&show_private=$show_private\">$Pnyr</A></TH>
	</TR>
</TABLE>
</TH>
</TR>

<!-- print day's header -->
<TR>
<TH WIDTH=65>Sunday</TH>
<TH WIDTH=130>  Monday   </TH>
<TH WIDTH=130>  Tuesday  </TH>
<TH WIDTH=130>  Wednesday</TH>
<TH WIDTH=130>  Thursday </TH>
<TH WIDTH=130>  Friday   </TH>
<TH WIDTH=65>  Saturday</TH>
</TR>
";

   print "<TR>\n";
   

   # Get us spaced over to the appropriate day of the week
   for ($i=0; $i<$da; $i++) {
       &printday("",$mo,$yr);
   }

   # Print out the actual calendar
   for ($i=1; $i <= $days[$mo]; $i++ ) {
       if (($i==3) && ($days[$mo]==19)) { # attend to the fruitbasket
          $i+=11;
          $days[$mo]+=11;
       }
       &printday($i,$mo,$yr,$da,"short","$url");
       $da++;
       if ($da > 6) {
          $da=0;
          print "</TR>\n<TR>\n" if ($i<$days[$mo]);
       }
   }

   # Fill out the rest of the calendar
   while (($da>0)&&($da<7)) {
      $da++;
      &printday("",$mo,$yr);
   }

   print "\n</TR>\n</TABLE>\n"; # wrap it all up
   print "</CENTER>";
} # month

#--------------------------------------------------------------------------
# Print out the day's calendar
#--------------------------------------------------------------------------
sub day {
   local($jour,$mo,$yr,$url)=@_;
   local($da,$i);

   @DayOfTheWeek = ( "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday" );

   $da=&firstdom($mo,$yr);

   $pjour=$jour-1;
   $pmo=$mo;
   $pyr=$yr;
   if ( $pjour < 1 ) {
	$pmo=$mo-1;
	if ( $pmo < 1 ) {
		$pmo=12;
		$pyr=$yr-1;
	}
	$pjour=$days[$pmo];
   }
   $njour=$jour+1;
   $nmo=$mo;
   $nyr=$yr;
   if ( $njour > $days[$mo] ) {
	$nmo=$mo+1;
	if ( $nmo > 12 ) {
		$nmo=1;
		$nyr=$yr+1;
	}
	$njour=1;
   }

   $holiday_text="";
   foreach $line ( @PlanHoliday ) {
	@tmp=split(/;/,$line);
	@hol=split(/\./,$tmp[1]);
	if ( $hol[0] eq $jour && $hol[1] eq $mo ) {
		$holiday_text=$tmp[2];
		last;
	}
   }

   print "
<CENTER>
<TABLE BORDER=2  CELLPADDING=0 CELLSPACING=0>

<!-- print calendar title -->
<TR>
<TH COLSPAN=5>
<TABLE border=0 CELLPADDING=0 CELLSPACING=0>
<TR>
	<TH width=80><A HREF=\"$url?group=$Group&jour=$pjour&month=$pmo&years=$pyr&show_appt=$show_appt&show_private=$show_private\">Previous</A></TH>
	<TH width=620>$DayOfTheWeek[($da+$jour-1)%7] $month[$mo] $jour $yr (<FONT COLOR=\"#ff0000\">$holiday_text</FONT>)</TH>
	<TH width=80><A HREF=\"$url?group=$Group&jour=$njour&month=$nmo&years=$nyr&show_appt=$show_appt&show_private=$show_private\">Next</A></TH>
</TR>
</TABLE>
</TH>
</TR>
<TH WIDTH=60>&nbsp</TH>
<TH WIDTH=80>From</TH>
<TH WIDTH=80>Length</TH>
<TH WIDTH=80>Who?</TH>
<TH WIDTH=480>Description</TH>
</TR>
";

   print "<TR>\n";

   # Print out the actual calendar
   
   &printday($jour,$mo,$yr,$da,"long","$url");


   print "\n</TR>\n</TABLE>\n"; # wrap it all up
   print "</CENTER>";

} # day


1;

#--------------------------------------------------------------------------
# Get the list of user on netplan server
#--------------------------------------------------------------------------
sub get_group {
	open(IN,"/usr/local/bin/plan -W localhost -F | sort |");
	@PlanUsers=<IN>;
	close(IN);

	return @PlanUsers;
}

#--------------------------------------------------------------------------
# read data from plan's output for the specified month and year
#--------------------------------------------------------------------------
sub get_plan_data {
	local($jj,$mo,$yr,$who)=@_;
	local($Who,@PlanData);

        if ( $who eq "none" ) {
		$Who="-o";
	} else {
		$Who="-u $who,Team";
	}
	
	$da=&firstdom($mo,$yr);

        if ( $jj eq 0 ) {
		open ( IN , "/usr/local/bin/plan -W $Who -i -t 1\.$mo\.$yr $days[$mo] |" );
	} else {
		open ( IN , "/usr/local/bin/plan -W $Who -i -t $jj\.$mo\.$yr 1 |" );
	}
	@PlanData=<IN>;
	close(IN);

	return @PlanData;
}
#--------------------------------------------------------------------------
# Get User data in user:serverName
#--------------------------------------------------------------------------
sub obtain_user {
   local(@PlanData);
   open ( IN , "/usr/local/bin/plan -W -F | sort | " );
   @PlanData=<IN>;
   close(IN);

   return @PlanData;
} # obtain_user

#--------------------------------------------------------------------------
# Print User's List
#--------------------------------------------------------------------------
sub listUser {
   print "<PRE>\n";
   print "<B><U>List of Users and their NetPlan server</B></U>:<br><br>\n";
   foreach $line ( @PlanData ) {
	   @info=split(/;/,$line);
	   chop $info[1];
	   print "$info[0]\t\t$info[1]\n";
   }
   print "</PRE>\n";
   print "<hr>";
} # listUser

#--------------------------------------------------------------------------
# read holiday data
#--------------------------------------------------------------------------
sub get_holiday {
	local ($yr) = @_;

	open ( IN , "/usr/local/bin/plan -W -H $yr |" );
	@PlanHoliday=<IN>;
	close(IN);

	return @PlanHoliday;
}
#--------------------------------------------------------------------------
# Get User's List according to who.txt DB
#--------------------------------------------------------------------------

sub getUserList {
        $first=1;
        $userList="";

        open ( DB , "who.txt" ) || die "can\'t open who.txt: $!";;
        while(<DB>) {
                next if /^#/ ;
                chop ;
                ($region,$fonction,$cie,$nickname,$firstName,$lastName,$expertise)=split(":",$_);
                if( $What ne Team ) {
                        next if ( $region ne $What ) ;
                }
                if( $Who ne Team ) {
                        if( $Who eq Cray ) {
                                next if ( $cie ne Cray ) ;
                        } elsif ( $Who eq Mgr ) {
                                next if ( $fonction ne Mgr );
                        } elsif ( $Who eq Ec ) {
                                next if ( $fonction ne Ec );
                        } else {
                                next if ( index("$expertise","$Who") eq -1 );
                        }
                }
                #print "$region $fonction $cie $nickname $firstName $lastName $expertise\n";
                if(!$first) {
                        $userList="$userList,";
                }
                $userList="$userList$nickname";
                $first=0;

        }
        close(DB);
        return $userList;
}


#--------------------------------------------------------------------------
# Prepare a form to add a new entry
#--------------------------------------------------------------------------
sub add_entry {
   local ($goCal) = @_;

   print "<br>";
   print "<CENTER>";
   print "<TABLE border=0 CELLPADDING=0 CELLSPACING=0>";
   print "<TR><TH nowrap>";
   print "<FORM method=post action=webAdd.cgi TARGET=\"user-info\">\n";


   @group_list=&get_group;
   
   print "<FONT size=2.0>";

#  User

   print "User: ";
   print "<SELECT NAME=\"group\">\n";
   print "<OPTION SELECTED VALUE=\"none\"> (None) \n"; 
   foreach (@group_list) {
       $group_elem=$_; 
       print "<OPTION ";
       print "VALUE=\"$group_elem\">", $group_elem, "\n";
   }
   print "</SELECT>\n";
#  print "<br>";

#  Start Date

   print "Start Date: ";

#  print "<SELECT NAME=\"yr_start\">\n";
#  for ($i=1998;$i<=2034;$i++) {
#      print "<OPTION ";
#      print "SELECTED " if ($i == $cur_year );
#      print "VALUE=\"$i\">", $i, "\n";
#  }
#  print "</SELECT>\n";

   print "<SELECT NAME=\"mo_start\">\n";
   for ($i=1;$i<=12;$i++) {
       print "<OPTION ";
       print "SELECTED " if ($i == $Month );
       print "VALUE=\"$i\">", $month[$i], "\n";
   }
   print "</SELECT>\n";

   print "<SELECT NAME=\"jr_start\">\n";
   for ($i=1;$i<=31;$i++) {
       print "<OPTION ";
       print "SELECTED " if ($i == $Today_Day && $Month == $Today_Month );
       print "VALUE=\"$i\">", $i, "\n";
   }
   print "</SELECT>\n";
#  print "<br>";

#  Type

   print "Type: ";
   print "<SELECT NAME=\"type\">\n";
   print "<OPTION VALUE=\"Appointment\">Appointment\n";
   print "<OPTION SELECTED VALUE=\"DayEvent\">All-day Event\n";
   print "</SELECT>\n";
#  print "DayEvent do not require Hour:Min Entry  ";

   print "</table><p><table border=0>\n";
   print "<tr><td align=right>If Appointment, enter Time:";
   print "<td><SELECT NAME=\"hr_start\">\n";
   for($i=0;$i<=23;$i++) {
	print "<OPTION VALUE=\"$i*100\">", $i, ":00\n";
	print "<OPTION VALUE=\"$i*100+30\">", $i, ":30\n";
   }
   print "</SELECT>\n";

   print "Length: ";
   print "<SELECT NAME=\"hr_end\">\n";
   for($i=0;$i<=23;$i++) {
	print "<OPTION VALUE=\"0\">", "No\n" if ( $i == 0 );
	print "<OPTION VALUE=\"$i*100\">", $i, ":00\n" unless ( $i == 0 );
	print "<OPTION VALUE=\"$i*100+30\">", $i, ":30\n";
   }
   print "</SELECT>\n";

#  Repeat Option

   print "<tr><td align=right>Repeat on a day of the month:";
   print "<td><SELECT NAME=\"mday\">\n";
   print "<OPTION SELECTED VALUE=\"No\">No\n";
   print "<OPTION VALUE=\"1\">On 1st\n";
   print "<OPTION VALUE=\"2\">On 2nd\n";
   print "<OPTION VALUE=\"3\">On 3rd\n";
   print "<OPTION VALUE=\"4\">On 4th\n";
   print "<OPTION VALUE=\"5\">On 5th\n";
   print "<OPTION VALUE=\"6\">On 6th\n";
   print "<OPTION VALUE=\"7\">On 7th\n";
   print "<OPTION VALUE=\"8\">On 8th\n";
   print "<OPTION VALUE=\"9\">On 9th\n";
   print "<OPTION VALUE=\"10\">On 10th\n";
   print "<OPTION VALUE=\"11\">On 11th\n";
   print "<OPTION VALUE=\"12\">On 12th\n";
   print "<OPTION VALUE=\"13\">On 13th\n";
   print "<OPTION VALUE=\"14\">On 14th\n";
   print "<OPTION VALUE=\"15\">On 15th\n";
   print "<OPTION VALUE=\"16\">On 16th\n";
   print "<OPTION VALUE=\"17\">On 17th\n";
   print "<OPTION VALUE=\"18\">On 18th\n";
   print "<OPTION VALUE=\"19\">On 19th\n";
   print "<OPTION VALUE=\"20\">On 20th\n";
   print "<OPTION VALUE=\"21\">On 21st\n";
   print "<OPTION VALUE=\"22\">On 22nd\n";
   print "<OPTION VALUE=\"23\">On 23rd\n";
   print "<OPTION VALUE=\"24\">On 24th\n";
   print "<OPTION VALUE=\"25\">On 25th\n";
   print "<OPTION VALUE=\"26\">On 26th\n";
   print "<OPTION VALUE=\"27\">On 27th\n";
   print "<OPTION VALUE=\"28\">On 28th\n";
   print "<OPTION VALUE=\"29\">On 29th\n";
   print "<OPTION VALUE=\"30\">On 30th\n";
   print "<OPTION VALUE=\"31\">On 31st\n";
   print "<OPTION VALUE=\"0\">On Last\n";
   print "</SELECT>\n";

   print "<tr><td align=right>Repeat on a weekday: ";
   print "<td><SELECT NAME=\"wday\">\n";
   print "<OPTION SELECTED VALUE=\"No\">No\n";
   print "<OPTION VALUE=\"0\">On Monday\n";
   print "<OPTION VALUE=\"1\">On Tuesday\n";
   print "<OPTION VALUE=\"2\">On Wednesday\n";
   print "<OPTION VALUE=\"3\">On Thursday\n";
   print "<OPTION VALUE=\"4\">On Friday\n";
   print "<OPTION VALUE=\"5\">On Saturday\n";
   print "<OPTION VALUE=\"6\">On Sunday\n";
   print "</SELECT>\n";

   print "<tr><td align=right>Repeat every n days: ";
   print "<td><SELECT NAME=\"ndays\">\n";
   print "<OPTION SELECTED VALUE=\"No\">No\n";
   print "<OPTION VALUE=\"1\">Every day\n";
   print "<OPTION VALUE=\"2\">Every 2 days\n";
   print "<OPTION VALUE=\"3\">Every 3 days\n";
   print "<OPTION VALUE=\"4\">Every 4 days\n";
   print "<OPTION VALUE=\"5\">Every 5 days\n";
   print "<OPTION VALUE=\"6\">Every 6 days\n";
   print "<OPTION VALUE=\"7\">Every 7 days\n";
   print "<OPTION VALUE=\"14\">Every 2 weeks\n";
   print "<OPTION VALUE=\"21\">Every 3 weeks\n";
   print "<OPTION VALUE=\"28\">Every 4 weeks\n";
   print "<OPTION VALUE=\"35\">Every 5 weeks\n";
   print "<OPTION VALUE=\"42\">Every 6 weeks\n";
   print "<OPTION VALUE=\"49\">Every 7 weeks\n";
   print "<OPTION VALUE=\"56\">Every 8 weeks\n";
   print "</SELECT>\n";

   print "<tr><td align=right>If Repeat is set, enter Stop Date: ";

#  print "<SELECT NAME=\"yr_end\">\n";
#  for ($i=1998;$i<=2034;$i++) {
#      print "<OPTION ";
#  print "</SELECT>\n";
#
#  print "If Repeat is set, enter Stop Date: ";
#
#  print "<SELECT NAME=\"yr_end\">\n";
#  for ($i=1998;$i<=2034;$i++) {
#      print "<OPTION ";
#      print "SELECTED " if ($i == $cur_year );
#      print "VALUE=\"$i\">", $i, "\n";
#  }
#  print "</SELECT>\n";

   print "<td><SELECT NAME=\"mo_end\">\n";
   for ($i=1;$i<=12;$i++) {
       print "<OPTION ";
       print "SELECTED " if ($i == $cur_month );
       print "VALUE=\"$i\">", $month[$i], "\n";
   }
   print "</SELECT>\n";

   print "<SELECT NAME=\"jr_end\">\n";
   for ($i=1;$i<=31;$i++) {
       print "<OPTION VALUE=\"$i\">", $i, "\n";
   }
   print "</SELECT>\n";
   print "</table><p>";

#  Text

   print "<table>";
   if ( $goCal eq 1 ) {
   	print "<TD><PRE>";
   	open(CAL,"/usr/bin/cal $Month $Year |");
   	while(<CAL>) {
		chop;
		print "$_<br>";
   	}
   	close(CAL);
   	print "</PRE></TD>"
   }
   print "</TR><TR><TD COLSPAN=2><CENTER>";

   print "Note: <INPUT TYPE=TEXT NAME=\"descr\" SIZE=50>";

   print "<p><INPUT TYPE=SUBMIT VALUE=\"Add it\"> \n<CENTER>" ;

   print "</font>";
   print "</form>\n" ;

   print "</TD></TR></TABLE></CENTER>";

} # form
