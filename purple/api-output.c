/*
 * The Output API, used by plug-in code to emit resulting values. Simple wrapper for
 * graph output calls.
*/

#include <stdarg.h>

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

void p_output_boolean(PPOutput out, boolean v)
{
	graph_port_output_set(out, P_VALUE_BOOLEAN, v);
}

void p_output_int32(PPOutput out, int32 v)
{
	graph_port_output_set(out, P_VALUE_INT32, v);
}

void p_output_uint32(PPOutput out, uint32 v)
{
	graph_port_output_set(out, P_VALUE_UINT32, v);
}

void p_output_real32(PPOutput out, real32 v)
{
	graph_port_output_set(out, P_VALUE_REAL32, v);
}

void p_output_real32_vec2(PPOutput out, const real32 *v)
{
	graph_port_output_set(out, P_VALUE_REAL32_VEC2, v);
}

void p_output_real32_vec3(PPOutput out, const real32 *v)
{
	graph_port_output_set(out, P_VALUE_REAL32_VEC3, v);
}

void p_output_real32_vec4(PPOutput out, const real32 *v)
{
	graph_port_output_set(out, P_VALUE_REAL32_VEC4, v);
}

void p_output_real32_mat16(PPOutput out, const real32 *v)
{
	graph_port_output_set(out, P_VALUE_REAL32_MAT16, v);
}

void p_output_real64(PPOutput out, real64 v)
{
	graph_port_output_set(out, P_VALUE_REAL64, v);
}

void p_output_real64_vec2(PPOutput out, const real64 *v)
{
	graph_port_output_set(out, P_VALUE_REAL64_VEC2, v);
}

void p_output_real64_vec3(PPOutput out, const real64 *v)
{
	graph_port_output_set(out, P_VALUE_REAL64_VEC3, v);
}

void p_output_real64_vec4(PPOutput out, const real64 *v)
{
	graph_port_output_set(out, P_VALUE_REAL64_VEC4, v);
}

void p_output_real64_mat16(PPOutput out, const real64 *v)
{
	graph_port_output_set(out, P_VALUE_REAL64_MAT16, v);
}

void p_output_string(PPOutput out, const char *v)
{
	graph_port_output_set(out, P_VALUE_STRING, v);
}

PONode * p_output_node(PPOutput out, PINode *v)
{
	Node	*n;

	if((n = nodedb_new_copy((Node *) v)) != NULL)
		graph_port_output_set_node(out, n);
	return NULL;
}

PONode * p_output_node_create(PPOutput out, VNodeType type, const char *name)
{
	Node	*n;

	if((n = nodedb_new(type)) != NULL)
	{
		nodedb_rename(n, name);
		graph_port_output_set_node(out, n);
	}
	return n;
}
