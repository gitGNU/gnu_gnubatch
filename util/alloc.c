/* alloc.c -- memory allocation for helpfile parsing

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
#include <stdio.h>
#include <setjmp.h>
#include "incl_unix.h"
#include "hdefs.h"

extern	int		errors, line_count;
struct	helpfile	helpfiles[MAXHELPFILES];
struct	program		programs[MAXPROGS];

#define	MACROHASHMOD	59
#define	VALHASHMOD	521

struct	macro	*macrohashtab[MACROHASHMOD];
struct	module	*modhashtab[MACROHASHMOD];
struct	valname	*valhash[VALHASHMOD];

char	*stracpy(const char *str)
{
	char	*result;
	if  (!(result = malloc((unsigned) (strlen(str) + 1))))
		abort();
	strcpy(result, str);
	return  result;
}

static	unsigned	vhashcalc(const char *name)
{
	unsigned  result = 0;
	while  (*name)
		result = (result << 1) ^ *name++;
	return  result % VALHASHMOD;
}

struct	valname	*lookupname(const char * name)
{
	struct	valname	*hp;
	unsigned  hashval = vhashcalc(name);

	for  (hp = valhash[hashval];  hp;  hp = hp->vn_next)
		if  (strcmp(hp->vn_string, name) == 0)
			return  hp;
	if  (!(hp = (struct valname *) malloc(sizeof(struct valname))))
		abort();
	hp->vn_next = valhash[hashval];
	valhash[hashval] = hp;
	hp->vn_string = stracpy(name);
	hp->vn_flags = 0;
	hp->vn_value = UNDEFINED_VALUE;
	hp->vn_hlist = (struct helpfile_list *) 0;
	return  hp;
}

struct	valexpr	*alloc_expr()
{
	struct	valexpr	*result = (struct valexpr *) malloc(sizeof(struct valexpr));
	if  (!result)
		abort();
	return  result;
}

struct	valexpr	*make_name(struct valname *vn)
{
	struct	valexpr  *result = alloc_expr();
	result->val_op = VAL_NAME;
	result->val_left = (struct valexpr *) 0;
	result->val_un.val_name = vn;
	return  result;
}


struct	valexpr	*make_value(const int n)
{
	struct	valexpr	*result = alloc_expr();

	result->val_op = VAL_VALUE;
	result->val_left = (struct valexpr *) 0;
	result->val_un.val_value = n;
	return  result;
}

struct	valexpr	*make_sum(struct valname *vn, const char op, const int n)
{
	struct	valexpr	*result = alloc_expr();
	result->val_op = op;
	result->val_left = make_name(vn);
	result->val_un.val_right = make_value(n);
	return  result;
}

static	jmp_buf	jb;

static	long	eval(struct valexpr *ve)
{
	switch  (ve->val_op)  {
	default:
		fprintf(stderr, "Funny operator??\n");
		errors++;
		longjmp(jb, 1);
	case  VAL_NAME:
		if  (!(ve->val_un.val_name->vn_flags & VN_HASVALUE))  {
			fprintf(stderr, "Undefined name %s near line %d\n", ve->val_un.val_name->vn_string, line_count);
			errors++;
			longjmp(jb, 1);
		}
		return  ve->val_un.val_name->vn_value;
	case  VAL_VALUE:
	case  VAL_CHVALUE:
		return  ve->val_un.val_value;
	case  VAL_ROUND:
		{
			long	l = eval(ve->val_left), r = eval(ve->val_un.val_right);
			if  (r <= 1)
				return  l;
			return  (l/r + 1) * r;
		}
	case  '+':
		return  eval(ve->val_left) + eval(ve->val_un.val_right);
	case  '-':
		return  eval(ve->val_left) - eval(ve->val_un.val_right);
	case  '*':
		return  eval(ve->val_left) * eval(ve->val_un.val_right);
	case  '/':
		return  eval(ve->val_left) / eval(ve->val_un.val_right);
	case  '%':
		return  eval(ve->val_left) % eval(ve->val_un.val_right);
	}
}

long	evaluate(struct	valexpr *ve)
{
	if  (setjmp(jb))
		return  UNDEFINED_VALUE;
	return  eval(ve);
}

void	throwaway_expr(struct valexpr *ve)
{
	switch  (ve->val_op)  {
	case  '+':
	case  '-':
	case  '*':
	case  '/':
	case  '%':
	case  VAL_ROUND:
		throwaway_expr(ve->val_left);
		throwaway_expr(ve->val_un.val_right);
	}
	free((char *) ve);
}

/*
 *	Used for both modules and macros
 */

