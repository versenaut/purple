/*
 * This plug-in creates a two-node box, using inputs to set side length and tesselation.
*/

#include <stdio.h>

#include "purple.h"

typedef enum { TOP, BOTTOM, FRONT, BACK, LEFT, RIGHT } Face;

/* Compute a vertex on a tesselated cube's surface. */
static void cube_vertex_compute(real64 *vtx, const real64 *size, uint32 splits, uint32 x, uint32 y, uint32 z)
{
	vtx[0] = -0.5 * size[0] + x * (size[0] / splits);
	vtx[1] =  /*half*/size[1] - y * (size[1] / splits);
	vtx[2] = -0.5 * size[2] + z * (size[2] / splits);

	printf("cube XYZ for (%u,%u,%u): (%g,%g,%g)\n", x, y, z, vtx[0], vtx[1], vtx[2]);
}

static void cube_uvmap_compute(uint32 poly, PNGLayer *ulay, PNGLayer *vlay, Face face, uint32 splits, uint32 x, uint32 y)
{
	real64	u[4], v[4], n = 1.0 / splits;

	if(ulay == NULL || vlay == NULL)
		return;
	switch(face)
	{
	case TOP:
	case BOTTOM:
	case FRONT:
	case BACK:
	case LEFT:
	case RIGHT:
		u[0] = x * n;
		u[1] = (x + 1) * n;
		u[2] = u[1];
		u[3] = u[0];
		v[0] = (y + 1) * n;
		v[1] = v[0];
		v[2] = y * n;
		v[3] = v[2];
		break;
	default:
		printf("cube: Can't UV-map face %d -- not implemented\n", face);
		return;
	}
	printf("UV for (%u,%u) (%u): [(%g,%g),(%g,%g),(%g,%g),(%g,%g)] splits=%u n=%g\n", x, y, poly,
	       u[0], v[0],
	       u[1], v[1],
	       u[2], v[2],
	       u[3], v[3], splits, n);
	p_node_g_polygon_set_corner_real64(ulay, poly, u[0], u[1], u[2], u[3]);
	p_node_g_polygon_set_corner_real64(vlay, poly, v[0], v[1], v[2], v[3]);
}

