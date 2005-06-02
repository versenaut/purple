/*
 * api-output.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * The Output API, used by plug-in code to emit resulting values. Simple wrapper for
 * graph output calls.
*/

#include <stdarg.h>
#include <stdio.h>

#define PURPLE_INTERNAL

#include "purple.h"

#include "dynarr.h"
#include "dynstr.h"
#include "list.h"
#include "value.h"
#include "nodeset.h"
#include "textbuf.h"

#include "nodedb.h"
#include "graph.h"
#include "port.h"

/* ----------------------------------------------------------------------------------------- */

/** \defgroup api_output Output Functions
 * 
 * These are functions for setting a plug-in's output to a value.  This is the only way for a
 * plug-in to output its result, and make it available for use by other plug-ins. The Purple
 * engine tracks outputs, and notices when they change. Such a change can be used to infer that
 * any plug-in instance that has an input connected to the changing output needs to be scheduled
 * for re-execution.
 * 
 * Purple plug-in outputs can hold more than just a single value, though. The exact definition
 * is something like this:
 *
 * The output can hold, at the same time:
 * - One of each of the "simple" value types (boolean, integer, float, vector, matrix etc)
 * - Any number of nodes.
 * 
 * Basically, you can think of the output as a structure with one field for each of the basic
 * types, plus a linked list for the nodes. This is not exactly how it is implemented under the
 * hood, but not too far from it either. This means that if you do this:
 * \code
 * p_output_int32(output, 17);
 * p_output_int32(output, 4711);
 * \endcode
 * The \c int32 value on the output will be \c 4711, the \c 17 will be overwritten. If another
 * plug-in has its input connected to this output, and it runs \c p_input_int32(), it will read
 * the value \c 4711.
 * 
 * For the "simple" types that are still passed by reference, such as strings, vectors and
 * matrices, Purple will copy the data before the output call returns. So there is no need
 * to retain the pointer in the plug-in; passing a pointer to an on-stack (automatic) variable
 * is perfectly fine.
 * @{
*/

/** \brief Output a boolean.
 * 
 * This function sets a plug-in's output to a boolean value, i.e. 0 or 1.
*/
PURPLEAPI void p_output_boolean(PPOutput out	/** The output to set. */,
				boolean v	/** The value to set the output to. */)
{
	graph_port_output_set(out, P_VALUE_BOOLEAN, v);
}

/** \brief Output a 32-bit signed integer.
 * 
 * This function sets a plug-in's output to a 32-bit signed integer value.
*/
PURPLEAPI void p_output_int32(PPOutput out	/** The output to be set. */,
			      int32 v		/** The value to set the output to. */)
{
	graph_port_output_set(out, P_VALUE_INT32, v);
}

/** \brief Output a 32-bit unsigned integer.
 * 
 * This function sets a plug-in's output to a 32-bit unsigned integer value.
*/
PURPLEAPI void p_output_uint32(PPOutput out	/** The output to be set. */,
			       uint32 v		/** The value to set the output to. */)
{
	graph_port_output_set(out, P_VALUE_UINT32, v);
}

/** \brief Output a 32-bit floating point number.
 * 
 * This function sets a plug-in's output to a 32-bit floating point value.
*/
PURPLEAPI void p_output_real32(PPOutput out	/** The output to be set. */,
			       real32 v		/** The value to set the output to. */)
{
	graph_port_output_set(out, P_VALUE_REAL32, v);
}

/** \brief Output a 2D vector of 32-bit floating point numbers.
 * 
 * This function sets a plug-in's output to a 2D vector of 32-bit floating point values.
*/
PURPLEAPI void p_output_real32_vec2(PPOutput out	/** The output to be set. */,
				    const real32 *v	/** Pointer to two numbers making up the vector to set the output to. */)
{
	graph_port_output_set(out, P_VALUE_REAL32_VEC2, v);
}

/** \brief Output a 3D vector of 32-bit floating point numbers.
 * 
 * This function sets a plug-in's output to a 3D vector of 32-bit floating point values.
*/
PURPLEAPI void p_output_real32_vec3(PPOutput out	/** The output to be set. */,
				    const real32 *v	/** Pointer to three numbers making up the vector to set the output to. */)
{
	graph_port_output_set(out, P_VALUE_REAL32_VEC3, v);
}

/** \brief Output a 4D vector of 32-bit floating point numbers.
 * 
 * This function sets a plug-in's output to a 4D vector of 32-bit floating point values.
*/
PURPLEAPI void p_output_real32_vec4(PPOutput out	/** The output to be set. */,
				    const real32 *v	/** Pointer to four numbers making up the vector to set the output to. */)
{
	graph_port_output_set(out, P_VALUE_REAL32_VEC4, v);
}

