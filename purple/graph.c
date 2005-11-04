/*
 * graph.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Graph editing module.
*/

/** \page devui Writing Purple Interfaces
 * This page describes Purple from the perspective of a developer who wants to write a Purple user
 * interface client. Such a program is a stand-alone Verse client, i.e. it is not a Purple plug-in
 * or (directly) related to the Purple API at all. It uses the regular low-level Verse API, or
 * whatever higher-level wrapper of it is suitable. A user interface client can be written in any
 * programming language (or mix of languages) for which the Verse API is available.
 * 
 * To write a user interface for Purple, several things are needed from the system:
 * - You need to find out if a given Verse server contains a Purple engine
 * - If it does, you need ways to:
 *   - Learn which plug-ins it has, and information about them
 *   - Learn which graphs exist, and their contents
 *   - Instruct it to instantiate a plug-in
 *   - Instruct it to change contents of a graph
 *
 * The sections below detail how these needs are addressed by the Purple architecture:
 * - \ref findpurple
 * - \ref learning
 *   - \ref xmlplugins
 *   - \ref xmlgraphs
 *   - \ref xmlgraph
 * - \ref changing
 *   - \ref methods
 *   - \ref results
 * 
 * \section findpurple Finding the Purple Engine
 * There is no hard, safe and guaranteed way of localizing a Purple instance after connecting
 * to a Verse server. This is mainly because the Verse database is fairly simplistic, and doesn't
 * provide a reliable way to identify something. A reasonably good criterion to use is:
 * the Purple engine's avatar is an object node, linked to a text node named "PurpleMeta" with
 * a link label of "meta". Further idenfifying traits are given below.
 * 
 * \section learning Learning about Purple's State
 * There is plenty of state inside the Purple engine; this section cares mainly about three areas
 * of state information: which plug-ins are available, which graphs have been created, and the
 * definition of each of the graphs.
 * 
 * These needs are all addressed in similar ways by the Purple engine: by serializing the
 * state into XML that is published. Where this XML is kept then becomes an intersting question,
 * of course. The answer is two-fold:
 *  - XML describing core data structures is kept in buffers in the PurpleMeta text node.
 *    - Plug-ins are listed in the "plugins" buffer.
 *    - Existing graphs are listed in the "graphs" buffer.
 *  - Actual graph contents lives in other text node buffers, specified on creation.
 * 
 * The following figure tries to illustrate these concepts together:
 * \dot
 * digraph G {
 *   rankdir=LR;
 *   subgraph left {
 *     Purple;
 *     Purple -> PurpleMeta [label="\"meta\" link"];
 *     node [shape="box"];
 *     plugins [URL="\ref xmlplugins"];
 *     graphs [URL="\ref xmlgraphs"];
 *     PurpleMeta -> plugins [label="buffer"];
 *     PurpleMeta -> graphs  [label="buffer"];
 *   }
 *   subgraph right {
 *     node [shape="diamond"];
 *     "foo.x" [URL="\ref xmlgraph"];
 *     "bar.y" [URL="\ref xmlgraph"];
 *   }
 *   graphs -> "foo.x" [label="XML ref"];
 *   graphs -> "bar.y" [label="XML ref"];
 * }
 * \enddot
 * Here, ellipses denote Verse nodes, the text buffers contained inside the "PurpleMeta" text
 * node are shown as rectangles, and diamonds are used for buffers in text nodes not owned by Purple.
 * \note Arrows in the above graph are \b not simply links. In fact, three different kind of
 * reference are shown using the same graphics. The labels try to hint what kind of connection
 * exists between the connected symbols.
 * 
 * \subsection xmlplugins The "plugins" XML Buffer
 * This buffer in the PurpleMeta text node contains the engine's index of available plug-ins.
 * The index is a simple flat listing of the plug-ins. Each plug-in is assigned a unique small
 * integer identifier, which is used in all transactions (see below) to refer to the plug-in. An
 * exact formal definition of the document type/schema used for the "plugins" XML is not yet
 * available.
 * 
 * \code
<?xml version="1.0" standalone="yes" encoding="ISO-8859-1"?>

<purple-plugins>
<plug-in id="1" name="node-input">
 <inputs>
  <input type="string">
   <name>name</name>
   <flag name="required" value="true"/>
  </input>
 </inputs>
 <meta>
  <entry category="author">Emil Brink</entry>
  <entry category="desc/purpose">Built-in plug-in, outputs the single node whose name is given</entry>
 </meta>
</plug-in>
<!-- lots more -->
</purple-plugins>
 * \endcode
 * 
 * An interface client is expected to take this document and parse it. The resulting parse tree
 * can then be used to construct interface elements, such as populating a "Create Instance" menu
 * with the names of available plug-ins.
 * \note Actual user interface design concepts are outside the scope of this document. Examples
 * will tend to be very short and simple; an actual interface client should probably do better
 * in most cases.
 * 
 * It should be fairly obvious how the information handed to Purple by a plug-in's \c init()
 * function ends up in the XML shown above. The \c inputs element has one child \c input for
 * call to \c p_init_input(), the \c meta element simply lists all the category/value
 * pairs from calls to \c p_init_meta(), etc.
 * 
 * \subsection xmlgraphs The "graphs" XML Buffer
 * This buffer in the PurpleMeta node contains an index of all existing graphs. It does \b not
 * contain the actual graphs themselves, just references to where they are stored. See
 * \ref xmlgraph "below" for details.
 * 
 * \code
 * <?xml version="1.0" standalone="yes"?>
 *  <purple-graphs>
 *   <graph id="1" name="foo">
 *    <at>
 *     <node>Text_Node_2</node>
 *     <buffer>0</buffer>
 *    </at>
 *  </graph>
 * </purple-graphs>
 * \endcode
 * 
 * The above snippet tells you that there exists a graph named "foo", with engine-assigned
 * global identifier 1. This graph has its contents stored in the text node named "Text_Node_2",
 * buffer 0.
 * 
 * \subsection xmlgraph Graph Contents
 * The contents of a graph, i.e. the set of plug-ins which are instantiated in it and their
 * connections and input values, are represented as XML, too. However, this XML is not in a
 * buffer in the PurpleMeta node. Instead, an external user that wishes to create a new graph
 * tells Purple the name of a text node to store the data in, and also which buffer in the
 * node to use.
 * 
 * Purple then assumes control over that buffer, clears it, and starts maintaining an XML
 * serialization of the new graph there. As the graph is further edited, the XML changes to
 * stay up to date. Here's a sample of a graph XML description:
 * \code
 * <graph>
 *  <module id="0" plug-in="6">
 *   <set input="0" type="real32">1.41</set>
 *  </module>
 *  <module id="1" plug-in="3">
 *   <set input="0" type="module">0</set>
 *  </module>
 *  <module id="2" plug-in="21">
 *   <set input="0" type="module">0</set>
 *   <set input="1" type="module">1</set>
 *  </module>
 *  <module id="3" plug-in="2">
 *   <set input="0" type="module">2</set>
 *  </module>
 * </graph>
 * \endcode
 * A couple of points to notice:
 * - The graph is free of any identification information; you must parse the index first.
 * - Each module has a local ID inside the graph, and an indication of which plug-in it instantiates.
 * - Inputs are set using the \c set element, whose \c type atttribute indicates the external type.
 * - The \c input attribute of the \c set element maps to the first argument of \c p_init_input().
 * - It is not possible to determine if an input is set externally or by Purple itself, i.e. it
 *   is set to its default value. Both will appear as just a \c set element.
 *
 * The above information can be used to construct a graphical view of the graph (module IDs are 
 * not shown in this graph, instead the plug-in IDs they instantiate are used):
 * \dot
 * digraph G {
 *  rankdir=LR;
 *  none  [style="invis"];
 *  0     [label="6"];
 *  1     [label="3"];
 *  2     [label="21"];
 *  3     [label="2"];
 *
 *  none->0 [label="1.41"];
 *  2->3;
 *  0->2;
 *  1->2;
 *  0->1;
 * }
 * \enddot
 * 
 * This graph is still not very communicative, you really need the plug-in index to be able to look up
 * e.g. names of inputs and plug-ins. One hint is that plug-in ID \c 2, as used for the rightmost module,
 * is guaranteed to refer to the built-in \c node-output plug-in. So, the above looks a lot like a
 * processing chain that uses the parameter \c 1.41 to generate some node data in the leftmost module,
 * sends it through a couple of other plug-ins, and then sends the result out to the Verse server.
 * 
 * \section changing Changing Purple's State
 * Above, we've seen that Purple's state is made public through the publishing of structured text
 * in XML format, that is updated as the state changes. The question now is: how do you convince
 * Purple to change its state?
 * 
 * The most obvious answer, perhaps, could be "by editing the XML yourself". This, however, is not
 * the way that has been chosen. Mainly because editing XML is rather ... vague, as input mechanisms
 * go. It is hard to know when an editing operation begins, and (as usual with Verse) impossible to
 * know when it ends. Since input-changing needs to be an event in Purple, that can trigger plug-in
 * computation, it would be nice with something a little more well-defined. Luckily, Verse has just
 * such a mechanism built-in: object node methods.
 * 
 * Verse object nodes support defining groups of \e methods, which are simply named entry points
 * that can be called, and that accept a number of \e parameters when called. Calling a method does
 * not cause the Verse host to do anything special, the call is simply distributed to all clients
 * that subscribe to the node. In the case of Purple, the Purple engine subscribes to its own
 * avatar, and will act on method calls.
 * \see The Verse specification on the object node, <http://www.blender.org/modules/verse/verse-spec/n-object.html#o-methods>.
 * 
 * \subsection methods Purple Graph Editing Methods
 * The graph editing methods exported by the Purple engine all reside in a single group, called
 * "PurpleGraph". The methods could be further divided into three subgroups, although the Verse
 * object node does not support doing so. They are:
 * - Graph create/destroy
 *   - \c create(VNodeID node_id, VLayerID buffer_id, string name) -- Create a new graph in the indicated text node and buffer.
 *   - \c destroy(uint32 graph_id) -- Destroy a graph.
 * - Module create/destroy
 *   - \c mod_create(uint32 graph_id, uint32 plugin_id) -- Create an instance of the given plug-in in a graph.
 *   - \c mod_destroy(uint32 graph_id, uint32 module_id) -- Destroy a plug-in instance.
 * - Module input setting
 *   - \c mod_input_clear(uint32 graph_id, uint32 module_id, uint8 input_id) -- Clear the indicated input, removing any previous assignment or connection.
 *   - \c mod_set_boolean(uint32 graph_id, uint32 module_id, uint8 input_id, uint8 value) -- Set an input to a boolean value. 0 is \c false, all other values are considered \c true.
 *   - \c mod_set_int32(uint32 graph_id, uint32 module_id, uint8 input_id, int32 value) -- Set an input to a signed 32-bit integer value.
 *   - \c mod_set_uint32(uint32 graph_id, uint32 module_id, uint8 input_id, uint32 value) -- Set an input to an unsigned 32-bit integer value.
 *   - \c mod_set_real32(uint32 graph_id, uint32 module_id, uint8 input_id, real32 value) -- Set an input to a 32-bit floating point value.
 *   - \c mod_set_r32v2(uint32 graph_id, uint32 module_id, uint8 input_id, real32_vec2 value) -- Set an input to a 2D vector of 32-bit floating point values.
 *   - \c mod_set_r32v3(uint32 graph_id, uint32 module_id, uint8 input_id, real32_vec3 value) -- Set an input to a 3D vector of 32-bit floating point values.
 *   - \c mod_set_r32v4(uint32 graph_id, uint32 module_id, uint8 input_id, real32_vec4 value) -- Set an input to a 4D vector of 32-bit floating point values.
 *   - \c mod_set_r32m16(uint32 graph_id, uint32 module_id, uint8 input_id, real32_mat16 value) -- Set an input to a 4x4 matrix of 32-bit floating point values.
 *   - \c mod_set_real64(uint32 graph_id, uint32 module_id, uint8 input_id, real64 value) -- Set an input to a 64-bit floating point value.
 *   - \c mod_set_r64v2(uint32 graph_id, uint32 module_id, uint8 input_id, real64_vec2 value) -- Set an input to a 2D vector of 64-bit floating point values.
 *   - \c mod_set_r64v3(uint32 graph_id, uint32 module_id, uint8 input_id, real64_vec3 value) -- Set an input to a 3D vector of 64-bit floating point values.
 *   - \c mod_set_r64v4(uint32 graph_id, uint32 module_id, uint8 input_id, real64_vec4 value) -- Set an input to a 4D vector of 64-bit floating point values.
 *   - \c mod_set_r64m16(uint32 graph_id, uint32 module_id, uint8 input_id, real64_mat16 value) -- Set an input to a 4x4 matrix of 64-bit floating point values.
 *   - \c mod_set_string(uint32 graph_id, uint32 module_id, uint8 input_id, string value) -- Set an input to a text string.
 *   - \c mod_set_module(uint32 graph_id, uint32 module_id, uint8 input_id, uint32 value) -- Set an input to reference another module's output. Creates graph edges.
 * 
 * The methods are defined using the types from the Verse specification; the above are not C prototypes. The type names have been slightly
 * simplified to keep the method list readable (lower-cased, and the prefix \c VN_O_METHOD_PTYPE_ has been removed). How to map the methods into whatever
 * langauge is being used to write the interface client is outside the scope of this document; please refer to the language binding in use.
 * \see The reference C API's \c verse_method_call_pack() function; here: <http://www.blender.org/modules/verse/verse-spec/verse_method_call_pack.html>.
 * 
 * Calling these methods causes the Purple engine to respond in various ways. See below for details.
 * 
 * \subsection results Results of Calling Graph Methods
 * As shown above, the graph method calls can be split into three groups. Here is what happens inside Purple, and to the externally-visible
 * XML published by it, as method calls are processed:
 * 
 * -# The first pair of methods, \c create() and \c destroy(), are used to create and destroy whole graphs. Creating a new graph involves telling Purple
 *    a location for the graph's XML description, in the form of a text node ID and a buffer ID. You must also supply Purple with the desired name of
 *    the graph. Graph names are used to identify them for human users, and should be unique. Think of them as file or project names. What happens when
 *    you send a \c create() call? The following:
 *     -# Purple creates the internal data structures needed to represent the graph.
 *     -# Purple appends a \c graph element to the \c graphs XML buffer in PurpleMeta.
 *     -# Purple clears the user-supplied text buffer, and fills in a barebones \c graph element there.
 *     .
 *    So, to learn which numerical identifier got assigned to the newly created graph, a client must parse the changes in the XML and extract
 *    the information.
 *    .
 * -# The second pair of methods, \c mod_create() and \c mod_destroy(), are used to create and destroy modules inside a graph. They require the numerical
 *    ID of the graph in which to operate as their first parameter. The \c mod_create() method also needs the numerical ID of the plug-in to be instantiated,
 *    this comes from the "plugins" XML buffer described above. When received by the Purple engine, the following happens:
 *     -# Purple looks up the graph in its internal data base.
 *     -# Purple looks up the the plug-in, too.
 *     -# An instance is created, and added to the graph.
 *     -# The change is made public by updating the user-supplied graph text buffer, where an empty \c module element is added.
 *     .
 * -# The third group of methods, consisting of \c mod_input_clear() and fifteen \c mod_set_XXX() varieties, all require numerical IDs for the graph
 * and module that is to be affected, and the ID (or index) of the input. These latter IDs correspond directly to the first argument used in the
 * \c p_init_input() call that registers the inputs with Purple. The following occurs when Purple receives an input-changing method:
 *     -# The graph and module are looked up.
 *     -# If the input index is valid, the internal representation is modified according to the method call.
 *     -# The module is scheduled for re-computation, since an input changed.
 *     -# The change is made public by updating the user-supplied graph text buffer, editing (or removing) a \c set element.
 * 
 * Basically, "all" a Purple user interface client has to do is parse all the related XML, figure out ways to present
 * the information to a user, and allow interaction with it. Interaction needs to result in methods being called, which
 * in turn will cause Purple to update the XML, and the circle is complete.
 * 
 * Purple plug-ins can provide a lot of information about their inputs, by using the variable-arguments list in the \c p_init_input()
 * call. This information, which can include things like a default value, minimum and maximum values, is intended to make it possible
 * to build "richer" user interfaces. For instance, a \c uint32 input with a "small" allowed range might be represented by a slider,
 * while a larger range calls for an entry box, and so on.
 * 
*/

