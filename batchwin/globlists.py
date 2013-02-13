#   Copyright 2013 Free Software Foundation, Inc.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Master job and variable lists
# Uses "ident" as hash to identify jobs and variables

import gbident
import time

Job_list = dict()
Var_list = dict()
Var_names = dict()

def addjob(job):
    """Add job to list"""
    global Job_list
    job.queuetime = time.time()     # Aid sorting
    Job_list[job] = job

def deljob(jid):
    """Delete a job (specified by id). Not an error if not there."""
    global Job_list
    # Try to delete it but don't worry if it's not there
    try:
        del Job_list[jid]
    except KeyError:
        pass

def addvar(var):
    """Add var details to list"""
    global Var_list
    global Var_names
    Var_list[var] = var
    sv = set((var,))
    try:
        Var_names[var.var_name] |= sv
    except KeyError:
        Var_names[var.var_name] = sv

def delvar(vid):
    """Delete var details from list"""
    global Var_list
    try:
        v = Var_list[vid]
        Var_names[v.var_name] -= set((v,))
        del Var_list[vid]
    except KeyError:
        pass

def jobsfor(ip):
    """Get a list of jobs for the given ip"""
    global Job_list
    return [j for j in Job_list.values() if j.samehost(ip)]

def varsfor(ip):
    """Get a list of variables for the given ip"""
    global Var_list
    return [v for v in Var_list.values() if v.samehost(ip)]

def get_queue_names():
    """Get a list of queue names we know about"""
    global Job_list
    result = set()
    for j in Job_list.values():
        try:
            if j.isvisible:
                tit = j.bj_title.split(':',1)
                if len(tit) > 1: result.add(tit[0])
        except AttributeError:
            pass
    return sorted(list(result))

def lookup_var(hostid, varname):
    """Look up a variable by name"""
    global Var_names
    try:
        vs = Var_names[varname]
    except KeyError:
        return None
    vl = [v for v in vs if v.samehost(hostid)]
    if len(vl) == 1: return vl[0]
    return None

    