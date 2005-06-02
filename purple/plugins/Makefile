#
# A simple Makefile for building Purple plug-ins.
#

CC=gcc
CFLAGS=-I.. -I../../verse/ -shared -fPIC -Wstrict-prototypes -Wmissing-prototypes

ALL:	libaudioecho.so libaudiotone.so libaudioscale.so libbbox.so libbmblend.so \
	libbmoilify.so \
	libchecker.so libcone.so libcube.so libcurvetest.so libdisc.so \
	libhello.so libfactors.so libmath.so libmattest.so libmeasure.so \
	libnew-node.so \
	libplane.so \
	libscale.so libstringop.so \
	libtagtest.so libtextnum.so libwarp.so

lib%.so : %.c
	$(CC) $(CFLAGS) -o $@ $^

# ---------------------------------------------------------

clean:
	rm -f *.so *.o
