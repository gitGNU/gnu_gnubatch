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

# Messages

SYS_REQ     =   0       # System-level requests
SYS_REPLY   =   0x040   # Replies regarding system req
JOB_REQ     =   0x100   # Requests regarding jobs
JOB_REPLY   =   0x140   # Replies regarding jobs
VAR_REQ     =   0x200   # Requests regarding vars
VAR_REPLY   =   0x240   # Replies regarding vars
NET_REQ     =   0x300   # Network request
NET_REPLY   =   0x340   # Network reply

J_NOREPLY   =   0       # Do not reply to caller
REP_AMPARENT = J_NOREPLY    # Am parent process - no reply yet
REQ_TYPE    =   0xFC0   # Mask for request type
SHREQ_CODE  =   0x0FFF  # Mask of child bit
SHREQ_CHILD =   0x1000  # Reply comes from child process

# System requests

O_LOGON     =   (SYS_REQ|1)     # Log on to system
O_LOGOFF    =   (SYS_REQ|2)     # Log off
O_STOP      =   (SYS_REQ|3)     # Halt
O_PWCHANGED =   (SYS_REQ|4)     # Password changed

# Replies

O_OK        =   (SYS_REPLY|0)   # All ok
O_CSTOP     =   (SYS_REPLY|1)   # Pass on stop message
O_JREMAP    =   (SYS_REPLY|2)   # Job remap message
O_VREMAP    =   (SYS_REPLY|3)   # Var remap message
O_NOPERM    =   (SYS_REPLY|4)   # Operation not permitted

# Job requests

J_CREATE    =   (JOB_REQ|1)     # Queue job
J_DELETE    =   (JOB_REQ|2)     # Delete job
J_CHANGE    =   (JOB_REQ|3)     # Change job
J_CHOWN     =   (JOB_REQ|4)     # Change owner
J_CHGRP     =   (JOB_REQ|5)     # Change group
J_CHMOD     =   (JOB_REQ|6)     # Change modes
J_KILL      =   (JOB_REQ|7)     # Kill/abort job
J_FORCE     =   (JOB_REQ|8)     # Force job
J_FORCENA   =   (JOB_REQ|9)     # Force job no advance time
J_CHANGED   =   (JOB_REQ|10)    # Remote wants to change job
J_HCHANGED  =   (JOB_REQ|11)    # Remote wants to change job header
J_BCHANGED  =   (JOB_REQ|12)    # Remote advises changes to job
J_BHCHANGED =   (JOB_REQ|13)    # Remote advises changes to job header
J_BOQ       =   (JOB_REQ|14)    # Remote advises job moved to back of queue
J_BFORCED   =   (JOB_REQ|15)    # Remote advises job forced
J_BFORCEDNA =   (JOB_REQ|16)    # Remote advises job forced - no adv time
J_DELETED   =   (JOB_REQ|17)    # Remote advises Delete job
J_CHMOGED   =   (JOB_REQ|18)    # Remote advises change mode/owner/group

J_STOK      =   (JOB_REQ|19)    # Child process - started ok
J_NOFORK    =   (JOB_REQ|20)    # Child process - fork fail
J_COMPLETED =   (JOB_REQ|21)    # Child process - job completed

J_PROPOSE   =   (JOB_REQ|22)    # Propose to execute remote job
J_PROPOK    =   (JOB_REQ|23)    # Proposal OK (internal network message)
J_CHSTATE   =   (JOB_REQ|24)    # Network message - job changed state
J_RNOTIFY   =   (JOB_REQ|25)    # Network message - notify status
J_RRCHANGE  =   (JOB_REQ|26)    # Remote run changed state

J_RESCHED   =   (JOB_REQ|27)    # Network message - do resched after var hacks
J_RESCHED_NS=   (JOB_REQ|28)    # Network message - do resched after var hacks no start

J_DSTADJ    =   (JOB_REQ|29)    # Adjust DST

# Replies