/** \brief Output a 4x4 matrix of 32-bit floating point numbers.
 * 
 * This function sets a plug-in's output to a 4D matrix of 32-bit floating point values.
*/
PURPLEAPI void p_output_real32_mat16(PPOutput out	/** The output to be set. */,
				     const real32 *v	/** Pointer to 16 numbers making up the matrix to set the output to. */)
{
	graph_port_output_set(out, P_VALUE_REAL32_MAT16, v);
}

/** \brief Output a 64-bit floating point number.
 * 
 * This function sets a plug-in's output to a 64-bit floating point value.
*/
PURPLEAPI void p_output_real64(PPOutput out	/** The output to be set. */,
			       real64 v		/** The value to set the output to. */)
{
	graph_port_output_set(out, P_VALUE_REAL64, v);
}

/** \brief Output a 2D vector of 64-bit floating point numbers.
 * 
 * This function sets a plug-in's output to a 2D vector of 64-bit floating point values.
*/
PURPLEAPI void p_output_real64_vec2(PPOutput out	/** The output to be set. */,
				    const real64 *v	/** Pointer to two numbers making up the vector to set the output to. */)
{
	graph_port_output_set(out, P_VALUE_REAL64_VEC2, v);
}

/** \brief Output a 3D vector of 64-bit floating point numbers.
 * 
 * This function sets a plug-in's output to a 3D vector of 64-bit floating point values.
*/
PURPLEAPI void p_output_real64_vec3(PPOutput out	/** The output to be set. */,
				    const real64 *v	/** Pointer to three numbers making up the vector to set the output to. */)
{
	graph_port_output_set(out, P_VALUE_REAL64_VEC3, v);
}

/** \brief Output a 4D vector of 64-bit floating point numbers.
 * 
 * This function sets a plug-in's output to a 4D vector of 64-bit floating point values.
*/
PURPLEAPI void p_output_real64_vec4(PPOutput out	/** The output to be set. */,
				    const real64 *v	/** Pointer to four numbers making up the vector to set the output to. */)
{
	graph_port_output_set(out, P_VALUE_REAL64_VEC4, v);
}

/** \brief Output a 4x4 matrix of 64-bit floating point numbers.
 * 
 * This function sets a plug-in's output to a 4D matrix of 64-bit floating point values.
*/
PURPLEAPI void p_output_real64_mat16(PPOutput out	/** The output to be set. */,
				     const real64 *v	/** Pointer to 16 numbers making up the matrix to set the output to. */)
{
	graph_port_output_set(out, P_VALUE_REAL64_MAT16, v);
}

/** \brief Output a string.
*/
PURPLEAPI void p_output_string(PPOutput out	/** The output to be set. */,
			       const char *v	/** The string to set the output to. */)
{
	graph_port_output_set(out, P_VALUE_STRING, v);
}

/** \brief Output a node.
 * 
 * This function adds the indicated node to the set of nodes output by a plug-in. This involves
 * internally creating a copy, to make it writable. The copy is also returned by the function,
 * and can be modified. The final state of the node as \c compute() returns, is what is output,
 * not the state of the node at the time of the copying.
*/
PURPLEAPI PONode * p_output_node(PPOutput out, PINode *v)
{
	PNode	*n;

	if((n = nodedb_new_copy((PNode *) v)) != NULL)
	{
		printf("outputting node at %p, type %d\n", n, ((PNode *) n)->type);
		graph_port_output_set_node(out, n);
	}
	return n;
}

PURPLEAPI PONode * p_output_node_o_link(PPOutput out, PONode *node, const char *label)
{
	PNode	*n;

	if((n = nodedb_o_link_get_local((NodeObject *) node, label, 0)) != NULL)
	{
		n = nodedb_new_copy((PNode *) n);
		nodedb_o_link_set_local((NodeObject *) node, n, label, 0);
		graph_port_output_set_node(out, n);
	}
	else
		printf("api-output: couldn't get %s local link from %p\n", label, node);
	return n;
}

