/*
 * This plug-in creates a two-node box, using an input to set side length
*/

typedef struct
{
	PONode	*obj;	/* The nodes we created. Kept around for duration of plug-in. */
	PONode	*geo;
} State;

/* This gets called whenever the input, the size, changes. Create a cube with the given side length. */
static void compute(PPInput *input, PPOutput output, void *state_untyped)
{
	State	*state = state_untyped;
	real32	size = p_input_real32(input[0]);	/* Read out the size. */;
	int	i;
	/* Data used to describe corners of a unit cube. Scaled with size before set. */
	static const int8	corner[][3] = {
		{ -1,  1, -1 }, { 1,  1, -1 }, { 1,  1, 1 }, { -1,  1, 1 },
		{ -1, -1, -1 }, { 1, -1, -1 }, { 1, -1, 1 }, { -1, -1, 1 }
	};

	if(state->obj == NULL)	/* No nodes created yet? Then it's certainly time to do so. */
	{
		state->obj = p_output_node_create(output, P_NODE_OBJECT, "cube");
		state->geo = p_output_node_create(output, P_NODE_GEOMETRY, "cube-geo");
		p_node_o_link_set(state->obj, state->geo, "geometry");
		/* Create the polygons. Vertex references will dangle (for a while), but that's OK. */
		p_node_g_polygon_set_corner_uint32(state->geo, 1, 0,  0, 1, 2, 3);
		p_node_g_polygon_set_corner_uint32(state->geo, 1, 1,  3, 2, 6, 7);
		p_node_g_polygon_set_corner_uint32(state->geo, 1, 2,  7, 6, 5, 4);
		p_node_g_polygon_set_corner_uint32(state->geo, 1, 3,  1, 0, 4, 5);
		p_node_g_polygon_set_corner_uint32(state->geo, 1, 4,  0, 3, 7, 4);
		p_node_g_polygon_set_corner_uint32(state->geo, 1, 5,  2, 1, 5, 6);
	}
	else
		p_output_node_again(state->geo);	/* Should this be implicit? That's hard... */

	/* Loop and set eight scaled corners. */
	for(i = 0; i < 8; i++)
		p_node_g_vertex_set_xyz(state->geo, "vertex", corner,
					size * corner[i][0], size * corner[i][1], size * corner[i][2]);

	return P_COMPUTE_DONE;	/* Sleep until size changes. */
}

static void ctor(void *state_untyped)
{
	State	*state = state_untyped;

	state->obj = NULL;
	state->geo = NULL;
}

void init(void)
{
	p_init_create("cube");
	p_init_state(sizeof (State), ctor, NULL);
	p_init_input(0, P_VALUE_REAL32, "size", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
