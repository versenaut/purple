/*
 * 
*/

/* Events that need handling might be:
 * Simple events that don't affect the graphs
 * - A plug-in appears
 * - A plug-in is removed (not needed, but might be nice for control freaks...)
 * Complicated events that cause graph-recomputing (and other things)
 * - Verse data arrives
 *   · Update Verse data model
 *   · Is the addressed node monitored by any graphs? Then recompute them!
 * - Method call
 *   · Module create/destroy: update XML, possibly remove (on destroy) monitoring.
 *   · Input set: recompute affected graph.
*/

enum EventID { EVENT_PLUGIN_LOADED };

typedef struct
{
	enum EventID	id;
} Event;

typedef struct {
	Event	head;
	char	name[64];
	DynLib	code;
} EventPluginLoaded;
