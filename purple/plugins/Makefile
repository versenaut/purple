#
# A simple Makefile for building Purple plug-ins. Pretty GNU.
#

CC=gcc

# Compute how to build a shared library. This should work on
# Linux and Mac OS X, at least.
ifeq ($(shell uname),Darwin)
 SHARED="-bundle -flat_namespace -undefined suppress"
else
 SHARED=-shared
endif

CFLAGS=-I.. -I../../verse/ $(SHARED) -fPIC -Wstrict-prototypes -Wmissing-prototypes

ALL:	$(patsubst %.c,lib%.so,$(wildcard *.c))

lib%.so : %.c
	$(CC) $(CFLAGS) -o $@ $^

# ---------------------------------------------------------

clean:
	rm -f *.so *.o
