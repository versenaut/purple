/*
 * An audio Purple plug-in that adds echo to some audio, i.e. an
 * attenuated and delayed addition of the signal to itself.
*/

#include "purple.h"
#include "purple-plugin.h"

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode		*in;
	PONode		*out;
	PNABuffer	*buffer;
	real64		bin[768], bout[768], delay, factor = 0.333;
	unsigned int	i, spos, slen, dpos, dlen, j, tot = 0;

	in  = p_input_node(input[0]);
	if(p_node_get_type(in) != V_NT_AUDIO)
	{
		printf("input node type at %p is type %d, aborting\n", in, p_node_get_type(in));
		return P_COMPUTE_DONE;
	}
	out   = p_output_node_copy(output, in, 0);
	delay = p_input_real32(input[1]);

	printf("In audioecho compute(), in=%p out=%p delay=%f\n", in, out, delay);
	for(i = 0; (buffer = p_node_a_buffer_nth(out, i)) != NULL; i++)
	{
		real64	f = p_node_a_buffer_get_frequency(buffer);
		uint32	dist = delay * f;

		printf(" buffer has freq %g Hz -> dist=%u samples\n", f, dist);
		/* Loop through the buffer, asking Purple for convenient-sized blocks of samples. */
		for(spos = 0;
		    (slen = p_node_a_buffer_read_samples(buffer, spos, bin, sizeof bin / sizeof *bin)) != 0;
		    spos += slen)
		{
			printf("  got %u samples at %u, reading out ahead\n", slen, spos);
			dlen = p_node_a_buffer_read_samples(buffer, spos + dist, bout, sizeof bout / sizeof *bout);
			if(dlen > 0)
			{
				printf("   got %u samples at %u to add echo to\n", dlen, spos + dist);
				for(i = 0; i < dlen; i++)
					bout[i] = bout[i] + factor * bin[i];
				p_node_a_buffer_write_samples(buffer, spos + dist, bout, dlen);
			}
		}
	}
	return P_COMPUTE_DONE;
}

void init(void)
{
	p_init_create("audioecho");
	p_init_input(0, P_VALUE_MODULE, "node",  P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL32, "delay", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
