/*
 * Test the handy functions in the dynamic string module.
*/

#include <stdio.h>
#include <string.h>

#include "test.h"

#include "dynstr.h"

int main(void)
{
	DynStr	*a;

	test_package_begin("strutil", "String utility functions");

	test_begin("new()");
	{
		a = dynstr_new(NULL);
		test_result(a != NULL);
		dynstr_destroy(a, 1);
	}
	test_end();

	test_begin("new() with text");
	{
		a = dynstr_new("foo");
		test_result(a != NULL && strcmp(dynstr_string(a), "foo") == 0);
		dynstr_destroy(a, 1);
	}
	test_end();

	test_begin("length()");
	{
		const char	*test[] = { "", "a", "foo\nbar", "foo\vbar\tbaz" };
		int		i, ok = 0;

		for(i = 0; i < sizeof test / sizeof *test; i++)
		{
			a = dynstr_new(test[i]);
			ok += dynstr_length(a) == strlen(test[i]);
			dynstr_destroy(a, 1);
		}
		test_result(ok == sizeof test / sizeof *test);
	}
	test_end();

	test_begin("assign()");
	{
		a = dynstr_new("foo");
		dynstr_assign(a, "bar");
		test_result(a != NULL && strcmp(dynstr_string(a), "bar") == 0);
		dynstr_destroy(a, 1);
	}
	test_end();

	test_begin("append()");
	{
		a = dynstr_new_sized(16);
		dynstr_append(a, "f");
		dynstr_append(a, "o");
		dynstr_append(a, NULL);
		dynstr_append(a, "o");
		dynstr_append(a, "ba");
		dynstr_append(a, "");
		dynstr_append(a, "rb");
		dynstr_append(a, "az");
		test_result(strcmp(dynstr_string(a), "foobarbaz") == 0);
		dynstr_destroy(a, 1);
	}
	test_end();

	test_begin("append_c()");
	{
		a = dynstr_new_sized(16);
		dynstr_append_c(a, 'f');
		dynstr_append_c(a, 'o');
		dynstr_append_c(a, 'o');
		dynstr_append_c(a, '\0');
		dynstr_append_c(a, 'b');
		dynstr_append_c(a, 'a');
		dynstr_append_c(a, 'r');
		test_result(strcmp(dynstr_string(a), "foobar") == 0);
		dynstr_destroy(a, 1);
	}
	test_end();

	test_begin("printf() (simple test)");
	{
		int	ok = 0;

		a = dynstr_new(NULL);
		dynstr_printf(a, "%c%c%c=%u", 'f', 'o', 'o', 4711);
		if(strcmp(dynstr_string(a), "foo=4711") == 0)
		{
			dynstr_printf(a, "%s%s=%s%g", "", "foo", "", 4711.0);
			if(strcmp(dynstr_string(a), "foo=4711") == 0)
				ok = 1;
		}
		dynstr_destroy(a, 1);
		test_result(ok);
	}
	test_end();

	test_begin("append_printf()");
	{
		a = dynstr_new_sized(32);
		dynstr_append_printf(a, "%s", "f");
		dynstr_append_printf(a, "%c%s", 'o', "o=");
		dynstr_append_printf(a, NULL);
		dynstr_append_printf(a, "%s", "4");
		dynstr_append_printf(a, "%u", 71);
		dynstr_append_printf(a, "%c", '1');
		test_result(strcmp(dynstr_string(a), "foo=4711") == 0);
		dynstr_destroy(a, 1);
	}
	test_end();

	test_begin("trim()");
	{
		a = dynstr_new(" \n \n\v \r\v a test\t\t\n\n\r  \t\t ");
		dynstr_trim(a);
		test_result(strcmp(dynstr_string(a), "a test") == 0);
		dynstr_destroy(a, 1);
	}
	test_end();

	test_begin("truncate()");
	{
		int	ok = 0;

		a = dynstr_new("foobarbaz");
		dynstr_truncate(a, 3);
		if(strcmp(dynstr_string(a), "foo") == 0)
		{
			dynstr_truncate(a, -3);	/* Wraps. */
			if(strcmp(dynstr_string(a), "foo") == 0)
				ok = 1;
		}
		test_result(ok);
		dynstr_destroy(a, 1);
	}
	test_end();

	return test_package_end();
}
