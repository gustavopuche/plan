/*
 * Create and destroy the network popup that defines the netplan daemon.
 * It is called from the config pulldown, and comes up automatically when
 * plan 1.5 is started for the first time.
 *
 *	detach_from_network()
 *	attach_to_network()
 *	name_to_file(name)
 *	server_entry_op(entry, op)
 *	read_from_server(uid, rid)
 *	puts_server(fd, msg)
 *	gets_server(fd, msg, num, cont)
 *	got_network_event(fd)
 *	get_netplan_filelist(fd)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#if defined(IBM) || defined (__EMX__)
#include <sys/select.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#ifdef NEWSOS4
#include <machine/endian.h>
#endif
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pwd.h>
#include <Xm/Xm.h>
#include "cal.h"
#include "netplan_if.h"

#define SBUFSZ		1024		/* send buffer size */
#define RBUFSZ		1024		/* receive buffer size */


extern char		*progname;	/* argv[0] */
extern struct config	config;		/* global configuration data */
extern struct user	*user;		/* user list (from file_r.c) */
extern int		nusers;		/* number of users in user list */
extern int		maxusers;	/* max # of users in user list */
extern struct plist	*mainlist;	/* list of all schedule entries */
extern Widget		mainwindow;	/* popup menus hang off main window */
static struct host	*host_list;	/* list of connected hosts */


/*
 * break all network connections. This is done by read_mainlist() before the
 * main .dayplan file that contains the user list is read. The user list is
 * also the list of servers from which appointments are read; after it is
 * known read_mainlist() can reconnect using attach_to_network().
 */

void detach_from_network(void)
{
	int		u;
	struct host	*hp, *next;

	for (hp=host_list; hp; hp=next) {
		next = hp->next;
		unregister_X_input(hp->x_id);
		close(hp->fd);
		free(hp->name);
		free(hp);
	}
	host_list = 0;
	for (u=0; u < nusers; u++) {
		user[u].host    = 0;
		user[u].file_id = -1;
	}
}


/*
 * socket connection fd broke (because the server died or something). Put
 * up an error popup and shut down the link to avoid an infinite loop.
 */

static void lost_connection_error(
	int		fd,		/* socket that died */
	int		error)		/* errno from read or write */
{
	struct host	*hp, *prev = 0;
	char		*host = 0;
	int             u;

	for (hp=host_list; hp; prev=hp, hp=hp->next)
		if (fd == hp->fd) {
			host = hp->name;
			if (prev)
				prev->next = hp->next;
			else
				host_list = hp->next;
			unregister_X_input(hp->x_id);
			for (u = 0; u < nusers; u++)
				if (user[u].host == hp)
					user[u].host = 0;
			close(hp->fd);
			free(hp);
			break;
		}
	if (mainwindow)
		create_error_popup(mainwindow, error,
_("Lost connection to netplan server %s.\n\n\
Please start it up again, then choose\n\
`File->Reread files' to reconnect and re-read.\n\
You may have lost recent changes."), host ? host : _("<unknown>"));
	if (host)
		free(host);
}


/*
 * attach to network. This is called by read_mainlist() after the main
 * .dayplan was read and the user list is known. Attach to anything that
 * plan isn't already attached to. Don't detach, that would release all
 * existing locks.
 */

char *attach_to_network(void)
{
	int		u;		/* user counter */
	struct host	*hp;		/* for checking already-known hosts */
	char		*uhost, *p;	/* requested host name without '/'s */
	int		fd;		/* temporary socket descriptor */
	char		msg[10240];	/* problem report */

	*msg = 0;
	for (u=0; u < nusers; u++) {
		if (strlen(msg) > sizeof(msg)-1024) {
			strcat(msg, _("Too many errors, aborting.\n"));
			break;
		}
		if (!user[u].fromserver || !user[u].name || !user[u].server)
			continue;
		uhost = user[u].server;
		if ((p = strrchr(uhost, '/')))		/* extract hostname */
			uhost = p+1;

		for (hp=host_list; hp; hp=hp->next)	/* already connected?*/
			if (!strcmp(hp->name, uhost))
				break;
		if (hp) {
			user[u].host = hp;
			continue;
		}
		fd = connect_netplan_server(uhost, user[u].name, msg);
		if (fd >= 0) {
			if (!(hp = (struct host *)malloc(sizeof(*hp))))
				fatal(_("no memory"));
			hp->name  = mystrdup(uhost);
			hp->fd    = fd;
			hp->open  = TRUE;
			hp->next  = host_list;
			hp->x_id  = register_X_input(fd);
			host_list = hp;
			user[u].host = hp;
		}
	}
	return(*msg ? mystrdup(msg) : 0);
}


