/* cursproc.c -- curses routines for hostedit

   Copyright 2008 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include <sys/types.h>
#include <curses.h>
#include <ctype.h>
#include <setjmp.h>
#include <pwd.h>
#include "defaults.h"
#include "incl_unix.h"
#include "networkincl.h"
#include "remote.h"
#include "hostedit.h"

#define	HOST_P		0
#define	ALIAS_P		20
#define	IPADDR_P	35
#define	PROBE_P		51
#define	MANUAL_P	57
#define	TRUSTED_P	65
#define	TO_P		77

#define	TYPE_P		51
#define	USER_P		60
#define	PWCHK_P		70

#define	UUSER_P		0
#define	WUSER_P		12
#define	DMCH_P		(IPADDR_P + 16)

#define	HDRLINES	5

jmp_buf	escj;

int	ask(const int row, const char *quest, const int dflt)
{
	move(row, 0);
	clrtoeol();
	printw("%s? [%c] ", quest, dflt? 'Y': 'N');
	refresh();
	for  (;;)  {
		switch  (getch())  {
		default:
			printw(dflt? "Yes": "No");
			refresh();
			return  dflt;
		case  'Y':case  'y':
			printw("Yes");
			refresh();
			return  1;
		case  'N':case  'n':
			printw("No");
			refresh();
			return  0;
		case  '[' & 0x1F:
			longjmp(escj, 1);
		}
	}
}

unsigned	askn(const int row, const char *msg)
{
	int		ch, starta;
	unsigned	result;

	mvprintw(row, 0, "%s: ", msg);
	getyx(stdscr, ch, starta);
	refresh();

	result = 0;

	for  (;;)  {

		switch  ((ch = getch()))  {
		default:
			continue;

		case  '[' & 0x1F:
			longjmp(escj, 1);

		case '0':case '1':case '2':case '3':case '4':
		case '5':case '6':case '7':case '8':case '9':

			result = result * 10 + ch - '0';
		redisp:
			mvprintw(row, starta, "%6u", result);
			refresh();
			continue;

#ifdef	KEY_LEFT
		case  KEY_LEFT:
#endif
#ifdef	KEY_DC
		case  KEY_DC:
#endif
#ifdef	KEY_BACKSPACE
		case  KEY_BACKSPACE:
#endif
		case  'h' & 0x1F:
			result /= 10;
			goto  redisp;

#ifdef	KEY_ENTER
		case  KEY_ENTER:
#endif
		case  '\n':
		case  '\r':
			return  result;
		}
	}
}

void	asktimeout(struct remote *rp, const int row)
{
	if  (ask(row, "Default timeout value OK", 1))
		rp->ht_timeout = NETTICKLE;
	else
		rp->ht_timeout = askn(row+2, "Timeout value");
}

enum IPatype  askhname(char *hnp, const int row, const char *htype, struct remote *rp)
{
	int	ch, starta, cnt;

	mvprintw(row, 0, "%s: ", htype);
	getyx(stdscr, ch, starta);
	refresh();

	cnt = 0;

	for  (;;)  {

		switch  ((ch = getch()))  {
		default:
			continue;

		case  '[' & 0x1F:
			longjmp(escj, 1);

		case  '.':case '-':case '_':
		case '0':case '1':case '2':case '3':case '4':
		case '5':case '6':case '7':case '8':case '9':
		case 'a':case 'b':case 'c':case 'd':case 'e':
		case 'f':case 'g':case 'h':case 'i':case 'j':
		case 'k':case 'l':case 'm':case 'n':case 'o':
		case 'p':case 'q':case 'r':case 's':case 't':
		case 'u':case 'v':case 'w':case 'x':case 'y':
		case 'z':
		case 'A':case 'B':case 'C':case 'D':case 'E':
		case 'F':case 'G':case 'H':case 'I':case 'J':
		case 'K':case 'L':case 'M':case 'N':case 'O':
		case 'P':case 'Q':case 'R':case 'S':case 'T':
		case 'U':case 'V':case 'W':case 'X':case 'Y':
		case 'Z':
			if  (cnt >= HOSTNSIZE)
				continue;
			hnp[cnt++] = ch;
			addch(ch);
			refresh();
			continue;

#ifdef	KEY_LEFT
		case  KEY_LEFT:
#endif
#ifdef	KEY_DC
		case  KEY_DC:
#endif
#ifdef	KEY_BACKSPACE
		case  KEY_BACKSPACE:
#endif
		case  'h' & 0x1F:
			if  (cnt > 0)  {
				cnt--;
				hnp[cnt] = '\0';
				mvaddch(row, starta+cnt, ' ');
				move(row, starta+cnt);
				refresh();
			}
			continue;

#ifdef	KEY_ENTER
		case  KEY_ENTER:
#endif
		case  '\n':
		case  '\r':
			if  (cnt <= 0)  {
				move(row-1, 0);
				clrtoeol();
				standout();
				addstr("You must give some host name");
				break;
			}
			hnp[cnt] = '\0';
			if  (rp  &&  isdigit(hnp[0]))  {
				netid_t	resip;
#ifdef	DGAVIION
				struct	in_addr	ina_str;
				ina_str = inet_addr(hnp);
				resip = ina_str.s_addr;
#else
				resip = inet_addr(hnp);
#endif
				if  (resip != -1L)  {
					if  (ipclashes(resip))  {
						move(row-1, 0);
						clrtoeol();
						standout();
						printw("%s clashes with existing IP addr", hnp);
						break;
					}
					rp->hostid = resip;
					rp->ht_flags |= HT_HOSTISIP;
					return  IPADDR_IP;
				}
				move(row-1, 0);
				clrtoeol();
				standout();
				printw("%s is not a valid IP addr", hnp);
				break;
			}
			else  {
				struct	hostent	*hp;
				if  (hnameclashes(hnp))  {
					move(row-1, 0);
					clrtoeol();
					standout();
					printw("%s clashes with existing name", hnp);
					break;
				}
				hp = gethostbyname(hnp);
				if  (hp)  {
					char	*cp;
					if  ((cp = ipclashes(*(netid_t *) hp->h_addr)))  {
						move(row-1, 0);
						clrtoeol();
						standout();
						printw("IP addr for %s clashes with existing for %s", hnp, cp);
						break;
					}
					if  (rp)
						rp->hostid = *(netid_t *) hp->h_addr;
					return  IPADDR_NAME;
				}
				move(row-1, 0);
				clrtoeol();
				standout();
				printw("%s is not a valid host name", hnp);
				break;
			}
		}
		standend();
		move(row, starta+cnt);
		refresh();
	}
}

/* Ask alias name, last arg true means treat as host name for name given as IP */
void	askalias(struct	remote *rp, const int row, const int musthave)
{
	int	ch, starta, cnt;
	char	*hnp;

	if  (musthave)  {
		hnp = rp->hostname;
		mvprintw(row, 0, "Please give a name for this machine: ");
	}
	else  {
		hnp = rp->alias;
		mvprintw(row, 0, "Any alias for %s%s: ", rp->hostname, strlen(rp->hostname) > 8? " (probably a good idea)": "");
	}

	getyx(stdscr, ch, starta);
	refresh();

	cnt = 0;

	for  (;;)  {

		switch  ((ch = getch()))  {
		default:
			continue;

		case  '[' & 0x1F:
			longjmp(escj, 1);

		case  '.':case '-':case '_':
		case '0':case '1':case '2':case '3':case '4':
		case '5':case '6':case '7':case '8':case '9':
		case 'a':case 'b':case 'c':case 'd':case 'e':
		case 'f':case 'g':case 'h':case 'i':case 'j':
		case 'k':case 'l':case 'm':case 'n':case 'o':
		case 'p':case 'q':case 'r':case 's':case 't':
		case 'u':case 'v':case 'w':case 'x':case 'y':
		case 'z':
		case 'A':case 'B':case 'C':case 'D':case 'E':
		case 'F':case 'G':case 'H':case 'I':case 'J':
		case 'K':case 'L':case 'M':case 'N':case 'O':
		case 'P':case 'Q':case 'R':case 'S':case 'T':
		case 'U':case 'V':case 'W':case 'X':case 'Y':
		case 'Z':
			if  (cnt >= HOSTNSIZE)
				continue;
			hnp[cnt++] = ch;
			addch(ch);
			refresh();
			continue;

#ifdef	KEY_LEFT
		case  KEY_LEFT:
#endif
#ifdef	KEY_DC
		case  KEY_DC:
#endif
#ifdef	KEY_BACKSPACE
		case  KEY_BACKSPACE:
#endif
		case  'h' & 0x1F:
			if  (cnt > 0)  {
				cnt--;
				hnp[cnt] = '\0';
				mvaddch(row, starta+cnt, ' ');
				move(row, starta+cnt);
				refresh();
			}
			continue;

#ifdef	KEY_ENTER
		case  KEY_ENTER:
#endif
		case  '\n':
		case  '\r':
			hnp[cnt] = '\0';
			if  (cnt <= 0)  {
				if  (!musthave)
					return;
				move(row-1, 0);
				clrtoeol();
				standout();
				addstr("You must give some name for this host");
				break;
			}
			if  (isdigit(hnp[0]))  {
				move(row-1, 0);
				clrtoeol();
				standout();
				printw("%s is not a valid alias name", hnp);
				break;
			}
			if  (hnameclashes(hnp))  {
				move(row-1, 0);
				clrtoeol();
				standout();
				printw("%s is already a host/alias name", hnp);
				break;
			}
			return;
		}
		standend();
		move(row, starta+cnt);
		refresh();
	}
}

