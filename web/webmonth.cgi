#! /usr/bin/perl

# use diagnostics;


#
# define days in month
#
@days=(0,31,28,31,30,31,30,31,31,30,31,30,31 );

@month=("", "January", "February", "March", "April", "May", "June",
         "July", "August", "September", "October", "November", "December" );

#
#  read html functions
#
require ("./cgi-lib.pl");
require ("./common.pl");


# Get Today's Date and remember it

@timelist = localtime(time());
$Today_Month=$timelist[4]+1;
$Today_Year =$timelist[5]+1900;
$Today_Day  =$timelist[3];

&ReadParse;

if ( $in{month} eq "" ) {
	$Month=$Today_Month;
} else {
	$Month=$in{month};
}
if ( $in{years} eq "" ) {
	$Year=$Today_Year;
} else {
	$Year=$in{years};
}

if ( $in{group} eq "" ) {
	$Group="none";
} else {
	$Group=$in{group};
}

if ( $in{jour} eq "" ) {
	$Jour=0;
} else {
	$Jour=$in{jour};
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


#
# read data from plan's output for the specified month and year
#
@PlanData=&get_plan_data($Jour,$Month,$Year,$Group);


@PlanHoliday=&get_holiday($Year);

#
# we now have the data to display, we need to calculate the height of the calendar
# in vertical, in order to have the same size in each days, and each
#


$height=0;
$nbline=0;
$date="1\.$Month\.$Year";

foreach $line (@PlanData) {
    # split plan line in parts
    @SplitLine=split(/\;/,$line);

    # Filter on user Selected : none == ALL
    # Can NOT ignore Entire Group entries == Team
    if ( $SplitLine[4] ne Team ) {
    	# Filter Private Appointment
    	if ( $show_private eq "no" && substr($SplitLine[5],0,1) eq "." ) { 
		next;
    	}
	
    	# Filter Hourly Appointment
    	if ( $show_appt eq "no" && $SplitLine[2] ne "-" ) { 
		next;
    	}
    }

    # the date have day,date, format
    @tmpdate=split(/,/,$SplitLine[1]);

    $tmpdate[1]=~s/[ ]*//;    

    if ($tmpdate[1] == $date) {
       $nbline++;
    } else {
       if ($nbline > $height) {
          $height=$nbline;
       }
       $date=$tmpdate[1];
       $nbline=1;
    }
}


print &PrintHeader;
print &HtmlTop;

if ( $Jour == 0 ) {
	&month ($Month, $Year,"webmonth.cgi");
	&add_entry(0);
}
else {
	&day ( $Jour, $Month, $Year ,"webmonth.cgi");
	&add_entry(1);
}

print &HtmlBot;
