/*
 * 
*/

typedef struct PNodes	PNodes;

typedef struct {
	PValue	value;		/* Values are written (set) into here. */
	PValue	cache;		/* Holds cached values, i.e. transforms between types. */
	PNodes	*nodes;		/* Simply NULL if no nodes present. Not worth optimizing out of struct. */
} PPort;

