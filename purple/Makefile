#
# Build Purple. Simple hand-written (and rather verbose) Makefile, for now.
#

# Location of a (compiled) Verse core directory. Must contain verse.h and libverse.a.
VERSE=../verse


TARGETS=purple

ALL:	$(TARGETS) all-plugins


CFLAGS=-g -Wall -rdynamic -I$(VERSE) -L$(VERSE)
LDLIBS=-ldl -lverse

purple:		purple.c api-init.o client.o cron.o dynarr.o dynlib.o dynstr.o graph.o \
		filelist.o hash.o idset.o list.o log.o mem.o memchunk.o plugins.o strutil.o textbuf.o \
		timeval.o xmlnode.o xmlutil.o \
		$(VERSE)/libverse.a

api-init.o:	api-init.c purple.h

client.o:	client.c client.h

cron.o:		cron.c cron.h

dynarr.o:	dynarr.c dynarr.h

dynlib.o:	dynlib.c dynlib.h

dynstr.o:	dynstr.c dynstr.h

filelist.o:	filelist.c filelist.h

graph.o:	graph.c graph.h

hash.o:		hash.c hash.h

idset.o:	idset.c idset.h

list.o:		list.c list.h

log.o:		log.c log.h

mem.o:		mem.c mem.h

memchunk.o:	memchunk.c memchunk.h mem.h

plugins.o:	plugins.c plugins.h

strutil.o:	strutil.c strutil.h

textbuf.o:	textbuf.c textbuf.h

timeval.o:	timeval.c timeval.h

xmlnode.o:	xmlnode.c xmlnode.h

xmlutil.o:	xmlutil.c xmlutil.h

# ---------------------------------------------------------

all-plugins:
	cd plugins && $(MAKE)

# ---------------------------------------------------------

clean:	clean-plugins
	rm -f *.o *.so $(TARGETS)

clean-plugins:
	cd plugins && $(MAKE) clean
