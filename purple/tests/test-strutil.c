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

	test_begin("strdup");
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

	test_begin("strncpy");
	{
		char	dest[4],
			src[4] = "HELO";	/* Does not get terminator. */

		stu_strncpy(dest, sizeof dest, src);	/* Truncates source to fit. */
		test_result(strcmp(dest, "HEL") == 0);
	}
	test_end();

	return test_package_end();
}
