#
# grapharea.py -- Code for displaying and editing Purple graphs.
#
# Copyright (C) 2004-2005 PDC, KTH. See COPYING for license details.
#

import gtk
import gtk.gtkgl

from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *

TITLE_HEIGHT=0.40
INPUT_HEIGHT=0.38
ARROW_WIDTH=0.15
ARROW_MARGIN=0.04

def _set_color_fg():
	glColor3(0.8, 0.6, 0.8)

def _set_color_fg_light():
	glColor3(0.9, 0.7, 0.9)

def _set_color_bg():
 	glColor3(0.5, 0.3, 0.5)

def _set_color_bg_dark():
	glColor3(0.4, 0.2, 0.4)

def _set_color_sel():
	glColor3(0.9, 0.8, 0.9)

def _set_color_input():
	glColor3(0.7, 0.5, 0.7)

def _set_shadow_dark():
	glColor4(0, 0, 0, 0.5)

def _set_shadow_light():
	glColor4(0, 0, 0, 0)

def _rect_shadow(origin, size, factor = 0.05):
	"Draw a fancy drop shadow, by simply filling out a black rectangle, slightly offset."
	bevel = size[0] * factor
	x = origin[0] + bevel
	y = origin[1] - bevel
	lx = x
	rx = x + size[0]
	by = y - size[1]
	ty = y
	glBegin(GL_TRIANGLE_FAN)
	_set_shadow_light()
	glVertex2d(lx + bevel, by)
	_set_shadow_dark()
	glVertex2d(lx, by + bevel)

	_set_shadow_dark()
	glVertex2d(lx, ty - bevel)
	_set_shadow_dark()
	glVertex2d(lx + bevel, ty)

	_set_shadow_dark()
	glVertex2d(rx - bevel, ty)
	_set_shadow_light()
	glVertex2d(rx, ty - bevel)

	_set_shadow_light()
	glVertex2d(rx, by + bevel)
	_set_shadow_light()
	glVertex2d(rx - bevel, by)
	glEnd()

def _rect_filled(origin, size, factor = 0.05):
	bevel = size[0] * factor
	x = origin[0]
	y = origin[1]
	lx = x
	rx = x + size[0]
	by = y - size[1]
	ty = y
	glBegin(GL_TRIANGLE_FAN)
	glVertex2d(lx + bevel, by)
	glVertex2d(lx, by + bevel)

	glVertex2d(lx, ty - bevel)
	glVertex2d(lx + bevel, ty)

	glVertex2d(rx - bevel, ty)
	glVertex2d(rx, ty - bevel)

	glVertex2d(rx, by + bevel)
	glVertex2d(rx - bevel, by)
	glEnd()

def _rect(origin, size, factor = 0.05):
	bevel = size[0] * factor
	x = origin[0]
	y = origin[1]
	lx = x
	rx = x + size[0]
	by = y - size[1]
	ty = y
	glLineWidth(2.5)
	glBegin(GL_LINE_LOOP)
	glVertex2d(lx + bevel, by)
	glVertex2d(lx, by + bevel)

	glVertex2d(lx, ty - bevel)
	glVertex2d(lx + bevel, ty)

	glVertex2d(rx - bevel, ty)
	glVertex2d(rx, ty - bevel)

	glVertex2d(rx, by + bevel)
	glVertex2d(rx - bevel, by)
	glEnd()
	glLineWidth(1)

# Draw bevelled rectangle with top left corner at origin, and extending size to the right and down.
def _beveled_rect(origin, size, factor = 0.05):
	bevel = size[0] * factor
	x = origin[0]
	y = origin[1]
	lx = x
	rx = x + size[0]
	by = y - size[1]
	ty = y
	glEnable(GL_BLEND)
	glBlendFunc(GL_SRC_ALPHA,  GL_ONE_MINUS_SRC_ALPHA)
