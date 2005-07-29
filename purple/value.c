/*
 * value.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Value management. Values are used to hold both things assigned to inputs, and the result of
 * computation, i.e. output buffers. They are rather heavy-weight because of this duality, but
 * it is hopefully worth it.
*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "verse.h"

#include "dynstr.h"
#include "mem.h"
#include "list.h"
#include "log.h"
#include "strutil.h"
#include "vecutil.h"

#include "purple.h"

#include "value.h"

/* ----------------------------------------------------------------------------------------- */

#define	VALUE_SET(v,t)	((v)->set |= (1 << t))
#define	VALUE_SETS(v,t)	((v)->set & (1 << (t)))

static struct TypeName
{
	PValueType	type;
	const char	*name;
} type_map_by_value[] = {
	{ P_VALUE_BOOLEAN,	"boolean" },
	{ P_VALUE_INT32,	"int32" },
	{ P_VALUE_UINT32,	"uint32" },
	{ P_VALUE_REAL32,	"real32" },
	{ P_VALUE_REAL32_VEC2,	"real32_vec2" },
	{ P_VALUE_REAL32_VEC3,	"real32_vec3" },
	{ P_VALUE_REAL32_VEC4,	"real32_vec4" },
	{ P_VALUE_REAL32_MAT16,	"real32_mat16" },
	{ P_VALUE_REAL64,	"real64" },
	{ P_VALUE_REAL64_VEC2,	"real64_vec2" },
	{ P_VALUE_REAL64_VEC3,	"real64_vec3" },
	{ P_VALUE_REAL64_VEC4,	"real64_vec4" },
	{ P_VALUE_REAL64_MAT16,	"real64_mat16" },
	{ P_VALUE_STRING,	"string" },
	{ P_VALUE_MODULE,	"module" },	/* Rather special, never used in output case. */
	}, type_map_by_name[sizeof type_map_by_value / sizeof *type_map_by_value];

const char * value_type_to_name(PValueType type)
{
	if(type > P_VALUE_NONE && type <= P_VALUE_MODULE)
		return type_map_by_value[type].name;
	return "none";
}

/* bsearch() callback for comparing a string, <key>, with a struct TypeName. */
static int srch_type_name(const void *key, const void *t)
{
	return strcmp(key, ((const struct TypeName *) t)->name);
}

static int cmp_type_name(const void *a, const void *b)
{
	const struct TypeName	*tna = a, *tnb = b;

	return strcmp(tna->name, tnb->name);
}

PValueType value_type_from_name(const char *name)
{
	const struct TypeName	*tna;

	if(name != NULL)
	{
		static int	sorted = 0;

		if(!sorted)	/* I think we can afford this. */
		{
			size_t	i;

			for(i = 0; i < sizeof type_map_by_name / sizeof *type_map_by_name; i++)
				type_map_by_name[i] = type_map_by_value[i];
			qsort(type_map_by_name,
			      sizeof type_map_by_name / sizeof *type_map_by_name,
			      sizeof *type_map_by_name, cmp_type_name);
			sorted = 1;
		}
		if((tna = bsearch(name, type_map_by_name,
				  sizeof type_map_by_name / sizeof *type_map_by_name,
				  sizeof *type_map_by_name, srch_type_name)) != NULL)
			return tna->type;
	}
	return P_VALUE_NONE;
}

/* ----------------------------------------------------------------------------------------- */

void value_init(PValue *v)
{
	if(v == NULL)
		return;
	v->set = 0;		/* Indicates value is brand spanking new. */
}

void value_clear(PValue *v)
{
	if(v == NULL || v->set == 0)
		return;
	if(VALUE_SETS(v, P_VALUE_STRING))
	{
		mem_free(v->v.vstring);
	}
	v->set = 0;
}

boolean value_type_present(const PValue *v, PValueType type)
{
	return v != NULL ? VALUE_SETS(v, type) : 0;
}

const char * value_type_name(const PValue *v)
{
	PValueType	i;
	uint16		mask = 1;

	if(v == NULL)
		return NULL;
	for(i = P_VALUE_BOOLEAN; i <= P_VALUE_MODULE; i++, mask <<= 1)
	{
		if(v->set & mask)	/* Bit found? */
		{
			if(v->set & ~mask)	/* More bits set? */
				return NULL;	/* Can't resolve ambiguity, give up. */
			return value_type_to_name(i);
		}
	}
	return NULL;
}

