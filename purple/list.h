/*
 * A simple doubly-linked list datatype. Many operations are O(n).
*/

typedef struct List	List;

extern void	list_init(void);

extern List *	list_new(void *data);
extern List *	list_append(List *list, void *data);
extern List *	list_prepend(List *list, void *data);

extern List *	list_concat(List *head, List *tail);
extern List *	list_reverse(List *list);

extern List *	list_unlink(List *list, List *element);

extern void *	list_data(const List *list);
extern List *	list_prev(const List *list);
extern List *	list_next(const List *list);

extern size_t	list_length(const List *list);
extern List *	list_nth(List *list, size_t n);
extern size_t	list_index(List *list);
extern List *	list_first(List *list);
extern List *	list_last(List *list);

extern void	list_foreach(const List *list, int (*callback)(void *data, void *userdata), void *userdata);
extern List *	list_find(List *list, int (*test)(void *data, void *userdata), void *userdata);

extern void	list_destroy(List *list);