#	_rect_shadow(origin, size, factor)
	glDisable(GL_BLEND)
	_set_color_bg()
	glBegin(GL_TRIANGLE_FAN)
	glVertex2d(lx + bevel, by)
	glVertex2d(lx, by + bevel)

	glVertex2d(lx, ty - bevel)
	glVertex2d(lx + bevel, ty)

	glVertex2d(rx - bevel, ty)
	glVertex2d(rx, ty - bevel)

	_set_color_bg_dark()
	glVertex2d(rx, by + bevel)
	glVertex2d(rx - bevel, by)
	glEnd()
	_set_color_bg()

	glLineWidth(2.5)
	_set_color_fg()
	glBegin(GL_LINE_LOOP)
	glVertex2d(lx + bevel, by)
	glVertex2d(lx, by + bevel)

	glVertex2d(lx, ty - bevel)
	glVertex2d(lx + bevel, ty)

	glVertex2d(rx - bevel, ty)
	glVertex2d(rx, ty - bevel)

	glVertex2d(rx, by + bevel)
	glVertex2d(rx - bevel, by)
	glEnd()
	glLineWidth(1)

def _arrow_right_paint(pos, size):
	glBegin(GL_TRIANGLES)
	glVertex2d(pos[0], pos[1])
	glVertex2d(pos[0] + size[0], pos[1] - size[1] / 2)
	_set_color_fg_light()
	glVertex2d(pos[0], pos[1] - size[1])
	glEnd()
	_set_color_fg()

class GraphArea(gtk.DrawingArea, gtk.gtkgl.Widget):
	class Module:
		def _rounded_rect(origin, size):
			x = origin[0]
			y = origin[1]
			lx = x - size[0]
			rx = x + size[0]
			ty = y + size[1]
			by = y - size[1]
			glBegin(GL_TRIANGLE_FAN)
			glVertex2(lx, ty)
			glVertex2(lx, by)
			glVertex2(rx, by)
			glVertex2(rx, ty)
			glEnd()

		def __init__(self):
			pass

	def __init__(self, glconfig, inputs, purpleinfo):
		"Initialize a GraphArea."
		gtk.DrawingArea.__init__(self)
		self.add_events(gtk.gdk.BUTTON_PRESS_MASK | gtk.gdk.BUTTON_RELEASE_MASK | gtk.gdk.BUTTON_MOTION_MASK)
#		self.add_events(gtk.gdk.BUTTON_MOTION_MASK)

		self.set_gl_capability(glconfig)

		self.connect_after("realize",        self._evt_realize)
		self.connect("configure_event",      self._evt_configure)
		self.connect("expose_event",         self._evt_expose)
		self.connect("button_press_event",   self._evt_button_press)
		self.connect("button_release_event", self._evt_button_release)
		self.connect("motion_notify_event",  self._evt_motion_notify)
		self.connect("scroll_event",	     self._evt_scroll)

		self.inputs = inputs
		self.purpleinfo = purpleinfo

		self.graph_id = -1
		self.graph = None
		self.graphExtra = {}		# ((x,y), (w,h)) tuples keyed on module ID.

		self.home = (0, 0)
		self.homeActual = (0, 0)	# The one used in display (interpolated).
		self.zoom = 1.0
		self.zoomActual = 1.0		# The one used in display (interpolated).
		self.drag        = 0
		self.drag_anchor = None
		self.drag_now    = None
		self.drag_last   = None
		self.drag_output = -1
		self.drag_input  = (-1, -1)	# Module and input IDs of hovered-over input.
		self.selection   = []
		self.sel_input   = (-1, -1)	# Module and input IDs of selected input.
		self.purple = None

	def set_graph(self, id, graph):
		self.graph_id = id
		self.graph = graph
		self.refresh()
		# Filter out non-existing module IDs from selection. Handy after module delete.
		for id in self.selection:
			q = "graph/module[@id='%s']" % str(id)
			if not self.graph.xpathEval(q): self.selection.remove(id)

	def set_purple(self, purple):
		self.purple = purple

	def refresh(self):
		"Dead stupid refresh method, that simply queues a redraw. Handy when graph changes."
		self.queue_draw_area(0, 0, self.allocation.width, self.allocation.height)

	def string_render_boxed(self, pos, size, string, columns = -1, lines = 1):
		if columns < 1 : columns = len(string)
		glPushMatrix()
		glTranslate(pos[0], pos[1], 0.0)
		FW = 104.76
		FT = 119.05+8
		FB = 33.33
		FH = FT+FB
		glScale(size[0] / (columns * FW), size[1] / (lines * FH), 1.0)
		glTranslated(0.0, -FT, 0.0)
		glPushMatrix()
		glTranslated(8.0, -6.0, 0.0)	# Drop shadow.
		glColor3(0.0, 0.0, 0.0)
		if self.zoom <= 0.5:	glLineWidth(1.0)
		else:			glLineWidth(2.0)
