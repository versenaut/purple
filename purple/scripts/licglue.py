#! /usr/bin/python
#
# licglue.py
#
# A simple Python program to "glue" a license blurb into C code, and also
# add the filename into each file's top comment. Written by Emil Brink.
#
# Hm. This would have been about the same length in C... I must learn more.
#

import os
import string
import sys

LICENSE_TEMPLATE=" * Copyright (C) 2004 COPYRIGHT-PLACEHOLDER. See COPYING for license details."
LICENSE=LICENSE_TEMPLATE

# Insert comment blurb at the top, either creating new comment or extending existing.
def insert(name, file, blurb):
	if file[0] != "/*":
		file.insert(0, "/*")
		file.insert(1, "*/")
	elif file[0] == "/*":		# Comment already exists?
		for i in range(min(10, len(file))):
			if file[i] == blurb:
				print name + ": ignoring, old license blurb found"
				return None
		print name + ": extending existing comment"
	file.insert(1, " * " + os.path.basename(name))
	file.insert(2, " * ")
	file.insert(3, blurb)
	file.insert(4, " * ")
	return file

# Get size of file.
def filesize(filename):
	s = os.stat(filename)
	return s.st_size

# Load a text file, returning its lines as a list.
def load(filename):
	f = open(filename, "rt")
	data = f.read()
	lines = data.split("\n")
	f.close()
	return lines[:len(lines) - 1]

for a in sys.argv[1:]:
	if a[0:7] == "--copy=":
		copyright = a[7:]
		LICENSE = string.replace(LICENSE_TEMPLATE, "COPYRIGHT-PLACEHOLDER", copyright)
		continue
	f = load(a)
	if len(f) > 0:
		# Insert blurb.
		f = insert(a, f, LICENSE)
		# Save it to temporary file.
		outname = a + "-blurbed"
		out = open(outname, "wt")
		if out != None:
			for l in f:
				out.write(l + "\n")
			out.close()
			insize = filesize(a)
			outsize = filesize(outname)
			# Rename over original.
			if outsize - insize > len(LICENSE):
				os.rename(outname, a)
		else:
			print "Open failed"