/*
 * open a connection to a netplan server by host name. This is used by the
 * above function, and to obtain the list of files available on a server.
 * Return the socket fd, or -1 on failure.
 */

int connect_netplan_server(
	char		*uhost,		/* requested server host name */
	char		*uname,		/* user file name req'd from server */
	char		*msg)		/* for appending error message */
{
	struct hostent	*hent;		/* for hostname -> IP addr lookup */
	struct sockaddr_in addr;	/* IP address */
	int		on;		/* for setting socket options */
	int		fd;		/* socket descriptor */
	char		*p;		/* for identifying plan to server */
	char		buf[1024];	/* banner reply from server */

	if (!config.net_port) {
		struct servent *serv = getservbyname("netplan", "tcp");
		config.net_port = serv ? serv->s_port : htons(PORT);
	}
	if (!(hent = gethostbyname(uhost))) {	/* open connection */
		sprintf(msg+strlen(msg),
			_("User file %s on host %s: unknown host\n"),
			uname, uhost);
		return(-1);
	}
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		sprintf(msg+strlen(msg),
			_("User file %s on host %s: cannot open socket\n"),
			uname, uhost);
		return(-1);
	}
	addr.sin_family = AF_INET;
	addr.sin_port   = config.net_port;
	memcpy(&addr.sin_addr, hent->h_addr, sizeof(hent->h_length));
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		sprintf(msg+strlen(msg),
			_("User file %s on host %s: cannot connect\n"),
			uname, uhost);
		return(-1);
	}
	on = TRUE;
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
	on = TRUE;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));

	if (gets_server(fd, buf, sizeof(buf), TRUE, FALSE) < 0)
		sprintf(msg+strlen(msg),
			_("Server on host %s does not reply\n"), uhost);
	else if (*buf != '!')
		sprintf(msg+strlen(msg),
			_("Server on host %s: protocol error\n"), uhost);
#if 0
	else
		fprintf(stderr, _("plan: connected to host %s: %s"),
			uhost, buf+1);
#endif
	p = strrchr(progname, '/');
	sprintf(buf, "=%s<uid=%d,gid=%d,pid=%d>\n",
			p && p[1] ? p+1 : progname,
			getuid(), getgid(), getpid());
	puts_server(fd, buf);
	puts_server(fd, 0);
	return(fd);
}


/*
 * have a user name, need the file ID that connects us with this user.
 * Return -1 if there is no such connection. This is used when the user
 * moves an appointment into a different group in the edit menu.
 */

int name_to_file(
	char		*name)		/* name to locate */
{
	struct host	*hp;		/* hos scan pointer */

	for (hp=host_list; hp; hp=hp->next)
		if (!strcmp(hp->name, name))
			return(hp->fd);
	return(-1);
}


/*
 * check server reply whether it failed and whether it is the expected
 * reply. If not, pop up a meaningful error message and return FALSE.
 */

static BOOL check_server_reply(
	int		ret,		/* <0: gets_server failed */
	char		*msg,		/* message returned by server */
	char		cmd,		/* expected command code, 0=any */
	char		*host,		/* server host name */
	char		*fname)		/* file name or 0 */
{
	char		err[10240];	/* error message buffer */

	*err = 0;
	if (ret < 0) {
		sprintf(err, _("Server on host %s is unreachable."),
					host ? host : _("<unknown>"));
	} else if (cmd && *msg != cmd) {
		sprintf(err,
_("Server host \"%s\" protocol error:\n\
Expected reply code `%c' but server sent `%c'.\n\
Full reply message in error was:\n%s"),
			host, cmd, *msg, msg);
	}
	if (*err) {
		if (fname)
			sprintf(err+strlen(err),
				_("\n(Trying to access file \"%s\".)"), fname);
		strcat(err, "\n");
		create_error_popup(0, 0, "%s", err);
		return(FALSE);
	}
	return(TRUE);
}


/*
 * editing functions, called when the user edits a single appointment. When
 * editing starts, the entry is locked so nobody else can edit the same one,
 * and when editing finishes it is either deleted, updated and unlocked, or
 * just unlocked if nothing changed.
 */

static int  host_fd;
static BOOL write_to_net(line) char *line;
	{ puts_server(host_fd, line); return(TRUE); }

