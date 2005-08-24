/*
 * api-init.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Initialize a library, letting it describe the plug-ins it defines.
*/

/** \page model Purple's Computational Model
 * 
 * Purple is a \e computational \e engine. This means it needs to have some way of actually
 * performing computations, and this page details what this way is.
 * 
 * \section modelplugin Plug-Ins
 * The core is of course the plug-in. A plug-in is conceptually a box, with a number of
 * inputs and one single output:
 * \image html plugin.png
 * The value on the output is assumed to be a "pure" function of the values on the inputs.
 * This means that the value stays constant as long as the inputs don't change, and that
 * a given combination of input values should always result in the same output. A plug-in
 * is assumed to have some way of actually \e doing some computation, at the request of the
 * Purple engine.
 * 
 * \subsection triggering Triggering
 * Whenever a plug-in is instantiated, Purple will monitor its inputs. If an input changes,
 * Purple adds the instance to a internal queue of modules that need to be re-computed. It
 * will then try to empty this queue by letting the modules run.
 * 
 * Since Purple can handle many instances of the same plug-in at once, the word \e module
 * is often used to refer to such an instance. The terms plug-in and module have a simliar
 * relationship as the terms class and object, as used in object-oriented programming. In
 * this document, the two terms are used a bit more loosely.
 * 
 * \subsection inputs Input Semantics
 * Inputs to a Purple plug-in are slightly complicated by the support for default values.
 * This means that rather than being in either of two states (set or cleared), an input
 * can be restricted to always being set.
 * 
 * If a default value exists for a given input, the Purple engine will set the input to
 * that value unless it is overridden by an explicit command to set it to some other value.
 * If an attempt to clear the input is made, the input will simply revert to the default
 * value. In such cases, the plug-in code never has to deal with an unset value.
 * 
 * Default values can trigger computation. Consider a plug-in with N inputs, all of which
 * have a default value assigned. As soon as that plug-in is instantiated into a module,
 * all the module's inputs will be set to their proper default values. This changes the
 * inputs (from the original, very short-lived, "not set" state), and thus triggers computation.
 * 
 * Inputs that are set to their default values will still be described externally (in the
 * graph XML representation) as any other input; it is not treated as a special case from
 * the external perspective. This means that asking Purple to clear an input set to some
 * value, might end up (in the XML) setting it to some other, namely the default.
 *
 * \section modelgraph Graphs
 * Plug-ins alone cannot accomplish much. Things get interesting when several plug-ins are
 * connected together, to form \e graphs. A graph is simply a "container" for modules, a
 * place where they are created. A module belongs to exactly one graph; it is not possible
 * to create modules outside of a graph, and it is not possible to share a single module
 * between several graphs. Of course, a \e plug-in is perfectly sharable, and it is possible
 * to create instances of a single plug-in in any number of graphs at the same time. None
 * of these relationships should be very surprising.
 * 
 * For this discussion, a graph is simply a named container holding any number of modules.
 * It also stores information about module input connections, and any constant values assigned
 * to inputs.
 *
 * The below image shows a sample graph structure:
 * \image html purple-graph.png
 * Here, individual plug-in inputs are not shown, connections are simply made between "boxes".
 * The depicted graph is based on a real, working example. It does the following, from left
 * to right:
 * -# The \c cube plug-in generates a basic cube object and geometry.
 * -# The \c bbox plug-in computes the bounding-box of the input geometry.
 * -# The \c warp plug-in does a simple rotation along the Y axis of its input.
 * -# The built-in (see below) \c node-output plug-in sends the results out to the Verse host.
 * 
 * \section builtins Built-In Plug-Ins
 * There are two special needs that arise when contemplating doing some kind of data-driven
 * computation in a Verse setting: getting node data into the computation, and getting results
 * back out. These needs are solved through a couple of \e built-in plug-ins, that are described
 * below:
 *
 * \subsection builtininput Input
 * The first need is easy enough to describe: imagine having written a Purple plug-in that
 * does some operation to a bitmap, for like applying a "blur" filter. Now you want to
 * apply this to a bitmap you know resides on a Verse host, perhaps because you've just
 * created it in some Verse-aware paint program. You can create a graph and instantiate your
 * plug-in into it, but then what? How do you connect your graph to an actual Verse node?
 * 
 * The solution is to create an instance of the built-in plug-in \c "node-input". This plug-in
 * is implemented inside the Purple engine, and works like this:
 * - It has a single input, accepting a string named "name".
 * - It outputs the Verse node matching that name, if it exists.
 * - It monitors the \b node for changes, and outputs it again if it changes.
 * 
 * The last point might need some further explanation: what it means is that the plug-in
 * behaves as if its output changed, whenever the input \b node changes. This kind of
 * node "watching" cannot be directly implemented using the Purple API, but since
 * \c node-input is internal to the engine it can provide features like this.
 * 
 * The \c node-input plug-in is currently limited to watching only a single Verse node;
 * this is a restriction that might be lifted in the future. For now, it provides a good
 * solution to the problem of getting data into Purple.
 * 
 * \subsection builtinoutput Output
 * To continue the above example, once a bitmap has passed through our "blur" filter plug-in,
 * how do we get the data back out?
 * 
 * One obvious answer is that it could be implicit, that Purple automatically should take
 * any processed data and send it out to the Verse server is possible. However, this is not
 * the direction taken by the current Purple system. Instead, a similiar explicit approach
 * as with input is used. There is another built-in plug-in called \c "node-output", that
 * works like this:
 * - It has a single input, accepting node data.
 * - All nodes are added to the internal \ref synchronizing "node synchronization" queue.
 * - This results in all the node data being sent out to the Verse host.
 * 
 * Through this plug-in, data processed (or generated) with-in Purple is exported "out"
 * to a Verse host, where it appears like any other. It is therefore sharable, and viewable
 * with standard Verse tools.
 *
 * \section synchronizing The Synchronizer
 * Purple contains an "intelligent node comparator", called \e the \e synchronizer, whose job
 * it is to compare two nodes, and generate a list of Verse commands from the differences.
 * These commands are sent to the Verse host, where they are applied to modify one node into
 * a copy of another.
 * 
 * This is used by the \c node-output plugin. The nodes that are input into the plug-in are
 * handed off to the synchronizer. The synchronizer runs from Purple's main loop, and on
 * each iteration through the loop, it gets a chance to do some work. The work it does it
 * split over many iterations of the main loop, since it often takes a lot of time to complete.
 * 
 * It compares nodes using hand-written node-specific code, and any differences found
 * between the local node (the result of a graph's computation) and the remote one (as
 * described to the engine by the Verse host) result in changes being sent to change
 * the state of the remote node.
 * 
 * If no changes are found in a comparison, the node is removed from the synchronizer's
 * working set.
 */

