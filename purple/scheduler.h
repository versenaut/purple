/*
 * scheduler.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * The scheduler module is responsible for making plug-in code run, by calling (indirectly)
 * their compute() entry points.
*/

/* Add the <inst> instance to the set of plug-ins that need to be run. The only way to stop
 * running is to return P_COMPUTE_DONE from compute(), there is no remove().
*/

extern void	sched_add(PInstance *inst);

/* Give the scheduler CPU time to spend running plug-ins. It will time itself and stop
 * running code after a (currently hard-coded) time has passed. This is only co-operative
 * however, no preemption is done.
*/
extern void	sched_update(void);