BOOL server_entry_op(
	struct entry	*entry,		/* entry to lock/unlock/delete */
	char		op)		/* l=lock, u=unlock, d=del, w=write */
{
	char		buf[128];	/* command to server, reply */
	BOOL		ret, r;		/* read from server succeeded? */
	int		u;		/* number of user that owns entry */
	struct host	*host;		/* for checking already-known hosts */
	char		*name;		/* file name on server */
	char            *hostname;      /* server's host name */

	u = name_to_user(entry->user);
	if ((!entry->id && op != 'w') || !user[u].fromserver ||
	    (entry->private && (op == 'l' || op == 'w')))
		return(TRUE);
	host = user[u].host;
	hostname = user[u].server;
	if (!user[u].host)
		return(FALSE);
	name = user[u].name;
	sprintf(buf, "%c%d %d%c", op, entry->file, entry->id,
				  op == 'w' ? ' ' : '\n');
	puts_server(host->fd, buf);
	if (!user[u].host)
		return(FALSE);
	switch(op) {
	  case 'l':
		puts_server(host->fd, 0);
		if (!user[u].host)
			return(FALSE);
		ret = gets_server(host->fd, buf, sizeof(buf), TRUE, FALSE);
		if (!check_server_reply(ret, buf, op, hostname, name))
			return(FALSE);
		return(buf[1] == 't');

	  case 'w':
		host_fd = host->fd;
		ret = write_one_entry(write_to_net, entry);
		if (!user[u].host)
			return(FALSE);
		puts_server(host->fd, 0);
		if (!user[u].host)
			return(FALSE);
		r = gets_server(host->fd, buf, sizeof(buf), TRUE, FALSE);
		if (!check_server_reply(r, buf, op, hostname, name))
			return(FALSE);
		if (buf[1] == 't')
			entry->id = atoi(buf+2);
		return(ret && buf[1] == 't');
	}
	puts_server(host->fd, 0);
	return(TRUE);
}


/*
 * read one or all entries from a server synchronously. <uid> is an index
 * into the user[] list, or -1 for own appointments. <rid> is the row ID
 * of the entry to be read, or 0 for "all entries". Return after all the
 * entries are read. See netplan_if.h for server protocol specifications.
 */

int open_and_count_rows_on_server(
	int		uid)		/* user[] index */
{
	char		buf[1024];	/* command to server, reply */
	char		*p;		/* for preparing raw reply lines */
	struct host	*host;		/* info about host and file */
	char		*uname;		/* file name to read */
	int		ret;		/* return code from gets_server */
	int		fid, *fidp;	/* file handle while open */

	host  = user[uid].host;
	uname = user[uid].name;
	fidp  = &user[uid].file_id;
	if (!host) /* connect has failed */
		return(-1);

	if ((fid = *fidp) < 0) {			/* need to open? */
		sprintf(buf, "o%s\n", uname);
		puts_server(host->fd, buf);
		puts_server(host->fd, 0);
		ret = gets_server(host->fd, buf, sizeof(buf), TRUE, FALSE);
		if (!check_server_reply(ret, buf, 'o', user[uid].server,uname))
			return(-1);
		if (buf[1] == 'f')			/* file not found */
			return(-1);
		if (uid >= 0)				/* read-only? */
			user[uid].readonly = buf[2] == 'r';
		else if (buf[2] == 'r')
			create_error_popup(0, 0,
				_("Your own appointment file is read-only!"));

		*fidp = fid = atoi(buf+3);		/* file ID */
	}
	sprintf(buf, "n%d\n", fid);			/* number of rows? */
	puts_server(host->fd, buf);
	puts_server(host->fd, 0);
	ret = gets_server(host->fd, buf, sizeof(buf), TRUE, FALSE);
	if (!check_server_reply(ret, buf, 'n', host->name, uname))
		return(-1);
	for (p=buf; *p && *p != ' '; p++);
	return(atoi(p));
}


void read_from_server(
	struct plist	**list,		/* list to add to */
	int		uid,		/* user[] index or -1 */
	int		rid)		/* entry ID, or 0 for all */
{
	char		buf[1024];	/* command to server, reply */
	struct host	*host;		/* info about host and file */
	char		*uname;		/* file name to read */
	int		ret;		/* return code from gets_server */
	int		num;		/* # of rows to read from server */

	host  = user[uid].host;
	uname = user[uid].name;
	if (!host) /* connect has failed */
		return;
	if ((num = open_and_count_rows_on_server(uid)) < 0)
		return;
	if (rid)					/* one row or all? */
		num = 1;

	sprintf(buf, "r%d %d\n",			/* request rows */
			user[uid].file_id, rid);
	puts_server(host->fd, buf);
	puts_server(host->fd, 0);

	while (num-- > 0) {				/* row loop */
		ret = gets_server(host->fd, buf, sizeof(buf), TRUE, FALSE);
		if (!check_server_reply(ret, buf, 'r', host->name, uname))
			return;
		buf[1] = '@'; /* flag for parse_file_line */
		parse_file_line(list, host->name, uname, buf+1);
		while (ret == 0) {
			ret = gets_server(host->fd, buf,sizeof(buf),TRUE,TRUE);
			if (!check_server_reply(ret, buf, 0, host->name,uname))
				return;
			parse_file_line(list, host->name, uname, buf);
		}
	}
}