/**
 * \page devplugin Writing Purple Plug-Ins
 * 
 * This page introduces the concept of the Purple plug-in; the core computational element
 * in the Purple system. It is intended to be read by developers interested in writing
 * their own plug-ins.
 * 
 * \section anatomy Anatomy of a Library
 * Purple plug-ins live in \e libraries. A library is simply a piece of program code, that
 * can be dynamically loaded. How this is done varies with the platform that Purple is
 * running on. Each library can contain one or more actual plug-ins.
 * \note In Windows, such libraries are called "dynamic load libraries", or DLLs. In
 * Unix-style environments, the term is "shared objects". The author of a library typically
 * does not need to care about the difference.
 * 
 * A library has a single externally-visible part: the function \c init(), which is called by
 * Purple after the library has loaded. It is up to the \c init() function to \e describe the
 * library's contents in a way understandable to the Purple engine. This is done by simply
 * calling functions in the \ref api_init "initialization API".
 *
 * The part of a plug-in that does the actual work is another function, often referred to
 * as \c compute(). This function is registered with Purple during the initialization phase
 * outlined above. Purple will run this function whenever it decides that the plug-in needs
 * to re-compute its output value(s). Typically, this is because one or more input changed.
 * 
 * There is one \c compute() per (logical) plug-in, and often the fact that each plug-in lives
 * in a library is ignored.
 * \note There is no enforced name that must be used for the computational callback. Since
 * it is never referenced by name by Purple, it is not possible to specify one either. In
 * this document, it is typically referred to as if named \c compute(), but it could just as
 * well be named \c foo_bar_baz_banana(). Purple only learns about a plug-in's computational
 * callback through the pointer passed to \c p_init_compute(). The reason the function must
 * be explicitly registered, rather than being found by a default name, is to support multiple
 * plug-ins per library.
 * 
 * \section work Getting Work Done
 * Once inside a plug-in's \c compute() callback, there needs to be a way to access inputs,
 * and/or create results. In the general case, this involves three steps:
 * - Read inputs, to get parameters controlling the operation to be performed and/or data to
 * perform operations on.
 * - Create, destroy, traverse and/or modify node data structures.
 * - Output some kind of result.
 * 
 * Purple has sub-APIs for each of these three tasks, in the form of the \ref api_input, the
 * \ref api_node, and the \ref api_output.
 * 
 * A few general points to notice about how a plug-ins computational callback should be
 * structured:
 * - Group the input-reading close to the start of the function, and try to use the same
 * order as the inputs were registered in. This makes it easier to compare against the
 * initialization code.
 * - Purple is a cooperative multitasking system. It cannot stop a plug-in from running,
 * once given the CPU. So plug-in's need to be short and efficient in their \c compute()
 * functions.
 * - Code needs to be manually structured not to take "too long" to exeucte. If the plug-in
 * aborts prematurely in order to conserve cycles, it should return \c P_COMPUTE_AGAIN. This
 * signals to Purple that the plug-in didn't finish, and keeps it scheduled to run.
 * - A plug-in that aborts and returns \c P_COMPUTE_AGAIN must remember its internal state
 * on its own, so it can continue where it left off when called again. The per-instance
 * state created with \c p_init_state() can be useful here.
 * 
 * \section meta Meta Information
 * Through the use of the \c p_init_meta() initialization call, Purple plug-ins have the
 * ability to tell the Purple engine, and through that the world, about themselves. This
 * is currently done by registering category/text-pairs. Each category's text can only be
 * set once; if the same category is registered twice, it is as if the first never happened.
 *
 * The slash character has a special meaning in categories. It is used as a hierarchy separator;
 * allowing categories to express several levels of hierarchy. The total length of the category
 * string as given in the call must still not exceed 64 characters, including the slashes.
 * 
 * The following table summarizes the recommended meta categories:
 * <table>
 * <tr><th>Category</th><th>Recommended Usage</th></tr>
 * 
 * <tr><td>authors</td>
 * <td>Used to names the author(s) of the plug-in. Separate multiple names by semicolons. Suggested
 * form is simply "givenname surname", i.e. "Emil Brink". Sorting is optional; it is assumed
 * that a sufficiently advanced UI client might sort the list when presenting it.
 * </tr>
 * <tr><td>copyright</td>
 * <td>Used to indicate the name of the copyright holder of this plug-in. Suggested form is
 * "time holder", where \e time is a year or a list of years, such as "2005", or "2001, 2002"
 * or "1998-2005". The \e holder part is simply the name of whatever entity has the copyright.
 * Separate multiple holders with semicolons, as in "1998-2001 Example.com; 2001-2005 Foo".</td>
 * </tr>
 * <tr><td>license</td>
 * <td>Used to give the name of the license under which this plug-in is made available.</td>
 * <tr>
 * <td>meta/purpose</td>
 * <td>Describes the purpose of the plug-in, using a few English phrases.</td>
 * </tr>
 * </table>
*/

