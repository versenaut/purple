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

/*	test_begin("insert");
	{
		char	buf[256];
		IdList	*il;
		int	ok = 0;

		il = idlist_new();
		idlist_insert(il, 3);
		idlist_insert(il, 18);
		idlist_insert(il, 11);
		idlist_insert(il, 22);
		idlist_insert(il, 3);
		idlist_insert(il, 22);
		idlist_insert(il, 22);
		idlist_insert(il, 11);
		idlist_test_as_string(il, buf, sizeof buf);
		if(strcmp(buf, "[(3;2) (11;2) (18;1) (22;3)]") == 0)
		{
			idlist_remove(il, 22);
			idlist_remove(il, 18);
			idlist_remove(il, 22);
			idlist_remove(il, 22);
			idlist_test_as_string(il, buf, sizeof buf);
			ok = strcmp(buf, "[(3;2) (11;2)]") == 0;
		}
		test_result(ok);
		idlist_destroy(il);
	}
	test_end();
*/
	return test_package_end();
}
