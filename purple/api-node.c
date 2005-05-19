/*
 * api-node.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * 
*/

#include <stdarg.h>

#include "verse.h"

#define PURPLE_INTERNAL

#include "purple.h"

#include "dynarr.h"
#include "list.h"
#include "iter.h"
#include "strutil.h"
#include "textbuf.h"

#include "nodedb.h"

/* ----------------------------------------------------------------------------------------- */

/**
 * Return the type of a node.
*/
PURPLEAPI VNodeType p_node_get_type(PINode *node	/** The node whose type is to be queried. */)
{
	return nodedb_type_get(node);
}

/**
 * Return the name of a node.
*/
PURPLEAPI const char * p_node_get_name(const Node *node	/** The node whose name is to be queried. */)
{
	return node != NULL ? node->name : NULL;
}

/**
 * Set the name of a node.
 * 
 * Node names are limited in length by Verse's data model. The current limitation is 511 bytes.
*/
PURPLEAPI void p_node_set_name(PONode *node	/** The node whose name is to be changed. */,
			       const char *name	/** The new name. */)
{
	nodedb_rename(node, name);
}

/* ----------------------------------------------------------------------------------------- */

/**
 * Return the number of tag groups in a node.
*/
PURPLEAPI unsigned int p_node_tag_group_num(PINode *node	/** The node whose number of tag groups is to be queried. */)
{
	return nodedb_tag_group_num(node);
}

/**
 * Return a tag group, by index. Valid index range is 0 up to, but not including, the value returned by the
 * \c p_node_tag_group_num() function. Specifying an index outside of this range causes \c NULL to be returned.
*/
PURPLEAPI PNTagGroup * p_node_tag_group_nth(PINode *node	/** The node whose tag group is to be accessed. */,
					    unsigned int n	/** The index of the tag group to access. */)
{
	return nodedb_tag_group_nth(node, n);
}

/**
 * Return a tag group, by name. If no tag group with the specified name exists, \c NULL is returned.
*/
PURPLEAPI PNTagGroup * p_node_tag_group_find(PINode *node	/** The node whose tag group is to be accessed. */,
					     const char *name	/** The name of the tag group to access. */)
{
	return nodedb_tag_group_find(node, name);
}

/**
 * Initialize an iterator over a node's tag groups.
 * 
 * Once the iterator is initialized, the \c p_iter_data() and \c p_iter_next() functions can be used to
 * traverse over all existing tag groups.
*/
PURPLEAPI void p_node_tag_group_iter(PINode *node	/** The node whose tag groups are to be iterated. */,
				     PIter *iter	/** The iterator to initialize. */)
{
	iter_init_dynarr_string(iter, ((Node *) node)->tag_groups, offsetof(NdbTagGroup, name));
}

/**
 * Return the name of tag group.
*/
PURPLEAPI const char * p_node_tag_group_get_name(const PNTagGroup *group	/** The tag group whose name is returned. */)
{
	if(group != NULL)
		return ((NdbTagGroup *) group)->name;
	return NULL;
}

/**
 * Create a new tag group. Returns a reference to the new group, or \c NULL on error.
*/
PURPLEAPI PNTagGroup * p_node_tag_group_create(PONode *node	/** The node in which the tag group is to be created. */,
					       const char *name	/** The name of the new tag group. Must be unique within the node. */)
{
	return nodedb_tag_group_create(node, ~0, name);
}

/**
 * Destroy a tag group, and any tags contained in the group.
*/
PURPLEAPI void p_node_tag_group_destroy(PONode *node, PNTagGroup *group)
{
	nodedb_tag_group_destroy(group);
}

/**
 * Return the number of tags in a tag group.
*/
PURPLEAPI unsigned int p_node_tag_group_tag_num(const PNTagGroup *group	/** The tag group whose number of tags is queried. */)
{
	return nodedb_tag_group_tag_num((NdbTagGroup *) group);
}

/**
 * Return a tag, by index. Valid index range is 0 up to, but not including, the value returned by the
 * \c p_node_tag_group_tag_num() function. Specifying an index outside of this range causes \c NULL to be returned.
*/
PURPLEAPI PNTag * p_node_tag_group_tag_nth(const PNTagGroup *group	/** The tag group whose tag is to be accessed. */,
					   unsigned int n		/** The index of the tag to access. */)
{
	return nodedb_tag_group_tag_nth((NdbTagGroup *) group, n);
}

/**
 * Return a tag, by name. If no tag by the given name exists, \c NULL is returned. 
*/
PURPLEAPI PNTag * p_node_tag_group_tag_find(const PNTagGroup *group	/** The tag group whose tags are to be searched. */,
					    const char *name		/** The name of the tag to search for. */)
{
	return nodedb_tag_group_tag_find(group, name);
}

/**
 * Initialize an iterator over a tag group's tags.
*/
PURPLEAPI void p_node_tag_group_tag_iter(const PNTagGroup *group	/** The tag group whose tags are to be iterated. */,
					 PIter *iter			/** The iterator to initialize. */)
{
	if(group == NULL)
		return;
	iter_init_dynarr_string(iter, ((NdbTagGroup *) group)->tags, offsetof(NdbTag, name));
}

/**
 * Create a new tag. Tags are named values that reside in nodes. For more detail, please see the Verse
 * specification.
 * 
 * Note that there is \b no function named p_node_tag_set() or similiar. To change the value of an existing
 * tag, use this function with the name of the old tag.
*/
PURPLEAPI void p_node_tag_group_tag_create(PNTagGroup *group	/** The group in which the tag is to be created. */,
				 const char *name,	/** The name of the new tag. Must be unique within the tag group. */
				 VNTagType type		/** The type of the new tag's value. */,
				 const VNTag *value	/** Initial value for the new tag. */)
{
	nodedb_tag_create(group, ~0, name, type, value);
}

