/*
 * An audio Purple plug-in that creates a pure sine-wave tone, of a given
 * frequency and duration.
*/

#include <limits.h>
#include <math.h>

#include "purple.h"
#include "purple-plugin.h"

#if !defined M_PI
#define	M_PI	3.14159265358979323846	/* Don't trust <math.h> to have it. */
#endif

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	VNALayerType	type = VN_A_LAYER_INT16;
	real64		sfreq = 44100.0, tone, dur, t, ang;
	PONode		*out;
	PNALayer	*layer;
	unsigned int	i, pos, len, to_go, chunk;

	tone = p_input_real32(input[0]);
	dur  = p_input_real32(input[1]);
	/* FIXME: Actually use inputs 2 and 3, here. With defaults, preferably. */

	out = p_output_node_create(output, V_NT_AUDIO, 0);
	layer = p_node_a_layer_create(out, "tone", type, sfreq);

	printf("Generating %g seconds of %g Hz sine, sampled at %g Hz\n", dur, tone, sfreq);
	len = dur * sfreq;
	printf(" that's %lu samples\n", len);
	if(type == VN_A_LAYER_INT16)
	{
		int16	block[1024];	/* Generate tone in blocks of 1 ksamples. */

		for(pos = 0, to_go = len; to_go > 0; pos += chunk, to_go -= chunk)
		{
			chunk = to_go > sizeof block / sizeof *block ? sizeof block / sizeof *block : to_go;
			printf("building chunk %u\n", chunk);
			for(i = 0; i < chunk; i++)
			{
				t   = (pos + i) / sfreq;
				ang = 2.0 * M_PI * (t * tone);
				block[i] = pow(2, CHAR_BIT * sizeof *block - 1) * sin(ang);
			}
			p_node_a_layer_write_samples(layer, pos, block, chunk);
		}
	}
	return P_COMPUTE_DONE;
}

void init(void)
{
	p_init_create("audiotone");
	p_init_input(0, P_VALUE_REAL32, "frequency", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL32, "duration",  P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(2, P_VALUE_UINT32, "format",   P_INPUT_DEFAULT(VN_A_LAYER_INT16), P_INPUT_DONE);
	p_init_input(3, P_VALUE_REAL32, "samplefrequency", P_INPUT_DEFAULT(44100.0), P_INPUT_DONE);
	p_init_compute(compute);
}