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
	{
		printf("outputting node at %p, type %d\n", n, ((Node *) n)->type);
		graph_port_output_set_node(out, n);
	}
	return n;
}

PONode * p_output_node_o_link(PPOutput out, PONode *node, const char *label)
{
	Node	*n;

	if((n = nodedb_o_link_get_local((NodeObject *) node, label, 0)) != NULL)
	{
		n = nodedb_new_copy((Node *) n);
		nodedb_o_link_set_local((NodeObject *) node, n, label, 0);
		graph_port_output_set_node(out, n);
	}
	else
		printf("api-output: couldn't get %s local link from %p\n", label, node);
	return n;
}

PONode * p_output_node_create(PPOutput out, VNodeType type, uint32 label)
{
	PONode	*n = graph_port_output_node_create(out, type, label);

	/* FIXME: This clearing goes against the caching idea that is the reason for the
	 * labelled storage in the first place. This conflict might need to be resolved...
	*/
	if(type == V_NT_GEOMETRY)
	{
		NdbGLayer	*l;

		if((l = nodedb_g_layer_find((NodeGeometry *) n, "vertex")) != NULL)
		{
			real64	gone[] = { V_REAL64_MAX, V_REAL64_MAX, V_REAL64_MAX };
			uint32	i, s;

			s = dynarr_size(l->data);
			for(i = 0; i < s; i++)
				dynarr_set(l->data, i, gone);
			printf("cleared %u vertex slots\n", i);
		}
		if((l = nodedb_g_layer_find((NodeGeometry *) n, "polygon")) != NULL)
		{
			uint32	gone[] = { ~0u, ~0u, ~0u, ~0u }, i, s;

			s = dynarr_size(l->data);
			for(i = 0; i < s; i++)
				dynarr_set(l->data, i, gone);
			printf("cleared %u polygon slots\n", i);
		}
	}
	else if(type == V_NT_MATERIAL)
	{
		dynarr_clear(((NodeMaterial *) n)->fragments);
	}
	return n;
}

PONode * p_output_node_copy(PPOutput out, PINode *node, uint32 label)
{
	return graph_port_output_node_copy(out, node, label);
}

PONode * p_output_node_pass(PPOutput out, PINode *node)
{
	if(out != NULL && node != NULL)
	{
		graph_port_output_set_node(out, (PONode *) node);
		return (PONode *) node;
	}
	return NULL;
}