/** \fn void create(void) */

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "verse.h"

#include "purple.h"

#include "dynarr.h"
#include "dynstr.h"
#include "hash.h"
#include "idlist.h"
#include "idset.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "memchunk.h"
#include "value.h"
#include "nodeset.h"
#include "plugins.h"
#include "scheduler.h"
#include "strutil.h"
#include "textbuf.h"
#include "port.h"
#include "xmlutil.h"

#include "nodedb.h"

#include "client.h"

#include "graph.h"

/* ----------------------------------------------------------------------------------------- */

/* Instances of this are used to temporarily hold resume information, extracted from XML
 * and then stored here until needed (which is when synchronizer wants to write to the
 * node). Not exactly clinically clean, I guess.
*/
typedef struct
{
	uint32	label;
	char	name[16];
} OResume;

typedef struct
{
	DynArr	*node;
	uint32	next;		/* Next expected label. These must be "tight", enforced by create(). */
} ONodes;

typedef struct
{
	PPort	port;		/* Result is stored here. */
	IdList	dependants;	/* Tracks dependants, to notify when output changes. */
	boolean	changed;	/* Output changed recently? Don't notify all the time... */
	ONodes	nodes;		/* Output nodes. Caches between invocations, using 'label' on create(). */
	List	*resume;	/* Information about output nodes, from resume parse of old XML. */
} Output;

