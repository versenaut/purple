/*
 * 
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
			qsort(type_map_by_name,
			      sizeof type_map_by_name / sizeof *type_map_by_name,
			      sizeof *type_map_by_name, cmp_type_name);
			sorted = 1;
		}
		if((tna = bsearch(name, type_map_by_name,
				  sizeof type_map_by_name / sizeof *type_map_by_name,
				  sizeof type_map_by_name, srch_type_name)) != NULL)
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
	value_clear(v);
	v->set |= (1 << type);
	switch(type)
	{
	case P_VALUE_BOOLEAN:
		v->v.vboolean = (boolean) va_arg(arg, int);
		return 0;
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

/* ----------------------------------------------------------------------------------------- */

#define	DO_SET(v,t)	VALUE_SET(v, P_VALUE_ ##t)
#define	IF_SET(v,t)	if(VALUE_SETS(v, P_VALUE_ ##t))

#define	GET_FAIL(v,t,d)	LOG_WARN(("Failed to parse set %u as %s", (v)->set, t)); return d;

static boolean get_as_boolean(const PValue *v)
{
	if(v == NULL)
		return FALSE;
	if(v->set == 0)
		return FALSE;
	else IF_SET(v, BOOLEAN)
		return v->v.vboolean;
	else IF_SET(v, UINT32)
		return v->v.vuint32 > 0;
	else IF_SET(v, INT32)
		return v->v.vint32 != 0;
	else IF_SET(v, REAL64)
		return v->v.vreal64 > 0.0;
	else IF_SET(v, REAL32)
		return v->v.vreal32 > 0.0f;
	else IF_SET(v, REAL64_VEC2)
		return vec_real64_vec2_magnitude(v->v.vreal64_vec2) > 0.0;
	else IF_SET(v, REAL64_VEC3)
		return vec_real64_vec3_magnitude(v->v.vreal64_vec3) > 0.0;
	else IF_SET(v, REAL64_VEC4)
		return vec_real64_vec4_magnitude(v->v.vreal64_vec4) > 0.0;
	else IF_SET(v, REAL32_VEC2)
		return vec_real32_vec2_magnitude(v->v.vreal32_vec2) > 0.0;
	else IF_SET(v, REAL32_VEC3)
		return vec_real32_vec3_magnitude(v->v.vreal32_vec3) > 0.0;
	else IF_SET(v, REAL32_VEC4)
		return vec_real32_vec4_magnitude(v->v.vreal32_vec4) > 0.0;
	else IF_SET(v, STRING)
		return strcmp(v->v.vstring, "TRUE") == 0;
	GET_FAIL(v, "boolean", FALSE);
}

boolean value_get_boolean(const PValue *v, PValue *cache)
{
	if(v == NULL)
		return FALSE;
	IF_SET(v, BOOLEAN)
		return v->v.vboolean;
	else if(cache != NULL)
	{
		IF_SET(cache, BOOLEAN)
			return cache->v.vboolean;
		DO_SET(cache, BOOLEAN);
		return cache->v.vboolean = get_as_boolean(v);
	}
	return get_as_boolean(v);
}


int32 value_get_int32(const PValue *v, PValue *cache)
{
	return 0;	/* FIXME: Not implemented. */
}

uint32 value_get_uint32(const PValue *v, PValue *cache)
{
	return 0u;	/* FIXME: Not implemented. */
}

