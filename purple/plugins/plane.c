/*
 * This plug-in creates a two-node plane, using an input to set side length.
*/

#include "purple.h"

/* This gets called whenever the input, the size, changes. Create a cube with the given side length. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	enum { MY_OBJECT, MY_GEOMETRY };			/* Node labels. */
	PONode		*obj, *geo;
	real32		size = p_input_real32(input[0]), xp, yp;
	uint32		splits = p_input_uint32(input[1]), x, y, i, n;
	PNGLayer	*lay;

	obj = p_output_node_create(output, V_NT_OBJECT, MY_OBJECT);
	geo = p_output_node_create(output, V_NT_GEOMETRY, MY_GEOMETRY);

	p_node_o_link_set(obj, geo, "geometry", 0);
	/* (Re)Create the polygons. Vertex references will dangle (for a while), but that's OK. */
	lay = p_node_g_layer_find(geo, "polygon");
	for(y = 0, i = 0; y < splits; y++)
	{
		for(x = 0; x < splits; x++, i++)
		{
			n = y * (splits + 1) + x;
			p_node_g_polygon_set_corner_uint32(lay, i, n, n + 1, n + (splits + 1) + 1, n + (splits + 1));
		}
	}
	/* Create the vertices. */
	lay = p_node_g_layer_find(geo, "vertex");
	for(y = i = 0; y <= splits; y++)
	{
		yp = (splits - y) * (size / splits);
		for(x = 0; x <= splits; x++, i++)
		{
			xp = -0.5 * size + (x * size) / splits;
			p_node_g_vertex_set_xyz(lay, i, xp, yp, 0.0);
		}
	}
	printf("plane created with %u vertices, %u polygons\n", i, splits * splits);

	return P_COMPUTE_DONE;	/* Sleep until size changes. */
}

void init(void)
{
	p_init_create("plane");
	p_init_input(0, P_VALUE_REAL32, "size",   P_INPUT_REQUIRED, P_INPUT_DEFAULT(10.0), P_INPUT_DONE);
	p_init_input(1, P_VALUE_UINT32, "splits", P_INPUT_DEFAULT(1), P_INPUT_DEFAULT(1), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
