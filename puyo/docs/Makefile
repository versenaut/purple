#
# Simple Makefile for Puyo's documentation.
#

DISTS=puyo-guide.tar.gz puyo-guide.zip

ALL:		index.html $(ALL)

.PHONY:		clean

index.html:	puyo.docbook
		docbook2html $< -V '%use-id-as-filename%'

# Handy for building downloadable archives of the manual.
dist:			$(DISTS)

puyo-guide:		index.html
			rm -rf puyo-guide && mkdir -p puyo-guide && cp -r media puyo-guide && cp *.html puyo-guide && find puyo-guide -name 'CVS' -prune -exec rm -r \{\} \;

puyo-guide.zip:		puyo-guide
			zip -r puyo-guide.zip puyo-guide/

puyo-guide.tar.gz:	puyo-guide
			tar czf puyo-guide.tar.gz puyo-guide/


clean:
	rm -f *.html $(DISTS) purple-guide/