J_OK        =   (JOB_REPLY|0)   # Ok
J_NEXIST    =   (JOB_REPLY|1)   # Job does not exist
J_VNEXIST   =   (JOB_REPLY|2)   # Job variable does not exist
J_NOPERM    =   (JOB_REPLY|3)   # Operation not permitted
J_VNOPERM   =   (JOB_REPLY|4)   # Operation not permitted on var
J_NOPRIV    =   (JOB_REPLY|5)   # Operation not privileged
J_SYSVAR    =   (JOB_REPLY|6)   # Affects system var incorrectly
J_SYSVTYPE  =   (JOB_REPLY|7)   # Affects system var incorrectly
J_FULLUP    =   (JOB_REPLY|8)   # Choc-a-bloc
J_ISRUNNING =   (JOB_REPLY|9)   # Job running
J_REMVINLOCJ =  (JOB_REPLY|10)  # Remote variable in local job
J_LOCVINEXPJ =  (JOB_REPLY|11)  # Local variable in export job
J_MINPRIV   =   (JOB_REPLY|12)  # Too few privs
J_ISSKEL    =   (JOB_REPLY|13)  # Incomplete job cannot do it

J_START     =   (JOB_REPLY|16)  # Start job

# Variable related requests

V_CREATE    =   (VAR_REQ|1)     # Create variable
V_DELETE    =   (VAR_REQ|2)     # Delete variable
V_ASSIGN    =   (VAR_REQ|3)     # Assign variable
V_CHOWN     =   (VAR_REQ|4)     # Change owner
V_CHGRP     =   (VAR_REQ|5)     # Change group
V_CHMOD     =   (VAR_REQ|6)     # Change modes
V_CHCOMM    =   (VAR_REQ|7)     # Change comment
V_NEWNAME   =   (VAR_REQ|8)     # Change name
V_CHFLAGS   =   (VAR_REQ|9)     # Change flags

V_DELETED   =   (VAR_REQ|16)    # Broadcast variable deleted
V_ASSIGNED  =   (VAR_REQ|17)    # Broadcast variable assigned
V_CHMOGED   =   (VAR_REQ|18)    # Broadcast variable change mode/owner/group
V_RENAMED   =   (VAR_REQ|19)    # Broadcast variable change name

# Replies to above

V_OK        =   (VAR_REPLY|0)   # OK
V_EXISTS    =   (VAR_REPLY|1)   # Variable exists
V_NEXISTS   =   (VAR_REPLY|2)   # Variable does not exist
V_CLASHES   =   (VAR_REPLY|3)   # Variable clashes with similar
V_NOPERM    =   (VAR_REPLY|4)   # Operation not permitted
V_NOPRIV    =   (VAR_REPLY|5)   # Operation not privileged
V_SYNC      =   (VAR_REPLY|6)   # Conflict with another user
V_SYSVAR    =   (VAR_REPLY|7)   # Clashes with system variable
V_SYSVTYPE  =   (VAR_REPLY|8)   # System variable wrong type
V_FULLUP    =   (VAR_REPLY|9)   # Choc-a-bloc
V_DSYSVAR   =   (VAR_REPLY|10)  # Attempt to delete system var
V_INUSE     =   (VAR_REPLY|11)  # Variable in use
V_MINPRIV   =   (VAR_REPLY|12)  # Too few privs
V_DELREMOTE =   (VAR_REPLY|13)  # Deleting remote var
V_UNKREMUSER=   (VAR_REPLY|14)  # Unknown remote user
V_UNKREMGRP =   (VAR_REPLY|15)  # Unknown remote group
V_RENEXISTS =   (VAR_REPLY|16)  # Variable exists trying to rename
V_NOTEXPORT =   (VAR_REPLY|17)  # Variable is not exported
V_RENAMECLUST=  (VAR_REPLY|18)  # Attempt to rename cluster var

# Network requests

N_SHUTHOST  =   (NET_REQ|0)     # Tell everyone we're dying
N_ABORTHOST =   (NET_REQ|1)     # Worked out that someone has died
N_NEWHOST   =   (NET_REQ|2)     # New host arrived (internal)
N_TICKLE    =   (NET_REQ|3)     # Check still alive
N_CONNECT   =   (NET_REQ|4)     # Attempt connection
N_DISCONNECT=   (NET_REQ|5)     # Attempt disconnection
N_PCONNOK   =   (NET_REQ|6)     # Probe connect ok (internal)
N_REQSYNC   =   (NET_REQ|7)     # Request sync
N_ENDSYNC   =   (NET_REQ|8)     # End sync
N_REQREPLY  =   (NET_REQ|9)     # Reply to user's request
N_DOCHARGE  =   (NET_REQ|10)    # Send charge record
N_WANTLOCK  =   (NET_REQ|11)    # Want remote lock
N_UNLOCK    =   (NET_REQ|12)    # Want remote unlock
N_SYNCSINGLE=   (NET_REQ|13)    # Sync a single job
N_RJASSIGN  =   (NET_REQ|15)    # Network message - given job remote assigns
N_XBNATT    =   (NET_REQ|17)    # Request to attach xbnetserv process
N_ROAMUSER  =   (NET_REQ|18)    # Note user logged in to xbnetserv

