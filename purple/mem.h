/*
 * Memory handling functions, to ease debugging and stuff in the future. 
*/

#if !defined MEM_H
#define	MEM_H

enum MemMode { MEM_NULL_RETURN, MEM_NULL_WARN, MEM_NULL_ERROR };

extern void	mem_mode_set(enum MemMode mode);

extern void *	mem_alloc(size_t size);
extern void *	mem_realloc(void *ptr, size_t size);
extern void	mem_free(void *ptr);

#endif		/* MEM_H */