/* ----------------------------------------------------------------------------------------- */

static int do_set(PValue *v, PValueType type, va_list arg)
{
	uint16	old_set = v->set;

	v->set |= (1 << type);
	switch(type)
	{
	case P_VALUE_BOOLEAN:
		v->v.vboolean = (boolean) va_arg(arg, int);
		return 1;
	case P_VALUE_INT32:
		v->v.vint32 = (int32) va_arg(arg, int32);
		return 1;
	case P_VALUE_UINT32:
		v->v.vuint32 = (uint32) va_arg(arg, uint32);
		return 1;
	case P_VALUE_REAL32:
		v->v.vreal32 = (real32) va_arg(arg, double);
		return 1;
	case P_VALUE_REAL32_VEC2:
		{
			const real32 *data = (const real32 *) va_arg(arg, const real32 *);
			v->v.vreal32_vec2[0] = data[0];
			v->v.vreal32_vec2[1] = data[1];
		}
		return 1;
	case P_VALUE_REAL32_VEC3:
		{
			const real32 *data = (const real32 *) va_arg(arg, const real32 *);
			v->v.vreal32_vec3[0] = data[0];
			v->v.vreal32_vec3[1] = data[1];
			v->v.vreal32_vec3[2] = data[2];
		}
		return 1;
	case P_VALUE_REAL32_VEC4:
		{
			const real32 *data = (const real32 *) va_arg(arg, const real32 *);
			v->v.vreal32_vec4[0] = data[0];
			v->v.vreal32_vec4[1] = data[1];
			v->v.vreal32_vec4[2] = data[2];
			v->v.vreal32_vec4[3] = data[3];
		}
		return 1;
	case P_VALUE_REAL32_MAT16:
		{
			const real32	*data = (const real32 *) va_arg(arg, const real32 *);
			int		i;

			for(i = 0; i < 16; i++)
				v->v.vreal32_mat16[i] = data[i];
		}
		return 1;
	case P_VALUE_REAL64:
		v->v.vreal64 = (real64) va_arg(arg, double);
		return 1;
	case P_VALUE_REAL64_VEC2:
		{
			const real64 *data = (const real64 *) va_arg(arg, const real64 *);
			v->v.vreal64_vec2[0] = data[0];
			v->v.vreal64_vec2[1] = data[1];
		}
		return 1;
	case P_VALUE_REAL64_VEC3:
		{
			const real64 *data = (const real64 *) va_arg(arg, const real64 *);
			v->v.vreal64_vec3[0] = data[0];
			v->v.vreal64_vec3[1] = data[1];
			v->v.vreal64_vec3[2] = data[2];
		}
		return 1;
	case P_VALUE_REAL64_VEC4:
		{
			const real64 *data = (const real64 *) va_arg(arg, const real64 *);
			v->v.vreal64_vec4[0] = data[0];
			v->v.vreal64_vec4[1] = data[1];
			v->v.vreal64_vec4[2] = data[2];
			v->v.vreal64_vec4[3] = data[3];
		}
		return 1;
	case P_VALUE_REAL64_MAT16:
		{
			const real64	*data = (const real64 *) va_arg(arg, const real64 *);
			int		i;

			for(i = 0; i < 16; i++)
				v->v.vreal64_mat16[i] = data[i];
		}
		return 1;
	case P_VALUE_MODULE:
		v->v.vmodule = (uint32) va_arg(arg, uint32);
		return 1;
	case P_VALUE_STRING:
		if(old_set & (1 << P_VALUE_STRING))
		{
			size_t		ol, nl;
			const char	*ns = (const char *) va_arg(arg, const char *);

			ol = strlen(v->v.vstring);
			nl = strlen(ns);
			if(nl <= ol)
				strcpy(v->v.vstring, ns);
			else
			{
				mem_free(v->v.vstring);
				v->v.vstring = stu_strdup(ns);
			}
		}
		else
			v->v.vstring = stu_strdup((const char *) va_arg(arg, const char *));
		return 1;
	default:
		LOG_WARN(("Problem in do_set(): type %d unhandled", type));
	}
	return 0;
}

