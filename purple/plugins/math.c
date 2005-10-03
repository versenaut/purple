/*
 * Basic math functions.
*/

#include "purple.h"

static PONode * bitmap_create(PINode *a, PINode *b, uint16 *dim, PPOutput output)
{
	uint16	dima[3], dimb[3];
	PONode	*out;

	if(a != NULL)
		p_node_b_get_dimensions(a, dima, dima + 1, dima + 2);
	if(b != NULL)
		p_node_b_get_dimensions(b, dimb, dimb + 1, dimb + 2);
	if(a != NULL && b != NULL)
	{
		int	i;

		for(i = 0; i < 3; i++)
			dim[i] = dima[i] > dimb[i] ? dima[i] : dimb[i];
	}
	else
		memcpy(dim, a != NULL ? dima : dimb, sizeof dima);
	out = p_output_node_create(output, V_NT_BITMAP, 0);
	p_node_b_set_dimensions(out, dim[0], dim[1], dim[2]);
	return out;
}

static real64 do_b_add(real64 a, real64 b)
{
	return a + b;
}

static real64 do_b_sub(real64 a, real64 b)
{
	return a - b;
}

static real64 do_b_mul(real64 a, real64 b)
{
	return a * b;
}

static real64 do_b_div(real64 a, real64 b)
{
	return a / b;
}

/* Perform binary operation involving (one or two) bitmaps. */
static PComputeStatus bitmap_math(PINode *a, PINode *b, real64 c, real64 d, PPOutput output, real64 (*op)(real64 a, real64 b))
{
	const char	*ln[] = { "col_r", "col_g", "col_b" };
	uint16		dim[3], x, y, z;
	real64		fx, fy, fz;
	PONode		*out;
	PNBLayer	*la, *lb, *lay;
	int		i;

	out = bitmap_create(a, b, dim, output);

	for(i = 0; i < sizeof ln / sizeof *ln; i++)
	{
		la = lb = NULL;
		if(a != NULL)
		{
			la = p_node_b_layer_find(a, ln[i]);
			if(la == NULL)
				continue;
		}
		if(b != NULL)
		{
			lb = p_node_b_layer_find(b, ln[i]);
			if(lb == NULL)
				continue;
		}
		lay = p_node_b_layer_create(out, ln[i], p_node_b_layer_get_type(la != NULL ? la : lb));
		for(z = 0; z < dim[2]; z++)
		{
			fz = (real64) z / dim[2];
			for(y = 0; y < dim[1]; y++)
			{
				fy = (real64) y / dim[1];
				/* Decide which kind of operation is needed. There are only three;
				 * "constant <op> constant" is not a bitmap operation.
				*/
				if(a != NULL && b != NULL)
				{
					for(x = 0; x < dim[0]; x++)
					{
						fx = (real64) x / dim[0];
						p_node_b_layer_pixel_write(out, lay, x, y, z, op(p_node_b_layer_pixel_read_filtered(a, la, P_B_FILTER_NEAREST, fx, fy, fz),
												 p_node_b_layer_pixel_read_filtered(b, lb, P_B_FILTER_NEAREST, fx, fy, fz)));
					}
				}
				else if(a == NULL)
				{
					for(x = 0; x < dim[0]; x++)
					{
						fx = (real64) x / dim[0];
						p_node_b_layer_pixel_write(out, lay, x, y, z, op(c, p_node_b_layer_pixel_read_filtered(b, lb, P_B_FILTER_NEAREST, fx, fy, fz)));
					}
				}
				else if(b == NULL)
				{
					for(x = 0; x < dim[0]; x++)
					{
						fx = (real64) x / dim[0];
						p_node_b_layer_pixel_write(out, lay, x, y, z, op(p_node_b_layer_pixel_read_filtered(a, la, P_B_FILTER_NEAREST, fx, fy, fz), d));
					}
				}
			}
		}
	}
	return P_COMPUTE_DONE;
}

static PComputeStatus bitmap_add(PINode *a, PINode *b, real64 c, real64 d, PPOutput output)
{
	return bitmap_math(a, b, c, d, output, do_b_add);
}

static PComputeStatus bitmap_sub(PINode *a, PINode *b, real64 c, real64 d, PPOutput output)
{
	return bitmap_math(a, b, c, d, output, do_b_sub);
}

static PComputeStatus bitmap_mul(PINode *a, PINode *b, real64 c, real64 d, PPOutput output)
{
	return bitmap_math(a, b, c, d, output, do_b_mul);
}

static PComputeStatus bitmap_div(PINode *a, PINode *b, real64 c, real64 d, PPOutput output)
{
	return bitmap_math(a, b, c, d, output, do_b_div);
}

static int collect_inputs(PPInput *input, PINode **a, PINode **b, real64 *c, real64 *d, VNodeType type)
{
	/* Try to read out two nodes. */
	*a = p_input_node_first_type(input[0], type);
	*b = p_input_node_first_type(input[1], type);

	/* If node reading failed, read real64s instead. */
	if(*a == NULL)
		*c = p_input_real64(input[0]);
	if(*b == NULL)
		*d = p_input_real64(input[1]);
	return *a != NULL || *b != NULL;
}

static PComputeStatus compute_add(PPInput *input, PPOutput output, void *state)
{
	PINode	*a, *b;
	real64	c, d;

	if(collect_inputs(input, &a, &b, &c, &d, V_NT_BITMAP))
		return bitmap_add(a, b, c, d, output);
	else
		p_output_real64(output, c + d);
	return P_COMPUTE_DONE;
}

static PComputeStatus compute_sub(PPInput *input, PPOutput output, void *state)
{
	PINode	*a, *b;
	real64	c, d;

	if(collect_inputs(input, &a, &b, &c, &d, V_NT_BITMAP))
		return bitmap_sub(a, b, c, d, output);
	else
		p_output_real64(output, c - d);
	return P_COMPUTE_DONE;
}

static PComputeStatus compute_mul(PPInput *input, PPOutput output, void *state)
{
	PINode	*a, *b;
	real64	c, d;

	if(collect_inputs(input, &a, &b, &c, &d, V_NT_BITMAP))
		return bitmap_mul(a, b, c, d, output);
	else
		p_output_real64(output, c * d);
	return P_COMPUTE_DONE;
}

static PComputeStatus compute_div(PPInput *input, PPOutput output, void *state)
{
	PINode	*a, *b;
	real64	c, d;

	if(collect_inputs(input, &a, &b, &c, &d, V_NT_BITMAP))
		return bitmap_div(a, b, c, d, output);
	else
		p_output_real64(output, c / d);
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("add");
	p_init_input(0, P_VALUE_MODULE, "a", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_MODULE, "b", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute_add);

	p_init_create("sub");
	p_init_input(0, P_VALUE_MODULE, "a", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_MODULE, "b", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute_sub);

	p_init_create("mul");
	p_init_input(0, P_VALUE_MODULE, "a", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_MODULE, "b", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute_mul);

	p_init_create("div");
	p_init_input(0, P_VALUE_MODULE, "a", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_MODULE, "b", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute_div);
}
