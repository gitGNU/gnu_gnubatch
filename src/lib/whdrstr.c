/* whdrstr.c -- Curses routine to get a header

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
#include <curses.h>

#ifndef HAVE_TERMINFO
#define chtype  int
#endif

void  whdrstr(WINDOW *wp, const char *str)
{
        int     isld = 0, isenh = 0;
        chtype  ch;

        while  (*str)  {
                switch  (*str)  {
                case  '\\':
                        switch  (*++str)  {
                        default:
                                waddch(wp, (chtype) *str);
                                str++;
                                continue;
#ifdef  HAVE_TERMINFO
                        case  '\0':
                                if  (isenh)
                                        wattrset(wp, A_NORMAL);
                                return;
                        case  'B':case  'b':            /* \B set bold */
                                wattron(wp, A_BOLD);
                                isenh++;
                                str++;
                                continue;
                        case  'D':case  'd':            /* \D set dim */
                                wattron(wp, A_DIM);
                                isenh++;
                                str++;
                                continue;
                        case  'I':case  'i':            /* \I set inverse */
                                wattron(wp, A_REVERSE);
                                isenh++;
                                str++;
                                continue;
                        case  'F':case  'f':            /* \F set flashing */
                                wattron(wp, A_BLINK);
                                isenh++;
                                str++;
                                continue;
                        case  'U':case  'u':            /* \U set underline */
                                wattron(wp, A_UNDERLINE);
                                isenh++;
                                str++;
                                continue;
                        case  'S':case  's':            /* \S set standout */
                                wstandout(wp);
                                isenh++;
                                str++;
                                continue;
                        case  'N':case  'n':            /* \N set normal */
                                wattrset(wp, A_NORMAL);
                                isenh = 0;
                                str++;
                                continue;
#else
                        case  '\0':
                                if  (isenh)
                                        wstandend(wp);
                                return;
                        case  'B':case  'b':            /* \B set bold */
                        case  'D':case  'd':            /* \D set dim */
                        case  'I':case  'i':            /* \I set inverse */
                        case  'F':case  'f':            /* \F set flashing */
                        case  'U':case  'u':            /* \U set underline */
                        case  'S':case  's':            /* \S set standout */
                                wstandout(wp);
                                str++;
                                isenh++;
                                continue;

                        case  'N':case  'n':            /* \N set normal */
                                wstandend(wp);
                                str++;
                                isenh = 0;
                                continue;
#endif
                        case  'L':                      /* \L top left */
                        case  'l':                      /* \l bottom left */
                        case  'R':                      /* \R top right */
                        case  'r':                      /* \r bottom right */
                        case  '<':                      /* T left */
                        case  '>':                      /* T right */
                        case  '^':                      /* T top */
                        case  'v':case  'V':            /* T bottom */
                        case  '+':                      /* Intersection */
                        case  '|':                      /* Vertical bar */
                        case  '-':                      /* Horizontal bar */
                                isld = 1;
                                break;
                        }

                case  'L':                      /* \L top left */
                case  'l':                      /* \l bottom left */
                case  'R':                      /* \R top right */
                case  'r':                      /* \r bottom right */
                case  '<':                      /* T left */
                case  '>':                      /* T right */
                case  '^':                      /* T top */
                case  'v':case  'V':            /* T bottom */
                case  '+':                      /* Intersection */
                case  '|':                      /* Vertical bar */
                case  '-':                      /* Horizontal bar */
                case  '.':                      /* Dot turn off */
                        ch = *str++;
                        if  (isld)  switch  (ch)  {
#if     defined(HAVE_TERMINFO) && defined(ACS_ULCORNER)
                        case  'L':                      /* \L top left */
                                ch = ACS_ULCORNER;
                                break;
                        case  'l':                      /* \l bottom left */
                                ch = ACS_LLCORNER;
                                break;
                        case  'R':                      /* \R top right */
                                ch = ACS_URCORNER;
                                break;
                        case  'r':                      /* \r bottom right */
                                ch = ACS_LRCORNER;
                                break;
                        case  '<':                      /* T left */
                                ch = ACS_LTEE;
                                break;
                        case  '>':                      /* T right */
                                ch = ACS_RTEE;
                                break;
                        case  '^':                      /* T top */
                                ch = ACS_TTEE;
                                break;
                        case  'v':case  'V':            /* T bottom */
                                ch = ACS_BTEE;
                                break;
                        case  '+':                      /* Intersection */
                                ch = ACS_PLUS;
                                break;
                        case  '|':                      /* Vertical bar */
                                ch = ACS_VLINE;
                                break;
                        case  '-':                      /* Horizontal bar */
                                ch = ACS_HLINE;
                                break;
#else
                        case  'L':                      /* \L top left */
                        case  'l':                      /* \l bottom left */
                        case  'R':                      /* \R top right */
                        case  'r':                      /* \r bottom right */
                        case  '<':                      /* T left */
                        case  '>':                      /* T right */
                        case  '^':                      /* T top */
                        case  'v':case  'V':            /* T bottom */
                                ch = '+';
                        case  '+':                      /* Intersection */
                        case  '|':                      /* Vertical bar */
                        case  '-':                      /* Horizontal bar */
                                break;
#endif
                        case  '.':
                                isld = 0;
                                continue;
                        }
                        break;

                default:
                        isld = 0;
                        ch = *str++;
                        break;
                }

                waddch(wp, ch);
        }
}

void  mvwhdrstr(WINDOW *wp, const int row, const int col, const char *str)
{
        wmove(wp, row, col);
        whdrstr(wp, str);
}
