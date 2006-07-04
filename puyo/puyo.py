#! /usr/bin/env python
#
# "Puyo", a prototype-quality Purple interface client. Written by Emil Brink.
#
# Copyright (C) 2004-2005 PDC, KTH. See COPYING for license details.
#

import pygtk
pygtk.require("2.0")	# Silly.
import gobject
import gtk
import gtk.gtkgl
import string
import sys

import verse as v

import libxml2

from grapharea  import GraphArea
from inputarea  import InputArea
from purpleinfo import PurpleInfo

# Suppress error messages from libxml2.
def libxml2_noerr():
	pass

class Node:
	def __init__(self, node_id, type):
		self.id = node_id
		self.type = type
		self.name = ""
		the_db.add(self)

	def name_set(self, name):
		self.name = name
		print self.id, "renamed to", self.name

class MethodGroup:
	"Represents a group of methods in an object node"
	def __init__(self, gid, name):
		self.id = gid
		self.name = name
		self.methods = {}
		print "Method group ", self.id, ",", self.name, "created"

	def method_create(self, mid, name, args):
		self.methods[mid] = (mid, name, args)
		print self.name + "." + name, "created", args

	def method_get_named(self, name):
		for m in self.methods.values():
			if m[1] == name:
				return m
		return None

	def call(self, name, args):
		for m in self.methods:
			if m[1] == name:
				print "found call to ", name, " i.e. ", m[0]
				return

class NodeObject(Node):
	def __init__(self, node_id):
		Node.__init__(self, node_id, v.OBJECT)
		self.method_groups = {}

	def method_group_create(self, mid, name):
#		print "creating group", mid, name, "in", self
		self.method_groups[mid] = MethodGroup(mid, name)

	def method_group_get(self, mid):
		for g in self.method_groups.values():
			if g.id == mid:
				return g
		return None

	def method_group_get_named(self, name):
#		print "getting method group named", name, "in", self.method_groups
		for g in self.method_groups.values():
			if g.name == name:
				return g
		return None

class PurpleAvatar(NodeObject):
#	def __self__(id):
#		NodeObject.__init__(self, id)

	def __init__(self, node):
		print "initializing purpleavatar, copying", node
		Node.__init__(self, node.id, v.OBJECT)
		self.method_groups = node.method_groups

	def get_graph_method(self, name):
		g = self.method_group_get_named("PurpleGraph")
		return g, g.method_get_named(name)

	def graph_create():
		pass

	def graph_destroy():
		pass

	def _send_mod_input_set(self, typename, graph, module, input, value):
		group, m = self.get_graph_method("mod_set_" + typename)
		if m != None:
			print "sending input set", (graph, module, input), "to", value, "(%s)" % typename
			v.send_o_method_call(self.id, group.id, m[0], 0, (graph, module, input, value))

	def mod_create(self, graph, module):
		group,m = self.get_graph_method("mod_create")
		if m != None:
			v.send_o_method_call(self.id, group.id, m[0], 0, (graph, module))

	def mod_destroy(self, graph, module):
		group, m = self.get_graph_method("mod_destroy")
		if m != None:
			v.send_o_method_call(self.id, group.id, m[0], 0, (graph, module))

	def mod_input_clear(self, graph, module, input):
		group, m = self.get_graph_method("mod_input_clear")
		if m != None:
			print "sending input clear", (graph, module, input)
			v.send_o_method_call(self.id, group.id, m[0], 0, (graph, module, input))

	def mod_input_set_boolean(self, graph, module, input, value):
		self._send_mod_input_set("boolean", graph, module, input, value == True)

	def mod_input_set_uint32(self, graph, module, input, value):
		self._send_mod_input_set("uint32", graph, module, input, value)

	def mod_input_set_string(self, graph, module, input, value):
		self._send_mod_input_set("string", graph, module, input, value)

	def mod_input_set_real32(self, graph, module, input, value):
		self._send_mod_input_set("real32", graph, module, input, value)

	def mod_input_set_real64(self, graph, module, input, value):
		self._send_mod_input_set("real64", graph, module, input, value)

	def mod_input_set_real64_vec(self, graph, module, input, value):
		if len(value) == 3:
			self._send_mod_input_set("r64v3", graph, module, input, value);
	    	else:
			print "missing %u-dimension vector setting" % len(value)

	def mod_input_set_module(self, graph, module, input, value):
		self._send_mod_input_set("module", graph, module, input, value)

