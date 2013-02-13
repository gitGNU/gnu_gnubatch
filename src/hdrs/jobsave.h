/* jobsave.h -- Layout of saved job file.

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

struct  Jsave   {
        jobno_t         sj_job;
        time_t          sj_time;
        time_t          sj_stime;
        time_t          sj_etime;

        unsigned  char  sj_progress;
        unsigned  char  sj_pri;
        SHORT           sj_wpri;

        USHORT          sj_ll;
        USHORT          sj_umask;

        USHORT          sj_nredirs,
                        sj_nargs,
                        sj_nenv;
        SHORT           sj_redirs;

        SHORT           sj_env;
        SHORT           sj_arg;
        SHORT           sj_title;
        SHORT           sj_direct;

        USHORT          sj_jflags;
        USHORT          sj_autoksig;

        USHORT          sj_runon;
        USHORT          sj_deltime;

        Btmode          sj_mode;
        char            sj_cmdinterp[CI_MAXNAME+1];
        struct Sjcond   {
                unsigned  char  sjc_compar;
                unsigned  char  sjc_crit;
                Vref            sjc_varind;
                Btcon           sjc_value;
        }  sj_conds[MAXCVARS];
        struct  Sjass   {
                USHORT          sja_flags;
                unsigned  char  sja_op;
                unsigned  char  sja_crit;
                Vref            sja_varind;
                Btcon           sja_con;
        }  sj_asses[MAXSEVARS];
        Timecon         sj_times;
        ULONG           sj_runtime;
        LONG            sj_ulimit;
        Exits           sj_exits;
        char            sj_space[JOBSPACE];
};
