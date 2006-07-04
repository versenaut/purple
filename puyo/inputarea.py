#
# inputarea.py -- Code for building and managing GTK+ interface for module input setting.
#
# Copyright (C) 2004-2005 PDC, KTH. See COPYING for license details.
#

import pygtk
pygtk.require("2.0")
import gobject
import gtk

class InputArea(gtk.Notebook):
	"An area that knows about Purple module inputs."

	def __init__(self, purpleinfo):
		"Initialize a InputArea."
		gtk.Notebook.__init__(self)
		self.purpleinfo = purpleinfo
		self.set_show_tabs(0)
		self.pages = {}	# (pageno,InputTable) for each module ID.
		self.purple = None
		self.graph = None
		self.graph_id = None
		self.current_module = -1
		self.current_sequence = None

    	def set_purple(self, purple):
		self.purple = purple

	class InputTable(gtk.Table):
		"A table of input widgets. Knows which widget maps to which input index."

		def __init__(self, area, pid, height):
			gtk.Table.__init__(self, height, 2)
			self.area = area
			self.pid  = pid
			self.height = height
			self.inputs = {}
			self.set_size_request(200, -1)

		def _cb_change_boolean(self, wid, inp):
			self.area.purple.mod_input_set_boolean(self.area.graph_id, inp.iid[0], inp.iid[1], wid.get_active())

		def _cb_change_uint32(self, wid, inp):
			self.area.purple.mod_input_set_uint32(self.area.graph_id, inp.iid[0], inp.iid[1], int(wid.get_value()))

		def _cb_change_uint32_enum(self, wid, inp):
			self.area.purple.mod_input_set_uint32(self.area.graph_id, inp.iid[0], inp.iid[1], wid._get_value())

		def _cb_change_real32(self, adj, inp):
			self.area.purple.mod_input_set_real32(self.area.graph_id, inp.iid[0], inp.iid[1], adj.get_value())

		def _cb_change_real64(self, adj, inp):
			self.area.purple.mod_input_set_real64(self.area.graph_id, inp.iid[0], inp.iid[1], adj.get_value())

		def _cb_change_real64_vec(self, adj, inp):
			self.area.purple.mod_input_set_real64_vec(self.area.graph_id, inp.iid[0], inp.iid[1], inp.get_vector())

		def _cb_change_module(self, wid, inp):
			self.area.purple.mod_input_set_module(self.area.graph_id, inp.iid[0], inp.iid[1], wid.get_active())

		def _cb_activate_string(self, wid, inp):
			self.area.purple.mod_input_set_string(self.area.graph_id, inp.iid[0], inp.iid[1], wid.get_text())

		class Input:
			"Base class for input widgetry."
			def __init__(self, iid):
				self.iid = iid
				self.block_obj = None
				self.block_sig = 0
				self.area = None

			def _connect(self, signame, handler, data, obj = None):
				if obj == None:	obj = self
				self.block_sig = obj.connect(signame, handler, data)
				self.block_obj = obj

			def _block(self):
				if self.block_sig > 0: self.block_obj.handler_block(self.block_sig)

			def _unblock(self):
				if self.block_sig > 0: self.block_obj.handler_unblock(self.block_sig)

			def set_default(self, value):
				self._set_direct(value)

			def set(self, set, modules):
				self._block()
				self._set(set, modules)
				self._unblock()

		class InputBoolean(gtk.CheckButton, Input):
			def __init__(self, iid, rangeSpec = None):
				gtk.CheckButton.__init__(self)
				InputArea.InputTable.Input.__init__(self, iid)

			def _set_direct(self, value):
				self.set_active(value > 0.0)

			def _set(self, set, modules):
				if set[1] == "boolean":
					self.set_active(set[2] == "true")

		class InputUInt32(gtk.SpinButton, Input):
		    	def __init__(self, iid, rangeSpec = None):
				gtk.SpinButton.__init__(self, digits = 0)
				self.set_increments(1, 10)
				self.set_range(0, pow(2, 32) - 1)
				self.set_numeric(1)
				InputArea.InputTable.Input.__init__(self, iid)

			def _set_direct(self, value):
				self.get_adjustment().set_value(int(value))

			def _set(self, set, modules):
				if set[1] == "uint32" or set[1] == "real32":
					self.get_adjustment().set_value(int(set[2]))

		class InputUInt32Enum(gtk.ComboBox, Input):
			def __init__(self, iid, rangeSpec, enums):
				self.model = gtk.ListStore(gobject.TYPE_STRING)
				gtk.ComboBox.__init__(self, self.model)
				cell = gtk.CellRendererText()
				self.pack_start(cell, gtk.TRUE)
				self.add_attribute(cell, 'text', 0)
				self.enums = enums

				for e in self.enums:
					self.append_text(e[1])

				InputArea.InputTable.Input.__init__(self, iid)

			def _get_value(self):
				x = self.get_active()
				return self.enums[x][0]

			def _set_direct(self, value):
				"""Set to show value. Needs to look up which list index that maps to."""
				for i in range(len(self.enums)):
					e = self.enums[i]
					if e[0] == value:
						self.set_active(i)
						return
				print "Enum is angered by being asked to show", value, "which is not in", self.enums

			def _set(self, set, modules):
				self._set_direct(int(set[2]))
				

		class InputReal(gtk.HScale, Input):
			def __init__(self, iid, rangeSpec = None):
				mn = 0.0
				mx = 100.0
				df = 0.0
				if rangeSpec != None:
					try:
						mn = rangeSpec["min"]
						mx = rangeSpec["max"]
						df = rangeSpec["def"]
					except:
						pass
				a = gtk.Adjustment(df, mn, mx, (mx - mn) / 1000.0, (mx - mn) / 1000.0)
				gtk.HScale.__init__(self, a)
				self.set_value_pos(gtk.POS_RIGHT)
				self.set_digits(3)
				InputArea.InputTable.Input.__init__(self, iid)

			def _connect(self, signame, handler, data):
				InputArea.InputTable.Input._connect(self, signame, handler, data, self.get_adjustment())

			def _set_direct(self, v):
				self._set((None, "real64", v), None)

			def _set(self, set, modules):
				if set[1] == "real32" or set[1] == "real64":
					self.get_adjustment().set_value(float(set[2]))

		class InputRealVec(gtk.Table, Input):
			def __init__(self, iid, rangeSpec = None, dimension = 3):
				self.slider = []
				gtk.Table.__init__(self, dimension, 2)
				for i in range(dimension):
					l = gtk.Label(["x", "y", "z", "w"][i])
					self.attach(l, 0, 1, i, i + 1)
					mn = 0.0
					mx = 100.0
					df = 0.0
					if rangeSpec != None:
						try:
							mn = rangeSpec["min"][i]
							mx = rangeSpec["max"][i]
							df = rangeSpec["def"][i]
						except:
							pass
					s = gtk.HScale(gtk.Adjustment(df, mn, mx, (mx - mn) / 1000.0, (mx - mn) / 1000.0))
					s.set_value_pos(gtk.POS_RIGHT)
					s.set_digits(3)
					self.attach(s, 1, 2, i, i + 1)
					self.slider.append(s)
				self.dim = dimension
				InputArea.InputTable.Input.__init__(self, iid)
				self.sig = [ -1 ] * self.dim	# Overwrite sig member from Input.
				print "initialized in", self, "sig=", self.sig

			def _set_direct(self, v):
				for i in range(len(v)):
					s = self.slider[i]
					x = v[i]
					s.set_value(x)

			def _set(self, set, modules):
				print "In vector _set(), set=", set

			# Return tuple holding vector slider value.
			def get_vector(self):
				v = []
				for i in range(self.dim):
					v.append(self.slider[i].get_value())
				return tuple(v)

			def _connect(self, signame, handler, data):
				print "connecting, self=", self
				print " and sig=", self.sig
				for i in range(self.dim):
					self.sig[i] = self.slider[i].connect(signame, handler, data)

			def _block(self):
				for i in range(self.dim):
					if self.sig[i] > 0: self.slider[i].handler_block(self.sig[i])

			def _unblock(self):
				for i in range(self.dim):
					if self.sig[i] > 0: self.slider[i].handler_unblock(self.sig[i])

		class InputModule(gtk.ComboBox, Input):
			def __init__(self, iid, rangeSpec = None, purpleinfo = None):
				# Retrace the steps taken by gtk.combo_box_new_string(), for subclass.
				liststore = gtk.ListStore(gobject.TYPE_STRING)
				gtk.ComboBox.__init__(self, liststore)
				cell = gtk.CellRendererText()
				self.pack_start(cell, True)
			    	self.add_attribute(cell, 'text', 0)
				InputArea.InputTable.Input.__init__(self, iid)

			def _set(self, set, modules):
				"Make widget reflect current value, in set."
				for i in range(256):	# Arbitrary limit, don't know how to read out count. :/
					self.remove_text(0)
				i = -1
				for m in modules:
					self.append_text("%u (%s)" % m)
					if int(set[2]) == m[0]: i = modules.index(m)
				self.set_active(i)

		class InputString(gtk.Entry, Input):
			def __init__(self, iid):
				gtk.Entry.__init__(self)
				InputArea.InputTable.Input.__init__(self, iid)

			def _set_direct(self, v):
				self._set(v, None)

			def _set(self, set, modules):
				gtk.Entry.set_text(self, set[2])

		def _set_current(self, widget, iid, cv):
			# We don't need to actually do anything here, since the proper
			# action will be taken by the imminent _refresh() call.
			pass

		def _set_default(self, widget, iid, rng):
			try:	df = rng["def"]
			except:	return
			widget.set_default(df)

		def _evt_label_button_press(self, *args):
			evt = args[1]
			if evt.button == 3:
				iid = args[2]
				self.area.purple.mod_input_clear(self.area.graph_id, iid[0], iid[1])

		def add(self, iid, label, type, cv, rng, enums):
			index = iid[1]
			lb = gtk.EventBox()
			l = gtk.Label(label)
			lb.add(l)
			lb.connect("button_press_event", self._evt_label_button_press, iid)
			self.attach(lb, 0, 1, index, index + 1)
			if type == "boolean":
				w = InputArea.InputTable.InputBoolean(iid)
				w._connect("toggled", self._cb_change_boolean, w)
			elif type == "int32":
				w = gtk.SpinButton(digits=0)
				w.set_increments(1, 10)
				w.set_range(-pow(2, 31), pow(2, 31) - 1)
			elif type == "uint32":
				if enums == None:
					w = InputArea.InputTable.InputUInt32(iid, rng)
					w._connect("value_changed", self._cb_change_uint32, w)
				else:
					w = InputArea.InputTable.InputUInt32Enum(iid, rng, enums)
					w._connect("changed", self._cb_change_uint32_enum, w)
			elif type == "real32":
				w = InputArea.InputTable.InputReal(iid, rng)
				w._connect("value_changed", self._cb_change_real32, w)
			elif type == "real64":
				w = InputArea.InputTable.InputReal(iid, rng)
				w._connect("value_changed", self._cb_change_real64, w)
			elif type == "real64_vec3":
				w = InputArea.InputTable.InputRealVec(iid, rng, 3)
				w._connect("value_changed", self._cb_change_real64_vec, w)
			elif type == "real64_vec4":
				w = InputArea.InputTable.InputRealVec(iid, rng, 4)
				w._connect("value_changed", self._cb_change_real64_vec, w)
			elif type == "module":
				w = InputArea.InputTable.InputModule(iid)
				w._connect("changed", self._cb_change_module, w)
			elif type == "string":
				w = InputArea.InputTable.InputString(iid)
				w._connect("activate", self._cb_activate_string, w)
			else:
				w = gtk.Label(type)
			if cv == None:
				self._set_default(w, iid, rng)
			else:
				self._set_current(w, iid, cv)
			self.attach(w, 1, 2, index, index+1)
			self.inputs[index] = (label, type, w)
			return w

		def _refresh(self, sets, modules):
			"""Refresh widgets on page, given current sets and modules."""
			for s in sets:
				iid = s[0]				# Input ID of input to set.
				it = self.inputs[iid][1]		# Input type.
				self.inputs[iid][2]._block()		# Block widget, we don't want recursion here.
				self.inputs[iid][2].set(s, modules)
				self.inputs[iid][2]._unblock()

	def _get_input_set(self, mid, input):
		"""Retreive current value of a module's input. Returns (value, type) tuple, where both fields are strings from the XML. Returns None if input is not set."""
		try:		
