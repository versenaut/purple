#
# Makefile for Purple spec article.
#

ALL:	spec

spec:			spec.docbook inputmethods.snip
			docbook2html $<

inputmethods.snip:	inputmethods.sh
			$^ >$@

# ---------------------------------------------------------

dist:
	mkdir purple-spec && cp *.png *.html purple-spec && tar czf purple-spec.tgz purple-spec && rm -r purple-spec

clean:
	rm -rf *.html *.snip
