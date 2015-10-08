#!/bin/bash
# popup a small notification with 'notify-send'
dis=$(formail -X From: -X Subject:)
# sometimes the order is difference - in very short headers
# check for both
if [[ "$dis" =~ From:(.+)Subject:(.+) ]]; then
	from=${BASH_REMATCH[1]}
	sub=${BASH_REMATCH[2]}
fi
if [[ "$dis" =~ Subject:(.+)From:(.+) ]]; then
	from=${BASH_REMATCH[2]}
	sub=${BASH_REMATCH[1]}
fi


# tweaks < > are special
from=${from//</\(}
from=${from//>/\)}
from=${from//&/\.}
sub=${sub//</\(}
sub=${sub//>/\)}
sub=${sub//&/\.}

sub=${sub:0:75}
from=${from:0:75}
TM=2000

echo ==$dis==
echo ==$from==
echo ==$sub==
# from http://gnome-hacks.jodrell.net/hacks.html?id=82
# modified for GNOME-2.14
pids=`pgrep -u allen gnome-session`
for pid in $pids; do
	# find DBUS session bus for this session
	ADDRESS=`grep -z DBUS_SESSION_BUS_ADDRESS \
		/proc/$pid/environ | sed -e 's/DBUS_SESSION_BUS_ADDRESS=//'`
	# use it
	DBUS_SESSION_BUS_ADDRESS=$ADDRESS \
	DISPLAY=:0 \
	/usr/bin/notify-send -u normal -t $TM "$sub" "$from"
done
