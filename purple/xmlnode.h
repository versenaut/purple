/*
 * xmlnode.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * A bare-bones, non-standards-compliant XML parser. This is very poor, but should be
 * enough to at least allow some progress, and if it turns out to be a weak area (not
 * expected, but you never know) it should be at least possible to replace it later.
 * 
 * This does not attempt to follow any of the more or less established standard ways
 * of parsing XML and representing it in memory, but...
*/

#include "list.h"

typedef struct XmlNode	XmlNode;

/* Create XML parse tree from textual representation in <buffer>. */
extern XmlNode	*	xmlnode_new(const char *buffer);

extern const char *	xmlnode_get_name(const XmlNode *node);

/* Get value of named <attribute>. */
extern const char *	xmlnode_attrib_get_value(const XmlNode *node, const char *attribute);

typedef enum
{
	XMLNODE_AXIS_SELF = 0,
	XMLNODE_AXIS_ANCESTOR,
	XMLNODE_AXIS_CHILD,
	XMLNODE_AXIS_PREDECESSOR,
	XMLNODE_AXIS_SUCCESSOR
} XmlNodeAxis;

typedef enum
{
	XMLNODE_FILTER_ACCEPT = 0,
	XMLNODE_FILTER_NAME,
	XMLNODE_FILTER_ATTRIB,
	XMLNODE_FILTER_ATTRIB_VALUE
} XmlNodeFilter;

#define	XMLNODE_DONE			(void *) XMLNODE_FILTER_ACCEPT
#define	XMLNODE_NAME(n)			(void *) XMLNODE_FILTER_NAME, (n)
#define	XMLNODE_ATTRIB(a)		(void *) XMLNODE_FILTER_ATTRIB, (a)
#define	XMLNODE_ATTRIB_VAL(a,v)		(void *) XMLNODE_FILTER_ATTRIB_VALUE, (a), (v)

/*
 * xmlnode_nodeset_get(root, XMLNODE_AXIS_CHILDREN, XMLNODE_NAME("plugin"), XMLNODE_DONE);
*/
extern List *		xmlnode_nodeset_get(const XmlNode *node, XmlNodeAxis axis, ...);

/*
 * xmlnode_eval_single(root, "at/node");
*/
extern const char *	xmlnode_eval_single(const XmlNode *node, const char *path);

/* Print outline of XML parse tree rooted at <root>. Mainly for debugging. */
extern void		xmlnode_print_outline(const XmlNode *root);

/* Destroy an XML parse tree. */
extern void		xmlnode_destroy(XmlNode *root);