class TextBuffer:
	"Represents text in a text node."

	def __init__(self, node, buf_id, name):
		self.node = node		# Retain pointer to parent, for XML-checking.
		self.id = buf_id
		self.name = name
		self.text = ""
		self.xml = None
		self.graphs = None
		self.notify = []
		self.notify_next = 0

	def xml_notify_add(self, method, data):
		item = (self.notify_next, method, data)
		self.notify.append(item)
		self.notify_next += 1
		return item[0]

	def xml_notify_remove(self, not_id):
		for i in range(len(self.notify)):
			if self.notify[i][0] == not_id:
				del self.notify[i]
				return

	def xml_refresh(self):
		if not self.node.is_xml:
			return
		try:	doc = libxml2.parseDoc(self.text)
		except:	return
		if doc != None:
			if self.xml != None: self.xml.freeDoc()
			self.xml = doc
			for n in self.notify:
				n[1](self.xml, n[2])

	def set(self, pos, length, text):
		self.text = self.text[0:pos] + text + self.text[pos+length:]
		self.xml_refresh()

class NodeText(Node):
	def __init__(self, node_id):
		Node.__init__(self, node_id, v.TEXT)
		self.language = ""
		self.buffers = {}
		self.is_xml = 0

	def buffer_get(self, buf_id):
		buf_id = int(buf_id)
		try:
			b = self.buffers[buf_id]
		except:
			b = None
		return b

	def language_set(self, lang):
		self.language = lang
		if lang.find("xml", 0) == 0:
			self.is_xml = 1
			for b in self.buffers.values():
				b.xml_refresh()
		else:
			self.is_xml = 0

	def buffer_create(self, buf_id, name):
		buf_id = int(buf_id)	# Make sure.
		b = self.buffer_get(buf_id)
		if b != None:
			return b
		b = TextBuffer(self, buf_id, name)
		self.buffers[buf_id] = b
		print "added buffer", buf_id, ",", name, "in", self.id
		return b

	def text_set(self, buffer_id, pos, length, text):
		b = self.buffer_get(buffer_id)
		if b != None:
			b.set(pos, length, text)

	def text_get(self, buffer_id):
		b = self.buffer_get(buffer_id)
		if b != None:
			return b.text
		return None

class Db:
	def __init__(self):
		self.nodes = {}
		self.purple = None
		self.expect = []

	def send_create_named(self, type, name):
		v.send_node_create(~0, type, 0)
		self.expect.append((type, name))		

	def add(self, node, owner = v.OWNER_OTHER):
		self.nodes[node.id] = node
		if owner == v.OWNER_MINE:
			for e in self.expect:
				if e[0] == node.type:
					v.send_node_name_set(node.id, e[1])
					if node.type == v.TEXT:
						v.send_t_set_language(node.id, "xml/purple/graph")
					self.expect.remove(e)
					break
		return node


	def get(self, node_id):
		try:
			n = self.nodes[node_id]
		except:
			return None
		return n

	def get_named(self, name):
		for a in self.nodes.values():
#			print a.name, name
			if a.name == name:
				return a
		return None

	def get_typed(self, t):
		nodes = [ ]
		for a in self.nodes.values():
			if a.type == t:
				nodes.append(a)
		return nodes

	def typeof(self, node_id):
		return self.nodes[node_id].type

def node_get(node_id, type, owner = v.OWNER_OTHER):
	"Get a node from the database, or create it if it doesn't exist."
	n = the_db.get(node_id)
	if n != None:
		return n
	if type == v.OBJECT:
		return the_db.add(NodeObject(node_id), owner)
	elif type == v.TEXT:
		return the_db.add(NodeText(node_id), owner)

# ---------------------------------------------------------------------------------------------

