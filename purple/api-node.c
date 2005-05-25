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

/** \defgroup api_node Node Functions
 * @{
*/

/** \defgroup api_node_common Common Functions
 * \ingroup api_node
 * 
 * These are functions that work on nodes of all types. They let you get and set a node's name,
 * as well as work with any \e tags the node might contain.
 * @{
*/

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
PURPLEAPI const char * p_node_get_name(const PNode *node	/** The node whose name is to be queried. */)
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
	iter_init_dynarr_string(iter, ((PNode *) node)->tag_groups, offsetof(NdbTagGroup, name));
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
 * \brief Destroy a tag.
 * 
 * This function removes a tag from a tag group.
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
 * 
 * Here's how to set three tags in a group called "physics", perhaps to work as inputs for some kind of
 * physics simulator:
 * \code
 * real64 cog[] = { 0.0, 1.0, 0.0 };	// One meter up from the origin.
 * 
 * p_node_tag_create_path(node, "physics/mass", VN_TAG_REAL64, 100.0);    // Mass 100 kg.
 * p_node_tag_create_path(node, "physics/cog",  VN_TAG_REAL64_VEC3, cog); // Centre of gravity.
 * p_node_tag_create_path(node, "physics/rigid", VN_TAG_BOOLEAN, 1);      // Object has rigid body.
 * \endcode
 * \note Purple does not itself contain or define any physics simulation system; the above is
 * simply an example of what such a system might use.
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

/** @} */

/* ----------------------------------------------------------------------------------------- */

/** \defgroup api_node_object Object Node Functions
 * \ingroup api_node
 * 
 * These are functions that work only on object nodes. Object nodes in Verse can be light sources, and
 * they can link to other nodes. Both of these features are exposed in the API.
 * @{
*/

/**
 * \brief Set the light values for an object node.
 * 
 * This function sets the RGB color of light emitted by an object, effectively turning it into a light
 * source. A setting of (0,0,0) turns the light off.
*/
PURPLEAPI void p_node_o_light_set(PONode *node	/** The object node whose light values are to be set. */,
				  real64 red	/** Amount of red light emitted by the object. */,
				  real64 green	/** Amount of green light emitted by the object. */,
				  real64 blue	/** Amount of blue light emitted by the object. */)
{
	nodedb_o_light_set((NodeObject *) node, red, green, blue);
}

/**
 * \brief Retrieve the light values for an object node.
 * 
 * This function retreives the current light settings of an object node, and fills in the provided
 * variables with the values.
*/
PURPLEAPI void p_node_o_light_get(PINode *node	/** The object node whose light values are to be read. */,
				  real64 *red	/** Pointer to \c real64 that is set to the amount of red light emitted by the object. */,
				  real64 *green	/** Pointer to \c real64 that is set to the amount of green light emitted by the object. */,
				  real64 *blue	/** Pointer to \c real64 that is set to the amount of blue light emitted by the object. */)
{
	nodedb_o_light_get((NodeObject *) node, red, green, blue);
}

/**
 * \brief Set a link from an object node to another node.
 *
 * This function adds a link from an object node to some other node. The type of the destination node need not be
 * an object and in many cases isn't.
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

/** @} */

/* ----------------------------------------------------------------------------------------- */

/** \defgroup api_node_geometry Geometry Node Functions
 * 
 * These are functions for working with geometry nodes. Geometry is stored in layers, which are
 * typed. Each layerb consists of a number of "slots" where the data is stored. The number of
 * actual values stored in a slot varies with the type of the layer; one, three, or four.
 * 
 * Geometry nodes also contain information about how the defined mesh is to be creased during
 * subdividion, and has the ability to define bones for animation. Bones are at the moment
 * not supported in Purple.
 * 
 * \see The Verse specification on the geometry node: <http://www.blender.org/modules/verse/verse-spec/n-geometry.html>.
 * @{
*/

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

