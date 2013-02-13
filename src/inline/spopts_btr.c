/* spopts_btr.c -- spit options for gbch-r and friends

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

        fprintf(dest, "%s", name);
        cancont = spitoption(Verbose? $A{btr arg verb}: $A{btr arg noverb}, $A{btr arg explain}, dest, '=', cancont);
        cancont = spitoption(JREQ->h.bj_progress == BJP_DONE? $A{btr arg done}:
                             JREQ->h.bj_progress != BJP_CANCELLED? $A{btr arg norm}:
                             $A{btr arg canc}, $A{btr arg explain}, dest, ' ', cancont);
        cancont = spit_msg(dest, JREQ, cancont);
        cancont = spitoption(JREQ->h.bj_jflags & BJ_NOADVIFERR? $A{btr arg noadverr}: $A{btr arg adverr}, $A{btr arg explain}, dest, ' ', cancont);
        cancont = spitoption(JREQ->h.bj_jflags & BJ_REMRUNNABLE? $A{btr arg fullexport}:
                             JREQ->h.bj_jflags & BJ_EXPORT? $A{btr arg export}: $A{btr arg loco},
                             $A{btr arg explain}, dest, ' ', cancont);
        cancont = spitoption($A{btr arg cancio}, $A{btr arg explain}, dest, ' ', cancont);
        cancont = spitoption($A{btr arg cancarg}, $A{btr arg explain}, dest, ' ', cancont);
        cancont = spitoption($A{btr arg canccond}, $A{btr arg explain}, dest, ' ', cancont);
        cancont = spitoption($A{btr arg cancset}, $A{btr arg explain}, dest, ' ', cancont);
        cancont = spitoption(Outside_env? $A{btr arg remenv}: $A{btr arg locenv}, $A{btr arg explain}, dest, ' ', cancont);
        cancont = spit_time(dest, &JREQ->h.bj_times, cancont, Timechanges, Timechanges & OF_MDSET? 1: 0);
        if  (Dispflags & DF_HAS_HDR  &&  job_title  &&  job_title[0])  {
                spitoption($A{btr arg title}, $A{btr arg explain}, dest, ' ', 0);
                putc(' ', dest);
                dumpstr(dest, job_title);
        }
        if  (jobqueue)  {
                spitoption($A{btr arg queue}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " \'%s\'", jobqueue);
        }
#ifdef  BTR_INLINE
        if  (Procparchanges & OF_INTERP_CHANGES)  {
                spitoption($A{btr arg interp}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %s", JREQ->h.bj_cmdinterp);
        }
#endif
#ifdef  RBTR_INLINE
        if  (Out_interp && Out_interp[0])  {
                spitoption($A{btr arg interp}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %s", Out_interp);
        }
#endif
        if  (Procparchanges & OF_PRIO_CHANGES)  {
                spitoption($A{btr arg pri}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %d", JREQ->h.bj_pri);
        }
        if  (Procparchanges & OF_LOADLEV_CHANGES)  {
                spitoption($A{btr arg ll}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %d", JREQ->h.bj_ll);
        }
        if  (Mode_set[0] != MODE_NONE || Mode_set[1] != MODE_NONE || Mode_set[2] != MODE_NONE)  {
#ifdef RBTR_INLINE
                char    sep = ' ';
#endif
                spitoption($A{btr arg mode}, $A{btr arg explain}, dest, ' ', 0);
#ifdef BTR_INLINE
                dumpmode(dest, " U", JREQ->h.bj_mode.u_flags);
                dumpmode(dest, ",G", JREQ->h.bj_mode.g_flags);
                dumpmode(dest, ",O", JREQ->h.bj_mode.o_flags);
#endif
#ifdef RBTR_INLINE
                if  (Mode_set[0] != MODE_NONE)  {
                        dumpemode(dest, sep, 'U', Mode_set[0], JREQ->h.bj_mode.u_flags);
                        sep = ',';
                }
                if  (Mode_set[1] != MODE_NONE)  {
                        dumpemode(dest, sep, 'G', Mode_set[1], JREQ->h.bj_mode.g_flags);
                        sep = ',';
                }
                if  (Mode_set[2] != MODE_NONE)
                        dumpemode(dest, sep, 'O', Mode_set[2], JREQ->h.bj_mode.o_flags);
#endif
        }
        for  (jn = 0;  jn < Condcnt;  jn++)  {
                struct  scond   *sc = &Condlist[jn];
                spit_cond(dest, sc->compar, sc->vd.crit, sc->vd.hostid, sc->vd.var, &sc->value);
        }
        for  (jn = 0;  jn < Asscnt;  jn++)  {
                struct  Sass    *as = &Asslist[jn];
                spit_ass(dest, as->op, as->flags, as->vd.crit, as->vd.hostid, as->vd.var, &as->con);
        }
        for  (jn = 0;  jn < Redircnt;  jn++)  {
                MredirRef       rp = &Redirs[jn];
                spitoption($A{btr arg io}, $A{btr arg explain}, dest, ' ', 0);
                spit_redir(dest, rp->fd, rp->action, rp->un.arg, rp->un.buffer);
        }
        for  (jn = 0;  jn < Argcnt;  jn++)  {
                spitoption($A{btr arg argument}, $A{btr arg explain}, dest, ' ', 0);
                putc(' ', dest);
                dumpstr(dest, Args[jn]);
        }
#ifdef BTR_INLINE
        if  (Oreq.sh_params.uuid != Realuid)  {
                spitoption($A{btr arg setu}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %s", prin_uname((uid_t) Oreq.sh_params.uuid));
        }
        if  (Oreq.sh_params.ugid != Realgid)  {
                spitoption($A{btr arg setg}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %s", prin_gname((gid_t) Oreq.sh_params.ugid));
        }
#endif
#ifdef RBTR_INLINE
        if  (Repluid != Realuid)  {
                spitoption($A{btr arg setu}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %s", prin_uname(Repluid));
        }
        if  (Replgid != Realgid)  {
                spitoption($A{btr arg setg}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %s", prin_gname(Replgid));
        }
#endif
        spit_pparm(dest, &JREQ->h, Procparchanges);
#ifdef BTR_INLINE
        if  (strcmp(job_cwd, Curr_pwd) != 0)
#endif
#ifdef  RBTR_INLINE
        if  (job_cwd && job_cwd[0])
#endif
        {
                spitoption($A{btr arg dir}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %s", job_cwd);
        }
        if  (Out_host)  {
                spitoption($A{btr arg host}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %s", look_host(Out_host));
        }
        putc('\n', dest);
}
