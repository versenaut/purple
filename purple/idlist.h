/*
 * A data type for handling a list of IDs. These IDs are expected to come from an IdSet,
 * and the intended use is to track dependents of a module. A single ID cannot occur mor
 * than once in the list.
*/

#include "list.h"

#define	IDLIST_QUICK_MAX	64

/* This type is public so it can be included directly where needed, to cut down on memory
 * allocations. Use only the below API to access, of course.
*/
typedef struct
{
	uint32	quick[IDLIST_QUICK_MAX/32];	/* *Must* be 32-bit, assumed in implementation. */
	List	*slow;
} IdList;

/* Iterator data used when iterating. Saves using defining a callback. */
typedef struct
{
	int		quick;
	List		*slow;
	unsigned int	id;
} IdListIter;

/* ----------------------------------------------------------------------------------------- */

extern IdList *	idlist_new(void);
extern void	idlist_construct(IdList *il);	/* Use on fresh allocated memory to initialize. */

extern void	idlist_add(IdList *il, unsigned int id);
extern void	idlist_remove(IdList *il, unsigned int id);

extern void	idlist_foreach(const IdList *il, int (*callback)(unsigned int id, void *data), void *data);

/* Iterator-based foreach. Use like this:
 * for(idlist_foreach_init(il, &iter); idlist_foreach_step(il, &iter);)
 * 	Process using iter.id as current ID.
*/
extern void	idlist_foreach_init(const IdList *il, IdListIter *iter);
extern boolean	idlist_foreach_step(const IdList *il, IdListIter *iter);

extern void	idlist_destruct(IdList *il);	/* Use on user-owned memory holding IdList before freeing. */
extern void	idlist_destroy(IdList *il);

extern void	idlist_test_as_string(const IdList *il, char *buf, size_t buf_max);
