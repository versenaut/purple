/*
 * 
*/

#define	CRON_ANONYMOUS	0

typedef enum
{
	CRON_ONESHOT = 0,
	CRON_PERIODIC,
} CronTimeType;

extern void		cron_init(void);

extern unsigned int	cron_add(CronTimeType type, double seconds, int (*handler)(void *data), void *data);
extern void		cron_set(unsigned int id, double seconds, int (*handler)(void *), void *data);
extern void		cron_remove(unsigned int handle);

extern void		cron_update(void);
