/*
 * 
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

extern const char *	value_type_to_name(PValueType type);
extern PValueType	value_type_from_name(const char *name);

/* Use right after allocation of a new PValue, to make sure it's sane. */
extern void		value_init(PValue *v);
/* Use before deallocation, and/or whenever a value needs to be reset to nothingness. */
extern void		value_clear(PValue *v);

/* Store a new value in the given value. Replaces any previous value of the same type. */
extern int		value_set(PValue *v, PValueType type, ...);

/* Check if the indicated value is present in the value. A present value is never returned from cache. */
extern boolean		value_type_present(const PValue *v, PValueType type);

/* Get a value, reading from (and updating) the optional cache if the source value does
 * not hold the desired type. Data ownership is always in the value or the cache.
 * */
extern boolean		value_get_boolean(const PValue *v, PValue *cache);
extern int32		value_get_int32(const PValue *v, PValue *cache);
extern uint32		value_get_uint32(const PValue *v, PValue *cache);
extern real32		value_get_real32(const PValue *v, PValue *cache);
extern const real32 *	value_get_real32_vec2(const PValue *v, PValue *cache);
extern const real32 *	value_get_real32_vec3(const PValue *v, PValue *cache);
extern const real32 *	value_get_real32_vec4(const PValue *v, PValue *cache);
extern const real32 *	value_get_real32_mat16(const PValue *v, PValue *cache);
extern real64		value_get_real64(const PValue *v, PValue *cache);
extern const real64 *	value_get_real64_vec2(const PValue *v, PValue *cache);
extern const real64 *	value_get_real64_vec3(const PValue *v, PValue *cache);
extern const real64 *	value_get_real64_vec4(const PValue *v, PValue *cache);
extern const real64 *	value_get_real64_mat16(const PValue *v, PValue *cache);
extern const char *	value_get_string(const PValue *v, PValue *cache);
