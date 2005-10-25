/*
 * Super-simple plug-in that just takes in a string, and outputs it as some other type. This
 * type conversion is not even strictly necessary (Purple can do string->number conversions),
 * but it improves performance slightly.
*/

#include <ctype.h>

#include "purple.h"

static int vector_parse(const char *str, real64 *vec, int max)
{
	char	*eptr;
	real64	x;
	int	i;

	for(i = 0; i < max; i++)
	{
		x = strtod(str, &eptr);
		if(eptr > str)
			vec[i] = x;
		else
			break;
		str = eptr;
		while(*str && (isspace(*str) || *str == ',') || *str == '[' || *str == ']')
			str++;
	}
	return i;
}

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	const char	*str = p_input_string(input[0]);
	const uint32	type = p_input_uint32(input[1]);

	switch(type)
	{
	case 0:
		p_output_string(output, str);
		break;
	case 1:
		{
			real64	t = 0.0;

			sscanf(str, "%lg", &t);
			p_output_real64(output, t);
		}
		break;
	case 2:
		{
			uint32	t = 0;

			sscanf(str, "%lu", &t);
			p_output_uint32(output, t);
		}
		break;
	case 3:
		{
			real64	v[4];
			int	len = vector_parse(str, v, sizeof v / sizeof *v);

			if(len == 1)
			{
				v[1] = v[2] = v[3] = v[0];
				p_output_real64_vec4(output, v);
			}
			else if(len == 2)
				p_output_real64_vec2(output, v);
			else if(len == 3)
				p_output_real64_vec3(output, v);
			else if(len == 4)
				p_output_real64_vec4(output, v);
		}
		break;
	case 4:
		{
			real64	m[16];
			int	len = vector_parse(str, m, sizeof m / sizeof *m), i;

			if(len == 0)
				return P_COMPUTE_DONE;
			if(len == 1)	/* Copy single value onto main diagonal, clearing the rest. */
			{
				for(i = 1; i < 16; i++)
					m[i] = ((i % 5) == 0) ? m[0] : 0.0;
			}
			else		/* Zero the rest. */
			{
				for(i = len; i < 16; i++)
					m[i] = 0.0;
			}
			p_output_real64_mat16(output, m);
		}
		break;
	}
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("constant");
	p_init_input(0, P_VALUE_STRING, "value", P_INPUT_REQUIRED,
		     P_INPUT_DESC("String representation of desired value is input here."), P_INPUT_DONE);
	p_init_input(1, P_VALUE_UINT32, "type", P_INPUT_REQUIRED,
		     P_INPUT_DESC("This controls what type the output value will have."),
		     P_INPUT_ENUM("0:String|1:Real64|2:UInt32|3:Real64_Vector|4:Real64_Matrix"), P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("desc/purpose", "Output a constant value. The value is created by interpreting an input string according to the 'type' "
		    "setting, and outputting the result. This is useful when the same value needs to be sent to several other plug-ins' "
		    "inputs; an instance of constant can be used to 'buffer' the value and make it possible to change it in just one place.");
	p_init_compute(compute);
}
