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
<TITLE>WebPlan User Administration</TITLE>
</HEAD>
<BODY>
<CENTER><P ALIGN=\"CENTER\"> <IMG SRC=\"rtsban.jpg\"></P></CENTER>
";
} # header

#--------------------------------------------------------------------------
# Print the content of the FORM
#--------------------------------------------------------------------------
sub form {
   
   print "<FORM method=post action=admin.cgi>\n";
   print "User: <INPUT TYPE=TEXT NAME=\"user\" SIZE=\"12\" \n>";
   print "Server: <INPUT TYPE=TEXT NAME=\"server\" SIZE=\"36\" \n>";
   print "<INPUT TYPE=SUBMIT NAME=action VALUE=\"Add\"> \n" ;
   print "<INPUT TYPE=SUBMIT NAME=action VALUE=\"Delete\"> \n" ;
   print "</FORM>\n" ;
   print "<br>\n";
   print "<B><U>Instructions</B></U><br>\n";
   print "<U>Add</U>    require to enter User AND server name (please include domain name)<br>\n";
   print ". If user \"Team\" do not exist please create it. This one is a general-purpose user for everyone.<br>\n";
   print "<U>Delete</U> require to enter only the username.<br>\n";
   print ". User is removed from the list<br>\n";
   print ". If the server  = WebServer : user data is <B><font color=\"#ff0000\">deleted</font></B><br>\n";
   print ". Else                       : data is <U>not</U> deleted on remote server<br>\n";

} # form

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

   &ReadParse;

   # print "User=$in{user} Server=$in{server} Action=$in{action}<br>\n";
   $err="";
   if ( ! ($in{action} eq "" || $in{user} eq "" ) )  {
	print "<hr>";
	if ( $in{action} eq Add ) {
		if ( $in{server} eq "" ) {
			$err="Please specify server when adding a user\n";
		}
		else {
			$dup=0;
			foreach $line ( @PlanData ) {
				@info=split(/;/,$line);
				if ( $info[0] eq $in{user} ) {
					$dup=1;
					last;
				}
			}
			if ( $dup == 1 ) {
				$err="User $in{user} already exist \!\!\n";
			}
			else {
				system("touch /usr/local/lib/netplan.dir/$in{user}");
				system("echo  $in{server} > /usr/local/lib/netplan.dir/.$in{user}");
			}
		}
	}
	elsif ( $in{action} eq Delete ) {
		$found=0;
		foreach $line ( @PlanData ) {
			@info=split(/;/,$line);
			if ( $info[0] eq $in{user} ) {
				$found=1;
				last;
			}
		}
		if ( $found == 1 ) {
			unlink "/usr/local/lib/netplan.dir/.$in{user}";
			unlink "/usr/local/lib/netplan.dir/$in{user}";
		} else {
			$err="User $in{user} does not exist \!\n";
		}
	}
	else {
		#err="Invalid action \"$in{action}\". Please report to WebPlan administrator.";
	}
	@PlanData=&obtain_user;
   }

   &listUser;
   &form;
   print "<FONT COLOR=\"#ff0000\">$err</FONT><br>";
   &footer;

