/*
 * This plug-in creates a cone, using inputs to set tesselation and height.
*/

#include <math.h>

#include "purple.h"

/* This gets called whenever the input, the size, changes. Create a cube with the given side length. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	enum { MY_OBJECT, MY_GEOMETRY };			/* Node labels. */
	PONode		*obj, *geo;
	real32		size = p_input_real32(input[0]);	/* Read out the size. */
	uint32		splits = p_input_uint32(input[1]);	/* And the tesselation level. */
	uint32		bottom_splits = p_input_uint32(input[2]);	/* And the tesselation level. */
	uint32		side_splits = p_input_uint32(input[3]);	/* And the tesselation level. */
	real64		x, y, f;
	uint32		i, j, poly, corner;
	uint32		v0, v1, v2, v3;
	PNGLayer	*lay;

	obj = p_output_node_create(output, V_NT_OBJECT, MY_OBJECT);
	geo = p_output_node_create(output, V_NT_GEOMETRY, MY_GEOMETRY);

	p_node_o_link_set(obj, geo, "geometry", 0);

	/* Create topside polygons. Vertex references will dangle (for a while), but that's OK. */
	lay = p_node_g_layer_find(geo, "polygon");
	corner = splits * (bottom_splits + side_splits);
	poly = 0;

/*	for(i = 0; i < splits; i++)
		p_node_g_polygon_set_corner_uint32(lay, poly++, corner, i , (i + 1) % splits, -1);
*/
	for(i = 0; i < splits * (bottom_splits + side_splits); i++)
		p_node_g_polygon_set_corner_uint32(lay, poly++, (i + 1) % splits, i, i + splits, (i + 1) % splits + splits);

/*	for(i = 0; i < splits; i++)
		p_node_g_polygon_set_corner_uint32(lay, poly++, corner + 1, (i + 1) % splits, i, -1);
*/
	lay = p_node_g_layer_find(geo, "vertex");

	for(i = 0; i < splits; i++)
	{
		x = sin((double)i / (double)splits * 2 * M_PI);
		y = cos((double)i / (double)splits * 2 * M_PI);
		for(j = 0; j < bottom_splits + 1; j++)
		{
			f = (real64) (j + 1) / (real64)(bottom_splits + 1);
			p_node_g_vertex_set_xyz(lay, i + j * splits,  x * f, 0, y * f);
		}
		for(j = 0; j < side_splits; j++)
		{
			f = 1.0 - ((real64)j / (real64)(side_splits + 1));
			p_node_g_vertex_set_xyz(lay, i + (bottom_splits + 1 + j) * splits, x * f, 1 - f, y * f);
		}
	}
	p_node_g_vertex_set_xyz(lay, corner, 0, 0, 0);
	p_node_g_vertex_set_xyz(lay, corner + 1, 0, 1, 0);
	return P_COMPUTE_DONE;	/* Sleep until size changes. */
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("cone");
	p_init_input(0, P_VALUE_REAL32, "size", P_INPUT_REQUIRED, P_INPUT_MIN(0.1), P_INPUT_MAX(200.0), P_INPUT_DEFAULT(10.0), P_INPUT_DONE);
	p_init_input(1, P_VALUE_UINT32, "splits", P_INPUT_REQUIRED, P_INPUT_MIN(1), P_INPUT_MAX(100), P_INPUT_DEFAULT(1), P_INPUT_DONE);
	p_init_input(2, P_VALUE_UINT32, "bottom splits", P_INPUT_REQUIRED, P_INPUT_MIN(1), P_INPUT_MAX(100), P_INPUT_DEFAULT(1), P_INPUT_DONE);
	p_init_input(3, P_VALUE_UINT32, "side splits", P_INPUT_REQUIRED, P_INPUT_MIN(1), P_INPUT_MAX(100), P_INPUT_DEFAULT(1), P_INPUT_DONE);
	p_init_compute(compute);
}
