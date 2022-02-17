
all: oracle.pc oracle.h
	cp oracle.pc ..
	cp oracle.h ..

.if exists(body.pc)
oracle.pc oracle.h: prefix.h prefix.pc body.pc postfix.h postfix.pc
.else
oracle.pc oracle.h: prefix.h prefix.pc postfix.h postfix.pc
.endif
	sh genpc.sh

clean:
	rm -f oracle.*