void	askuser(char *unam, const int row, const char *msg, const int validate, const int nonull)
{
	int	ch, starta, cnt;

	mvprintw(row, 0, "%s: ", msg);
	getyx(stdscr, ch, starta);
	refresh();

	cnt = 0;

	for  (;;)  {

		switch  ((ch = getch()))  {
		default:
			continue;
		case  '[' & 0x1F:
			longjmp(escj, 1);

		case  '.':case '-':case '_':
			if  (validate)
				continue;
		case '0':case '1':case '2':case '3':case '4':
		case '5':case '6':case '7':case '8':case '9':
			if  (cnt <= 0  && validate)
				continue;
		case 'a':case 'b':case 'c':case 'd':case 'e':
		case 'f':case 'g':case 'h':case 'i':case 'j':
		case 'k':case 'l':case 'm':case 'n':case 'o':
		case 'p':case 'q':case 'r':case 's':case 't':
		case 'u':case 'v':case 'w':case 'x':case 'y':
		case 'z':
		case 'A':case 'B':case 'C':case 'D':case 'E':
		case 'F':case 'G':case 'H':case 'I':case 'J':
		case 'K':case 'L':case 'M':case 'N':case 'O':
		case 'P':case 'Q':case 'R':case 'S':case 'T':
		case 'U':case 'V':case 'W':case 'X':case 'Y':
		case 'Z':
			if  (cnt >= UIDSIZE)
				continue;
			unam[cnt++] = ch;
			addch(ch);
			refresh();
			continue;
#ifdef	KEY_LEFT
		case  KEY_LEFT:
#endif
#ifdef	KEY_DC
		case  KEY_DC:
#endif
#ifdef	KEY_BACKSPACE
		case  KEY_BACKSPACE:
#endif
		case  'h' & 0x1F:
			if  (cnt > 0)  {
				cnt--;
				unam[cnt] = '\0';
				mvaddch(row, starta+cnt, ' ');
				move(row, starta+cnt);
				refresh();
			}
			continue;

#ifdef	KEY_ENTER
		case  KEY_ENTER:
#endif
		case  '\n':
		case  '\r':
			if  (cnt <= 0  &&  nonull)  {
				move(row-1, 0);
				clrtoeol();
				standout();
				addstr("You must give some user");
				standend();
				move(row, starta+cnt);
				refresh();
				continue;
			}
			unam[cnt] = '\0';
			if  (validate  &&  !getpwnam(unam))  {
				move(row-1, 0);
				clrtoeol();
				standout();
				printw("%s is not a valid user", unam);
				standend();
				move(row, starta+cnt);
				refresh();
				continue;
			}
			return;
		}
	}
}

