#
# A simple Makefile for building Purple plug-ins.
#

CC=gcc
CFLAGS=-I.. -I../../verse/ -shared -Wstrict-prototypes -Wmissing-prototypes

lib%.so : %.c
	$(CC) $(CFLAGS) -o $@ $^

libhello.so:	hello.c

# ---------------------------------------------------------

clean:
	rm -f *.so *.o
