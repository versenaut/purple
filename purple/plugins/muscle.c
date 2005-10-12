/*
 * Something very simple. Overly ambitious name, but that's what you get. This is inspired
 * by an old idea/vision for Purple; that it should be able to measure the distance between
 * two points of e.g. an arm (one above and one below the elbow joint), and then compute and
 * perform a deformation of the upper part of the arm depending on the angle. This should
 * then simulate the flexing of a muscle.
 *
 * *This* implementation is not quite as flashy: it expects two separate objects, and just
 * uses the rotation part of the second to rescale the first, around the Z axis. It even
 * hardcodes various factors and assumptions used to go from that angle to a size.
 * 
 * FIXME: This should be scrapped, and replaced with something based on bones and a proper
 * way to indicate which bone/pair of points are to affect the deformation.
*/

#include "purple.h"

static void planar_scale(PONode *geo, real64 factor)
{
	PNGLayer	*xyz;
	unsigned int	i, size;
	real64		p[3];

	printf("rescaling by %g\n", factor);
	xyz = p_node_g_layer_find(geo, "vertex");
	if(xyz == NULL)
		return;
	size = p_node_g_layer_get_size(xyz);
	for(i = 0; i < size; i++)
	{
		p_node_g_vertex_get_xyz(xyz, i, p + 0, p + 1, p + 2);
		p[0] *= factor;
		p[1] *= factor;
		p_node_g_vertex_set_xyz(xyz, i, p[0], p[1], p[2]);
	}
}

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode	*upper, *lower, *ug, *lg;
	PONode	*outup, *outlo, *outug, *outlg;
	real64	pos[3], rot[4], ang;

	if((upper = p_input_node_first_type(input[0], V_NT_OBJECT)) == NULL ||
	   (lower = p_input_node_first_type(input[1], V_NT_OBJECT)) == NULL)
		return P_COMPUTE_DONE;

	if((ug = p_node_o_link_get(upper, "geometry", NULL)) == NULL ||
	   (lg = p_node_o_link_get(lower, "geometry", NULL)) == NULL)
		return P_COMPUTE_DONE;

	outup = p_output_node_copy(output, upper, 0);
	outug = p_output_node_copy(output, ug, 1);
/*	printf("----------- about to set link to new geometry\n");*/
	p_node_o_link_set_single(outup, outug, "geometry");
/*	printf("----------- done\n");*/

	/* Shift the copy a bit to the right. */
	p_node_o_pos_get(outup, pos);
	pos[0] += 5.0;
	p_node_o_pos_set(outup, pos);

	outlo = p_output_node_copy(output, lower, 2);
/*	outlg = p_output_node_copy(output, lg, 3);
	p_node_o_link_set(outlo, outlg, "geometry", ~0u);
*/
	/* Shift the copy a bit to the right. Hardcoded, of course. */
	p_node_o_pos_get(outlo, pos);
	pos[0] += 5.0;
	p_node_o_pos_set(outlo, pos);

	p_node_o_rot_get(lower, rot);
	printf(" rotation: [%g %g %g %g]\n", rot[0], rot[1], rot[2], rot[3]);
	ang = 0.0996 - rot[3];
	if(ang < 0)
		ang = 0.0;
/*	printf(" intermediate: %g\n", ang);*/
	if(ang > 0.030)
		ang = 0.030;
	ang /= 0.030;
	printf("  -> angle=%g\n", ang);
	planar_scale(outug, 1.0 + 0.5 * ang);

/*	printf("output rotations:\n");
	p_node_o_rot_get(outup, rot);
	printf(" upper: [%g %g %g %g]\n", rot[0], rot[1], rot[2], rot[3]);
	p_node_o_rot_get(outlo, rot);
	printf(" lower: [%g %g %g %g]\n", rot[0], rot[1], rot[2], rot[3]);
*/	
	return P_COMPUTE_DONE;
}


PURPLE_PLUGIN void init(void)
{
	p_init_create("muscle");
	p_init_input(0, P_VALUE_MODULE, "upper", P_INPUT_REQUIRED, 0);
	p_init_input(1, P_VALUE_MODULE, "lower", P_INPUT_REQUIRED, 1);
	p_init_compute(compute);
}
