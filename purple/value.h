/*
 * 
*/

typedef enum
{
	P_VALUE_NONE = -1,
	P_VALUE_BOOLEAN = 0,
	P_VALUE_INT32,
	P_VALUE_UINT32,
	P_VALUE_REAL32,
	P_VALUE_REAL32_VEC2,
	P_VALUE_REAL32_VEC3,
	P_VALUE_REAL32_VEC4,
	P_VALUE_REAL32_MAT16,
	P_VALUE_REAL64,
	P_VALUE_REAL64_VEC2,
	P_VALUE_REAL64_VEC3,
	P_VALUE_REAL64_VEC4,
	P_VALUE_REAL64_MAT16,
	P_VALUE_STRING,
	P_VALUE_MODULE
} PValueType;

/* This declaration is public mainly so other code can efficiently allocate
 * vectors of values. Always use the value-API to manipulate.
*/
typedef struct
{
	PValueType	type;
	union
	{
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
	}		v;
} PValue;

/* Input port. Nothing but a value container. */
typedef struct
{
	PValue	simple;
} PPInput;

/* Output port. Must look as an input port in the beginning. */
typedef struct
{
	PValue	simple;		/* "Knows" when it's not set. */
	List	*nodes;		/* Or whatever, List for simplicity only. */
} PPOutput;

extern const char *	value_type_to_name(PValueType type);
extern PValueType	value_type_from_name(const char *name);

/* Use right after allocation of a new PValue, to make sure it's sane. */
extern void		value_init(PValue *v);
/* Use before deallocation, and/or whenever a value needs to be reset to nothingness. */
extern void		value_clear(PValue *v);

/* Change the value of a value. Profound. */
extern void		value_set(PValue *v, PValueType type, ...);

/* Return the type of the value present in <v>. This is called "peeking" to avoid confusion
 * with the "get" functions below, which actually return the value.
*/
extern PValueType	value_peek_type(const PValue *v);

/* Interpret value as given type, and return interpretation. Does *not* simply convert the value,
 * since a single value might be read as a different type by several users. This is why non-scalars
 * (vectors, matrices and strings) need user-supplied buffer space. This buffer space *might not* be
 * actually used, it is only needed when the types mismatch. The function's return value is always
 * valid, and no memory ownership changes take place across any of these calls.
*/
extern boolean		value_get_boolean(const PValue *v);
extern int32		value_get_int32(const PValue *v);
extern uint32		value_get_uint32(const PValue *v);
extern real32		value_get_real32(const PValue *v);
extern const real32 *	value_get_real32_vec2(const PValue *v, real32 *buf);
extern const real32 *	value_get_real32_vec3(const PValue *v, real32 *buf);
extern const real32 *	value_get_real32_vec4(const PValue *v, real32 *buf);
extern const real32 *	value_get_real32_mat16(const PValue *v, real32 *buf);
extern real64		value_get_real64(const PValue *v);
extern const real64 *	value_get_real64_vec2(const PValue *v, real64 *buf);
extern const real64 *	value_get_real64_vec3(const PValue *v, real64 *buf);
extern const real64 *	value_get_real64_vec4(const PValue *v, real64 *buf);
extern const real64 *	value_get_real64_mat16(const PValue *v, real64 *buf);
extern const char *	value_get_string(const PValue *v, char *buf, size_t buf_max);
