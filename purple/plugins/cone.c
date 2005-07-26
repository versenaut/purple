/*
 * This plug-in creates a cone, using inputs to set tesselation and height.
*/

#include <math.h>

#include "purple.h"

#define	POLY(l,i,v0,v1,v2,v3)	p_node_g_polygon_set_corner_uint32(l,i,v0,v1,v2,v3)

/* This gets called whenever the input, the size, changes. Create a cube with the given side length. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	enum { MY_OBJECT, MY_GEOMETRY };			/* Node labels. */
	PONode		*obj, *geo;
	real32		height = p_input_real32(input[0]);	/* Read out the size. */
	uint32		bottom_splits = p_input_uint32(input[1]);	/* And the tesselation level. */
	uint32		side_splits = p_input_uint32(input[2]);	/* And the tesselation level. */
	real64		angle, x, y, r;
	uint32		i, j, pos, poly, corner, apex, bottom;
	uint32		v0, v1, v2, v3;
	PNGLayer	*lay;

	obj = p_output_node_create(output, V_NT_OBJECT, MY_OBJECT);
	geo = p_output_node_create(output, V_NT_GEOMETRY, MY_GEOMETRY);

	p_node_o_link_set(obj, geo, "geometry", 0);

	/* Create bottom vertices. */
	lay = p_node_g_layer_find(geo, "vertex");
	for(i = 0; i < bottom_splits; i++)
	{
		angle = 2 * M_PI * (i / (double) bottom_splits);
		p_node_g_vertex_set_xyz(lay, i, cos(angle), 0.0, -sin(angle));
	}

	/* Create any vertices between the bottom and the top, for along-height splits. */
	lay = p_node_g_layer_find(geo, "vertex");
	for(j = 1; j < side_splits; j++)
	{
		y = j * height / side_splits;
		r = 1.0 - ((double) j / side_splits);
		for(i = 0; i < bottom_splits; i++, pos++)
		{
			angle = 2 * M_PI * (i / (double) bottom_splits);
			p_node_g_vertex_set_xyz(lay, j * bottom_splits + i, r * cos(angle), y, r * -sin(angle));
		}
	}
	/* Create apex vertex. */
	apex = side_splits * bottom_splits;
	p_node_g_vertex_set_xyz(lay, apex, 0.0, height, 0.0);
	/* Create bottom center, "anti-apex". It's at apex+1. */
	p_node_g_vertex_set_xyz(lay, apex + 1, 0.0, 0.0, 0.0);

	/* Create bottom surface. */
	lay = p_node_g_layer_find(geo, "polygon");
	for(i = 0; i < bottom_splits; i++)
		POLY(lay, i, i, (i + 1) % bottom_splits, apex + 1, ~0u);

	/* Create polygons (quads) between bottom and apex-touching triangles. */
	for(j = 1; j < side_splits; j++)
	{
		pos = (j - 1) * bottom_splits;
		for(i = 0; i < bottom_splits; i++)
		{
			POLY(lay, j * bottom_splits + i,
			     pos + i + bottom_splits,
			     pos + (i + 1) % bottom_splits + bottom_splits,
			     pos + (i + 1) % bottom_splits,
			     pos + i);
		}
	}

	/* Create final row of triangles touching the apex. */
	pos = apex - bottom_splits;
	for(i = 0; i < bottom_splits; i++)
		POLY(lay, bottom_splits + side_splits * bottom_splits + i, pos + (i + 1) % bottom_splits, pos + i, apex, ~0u);

	/* Set creases, cone should be sharp. */
	p_node_g_crease_set_vertex(geo, NULL, ~0u);
	p_node_g_crease_set_edge(geo, NULL, ~0u);

	return P_COMPUTE_DONE;	/* Sleep until size changes. */
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("cone");
	p_init_input(0, P_VALUE_REAL32, "height",        P_INPUT_REQUIRED, P_INPUT_MIN(0.1), P_INPUT_MAX(200.0), P_INPUT_DEFAULT(10.0), P_INPUT_DONE);
	p_init_input(1, P_VALUE_UINT32, "bottom splits", P_INPUT_REQUIRED, P_INPUT_MIN(1),   P_INPUT_MAX(128),   P_INPUT_DEFAULT(8), P_INPUT_DONE);
	p_init_input(2, P_VALUE_UINT32, "side splits",   P_INPUT_REQUIRED, P_INPUT_MIN(1),   P_INPUT_MAX(128),   P_INPUT_DEFAULT(1), P_INPUT_DONE);
	p_init_compute(compute);
}