unsigned	mhashcalc(char *name)
{
	unsigned	result = 0;

	while  (*name)
		result = (result << 1) ^ *name++;
	return  result % MACROHASHMOD;
}

void	macro_define(char *name, char *subdir, struct module_list *mlist)
{
	unsigned  mhash = mhashcalc(name);
	struct	macro	*mp;

	for  (mp = macrohashtab[mhash];  mp;  mp = mp->macro_next)
		if  (strcmp(mp->macro_name, name) == 0)  {
			fprintf(stderr, "Macro %s is redefined line %d\n", name, line_count);
			errors++;
			return;
		}

	mp = (struct macro *) malloc(sizeof(struct macro));
	if  (!mp)
		abort();
	mp->macro_next = macrohashtab[mhash];
	macrohashtab[mhash] = mp;
	mp->macro_name = name;
	mp->macro_ml = mlist;
	if  (subdir)
		for  (;  mlist;  mlist = mlist->ml_next)
			if  (!mlist->ml_mod->mod_subdir)
				mlist->ml_mod->mod_subdir = subdir;
}

struct	module_list	*alloc_modlist()
{
	struct	module_list	*res = (struct module_list *) malloc(sizeof(struct module_list));

	if  (!res)
		abort();
	res->ml_next = (struct module_list *) 0;
	res->ml_mod = (struct module *) 0;
	return  res;
}

struct	module_list	*alloc_module(char *name, char *subdir)
{
	unsigned  mhash = mhashcalc(name);
	struct	module	*mp;
	struct	module_list	*result = alloc_modlist();

	for  (mp = modhashtab[mhash]; mp; mp = mp->mod_next)
		if  (strcmp(mp->mod_name, name) == 0)  {
			result->ml_mod = mp;
			if  (!mp->mod_subdir)
				mp->mod_subdir = subdir;
			return  result;
		}

	mp = (struct module *) malloc(sizeof(struct module));
	if  (!mp)
		abort();
	mp->mod_next = modhashtab[mhash];
	modhashtab[mhash] = mp;
	mp->mod_name = name;
	mp->mod_subdir = subdir;
	mp->mod_pl = (struct program_list *) 0;
	mp->mod_scanned = 0;
	result->ml_mod = mp;
	return  result;
}

void	free_modlist(struct module_list *ml)
{
	while  (ml)  {
		struct  module_list  *nxt = ml->ml_next;
		free(ml);
		ml = nxt;
	}
}

struct	module_list	*copy_modlist(struct module_list *src)
{
	struct	module_list	*res = (struct module_list *) 0;

	if  (src)  {
		res = alloc_modlist();
		res->ml_mod = src->ml_mod;
		res->ml_next = copy_modlist(src->ml_next);
	}
	return  res;
}

struct	module_list	*lookupallocmods(char *name)
{
	struct	macro	*mp;

	for  (mp = macrohashtab[mhashcalc(name)];  mp;  mp = mp->macro_next)
		if  (strcmp(name, mp->macro_name) == 0)
			return  copy_modlist(mp->macro_ml);

	return	copy_modlist(alloc_module(name, (char *) 0));
}


struct	helpfile	*find_help(char *name, char *subdir)
{
	int	cnt;

	for  (cnt = 0;  cnt < MAXHELPFILES;  cnt++)  {
		if  (!helpfiles[cnt].hf_name)
			break;
		if  (strcmp(name, helpfiles[cnt].hf_name) != 0)
			continue;
		if  (subdir)  {
			if  (!helpfiles[cnt].hf_subdir)
				continue;
			if  (strcmp(helpfiles[cnt].hf_subdir, subdir) != 0)
				continue;
			free(name);
			free(subdir);
			return  &helpfiles[cnt];
		}
		else  {
			if  (helpfiles[cnt].hf_subdir)
				continue;
			free(name);
			return  &helpfiles[cnt];
		}
	}
	if  (cnt >= MAXHELPFILES)  {
		fprintf(stderr, "Sorry too many help files\n");
		exit(254);
	}
	helpfiles[cnt].hf_name = name;
	helpfiles[cnt].hf_subdir = subdir;
	return  &helpfiles[cnt];
}

