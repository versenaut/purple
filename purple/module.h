/*
 * 
*/

#define	P_MODULE_NAME_LEN	32

typedef struct
{
	const char	*category;
	const char	*text;
} PMeta;

#define	P_INPUT_NAME_LEN	32

typedef struct
{
	PInputType	type;
	char		name[P_INPUT_NAME_LEN];
} PInput;

/* A PPlugin represents a loaded plugin. It has not necessarily been instantiated yet. */
struct PPlugin
{
	char	name[P_MODULE_NAME_LEN];
	void	(*init)(void);
	void	(*compute)(void);

	int	num_meta;
	PMeta	**meta;

	int	num_input;
	PInput	*input;
};

/* A PModule is an instantiated plug-in. */
struct PModule
{
	uint32	id;
	PPlugin	*plugin;
	PModule	**input;
};

/*
 * Method interface used by UI client to control engine client. Feedback is through
 * updating of the shared textual description of the graph; this is done by the engine.
 * 
 * void module_create(uint32 graph, string32 plugin);
 * void module_destroy(uint32 graph, uint32 id);
 *
 * void module_input_set_module(uint32 graph, uint32 module, uint8 input, uint32 value);
 * void module_input_set_boolean(uint32 graph, uint32 module, uint8 input, boolean value);
 * void module_input_set_int32(uint32 graph, uint32 module, uint8 input, int32 value);
 * 
 */
