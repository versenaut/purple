#! /usr/bin/python
#
# licglue.py
#
# A simple Python program to "glue" a license blurb into C code, and also
# add the filename into each file's top comment. Written by Emil Brink.
#

import os
import sys

LICENSE=" * Copyright (C) 2004 COPYRIGHT-PLACEHOLDER. See COPYING for license details."

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

# Load a text file, returning its lines as a list.
def load(filename):
	f = open(filename, "rt")
	data = f.read()
	lines = data.split("\n")
	f.close()
	return lines

for a in sys.argv[1:]:
	f = load(a)
	if len(f) > 0:
		f = insert(a, f, LICENSE)
		if f == None: continue
		for i in range(10):
			print str(i) + ": '" + f[i] + "'"