/**
 * Destroy a tag, by name.
*/
PURPLEAPI void p_node_tag_group_tag_destroy(PNTagGroup *group	/** The group in which a tag is to be destroyed. */,
					    PNTag *tag		/** The tag to be destroyed. */)
{
	nodedb_tag_destroy(group, tag);
}

/**
 * Create (or set) a tag, by specifying its location using a textual address called a "path". The path is a string
 * of the form \c group/tag, where \a group is the name of the tag group to access, and \a tag is the name of the
 * tag within that group. The (forward) slash separates the group name from the tag name.
 * 
 * If the specified group does not exist, group will be created with the indicated name.
 * 
 * The purpose of this function is to make tag-setting code shorter and more to the point. It saves plug-in authors
 * from having to first create (or find) a tag group, before they can set the tag value. It makes tag-setting code
 * a lot more straightforward both to read and to write.
*/
PURPLEAPI void p_node_tag_create_path(PONode *node	/** The node in which to set the tag. */,
				      const char *path	/** "Path" to target tag, in \c group/tag format. See above. */,
				      VNTagType type	/** Type of the value to set. */,
				      ...)
{
	char	group[32], *put;

	if(node == NULL || path == NULL || *path == '\0')
		return;
	for(put = group; *path != '/' && (size_t) (put - group) < (sizeof group - 1);)
		*put++ = *path++;
	*put = '\0';
	if(*path == '/')
	{
		NdbTagGroup	*tg;
		NdbTag		*tag;
		VNTag		value;
		va_list		arg;

		if((tg = nodedb_tag_group_find(node, group)) == NULL)
			tg = nodedb_tag_group_create(node, ~0, group);
		path++;
		va_start(arg, type);
		switch(type)
		{
		case VN_TAG_BOOLEAN:
			value.vboolean = va_arg(arg, int);
			break;
		case VN_TAG_UINT32:
			value.vuint32 = va_arg(arg, uint32);
			break;
		case VN_TAG_REAL64:
			value.vreal64 = va_arg(arg, real64);
			break;
		case VN_TAG_STRING:
			value.vstring = va_arg(arg, char *);	/* Duplicated by nodedb. */
			break;
		case VN_TAG_REAL64_VEC3:
			value.vreal64_vec3[0] = va_arg(arg, real64);
			value.vreal64_vec3[1] = va_arg(arg, real64);
			value.vreal64_vec3[2] = va_arg(arg, real64);
			break;
		case VN_TAG_LINK:
			value.vlink = va_arg(arg, VNodeID);
			break;
		case VN_TAG_ANIMATION:
			value.vanimation.curve = va_arg(arg, VNodeID);
			value.vanimation.start = va_arg(arg, uint32);
			value.vanimation.end   = va_arg(arg, uint32);
			break;
		case VN_TAG_BLOB:
			value.vblob.size = va_arg(arg, unsigned int);
			value.vblob.blob = va_arg(arg, void *);
			break;
		case VN_TAG_TYPE_COUNT:
			;
		}
		va_end(arg);
		/* If tag exists, just (re)set the value, else create it. */
		if((tag = nodedb_tag_group_tag_find(tg, path)) != NULL)
			nodedb_tag_value_set(tag, type, &value);
		else
			nodedb_tag_create(tg, ~0, path, type, &value);
	}
}

/**
 * Destroy a tag, by specifying its location using a textual path. See the \c p_node_tag_create_path() function for
 * more details about paths.
 * 
 * If the path does not specify a tag part, the entire tag group will be destroyed.
*/
PURPLEAPI void p_node_tag_destroy_path(PONode *node	/** The node in which to destroy a tag (or tag group). */,
				       const char *path	/** The path to the tag or tag group to destroy. */)
{
	char		group[32], *put;
	NdbTagGroup	*g;

	if(node == NULL || path == NULL || *path == '\0')
		return;
	for(put = group; *path != '\0' && *path != '/' && (size_t) (put - group) < (sizeof group - 1);)
		*put++ = *path++;
	*put = '\0';
	if(*path == '/')
		path++;
	if((g = nodedb_tag_group_find(node, group)) != NULL)
	{
		if(*path == '\0')		/* If no second part, destroy group. */
			nodedb_tag_group_destroy(g);
		else if(*path == '*')		/* If asterisk, destroy all tags but leave group. */
			nodedb_tag_destroy_all(g);
		else				/* Else destroy named tag only. */
			nodedb_tag_destroy(g, nodedb_tag_group_tag_find(g, path));
	}
}

/**
 * Return the name of a tag.
*/
PURPLEAPI const char * p_node_tag_get_name(const PNTag *tag	/** The tag whose name is to be queried. */)
{
	return nodedb_tag_get_name(tag);
}

/**
 * Return the type of a tag.
*/
PURPLEAPI VNTagType p_node_tag_get_type(const PNTag *tag	/** The tag whose type is to be queried. */)
{
	if(tag != NULL)
		return ((NdbTag *) tag)->type;
	return -1;
}

/* ----------------------------------------------------------------------------------------- */

/**
 * Set the light values for an object node.
*/
PURPLEAPI void p_node_o_light_set(PONode *node	/** The object node whose light values are to be set. */,
				  real64 red	/** Amount of red light emitted by the object. */,
				  real64 green	/** Amount of green light emitted by the object. */,
				  real64 blue	/** Amount of blue light emitted by the object. */)
{
	nodedb_o_light_set((NodeObject *) node, red, green, blue);
}

