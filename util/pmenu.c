/* pmenu.c -- Curses based quick menu program

   Copyright 2009 Free Software Foundation, Inc.

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
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <curses.h>
#include "incl_unix.h"
#include <ctype.h>

#define	INITROWS	100
#define	INCROWS		30

int	nrows,
	maxrows,
	ncols1,
	ncols2;

int	Bigcols,		/* Number of big columns on screen */
	Bigrows,		/* Number of rows displayed at once */
	Colsinbigcol,		/* Number of screen cols in a big column + padding */
	Hirow = -1,		/* Highlighted row */
	Ahrow,			/* After header */
	Actlines,		/* Lines after header */
	multiselect,		/* Select multiple entries */
	numresult,		/* Give result as numeric option number */
	firstresult,		/* Only want "name" column */
	messnum;		/* Message number to look for */

int	delim = '\t';

char	*Header;		/* Header if any */

char	**names, **descrs, *selected;

char	*stracpy(const char *s)
{
	char	*r = malloc((unsigned) (strlen(s) + 1));
	if  (!r)  {
		fprintf(stderr, "Sorry out of memory\n");
		exit(255);
	}
	strcpy(r, s);
	return  r;
}

void	scaninput()
{
	int	lng;
	char	*tp;
	char	inbuf[120];

	if  (messnum != 0)  {
		while  (fgets(inbuf, sizeof(inbuf), stdin))  {
			int	innum, minus;

			if  (!isdigit(inbuf[0])  &&  inbuf[0] != '-')
				continue;

			lng = innum = minus = 0;
			if  (inbuf[0] == '-')  {
				lng++;
				minus = 1;
				if  (!isdigit(inbuf[1]))
					continue;
			}
			while  (isdigit(inbuf[lng]))
				innum = innum * 10 + inbuf[lng++] - '0';
			if  (inbuf[lng] != ':')
				continue;
			if  (minus)
				innum = -innum;
			if  (innum == messnum)
				goto  found;
		}
		fprintf(stderr, "Message %d not found\n", messnum);
		exit(100);
	}

 found:
	if  (Header  &&  strcmp(Header, "=") == 0)  {
		Header = (char *) 0;

		while  (fgets(inbuf, sizeof(inbuf), stdin))  {

			lng = strlen(inbuf);
			while  (lng > 0  &&  isspace(inbuf[lng-1]))
				lng--;
			inbuf[lng] = '\0';
			if  (lng <= 0)
				break;
			if  (Header)  {
				char	*nh = malloc((unsigned) (strlen(Header) + lng + 2));
				if  (!nh)  {
					fprintf(stderr, "Sorry out of memory\n");
					exit(255);
				}
				sprintf(nh, "%s\n%s", Header, inbuf);
				free(Header);
				Header = nh;
			}
			else
				Header = stracpy(inbuf);
		}
	}

	while  (fgets(inbuf, sizeof(inbuf), stdin))  {

		lng = strlen(inbuf);
		while  (lng > 0  &&  isspace(inbuf[lng-1]))
			lng--;
		inbuf[lng] = '\0';
		if  (lng <= 0)
			break;

		if  ((tp = strchr(inbuf, delim)))  {

			int	l1 = tp - inbuf, l2 = lng - l1 - 1;

			*tp++ = '\0';

			while  (*tp == delim)  {
				tp++;
				l2--;
			}

			if  (l1 > ncols1)
				ncols1 = l1;
			if  (l2 > ncols2)
				ncols2 = l2;

		}
		else  if  (lng > ncols1)
			ncols1 = lng;

		if  (nrows >= maxrows)  {
			if  (maxrows == 0)  {
				maxrows = INITROWS;
				names = (char **) malloc((unsigned) INITROWS * sizeof(char *));
				descrs = (char **) malloc((unsigned) INITROWS * sizeof(char *));
			}
			else  {
				maxrows += INCROWS;
				names = (char **) realloc((char *) names, (unsigned) (maxrows * sizeof(char *)));
				descrs = (char **) realloc((char *) descrs, (unsigned) (maxrows * sizeof(char *)));
			}
			if  (!names  ||  !descrs)  {
				fprintf(stderr, "Sorry, not enough memory\n");
				exit(255);
			}
		}

		names[nrows] = stracpy(inbuf);
		descrs[nrows] = stracpy(tp? tp: "");
		nrows++;
	}

	/*	Nothing to do if no input  */

	if  (nrows <= 0)
		exit(0);

	/*  If we are doing multiselection, allocate bytes to indicate selections  */

	if  (multiselect)  {
		if  (!(selected = malloc(nrows)))  {
			fprintf(stderr, "Sorry, no memory for selections\n");
			exit(255);
		}
		memset(selected, '\0', nrows);
	}
}

