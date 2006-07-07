#
# Simple Makefile for Puyo's documentation.
#

ALL:		index.html

.PHONY:		clean

index.html:	puyo.docbook
		docbook2html $< -V '%use-id-as-filename%'

# Handy for building a single, downloadable, archive of the manual.
dist:		index.html
		rm -rf puyo-guide && mkdir -p puyo-guide && cp -r media puyo-guide && cp *.html puyo-guide && find puyo-guide -name 'CVS' -prune -exec rm -r \{\} \;
		tar czf puyo-guide.tar.gz puyo-guide/
		zip -r puyo-guide.zip puyo-guide/
		rm -rf puyo-guide/

clean:
	rm -f *.html