/**
 * Retrieve the light values for an object node.
*/
PURPLEAPI void p_node_o_light_get(PINode *node	/** The object node whose light values are to be read. */,
				  real64 *red	/** Pointer to \c real64 that is set to the amount of red light emitted by the object. */,
				  real64 *green	/** Pointer to \c real64 that is set to the amount of green light emitted by the object. */,
				  real64 *blue	/** Pointer to \c real64 that is set to the amount of blue light emitted by the object. */)
{
	nodedb_o_light_get((NodeObject *) node, red, green, blue);
}

/**
 * Set a link from an object node to another node. The type of the destination node need not be an object,
 * and in many cases isn't.
 * 
 * The logical identification of a link is in two parts: one is the \a label, which is a human-readable string
 * like "geometry" or "material", and the other is a 32-bit unsigned integer that can be used to refer to a
 * part of the target node. Typically it is not used, and set to zero.
*/
PURPLEAPI void p_node_o_link_set(PONode *node		/** The object node in which to set a link. */,
				 const PONode *link	/** The node being linked to. */,
				 const char *label	/** The textual label of the link, used to identify the purpose. */,
				 uint32 target_id	/** Numerical identifier for addressing parts of a target node. */)
{
	if(node != NULL && link != NULL && label != NULL)
		nodedb_o_link_set_local((NodeObject *) node, link, label, target_id);
}

/**
 * Retreive information about a link from an object node. Returns the link target, if present, or \c NULL If not.
*/
PURPLEAPI PINode * p_node_o_link_get(const PONode *node	/** The object node whose link is to be retrieved. */,
				     const char *label	/** The label of the link to return. */,
				     uint32 target_id	/** The numerical identifier of the link to return. */)
{
	if(node != NULL && label != NULL)
	{
		PINode	*n = nodedb_o_link_get((NodeObject *) node, label, target_id);

		if(n == NULL)
			return (PINode *) nodedb_o_link_get_local((NodeObject *) node, label, target_id);
		return n;
	}
	return NULL;
}

/* ----------------------------------------------------------------------------------------- */

/**
 * Return the number of layers in a geometry node.
 * 
 * Typically, this value will always be at least two, since there are two layers automatically
 * created by the Verse host that will always be present. These two layers are the base layers
 * for vertex and polygon data, respectively, indexed \c 0 and \c 1 and named "vertex" and
 * "polygon".
*/
PURPLEAPI unsigned int p_node_g_layer_num(PINode *node	/** Then node whose layer-count is to be queried. */)
{
	return nodedb_g_layer_num((NodeGeometry *) node);
}

/**
 * Return a geometry layer, by index. Valid index range is 0 up to, but not including, the value returned
 * by the \c p_node_g_layer_num() function. Specifying an index outside of this range causes \c NULL to be returned.
*/
PURPLEAPI PNGLayer * p_node_g_layer_nth(PINode *node	/** The node whose layers are to be accessed. */,
					unsigned int n	/** The index of the tag group to access. */)
{
	return nodedb_g_layer_nth((NodeGeometry *) node, n);
}

/**
 * Return a geometry layer, by name. If no layer with the specified name exists, \c NULL is returned.
*/
PURPLEAPI PNGLayer * p_node_g_layer_find(PINode *node		/** The node whose layer is to be accessed. */,
					 const char *name	/** The name of the layer to look access. */)
{
	if(node->type != V_NT_GEOMETRY)
		return NULL;
	return nodedb_g_layer_find((NodeGeometry *) node, name);
}

/**
 * Return the size of a geometry layer, i.e. the number of "slots" it holds.
*/
PURPLEAPI size_t p_node_g_layer_get_size(const PNGLayer *layer	/** The layer whose size is to be queried. */)
{
	return nodedb_g_layer_get_size(layer);
}

/**
 * Return the name of a geometry layer.
*/
PURPLEAPI const char * p_node_g_layer_get_name(const PNGLayer *layer	/** The layer whose name is to be queried. */)
{
	return nodedb_g_layer_get_name(layer);
}

/**
 * Return the type of a geometry layer.
*/
PURPLEAPI VNGLayerType p_node_g_layer_get_type(const PNGLayer *layer	/** The layer whose type is to be queried. */)
{
	if(layer != NULL)
		return ((NdbGLayer *) layer)->type;
	return -1;
}

/**
 * Create a new geometry layer.
 * 
 * This function creates a new layer with the given name and type, in the given node. A pointer to the new layer
 * is returned, and can be immediately used to fill in the layer's content. If an error occurs in creating the
 * layer, \c NULL is returned.
*/
PURPLEAPI PNGLayer * p_node_g_layer_create(PONode *node		/** The node in which the new layer is to be created. */,
					   const char *name	/** The name of the new layer. Must be unique within the node. */,
					   VNGLayerType type	/** The type of the new layer. */,
					   uint32 def_int,	/** Default value for integer layers. */
					   real64 def_real	/** Default value for floating point layers. */)
{
	NdbGLayer	*l;

	l = nodedb_g_layer_create((NodeGeometry *) node, (VLayerID) ~0u, name, type);
	/* FIXME: Ignores default values. */
	return l;
}

/**
 * \brief Set value of an XYZ slot.
 * 
 * This function sets the value of a slot in an XYZ-type vertex layer. Each such slot holds three floating point
 * numbers, and is usually used to represent the location of a vertex.
*/
PURPLEAPI void p_node_g_vertex_set_xyz(PNGLayer *layer	/** The layer in which a slot is to be set. */,
				       uint32 id	/** The ID or index of the vertex to set. */,
				       real64 x		/** Value of the \a x part of the XYZ triplet. */,
				       real64 y		/** Value of the \a y part of the XYZ triplet. */,
				       real64 z		/** Value of the \a z part of the XYZ triplet. */)
{
	nodedb_g_vertex_set_xyz(layer, id, x, y, z);
}

