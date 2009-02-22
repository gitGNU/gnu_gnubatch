/* msgparse.y -- grammer for help file parsing

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
%{
#include "config.h"
#include <stdio.h>
#include "incl_unix.h"
#include "hdefs.h"

extern	int	yylex();
extern	void	valname_usage(struct valexpr *, const unsigned);
extern	void	apphelps(char *, struct valexpr *, char *, struct valexpr *, struct filelist *, struct textlist *);
extern	void	merge_helps(struct valexpr *, struct valexpr *);
 
int	errors = 0,
	pass1 = 1;

extern	int	line_count;

struct	valname	*last_assign;
struct	valexpr	*last_expr;
long		last_value;

extern	struct	helpfile	helpfiles[];

static	char	htlist[MAXHELPTYPES];

void	yyerror(char *msg)
{
	fprintf(stderr, "Parse error: %s on line %d\n", msg, line_count);
	errors++;
}

%}

%union {
	char			ch;
	int			num;
	char			*str;
	struct	valname		*vname;
	struct	valexpr		*vexpr;
	struct	module_list	*mlist;
	struct	program		*pgm;
	struct	program_list	*plist;
	struct	textlist	*tlist;
	struct	filelist	*flist;
};

%left <ch>	'+' '-'
%left <ch>	'*' '/' '%'
%left NEG
%token	DEFINE MODULES INCR DECR ROUND COPY REQUIRE
%token	<str>	FILENAME STRING COMMENT
%token	<vname>	VALNAME
%token	<num>	NUMBER
%token	<ch>	HELPTYPE CHNUMBER

%type	<num>	optcopy
%type	<str>	optsubdir helptypes optcomment
%type	<mlist>	module modlist modormacrolist modormacro
%type	<pgm>	defined_program
%type	<plist>	program proglist
%type	<flist>	optrequire reqlist req
%type	<vexpr>	valexpr primary optprimary optassign nameorexpr
%type	<tlist>	textstring textlist

%start	helpfiledefs

%%

helpfiledefs:	definitions helptexts;

definitions:	definition |
		definitions definition;

definition:	macrodef | helpusedin | progmodules | error ';' ;

macrodef:	DEFINE FILENAME optsubdir ':' modlist ';'
		{
			if  (pass1)
				macro_define($2, $3, $5);
			else
				free($2);
		}
		;

modlist:	module |
		modlist module
		{
			if  (pass1)  {
				struct  module_list  *ml;
				for  (ml = $1;  ml->ml_next;  ml = ml->ml_next)
					;
				ml->ml_next = $2;
			}
		}
		;

module:		optsubdir FILENAME
		{
			if  (pass1)
				$$ = alloc_module($2, $1);
			else
				free($2);
		}
		;

helpusedin:	FILENAME optsubdir ':' proglist ';'
		{
			if  (pass1)
				define_helpsfor($1, $2, $4);
			else
				free($1);
		}
		;

proglist:	program |
		proglist program
		{
			if  (pass1)  {
				struct  program_list  *pl;
				for  (pl = $1;  pl->pl_next;  pl = pl->pl_next)
					;
				pl->pl_next = $2;
			}
		}
		;

program:	FILENAME
		{
			if  (pass1)
				$$ = alloc_proglist($1);
			free($1);
		}
		;

progmodules:	MODULES defined_program optsubdir '=' modormacrolist ';'
		{
			if  (pass1)
				assign_progmods($2, $3, $5);
			free_modlist($5);
		}
		;

defined_program:	FILENAME
			{
				struct	program	*res = find_program($1);
				if  (!res)
					yyerror("Undefined program name");
				$$ = res;
				free($1);
			}
			;

modormacrolist:
		modormacro |
		modormacrolist modormacro
		{
			struct  module_list  *ml;
			for  (ml = $1;  ml->ml_next;  ml = ml->ml_next)
				;
			ml->ml_next = $2;
		}
		;

modormacro:	FILENAME
		{
			$$ = lookupallocmods($1);
		}
		;

optsubdir:	/* empty */
		{
			$$ = (char *) 0;
		}
		|
		'(' FILENAME ')'
		{
			if  (pass1)
				$$ = $2;
			else
				free($2);
		}
		;