def childElement(n, name):
	for ch in n.childNodes:
		if ch.nodeType == xml.dom.Node.ELEMENT_NODE:
			if ch.nodeName == name:
				return ch
	return None

class GraphHandle:
	def __init__(self, graphId, nodeName, bufferId):
		self.graphId = graphId
		self.node = the_db.get_named(nodeName)
		self.bufferId = bufferId

class Puyo:

	def evt_destroy(self, widget, user):
		gtk.main_quit()

	def cb_current_graph_update(self, xml, data):
		self.inputs.set_graph(self.currentGraph[0][0], xml)
		self.area.set_graph(self.currentGraph[0][0], xml)

	# Look through the parsed XML purple-graphs document for a graph named <name>.
	# Returns a (graph-id,node-name,buffer-id) tuple.
	def graph_find(self, name):
		q = "purple-graphs/graph[@id and @name='" + name + "']"
		r = self.graphs.xpathEval(q)
		if len(r) == 1:
			r = r[0]
			gid = int(r.xpathEval("@id")[0].content)
			nn  = r.xpathEval("at/node")[0].content
			bid = int(r.xpathEval("at/buffer")[0].content)
		        return (gid, nn, bid)
		return None

	def get_string(self, help, label, preset = None):
		dlg = gtk.Dialog()
		dlg.vbox.pack_start(gtk.Label(help), False, False)
		hbox = gtk.HBox()
		hbox.pack_start(gtk.Label("Name:"))
		entry = gtk.Entry()
		if preset != None:
			entry.set_text(preset)
		hbox.pack_start(entry)
		dlg.vbox.pack_start(hbox)
		dlg.add_button(gtk.STOCK_OK,     1)
		dlg.add_button(gtk.STOCK_CANCEL, 0)
		dlg.set_default_response(1)
		dlg.show_all()
		entry.grab_focus()
		r = dlg.run()
		if r == 1:
			ret = entry.get_text()
		else:	ret = None		# Differentiate between "" and cancel.
		dlg.destroy()
		return ret

	# Open dialog that lets user pick a text node and a buffer. Returns buffer object pointer.
	def pick_buffer(self, edit = False, help = None):
		def _get_buffer():
			buffer = None
			model, iter = nview.get_selection().get_selected()
			if iter != None and model.iter_parent(iter) != None:
				buffer = model.get_value(iter, 0)
			return buffer

		def _create_node(*args):
			n = self.get_string("Enter name of new\ntext node:", "Name:")
			if n != None:
				n = string.strip(n)
				if len(n) > 0:
					the_db.send_create_named(v.TEXT, n)
#			cdlg.destroy()

		def _create_buffer(*args):
			b = _get_buffer()
			if b == None:
				model, iter = nview.get_selection().get_selected()
				if iter != None and model.iter_parent(iter) == None:
					node = model.get_value(iter, 0)
			else:
				node = b.node
			if node != None:
				n = self.get_string("Enter name of new\ntext buffer:", "Name:")
				if n != None:
					n = string.strip(n)
					if len(n) > 0:
						v.send_t_buffer_create(node.id, ~0, n)
