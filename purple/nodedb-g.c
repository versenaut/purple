/*
 * 
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"
#include "purple.h"

#include "dynarr.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "strutil.h"
#include "textbuf.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

void nodedb_g_construct(NodeGeometry *n)
{
	n->layers = NULL;
	n->bones  = NULL;
}

static void cb_copy_layer(void *d, const void *s, void *user)
{
	const NdbGLayer		*src = s;
	NdbGLayer		*dst = d;
	const NodeGeometry	*node = user;
}

void nodedb_g_copy(NodeGeometry *n, const NodeGeometry *src)
{
	if(src->layers != NULL)
		n->layers = dynarr_new_copy(src->layers, cb_copy_layer, n);
}

void nodedb_g_destruct(NodeGeometry *n)
{
	if(n->layers != NULL)
	{
		unsigned int	i;
		NdbGLayer	*layer;

		for(i = 0; i < dynarr_size(n->layers); i++)
		{
			if((layer = dynarr_index(n->layers, i)) == NULL || layer->name[0] == '\0')
				continue;
			mem_free(layer->data);
		}
		dynarr_destroy(n->layers);
	}
	/* FIXME: Bones here, too. */
}

void nodedb_g_register_callbacks(void)
{
}
