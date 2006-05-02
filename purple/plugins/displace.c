/*
 * Simplistic displacement mapping.
*/

#define _USE_MATH_DEFINES
#include <math.h>

#include <stdio.h>

#include "purple.h"

/* --------------------------------------------------------------------------------------------- */

static void project_2d(real64 *flat, real64 *point)
{
	flat[0] = point[0];
	flat[1] = point[2];	/* Make Y = Z. */
}

/* Returns polygon normal, from computing the cross product of the first two edge vectors. */
static void polygon_normal(real64 *norm, const PNGLayer *vtx, uint32 v0, uint32 v1, uint32 v2, uint32 v3)
{
	real64	p0[3], p1[3], p2[3], e0[3], e1[3], f;

	/* Retreive first three corners. Assume quads are flat, and ignore fourth corner. */
	p_node_g_vertex_get_xyz(vtx, v0, p0, p0 + 1, p0 + 2);
	p_node_g_vertex_get_xyz(vtx, v1, p1, p1 + 1, p1 + 2);
	p_node_g_vertex_get_xyz(vtx, v2, p2, p2 + 1, p2 + 2);

	/* Form "edge vectors", i.e. vectors from v0 to v1 and from v0 to v2. */
	e0[0] = p1[0] - p0[0];
	e0[1] = p1[1] - p0[1];
	e0[2] = p1[2] - p0[2];
	e1[0] = p1[0] - p2[0];
	e1[1] = p1[1] - p2[1];
	e1[2] = p1[2] - p2[2];

	/* Now we need to compute cross product of e0 and e1. */
	norm[0] = e0[1] * e1[2] - e0[2] * e1[1];
	norm[1] = e0[2] * e1[0] - e0[0] * e1[2];
	norm[2] = e0[0] * e1[1] - e0[1] * e1[0];

	/* Compute normalizing factor. */
	f = 1.0 / sqrt(norm[0] * norm[0] + norm[1] * norm[1] + norm[2] * norm[2]);
	/* And apply it. */
	norm[0] *= f;
	norm[1] *= f;
	norm[2] *= f;
}

/* Create buffer holding the proper weighted normal for each vertex of <vertex>. */
static real64 * normal_buffer(const PNGLayer *vertex, const PNGLayer *polygon)
{
	size_t	numv, nump, i;
	real64	*norm, pn[3], vtx[3], *p, f;

	numv = p_node_g_layer_get_size(vertex);
	norm = malloc(numv * 3 * sizeof *norm);
	if(norm == NULL)
	{
		printf("displace: Couldn't allocate %u bytes for %u-vertex normal buffer\n", numv * 3 * sizeof *norm, numv);
		return NULL;
	}
	/* Clear every normal to (0,0,0). */
	for(i = 0; i < 3 * numv; i++)
		norm[i] = 0.0;

	/* Go through all *polygons*, and compute normals as they occur, adding to the right vertex bucket(s). */
	nump = p_node_g_layer_get_size(polygon);
	for(i = 0; i < nump; i++)
	{
		uint32	v0, v1, v2, v3;

		p_node_g_polygon_get_corner_uint32(polygon, i, &v0, &v1, &v2, &v3);
		polygon_normal(pn, vertex, v0, v1, v2, v3);
		if(v0 == ~0u || v1 == ~0u || v2 == ~0u)
			continue;
		norm[3 * v0 + 0] += pn[0];
		norm[3 * v0 + 1] += pn[1];
		norm[3 * v0 + 2] += pn[2];
		norm[3 * v1 + 0] += pn[0];
		norm[3 * v1 + 1] += pn[1];
		norm[3 * v1 + 2] += pn[2];
		norm[3 * v2 + 0] += pn[0];
		norm[3 * v2 + 1] += pn[1];
		norm[3 * v2 + 2] += pn[2];
		if(v3 != ~0u)
		{
			norm[3 * v3 + 0] += pn[0];
			norm[3 * v3 + 1] += pn[1];
			norm[3 * v3 + 2] += pn[2];
		}
	}
	/* Finally, go through all to-be normals and ... normalize them. */
	for(i = 0, p = norm; i < numv; i++, p += 3)
	{
		f = 1.0 / sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
		p[0] *= f;
		p[1] *= f;
		p[2] *= f;
	}
	return norm;
}

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode		*in = NULL, *inobj = NULL, *ingeo = NULL, *inbm = NULL;
	PONode		*obj, *geo;
	size_t		i, size;
	real64		min[2], max[2], point[3], flat[2], v, scale, xr, yr;
	PNGLayer	*inlayer, *outlayer, *inpoly;
	const uint8	*pixel;
	uint16		dim[3], x, y;

	/* Look up first incoming object that also has a geometry link. */
	for(i = 0; (in = p_input_node_nth(input[0], i)) != NULL; i++)
	{
		PNGLayer *inlayer, *outlayer;

		if(p_node_get_type(in) != V_NT_OBJECT)
			continue;
		inobj = in;
		ingeo = p_node_o_link_get(in, "geometry", 0);
		if(p_node_get_type(ingeo) != V_NT_GEOMETRY)
			continue;
		break;
	}
	/* Iterate second input's nodes, this time looking for bitmap. */
	for(i = 0; (in = p_input_node_nth(input[1], i)) != NULL; i++)
	{
		if(p_node_get_type(in) == V_NT_BITMAP)
		{
			inbm = in;
			break;
		}
	}
	if(inobj == NULL || inbm == NULL || ingeo == NULL)
	{
		printf("displace: aborting, obj=%p geo=%p bm=%p\n", inobj, ingeo, inbm);
		return P_COMPUTE_DONE;
	}

	scale = p_input_real64(input[2]);
	if(scale == 0.0)
	{
		printf("displace: aborting, bscale is zero, little point in that\n");
		return P_COMPUTE_DONE;
	}

	obj = p_output_node_copy(output, inobj, 0);
	geo = p_output_node_copy(output, ingeo, 1);
	p_node_o_link_set(obj, geo, "geometry", 0);

	inlayer  = p_node_g_layer_find(ingeo, "vertex");
	inpoly   = p_node_g_layer_find(ingeo, "polygon");
	outlayer = p_node_g_layer_find(geo, "vertex");
	size     = p_node_g_layer_get_size(inlayer);		/* Safely handles NULL layer. */

	/* Compute projected bounding box. */
	min[0] = min[1] = 1E300;
	max[0] = max[1] = -1E300;
	for(i = 0; i < size; i++)
	{
		p_node_g_vertex_get_xyz(inlayer, i, point, point + 1, point + 2);
		project_2d(flat, point);
		if(flat[0] < min[0])
			min[0] = flat[0];
		if(flat[0] > max[0])
			max[0] = flat[0];
		if(flat[1] < min[1])
			min[1] = flat[1];
		if(flat[1] > max[1])
			max[1] = flat[1];
	}
	/* Compute X and Y ranges. */
	xr = max[0] - min[0];
	yr = max[1] - min[1];