void	dohelp(const int crow)
{
	char	*fname = names[crow], *sp;
	unsigned  lng, cline, rcnt, widest = 0;
	int	r;
	FILE	*hf;
	char	**lines_buff, *line;
	WINDOW	*helpwin;

	if  ((lng = strlen(fname)) == 0)
		goto  nohelp;
	lng += 10;		/* /Menuhelp + null char */
	if  (!(fname = malloc(lng)))
		goto  nohelp;

	/* Catch case of tabulated data */

	if  ((sp = strchr(names[crow], ' ')))
		*sp = '\0';
	sprintf(fname, "%s/Menuhelp", names[crow]);
	if  ((hf = fopen(fname, "r")))
		goto  found;
	sprintf(fname, "%s.menuhelp", names[crow]);
	if  ((hf = fopen(fname, "r")))
		goto  found;
	if  (!(hf = fopen("Menu-summary", "r")))  {
		if  (sp)
			*sp = ' ';
		free(fname);
		goto  nohelp;
	}
 found:
	if  (sp)
		*sp = ' ';
	free(fname);
	if  (!(lines_buff = malloc((unsigned) (LINES-2)*sizeof(char *))))  {
		fclose(hf);
		goto  nohelp;
	}
	if  (!(line = malloc((unsigned) COLS-1)))  {
		free(lines_buff);
		fclose(hf);
		goto  nohelp;
	}
	cline = 0;
	rcnt = LINES - 2;

	while  (fgets(line, COLS-2, hf)  &&  cline < rcnt)  {
		unsigned  ll = strlen(line);
		if  (ll > 0  &&  line[ll-1] == '\n')
			line[--ll] = '\0';
		if  (ll > widest)
			widest = ll;
		lines_buff[cline] = stracpy(line);
		cline++;
	}
	free(line);
	fclose(hf);

	if  (cline == 0)  {
		lines_buff[0] = stracpy("help file empty");
		cline = 1;
		widest = strlen(lines_buff[0]);
	}

 disp:
	if  (!(helpwin = newwin(cline+2, widest+2, (LINES - cline - 2)/2, (COLS - widest - 2)/2)))  {
		for  (r = 0;  r < cline;  r++)
			free(lines_buff[r]);
		free((char *) lines_buff);
		return;
	}
#ifdef	HAVE_TERMINFO
	box(helpwin, 0, 0);
#else
	box(helpwin, '|', '-');
#endif
	for  (r = 0;  r < cline;  r++)  {
		mvwaddstr(helpwin, r+1, 1, lines_buff[r]);
		free(lines_buff[r]);
	}
	free((char *) lines_buff);
	wmove(helpwin, cline, widest);
	wrefresh(helpwin);
	wgetch(helpwin);
	delwin(helpwin);
	return;

 nohelp:
	lines_buff = malloc(sizeof(char *));
	if  (!lines_buff)
		return;
	lines_buff[0] = stracpy("No help for this option");
	widest = strlen(lines_buff[0]);
	cline = 1;
	goto  disp;
}

void	fillscreen(int startrow, int hiliterow)
{
	int	cc, rr, roff = 0;

	clear();
	Hirow = hiliterow;

	if  (Header)  {
		char	*hp = Header, *cp;
		int	Hlng;
		Ahrow = 0;
		while  ((cp = strchr(hp, '\n')))  {
			Hlng = cp - hp;
			*cp = '\0';
			mvaddstr(Ahrow, (COLS-Hlng)/2, hp);
			*cp = '\n';
			Ahrow++;
			hp = cp + 1;
		}
		Hlng = strlen(hp);
		mvaddstr(Ahrow, (COLS-Hlng)/2, hp);
		Ahrow += 2;
	}

	for  (cc = 0;  cc < Bigcols;  cc++)  {
		int	wcol = cc * Colsinbigcol;
		for  (rr = 0;  rr < Actlines;  rr++)  {
			int	wrow = startrow + rr + roff;
			if  (wrow >= nrows)
				return;
			if  (wrow == hiliterow)
				standout();
			mvprintw(rr + Ahrow, wcol, "%-*.*s%c%-*.*s", ncols1, ncols1, names[wrow],
				 multiselect && selected[wrow]? '+': ' ',
				 ncols2, ncols2, descrs[wrow]);
			if  (wrow == hiliterow)
				standend();
		}
		roff += Actlines;
	}
}

