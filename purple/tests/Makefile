#
# Simplistic Makefile for building automated tests.
#

VERSE=../../verse
CFLAGS=-g -Wall -I.. -I$(VERSE)

# List individual module testers here.
ALL=test-diff test-dynarr test-dynstr test-hash test-idlist test-idset test-list test-memchunk test-strutil test-textbuf

ALL:		$(ALL)

test-diff:	test-diff.c libtest.a

test-dynarr:	test-dynarr.c libtest.a

test-dynstr:	test-dynstr.c libtest.a

test-hash:	test-hash.c libtest.a

test-idlist:	test-idlist.c libtest.a

test-idset:	test-idset.c libtest.a

test-list:	test-list.c libtest.a

test-memchunk:	test-memchunk.c libtest.a

test-strutil:	test-strutil.c libtest.a

test-textbuf:	test-textbuf.c libtest.a


# Framework for testing. Ultra-simple.
test.o:		test.c test.h

# Code to test, more or less the "utility" parts of the Purple codebase, as needed.
libtest.a:	../diff.o ../dynarr.o ../dynstr.o ../hash.o ../idlist.o ../idset.o ../list.o \
		../log.o ../memchunk.o ../mem.o ../strutil.o ../textbuf.o test.o
		ar cr $@ $^

# -------------------------------------------------------------

clean:
	rm -f *.o *.a $(ALL)