void	proc_addunixhost()
{
	struct	remote	resrp;

	memset((void *) &resrp, '\0', sizeof(resrp));

	clear();
	askalias(&resrp, 4, askhname(resrp.hostname, 2, "Unix host name", &resrp) == IPADDR_IP);
	if  (ask(6, "Probe (check alive) before connecting", 1))
		resrp.ht_flags |= HT_PROBEFIRST;
	if  (ask(8, "Trust host with user information", 1))
		resrp.ht_flags |= HT_TRUSTED;
	if  (ask(10, "Manual Connections only", 0))
		resrp.ht_flags |= HT_MANUAL;

	asktimeout(&resrp, 12);
	addhostentry(&resrp);
}

void	proc_addwinhost()
{
	int	nxtrow;
	struct	remote	resrp;

	memset((void *) &resrp, '\0', sizeof(resrp));

	resrp.ht_flags |= HT_DOS;

	clear();
	nxtrow = 10;

	if  (ask(2, "Specific Windows Host (Y) or `roaming user' (N)", 1))  {
		askalias(&resrp, 6, askhname(resrp.hostname, 4, "Windows client host name", &resrp) == IPADDR_IP);
		askuser(resrp.dosuser, 8, "Default user", 1, 0);
	}
	else  {
		resrp.ht_flags |= HT_ROAMUSER;
		askuser(resrp.hostname, 4, "Unix user name", 1, 1);
		askuser(resrp.alias, 6, "Windows user name", 0, 0);
		if  (ask(8, "Do you want to specify a default machine name", 0))  {
			askhname(resrp.dosuser, 10, "Default unix host name", (struct remote *) 0);
			nxtrow = 12;
		}
	}
	if  (ask(nxtrow, "Password-check user(s)", 0))
		resrp.ht_flags |= HT_PWCHECK;

	asktimeout(&resrp, nxtrow+2);
	addhostentry(&resrp);
}

