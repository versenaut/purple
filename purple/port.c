/*
 * Ports are... cached values.
*/

#include <stdarg.h>

#include "purple.h"

#include "dynstr.h"
#include "value.h"

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
	value_clear(&port->value);
	/* FIXME: Can't clear node set. */
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
		*module_id = port->value.v.vmodule;
		return TRUE;
	}
	return FALSE;
}

void port_append_value(const PPort *port, DynStr *d)
{
	value_append(&port->value, d);
}

int port_set_va(PPort *port, PValueType type, va_list arg)
{
	return value_set(&port->value, type, arg);
}

/* Save some typing in these (inelegant?) accessor function definitions. */
#define	ACCESSOR(t, n)	t port_input_ ##n (PPort *port) { return value_get_ ##n (&port->value, &port->cache); }

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