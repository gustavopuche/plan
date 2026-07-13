/*
 * interface for the network plan daemon, included by the daemon and its
 * client programs. This is a separate file to allow the daemon to be used
 * with other programs too. At this time, the daemon knows about the "plan"
 * and "xmbase-grok" client programs (see client.type in netplan.h).
 *
 * The message protocol: F=file ID, R=row ID, S=success flag (f=error, t=ok),
 * [a-z \n]=verbatim characters, <...>=ascii string. All messages end with \n.
 * Verbatim \n is strings must be escaped with \.
 *
 * -          -> !SV <msg>	version banner after connect
 * t<type>    ->		set client type: 0=plan, 1=grok
 * m<dname>   -> mS		make directory (not used)
 * o<fname>   -> oSwF		open file, is writable
 *            -> oSrF		open file, is read-only
 * cF         ->		close file
 * rF R       -> rSF R <row>	read all (R=0) or one row. Multiple replies.
 * -          -> RSF R <row>	unsolicited row msg: somebody else changed it
 * wF R <row> -> wSR		write one row, R=0: create
 * dF R       ->		delete a row
 * lF R       -> lS		lock row for editing
 * uF R       ->		unlock row
 * L          ->		list of users
 * q          ->                close client connection explicitly
 * -          -> ?<msg>		server wants client to put up error popup
 *
 * To test the server, type "telnet localhost 2983" after starting the daemon.
 * PORT is used only if "/netplan/tcp" is not defined in /etc/services, and
 * HOMEDIR is overriden by the compiler option "-DDIR=...". Note: IANA has
 * assigned port 2983 in December 1999, but the old port 5444 is still
 * listened to.
 */

#ifndef LIB
#error Use -DLIB="/usr/local/lib" when compiling, see Makefile
#endif

#define DVERSION	"2.2"			/* daemon version number */
#define PORT		2983			/* default IANA IP port */
#define OLDPORT		5444			/* old IP port */
#define HOMEDIR		LIB			/* data file dir go here */
