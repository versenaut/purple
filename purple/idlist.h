/*
 * A data type for handling a list of IDs. These IDs are expected to come from an IdSet,
 * and the intended use is to track dependents of a module. A single ID cannot occur mor
 * than once in the list.
*/

typedef struct IdList	IdList;

extern IdList *	idlist_new(void);

extern void	idlist_add(IdList *il, unsigned int id);
extern void	idlist_remove(IdList *il, unsigned int id);

extern void	idlist_foreach(const IdList *il, int (*callback)(unsigned int id, void *data), void *data);

extern void	idlist_destroy(IdList *il);
