/*
 * Basic math functions.
*/

#include <stdio.h>
#include <string.h>

#include "purple.h"

/* --------------------------------------------------------------------------------------------- */

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

/* --------------------------------------------------------------------------------------------- */

static void do_g_add(real64 *a, const real64 *b)
{
	a[0] += b[0];
	a[1] += b[1];
	a[2] += b[2];
}

static void do_g_sub(real64 *a, const real64 *b)
{
	a[0] -= b[0];
	a[1] -= b[1];
	a[2] -= b[2];
}

static void do_g_mul(real64 *a, const real64 *b)
{
	a[0] *= b[0];
	a[1] *= b[1];
	a[2] *= b[2];
}

static void do_g_div(real64 *a, const real64 *b)
{
	a[0] /= b[0];
	a[1] /= b[1];
	a[2] /= b[2];
}

static PComputeStatus geometry_math(PINode *a, PINode *b, real64 c, real64 d, PPOutput output, void (*op)(real64 *a, const real64 *b), const char *opname)
{
	char		buf[32];
	PINode		*ga, *gb;
	PONode		*obj, *geo;
	const PNGLayer	*va, *vb;
	PNGLayer	*lay;
	unsigned int	i, sa, sb, size;

	ga = p_node_o_link_get(a, "geometry", NULL);
	gb = p_node_o_link_get(b, "geometry", NULL);

	printf("geometry math: nodes %p and %p\n", ga, gb);

	/* Look up the vertex layers. */
	va = p_node_g_layer_find(ga, "vertex");
	vb = p_node_g_layer_find(gb, "vertex");

	if(va == NULL && vb == NULL)
		return P_COMPUTE_DONE;

	sa = p_node_g_layer_get_size(va);
	sb = p_node_g_layer_get_size(vb);

	if(va != NULL && vb != NULL && sa != sb)
		return P_COMPUTE_DONE;
	size = va != NULL ? sa : sb;

	printf(" sizes: %u and %u -> %u\n", sa, sb, size);

	obj = p_output_node_create(output, V_NT_OBJECT, 0);
	/* Compute a pretty name for the result. */
	if(a != NULL && b != NULL)
		sprintf(buf, "%s%s%s", p_node_get_name(a), opname, p_node_get_name(b));
	else if(a == NULL)
		sprintf(buf, "%g%s%s", c, opname, p_node_get_name(b));
	else if(b == NULL)
		sprintf(buf, "%s%s%g", p_node_get_name(a), opname, d);
	p_node_set_name(obj, buf);
	geo = p_output_node_copy(output, ga != NULL ? ga : gb, 1);	/* Copy one of the nodes, doesn't matter which one. */
	p_node_o_link_set(obj, geo, "geometry", 0u);

	lay = p_node_g_layer_find(geo, "vertex");
	for(i = 0; i < size; i++)
	{
		real64	n[3], xyza[3], xyzb[3];

		if(va != NULL)
			p_node_g_vertex_get_xyz(va, i, xyza, xyza + 1, xyza + 2);
		else
			xyza[0] = xyza[1] = xyzb[2] = c;
		if(vb != NULL)
			p_node_g_vertex_get_xyz(vb, i, xyzb, xyzb + 1, xyzb + 2);
		else
			xyzb[0] = xyzb[1] = xyzb[2] = d;
		op(xyza, xyzb);
		p_node_g_vertex_set_xyz(lay, i, xyza[0], xyza[1], xyza[2]);
		printf(" got (%g,%g,%g) and (%g,%g,%g)\n",
		       xyza[0], xyza[1], xyza[2],
		       xyzb[0], xyzb[1], xyzb[2]);
		printf("  emitting vertex %u: (%g,%g,%g)\n", i, xyza[0], xyza[1], xyza[2]);
	}
	return P_COMPUTE_DONE;
}

static PComputeStatus geometry_add(PINode *a, PINode *b, real64 c, real64 d, PPOutput output)
{
	return geometry_math(a, b, c, d, output, do_g_add, "+");
}

static PComputeStatus geometry_sub(PINode *a, PINode *b, real64 c, real64 d, PPOutput output)
{
	return geometry_math(a, b, c, d, output, do_g_sub, "-");
}

