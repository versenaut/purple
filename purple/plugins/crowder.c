/*
 * A simple Purple plug-in, that will "crowd" an object with clones of it, in a surrounding pattern.
*/

#include <stdio.h>

#include "purple.h"

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode	*obj, *geo;
	PONode	*clone, *outgeo;
	uint32	w = p_input_uint32(input[1]), h = p_input_uint32(input[2]), x = p_input_uint32(input[3]), y = p_input_uint32(input[4]);
	uint32	label, gx, gy;
	real64	pos[3], origin[3];

	if(x >= w)
		x = w - 1;
	if(y >= h)
		y = h - 1;

	if(w * h > 200)
		return P_COMPUTE_DONE;

	if((obj = p_input_node_first_type(input[0], V_NT_OBJECT)) == NULL)
	{
		printf("crowder didn't find an input object node\n");
		return P_COMPUTE_DONE;
	}
	if((geo = p_node_o_link_get(obj, "geometry", NULL)) == NULL || p_node_get_type(geo) != V_NT_GEOMETRY)
	{
		printf("crowder had problems following geometry link from object \"%s\"\n", p_node_get_name(obj));
		return P_COMPUTE_DONE;
	}

	p_node_o_pos_get(obj, origin);
/*	outgeo = p_output_node_copy(output, geo, 0);*/

	printf("crowder generating %ux%u crowd of %u, origin at (%g,%g,%g)\n", w, h, w * h - 1, origin[0], origin[1], origin[2]);
	for(gy = 0, label = 0; gy < h; gy++)
	{
		for(gx = 0; gx < w; gx++)
		{
			if(gy == y && gx == x)
				continue;
			printf(" creating object %u\n", label);
			clone = p_output_node_copy(output, obj, label++);
			pos[0] = origin[0] + (int) (gx - x) * 2.0;
			pos[1] = origin[1];
			pos[2] = origin[2] + (int) (gy - y) * 2.0;
			p_node_o_pos_set(clone, pos);
			p_node_o_link_set(clone, geo, "geometry", 0u);
		}
	}
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("crowder");
	p_init_input(0, P_VALUE_MODULE, "object", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_UINT32, "width",  P_INPUT_REQUIRED, P_INPUT_DEFAULT(3), P_INPUT_DONE);
	p_init_input(2, P_VALUE_UINT32, "height", P_INPUT_REQUIRED, P_INPUT_DEFAULT(3), P_INPUT_DONE);
	p_init_input(3, P_VALUE_UINT32, "x",      P_INPUT_REQUIRED, P_INPUT_DEFAULT(1), P_INPUT_DONE);
	p_init_input(4, P_VALUE_UINT32, "y",      P_INPUT_REQUIRED, P_INPUT_DEFAULT(1), P_INPUT_DONE);
	p_init_compute(compute);
}
