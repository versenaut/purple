/*
 * Tests of the doubly-linked list module.
*/

#include <string.h>

#include "test.h"

#include "list.h"

#if 0
static void print_int(const List *l)
{
	for(; l != NULL; l = list_next(l))
		printf("%d ", (int) list_data(l));
}
#endif

static int cmp_int(const void *a, const void *b)
{
	int	ai = (int) a, bi = (int) b;

	return ai < bi ? -1 : ai > bi;
}

int main(void)
{
	List	*a, *b, *c, *l;

	test_package_begin("list", "Doubly linked list data structure with operations");
	list_init();

	test_begin("List allocation");
	{
		List	*a;

		a = list_new(NULL);
		test_result(a != NULL);
		list_destroy(a);
	}
	test_end();

	test_begin("Multiple list allocations");
	{
		a = list_new(NULL);
		b = list_new(NULL);
		c = list_new(NULL);
		test_result(a != b && b != c && a != c);
		list_destroy(a);
		list_destroy(b);
		list_destroy(c);
	}
	test_end();

	test_begin("Data storage");
	{
		a = list_new((void *) 4711);
		test_result(a != NULL && list_data(a) == (void *) 4711);
		list_destroy(a);
	}
	test_end();

	test_begin("Concatenation");
	{
		a = list_new((void *) 17);
		b = list_new((void *) 42);
		c = list_new((void *) 4711);

		l = list_concat(a, b);
		l = list_concat(l, c);
		test_result(list_prev(l) == NULL &&
			    list_next(l) == b &&
			    list_prev(b) == l &&
			    list_next(b) == c &&
			    list_prev(c) == b && 
			    list_next(c) == NULL);
		list_destroy(l);
	}
	test_end();
	
	test_begin("Indexing");
	{
		a = list_new("foo");
		b = list_new("bar");
		c = list_new("baz");

		l = list_concat(a, b);
		l = list_concat(l, c);

		test_result(list_index(a) == 0 &&
			    list_index(b) == 1 &&
			    list_index(c) == 2);
	}
	test_end();

	test_begin("Append integrity, n:th accessor");
	{
		l = list_append(NULL, (void *) 17);
		l = list_append(l, (void *) 42);
		l = list_append(l, (void *) 4711);
		a = list_nth(l, 0);
		b = list_nth(l, 1);
		c = list_nth(l, 2);

		test_result(list_prev(l) == NULL &&
			    list_next(l) == b &&
			    list_prev(b) == l &&
			    list_next(b) == c &&
			    list_prev(c) == b && 
			    list_next(c) == NULL);
		list_destroy(l);
	}
	test_end();

	test_begin("Length operator");
	{
		l = list_append(NULL, "hello");
		l = list_append(l, "this");
		l = list_append(l, "is");
		l = list_append(l, "a");
		l = list_append(l, "test");
		test_result(list_length(l) == 5);
		list_destroy(l);
	}
	test_end();
	
	test_begin("Insert");
	{
		int	ok = 0;

		l = list_insert_before(NULL, NULL, "foo");
		if(list_length(l) == 1)
		{
			l = list_insert_before(l, l, "bar");
			if(list_length(l) == 2)
			{
				l = list_insert_before(l, l, "baz");
				if(list_length(l) == 3)
				{
					a = list_nth(l, 0);
					b = list_nth(l, 1);
					c = list_nth(l, 2);
					ok = strcmp(list_data(a), "baz") == 0 &&
					     strcmp(list_data(b), "bar") == 0 &&
					     strcmp(list_data(c), "foo") == 0;
				}
			}
		}
		list_destroy(l);
		test_result(ok);
	}
	test_end();

	test_begin("Insert sorted");
	{
		int	ok = 0;

		l = list_insert_sorted(NULL, (void *) 4711, cmp_int);
		l = list_insert_sorted(l, (void *) 17, cmp_int);
		l = list_insert_sorted(l, (void *) 42, cmp_int);
		l = list_insert_sorted(l, (void *) 2, cmp_int);
		if(list_length(l) == 4)
		{
			ok = list_data(list_nth(l, 0)) == (void *) 2 &&
			     list_data(list_nth(l, 1)) == (void *) 17 &&
			     list_data(list_nth(l, 2)) == (void *) 42 &&
			     list_data(list_nth(l, 3)) == (void *) 4711;
		}
		list_destroy(l);
		test_result(ok);
	}
	test_end();

	test_begin("List reversal");
	{
		int	ok = 0;

		l = list_append(NULL, (void *) 1);
		l = list_append(l, (void *) 2);
		l = list_append(l, (void *) 3);
		l = list_append(l, (void *) 4);
		if(list_length(l) == 4)
		{
			l = list_reverse(l);
			ok = list_data(list_nth(l, 0)) == (void *) 4 &&
			     list_data(list_nth(l, 1)) == (void *) 3 &&
			     list_data(list_nth(l, 2)) == (void *) 2 &&
			     list_data(list_nth(l, 3)) == (void *) 1;
		}
		list_destroy(l);
		test_result(ok);
	}
	test_end();

	test_begin("First/last accessors");
	{
		char	*a = "a", *b = "b", *c = "c";
		int	ok = 0;

		l = list_append(NULL, a);
		l = list_append(l, b);
		l = list_append(l, c);
		if(list_length(l) == 3)
		{
			if(list_data(list_first(l)) == a && list_data(list_last(l)) == c)
				ok = 1;
		}
		list_destroy(l);
		test_result(ok);
	}
	test_end();

	test_begin("Find in sorted list");
	{
		int	ok = 0;

		l = list_append(NULL, (void *) 1);
		l = list_append(l, (void *) 11);
		l = list_append(l, (void *) 19);
		l = list_append(l, (void *) 22);
		l = list_append(l, (void *) 59);
		if(list_find_sorted(l, (const void *) 22, cmp_int) != NULL)
		{
			l = list_reverse(l);	/* Reverse list, find should now fail. */
			if(list_find_sorted(l, (const void *) 22, cmp_int) == NULL)
				ok = 1;
		}
		list_destroy(l);
		test_result(ok);
	}
	test_end();

	test_begin("Unlink");
	{
		int	ok = 0;

		a = list_new("this");
		b = list_new("is");
		c = list_new("testing");

		l = list_concat(a, b);
		l = list_concat(l, c);

		if(list_length(l) == 3)
		{
			l = list_unlink(l, c);
			if(list_length(l) == 2)
				ok = 1;
		}
		list_destroy(l);
		test_result(ok);
	}
	test_end();

	test_begin("Find custom");
	{
		int	ok = 0;

		l = list_new((void *) 4);
		l = list_append(l, (void *) 11);
		l = list_append(l, (void *) 17);
		l = list_append(l, (void *) 21);
		l = list_append(l, (void *) 44);
		ok = list_find_custom(l, (void *) 1, cmp_int) == NULL &&
			list_find_custom(l, (void *) 4, cmp_int) == l &&
			list_find_custom(l, (void *) 44, cmp_int) == list_last(l);
		list_destroy(l);
		test_result(ok);
	}
	test_end();

	return test_package_end();
}