#		print "getting current value for", (mid, input), ":"
			s = self.graph.xpathEval("graph/module[@id='%d']/set[@input='%d']" % (mid, input))[0]
#		print " set:", s
			t = s.xpathEval("@type")[0].content
#		print " type:", t
			v = s.content
#		print " value:", v
			return (v, t)
		except:	return None

	def _update_inputs(self, mid):
		"Refresh current page's input widgets by reading out current values in graph."
		try:	page = self.pages[mid]
		except:
			print "no page for module", mid, type(mid)
			print " pages:", self.pages
			return
#		print "update page module %d, page %d" % (mid, page[0])
		# Prepare handy list of (module,plug-in) tuples, so combos can be set up.
		mods = []
		for m in self.graph.xpathEval("graph/module[@id and @plug-in]"):
			mods.append((int(m.xpathEval("@id")[0].content), self.purpleinfo.plugin_get_name(int(m.xpathEval("@plug-in")[0].content))))
#		print "modules are:", mods
		# Also prepare handy list of input values, so everything can be set up.
		sets = []
		for s in self.graph.xpathEval("graph/module[@id='%d']/set" % mid):
			sets.append((int(s.xpathEval("@input")[0].content), s.xpathEval("@type")[0].content, s.content))
#		print "sets are:", sets
#		print "page is:", page
		page[1]._refresh(sets, mods)
			
	def set_graph(self, gid, graph):
		"Set a new XML graph to be the one we're processing inputs for."
		# If it's the same graph, don't wipe the UI.
		if gid != self.graph_id:
			while(self.get_n_pages() > 0):
				self.remove_page(0)
			self.graph_id = gid
		self.graph = graph	# Always store the new XML, though.

		# Just re-focus the module that was focused just before. It will do the right thing.
		self.set_focus(self.current_module)

	def _build_page(self, mid):
		"Build UI page for module <mid>."
		page = gtk.VBox()
		try:	m = self.graph.xpathEval("graph/module[@id='%d']" % mid)[0]
		except:
			print "failed to extract XML for module", mid, "in graph", self.graph_id
			return
		pid = int(m.xpathEval("@plug-in")[0].content)
		label = gtk.Label("<b>%s</b> (%u)" % (self.purpleinfo.plugin_get_name(pid), mid))
		label.set_use_markup(1)
		page.pack_start(label, 0, 0, 0)
		nid = self.purpleinfo.plugin_get_input_num(pid)
		page.pack_start(gtk.HSeparator(), 0, 0, 0)
		table = InputArea.InputTable(self, pid, nid)
		for i in range(nid):
			itype = self.purpleinfo.plugin_get_input_type(pid, i)
			label = self.purpleinfo.plugin_get_input_name(pid, i)
			rng = self.purpleinfo.plugin_get_input_range(pid, i)
			cv = self._get_input_set(mid, i)
			enums = self.purpleinfo.plugin_get_input_enums(pid, i)
