/* helpparse.c -- helpfile parsing main module

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
#include <limits.h>
#include <stdio.h>
#include "incl_unix.h"
#include "incl_dir.h"
#include "hdefs.h"

static  char  codelst[] = "EHPQAKNSX";
extern	struct	helpfile	helpfiles[];
extern	int	errors, line_count, pass1;

char	*srcdir, *destdir;
static	FILE	*kdescrfile;

#define	VP_PL_MAX	5

extern	void	resetlex();
extern	int	yyparse();

void	add_helpref(struct valname *vp, struct module *mp, unsigned flags)
{
	struct	program_list	*pl;
	struct	helpfile	*hf;
	struct	helpfile_list	*hfl;

	/*
	 *	Find each program that the module appears in....
	 */

	for  (pl = mp->mod_pl;  pl;  pl = pl->pl_next)  {
		hf = pl->pl_prog->prog_hf; /* Helpfile for the program */
		for  (hfl = vp->vn_hlist;  hfl;  hfl = hfl->hfl_next)
			if  (hfl->hfl_hf == hf)	{
				hfl->hfl_flags |= flags;
				goto  nxt;
			}
		hfl = (struct helpfile_list *) malloc(sizeof(struct helpfile_list));
		if  (!hfl)
			abort();
		hfl->hfl_hf = hf;
		hfl->hfl_next = vp->vn_hlist;
		hfl->hfl_flags = flags;
		vp->vn_hlist = hfl;
	nxt:
		;
	}
}

void	record_help(struct helpfile *hf, const char htype, const long h1, const long h2)
{
	unsigned  long	hashval = (unsigned long) (((int) htype) * 97 + h2 * 13 + h1) % HELPHASH;
	struct	hadhelp	*hv;

	for  (hv = hf->hf_hash[hashval];  hv;  hv = hv->had_next)
		if  (hv->had_char == htype && hv->had_value == h1 && hv->had_state == h2)  {
			fprintf(stderr, "Warning: duplicate %ld %c %ld in help file %s\n", h2, htype, h1, hf->hf_name);
			return;
		}

	if  (!(hv = (struct hadhelp *) malloc(sizeof(struct hadhelp))))
		abort();
	hv->had_next = hf->hf_hash[hashval];
	hf->hf_hash[hashval] = hv;
	hv->had_value = h1;
	hv->had_state = h2;
	hv->had_char = htype;
}

