
#include <stdlib.h>

#include "verse.h"

#include "idset.h"

typedef struct
{
	char		name[VN_TAG_NAME_SIZE];
	VNTagType	type;
	VNTag		value;
} Tag;

typedef struct
{
	char		name[VN_TAG_GROUP_SIZE];
	IdSet		*tags;
} TagGroup;

struct Node
{
	VNodeID		id;
	VNodeType	type;
	char		name[32];
	IdSet		*tag_groups;
};
