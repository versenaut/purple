/*
 * A "cron" module, for handling scheduling of jobs. A "job" here can be anything,
 * it's just represented as a function to be called. We support either jobs to run
 * at a given time relative from when it's scheduled, as well as periodic jobs that
 * are run with a given (approximate, Purple is a cooperative system) period.
*/

#include <stdio.h>
#include <stdlib.h>

#include "memchunk.h"
#include "list.h"
#include "timeval.h"

#include "cron.h"

/* ----------------------------------------------------------------------------------------- */

#define	PERIODIC_MASK	(1 << 31)
#define	PERIODIC_SET(i)	((i) | PERIODIC_MASK)
#define	PERIODIC_CLR(i)	((i) & ~PERIODIC_MASK)
#define	IS_PERIODIC(i)	((id) & PERIODIC_MASK)

typedef struct
{
	double		period, bucket;
} TimePeriodic;

typedef struct
{
	unsigned int	id;
	union
	{
	TimeVal		oneshot;
	TimePeriodic	periodic;
	}		when;
	int		(*handler)(void *data);
	void		*data;
} Job;

static struct
{
	MemChunk	*chunk_job;
	unsigned int	id_next;
	List		*id_reuse;
	TimeVal		epoch, now;
	List		*oneshot;
	List		*periodic;
} cron_info;

/* ----------------------------------------------------------------------------------------- */

void cron_init(void)
{
	cron_info.chunk_job = memchunk_new("cron/job", sizeof (Job), 8);
	timeval_now(&cron_info.epoch);
	cron_info.now = cron_info.epoch;
	cron_info.id_next  = 1;
	cron_info.id_reuse = NULL;
	cron_info.oneshot  = NULL;
	cron_info.periodic = NULL;
}

unsigned int cron_add(CronTimeType type, double seconds, int (*handler)(void *data), void *data)
{
	Job	*j;
	List	*id;

	if(type > 1)
		return 0;
	if(handler == NULL)
		return 0;

	j = memchunk_alloc(cron_info.chunk_job);
	if((id = cron_info.id_reuse) != NULL)
	{
		j->id = (unsigned int) list_data(id);
		cron_info.id_reuse = list_unlink(cron_info.id_reuse, cron_info.id_reuse);
	}
	else
		j->id = cron_info.id_next++;
	j->handler = handler;
	j->data = data;

	if(type == CRON_ONESHOT)
	{
		timeval_future(&j->when.oneshot, seconds);
		cron_info.oneshot = list_append(cron_info.oneshot, j);
	}
	else if(type == CRON_PERIODIC)
	{
		j->id = PERIODIC_SET(j->id);
		j->when.periodic.period = seconds;
		j->when.periodic.bucket = 0.0;
		cron_info.periodic = list_append(cron_info.periodic, j);
	}
	return j->id;
}

/* Find a job, given its ID number. Sets <head> to the list containing it. */
static List * job_find(unsigned int id, List ***head)
{
	List	**list, *iter;

	if(IS_PERIODIC(id))
		list = &cron_info.periodic;
	else
		list = &cron_info.oneshot;

	for(iter = *list; iter != NULL; iter = list_next(iter))
	{
		if(((Job *) list_data(iter))->id == id)
			break;
	}
	if(head)
		*head = list;

	return iter;
}

void cron_set(unsigned int id, double seconds, int (*handler)(void *), void *data)
{
	List	*j;

	if(id == 0 || seconds <= 0.0 || handler == NULL)
		return;

	if((j = job_find(id, NULL)) != NULL)
	{
		Job	*job = list_data(j);

		if(IS_PERIODIC(id))
			job->when.periodic.period = seconds;
		else
			fprintf(stderr, "cron: Don't know how to set() non-periodic job\n");
	}
}

void cron_remove(unsigned int id)
{
	List	*job, **list;

	if((job = job_find(id, &list)) != NULL)
	{
		Job	*j = list_data(job);

		*list = list_unlink(*list, job);
		printf("removing id %u, data='%s'\n", j->id, (const char *) j->data);
		cron_info.id_reuse = list_prepend(cron_info.id_reuse, (void *) PERIODIC_CLR(j->id));
		memchunk_free(cron_info.chunk_job, list_data(job));
		list_destroy(job);
	}
}

/* Remove a job from a list, and free it. Doubly indirect list, since it might re-head. */
static void job_remove(List **list, List *iter)
{
	*list = list_unlink(*list, iter);
	memchunk_free(cron_info.chunk_job, (Job *) list_data(iter));
	list_destroy(iter);
}

void cron_update(void)
{
	TimeVal	now;
	double	dt;
	List	*iter, *next;

	timeval_now(&now);
	dt = timeval_elapsed(&cron_info.now, &now);

	for(iter = cron_info.oneshot; iter != NULL; iter = next)
	{
		Job	*job = list_data(iter);

		next = list_next(iter);
		if(timeval_passed(&job->when.oneshot, &now))
		{
			if(job->handler == (void *) printf)
				printf("%s\n", (const char *) job->data);
			else
				job->handler(job->data);
			job_remove(&cron_info.oneshot, iter);
		}
	}

	for(iter = cron_info.periodic; iter != NULL; iter = next)
	{
		Job	*job = list_data(iter);

		next = list_next(iter);
		job->when.periodic.bucket += dt;
		if(job->when.periodic.bucket >= job->when.periodic.period)
		{
			if(job->handler == (int (*)(void *)) printf)	/* Clever? */
				printf("%s\n", (const char *) job->data);
			else
			{
				if(!job->handler(job->data))
					job_remove(&cron_info.periodic, iter);
				else
					job->when.periodic.bucket = 0.0;
			}
		}
	}
	cron_info.now = now;
}
