/*
 * Sketch of a plug-in to create a new audio source. This is going to have to be
 * updated once I learn more about how the audio system expects a source to look.
*/

#include "purple.h"

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	const char	*url = p_input_string(input[0]);
	PONode		*object, *audio;

	audio = p_output_node_create(output, V_NT_AUDIO, 0);
	p_node_tag_create_path(audio, "source/url", VN_TAG_STRING, url);

	object = p_output_node_create(output, V_NT_OBJECT, 1);
	p_node_o_link_set(object, audio, "source", ~0);

	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("new-source");
	p_init_input(0, P_VALUE_STRING, "URL", P_INPUT_REQUIRED, P_INPUT_DONE);

	p_init_compute(compute);
}
