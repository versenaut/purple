/*
 * Simplistic displacement mapping.
*/

#define _USE_MATH_DEFINES
#include <math.h>

#include "purple.h"

/* --------------------------------------------------------------------------------------------- */

static void project_2d(real64 *flat, real64 *point)
{
	flat[0] = point[0];
	flat[1] = point[2];	/* Make Y = Z. */
}

static void polygon_normal(real64 *norm, const PNGLayer *vtx, uint32 v0, uint32 v1, uint32 v2, uint32 v3)
{
	real64	p0[3], p1[3], p2[3], e0[3], e1[3], cp[3], n;

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
	cp[0] = e0[1] * e1[2] - e0[2] * e1[1];
	cp[1] = e0[2] * e1[0] - e0[0] * e1[2];
	cp[2] = e0[0] * e1[1] - e0[1] * e1[0];

	/* All we need to do now is normalize the ... normal, and return it. */
	n = sqrt(cp[0] * cp[0] + cp[1] * cp[1] + cp[2] * cp[2]);

	norm[0] = cp[0] / n;
	norm[1] = cp[1] / n;
	norm[2] = cp[2] / n;
}

static void vertex_normal(real64 *norm, uint32 index, const PNGLayer *vtx, const PNGLayer *poly)
{
	size_t	num_vtx, num_poly;
	uint32	i, cnt = 0;

	norm[0] = norm[1] = norm[2] = 0.0;

	/* Look for given vertex in polygon definitions. */
	num_poly = p_node_g_layer_get_size(poly);
	for(i = 0; i < num_poly; i++)
	{
		uint32	v0, v1, v2, v3;

		p_node_g_polygon_get_corner_uint32(poly, i, &v0, &v1, &v2, &v3);
		if(v0 == index || v1 == index || v2 == index || v3 == index)
		{
			real64	here[3];
			polygon_normal(here, vtx, v0, v1, v2, v3);
			norm[0] += here[0];
			norm[1] += here[1];
			norm[2] += here[2];
			cnt++;
			break;
		}
	}
	if(cnt > 0)
	{
		norm[0] /= cnt;
		norm[1] /= cnt;
		norm[2] /= cnt;
	}
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
	if((pixel = p_node_b_layer_read_multi_begin(inbm, VN_B_LAYER_UINT8, "col_r", NULL)) != NULL)
	{
		p_node_b_get_dimensions(inbm, dim, dim + 1, dim + 2);
		printf("displace: computing for %ux%u bitmap, and %u vertices\n", dim[0], dim[1], size);
		for(i = 0; i < size; i++)
		{
			real64	norm[3];
			p_node_g_vertex_get_xyz(inlayer, i, point, point + 1, point + 2);
			project_2d(flat, point);
			flat[0] = (flat[0] - min[0]) / xr;		/* Convert to UV space. */
			flat[1] = (flat[1] - min[1]) / yr;
			x = flat[0] * (dim[0] - 1);			/* Compute integer UV coordinate. */
			y = flat[1] * (dim[1] - 1);
			v = pixel[y * dim[0] + x] / 255.0;		/* Read out pixel, and scale it to [0,1] range. */
			vertex_normal(norm, i, inlayer, inpoly);	/* Compute normal. */
			p_node_g_vertex_set_xyz(outlayer, i, point[0] + norm[0] * scale * v, point[1] + norm[1] * scale * v, point[2] + norm[2] * scale * v);
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
		     P_INPUT_DESC("Displacement scale factor."));
	p_init_meta("authors", "Emil Brink");
	p_init_meta("copyright", "2005 PDC, KTH");
	p_init_meta("desc/purpose", "Displaces geometry, using a bitmap as a displacement map.");
	p_init_compute(compute);
}
