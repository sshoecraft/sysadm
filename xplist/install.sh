#!/bin/bash
#
THREADS=512
#
#echo "0: $0, 1: $1, 2: $2"
if test "$1" = "doinst"; then
	./doinst $2
	exit
fi
pulldb '[d|g][0-9]t[0-9][0-9][0-9][0-9]' 's[0-9][0-9]t[0-9][0-9][0-9][0-9]' > /tmp/servers
worker /tmp/servers $THREADS $0 doinst 2>/dev/null
rm -f /tmp/servers