void	proc_addhost()
{
	clear();
	if  (ask(LINES/2, "Is new entry a Unix host(Y) or Client(N)", 1))
		proc_addunixhost();
	else
		proc_addwinhost();
}

void	proc_chnghost(struct remote *rp)
{
	int	changes = 0, nxtline = 0;
	struct	remote	resrp;
	resrp = *rp;

	clear();

	if  (resrp.ht_flags & HT_DOS)  {
		if  (resrp.ht_flags & HT_ROAMUSER)  {
			mvprintw(nxtline, 0, "Roaming user %s windows %s default client %s",
				 resrp.hostname,
				 resrp.alias[0]? resrp.alias: "<none>",
				 resrp.dosuser[0]? resrp.dosuser: "<none>");
			nxtline += 2;
			if  (ask(nxtline, "Change Unix user name", 0))  {
				nxtline += 2;
				askuser(resrp.hostname, nxtline, "New unix user", 1, 1);
				changes++;
				nxtline += 2;
			}
			if  (ask(nxtline, resrp.alias[0]? "Change windows user/alias": "Specify windows/user/alias", 0))  {
				nxtline += 2;
				askuser(resrp.alias, nxtline, "Windows user", 0, 0);
				changes++;
				nxtline += 2;
			}
			if  (ask(nxtline, resrp.dosuser[0]? "Change default client": "Specify default client", 0))  {
				nxtline += 2;
				askhname(resrp.dosuser, nxtline, "Default unix host name", (struct remote *) 0);
				changes++;
				nxtline += 2;
			}
		}
		else  {
			if  (resrp.ht_flags & HT_HOSTISIP)
				mvprintw(nxtline, 0, "Windows client %s named %s", phname(resrp.hostid, 0), resrp.hostname);
			else  {
				mvprintw(nxtline, 0, "Windows client %s", resrp.hostname);
				if  (resrp.alias[0])
					printw(" alias %s", resrp.alias);
			}
			if  (resrp.dosuser[0])
				printw("Default user %s", resrp.dosuser);
			nxtline += 2;
			if  (resrp.ht_flags & HT_HOSTISIP)  {
				if  (ask(nxtline, "Change host name", 0))  {
					nxtline += 2;
					askalias(&resrp, nxtline, 1);
					changes++;
					nxtline += 2;
				}
				if  (ask(nxtline, resrp.dosuser[0]? "Change default client": "Specify default client", 0))  {
					nxtline += 2;
					askhname(resrp.dosuser, nxtline, "Host name", (struct remote *) 0);
					nxtline += 2;
					changes++;
				}
			}
			else  {
				if  (ask(nxtline, resrp.alias[0]? "Change alias name": "Specify alias name", 0))  {
					nxtline += 2;
					askalias(&resrp, nxtline, 0);
					changes++;
					nxtline += 2;
				}
				if  (ask(nxtline, resrp.dosuser[0]? "Change default user": "Specify default user", 0))  {
					nxtline += 2;
					askuser(resrp.dosuser, nxtline, "Default user", 1, 0);
					changes++;
					nxtline += 2;
				}
			}
		}

		if  (ask(nxtline, resrp.ht_flags & HT_PWCHECK? "Turn off password check": "Turn on password check", 0))  {
			nxtline += 2;
			changes++;
			resrp.ht_flags ^= HT_PWCHECK;
		}
	}
	else  {
		if  (resrp.ht_flags & HT_HOSTISIP)
			mvprintw(nxtline, 0, "Unix host %s named %s", phname(resrp.hostid, 0), resrp.hostname);
		else  {
			mvprintw(nxtline, 0, "Unix host %s", resrp.hostname);
			if  (resrp.alias[0])
				printw(" alias %s", resrp.alias);
		}
		nxtline += 2;
		if  (resrp.ht_flags & HT_HOSTISIP)  {
			if  (ask(nxtline, "Change host name", 0))  {
				nxtline += 2;
				askalias(&resrp, nxtline, 1);
				changes++;
			}
		}
		else  if  (ask(nxtline, resrp.alias[0]? "Change alias name": "Specify alias name", 0))  {
			nxtline += 2;
			askalias(&resrp, nxtline, 0);
			changes++;
		}
		nxtline += 2;
		if  (ask(nxtline, resrp.ht_flags & HT_PROBEFIRST? "Turn off probe": "Turn on probe", 0))  {
			changes++;
			resrp.ht_flags ^= HT_PROBEFIRST;
		}
		nxtline += 2;
		if  (ask(nxtline, resrp.ht_flags & HT_MANUAL? "Turn off manual connect": "Turn on manual connect", 0))  {
			changes++;
			resrp.ht_flags ^= HT_MANUAL;
		}
		nxtline += 2;
		if  (ask(nxtline, resrp.ht_flags & HT_TRUSTED? "Turn off trust": "Turn on trust", 0))  {
			changes++;
			resrp.ht_flags ^= HT_TRUSTED;
		}
		nxtline += 2;
	}

	if  (resrp.ht_timeout != NETTICKLE)
		mvprintw(nxtline, 0, "Timeout value is set to %u (default %u).", resrp.ht_timeout, NETTICKLE);
	else
		mvprintw(nxtline, 0, "Timeout set to default of %u", resrp.ht_timeout);

	if  (ask(nxtline+2, "Change it", 0))  {
		resrp.ht_timeout = askn(nxtline+4, "New timeout value");
		changes++;
	}
	if  (changes)
		*rp = resrp;
}

