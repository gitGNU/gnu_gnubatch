/* spopts_jd.c -- spit options for jobdump

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

void  spit_options(FILE *dest, const char *name)
{
        int     cancont = 0, jn;
#ifdef XMBTR_INLINE
        BtjobRef  jp = &default_job;
#else
        BtjobRef  jp = Wj;
#endif
        fprintf(dest, "%s", name);

#ifdef XMBTR_INLINE
        cancont = spitoption(default_pend.Verbose? $A{btr arg verb}: $A{btr arg noverb}, $A{btr arg explain}, dest, '=', cancont);
        cancont = spitoption(jp->h.bj_progress == BJP_DONE? $A{btr arg done}:
                             jp->h.bj_progress != BJP_CANCELLED? $A{btr arg norm}:
                             $A{btr arg canc}, $A{btr arg explain}, dest, ' ', cancont);
#else
        cancont = spitoption(Verbose == VERBOSE? $A{btr arg verb}: $A{btr arg noverb}, $A{btr arg explain}, dest, '=', cancont);
        cancont = spitoption(pardump == ASRUNN? $A{btr arg norm}: $A{btr arg canc}, $A{btr arg explain}, dest, ' ', cancont);
#endif
        cancont = spit_msg(dest, jp, cancont);
        cancont = spitoption(jp->h.bj_jflags & BJ_NOADVIFERR? $A{btr arg noadverr}: $A{btr arg adverr}, $A{btr arg explain}, dest, ' ', cancont);
        cancont = spitoption(jp->h.bj_jflags & BJ_REMRUNNABLE? $A{btr arg fullexport}:
                             jp->h.bj_jflags & BJ_EXPORT? $A{btr arg export}: $A{btr arg loco},
                             $A{btr arg explain}, dest, ' ', cancont);
        cancont = spitoption($A{btr arg cancio}, $A{btr arg explain}, dest, ' ', cancont);
        cancont = spitoption($A{btr arg cancarg}, $A{btr arg explain}, dest, ' ', cancont);
        cancont = spitoption($A{btr arg canccond}, $A{btr arg explain}, dest, ' ', cancont);
        cancont = spitoption($A{btr arg cancset}, $A{btr arg explain}, dest, ' ', cancont);
        cancont = spit_time(dest, &jp->h.bj_times, cancont, OF_AVOID_CHANGES|OF_DATESET, -1);

#ifdef XMBTR_INLINE
        spitoption($A{btr arg setu}, $A{btr arg explain}, dest, ' ', 0);
        fprintf(dest, " %s", prin_uname(default_pend.userid));
        spitoption($A{btr arg setg}, $A{btr arg explain}, dest, ' ', 0);
        fprintf(dest, " %s", prin_gname(default_pend.grpid));
#else
        /* I don't know what to do about environments, I just use the
           one local to the machine being dumped on.  */
        cancont = spitoption($A{btr arg locenv}, $A{btr arg explain}, dest, ' ', cancont);
#endif

        spitoption($A{btr arg queue}, $A{btr arg explain}, dest, ' ', 0);
        putc(' ', dest);

        if  (jp->h.bj_title >= 0)  {
                char    *title = &jp->bj_space[jp->h.bj_title], *colp;
                if  ((colp = strchr(title, ':')))
                        dumpnstr(dest, title, colp - title);
#ifdef XMBTR_INLINE
                else  if  (default_pend.jobqueue)
                        dumpstr(dest, default_pend.jobqueue);
#endif
                else
                        putc('-', dest);
        }
#ifdef XMBTR_INLINE
        else  if  (default_pend.jobqueue)
                dumpstr(dest, default_pend.jobqueue);
#endif
        else
                putc('-', dest);

        spitoption($A{btr arg interp}, $A{btr arg explain}, dest, ' ', 0);
        fprintf(dest, " %s", jp->h.bj_cmdinterp);
        spitoption($A{btr arg pri}, $A{btr arg explain}, dest, ' ', 0);
        fprintf(dest, " %d", jp->h.bj_pri);
        spitoption($A{btr arg ll}, $A{btr arg explain}, dest, ' ', 0);
        fprintf(dest, " %d", jp->h.bj_ll);
        spitoption($A{btr arg mode}, $A{btr arg explain}, dest, ' ', 0);
        dumpmode(dest, " U", jp->h.bj_mode.u_flags);
        dumpmode(dest, ",G", jp->h.bj_mode.g_flags);
        dumpmode(dest, ",O", jp->h.bj_mode.o_flags);
        for  (jn = 0;  jn < MAXCVARS;  jn++)  {
                JcondRef  jcp = &jp->h.bj_conds[jn];
                BtvarRef        vp;
                if  (jcp->bjc_compar == C_UNUSED)
                        break;
                vp = &Var_seg.vlist[jcp->bjc_varind].Vent;
                spit_cond(dest, jcp->bjc_compar, jcp->bjc_iscrit, vp->var_id.hostid, vp->var_name, &jcp->bjc_value);
        }
        for  (jn = 0;  jn < MAXSEVARS;  jn++)  {
                JassRef jap = &jp->h.bj_asses[jn];
                BtvarRef        vp;
                if  (jap->bja_op == BJA_NONE)
                        break;
                vp = &Var_seg.vlist[jap->bja_varind].Vent;
                spit_ass(dest, jap->bja_op, jap->bja_flags, jap->bja_iscrit, vp->var_id.hostid, vp->var_name, &jap->bja_con);
        }
        for  (jn = 0;  jn < (int) jp->h.bj_nredirs;  jn++)  {
                RedirRef  rp = REDIR_OF(jp, jn);
                spitoption($A{btr arg io}, $A{btr arg explain}, dest, ' ', 0);
                spit_redir(dest, rp->fd, rp->action, rp->arg, &jp->bj_space[rp->arg]);
        }
        for  (jn = 0;  jn < (int) jp->h.bj_nargs;  jn++)  {
                char    *cp = ARG_OF(jp, jn);
                spitoption($A{btr arg argument}, $A{btr arg explain}, dest, ' ', 0);
                putc(' ', dest);
                dumpstr(dest, cp);
        }
        spit_pparm(dest, &jp->h, 0xFFFFFFFFL);
        if  (jp->h.bj_direct >= 0)  {
                spitoption($A{btr arg dir}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %s", &jp->bj_space[jp->h.bj_direct]);
        }
        putc('\n', dest);
}
