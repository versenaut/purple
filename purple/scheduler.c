/*
 * A scheduler. Sounds a lot more sophisticated than it really is, at this point.
*/

#include <stdlib.h>

#include "list.h"
#include "log.h"
#include "memchunk.h"
#include "plugins.h"
#include "timeval.h"

#include "scheduler.h"

typedef struct
{
	PInstance	*inst;
} Task;

static struct
{
	MemChunk	*chunk_task;
	List		*ready;
	List		*ready_iter;	/* Remembers position in ready-list between update()s. */
} sched_info = { NULL };

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
	t = memchunk_alloc(sched_info.chunk_task);
	t->inst = inst;
	sched_info.ready = list_append(sched_info.ready, t);
	LOG_MSG(("There are now %u scheduled ready tasks", list_length(sched_info.ready)));
	sched_info.ready_iter = NULL;
}

#define	RUNTIME_LIMIT	1.0

void sched_update(void)
{
	TimeVal	t;
	List	*iter, *next;

	timeval_now(&t);
	iter = sched_info.ready_iter;
	if(iter == NULL)
		iter = sched_info.ready;
	for(; iter != NULL; iter = next)
	{
		PluginStatus	res;
		Task		*task;

		if(timeval_elapsed(&t, NULL) > RUNTIME_LIMIT)
			break;
		next = list_next(iter);
		task = list_data(iter);
		res = plugin_instance_compute(task->inst);
		if(res >= PLUGIN_STOP)
		{
			sched_info.ready = list_unlink(sched_info.ready, iter);
			memchunk_free(sched_info.chunk_task, task);
			list_destroy(iter);
			LOG_MSG(("Task removed, there are now %u ready tasks", list_length(sched_info.ready)));
		}
	}
	sched_info.ready_iter = iter;
}
