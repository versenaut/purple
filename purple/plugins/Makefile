#
# A simple Makefile for building Purple plug-ins.
#

CC=gcc
CFLAGS=-I.. -I../../verse/ -shared -Wstrict-prototypes -Wmissing-prototypes

ALL:	libbbox.so libchecker.so libcube.so libdisc.so libhello.so libfactors.so libmath.so \
	libmeasure.so libscale.so libstringop.so libwarp.so

lib%.so : %.c
	$(CC) $(CFLAGS) -o $@ $^

libbbbox.so:	bbox.c

libchecker.so:	checker.c

libcube.so:	cube.c

libdisc.so:	disc.c

libhello.so:	hello.c

libmath.so:	math.c

libmeasure.so:	measure.c

libscale.so:	scale.c

libstringop.so:	stringop.c

libwarp.so:	warp.c

# ---------------------------------------------------------

clean:
	rm -f *.so *.o