#ifdef	BUGGY_INCH
#define	rehighlight	fillscreen
#else
void	rehighlight(int startrow, int hiliterow)
{
	int	cc;
	int	relrow, bcol, brow;

	if  (Hirow == hiliterow)
		return;

	if  (Hirow >= startrow)  {
		relrow = Hirow - startrow;
		bcol = (relrow / Actlines) * Colsinbigcol;
		brow = relrow % Actlines;
		move(brow + Ahrow, bcol);
#ifdef	HAVE_TERMINFO
		for  (cc = 0;  cc < Colsinbigcol;  cc++)
			addch(inch() & A_CHARTEXT);
#else
		for  (cc = 0;  cc < Colsinbigcol;  cc++)
			addch(inch());
#endif
	}

	Hirow = hiliterow;
	relrow = hiliterow - startrow;
	bcol = (relrow / Actlines) * Colsinbigcol;
	brow = relrow % Actlines;
	move(brow + Ahrow, bcol);
#ifdef	HAVE_TERMINFO
	for  (cc = 0;  cc < Colsinbigcol;  cc++)
		addch(inch() | A_STANDOUT);
#else
	standout();
	for  (cc = 0;  cc < Colsinbigcol;  cc++)
		addch(inch());
	standend();
#endif
}
#endif /* not BUGGY_INCH */

void	splurge(const int crow, FILE *outf)
{
	if  (numresult)
		fprintf(outf, "%d\n", crow+1);
	else  if  (!firstresult  &&  strlen(descrs[crow]) > 0)
		fprintf(outf, "%s%c%s\n", names[crow], delim, descrs[crow]);
	else
		fprintf(outf, "%s\n", names[crow]);
}

void	splurgeall(FILE *outf)
{
	int	crow;
	for  (crow = 0;  crow < nrows;  crow++)
		if  (selected[crow])
			splurge(crow, outf);
}

int	search(const int strow, int ch)
{
	int	cnt;

	ch = tolower(ch);

	for  (cnt = strow+1;  cnt < nrows;  cnt++)  {
		if  (tolower(names[cnt][0]) == ch)
			return  cnt;
	}
	for  (cnt = 0;  cnt < strow;  cnt++)  {
		if  (tolower(names[cnt][0]) == ch)
			return  cnt;
	}
	return  -1;
}

void	process(FILE *outf, int crow)
{
	int	toprow, lastch = 9999;

	/* Get our screen geometry etc */

	initscr();
	raw();
	nonl();
	noecho();
	keypad(stdscr, TRUE);

	Actlines = LINES;

	if  (Header)  {
		char	*cp = Header, *np;
		while  ((np = strchr(cp, '\n')))  {
			cp = np + 1;
			Actlines--;
		}
		Actlines -= 2;
	}

	/* Work out how many big cols we have, and rows per page */

	Colsinbigcol = ncols1 + ncols2 + 2;
	Bigcols = COLS / Colsinbigcol;
	if  (Bigcols <= 0)
		Bigcols = 1;

	Bigrows = Actlines * Bigcols;

	if  (Bigcols == 1)  {
		if  (Colsinbigcol > COLS)  {
			Colsinbigcol = COLS;
			ncols2 = Colsinbigcol - ncols1 - 2;
		}
	}

	if  (crow < 0)
		crow = 0;
	else  if  (crow >= nrows)
		crow = nrows - 1;
	toprow = (crow / Bigrows) * Bigrows;

	fillscreen(toprow, crow);

	for  (;;)  {
		int	ch, srow;

		move(LINES-1, COLS-1);
		refresh();

		switch  ((ch = getch()))  {
		default:
			srow = search(crow, ch);
			if  (srow < 0)
				break;
			crow = srow;
			srow = (crow / Bigrows) * Bigrows;
			if  (toprow != srow)  {
				toprow = srow;
				fillscreen(toprow, crow);
			}
			else
				rehighlight(toprow, crow);
			break;

		case  '?':
#ifdef	KEY_F
		case  KEY_F(1):
#endif
			dohelp(crow);
#ifdef	CURSES_OVERLAP_BUG
			clear();
			fillscreen(toprow, crow);
#else
			touchwin(stdscr);
#endif
			continue;

		case  'q':
			if  (multiselect)
				splurgeall(outf);
			clear();
			refresh();
			endwin();
			exit(0);

#ifdef	KEY_LEFT
		case  KEY_LEFT:
#endif
		case  'h':
		case  '4':
			crow -= Actlines;
		decrest:
			if  (crow < toprow)  {
				toprow -= Bigrows;
				if  (toprow < 0)
					toprow = 0;
				if  (crow < toprow)
					crow = toprow;
				fillscreen(toprow, crow);
			}
			else
				rehighlight(toprow, crow);
			break;


#ifdef	KEY_RIGHT
		case  KEY_RIGHT:
#endif
		case  'l':
		case  '6':
			crow += Actlines;
		increst:
			if  (crow >= nrows)
				crow = nrows - 1;
			if  (crow >= toprow + Bigrows)  {
				toprow += Bigrows;
				fillscreen(toprow, crow);
			}
			else
				rehighlight(toprow, crow);
			break;

#ifdef	KEY_UP
		case  KEY_UP:
#endif
		case  'k':
		case  '8':
			crow--;
			goto  decrest;

#ifdef	KEY_DOWN
		case  KEY_DOWN:
#endif
		case  'j':
		case  '2':
			crow++;
			goto  increst;

#ifdef	KEY_NEXT
		case  KEY_NEXT:
#endif
#ifdef	KEY_SRIGHT
		case  KEY_SRIGHT:
#endif
#ifdef	KEY_NPAGE
		case  KEY_NPAGE:
#endif
		case  'N':
			crow += Bigrows;
			goto  increst;

#ifdef	KEY_PREVIOUS
		case  KEY_PREVIOUS:
#endif
#ifdef	KEY_SLEFT
		case  KEY_SLEFT:
#endif
#ifdef	KEY_PPAGE
		case  KEY_PPAGE:
#endif
		case  'P':
			crow -= Bigrows;
			goto  decrest;

		case  ' ':
			if  (multiselect)  {
				selected[crow] = !selected[crow];
				move((crow - toprow) % Actlines + Ahrow, ncols1 + ((crow-toprow) / Actlines) * Colsinbigcol);
				standout();
				addch(selected[crow]? '+': ' ');
				standend();
			}
			break;

#ifdef	KEY_ENTER
		case  KEY_ENTER:
#endif
		case  '\n':
		case  '\r':
			if  (multiselect)  {
				if  (lastch != ch  &&  lastch != ' ')  {
					selected[crow] = !selected[crow];
					move((crow - toprow) % Actlines + Ahrow, ncols1 + ((crow-toprow) / Actlines) * Colsinbigcol);
					standout();
					addch(selected[crow]? '+': ' ');
					standend();
					break;
				}
				splurgeall(outf);
			}
			else
				splurge(crow, outf);
			clear();
			refresh();
			endwin();
			exit(0);
		}

		lastch = ch;
	}
}

