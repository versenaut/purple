/*
 * A little something we call Purple.
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/select.h>

#include "verse.h"
#include "purple.h"

#include "cron.h"
#include "dynarr.h"
#include "dynlib.h"
#include "dynstr.h"
#include "filelist.h"
#include "hash.h"
#include "idset.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "memchunk.h"
#include "plugins.h"
#include "strutil.h"
#include "textbuf.h"
#include "xmlnode.h"
#include "xmlutil.h"

#include "nodedb.h"
#include "client.h"
#include "graph.h"

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

static void test_textbuf(void)
{
	TextBuf	*tb;

	tb = textbuf_new(32);
	textbuf_insert(tb, 100, "apapapa");
	printf("Contents: '%s'\n", textbuf_text(tb));

	textbuf_insert(tb, 3, "-a");
	textbuf_insert(tb, 7, "-a");
	printf("Contents: '%s'\n", textbuf_text(tb));

	textbuf_delete(tb, 4, 3);
	printf("Contents: '%s'\n", textbuf_text(tb));

	textbuf_insert(tb, 1000, "--monster--");
	printf("Contents: '%s'\n", textbuf_text(tb));

	textbuf_delete(tb, 2, 1000);
	textbuf_insert(tb, -1, "skalle");
	printf("Contents: '%s'\n", textbuf_text(tb));
	textbuf_delete(tb, 2, 2);
	printf("Contents: '%s'\n", textbuf_text(tb));
	textbuf_insert(tb, 2, "b");
	printf("Contents: '%s'\n", textbuf_text(tb));

	textbuf_delete(tb, 5, 100);
	printf("Contents: '%s'\n", textbuf_text(tb));

	textbuf_insert(tb, 100, "t");
	printf("Contents: '%s'\n", textbuf_text(tb));

	textbuf_delete(tb, 0, ~0);
	printf("Contents: '%s'\n", textbuf_text(tb));

	textbuf_destroy(tb);
}

static void test_xmlnode(void)
{
	XmlNode	*node;

/*	node = xmlnode_new("<?xml version=\"1.0\"?>\n"
		    "<this flavor='crazy &amp; dude'     quote='\"' \t\t\t less='more'>"
		    "<!-- ignore me, please -->\n"
		    "is a &quot;test&quot;"
		    "</this>");*/
	if((node = xmlnode_new("<a x='y' a='whiz' dork='fine' nrg='total' master='yes' fine='undo'>hello</a>")) != NULL)
	{
		printf("a=%s\n", xmlnode_attrib_get(node, "a"));
		printf("dork=%s\n", xmlnode_attrib_get(node, "dork"));
		xmlnode_print_outline(node);
		xmlnode_destroy(node);

/*		xmlnode_nodeset_get(node, XMLNODE_AXIS_CHILD, TEST_NAME("dork"), TEST_ATTRIB_VALUE("size", "30"));*/
	}
}

static void test_idset(void)
{
	IdSet		*is;
	const char	*a = "a", *b = "b", *c = "c";
	void		*p;
	unsigned int	id;

	is = idset_new(33);
	printf("inserting a at %p\n", a);
	idset_insert(is, a);
	idset_insert(is, b);
	idset_insert(is, c);
	for(id = idset_foreach_first(is); (p = idset_lookup(is, id)) != NULL; id = idset_foreach_next(is, id))
	{
		printf("%u: %s\n", id, (const char *) p);
	}
	printf("removing 34, 33\n");
	idset_remove(is, 34);
	idset_remove(is, 33);
	for(id = idset_foreach_first(is); (p = idset_lookup(is, id)) != NULL; id = idset_foreach_next(is, id))
	{
		printf("%u: %s\n", id, (const char *) p);
	}
	printf("re-inserting\n");
	idset_insert(is, b);
	idset_insert(is, a);
	for(id = idset_foreach_first(is); (p = idset_lookup(is, id)) != NULL; id = idset_foreach_next(is, id))
	{
		printf("%u: %s\n", id, (const char *) p);
	}

	idset_destroy(is);
}

static int cron_handler(void *data)
{
	static int	count = 3;

	printf("count: %d\n", --count);

	return count > 0;
}

