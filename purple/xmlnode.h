/*
 * A bare-bones, non-standards-compliant XML parser. This is very poor, but should be
 * enough to at least allow some progress, and if it turns out to be a weak area (not
 * expected, but you never know) it should be at least possible to replace it later.
 * 
 * This does not attempt to follow any of the more or less established standard ways
 * of parsing XML and representing it in memory, but...
*/

typedef struct XmlNode	XmlNode;

/* Create XML parse tree from textual representation in <buffer>. */
extern XmlNode	*	xmlnode_new(const char *buffer);

/* Get value of named <attribute>. */
extern const char *	xmlnode_attrib_get(const XmlNode *node, const char *attribute);

/* Print outline of XML parse tree rooted at <root>. Mainly for debugging. */
extern void		xmlnode_print_outline(const XmlNode *root);

/* Destroy an XML parse tree. */
extern void		xmlnode_destroy(XmlNode *root);
