/*
 * IdSet, a simple data structure for managing a set of objects, each of which is
 * given a numerical ID. Objects can be inserted and removed, with IDs re-used from
 * removed objects. IDs of inserted objects never change.
*/

typedef struct IdSet	IdSet;

extern IdSet *		idset_new(void);

extern unsigned int	idset_insert(IdSet *is, void *object);
extern void		idset_remove(IdSet *is, unsigned int id);

extern size_t		idset_size(const IdSet *is);

extern void *		idset_lookup(const IdSet *is, unsigned int id);

extern void *		idset_foreach_next(const IdSet *is, unsigned int *id);

extern void		idset_destroy(IdSet *is);