#				cdlg.destroy()

		def _refresh(*args):
			model = gtk.TreeStore(gobject.TYPE_PYOBJECT, gobject.TYPE_STRING)
			nodes = the_db.get_typed(v.TEXT)
			for a in nodes:
				iter = model.insert_after(None, None)
				model.set_value(iter, 0, a)
				model.set_value(iter, 1, a.name)
				if len(a.buffers) > 0:
					for b in a.buffers.values():
						chiter = model.insert_after(iter, None)
						model.set_value(chiter, 0, b)
						model.set_value(chiter, 1, b.name)
			nview.set_model(model)
			nview.expand_all()

		nview = gtk.TreeView()
		renderer = gtk.CellRendererText()
		column = gtk.TreeViewColumn("Text Nodes", renderer, text = 1)
		nview.append_column(column)
		dlg = gtk.Dialog()
		if help != None:
			dlg.vbox.pack_start(gtk.Label(help))
		dlg.vbox.pack_start(nview)
		dlg.add_button(gtk.STOCK_OK,     1)
		dlg.add_button(gtk.STOCK_CANCEL, 0)
		dlg.set_default_response(1)
		hbox = gtk.HBox()
		if edit:
			cbut = gtk.Button("Create Node")
			cbut.connect("clicked", _create_node)
			hbox.pack_start(cbut)
			cbut = gtk.Button("Create Buffer")
			cbut.connect("clicked", _create_buffer)
			hbox.pack_start(cbut)
		rbut = gtk.Button("Refresh")
		rbut.connect("clicked", _refresh)
		hbox.pack_start(rbut)
		dlg.vbox.pack_start(hbox, False)
		_refresh()
		dlg.show_all()
		dlg.run()
		buffer = _get_buffer()
		dlg.destroy()
		return buffer

	def evt_action_graph_create(self, action, user):
		buf = self.pick_buffer(True, help="Pick a text node and a buffer in\nwhich to create a new graph:")
		if buf != None:
			group, m = the_db.purple.get_graph_method("create")
			if m != None:
				n = self.get_string("Enter name of new\ngraph to create:", "Name:", "graphN")
				print "about to create graph in", buf.node.id, buf.id
				v.send_o_method_call(the_db.purple.id, group.id, m[0], 0, (buf.node.id, buf.id, n))

	def pick_graph(self, help = None):
		"""Pop up a dialog showing existing graphs, letting the user pick one to edit."""

		def _get_selection(view):
			"""Returns quadtuple of (graph id, name, text node id, buffer id) for the selection, or None."""
			sel = view.get_selection()
			if sel != None:
				model, iter = sel.get_selected()
				if iter != None:
					return (int(model.get_value(iter, 0)), model.get_value(iter, 1), model.get_value(iter, 2), int(model.get_value(iter, 3)))
			return None
			

		def _build_model(graphs):
			model = gtk.ListStore(gobject.TYPE_STRING, gobject.TYPE_STRING, gobject.TYPE_STRING, gobject.TYPE_STRING)
			for g in graphs.xpathEval("purple-graphs/graph[@id and @name]"):
				gid   = g.xpathEval("@id")[0].content
				gname = g.xpathEval("@name")[0].content
				nname = g.xpathEval("at/node")[0].content
				bid   = g.xpathEval("at/buffer")[0].content
				iter = model.insert_after(None, None)
				model.set_value(iter, 0, gid)
				model.set_value(iter, 1, gname)
				model.set_value(iter, 2, nname)
				model.set_value(iter, 3, bid)
			return model

		view = gtk.TreeView()
		renderer = gtk.CellRendererText()
		column = gtk.TreeViewColumn("Graph ID", renderer, text = 0)
		view.append_column(column)
		column = gtk.TreeViewColumn("Graph Name", renderer, text = 1)
		view.append_column(column)
		column = gtk.TreeViewColumn("Node Name", renderer, text = 2)
		view.append_column(column)
		column = gtk.TreeViewColumn("Buffer ID", renderer, text = 3)
		view.append_column(column)
		view.set_model(_build_model(self.graphs))
		view.set_size_request(-1, 128)

		dlg = gtk.Dialog()
		if help != None:
			dlg.vbox.pack_start(gtk.Label(help), False, False)
		scwin = gtk.ScrolledWindow()
		scwin.set_policy(gtk.POLICY_NEVER, gtk.POLICY_AUTOMATIC)
		scwin.add(view)
		dlg.vbox.pack_start(scwin)
		dlg.add_button(gtk.STOCK_OK,     1)
		dlg.add_button(gtk.STOCK_CANCEL, 0)
		dlg.set_default_response(1)
		dlg.show_all()
		r = dlg.run()
		g = None
		if r == 1:
			g = _get_selection(view)
		dlg.destroy()
		return g

	def switch_to_graph(self, g):
		print "graph:", g

	def evt_action_graph_edit(self, action, user):
		g = self.pick_graph("Pick graph to edit:")
		self.switch_to_graph(g)
		if g == None:
			return
		what = self.graph_find(g[1])
		if what == None: return
		n = the_db.get_named(what[1])
		if n != None:
			if self.currentGraph != None:
				oldWhat = self.currentGraph[0]
				oldNode = self.currentGraph[1]
				if oldNode != None:
					oldBuf  = oldNode.buffer_get(oldWhat[2])
					ob = oldNode.buffer_get(oldWhat[2])
					if ob != None:
						ob.xml_notify_remove(self.currentGraph[2])
			buf = n.buffer_get(what[2])
			self.currentGraph = (what, n, buf.xml_notify_add(self.cb_current_graph_update, None))
			self.inputs.set_graph(what[0], buf.xml)
			self.area.set_graph(what[0], buf.xml)
		else:
			print "*** Couldn't set current graph, no node named", what[1], "found"

	def evt_action_graph_destroy(self, action, user):
		group, m = the_db.purple.get_graph_method("destroy")
		if m != None:
			g = self.pick_graph("Pick graph to destroy:")
			if g != None:
				v.send_o_method_call(the_db.purple.id, group.id, m[0], 0, (g[0], ))

	def evt_action_module_align_left(self, action, user):
		self.area.module_align(gtk.ARROW_LEFT)

	def evt_action_module_align_right(self, action, user):
		self.area.module_align(gtk.ARROW_RIGHT)

	def evt_action_module_align_top(self, action, user):
		self.area.module_align(gtk.ARROW_UP)

	def evt_action_module_align_bottom(self, action, user):
		self.area.module_align(gtk.ARROW_DOWN)

	def evt_action_module_spread_horiz(self, action, user):
		self.area.module_spread(gtk.ORIENTATION_HORIZONTAL)

	def evt_action_module_spread_vert(self, action, user):
		self.area.module_spread(gtk.ORIENTATION_VERTICAL)

	def evt_action_view_zoom_one(self, action, user):
		"Handle the ViewZoomOne action."
		self.area.zoom_normal()

	def evt_action_view_zoom_to_fit(self, action, user):
		"Zoom view so that selected, or all, modules fit the view."
		if self.currentGraph == None: return
		self.area.zoom_to_fit()

	def __init__(self):
		self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
		self.window.connect("delete_event", self.evt_destroy)
		self.window.set_default_size(640, 480)
		self.currentGraph = None	# (Descriptor tuple, node, notify ID).