static void console_parse_module_input_set(const char *line)
{
	char		tcode;
	const char	*literal,
			tsel[] = "bdurRs", *tpos;
	const PInputType tarr[] = {
		P_INPUT_BOOLEAN, P_INPUT_INT32, P_INPUT_UINT32, P_INPUT_REAL32, P_INPUT_REAL64,
		P_INPUT_STRING
	};
	char		string[1024];
	uint32		g, m, i, got;
	PInputType	type;
	PInputValue	value;

	if(sscanf(line, "mis%c %u %u %u", &tcode, &g, &m, &i) != 4 || (literal = strchr(line, ':')) == NULL)
	{
		printf("Syntax: mis<TYPE> <GRAPH> <MODULE> <INPUT> : <VALUE>\n");
		printf(" Valid types: 'r' -- real number\n");
		return;
	}
	literal++;

	if((tpos = strchr(tsel, tcode)) == NULL)
	{
		printf("Unknown type character '%c' in mis command--use one of %s\n", tcode, tsel);
		return;
	}
	value.type = tarr[tpos - tsel];

	switch(value.type)
	{
	case P_INPUT_BOOLEAN:
		got = sscanf(literal, "%u", &value.v.vboolean);
		break;
	case P_INPUT_INT32:
		got = sscanf(literal, "%d", &value.v.vint32);
		break;
	case P_INPUT_UINT32:
		got = sscanf(literal, "%u", &value.v.vuint32);
		break;
	case P_INPUT_REAL32:
		got = sscanf(literal, "%g", &value.v.vreal32);
		break;
	case P_INPUT_REAL64:
		got = sscanf(literal, "%lg", &value.v.vreal64);
		break;
	case P_INPUT_STRING:
		got = sscanf(literal, " \"%[^\"]\"", string);
		value.v.vstring = string;
		break;
	default:
		;
	}
	if(got == 1)
		graph_method_send_call_mod_input_set(g, m, i, &value);
	else
		printf("mis couldn't parse %s as type %c literal\n", literal, tcode);
}

static void console_update(void)
{
	int	fd;
	fd_set	fds;
	struct timeval	timeout;

	fd = fileno(stdin);
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	timeout.tv_sec  = 0;
	timeout.tv_usec = 1000;
	if(select(fd + 1, &fds, NULL, NULL, &timeout))
	{
		char	line[1024];
		int	got;

		if((got = read(fd, line, sizeof line)) > 0)
		{
			line[got] = '\0';
			while(got > 0 && isspace(line[got - 1]))
				line[--got] = '\0';
			if(strncmp(line, "gc ", 3) == 0)
			{
				VNodeID	node;
				uint16	buf;
				char	name[64];

				if(sscanf(line, "gc %u %u %s", &node, &buf, name) == 3)
					graph_method_send_call_create(node, buf, name);
			}
			else if(strncmp(line, "gd ", 3) == 0)
			{
				uint32	id;

				if(sscanf(line, "gd %u", &id) == 1)
					graph_method_send_call_destroy(id);
			}
			else if(strncmp(line, "mc ", 3) == 0)
			{
				uint32	g, p;

				if(sscanf(line, "mc %u %u", &g, &p) == 2)
					graph_method_send_call_mod_create(g, p);
			}
			else if(strncmp(line, "mis", 3) == 0)
				console_parse_module_input_set(line);
			else if(strncmp(line, "mic ", 4) == 0)
			{
				uint32	g, m, i;

				if(sscanf(line, "mic %u %u %u", &g, &m, &i) == 3)
					graph_method_send_call_mod_input_clear(g, m, i);
			}
			else if(strncmp(line, "nc ", 3) == 0)
			{
				if(strcmp(line + 3, "text") == 0)
					verse_send_node_create(0, V_NT_TEXT, client_info.avatar);
				else
					printf("Use \"nc text\" to create a new text node\n");
			}
			else if(strncmp(line, "ns ", 3) == 0)
			{
				VNodeID	node;

				if(sscanf(line, "ns %u", &node) == 1)
					verse_send_node_subscribe(node);
			}
			else if(strncmp(line, "nns ", 4) == 0)
			{
				VNodeID	node;
				char	name[64];

				if(sscanf(line, "nns %u %s", &node, name) == 2)
					verse_send_node_name_set(node, name);
			}
			else if(strncmp(line, "tbc ", 4) == 0)
			{
				VNodeID	node;
				char	name[32];

				if(sscanf(line, "tbc %u %s", &node, name) == 2)
					verse_send_t_buffer_create(node, ~0, 0, name);
			}
			else if(strncmp(line, "tbs ", 4) == 0)
			{
				VNodeID	node;
				VLayerID buffer;

				if(sscanf(line, "tbs %u %hu", &node, &buffer) == 2)
				{
					printf("sending subscribe to text buffer %u,%u\n", node, buffer);
					verse_send_t_buffer_subscribe(node, buffer);
				}
			}
			else if(strncmp(line, "tsl ", 4) == 0)
			{
				VNodeID	node;
				char	lang[32];

				if(sscanf(line, "tsl %u %s", &node, lang) == 2)
					verse_send_t_set_language(node, lang);
			}
			else
				printf("Input: '%s'\n", line);
		}
	}
}

int main(void)
{
	cron_init();
	dynarr_init();
	hash_init();
	list_init();
	plugins_init("/home/emil/data/projects/purple/plugins/");

	graph_init();

/*	test_chunk();
	test_list();
	test_dynarr();
	test_hash();
	test_filelist();
	test_strutil();
	test_dynstr();
	test_textbuf();
	test_xmlnode();
	test_idset();
	return 0;
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
			console_update();
		}
	}
	return 0;
}
