#
# A simple Makefile for building Purple plug-ins.
#

CC=gcc
CFLAGS=-I.. -I../../verse/ -shared -Wstrict-prototypes -Wmissing-prototypes

ALL:	libcube.so libhello.so libfactors.so libmath.so libmeasure.so libscale.so libstringop.so

lib%.so : %.c
	$(CC) $(CFLAGS) -o $@ $^

libcube.so:	cube.c

libhello.so:	hello.c

libmath.so:	math.c

libmeasure.so:	measure.c

libscale.so:	scale.c

libstringop.so:	stringop.c

# ---------------------------------------------------------

clean:
	rm -f *.so *.o
