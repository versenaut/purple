/*
 * timeval.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Functions for measuring time.
*/

#include <stdio.h>
#include <stdlib.h>

#if defined __win32
#else
#include <sys/time.h>
#endif

#include "timeval.h"

/* ----------------------------------------------------------------------------------------- */

void timeval_jurassic(TimeVal *tv)
{
	if(tv == NULL)
		return;
	tv->sec = tv->usec = 0;
}

void timeval_now(TimeVal *tv)
{
	if(tv == NULL)
		return;
#if defined __win32
	/* Untested code, beware. */
	{
		struct _timeb	now;

		_ftime(&now);
		tv->sec  = now.time;
		tv->usec = 1000 * now.millitm;
	}
#else
	{
		struct timeval	now;

		gettimeofday(&now, NULL);
		tv->sec  = now.tv_sec;
		tv->usec = now.tv_usec;
	}
#endif
}

void timeval_future(TimeVal *tv, double seconds)
{
	if(tv == NULL)
		return;
	if(seconds <= 0.0)
		return;
	timeval_now(tv);
	tv->sec  += (int) seconds;
	tv->usec += 1000000.0 * (seconds - (int) seconds);
}

int timeval_passed(const TimeVal *sometime, const TimeVal *now)
{
	if(sometime == NULL)
		return 0;
	if(now == NULL)
	{
		static TimeVal	n;

		timeval_now(&n);
		now = &n;
	}
	if(sometime->sec > now->sec)
		return 0;
	if(sometime->sec < now->sec)
		return 1;
	return sometime->usec < now->usec;
}

double timeval_elapsed(const TimeVal *then, const TimeVal *now)
{
	if(then == NULL)
		return 0.0;
	if(now == NULL)
	{
		static TimeVal	n;

		timeval_now(&n);
		now = &n;
	}
	return (now->sec - then->sec) + 1E-6 * (now->usec - then->usec);
}