/**
 * 
 * \page pluginex Example Purple Plug-Ins
 *
 * Here is a complete, working example plug-in:
 * 
 * \code
 * // Sample Purple plug-in to add two integers.
 * 
 * #include "purple.h"
 * 
 * static PComputeStatus compute(PPInput *input, PPOutput output, void *user)
 * {
 * 	int32	a = p_input_int32(input[0]), b = p_input_int32(input[1]);
 * 
 * 	p_output_int32(output, a + b);
 * 	return P_COMPUTE_DONE;
 * }
 * 
 * PURPLE_PLUGIN void init(void)
 * {
 * 	p_init_create("add");
 * 	p_init_compute(compute);
 * }
 * \endcode
 * Things to notice about the code:
 * - A plug-in needs to include the \c purple.h header.
 * - The \c compute() function is \c static.
 * - This computation is very quick, so \c compute() always returns \c P_COMPUTE_DONE.
 * - The \c init() function must be prefixed with the \c PURPLE_PLUGIN macro, to ensure it's visible to the Purple engine.
 * 
*/

#include <stdarg.h>
#include <stdio.h>

#define PURPLE_INTERNAL

#include "log.h"
#include "purple.h"
#include "plugins.h"

/* ----------------------------------------------------------------------------------------- */

static struct
{
	Library	*owner;
	Plugin	*plugin;
} init_info = { NULL };