/** \brief Destroy a geometry layer.
 * 
 * This function destroys a geometry layer. After being destroyed, the layer pointer is no longer valid and
 * must not be referenced further.
*/
PURPLEAPI void p_node_g_layer_destroy(PONode *node	/** The node in which a layer is to be destroyed. */,
				      PNGLayer *layer	/** The layer to be destroyed. */)
{
	nodedb_g_layer_destroy((NodeGeometry *) node, layer);
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

/** @} */

/* ----------------------------------------------------------------------------------------- */

/** \defgroup api_node_material Material Node Functions
 * \ingroup api_node
 *
 * These are functions for working with material nodes. You can iterate over existing fragments, as well as create
 * new fragments of all the supported types. Fragments are referenced by pointers to the opaque type \c PNMFragment.
 * 
 * \note Currently there is no functionality for actually reading out the contents of an existing fragment. This
 * is something that will have to be added fairly soon, though.
 * 
 * \see The Verse specification on the material node: <http://www.blender.org/modules/verse/verse-spec/n-material.html>.
 * @{
*/

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
 * This function creates a new fragment of type color in a material node. Color fragments are used
 * to introduce constant RGB colors into a material graph.
*/
PURPLEAPI PNMFragment * p_node_m_fragment_create_color(PONode *node	/** The node in which a fragment is to be created. */,
						       real64 red	/** The red component of the color. */,
						       real64 green	/** The green component of the color. */,
						       real64 blue	/** The blue component of the color. */)
{
	return nodedb_m_fragment_create_color((NodeMaterial *) node, red, green, blue);
}

/** \brief Create a new light fragment.
 * 
 * This function creates a new fragment of type light in a material node. Light fragments are used
 * to represent light falling on a surface. There are several different types of light fragment; see
 * the Verse specification for details.
*/
PURPLEAPI PNMFragment * p_node_m_fragment_create_light(PONode *node		/** The node in which a fragment is to be created. */,
						       VNMLightType type	/** The type of light fragment to create. */,
						       real64 normal_falloff	/** The normal falloff, controls how shiny the surface is. */,
						       PINode *brdf		/** Pointer to a node. */,
						       const char *brdf_red, const char *brdf_green, const char *brdf_blue)	/* FIXME: Document! */
{
	return nodedb_m_fragment_create_light((NodeMaterial *) node, type, normal_falloff, (PNode *) brdf,
					      brdf_red, brdf_green, brdf_blue);
}

/** \brief Create a new reflection fragment.
 * 
 * This function creates a new fragment of type reflection in a material node. Reflection fragments
 * are used to represent light being reflected off of a surface. They are incorporated into materials
 * to make them more "shiny".
*/
PURPLEAPI PNMFragment * p_node_m_fragment_create_reflection(PONode *node		/** The node in which a fragment is to be created. */,
							    real64 normal_falloff	/** The normal falloff, controls how shiny the reflection is. */)
{
	return nodedb_m_fragment_create_reflection((NodeMaterial *) node, normal_falloff);
}

/** \brief Create a new transparency fragment.
 * 
 * This function creates a new fragment of type transparency in a material node. Transparency fragments
 * are used to represent the light falling on the back side of a surface, i.e. the light that would reach
 * the viewpoint if the surface was not there.
*/
PURPLEAPI PNMFragment * p_node_m_fragment_create_transparency(PONode *node		/** The node in which a fragment is to be created. */,
							      real64 normal_falloff	/** Micro-geometry parameter, defines how smooth the transparency is. */,
							      real64 refraction_index	/** Index of refraction as light passes through the surface. */)
{
	return nodedb_m_fragment_create_transparency((NodeMaterial *) node, normal_falloff, refraction_index);
}

/** \brief Create a new volume fragment.
 * 
 * Create a new fragment of type volume in a material node.
*/
PURPLEAPI PNMFragment * p_node_m_fragment_create_volume(PONode *node, real64 diffusion, real64 col_r, real64 col_g, real64 col_b,
							const PNMFragment *color)
{
	return nodedb_m_fragment_create_volume((NodeMaterial *) node, diffusion, col_r, col_g, col_b, color);
}

/** \brief Create a new geometry fragment.
 * 
 * Create a new fragment of type geometry in a material node. Geometry fragments are used to import data from
 * geometry layers. Most typically used to express various forms of mapping, by reading mapping coordinates
 * from geometry layers.
*/
PURPLEAPI PNMFragment * p_node_m_fragment_create_geometry(PONode *node		/** The node in which a fragment is to be created. */,
							  const char *layer_r	/** Name of the geometry layer from which 'red' data is to be read. */,
							  const char *layer_g	/** Name of the geometry layer from which 'green' data is to be read. */,
							  const char *layer_b	/** Name of the geometry layer from which 'blue' data is to be read. */)
{
	return nodedb_m_fragment_create_geometry((NodeMaterial *) node, layer_r, layer_g, layer_b);
}

/** \brief Create a new texture fragment.
 * 
 * Create a new fragment of type texture in a material node. Texture fragments are used to import data from
 * bitmap layers. Most typically used to express various forms of mapping, by storing color in a bitmap.
*/
PURPLEAPI PNMFragment * p_node_m_fragment_create_texture(PONode *node		/** The node in which a fragment is to be created. */,
							 PINode *bitmap		/** Reference to a texture to read from. */,
							 const char *layer_r	/** Name of layer to read 'red' data from. */,
							 const char *layer_g	/** Name of layer to read 'green' data from. */,
							 const char *layer_b	/** Name of layer to read 'blue' data from. */,
							 const PNMFragment *mapping	/** Fragment to use for mapping current point on surface into a point on the bitmap. See \c p_node_m_fragment_create_geometry(). */)
{
	return nodedb_m_fragment_create_texture((NodeMaterial *) node, bitmap, layer_r, layer_g, layer_b, mapping);
}

/** \brief Create a new noise fragment.
 * 
 * Create a new fragment of type noise in a material node. Noise fragments are used to represent random
 * noise. The noise is assumed to be generated procedurally during rendering.
*/
PURPLEAPI PNMFragment * p_node_m_fragment_create_noise(PONode *node		/** The node in which a fragment is to be created. */,
						       VNMNoiseType type	/** The type of noise to generate. See Verse specification. */,
						       const PNMFragment *mapping	/** Fragment to use for mapping current point on surface into a point in the noise. */)
{
	return nodedb_m_fragment_create_noise((NodeMaterial *) node, type, mapping);
}

/** \brief Create a new blender fragment.
 * 
 * Create a new fragment of type blender in a material node. Blender fragments are used to perform
 * arithmetic on the values of two other fragments, optionally controlled by a third. Blenders can
 * represent the addition, multiplication, and fade of two light sources, among other things. These
 * are very central operations, and most real-world material graphs will use many blender fragments.
*/
PURPLEAPI PNMFragment * p_node_m_fragment_create_blender(PONode *node		/** The node in which a fragment is to be created. */,
							 VNMBlendType type	/** The type of blender to create. The type controls the operation performed. See the Verse specification. */,
							 const PNMFragment *data_a	/** Fragment to use as one of the data sources/operands. */,
							 const PNMFragment *data_b	/** Fragment to use as the other data source/operand. */,
							 const PNMFragment *ctrl	/** Fragment that controls the operation. Often left unconnected, depending on the operation performed. */)
{
	return nodedb_m_fragment_create_blender((NodeMaterial *) node, type, data_a, data_b, ctrl);
}

/** \brief Create a new matrix fragment.
 * 
 * Create a new fragment of type matrix in a material node. Matrix fragments represent a standard transform
 * of the input values by a 4x4 matrix. This can be used to scale certain components, to swizzle components,
 * and so on.
*/
PURPLEAPI PNMFragment * p_node_m_fragment_create_matrix(PONode *node		/** The node in which a fragment is to be created. */,
							const real64 *matrix	/** The values for the matrix. Should be 16 \c real64 values, in column-major format. */,
							const PNMFragment *data	/** Input data to transform by the matrix. */)
{
	return nodedb_m_fragment_create_matrix((NodeMaterial *) node, matrix, data);
}

/** \brief Create a new ramp fragment.
 * 
 * Create a new fragment of type ramp in a material node. Ramp fragments act as 1D textures, defined as
 * interpolated gradients between a number of fixed points.
*/
PURPLEAPI PNMFragment * p_node_m_fragment_create_ramp(PONode *node	/** The node in which a fragment is to be created. */,
						      VNMRampType type	/** The kind of interpolation done by the ramp. See the Verse specification. */,
						      uint8 channel	/** Which input channel to use as index into the ramp (0=red, 1=green, 2=blue). */,
						      const PNMFragment *mapping	/** Input data to use as index into the ramp. */,
						      uint8 point_count	/** Number of points in ramp. Must be in range [1,48]. */,
						      const VNMRampPoint *ramp	/** Ramp points. See Verse specification for the data structure's definition. */)
{
	return nodedb_m_fragment_create_ramp((NodeMaterial *) node, type, channel, mapping, point_count, ramp);
}

/** \brief Create a new animation fragment.
 * 
 * Create a new fragment of type animation in a material node. Animation fragments are used to refer to
 * the parent object's animation data in order to create animated materials.
*/
/* FIXME: DOCUMENT! */
PURPLEAPI PNMFragment * p_node_m_fragment_create_animation(PONode *node		/** The node in which a fragment is to be created. */,
							   const char *label	/** Label. */)
{
	return nodedb_m_fragment_create_animation((NodeMaterial *) node, label);
}

/** \brief Create a new alternative fragment.
 * 
 * Create a new fragment of type alternative in a material node. Alternative fragments split the fragment path in two,
 * allowing the parser to choose one of the children.
*/
PURPLEAPI PNMFragment * p_node_m_fragment_create_alternative(PONode *node	/** The node in which a fragment is to be created. */,
							     const PNMFragment *alt_a	/** One path. */,
							     const PNMFragment *alt_b	/** The other path. */)
{
	return nodedb_m_fragment_create_alternative((NodeMaterial *) node, alt_a, alt_b);
}

/** \brief Create a new output fragment.
 * 
 * Create a new fragment of type output in a material node. Outputs are the roots of individual material trees; in order
 * for a material node to produce a useful result, it must have at least one output fragment. Output fragments are labelled,
 * and the label declares what property of geometry is being described by the tree rooted in that fragment. The most
 * common label is "color", which indicates an ordinary material description. Other choices include "displacement", to express
 * geometry deformation.
 * 
 * An output is actually the root of \b two trees; one for the front side of surfaces using the material, and one for the
 * back side. If either of these is left unconnected, surfaces do not have a valid material if viewed from that direction,
 * and thus will not be visible. Making \c front ==  \c back creates a two-sided material that looks the same from either side.
 * 
 * Here's an example of creating a material that defines color as the (multiplicative) blend of
 * incoming light (direct and ambient) and a color:
 * \code
 * PNMFragment	*color, *light, *blender, *output;
 * 
 * color = p_node_m_fragment_create_color(node, 0.7, 0.2, 0.3);
 * light = p_node_m_fragment_create_light(node, VN_M_LIGHT_DIRECT_AND_AMBIENT, 0.0, NULL, NULL, NULL);	// No BRDF.
 * blender = p_node_m_fragment_create_blender(node, VN_M_BLEND_MULTIPLY, color, light, NULL);
 * output = p_node_m_fragment_create_output(node, "color", blender, NULL);	// Single-sided material.
 * \endcode
*/
PURPLEAPI PNMFragment * p_node_m_fragment_create_output(PONode *node		/** The node in which a fragment is to be created. */,
							const char *label	/** The label of the output, defines the type of material described. */,
							const PNMFragment *front	/** The first fragment in the definition of the front side of the  material. */,
							const PNMFragment *back		/** The first fragment in the definition of the back side of the material. */)
{
	return nodedb_m_fragment_create_output((NodeMaterial *) node, label, front, back);
}

/** @} */

/* ----------------------------------------------------------------------------------------- */

/** \defgroup api_node_bitmap Bitmap Node Functions
 * \ingroup api_node
 * 
 * These are functions for working with bitmap nodes. Bitmaps are arrays of pixels, in one to three
 * dimensions (inclusive).
 * 
 * \see The Verse specification on the bitmap node: <http://www.blender.org/modules/verse/verse-spec/n-bitmap.html>.
 * 
 * @{
*/

/** \brief Set size of a bitmap.
 * 
 * Set the size of a bitmap. All layers in a bitmap share the same size, specified by this function. The size is
 * given in pixels for each of the three dimensions. Flat and linear textures are supported too, but any size
 * that is 1 must come after any dimension that isn't. So, (1,3,1) is not a valid size, but (3,1,1) is. No
 * size can be zero, as that would create a null volume of pixels. There are no other limitations on the size of a bitmap.
*/
PURPLEAPI void p_node_b_set_dimensions(PONode *node	/** The node whose size is to be set. */,
				       uint16 width	/** The width of the bitmap, in pixels. */,
				       uint16 height	/** The height of the bitmap, in pixels. Set to 1 for a 1D linear bitmap. */,
				       uint16 depth	/** The depth of the bitmap, in pixels. Set to 1 for a 1D or 2D bitmap. */)
{
	if(node == NULL)
		return;
	nodedb_b_set_dimensions((NodeBitmap *) node, width, height, depth);
}

/** \brief Retreive size of a bitmap.
 * 
 * Get the size of a bitmap. All layers in a bitmap share the same size. The size is set by the \c p_node_b_set_dimensions() function.
*/
PURPLEAPI void p_node_b_get_dimensions(PINode *node	/** The node whose size is to be retrieved. */,
				       uint16 *width	/** Pointer to \c uint16 that is set to the width of the bitmap. */,
				       uint16 *height	/** Pointer to \c uint16 that is set to the height of the bitmap. */,
				       uint16 *depth	/** Pointer to \c uint16 that is set to the depth of the bitmap. */)
{
	nodedb_b_get_dimensions((NodeBitmap *) node, width, height, depth);
}

/** \brief Return the number of layers in a bitmap node.
 * 
*/
PURPLEAPI unsigned int p_node_b_layer_num(PINode *node	/** The node whose number of layers is to be queried. */)
{
	return nodedb_b_layer_num((NodeBitmap *) node);
}

/** \brief Return bitmap layer by index.
 * 
 * Return a bitmap layer, by index. Valid index range is 0 up to, but not including, the value returned by the
 * \c p_node_b_layer_num() function. Specifying an index outside of this range causes \c NULL to be returned.
*/
PURPLEAPI PNBLayer * p_node_b_layer_nth(PINode *node	/** The node whose layer is to be accessed. */,
					unsigned int n	/** The index of the layer to access. */)
{
	return nodedb_b_layer_nth((NodeBitmap *) node, n);
}

/** \brief Return bitmap layer by name.
 * 
 * Search through a bitmap node looking for a named layer. If no layer with the requested name exists, \c NULL is returned. */
PURPLEAPI PNBLayer * p_node_b_layer_find(PINode *node		/** The node whose layers are to be searched. */,
					 const char *name	/** The name of the layer to search for. */)
{
	return nodedb_b_layer_find((NodeBitmap *) node, name);
}

/** \brief Return the name of a bitmap layer. */
PURPLEAPI const char * p_node_b_layer_get_name(const PNBLayer *layer	/** The layer whose name is returned. */)
{
	if(layer != NULL)
		return ((NdbBLayer *) layer)->name;
	return NULL;
}

/** \brief Return the type of a bitmap layer. */
PURPLEAPI VNBLayerType p_node_b_layer_get_type(const PNBLayer *layer	/** The layer whose type is returned. */)
{
	if(layer != NULL)
		return ((NdbBLayer *) layer)->type;
	return -1;
}

/** \brief Create a new bitmap layer.
 * 
 * This function creates a new layer in a bitmap node. Bitmap layers each store one value per pixel, and can be
 * of different types ranging from 1-bit up to 64-bit floating point.
 * 
 * The new layer is returned, and can be immediately filled-in with the desired content.
*/
PURPLEAPI PNBLayer * p_node_b_layer_create(PONode *node		/** The node in which a layer is to be created. */,
					   const char *name	/** The name of the layer. Layer names must be unique within the node. */,
					   VNBLayerType type	/** The type of pixel data this layer should store. */)
{
	PNBLayer	*l;

	if(node == NULL)
		return NULL;
	if((l = nodedb_b_layer_find((NodeBitmap *) node, name)) != NULL)
		return l;
	return nodedb_b_layer_create((NodeBitmap *) node, ~0, name, type);
}

/** \brief Gain access to layer pixels.
 * 
 * This function returns a pointer to the pixels of a given bitmap layer, so they can be directly manipulated
 * by the plug-in. The returned pointer points to the pixel at (0,0,0), and the pixels are arranged sequentially
 * in a left-to-right, top-to-bottom, front-to-back manner. There are no gaps between pixels.
 * 
 * When done with the access, you must hand the pixels back to the Purple engine by calling the \c p_node_b_layer_access_end() function.
*/
PURPLEAPI void * p_node_b_layer_access_begin(PONode *node	/** The node whose layer is to be accessed. */,
					     PNBLayer *layer	/** The layer to access. */)
{
	return nodedb_b_layer_access_begin((NodeBitmap *) node, layer);
}

/** Stop accessing layer pixels.
 * 
 * This function hands a pixel buffer that was returned by \c p_node_b_layer_access_begin() back to Purple. After
 * this call, the pixel buffer pointer is no longer valid.
*/
PURPLEAPI void p_node_b_layer_access_end(PONode *node		/** The node whose pixels have been accessed. */,
					 PNBLayer *layer	/** The layer the pixels belong to. */,
					 void *framebuffer	/** The pixel buffer pointer. */)
{
	nodedb_b_layer_access_end((NodeBitmap *) node, layer, framebuffer);
}

/** \brief Compute layer pixels through callback.
 *
 * This function lets you compute the value for each pixel by defining a callback funtion, that Purple
 * then calls. The callback function is passed the coordinates of the pixel whose value is to be computed, and
 * a user pointer to maintain state. The callback should return the pixel value, in \c real64 format. The
 * Purple engine does any necessary conversion to store that value in the actual layer.
 * 
 * Example:
 * \code
 * // Example to set a bitmap to an interesting "concentric-circles" type of pattern.
 * // The 'user' pointer is not used, and is set to NULL in the call.
 * 
 * static real64 pixel(uint32 x, uint32 y, uint32 z, void *user)
 * {
 * 	return x * x + y * y + z * z;
 * }
 * 
 * p_node_b_layer_foreach_set(node, layer, compute, NULL);
 * \endcode
*/
PURPLEAPI void p_node_b_layer_foreach_set(PONode *node		/** The node in which a layer is to be set. */,
					  PNBLayer *layer	/** The layer to be set. */,
				real64 (*pixel)(uint32 x, uint32 y, uint32 z, void *user)	/** The callback function that computes a pixel value. */,
					  void *user		/** User-defined pointer passed to the callback. */)
{
	nodedb_b_layer_foreach_set((NodeBitmap *) node, layer, pixel, user);
}

/** \brief Destroy a bitmap layer. */
PURPLEAPI void p_node_b_layer_destroy(PONode *node	/** The node in which a layer is to be destroyed. */,
				      PNBLayer *layer	/** The layer to be destroyed. */)
{
/* FICXME: Implement backend!	nodedb_b_layer_destroy(layer);*/
}

/** \brief Create "interleaved" buffer for multi-layer access.
 * 
 * This function requests that Purple merge several layers into a single buffer, for convenient access by 
 * plug-in code. Since the Verse bitmap format only supports one value per pixel in a layer, representing
 * i.e. an ordinary RGB color image requires the use of three separate layers. Doing per-pixel operations
 * on such data can be a bit inconvenient, which is why this function exists to make it smoother.
 * 
 * The function will accept up to 16 layer names (strings), and return a buffer that begins with pixel (0,0,0)
 * for the first layer, followed by pixel (0,0,0) for the second layer, for each layer. Then comes the next
 * pixel in the first layer, and so on. The pixel ordering is as for the \c p_node_b_layer_access_begin()
 * function above (left-to-right, top-to-bottom, and front-to-back).
 * 
 * The list of names specified in the ... part of the function should be terminated by a \c NULL pointer. If
 * any of the named layers doesn't exist in the node, the function aborts, since it would be impossible
 * for the plug-in to correctly interpret the returned buffer. In this case, \c NULL is returned.
 * 
 * The returned buffer can be in any supported pixel format, Purple will convert to and from the desired
 * format as needed.
 * 
 * Once the plug-in is done accessing the pixel data, it must be handed back to Purple by calling the
 * \c p_node_b_layer_access_multi_end() function.
 * 
 * \code
 * // Example to set a standard color bitmap to "all white", using 8-bit intermediate precision.
 * uint16 width, height, depth;
 * uint8  *pixels;
 * 
 * p_node_b_get_dimensions(node, &width, &height, &depth);
 * pixels = p_node_b_layer_access_multi_begin(node, VN_B_LAYER_UINT8, "col_r", "col_g", "col_b", NULL);
 * memset(pixels, 255, width * height * depth);
 * p_node_b_layer_access_multi_end(pixels);
 * \endcode
*/
PURPLEAPI void * p_node_b_layer_access_multi_begin(PONode *node	/** The node whose layers is to be accessed. */,
						   VNBLayerType format	/** The format in which the plug-in wishes to access the pixels. */,
						   ...	/** Skaab. */)
{
	va_list	layers;
	void	*fb;

	va_start(layers, format);
	fb = nodedb_b_layer_access_multi_begin((NodeBitmap *) node, format, layers);
	va_end(layers);
	return fb;
}

/** \brief Stop accessing interleaved multi-layer buffer.
 * 
 * This function hands a multi-layer buffer created by a call to \c p_node_b_layer_access_multi_begin() back to
 * Purple. It will convert the data in the layer to whatever format each of the source layers is in, and write
 * it back into the layers.
 * 
 * You must call this function on the buffer, or memory will be lost.
*/
PURPLEAPI void p_node_b_layer_access_multi_end(PONode *node, void *framebuffer)
{
	nodedb_b_layer_access_multi_end((NodeBitmap *) node, framebuffer);
}

/** @} */

/* ----------------------------------------------------------------------------------------- */

/** \defgroup api_node_curve Curve Node Functions
 * \ingroup api_node
 * 
 * These are functions for working with curve nodes.
 * 
 * \see The Verse specification on the curve node: <http://www.blender.org/modules/verse/verse-spec/n-curve.html>.
 * @{
*/

/** \brief Return number of curves in a curve node.
*/
PURPLEAPI unsigned int p_node_c_curve_num(PINode *node	/** The node whose number of curves is to be queried. */)
{
	return nodedb_c_curve_num((NodeCurve *) node);
}

/** \brief Return curve by index.
 * 
 * Valid index range is 0 up to, but not including, the value returned by the \c p_node_c_curve_num() function.
 * Specifying an index outside of this range causes \c NULL to be returned.
*/
PURPLEAPI PNCCurve * p_node_c_curve_nth(PINode *node	/** The node whose curve is to be accessed. */,
					unsigned int n	/** The index of the curve to access. */)
{
	return nodedb_c_curve_nth((NodeCurve *) node, n);
}

/** \brief Return curve by name.
 * 
 * If no curve with the specified name exists, \c NULL is returned.
*/
PURPLEAPI PNCCurve * p_node_c_curve_find(PINode *node		/** The node whose curve is to be accessed. */,
					 const char *name	/** The name of the curve to access. */)
{
	return nodedb_c_curve_find((NodeCurve *) node, name);
}

/** \brief Initialize iterator over curve node's curves.
 * 
 * This function initializes an iterator so that it can be used to iterate over the individual curves
 * in a curve node.
*/
PURPLEAPI void p_node_c_curve_iter(PINode *node	/** The node whose curves are to be iterated. */,
				   PIter *iter	/** The iterator to initialize. */)
{
	if(node != NULL)
		iter_init_dynarr_string(iter, ((NodeCurve *) node)->curves, offsetof(NodeCurve, curves));
}

/** \brief Return the number of a curve.
 * 
 * This function returns the name of a curve node curve. Please note that it does not return the name
 * of the node itself, but of one of the individual curves stored in the node.
*/
PURPLEAPI const char * p_node_c_curve_get_name(const PNCCurve *curve	/** The curve whose name is to be queried. */)
{
	if(curve != NULL)
		return ((NdbCCurve *) curve)->name;
	return NULL;
}

/** \brief Return the number of dimensions of a curve.
 * 
 * This function returns the number of dimensions of a given curve node curve. Curves are defined with
 * one to four dimensions, inclusive.
*/
PURPLEAPI uint8 p_node_c_curve_get_dimensions(const PNCCurve *curve	/** The curve whose number of dimensions is to be queried. */)
{
	if(curve != NULL)
		return ((NdbCCurve *) curve)->dimensions;
	return 0;
}

/** \brief Create a new curve.
 * 
 * This function creates a new curve in a curve node. The created curve is returned and can be
 * used immediately.
*/
PURPLEAPI PNCCurve * p_node_c_curve_create(PONode *node		/** The node in which a curve is to be created. */,
					   const char *name	/** The name of the curve to be created. Names must be unique within a node. */,
					   uint8 dimensions	/** The number of dimensions of the new curve. Must be one, two, three or four. */)
{
	PNCCurve	*c;

	if((c = nodedb_c_curve_find((NodeCurve *) node, name)) != NULL)
		return c;
	return nodedb_c_curve_create((NodeCurve *) node, ~0, name, dimensions);
}

/** \brief Destroy a curve.
 * 
 * This function destroys a curve in a curve node. Accessing the destroyed curve is not permitted, and invokes undefined behavior.
*/
PURPLEAPI void p_node_c_curve_destroy(PONode *node	/** The node in which a curve is to be destroyed. */,
				      PNCCurve *curve	/** The curve to be destroyed. */)
{
	nodedb_c_curve_destroy((NodeCurve *) node, curve);
}

/** \brief Return the number of keys in a curve.
 * 
 * This function returns the number of keys in a single curve. Keys are used to define the curve, by interpolating between them.
*/
PURPLEAPI size_t p_node_c_curve_key_num(const PNCCurve *curve	/** The curve whose number of keys is to be queried. */)
{
	return nodedb_c_curve_key_num(curve);
}

/** \brief Return a curve key, by index.
 * 
 * Valid index range is 0 up to, but not including, the value returned by the \c p_node_c_curve_key_num() function.
 * Specifying an index outside of this range causes \c NULL to be returned.
*/ 
PURPLEAPI PNCKey * p_node_c_curve_key_nth(const PNCCurve *curve	/** The curve whose keys is to be accessed. */,
					  unsigned int n	/** The index of the key to access. */)
{
	return nodedb_c_curve_key_nth(curve, n);
}

/** \brief Return position of a curve key.
*/
PURPLEAPI real64 p_node_c_curve_key_get_pos(const PNCKey *key	/** The key whose position is being queried. */)
{
	return key != NULL ? ((NdbCKey *) key)->pos : 0.0;
}

/** \brief Return value of a key.
 * 
 * This function returns the value of a key. A key can have a different value for each of the curve's dimensions,
 * if multi-dimensional.
*/
PURPLEAPI real64 p_node_c_curve_key_get_value(const PNCKey *key	/** The key whose value is being queried. */,
					      uint8 dimension	/** The dimension for which the value is being queried. */)
{
	return key != NULL ? ((NdbCKey *) key)->value[dimension] : 0.0;
}

/** \brief Get the "pre-point" of a key.
 * 
 * This function returns information about the control point that controls how the curve reaches a given key.
 * Since this is before the key, it is called the "pre-point".
 *
 * The information has two components: one is the horizontal distance from the pre-point to the key, which is
 * returned by the function. The other is the vertical position of the control point, and that is optionally returned
 * through the \c value argument. There is one unique pre-point per dimension.
 * 
 * More more information how these values are to be interpreted, please see the Verse specification.
*/
PURPLEAPI uint32 p_node_c_curve_key_get_pre(const PNCKey *key	/** The key whose pre-point is to be queried. */,
					    uint8 dimension	/** The dimension for which the pre-point is to be queried. */,
					    real64 *value	/** Optional pointer to \c real64 that is set to the value of the pre-point. */)
{
	if(key != NULL)
	{
		if(value != NULL)
			*value = ((NdbCKey *) key)->pre.value[dimension];
		return ((NdbCKey *) key)->pre.pos[dimension];
	}
	return 0;
}

/** \brief Get the "post-point" of a key.
 * 
 * This function returns information about the control point that controls how the curve leaves a given key.
 * Since this is after the key, it is called the "post-point".
 *
 * The information has two components: one is the horizontal distance from the key to the post-point, which is
 * returned by the function. The other is the vertical position of the control point, and that is optionally returned
 * through the \c value argument. There is one unique post-point per dimension.
 * 
 * More more information how these values are to be interpreted, please see the Verse specification.
*/
PURPLEAPI uint32 p_node_c_curve_key_get_post(const PNCKey *key	/** The key whose post-point is to be queried. */,
					     uint8 dimension	/** The dimension for which the post-point is to be qieried. */,
					     real64 *value	/** Pointer to \c real64 that is set to the value of the post-point. */)
{
	if(key != NULL)
	{
		if(value != NULL)
			*value = ((NdbCKey *) key)->post.value[dimension];
		return ((NdbCKey *) key)->post.pos[dimension];
	}
	return 0;
}

/** \brief Create a curve key.
 * 
 * This function creates a new key in a curve. The five final arguments are pointers, since they are vectors. Only the
 * position is scalar.
 * 
 * \see The Verse specification on the curve node: <http://www.blender.org/modules/verse/verse-spec/n-curve.html>.
*/
PURPLEAPI PNCKey * p_node_c_curve_key_create(PNCCurve *curve		/** The curve in which the key is to be created. */,
					     real64 pos			/** The position of the new key. Must be unique. */,
					     const real64 *value	/** The value of the key, one per dimension. */,
					     const uint32 *pre_pos	/** Pre-point positions for this key, one per dimension. */,
					     const real64 *pre_value	/** Pre-point values for this key, one per dimension. */,
					     const uint32 *post_pos	/** Post-point positions for this key, one per dimension. */,
					     const real64 *post_value	/** Post-point value for this key, one per dimension. */)
{
	return (PNCKey *) nodedb_c_key_create(curve, ~0, pos, value, pre_pos, pre_value, post_pos, post_value);
}

/** \brief Destroy a curve node.
 * 
 * This function destroys a key in a curve node. The curve will interpolate through the new sequence of
 * keys, if any remain.
*/
PURPLEAPI void p_node_c_curve_key_destroy(PNCCurve *curve	/** The curve in which a key is to be destroyed. */,
					PNCKey *key		/** The key to be destroyed. */)
{
	nodedb_c_key_destroy((NdbCCurve *) curve, (NdbCKey *) key);
}

/** @} */

/* ----------------------------------------------------------------------------------------- */

/** \defgroup api_node_text Text Node Functions
 * \ingroup api_node
 * 
 * These are functions for working with text nodes. You can get and set the language setting of a node, as well
 * as create, destroy, read and write the buffers that hold the actual text.
 * 
 * \see The Verse specification on the text node: <http://www.blender.org/modules/verse/verse-spec/n-text.html>.
 * @{
*/

/** \brief Return the langauge of a text node.
 * 
 * This function returns the language of a text node. The language is simply a string that defines in which
 * language the content of the node is.
*/
PURPLEAPI const char * p_node_t_language_get(PINode *node	/** The text node whose language is to be queried. */)
{
	return nodedb_t_language_get((NodeText *) node);
}

/** \brief Set the language of a text node.
 * 
 * This function sets the language of a text node. Changing the language of a text node has no immediate effect
 * on the actual contents of the node's buffers. Programs that try to parse the contents might care though.
*/
PURPLEAPI void p_node_t_language_set(PONode *node		/** The node whose language is to be set. */,
				     const char *language	/** The new langauge. */)
{
	nodedb_t_language_set((NodeText *) node, language);
}

/** \brief Return the number of buffers in a text node.
*/
PURPLEAPI unsigned int p_node_t_buffer_num(PINode *node	/** The text node whose number of buffers is to be queried. */)
{
	return nodedb_t_buffer_num((NodeText *) node);
}

/** \brief Return text buffer, by index.
 *
 * Valid index range is 0 up to, but not including, the value returned by the \c p_node_t_buffer_num() function.
 * Specifying an index outside of this range causes \c NULL to be returned.
 */
PURPLEAPI PNTBuffer * p_node_t_buffer_nth(PINode *node		/** The node whose buffers is to be accessed. */,
					  unsigned int n	/** The index of the buffer to access. */)
{
	return nodedb_t_buffer_nth((NodeText *) node, n);
}

/** \brief Return text buffer, by name.
 * 
 * If no text buffer with the specified name exists, \c NULL is returned.
*/
PURPLEAPI PNTBuffer * p_node_t_buffer_find(PINode *node		/** The node whose text buffers are to be searched. */,
					   const char *name	/** The name of the text buffer to search for. */)
{
	return nodedb_t_buffer_find((NodeText *) node, name);
}

/** \brief Return name of a text buffer.
*/
PURPLEAPI const char * p_node_t_buffer_get_name(const PNTBuffer *buffer	/** The text buffer whose name is being queried. */)
{
	if(buffer != NULL)
		return ((NdbTBuffer *) buffer)->name;
	return NULL;
}

/** \brief Create a new text buffer.
 * 
 * This function creates a new text buffer in a node. Text buffers require very few parameters, just a name.
 * 
 * The created buffer is returned and can be used immediately.
*/
PURPLEAPI PNTBuffer * p_node_t_buffer_create(PONode *node	/** The text node in which a buffer is to be created. */,
					     const char *name	/** The name of the text buffer to create. Must be unique within the node. */)
{
	PNTBuffer	*b;

	if((b = nodedb_t_buffer_find((NodeText *) node, name)) != NULL)
	{
		nodedb_t_buffer_clear(b);
		return b;
	}
	return nodedb_t_buffer_create((NodeText *) node, ~0, name);
}

/** \brief Destroy a text buffer.
 * 
 * This function destroys a buffer in a text node. The buffer reference immediately becomes invalid, and
 * can not be used in any further calls.
*/
PURPLEAPI void p_node_t_buffer_destroy(PONode *node		/** The text node in which a buffer is to be destroyed. */,
				       PNTBuffer *buffer	/** The buffer to be destroyed. */)
{
	nodedb_t_buffer_destroy((NodeText *) node, buffer);
}

/** \brief Read out text, indexing by line.
 * 
 * This function lets you read out a line of a text buffer's contents. It assumes the text is stored
 * with some kind of end-of-line marker. Currently, Purple considers \b any of these combinations a
 * valid line separator:
 * - A lone line feed (LF, \c '\\n', ASCII value 10) character, as used in Unix
 * - A lone carriage return (CR, \c '\\r', ASCII value 13) symbol, as used in Windows
 * - A LF followed by any number of CRs
 * - A CR followed by any number of LFs
 * 
 * This might be too permissive and cause problems with multiple blank lines in a row for a data with
 * mixed-platform line endings. If so, it will be tightened in the future.
 * 
 * The end-of-line marker is not included in the line as returned.
 * 
 * The function returns a pointer to the line, or \c NULL if the line index was out of range. This
 * means a loop like this can be used to iterate over all lines of a buffer:
 * \code
 * char line[1024];
 * unsigned int i;
 * 
 * for(i = 0; p_node_t_buffer_read_line(buffer, i, line, sizeof line) != NULL; i++)
 * 	printf("Line %u: '%s'\n", i, line);
 * \endcode
 * 
 * As can be seen above, \c sizeof is the recommended way of accessing the size of a static buffer
 * to be sent to this function; the \c putmax argument includes the terminating \c NUL symbol and
 * should be interpreted as "don't touch more than this many bytes at put".
*/
PURPLEAPI char * p_node_t_buffer_read_line(PNTBuffer *buffer	/** The buffer to read from. */,
					   unsigned int line	/** The line number to read, starting at 0. */,
					   char *put		/** Buffer to store the line in. */,
					   size_t putmax	/** Maximum number of bytes that can be stored at \c put. */)
{
	return nodedb_t_buffer_read_line(buffer, line, put, putmax);
}

/** \brief Insert text in a text buffer.
 * 
 * This function inserts a piece of text in a text buffer. Text is really \b inserted, meaning that any text
 * previously at the indicated buffer position will be pushed toward the end of the buffer.
 * 
 * The position of the first character in the buffer is 0. If the position given is beyond the end of the
 * buffer, it will be clamped and the operation will effectively turn into an append.
*/
PURPLEAPI void p_node_t_buffer_insert(PNTBuffer *buffer	/** The buffer in which text is to be inserted. */,
				      size_t pos	/** The position at which insertion will start. */,
				      const char *text	/** The text to be inserted. */)
{
	return nodedb_t_buffer_insert(buffer, pos, text);
}

/** \brief Delete text from a text buffer.
 * 
 * This function deletes a range in a text buffer. The deleted text is replaced by nothing, making any
 * text before and after come together.
 * 
 * The position of the first character is 0. If the length specifies a range that goes outside of the
 * text, it is clamped to the end of the buffer.
*/
PURPLEAPI void p_node_t_buffer_delete(PNTBuffer *buffer, size_t pos, size_t length)
{
	return nodedb_t_buffer_delete(buffer, pos, length);
}

/** \brief Append text to a text buffer.
 * 
 * This function appends text to a buffer, adding the new text last in the buffer. This is a simple
 * convenience function, that is simply the equivalent of doing this:
 * \code
 * p_node_t_buffer_insert(buffer, ~0u, text);
 * \endcode
 * It is still recommended to use it whenever an append is the desired operation, since it reads better
 * and could possibly be implemented in a more efficient manner internally.
*/
PURPLEAPI void p_node_t_buffer_append(PNTBuffer *buffer	/** Buffer to which text is to be appended. */,
				      const char *text	/** The text to append. */)
{
	return nodedb_t_buffer_append(buffer, text);
}

/** @} */

/* ----------------------------------------------------------------------------------------- */

/** \defgroup api_node_audio Audio Node Functions
 * \ingroup api_node
 * 
 * These are functions for working with audio nodes. You can create, destroy and edit buffers used
 * to store audio in uncompressed PCM sample format.
 * 
 * \see The Verse specification on the audio node: <http://www.blender.org/modules/verse/verse-spec/n-audio.html>.
 * 
 * \note Audio streams are currently not supported by Purple; only the "passive" sample storage buffers are.
 * @{
*/

/** \brief Return number of buffers in an audio node.
*/
PURPLEAPI unsigned int p_node_a_buffer_num(PINode *node	/** The node whose number of buffers is to be queried. */)
{
	return nodedb_a_buffer_num((NodeAudio *) node);
}

/** \brief Return an audio buffer, by index.
 * 
 * Valid index range is 0 up to, but not including, the value returned by the \c p_node_a_buffer_num() function.
 * Specifying an index outside of this range causes \c NULL to be returned.
*/
PURPLEAPI PNABuffer * p_node_a_buffer_nth(PINode *node, unsigned int n)
{
	return nodedb_a_buffer_nth((NodeAudio *) node, n);
}


/** \brief Return an audio buffer, by name.
 * 
 * If no buffer with the specified name exists, \c NULL is returned.
*/
PURPLEAPI PNABuffer * p_node_a_buffer_find(PINode *node		/** The node whose buffers are to be searched. */,
					   const char *name	/** The name of the buffer to search for. */)
{
	return nodedb_a_buffer_find((NodeAudio *) node, name);
}

/** \brief Return the name of an audio buffer.
*/
PURPLEAPI const char * p_node_a_buffer_get_name(const PNABuffer *buffer	/** The buffer whose name is to be queried. */)
{
	if(buffer != NULL)
		return ((NdbABuffer *) buffer)->name;
	return NULL;
}

/** \brief Return the frequence of an audio buffer.
*/
PURPLEAPI real64 p_node_a_buffer_get_frequency(const PNABuffer *buffer	/** The buffer whose frequency is being queried. */)
{
	return buffer != NULL ? ((NdbABuffer *) buffer)->frequency : 0.0;
}

/** \brief Create a new audio buffer.
 * 
 * This function creates a new audio buffer in an audio node. Audio buffers is where audio data can be
 * stored and manipulated.
 *
 * You must specify the type of data to store, and the sampling frequency the data is representing. The
 * sampling frequency is not needed to just store the data, but it is an integral part of how the data
 * is interpreted.
 *
 * The buffer is returned and can be used immediately.
*/
PURPLEAPI PNABuffer * p_node_a_buffer_create(PONode *node	/** The audio node in which a new buffer is to be created. */,
					     const char *name	/** The name of the new buffer. Must be unique within the node. */,
					     VNABlockType type	/** The type of the samples stored in this buffer. */,
					     real64 frequency	/** The sample frequency the data represents. */)
{
	PNABuffer	*l;

	if(node == NULL)
		return NULL;
	if((l = nodedb_a_buffer_find((NodeAudio *) node, name)) != NULL)
		return l;
	return nodedb_a_buffer_create((NodeAudio *) node, ~0, name, type, frequency);
}

/** \brief Read out audio samples.
 * 
 * This function is used to read out a sequence of samples from an audio buffer. It will convert the
 * samples to the largest supported format, 64-bit floating point, in order to preserve data while
 * removing the need for the user to care about the format.
*/
PURPLEAPI unsigned int p_node_a_buffer_read_samples(const PNABuffer *buffer	/** The audio buffer from which samples are to be read. */,
						    unsigned int start		/** Index of the first sample to read out. */,
						    real64 *samples		/** Buffer to write sample data into. */,
						    unsigned int length		/** Number of \b samples to read from buffer. Not bytes. */)
{
	return nodedb_a_buffer_read_samples((NdbABuffer *) buffer, start, samples, length);
}

/** \brief Write audio samples into buffer.
 * 
 * This function is used to write (back) a sequence of samples into an audio buffer. It is the only
 * way to store data in a buffer. Data is always handed to the function in 64-bit floating point,
 * and internally converted by Purple to whatever type the buffer was created to hold.
 * 
 * Specifying a \c start beyond the current length is perfectly valid, and will fill the intermediate
 * samples with silence. Existing data at the touched indices will be overwritten.
*/
PURPLEAPI void p_node_a_buffer_write_samples(PNABuffer *buffer	/** The buffer into which samples are to be written. */,
					     unsigned int start	/** Index of the first sample to overwrite. */,
					     const real64 *samples	/** Buffer to read samples from. */,
					     unsigned int length	/** Number of \b samples to write into buffer. Not bytes. */)
{
	nodedb_a_buffer_write_samples((NdbABuffer *) buffer, start, samples, length);
}

/** @} */	/* api_node_audio */

/** @} */	/** api_node */