void	proc_locaddr()
{
	int	nxtline = LINES / 3;

	clear();

	if  (hadlocaddr != NO_IPADDR)  {
		if  (ask(nxtline, "Local address set, do you want to unset it", 0))  {
			struct	hostent	*hp;
			char	myname[256];
			myname[sizeof(myname) - 1] = '\0';
			gethostname(myname, sizeof(myname) - 1);
			if  ((hp = gethostbyname(myname)))  {
				myhostid = *(netid_t *) hp->h_addr;
				hadlocaddr = NO_IPADDR;
			}
		}
	}
	else  if  (ask(nxtline, "Do you want to set a local address", 0))  {
		struct	remote	dummr;
		memset(&dummr, '\0', sizeof(dummr));
		hadlocaddr = askhname(dummr.hostname, nxtline+2, "Local address", &dummr);
		myhostid = dummr.hostid;
	}
}

void	proc_defuser()
{
	int	nxtline = LINES / 3;

	clear();

	if  (defcluser[0])  {
		if  (ask(nxtline, "Default user set, do you want to unset it", 0))
			defcluser[0] = '\0';
	}
	else  if  (ask(nxtline, "Do you want to set a default client user name", 0))  {
		char	wuser[UIDSIZE+1];
		askuser(wuser, nxtline+1, "Default user", 1, 1);
		strcpy(defcluser, wuser);
	}
}

