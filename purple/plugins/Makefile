#
# A simple Makefile for building Purple plug-ins. Pretty GNU.
#

CC=gcc
CFLAGS=-I.. -I../../verse/ -shared -fPIC -Wstrict-prototypes -Wmissing-prototypes

ALL:	$(patsubst %.c,lib%.so,$(wildcard *.c))

lib%.so : %.c
	$(CC) $(CFLAGS) -o $@ $^

# ---------------------------------------------------------

clean:
	rm -f *.so *.o
