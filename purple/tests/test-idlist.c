/*
 *
*/

#include "verse.h"

#include <stdio.h>

#include "test.h"

#include "idlist.h"

int main(void)
{
	list_init();

	test_package_begin("idlist", "Multiset of IDs");

	test_begin("new");
	{
		IdList	*il1, *il2;

		il1 = idlist_new();
		il2 = idlist_new();
		test_result(il1 != NULL && il2 != NULL && il1 != il2);
		idlist_destroy(il1);
		idlist_destroy(il2);
	}
	test_end();

	test_begin("insert");
	{
		IdList	*il;

		il = idlist_new();
		idlist_insert(il, 3);
		idlist_insert(il, 18);
		idlist_insert(il, 11);
		idlist_insert(il, 22);
		idlist_insert(il, 3);
		idlist_insert(il, 22);
		idlist_insert(il, 22);
		idlist_insert(il, 11);
		idlist_test_as_string(il, NULL, 0);
		idlist_remove(il, 22);
		idlist_remove(il, 18);
		idlist_remove(il, 22);
		idlist_remove(il, 22);
		idlist_test_as_string(il, NULL, 0);
		test_result(1);		/* FIXME: Crap, obviously. */
		idlist_destroy(il);
	}
	test_end();

	return test_package_end();
}
