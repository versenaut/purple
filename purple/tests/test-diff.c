/*
 * Test the handy functions in the diff module.
*/

#include <stdio.h>
#include <string.h>

#include "test.h"

#include "dynarr.h"

#include "diff.h"

static void print_diff_edits(const DynArr *diff)
{
	const char	*opname[] = { "<none>", "MATCH", "DELETE", "INSERT" };
	unsigned int	i;

	for(i = 0; i < dynarr_size(diff); i++)
	{
		const DiffEdit	*e = dynarr_index(diff, i);

		if(e == NULL)
			break;
		printf("%u: %s %u %u\n", i, opname[e->op], e->off, e->len);
	}
}

int main(void)
{
	int	d;
	DynArr	*diff;

	dynarr_init();
	diff = dynarr_new(sizeof (DiffEdit), 8);

	test_package_begin("diff", "General-purpose comparison engine");

	test_begin("no-op");
	d = diff_compare_simple(NULL, 0, NULL, 0,  NULL);
	test_result(d == 0);
	test_end();
	
	test_begin("empty string");
	d = diff_compare_simple("", 1,  "", 1,  NULL);
	test_result(d == 0);
	test_end();

	test_begin("single insert");
	d = diff_compare_simple(NULL, 0,  "a", 1,  NULL);
	test_result(d == 1);
	test_end();
	
	test_begin("single delete");
	d = diff_compare_simple("a", 1,  NULL, 0,  NULL);
	test_result(d == 1);
	test_end();

	test_begin("single letter");
	d = diff_compare_simple("a", 1,  "b", 1,   NULL);
	test_result(d == 2);		/* Needs delete + insert to change. */
	test_end();

	test_begin("word insert");
	d = diff_compare_simple("this day", strlen("this day"),  "this fine day", strlen("this fine day"),  diff);
	test_result(d == 5);
	test_end();

	dynarr_destroy(diff);

	return test_package_end();
}
