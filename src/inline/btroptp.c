/* btroptp.c -- opt params for gbch-r and friends

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

optparam        optprocs[] = {
o_explain,
o_noverbose,    o_verbose,      o_mail,         o_write,
o_title,        o_cancmailwrite,o_interpreter,  o_priority,
o_loadlev,      o_mode_job,     o_normal,       o_ascanc,
o_notime,       o_time,         o_norepeat,     o_deleteatend,
o_repeat,       o_avoiding,     o_skip,         o_hold,
o_resched,      o_catchup,      o_cancio,       o_io,
o_directory,    o_cancargs,     o_argument,     o_canccond,
o_condition,    o_cancset,      o_flags,        o_set,
o_umask,        o_ulimit,       o_exits,        o_advterr,
o_noadvterr,    o_owner,        o_group,        o_noexport,
o_export,       o_fullexport,   o_condcrit,     o_nocondcrit,
o_asscrit,      o_noasscrit,    o_jobqueue,     o_queuehost,
o_localenv,     o_outsideenv,   o_deltime,      o_runtime,
o_whichsig,     o_gracetime,    o_asdone,       o_freezecd,
o_freezehd
};
