/*
 * A "cron" module, for handling scheduling of jobs. A "job" here can be anything,
 * it's just represented as a function to be called. We support either jobs to run
 * at a given time relative from when it's scheduled, as well as periodic jobs that
 * are run with a given (approximate, Purple is a cooperative system) period.
*/

typedef enum
{
	CRON_ONESHOT = 0,
	CRON_PERIODIC,
} CronTimeType;

extern void		cron_init(void);

extern unsigned int	cron_add(CronTimeType type, double seconds, int (*handler)(void *data), void *data);
extern void		cron_set(unsigned int id, double seconds, int (*handler)(void *data), void *data);
extern void		cron_remove(unsigned int handle);

extern void		cron_update(void);
