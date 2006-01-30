#
# Build Purple. Simple hand-written (and rather verbose) Makefile, for now.
#

# Location of a (compiled) Verse core directory. Must contain verse.h and libverse.a.
VERSE=../verse


TARGETS=purple

ALL:		$(TARGETS) all-plugins


CFLAGS=-g -Wall -I$(VERSE) -L$(VERSE) -Wsign-compare
LDLIBS=-ldl -lverse -lm

# The -rdynamic flag is not needed on Mac OS X, but is assumed to be needed elsewhere.
ifneq ($(shell uname),Darwin)
  CFLAGS += -rdynamic
endif


purple:		purple.c \
		api-init.o api-input.o api-iter.o api-node.o api-output.o \
		bintree.o client.o cron.o diff.o dynarr.o dynlib.o dynstr.o graph.o \
		filelist.o hash.o idlist.o idset.o idtree.o list.o log.o mem.o memchunk.o \
		nodedb.o nodedb-a.o nodedb-b.o nodedb-c.o nodedb-g.o nodedb-m.o nodedb-o.o nodedb-t.o \
		nodeset.o plugins.o plugin-clock.o plugin-input.o plugin-output.o \
		port.o resume.o scheduler.o strutil.o synchronizer.o textbuf.o timeval.o \
		value.o vecutil.o xmlnode.o xmlutil.o \
		$(VERSE)/libverse.a

xmlnode.a:	xmlnode.o dynstr.o mem.o memchunk.o list.o strutil.o log.o
		ar -cr $@ $^

api-init.o:	api-init.c purple.h

api-input.o:	api-input.c purple.h

api-node.o:	api-node.c purple.h

api-output.o:	api-output.c purple.h

bintree.o:	bintree.c bintree.h

client.o:	client.c client.h

cron.o:		cron.c cron.h

diff.o:		diff.c diff.h

dynarr.o:	dynarr.c dynarr.h

dynlib.o:	dynlib.c dynlib.h

dynstr.o:	dynstr.c dynstr.h

filelist.o:	filelist.c filelist.h

graph.o:	graph.c graph.h

hash.o:		hash.c hash.h

idlist.o:	idlist.c idlist.h

idset.o:	idset.c idset.h

idtree.o:	idtree.c idtree.h

list.o:		list.c list.h

log.o:		log.c log.h

mem.o:		mem.c mem.h

memchunk.o:	memchunk.c memchunk.h mem.h

nodedb.o:	nodedb.c nodedb.h nodedb-internal.h

nodedb-g.o:	nodedb-g.c nodedb-g.h nodedb.h nodedb-internal.h

nodedb-m.o:	nodedb-m.c nodedb-m.h nodedb.h nodedb-internal.h

nodedb-b.o:	nodedb-b.c nodedb-b.h nodedb.h nodedb-internal.h

nodedb-c.o:	nodedb-c.c nodedb-c.h nodedb.h nodedb-internal.h

nodedb-o.o:	nodedb-o.c nodedb-o.h nodedb.h nodedb-internal.h

nodedb-t.o:	nodedb-t.c nodedb-t.h nodedb.h nodedb-internal.h

nodedb-a.o:	nodedb-a.c nodedb-a.h nodedb.h nodedb-internal.h

nodeset.o:	nodeset.c nodeset.h

plugins.o:	plugins.c plugins.h

plugin-input.o:	plugin-input.c purple.h

plugin-output.o:	plugin-output.c purple.h

port.o:		port.c port.h value.h

resume.o:	resume.c resume.h

scheduler.o:	scheduler.c scheduler.h

strutil.o:	strutil.c strutil.h

synchronizer.o:	synchronizer.c synchronizer.h

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

DIST=purple-`date +"%Y%m%d"`-`uname -s | tr -d ' ' | tr [A-Z] [a-z]`-`uname -m | tr -d ' '`
FILES=purple README plugins/*.so

dist:	purple all-plugins
	strip purple && tar czf $(DIST).tar.gz $(FILES)

# ---------------------------------------------------------

clean:	clean-plugins
	rm -f *.o *.so $(TARGETS)

clean-plugins:
	cd plugins && $(MAKE) clean
