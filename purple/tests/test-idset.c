/*
 *
*/

#include <string.h>

#include "verse.h"

#include <stdio.h>

#include "dynarr.h"
#include "list.h"

#include "test.h"

#include "idset.h"

int main(void)
{
	dynarr_init();
	list_init();

	test_package_begin("idset", "Set of objects idenfied by ID");

	test_begin("new");
	{
		IdSet	*is1, *is2;

		is1 = idset_new(0);
		is2 = idset_new(0);
		test_result(is1 != NULL && is2 != NULL && is1 != is2);
		idset_destroy(is1);
		idset_destroy(is2);
	}
	test_end();

	test_begin("insert");
	{
		IdSet		*is;
		unsigned int	i, j, k;

		is = idset_new(0);
		i = idset_insert(is, "i");
		j = idset_insert(is, "j");
		k = idset_insert(is, "k");
		test_result(i != j && j != k && i != k);	/* IDs must be unique. */
		idset_destroy(is);
	}
	test_end();

	test_begin("lookup");
	{
		IdSet		*is;
		unsigned int	i, j, k;
		const void	*io, *jo, *ko;

		is = idset_new(0);
		i = idset_insert(is, "i");
		j = idset_insert(is, "j");
		k = idset_insert(is, "k");
		io = idset_lookup(is, i);
		jo = idset_lookup(is, j);
		ko = idset_lookup(is, k);
		test_result(io != NULL && jo != NULL && ko != NULL &&
			    strcmp(io, "i") == 0 &&
			    strcmp(jo, "j") == 0 &&
			    strcmp(ko, "k") == 0);
		idset_destroy(is);
	}
	test_end();
	
	test_begin("insert+remove");
	{
		IdSet		*is;
		unsigned int	i, j, k;

		is = idset_new(0);
		i = idset_insert(is, "i");
		j = idset_insert(is, "j");
		idset_remove(is, i);
		k = idset_insert(is, "k");
		test_result(i != j && i == k);	/* Make sure ID is re-used. */
		idset_destroy(is);
	}
	test_end();

	test_begin("insert_with_id()");
	{
		IdSet		*is;
		unsigned int	i;

		is = idset_new(0);
		i = idset_insert_with_id(is, 10, "ten");
		test_result(i == 10);	/* Check that we got the expected ID. */
		idset_remove(is, i);
	}
	test_end();
	
	test_begin("insert_with_id()+insert()s");
	{
		IdSet		*is;
		unsigned int	i, j[5], jj, jk, ok = 0;

		is = idset_new(0);
		i = idset_insert_with_id(is, sizeof j / sizeof *j, "foo");
		for(jj = 0; jj < sizeof j / sizeof *j; jj++)
			j[jj] = idset_insert(is, "a j");
		/* Verify that insert()s are all distinct, and below with_id(). */
		ok = i == sizeof j / sizeof *j;
		for(jj = 0; jj < sizeof j / sizeof *j; jj++)
		{
			if(j[jj] >= sizeof j / sizeof *j)
				ok = 0;
			for(jk = 0; jk < sizeof j / sizeof *j; jk++)
			{
				if(jj == jk)
					continue;
				if(j[jj] == j[jk])
					ok = 0;
			}
		}
		test_result(ok);
		idset_destroy(is);
	}
	test_end();

	test_begin("insert_with_id() twice");
	{
		IdSet		*is;
		unsigned int	i, j;

		is = idset_new(0);
		i = idset_insert_with_id(is, 10, "ten");
		j = idset_insert_with_id(is,  5, "five");
		test_result(i == 10 && j == 5);
		idset_destroy(is);
	}
	test_end();

	test_begin("new() with offset");
	{
		IdSet		*is;
		unsigned int	i;

		is = idset_new(10);
		i = idset_insert_with_id(is, 22, "foo");
		test_result(i == 22 && strcmp(idset_lookup(is, 22), "foo") == 0);
		idset_destroy(is);
	}
	test_end();

	return test_package_end();
}
