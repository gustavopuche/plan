#
# Convert plan.help to troff -ms format.
#
# author : christian.wagner@univ-pau.fr
#          wagner@crisv2.univ-pau.fr
#
# Inspired from a perl script written
# by gregg hanna <gregor@kafka.saic.com>
#

BEGIN { 
  nofill = 0; indent = 0; MxSp = 6; outs = "" ;
  print ".lg"
  print ".TL\nPLAN - documentation"
  print "\n.br"
  print "\\fI Thomas Driemeyer\\fR\n.br"
  print "thomas@bitrot.de"
# to be updated for double-side printing
  print ".OH 'Plan - documentation'''"
  print ".OF '\\*(DY''%'"
  print ".EH 'Plan - documentation'''"
  print ".EF '\\*(DY''%'"
}

{
#- `#' as first charcter
  if ( index($1,"#") == 1 ) {
    if ( indent ) indent = 0
  }
#- `%' as first character
  else if ( index($1,"%") == 1 ) {
   if ( indent ) indent = 0
   outs = ".SH\n"
   getline
   old_fs = FS
   FS = "\t"
   outs = outs$2"\\fR\n.br"
   FS = old_fs
  }
#- `TAB' as first character
  else if ( index($0,"\t") == 1 ) {
    if ( index($0, " -- ") != 0 ) {
      if ( indent ) indent = 0
      old_fs = FS
      FS = " -- "
      if ( $1 == "\tDon't show today's past" )
        $1 = "\tDon\\'t show today\\'s past"
      outs = ".IP \"\\fI"$1"\\fR\"\n "$2
      FS = "\t "
      while ( $0 != "" ) {getline; outs = outs$2}
      FS = old_fs
    }
    else if ( index($0,"\t\*") == 1 ) {
      if ( indent ) indent = 0
      old_fs = FS
      FS = "\t\*"
      outs = "\n"$2
      FS = "\t    "
      getline
      while ( $0 != "" ) { outs = outs" "$2 ; getline }
      FS = old_fs
    }
    else if ( match($0,/^\t[0-9]/) == 1 ) {
      if ( indent ) indent = 0
      old_fs = FS
      FS = "\t"
      outs = $2
      getline
      while ( $0 != "" ) {
        str = substr($2,index($2," "))
        while ( index(str," ") == 1 ) str = substr(str, 2)
        outs = outs" "str
	getline
      }
      outs = outs"\n"
    }
    else if ( $0 == "\t" ) {
        outs = ".LP"
	if ( indent ) indent = 0
    }
    else if ( index($0,"\t ") == 1 ) {
        str = substr($0,index($0," "))
        pos = 0
        while ( (n = index(str," ")) == 1 ) {
	  pos++
	  str = substr(str,2)
        }
	if ( index(str,"\.") == 1 )
	  str = " "str
        if ( pos > MxSp ) {
          if ( nofill ) print str
          else { nofill = 1; print ".nf\n"str }
        }
        else if ( indent ) {
          outs = ".br\n"str
        }
        else { indent = 1; outs = ".IP \"\" "pos"\n"str }
    }
    #- other cases where `TAB' is the first character
    else {
      if ( indent ) indent = 0
      old_fs = FS
      FS = "\t"
      outs = $2
      FS = old_fs
    }
  }
#- end first character = `TAB'
  else if ( $0 == "" ) {
    outs = ".LP"
    if ( indent ) indent = 0
  }
  else {
    outs = $0
    if ( indent ) indent = 0
  }
  if ( nofill ) { nofill = 0; print ".fi" }
  if ( outs != "" ) { print outs; outs = "" }
}