void	enhancep(const char *str)
{
	while  (*str)  {
		if  (*str == '&')  {
			if  (!*++str)
				return;
#ifdef	HAVE_TERMINFO
			attron(A_UNDERLINE);
#else
			standout();
#endif
			addch(*str);
#ifdef	HAVE_TERMINFO
			attroff(A_UNDERLINE);
#else
			standend();
#endif
		}
		else
			addch(*str);
		str++;
	}
}

void	disp_hostlist(const int startrow)
{
	int	cnt, row;

	clear();

	enhancep("&add host &change host &delete host &local addr default &user &quit");
	mvprintw(1, HOST_P, "Host");
	mvprintw(1, ALIAS_P, "Alias");
	mvprintw(1, IPADDR_P, "IP Addr");
	mvprintw(2, UUSER_P, "Unix");
	mvprintw(2, WUSER_P, "Win");
	mvprintw(2, USER_P, "User");
	mvprintw(2, DMCH_P, "Dft mc");

	if  (hadlocaddr != NO_IPADDR)  {
		mvprintw(3, HOST_P, "Local Address:");
		if  (hadlocaddr == IPADDR_NAME)
			printw("%s", phname(myhostid, IPADDR_NAME));
	}
	else
		mvprintw(3, HOST_P, "Current: %s", phname(myhostid, IPADDR_NAME));
	mvprintw(3, IPADDR_P, phname(myhostid, IPADDR_IP));
	if  (defcluser[0])
		mvprintw(4, HOST_P, "Default client user: %s", defcluser);

	for  (row = HDRLINES, cnt = startrow;  row < LINES  &&  cnt < hostnum;  row++, cnt++)  {
		struct	remote	*rp = &hostlist[cnt];
		if  (rp->ht_flags & HT_DOS)  {
			if  (rp->ht_flags & HT_ROAMUSER)  {
				mvprintw(row, UUSER_P, "%s", rp->hostname);
				if  (rp->alias[0])
					mvprintw(row, WUSER_P, "%s", rp->alias);
				mvprintw(row, IPADDR_P, "Roam");
				if  (rp->dosuser[0])
					mvprintw(row, DMCH_P, "%s", rp->dosuser);
			}
			else  {
				if  (rp->ht_flags & HT_HOSTISIP)  {
					mvprintw(row, HOST_P, "%s", phname(rp->hostid, 1));
					mvprintw(row, ALIAS_P, "%s", rp->hostname);
				}
				else  {
					mvprintw(row, HOST_P, "%s", rp->hostname);
					if  (rp->alias[0])
						mvprintw(row, ALIAS_P, rp->alias);
					mvprintw(row, IPADDR_P, "%s", phname(rp->hostid, 1));
				}
				mvprintw(row, TYPE_P, "Client");
				if  (rp->dosuser[0])
					mvprintw(row, USER_P, "%s", rp->dosuser);
			}
			if  (rp->ht_flags & HT_PWCHECK)
				mvprintw(row, PWCHK_P, "Pwchk");
		}
		else  {
			if  (rp->ht_flags & HT_HOSTISIP)  {
				mvprintw(row, HOST_P, "%s", phname(rp->hostid, 1));
				mvprintw(row, ALIAS_P, "%s", rp->hostname);
			}
			else  {
				mvprintw(row, HOST_P, "%s", rp->hostname);
				if  (rp->alias[0])
					mvprintw(row, ALIAS_P, rp->alias);
				mvprintw(row, IPADDR_P, "%s", phname(rp->hostid, 1));
			}

			if  (rp->ht_flags & HT_PROBEFIRST)
				mvprintw(row, PROBE_P, "probe");
			if  (rp->ht_flags & HT_MANUAL)
				mvprintw(row, MANUAL_P, "manual");
			if  (rp->ht_flags & HT_TRUSTED)
				mvprintw(row, TRUSTED_P, "trusted");
		}
		if  (rp->ht_timeout != NETTICKLE)
			mvprintw(row, TO_P, "TO");
	}
}

