/*
 * 
*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "verse.h"

#include "mem.h"
#include "list.h"
#include "log.h"
#include "strutil.h"
#include "vecutil.h"

#include "value.h"

/* ----------------------------------------------------------------------------------------- */

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
	v->type = P_VALUE_NONE;
	v->v.vstring = NULL;		/* Extra important? Naah. */
}

void value_clear(PValue *v)
{
	if(v == NULL || v->type == P_VALUE_NONE)
		return;
	if(v->type == P_VALUE_STRING)
	{
		mem_free(v->v.vstring);
		v->v.vstring = NULL;
	}
	v->type = P_VALUE_NONE;
}

PValueType value_peek_type(const PValue *v)
{
	return v != NULL ? v->type : P_VALUE_NONE;
}

/* ----------------------------------------------------------------------------------------- */

static int do_set(PValue *v, PValueType type, va_list arg)
{
	value_clear(v);
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

void value_set(PValue *v, PValueType type, ...)
{
	va_list	arg;

	if(v == NULL)
		return;
	va_start(arg, type);
	do_set(v, type, arg);
	va_end(arg);
}

void value_set_va(PValue *v, PValueType type, va_list arg)
{
	if(v == NULL)
		return;
	do_set(v, type, arg);
}

/* ----------------------------------------------------------------------------------------- */

/* Get a value as the largest scalar type, real64. This is then used to implement
 * the other scalar getters. Will mash vectors and matrices down to a single scalar.
*/
static real64 get_as_real64(const PValue *v)
{
	if(v == NULL)
		return 0.0;
	switch(v->type)
	{
	case P_VALUE_NONE:
		return 0.0;
	case P_VALUE_BOOLEAN:
		return v->v.vboolean;
	case P_VALUE_INT32:
		return v->v.vint32;
	case P_VALUE_UINT32:
		return v->v.vuint32;
	case P_VALUE_REAL32:
		return v->v.vreal32;
	case P_VALUE_REAL32_VEC2:
		return vec_real32_vec2_magnitude(v->v.vreal32_vec2);
	case P_VALUE_REAL32_VEC3:
		return vec_real32_vec3_magnitude(v->v.vreal32_vec3);
	case P_VALUE_REAL32_VEC4:
		return vec_real32_vec3_magnitude(v->v.vreal32_vec4);
	case P_VALUE_REAL32_MAT16:
		return vec_real32_mat16_determinant(v->v.vreal32_mat16);
	case P_VALUE_REAL64:
		return v->v.vreal64;
	case P_VALUE_REAL64_VEC2:
		return vec_real64_vec2_magnitude(v->v.vreal64_vec2);
	case P_VALUE_REAL64_VEC3:
		return vec_real64_vec3_magnitude(v->v.vreal64_vec3);
	case P_VALUE_REAL64_VEC4:
		return vec_real64_vec3_magnitude(v->v.vreal64_vec4);
	case P_VALUE_REAL64_MAT16:
		return vec_real64_mat16_determinant(v->v.vreal64_mat16);
	case P_VALUE_STRING:
		return strtod(v->v.vstring, NULL);
	default:
		LOG_WARN(("This can't happen\n"));
	}
	return 0.0;
}

boolean value_get_boolean(const PValue *v)
{
	return get_as_real64(v);
}

int32 value_get_int32(const PValue *v)
{
	return get_as_real64(v);
}

uint32 value_get_uint32(const PValue *v)
{
	return get_as_real64(v);
}

real32 value_get_real32(const PValue *v)
{
	return get_as_real64(v);
}

real64 value_get_real64(const PValue *v)
{
	return get_as_real64(v);
}

const char * value_get_string(const PValue *v, char *buf, size_t buf_max)
{
	int	put = -1;

	if(v == NULL)
		return "";
	if(v->type == P_VALUE_STRING)
		return v->v.vstring;
	if(buf == NULL || buf_max < 2)
		return NULL;		/* Signals failure, might be handy? */
	switch(v->type)
	{
	case P_VALUE_BOOLEAN:
		put = snprintf(buf, buf_max, "%s", v->v.vboolean ? "TRUE" : "FALSE");
		break;
	case P_VALUE_INT32:
		put = snprintf(buf, buf_max, "%d", v->v.vint32);
		break;
	case P_VALUE_UINT32:
		put = snprintf(buf, buf_max, "%u", v->v.vuint32);
		break;
	case P_VALUE_REAL32:
		put = snprintf(buf, buf_max, "%g", v->v.vreal32);
		break;
	case P_VALUE_REAL32_VEC2:
		put = snprintf(buf, buf_max, "[%g %g]", v->v.vreal32_vec2[0], v->v.vreal32_vec2[1]);
		break;
	case P_VALUE_REAL32_VEC3:
		put = snprintf(buf, buf_max, "[%g %g %g]", v->v.vreal32_vec3[0], v->v.vreal32_vec3[1], v->v.vreal32_vec3[2]);
		break;
	case P_VALUE_REAL32_VEC4:
		put = snprintf(buf, buf_max, "[%g %g %g %g]",
			 v->v.vreal32_vec4[0], v->v.vreal32_vec4[1], v->v.vreal32_vec4[2], v->v.vreal32_vec4[3]);
		break;
	case P_VALUE_REAL32_MAT16:
		put = snprintf(buf, buf_max, "[[%g %g %g %g][%g %g %g %g][%g %g %g %g][%g %g %g %g]]",
			 v->v.vreal32_mat16[0], v->v.vreal32_mat16[1], v->v.vreal32_mat16[2], v->v.vreal32_mat16[3],
			 v->v.vreal32_mat16[4], v->v.vreal32_mat16[5], v->v.vreal32_mat16[6], v->v.vreal32_mat16[7],
			 v->v.vreal32_mat16[8], v->v.vreal32_mat16[9], v->v.vreal32_mat16[10], v->v.vreal32_mat16[11],
			 v->v.vreal32_mat16[12], v->v.vreal32_mat16[13], v->v.vreal32_mat16[14], v->v.vreal32_mat16[15]);
		break;
	case P_VALUE_REAL64:
		put = snprintf(buf, buf_max, "%.10g", v->v.vreal64);
		break;
	case P_VALUE_REAL64_VEC2:
		put = snprintf(buf, buf_max, "[%.10g %.10g]", v->v.vreal64_vec2[0], v->v.vreal64_vec2[1]);
		break;
	case P_VALUE_REAL64_VEC3:
		put = snprintf(buf, buf_max, "[%.10g %.10g %.10g]", v->v.vreal64_vec3[0], v->v.vreal64_vec3[1], v->v.vreal64_vec3[2]);
		break;
	case P_VALUE_REAL64_VEC4:
		put = snprintf(buf, buf_max, "[%.10g %.10g %.10g %.10g]",
			 v->v.vreal64_vec4[0], v->v.vreal64_vec4[1], v->v.vreal64_vec4[2], v->v.vreal64_vec4[3]);
		break;
	case P_VALUE_REAL64_MAT16:
		put = snprintf(buf, buf_max, "[[%.10g %.10g %.10g %.10g][%.10g %.10g %.10g %.10g]"
			 "[%.10g %.10g %.10g %.10g][%.10g %.10g %.10g %.10g]]",
			 v->v.vreal64_mat16[0], v->v.vreal64_mat16[1], v->v.vreal64_mat16[2], v->v.vreal64_mat16[3],
			 v->v.vreal64_mat16[4], v->v.vreal64_mat16[5], v->v.vreal64_mat16[6], v->v.vreal64_mat16[7],
			 v->v.vreal64_mat16[8], v->v.vreal64_mat16[9], v->v.vreal64_mat16[10], v->v.vreal64_mat16[11],
			 v->v.vreal64_mat16[12], v->v.vreal64_mat16[13], v->v.vreal64_mat16[14], v->v.vreal64_mat16[15]);
		break;
	default:
		;
	}
	if(put > 0)
		return buf;
	return "";
}
