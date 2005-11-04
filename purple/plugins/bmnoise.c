/*
 * 
*/

#define _USE_MATH_DEFINES
#include <math.h>

#include <stdio.h>

#include "purple.h"

static real32 noise1(uint32 x, uint32 y, uint32 seed)
{
	x += y * 57;
	x = (x << 13) ^ x;
	return 1.0f - ((x * (x * x * 15731u + 789221u + seed) + 1376312589u) & 0x7fffffff) / 1073741824.0;
}

static real32 noise2(uint32 x, uint32 y, uint32 seed)
{
	x += y * 57;
	x = (x << 13) ^ x;
	return 1.0f - ((x * (x * x * 16661u + 789389u + seed) + 1376312589u) & 0x7fffffff) / 1073741824.0;
}

static real32 noise3(uint32 x, uint32 y, uint32 seed)
{
	x += y * 57;
	x = (x << 13) ^ x;
	return 1.0f - ((x * (x * x * 16333u + 790057u + seed) + 1376312589u) & 0x7fffffff) / 1073741824.0;
}

static real32 noise4(uint32 x, uint32 y, uint32 seed)
{
	x += y * 57;
	x = (x << 13) ^ x;
	return 1.0f - ((x * (x * x * 15541u + 788897u + seed) + 1376312589u) & 0x7fffffff) / 1073741824.0;
}

static real32 interpolate_linear(real32 a, real32 b, real32 t)
{
	return a * (1.0f - t) + b * t;
}

static real32 interpolate_cosine(real32 a, real32 b, real32 t)
{
	register real32	f = (1.0f - cosf(t * M_PI)) * 0.5f;

	return a * (1.0 - f) + b * f;
}

static real32 smooth_noise(real32 x, real32 y, real32 (*noise)(uint32 x, uint32 y, uint32 seed), uint32 seed)
{
	register real32 co, si, ce;

	co = (noise(x - 1.0f, y - 1.0f, seed) + noise(x + 1.0f, y - 1.0f, seed) + noise(x + 1.0f, y + 1.0f, seed) + noise(x - 1.0f, y + 1.0f, seed)) / 16.0f;
	si = (noise(x - 1.0f, y, seed)        + noise(x + 1.0, y, seed)         + noise(x, y - 1.0f, seed)        + noise(x, y + 1.0f, seed)) / 8.0f;
	ce = noise(x, y, seed) / 4.0;

	return co + si + ce;
}

static real32 interpolated_noise(real32 x, real32 y, real32 (*noise)(uint32 x, uint32 y, uint32 seed), uint32 seed,
				 real32 (*interpolate)(real32 a, real32 b, real32 t))
{
	uint32	xi = x, yi = y;
	real32	xf = x - xi, yf = y - yi, v1, v2, v3, v4, i1, i2;

	v1 = smooth_noise(xi,     yi,     noise, seed);
	v2 = smooth_noise(xi + 1, yi,     noise, seed);
	v3 = smooth_noise(xi,     yi + 1, noise, seed);
	v4 = smooth_noise(xi + 1, yi + 1, noise, seed);

	i1 = interpolate(v1, v2, xf);
	i2 = interpolate(v3, v4, xf);

	return interpolate(i1, i2, yf);
}

static real32 (*noise[])(uint32 x, uint32 y, uint32 seed) = { noise1, noise2, noise3, noise4 };

static real32 perlin_2d(real32 x, real32 y, uint32 seed)
{
	real32	total = 0, p = 0.5f, freq = 0.2f, ampl = 1.0f;
	int	i;

	for(i = 0; i < 4; i++, freq *= 2.0, ampl *= p)
		total += ampl * interpolated_noise(x * freq, y * freq, noise[i], seed, interpolate_linear);
	return total;
}

/* --------------------------------------------------------------------------------------------- */

#if !defined STANDALONE

static real64 cb_noise(uint32 x, uint32 y, uint32 z, void *user)
{
	real32	v = (1.0f + perlin_2d(x, y, *(uint32 *) user)) / 2.0f;	/* Move into proper [0,1] color range. */

	return (v < 0.0f) ? 0.0f : (v > 1.0f) ? 1.0f : v;
}

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	const char	*lname[] = { "col_r", "col_g", "col_b" };
	PONode		*node;
	PNBLayer	*layer;
	uint32		width, height, seed = 0, i;

	width  = p_input_uint32(input[0]);
	height = p_input_uint32(input[1]);
	seed   = p_input_uint32(input[2]);

	node = p_output_node_create(output, V_NT_BITMAP, 0);
	p_node_b_set_dimensions(node, width, height, 1);
	for(i = 0; i < sizeof lname / sizeof *lname; i++)
	{
		layer = p_node_b_layer_create(node, lname[i], VN_B_LAYER_UINT8);
		p_node_b_layer_foreach_set(node, layer, cb_noise, &seed);
	}
	printf("Done computing %ux%u-pixel Perlin 2D noise texture\n", width, height);
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("bmnoise");
	p_init_input(0, P_VALUE_UINT32, "width", P_INPUT_REQUIRED, P_INPUT_DEFAULT(32.0), P_INPUT_MAX(1024),
		     P_INPUT_DESC("Width of bitmap to create."), P_INPUT_DONE);
	p_init_input(1, P_VALUE_UINT32, "height", P_INPUT_REQUIRED, P_INPUT_DEFAULT(32.0), P_INPUT_MAX(1024),
		     P_INPUT_DESC("Height of bitmap to create."), P_INPUT_DONE);
	p_init_input(2, P_VALUE_UINT32, "seed", P_INPUT_REQUIRED, P_INPUT_DEFAULT(0.0),
		     P_INPUT_DESC("Seed value for pseudo random number generation."), P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("copyright", "2005 PDC, KTH");
	p_init_meta("desc/purpose", "Creates a 2D bitmap of Perlin noise.");
	p_init_compute(compute);
}

#endif		/* ! STANDALONE */

/* --------------------------------------------------------------------------------------------- */

#if defined STANDALONE
int main(void)
{
	int	x, y, high;
	double	here, max = -1E30;

	for(y = high = 0; y < 1024; y++)
	{
		for(x = 0; x < 1024; x++)
		{
			here = perlin_2d(x, y);
			if(here > max)
				max = here;
			if(here > 0.9)
				high++;
		}
	}
	printf("max: %g\n", max);
	printf("high=%d (%.2g%%)\n", high, 100.f * high / (x * y));
	return 0;
}
#endif
