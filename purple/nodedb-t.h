/*
 * 
*/

typedef struct
{
	Node	node;
	char	language[32];
	DynArr	*buffers;
} NodeText;

typedef struct
{
	char	name[16];
	char	content;
} NdbTBuffer;
