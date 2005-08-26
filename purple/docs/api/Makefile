#
#
#

.PHONY:	clean dist

html/index.html:
	doxygen


# -------------------------------------------------------------

dist:
	cp -R html purple-api/ && tar czf purple-api.tgz purple-api && rm -r purple-api

clean:
	rm -f html/*.html
