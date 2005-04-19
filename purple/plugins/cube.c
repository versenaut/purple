/*
 * This plug-in creates a two-node box, using inputs to set side length and tesselation.
*/

#include "purple.h"
#include "purple-plugin.h"

/* Compute a vertex on a tesselated cube's surface. */
static void cube_vertex_compute(real64 *vtx, real32 size, uint32 splits, uint32 x, uint32 y, uint32 z)
{
	real32	d = size / splits, half = 0.5 * size;

	vtx[0] = -half + x * d;
	vtx[1] =  /*half*/size - y * d;
	vtx[2] = -half + z * d;
}

/* This gets called whenever the input, the size, changes. Create a cube with the given side length. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	enum { MY_OBJECT, MY_GEOMETRY };			/* Node labels. */
	PONode		*obj, *geo;
	real32		size = p_input_real32(input[0]);	/* Read out the size. */;
	uint32		splits = p_input_uint32(input[1]);	/* And the tesselation level. */
	real64		vtx[3];
	int		x, y, z, vid, poly, n, home, row, step;
	uint32		v0, v1, v2, v3;
	PNGLayer	*lay;

	obj = p_output_node_create(output, V_NT_OBJECT, MY_OBJECT);
	geo = p_output_node_create(output, V_NT_GEOMETRY, MY_GEOMETRY);

	p_node_o_link_set(obj, geo, "geometry", 0);

	/* Create topside polygons. Vertex references will dangle (for a while), but that's OK. */
	lay = p_node_g_layer_find(geo, "polygon");
	for(y = poly = 0; y < splits; y++)
	{
		for(x = 0; x < splits; x++, poly++)
		{
			n = y * (splits + 1) + x;
			p_node_g_polygon_set_corner_uint32(lay, poly, n, n + 1, n + (splits + 1) + 1, n + (splits + 1));
		}
	}

	/* Create bottom. */
	home = 6 * (splits * splits) + 2 - ((splits + 1) * (splits + 1));
	row  = splits + 1;
	for(y = 0; y < splits; y++)
	{
		for(x = 0; x < splits; x++, poly++)
			p_node_g_polygon_set_corner_uint32(lay, poly, home + y * row + x + 1, home + y * row + x, home + (y + 1) * row + x, home + (y + 1) * row + 1 + x);
	}

	/* Create front (z=+) side. */
	/*  First the ones connected to the top face. */
	row  = 4 * splits;
	home = (splits + 1) * (splits + 1) - (splits + 1);
	for(x = 0; x < splits; x++, poly++)
		p_node_g_polygon_set_corner_uint32(lay, poly, home + x, home + x + 1, home + x + 1 + row, home + x + row);
	/*  Then, the intermediate quads, if any. These are not connected either the top or bottom faces. */
	if(splits > 2)
	{
		row  = 4 * splits;
		home = (splits + 1) * (splits + 1) + row - (splits + 1);
		for(y = 0; y < splits - 2; y++, home += row)
		{
			for(x = 0; x < splits; x++, poly++)
				p_node_g_polygon_set_corner_uint32(lay, poly, home + x, home + x + 1, home + x + 1 + row, home + x + row);
		}
	}
	/*  Last, the quads connected to the bottom face. */
	home = 6 * (splits * splits) + 2 - (splits + 1);
	row  = (splits + 1) * (splits + 1);
	for(x = 0; x < splits; x++, poly++)
		p_node_g_polygon_set_corner_uint32(lay, poly, home + x + 1, home + x, home + x - row, home + x + 1 - row);

	/* Create back (z=-) side. */
	/*  First the quads connected to the top face, as usual. */
	row  = (splits + 1 ) * (splits + 1);
	home = 0;
	for(x = 0; x < splits; x++, poly++)
		p_node_g_polygon_set_corner_uint32(lay, poly, home + x, home + x + row, home + x + row + 1, home + x + 1);
	/*  Then, the intermediate quads, if any. Not connected to either top or bottom faces. */
	if(splits > 2)
	{
		row  = 4 * splits;
		home = (splits + 1) * (splits + 1);
		for(y = 0; y < splits - 2; y++, home += row)
		{
			for(x = 0; x < splits; x++, poly++)
				p_node_g_polygon_set_corner_uint32(lay, poly, home + x + 1, home + x, home + x + row, home + x + 1 + row);
		}
	}
	/*  Last, the quads connected to the bottom face. */
	home = 6 * (splits * splits) + 2 - ((splits + 1) * (splits + 1));
	for(x = 0; x < splits; x++, poly++)
		p_node_g_polygon_set_corner_uint32(lay, poly,  home + x, home + x + 1, home - row + x + 1, home - row + x);

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
				v0 = v1;
				v3 = v2;
				v1 += 2;
				v2 += 2;
			}
		}
	}
	/*  Finally, set the bottom row. */
	v0 = (splits + 1) * (splits + 1) + (splits - 2) * (2 * (splits + 1) + (2 * (splits - 1)));
	v1 = v0 + (splits + 1);
	v2 = v1 + 2 * (splits - 1) + 2 * (splits + 1);
	v3 = v2 - (splits + 1);
	for(x = 0; x < splits; x++, poly++)
	{
		p_node_g_polygon_set_corner_uint32(lay, poly, v0, v1, v2, v3);
		v0 = v1;
		v3 = v2;
		v1 += 2;
		v2 += (splits + 1);
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
				v0 = v1;
				v3 = v2;
				v1 -= 2;
				v2 -= 2;
			}
		}
	}
	/*  Last, the final bottom row. */
	v0 = (splits + 1) * (splits + 1) - 1 + (splits - 1) * (2 * (splits + 1) + 2 * (splits - 1));
	v1 = v0 - (splits + 1);
	v3 = v0 + (splits + 1) * (splits + 1);
	v2 = v3 - (splits + 1);
	for(x = 0; x < splits; x++, poly++)
	{
		p_node_g_polygon_set_corner_uint32(lay, poly, v0, v1, v2, v3);
		v0 = v1;
		v3 = v2;
		v1 -= 2;
		v2 -= (splits + 1);
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

	return P_COMPUTE_DONE;	/* Sleep until size changes. */
}

void init(void)
{
	p_init_create("cube");
	p_init_input(0, P_VALUE_REAL32, "size", P_INPUT_REQUIRED, P_INPUT_MIN(0.1), P_INPUT_MAX(200.0), P_INPUT_DEFAULT(10.0), P_INPUT_DONE);
	p_init_input(1, P_VALUE_UINT32, "splits", P_INPUT_REQUIRED, P_INPUT_MIN(1), P_INPUT_MAX(100), P_INPUT_DEFAULT(1), P_INPUT_DONE);
	p_init_compute(compute);
}
