#
# A simple Makefile for building Purple plug-ins.
#

CC=gcc
CFLAGS=-I.. -I../../verse/ -shared -Wstrict-prototypes -Wmissing-prototypes

ALL:	libhello.so libmath.so

lib%.so : %.c
	$(CC) $(CFLAGS) -o $@ $^

libhello.so:	hello.c

libmath.so:	math.c

# ---------------------------------------------------------

clean:
	rm -f *.so *.o