/**
 * \brief Retreive value of an XYZ slot.
 * 
 * This function reads the value of a slot in an XYZ-type geometry layer. It is very useful when doing any kind of 
 * modification to vertex data.
*/
PURPLEAPI void p_node_g_vertex_get_xyz(const PNGLayer *layer	/** The layer in which a slot is to be queried. */,
				       uint32 id		/** The ID or index of the vertex to query. */,
				       real64 *x		/** Pointer to a \c real64 that is set to the X part of the XYZ triplet. */,
				       real64 *y		/** Pointer to a \c real64 that is set to the Y part of the XYZ triplet. */,
				       real64 *z		/** Pointer to a \c real64 that is set to the Z part of the XYZ triplet. */)
{
	nodedb_g_vertex_get_xyz(layer, id, x, y, z);
}

/**
 * \brief Set value of a corner \c uint32 slot.
 * 
 * This function sets the value of a slot in a corner integer-type layer. Each slot holds four \c uint32 numbers,
 * and is usually used to represent polygons.
 * 
 * Defining a polygon is done by setting each of the four integers to the IDs of the vertices that are to make up
 * the corners in a quad, in clock-wise order. Leaving out the last value, by setting it to \c ~0, defines a
 * single triangle instead. Because of how the underlying Verse datamodel works, using quads whenever possible
 * is very much recommended.
*/
PURPLEAPI void p_node_g_polygon_set_corner_uint32(PNGLayer *layer	/** The layer in which a slot is to be set. */,
						  uint32 id		/** The ID or index of the polygon to set. */,
						  uint32 v0,		/** Value of the first part of the corner quartet. */
						  uint32 v1		/** Value of the second part of the corner quartet. */,
						  uint32 v2,		/** Value of the third part of the corner quartet. */
						  uint32 v3		/** Value of the fourth part of the corner quartet. */)
{
	nodedb_g_polygon_set_corner_uint32(layer, id, v0, v1, v2, v3);
}

/**
 * \brief Retreive value of a corner integer slot.
 * 
 * This function reads the value of a slot in a corner integer-type layer.
 * 
*/ 
PURPLEAPI void p_node_g_polygon_get_corner_uint32(const PNGLayer *layer	/** The layer in which a slot is to be queried. */,
						  uint32 id		/** The ID or index of the polygon to query. */,
						  uint32 *v0		/** Pointer to \c uint32 that is set to the first part of the slot's quartet. */,
						  uint32 *v1		/** Pointer to \c uint32 that is set to the second part of the slot's quartet. */,
						  uint32 *v2		/** Pointer to \c uint32 that is set to the third part of the slot's quartet. */,
						  uint32 *v3		/** Pointer to \c uint32 that is set to the fourth part of the slot's quartet. */)
{
	nodedb_g_polygon_get_corner_uint32(layer, id, v0, v1, v2, v3);
}

/** \brief Set value of a corner real slot with 32-bit precision.
 * 
 * This function sets the value of a slot in a corner real-type layer. The values are set with 32-bit precision.
 */
PURPLEAPI void p_node_g_polygon_set_corner_real32(PNGLayer *layer	/** The layer in which a slot is to be set. */,
						  uint32 id		/** The ID or index of the slot to set. */,
						  real32 v0		/** Value of the first part of the slot's quartet. */,
						  real32 v1		/** Value of the second part of the slot's quartet. */,
						  real32 v2		/** Value of the third part of the slot's quartet. */,
						  real32 v3		/** Value of the fourth part of the slot's quartet. */)
{
	nodedb_g_polygon_set_corner_real32(layer, id, v0, v1, v2, v3);
}

/** \brief Retreive value of a polygon corner real slot with 32-bit precision.
 * 
 * This function reads the value of a slot in a corner real-type layer, with 32-bit precision.
*/
PURPLEAPI void p_node_g_polygon_get_corner_real32(const PNGLayer *layer	/** Layer in which a slot is to be queried. */,
						  uint32 id		/** The ID or index of the slot to query. */,
						  real32 *v0		/** Pointer to \c real32 that is set to the first part of the slot's quartet. */,
						  real32 *v1		/** Pointer to \c real32 that is set to the second part of the slot's quartet. */,
						  real32 *v2		/** Pointer to \c real32 that is set to the third part of the slot's quartet. */,
						  real32 *v3		/** Pointer to \c real32 that is set to the fourth part of the slot's quartet. */)
{
	nodedb_g_polygon_get_corner_real32(layer, id, v0, v1, v2, v3);
}

/** \brief Set value of a polygon corner real slot, with 64-bit precision.
 * 
 * This function sets the value of a slot in a corner real-type layer. The values are set with 64-bit precision.
*/
PURPLEAPI void p_node_g_polygon_set_corner_real64(PNGLayer *layer	/** The layer in which a slot is to be set. */,
						  uint32 id		/** The ID or index of the slot to set. */,
						  real64 v0		/** The first part of the slot's quartet. */,
						  real64 v1		/** The second part of the slot's quartet. */,
						  real64 v2		/** The third part of the slot's quartet. */,
						  real64 v3		/** The fourth part of the slot's quartet. */)
{
	nodedb_g_polygon_set_corner_real64(layer, id, v0, v1, v2, v3);
}