/* Get a value as the largest scalar type, real64. This is then used to implement
 * the other scalar getters. Will mash vectors and matrices down to a single scalar.
*/
static real64 get_as_real64(const PValue *v)
{
	if(v == NULL)
		return 0.0;
	if(v->set == 0)	/* This should probably not be possible due to checks higher up. */
		return 0.0;
	else IF_SET(v, REAL64)
		return v->v.vreal64;
	else IF_SET(v, REAL64_VEC2)
		return vec_real64_vec2_magnitude(v->v.vreal64_vec2);
	else IF_SET(v, REAL64_VEC3)
		return vec_real64_vec3_magnitude(v->v.vreal64_vec3);
	else IF_SET(v, REAL64_VEC4)
		return vec_real64_vec4_magnitude(v->v.vreal64_vec4);
	else IF_SET(v, REAL32)
		return v->v.vreal32;
	else IF_SET(v, REAL32_VEC2)
		return vec_real32_vec2_magnitude(v->v.vreal32_vec2);
	else IF_SET(v, REAL32_VEC3)
		return vec_real32_vec3_magnitude(v->v.vreal32_vec3);
	else IF_SET(v, REAL32_VEC4)
		return vec_real32_vec4_magnitude(v->v.vreal32_vec4);
	else IF_SET(v, INT32)
		return v->v.vint32;
	else IF_SET(v, UINT32)
		return v->v.vuint32;
	else IF_SET(v, REAL64_MAT16)
		return vec_real64_mat16_determinant(v->v.vreal64_mat16);
	else IF_SET(v, REAL32_MAT16)
		return vec_real32_mat16_determinant(v->v.vreal32_mat16);
	else IF_SET(v, STRING)
		return strtod(v->v.vstring, NULL);
	GET_FAIL(v, "real64", 0.0);
}

real32 value_get_real32(const PValue *v, PValue *cache)
{
	if(v == NULL)
		return 0.0;
	IF_SET(v, REAL32)
		return v->v.vreal32;
	else if(cache != NULL)
	{
		IF_SET(cache, REAL32)
			return cache->v.vreal32;
		DO_SET(cache, REAL32);
		return cache->v.vreal32 = get_as_real64(v);	/* Drop precision. */
	}
	return get_as_real64(v);
}

const real32 * value_get_real32_vec2(const PValue *v, PValue *cache)
{
	return NULL;	/* FIXME: Implement heuristics. */
}

const real32 * value_get_real32_vec3(const PValue *v, PValue *cache)
{
	return NULL;	/* FIXME: Implement heuristics. */
}

const real32 * value_get_real32_vec4(const PValue *v, PValue *cache)
{
	return NULL;	/* FIXME: Implement heuristics. */
}

const real32 * value_get_real32_mat16(const PValue *v, PValue *cache)
{
	return NULL;	/* FIXME: Implement heuristics. */
}

real64 value_get_real64(const PValue *v, PValue *cache)
{
	if(v == NULL)
		return 0.0;
	IF_SET(v, REAL64)
		return v->v.vreal64;
	else if(cache != NULL)
	{
		IF_SET(cache, REAL64)
			return cache->v.vreal64;
		DO_SET(cache, REAL64);
		return cache->v.vreal64 = get_as_real64(v);
	}
	return get_as_real64(v);
}

const real64 * value_get_real64_vec2(const PValue *v, PValue *cache)
{
	return NULL;	/* FIXME: Implement heuristics. */
}

const real64 * value_get_real64_vec3(const PValue *v, PValue *cache)
{
	return NULL;	/* FIXME: Implement heuristics. */
}

const real64 * value_get_real64_vec4(const PValue *v, PValue *cache)
{
	return NULL;	/* FIXME: Implement heuristics. */
}

const real64 * value_get_real64_mat16(const PValue *v, PValue *cache)
{
	return NULL;	/* FIXME: Implement heuristics. */
}

const char * value_get_string(const PValue *v, PValue *cache)
{
	if(v == NULL)
		return "";
	if(v->set == 0)
		return NULL;
	IF_SET(v, STRING)
		return v->v.vstring;
	else if(cache != NULL)
	{
		char		buf[1024];	/* Used for temporary formatting, then copied dynamically. Not free. */
		const char	*p;
		size_t		used;

		IF_SET(cache, STRING)
			return cache->v.vstring;
		else if((p = value_as_string(v, buf, sizeof buf, &used)) != NULL)	/* Never returns v->v.vstring. */
		{
			cache->v.vstring = mem_alloc(used);
			memcpy(cache->v.vstring, buf, used);
			DO_SET(cache, STRING);
			return cache->v.vstring;
		}
		GET_FAIL(v, "string", "");
	}
	return NULL;
}

