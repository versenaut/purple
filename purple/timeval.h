/*
 * timeval.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * A little something to measure elapsed time.
*/

typedef struct
{
	long	sec, usec;
} TimeVal;

/* ----------------------------------------------------------------------------------------- */

/* Initialize <tv> to sometime very long ago. */
extern void	timeval_jurassic(TimeVal *tv);

/* Set <tv> to represent the current time. This is not immediately convertible
 * to any known human-readable time format.
*/
extern void	timeval_now(TimeVal *tv);

/* Set <tv> to represent a time <seconds> seconds into the future from now. */
extern void	timeval_future(TimeVal *tv, double seconds);

/* Determine whether <sometime> has passed, given the reference <now> time. If
 * <now> is NULL, current time from timeval_now() is called internally.
*/
extern int	timeval_passed(const TimeVal *sometime, const TimeVal *now);

/* Return elapsed time in seconds between <then> and <now>. If <now> is NULL,
 * current time from timeval_now() is used.
*/
extern double	timeval_elapsed(const TimeVal *then, const TimeVal *now);