/* ----------------------------------------------------------------------------------------- */

/* Begin use of Init API, expecting calls to come from code held in <owner>. If the owning
 * library is NULL, a "magical" plug-in that is part of the Purple core is being initialized.
*/
void api_init_begin(Library *owner)
{
	init_info.owner = owner;
	if(init_info.plugin != NULL)
		LOG_WARN(("Previous plug-in pointer not reset (was %p)", init_info.plugin));
	init_info.plugin = NULL;
}

static void plugin_flush(void)
{
	if(init_info.plugin != NULL)
		plugins_register(init_info.owner, init_info.plugin);
	init_info.plugin = NULL;
}

void api_init_end(void)
{
	plugin_flush();
	init_info.owner = NULL;
}

/* ----------------------------------------------------------------------------------------- */

/* These are the plug-in-visible actual Purple API functions. */

/** \defgroup api_init Plug-In Initialization Functions
 * 
 * Functions in this group are used to initialize a plug-in. They are always used exclusively from
 * a library's \c init() function, and never from the \c compute() callback. There are five functions,
 * three of which are optional.
 * 
 * The most important functions are \c p_init_create() and \c p_init_compute(), they must be used
 * in every library's \c init() function. Any plug-in that needs inputs to function, which will be
 * vast majority, needs to have them defined using a number of calls to \c p_init_input(). Using
 * \c p_init_meta() to register meta information is a very good idea, but not mandatory at the time
 * of writing. If per-instance persistent state is desired, use the \c p_init_state() function to
 * define it.
 * 
 * Here's an example of a library \c init() function that defines three separate plug-ins that all
 * share the same \c compute() code:
 * 
 * \code
 * #include "purple.h"
 * 
 * static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
 * {
 * 	// ... code here
 * 	return P_COMPUTE_DONE;
 * }
 * 
 * PURPLE_PLUGIN void init(void)
 * {
 * 	p_init_create("foo");
 * 	p_init_compute(compute);
 * 
 * 	p_init_create("bar");
 * 	p_init_compute(compute);
 * 
 * 	p_init_create("baz");
 * 	p_init_compute(compute);
 * }
 * \endcode
 * 
 * Things to note:
 * - A new plug-in is created by each call to the \c p_init_create() function.
 * - The \c compute() callback is set by \c p_init_compute().
 * - A plug-in does \b not "inherit" any initialization from the previous one.
 * - The \c compute() function does not need to be seen from the outside, it can
 * be defined as \c static. This is true for \b all functions a library might
 * contain, except \c init().
 * - The init() function needs to be "prefixed" with the PURPLE_PLUGIN macro to
 * work properly on Windows platforms.
 * @{
*/

/**
 * This function is called from within the \c init() function of a plug-in library, to register a new
 * plug-in with the Purple engine. You can do this any number of times, since a single library (i.e.
 * the shared object or DLL file on disk) can define many actual plug-ins. This is often useful when
 * one wants to share code between two plug-ins in an easy manner.
 * 
 * All calls to other init functions affect the current plug-in, which is the one that was registered last.
 * 
 * There \b must be at least one call to this function in a library's \c init() function, or the
 * library will be ignored by the Purple engine.
*/
PURPLEAPI void p_init_create(const char *name /** The name of the plug-in to create. Plug-in names should
					       be short and descriptive, and not include any whitespace.
					       Use a dash or underscore instead of a space. */)
{
	plugin_flush();
	init_info.plugin = plugin_new(name);
}