struct Graph
{
	char	name[32];

	uint32	index_start, index_length;

	VNodeID	node;
	uint16	buffer;
	uint32	desc_start;		/* Base location in graph XML buffer, first module starts here. */
	IdSet	*modules;
};

/* A module is an instance of a plug-in, i.e. a node in a Graph. Can't call it "node", collides w/ Verse. */
typedef struct
{
	uint32		id;
	Graph		*graph;		/* Which graph does this module belong to? */
	Plugin		*plugin;
	PInstance	instance;	/* Input values, state data. */
	Output		out;		/* Things having to do with output/result, see above. */

	uint32		start, length;	/* Region in graph XML buffer used for this module. */
} Module;

/* Bookkeeping structure used to keep track of the various methods used to control the Purple engine. */
typedef struct
{
	const char	   *name;
	size_t		   param_count;
	const VNOParamType param_type[4];	/* Wastes a bit, but simplifies code. */
	const char	   *param_name[4];
	uint8		   id;			/* Filled-in once created. */ 
} MethodInfo;

/* Enumeration to allow simple IDs for methods. */
enum
{
	CREATE, DESTROY, MOD_CREATE, MOD_DESTROY, MOD_INPUT_CLEAR,
	MOD_INPUT_SET_BOOLEAN, MOD_INPUT_SET_INT32, MOD_INPUT_SET_UINT32,
	MOD_INPUT_SET_REAL32, MOD_INPUT_SET_REAL32_VEC2, MOD_INPUT_SET_REAL32_VEC3, MOD_INPUT_SET_REAL32_VEC4, MOD_INPUT_SET_REAL32_MAT16,
	MOD_INPUT_SET_REAL64, MOD_INPUT_SET_REAL64_VEC2, MOD_INPUT_SET_REAL64_VEC3, MOD_INPUT_SET_REAL64_VEC4, MOD_INPUT_SET_REAL64_MAT16,
	MOD_INPUT_SET_STRING,
	MOD_INPUT_SET_MODULE,
};

/* Create name of input-setting method. */
#define	MI_INPUT_NAME(lct)	"mod_set_" #lct

