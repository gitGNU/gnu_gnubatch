/* btconst.h -- format of "constant" (variable value and in assign/condition)

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

#define BTC_VALUE       49              /* Maximum string length */

typedef struct  {
        SHORT   const_type;
#define CON_NONE        0
#define CON_LONG        1
#define CON_STRING      2
        union   {
                char    con_string[BTC_VALUE+1];
                LONG    con_long;
        }  con_un;
}  Btcon, *BtconRef;

typedef const   Btcon   *CBtconRef;

/* Results of comparisons */

#define COMPAR_LT       0
#define COMPAR_EQ       1
#define COMPAR_GT       2
#define COMPAR_UNDEF    3
