/*
 * resume.c.
 * 
 * Copyright (C) 2005 PDC, KTH. See COPYING for license details.
 * 
 * Logic for "resuming" a Purple session, by finding whatever meta data
 * was left by a previous instance, and trying to replicate it into our
 * own state. Pretty hairy in places.
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"

#include "purple.h"

#include "cron.h"
#include "mem.h"
#include "nodedb.h"
#include "plugins.h"
#include "xmlnode.h"

#include "resume.h"

static struct
{
	int	enabled;		/* Set to 1 in init(), queried by resume_enabled(). */
	char	meta[32];
} resume_info = { 0 };

static int 	resume_update(void *data);

/* ----------------------------------------------------------------------------------------- */

void resume_init(const char *options)
{
	printf("Initializing resume-mode\n");
	resume_info.enabled = 1;
	strcpy(resume_info.meta, "PurpleMeta");
	cron_add(CRON_ONESHOT, 10.0, resume_update, NULL);
}

int resume_enabled(void)
{
	return resume_info.enabled;
}

/* ----------------------------------------------------------------------------------------- */

/* Create an array that for each ID in the previous instance's plug-in space, contains
 * the ID of *this* instance's equivalent plug-in.
*/
static unsigned int * plugins_mapping_create(const XmlNode *old)
{
	List		*pi, *iter;
	unsigned int	num, *map, i;

	pi  = xmlnode_nodeset_get(old, XMLNODE_AXIS_CHILD, XMLNODE_NAME("plug-in"), XMLNODE_DONE);
	num = list_length(pi);
	map = mem_alloc(num * sizeof *map);
	for(i = 0, iter = pi; i < num && iter != NULL; i++, iter = list_next(iter))
	{
		const Plugin	*p;

		printf("Looking for plug-in named '%s' in current set\n", xmlnode_attrib_get_value(list_data(iter), "name"));
		if((p = plugin_lookup_by_name(xmlnode_attrib_get_value(list_data(iter), "name"))) != NULL)
		{
			map[i] = plugin_id(p);
			printf(" found with ID %u\n", map[i]);
		}
		else
		{
			printf(" Failed!\n");
			mem_free(map);
			return NULL;
		}
	}
	list_destroy(pi);
	return map;
}

/* This cron job only runs once, after the user-specified resume delay has passed. */
static int resume_update(void *data)
{
	Node		*m;
	NodeText	*meta;
	const char	*text;
	XmlNode		*plugins, *graphs;
	NdbTBuffer	*buf;
	unsigned int	*map;
	const List	*gl, *iter;

	printf("Now in resume_update(), time to do stuff\n");
	if((m = nodedb_lookup_by_name(resume_info.meta)) != NULL)
	{
		printf("Found %s node, ID %u\n", resume_info.meta, m->id);
		if(nodedb_type_get(m) != V_NT_TEXT)
		{
			printf("Type is %d, not TEXT--aborting resume attempt\n", nodedb_type_get(m));
			return 0;
		}
	}
	else
	{
		printf("Couldn't find a node named '%s' -- aborting resume attempt\n", resume_info.meta);
		return 0;
	}
	meta = (NodeText *) m;
	if((buf = nodedb_t_buffer_find(meta, "plugins")) == NULL)
	{
		printf("Couldn't find 'plugins' text buffer in meta node -- aborting resume attempt\n");
		return 0;
	}
	if((text = nodedb_t_buffer_read_begin(buf)) == NULL)
	{
		printf("Couldn't access contents of 'plugin' buffer -- aborting resume attempt\n");
		return 0;
	}
	plugins = xmlnode_new(text);
	nodedb_t_buffer_read_end(buf);
	if(plugins == NULL)
	{
		printf("Couldn't parse plugins buffer as XML -- aborting resume attempt\n");
		return 0;
	}
	printf("Got XML from plugins OK\n");
	map = plugins_mapping_create(plugins);
	xmlnode_destroy(plugins);
	if(map == NULL)
	{
		printf("Couldn't create plugin mapping -- aborting resume attempt\n");
		return 0;
	}

	/* 1. Parse out graphs buffer. */
	if((buf = nodedb_t_buffer_find(meta, "graphs")) == NULL)
	{
		printf("Couldn't find 'graphs' text buffer in meta node -- aborting resume attempt\n");
		mem_free(map);
		return 0;
	}
	if((text = nodedb_t_buffer_read_begin(buf)) == NULL)
	{
		printf("Couldn't access contents of 'graphs' buffer -- aborting resume attempt\n");
		mem_free(map);
		return 0;
	}
	graphs = xmlnode_new(text);
	nodedb_t_buffer_read_end(buf);
	if(graphs == NULL)
	{
		printf("Couldn't parse graphs buffer as XML -- aborting resume attempt\n");
		mem_free(map);
		return 0;
	}
	printf("ready to iterate graphs\n");
	
	/* 2. For each graph: */
	/* 2.1. Look up graph node & buffer. */
	/* 2.2. Create Graph structure and populate, remapping plug-ins. */
	/* 2.3. Clear text buffer, replace with new description. */
	/* 3. Be happy. */
	xmlnode_destroy(graphs);
	mem_free(map);

	return 0;
}
