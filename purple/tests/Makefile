#
# Simplistic Makefile for building automated tests.
#

CFLAGS=-g -Wall -I..

# List individual module testers here.
ALL=test-list

ALL:		$(ALL)

test-list:	test-list.c libtest.a


# Framework for testing. Ultra-simple.
test.o:		test.c test.h

# Code to test, more or less the "utility" parts of the Purple codebase, as needed.
libtest.a:	../list.o ../memchunk.o ../mem.o ../log.o ../strutil.o test.o
		ar cr $@ $^

# -------------------------------------------------------------

clean:
	rm -f *.o *.a $(ALL)