/* This gets called whenever the input, the size, changes. Create a cube with the given side length. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	enum { MY_OBJECT, MY_GEOMETRY };			/* Node labels. */
	PONode		*obj, *geo;
	const real64	*size = p_input_real64_vec3(input[0]);	/* Read out the size. */
	uint32		splits = p_input_uint32(input[1]);	/* And the tesselation level. */
	boolean		uv_map = p_input_boolean(input[2]);	/* And whether an UV map should be created. */
	real64		vtx[3];
	int		x, y, z, vid, poly, n, home, row, step;
	uint32		v0, v1, v2, v3, crease = p_input_uint32(input[3]);
	PNGLayer	*lay, *ulay, *vlay;

	obj = p_output_node_create(output, V_NT_OBJECT, MY_OBJECT);
	p_node_set_name(obj, "cube");
	geo = p_output_node_create(output, V_NT_GEOMETRY, MY_GEOMETRY);
	p_node_set_name(obj, "cube-geo");

	p_node_o_link_set(obj, geo, "geometry", 0);

	if(uv_map)
	{
		ulay = p_node_g_layer_create(geo, "map_u", VN_G_LAYER_POLYGON_CORNER_REAL, 0, 0.0f);
		vlay = p_node_g_layer_create(geo, "map_v", VN_G_LAYER_POLYGON_CORNER_REAL, 0, 0.0f);
	}
	else
		ulay = vlay = NULL;

	/* Create topside polygons. Vertex references will dangle (for a while), but that's OK. */
	lay = p_node_g_layer_find(geo, "polygon");
	for(y = poly = 0; y < splits; y++)
	{
		for(x = 0; x < splits; x++, poly++)
		{
			n = y * (splits + 1) + x;
			p_node_g_polygon_set_corner_uint32(lay, poly, n, n + 1, n + (splits + 1) + 1, n + (splits + 1));
			cube_uvmap_compute(poly, ulay, vlay, TOP, splits, x, y);
		}
	}

	/* Create bottom. */
	home = 6 * (splits * splits) + 2 - ((splits + 1) * (splits + 1));
	row  = splits + 1;
	for(y = 0; y < splits; y++)
	{
		for(x = 0; x < splits; x++, poly++)
		{
			p_node_g_polygon_set_corner_uint32(lay, poly, home + y * row + x + 1, home + y * row + x, home + (y + 1) * row + x, home + (y + 1) * row + 1 + x);
			cube_uvmap_compute(poly, ulay, vlay, BOTTOM, splits, x, y);
		}
	}

	/* Create front (z=+) side. */
	/*  First the ones connected to the top face. */
	row  = 4 * splits;
	home = (splits + 1) * (splits + 1) - (splits + 1);
	for(x = 0; x < splits; x++, poly++)
	{
		p_node_g_polygon_set_corner_uint32(lay, poly, home + x, home + x + 1, home + x + 1 + row, home + x + row);
		cube_uvmap_compute(poly, ulay, vlay, FRONT, splits, x, 0);
	}
	/*  Then, the intermediate quads, if any. These are not connected either the top or bottom faces. */
	if(splits > 2)
	{
		row  = 4 * splits;
		home = (splits + 1) * (splits + 1) + row - (splits + 1);
		for(y = 0; y < splits - 2; y++, home += row)
		{
			for(x = 0; x < splits; x++, poly++)
			{
				p_node_g_polygon_set_corner_uint32(lay, poly, home + x, home + x + 1, home + x + 1 + row, home + x + row);
				cube_uvmap_compute(poly, ulay, vlay, FRONT, splits, x, 1 + y);
			}
		}
	}
	/*  Last, the quads connected to the bottom face. */
	if(splits > 1)
	{
		row  = (splits + 1) * (splits + 1);
		home = 6 * (splits * splits) + 2 - (splits + 1);
		for(x = 0; x < splits; x++, poly++)
		{
			p_node_g_polygon_set_corner_uint32(lay, poly, home + x + 1, home + x, home + x - row, home + x + 1 - row);
			cube_uvmap_compute(poly, ulay, vlay, FRONT, splits, x, splits - 1);
		}
	}

	/* Create back (z=-) side. */
	/*  First the quads connected to the top face, as usual. */
	row  = (splits + 1 ) * (splits + 1);
	home = 0;
	for(x = 0; x < splits; x++, poly++)
	{
		p_node_g_polygon_set_corner_uint32(lay, poly, home + x, home + x + row, home + x + row + 1, home + x + 1);
		cube_uvmap_compute(poly, ulay, vlay, BACK, splits, x, 0);
	}
	/*  Then, the intermediate quads, if any. Not connected to either top or bottom faces. */
	if(splits > 2)
	{
		row  = 4 * splits;
		home = (splits + 1) * (splits + 1);
		for(y = 0; y < splits - 2; y++, home += row)
		{
			for(x = 0; x < splits; x++, poly++)
			{
				p_node_g_polygon_set_corner_uint32(lay, poly, home + x + 1, home + x, home + x + row, home + x + 1 + row);
				cube_uvmap_compute(poly, ulay, vlay, BACK, splits, x, 1 + y);
			}
		}
	}
	/*  Last, the quads connected to the bottom face. */
	if(splits > 1)
	{
		row  = 2 * (splits + 1) + 2 * (splits - 1);
		home = 2 * (splits + 1) * (splits + 1) + (splits - 1) * (2 * (splits + 1) + 2 * (splits - 1)) - 1 - (splits + 1) * (splits + 1) + (splits + 1);
		for(x = 0; x < splits; x++, poly++)
		{
			p_node_g_polygon_set_corner_uint32(lay, poly,  home - x, home - x - row, home - x - row - 1, home - x - 1);
			cube_uvmap_compute(poly, ulay, vlay, BACK, splits, x, splits - 1);
		}
	}

	/* Create left (x=-) side. */
	/*  First the quads connected to the top face. */
	row  = (splits + 1) * (splits + 1);
	step = (splits + 1);
	v0 = 0;
	v1 = v0 + (splits + 1);
	v2 = splits + 1 + (splits + 1) * (splits + 1);
	v3 = v2 - (splits + 1);
	for(x = 0; x < splits; x++, poly++)
	{
		p_node_g_polygon_set_corner_uint32(lay, poly, v0, v1, v2, v3);
		cube_uvmap_compute(poly, ulay, vlay, LEFT, splits, x, 0);
		v0 = v1;
		v3 = v2;
		v1 += step;
		v2 += 2;
	}
	/*  Then any internal ones, not touching top and bottom edges. */
	if(splits > 2)
	{
		for(y = 0; y < splits - 2; y++)
		{
			v0 = (splits + 1) * (splits + 1) + y * (2 * (splits + 1) + (2 * (splits - 1)));
			v1 = v0 + splits + 1;
			v3 = v0 + (splits + 1) + 2 * (splits - 1) + (splits + 1);
			v2 = v3 + (splits + 1);
			for(x = 0; x < splits; x++, poly++)
			{
				p_node_g_polygon_set_corner_uint32(lay, poly, v0, v1, v2, v3);
				cube_uvmap_compute(poly, ulay, vlay, LEFT, splits, x, 1 + y);
				v0 = v1;
				v3 = v2;
				v1 += 2;
				v2 += 2;
			}
		}
	}
	/*  Finally, set the bottom row. */
	if(splits > 1)
	{
		v0 = (splits + 1) * (splits + 1) + (splits - 2) * (2 * (splits + 1) + (2 * (splits - 1)));
		v1 = v0 + (splits + 1);
		v2 = v1 + 2 * (splits - 1) + 2 * (splits + 1);
		v3 = v2 - (splits + 1);
		for(x = 0; x < splits; x++, poly++)
		{
			p_node_g_polygon_set_corner_uint32(lay, poly, v0, v1, v2, v3);
			cube_uvmap_compute(poly, ulay, vlay, LEFT, splits, x, splits - 1);
			v0 = v1;
			v3 = v2;
			v1 += 2;
			v2 += (splits + 1);
		}
	}

	/* Create right (x=+) side. */
	/*  First the quads connected to the top face, as usual. */
	v0 = (splits + 1) * (splits + 1) - 1;
	v1 = v0 - (splits + 1);
	v3 = v0 + 2 * (splits + 1) + 2 * (splits - 1);
	v2 = v3 - (splits + 1);
	for(x = 0; x < splits; x++, poly++)
	{
		p_node_g_polygon_set_corner_uint32(lay, poly, v0, v1, v2, v3);
		cube_uvmap_compute(poly, ulay, vlay, RIGHT, splits, x, 0);
		v0 = v1;
		v3 = v2;
		v1 -= (splits + 1);
		v2 -= 2;
	}
	/*  Then the internal ones, if any. */
	if(splits > 2)
	{
		for(y = 1; y < splits - 1; y++)
		{
			v0 = (splits + 1) * (splits + 1) - 1 + y * (2 * (splits + 1) + 2 * (splits - 1));
			v1 = v0 - (splits + 1);
			v3 = v0 + 2 * (splits + 1) + 2 * (splits - 1);
			v2 = v3 - (splits + 1);
			for(x = 0; x < splits; x++, poly++)
			{
				p_node_g_polygon_set_corner_uint32(lay, poly, v0, v1, v2, v3);
				cube_uvmap_compute(poly, ulay, vlay, RIGHT, splits, x, y);
				v0 = v1;
				v3 = v2;
				v1 -= 2;
				v2 -= 2;
			}
		}
	}
	/*  Last, the final bottom row. */
	if(splits > 1)
	{
		v0 = (splits + 1) * (splits + 1) - 1 + (splits - 1) * (2 * (splits + 1) + 2 * (splits - 1));
		v1 = v0 - (splits + 1);
		v3 = v0 + (splits + 1) * (splits + 1);
		v2 = v3 - (splits + 1);
		for(x = 0; x < splits; x++, poly++)
		{
			p_node_g_polygon_set_corner_uint32(lay, poly, v0, v1, v2, v3);
			cube_uvmap_compute(poly, ulay, vlay, RIGHT, splits, x, splits - 1);
			v0 = v1;
			v3 = v2;
			v1 -= 2;
			v2 -= (splits + 1);
		}
	}

	printf("Done, created %d polygons\n", poly);

	lay = p_node_g_layer_find(geo, "vertex");
	/* Create vertices. Compared with the polygons, this is really simple stuff. */
	for(y = 0, vid = 0; y <= splits; y++)
	{
		for(z = 0; z <= splits; z++)
		{
			for(x = 0; x <= splits; x++)
			{
				if(y > 0 && y < splits)	/* Not first or last "floor"? */
				{
					if((x > 0 && x < splits) && (z > 0 && z < splits))	/* Skip internal vertices. */
						continue;
				}
				cube_vertex_compute(vtx, size, splits, x, y, z);
				p_node_g_vertex_set_xyz(lay, vid++,  vtx[0], vtx[1], vtx[2]);
			}
		}
	}
	/* Set creases, we do want this cube to be ... cubistic. */
	if(crease == 1)		/* Defaults-mode? */
	{
		p_node_g_crease_set_vertex(geo, NULL, ~0u);
		p_node_g_crease_set_edge(geo, NULL, ~0u);
	}
	else if(crease == 2)	/* Full layers mode? */
	{
		PNGLayer	*vc, *ec;
		size_t		i, size;

		lay = p_node_g_layer_find(geo, "vertex");
		size = p_node_g_layer_get_size(lay);
		vc = p_node_g_layer_create(geo, "crease_vertex", VN_G_LAYER_VERTEX_UINT32, ~0u, 0.0);
		for(i = 0; i < size; i++)
			p_node_g_vertex_set_uint32(vc, i, ~0u);
		p_node_g_crease_set_vertex(geo, p_node_g_layer_get_name(vc), ~0u);
			
		lay = p_node_g_layer_find(geo, "polygon");
		size = p_node_g_layer_get_size(lay);
		ec = p_node_g_layer_create(geo, "crease_edge",   VN_G_LAYER_POLYGON_CORNER_UINT32, ~0u, 0.0);
		for(i = 0; i < size; i++)
			p_node_g_polygon_set_corner_uint32(ec, i, ~0u, ~0u, ~0u, ~0u);
		p_node_g_crease_set_edge(geo, p_node_g_layer_get_name(ec), ~0u);
	}
	return P_COMPUTE_DONE;	/* Sleep until size changes. */
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("cube");
	p_init_input(0, P_VALUE_REAL64_VEC3, "size", P_INPUT_REQUIRED, P_INPUT_MIN_VEC3(0.1, 0.1, 0.1), P_INPUT_MAX_VEC3(10.0, 10.0, 10.0), P_INPUT_DEFAULT_VEC3(1.0, 1.0, 1.0),
		     P_INPUT_DESC("The side lengths of the cube, in all three dimensions."), P_INPUT_DONE);
	p_init_input(1, P_VALUE_UINT32, "splits", P_INPUT_REQUIRED, P_INPUT_MIN(1), P_INPUT_MAX(100), P_INPUT_DEFAULT(1),
		     P_INPUT_DESC("The number of splits to do along each axis."), P_INPUT_DONE);
	p_init_input(2, P_VALUE_BOOLEAN, "uv-map", P_INPUT_DEFAULT(0),
		     P_INPUT_DESC("Controls whether or not UV mapping data is created."), P_INPUT_DONE);
	p_init_input(3, P_VALUE_UINT32, "crease", P_INPUT_DEFAULT(0),
		     P_INPUT_ENUM("0:None|1:Defaults|2:Full Layers"),
		     P_INPUT_DESC("Controls the creasing set for this cube. Without crease information, subdiving "
				  "renderers will make the cube very round."), P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("copyright", "2005 PDC, KTH");
	p_init_meta("desc/purpose", "Creates a cube object.");
	p_init_compute(compute);
}
