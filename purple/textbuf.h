/*
 * textbuf.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Data structure for holding editable text. Intended to serve as the "backing store"
 * for a Verse text node buffer.
*/

#if !defined TEXTBUF_H
#define	TEXTBUF_H

typedef struct TextBuf	TextBuf;

/* Create a fresh empty text buffer. Preallocate <initial_size> bytes of space,
 * if non-zero.
*/
extern TextBuf *	textbuf_new(size_t initial_size);

/* Insert <text>, beginning at <offset> in the given buffer. Any trailing text will
 * be pushed towards the end of the buffer; no characters are lost. If <offset> is
 * beyond end of buffer, insert will become an append. Use offset=0 to prepend.
*/
extern void		textbuf_insert(TextBuf *tb, size_t offset, const char *text);

/* Delete region [offset, offset+length-1]. If <offset> is outside text, nothing
 * happens.
*/
extern void		textbuf_delete(TextBuf *tb, size_t offset, size_t length);

/* Truncate to <length> characters, if possible. Does not free storage. */
extern void		textbuf_truncate(TextBuf *tb, size_t length);

/* Return the text, as an ordinary C string with NUL termination and everything.
 * Depending on the underlying implementation, this is potentially expensive.
*/
extern const char *	textbuf_text(TextBuf *tb);

/* Return the length of the text buffer, in characters. */
extern size_t		textbuf_length(const TextBuf *tb);

/* Destroy a text buffer, losing the text of course. */
extern void		textbuf_destroy(TextBuf *tb);

#endif		/* TEXTBUF_H */
