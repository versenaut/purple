/*
 * An audio Purple plug-in that rescales some audio, i.e. amplifies or
 * attenuates the signal.
*/

#include "purple.h"

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode		*in;
	PONode		*out;
	PNABuffer	*buffer;
	real64		buf[768], scale;
	unsigned int	i, pos, len, j, tot = 0;

	printf("now in audioscale::compute()\n");
	in  = p_input_node(input[0]);
	if(p_node_get_type(in) != V_NT_AUDIO)
	{
		printf("input node type at %p is type %d, aborting\n", in, p_node_get_type(in));
		return P_COMPUTE_DONE;
	}
	out = p_output_node_copy(output, in, 0);
	scale = p_input_real32(input[1]);

	printf("In compute(), in=%p out=%p scale=%f\n", in, out, scale);
	for(i = 0; (buffer = p_node_a_buffer_nth(out, i)) != NULL; i++)
	{
		/* Loop through the buffer, asking Purple for convenient-sized blocks of samples. */
		for(pos = 0; (len = p_node_a_buffer_read_samples(buffer, pos, buf, sizeof buf / sizeof *buf)) != 0; pos += len)
		{
			/* Perform the rescaling, this is why we're here. */
			for(i = 0; i < len; i++)
				buf[i] *= scale;
			/* And let Purple have the data back. */
			p_node_a_buffer_write_samples(buffer, pos, buf, len);
		}
		printf("Done with buffer '%s', processed %u bytes\n", p_node_a_buffer_get_name(buffer), pos);
	}
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("audioscale");
	p_init_input(0, P_VALUE_MODULE, "node",   P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL32, "factor", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
