/*
 * Data structure for holding editable text. Intended to serve as the "backing store"
 * for a Verse text node buffer.
*/

typedef struct TextBuf	TextBuf;

extern TextBuf *	textbuf_new(size_t initial_size);

extern void		textbuf_insert(TextBuf *tb, size_t offset, const char *text);
extern void		textbuf_delete(TextBuf *tb, size_t offset, size_t length);

extern const char *	textbuf_text(TextBuf *tb);
extern size_t		textbuf_length(const TextBuf *tb);

extern void		textbuf_destroy(TextBuf *tb);