/** \brief Create a new node, and output it.
 * 
 * This function creates a new node of the given type, and makes sure it is added to the
 * set of node data emitted by the given output.
 * 
 * The label parameter is used to give the output node an unique identity. This is not
 * a perfect solution; but it is how things work. Basically, a plug-in should give
 * Purple unique integers, monotonically increasing from 0, as values here.
 * 
 * The reason these IDs are needed is that Purple internally needs to be able to buffer
 * the nodes, and re-use them for the next invocation of a plug-in. It cannot assign the
 * numbers itself, since branching in the plug-in code might make calls to the
 * \c p_output_node_create() function occur in different order at different times.
 * 
 * Here's an example of a plug-in that outputs two bitmap nodes (only the \c compute()
 * function is shown):
 * \code
 * #include "purple.h"
 * 
 * static PComputeStatus compute(PPInput *input, PPOutput output, void *user)
 * {
 * 	PONode	*bm0, *bm1;
 * 
 * 	bm0 = p_output_node_create(output, V_NT_BITMAP, 0);	// Use 0 for first node.
 * 	bm1 = p_output_node_create(output, V_NT_BITMAP, 1);	// Increment to 1 for second.
 * 
 * 	return P_COMPUTE_DONE;
 * }
 * \endcode
*/
PURPLEAPI PONode * p_output_node_create(PPOutput out	/** The output to use. */,
					VNodeType type	/** The type of node to be created. */,
					uint32 label	/** A unique small number that identifies this particular output call. */)
{
	PONode	*n = graph_port_output_node_create(out, type, label);

	/* FIXME: This clearing goes against the caching idea that is the reason for the
	 * labelled storage in the first place. This conflict might need to be resolved...
	*/
	if(type == V_NT_GEOMETRY)
	{
		NdbGLayer	*l;
		int		i;

		if((l = nodedb_g_layer_find((NodeGeometry *) n, "vertex")) != NULL)
		{
			real64	gone[] = { V_REAL64_MAX, V_REAL64_MAX, V_REAL64_MAX };
			uint32	i, s;

			s = dynarr_size(l->data);
			for(i = 0; i < s; i++)
				dynarr_set(l->data, i, gone);
/*			printf("cleared %u vertex slots\n", i);*/
		}
		if((l = nodedb_g_layer_find((NodeGeometry *) n, "polygon")) != NULL)
		{
			uint32	gone[] = { ~0u, ~0u, ~0u, ~0u }, i, s;

			s = dynarr_size(l->data);
			for(i = 0; i < s; i++)
				dynarr_set(l->data, i, gone);
/*			printf("cleared %u polygon slots\n", i);*/
		}
		for(i = 2; i < 100; i++)
		{
			l = nodedb_g_layer_nth((NodeGeometry *) n, i);
			if(l != NULL)
				nodedb_g_layer_destroy((NodeGeometry *) n, l);
		}
	}
	else if(type == V_NT_MATERIAL)
	{
		dynarr_clear(((NodeMaterial *) n)->fragments);
	}
	return n;
}

/** \brief Copy an input node to the output.
 * 
 * This function \b copies an input node, and adds the resulting copy to the plug-in's
 * set of output node data.
 * 
 * Using this function is the only way to create a plug-in that acts as a "pass-through"
 * for node data if you want to be modifying it in the process.
 * 
 * \note This function creates an identical copy of the input, that shares no storage.
 * This means that all data in layers/buffers/fragments and so on is duplicated. This
 * is quite an expensive operation.
 * 
 * During the copy, the node's name will have "copy of" prepended. If it is already
 * there, a number will be inserted, and incremented on each subsequent copy (i.e.
 * "copy 2 of foo", "copy 3 of foo", and so on).
 * 
 * The label parameter works just the same as with \c p_output_node_create() above.
 * 
 * Here's a simple example that filters out the first object node in the input,
 * and renames it to "banana":
 * \code
 * #include "purple.h"
 * 
 * static PComputeStatus compute(PPInput *input, PPOutput output, void *user)
 * {
 * 	int	i;
 * 	PINode	*in;
 * 	PONode	*out;
 * 
 * 	for(i = 0; (in = p_input_node_nth(input[0], i)) != NULL; i++)
 * 	{
 * 		if(p_node_get_type(in) == V_NT_OBJECT)
 * 		{
 * 			PONode	*out;
 * 
 * 			out = p_output_node_copy(output, in, 0);
 * 			p_node_set_name(out, "banana");		// Change name of copy.
 * 			break;
 * 		}
 * 	}
 * 	return P_COMPUTE_DONE;
 * }
 * \endcode
*/
PURPLEAPI PONode * p_output_node_copy(PPOutput out	/** The output to use. */,
				      PINode *node	/** The input node that is to be copied. */,
				      uint32 label	/** A unique small number that identifies this particular copy call. */)
{
	return graph_port_output_node_copy(out, node, label);
}

/** @} */