void	scanmodule(struct module *mp)
{
	FILE	*modin;
	FILE	*modout;
	int	mod_line = 1;
	int	vp_count = 0, vncount, cnt, ch;
	unsigned	flags;
	long	finger;
	struct	valname	*vp, *vp_list[VP_PL_MAX];
	char	valnbuf[100];

	if  (mp->mod_subdir)  {
		char	mod_path[PATH_MAX];
		sprintf(mod_path, "%s/%s/%s", srcdir, mp->mod_subdir, mp->mod_name);
		modin = fopen(mod_path, "r");
		if  (!modin)  {
			fprintf(stderr, "Sorry cannot open source file %s\n", mod_path);
			exit(20);
		}
		sprintf(mod_path, "%s/%s/%s", destdir, mp->mod_subdir, mp->mod_name);
		modout = fopen(mod_path, "w");
		if  (!modout)  {
			sprintf(mod_path, "%s/%s", destdir, mp->mod_subdir);
			if  (mkdir(mod_path, 0777) < 0)  {
				fprintf(stderr, "Sorry cannot make subdirectory %s in %s\n", mp->mod_subdir, destdir);
				exit(21);
			}
			sprintf(mod_path, "%s/%s/%s", destdir, mp->mod_subdir, mp->mod_name);
			modout = fopen(mod_path, "w");
			if  (!modout)  {
				fprintf(stderr, "Sorry cannot write dest file %s\n", mod_path);
				exit(22);
			}
		}
	}
	else  {
		char	mod_path[PATH_MAX];
		sprintf(mod_path, "%s/%s", srcdir, mp->mod_name);
		modin = fopen(mod_path, "r");
		if  (!modin)  {
			fprintf(stderr, "Sorry cannot open source file %s\n", mod_path);
			exit(20);
		}
		sprintf(mod_path, "%s/%s", destdir, mp->mod_name);
		modout = fopen(mod_path, "w");
		if  (!modout)  {
			fprintf(stderr, "Sorry cannot write dest file %s\n", mod_path);
			exit(22);
		}
	}

 restart:
	while  ((ch = getc(modin)) != EOF)  {
		if  (ch == '\n')  {
			if  (vp_count > 0)  {
				int  cnt;
				fputs("\t/* ", modout);
				for  (cnt = 0;  cnt < vp_count;  cnt++)
					fprintf(modout, "{%s} ", vp_list[cnt]->vn_string);
				fputs("*/", modout);
				vp_count = 0;
			}
			mod_line++;
			putc(ch, modout);
			continue;
		}
		if  (ch != '$')  {
			putc(ch, modout);
			continue;
		}
		finger = ftell(modin);
		flags = 0;
		for  (;;)  {
			ch = getc(modin);
			switch  (ch)  {
			default:
				fseek(modin, finger, 0L);
				putc('$', modout);
				goto  restart;
			case  'E':	flags |= VN_DEFDE;	break;
			case  'H':	flags |= VN_DEFDH;	break;
			case  'P':	flags |= VN_DEFDP;	break;
			case  'Q':	flags |= VN_DEFDQ;	break;
			case  'R':	flags |= VN_DEFDQ;	break;
			case  'A':	flags |= VN_DEFDA;	break;
			case  'N':	flags |= VN_DEFDN;	break;
			case  'K':	flags |= VN_DEFDK;	break;
			case  'S':	flags |= VN_DEFDS;	break;
			case  'X':	flags |= VN_DEFDX;	break;
			case  '{':	goto  getstr;
			}
		}
	getstr:
		if  (!flags)  {
			fseek(modin, finger, 0L);
			putc('$', modout);
			goto  restart;
		}
		vncount = 0;
		for  (;;)  {
			ch = getc(modin);
			if  (ch == '}')
				break;
			if  (ch == EOF || ch == '\n' || vncount >= sizeof(valnbuf) - 2)  {
				fprintf(stderr, "Unterminated $ construct module %s line %d\n", mp->mod_name, mod_line);
				fseek(modin, finger, 0L);
				putc('$', modout);
				errors++;
				goto  restart;
			}
			valnbuf[vncount++] = ch;
		}
		valnbuf[vncount] = '\0';
		vp = lookupname(valnbuf);
		if  (!(vp->vn_flags & VN_HASVALUE))  {
			fprintf(stderr, "Undefined name {%s} module %s line %d\n", valnbuf, mp->mod_name, mod_line);
			errors++;
		}
		else if  ((vp->vn_flags & flags) != flags)  {
			for  (cnt = 0;  cnt < sizeof(codelst)-1;  cnt++)  {
				if  (flags & (1 << cnt)  &&  !(vp->vn_flags & (1 << cnt)))  {
					fprintf(stderr, "No %c definition for {%s} required module %s line %d\n",
						       codelst[cnt], valnbuf, mp->mod_name, mod_line);
					errors++;
				}
			}
		}
		vp_list[vp_count++] = vp;
		fprintf(modout, "%d", vp->vn_value);
		add_helpref(vp, mp, flags);
	}
	fclose(modin);
	fclose(modout);
}

void	valname_usage(struct valexpr *ve, const unsigned flags)
{
	switch  (ve->val_op)  {
	case  VAL_NAME:
		ve->val_un.val_name->vn_flags |= flags;
		break;
	case  '+':
	case  '-':
	case  '*':
	case  '/':
	case  '%':
		valname_usage(ve->val_left, flags);
		valname_usage(ve->val_un.val_right, flags);
		break;
	}
}

static	unsigned	helpusedin(struct valexpr *ve, struct helpfile *hf)
{
	struct	helpfile_list	*hfl;

	switch  (ve->val_op)  {
	default:
		return  0;
	case  VAL_NAME:
		for  (hfl = ve->val_un.val_name->vn_hlist;  hfl;  hfl = hfl->hfl_next)
			if  (hfl->hfl_hf == hf)
				return  hfl->hfl_flags;
		return  0;
	case  '+':
	case  '-':
	case  '*':
	case  '/':
	case  '%':
		return  helpusedin(ve->val_left, hf) | helpusedin(ve->val_un.val_right, hf);
	}
}