#		glRasterPos2d(pos[0], pos[1] - 6.0)
		for ch in string:
#			glutBitmapCharacter(GLUT_BITMAP_8_BY_13, ord(ch))
			glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, ord(ch))
		glPopMatrix()
		_set_color_fg()
#		glRasterPos2d(pos[0], pos[1])
		for ch in string:
#			glutBitmapCharacter(GLUT_BITMAP_8_BY_13, ord(ch))
			glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, ord(ch))
		glLineWidth(1.0)
		glPopMatrix()

	def _bezier(self, a, b, c, d, t):
		"""Return interpolated point for Bezier curve defined by four control points a,b,c,d, at parameter t (t in 0..1 range)."""
		omt = 1.0 - t
		x = a[0] * omt ** 3 + 3 * b[0] * t * omt ** 2 + 3 * c[0] * t ** 2 * omt + d[0] * t ** 3
		y = a[1] * omt ** 3 + 3 * b[1] * t * omt ** 2 + 3 * c[1] * t ** 2 * omt + d[1] * t ** 3
		return (x, y)

	def _connector_paint(self, x1, y1, x2, y2, steps = 12):
	    	glBegin(GL_LINE_STRIP)
		glVertex2d(x1, y1)
		steps = float(steps)
		for oot in range(1, int(steps) + 1):
			t = oot / steps
			p = self._bezier((x1, y1), (x2, y1), (x1, y2), (x2, y2), t)
			glVertex2d(p[0], p[1])
		glEnd()

	def _connections_paint(self, mid, xtra):
		"""Paint lines from module outputs to inputs referencing them."""
		res = self.graph.xpathEval("graph/module[@id='" + str(int(mid)) + "']/set[@type='module']")
		_set_color_fg()
		glLineWidth(2.5)
		for i in res:
			src = int(i.content)
			try:	srcx = self.graphExtra[src]
			except: continue
			indx = int(i.xpathEval("@input")[0].content)
			self._connector_paint(srcx[0][0] + srcx[1][0], srcx[0][1] - 0.20, xtra[0][0], xtra[0][1] - TITLE_HEIGHT - INPUT_HEIGHT / 2 - INPUT_HEIGHT * indx)
		glLineWidth(1.0)

	def _graph_paint(self):
		x = 0
		y = 0
		gw = 1.0
		gth = TITLE_HEIGHT
		modules = self.graph.xpathEval("graph/module[@id and @plug-in]")
		conn = []
		for m in modules:
			mid = int(m.xpathEval("@id")[0].content)
			mpi = m.xpathEval("@plug-in")[0].content
			n = self.purpleinfo.plugin_get_name(mpi)
			if n != None:
				inps = self.purpleinfo.plugin_get_input_num(mpi)
				h = gth + inps * INPUT_HEIGHT
				try:	xtra = self.graphExtra[mid]
				except:	xtra = None
				if xtra == None:
					xtra = ((x,y), (gw, h))
					self.graphExtra[mid] = xtra
				else:
					x = xtra[0][0]
					y = xtra[0][1]
				_beveled_rect((x, y), (gw, h))
				glBegin(GL_LINES)	# Separator line between name and inputs.
				glVertex2d(x, y - gth)
				glVertex2d(x + gw, y - gth)
				glEnd()
				self.string_render_boxed((x + 0.05, y), (gw - 0.10, gth), n, 16)
				if mpi != "2":	# Know about node-output, and don't draw output arrow for it. Sneaky.
					_arrow_right_paint((x + gw - ARROW_WIDTH, y - ARROW_MARGIN), (ARROW_WIDTH, gth - 2 * ARROW_MARGIN))
				# Highlight hovered-over input, if any.
				if mid == self.drag_input[0] and self.drag_input[1] >= 0:
					i = self.drag_input[1]
					_set_color_input()
					_rect_filled((x, y - TITLE_HEIGHT - i * INPUT_HEIGHT), (gw, INPUT_HEIGHT))
				# Highlight selected input, if any.
				if self.sel_input[0] == mid and self.sel_input[1] >= 0:
					i = self.sel_input[1]
					_set_color_input()
					_rect_filled((x, y - TITLE_HEIGHT - i * INPUT_HEIGHT), (gw, INPUT_HEIGHT))
				_set_color_fg()
				for i in range(inps):
					inpn = self.purpleinfo.plugin_get_input_name(mpi, i)
					_rect_filled((x, y - gth - i * INPUT_HEIGHT - INPUT_HEIGHT / 2.0 + 0.05 / 2), (0.04, 0.05))
					self.string_render_boxed((x + 0.05, y - gth - i * INPUT_HEIGHT), \
								 (gw - 0.10, INPUT_HEIGHT), inpn, 16)
				if mid in self.selection:
					_set_color_sel()
					_rect((x, y), (gw, h))
				conn.append((mid, xtra))
			x += 1.10 * gw
		# Loop again, over the accumulated connection information, and render them.
		for c in conn:
			self._connections_paint(c[0], c[1])

	def _output_connect_paint(self):
		"Paint output-connection in progress graphics."
		if self.drag_now == None: return
		try:	xtra = self.graphExtra[self.drag_output]
		except:	return
		glLineWidth(2)
