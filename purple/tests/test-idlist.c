/*
 *
*/

#include <string.h>

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

	return test_package_end();
}
