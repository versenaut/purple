/*
 * Tests of the hash table module.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

#include "hash.h"

static unsigned int int_hash(const void *key)
{
	return (unsigned int) key;
}

static int int_key_eq(const void *key1, const void *key2)
{
	return (unsigned int) key1 == (unsigned int) key2;
}

int main(void)
{
	test_package_begin("hash", "Externally linked hash table data structure with operations");

	hash_init();

	test_begin("Hash allocation");
	{
		Hash	*a;

		a = hash_new(int_hash, int_key_eq);
		test_result(a != NULL);
		hash_destroy(a);
	}
	test_end();

	test_begin("Multiple allocs");
	{
		Hash	*a, *b;

		a = hash_new(int_hash, int_key_eq);
		b = hash_new(int_hash, int_key_eq);
		test_result(a != NULL && b != NULL && a != b);
		hash_destroy(b);
		hash_destroy(a);
	}
	test_end();

	test_begin("Insert/lookup");
	{
		Hash	*a;
		const char	*t;

		a = hash_new(int_hash, int_key_eq);
		hash_insert(a, (const void *) 17, "foo");
		test_result((t = hash_lookup(a, (const void *) 17)) != NULL &&
			    strcmp(t, "foo") == 0);
		hash_destroy(a);
	}
	test_end();

	test_begin("Resize");
	{
		Hash		*a;
		const char	*t;
		unsigned int	i;

		a = hash_new(int_hash, int_key_eq);
		hash_insert(a, (const void *) 4711, "foo");
		for(i = 0; i < 100; i++)
			hash_insert(a, (const void *) (3519 + i), "test-data");
		test_result((t = hash_lookup(a, (const void *) 4711)) != NULL &&
			    strcmp(t, "foo") == 0);
		hash_destroy(a);
	}
	test_end();

	return test_package_end();
}
