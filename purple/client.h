/*
 * A module to hold the things that make this program into an actual Verse client.
*/

typedef struct
{
	uint16		buffer;
	unsigned int	cron;
	TextBuf		*text;		/* Remote (shared, server-side) plugins description. */
} PluginsMeta;

typedef struct
{
	uint16		buffer;
	size_t		start;		/* Location of first content character, after header. */
	unsigned int	cron;
	TextBuf		*text;
} GraphsMeta;

typedef struct
{
	int		connected;
	int		conn_count;
	char		*address;
	VSession	*connection;

	VNodeID		avatar;

	VNodeID		meta;
	PluginsMeta	plugins;
	GraphsMeta	graphs;

	uint16		gid_control;
} ClientInfo;

extern ClientInfo	client_info;

extern void	client_init(void);
extern int	client_connect(const char *address);
