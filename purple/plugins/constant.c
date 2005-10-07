/*
 * Super-simple plug-in that just takes in a string, and outputs it as some other type. This
 * type conversion is not even strictly necessary (Purple can do string->number conversions),
 * but it improves performance slightly.
*/

#include "purple.h"

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
		     P_INPUT_ENUM("0:String|1:Real64|2:UInt32"), P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("desc/purpose", "Output a constant value. The value is created by interpreting an input string according to the 'type' "
		    "setting, and outputting the result. This is useful when the same value needs to be sent to several other plug-ins' "
		    "inputs; an instance of constant can be used to 'buffer' the value and make it possible to change it in just one place.");
	p_init_compute(compute);
}