static	void	dumpexpr(struct valexpr *ve)
{
	switch  (ve->val_op)  {
	default:
		fprintf(stderr, "???");
		return;
	case  VAL_VALUE:
		fprintf(stderr, "%ld", ve->val_un.val_value);
		return;
	case  VAL_CHVALUE:
		fprintf(stderr, "\'%c\'", (char) ve->val_un.val_value);
		return;
	case  VAL_NAME:
		fprintf(stderr, "{%s}", ve->val_un.val_name->vn_string);
		return;
	case  VAL_ROUND:
		fprintf(stderr, "ROUND(");
		dumpexpr(ve->val_left);
		fprintf(stderr, ", ");
		dumpexpr(ve->val_un.val_right);
		putc(')', stderr);
		return;
	case  '+':
	case  '-':
	case  '*':
	case  '/':
	case  '%':
		putc('(', stderr);
		dumpexpr(ve->val_left);
		putc(ve->val_op, stderr);
		dumpexpr(ve->val_un.val_right);
		putc(')', stderr);
		return;
	}
}

static	void	putcomment(char *comment, FILE *f)
{
	while  (*comment)  {
		while  (*comment == '\n')  {
			putc('\n', f);
			comment++;
		}
		if  (!*comment)
			break;
		fprintf(f, "# ");
		do  {
			putc(*comment, f);
			comment++;
		}  while  (*comment && *comment != '\n');
	}
	putc('\n', f);
}

