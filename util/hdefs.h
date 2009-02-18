/* hdefs.h -- helpfile parsing defs

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

#define	MAXHELPTYPES	11	/* Help type letters */
#define	MAXHELPFILES	10	/* Max number of help files */
#define	MAXPROGS	60	/* Max number of programs (not modules) */
#define	HELPHASH	127

struct	filelist  {		/* Currently only used for "REQUIRE" */
	struct	filelist	*next;			/* In chain */
	char			*name;			/* Thing */
};

struct	hadhelp		{	/* See if we've had it */
	struct	hadhelp	*had_next;
	long		had_value;
	long		had_state;
	char		had_char;
};

struct	helpfile	{
	FILE		*hf_filep;			/* Output file when we generate it */
	char		*hf_name;			/* Name (e.g. spq.h) */
	char		*hf_subdir;			/* Subdirectory or null */
	struct	hadhelp	*hf_hash[HELPHASH]; 		/* Hash table for checking purpose */
};

struct	helpfile_list	{
	struct	helpfile_list	*hfl_next;		/* Next in chain */
	struct	helpfile	*hfl_hf;		/* Helpfile referred to */
	unsigned		hfl_flags;		/* Flags giving usage */
};

struct	program	{
	char			*prog_name;		/* Name of program */
	struct	helpfile	*prog_hf;		/* Helpfile it uses */
};

struct	program_list	{
	struct	program_list	*pl_next;		/* Next in chain */
	struct  program		*pl_prog; 		/* Program referred */
};

struct	module		{
	struct	module		*mod_next; 		/* Next in hash chain  */
	char			*mod_name;		/* Name of module */
	char			*mod_subdir;		/* Subdirectory or null */
	struct  program_list	*mod_pl;		/* Programs used in */
	unsigned  short		*mod_scanned; 		/* Done this one */
};

struct	module_list	{
	struct	module_list	*ml_next;		/* Next in list */
	struct	module		*ml_mod;		/* Module referred to */
};

struct	macro		{
	struct	macro		*macro_next; 		/* Next in hash chain */
	char			*macro_name;		/* Name of macro e.g. LIB1 */
	struct	module_list	*macro_ml;		/* Module list involved */
};

struct	valname {
	struct	valname		*vn_next;		/* Next in collision chain */
	char			*vn_string;		/* Name string */
	short			vn_value;		/* Value */
	unsigned  short		vn_flags;		/* Defined things */
#define	VN_DEFDE		(1 << 0)		/* Used in error message */
#define	VN_DEFDH		(1 << 1)		/* Used in help message */
#define	VN_DEFDP		(1 << 2)		/* Used in prompt */
#define	VN_DEFDQ		(1 << 3)		/* Used in alternative */
#define	VN_DEFDA		(1 << 4)		/* Used in arg def */
#define	VN_DEFDK		(1 << 5)		/* Used in key def */
#define	VN_DEFDN		(1 << 6)		/* Used in numeric code no text */
#define	VN_DEFDS		(1 << 7)		/* Used in state code no text */
#define	VN_DEFDX		(1 << 8)		/* Used in execute */
#define	VN_DEFDR		(1 << 14)		/* Default value for Q */
#define	VN_HASVALUE		(1 << 15)		/* Has a value */
	struct	helpfile_list	*vn_hlist;		/* List of helpfiles required for */
};

#define	UNDEFINED_VALUE		-32768

struct	valexpr	{
	unsigned  char		val_op;			/* Operation or as below */
#define	VAL_VALUE	0
#define	VAL_CHVALUE	1
#define	VAL_NAME	2
#define	VAL_ROUND	3
	struct	valexpr		*val_left;		/* LHS of diadic op */
	union  {
		struct	valexpr		*val_right;	/* RHS of diadic op */
		struct	valname		*val_name;	/* Name when name */
		long			val_value;	/* Value when value */
	}  val_un;
};

struct	textlist	{
	struct	textlist	*tl_next;
	char			*tl_text;
};

extern	void	throwaway_expr(struct valexpr *),
		macro_define(char *, char *, struct module_list *),
		free_modlist(struct module_list *),
		define_helpsfor(char *, char *, struct program_list *),
		assign_progmods(struct program *, char *, struct module_list *),
		throwaway_strs(struct textlist *);

extern	char	*stracpy(const char *);

extern	struct	valname	*lookupname(const char *);

extern	struct	valexpr	*alloc_expr(),
			*make_value(const int),
			*make_name(struct valname *),
			*make_sum(struct valname *, const char, const int);

extern	long	evaluate(struct	valexpr *);

extern	struct	module_list	*alloc_modlist(),
				*alloc_module(char *, char *),
				*lookupallocmods(char *);

extern	struct	helpfile	*find_help(char *, char *);

extern	struct	program	*find_program(char *);

extern	struct	program_list	*alloc_proglist(char *);

extern	struct	textlist	*alloc_textlist(char *);

extern	void	scanmodules(),
		scanmodule(struct module *);

