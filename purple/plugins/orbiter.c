/*
 * A simple Purple-plug-in, that will make an object "orbit" around a point. The orbit consists of N copies
 * of the input object. The speed and radius (in both X and Z dimensions) are settable, too.
*/

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>

#include "purple.h"

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	const real64	*origin = p_input_real64_vec3(input[6]);
	real64		t = p_input_real64(input[0]), angle, delta, rx, rz, pos[3];
	int		i, n = p_input_uint32(input[4]);
	PINode		*obj, *geo, *audio;
	PONode		*clone, *clonegeo = NULL, *cloneaudio = NULL;
	uint32		label = 0;

	if((obj = p_input_node_first_type(input[5], V_NT_OBJECT)) == NULL)
	{
		printf("orbiter didn't find an input object node\n");
		return P_COMPUTE_DONE;
	}
	if((geo = p_node_o_link_get(obj, "geometry", NULL)) != NULL && p_node_get_type(geo) != V_NT_GEOMETRY)
	{
		printf("orbiter had problems following geometry link from object \"%s\"\n", p_node_get_name(obj));
		return P_COMPUTE_DONE;
	}
	if((audio = p_node_o_link_get(obj, "audio", NULL)) != NULL && p_node_get_type(audio) != V_NT_AUDIO)
	{
		printf("orbiter found non-audio node at end of \"audio\" link, aborting\n");
		return P_COMPUTE_DONE;
	}

	if(geo != NULL)
		clonegeo = p_output_node_copy(output, geo, label++);
	if(audio != NULL)
		cloneaudio = p_output_node_copy(output, audio, label++);

	angle = t * p_input_real64(input[1]) * 2 * M_PI;
	delta = (2.0 * M_PI) / n;
	rx = p_input_real64(input[2]);
	rz = p_input_real64(input[3]);
	for(i = 0; i < n; i++, angle += delta)
	{
		pos[0] = origin[0] + rx * cos(angle);
		pos[1] = origin[1];
		pos[2] = origin[2] + rz * sin(angle);
		clone = p_output_node_copy(output, obj, label++);
		if(clonegeo != NULL)
			p_node_o_link_set(clone, clonegeo, "geometry", 0u);
		if(cloneaudio != NULL)
			p_node_o_link_set(clone, cloneaudio, "audio", 0u);
		p_node_o_pos_set(clone, pos);
	}
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("orbiter");
	p_init_input(0, P_VALUE_REAL64, "time",		P_INPUT_DEFAULT(0.0f), P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL32, "speed",	P_INPUT_DEFAULT(1.0f), P_INPUT_DESC("Angular velocity, in degrees per second."), P_INPUT_DONE);
	p_init_input(2, P_VALUE_REAL32, "radius-x",	P_INPUT_DEFAULT(1.0), P_INPUT_DESC("X-radius for the orbit."), P_INPUT_DONE);
	p_init_input(3, P_VALUE_REAL32, "radius-z",	P_INPUT_DEFAULT(1.0), P_INPUT_DESC("Z-radius for the orbit."), P_INPUT_DONE);
	p_init_input(4, P_VALUE_UINT32, "n",		P_INPUT_DEFAULT(1), P_INPUT_MIN(1), P_INPUT_MAX(16), P_INPUT_DESC("Number of clones to orbit."), P_INPUT_DONE);
	p_init_input(5, P_VALUE_MODULE, "object",	P_INPUT_REQUIRED, P_INPUT_DESC("First object found here will be cloned and made to orbit."), P_INPUT_DONE);
	p_init_input(6, P_VALUE_REAL32_VEC3, "origin",	P_INPUT_REQUIRED, P_INPUT_DEFAULT_VEC3(0.0, 0.0, 0.0), P_INPUT_DONE);
	p_init_compute(compute);
}