void	apphelps(char *comment, struct valexpr *stateexpr, char *helptypes, struct valexpr *ve, struct filelist *flst, struct textlist *tl)
{
	int			cnt, rcnt, rccnt;
	long			ev1, ev2;
	unsigned		flgs;
	struct	textlist	*ctl;
	char			*chelptypes, *str;
	struct	helpfile	*hf, *helps_used[MAXHELPFILES];
	unsigned		helps_flags[MAXHELPFILES];

	rcnt = 0;		/* Count of help files used in (hwm in helps_used, helps_flags) */

	if  (flst)  {
		/* Required in list get flags from helptypes */
		flgs = 0;
		for  (chelptypes = helptypes;  *chelptypes;  chelptypes++)
			for  (cnt = 0;  cnt < sizeof(codelst)-1;  cnt++)
				if  (codelst[cnt] == *chelptypes)  {
					flgs |= (1 << cnt);
					break;
				}
		do  {
			char	*fname = flst->name;
			for  (cnt = 0;  cnt < MAXHELPFILES && helpfiles[cnt].hf_name;  cnt++)
				if  (strcmp(helpfiles[cnt].hf_name, fname) == 0)  {
					for  (rccnt = 0;  rccnt < rcnt;  rccnt++)
						if  (helps_used[rccnt] == &helpfiles[cnt])  {
							helps_flags[rccnt] |= flgs;
							goto  gotit;
						}
					helps_used[rcnt] = &helpfiles[cnt];
					helps_flags[rcnt] = flgs;
					rcnt++;
				gotit:
					;
				}
		}  while  ((flst = flst->next));
	}

	for  (cnt = 0;  cnt < MAXHELPFILES && helpfiles[cnt].hf_name;  cnt++)  {
		flgs = helpusedin(ve, &helpfiles[cnt]);
		if  (stateexpr)
			flgs |= helpusedin(stateexpr, &helpfiles[cnt]);
		if  (flgs)  {
			for  (rccnt = 0;  rccnt < rcnt;  rccnt++)
				if  (helps_used[rccnt] == &helpfiles[cnt])  {
					helps_flags[rccnt] |= flgs;
					goto  dunf;
				}
			helps_used[rcnt] = &helpfiles[cnt];
			helps_flags[rcnt] = flgs;
			rcnt++;
		dunf:
			;
		}
	}

	if  (rcnt <= 0)  {
		fprintf(stderr, "Help text definition at line %d not used anywhere: ", line_count);
		dumpexpr(ve);
		putc('\n', stderr);
		return;
	}

	for  (cnt = 0;  cnt < rcnt;  cnt++)  {
		char	*comm = comment;
		hf = helps_used[cnt];
		ctl = tl;

		for  (chelptypes = helptypes;  *chelptypes;  chelptypes++)  {
			switch  (*chelptypes)  {
			case  'N':
				if  (!(helps_flags[cnt] & VN_DEFDN))
					continue;
				if  (stateexpr)  {
					if  (comm)  {
						putcomment(comm, hf->hf_filep);
						comm = (char *) 0;
					}
					fprintf(hf->hf_filep, "%ldN%ld\n", ev2 = evaluate(stateexpr), ev1 = evaluate(ve));
					record_help(hf, 'N', ev1, ev2);
				}
			case  'S':
				continue;
			case  'P':
				if  (!(helps_flags[cnt] & VN_DEFDP))
					break;
				if  (comm)  {
					putcomment(comm, hf->hf_filep);
					comm = (char *) 0;
				}
				fprintf(hf->hf_filep, "%ldP:%s\n", ev1 = evaluate(ve), ctl->tl_text);
				record_help(hf, 'P', ev1, 0L);
				break;
			case  'A':
				if  (!(helps_flags[cnt] & VN_DEFDA))
					break;
				goto  akrest;
			case  'K':
				if  (!(helps_flags[cnt] & VN_DEFDK))
					break;
			akrest:
				str = ctl->tl_text;
				if  (comm)  {
					putcomment(comm, hf->hf_filep);
					comm = (char *) 0;
				}
				ev1 = evaluate(ve);
				ev2 = 0L;
				if  (stateexpr)
					fprintf(hf->hf_filep, "%ld%c%ld:%s\n", ev2 = evaluate(stateexpr), *chelptypes, ev1, str);
				else
					fprintf(hf->hf_filep, "%c%ld:%s\n", *chelptypes, ev1, str);
				record_help(hf, *chelptypes, ev1, ev2);
				if  (kdescrfile  &&  *chelptypes == 'K'  &&  ve->val_op == VAL_NAME)  {
					fprintf(kdescrfile, "%s: ", hf->hf_name);
					if  (stateexpr)
						fprintf(kdescrfile, "%ld STATE", ev2);
					else
						fprintf(kdescrfile, "GLOBAL");
					fprintf(kdescrfile, " %ld {%s}\n", ev1, ve->val_un.val_name->vn_string);
				}
				break;

			case  'Q':
				if  (!(helps_flags[cnt] & VN_DEFDQ))
					break;
				str = ctl->tl_text;
				if  (comm)  {
					putcomment(comm, hf->hf_filep);
					comm = (char *) 0;
				}
				ev1 = evaluate(ve);
				ev2 = 0L;
				if  (stateexpr)
					fprintf(hf->hf_filep, "%ldA%ld:%s\n", ev1, ev2 = evaluate(stateexpr), str);
				else
					fprintf(hf->hf_filep, "%ldA:%s\n", ev1, str);
				record_help(hf, 'Q', ev1, ev2);
				break;

			case  'R':
				if  (!(helps_flags[cnt] & VN_DEFDQ))
					break;
				str = ctl->tl_text;
				if  (comm)  {
					putcomment(comm, hf->hf_filep);
					comm = (char *) 0;
				}
				ev1 = evaluate(ve);
				ev2 = 0L;
				if  (stateexpr)
					fprintf(hf->hf_filep, "%ldAD%ld:%s\n", ev1, ev2 = evaluate(stateexpr), str);
				else
					fprintf(hf->hf_filep, "%ldAD:%s\n", ev1, str);
				record_help(hf, 'Q', ev1, ev2);
				break;

			case  'X':
				if  (!(helps_flags[cnt] & VN_DEFDX))
					break;
				goto  herest;
			case  'H':
				if  (!(helps_flags[cnt] & VN_DEFDH))
					break;
				goto  herest;
			case  'E':
				if  (!(helps_flags[cnt] & VN_DEFDE))
					break;
			herest:
				str = ctl->tl_text;
				if  (comm)  {
					putcomment(comm, hf->hf_filep);
					comm = (char *) 0;
				}
				ev1 = evaluate(ve);
				record_help(hf, *chelptypes, ev1, 0L);
				while  (*str)  {
					fprintf(hf->hf_filep, "%c%ld:", *chelptypes, ev1);
					while  (*str && *str != '\n')  {
						putc(*str, hf->hf_filep);
						str++;
					}
					if  (*str == '\n')  {
						putc('\n', hf->hf_filep);
						str++;
					}
				}
				putc('\n', hf->hf_filep);
				break;
			}
			ctl = ctl->tl_next;
		}
	}
}