int value_set(PValue *v, PValueType type, ...)
{
	va_list	arg;
	int	ret;

	if(v == NULL)
		return 0;
	va_start(arg, type);
	ret = do_set(v, type, arg);
	va_end(arg);

	return ret;
}

int value_set_va(PValue *v, PValueType type, va_list arg)
{
	if(v == NULL)
		return 0;
	return do_set(v, type, arg);
}

int value_set_defminmax_va(PValue *v, PValueType type, va_list *arg)
{
	if(type == P_VALUE_STRING)
		return value_set_va(v, type, *arg);
	else
	{
		double	d = va_arg(*arg, double);

		switch(type)
		{
		case P_VALUE_BOOLEAN:		return value_set(v, type, (boolean) (d != 0.0));
		case P_VALUE_INT32:		return value_set(v, type, (int32) d);
		case P_VALUE_UINT32:		return value_set(v, type, (uint32) d);
		case P_VALUE_REAL32:		return value_set(v, type, (real32) d);
		case P_VALUE_REAL32_VEC2:
		case P_VALUE_REAL32_VEC3:
		case P_VALUE_REAL32_VEC4:
			{
				real32	vec[4];
				int	i;

				vec[0] = d;
				for(i = 1; i < 2 + type - P_VALUE_REAL32_VEC2; i++)
					vec[i] = va_arg(*arg, double);
				return value_set(v, type, vec);
			}
		case P_VALUE_REAL64:		return value_set(v, type, (real64) d);
		case P_VALUE_REAL64_VEC2:
		case P_VALUE_REAL64_VEC3:
		case P_VALUE_REAL64_VEC4:
			{
				real64	vec[4];
				int	i;

				vec[0] = d;
				for(i = 1; i < 2 + type - P_VALUE_REAL64_VEC2; i++)
					vec[i] = va_arg(*arg, double);
				return value_set(v, type, vec);
			}
		default:
			LOG_WARN(("Can't set default/min/max of type %d--not implemented", type));	/* FIXME */
		}
	}
	return 0;
}


int value_set_from_string(PValue *v, PValueType type, const char *value)
{
	if(v == NULL || value == NULL)
		return 0;
	switch(type)
	{
	case P_VALUE_BOOLEAN:
		if(strcmp(value, "true") == 0)
			return value_set(v, type, 1);
		else if(strcmp(value, "false"))
			return value_set(v, type, 0);
		else
			LOG_WARN(("Couldn't parse '%s' as boolean value", value));
		break;
	case P_VALUE_INT32:
		{
			int32	t;
			if(sscanf(value, "%d", &t) == 1)
				return value_set(v, type, t);
			else
				LOG_WARN(("Couldn't parse '%s' as int32 value", value));
		}
		break;
	case P_VALUE_UINT32:
		{
			uint32	t;
			if(sscanf(value, "%u", &t) == 1)
				return value_set(v, type, t);
			else
				LOG_WARN(("Couldn't parse '%s' as uint32 value", value));
		}
		break;
	case P_VALUE_REAL32:
		{
			real32	t;
			if(sscanf(value, "%g", &t) == 1)
				return value_set(v, type, t);
			else
				LOG_WARN(("Couldn't parse '%s' as real32 value", value));
		}
		break;
	case P_VALUE_REAL64:
		{
			real64	t;
			if(sscanf(value, "%lg", &t) == 1)
				return value_set(v, type, t);
			else
				LOG_WARN(("Couldn't parse '%s' as real64 value", value));
		}
		break;
	case P_VALUE_STRING:
		return value_set(v, type, value);
	case P_VALUE_MODULE:
		{
			uint32	t;
			if(sscanf(value, "%u", &t) == 1)
				return value_set(v, type, t);
			else
				LOG_WARN(("Couldn't parse '%s' as module ID value", value));
		}
		break;
	default:
		LOG_WARN(("No code for setting value type %s from string--failing", type_map_by_value[type].name));
	}
	return 0;
}

/* ----------------------------------------------------------------------------------------- */