#		self.accelGroup = gtk.AccelGroup()
		ag = gtk.ActionGroup("globals")
		ag.add_action(gtk.Action("GraphMenu", "Graphs", "Commands for working with graphs", None))
		self.actionGraphCreate = gtk.Action("GraphCreate", "Create...", "Create a new graph", gtk.STOCK_NEW)
		self.actionGraphCreate.connect("activate", self.evt_action_graph_create, self)
		ag.add_action_with_accel(self.actionGraphCreate, None)
		self.actionGraphEdit = gtk.Action("GraphEdit", "Edit...", "Edit an existing graph", gtk.STOCK_OPEN)
		self.actionGraphEdit.connect("activate", self.evt_action_graph_edit, self)
		ag.add_action_with_accel(self.actionGraphEdit, None)
		self.actionGraphDestroy = gtk.Action("GraphDestroy", "Destroy...", "Destroy an existing graph", gtk.STOCK_DELETE)
		self.actionGraphDestroy.connect("activate", self.evt_action_graph_destroy, self)
		ag.add_action(self.actionGraphDestroy)
		ag.add_action_with_accel(gtk.Action("GraphQuit", "Quit", "Quit the application", gtk.STOCK_QUIT), None)

		ag.add_action(gtk.Action("ModuleMenu", "Module", "Module commands", None))
		self.actionModuleCreate = gtk.Action("ModuleCreateMenu",  "Create", "Create a new module in current graph", None)
		self.actionModuleCreate.set_property("is-important", True)
		ag.add_action(self.actionModuleCreate)
		self.actionModuleDestroy = gtk.Action("ModuleDestroy", "Destroy", "Destroy currently selected graph module", gtk.STOCK_DELETE)
		self.actionModuleDestroy.connect("activate", self.evt_module_destroy, self)
		ag.add_action_with_accel(self.actionModuleDestroy, "Delete")

		ag.add_action(gtk.Action("ModuleAlignMenu", "Align", "Commands for aligning modules", None))
		self.actionModuleAlignLeft = gtk.Action("ModuleAlignLeft", "Left", "Align selected modules to the left", None)
		self.actionModuleAlignLeft.connect("activate", self.evt_action_module_align_left, self)
		ag.add_action(self.actionModuleAlignLeft)
		self.actionModuleAlignRight = gtk.Action("ModuleAlignRight", "Right", "Align selected modules to the right", None)
		ag.add_action(self.actionModuleAlignRight)
		self.actionModuleAlignRight.connect("activate", self.evt_action_module_align_right, self)
		self.actionModuleAlignTop = gtk.Action("ModuleAlignTop", "Top", "Align selected modules to the top", None)
		ag.add_action(self.actionModuleAlignTop)
		self.actionModuleAlignTop.connect("activate", self.evt_action_module_align_top, self)
		self.actionModuleAlignBottom = gtk.Action("ModuleAlignBottom", "Bottom", "Align selected modules to the bottom", None)
		ag.add_action(self.actionModuleAlignBottom)
		self.actionModuleAlignBottom.connect("activate", self.evt_action_module_align_bottom, self)

		ag.add_action(gtk.Action("ModuleSpreadMenu", "Spread", "Commands for spreading modules", None))
		self.actionModuleSpreadHoriz = gtk.Action("ModuleSpreadHoriz", "Horizontally", "Spread selected modules horizontally", None)
		ag.add_action(self.actionModuleSpreadHoriz)
		self.actionModuleSpreadHoriz.connect("activate", self.evt_action_module_spread_horiz, self)
		self.actionModuleSpreadVert = gtk.Action("ModuleSpreadVert", "Vertically", "Spread selected modules vertically", None)
		ag.add_action(self.actionModuleSpreadVert)
		self.actionModuleSpreadVert.connect("activate", self.evt_action_module_spread_vert, self)

		ag.add_action(gtk.Action("ViewMenu", "View", "Commands for working with the view", None))
		ag.add_action_with_accel(gtk.Action("ViewZoomIn", "Zoom In", "Zoom the view in, making things larger", gtk.STOCK_ZOOM_IN), "<Ctrl>plus")
		ag.add_action_with_accel(gtk.Action("ViewZoomOut", "Zoom Out", "Zoom the view out, making things smaller", gtk.STOCK_ZOOM_OUT), "<Ctrl>minus")
		self.actionViewZoomOne = gtk.Action("ViewZoomOne", "Normal Size", "Zoom to normal size", gtk.STOCK_ZOOM_100)
		self.actionViewZoomOne.connect("activate", self.evt_action_view_zoom_one, self)
		ag.add_action_with_accel(self.actionViewZoomOne, "<ctrl>period")
		self.actionViewZoomFit = gtk.Action("ViewZoomFit", "Best Fit", "Zoom the view so that current selection is visible", gtk.STOCK_ZOOM_FIT)
		self.actionViewZoomFit.connect("activate", self.evt_action_view_zoom_to_fit, self)
		ag.add_action_with_accel(self.actionViewZoomFit, "<ctrl><shift>period")

		ui = """
<ui>
  <menubar name="MainBar">
<!--
    <menu name="Server" action="GraphMenu">
     <menuitem name="Connect..." action="GraphCreate"/>
     <menuitem name="Disconnect" action="GraphCreate"/>
     <separator/>
     <menuitem name="Quit" action="GraphQuit"/>
    </menu>
-->
    <menu name="GraphMenu" action="GraphMenu">
     <menuitem name="Create"  action="GraphCreate"/>
     <separator/>
     <menuitem name="Edit"    action="GraphEdit"/>
     <menuitem name="Destroy" action="GraphDestroy"/>
     <separator/>
<!--     <menuitem name="Quit"    action="GraphQuit"/>-->
    </menu>
    <menu name="ModuleMenu" action="ModuleMenu">
     <menu name="ModuleCreateMenu" action="ModuleCreateMenu">
      <menuitem name="Foo" action="GraphQuit"/>
     </menu>
     <menuitem name="Destroy" action="ModuleDestroy"/>
     <menu name="ModuleAlignMenu" action="ModuleAlignMenu">
      <menuitem name="Left"   action="ModuleAlignLeft"/>
      <menuitem name="Right"  action="ModuleAlignRight"/>
      <menuitem name="Top"    action="ModuleAlignTop"/>
      <menuitem name="Bottom" action="ModuleAlignBottom"/>
     </menu>
     <menu name="ModuleSpreadMenu" action="ModuleSpreadMenu">
      <menuitem name="Horiz"   action="ModuleSpreadHoriz"/>
      <menuitem name="Vert"    action="ModuleSpreadVert"/>
     </menu>
    </menu>
    <menu name="View" action="ViewMenu">
     <menuitem name="ZoomIn"  action="ViewZoomIn"/>
     <menuitem name="ZoomOut" action="ViewZoomOut"/>
     <menuitem name="ZoomOne" action="ViewZoomOne"/>
     <menuitem name="ZoomFit" action="ViewZoomFit"/>
    </menu>
  </menubar>
</ui>
"""
		self.ui = gtk.UIManager()
		self.ui.insert_action_group(ag, 0)
		self.ui.add_ui_from_string(ui)
		vbox = gtk.VBox()
		mbar = self.ui.get_widget("/MainBar")
		vbox.pack_start(mbar, 0, 0)
		self.menu_tools = self.ui.get_widget("/MainBar/ModuleMenu/ModuleCreateMenu").get_submenu()	# Ha!

		hbox = gtk.HBox()
	        display_mode = (gtk.gdkgl.MODE_RGB | gtk.gdkgl.MODE_DOUBLE)
		try:
			glconfig = gtk.gdkgl.Config(mode = display_mode)
		except:
			return

		self.inputs = InputArea(the_purpleinfo)
		self.area = GraphArea(glconfig, self.inputs, the_purpleinfo)
		self.area.set_action_group(ag)

		hbox.pack_start(self.area)
