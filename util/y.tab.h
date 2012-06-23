/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
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

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NEG = 258,
     DEFINE = 259,
     MODULES = 260,
     INCR = 261,
     DECR = 262,
     ROUND = 263,
     COPY = 264,
     REQUIRE = 265,
     FILENAME = 266,
     STRING = 267,
     COMMENT = 268,
     VALNAME = 269,
     NUMBER = 270,
     HELPTYPE = 271,
     CHNUMBER = 272
   };
#endif
/* Tokens.  */
#define NEG 258
#define DEFINE 259
#define MODULES 260
#define INCR 261
#define DECR 262
#define ROUND 263
#define COPY 264
#define REQUIRE 265
#define FILENAME 266
#define STRING 267
#define COMMENT 268
#define VALNAME 269
#define NUMBER 270
#define HELPTYPE 271
#define CHNUMBER 272




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 49 "msgparse.y"

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



/* Line 2068 of yacc.c  */
#line 99 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


