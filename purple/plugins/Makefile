#
# A simple Makefile for building Purple plug-ins.
#

CC=gcc
CFLAGS=-I.. -I../../verse/ -shared -Wstrict-prototypes -Wmissing-prototypes

ALL:	libaudiotone.so libaudioscale.so libbbox.so libbmblend.so libchecker.so libcube.so libcurvetest.so libdisc.so \
	libhello.so libfactors.so libmath.so libmattest.so libmeasure.so libscale.so libstringop.so \
	libtagtest.so libtextnum.so libwarp.so

lib%.so : %.c
	$(CC) $(CFLAGS) -o $@ $^

# ---------------------------------------------------------

clean:
	rm -f *.so *.o
