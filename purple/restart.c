/*
 * restart.c.
 * 
 * Copyright (C) 2005 PDC, KTH. See COPYING for license details.
 * 
 * Logic for "restarting" a Purple session, by finding whatever meta data
 * was left by a previous instance, and trying to replicate it into our
 * own state. Pretty hairy in places.
*/

static struct
{
	char	meta[32];
} restart_info;

/* This cron job only runs once, after the user-specified restart delay has passed. */
int restart_update(void *data)
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