helptexts:	helptext |
		helptexts helptext;

helptext:	namedef
		|
		helpdefn
		|
		error
		{
			int  lc = line_count;
			do  yylex();
			while  (line_count == lc);
			yyclearin;
		}
		;

namedef:	VALNAME '=' valexpr
		{
			if  (pass1)  {
				if  ($1->vn_flags & VN_HASVALUE)
					yyerror("Symbol value redefined");
				else  {
					$1->vn_value = last_value = evaluate($3);
					$1->vn_flags |= VN_HASVALUE;
				}
			}
			else
				last_value = evaluate($3);
			last_assign = $1;
			if  (last_expr)  {
				throwaway_expr(last_expr);
				last_expr = (struct valexpr *) 0;
			}
			throwaway_expr($3);
		}
		;

helpdefn:	optcomment optrequire optprimary helptypes nameorexpr optcopy textlist
		{
			int	n, htcnt = 0, tlcnt = 0;
			struct  textlist  *tl;
			for  (n = 0;  n < MAXHELPTYPES && $4[n];  n++)
				if  (strchr("EHPAKQRX", htlist[n]))
					htcnt++;
			for  (tl = $7;  tl;  tl = tl->tl_next)
				tlcnt++;
			if  (tlcnt != htcnt)
				yyerror("Help types and text lists do not match");

			if  (pass1) {
				unsigned  uflags = 0;
				for  (n = 0;  n < MAXHELPTYPES && $4[n];  n++)
					switch  ($4[n])  {
					case  'E':	uflags |= VN_DEFDE;	break;
					case  'H':	uflags |= VN_DEFDH;	break;
					case  'P':	uflags |= VN_DEFDP;	break;
					case  'Q':	uflags |= VN_DEFDQ;	break;
					case  'R':	uflags |= VN_DEFDQ|VN_DEFDR;	break;
					case  'A':	uflags |= VN_DEFDA;	break;
					case  'K':	uflags |= VN_DEFDK;	break;
					case  'N':	uflags |= VN_DEFDN;	break;
					case  'S':	uflags |= VN_DEFDS;	break;
					case  'X':	uflags |= VN_DEFDX;	break;
					}
				valname_usage($5, uflags);
			}
			else  {
				if  ($6 && last_expr)
					merge_helps(last_expr, $5);
				apphelps($1, $3, $4, $5, $2, $7);
			}
			if  ($1)
				free($1);
			if  ($2)  {
				struct	filelist  *c = $2, *n;
				do  {
					n = c->next;
					free(c->name);
					free((char *) c);
				}  while  ((c = n));
			}
			throwaway_strs($7);
			if  (last_expr)
				throwaway_expr(last_expr);
			last_expr = $5;
		}
		;

optcomment:	/* empty */
		{	$$ = (char *) 0;	}
		|
		COMMENT
		;

optprimary:	/* empty */
		{
			$$ = (struct valexpr *) 0;
		}
		|
		primary
		;

primary:	'(' valexpr ')'
		{
			$$ = $2;
		}
		;

helptypes:	HELPTYPE
		{
			memset(htlist, '\0', sizeof(htlist));
			htlist[0] = $1;
			$$ = htlist;
		}
		|
		helptypes ',' HELPTYPE
		{
			char  *cp = htlist;
			while  (*cp)
				cp++;
			*cp = $3;
			$$ = htlist;
		}
		;

nameorexpr:	valexpr optassign
		{
			if  ($2)  {
				if  ($1->val_op != VAL_NAME)
					yyerror("Cannot assign to expression");
				else  {
					struct	valname  *vn = $1->val_un.val_name;
					if  (pass1  &&  vn->vn_flags & VN_HASVALUE)
						yyerror("Symbol value redefined in message def");
					vn->vn_flags |= VN_HASVALUE;
					last_assign = vn;
					vn->vn_value = last_value = evaluate($2);
				}
			}
			else
				last_value = evaluate($1);
			$$ = $1;
		}
		;

