/*
 * Some basic string operations.
*/

#include <stdio.h>
#include <string.h>

#include "purple.h"

static PComputeStatus strlength_compute(PPInput *input, PPOutput output, void *state)
{
	const char	*str;

	if((str = p_input_string(input[0])) != NULL)
		p_output_uint32(output, strlen(str));
	return P_COMPUTE_DONE;
}

static PComputeStatus strjoin_compute(PPInput *input, PPOutput output, void *state)
{
	const char	*str1, *str2;

	if((str1 = p_input_string(input[0])) != NULL &&
	   (str2 = p_input_string(input[1])) != NULL)
	{
		size_t	l1 = strlen(str1), l2 = strlen(str2);
		char	*buf;

		printf("strjoin on '%s' and '%s'\n", str1, str2);
		/* Actual joining needs to use a temporary buffer, that is then copied into another
		 * dynamically allocated buffer by the Purple core. Not... optimal, I guess. :/
		 */
		buf = malloc(l1 + l2 + 1);
		strcpy(buf, str1);
		strcpy(buf + l1, str2);
		p_output_string(output, buf);
		printf(" done, output '%s'\n", buf);
		free(buf);
	}
	return P_COMPUTE_DONE;
}

static PComputeStatus strcut_compute(PPInput *input, PPOutput output, void *state)
{
	const char	*str;

	if((str = p_input_string(input[0])) != NULL)
	{
		uint32	start, length;
		size_t	slen;

		start  = p_input_uint32(input[1]);
		length = p_input_uint32(input[2]);
		slen = strlen(str);
		if(slen == 0)
			p_output_string(output, "");
		else
		{
			char	*buf;

			if(start >= slen)
				start = slen - 1;
			if(length > slen - start)
				length = slen - start;
			buf = malloc(length + 1);		/* Use strdup()? Naah. */
			strncpy(buf, str + start, length);
			buf[length] = '\0';
			printf(" strcut output '%s'\n", buf);
			p_output_string(output, buf);
			free(buf);
		}
	}
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("str-length");
	p_init_input(0, P_VALUE_STRING, "str", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_meta("desc/purpose", "Compute length of string (number of characters)");
	p_init_compute(strlength_compute);

	p_init_create("str-join");
	p_init_input(0, P_VALUE_STRING, "str1", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_STRING, "str2", P_INPUT_DONE);
	p_init_meta("desc/purpose", "Join (concatenate) two strings, outputting the result.");
	p_init_compute(strjoin_compute);

	p_init_create("str-cut");
	p_init_input(0, P_VALUE_STRING, "str", P_INPUT_REQUIRED,
		     P_INPUT_DESC("The string to cut a substring from."), P_INPUT_DONE);
	p_init_input(1, P_VALUE_UINT32, "start",
		     P_INPUT_DESC("The position, counting from zero, of the first character to include in the cut-out substring. If this "
				  "exceeds the length of the string, the substring will be empty."), P_INPUT_DONE);
	p_init_input(2, P_VALUE_UINT32, "length", P_INPUT_DEFAULT(~0u), P_INPUT_MIN(1),
		     P_INPUT_DESC("The number of characters to include in the substring. Must be one or greater. If the nunber specified "
				  "exceeds the number of characters available at the starting point, the substring will end at the input's end."), P_INPUT_DONE);
	p_init_meta("desc/purpose", "Cut out a substring from a string, and output that.");
	p_init_compute(strcut_compute);
}
