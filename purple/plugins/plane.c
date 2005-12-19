/*
 * This plug-in creates a two-node plane, using an input to set side length.
*/

#include <stdio.h>

#include "purple.h"

/* This gets called whenever the input, the size, changes. Create a cube with the given side length. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	enum { MY_OBJECT, MY_GEOMETRY };			/* Node labels. */
	PONode		*obj, *geo;
	real32		size = p_input_real32(input[0]), xp, yp;
	uint32		splits = p_input_uint32(input[1]), x, y, i, n;
	boolean		uv_map = p_input_boolean(input[2]);
	PNGLayer	*ulay, *vlay;
	PNGLayer	*lay;

	obj = p_output_node_create(output, V_NT_OBJECT, MY_OBJECT);
	p_node_set_name(obj, "plane");
	geo = p_output_node_create(output, V_NT_GEOMETRY, MY_GEOMETRY);
	p_node_set_name(geo, "geo-plane");

	p_node_o_link_set(obj, geo, "geometry", 0);

	if(uv_map)
	{
		ulay = p_node_g_layer_create(geo, "map_u", VN_G_LAYER_POLYGON_CORNER_REAL, 0, 0.0);
		vlay = p_node_g_layer_create(geo, "map_v", VN_G_LAYER_POLYGON_CORNER_REAL, 0, 0.0);
	}
	else
		ulay = vlay = NULL;

	/* (Re)Create the polygons. Vertex references will dangle (for a while), but that's OK. */
	lay = p_node_g_layer_find(geo, "polygon");
	for(y = 0, i = 0; y < splits; y++)
	{
		for(x = 0; x < splits; x++, i++)
		{
			n = y * (splits + 1) + x;
			p_node_g_polygon_set_corner_uint32(lay, i,
							   n,
							   n + 1,
							   n + (splits + 1) + 1,
							   n + (splits + 1));
			if(uv_map)
			{
				p_node_g_polygon_set_corner_real64(ulay, i, (real64) x / splits, (real64) (x + 1) / splits, (real64) (x + 1) / splits, (real64) x / splits);
				p_node_g_polygon_set_corner_real64(vlay, i,
								   1.0 - y / (real64) splits,
								   1.0 - y / (real64) splits,
								   1.0 - (y + 1) / (real64) splits,
								   1.0 - (y + 1) / (real64) splits);
			}
		}
	}
	/* Create the vertices. */
	lay = p_node_g_layer_find(geo, "vertex");
	for(y = i = 0; y <= splits; y++)
	{
		yp = -0.5f * size + y * (size / splits);
		for(x = 0; x <= splits; x++, i++)
		{
			xp = -0.5 * size + (x * size) / splits;
			p_node_g_vertex_set_xyz(lay, i, xp, 0.0, yp);
		}
	}
	printf("plane created with %u vertices, %u polygons\n", i, splits * splits);
	return P_COMPUTE_DONE;	/* Sleep until size changes. */
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("plane");
	p_init_input(0, P_VALUE_REAL32, "size",   P_INPUT_REQUIRED, P_INPUT_DEFAULT(10.0),
		     P_INPUT_DESC("Side length of plane."), P_INPUT_DONE);
	p_init_input(1, P_VALUE_UINT32, "splits", P_INPUT_DEFAULT(1), P_INPUT_DEFAULT(1), P_INPUT_REQUIRED,
		     P_INPUT_DESC("Number of times to split each side."), P_INPUT_DONE);
	p_init_input(2, P_VALUE_BOOLEAN, "uv-map", P_INPUT_DEFAULT(0),
		     P_INPUT_DESC("Controls whether or not UV mapping data is created."), P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("copyright", "2005 PDC, KTH");
	p_init_meta("desc/purpose", "Create a simple polygonal plane, consisting of many quadrilaterals.");
	p_init_compute(compute);
}