/** \brief Retrieve value of a polygon corner real slot, with 64-bit precision.
 * 
 * This function reads the value of a slot in a corner real-type layer, with 64-bit precision.
*/
PURPLEAPI void p_node_g_polygon_get_corner_real64(const PNGLayer *layer	/** The layer in which a slot is to be queried. */,
						  uint32 id		/** The ID or index of the slot to query. */,
						  real64 *v0		/** A pointer to a \c real64 that is set to the first part of the slot's quartet. */,
						  real64 *v1		/** A pointer to a \c real64 that is set to the second part of the slot's quartet. */,
						  real64 *v2		/** A pointer to a \c real64 that is set to the third part of the slot's quartet. */,
						  real64 *v3		/** A pointer to a \c real64 that is set to the fourth part of the slot's quartet. */)
{
	nodedb_g_polygon_get_corner_real64(layer, id, v0, v1, v2, v3);
}

/** \brief Set value of an 8-bit integer polygon face slot.
 * 
 * This function sets the value of a slot in an 8-bit integer polygon face slot.
*/
PURPLEAPI void p_node_g_polygon_set_face_uint8(PNGLayer *layer	/** The layer in which a slot is to be set. */,
					       uint32 id	/** The ID or index of the slot to set. */,
					       uint8 value	/** The new value to store in the slot. */)
{
	nodedb_g_polygon_set_face_uint8(layer, id, value);
}

/** \brief Retreieve value of a polygon face 8-bit integer slot.
 * 
 * This function reads the value of a slot in a an 8-bit corner integer polygon face slot. The value is
 * simply returned from the function, there is no need to pass a pointer to where to store it.
*/
PURPLEAPI uint8 p_node_g_polygon_get_face_uint8(const PNGLayer *layer	/** The layer in which a slot is to be queried. */,
						uint32 id		/** The ID or index of the slot to query. */)
{
	return nodedb_g_polygon_get_face_uint8(layer, id);
}

/** \brief Set value of a 32-bit polygon face integer slot.
 * 
 * This function sets the value of a slot in an 32-bit integer polygon face slot.
*/
PURPLEAPI void p_node_g_polygon_set_face_uint32(PNGLayer *layer	/** The layer in which a slot is to be set. */,
						uint32 id	/** The ID or index of the slot to set. */,
						uint32 value	/** The new value to store in the slot. */)
{
	nodedb_g_polygon_set_face_uint32(layer, id, value);
}

/** \brief Retreieve value of a polygon face 32-bit integer slot.
 * 
 * This function reads the value of a slot in a an 32-bit face integer polygon face slot. The value is
 * simply returned from the function, there is no need to pass a pointer to where to store it.
*/
PURPLEAPI uint32 p_node_g_polygon_get_face_uint32(const PNGLayer *layer	/** The layer in which a slot is to be queried. */,
						  uint32 id		/** The ID or index of the slot to query. */)
{
	return nodedb_g_polygon_get_face_uint32(layer, id);
}

/** \brief Set value of a 64-bit polygon face real slot.
 * 
 * This function sets the value of a slot in an 32-bit real polygon face slot.
*/
PURPLEAPI void p_node_g_polygon_set_face_real64(PNGLayer *layer	/** The layer in which a slot is to be set. */,
						uint32 id	/** The ID or index of the slot to set. */,
						real64 value	/** The new value to store in the slot. */)
{
	nodedb_g_polygon_set_face_real64(layer, id, value);
}

/** \brief Retreieve value of a polygon face 64-bit real slot.
 * 
 * This function reads the value of a slot in a an 64-bit face real polygon face slot. The value is
 * simply returned from the function, there is no need to pass a pointer to where to store it.
*/
PURPLEAPI real64 p_node_g_polygon_get_face_real64(const PNGLayer *layer	/** The layer in which a slot is to be queried. */,
						  uint32 id		/** The ID or index of the slot to query. */)
{
	return nodedb_g_polygon_get_face_real64(layer, id);
}

/** \brief Set vertex creasing.
 * 
 * This function lets you specify the vertex creasing that is to be applied to a geometry node. Creasing controls
 * subdivision, by making vertices either hard (sharp, not subdivided) or smooth (subdivided), or anything in between.
 *
 * Crease values are 32-bit unsigned integers, where 0 means "no crease, smooth" and 0xfffffff means "maximum crease, sharp".
*/
PURPLEAPI void p_node_g_crease_set_vertex(PONode *node		/** The node whose creasing is to be set. */,
					  const char *layer	/** Name of a vertex integer layer holding per-vertex crease values. */,
					  uint32 def		/** Default value, if layer name is empty. Applied to all vertices. */)
{
	if(node == NULL || layer == NULL)
		return;
	nodedb_g_crease_set_vertex((NodeGeometry *) node, layer, def);
}

/** \brief Set edge creasing.
 * 
 * This function lets you specify the edge creasing that is to be applied to a geometry node. Creasing controls
 * subdivision, by making edges either hard (sharp, not subdivided) or smooth (subdivided), or anything in between.
 *
 * Crease values are 32-bit unsigned integers, where 0 means "no crease, smooth" and 0xfffffff means "maximum crease, sharp".
 * They are stored in corner integer layers, but applied to the edges between corners rather than to the corners themselves.
*/
PURPLEAPI void p_node_g_crease_set_edge(PONode *node		/** The node whose creasing is to be set. */,
					const char *layer	/** Name of a polygon corner integer layer holding per-edge crease values. */,
					uint32 def		/** Default value, if layer name is empty. Applied to all edges. */)
{
	if(node == NULL || layer == NULL)
		return;
	nodedb_g_crease_set_edge((NodeGeometry *) node, layer, def);
}

/* ----------------------------------------------------------------------------------------- */

