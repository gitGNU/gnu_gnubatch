#!/bin/sh
#
# Copyright (c) 2007 Javier Fernandez-Sanguino <jfs@debian.org>
#
# This is free software; you may redistribute it and/or modify
# it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2,
# or (at your option) any later version.
#
# This is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License with
# the Debian operating system, in /usr/share/common-licenses/GPL;  if
# not, write to the Free Software Foundation, Inc., 59 Temple Place,
# Suite 330, Boston, MA 02111-1307 USA
#
### BEGIN INIT INFO
# Provides:          gnubatch
# Required-Start:    $network $local_fs $remote_fs
# Required-Stop:     $network $local_fs $remote_fs
# Should-Start:      $named
# Should-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: GNUbatch batch scheduler
# Description:       GNUbatch is a batch scheduling system
#                    with advanced dependency control and
#                    network capability
### END INIT INFO

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin

DAEMON=/usr/bin/gbch-start # Introduce the server's location here
NAME=gnubatch              # Introduce the short server's name here
DESC='Batch scheduler process'              # Introduce a short description here
SPOOLDIR=/var/spool/gnubatch

test -x $DAEMON || exit 0

. /lib/lsb/init-functions

# Default options, these can be overriden by the information
# at /etc/default/$NAME
DAEMON_OPTS="-j 1000 -v 500"     # Additional options given to the server

DIETIME=10              # Time to wait for the server to die, in seconds
                        # If this value is set too low you might not
                        # let some servers to die gracefully and
                        # 'restart' will not work

STARTTIME=2             # Time to wait for the server to start, in seconds
                        # If this value is set each time the server is
                        # started (on start or restart) the script will
                        # stall to try to determine if it is running
                        # If it is not set and the server takes time
                        # to setup a pid file the log message might 
                        # be a false positive (says it did not start
                        # when it actually did)
                        
# Include defaults if available
if [ -f /etc/default/$NAME ] ; then
	. /etc/default/$NAME
fi

running()
{
    nproc=`pgrep btsched|wc -l`
    if [ "$nproc" -gt 0 ]
    then
	return 0
    else
	return 1
    fi
}

start_server() {
	# Delete previous memory-mapped stuff which might have got left behind
	rm -f $SPOOLDIR/btmm*
	# Start up with initial job and variable allocation from default
	$DAEMON $DAEMON_OPTS 2>/dev/null
}

stop_server() {
    /usr/bin/gbch-quit -y 2>/dev/null
}

force_stop() {
# Force the process to die killing it manually
	if running ; then
		pkill -15 btsched
		pkill -15 xbnetserv
	# Is it really dead?
		sleep "$DIETIME"
		if running ; then
			pkill -9 btsched
			pkill -9 xbnetserv
			/usr/sbin/gbch-ripc -d >/dev/null 2>&1
			sleep "$DIETIME"
			if running ; then
				echo "Cannot kill $NAME!"
				exit 1
			fi
		fi
	fi
}


case "$1" in
  start)
	log_daemon_msg "Starting $DESC " "$NAME"

        # Check if it's running first

        if running ;  then
            log_progress_msg "apparently already running"
            log_end_msg 0
            exit 0
        fi

	if ! grep -qs '^gnubatch' /etc/services ; then
	    log_daemon_msg "You need ports set up in /etc/services"
	    log_daemon_msg "- see /usr/share/doc/gnubatch/README.Debian"
            log_end_msg 1
	    exit 0
	fi

        if start_server ; then
            [ -n "$STARTTIME" ] && sleep $STARTTIME # Wait some time 
            if  running ;  then
                # It's ok, the server started and is running
                log_end_msg 0
            else
                # It is not running after we did start
                log_end_msg 1
            fi
        else
            # Either we could not start it
            log_end_msg 1
        fi
	;;
  stop)
        log_daemon_msg "Stopping $DESC" "$NAME"
        if running ; then
            stop_server
            log_end_msg 0
        else
            # If it's not running don't do anything
            log_progress_msg "apparently not running"
            log_end_msg 0
            exit 0
        fi
        ;;
  force-stop)
        # First try to stop the program gracefully
        $0 stop
        if running; then
            # If it's still running try to kill it more forcefully
            log_daemon_msg "Stopping (force) $DESC" "$NAME"
            force_stop
            log_end_msg 0
        fi
	;;
  restart|force-reload)
        log_daemon_msg "Restarting $DESC" "$NAME"
        stop_server
        # Wait some sensible amount, some server need this
        [ -n "$DIETIME" ] && sleep $DIETIME
        start_server
        [ -n "$STARTTIME" ] && sleep $STARTTIME
	errcode=0
        running || errcode=$?
        log_end_msg $errcode
	;;
  status)

        log_daemon_msg "Checking status of $DESC" "$NAME"
        if running ;  then
            log_progress_msg "running"
            log_end_msg 0
        else
            log_progress_msg "apparently not running"
            log_end_msg 1
            exit 1
        fi
        ;;
  # Use this if the daemon cannot reload
  reload)
        log_warning_msg "Reloading $NAME daemon: not implemented, as the daemon"
        log_warning_msg "cannot re-read the config file (use restart)."
        ;;
  *)
	N=/etc/init.d/$NAME
	echo "Usage: $N {start|stop|force-stop|restart|force-reload|status}" >&2
	exit 1
	;;
esac

exit 0
