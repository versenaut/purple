/*
 * purple.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * A little something we call Purple.
*/

/** \mainpage
 * 
 * This is the "User's Manual" for Purple, a plug-in based computational engine for the
 * Verse platform. "Purple" is really just a codename for a project whose official title
 * is something along the lines of "D3.3 Script Environment API". Perhaps you can see why
 * it's easier to just use "Purple", sans quotes even, to refer to it.
 * 
 * \section cont Contents
 * - \subpage model
 * - \subpage devplugin
 * - \subpage devui
 * 
 * The above list of contents reflects the fact that, because of the nature of the Purple
 * application, it is possible to be a "user" in many different ways. These ways are not
 * entierly distinct, but they are still varying enough to warrant listing:
 * - Users who want to use the Purple system
 * - Developers who want to write plug-ins
 * - Developers who want to write user interfaces
 *
 * \section starting Getting Started
 * The first category of users will mainly interact with Purple through an implementation
 * of an interface client. Since this document describes the Purple technology and server
 * "backend" system, such users will not be helped much here.
 * 
 * The second category of users, developers interested in writing plug-ins, will find this
 * document to contain a lot of useful information, starting below and continuing with the
 * \subpage devplugin page.
 * 
 * The third category of users, developers who want to work on Purple interface clients,
 * should read the \subpage devui material.
 * 
 * Both categories of developer might do well to first read the \subpage model page first,
 * to pick up the basics.
 * 
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined _WIN32
 #include <windows.h>
 #include <Shlwapi.h>
#elif defined __APPLE_CC__
 #include <unistd.h>
#endif

#define	PURPLE_CONSOLE
#undef	PURPLE_CONSOLE

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
#include "resume.h"

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
		{
			const char	*get = strchr(literal, '"');
			char		*put = string;

			if(get != NULL)
			{
				get++;
				while(*get != '\0' && *get != '"')
				{
					if(*get == '\\')
					{
						get++;
						if(*get == 'n')
						{
							*put++ = '\n';
							get++;
						}
						else if(*get == '\\')
						{
							*put++ = '\\';
							get++;
						}
						else if(*get == '"')
						{
							*put++ = '"';
							get++;
						}
						else
							printf("Unsupported backslash code \%c in string input literal\n", *get);
					}
					else
						*put++ = *get++;
				}
				*put = '\0';
				got = 1;
			}
		}
		value.v.vstring = string;
		break;
	default:
		;
	}
	if(got == 1)
		graph_method_send_call_mod_input_set(g, m, i, type, &value);
	else
		printf("mis couldn't parse %s as type %c (%d) literal\n", literal, tcode[0], type);
}

/* Pre-scripted sequence of console commands, to cut down on typing needed to test stuff. Hm. */
static int console_script(char *line, size_t line_size)
{
	static const char script[] =
	"nc text\n"
	"tbc 2 purpletest\n"
	"gc 2 0 graph0\n"

	"mc 1 36\n"
	"mc 1 11\n"
	"mc 1 19\n"

	"mism 1 2 0 : 0\n"
	"mism 1 2 1 : 1\n"

	"misr 1 0 0 : 1\n"
	"misr 1 0 1 : 32\n"

	"misu 1 1 0 : 32\n"
	"misu 1 1 1 : 32\n"
	"misu 1 1 2 : 0\n";
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

#endif		/* PURPLE_CONSOLE */

static int console_update(void)
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
			else if(strcmp(line, "quit") == 0)
				return 0;
			else if(line[0] != '\0')
				printf("Input: '%s'\n", line);
		}
	}
#endif		/* PURPLE_CONSOLE */
	return 1;
}

/* Make the directory where the running executable lives the current one. This is platform-dependant code.
 * The idea is that plugins/ is typically next to the exeuctable, and we want to find them. On some plat-
 * forms (Mac OS X Finder, Win32 Explorer), the process' current working directory lies.
*/
static int goto_home_dir(const char *argv0)
{
#if defined _WIN32
	TCHAR	buf[1024];

	if(GetModuleFileName(NULL, buf, sizeof buf) < sizeof buf - 1)
	{
		if(PathRemoveFileSpec(buf) != 0)
		{
			if(_chdir(buf) == 0)
				return 1;
			else
				fprintf(stderr, "Purple: Couldn't make \"%s\" the current directory, plug-in loading might fail\n", buf);
		}
		else
			fprintf(stderr, "Purple: Couldn't run PathRemoveFileSpec(\"%s\")\n", buf);
	}
	else
		fprintf(stderr, "Purple: Couldn't GetModuleFileName()\n");
	return 0;
#elif defined __APPLE_CC__
	char	buf[1024], *slash;

	strcpy(buf, argv0);
	if((slash = strrchr(buf, '/')) != NULL)
	{
		*slash = '\0';	/* Clobber the last slash, turning "/my/great/place/for/purple" to "/my/great/place/for". */
		if(chdir(buf) == 0)
			return 1;
		else
			fprintf(stderr, "Purple: Couldn't make \"%s\" the current directory\n", buf);
	}
	else
		fprintf(stderr, "Purple: Couldn't find slash in \"%s\", can't extract directory", buf);
	return 0;
#endif
	return 1;
}

int main(int argc, char *argv[])
{
	const char	*server = "localhost";
	int		i;

	goto_home_dir(argv[0]);

	bintree_init();
	cron_init();
	dynarr_init();
	hash_init();
	list_init();
	plugins_init("plugins");

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
			if(!console_update())
				break;
			sched_update();
			sync_update(1.0);
		}
	}
	return EXIT_SUCCESS;
}
