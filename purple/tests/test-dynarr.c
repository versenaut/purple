/*
 * 
*/

#include <stdio.h>

#include "test.h"

#include "dynarr.h"

static void cb_copy(void *dst, const void *src, void *user)
{
	const int	*s = src;
	int		*d = dst;

	*d = (*s) + 1;
}

int main(void)
{
	test_package_begin("dynarr", "Dynamic array data type");

	dynarr_init();

	test_begin("new()");
	{
		DynArr	*a, *b;

		a = dynarr_new(32, 4);
		b = dynarr_new(72, 8);
		test_result(a != NULL && b != NULL && a != b);
		dynarr_destroy(a);
		dynarr_destroy(b);
	}
	test_end();

	test_begin("insert");
	{
		DynArr	*a;
		int	data = 4711, *test;

		a = dynarr_new(sizeof data, 8);
		dynarr_set(a, 0, &data);
		test = dynarr_index(a, 0);
		test_result(test != NULL && *test == data);
		dynarr_destroy(a);
	}
	test_end();

	test_begin("default");
	{
		DynArr	*a;
		int	data = 42, def = 4711, *test, ok = 0, i;

		a = dynarr_new(sizeof data, 8);
		dynarr_set_default(a, &def);
		dynarr_set(a, 3, &data);
		for(i = 0; i < 4; i++)
		{
			if((test = dynarr_index(a, i)) != NULL)
				ok += (i == 3) ? *test == data : *test == def;
		}
		test_result(ok == 4);
		dynarr_destroy(a);
	}
	test_end();

	test_begin("new_copy()");
	{
		DynArr	*a, *b;
		int	data = 0xdeadf00d, def = 0xcafe1ad1, *test, ok, i;

		a = dynarr_new(sizeof data, 4);
		dynarr_set_default(a, &def);
		dynarr_set(a, 5, &data);
		b = dynarr_new_copy(a, NULL, NULL);
		dynarr_set(b, 7, &data);
		for(ok = 0, i = 0; i < 8; i++)
		{
			if((test = dynarr_index(b, i)) != NULL)
				ok += (i == 5 || i == 7) ? *test == data : *test == def;
		}
		test_result(ok == 8);
	}
	test_end();

	test_begin("new_copy() with callback");
	{
		DynArr	*a, *b;
		int	data = 0xdeadf00d, def = 0xcafe1ad1, *test, ok, i;

		a = dynarr_new(sizeof data, 4);
		dynarr_set_default(a, &def);
		dynarr_set(a, 5, &data);
		b = dynarr_new_copy(a, cb_copy, NULL);
		dynarr_set(b, 7, &data);
		for(ok = 0, i = 0; i < 8; i++)
		{
			if((test = dynarr_index(b, i)) != NULL)
			{
				if(i < 5)
					ok += *test == def + 1;
				else if(i == 5)
					ok += *test == data + 1;
				else if(i == 6)
					ok += *test == def;
				else if(i == 7)
					ok += *test == data;
			}
		}
		test_result(ok == 8);
	}
	test_end();

	test_begin("new_copy() without callback");
	{
		DynArr	*a, *b;
		int	*test, ok, i;

		a = dynarr_new(sizeof i, 4);
		for(i = 0; i < 10; i++)
			dynarr_set(a, i, &i);
		b = dynarr_new_copy(a, NULL, NULL);
		for(ok = 0, i = 0; i < 10; i++)
		{
			if((test = dynarr_index(b, i)) != NULL)
				ok += *test == i;
		}
		test_result(ok == 10);
	}
	test_end();
	
	return test_package_end();
}
