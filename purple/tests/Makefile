#
# Simplistic Makefile for building automated tests.
#

CFLAGS=-g -Wall -I..

# List individual module testers here.
ALL=test-dynstr test-list test-strutil

ALL:		$(ALL)

test-dynstr:	test-dynstr.c libtest.a

test-list:	test-list.c libtest.a

test-strutil:	test-strutil.c libtest.a


# Framework for testing. Ultra-simple.
test.o:		test.c test.h

# Code to test, more or less the "utility" parts of the Purple codebase, as needed.
libtest.a:	../dynstr.o ../list.o ../log.o ../memchunk.o ../mem.o ../strutil.o test.o
		ar cr $@ $^

# -------------------------------------------------------------

clean:
	rm -f *.o *.a $(ALL)
