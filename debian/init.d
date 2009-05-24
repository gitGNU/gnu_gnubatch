#! /bin/sh
#

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/bin/gbch-start
NAME=GNUbatch
DESC="GNUbatch batch scheduler"
LABEL=GNUbatch
SPOOLDIR=/var/spool/gnubatch

test -x $DAEMON || exit 0

LOGDIR=/var/log/gnubatch
PIDFILE=/var/run/$NAME.pid
DODTIME=10                  # Time to wait for the server to die, in seconds
                            # If this value is set too low you might not
                            # let some servers to die gracefully and
                            # 'restart' will not work

# Include gnubatch defaults if available
if [ -f /etc/default/gnubatch ] ; then
	. /etc/default/gnubatch
fi

set -e

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

force_stop() {
# Forcefully kill the thing
    if running ; then
        pkill -15 btsched xbnetserv
        # Is it really dead?
        [ -n "$DODTIME" ] && sleep "$DODTIME"
        if running ; then
            pkill -9 btsched xbnetserv
            [ -n "$DODTIME" ] && sleep "$DODTIME"
            if running ; then
                echo "Cannot kill $LABEL!"
                exit 1
            fi
        fi
    fi
    return 0
}

case "$1" in
  start)
	echo -n "Starting $DESC: "

	# Delete previous memory-mapped stuff which might have got left behind

	rm -f $SPOOLDIR/btmm*

	# Start up with initial job and variable allocation from default
	
	$DAEMON $DAEMON_OPTS 2>/dev/null
        [ -n "$DODTIME" ] && sleep "$DODTIME"

        if running ; then
            echo "$NAME."
        else
            echo " ERROR."
        fi
	;;

  stop)
	echo -n "Stopping $DESC: "
	/usr/bin/gbch-quit -y 2>/dev/null
	echo "$NAME."
	;;

  force-stop)
	echo -n "Forcefully stopping $DESC: "
        force_stop
        if ! running ; then
            echo "$NAME."
        else
            echo " ERROR."
        fi
	;;

  force-reload)
	running && $0 restart || exit 0
	;;

  restart)
    echo -n "Restarting $DESC: "
	$0 stop
	[ -n "$DODTIME" ] && sleep $DODTIME
	$0 start
	echo "$NAME."
	;;

  status)
    echo -n "$LABEL is "
    if running ;  then
        echo "running"
    else
        echo " not running."
        exit 1
    fi
    ;;
  *)
	N=/etc/init.d/$NAME
	# echo "Usage: $N {start|stop|restart|reload|force-reload}" >&2
	echo "Usage: $N {start|stop|restart|force-reload|status|force-stop}" >&2
	exit 1
	;;
esac

exit 0