/** \brief Return the number of fragments in a material node.
 * 
 * This function returns the number of fragments in a materal node. Material fragments are "nodes" in a graph-based
 * system that specify a computation to be made over a surface.
*/
PURPLEAPI unsigned int p_node_m_fragment_num(PINode *node	/** The node to be queried. */)
{
	return nodedb_m_fragment_num((NodeMaterial *) node);
}

/** \brief Retrieve material fragment by index.
 * 
 * Return a material fragment, by index. Valid index range is 0 up to, but not including, the value returned by the
 * \c p_node_m_fragment_num() function. Specifying an index outside of this range causes \c NULL to be returned.
*/
PURPLEAPI PNMFragment * p_node_m_fragment_nth(PINode *node	/** The node whose fragment is to be accessed. */,
					    unsigned int n	/** The index of the fragment to access. */)
{
	return nodedb_m_fragment_nth((NodeMaterial *) node, n);
}

/** \brief Initialize iterator over a material node's fragments.
 *
 * This function sets an iterator to point at the first fragment in a material node, and
 * sets it up to iterate over all the others.
*/
PURPLEAPI void p_node_m_fragment_iter(PINode *node	/** The node whose fragments are to be iterated. */,
				      PIter *iter	/** The iterator to initialize. */)
{
	iter_init_dynarr_enum_negative(iter, ((NodeMaterial *) node)->fragments, offsetof(NdbMFragment, type));
}

/** \brief Return type of a material fragment.
 * 
 * This function returns the type of a material fragment. If the fragment pointer is invalid, -1 is returned.
*/
PURPLEAPI VNMFragmentType p_node_m_fragment_get_type(const PNMFragment *fragment	/** The fragment whose type is to be queried. */)
{
	return fragment != NULL ? ((NdbMFragment *) fragment)->type : (VNMFragmentType) -1;
}

/** \brief Create a new color fragment.
 * 
 * This function creates a new fragment of type Color in a material node. Color fragments are used
 * to introduce constant RGB colors into a material graph.
*/
PURPLEAPI PNMFragment * p_node_m_fragment_create_color(PONode *node, real64 red, real64 green, real64 blue)
{
	return nodedb_m_fragment_create_color((NodeMaterial *) node, red, green, blue);
}

/** \brief Create a new light fragment.
 * 
 * This function creates a new fragment of type Light in a material node. Light fragments are used
 * to represent light falling on a surface.
*/
PURPLEAPI PNMFragment * p_node_m_fragment_create_light(PONode *node, VNMLightType type, real64 normal_falloff,
						       PINode *brdf, const char *brdf_red, const char *brdf_green, const char *brdf_blue)
{
	return nodedb_m_fragment_create_light((NodeMaterial *) node, type, normal_falloff, (Node *) brdf,
					      brdf_red, brdf_green, brdf_blue);
}

PURPLEAPI PNMFragment * p_node_m_fragment_create_reflection(PONode *node, real64 normal_falloff)
{
	return nodedb_m_fragment_create_reflection((NodeMaterial *) node, normal_falloff);
}

PURPLEAPI PNMFragment * p_node_m_fragment_create_transparency(PONode *node, real64 normal_falloff, real64 refraction_index)
{
	return nodedb_m_fragment_create_transparency((NodeMaterial *) node, normal_falloff, refraction_index);
}

PURPLEAPI PNMFragment * p_node_m_fragment_create_volume(PONode *node,  real64 diffusion, real64 col_r, real64 col_g, real64 col_b,
							const PNMFragment *color)
{
	return nodedb_m_fragment_create_volume((NodeMaterial *) node, diffusion, col_r, col_g, col_b, color);
}

PURPLEAPI PNMFragment * p_node_m_fragment_create_geometry(PONode *node,
						const char *layer_r, const char *layer_g, const char *layer_b)
{
	return nodedb_m_fragment_create_geometry((NodeMaterial *) node, layer_r, layer_g, layer_b);
}

PURPLEAPI PNMFragment * p_node_m_fragment_create_texture(PONode *node, PINode *bitmap,
					    const char *layer_r, const char *layer_g, const char *layer_b,
					    const PNMFragment *mapping)
{
	return nodedb_m_fragment_create_texture((NodeMaterial *) node, bitmap, layer_r, layer_g, layer_b, mapping);
}

PURPLEAPI PNMFragment * p_node_m_fragment_create_noise(PONode *node, VNMNoiseType type, const PNMFragment *mapping)
{
	return nodedb_m_fragment_create_noise((NodeMaterial *) node, type, mapping);
}

PURPLEAPI PNMFragment * p_node_m_fragment_create_blender(PONode *node, VNMBlendType type,
					       const PNMFragment *data_a, const PNMFragment *data_b, const PNMFragment *ctrl)
{
	return nodedb_m_fragment_create_blender((NodeMaterial *) node, type, data_a, data_b, ctrl);
}

PURPLEAPI PNMFragment * p_node_m_fragment_create_matrix(PONode *node, const real64 *matrix, const PNMFragment *data)
{
	return nodedb_m_fragment_create_matrix((NodeMaterial *) node, matrix, data);
}

PURPLEAPI PNMFragment * p_node_m_fragment_create_ramp(PONode *node, VNMRampType type, uint8 channel,
						      const PNMFragment *mapping, uint8 point_count,
						      const VNMRampPoint *ramp)
{
	return nodedb_m_fragment_create_ramp((NodeMaterial *) node, type, channel, mapping, point_count, ramp);
}

PURPLEAPI PNMFragment * p_node_m_fragment_create_animation(PONode *node, const char *label)
{
	return nodedb_m_fragment_create_animation((NodeMaterial *) node, label);
}

PURPLEAPI PNMFragment * p_node_m_fragment_create_alternative(PONode *node, const PNMFragment *alt_a, const PNMFragment *alt_b)
{
	return nodedb_m_fragment_create_alternative((NodeMaterial *) node, alt_a, alt_b);
}