#		self.inputs.append_page(gtk.HScale(), gtk.Label("Page"))	# Dummy widget to make notebook wake up. Not needed?
		hbox.pack_start(self.inputs, 0, 0, 0)
		vbox.pack_start(hbox)
		self.window.add(vbox)

		self.window.show_all()

	def graph_current_refresh(self, buffer):
		area.set_graph(self.currentGraph[0][0], buffer.xml)
		area.refresh()

	def graphs_refresh(self, xml, buffer):
		self.graphs = xml

	def evt_module_create(self, *args):
    		if the_db.purple == None: return
		print "current graph:", self.currentGraph
		the_db.purple.mod_create(self.currentGraph[0][0], int(args[1]))

	def evt_module_destroy(self, *args):
		if the_db.purple == None: return
		for m in self.area.selection:
			the_db.purple.mod_destroy(self.currentGraph[0][0], m)

	def plugins_refresh(self, xmldoc, buffer):
		the_purpleinfo.doc_set(xmldoc)
		plugins = the_purpleinfo.plugins_get_desc()
		pos = 0
		for p in plugins:
			i = gtk.MenuItem(p[0], 0)
			self.menu_tools.insert(i, pos)
			i.connect("activate", self.evt_module_create, p[1])
			pos += 1
		self.menu_tools.show_all()
		# Hide the dummy Quit entry that is in the menu to make it non-empty.
		cq = self.ui.get_widget("/MainBar/ModuleMenu/ModuleCreateMenu/Foo")
		cq.hide()

	def main(self):
		gtk.main()

