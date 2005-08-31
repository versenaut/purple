#
# purpleinfo.py -- A helper class that does various lookups.
#
# Copyright (C) 2004-2005 PDC, KTH. See COPYING for license details.
#

import string

import libxml2

class PurpleInfo:
	def __init__(self):
		self.doc = None
		self._plugins = {}
		self._plugin_names = {}

	def doc_set(self, doc):
		if self.doc != None: self.doc.freeDoc()
		self.doc = doc

    	def plugins_get_desc(self):
		"Returns list of (name,id) tuples describing the available plug-ins."
		res = self.doc.xpathEval("purple-plugins/plug-in[@name]")
		ret = []
		for r in res:
			ret.append((r.xpathEval("@name")[0].content, r.xpathEval("@id")[0].content))
		return ret

	def plugin_get_name(self, pid):
		q = "purple-plugins/plug-in[@id='%d']/@name" % int(pid)
		a = self.doc.xpathEval(q)
		if a == []:
			return "(Unknown)"
		return a[0].content

	def plugin_get_input_num(self, pid):
		q = "count(purple-plugins/plug-in[@id='%d']/inputs/input)" % int(pid)
		return int(self.doc.xpathEval(q))

	def plugin_get_input_name(self, pid, index):
		"Return name of input <iid> in plug-in <pid>."
		q = "purple-plugins/plug-in[@id='%d']/inputs/input[%d]/name" % (int(pid), int(index + 1))
		r = self.doc.xpathEval(q)
		return r[0].content

	def plugin_get_input_type(self, pid, index):
		"Return type of input <index> in plug-in <pid>."
		q = "purple-plugins/plug-in[@id='%d']/inputs/input[%d]/@type" % (int(pid), int(index + 1))
		return self.doc.xpathEval(q)[0].content

	def plugin_get_input_range(self, pid, index):
		"Return dictionary holding default, min, max values for input <index> in plug-in <pid>."
		q = "purple-plugins/plug-in[@id='%d']/inputs/input[%d]/range" % (int(pid), int(index + 1))
		r = self.doc.xpathEval(q)
		qt = "purple-plugins/plug-in[@id='%d']/inputs/input[%d]/@type" % (int(pid), int(index + 1))
		ctype = self.doc.xpathEval(qt)[0].content
		if r != None and len(r) > 0:
			sub = [ "def", "min", "max" ]
			rng = { }
			for s in sub:
				q = r[0].xpathEval(s)
				if q != None and len(q) > 0:
					cont = q[0].content
					if cont == "false":
						rv = 0.0
					elif cont == "true":
						rv = 1.0
					elif ctype == "real64_vec3":
						t = eval("(" + string.replace(string.strip(cont, "[]"), " ", ",") + ")")	# Convert e.g. "[1 2 3]" to "(1, 2, 3)", and eval.
						rv = (float(t[0]), float(t[1]), float(t[2]))
					elif ctype == "string":
						rv = cont
					else:
						rv = float(cont)
					rng[s] = rv
#			print " returning range:", rng
			return rng
		return None

	def plugin_get_input_enums(self, pid, index):
		"""Return list of enum tuples, of the form (value,label). If there are no enums, None is returned."""
		q = "purple-plugins/plug-in[@id='%d']/inputs/input[%d]/enums" % (int(pid), int(index) + 1)
		r = self.doc.xpathEval(q)
		try:
			es = r[0].content
			es = es.strip("|").split("|")	# Split into list of "value:label" strings.
			enums = [(int(x.split(":")[0]),x.split(":")[1]) for x in es]	# Comprehend that.
		except:	enums = None
		return enums
