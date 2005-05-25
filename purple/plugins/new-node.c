/*
 * A bunch of Purple plug-ins that create new nodes.
*/

#include "purple.h"

/* ----------------------------------------------------------------------------------------- */

static PComputeStatus compute_audio(PPInput *input, PPOutput output, void *state)
{
	const char	*name;
	PONode		*out;

	name = p_input_string(input[0]);
	if(name == NULL || *name == '\0')
		return P_COMPUTE_DONE;

	if((out = p_output_node_create(output, V_NT_AUDIO, 0)) != NULL)
		p_node_set_name(out, name);
	return P_COMPUTE_DONE;
}

static PComputeStatus compute_bitmap(PPInput *input, PPOutput output, void *state)
{
	const char	*name;
	PONode		*out;

	name = p_input_string(input[0]);
	if(name == NULL || *name == '\0')
		return P_COMPUTE_DONE;

	if((out = p_output_node_create(output, V_NT_BITMAP, 0)) != NULL)
		p_node_set_name(out, name);
	return P_COMPUTE_DONE;
}

static PComputeStatus compute_curve(PPInput *input, PPOutput output, void *state)
{
	const char	*name;
	PONode		*out;

	name = p_input_string(input[0]);
	if(name == NULL || *name == '\0')
		return P_COMPUTE_DONE;

	if((out = p_output_node_create(output, V_NT_CURVE, 0)) != NULL)
		p_node_set_name(out, name);
	return P_COMPUTE_DONE;
}

static PComputeStatus compute_geometry(PPInput *input, PPOutput output, void *state)
{
	const char	*name;
	PONode		*out;

	name = p_input_string(input[0]);
	if(name == NULL || *name == '\0')
		return P_COMPUTE_DONE;

	if((out = p_output_node_create(output, V_NT_GEOMETRY, 0)) != NULL)
		p_node_set_name(out, name);
	return P_COMPUTE_DONE;
}

static PComputeStatus compute_material(PPInput *input, PPOutput output, void *state)
{
	const char	*name;
	PONode		*out;

	name = p_input_string(input[0]);
	if(name == NULL || *name == '\0')
		return P_COMPUTE_DONE;

	if((out = p_output_node_create(output, V_NT_MATERIAL, 0)) != NULL)
		p_node_set_name(out, name);
	return P_COMPUTE_DONE;
}

static PComputeStatus compute_object(PPInput *input, PPOutput output, void *state)
{
	const char	*name;
	PONode		*out;

	name = p_input_string(input[0]);
	if(name == NULL || *name == '\0')
		return P_COMPUTE_DONE;

	if((out = p_output_node_create(output, V_NT_OBJECT, 0)) != NULL)
		p_node_set_name(out, name);
	return P_COMPUTE_DONE;
}

static PComputeStatus compute_text(PPInput *input, PPOutput output, void *state)
{
	const char	*name;
	PONode		*out;

	name = p_input_string(input[0]);
	if(name == NULL || *name == '\0')
		return P_COMPUTE_DONE;

	if((out = p_output_node_create(output, V_NT_TEXT, 0)) != NULL)
		p_node_set_name(out, name);
	return P_COMPUTE_DONE;
}

/* ----------------------------------------------------------------------------------------- */

static void init_meta(const char *type)
{
	char	purp[128];

	p_init_meta("authors", "Emil Brink");
	p_init_meta("copyright", "2005 KTH");
	sprintf(purp, "Creates a new %s node, with a given name.", type);
	p_init_meta("help/purpose", purp);
}

void init(void)
{
	p_init_create("new-audio");
	p_init_input(0, P_VALUE_STRING, "name", P_INPUT_REQUIRED, P_INPUT_DONE);
	init_meta("audio");
	p_init_compute(compute_audio);

	p_init_create("new-bitmap");
	p_init_input(0, P_VALUE_STRING, "name", P_INPUT_REQUIRED, P_INPUT_DONE);
	init_meta("bitmap");
	p_init_compute(compute_bitmap);

	p_init_create("new-curve");
	p_init_input(0, P_VALUE_STRING, "name", P_INPUT_REQUIRED, P_INPUT_DONE);
	init_meta("curve");
	p_init_compute(compute_curve);

	p_init_create("new-geometry");
	p_init_input(0, P_VALUE_STRING, "name", P_INPUT_REQUIRED, P_INPUT_DONE);
	init_meta("geometry");
	p_init_compute(compute_geometry);

	p_init_create("new-material");
	p_init_input(0, P_VALUE_STRING, "name", P_INPUT_REQUIRED, P_INPUT_DONE);
	init_meta("material");
	p_init_compute(compute_material);

	p_init_create("new-object");
	p_init_input(0, P_VALUE_STRING, "name", P_INPUT_REQUIRED, P_INPUT_DONE);
	init_meta("object");
	p_init_compute(compute_object);

	p_init_create("new-text");
	p_init_input(0, P_VALUE_STRING, "name", P_INPUT_REQUIRED, P_INPUT_DONE);
	init_meta("text");
	p_init_compute(compute_text);
}
