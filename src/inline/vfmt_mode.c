/* vfmt_mode.c -- display variable permissions for variable display

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

VFORMAT(fmt_mode)
{
        fmt_t  lng = 0;
        if  (mpermitted(&vp->var_mode, BTM_RDMODE, 0))  {
                lng = fmtmode(lng, "U", vp->var_mode.u_flags);
                lng = fmtmode(lng, ",G", vp->var_mode.g_flags);
                lng = fmtmode(lng, ",O", vp->var_mode.o_flags);
        }
        return  lng;
}
