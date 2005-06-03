#
# Makefile for the Verse bitmap viewer, a GTK+ program to view
# Verse bitmaps. Yes really.
#

VERSE=../verse

CFLAGS=-Wall -ansi -g `pkg-config --cflags gtk+-2.0` -I$(VERSE)
LDFLAGS=-L$(VERSE)
LDLIBS=`pkg-config --libs gtk+-2.0` -lverse

.PHONY:	clean

vbv:	vbv.c

# -------------------------------------------------------------

clean:
	rm -f vbv