void	inithelps()
{
	int	cnt, ch;
	struct	helpfile	*hf;
	FILE	*hlpin;

	for  (cnt = 0;  cnt < MAXHELPFILES && helpfiles[cnt].hf_name;  cnt++)  {
		hf = &helpfiles[cnt];
		if  (hf->hf_subdir)  {
			char	hlp_path[PATH_MAX];
			sprintf(hlp_path, "%s/%s/%s", srcdir, hf->hf_subdir, hf->hf_name);
			hlpin = fopen(hlp_path, "r");
			if  (!hlpin)  {
				fprintf(stderr, "Sorry cannot open source file %s\n", hlp_path);
				exit(20);
			}
			sprintf(hlp_path, "%s/%s/%s", destdir, hf->hf_subdir, hf->hf_name);
			hf->hf_filep = fopen(hlp_path, "w");
			if  (!hf->hf_filep)  {
				sprintf(hlp_path, "%s/%s", destdir, hf->hf_subdir);
				if  (mkdir(hlp_path, 0777) < 0)  {
					fprintf(stderr, "Sorry cannot make subdirectory %s in %s\n", hf->hf_subdir, destdir);
					exit(21);
				}
				sprintf(hlp_path, "%s/%s/%s", destdir, hf->hf_subdir, hf->hf_name);
				hf->hf_filep = fopen(hlp_path, "w");
				if  (!hf->hf_filep)  {
					fprintf(stderr, "Sorry cannot write dest file %s\n", hlp_path);
					exit(22);
				}
			}
		}
		else  {
			char	hlp_path[PATH_MAX];
			sprintf(hlp_path, "%s/%s", srcdir, hf->hf_name);
			hlpin = fopen(hlp_path, "r");
			if  (!hlpin)  {
				fprintf(stderr, "Sorry cannot open source file %s\n", hlp_path);
				exit(20);
			}
			sprintf(hlp_path, "%s/%s", destdir, hf->hf_name);
			hf->hf_filep = fopen(hlp_path, "w");
			if  (!hf->hf_filep)  {
				fprintf(stderr, "Sorry cannot write dest file %s\n", hlp_path);
				exit(22);
			}
		}

		while  ((ch = getc(hlpin)) != EOF)
			putc(ch, hf->hf_filep);
		fclose(hlpin);
	}
}

void	closehelps()
{
	int	cnt;

	for  (cnt = 0;  cnt < MAXHELPFILES && helpfiles[cnt].hf_name;  cnt++)
		fclose(helpfiles[cnt].hf_filep);
}

static	void	nuke_dests(char *dst)
{
	DIR	*dd;
	struct	dirent	*dp;
	struct	stat	dbuf;
	char	dpath[PATH_MAX];

	if  (!(dd = opendir(dst)))
		return;

	while  ((dp = readdir(dd)))  {
		if  (dp->d_name[0] == '.'  &&  (dp->d_name[1] == '\0' || (dp->d_name[1] == '.' && dp->d_name[2] == '\0')))
			continue;
		sprintf(dpath, "%s/%s", dst, dp->d_name);
		if  (lstat(dpath, &dbuf) < 0  ||  (dbuf.st_mode & S_IFMT) != S_IFDIR)
			continue;
		nuke_dests(dpath);
	}

	rewinddir(dd);
	while  ((dp = readdir(dd)))  {
		if  (dp->d_name[0] == '.'  &&  (dp->d_name[1] == '\0' || (dp->d_name[1] == '.' && dp->d_name[2] == '\0')))
			continue;
		sprintf(dpath, "%s/%s", dst, dp->d_name);
		if  (lstat(dpath, &dbuf) < 0  ||  (dbuf.st_mode & S_IFMT) != S_IFREG)
			continue;
		unlink(dpath);
	}
	closedir(dd);
}

