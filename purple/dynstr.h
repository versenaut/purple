/*
 * A dynamically allocated string. Optimized solely for appending, and not at all
 * useful for e.g. UTF-8 or other exotic multibyte encodings. It will break. Also
 * cannot contain null bytes.
*/

#if !defined DYNSTR_H
#define	DYNSTR_H

typedef struct DynStr	DynStr;

extern DynStr *	dynstr_new(const char *text);
extern void	dynstr_assign(DynStr *str, const char *text);

extern void	dynstr_printf(DynStr *str, const char *fmt, ...);

extern void	dynstr_append(DynStr *str, const char *text);
extern void	dynstr_append_c(DynStr *str, char c);
extern void	dynstr_append_printf(DynStr *str, const char *fmt, ...);

extern const char * dynstr_string(const DynStr *str);
extern size_t	dynstr_length(const DynStr *str);

/* Destroy a dynamic string. If <destroy_buffer> is non-zero, it will
 * also destroy the actual string buffer. If not, it is returned.
*/
extern char *	dynstr_destroy(DynStr *ds, int destroy_buffer);

#endif		/* DYNSTR_H */
