/*
 * Ports are... cached values.
*/

#include <stdarg.h>

#include "purple.h"

#include "dynstr.h"
#include "value.h"
#include "nodeset.h"

#include "port.h"

void port_init(PPort *port)
{
	value_init(&port->value);
	value_init(&port->cache);
	port->nodes = NULL;
}

void port_clear(PPort *port)
{
	value_clear(&port->value);
	value_clear(&port->cache);
	nodeset_clear(port->nodes);
}

boolean port_is_unset(const PPort *port)
{
	return port->value.set == 0;
}

const char * port_get_type_name(const PPort *port)
{
	return value_type_name(&port->value);
}

boolean port_peek_module(const PPort *port, uint32 *module_id)
{
	if(port->value.set & (1 << P_VALUE_MODULE))
	{
		if(module_id != NULL)
			*module_id = port->value.v.vmodule;
		return TRUE;
	}
	return FALSE;
}

void port_append_value(const PPort *port, DynStr *d)
{
	char		buf[1024];
	const char	*p;

	if((p = value_as_string(&port->value, buf, sizeof buf, NULL)) != NULL)
		dynstr_append(d, p);
}

int port_set_va(PPort *port, PValueType type, va_list arg)
{
	return value_set_va(&port->value, type, arg);
}

int port_set_node(PPort *port, PONode *node)
{
	port->nodes = nodeset_add(port->nodes, node);
	return 1;
}

/* Save some typing in these (inelegant?) accessor function definitions. Trampolines. */
#define	ACCESSOR(t, n)	t port_input_ ##n (PPort *port)\
{\
	t	tmp;\
\
	if(value_get_ ## n(&port->value, &port->cache, &tmp))\
		return tmp;\
	if(nodeset_get_ ## n(port->nodes, &port->cache))\
		return port->cache.v.v ## n;\
	return value_get_default_ ##n();\
}

ACCESSOR(boolean, boolean)
ACCESSOR(int32, int32)
ACCESSOR(uint32, uint32)
ACCESSOR(real32, real32)
ACCESSOR(const real32 *, real32_vec2)
ACCESSOR(const real32 *, real32_vec3)
ACCESSOR(const real32 *, real32_vec4)
ACCESSOR(const real32 *, real32_mat16)
ACCESSOR(real64, real64)
ACCESSOR(const real64 *, real64_vec2)
ACCESSOR(const real64 *, real64_vec3)
ACCESSOR(const real64 *, real64_vec4)
ACCESSOR(const real64 *, real64_mat16)
ACCESSOR(const char *, string)

PINode * port_input_node(PPort *port)
{
	return port_input_node_nth(port, 0);
}

PINode * port_input_node_nth(PPort *port, unsigned int index)
{
	return port != NULL ? nodeset_retreive_nth(port->nodes, index) : NULL;
}
