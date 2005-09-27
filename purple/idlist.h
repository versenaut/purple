/*
 * idlist.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * A data type for handling a list of IDs. These IDs are expected to come from an IdSet,
 * and the intended use is to track dependents of a module. A single ID cannot occur more
 * than once in the list.
 * 
 * NOTE: To come closer to feeling like someone who at least vaguely *remembers* the word
 * "efficiency", the implementation limits IDs in practical use to 16 bits, and supports
 * at most 2^16-1 insertions of the same ID in a set. No bonus for figuring out why.
*/

#include "list.h"

/* This type is public so it can be included directly where needed, to cut down on
 * memory allocations. Use only the below API to access, of course.
*/
typedef struct
{
	List	*entries;	/* Oh well. */
} IdList;

/* Iterator data used when iterating. Saves user from defining a callback. */
typedef struct
{
	List		*iter;
	unsigned int	id;
} IdListIter;

/* ----------------------------------------------------------------------------------------- */

extern IdList *	idlist_new(void);
extern void	idlist_construct(IdList *il);	/* Use on fresh allocated memory to initialize. */

extern void	idlist_insert(IdList *il, unsigned int id);
extern void	idlist_remove(IdList *il, unsigned int id);

/* Iterator-based foreach. Use like this:
 * for(idlist_foreach_init(il, &iter); idlist_foreach_step(il, &iter);)
 * 	Process using iter.id as current ID.
*/
extern void	idlist_foreach_init(const IdList *il, IdListIter *iter);
extern boolean	idlist_foreach_step(const IdList *il, IdListIter *iter);

extern void	idlist_destruct(IdList *il);	/* Use on user-owned memory holding IdList before freeing. */
extern void	idlist_destroy(IdList *il);

extern void	idlist_test_as_string(const IdList *il, char *buf, size_t buf_max);