/* Create initializer for ordinary scalar input setting method. */
#define	MI_INPUT(lct, uct)	\
	{ MI_INPUT_NAME(lct), 4, { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT8, \
		VN_O_METHOD_PTYPE_ ##uct }, { "graph_id", "module_id", "input", "value" } }

/* Create initializer for vector input setting method. Use shorter type name. */
#define MI_INPUT_VEC(lct, uct, len)	\
	{ MI_INPUT_NAME(lct) "v" #len, 4, { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT8, \
	VN_O_METHOD_PTYPE_ ## uct ##_VEC ## len }, { "graph_id", "module_id", "input", "value" } }

/* Initialize the method bookkeeping information. This data is duplicated and sorted to create a by-name version; this
 * literal one is for lookup by the enumeration above so the order of initializers *must* match.
*/
static MethodInfo method_info[] = {
	{ "create",  3, { VN_O_METHOD_PTYPE_NODE, VN_O_METHOD_PTYPE_LAYER, VN_O_METHOD_PTYPE_STRING },
			{ "node_id", "buffer_id", "name" } },
	{ "destroy",	1, { VN_O_METHOD_PTYPE_UINT32 }, { "graph_id" } },
	
	{ "mod_create",  2, { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32 }, { "graph_id", "plugin_id" } },
	{ "mod_destroy", 2, { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32 }, { "graph_id", "module_id" } },
	{ "mod_input_clear", 3, { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT8 },
			{ "graph_id", "module_id", "input" } },
	MI_INPUT(boolean, UINT8),
	MI_INPUT(int32, INT32),
	MI_INPUT(uint32, UINT32),
	MI_INPUT(real32, REAL32),
	MI_INPUT_VEC(r32, REAL32, 2),
	MI_INPUT_VEC(r32, REAL32, 3),
	MI_INPUT_VEC(r32, REAL32, 4),
	MI_INPUT(r32m16, REAL32_MAT16),
	MI_INPUT(real64, REAL64),
	MI_INPUT_VEC(r64, REAL64, 2),
	MI_INPUT_VEC(r64, REAL64, 3),
	MI_INPUT_VEC(r64, REAL64, 4),
	MI_INPUT(r64m16, REAL64_MAT16),
	MI_INPUT(string, STRING),
	MI_INPUT(module, UINT32)
};

static struct
{
	size_t		to_register;
	IdSet		*graphs;
	Hash		*graphs_name;	/* Graphs hashed on name. */

	MemChunk	*chunk_module;
} graph_info = { sizeof method_info / sizeof *method_info };

/* ----------------------------------------------------------------------------------------- */

static void	module_create(unsigned int module_id, uint32 graph_id, uint32 plugin_id);
static void	module_input_set_from_string(Graph *g, uint32 module_id, uint8 input_index, PValueType type, ...);
static void	module_input_clear_links_to(Graph *g, uint32 module_id, uint32 rm);
static void	module_describe(Module *m);

/* ----------------------------------------------------------------------------------------- */

void graph_init(void)
{
	unsigned int	i;

	graph_info.graphs = idset_new(1);
	graph_info.graphs_name = hash_new_string();
	graph_info.chunk_module = memchunk_new("graph/module", sizeof (Module), 8);

	for(i = 0; i < sizeof method_info / sizeof *method_info; i++)
		method_info[i].id = (uint8) ~0u;
}

void graph_method_send_creates(uint32 avatar, uint8 group_id)
{
	unsigned int	i;

	for(i = 0; i < sizeof method_info / sizeof *method_info; i++)
	{
		verse_send_o_method_create(avatar, group_id, (uint8) ~0u, method_info[i].name,
					   method_info[i].param_count,
					   (VNOParamType *) method_info[i].param_type,
					   (const char **) method_info[i].param_name);
	}
}

/* ----------------------------------------------------------------------------------------- */

void graph_method_check_created(NodeObject *obj)
{
	NdbOMethodGroup	*g;
	unsigned int	i;

	if(graph_info.to_register == 0)
		return;
	if((g = nodedb_o_method_group_lookup(obj, "PurpleGraph")) == NULL)
		return;

	for(i = 0; i < sizeof method_info / sizeof *method_info; i++)
	{
		const NdbOMethod	*m;

		if(method_info[i].id == (uint8) ~0u && (m = nodedb_o_method_lookup(g, method_info[i].name)) != NULL)
		{
			method_info[i].id = m->id;
			graph_info.to_register--;
			if(graph_info.to_register == 0)
				LOG_MSG(("Caught all %u methods, ready to send calls", sizeof method_info / sizeof *method_info));
			return;
		}
	}
	LOG_WARN(("Received unknown (non graph-related?) method creation"));
}

/* ----------------------------------------------------------------------------------------- */

/* Build the XML description of a graph, for the index. */
static void graph_index_build(uint32 id, const Graph *g, char *buf, size_t bufsize)
{
	const PNode	*node;

	if((node = nodedb_lookup(g->node)) == NULL)
	{
		snprintf(buf, bufsize, " <graph id=\"%u\" name=\"%s\"/>\n", id, g->name);
		return;
	}
	snprintf(buf, bufsize,	" <graph id=\"%u\" name=\"%s\">\n"
				"  <at>\n"
				"   <node>%s</node>\n"
				"   <buffer>%u</buffer>\n"
				"  </at>\n"
				" </graph>\n", id, g->name, node->name, g->buffer);
}

/* Go through all graphs, and recompute XML starting points. Handy when a graph has been
 * deleted, or when the length of one or more graph's XML representation has changed.
*/
static void graph_index_renumber(void)
{
	unsigned int	id, pos;
	Graph		*g;

	pos = client_info.graphs.start;
	for(id = idset_foreach_first(graph_info.graphs); (g = idset_lookup(graph_info.graphs, id)) != NULL; id = idset_foreach_next(graph_info.graphs, id))
	{
		g->index_start = pos;
		pos += g->index_length;
	}
}

/* Create a new graph. */
static Graph * graph_create(unsigned int id, VNodeID node_id, uint16 buffer_id, const char *name)
{
	unsigned int	i, pos;
	Graph		*g, *me;
	char		xml[256];

	/* Make sure name is unique. */
	if(hash_lookup(graph_info.graphs_name, name) != NULL)
		return NULL;	/* It wasn't. */
	/* More expensive, but good for sanity: make sure the (node,buffer) combo is unique,
	 * since a single buffer can only hold one graph representation (we clear it, below).
	*/
	for(i = idset_foreach_first(graph_info.graphs); (g = idset_lookup(graph_info.graphs, i)) != NULL; i = idset_foreach_next(graph_info.graphs, i))
	{
		if(g->node == node_id && g->buffer == buffer_id)
		{
			LOG_WARN(("Can't create graph \"%s\" in %u.%u, already used by graph \"%s\" -- aborting", name, node_id, buffer_id, g->name));
			return NULL;
		}
	}

	me = g = mem_alloc(sizeof *g);
	stu_strncpy(g->name, sizeof g->name, name);
	g->node   = node_id;
	g->buffer = buffer_id;
	if(id == ~0u)
		id = idset_insert(graph_info.graphs, g);
	else
	{
		if(idset_insert_with_id(graph_info.graphs, id, g) != id)
		{
			LOG_ERR(("Couldn't insert graph '%s' with ID %u", name, id));
			mem_free(g);
			return NULL;
		}
	}
	g->desc_start = 0;
	g->modules = NULL;		/* Be a bit lazy. */

	for(i = 0, pos = client_info.graphs.start; i < id; i++)
	{
		g = idset_lookup(graph_info.graphs, i);
		if(g == NULL)
			continue;
		pos += g->index_length;
	}
	me->index_start = pos;
	graph_index_build(id, me, xml, sizeof xml);
	me->index_length = strlen(xml);
	verse_send_t_text_set(client_info.meta, client_info.graphs.buffer, me->index_start, 0, xml);
	hash_insert(graph_info.graphs_name, me->name, me);
	snprintf(xml, sizeof xml, "<graph>\n</graph>\n");
	verse_send_t_text_set(me->node, me->buffer, 0, ~0u, xml);
	me->desc_start = strchr(xml, '/') - xml - 1;

	return me;
}

/* Create a Graph, based on data found in XmlNode at <gdesc>, and stuff it refers to. */
Graph * graph_create_resume(const XmlNode *gdesc, const unsigned int *pmap)
{
	const char	*ids, *name, *nn, *gtext;
	unsigned long	gid, bufid;
	char		*eptr;
	NodeText	*node;
	NdbTBuffer	*buf;
	XmlNode		*graph;
	Graph		*g;
	List		*mods;
	const List	*iter;

	ids  = xmlnode_eval_single(gdesc, "@id");
	name = xmlnode_eval_single(gdesc, "@name");

	gid = strtoul(ids, &eptr, 10);
	if(eptr == ids)
	{
		LOG_ERR(("Couldn't parse numeric graph ID from '%s'", ids));
		return NULL;
	}
	nn = xmlnode_eval_single(gdesc, "at/node");
	if((node = (NodeText *) nodedb_lookup_by_name_with_type(nn, V_NT_TEXT)) == NULL)
	{
		LOG_ERR(("Couldn't find node '%s' for graph '%s'", nn, name));
		return NULL;
	}
	nn = xmlnode_eval_single(gdesc, "at/buffer");
	bufid = strtoul(nn, &eptr, 10);
	if(eptr == nn)
	{
		LOG_ERR(("Couldn't parse numeric buffer ID from '%s', graph '%s'", nn, name));
		return NULL;
	}
	buf = nodedb_t_buffer_nth(node, bufid);
	if(buf == NULL)
	{
		LOG_ERR(("Couldn't find buffer %u in node '%s for graph '%s'", bufid, nn, name));
		return NULL;
	}
	gtext = nodedb_t_buffer_read_begin(buf);
	graph = xmlnode_new(gtext);
	nodedb_t_buffer_read_end(buf);
	if(graph == NULL)
	{
		LOG_ERR(("Couldn't parse graph description in node '%s' %u as XML for graph '%s'", nn, bufid, name));
		return NULL;
	}
	/* At this point, it doesn't matter if the server-side graph XML is wiped; we have
	 * already parsed it after all, so we can re-create it (and we will, painstakingly).
	*/
	printf("We can now create the graph '%s', in node %lu, buffer %lu\n", name, (unsigned long) node->node.id, bufid);
	g = graph_create(gid, node->node.id, bufid, name);
	mods = xmlnode_nodeset_get(graph, XMLNODE_AXIS_CHILD, XMLNODE_NAME("module"), XMLNODE_DONE);
	/* First create all required modules. */
	for(iter = mods; iter != NULL; iter = list_next(iter))
	{
		const char	*tmp;
		const XmlNode	*here = list_data(iter);
		unsigned long	id, pi;

		tmp = xmlnode_eval_single(here, "@id");
		id = strtoul(tmp, &eptr, 10);
		if(eptr == tmp)
		{
			LOG_ERR(("Couldn't parse numerical module ID from '%s'", tmp));
			continue;
		}
		tmp = xmlnode_eval_single(here, "@plug-in");
		pi = strtoul(tmp, &eptr, 10);
		if(eptr == tmp)
		{
			LOG_ERR(("Couldn't parse numerical plug-in ID from '%s'", tmp));
			continue;
		}
		module_create(id, gid, pmap[pi]);
	}
	/* Now that the modules exist, go through again and set inputs. This is best done
	 * in a separate stage, since it ensures that any inputs that refer to modules with
	 * a higher index than the source module actually work; all modules are there now.
	*/
	for(iter = mods; iter != NULL; iter = list_next(iter))
	{
		const XmlNode	*here = list_data(iter);
		const char	*tmp;
		char		*eptr;
		List		*sets, *outs;
		const List	*siter;
		unsigned long	id, index;
		Module		*m;

		tmp = xmlnode_eval_single(here, "@id");
		id = strtoul(tmp, &eptr, 10);
		if(eptr == tmp)
		{
			LOG_ERR(("Couldn't parse numerical module ID from '%s'", tmp));
			continue;
		}
		sets = xmlnode_nodeset_get(here, XMLNODE_AXIS_CHILD, XMLNODE_NAME("set"), XMLNODE_DONE);
		for(siter = sets, index = 0; siter != NULL; siter = list_next(siter), index++)
		{
			const XmlNode	*set = list_data(siter);
			const char	*tname, *value;
			PValueType	type;

			tname = xmlnode_eval_single(set, "@type");
			type  = value_type_from_name(tname);
			value = xmlnode_eval_single(set, "");
/*			printf("  set input %lu.%s to %s, type %d\n", id, xmlnode_eval_single(set, "@input"), xmlnode_eval_single(set, ""), type);*/
			module_input_set_from_string(g, id, index, type, value);
		}
		list_destroy(sets);

		if((m = idset_lookup(g->modules, id)) == NULL)
		{
			LOG_WARN(("Couldn't look up module %u", id));
			continue;
		}
		outs = xmlnode_nodeset_get(here, XMLNODE_AXIS_CHILD, XMLNODE_NAME("out"), XMLNODE_DONE);
		for(siter = outs; siter != NULL; siter = list_next(siter))
		{
			const XmlNode	*out = list_data(siter);
			uint32		label;

			printf("parsing out\n");
			if((tmp = xmlnode_eval_single(out, "@label")) != NULL)
			{
				label = strtoul(tmp, &eptr, 10);
				if(eptr == tmp)
				{
					LOG_ERR(("Couldn't parse numerical label from '%s'", tmp));
					continue;
				}
				printf(" label %u\n", label);
				if((tmp = xmlnode_eval_single(out, "@name")) != NULL)
				{
					OResume	*r;

					r = mem_alloc(sizeof *r);
					r->label = label;
					stu_strncpy(r->name, sizeof r->name, tmp);
					
					m->out.resume = list_prepend(m->out.resume, r);
					printf("  stored, name '%s'\n", r->name);
				}
			}
			else
				LOG_WARN(("Couldn't find label attribute in out element"));
		}
		list_destroy(outs);
	}
	list_destroy(mods);
	xmlnode_destroy(graph);

	return g;
}

/* Destroy a graph. */
static void graph_destroy(uint32 id)
{
	Graph	*g;

	if((g = idset_lookup(graph_info.graphs, id)) == NULL)
	{
		LOG_WARN(("Couldn't destroy graph %u, not found", id));
		return;
	}
	g->name[0] = '\0';
	hash_remove(graph_info.graphs_name, g->name);
	idset_remove(graph_info.graphs, id);
	mem_free(g);
	verse_send_t_text_set(client_info.meta, client_info.graphs.buffer, g->index_start, g->index_length, NULL);
	graph_index_renumber();
}

/* ----------------------------------------------------------------------------------------- */

static void module_dep_add(const Graph *g, uint32 module_id, uint32 dep_new)
{
	Module	*m;

	if((m = idset_lookup(g->modules, module_id)) == NULL)
		return;
	idlist_insert(&m->out.dependants, dep_new);
	printf("dep %u added\n", dep_new);
}

static void module_dep_remove(const Graph *g, uint32 module_id, uint32 dep_old)
{
	Module	*m;

	if((m = idset_lookup(g->modules, module_id)) == NULL)
		return;
	idlist_remove(&m->out.dependants, dep_old);
	printf("dep %u removed\n", dep_old);
}

/* Module <m> is about to be destroyed. Notify all dependants, in both directions. Not too expensive. */
static void module_dep_destroy_warning(Module *m)
{
	IdListIter	iter;
	size_t		num, i;
	uint32		lt;

	/* First, remove input links to this module from others, the dependants. */
	for(idlist_foreach_init(&m->out.dependants, &iter); idlist_foreach_step(&m->out.dependants, &iter); )
		module_input_clear_links_to(m->graph, iter.id, m->id);
	/* Second, tell sources this module no longer depends on them. */
	num = plugin_portset_size(m->instance.inputs);
	for(i = 0; i < num; i++)
	{
		if(plugin_portset_get_module(m->instance.inputs, i, &lt))
			module_dep_remove(m->graph, lt, m->id);
	}
}

/* Mildly hackish perhaps, but simplifies interfaces. We know the port is part of a Module, so use that fact. */
#define	MODULE_FROM_PORT(p)	(Module *) ((char *) (p) - offsetof(Module, out.port))

void graph_port_output_begin(PPOutput port)
{
	Module	*m = MODULE_FROM_PORT(port);

	port_clear(port);
	m->out.changed = FALSE;
}

void graph_port_output_set(PPOutput port, PValueType type, ...)
{
	Module		*m = MODULE_FROM_PORT(port);
	va_list		arg;

	va_start(arg, type);
	port_set_va(port, type, arg);
	m->out.changed = TRUE;
	va_end(arg);
}

void graph_port_output_set_node(PPOutput port, PONode *node)
{
	Module		*m = MODULE_FROM_PORT(port);

	port_set_node(port, node);
	m->out.changed = TRUE;
}

/* A <node> was just created by <m>, check if there is resume-information for it. */
static int node_create_check_resume(Module *m, PNode *node, VNodeType type, uint32 label)
{
	List	*iter;

	for(iter = m->out.resume; iter != NULL; iter = list_next(iter))
	{
		OResume	*res = list_data(iter);

		if(res->label == label)
		{
			PNode	*n;

			if((n = nodedb_lookup_by_name(res->name)) != NULL)
			{
				if(n->type == type)
				{
					node->creator.remote = n;
					node->id = n->id;
					m->out.resume = list_unlink(m->out.resume, iter);
					mem_free(res);
					list_destroy(iter);
					LOG_MSG(("Node resumed to %u", node->id));
					graph_port_output_create_notify(node);
					return 1;
				}
			}
		}
	}
	return 0;
}

PONode * graph_port_output_node_create(PPOutput port, VNodeType type, uint32 label)
{
	Module	*m = MODULE_FROM_PORT(port);
	PNode	**node = NULL;

	if(label < m->out.nodes.next)	/* Lookup of existing? */
	{
		if(m->out.nodes.node == NULL)
			return NULL;
		if((node = dynarr_index(m->out.nodes.node, label)) != NULL)
		{
			printf("Returning previously created node with label %u\n", label);
			graph_port_output_set_node(port, *node);
			return *node;
		}
		return NULL;
	}
	else if(label == m->out.nodes.next)
	{
		printf("This would be a good time to create a new node and label it %u\n", label);
		if(m->out.nodes.node == NULL)
			m->out.nodes.node = dynarr_new(sizeof *node, 1);
		if(m->out.nodes.node != NULL)
		{
			if((node = dynarr_set(m->out.nodes.node, m->out.nodes.next, NULL)) != NULL)
			{
				if((*node = nodedb_new(type)) != NULL)
				{
					(*node)->creator.port  = port;
					if(type == V_NT_GEOMETRY)
					{
						nodedb_g_layer_create((NodeGeometry *) *node, (VLayerID) 0u, "vertex", VN_G_LAYER_VERTEX_XYZ, 0u, 0.0);
						nodedb_g_layer_create((NodeGeometry *) *node, (VLayerID) 1u, "polygon", VN_G_LAYER_POLYGON_CORNER_UINT32, 0u, 0.0);
					}
					m->out.nodes.next++;
					nodedb_ref(*node);
					node_create_check_resume(m, *node, type, label);
				}
			}
		}
		if(node != NULL)
			graph_port_output_set_node(port, *node);
		return node != NULL ? *node : NULL;
	}
	else
		LOG_WARN(("Mismatched node label %u, expected %u", label, m->out.nodes.next));
	return NULL;
}

PONode * graph_port_output_node_copy(PPOutput port, PINode *node, uint32 label)
{
	Module	*m = MODULE_FROM_PORT(port);
	PNode	**store = NULL;

	if(label < m->out.nodes.next)	/* Lookup of existing? */
	{
		if(m->out.nodes.node == NULL)
			return NULL;
		if((store = dynarr_index(m->out.nodes.node, label)) != NULL)
		{
			printf("Returning previously copied node with label %u\n", label);
			printf(" re-setting contents, though\n");
			nodedb_set(*store, node);
			graph_port_output_set_node(port, *store);
			return *store;
		}
		return NULL;
	}
	else if(label == m->out.nodes.next)
	{
		printf("This would be a good time to create a new node as copy of %p, and label it %u\n", node, label);
		if(m->out.nodes.node == NULL)
			m->out.nodes.node = dynarr_new(sizeof *node, 1);
		if(m->out.nodes.node != NULL)
		{
			if((store = dynarr_set(m->out.nodes.node, m->out.nodes.next, NULL)) != NULL)
			{
				if((*store = nodedb_new_copy(node)) != NULL)
				{
					(*store)->creator.port = port;
					m->out.nodes.next++;
					nodedb_ref(*store);
				}
			}
		}
		if(store != NULL)
			graph_port_output_set_node(port, *store);
		return store != NULL ? *store : NULL;
	}
	return NULL;
}

void graph_port_output_end(PPOutput port)
{
	Module		*m = MODULE_FROM_PORT(port), *dep;
	IdListIter	iter;

	if(m->out.changed)
	{
		/* Our output changed, so ask scheduler to compute() any dependant modules. */
		for(idlist_foreach_init(&m->out.dependants, &iter); idlist_foreach_step(&m->out.dependants, &iter); )
		{
			if((dep = idset_lookup(m->graph->modules, iter.id)) != NULL)
				sched_add(&dep->instance);
			else
				printf("Couldn't find module %u in graph %s, a dependant of module %u\n", iter.id, m->graph->name, m->id);
		}
	}
}

static void cb_node_output_notify(PNode *node, NodeNotifyEvent e, void *user)
{
	Module	*m = MODULE_FROM_PORT(((PNode *) user)->creator.port);

/*	printf("Got name of watched node owned by %s instance: '%s' (event=%d)\n", plugin_name(m->plugin), node->name, e);*/
	if(e != NODEDB_NOTIFY_NAME)
		return;
	module_describe(m);
}

void graph_port_output_create_notify(const PNode *local)
{
	nodedb_notify_node_add(local->creator.remote, cb_node_output_notify, (void *) local);
}

/* ----------------------------------------------------------------------------------------- */

/* Information used while traversing modules checking for cycles, helps keep argument count down. */
struct traverse_info
{
	uint32	module_id;
	uint8	input;
	uint32	source;
};

/* Traverse module links from <module_id>, looking for <source_id>. Returns FALSE if cyclic link found. */
static boolean traverse_module(const Graph *g, uint32 module_id, uint32 source_id, const struct traverse_info *ti)
{
	const Module	*m;

	if((m = idset_lookup(g->modules, module_id)) != NULL)
	{
		unsigned int	i;

		for(i = plugin_portset_size(m->instance.inputs); i-- > 0;)
		{
			uint32	link;

			if(module_id == ti->module_id && i == ti->input)
				link = ti->source;
			else if(plugin_portset_get_module(m->instance.inputs, i, &link) && idset_lookup(g->modules, link))
				;
			else
				continue;
			if(link == source_id)
				return FALSE;
			if(!traverse_module(g, link, source_id, ti))
				return FALSE;
		}
	}
	return TRUE;
}

/* Answer the fairly specific question: does the graph <graph_id> become cyclic if module
 * <module_id>'s <input>:th input is set to be <source>? Used to disallow such setting.
 * This implementation is totally naive, and requires O(n^2) for a graph with n nodes.
*/
static boolean graph_cyclic_after(uint32 graph_id, uint32 module_id, uint8 input, uint32 source)
{
	const Graph	*g;
	unsigned int	i;
	size_t		num;
	struct traverse_info	ti;

	if((g = idset_lookup(graph_info.graphs, graph_id)) == NULL)
		return FALSE;

	num = idset_size(g->modules);

	ti.module_id = module_id;
	ti.input     = input;
	ti.source    = source;

	for(i = 0; i < num; i++)
	{
		if(!traverse_module(g, i, i, &ti))
			return TRUE;
	}
	return FALSE;
}

/* ----------------------------------------------------------------------------------------- */

/* Build description of a single module, as a freshly created dynamic string. */
static DynStr * module_build_desc(const Module *m)
{
	DynStr	*d;
	uint32	i;

	d = dynstr_new_sized(256);
	dynstr_append_printf(d, " <module id=\"%u\" plug-in=\"%u\">\n", m->id, plugin_id(m->plugin));
	plugin_portset_describe(m->instance.inputs, d);
	for(i = 0; i < m->out.nodes.next; i++)
	{
		const PNode	**n = dynarr_index(m->out.nodes.node, i);

		if(n != NULL && (*n)->creator.remote != NULL)
		{
			dynstr_append_printf(d, "  <out label=\"%u\" name=\"", i);
			xml_dynstr_append(d, (*n)->creator.remote->name);
			dynstr_append(d, "\"/>\n");
		}
	}
	dynstr_append(d, " </module>\n");

	return d;
}

/* Update start positions of module descriptions. Handy when one changes/appears. */
static void graph_modules_desc_start_update(Graph *g)
{
	unsigned int	id;
	Module		*m;
	size_t		pos;

	for(id = idset_foreach_first(g->modules), pos = g->desc_start;
	    (m = idset_lookup(g->modules, id)) != NULL;
	    id = idset_foreach_next(g->modules, id))
	{
		m->start = pos;
		pos += m->length;
	}
}

/* Update description of module <m>. */
static void module_describe(Module *m)
{
	DynStr	*desc;

	desc = module_build_desc(m);
	verse_send_t_text_set(m->graph->node, m->graph->buffer, m->start, m->length, dynstr_string(desc));
	m->length = dynstr_length(desc);
	dynstr_destroy(desc, 1);
	graph_modules_desc_start_update(m->graph);
}


static PPOutput cb_module_lookup(uint32 module_id, void *data)
{
	Module	*m;

	if((m = idset_lookup(((Graph *) data)->modules, module_id)) != NULL)
		return &m->out.port;
	return NULL;
}

/* Create a new module, i.e. a plug-in instance, in a graph. If <module_id> is not ~0,
 * the module is created with it as its ID. Very handy during resume.
*/
static void module_create(unsigned int module_id, uint32 graph_id, uint32 plugin_id)
{
	Plugin	*p;
	Graph	*g;
	Module	*m;
	DynStr	*desc;

	if((p = plugin_lookup(plugin_id)) == NULL)
	{
		LOG_WARN(("Attempted to instantiate plug-in %u; not found", plugin_id));
		return;
	}
	if((g = idset_lookup(graph_info.graphs, graph_id)) == NULL)
	{
		LOG_WARN(("Attempted to instantiate plug-in %u in unknown graph %u, aborting", plugin_id, graph_id));
		return;
	}
	m = memchunk_alloc(graph_info.chunk_module);
	if(m == NULL)
	{
		LOG_WARN(("Module allocation failed in graph %u, plug-in %u", graph_id, plugin_id));
		return;
	}
	m->graph = g;
	m->plugin = p;
	port_init(&m->out.port);
	plugin_instance_init(m->plugin, &m->instance);
	plugin_instance_set_output(&m->instance, &m->out.port);
	plugin_instance_set_link_resolver(&m->instance, cb_module_lookup, g);
	idlist_construct(&m->out.dependants);
	m->out.nodes.node = NULL;
	m->out.nodes.next = 0;
	m->out.resume = NULL;
	m->start = m->length = 0;
	if(g->modules == NULL)
		g->modules = idset_new(0);
	if(module_id == ~0u)
		m->id = idset_insert(g->modules, m);
	else
		m->id = idset_insert_with_id(g->modules, module_id, m);
	LOG_MSG(("Module %u.%u is plug-in %u (%s) in graph at %p", graph_id, m->id, plugin_id, plugin_name(p), g));
/*	{
		Module	*m2;

		m2 = idset_lookup(g->modules, m->id);
		if(m != m2)
		{
			LOG_ERR(("Module look-up failed!!"));
			exit(EXIT_FAILURE);
		}
	}
*/	desc = module_build_desc(m);
	m->length = dynstr_length(desc);
	graph_modules_desc_start_update(g);
	verse_send_t_text_set(g->node, g->buffer, m->start, 0, dynstr_string(desc));
	dynstr_destroy(desc, 1);

	/* Newly created plug-in might be ready to run right away, thanks to defaults. Check, and schedule if so. */
	if(plugin_instance_inputs_ready(&m->instance))
		sched_add(&m->instance);
}

/* Release all labeled nodes created by an instance. */
static void output_nodes_clear(Output *o)
{
	if(o->nodes.node != NULL)
	{
		unsigned int	i;
		PNode		**n;

		for(i = 0; i < o->nodes.next; i++)
		{
			if((n = dynarr_index(o->nodes.node, i)) != NULL)
				nodedb_unref(*n);
		}
		dynarr_destroy(o->nodes.node);
	}
}

/* Destroy a module. */
static void module_destroy(uint32 graph_id, uint32 module_id)
{
	Graph	*g;
	Module	*m;

	if((g = idset_lookup(graph_info.graphs, graph_id)) == NULL)
	{
		LOG_WARN(("Attempted to destroy module %u in unknown graph %u, aborting", module_id, graph_id));
		return;
	}
	if((m = idset_lookup(g->modules, module_id)) == NULL)
	{
		LOG_WARN(("Attempted to destroy unknown module %u in graph %u, aborting", module_id, graph_id));
		return;
	}
	module_dep_destroy_warning(m);
	idset_remove(g->modules, module_id);
	verse_send_t_text_set(g->node, g->buffer, m->start, m->length, NULL);
	idlist_destruct(&m->out.dependants);
	plugin_instance_free(&m->instance);
	port_clear(&m->out.port);
	output_nodes_clear(&m->out);
	memchunk_free(graph_info.chunk_module, m);
	graph_modules_desc_start_update(g);
}

static void do_module_input_set(Graph *g, uint32 module_id, uint8 input_index, PValueType type, int string, va_list arg)
{
	Module	*m;
	DynStr	*desc;
	uint32	old_link;

	if((m = idset_lookup(g->modules, module_id)) == NULL)
	{
		LOG_WARN(("Attempted to set module input in non-existant module %u", module_id));
		return;
	}

	printf("setting module input %u, type %d\n", input_index, type);

	/* Remove any existing dependency by this input. */
	if(plugin_portset_get_module(m->instance.inputs, input_index, &old_link))
		module_dep_remove(g, old_link, m->id);

	if(string)
		plugin_portset_set_from_string(m->instance.inputs, input_index, type, va_arg(arg, const char *));
	else
		plugin_portset_set_va(m->instance.inputs, input_index, type, arg);
	desc = module_build_desc(m);
	verse_send_t_text_set(g->node, g->buffer, m->start, m->length, dynstr_string(desc));
	m->length = dynstr_length(desc);
	dynstr_destroy(desc, 1);
	graph_modules_desc_start_update(g);

	/* Did we just set a link to someone? Then notify that someone about new dependant. */
	if(type == P_VALUE_MODULE)
	{
		uint32	other;

		plugin_portset_get_module(m->instance.inputs, input_index, &other);
		module_dep_add(g, other, m->id);
	}
	sched_add(&m->instance);
}

/* Set a module input to a value. The value might be either a literal, or a reference to another module's output. */
static void module_input_set(uint32 graph_id, uint32 module_id, uint8 input_index, PValueType type, ...)
{
	va_list	arg;
	Graph	*g;

	if((g = idset_lookup(graph_info.graphs, graph_id)) == NULL)
	{
		LOG_WARN(("Attempted to set module input in non-existant graph %u", graph_id));
		return;
	}
	va_start(arg, type);
	do_module_input_set(g, module_id, input_index, type, 0, arg);
	va_end(arg);
}

/* Set module input from a string representation, passed as the single vararg. Only used by resume. */
static void module_input_set_from_string(Graph *g, uint32 module_id, uint8 input_index, PValueType type, ...)
{
	va_list	arg;

	va_start(arg, type);
	do_module_input_set(g, module_id, input_index, type, 1, arg);
	va_end(arg);
}

/* Clear a module input, i.e. remove any assigned value and leave it "floating" as after module creation. */
static void module_input_clear(uint32 graph_id, uint32 module_id, uint8 input_index)
{
	Graph	*g;
	Module	*m;
	DynStr	*desc;
	boolean	was_link;
	uint32	old_link;

	if((g = idset_lookup(graph_info.graphs, graph_id)) == NULL)
	{
		LOG_WARN(("Attempted to clear module input in non-existant graph %u", graph_id));
		return;
	}
	if((m = idset_lookup(g->modules, module_id)) == NULL)
	{
		LOG_WARN(("Attempted to clear module input in non-existant module %u.%u", graph_id, module_id));
		return;
	}
	was_link = plugin_portset_get_module(m->instance.inputs, input_index, &old_link);
	plugin_portset_clear(m->instance.inputs, input_index);

	desc = module_build_desc(m);
	verse_send_t_text_set(g->node, g->buffer, m->start, m->length, dynstr_string(desc));
	m->length = dynstr_length(desc);
	dynstr_destroy(desc, 1);
	graph_modules_desc_start_update(g);

	/* If a link was cleared, notify other end it has one less dependants. */
	if(was_link)
		module_dep_remove(g, old_link, m->id);

	/* The module might need to re-compute, if clearing the input reverted
	 * it to a default value, and all other inputs are ready.
	*/
	if(plugin_instance_inputs_ready(&m->instance))
		sched_add(&m->instance);
}

/* Go through module's inputs, and clear any inputs that refer to module <rm>. This typically happens
 * because it (rm) is about to be deleted, and we don't want any dangling links afterwards.
*/
static void module_input_clear_links_to(Graph *g, uint32 module_id, uint32 rm)
{
	Module	*m;
	size_t	ni, i;
	boolean	refresh = FALSE;
	uint32	lt;

	if((m = idset_lookup(g->modules, module_id)) == NULL)
		return;
	ni = plugin_portset_size(m->instance.inputs);
	for(i = 0; i < ni; i++)
	{
		if(plugin_portset_get_module(m->instance.inputs, i, &lt) && lt == rm)
		{
			plugin_portset_clear(m->instance.inputs, i);
			refresh = TRUE;
		}
	}
	if(refresh)
	{
		DynStr	*desc;

		desc = module_build_desc(m);
		verse_send_t_text_set(g->node, g->buffer, m->start, m->length, dynstr_string(desc));
		m->length = dynstr_length(desc);
		dynstr_destroy(desc, 1);
		graph_modules_desc_start_update(g);
	}
}

/* ----------------------------------------------------------------------------------------- */

void send_method_call(int method, const VNOParam *param)
{
	const MethodInfo *mi;
	VNOPackedParams *pack;

	if(method < 0 || (size_t) method >= sizeof method_info / sizeof *method_info)
		return;
	mi = method_info + method;
	if(mi->id == (uint8) ~0u)
	{
		LOG_WARN(("Can't send call to method %s(), not created yet", mi->name));
		return;
	}
	if((pack = verse_method_call_pack(mi->param_count, mi->param_type, param)) != NULL)
		verse_send_o_method_call(client_info.avatar, client_info.gid_control, mi->id, 0, pack);
}

void graph_method_send_call_create(VNodeID node, VLayerID buffer, const char *name)
{
	VNOParam	param[3];

	param[0].vnode   = node;
	param[1].vlayer  = buffer;
	param[2].vstring = (char *) name;
	send_method_call(CREATE, param);
}

void graph_method_send_call_destroy(uint32 id)
{
	VNOParam	param[1];

	param[0].vuint32 = id;
	send_method_call(DESTROY, param);
}

void graph_method_send_call_mod_create(uint32 graph_id, uint32 plugin_id)
{
	VNOParam	param[2];

	param[0].vuint32 = graph_id;
	param[1].vuint32 = plugin_id;
	send_method_call(MOD_CREATE, param);
}

void graph_method_send_call_mod_destroy(uint32 graph_id, uint32 module_id)
{
	VNOParam	param[2];

	param[0].vuint32 = graph_id;
	param[1].vuint32 = module_id;
	send_method_call(MOD_DESTROY, param);
}

void graph_method_send_call_mod_input_set(uint32 graph_id, uint32 mod_id, uint32 index, PValueType vtype, const PValue *value)
{
	VNOParam	param[4];
	VNOParamType	type[4] = { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT8 };
	VNOPackedParams	*pack;
	int		method = 0, i;

	param[0].vuint32 = graph_id;
	param[1].vuint32 = mod_id;
	param[2].vuint8  = index;
	/* Map module input type "down" to Verse method call parameter type. */
	switch(vtype)
	{
	case P_VALUE_BOOLEAN:
		type[3] = VN_O_METHOD_PTYPE_UINT8;
		param[3].vuint8 = value->v.vboolean;
		method = MOD_INPUT_SET_BOOLEAN;
		break;
	case P_VALUE_INT32:
		type[3] = VN_O_METHOD_PTYPE_INT32;
		param[3].vint32 = value->v.vint32;
		method = MOD_INPUT_SET_INT32;
		break;
	case P_VALUE_UINT32:
		type[3] = VN_O_METHOD_PTYPE_UINT32;
		param[3].vuint32 = value->v.vuint32;
		method = MOD_INPUT_SET_UINT32;
		break;
	case P_VALUE_REAL32:
		type[3] = VN_O_METHOD_PTYPE_REAL32;
		param[3].vreal32 = value->v.vreal32;
		method = MOD_INPUT_SET_REAL32;
		break;
	case P_VALUE_REAL32_VEC2:
		type[3] = VN_O_METHOD_PTYPE_REAL32_VEC2;
		param[3].vreal32_vec[0] = value->v.vreal32_vec2[0];
		param[3].vreal32_vec[1] = value->v.vreal32_vec2[1];
		method = MOD_INPUT_SET_REAL32_VEC2;
		break;
	case P_VALUE_REAL32_VEC3:
		type[3] = VN_O_METHOD_PTYPE_REAL32_VEC3;
		param[3].vreal32_vec[0] = value->v.vreal32_vec3[0];
		param[3].vreal32_vec[1] = value->v.vreal32_vec3[1];
		param[3].vreal32_vec[2] = value->v.vreal32_vec3[2];
		method = MOD_INPUT_SET_REAL32_VEC3;
		break;
	case P_VALUE_REAL32_VEC4:
		type[3] = VN_O_METHOD_PTYPE_REAL32_VEC4;
		param[3].vreal32_vec[0] = value->v.vreal32_vec4[0];
		param[3].vreal32_vec[1] = value->v.vreal32_vec4[1];
		param[3].vreal32_vec[2] = value->v.vreal32_vec4[2];
		param[3].vreal32_vec[3] = value->v.vreal32_vec4[3];
		method = MOD_INPUT_SET_REAL32_VEC4;
		break;
	case P_VALUE_REAL32_MAT16:
		type[3] = VN_O_METHOD_PTYPE_REAL32_MAT16;
		for(i = 0; i < 16; i++)
			param[3].vreal32_mat[i] = value->v.vreal32_mat16[i];
		method = MOD_INPUT_SET_REAL32_MAT16;
		break;
	case P_VALUE_REAL64:
		type[3] = VN_O_METHOD_PTYPE_REAL64;
		param[3].vreal64 = value->v.vreal64;
		method = MOD_INPUT_SET_REAL64;
		break;
	case P_VALUE_REAL64_VEC2:
		type[3] = VN_O_METHOD_PTYPE_REAL64_VEC2;
		param[3].vreal64_vec[0] = value->v.vreal64_vec2[0];
		param[3].vreal64_vec[1] = value->v.vreal64_vec2[1];
		method = MOD_INPUT_SET_REAL64_VEC2;
		break;
	case P_VALUE_REAL64_VEC3:
		type[3] = VN_O_METHOD_PTYPE_REAL64_VEC3;
		param[3].vreal64_vec[0] = value->v.vreal64_vec3[0];
		param[3].vreal64_vec[1] = value->v.vreal64_vec3[1];
		param[3].vreal64_vec[2] = value->v.vreal64_vec3[2];
		method = MOD_INPUT_SET_REAL64_VEC3;
		break;
	case P_VALUE_REAL64_VEC4:
		type[3] = VN_O_METHOD_PTYPE_REAL64_VEC4;
		param[3].vreal64_vec[0] = value->v.vreal64_vec4[0];
		param[3].vreal64_vec[1] = value->v.vreal64_vec4[1];
		param[3].vreal64_vec[2] = value->v.vreal64_vec4[2];
		param[3].vreal64_vec[3] = value->v.vreal64_vec4[3];
		method = MOD_INPUT_SET_REAL64_VEC4;
		break;
	case P_VALUE_REAL64_MAT16:
		type[3] = VN_O_METHOD_PTYPE_REAL64_MAT16;
		for(i = 0; i < 16; i++)
			param[3].vreal64_mat[i] = value->v.vreal64_mat16[i];
		method = MOD_INPUT_SET_REAL64_MAT16;
		break;
	case P_VALUE_MODULE:
		type[3] = VN_O_METHOD_PTYPE_UINT32;
		param[3].vuint32 = value->v.vmodule;
		method = MOD_INPUT_SET_MODULE;
		break;
	case P_VALUE_STRING:
		type[3] = VN_O_METHOD_PTYPE_STRING;
		param[3].vstring = value->v.vstring;
		method = MOD_INPUT_SET_STRING;
		break;
	default:
		LOG_WARN(("Can't prepare input setting, type %d", vtype));
		return;
	}
	if((pack = verse_method_call_pack(4, type, param)) != NULL)
		verse_send_o_method_call(client_info.avatar, client_info.gid_control,
					 method_info[method].id, 0, pack);

}

void graph_method_send_call_mod_input_clear(uint32 graph_id, uint32 module_id, uint32 input)
{
	VNOParam	param[3];

	param[0].vuint32 = graph_id;
	param[1].vuint32 = module_id;
	param[2].vuint8  = input;
	send_method_call(MOD_INPUT_CLEAR, param);
}

void graph_method_receive_call(uint8 id, const VNOPackedParams *param)
{
	VNOParam	arg[8];
	unsigned int	i;

	for(i = 0; i < sizeof method_info / sizeof *method_info; i++)
	{
		const MethodInfo	*mi = method_info + i;

		if(id != mi->id)
			continue;
		verse_method_call_unpack(param, mi->param_count, mi->param_type, arg);
		switch(i)
		{
		case CREATE:	graph_create(~0u, arg[0].vnode, arg[1].vlayer, arg[2].vstring);	break;
		case DESTROY:	graph_destroy(arg[0].vuint32);					break;
		case MOD_CREATE: module_create(~0u, arg[0].vuint32, arg[1].vuint32);		break;
		case MOD_DESTROY: module_destroy(arg[0].vuint32, arg[1].vuint32);		break;
		case MOD_INPUT_CLEAR:
			module_input_clear(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8);
			break;
		case MOD_INPUT_SET_BOOLEAN:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_BOOLEAN, arg[3].vuint8);
			break;
		case MOD_INPUT_SET_INT32:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_INT32, arg[3].vint32);
			break;
		case MOD_INPUT_SET_UINT32:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_UINT32, arg[3].vuint32);
			break;
		case MOD_INPUT_SET_REAL32:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL32, arg[3].vreal32);
			break;
		case MOD_INPUT_SET_REAL32_VEC2:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL32_VEC2, &arg[3].vreal32_vec);
			break;
		case MOD_INPUT_SET_REAL32_VEC3:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL32_VEC3, &arg[3].vreal32_vec);
			break;
		case MOD_INPUT_SET_REAL32_VEC4:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL32_VEC4, &arg[3].vreal32_vec);
			break;
		case MOD_INPUT_SET_REAL32_MAT16:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL32_MAT16, &arg[3].vreal32_mat);
			break;
		case MOD_INPUT_SET_REAL64:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL64, arg[3].vreal64);
			break;
		case MOD_INPUT_SET_REAL64_VEC2:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL64_VEC2, &arg[3].vreal64_vec);
			break;
		case MOD_INPUT_SET_REAL64_VEC3:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL64_VEC3, &arg[3].vreal64_vec);
			break;
		case MOD_INPUT_SET_REAL64_VEC4:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL64_VEC4, &arg[3].vreal64_vec);
			break;
		case MOD_INPUT_SET_REAL64_MAT16:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL64_MAT16, &arg[3].vreal64_mat);
			break;
		case MOD_INPUT_SET_MODULE:
			if(!graph_cyclic_after(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, arg[3].vuint32))
				module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_MODULE, arg[3].vuint32);
			break;
		case MOD_INPUT_SET_STRING:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_STRING, arg[3].vstring);
			break;
		}
		return;
	}
	LOG_WARN(("Received call to unknown graph method %u", id));
}
