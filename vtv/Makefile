#
#
#

VERSE=../verse

CFLAGS=-Wall -ansi `gtk-config --cflags` -I$(VERSE)
LDFLAGS=-L$(VERSE)
LDLIBS=`gtk-config --libs` -lverse

vtv:	vtv.c

# -------------------------------------------------------------

clean:
	rm vtv