#define	DO_SET(v,t)	VALUE_SET(v, P_VALUE_ ##t)
#define	IF_SET(v,t)	if(VALUE_SETS(v, P_VALUE_ ##t))

#define	GET_FAIL(v,t,d)	LOG_WARN(("Failed to parse set %u as %s", (v)->set, t)); return d;

static boolean get_as_boolean(const PValue *in, boolean *out)
{
	if(in == NULL || out == NULL)
		return FALSE;
	if(in->set == 0)
		return FALSE;
	else IF_SET(in, BOOLEAN)
		*out = in->v.vboolean;
	else IF_SET(in, UINT32)
		*out = in->v.vuint32 > 0;
	else IF_SET(in, INT32)
		*out =  in->v.vint32 != 0;
	else IF_SET(in, REAL64)
		*out = in->v.vreal64 > 0.0;
	else IF_SET(in, REAL32)
		*out =  in->v.vreal32 > 0.0f;
	else IF_SET(in, REAL64_VEC2)
		*out =  vec_real64_vec2_magnitude(in->v.vreal64_vec2) > 0.0;
	else IF_SET(in, REAL64_VEC3)
		*out =  vec_real64_vec3_magnitude(in->v.vreal64_vec3) > 0.0;
	else IF_SET(in, REAL64_VEC4)
		*out = vec_real64_vec4_magnitude(in->v.vreal64_vec4) > 0.0;
	else IF_SET(in, REAL32_VEC2)
		*out = vec_real32_vec2_magnitude(in->v.vreal32_vec2) > 0.0;
	else IF_SET(in, REAL32_VEC3)
		*out = vec_real32_vec3_magnitude(in->v.vreal32_vec3) > 0.0;
	else IF_SET(in, REAL32_VEC4)
		*out = vec_real32_vec4_magnitude(in->v.vreal32_vec4) > 0.0;
	else IF_SET(in, STRING)
		*out = strcmp(in->v.vstring, "true") == 0;
	else
		return FALSE;
	return TRUE;
}

static boolean get_as_int32(const PValue *in, int32 *out)
{
	if(in == NULL || out == NULL)
		return FALSE;
	if(in->set == 0)
		return FALSE;
	else IF_SET(in, INT32)
		*out = in->v.vint32;
	else IF_SET(in, REAL32)
		*out = in->v.vreal32;
	else IF_SET(in, REAL64)
		*out = in->v.vreal64;
	else IF_SET(in, REAL32_VEC2)
		*out =vec_real32_vec2_magnitude(in->v.vreal32_vec2);
	else IF_SET(in, REAL32_VEC3)
		*out =vec_real32_vec3_magnitude(in->v.vreal32_vec3);
	else IF_SET(in, REAL32_VEC4)
		*out =vec_real32_vec4_magnitude(in->v.vreal32_vec4);
	else IF_SET(in, REAL64_VEC2)
		*out =vec_real64_vec2_magnitude(in->v.vreal64_vec2);
	else IF_SET(in, REAL64_VEC3)
		*out =vec_real64_vec3_magnitude(in->v.vreal64_vec3);
	else IF_SET(in, REAL64_VEC4)
		*out =vec_real64_vec4_magnitude(in->v.vreal64_vec4);
	else IF_SET(in, STRING)
		*out = strtol(in->v.vstring, NULL, 0);
	else IF_SET(in, UINT32)
		*out = in->v.vuint32;
	else IF_SET(in, BOOLEAN)
		*out = in->v.vboolean;
	else
		return FALSE;
	return TRUE;
}

