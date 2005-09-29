/*
 * Basic vector maths.
*/

#include "purple.h"

/* Vector addition. This works at the maximum size of vectors that Purple supports, i.e. 4D.
 * This means that e.g. [1 2] + [3 4] will be [4 6 0 0], i.e. everything gets extended to
 * 4D. This shouldn't matter though, since p_input_X_vecN() will do the right thing.
*/
static PComputeStatus compute_vec_add(PPInput *input, PPOutput output, void *state)
{
	const real64	*x, *y;
	real64		r[4];

	if((x = p_input_real64_vec4(input[0])) != NULL && (y = p_input_real64_vec4(input[1])) != NULL)
	{
		int	i;

		for(i = 0; i < 4; i++)
			r[i] = x[i] + y[i];
		p_output_real64_vec4(output, r);
	}
	return P_COMPUTE_DONE;
}

/* Vector subtraction. See comment for addition, above. */
static PComputeStatus compute_vec_sub(PPInput *input, PPOutput output, void *state)
{
	const real64	*x, *y;
	real64		r[4];

	if((x = p_input_real64_vec4(input[0])) != NULL && (y = p_input_real64_vec4(input[1])) != NULL)
	{
		int	i;

		for(i = 0; i < 4; i++)
			r[i] = x[i] - y[i];
		p_output_real64_vec4(output, r);
	}
	return P_COMPUTE_DONE;
}

static PComputeStatus compute_vec_multiply(PPInput *input, PPOutput output, void *state)
{
	const real64	*x, y = p_input_real64(input[1]);
	real64		r[4];

	if((x = p_input_real64_vec4(input[0])) != NULL)
	{
		int	i;

		for(i = 0; i < 4; i++)
			r[i] = x[i] * y;
		p_output_real64_vec4(output, r);
	}
	return P_COMPUTE_DONE;
}

static PComputeStatus compute_vec_divide(PPInput *input, PPOutput output, void *state)
{
	const real64	*x, y = p_input_real64(input[1]);
	real64		r[4];

	if((x = p_input_real64_vec4(input[0])) != NULL)
	{
		int	i;

		for(i = 0; i < 4; i++)
			r[i] = x[i] / y;
		p_output_real64_vec4(output, r);
	}
	return P_COMPUTE_DONE;
}

static PComputeStatus compute_vec_cross(PPInput *input, PPOutput output, void *state)
{
	const real64	*x, *y;
	real64		r[3];

	if((x = p_input_real64_vec3(input[0])) != NULL && (y = p_input_real64_vec3(input[1])) != NULL)
	{
		r[0] = x[1] * y[2] - x[2] * y[1];
		r[1] = x[2] * y[0] - x[0] * y[2];
		r[2] = x[0] * y[1] - x[1] * y[0];
		p_output_real64_vec3(output, r);
	}
	return P_COMPUTE_DONE;
}

static PComputeStatus compute_vec_scalar(PPInput *input, PPOutput output, void *state)
{
	const real64	*x, *y;

	/* Compute from 4D vectors, will give result according to overlap. E.g. 2D vs 3D will compute as 2D, etc. */
	if((x = p_input_real64_vec4(input[0])) != NULL && (y = p_input_real64_vec4(input[1])) != NULL)
		p_output_real64(output, x[0] * y[0] + x[1] * y[1] + x[2] * y[2] + x[3] * y[3]);
	return P_COMPUTE_DONE;
}

static PComputeStatus compute_vec_length(PPInput *input, PPOutput output, void *state)
{
	const real64	*vec;
	real64		x;

	if((vec = p_input_real64_vec4(input[0])) != NULL)
		x = sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2] + vec[3] * vec[3]);
	else
		x = 0.0;
	p_output_real64(output, x);
	return P_COMPUTE_DONE;
}

static PComputeStatus compute_vec_swizzle(PPInput *input, PPOutput output, void *state)
{
	const real64	*vec;
	const char	*pattern;

	if((vec = p_input_real64_vec4(input[0])) != NULL)
	{
		if((pattern = p_input_string(input[1])) != NULL)
		{
			real64	out[4] = { 0.0, 0.0, 0.0, 0.0 };
			int	i, p;

			for(i = p = 0; pattern[i] != '\0' && p < 4; i++)
			{
				if(pattern[i] == 'x')
					out[p++] = vec[0];
				else if(pattern[i] == 'y')
					out[p++] = vec[1];
				else if(pattern[i] == 'z')
					out[p++] = vec[2];
				else if(pattern[i] == 'w')
					out[p++] = vec[3];
			}
		}
	}
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("vec-add");
	p_init_input(0, P_VALUE_REAL64_VEC4, "x", P_INPUT_DESC("A vector will be parsed from this input, to serve as one of the terms of the addition."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL64_VEC4, "y", P_INPUT_DESC("A vector will be parsed from this input, to serve as the other term of the addition."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute_vec_add);

	p_init_create("vec-multiply");
	p_init_input(0, P_VALUE_REAL64_VEC4, "x", P_INPUT_DESC("A vector will be parsed from this input, and multiplied by the other input's value."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL64, "y", P_INPUT_DESC("A real number is read here, and used to scale the vector."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute_vec_multiply);

	p_init_create("vec-divide");
	p_init_input(0, P_VALUE_REAL64_VEC4, "x", P_INPUT_DESC("A vector will be parsed from this input, and divided by the other input's value."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL64, "y", P_INPUT_DESC("A real number is read here, and used to scale the vector."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute_vec_divide);

	p_init_create("vec-cross");
	p_init_input(0, P_VALUE_REAL64_VEC4, "x", P_INPUT_DESC("A 3D vector will be read here, and used as one part of the cross product computation."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL64_VEC4, "y", P_INPUT_DESC("A 3D vector will be read here, and used as one part of the cross product computation."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute_vec_cross);
	
	p_init_create("vec-scalar");
	p_init_input(0, P_VALUE_REAL64_VEC4, "x", P_INPUT_DESC("A 3D vector will be read here, and used as one part of the scalar product computation."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL64_VEC4, "y", P_INPUT_DESC("A 3D vector will be read here, and used as one part of the scalar product computation."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute_vec_scalar);

	p_init_create("vec-length");
	p_init_input(0, P_VALUE_REAL64_VEC4, "x", P_INPUT_DESC("A vector will be parsed from this input, and the length will be computed and output."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute_vec_length);

	p_init_create("vec-swizzle");
	p_init_input(0, P_VALUE_REAL64_VEC4, "x", P_INPUT_DESC("A vector to swizzle."), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_STRING, "pattern", P_INPUT_DESC("Pattern to control swizzling. Use 'x', 'y', 'z' and 'w' to refer to the input vector's "
								"components. E.g. \"xyzw\" is the identity transform."), P_INPUT_REQUIRED);
	p_init_compute(compute_vec_swizzle);
}
