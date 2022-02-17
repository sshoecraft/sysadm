#!/bin/bash
if test "$1" = "getip"; then
	h=`host $name`
	ip=`echo $h | awk '{
		i=index($0,"has address");
		if (i)
			print substr($0,i+12);
		else
			print $0;
	}'`
	echo $ip >> /tmp/server_ips
	exit 0
fi
echo building ip list...
#pullv | awk '{ i=index($0,":"); if (i) print substr($0,i+1); else print $0 }' > /tmp/servers
#pulldb '[d|g][0-9]t[0-9][0-9][0-9][0-9]' 's[0-9][0-9]t[0-9][0-9][0-9][0-9]' > /tmp/servers
#. $HOME/.saenv
while read name
do
	h=`host $name`
	if [ `echo $h | grep -c 'has address'` -eq 0 ]; then
		# try with no domain
		n=`echo $name | awk '{
			i=index($0,".");
			if (i)
				print substr($0,1,i-1)
			else
				print $0
		}'`
#		echo "name: $n"
		h=`host $n`
		if [ `echo $h | grep -c 'has address'` -eq 0 ]; then
#			echo "noooo...."
			found=0
#			while read domain
#			do
#				t=$n.$domain
#				echo "trying: $t..."
#				h=`host $t`
#				if [ `echo $h | grep -c 'has address'` -gt 0 ]; then
#					echo ">>>>> $h"
#					found=1
#					break
#				fi
#			done < $HOME/data/lists/alldomains
#			echo "found: $found"
			[ $found -eq 0 ] && continue
		fi
	fi
	ip=`echo $h | awk '{
		i=index($0,"has address");
		if (i)
			print substr($0,i+12);
		else
			print $0;
	}'`
	echo $ip >> /tmp/server_ips
#	if [ `echo $name | grep -c -e '^d3t'` -gt 0 ]; then
#		exit 1
#	fi
done < /tmp/servers
rm -f /tmp/servers
echo installing...
#worker /tmp/server_ips 512 ./doinst 2>/dev/null
#rm -f /tmp/server_ips
