/* freezecode.c -- code for "freeze" options

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

if  (Anychanges & OF_ANY_FREEZE_WANTED)  {
        int     ret1 = 0, ret2 = 0;
        if  (Anychanges & OF_ANY_FREEZE_CURRENT)  {
                char    *varname = make_varname();
                if  (!Curr_pwd  &&  !(Curr_pwd = getenv("PWD")))
                        Curr_pwd = runpwd();
                if  ((ret1 = proc_save_opts(Curr_pwd, varname, spit_options)) != 0)  {
                        disp_str = Curr_pwd;
                        print_error(ret1);
                }
        }
        if  (Anychanges & OF_ANY_FREEZE_HOME)  {
                char    *varname = make_varname();
                if  ((ret2 = proc_save_opts((const char *) 0, varname, spit_options)) != 0)  {
                        disp_str = "(Home)";
                        print_error(ret2);
                }
        }
#ifdef  FREEZE_EXIT
        if  (argv[0] == (char *) 0)
                exit(ret1 != 0  ||  ret2 != 0? E_SETUP: 0);
#else
        if  (ret1  ||  ret2)
                exit(E_SETUP);
#endif
}