static boolean get_as_uint32(const PValue *in, uint32 *out)
{
	if(in == NULL || out == NULL)
		return FALSE;
	if(in->set == 0)
		return FALSE;
	else IF_SET(in, UINT32)
		*out = in->v.vuint32;
	else IF_SET(in, INT32)
		*out = in->v.vint32;
	else IF_SET(in, REAL32)
		*out = in->v.vreal32;
	else IF_SET(in, REAL64)
		*out = in->v.vreal64;
	else IF_SET(in, REAL32_VEC2)
		*out = vec_real32_vec2_magnitude(in->v.vreal32_vec2);
	else IF_SET(in, REAL32_VEC3)
		*out = vec_real32_vec3_magnitude(in->v.vreal32_vec3);
	else IF_SET(in, REAL32_VEC4)
		*out = vec_real32_vec4_magnitude(in->v.vreal32_vec4);
	else IF_SET(in, REAL64_VEC2)
		*out = vec_real64_vec2_magnitude(in->v.vreal64_vec2);
	else IF_SET(in, REAL64_VEC3)
		*out = vec_real64_vec3_magnitude(in->v.vreal64_vec3);
	else IF_SET(in, REAL64_VEC4)
		*out = vec_real64_vec4_magnitude(in->v.vreal64_vec4);
	else IF_SET(in, STRING)
		*out = strtol(in->v.vstring, NULL, 0);
	else IF_SET(in, BOOLEAN)
		*out = in->v.vboolean;
	else
		return FALSE;
	return TRUE;
}

/* Get a value as the largest scalar type, real64. This is then used to implement
 * the other scalar getters. Will mash vectors and matrices down to a single scalar.
*/
static boolean get_as_real64(const PValue *in, real64 *out)
{
	if(in == NULL || out == NULL)
		return FALSE;
	if(in->set == 0)
		return FALSE;
	else IF_SET(in, REAL64)
		*out = in->v.vreal64;
	else IF_SET(in, REAL64_VEC2)
		*out = vec_real64_vec2_magnitude(in->v.vreal64_vec2);
	else IF_SET(in, REAL64_VEC3)
		*out = vec_real64_vec3_magnitude(in->v.vreal64_vec3);
	else IF_SET(in, REAL64_VEC4)
		*out = vec_real64_vec4_magnitude(in->v.vreal64_vec4);
	else IF_SET(in, REAL32)
		*out = in->v.vreal32;
	else IF_SET(in, REAL32_VEC2)
		*out = vec_real32_vec2_magnitude(in->v.vreal32_vec2);
	else IF_SET(in, REAL32_VEC3)
		*out = vec_real32_vec3_magnitude(in->v.vreal32_vec3);
	else IF_SET(in, REAL32_VEC4)
		*out = vec_real32_vec4_magnitude(in->v.vreal32_vec4);
	else IF_SET(in, INT32)
		*out = in->v.vint32;
	else IF_SET(in, UINT32)
		*out = in->v.vuint32;
	else IF_SET(in, REAL64_MAT16)
		*out = vec_real64_mat16_determinant(in->v.vreal64_mat16);
	else IF_SET(in, REAL32_MAT16)
		*out = vec_real32_mat16_determinant(in->v.vreal32_mat16);
	else IF_SET(in, STRING)
		*out = strtod(in->v.vstring, NULL);
	else
		return FALSE;
	return TRUE;
}

static boolean get_as_real32(const PValue *in, real32 *out)
{
	real64	tmp;

	if(get_as_real64(in, &tmp))
	{
		*out = tmp;
		return TRUE;
	}
	return FALSE;
}

static boolean get_as_real32_vec2(const PValue *in, real32 *out)
{
	return FALSE;
}

static boolean get_as_real32_vec3(const PValue *in, real32 out[])
{
	return FALSE;
}

static boolean get_as_real32_vec4(const PValue *in, real32 out[])
{
	return FALSE;
}

static boolean get_as_real32_mat16(const PValue *in, real32 out[])
{
	return FALSE;
}

static boolean get_as_real64_vec2(const PValue *in, real64 out[])
{
	return FALSE;
}

static boolean get_as_real64_vec3(const PValue *in, real64 out[])
{
	return FALSE;
}

static boolean get_as_real64_vec4(const PValue *in, real64 out[])
{
	return FALSE;
}

static boolean get_as_real64_mat16(const PValue *in, real64 out[])
{
	return FALSE;
}