#		self._connector_paint(xtra[0][0] + xtra[1][0], xtra[0][1] - TITLE_HEIGHT / 2, self.drag_last.rx - self.home[0], self.drag_last.ry - self.home[1])
		glBegin(GL_LINES)
		glVertex2d(xtra[0][0] + xtra[1][0], xtra[0][1] - TITLE_HEIGHT / 2)
		glVertex2d(self.drag_last.rx - self.home[0], self.drag_last.ry - self.home[1])
		glEnd()
		glLineWidth(1)

	class Point:
		def __init__(self, x, y):
			self.x = x
			self.y = y
			self.rx = 0.0
			self.ry = 0.0

		def copy(self):
			c = GraphArea.Point(self.x, self.y)
			c.rx = self.rx
			c.ry = self.ry
			return c

		def map_gl(self, home, w, h, zoom = 1.0):
			self.rx = float(self.x) / (w / 2) - 1.0
			self.ry = 1.0 - float(self.y) / (h / 2)

			self.rx /= zoom
			self.ry /= zoom

		def mapped_delta(self, other):
			return self.rx - other.rx, self.ry - other.ry

		def __str__(self):
			return str(self.rx)+","+str(self.ry)

	def _module_hit(self, x, y):
		"Check if click at (x,y) in graph-space is inside a module."
		if not self.graph: return
		modules = self.graph.xpathEval("graph/module[@id and @plug-in]")
		for m in modules:
			mid = int(m.xpathEval("@id")[0].content)
			try:
				xtra = self.graphExtra[mid]
			except:
				continue
			if x > xtra[0][0] and x < xtra[0][0] + xtra[1][0] and \
			   y < xtra[0][1] and y > xtra[0][1] - xtra[1][1]:
				return mid
		return -1

	def _module_output_hit(self, x, y, mid):
		"Check if (x,y) is in module mid's output arrow."
		try:	xtra = self.graphExtra[mid]
		except:	return
		return x > xtra[0][0] + xtra[1][0] - ARROW_WIDTH and \
		       x < xtra[0][0] + xtra[1][0] and \
		       y < xtra[0][1] - ARROW_MARGIN and \
		       y > xtra[0][1] - TITLE_HEIGHT + ARROW_MARGIN

	def _module_input_hit(self, x, y, mid):
		"Check if (x,y) is over one of mid's inputs."
		pid = self.graph.xpathEval("graph/module[@id=" + str(mid) + "]/@plug-in")
		if pid != []:
			pid = int(pid[0].content)	# Unwrap, convert to integer.
			num = self.purpleinfo.plugin_get_input_num(pid)
			try:	xtra = self.graphExtra[mid]
			except:	return -1
			rely = xtra[0][1] - y
			if rely > TITLE_HEIGHT:	return int((rely - TITLE_HEIGHT) / INPUT_HEIGHT)
		return -1

	def _evt_button_press(self, *args):
		evt = args[1]
		here = self.Point(evt.x, evt.y)
		here.map_gl(self.home, self.allocation.width, self.allocation.height, self.zoom)
		if evt.state & gtk.gdk.CONTROL_MASK:
			if evt.button == 1:
				self.drag = 1
		else:
			m = int(self._module_hit(here.rx - self.home[0], here.ry - self.home[1]))
			if m >= 0:
				mpi = int(self.graph.xpathEval("graph/module[@id='"+str(m)+"']/@plug-in")[0].content)
				if mpi != 2 and self._module_output_hit(here.rx - self.home[0], here.ry - self.home[1], m):
					self.drag = 3
					self.drag_output = m
				else:
					if evt.state & gtk.gdk.SHIFT_MASK:
						if not m in self.selection:
							self.selection.append(m)
						else:
							self.selection.remove(m)
					else:
						self.selection = [m]
						self.inputs.set_focus(m)
					self.drag = 2