/*
 * send server message. This function can be called multiple times for a
 * single message; the message is terminated by calling puts_server with
 * a null argument. All newlines except the last one are escaped because
 * messages always end with a newline. Always buffer messages, and send
 * the buffer when it fills up or when puts_server(0) is called. If the
 * buffer fills up strip the trailing newline because it's not known yet
 * whether it needs to be escaped or not.
 */

static BOOL flush_server(
	int		fd,		/* socket descriptor to send to */
	char		*msg,		/* message string to send */
	int		num)		/* number of bytes in *msg */
{
	int		n;

	while (num) {
		n = write(fd, msg, num);
		if (n <= 0) {
			lost_connection_error(fd, errno);
			return(FALSE);
		}
		num -= n;
		msg += n;
	}
	return(TRUE);
}

BOOL puts_server(
	int		fd,		/* socket descriptor to send to */
	char		*msg)		/* message string to send, 0-term */
{
	static char	buf[SBUFSZ+2];	/* message buffer plus space for \n */
	static char	*endbuf;	/* next free byte in buffer */
	static BOOL	saved_nl;	/* \n stripped off in last call? */
	int		n;

	if (!endbuf)
		endbuf = buf;
	if (!msg) {
		if (saved_nl)
			*endbuf++ = '\n';
		if (!flush_server(fd, buf, endbuf-buf))
			return(FALSE);
		endbuf = buf;
		saved_nl = FALSE;
	} else {
		n = strlen(msg);
		if (saved_nl) {
			*endbuf++ = '\\';
			*endbuf++ = '\n';
			saved_nl = FALSE;
		}
		if ((saved_nl = n && msg[n-1] == '\n'))
			msg[n-1] = 0;

		while (*msg) {
			if (endbuf == buf+SBUFSZ) {
				flush_server(fd, buf, endbuf-buf);
				endbuf = buf;
			}
			if (*msg == '\n')
				*endbuf++ = '\\';
			*endbuf++ = *msg++;
		}
	}
	return(TRUE);
}


/*
 * receive a message from a server, one line at a time. Since the standard
 * file reading routines in file_r.c can deal with multi-line entries there
 * is no need to deal with embedded newlines. Therefore, newlines escaped
 * with backslashes are accepted as newlines. Also, the file routines have
 * a line length limit of 1024, so that's ok here too. Return 1 if the line
 * ends with a non-escape newline, 0 if there will be a continuation line,
 * and -1 for errors.
 * This function also interprets asynchronous messages (except when reading
 * a continuation line): '?' for server error messages and 'R' for async
 * data sends.
 * Warning - there is only one buffer, not one per server, so there should
 * never be two communications at once.
 */

static BOOL gets_server_line(int, char *, int, BOOL);

BOOL gets_server(
	int		fd,		/* socket descriptor to send to */
	char		*msg,		/* buffer for incoming message */
	int		num,		/* size of msg buffer (paranoia...) */
	BOOL		wait,		/* need reply, not just '?' and 'R' */
	BOOL		cont)		/* expecting a continuation line? */
{
	BOOL		ret;		/* did read work? */
	struct host	*hp;		/* for finding host name */
	char		*host;		/* host name */
	char		*uname;		/* user name */
	int		fid, u, i;	/* file ID from 'R' line (for user#) */

	for (hp=host_list; hp; hp=hp->next)
		if (hp->fd == fd)
			break;
	host = hp ? hp->name : _("<unknown>");
	for (;;) {
		*msg = 0;
		ret = gets_server_line(fd, msg, num, wait);
		if (cont)
			return(ret);
		switch(*msg) {
		  case '?':
			i = strlen(msg);
			while (!ret && msg[i-1] == '\n') {
				ret = gets_server_line(fd, msg+i, num-i, wait);
				i += strlen(msg+i);
			}
			create_error_popup(mainwindow, 0,
				_("Server host %s reports a problem:\n\n%s"),
				host, msg+1);
			break;
		  case 'R':
			msg[1] = '@'; /* flag for parse_file_line */
			fid = atoi(msg+2);
			for (u=0; u < nusers; u++)
				if (user[u].file_id == fid)
					break;
			uname = u == nusers ? 0 : user[u].name;
			parse_file_line(&mainlist, host, uname, msg+1);
			while (ret == 0) {
				ret = gets_server_line(fd, msg, num, wait);
				if (!check_server_reply(ret,msg,0,host,uname))
					return(0);
				parse_file_line(&mainlist, host, uname, msg);
			}
			break;
		  default:
			return(ret);
		}
	}
}


