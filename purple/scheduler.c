/*
 * scheduler.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * A scheduler. Sounds a lot more sophisticated than it really is, at this point.
*/

#include <stdio.h>
#include <stdlib.h>

#include "purple.h"

#include "dynarr.h"
#include "list.h"
#include "log.h"
#include "memchunk.h"
#include "plugins.h"
#include "textbuf.h"
#include "timeval.h"
#include "value.h"

#include "nodedb.h"
#include "graph.h"

#include "scheduler.h"

typedef struct
{
	PInstance	*inst;
	unsigned long	count;		/* Counts number of times compute() has been run. */
} Task;

static struct
{
	MemChunk	*chunk_task;
	List		*ready;
	List		*ready_iter;	/* Remembers position in ready-list between update()s. */
} sched_info;

/* ----------------------------------------------------------------------------------------- */

static int cmp_task_by_inst(const void *listdata, const void *data)
{
	const Task	*t = listdata;

	return t->inst < (PInstance *) data ? -1 : t->inst > (PInstance *) data;
}

void sched_add(PInstance *inst)
{
	Task	*t;

	if(list_find_custom(sched_info.ready, inst, cmp_task_by_inst) != NULL)
		return;
	if(sched_info.chunk_task == NULL)
		sched_info.chunk_task = memchunk_new("Task", sizeof (Task), 16);
	if((t = memchunk_alloc(sched_info.chunk_task)) == NULL)
		return;
	t->inst  = inst;
	t->count = 0;
	sched_info.ready = list_append(sched_info.ready, t);
	LOG_MSG(("Added %s to ready-list, there are now %u ready tasks", plugin_name(inst->plugin), list_length(sched_info.ready)));
	sched_info.ready_iter = NULL;
}

#define	RUNTIME_LIMIT	1.0	/* Lower bound on maximum time to spend running compute(). Merely co-operative. :/ */

void sched_update(void)
{
	TimeVal	t;
	List	*iter, *next;
	TimeVal	t1;

	timeval_now(&t);
	iter = sched_info.ready_iter;
	while(timeval_elapsed(&t, NULL) < RUNTIME_LIMIT)
	{
		PluginStatus	res;
		Task		*task;

		if(iter == NULL)
			iter = sched_info.ready;	/* Restart if all computations done. */
		if(iter == NULL)			/* If no tasks need running, don't waste CPU here. */
			break;
		next = list_next(iter);
		task = list_data(iter);
		if(task->count == 0)			/* First time we run it, make it prepare. */
			graph_port_output_begin(task->inst->output);
		task->count++;
		timeval_now(&t1);
		res = plugin_instance_compute(task->inst);
		printf("Spent %g seconds running compute() of %s\n", timeval_elapsed(&t1, NULL), plugin_name(task->inst->plugin));
		if(res >= PLUGIN_STOP)
		{
			PPOutput	out = task->inst->output;	/* Buffer across free(). */
			sched_info.ready = list_unlink(sched_info.ready, iter);
			memchunk_free(sched_info.chunk_task, task);
			list_destroy(iter);
			graph_port_output_end(out);		/* Don't notify dependants until done. */
			LOG_MSG(("Task removed, there are now %u ready tasks", list_length(sched_info.ready)));
		}
		iter = next;
	}
	sched_info.ready_iter = iter;
}
