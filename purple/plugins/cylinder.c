/*
 * This plug-in creates a cylinder, using inputs to set tesselation and height.
*/

#define _USE_MATH_DEFINES
#include <math.h>

#include "purple.h"

#define	POLY(lab,l,i,v0,v1,v2,v3)	do { /*printf(lab " polygon %u: %u-%u-%u-%u\n", i, v0, v1, v2, v3);*/ p_node_g_polygon_set_corner_uint32(l,i,v0,v1,v2,v3); } while(0)

/* This gets called whenever the input, the size, changes. Create a cube with the given side length. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	enum { MY_OBJECT, MY_GEOMETRY };			/* Node labels. */
	PONode		*obj, *geo;
	real32		height = p_input_real32(input[0]);	/* Read out the size. */
	uint32		end_splits = p_input_uint32(input[1]);	/* And the top/bottom tesselation level. */
	uint32		side_splits = p_input_uint32(input[2]);	/* And the side tesselation level. */
	real64		angle, x, y;
	uint32		i, j, pos, poly, corner, bc, tc;
	uint32		v0, v1, v2, v3;
	PNGLayer	*lay;

	obj = p_output_node_create(output, V_NT_OBJECT, MY_OBJECT);
	p_node_set_name(obj, "cylinder");
	geo = p_output_node_create(output, V_NT_GEOMETRY, MY_GEOMETRY);

	p_node_o_link_set(obj, geo, "geometry", 0);

	/* Create outer vertices. */
	lay = p_node_g_layer_find(geo, "vertex");
	for(j = 0; j <= side_splits; j++)
	{
		y = j * height / side_splits;
		for(i = 0; i < end_splits; i++, pos++)
		{
			angle = 2 * M_PI * (i / (double) end_splits);
			p_node_g_vertex_set_xyz(lay, j * end_splits + i, cos(angle), y, -sin(angle));
		}
	}
	/* Create bottom center vertex. */
	bc = (side_splits + 1) * end_splits;
	p_node_g_vertex_set_xyz(lay, bc, 0.0, 0.0, 0.0);
	/* Create top center. It's at bc+1. */
	tc = bc + 1;
	p_node_g_vertex_set_xyz(lay, tc, 0.0, height, 0.0);
	/* Create bottom surface. */
	lay = p_node_g_layer_find(geo, "polygon");
	for(i = 0; i < end_splits; i++)
		POLY("bottom", lay, i, i, (i + 1) % end_splits, bc, ~0u);
	/* Create side polygons (quads). */
	for(j = 0; j < side_splits; j++)
	{
		pos = j * end_splits;
		for(i = 0; i < end_splits; i++)
		{
			POLY("side", lay, end_splits + j * end_splits + i,
			     pos + i + end_splits,
			     pos + (i + 1) % end_splits + end_splits,
			     pos + (i + 1) % end_splits,
			     pos + i);
		}
	}
	/* Create top surface. */
	pos = tc - 1 - end_splits;
	for(i = 0; i < end_splits; i++)
		POLY("top", lay, end_splits + side_splits * end_splits + i, pos + (i + 1) % end_splits, pos + i, tc, ~0u);

	p_node_g_crease_set_vertex(geo, NULL, ~0u);
	p_node_g_crease_set_edge(geo, NULL, ~0u);

	return P_COMPUTE_DONE;	/* Sleep until size changes. */
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("cylinder");
	p_init_input(0, P_VALUE_REAL32, "height",      P_INPUT_REQUIRED, P_INPUT_MIN(0.1), P_INPUT_MAX(200.0), P_INPUT_DEFAULT(10.0),
		     P_INPUT_DESC("The height of the cylinder; distance from base to top surface."), P_INPUT_DONE);
	p_init_input(1, P_VALUE_UINT32, "end splits",  P_INPUT_REQUIRED, P_INPUT_MIN(1),   P_INPUT_MAX(128),   P_INPUT_DEFAULT(8),
		     P_INPUT_DESC("The number of splits of the bottom and top ends. Controls the roundness of the base mesh."), P_INPUT_DONE);
	p_init_input(2, P_VALUE_UINT32, "side splits", P_INPUT_REQUIRED, P_INPUT_MIN(1),   P_INPUT_MAX(128),   P_INPUT_DEFAULT(1),
		     P_INPUT_DESC("The number of splits along the cylinder's axis."), P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("desc/purpose", "Creates a cylinder object with matching geometry.");
	p_init_compute(compute);
}
