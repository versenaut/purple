/*
 * A little something we call Purple.
*/

#include <stdio.h>
#include <stdlib.h>

#include "verse.h"

#include "client.h"
#include "cron.h"
#include "dynarr.h"
#include "dynlib.h"
#include "dynstr.h"
#include "filelist.h"
#include "hash.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "memchunk.h"
#include "plugins.h"
#include "strutil.h"
#include "xmlutil.h"

#include "command-structs.h"

static void test_chunk(void)
{
	MemChunk	*c;
	struct vertex {
		float x, y, z;
	} *v1, *v2, *v[16];
	int	i;

	c = memchunk_new("test", sizeof (struct vertex), 16);
	v1 = memchunk_alloc(c);
	v2 = memchunk_alloc(c);
	printf("v1=%p v2=%p\n", v1, v2);
	memchunk_free(c, v2);
	memchunk_free(c, v1);
	v1 = memchunk_alloc(c);
	v2 = memchunk_alloc(c);
	printf("v1=%p v2=%p\n", v1, v2);
	memchunk_free(c, v1);
	memchunk_free(c, v2);

	for(i = 0; i < sizeof v / sizeof *v; i++)
		v[i] = memchunk_alloc(c);
	for(i = 0; i < sizeof v / sizeof *v; i++)
		memchunk_free(c, v[i]);

	memchunk_destroy(c);
}

static int cb_print_int(void *data, void *userdata)
{
	printf(" %d", (int) data);
	return 1;
}

static void test_list(void)
{
	List	*ints = NULL;
	List	*n;
	size_t	i;

	list_init();
	ints = list_append(ints, (void *) 0);
	ints = list_append(ints, (void *) 1);
	ints = list_append(ints, (void *) 2);
	ints = list_append(ints, (void *) 3);
	ints = list_append(ints, (void *) 4);
	printf("%u\n", list_length(ints));

	printf("[");
	list_foreach(ints, cb_print_int, NULL);
	printf(" ]\n");

	printf("testing unlink of head\n");
	ints = list_unlink(ints, ints);
	printf("[");
	list_foreach(ints, cb_print_int, NULL);
	printf(" ]\n");

	printf("testing unlink of head\n");
	ints = list_unlink(ints, ints);
	printf("[");
	list_foreach(ints, cb_print_int, NULL);
	printf(" ]\n");

	n = list_nth(ints, 3);
	printf("%p\n", list_data(n));
	n = list_last(ints);
	printf("%p\n", list_data(n));
	printf("%p\n", list_data(list_prev(n)));

	ints = list_reverse(ints);
	printf("[");
	list_foreach(ints, cb_print_int, NULL);
	printf(" ]\n");

	n = list_nth(ints, 3);
	printf("%p\n", list_data(n));
	i = list_index(n);
	printf("index: %u\n", i);
	n = list_last(ints);
	printf("%p\n", list_data(n));
	printf("%p\n", list_data(list_prev(n)));
}

static void test_dynarr(void)
{
	DynArr	*a;

	if((a = dynarr_new(sizeof (int), 4)) != NULL)
	{
		int	i;
		int	data[32];

		for(i = 0; i < sizeof data / sizeof *data; i++)
			data[i] = i;

		for(i = sizeof data / sizeof *data - 1; i >= 0; i--)
			dynarr_set(a, i, data + i);
		for(i = 0; i < sizeof data / sizeof *data; i++)
		{
			const void	*p = dynarr_index(a, i);

			if(p != NULL)
				printf("%d ", *(int *) p);
		}
		printf("\n");
	}
}

static int foreach_test(const void *data, void *user)
{
	printf("Here's '%s'\n", (const char *) data);
	return 1;
}

static void test_hash(void)
{
	Hash	*h;

	if((h = hash_new_string()) != NULL)
	{
		const char	*object = "raj raj\0monster", *obj2 = "monster";
		void		*p;

		hash_insert(h, object, object);
		hash_insert(h, obj2, obj2);
		p = hash_lookup(h, "raj raj");
		printf("hash: p=%p\n", p);
		hash_foreach(h, foreach_test, NULL);
		printf("size=%u, removing '%s'\n", hash_size(h), object);
		hash_remove(h, object);
		hash_foreach(h, foreach_test, NULL);
		printf("size=%u, removing '%s'\n", hash_size(h), obj2);
		hash_remove(h, obj2);
		hash_foreach(h, foreach_test, NULL);
		hash_destroy(h);
	}
}

static void test_filelist(void)
{
	FileList	*fl;
	size_t		i;

	fl = filelist_new(".", ".c");
	printf("Found %u filenames:\n", filelist_size(fl));
	for(i = 0; i < filelist_size(fl); i++)
		printf("%2u: '%s'\n", i, filelist_filename(fl, i));
	filelist_destroy(fl);
}

static void test_strutil(void)
{
	char	**p;
	int	i;

	p = stu_split("/home/emil/data/projects/purple/plugins/:/usr/share/purple/lib/plugins/", ':');
	for(i = 0; p[i]; i++)
		printf("%2d: '%s'\n", i, p[i]);
	mem_free(p);
}

static void test_dynstr(void)
{
	DynStr	*ds;

	ds = dynstr_new("");
	dynstr_append_c(ds, 'h');
	dynstr_append_c(ds, 'e');
	dynstr_append_c(ds, 'l');
	dynstr_append_c(ds, 'l');
	dynstr_append_c(ds, 'o');
	printf("string: '%s'\n", dynstr_string(ds));

	dynstr_assign(ds, "");
	xml_dynstr_append(ds, "\"me\" & my <friends>");
	printf("string: '%s'\n", dynstr_string(ds));

	dynstr_destroy(ds, TRUE);
}

static int cron_handler(void *data)
{
	static int	count = 3;

	printf("count: %d\n", --count);

	return count > 0;
}

int main(void)
{
	cron_init();
	dynarr_init();
	hash_init();
	list_init();
	plugins_init("/home/emil/data/projects/purple/plugins/");

/*	test_chunk();
	test_list();
	test_dynarr();
	test_hash();
	test_filelist();
	test_strutil();
	test_dynstr();
*/
	plugins_libraries_load();
	plugins_libraries_init();

	plugins_build_xml();

	client_init();

	if(client_connect("localhost"))
	{
		LOG_MSG(("------------------------------------------------------------------------"));
		LOG_MSG(("Entering main loop"));
		for(;;)
		{
/*			printf("Buffer: %u\n", verse_session_get_size());*/
			verse_callback_update(100);
			cron_update();
		}
	}
	return 0;
}
