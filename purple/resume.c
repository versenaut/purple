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

#include "xmlnode.h"

#include "resume.h"

static struct
{
	int	enabled;		/* Set to 1 in init(), queried by resume_enabled(). */
	char	meta[32];
} resume_info = { 0 };

/* ----------------------------------------------------------------------------------------- */

void resume_init(const char *options)
{
	printf("Initializing resume-mode\n");
	resume_info.enabled = 1;
	strcpy(resume_info.meta, "PurpleMeta");
}

int resume_enabled(void)
{
	return resume_info.enabled;
}

/* ----------------------------------------------------------------------------------------- */

/* This cron job only runs once, after the user-specified resume delay has passed. */
int resume_update(void *data)
{
	XmlNode	*graphs;

	/* 1. Parse out graphs buffer. */
	/* 2. For each graph: */
	/* 2.1. Look up graph node & buffer. */
	/* 2.2. Create Graph structure and populate, remapping plug-ins. */
	/* 2.3. Clear text buffer, replace with new description. */
	/* 3. Be happy. */

	return 0;
}
