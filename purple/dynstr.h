/*
 * A dynamically allocated string. Optimized solely for appending, and not at all
 * useful for e.g. UTF-8 or other exotic multibyte encodings. It will break. Also
 * cannot contain null bytes.
*/

#if !defined DYNSTR_H
#define	DYNSTR_H

typedef struct DynStr	DynStr;

/* Create a new, empty string. */
extern DynStr *	dynstr_new(const char *text);

/* Create a new, empty string, ready to hold <size> characters. */
extern DynStr *	dynstr_new_sized(size_t size);

/* Set the string <str> to <text>. */
extern void	dynstr_assign(DynStr *str, const char *text);

/* Format content into <str>. */
extern void	dynstr_printf(DynStr *str, const char *fmt, ...);

/* Append <text> to <str>. */
extern void	dynstr_append(DynStr *str, const char *text);

/* Append the single character <c> to <str>. */
extern void	dynstr_append_c(DynStr *str, char c);

/* Append formatted text to <str>. */
extern void	dynstr_append_printf(DynStr *str, const char *fmt, ...);

/* Return the string <str>'s character buffer. Read only, please. */
extern const char * dynstr_string(const DynStr *str);

/* Return the length of <str>. O(1). */
extern size_t	dynstr_length(const DynStr *str);

/* Destroy string <str>. If <destroy_buffer> is non-zero, it will
 * also destroy the actual string buffer. If not, it is returned.
*/
extern char *	dynstr_destroy(DynStr *str, int destroy_buffer);

#endif		/* DYNSTR_H */
