/*
 * A data type for handling a list of IDs. These IDs are expected to come from an IdSet,
 * and the intended use is to track dependents of a module. A single ID cannot occur mor
 * than once in the list.
*/

#include "list.h"

#define	QUICK_MAX	64

/* This type is public so it can be included directly where needed, to cut down on memory
 * allocations. Use only the below API to access, of course.
*/
typedef struct
{
	uint32	quick[QUICK_MAX/32];	/* *Must* be 32-bit, assumed in code below. */
	List	*slow;
} IdList;

/* ----------------------------------------------------------------------------------------- */

extern IdList *	idlist_new(void);
extern void	idlist_construct(IdList *il);	/* Use on fresh allocated memory to initialize. */

extern void	idlist_add(IdList *il, unsigned int id);
extern void	idlist_remove(IdList *il, unsigned int id);

extern void	idlist_foreach(const IdList *il, int (*callback)(unsigned int id, void *data), void *data);

extern void	idlist_destruct(IdList *il);	/* Use on user-owned memory holding IdList before freeing. */
extern void	idlist_destroy(IdList *il);

extern void	idlist_test_as_string(const IdList *il, char *buf, size_t buf_max);
