#! /bin/sh
### BEGIN INIT INFO
# Provides:          remote exp control by Yggdrasil
# Required-Start:    $all
# Required-Stop:     $all
# Default-Start:     2 3 4 5
# Default-Stop:	     0 1 6
# Short-Description: activate yggdrasil control process
# Description:       enables the yggdrasil control process that is used to manage experiences with yggdrasil.
### END INIT INFO

LOGDIRECTORY=/var/log/yggdrasil
OUTPUTFILE=output.out
YGGHOME=/home/yggdrasil

case "$1" in
  start|"")
  echo 3600 >/proc/sys/net/ipv4/neigh/default/gc_stale_time
  echo 10 >/proc/sys/net/ipv4/tcp_retries2
  #echo 45 >/proc/sys/net/ipv4/tcp_keepalive_time
  echo 20 >/proc/sys/net/ipv4/tcp_keepalive_time

  if [ ! -d "$LOGDIRECTORY" ]; then
     mkdir $LOGDIRECTORY
  fi

  configfile=$YGGHOME"/tools/runonboot.txt"

  cd $YGGHOME

  if [ -f $configfile ]; then

    command=`cat $configfile`

    if [ ! -z "$command" ]; then
       c=$(ls -1 /var/log/yggdrasil/errors.log.* | wc -l)
       c=$((c+1))
       /usr/bin/nohup ./$command 2> ${LOGDIRECTORY}/errors.log.$c
    fi
  fi
	;;
  restart|reload|force-reload)
	# No-op
        ;;
  stop)
	# No-op
  killall -3 YggdrasilControlProcess
  if [ -f /home/yggdrasil/output.out ]; then
     count=$(ls -1 /var/log/yggdrasil/output.out* | wc -l)
     count=$((count+1))
     mv /home/yggdrasil/output.out /var/log/yggdrasil/output.out.$count
  fi
	;;
  status)
	;;
  *)
	;;
esac

:
