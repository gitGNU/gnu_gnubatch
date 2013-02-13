/* btenvir.h -- Environment variables and arguments

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

typedef struct  {
        USHORT          e_name;                 /* Offset of name */
        USHORT          e_value;                /* Offset of value */
}  Envir, *EnvirRef;

typedef const   Envir   *CEnvirRef;

typedef struct  {
        char    *e_name;                        /* As strings */
        char    *e_value;
}  Menvir, *MenvirRef;

typedef const   Menvir  *CMenvirRef;

typedef USHORT  Jarg, *JargRef;
typedef const   USHORT  *CJargRef;

/* Extract Args and Environment from job */

#define ARG_OF(jp, cnt) &jp->bj_space[((JargRef) &jp->bj_space[jp->h.bj_arg])[cnt]]
extern  void    unpackenv();
#define ENV_OF(jp, cnt, name, value)    unpackenv(jp, cnt, &name, &value)

/* Only used by clients but define here */

extern  Menvir  Envs[];