#				inp = self._module_input_hit(here.rx - self.home[0], here.ry - self.home[1], m)
#				self.sel_input = (m, inp)
				self.refresh()
			elif len(self.selection) > 0 and not (evt.state & gtk.gdk.SHIFT_MASK):
				self.selection = []
				self.sel_input = (-1, -1)
				self.refresh()
				self.drag = 0
		self.drag_anchor = here
		self.drag_last = self.drag_anchor

	def _evt_button_release(self, *args):
		if self.drag == 3:	# Connect output-mode?
			if self.drag_input[1] >= 0 and self.purple != None:
				self.purple.mod_input_set_module(self.graph_id, self.drag_input[0], self.drag_input[1], self.drag_output)
		self.drag = 0
		self.drag_input = (-1, -1)
		self.refresh()

	def _evt_scroll(self, *args):
		if args[1].direction == gtk.gdk.SCROLL_UP:
			self.zoom_step(0.10)
		elif args[1].direction == gtk.gdk.SCROLL_DOWN:
			self.zoom_step(-0.10)

	def _evt_motion_notify(self, *args):
		if self.drag == 0: return
		self.drag_now = self.Point(args[1].x, args[1].y)
		self.drag_now.map_gl(self.home, self.allocation.width, self.allocation.height, self.zoom)
		dx, dy = self.drag_now.mapped_delta(self.drag_last)
		if self.drag == 1:	# Pan?
			self.home = (self.home[0] + dx,self.home[1] + dy)
		elif self.drag == 2:	# Move selection?
			for m in self.selection:
				try:
					xtra = self.graphExtra[m]
				except: continue
				self.graphExtra[m] = ((xtra[0][0] + dx, xtra[0][1] + dy), xtra[1])
		elif self.drag == 3:	# Connect output?
			over = self._module_hit(self.drag_now.rx - self.home[0], self.drag_now.ry - self.home[1])
			indx = -1
			if over >= 0:
				indx = self._module_input_hit(self.drag_now.rx - self.home[0], self.drag_now.ry - self.home[1], over)
			self.drag_input = (over, indx)
		self.refresh()
		self.drag_last = self.drag_now.copy()

	def _evt_realize(self, *args):
	        # Obtain a reference to the OpenGL drawable and rendering context.
		gldrawable = self.get_gl_drawable()
		glcontext = self.get_gl_context()	

		# OpenGL begin.
		if not gldrawable.gl_begin(glcontext):
			return

		glFrontFace(GL_CW)
		glCullFace(GL_BACK)
		glEnable(GL_CULL_FACE)
		glDisable(GL_DEPTH_TEST)

		glClearColor(0.328, 0.265, 0.328, 1)

		# OpenGL end.
		gldrawable.gl_end()

	def _evt_configure(self, *args):
		# Obtain a reference to the OpenGL drawable and rendering context.
		gldrawable = self.get_gl_drawable()
		glcontext = self.get_gl_context()

		# OpenGL begin
		if not gldrawable.gl_begin(glcontext):
			return gtk.FALSE

		glViewport(0, 0, self.allocation.width, self.allocation.height)
		glMatrixMode(GL_PROJECTION)
		glLoadIdentity()
		gluOrtho2D(-1.0, 1.0, -1.0, 1.0)
		glMatrixMode(GL_MODELVIEW)
		glLoadIdentity()

		# OpenGL end
		gldrawable.gl_end()
		return gtk.FALSE

	def _evt_expose(self, *args):
		# Obtain a reference to the OpenGL drawable and rendering context.
		gldrawable = self.get_gl_drawable()
		glcontext = self.get_gl_context()
		# OpenGL begin
		if not gldrawable.gl_begin(glcontext):
			return gtk.FALSE

		glClear(GL_COLOR_BUFFER_BIT)
		glLoadIdentity()
		# Use 'actual' forms of zoom and home, for interpolation (see below).
		glScalef(self.zoomActual, self.zoomActual, self.zoomActual)
		glTranslated(self.homeActual[0], self.homeActual[1], 0.0)
		if self.graph != None:
			self._graph_paint()
			if self.drag == 3:
				self._output_connect_paint()

		if gldrawable.is_double_buffered():
			gldrawable.swap_buffers()
		else:
			glFlush()
		# OpenGL end.
		gldrawable.gl_end()

		# Be a little silly, and smoothly interpolate the pan and zoom values. Looks good. :)
		r = 0
		factor = 0.55
		limit  = 1E-3
		if self.home[0] != self.homeActual[0] or self.home[1] != self.homeActual[1]:
			self.homeActual = (self.homeActual[0] + factor * (self.home[0] - self.homeActual[0]), \
					   self.homeActual[1] + factor * (self.home[1] - self.homeActual[1]))
			if abs(self.home[0] - self.homeActual[0]) < limit and abs(self.home[1] - self.homeActual[1]) < limit:
				self.homeActual = self.home
			else: r = 1
		if self.zoom != self.zoomActual:
			self.zoomActual += factor * (self.zoom - self.zoomActual)
			if abs(self.zoom - self.zoomActual) < limit:
				self.zoomActual = self.zoom
			else: r = 1
		if r != 0: self.refresh()	# Keep posting refreshes until done, then cease. Saves CPU.

		return gtk.FALSE

	def module_align(self, corner):
		if len(self.selection) < 2:
			return
		root = self.graphExtra[self.selection[0]]
		for m in self.selection[1:]:
			xtra = self.graphExtra[m]
			if corner == gtk.ARROW_LEFT:	xtra = ((root[0][0], xtra[0][1]), xtra[1])
			elif corner == gtk.ARROW_RIGHT:	xtra = ((root[0][0] + root[1][0] - xtra[1][0], xtra[0][1]), xtra[1])
			elif corner == gtk.ARROW_UP:	xtra = ((xtra[0][0], root[0][1]), xtra[1])
			elif corner == gtk.ARROW_DOWN:	xtra = ((xtra[0][0], root[0][1] - root[1][1] + xtra[1][1]), xtra[1])
			self.graphExtra[m] = xtra
		self.refresh()

	def module_spread(self, orient):
		"""Spread the selected modules, so that they are evenly distributed along either the horizontal or the vertical axis."""
		if len(self.selection) < 3:
			return
		# Decide in which dimension we're working. Also controls sign of movement (s).
		if orient == gtk.ORIENTATION_HORIZONTAL:	d,s = 0,1
		else:						d,s = 1,-1
		# Grab first and last module's extra-data.
		a = self.graphExtra[self.selection[0]]
		b = self.graphExtra[self.selection[-1]]

		# Compute internal in given dimension.
		size = 0.0
		for m in self.selection[1:-1]:
			x = self.graphExtra[m]
			size += x[1][d]
		dist = abs(a[0][d] + s * a[1][d] - b[0][d])
		margin = (dist - size) / (len(self.selection) - 1)
		# Compute first module's position.
		p = a[0][d] + s * (a[1][d] + margin)
		# Iterate over inner modules, poking their positions.
		for m in self.selection[1:-1]:
			x = self.graphExtra[m]
			x0 = list(x[0])			# Trick around since we can't modify tuple.
			x0[d] = p
			x = (tuple(x0), x[1])
			self.graphExtra[m] = x
			p += s * ( x[1][d] + margin)	# Step to next suitable location.
		self.refresh()

	def zoom_step(self, delta):
		self.zoom += delta
		if self.zoom < 0.1:
			self.zoom = 0.1
		elif self.zoom > 10:
			self.zoom = 10
		self.refresh()
	
	def zoom_normal(self):
		"Set the zoom to a 'normal' level. Rather arbitrary."
		self.zoom = 1.0
		self.refresh()

	def zoom_to_fit(self):
		"Set camera to center of selection, and set zoom so that it just fits. Handy."
		b = self.get_graph_bounds()
		# Code lifted from the "Adamant" Verse1 material editor. Thanks, me in 2001.
		self.home = (-(b[0][0] + (b[1][0] - b[0][0]) / 2), -(b[0][1] - (b[0][1] - b[1][1]) / 2))
		full = 1.96	# Should logically be 2.0, but a little margin looks better.
		if b[1][0] - b[0][0] > b[1][1] - b[0][1]:
			self.zoom = full / (b[1][0] - b[0][0])
		else:	self.zoom = full / (b[1][1] - b[0][1])
		self.refresh()

	def get_graph_bounds(self):
		"Return bounds of area. Computes for selection if there is one, else entire graph."
		min_x =  1.0E10
		max_x = -1.0E10
		min_y =  1.0E10
		max_y = -1.0E10
		# Create sequence of module IDs. Can't just use graphExtra; it can contain deleted module's data.
		ids = []
		if len(self.selection) > 0: ids = self.selection
		else:
			modules = self.graph.xpathEval("graph/module[@id and @plug-in]")
			for m in modules: ids.append(int(m.xpathEval("@id")[0].content))
		for id in ids:
			x = self.graphExtra[id]
			if x[0][0] < min_x: min_x = x[0][0]
			if x[0][1] > max_y: max_y = x[0][1]
			if x[0][0] + x[1][0] > max_x: max_x = x[0][0] + x[1][0]
			if x[0][1] - x[1][1] < min_y: min_y = x[0][1] - x[1][1]
#		print "bounds:", min_x, min_y, max_x, max_y
		return ((min_x, min_y), (max_x, max_y))		# Return tuple.