struct	program	*find_program(char *name)
{
	int	cnt;
	for  (cnt = 0;  cnt < MAXPROGS;  cnt++)  {
		if  (!programs[cnt].prog_name)
			break;
		if  (strcmp(programs[cnt].prog_name, name) == 0)
			return  &programs[cnt];
	}
	if  (cnt >= MAXPROGS)  {
		fprintf(stderr, "Sorry too many programs\n");
		exit(253);
	}
	programs[cnt].prog_name = stracpy(name);
	programs[cnt].prog_hf = (struct helpfile *) 0;
	return  &programs[cnt];
}

void	define_helpsfor(char *name, char *subdir, struct program_list *pl)
{
	struct	helpfile	*whichh = find_help(name, subdir);
	struct	program		*prog;

	while  (pl)  {
		prog = pl->pl_prog;
		if  (prog->prog_hf)  {
			if  (prog->prog_hf != whichh)  {
				fprintf(stderr, "Program %s has helpfile defined twice as %s and %s\n",
					       prog->prog_name, whichh->hf_name, prog->prog_hf->hf_name);
				errors++;
			}
		}
		else
			prog->prog_hf = whichh;
		pl = pl->pl_next;
	}
}

struct	program_list	*alloc_pl()
{
	struct	program_list	*res = (struct program_list *) malloc(sizeof(struct program_list));
	if  (!res)
		abort();
	res->pl_next = (struct program_list *) 0;
	res->pl_prog = (struct program *) 0;
	return  res;
}

struct	program_list	*alloc_proglist(char *name)
{
	struct	program	*prog = find_program(name);
	struct  program_list  *res = alloc_pl();
	res->pl_prog = prog;
	return  res;
}

void	assign_progmods(struct program *prog, char *subdir, struct module_list *ml)
{
	struct	module	*mod;
	struct	program_list	*pl;

	while  (ml)  {
		mod = ml->ml_mod;
		for  (pl = mod->mod_pl;  pl;  pl = pl->pl_next)
			if  (pl->pl_prog == prog)
				goto  dun;
		pl = alloc_pl();
		pl->pl_prog = prog;
		pl->pl_next = mod->mod_pl;
		mod->mod_pl = pl;
		if  (!mod->mod_subdir)
			mod->mod_subdir = subdir;
	dun:
		ml = ml->ml_next;
	}
}

struct	textlist	*alloc_textlist(char *string)
{
	struct	textlist  *result = (struct textlist*) malloc(sizeof(struct textlist));

	result->tl_text = string;
	result->tl_next = (struct textlist *) 0;
	return  result;
}

void	throwaway_strs(struct textlist *tl)
{
	while  (tl)  {
		struct  textlist  *nxt = tl->tl_next;
		free(tl->tl_text);
		free((char *) tl);
		tl = nxt;
	}
}

void	scanmodules()
{
	unsigned	cnt;
	struct	module	*mp;

	for  (cnt = 0;  cnt < MACROHASHMOD;  cnt++)
		for  (mp = modhashtab[cnt];  mp;  mp = mp->mod_next)
			scanmodule(mp);
}

static	void	merge_helpname_name(struct valname *src, struct valname *dest)
{
	if  (!dest->vn_hlist)	/* Kludge for now */
		dest->vn_hlist = src->vn_hlist;
}

void	merge_helpname(struct valname *src, struct valexpr *dest)
{
	switch  (dest->val_op)  {
	default:
		return;
	case  VAL_NAME:
		merge_helpname_name(src, dest->val_un.val_name);
		return;
	case  VAL_ROUND:
	case  '+':
	case  '-':
	case  '*':
	case  '/':
	case  '%':
		merge_helpname(src, dest->val_left);
		merge_helpname(src, dest->val_un.val_right);
		return;
	}
}

void	merge_helps(struct valexpr *src, struct valexpr *dest)
{
	switch  (src->val_op)  {
	default:
		return;
	case  VAL_NAME:
		merge_helpname(src->val_un.val_name, dest);
		return;
	case  VAL_ROUND:
	case  '+':
	case  '-':
	case  '*':
	case  '/':
	case  '%':
		merge_helps(src->val_left, dest);
		merge_helps(src->val_un.val_right, dest);
		return;
	}
}