/**
 * Register a piece of "meta information" associated with the current plug-in. Such meta information is
 * intented to be presented to end users by user interface clients, and might also provide grouping support
 * so that, for instance, related tools can be grouped together in the interface.
 * 
 * Each piece of meta information is defined as a pair of strings, one being called the category and the
 * other the text. These can be considered "assignments", like so: \a category = \a text. This implies
 * that you cannot specify the same category more than once, doing so will replace whatever text was
 * previously set.
 * 
 * \see The \ref meta section.
*/
PURPLEAPI void p_init_meta(const char *category	/** The category to register meta text in. */,
			   const char *text	/** The meta text to register. */)
{
	plugin_set_meta(init_info.plugin, category, text);
}

/**
 * Add an input to the current plug-in. Inputs are "ports" where the plug-in can receive data from other
 * plug-ins, or directly from the user.
 * 
 * This function is varargs to support the specification of further detail. You can use the \c P_INPUT macros
 * to build a list of extra information, including minimum, maximum and default values, make the input
 * required, and so on. To end the list, use \c P_INPUT_DONE.
 * 
 * The full set of such macros is described below:
 * - \c P_INPUT_MIN(v) - Set minimum for input of type int32, uint32, real32, or real64.
 * - \c P_INPUT_MIN_VEC2(x,y) - Set minimum for input of type real32_vec2 or real64_vec2.
 * - \c P_INPUT_MIN_VEC3(x,y,z) - Set minimum for input of type real32_vec3 or real64_vec3.
 * - \c P_INPUT_MIN_VEC4(x,y,z,w) - Set minimum for input of type real32_vec4 or real64_vec4.
 * - \c P_INPUT_MAX(), P_INPUT_MAX_VEC2(), P_INPUT_MAX_VEC3(), P_INPUT_MAX_VEC4() - Like the _MIN_ macros, but sets maximum instead.
 * - \c P_INPUT_DEFAULT(v) - Set default value for input of type boolean, int32, uint32, real32, or real64.
 * - \c P_INPUT_DEFAULT_VEC2(), P_INPUT_DEFAULT_VEC3(), P_INPUT_DEFAULT_VEC4() - Set default for real32 or real64 vectors.
 * - \c P_INPUT_DEFAULT_STR(v) - Set default value for input of type string.
 * - \c P_INPUT_REQUIRED - If present, this signals that the input being described is mandatory; the plug-in will not execute if
 * no value is given for this input.
 * - \c P_INPUT_DESC(s) - Set the input's description to the text \c s.
 * - \c P_INPUT_ENUM(d) - Define enumerated symbolic values for this input, which must be of type \c uint32. The parameter \c d is a string,
 * containing value:name pairs and separated by a vertical bar. For instance: \c "0:False|1:True" would create two such symbols, one
 * named "False" that represents the value zero, and one named "True" representing one.
*/
PURPLEAPI void p_init_input(int index		/** Index of the input to add. Must begin at zero and monotonically increase. */,
			    PValueType type	/** Preferred type of input. This information is published in the XML description. */,
			    const char *name	/** Name of the input. */,
			    ...)
{
	va_list	args;

	va_start(args, name);
	plugin_set_input(init_info.plugin, index, type, name, args);
	va_end(args);
}

/**
 * Register persistent per-instance state for a plug-in. Every instance of the plug-in will have the state
 * automatically allocated by the Purple engine, and passed in as an argument to the \c compute() callback.
 * 
 * The Purple engine does not touch the buffer in any way, its contents are totally up to the plug-in to
 * read and write.
 * 
 * The state buffer will be automatically deallocated once the plug-in instance is destroyed, of course.
*/
PURPLEAPI void p_init_state(size_t size				/** The number of bytes of state required. Typically computed by \c sizeof on a plug-in internal structure. */,
			    void (*constructor)(void *state)	/** Optional function to initialize the state buffer, before it's first seen by the \c compute() callback. */,
			    void (*destructor)(void *state)	/** Optional function to destroy the state buffer, before it's deallocated. */)
{
	plugin_set_state(init_info.plugin, size, constructor, destructor);
}

/**
 * This function registers the main computational callback function. This is the function that the Purple
 * engine will call when it needs the plug-in to re-compute its output.
*/
PURPLEAPI void p_init_compute(PComputeStatus (*compute)(PPInput *input, PPOutput output, void *state) /** The callback function pointer. */)
{
	plugin_set_compute(init_info.plugin, compute);
}

/** @} */
