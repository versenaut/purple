/*
 * Value handling module. Values are containers that at any point can hold a set of values,
 * each having a distinct type. They are used to form "ports", which model plug-in inputs and
 * outputs. If the world was slightly more perfect, input ports would be implemented as unions,
 * since you cannot assign multiple literal values to an input. You can, however, store more
 * than one value in an output, so it was easier to make them based on the same value structure.
*/

/* A Purple simple value. Things that are not full nodes are considered "simple". */
typedef struct
{
	uint16	set;
	struct {
	boolean	vboolean;
	int32	vint32;
	uint32	vuint32;
	real32	vreal32;
	real32	vreal32_vec2[2];
	real32	vreal32_vec3[3];
	real32	vreal32_vec4[4];
	real32	vreal32_mat16[16];

	real64	vreal64;
	real64	vreal64_vec2[2];
	real64	vreal64_vec3[3];
	real64	vreal64_vec4[4];
	real64	vreal64_mat16[16];

	char	*vstring;
	uint32	vmodule;
	}	v;
} PValue;

/* Handy helper functions. */
extern const char *	value_type_to_name(PValueType type);
extern PValueType	value_type_from_name(const char *name);

/* Use right after allocation of a new PValue, to make sure it's sane. */
extern void		value_init(PValue *v);
/* Use before deallocation, and/or whenever a value needs to be reset to nothingness. */
extern void		value_clear(PValue *v);

/* Store a new value in the given value. Replaces any previous value of the same type. */
extern int		value_set_va(PValue *v, PValueType type, va_list arg);
/* Store a new value in the given value, with knowledge that is uses special default/min/max semantics. */
extern int		value_set_defminmax_va(PValue *v, PValueType type, va_list arg);
extern int		value_set(PValue *v, PValueType type, ...);

/* Check if the indicated value is present in the value. A present value is never returned from cache. */
extern boolean		value_type_present(const PValue *v, PValueType type);

/* Return name of the type the value is set to. Only works if there is exactly one type set. */
extern const char *	value_type_name(const PValue *v);

/* Get a value, reading from (and updating) the optional cache if the source value does
 * not hold the desired type. Data ownership is always in the value or the cache.
*/
extern boolean		value_get_boolean(const PValue *v, PValue *cache, boolean *out);
extern boolean		value_get_int32(const PValue *v, PValue *cache, int32 *out);
extern boolean		value_get_uint32(const PValue *v, PValue *cache, uint32 *out);
extern boolean		value_get_real32(const PValue *v, PValue *cache, real32 *out);
extern boolean		value_get_real32_vec2(const PValue *v, PValue *cache, const real32 **out);
extern boolean		value_get_real32_vec3(const PValue *v, PValue *cache, const real32 **out);
extern boolean		value_get_real32_vec4(const PValue *v, PValue *cache, const real32 **out);
extern boolean		value_get_real32_mat16(const PValue *v, PValue *cache, const real32 **out);
extern boolean		value_get_real64(const PValue *v, PValue *cache, real64 *out);
extern boolean		value_get_real64_vec2(const PValue *v, PValue *cache, const real64 **out);
extern boolean		value_get_real64_vec3(const PValue *v, PValue *cache, const real64 **out);
extern boolean		value_get_real64_vec4(const PValue *v, PValue *cache, const real64 **out);
extern boolean		value_get_real64_mat16(const PValue *v, PValue *cache, const real64 **out);
extern boolean		value_get_string(const PValue *v, PValue *cache, const char **out);
extern uint32		value_get_module(const PValue *v, PValue *cache);

extern boolean		value_get_default_boolean(void);
extern int32		value_get_default_int32(void);
extern uint32		value_get_default_uint32(void);
extern real32		value_get_default_real32(void);
extern const real32 *	value_get_default_real32_vec2(void);
extern const real32 *	value_get_default_real32_vec3(void);
extern const real32 *	value_get_default_real32_vec4(void);
extern const real32 *	value_get_default_real32_mat16(void);
extern real64		value_get_default_real64(void);
extern const real64 *	value_get_default_real64_vec2(void);
extern const real64 *	value_get_default_real64_vec3(void);
extern const real64 *	value_get_default_real64_vec4(void);
extern const real64 *	value_get_default_real64_mat16(void);
extern const char *	value_get_default_string(void);


/* Create string representation of <v>, which is assumed to contain a value of a
 * single type only. Useful in creating external representations, and also inten-
 * ally when getting as string. No clever size handling is done on the buffer,
 * and obviously no caching is involved, the string representation is not retained.
*/
extern const char *	value_as_string(const PValue *v, char *buf, size_t buf_max, size_t *buf_used);
