/*
 * Exercise the text node.
*/

#include "purple.h"
#include "purple-plugin.h"

static int32 append(int32 part, PNTBuffer *buffer, const char *word)
{
	if(part > 0)
		p_node_t_buffer_append(buffer, " ");
	p_node_t_buffer_append(buffer, word);
	return part + 1;
}

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PONode		*node;
	PNTBuffer	*buf;
	int32		number = p_input_int32(input[0]), part = 0;
	const char	*digit[] = { "zero", "one", "two", "three", "four",
				"five", "six", "seven", "eight", "nine" },
			*teen[] = { "eleven", "twelve", "thirteen", "fourteen", "fifteen",
				    "sixteen", "seventeen", "eighteen", "nineteen" },
			*ten[] = { "ten", "twenty", "thirty", "forty", "fifty",
				"sixty", "seventy", "eighty", "ninety" };
	char	line[1024];

	node = p_output_node_create(output, V_NT_TEXT, 0);
	p_node_set_name(node, "textnum");
/*	p_node_t_language_set(node, "English");*/		/* Yes, really. */
	buf = p_node_t_buffer_create(node, "number");
	if(number < 0)
	{
		p_node_t_buffer_append(buf, "minus ");
		number = -number;
	}
	if(number >= 100000)
	{
		append(0, buf, "a lot");
		return P_COMPUTE_DONE;
	}
	do
	{
/*		printf("%d: %d\n", part, number);*/
		if(number == 0)
			part = append(part, buf, "zero");
		else if(number < 10)
		{
			part = append(part, buf, digit[number]);
			number = 0;
		}
		else if(number > 10 && number < 20)
		{
			if(part > 0)
				part = append(part, buf, "and");
			part = append(part, buf, teen[number - 11]);
			number = 0;
		}
		else if(number < 100)
		{
			part = append(part, buf, ten[number / 10 - 1]);
			number %= 10;
		}
		else if(number < 1000)
		{
			part = append(part, buf, digit[number / 100]);
			part = append(part, buf, "hundred");
			number %= 100;
		}
		else if(number < 10000)
		{
			part = append(part, buf, digit[number / 1000]);
			part = append(part, buf, "thousand");
			number %= 1000;
		}
		else if(number < 100000)
		{
			part = append(part, buf, ten[number / 10000 - 1]);
			number %= 10000;
		}
	} while(number > 0);

/*	if(p_node_t_buffer_read_line(buf, 0, line, sizeof line) != NULL)
		printf("built: '%s'\n", line);
*/
	return P_COMPUTE_DONE;
}

void init(void)
{
	p_init_create("textnum");
	p_init_input(0, P_VALUE_INT32, "number", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