/* A macro to avoid writing N very simliar functions. Should cut down on the number of
 * stupid errors.
*/
#define	GETTER(n,f,t,pt)	\
boolean value_get_ ##n (const PValue *v, PValue *cache, t *out) \
{ \
	if(v == NULL || out == NULL) \
		return FALSE; \
	IF_SET(v, f) \
	{ \
		*out = (t) v->v.v##n; \
		return TRUE; \
	} \
	else if(cache != NULL) \
	{ \
		IF_SET(cache, f) \
		{ \
			*out = cache->v.v##n; \
			return TRUE; \
		} \
		if(get_as_##n(v, (pt) &cache->v.v##n)) /* Cast to get around &a<->a problem for array types. */ \
		{ \
			DO_SET(cache, f); \
			*out = cache->v.v##n; \
			return TRUE; \
		} \
	} \
	return FALSE; \
}

/* Use the above macro to define accessor functions for the types needed. */
GETTER(boolean, BOOLEAN, boolean, boolean *)
GETTER(int32, INT32, int32, int32 *)
GETTER(uint32, UINT32, uint32, uint32 *)
GETTER(real32, REAL32, real32, real32 *)
GETTER(real32_vec2, REAL32_VEC2, const real32 *, real32 *)
GETTER(real32_vec3, REAL32_VEC3, const real32 *, real32 *)
GETTER(real32_vec4, REAL32_VEC4, const real32 *, real32 *)
GETTER(real32_mat16, REAL32_MAT16, const real32 *, real32 *)
GETTER(real64, REAL64, real64, real64 *)
GETTER(real64_vec2, REAL64_VEC2, const real64 *, real64 *)
GETTER(real64_vec3, REAL64_VEC3, const real64 *, real64 *)
GETTER(real64_vec4, REAL64_VEC4, const real64 *, real64 *)
GETTER(real64_mat16, REAL64_MAT16, const real64 *, real64 *)

/* String-getting needs hand-written accessor function, since it has
 * different buffering semantics for cache misses.
*/
boolean value_get_string(const PValue *v, PValue *cache, const char **out)
{
	if(v == NULL || out == NULL)
		return FALSE;
	if(v->set == 0)
		return FALSE;
	IF_SET(v, STRING)
	{
		*out = v->v.vstring;
		return TRUE;
	}
	else if(cache != NULL)
	{
		char		buf[1024];	/* Used for temporary formatting, then copied dynamically. Not free. */
		const char	*p;
		size_t		used;

		IF_SET(cache, STRING)
		{
			*out = cache->v.vstring;
			return TRUE;
		}
		else if((p = value_as_string(v, buf, sizeof buf, &used)) != NULL)	/* Never returns v->v.vstring. */
		{
			cache->v.vstring = mem_alloc(used);
			memcpy(cache->v.vstring, p, used);
			DO_SET(cache, STRING);
			*out = cache->v.vstring;
			return TRUE;
		}
	}
	return FALSE;
}

uint32 value_get_module(const PValue *v, PValue *cache)
{
	if(v == NULL)
		return 0;
	if(v->set == 0)
		return 0;
	IF_SET(v, MODULE)
		return v->v.vmodule;
	/* No caching takes place here, it does not make sense for module links. */
	GET_FAIL(v, "module", 0);
	return 0;
}

/* ----------------------------------------------------------------------------------------- */

#define	DEF(n,t,d,v)	t value_get_default_ ##n(void) { d; return v; }

DEF(boolean, boolean,, FALSE)
DEF(int32,   int32,,   0)
DEF(uint32,  uint32,,  0)
DEF(real32,  real32,, 0.0)
DEF(real32_vec2, const real32 *, static real32 def[2] = { 0.0 }, def)
DEF(real32_vec3, const real32 *, static real32 def[3] = { 0.0 }, def)
DEF(real32_vec4, const real32 *, static real32 def[4] = { 0.0 }, def)
DEF(real32_mat16, const real32 *, static real32 def[16] = { 0.0 }, def)
DEF(real64,  real64,, 0.0)
DEF(real64_vec2, const real64 *, static real64 def[2] = { 0.0 }, def)
DEF(real64_vec3, const real64 *, static real64 def[3] = { 0.0 }, def)
DEF(real64_vec4, const real64 *, static real64 def[4] = { 0.0 }, def)
DEF(real64_mat16, const real64 *, static real64 def[16] = { 0.0 }, def)
DEF(string, const char *,, "")

/* ----------------------------------------------------------------------------------------- */

const char * value_as_string(const PValue *v, char *buf, size_t buf_max, size_t *used)
{
	int	put;

	/* Check for native string first. */
	IF_SET(v, STRING)
		return v->v.vstring;
	else IF_SET(v, BOOLEAN)
		put = snprintf(buf, buf_max, "%s", v->v.vboolean ? "true" : "false");
	else IF_SET(v, INT32)
		put = snprintf(buf, buf_max, "%d", v->v.vint32);
	else IF_SET(v, UINT32)
		put = snprintf(buf, buf_max, "%u", v->v.vuint32);
	else IF_SET(v, REAL32)
		put = snprintf(buf, buf_max, "%g", v->v.vreal32);
	else IF_SET(v, REAL64)
		put = snprintf(buf, buf_max, "%g", v->v.vreal64);
	else IF_SET(v, REAL32_VEC2)
		put = snprintf(buf, buf_max, "[%g %g]", v->v.vreal32_vec2[0], v->v.vreal32_vec2[1]);
	else IF_SET(v, REAL32_VEC3)
		put = snprintf(buf, buf_max, "[%g %g %g]", v->v.vreal32_vec3[0], v->v.vreal32_vec3[1], v->v.vreal32_vec3[2]);
	else IF_SET(v, REAL32_VEC4)
		put = snprintf(buf, buf_max, "[%g %g %g %g]", v->v.vreal32_vec4[0], v->v.vreal32_vec4[1],
			       v->v.vreal32_vec4[2], v->v.vreal32_vec4[3]);
	else IF_SET(v, REAL32_MAT16)
		put = snprintf(buf, buf_max, "[[%g %g %g %g][%g %g %g %g][%g %g %g %g][%g %g %g %g]]",
			 v->v.vreal32_mat16[0], v->v.vreal32_mat16[1], v->v.vreal32_mat16[2], v->v.vreal32_mat16[3],
			 v->v.vreal32_mat16[4], v->v.vreal32_mat16[5], v->v.vreal32_mat16[6], v->v.vreal32_mat16[7],
			 v->v.vreal32_mat16[8], v->v.vreal32_mat16[9], v->v.vreal32_mat16[10], v->v.vreal32_mat16[11],
			 v->v.vreal32_mat16[12], v->v.vreal32_mat16[13], v->v.vreal32_mat16[14], v->v.vreal32_mat16[15]);
	else IF_SET(v, REAL64)
		put = snprintf(buf, buf_max, "%.10g", v->v.vreal64);
	else IF_SET(v, REAL64_VEC2)
		put = snprintf(buf, buf_max, "[%.10g %.10g]", v->v.vreal64_vec2[0], v->v.vreal64_vec2[1]);
	else IF_SET(v, REAL64_VEC3)
		put = snprintf(buf, buf_max, "[%.10g %.10g %.10g]", v->v.vreal64_vec3[0], v->v.vreal64_vec3[1], v->v.vreal64_vec3[2]);
	else IF_SET(v, REAL64_VEC4)
		put = snprintf(buf, buf_max, "[%.10g %.10g %.10g %.10g]",
			 v->v.vreal64_vec4[0], v->v.vreal64_vec4[1], v->v.vreal64_vec4[2], v->v.vreal64_vec4[3]);
	else IF_SET(v, REAL64_MAT16)
		put = snprintf(buf, buf_max, "[[%.10g %.10g %.10g %.10g][%.10g %.10g %.10g %.10g]"
			 "[%.10g %.10g %.10g %.10g][%.10g %.10g %.10g %.10g]]",
			 v->v.vreal64_mat16[0], v->v.vreal64_mat16[1], v->v.vreal64_mat16[2], v->v.vreal64_mat16[3],
			 v->v.vreal64_mat16[4], v->v.vreal64_mat16[5], v->v.vreal64_mat16[6], v->v.vreal64_mat16[7],
			 v->v.vreal64_mat16[8], v->v.vreal64_mat16[9], v->v.vreal64_mat16[10], v->v.vreal64_mat16[11],
			 v->v.vreal64_mat16[12], v->v.vreal64_mat16[13], v->v.vreal64_mat16[14], v->v.vreal64_mat16[15]);
	else IF_SET(v, MODULE)
		put = snprintf(buf, buf_max, "%u", v->v.vmodule);

	if(put > 0)
	{
		if(used != NULL)
			*used = put + 1;
		return buf;
	}
	return NULL;
}