static	void	mod_copy(char *src, char *dst)
{
	DIR	*sd;
	struct	dirent	*dp;
	struct	stat	sbuf, dbuf;
	char	spath[PATH_MAX], dpath[PATH_MAX];

	if  (!(sd = opendir(src)))  {
		fprintf(stderr, "Sorry cannot open source dir %s\n", src);
		exit(25);
	}

	while  ((dp = readdir(sd)))  {
		if  (dp->d_name[0] == '.'  &&  (dp->d_name[1] == '\0' || (dp->d_name[1] == '.' && dp->d_name[2] == '\0')))
			continue;
		sprintf(spath, "%s/%s", src, dp->d_name);
		sprintf(dpath, "%s/%s", dst, dp->d_name);
		if  (lstat(spath, &sbuf) < 0)
			continue;
		if  (lstat(dpath, &dbuf) >= 0)  {
			if  ((sbuf.st_mode & S_IFMT) != (dbuf.st_mode & S_IFMT))  {
				if  ((dbuf.st_mode & S_IFMT) == S_IFLNK)
					continue;
				fprintf(stderr, "Confused by clashing type of %s\n", dpath);
				exit(26);
			}
			if  ((sbuf.st_mode & S_IFMT) == S_IFDIR)
				mod_copy(spath, dpath);
			continue;
		}
		if  ((sbuf.st_mode & S_IFMT) == S_IFREG  ||  (sbuf.st_mode & S_IFMT) == S_IFLNK)  {
			if  (link(spath, dpath) < 0)  {
				fprintf(stderr, "Unable to link %s to %s\n", spath, dpath);
				exit(27);
			}
		}
		else  if  ((sbuf.st_mode & S_IFMT) == S_IFDIR)  {
			if  (strcmp(dp->d_name, "RCS") == 0  ||  strcmp(dp->d_name, "U") == 0  ||  strcmp(dp->d_name, "config") == 0)
				continue;
			fprintf(stderr, "Creating directory %s\n", dpath);
			if  (mkdir(dpath, 0777) < 0)  {
				fprintf(stderr, "Sorry cannot make dest directory %s\n", dpath);
				exit(29);
			}
			mod_copy(spath, dpath);
		}
	}
	closedir(sd);
}

int  main(int argc, char **argv)
{
	int	nukem = 0;
	const	char	*pname = argv[0];
	struct	stat	sbuf;

	if  (strcmp(argv[1], "-d") == 0)  {
		nukem = 1;
		argc--;
		argv++;
	}

	if  (argc < 4 || argc > 5)  {
		fprintf(stderr, "%s: [-d] sourcefile srcdir destdir [keydescr]\n", pname);
		return  1;
	}

	srcdir = argv[2];
	destdir = argv[3];
	if  (stat(srcdir, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFDIR)  {
		fprintf(stderr, "Cannot find source directory %s\n", srcdir);
		return  2;
	}
	if  (stat(destdir, &sbuf) < 0)  {
		fprintf(stderr, "Creating directory %s\n", destdir);
		if  (mkdir(destdir, 0777) < 0)  {
			fprintf(stderr, "Sorry cannot create directory %s\n", destdir);
			return  3;
		}
	}
	else  if  ((sbuf.st_mode & S_IFMT) != S_IFDIR)  {
		fprintf(stderr, "%s is not a directory\n", destdir);
		return  4;
	}

	if  (!freopen(argv[1], "r", stdin))  {
		fprintf(stderr, "%s: cannot open source file %s\n", pname, argv[1]);
		return  5;
	}

	if  (argv[4]  &&  !(kdescrfile = fopen(argv[4], "w")))
		fprintf(stderr, "%s: warning: cannot create key descr file %s\n", pname, argv[4]);

	if  (yyparse())  {
		fprintf(stderr, "Aborted due to syntax error(s)\n");
		return  6;
	}

	if  (errors > 0)
		return  50;
	if  (nukem)
		nuke_dests(destdir);
	scanmodules();
	inithelps();
	rewind(stdin);
	pass1 = 0;
	line_count = 1;
	resetlex();
	yyparse();
	closehelps();
	mod_copy(srcdir, destdir);
	return  errors > 0? 50: 0;
}
