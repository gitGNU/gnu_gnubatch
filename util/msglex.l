%{
/* msglex.l -- lex script for helpfile parser

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
#include "incl_unix.h"
#include "hdefs.h"
#include "y.tab.h"

int	line_count = 1;
extern	int	errors;
static	char	filenbuf[256];
%}

%%

#.*$

[EHAPQRKNSX]{1}		yylval.ch = yytext[0];	return  HELPTYPE;

DEFINE			return DEFINE;
MODULES			return MODULES;
ROUND			return ROUND;
COPY			return COPY;
REQUIRE			return REQUIRE;

[a-zA-Z][a-zA-Z_][-a-zA-Z0-9_/.]+	{
						if  (yyleng >= sizeof(filenbuf)-1)
							strncpy(filenbuf, yytext, sizeof(filenbuf)-1);
						else
							strcpy(filenbuf, yytext);
						yylval.str = stracpy(filenbuf);
						return  FILENAME;
					}

"++"				return  INCR;

"--"				return  DECR;

[-+*/%;:,(){}=@]		yylval.ch = yytext[0];	return  yytext[0];


"{"[^}]*"}"			{  yytext[yyleng-1] = '\0';
				   yylval.vname = lookupname(yytext+1);
				   yytext[yyleng-1] = '}';
				   return  VALNAME;
			        }

\'.\'				yylval.ch = yytext[1];		return  CHNUMBER;

[0-9]+				yylval.num = atoi(yytext);	return  NUMBER;

[ \t]+

\n			line_count++;

\"			{
				int	ch;
				int	start_line = line_count;
				unsigned  lng = 0;
				char	inbuf[2048];

				for  (;;)  {
					ch = input();
					if  (ch == EOF)
						yyterminate();
					if  (ch == '\n')
						line_count++;
					else  if  (ch == '\\')  {
						ch = input();
						if  (ch == EOF)
							yyterminate();
						if  (ch == '\n')  {
							line_count++;
							continue;
						}
						if  (ch != '\"')
							inbuf[lng++] = '\\';
					}
					else  if  (ch == '\"')  {
						ch = input();
						if  (ch != '\"')
							break;
					}
					inbuf[lng++] = ch;
					if  (lng >= sizeof(inbuf) - 2)  {
						fprintf(stderr, "String overflow/unterminated on line %d started line %d\n",
							       line_count, start_line);
						errors++;
						while  (ch != '\"'  &&  ch != EOF)  {
							ch = input();
							if  (ch == '\n')
								line_count++;
						}
						if  (ch == EOF)
							yyterminate();
						break;
					}
				}
				unput(ch);
				inbuf[lng] = '\0';
				yylval.str = stracpy(inbuf);
				return  STRING;
			}

"["			{
				int	ch;
				int	start_line = line_count;
				unsigned  lng = 0;
				char	inbuf[2048];

				for  (;;)  {
					ch = input();
					if  (ch == EOF)
						yyterminate();
					if  (ch == '\n')
						line_count++;
					else  if  (ch == '\\')  {
						ch = input();
						if  (ch == EOF)
							yyterminate();
						if  (ch == '\n')  {
							line_count++;
							continue;
						}
						if  (ch != ']')
							inbuf[lng++] = '\\';
					}
					else  if  (ch == ']')  {
						ch = input();
						break;
					}
					inbuf[lng++] = ch;
					if  (lng >= sizeof(inbuf) - 2)  {
						fprintf(stderr, "Comment overflow/unterminated on line %d started line %d\n",
							       line_count, start_line);
						errors++;
						while  (ch != ']'  &&  ch != EOF)  {
							ch = input();
							if  (ch == '\n')
								line_count++;
						}
						if  (ch == EOF)
							yyterminate();
						break;
					}
				}
				unput(ch);
				inbuf[lng] = '\0';
				yylval.str = stracpy(inbuf);
				return  COMMENT;
			}

.		{
			fprintf(stderr, "Had a \'%c\' on line %d\n", yytext[0], line_count);
			errors++;
		}
%%

void	resetlex()
{
	yyrestart(yyin);
}
