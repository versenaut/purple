#
#
#

VERSE=../verse

CFLAGS=-Wall -ansi `pkg-config --cflags gtk+-2.0` -I$(VERSE)
LDFLAGS=-L$(VERSE)
LDLIBS=`pkg-config --libs gtk+-2.0` -lverse

vtv:	vtv.c

# -------------------------------------------------------------

clean:
	rm -f vtv