PURPLEAPI PNMFragment * p_node_m_fragment_create_output(PONode *node, const char *label, const PNMFragment *front, const PNMFragment *back)
{
	return nodedb_m_fragment_create_output((NodeMaterial *) node, label, front, back);
}

/* ----------------------------------------------------------------------------------------- */

PURPLEAPI void p_node_b_set_dimensions(PONode *node, uint16 width, uint16 height, uint16 depth)
{
	if(node == NULL)
		return;
	nodedb_b_set_dimensions((NodeBitmap *) node, width, height, depth);
}

PURPLEAPI void p_node_b_get_dimensions(PINode *node, uint16 *width, uint16 *height, uint16 *depth)
{
	nodedb_b_get_dimensions((NodeBitmap *) node, width, height, depth);
}

PURPLEAPI unsigned int p_node_b_layer_num(PINode *node)
{
	return nodedb_b_layer_num((NodeBitmap *) node);
}

PURPLEAPI PNBLayer * p_node_b_layer_nth(PINode *node, unsigned int n)
{
	return nodedb_b_layer_nth((NodeBitmap *) node, n);
}

PURPLEAPI PNBLayer * p_node_b_layer_find(PINode *node, const char *name)
{
	return nodedb_b_layer_find((NodeBitmap *) node, name);
}

PURPLEAPI const char * p_node_b_layer_get_name(const PNBLayer *layer)
{
	if(layer != NULL)
		return ((NdbBLayer *) layer)->name;
	return NULL;
}

PURPLEAPI VNBLayerType p_node_b_layer_get_type(const PNBLayer *layer)
{
	if(layer != NULL)
		return ((NdbBLayer *) layer)->type;
	return -1;
}

PURPLEAPI PNBLayer * p_node_b_layer_create(PONode *node, const char *name, VNBLayerType type)
{
	PNBLayer	*l;

	if(node == NULL)
		return NULL;
	if((l = nodedb_b_layer_find((NodeBitmap *) node, name)) != NULL)
		return l;
	return nodedb_b_layer_create((NodeBitmap *) node, ~0, name, type);
}

PURPLEAPI void * p_node_b_layer_access_begin(PONode *node, PNBLayer *layer)
{
	return nodedb_b_layer_access_begin((NodeBitmap *) node, layer);
}

PURPLEAPI void p_node_b_layer_access_end(PONode *node, PNBLayer *layer, void *framebuffer)
{
	nodedb_b_layer_access_end((NodeBitmap *) node, layer, framebuffer);
}

PURPLEAPI void p_node_b_layer_foreach_set(PONode *node, PNBLayer *layer,
				real64 (*pixel)(uint32 x, uint32 y, uint32 z, void *user), void *user)
{
	nodedb_b_layer_foreach_set((NodeBitmap *) node, layer, pixel, user);
}

PURPLEAPI void * p_node_b_layer_access_multi_begin(PONode *node, VNBLayerType format, ...)
{
	va_list	layers;
	void	*fb;

	va_start(layers, format);
	fb = nodedb_b_layer_access_multi_begin((NodeBitmap *) node, format, layers);
	va_end(layers);
	return fb;
}

PURPLEAPI void p_node_b_layer_access_multi_end(PONode *node, void *framebuffer)
{
	nodedb_b_layer_access_multi_end((NodeBitmap *) node, framebuffer);
}

/* ----------------------------------------------------------------------------------------- */

PURPLEAPI unsigned int p_node_c_curve_num(PINode *node)
{
	return nodedb_c_curve_num((NodeCurve *) node);
}

PURPLEAPI PNCCurve * p_node_c_curve_nth(PINode *node, unsigned int n)
{
	return nodedb_c_curve_nth((NodeCurve *) node, n);
}

PURPLEAPI PNCCurve * p_node_c_curve_find(PINode *node, const char *name)
{
	return nodedb_c_curve_find((NodeCurve *) node, name);
}

PURPLEAPI void p_node_c_curve_iter(PINode *node, PIter *iter)
{
	if(node != NULL)
		iter_init_dynarr_string(iter, ((NodeCurve *) node)->curves, offsetof(NodeCurve, curves));
}

PURPLEAPI const char * p_node_c_curve_get_name(const PNCCurve *curve)
{
	if(curve != NULL)
		return ((NdbCCurve *) curve)->name;
	return NULL;
}

PURPLEAPI uint8 p_node_c_curve_get_dimensions(const PNCCurve *curve)
{
	if(curve != NULL)
		return ((NdbCCurve *) curve)->dimensions;
	return 0;
}

PURPLEAPI PNCCurve * p_node_c_curve_create(PONode *node, const char *name, uint8 dimensions)
{
	PNCCurve	*c;

	if((c = nodedb_c_curve_find((NodeCurve *) node, name)) != NULL)
		return c;
	return nodedb_c_curve_create((NodeCurve *) node, ~0, name, dimensions);
}

PURPLEAPI void p_node_c_curve_destroy(PONode *node, PNCCurve *curve)
{
	nodedb_c_curve_destroy((NodeCurve *) node, curve);
}

PURPLEAPI size_t p_node_c_curve_key_num(const PNCCurve *curve)
{
	return nodedb_c_curve_key_num(curve);
}

PURPLEAPI PNCKey * p_node_c_curve_key_nth(const PNCCurve *curve, unsigned int n)
{
	return nodedb_c_curve_key_nth(curve, n);
}

PURPLEAPI real64 p_node_c_curve_key_get_pos(const PNCKey *key)
{
	return key != NULL ? ((NdbCKey *) key)->pos : 0.0;
}

