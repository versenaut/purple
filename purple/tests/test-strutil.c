/*
 * Test the handy functions in the strutil module.
*/

#include <stdio.h>
#include <string.h>

#include "test.h"

#include "mem.h"
#include "strutil.h"

int main(void)
{
	test_package_begin("strutil", "String utility functions");

	test_begin("strdup()");
	{
		char	*a, *b;

		a = stu_strdup("foo");
		b = stu_strdup("foo");
		/* Pointers should be distinct, strings equal. */
		test_result(a != b && strcmp(a, "foo") == 0 && strcmp(b, "foo") == 0 && strcmp(a, b) == 0);
		mem_free(a);
		mem_free(b);
	}
	test_end();

	test_begin("strdup() corners");
	{
		char	*a, *b;
		int	ok = 0;

		a = stu_strdup(NULL);
		b = stu_strdup(NULL);
		if(a == NULL && b == NULL)
		{
			a = strdup("");
			b = strdup("");
			if(a != b && a != NULL && b != NULL && *a == *b && *a == '\0')
				ok = 1;
			mem_free(a);
			mem_free(b);
		}
		test_result(ok);
	}
	test_end();

	test_begin("strdup() with maxlen");
	{
		char	*a;

		a = stu_strdup_maxlen("foobar", 3);
		test_result(strcmp(a, "fo") == 0);
	}
	test_end();

	test_begin("strncpy()");
	{
		char	dest[4],
			src[4] = "HELO";	/* Does not get terminator. */

		stu_strncpy(dest, sizeof dest, src);	/* Truncates source to fit. */
		test_result(strcmp(dest, "HEL") == 0);
	}
	test_end();

	test_begin("split()");
	{
		char	**arg;
		int	ok = 0;

		if((arg = stu_split("this|is|a|test", '|')) != NULL)
		{
			if(strcmp(arg[0], "this") == 0 &&
			   strcmp(arg[1], "is") == 0 &&
			   strcmp(arg[2], "a") == 0 &&
			   strcmp(arg[3], "test") == 0 &&
			   arg[4] == NULL)
				ok = 1;
			mem_free(arg);
		}
		test_result(ok);
	}
	test_end();

	return test_package_end();
}