#			print "range=", rng
#			print label, "has enums", enums
			table.add((mid, i), label, itype, cv, rng, enums)
		page.pack_start(table, 0, 0, 0)
		page.show_all()
		pnr = self.append_page(page)
#		print "built page number", pnr, "for module", mid, type(mid)
		self.pages[mid] = (pnr, table)
		self.current_module = mid
		self.current_sequence = int(m.xpathEval("@seq")[0].content)
#		print "**current sequence: ", self.current_sequence
		return pnr

	def set_focus(self, mid):
		"Set focus, i.e. the displayed inputs, to module with ID <mid>."
#		print "focusing module %d" % mid
		if mid == -1:
			if self.current_module >= 0:
				try:	p = self.pages[self.current_module]
				except:	p = None
				if p != None:
					del self.pages[self.current_module]
					self.remove_page(p[0])
#					print "something was deleted"
			self.current_module = -1
			return
		try:	p = self.pages[mid]
		except:	p = None
		if p == None:
			k = self._build_page(mid)
			p = self.pages[mid]
		modxml = self.graph.xpathEval("graph/module[@id='%u']" % mid)
		if len(modxml) == 0:
			self.set_focus(-1)
			return
		cpid = int(modxml[0].xpathEval("@plug-in")[0].content)
		mseq = int(modxml[0].xpathEval("@seq")[0].content)
#		print "focusing on plug-in %u, was %u" % (cpid, p[1].pid), "sequence is", mseq

		# If plug-in has changed, trash and rebuild the module.
		if cpid != p[1].pid:
			self.remove_page(p[0])
			self._build_page(mid)

			p = self.pages[mid]
		# Update to reflect current settings.
		self._update_inputs(mid)
		self.set_current_page(p[0])
		self.current_module = mid
		self.show()