PURPLEAPI real64 p_node_c_curve_key_get_value(const PNCKey *key, uint8 dimension)
{
	return key != NULL ? ((NdbCKey *) key)->value[dimension] : 0.0;
}

PURPLEAPI uint32 p_node_c_curve_key_get_pre(const PNCKey *key, uint8 dimension, real64 *value)
{
	if(key != NULL)
	{
		if(value != NULL)
			*value = ((NdbCKey *) key)->pre.value[dimension];
		return ((NdbCKey *) key)->pre.pos[dimension];
	}
	return 0;
}

PURPLEAPI uint32 p_node_c_curve_key_get_post(const PNCKey *key, uint8 dimension, real64 *value)
{
	if(key != NULL)
	{
		if(value != NULL)
			*value = ((NdbCKey *) key)->post.value[dimension];
		return ((NdbCKey *) key)->post.pos[dimension];
	}
	return 0;
}

PURPLEAPI PNCKey * p_node_c_curve_key_create(PNCCurve *curve,
					     real64 pos, const real64 *value,
					     const uint32 *pre_pos, const real64 *pre_value,
					     const uint32 *post_pos, const real64 *post_value)
{
	return (PNCKey *) nodedb_c_key_create(curve, ~0, pos, value, pre_pos, pre_value, post_pos, post_value);
}

PURPLEAPI void p_node_curve_key_destroy(PNCCurve *curve, PNCKey *key)
{
	nodedb_c_key_destroy((NdbCCurve *) curve, (NdbCKey *) key);
}

/* ----------------------------------------------------------------------------------------- */

PURPLEAPI const char * p_node_t_language_get(PINode *node)
{
	return nodedb_t_language_get((NodeText *) node);
}

PURPLEAPI void p_node_t_langauge_set(PONode *node, const char *language)
{
	nodedb_t_language_set((NodeText *) node, language);
}

PURPLEAPI unsigned int p_node_t_buffer_get_num(PINode *node)
{
	return nodedb_t_buffer_num((NodeText *) node);
}

PURPLEAPI PNTBuffer * p_node_t_buffer_nth(PINode *node, unsigned int n)
{
	return nodedb_t_buffer_nth((NodeText *) node, n);
}

PURPLEAPI PNTBuffer * p_node_t_buffer_find(PINode *node, const char *name)
{
	return nodedb_t_buffer_find((NodeText *) node, name);
}

PURPLEAPI const char * p_node_t_buffer_get_name(const PNTBuffer *buffer)
{
	if(buffer != NULL)
		return ((NdbTBuffer *) buffer)->name;
	return NULL;
}

PURPLEAPI PNTBuffer * p_node_t_buffer_create(PONode *node, const char *name)
{
	PNTBuffer	*b;

	if((b = nodedb_t_buffer_find((NodeText *) node, name)) != NULL)
	{
		nodedb_t_buffer_clear(b);
		return b;
	}
	return nodedb_t_buffer_create((NodeText *) node, ~0, name);
}

PURPLEAPI void p_node_t_buffer_insert(PNTBuffer *buffer, size_t pos, const char *text)
{
	return nodedb_t_buffer_insert(buffer, pos, text);
}

PURPLEAPI void p_node_t_buffer_delete(PNTBuffer *buffer, size_t pos, size_t length)
{
	return nodedb_t_buffer_delete(buffer, pos, length);
}

PURPLEAPI void p_node_t_buffer_append(PNTBuffer *buffer, const char *text)
{
	return nodedb_t_buffer_append(buffer, text);
}

PURPLEAPI char * p_node_t_buffer_read_line(PNTBuffer *buffer, unsigned int line, char *put, size_t putmax)
{
	return nodedb_t_buffer_read_line(buffer, line, put, putmax);
}

/* ----------------------------------------------------------------------------------------- */

PURPLEAPI unsigned int p_node_a_buffer_get_num(PINode *node)
{
	return nodedb_a_buffer_num((NodeAudio *) node);
}

PURPLEAPI PNABuffer * p_node_a_buffer_nth(PINode *node, unsigned int n)
{
	return nodedb_a_buffer_nth((NodeAudio *) node, n);
}

PURPLEAPI PNABuffer * p_node_a_buffer_find(PINode *node, const char *name)
{
	return nodedb_a_buffer_find((NodeAudio *) node, name);
}

PURPLEAPI const char * p_node_a_buffer_get_name(const PNABuffer *layer)
{
	if(layer != NULL)
		return ((NdbABuffer *) layer)->name;
	return NULL;
}

PURPLEAPI real64 p_node_a_buffer_get_frequency(const PNABuffer *layer)
{
	return layer != NULL ? ((NdbABuffer *) layer)->frequency : 0.0;
}

PURPLEAPI PNABuffer * p_node_a_buffer_create(PONode *node, const char *name, VNABlockType type, real64 frequency)
{
	PNABuffer	*l;

	if(node == NULL)
		return NULL;
	if((l = nodedb_a_buffer_find((NodeAudio *) node, name)) != NULL)
		return l;
	return nodedb_a_buffer_create((NodeAudio *) node, ~0, name, type, frequency);
}

PURPLEAPI unsigned int p_node_a_buffer_read_samples(const PNABuffer *buffer, unsigned int start, real64 *samples, unsigned int length)
{
	return nodedb_a_buffer_read_samples((NdbABuffer *) buffer, start, samples, length);
}

PURPLEAPI void p_node_a_buffer_write_samples(PNABuffer *buffer, unsigned int start, const real64 *samples, unsigned int length)
{
	nodedb_a_buffer_write_samples((NdbABuffer *) buffer, start, samples, length);
}