int	xdig(int ch)
{
	switch  (ch)  {
	default:
		fprintf(stderr, "Invalid hex digit in delimiter\n");
		exit(1);

	case  '0':case  '1':case  '2':case  '3':case  '4':
	case  '5':case  '6':case  '7':case  '8':case  '9':
		return  ch - '0';

	case  'a':case  'b':case  'c':case  'd':case  'e':case  'f':
		return  ch - 'a' + 10;

	case  'A':case  'B':case  'C':case  'D':case  'E':case  'F':
		return  ch - 'A' + 10;
	}
}

MAINFN_TYPE  main(int argc, char ** argv)
{
	extern	char	*optarg;
	char	*sfile = (char *) 0;
	int	ch, outfd, srow = 0;
	FILE	*outf;

	while  ((ch = getopt(argc, argv, "FmnN:f:h:d:s:")) != EOF)  {
		switch  (ch)  {
		default:
			fprintf(stderr, "Usage: %s [-F] [-m] [-n] [-N num] [ -f file ] [ -h header ] [ -d delim ] [ -s start ]\n", argv[0]);
			return  1;
		case  's':
			srow = atoi(optarg)-1;
			break;
		case  'f':
			sfile = optarg;
			break;
		case  'h':
			Header = optarg;
			break;
		case  'm':
			multiselect++;
			break;
		case  'n':
			numresult++;
			break;
		case  'F':
			firstresult++;
			break;
		case  'N':
			messnum = atoi(optarg);
			break;
		case  'd':
			if  (!isprint(optarg[0]))  {
			invd:
				fprintf(stderr, "Invalid delimiter\n");
				return  1;
			}
			if  (optarg[0] == '^')  {
				if  (!isprint(optarg[1]))
					goto  invd;
				if  (optarg[1] == '^')  {
					delim = '^';
					break;
				}
				delim = optarg[1] & 0x3f;
				break;
			}
			if  (optarg[0] == '\\')  {
				if  (!isprint(optarg[1]))
					goto  invd;
				switch  (optarg[1])  {
				default:
					goto  invd;
				case  '\\':
					delim = '\\';
					break;
				case  'b':
				case  'B':
					delim = '\b';
					break;
				case  't':
				case  'T':
					delim = '\t';
					break;
				case  'x':
				case  'X':
					delim = (xdig(optarg[2]) << 4) | xdig(optarg[3]);
					break;
				}
				break;
			}
			delim = optarg[0];
			break;
		}

	}

	if  (sfile  &&  !freopen(sfile, "r", stdin))  {
		fprintf(stderr, "%s: cannot open %s\n", argv[0], sfile);
		return  2;
	}

	scaninput();

	outfd = dup(1);
	outf = fdopen(outfd, "w");

	close(0);
	close(1);
	close(2);

	dup(dup(open("/dev/tty", O_RDWR)));

	process(outf, srow);
	return  0;
}
