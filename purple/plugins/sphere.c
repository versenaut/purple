/*
 * This plug-in creates a sphere, using inputs to set tesselation and height.
*/

#define _USE_MATH_DEFINES
#include <math.h>

#include "purple.h"

#define	POLY(lab,l,i,v0,v1,v2,v3)	do { /*printf(lab " polygon %u: %u-%u-%u-%u\n", i, v0, v1, v2, v3); */p_node_g_polygon_set_corner_uint32(l,i,v0,v1,v2,v3); } while(0)

static void compute_quad_uv(real64 *u, real64 *v, unsigned int x, unsigned int y, uint32 end_splits, uint32 side_splits)
{
	u[0] = (real64) x / end_splits;
	u[1] = u[0];
	u[2] = (real64) (x + 1) / end_splits;
	u[3] = u[2];

	if(v != NULL)
	{
		/* Invert the V coordinates, since Verse bitmaps have their origin in the top left corner. */
		v[0] = 1.0 - (0.5 + 0.5 * sin(-M_PI/2 + M_PI * y / side_splits));
		v[1] = 1.0 - (0.5 + 0.5 * sin(-M_PI/2 + M_PI * (y + 1) / side_splits));
		v[2] = v[1];
		v[3] = v[0];
	}
}

/* This gets called whenever the input, the size, changes. Create a cube with the given side length. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	enum { MY_OBJECT, MY_GEOMETRY };			/* Node labels. */
	PONode		*obj, *geo;
	real32		radius = p_input_real32(input[0]);	/* Read out the radius. */
	uint32		end_splits = p_input_uint32(input[1]);	/* And the top/bottom tesselation level. */
	uint32		side_splits = p_input_uint32(input[2]);	/* And the side tesselation level. */
	uint32		quad_levels;
	boolean		uv_map = p_input_boolean(input[3]);
	real64		angle, y, r;
	uint32		i, j, pos, bc, tc;
	PNGLayer	*lay, *ulay, *vlay;

	if(side_splits < 2)
		return P_COMPUTE_DONE;

	obj = p_output_node_create(output, V_NT_OBJECT, MY_OBJECT);
	p_node_set_name(obj, "sphere");
	geo = p_output_node_create(output, V_NT_GEOMETRY, MY_GEOMETRY);
	p_node_set_name(geo, "sphere-geo");

	p_node_o_link_set(obj, geo, "geometry", 0);

	quad_levels = side_splits - 2;	/* Top and bottom layers are triangles. */

	/* Create outer vertices. */
	lay = p_node_g_layer_find(geo, "vertex");
	for(j = 0; j <= quad_levels; j++)
	{
		y = radius * sin(-M_PI/2 + M_PI * (j + 1) / side_splits);
		r = radius * cos(-M_PI/2 + M_PI * (j + 1) / side_splits);
		for(i = 0; i < end_splits; i++)
		{
			angle = 2 * M_PI * (i / (real64) end_splits);
			p_node_g_vertex_set_xyz(lay, j * end_splits + i, r * cos(angle), y, r * -sin(angle));
		}
	}
	bc = (quad_levels + 1) * end_splits;
	p_node_g_vertex_set_xyz(lay, bc, 0.0, -radius, 0.0);
	tc = bc + 1;
	p_node_g_vertex_set_xyz(lay, tc, 0.0, radius,  0.0);

	lay = p_node_g_layer_find(geo, "polygon");
	/* Create bottom triangles. */
	for(i = 0; i < end_splits; i++)
		POLY("bottom triangle", lay, i, i, (i + 1) % end_splits, bc, ~0u);
	/* Create intermediary quads, if any. */
	for(i = 0; i < quad_levels; i++)
	{
		pos = i * end_splits;
		for(j = 0; j < end_splits; j++)
		{
			POLY("side quad", lay, end_splits + pos + j,
							   pos + j,
							   pos + j + end_splits,
							   pos + (j + 1) % end_splits + end_splits,
							   pos + (j + 1) % end_splits);
		}
	}
	/* Create top triangles. */
	for(i = 0; i < end_splits; i++)
		POLY("top triangle", lay, (quad_levels + 1) * end_splits + i,
						   tc - 2 - end_splits + (i + 1) % end_splits,
						   tc - 2 - end_splits + i,
						   tc, ~0u);

	if(uv_map)
	{
		uint32	poly;
		real64	u[4], v[4];

		ulay = p_node_g_layer_create(geo, "map_u", VN_G_LAYER_POLYGON_CORNER_REAL, 0, 0.0f);
		vlay = p_node_g_layer_create(geo, "map_v", VN_G_LAYER_POLYGON_CORNER_REAL, 0, 0.0f);

		/* Texture-map the bottom polygons, the ones touching the "south pole". */
		for(i = poly = 0; i < end_splits; i++, poly++)
		{
			compute_quad_uv(u, v, i, 0, end_splits, side_splits);
			p_node_g_polygon_set_corner_real64(ulay, poly,
							   u[0],
							   u[1],
							   0.5, 0.0);
			p_node_g_polygon_set_corner_real64(vlay, poly,	/* V values are the same for every triangle. Flipped, though. */
							   v[1], v[1],
							   1.0, 0.0);
		}
		/* Texture map the main polygons, the ones not touching a pole vertex. */
		for(j = 0; j < quad_levels; j++)
		{
			for(i = 0; i < end_splits; i++, poly++)
			{
				compute_quad_uv(u, v, i, j + 1, end_splits, side_splits);
				p_node_g_polygon_set_corner_real64(ulay, poly, u[0], u[1], u[2], u[3]);
				p_node_g_polygon_set_corner_real64(vlay, poly, v[0], v[1], v[2], v[3]);
			}
		}
		/* Texture-map the top polygons, the ones touching the "north pole". */
		for(i = 0; i < end_splits; i++, poly++)
		{
			compute_quad_uv(u, v, i, side_splits, end_splits, side_splits);	/* Works for tris to, if you're careful. */
			p_node_g_polygon_set_corner_real64(ulay, poly, u[0], u[1], u[2], u[3]);
			p_node_g_polygon_set_corner_real64(vlay, poly, v[1], v[1], 0.0, 0.0);
		}
	}

	p_node_g_crease_set_vertex(geo, NULL, ~0u);
	p_node_g_crease_set_edge(geo, NULL, ~0u);

	return P_COMPUTE_DONE;	/* Sleep until size changes. */
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("sphere");
	p_init_input(0, P_VALUE_REAL32, "radius",      P_INPUT_REQUIRED, P_INPUT_MIN(0.1), P_INPUT_MAX(200.0), P_INPUT_DEFAULT(1.0),
		     P_INPUT_DESC("The radius of the sphere."), P_INPUT_DONE);
	p_init_input(1, P_VALUE_UINT32, "end splits",  P_INPUT_REQUIRED, P_INPUT_MIN(3),   P_INPUT_MAX(128),   P_INPUT_DEFAULT(8),
		     P_INPUT_DESC("The number of polygons sitting side-to-side around the north/south axis of the sphere."), P_INPUT_DONE);
	p_init_input(2, P_VALUE_UINT32, "side splits", P_INPUT_REQUIRED, P_INPUT_MIN(2),   P_INPUT_MAX(128),   P_INPUT_DEFAULT(3),
		     P_INPUT_DESC("The number of polygons stacked on top of each other, along the north/south axis of the sphere."), P_INPUT_DONE);
	p_init_input(3, P_VALUE_BOOLEAN, "uv map", P_INPUT_DEFAULT(0),
		     P_INPUT_DESC("If true, UV mapping layers will be added. Currently only naive mapping is supported, the "
				  "texture should have width:height aspect of 2:1, and quite a bit of it will be wasted."), P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("desc/purpose", "Creates a polygonal mesh representation of a sphere. Lets you control how finely the mesh should be "
		    "tesselated along two axis.");
	p_init_compute(compute);
}
