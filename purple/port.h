/*
 * 
*/

typedef struct PPort
{
	PValue	value;		/* Values are written (set) into here. */
	PValue	cache;		/* Holds cached values, i.e. transforms between types. */
	NodeSet	*nodes;		/* Simply NULL if no nodes present. Not worth optimizing out of struct. */
} PPort;

extern void		port_init(PPort *port);
extern void		port_clear(PPort *port);

extern boolean		port_is_unset(const PPort *port);
extern boolean		port_peek_module(const PPort *port, uint32 *module_id);
extern const char *	port_get_type_name(const PPort *port);

extern void		port_append_value(const PPort *port, DynStr *d);

extern int		port_set_va(PPort *port, PValueType type, va_list arg);
extern int		port_set_node(PPort *port, PONode *node);

extern boolean		port_input_boolean(PPort *port);
extern int32		port_input_int32(PPort *port);
extern uint32		port_input_uint32(PPort *port);
extern real32		port_input_real32(PPort *port);
extern const real32 *	port_input_real32_vec2(PPort *port);
extern const real32 *	port_input_real32_vec3(PPort *port);
extern const real32 *	port_input_real32_vec4(PPort *port);
extern const real32 *	port_input_real32_mat16(PPort *port);
extern real64		port_input_real64(PPort *port);
extern const real64 *	port_input_real64_vec2(PPort *port);
extern const real64 *	port_input_real64_vec3(PPort *port);
extern const real64 *	port_input_real64_vec4(PPort *port);
extern const real64 *	port_input_real64_mat16(PPort *port);
extern const char *	port_input_string(PPort *port);
extern PINode *		port_input_node(PPort *port);
