cat prefix.h acr.h atc.h acc.h postfix.h > oracle.h
cat prefix.pc body.pc atc.pc postfix.pc > oracle.pc
exit 0
sqlplus `whoami`/`whoami` @alltabs > genpc.log
cp prefix.h oracle.h
cp prefix.pc oracle.pc
if [ -f body.pc ]; then
	cat body.pc >> oracle.pc
fi
cat genpc.log | while read line
do
	table=`echo $line|awk '{if(substr($1,1,4)=="ALL_") print tolower($1)}'`
	if [ "x$table" = "x" ]; then
		continue
	fi
#	prefix=`echo $table | awk '{print (substr($1,5,31))}'`
	if [ "$table" != "all_cluster_hash_expressions" ]; then
		genpc $table
		cat "$table".h >> oracle.h
		rm -f "$table".h
		cat "$table".pc >> oracle.pc
		rm -f "$table".pc
	fi
done
cat postfix.h >> oracle.h
cat postfix.pc >> oracle.pc
#rm -f genpc.log
exit 0