N_CONNOK    =   (NET_REPLY|0)   # Connection ok
N_REMLOCK_NONE= (NET_REPLY|1)   # Lock - no reply
N_REMLOCK_OK=   (NET_REPLY|2)   # Lock - done ok
N_REMLOCK_PRIO= (NET_REPLY|3)   # Lock - other machine has priority
N_NOFORK    =   (NET_REPLY|4)   # User reply - no fork
N_NBADMSGQ  =   (NET_REPLY|5)   # User reply - bad message queue
N_NTIMEOUT  =   (NET_REPLY|6)   # User reply - network timeout
N_HOSTOFFLINE=  (NET_REPLY|7)   # User reply - host off line
N_HOSTDIED  =   (NET_REPLY|8)   # User reply - host died
N_CONNFAIL  =   (NET_REPLY|9)   # User reply - connection failed
N_WRONGIP   =   (NET_REPLY|10)  # Wrong IP address

Message_lookup = {
    J_NEXIST:        "Job does not exist",
    J_VNEXIST:       "Job variable does not exist",
    J_NOPERM:        "Operation not permitted",
    J_VNOPERM:       "Operation not permitted on var",
    J_NOPRIV:        "Operation not privileged",
    J_SYSVAR:        "Affects system var incorrectly",
    J_SYSVTYPE:      "Affects system var incorrectly - wrong type",
    J_FULLUP:        "Server disk/memory exhausted",
    J_ISRUNNING:     "Job running",
    J_REMVINLOCJ:    "Remote variable in local job",
    J_LOCVINEXPJ:    "Local variable in export job",
    J_MINPRIV:       "Too few privs",
    J_ISSKEL:        "Incomplete job cannot do it",
    V_EXISTS:        "Variable exists",
    V_NEXISTS:       "Variable does not exist",
    V_CLASHES:       "Variable clashes with similar",
    V_NOPERM:        "Operation not permitted",
    V_NOPRIV:        "Operation not privileged",
    V_SYNC:          "Conflict with another user",
    V_SYSVAR:        "Clashes with system variable",
    V_SYSVTYPE:      "System variable wrong type",
    V_FULLUP:        "Choc-a-bloc",
    V_DSYSVAR:       "Attempt to delete system var",
    V_INUSE:         "Variable in use",
    V_MINPRIV:       "Too few privs",
    V_DELREMOTE:     "Deleting remote var",
    V_UNKREMUSER:    "Unknown remote user",
    V_UNKREMGRP:     "Unknown remote group",
    V_RENEXISTS:     "Variable exists trying to rename",
    V_NOTEXPORT:     "Variable is not exported",
    V_RENAMECLUST:   "Attempt to rename cluster var",
    N_REMLOCK_NONE:  "Lock - no reply",
    N_REMLOCK_OK:    "",         # done ok
    N_REMLOCK_PRIO:  "Lock - other machine has priority",
    N_NOFORK:        "User reply - no fork",
    N_NBADMSGQ:      "User reply - bad message queue",
    N_NTIMEOUT:      "User reply - network timeout",
    N_HOSTOFFLINE:   "User reply - host off line",
    N_HOSTDIED:      "User reply - host died",
    N_CONNFAIL:      "User reply - connection failed",
    N_WRONGIP:       "Wrong IP address"
}

def lookupcode(code):
    """Look up message corresponding to supplied code

    Empty string if all OK"""

    if code == J_OK or code == V_OK or code == N_CONNOK: return ""

    mtype = code & REQ_TYPE
    if mtype == SYS_REPLY:
        if code == O_NOPERM: return "Operation not permitted"
        return ""
    try:
        return Message_lookup[code]
    except KeyError:
        return "Unknown message code %d (0x%x)" % (code, code)