# ---------------------------------------------------------------------------------------------

def cb_t_text_set(node_id, buffer_id, pos, length, text):
	n = node_get(node_id, v.TEXT)
	n.text_set(buffer_id, pos, length, text)

def cb_t_buffer_create(node_id, buffer_id, name):
	n = node_get(node_id, v.TEXT)
	b = n.buffer_create(buffer_id, name)
	v.send_t_buffer_subscribe(node_id, buffer_id)
	if name == "plugins":
		b.xml_notify_add(the_puyo.plugins_refresh, b)
	elif name == "graphs":
		b.xml_notify_add(the_puyo.graphs_refresh, b)

def cb_t_set_language(node_id, language):
	n = node_get(node_id, v.TEXT)
	print "Setting language of", node_id, "to", language
	n.language_set(language)

def cb_o_link_set(node_id, link_id, link, label, target_id):
	print "Link from " + str(node_id) + "->" + str(link)

def cb_o_method_create(node_id, group_id, method_id, name, args):
	n = the_db.get(node_id)
	if n != None:
		g = n.method_group_get(group_id)
		if g != None:
			g.method_create(method_id, name, args)
		else:
			print " but group", group_id, "not found in node :/"
	else:
		print " but node not found in database"
			

def cb_o_method_group_create(node_id, group_id, name):
#	if the_db.purple == None:
#		print "Purple server not found ... weird"
#		return
#	if node_id != the_db.purple.id:
#		print node_id, " is not purple (", the_db.purple, "), aborting method create"
#		return
	print "method group: ", node_id, group_id, name
	if name == "PurpleGraph":
		n = the_db.get(node_id)
		if n != None:
			n.method_group_create(group_id, name)
			v.send_o_method_group_subscribe(node_id, group_id)
			print " subscribed to method group"

