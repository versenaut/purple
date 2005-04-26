/*
 * purple.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * A little something we call Purple.
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	PURPLE_CONSOLE
/*#undef	PURPLE_CONSOLE*/

#if defined PURPLE_CONSOLE

/* FIXME: The console is not very portable. */
#include <sys/select.h>
#include <unistd.h>

#endif

#include "verse.h"
#include "purple.h"

#include "bintree.h"
#include "cron.h"
#include "dynarr.h"
#include "dynlib.h"
#include "filelist.h"
#include "hash.h"
#include "idlist.h"
#include "idset.h"
#include "log.h"
#include "mem.h"
#include "memchunk.h"
#include "plugins.h"
#include "scheduler.h"
#include "synchronizer.h"
#include "textbuf.h"
#include "value.h"
#include "xmlnode.h"
#include "xmlutil.h"

#include "nodedb.h"
#include "client.h"
#include "graph.h"

/*#include "command-structs.h"*/

static void test_chunk(void)
{
	MemChunk	*c;
	struct vertex {
		float x, y, z;
	} *v1, *v2, *v[16];
	unsigned int	i;

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

static int foreach_test(const void *data, void *user)
{
	printf("Here's '%s'\n", (const char *) data);
	return 1;
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

#if 0
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

static void test_xmlnode(void)
{
	const char	*graph =
	"<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>\n"
	"<purple-graphs>\n"
	 "<graph id=\"1\" name=\"busta\">\n"
	  "<at>\n"
	   "<node>Text_Node_2</node>\n"
	   "<buffer name=\"sod\">0</buffer>\n"
	  "</at>\n"
	 "</graph>\n"
	"</purple-graphs>\n";
	XmlNode	*root;

	if((root = xmlnode_new(graph)) != NULL)
	{
		printf("got it\n");
		printf("node: '%s'\n", xmlnode_eval_single(root, "graph/at/node"));
		printf("name: '%s'\n", xmlnode_eval_single(root, "graph/at/buffer/@name"));
		xmlnode_destroy(root);
	}
	exit(0);
}
#endif

static void test_idset(void)
{
	IdSet		*is;
	char		*a = "a", *b = "b", *c = "c";
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

static int tree_compare(const void *k1, const void *k2)
{
	return k1 < k2 ? -1 : k1 > k2;
}

static void test_bintree(void)
{
	BinTree	*tree;

	tree = bintree_new(tree_compare);

	bintree_insert(tree, (void *) 8, "eight");
	bintree_insert(tree, (void *) 7, "seven");
	bintree_insert(tree, (void *) 2, "two");
	bintree_insert(tree, (void *) 6, "six");
	bintree_insert(tree, (void *) 11, "eleven");
	bintree_insert(tree, (void *) 9, "nine");
	bintree_insert(tree, (void *) 1, "one");
	bintree_print(tree);
	printf("size: %u\n", bintree_size(tree));
	printf("\n");
	bintree_remove(tree, (void *) 8);
	bintree_remove(tree, (void *) 6);
	bintree_remove(tree, (void *) 1);
	bintree_print(tree);
	printf("size: %u\n", bintree_size(tree));

	bintree_destroy(tree, NULL);
}

#if defined PURPLE_CONSOLE

static void console_parse_module_input_set(const char *line)
{
	char		tcode[4];
	const char	*literal,
			tsel[] = "bdurRms", *tpos;
	const PValueType tarr[] = {
		P_VALUE_BOOLEAN, P_VALUE_INT32, P_VALUE_UINT32, P_VALUE_REAL32, P_VALUE_REAL64,
		P_VALUE_MODULE, P_VALUE_STRING
	};
	char		string[1024];
	uint32		g, m, i, got;
	PValueType	type;
	PValue		value;

	if(sscanf(line, "mis%c%c%u %u %u", tcode, tcode + 1, &g, &m, &i) != 5 || (literal = strchr(line, ':')) == NULL)
	{
		printf("Syntax: mis<TYPE> <GRAPH> <MODULE> <INPUT> : <VALUE>\n");
		return;
	}
	literal++;

	if((tpos = strchr(tsel, tcode[0])) == NULL)
	{
		printf("Unknown type character '%c' in mis command--use one of %s\n", tcode[0], tsel);
		return;
	}
	type = tarr[tpos - tsel];
	if(type == P_VALUE_REAL32 || type == P_VALUE_REAL64)
	{
		if(tcode[1] == ' ')			/* No modifier? */
			;
		else if(tcode[1] == '2')		/* Vector? */
			type += 1;
		else if(tcode[1] == '3')
			type += 2;
		else if(tcode[1] == '4')
			type += 3;
		else if(tcode[1] == 'm')
			type += 4;		/* Matrix. */
		else
		{
			printf("Illegal vector/matrix code %c\n", tcode[1]);
			return;
		}
	}

	switch(type)
	{
	case P_VALUE_BOOLEAN:
		{
			unsigned int	tmp;
			got = sscanf(literal, "%u", &tmp) == 1;
			value.v.vboolean = tmp;
		}
		break;
	case P_VALUE_INT32:
		got = sscanf(literal, "%d", &value.v.vint32) == 1;
		break;
	case P_VALUE_UINT32:
		got = sscanf(literal, "%u", &value.v.vuint32) == 1;
		break;
	case P_VALUE_REAL32:
		got = sscanf(literal, "%g", &value.v.vreal32) == 1;
		break;
	case P_VALUE_REAL32_VEC2:
		got = sscanf(literal, "%g %g", &value.v.vreal32_vec2[0], &value.v.vreal32_vec2[1]) == 2;
		break;
	case P_VALUE_REAL32_VEC3:
		got = sscanf(literal, "%g %g %g", &value.v.vreal32_vec3[0], &value.v.vreal32_vec3[1],
			     &value.v.vreal32_vec3[2]) == 3;
		break;
	case P_VALUE_REAL32_VEC4:
		got = sscanf(literal, "%g %g %g %g", &value.v.vreal32_vec4[0], &value.v.vreal32_vec4[1],
			     &value.v.vreal32_vec4[2], &value.v.vreal32_vec4[3]) == 4;
		break;
	case P_VALUE_REAL32_MAT16:
		got = sscanf(literal, "%g %g %g %g "
			     " %g %g %g %g"
			     " %g %g %g %g"
			     " %g %g %g %g",
			     &value.v.vreal32_mat16[0], &value.v.vreal32_mat16[1],
			     &value.v.vreal32_mat16[2], &value.v.vreal32_mat16[3],
     			     &value.v.vreal32_mat16[4], &value.v.vreal32_mat16[5],
			     &value.v.vreal32_mat16[6], &value.v.vreal32_mat16[7],
			     &value.v.vreal32_mat16[8], &value.v.vreal32_mat16[9],
			     &value.v.vreal32_mat16[10], &value.v.vreal32_mat16[11],
			     &value.v.vreal32_mat16[12], &value.v.vreal32_mat16[13],
			     &value.v.vreal32_mat16[14], &value.v.vreal32_mat16[15]);
		if(got > 0)	/* Duplicate supplied values into vacant matrix positions, if any. */
		{
			int	i, j;

			for(i = got, j = 0; i < 16; i++, j = (j + 1) % got)
				value.v.vreal32_mat16[i] = value.v.vreal32_mat16[j];
			got = 1;
		}
		break;
	case P_VALUE_REAL64:
		got = sscanf(literal, "%lg", &value.v.vreal64) == 1;
		break;
	case P_VALUE_REAL64_VEC2:
		got = sscanf(literal, "%lg %lg", &value.v.vreal64_vec2[0], &value.v.vreal64_vec2[1]) == 2;
		break;
	case P_VALUE_REAL64_VEC3:
		got = sscanf(literal, "%lg %lg %lg", &value.v.vreal64_vec3[0], &value.v.vreal64_vec3[1],
			     &value.v.vreal64_vec3[2]) == 3;
		break;
	case P_VALUE_REAL64_VEC4:
		got = sscanf(literal, "%lg %lg %lg %lg", &value.v.vreal64_vec4[0], &value.v.vreal64_vec4[1],
			     &value.v.vreal64_vec4[2], &value.v.vreal64_vec4[3]) == 4;
		break;
	case P_VALUE_REAL64_MAT16:
		got = sscanf(literal, "%lg %lg %lg %lg "
			     " %lg %lg %lg %lg"
			     " %lg %lg %lg %lg"
			     " %lg %lg %lg %lg",
			     &value.v.vreal64_mat16[0], &value.v.vreal64_mat16[1],
			     &value.v.vreal64_mat16[2], &value.v.vreal64_mat16[3],
     			     &value.v.vreal64_mat16[4], &value.v.vreal64_mat16[5],
			     &value.v.vreal64_mat16[6], &value.v.vreal64_mat16[7],
			     &value.v.vreal64_mat16[8], &value.v.vreal64_mat16[9],
			     &value.v.vreal64_mat16[10], &value.v.vreal64_mat16[11],
			     &value.v.vreal64_mat16[12], &value.v.vreal64_mat16[13],
			     &value.v.vreal64_mat16[14], &value.v.vreal64_mat16[15]);
		if(got > 0)	/* Duplicate supplied values into vacant matrix positions, if any. */
		{
			int	i, j;

			for(i = got, j = 0; i < 16; i++, j = (j + 1) % got)
				value.v.vreal64_mat16[i] = value.v.vreal64_mat16[j];
			got = 1;
		}
		break;
	case P_VALUE_MODULE:
		got = sscanf(literal, "%u", &value.v.vmodule) == 1;
		break;
	case P_VALUE_STRING:
		got = sscanf(literal, " \"%[^\"]\"", string) == 1;
		value.v.vstring = string;
		break;
	default:
		;
	}
	if(got == 1)
		graph_method_send_call_mod_input_set(g, m, i, type, &value);
	else
		printf("mis couldn't parse %s as type %c literal\n", literal, tcode[0]);
}
#endif		/* PURPLE_CONSOLE */

/* Pre-scripted sequence of console commands, to cut down on typing needed to test stuff. Hm. */
static int console_script(char *line, size_t line_size)
{
	static const char script[] =
	"nc text\n"
	"tbc 2 klax\n"
	"gc 2 0 busta\n"

	"mc 1 27\n" /* 0: Plane. */
	"mc 1 6\n"  /* 1: Bbox. */
	"mc 1 34\n" /* 2: Warp. */
	"mc 1 2\n"  /* 3: Output. */
	"mism 1 3 0 : 2\n"
	"mism 1 2 0 : 0\n"
	"mism 1 2 1 : 1\n"
	"misr 1 2 2 : 0.1\n"
	"mism 1 1 0 : 0\n"
	"misr 1 0 0 : 10.0\n"
	"misu 1 0 1 : 3\n";
	static int	next_line = 0;

	if(strcmp(line, ".") == 0)
	{
		const char	*sline = script;
		int		i, curline = 0;

		for(i = 0; script[i] != '\0'; i++)
		{
			if(script[i] == '\n')
			{
				if(curline == next_line)
				{
					next_line++;
					memcpy(line, sline, (script + i - sline));
					line[script + i - sline] = '\0';
					printf("script: '%s'\n", line);
					return 1;
				}
				curline++;
			}
			else if(i > 0 && script[i - 1] == '\n')
				sline = script + i;
		}
	}
	return 0;
}

static void console_update(void)
{
#if defined PURPLE_CONSOLE
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
			console_script(line, sizeof line);
			if(strncmp(line, "gc ", 3) == 0)
			{
				VNodeID	node;
				uint16	buf;
				char	name[64];

				if(sscanf(line, "gc %u %hu %s", &node, &buf, name) == 3)
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
			else if(strncmp(line, "md ", 3) == 0)
			{
				uint32	g, p;

				if(sscanf(line, "md %u %u", &g, &p) == 2)
					graph_method_send_call_mod_destroy(g, p);
			}
			else if(strncmp(line, "mis", 3) == 0)
				console_parse_module_input_set(line);
			else if(strncmp(line, "mic ", 4) == 0)
			{
				uint32	g, m, i;

				if(sscanf(line, "mic %u %u %u", &g, &m, &i) == 3)
					graph_method_send_call_mod_input_clear(g, m, i);
			}
			else if(strncmp(line, "ml", 2) == 0)
			{
				unsigned int	i;
				Plugin		*p;

				for(i = 1; (p = plugin_lookup(i)) != NULL; i++)
					printf("%2d: %s\n", i, plugin_name(p));
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
					verse_send_t_buffer_create(node, ~0, name);
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
			else if(line[0] != '\0')
				printf("Input: '%s'\n", line);
		}
	}
#endif		/* PURPLE_CONSOLE */
}

int main(int argc, char *argv[])
{
	const char	*server = "localhost";
	int		i;

	bintree_init();
	cron_init();
	dynarr_init();
	hash_init();
	list_init();
	plugins_init("plugins/");

	graph_init();
	
	plugins_libraries_load();
	plugins_libraries_init();
	plugins_build_xml();

	for(i = 1; argv[i] != NULL; i++)
	{
		if(strncmp(argv[i], "-ip=", 4) == 0)
			server = argv[i] + 4;
		else if(strcmp(argv[i], "-resume") == 0 || strncmp(argv[i], "-resume=", 9) == 0)
			resume_init(argv[i][7] == '=' ? argv[i] + 8 : NULL);
	}

	client_init();
	sync_init();

	printf("Connecting to Verse server at %s\n", server);
	if(client_connect(server))
	{
		LOG_MSG(("------------------------------------------------------------------------"));
		LOG_MSG(("Purple running on Verse r%up%u%s", V_RELEASE_NUMBER, V_RELEASE_PATCH, V_RELEASE_LABEL));
		LOG_MSG(("Entering main loop"));
		for(;;)
		{
			verse_callback_update(10000);
			cron_update();
			console_update();
			sched_update();
			sync_update(1.0);
		}
	}
	return EXIT_SUCCESS;
}
