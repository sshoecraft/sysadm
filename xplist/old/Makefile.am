bin_PROGRAMS = xplist
xplist_SOURCES = main.c
xplist_CPPFLAGS=-I../lib
xplist_LDADD=../lib/libutils.a

test::
	sudo ./xplist
	sudo ./xplist -d
	sudo ./xplist -n