optcopy:	/* empty */
		{
			$$ = 0;
		}
		|
		COPY
		{
			$$ = 1;
		}
		;

optrequire:	/* empty */
		{
			$$ = (struct filelist *) 0;
		}
		|
		REQUIRE reqlist
		{
			$$ = $2;
		}
		;

reqlist:	req
		|
		reqlist ',' req
		{
			$3->next = $1;
			$$ = $3;
		}
		;

req:		FILENAME
		{
			if  (!($$ = (struct filelist *) malloc(sizeof(struct filelist))))
				abort();
			$$->next = (struct filelist *) 0;
			$$->name = $1;
			if  (pass1)  {
				int	cnt;
				for  (cnt = 0;  cnt < MAXHELPFILES && helpfiles[cnt].hf_name;  cnt++)
					if  (strcmp($1, helpfiles[cnt].hf_name) == 0)
						goto  ok;
				fprintf(stderr, "Unknown help file %s on line %d\n", $1, line_count);
				errors++;
			}
		ok:
			;
		}
		;

optassign:	/* empty */
		{
			$$ = (struct valexpr *) 0;
		}
		|
		INCR
		{
			$$ = make_value(last_value + 1);
		}
		|
		DECR
		{
			$$ = make_value(last_value - 1);
		}
		|
		'=' valexpr
		{
			$$ = $2;
		}
		;

valexpr:	VALNAME
		{
			$$ = make_name($1);
		}
		|
		NUMBER
		{
			$$ = make_value($1);
		}
		|
		'@'
		{
		        $$ = make_value(last_value);
		}
		|
		CHNUMBER
		{
			$$ = make_value($1);
			$$->val_op = VAL_CHVALUE;
		}
		|
		primary
		{
			$$ = $1;
		}
		|
		ROUND '(' valexpr ',' valexpr ')'
		{
			$$ = alloc_expr();
			$$->val_op = VAL_ROUND;
			$$->val_left = $3;
			$$->val_un.val_right = $5;
		}
		|
		valexpr '+' valexpr
		{
			$$ = alloc_expr();
			$$->val_op = $2;
			$$->val_left = $1;
			$$->val_un.val_right = $3;
		}
		|
		valexpr '-' valexpr
		{
			$$ = alloc_expr();
			$$->val_op = $2;
			$$->val_left = $1;
			$$->val_un.val_right = $3;
		}
		|
		'-' valexpr %prec NEG
		{
			$$ = alloc_expr();
			$$->val_op = $1;
			$$->val_un.val_right = $2;
			$$->val_left = alloc_expr();
			$$->val_left->val_op = VAL_VALUE;
			$$->val_left->val_left = (struct valexpr *) 0;
			$$->val_left->val_un.val_value = 0;
		}
		|
		valexpr '*' valexpr
		{
			$$ = alloc_expr();
			$$->val_op = $2;
			$$->val_left = $1;
			$$->val_un.val_right = $3;
		}
		|
		valexpr '/' valexpr
		{
			$$ = alloc_expr();
			$$->val_op = $2;
			$$->val_left = $1;
			$$->val_un.val_right = $3;
		}
		|
		valexpr '%' valexpr
		{
			$$ = alloc_expr();
			$$->val_op = $2;
			$$->val_left = $1;
			$$->val_un.val_right = $3;
		}
		;

textlist:	/*empty*/
		{	$$ = (struct textlist *) 0;	}
		|
		textstring
		{
			$$ = $1;
		}
		|
		textlist ',' textstring
		{
			if  ($1)  {
				struct  textlist  *tl;
				for  (tl = $1;  tl->tl_next;  tl = tl->tl_next)
					;
				tl->tl_next = $3;
				$$ = $1;
			}
			else
				$$ = $3;
		}
		;

textstring:	STRING
		{
			$$ = alloc_textlist($1);
		}
		;