void	proc_hostfile()
{
	int	topline, currline;

	initscr();
	raw();
	nonl();
	noecho();
	keypad(stdscr, TRUE);

	topline = currline = 0;

	setjmp(escj);
 refill:
	disp_hostlist(topline);

	for  (;;)  {
		int	ch;

		move(currline - topline + HDRLINES, 0);
		refresh();

		switch  ((ch = getch()))  {
		default:
			break;
		case  'q':
			clear();
			refresh();
			endwin();
			return;
#ifdef	KEY_UP
		case  KEY_UP:
#endif
		case  'k':
		case  '8':
			currline--;
		decrest:
			if  (currline >= topline)
				continue;
			if  (topline <= 0)  {
				currline = 0;
				continue;
			}
			topline = currline;
			if  (topline >= 0)
				goto  refill;
			topline = currline = 0;
			continue;

#ifdef	KEY_PREVIOUS
		case  KEY_PREVIOUS:
#endif
#ifdef	KEY_PPAGE
		case  KEY_PPAGE:
#endif
		case  'P':
			currline -= LINES-HDRLINES;
			goto  decrest;

#ifdef	KEY_DOWN
		case  KEY_DOWN:
#endif
		case  'j':
		case  '2':
			currline++;
		increst:
			if  (currline >= hostnum)  {
				currline = hostnum-1;
				if  (currline < 0)
					currline = 0;
			}
			if  (currline-topline < LINES-HDRLINES)
				continue;
			topline = currline + HDRLINES - LINES - 1;
			goto  refill;

#ifdef	KEY_NEXT
		case  KEY_NEXT:
#endif
#ifdef	KEY_NPAGE
		case  KEY_NPAGE:
#endif
		case  'N':
			currline += LINES-HDRLINES;
			goto  increst;

		case  'd':
			if  (hostnum <= 0)
				continue;
			if  (ask(currline-topline+HDRLINES, "Sure about deleting that", 0))  {
				hostnum--;
				for  (ch = currline;  ch < hostnum;  ch++)
					hostlist[ch] = hostlist[ch+1];
				if  (currline >= hostnum)  {
					currline--;
					if  (currline < topline)  {
						topline = currline;
						if  (topline < 0)
							currline = topline = 0;
					}
				}
			}
			disp_hostlist(topline);
			continue;

		case  'a':
			proc_addhost();
			goto  refill;

		case  'c':
			if  (hostnum <= 0)
				continue;
			proc_chnghost(&hostlist[currline]);
			goto  refill;

		case  'l':
			proc_locaddr();
			goto  refill;

		case  'u':
			proc_defuser();
			goto  refill;
		}
	}
}