def cb_node_name_set(node_id, name):
	print "Node " + str(node_id) + " is named " + name
	n = node_get(node_id, v.OBJECT)
	if n != None:
		n.name_set(name)
		if name == "PurpleEngine":
			the_db.purple = PurpleAvatar(n)
			the_puyo.area.set_purple(the_db.purple)
			the_puyo.inputs.set_purple(the_db.purple)
			print "found Purple engine avatar"

def cb_node_create(node_id, type, owner):
	node_get(node_id, type, owner)
	v.send_node_subscribe(node_id)
	print "subcribing to type", type, "node", node_id

def cb_connect_accept(my_avatar, address, host_id):
	print "Connected as " + str(my_avatar) + " to " + str(address)
	v.send_node_name_set(my_avatar, "Puyo")
	v.send_node_index_subscribe(1 << v.OBJECT | 1 << v.TEXT)

def evt_timeout():
	v.callback_update(10000)
	return 1

if __name__ == "__main__":
	server = "localhost"

	for a in sys.argv[1:]:
		if a.startswith("-ip="):
			server = a[4:]
		elif a.startswith("-help"):
			print "Usage: puyo [-ip=SERVER[:HOST]]"
			print "Puyo is a graphical Verse client, that acts as an interface to"
			print "a Purple engine running on the same server. For more information,"
			print "please see <http://www.uni-verse.org/>, <http://verse.blender.org/>,"
			print "and <http://purple.blender.org/>."
			sys.exit(0)
		else:
			print "Unknown option", a

	the_purpleinfo = PurpleInfo()
	the_puyo = Puyo()
	the_db = Db()
	gobject.timeout_add(50, evt_timeout)

	libxml2.registerErrorHandler(libxml2_noerr, None)

	v.callback_set(v.SEND_CONNECT_ACCEPT,		cb_connect_accept)
	v.callback_set(v.SEND_NODE_CREATE,		cb_node_create)
	v.callback_set(v.SEND_NODE_NAME_SET,		cb_node_name_set)
	v.callback_set(v.SEND_O_LINK_SET,		cb_o_link_set)
	v.callback_set(v.SEND_O_METHOD_GROUP_CREATE,	cb_o_method_group_create)
	v.callback_set(v.SEND_O_METHOD_CREATE,		cb_o_method_create)
	v.callback_set(v.SEND_T_SET_LANGUAGE,		cb_t_set_language)
	v.callback_set(v.SEND_T_BUFFER_CREATE,		cb_t_buffer_create)
	v.callback_set(v.SEND_T_TEXT_SET,		cb_t_text_set)

	v.send_connect("puyo", "<secret>", server, 0)
   
	the_puyo.main()

	v.send_connect_terminate(server, "User quitting")
	while v.session_get_size() > 10: pass
