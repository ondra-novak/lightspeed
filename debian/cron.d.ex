#
# Regular cron jobs for the liblightspeed package
#
0 4	* * *	root	[ -x /usr/bin/liblightspeed_maintenance ] && /usr/bin/liblightspeed_maintenance