/*	printf("displace: projected geometry range is (%g,%g)-(%g,%g) -> xr=%g yr=%g\n", min[0], min[1], max[0], max[1], xr, yr);*/

	/* Compute new vertex positions for all vertices, and set in output. */
	if((pixel = p_node_b_layer_read_multi_begin(inbm, VN_B_LAYER_UINT8, "color_r", NULL)) != NULL)
	{
		real64	*norm;

		p_node_b_get_dimensions(inbm, dim, dim + 1, dim + 2);
		printf("displace: computing for %ux%u bitmap, and %u vertices\n", dim[0], dim[1], size);
		if((norm = normal_buffer(inlayer, inpoly)) != NULL)
		{
			for(i = 0; i < size; i++)
			{
				p_node_g_vertex_get_xyz(inlayer, i, point, point + 1, point + 2);
				project_2d(flat, point);
				flat[0] = (flat[0] - min[0]) / xr;		/* Convert to UV space. */
				flat[1] = (flat[1] - min[1]) / yr;
				x = flat[0] * (dim[0] - 1);			/* Compute integer UV coordinates. */
				y = flat[1] * (dim[1] - 1);
				v = pixel[y * dim[0] + x] / 255.0;		/* Read out pixel, and scale it to [0,1] range. */
				p_node_g_vertex_set_xyz(outlayer, i,		/* Apply displacement. */
							point[0] + norm[3 * i + 0] * scale * v,
							point[1] + norm[3 * i + 1] * scale * v,
							point[2] + norm[3 * i + 2] * scale * v);
			}
			free(norm);
		}
		p_node_b_layer_read_multi_end(inbm, pixel);
	}
	else
		printf("displace: couldn't access bitmap for reading\n");
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("displace");
	p_init_input(0, P_VALUE_MODULE, "object", P_INPUT_REQUIRED,
		     P_INPUT_DESC("The first object with a geometry link will have its geometry displaced."), P_INPUT_DONE);
	p_init_input(1, P_VALUE_MODULE, "map", P_INPUT_REQUIRED,
		     P_INPUT_DESC("The first bitmap node will be used as the displacement map."), P_INPUT_DONE);
	p_init_input(2, P_VALUE_REAL64, "scale", P_INPUT_DEFAULT(1.0), P_INPUT_MIN(0.01),
		     P_INPUT_DESC("Displacement scale factor."), P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("copyright", "2005 PDC, KTH");
	p_init_meta("desc/purpose", "Displaces geometry, using a bitmap as a displacement map.");
	p_init_compute(compute);
}
