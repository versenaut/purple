
#include <stdlib.h>

#include "dynarray.h"

typedef struct {
	uint8		id;
	char		name[VN_TAG_NAME_SIZE];
	VNTagType	type;
	VNTag		value;
} Tag;

typedef struct
{
	uint16		id;
	char		name[VN_TAG_GROUP_SIZE];
	DynArr *	tags;
} TagGroup;

struct Node
{
	VNodeType	type;
	char		name[32];
	DynArr		*tag_groups;
};
