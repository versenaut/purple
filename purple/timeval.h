/*
 * A little something to measure elapsed time.
*/

typedef struct
{
	long	sec, usec;
} TimeVal;

/* ----------------------------------------------------------------------------------------- */

extern void	timeval_jurassic(TimeVal *tv);
extern void	timeval_now(TimeVal *tv);
extern void	timeval_future(TimeVal *tv, double seconds);

extern int	timeval_passed(const TimeVal *sometime, const TimeVal *now);

/* Return elapsed time in seconds between <then> and <now>. If <now> is NULL,
 * current time is used.
*/
extern double	timeval_elapsed(const TimeVal *then, const TimeVal *now);