static PComputeStatus geometry_mul(PINode *a, PINode *b, real64 c, real64 d, PPOutput output)
{
	return geometry_math(a, b, c, d, output, do_g_mul, "*");
}

static PComputeStatus geometry_div(PINode *a, PINode *b, real64 c, real64 d, PPOutput output)
{
	return geometry_math(a, b, c, d, output, do_g_div, "/");
}

/* --------------------------------------------------------------------------------------------- */

typedef enum { INPUT_PLAIN_BITMAP = 0, INPUT_OBJECT_WITH_GEOMETRY = 32 } InputMode;

/* Input a node, using the indicated mode. Modes are more or less hardcoded, for now. */
static PINode * input_node(PPInput input, InputMode mode)
{
	if(mode == INPUT_PLAIN_BITMAP)
		return p_input_node_first_type(input, V_NT_BITMAP);
	else if(mode == INPUT_OBJECT_WITH_GEOMETRY)
	{
		unsigned int	i;
		PINode		*obj;

		for(i = 0; (obj = p_input_node_nth(input, i)) != NULL; i++)
		{
			if(p_node_get_type(obj) != V_NT_OBJECT)
				continue;
			if(p_node_o_link_get(obj, "geometry", NULL) != NULL)
				return obj;
		}
	}
	return NULL;
}

static int collect_inputs(PPInput *input, PINode **a, PINode **b, real64 *c, real64 *d, InputMode mode)
{
	/* Try to read out two nodes. */
	*a = input_node(input[0], mode);
	*b = input_node(input[1], mode);

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

	if(collect_inputs(input, &a, &b, &c, &d, INPUT_PLAIN_BITMAP))
		return bitmap_add(a, b, c, d, output);
	else if(collect_inputs(input, &a, &b, &c, &d, INPUT_OBJECT_WITH_GEOMETRY))
		return geometry_add(a, b, c, d, output);
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
	else if(collect_inputs(input, &a, &b, &c, &d, INPUT_OBJECT_WITH_GEOMETRY))
		return geometry_sub(a, b, c, d, output);
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
	else if(collect_inputs(input, &a, &b, &c, &d, INPUT_OBJECT_WITH_GEOMETRY))
		return geometry_mul(a, b, c, d, output);
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
	else if(collect_inputs(input, &a, &b, &c, &d, INPUT_OBJECT_WITH_GEOMETRY))
		return geometry_div(a, b, c, d, output);
	else
		p_output_real64(output, c / d);
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("add");
	p_init_input(0, P_VALUE_MODULE, "a", P_INPUT_DESC("First term in addition."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_MODULE, "b", P_INPUT_DESC("Second term in addition."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("copyright", "2005 PDC, KTH");
	p_init_meta("desc/purpose", "Computes addition of two terms. The terms can be either real numbers, bitmaps, or object nodes with geometry links.");
	p_init_compute(compute_add);

	p_init_create("sub");
	p_init_input(0, P_VALUE_MODULE, "a", P_INPUT_DESC("The first term in the subtraction."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_MODULE, "b", P_INPUT_DESC("THe second term in the subtraction."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("copyright", "2005 PDC, KTH");
	p_init_meta("desc/purpose", "Computes difference of two terms. The terms can be either real numbers, bitmaps, or object nodes with geometry links.");
	p_init_compute(compute_sub);

	p_init_create("mul");
	p_init_input(0, P_VALUE_MODULE, "a", P_INPUT_DESC("The first factor in the multiplication."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_MODULE, "b", P_INPUT_DESC("The second factor in the multiplication."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("copyright", "2005 PDC, KTH");
	p_init_meta("desc/purpose", "Computes product of two factors. The factors can be either real numbers, bitmaps, or object nodes with geometry links.");
	p_init_compute(compute_mul);

	p_init_create("div");
	p_init_input(0, P_VALUE_MODULE, "a", P_INPUT_DESC("The nominator in the division."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_MODULE, "b", P_INPUT_DESC("The denominator in the division."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("copyright", "2005 PDC, KTH");
	p_init_meta("desc/purpose", "Computes quotient of nominator and denominators. These can be either real numbers, bitmaps, or object nodes with geometry links.");
	p_init_compute(compute_div);
}
