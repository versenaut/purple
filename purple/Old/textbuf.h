/*
 * A textbuf is an editable piece of text, which should be suitable for holding the
 * text of a Verse text node buffer. It is based on the simple "moving gap" type of
 * buffer, which should provide semi-efficient editing operations.
*/

typedef struct TextBuf TextBuf;

extern TextBuf *	textbuf_new(void);

extern void		textbuf_insert(TextBuf *tb, unsigned int offset, const char *text, size_t len);
extern void		textbuf_delete(TextBuf *tb, unsigned int offset, size_t len);

/* Return a pointer to all of the text, as a zero-terminated string without
 * gaps or anything. This is expensive when intermixed with editing calls.
*/
extern const char *	textbuf_compress(TextBuf *tb);
