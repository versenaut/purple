#
# Build Purple. Simple hand-written (and rather verbose) Makefile, for now.
#

# Location of a (compiled) Verse core directory. Must contain verse.h and libverse.a.
VERSE=../verse


TARGETS=purple

ALL:		$(TARGETS) all-plugins


CFLAGS=-g -Wall -rdynamic -I$(VERSE) -L$(VERSE)
LDLIBS=-ldl -lverse -lm

purple:		purple.c \
		api-init.o api-input.o api-node.o api-output.o \
		client.o cron.o dynarr.o dynlib.o dynstr.o graph.o \
		filelist.o hash.o idlist.o idset.o list.o log.o mem.o memchunk.o \
		nodedb.o nodedb-b.o nodedb-g.o nodedb-o.o nodedb-t.o \
		nodeset.o plugins.o plugin-input.o \
		port.o scheduler.o strutil.o textbuf.o timeval.o value.o vecutil.o xmlnode.o xmlutil.o \
		$(VERSE)/libverse.a

api-init.o:	api-init.c purple.h

api-input.o:	api-input.c purple.h

api-node.o:	api-node.c purple.h

api-output.o:	api-output.c purple.h

client.o:	client.c client.h

cron.o:		cron.c cron.h

dynarr.o:	dynarr.c dynarr.h

dynlib.o:	dynlib.c dynlib.h

dynstr.o:	dynstr.c dynstr.h

filelist.o:	filelist.c filelist.h

graph.o:	graph.c graph.h

hash.o:		hash.c hash.h

idlist.o:	idlist.c idlist.h

idset.o:	idset.c idset.h

list.o:		list.c list.h

log.o:		log.c log.h

mem.o:		mem.c mem.h

memchunk.o:	memchunk.c memchunk.h mem.h

nodedb.o:	nodedb.c nodedb.h nodedb-internal.h

nodedb-g.o:	nodedb-g.c nodedb-g.h nodedb.h nodedb-internal.h

nodedb-o.o:	nodedb-b.c nodedb-b.h nodedb.h nodedb-internal.h

nodedb-o.o:	nodedb-o.c nodedb-o.h nodedb.h nodedb-internal.h

nodedb-t.o:	nodedb-t.c nodedb-t.h nodedb.h nodedb-internal.h

nodeset.o:	nodeset.c nodeset.h

plugins.o:	plugins.c plugins.h

plugin-input.o:	plugin-input.c purple.h

port.o:		port.c port.h value.h

scheduler.o:	scheduler.c scheduler.h

strutil.o:	strutil.c strutil.h

textbuf.o:	textbuf.c textbuf.h

timeval.o:	timeval.c timeval.h

value.o:	value.c value.h

vecutil.o:	vecutil.c vecutil.h

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