uint32 value_get_module(const PValue *v, PValue *cache)
{
	if(v == NULL)
		return 0;
	if(v->set == 0)
		return 0;
	IF_SET(v, MODULE)
		return v->v.vmodule;
	else if(cache != NULL)
	{
		/* FIXME: Cache management code missing here. */
		GET_FAIL(v, "module", 0);
	}
	return 0;
}

const char * value_as_string(const PValue *v, char *buf, size_t buf_max, size_t *used)
{
	int	put;

	/* Check for native string first. */
	IF_SET(v, STRING)
		return v->v.vstring;
	else IF_SET(v, BOOLEAN)
		put = snprintf(buf, sizeof buf, "%s", v->v.vboolean ? "TRUE" : "FALSE");
	else IF_SET(v, INT32)
		put = snprintf(buf, sizeof buf, "%d", v->v.vint32);
	else IF_SET(v, UINT32)
		put = snprintf(buf, sizeof buf, "%u", v->v.vuint32);
	else IF_SET(v, REAL32)
		put = snprintf(buf, sizeof buf, "%g", v->v.vreal32);
	else IF_SET(v, REAL64)
		put = snprintf(buf, sizeof buf, "%10g", v->v.vreal64);
	else IF_SET(v, REAL32_VEC2)
		put = snprintf(buf, sizeof buf, "[%g %g]", v->v.vreal32_vec2[0], v->v.vreal32_vec2[1]);
	else IF_SET(v, REAL32_VEC3)
		put = snprintf(buf, sizeof buf, "[%g %g %g]", v->v.vreal32_vec3[0], v->v.vreal32_vec3[1], v->v.vreal32_vec3[2]);
	else IF_SET(v, REAL32_VEC4)
		put = snprintf(buf, sizeof buf, "[%g %g %g %g]", v->v.vreal32_vec4[0], v->v.vreal32_vec4[1],
			       v->v.vreal32_vec4[2], v->v.vreal32_vec4[3]);
	else IF_SET(v, REAL32_MAT16)
		put = snprintf(buf, sizeof buf, "[[%g %g %g %g][%g %g %g %g][%g %g %g %g][%g %g %g %g]]",
			 v->v.vreal32_mat16[0], v->v.vreal32_mat16[1], v->v.vreal32_mat16[2], v->v.vreal32_mat16[3],
			 v->v.vreal32_mat16[4], v->v.vreal32_mat16[5], v->v.vreal32_mat16[6], v->v.vreal32_mat16[7],
			 v->v.vreal32_mat16[8], v->v.vreal32_mat16[9], v->v.vreal32_mat16[10], v->v.vreal32_mat16[11],
			 v->v.vreal32_mat16[12], v->v.vreal32_mat16[13], v->v.vreal32_mat16[14], v->v.vreal32_mat16[15]);
	else IF_SET(v, REAL64)
		put = snprintf(buf, sizeof buf, "%.10g", v->v.vreal64);
	else IF_SET(v, REAL64_VEC2)
		put = snprintf(buf, sizeof buf, "[%.10g %.10g]", v->v.vreal64_vec2[0], v->v.vreal64_vec2[1]);
	else IF_SET(v, REAL64_VEC3)
		put = snprintf(buf, sizeof buf, "[%.10g %.10g %.10g]", v->v.vreal64_vec3[0], v->v.vreal64_vec3[1], v->v.vreal64_vec3[2]);
	else IF_SET(v, REAL64_VEC4)
		put = snprintf(buf, sizeof buf, "[%.10g %.10g %.10g %.10g]",
			 v->v.vreal64_vec4[0], v->v.vreal64_vec4[1], v->v.vreal64_vec4[2], v->v.vreal64_vec4[3]);
	else IF_SET(v, REAL64_MAT16)
		put = snprintf(buf, sizeof buf, "[[%.10g %.10g %.10g %.10g][%.10g %.10g %.10g %.10g]"
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
