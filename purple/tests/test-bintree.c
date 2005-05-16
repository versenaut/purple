/*
 * Test the binary tree module.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

#include "bintree.h"

/* Binary tree key comparison function. Keys are really ints. */
static int compare(const void *k1, const void *k2)
{
	return k1 < k2 ? -1 : k1 > k2;
}

int main(void)
{
	bintree_init();

	test_package_begin("bintree", "Binary tree");

	test_begin("Creation");
	{
		BinTree	*bt;

		bt = bintree_new(compare);
		test_result(bt != NULL);
		bintree_destroy(bt, NULL);
	}
	test_end();

	test_begin("Insertion");
	{
		BinTree	*bt;

		bt = bintree_new(compare);
		bintree_insert(bt, (void *) 8, "eight");
		test_result(bintree_size(bt) == 1);
		bintree_destroy(bt, NULL);
	}
	test_end();

	test_begin("Serious insertion");
	{
		BinTree	*bt;

		bt = bintree_new(compare);
		bintree_insert(bt, (void *) 3, "three");
		bintree_insert(bt, (void *) 5, "five");
		bintree_insert(bt, (void *) 2, "two");
		bintree_insert(bt, (void *) 4, "four");
		test_result(bintree_size(bt) == 4);
		bintree_destroy(bt, NULL);
	}
	test_end();

	test_begin("Insert and remove");
	{
		BinTree	*bt;

		bt = bintree_new(compare);
		bintree_insert(bt, (void *) 3, "three");
		bintree_insert(bt, (void *) 5, "five");
		bintree_insert(bt, (void *) 2, "two");
		bintree_insert(bt, (void *) 4, "four");
		bintree_remove(bt, (void *) 5);
		bintree_remove(bt, (void *) 2);
		test_result(bintree_size(bt) == 2);
	}
	test_end();

	test_begin("Finding");
	{
		BinTree	*bt;

		bt = bintree_new(compare);
		bintree_insert(bt, (void *) 7, "seven");
		bintree_insert(bt, (void *) 2, "two");
		bintree_insert(bt, (void *) 5, "five");
		bintree_insert(bt, (void *) 1, "one");
		bintree_insert(bt, (void *) 0, "zero");
		bintree_insert(bt, (void *) 6, "six");
		bintree_insert(bt, (void *) 3, "three");
		bintree_insert(bt, (void *) 4, "four");
		test_result(strcmp(bintree_lookup(bt, (void *) 1), "one") == 0 &&
			    strcmp(bintree_lookup(bt, (void *) 6), "six") == 0);
	}
	test_end();

	return test_package_end();
}