static BOOL gets_server_line(
	int		fd,		/* socket descriptor to send to */
	char		*msg,		/* buffer for incoming message */
	int		num,		/* size of msg buffer (paranoia...) */
	BOOL		wait)		/* return 0 if no data, don't block */
{
	static char	buf[RBUFSZ];	/* receive buffer */
	static int	enq, deq;	/* # of valid bytes, # of read bytes */
	char		c = 0;		/* next char to store */
	BOOL		esc = FALSE;	/* last char was a backslash */
	BOOL		prev_esc = FALSE;

	while (num > 1) {				/* emergency brake...*/
		if (deq == enq) {			/* need more data? */
			if (!wait) {
				fd_set rdset;
				struct timeval timeout;
				timeout.tv_sec = timeout.tv_usec = 0;
				FD_ZERO(&rdset);
				FD_SET(fd, &rdset);
				if (!select(fd+1, &rdset, 0, 0, &timeout))
					return(0);
			}
			deq = 0;
			enq = read(fd, buf, RBUFSZ);
			if (enq <= 0) {
				lost_connection_error(fd, errno);
				return(-1);
			}
		}
		c = buf[deq++];
		prev_esc = esc;
		if (!(esc = c == '\\' && !esc)) {	/* skip backslash */
			*msg++ = c;
			num--;
		}
		if (c == '\n')				/* stop at newline */
			break;
	}
	*msg++ = 0;					/* null-terminate */
	return(c == '\n' && !prev_esc);			/* unescaped newline?*/
}


/*
 * read the file list from netplan server <fd>. Returns an allocated array of
 * pointers. The caller must free every pointer of this list, and the list
 * itself.
 */

BOOL get_netplan_filelist(
	char		*uhost,		/* server host to request from */
	char		***list,	/* returned list */
	int		*num)		/* returned # of entries in list */
{
	int		fd;		/* socket connection to server */
	char		buf[256];	/* returned line from server */
	int		nalloc = 0;	/* number of lines allocated */
	char		msg[10240];	/* problem report */
	int		i;

	*list = 0;
	*num  = 0;
	*msg  = 0;
	if ((fd = connect_netplan_server(uhost, "", msg)) < 0)
		return(FALSE);
	strcpy(buf, "L\n");
	if (!puts_server(fd, buf) || !puts_server(fd, 0)) {
		close(fd);
		return(FALSE);
	}
	for (;;) {
		if (!gets_server(fd, buf, sizeof(buf), TRUE, FALSE) ||
							buf[1] != 't') {
			close(fd);
			return(FALSE);
		}
		if (buf[2] < ' ') {
			close(fd);
			return(TRUE);
		}
		if (*num >= nalloc) {
			int n = (nalloc += 32) * sizeof(char **);
			*list = *list ? (char **)realloc(*list, n)
				      : (char **)malloc(n);
			if (!*list)
				fatal("no memory");
		}
		if (*buf && buf[i = strlen(buf)-1] == '\n')
			buf[i] = 0;
		(*list)[(*num)++] = mystrdup(buf+2);
	}
}


/*
 * read all users from a netplan server, and add them all to the user list.
 */

void obtain_netplan_users(
	char		*uhost)		/* server host to request from */
{
	char		**list;		/* returned file list */
	int		num;		/* returned # of entries in list */
	int		i, u;		/* user list counters */

	/* if num is 0, list is not allocated so just return also - mjj */
	if (!get_netplan_filelist(uhost, &list, &num) || num == 0)
		return;
	for (i=0; i < num; i++) {
		for (u=0; u < nusers; u++)
			if (!strcmp(user[u].name, list[i]))
				break;
		if (u < nusers)
			continue;
		if (nusers >= maxusers) {
			u = nusers + 10;
			user = user ? realloc(user, u * sizeof(struct user))
				    : malloc(u * sizeof(struct user));
			if (!user)
				fatal(_("no memory"));
			memset(user+maxusers, 0,
					(u-maxusers) * sizeof(struct user));
			maxusers = u;
		}
		user[nusers].name	= mystrdup(list[i]);
		user[nusers].server	= mystrdup(uhost);
		user[nusers].fromserver	= TRUE;
		user[nusers].file_id	= -1;
		user[nusers].alarm	= TRUE;
		nusers++;
	}
	for (i=0; i < num; i++)
		free(list[i]);
	free(list);
}
