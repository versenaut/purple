/*
 * 
*/

typedef struct PNodes	PNodes;

typedef struct PPort {
	PValue	value;		/* Values are written (set) into here. */
	PValue	cache;		/* Holds cached values, i.e. transforms between types. */
	PNodes	*nodes;		/* Simply NULL if no nodes present. Not worth optimizing out of struct. */
} PPort;

extern void		port_init(PPort *port);
extern void		port_clear(PPort *port);

extern boolean		port_is_unset(const PPort *port);
extern boolean		port_peek_module(const PPort *port, uint32 *module_id);
extern const char *	port_get_type_name(const PPort *port);

extern void		port_append_value(const PPort *port, DynStr *d);

extern int		port_set_va(PPort *port, PValueType type, va_list arg);

extern boolean		port_input_boolean(PPort *port);
extern const char *	port_input_string(PPort *port);
