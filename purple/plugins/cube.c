/*
 * This plug-in creates a two-node box, using an input to set side length
*/

#include "purple.h"
#include "purple-plugin.h"

/* This gets called whenever the input, the size, changes. Create a cube with the given side length. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	enum { MY_OBJECT, MY_GEOMETRY };			/* Node labels. */
	PONode		*obj, *geo;
	real32		size = p_input_real32(input[0]);	/* Read out the size. */;
	int		i;
	PNGLayer	*lay;
	/* Data used to describe corners of a unit cube. Scaled with size before set. */
	static const int8	corner[][3] = {
		{ -1,  1, -1 }, { 1,  1, -1 }, { 1,  1, 1 }, { -1,  1, 1 },
		{ -1, -1, -1 }, { 1, -1, -1 }, { 1, -1, 1 }, { -1, -1, 1 }
	};

	obj = p_output_node_create(output, V_NT_OBJECT, MY_OBJECT);
	geo = p_output_node_create(output, V_NT_GEOMETRY, MY_GEOMETRY);

	p_node_o_link_set(obj, geo, "geometry", 0);
	/* (Re)Create the polygons. Vertex references will dangle (for a while), but that's OK. */
	lay = p_node_g_layer_lookup(geo, "polygon");
	p_node_g_polygon_set_corner_uint32(geo, lay, 0,  0, 1, 2, 3);
	p_node_g_polygon_set_corner_uint32(geo, lay, 1,  3, 2, 6, 7);
	p_node_g_polygon_set_corner_uint32(geo, lay, 2,  7, 6, 5, 4);
	p_node_g_polygon_set_corner_uint32(geo, lay, 3,  1, 0, 4, 5);
	p_node_g_polygon_set_corner_uint32(geo, lay, 4,  0, 3, 7, 4);
	p_node_g_polygon_set_corner_uint32(geo, lay, 5,  2, 1, 5, 6);

	lay = p_node_g_layer_lookup(geo, "vertex");
	for(i = 0; i < 8; i++)	/* Loop and set eight scaled corners. */
		p_node_g_vertex_set_xyz(geo, lay, i, size * corner[i][0], size * corner[i][1], size * corner[i][2]);

	return P_COMPUTE_DONE;	/* Sleep until size changes. */
}

void init(void)
{
	p_init_create("cube");
	p_init_input(0, P_VALUE_REAL32, "size", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
