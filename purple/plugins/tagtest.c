/*
 * Test the tag system.
*/

#include "purple.h"

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	uint32	mode = p_input_uint32(input[0]);
	PONode	*out;
	static uint32	blob[8];

	out = p_output_node_create(output, V_NT_OBJECT, 0);
	p_node_set_name(out, "tag-test");
	if(mode == 0)
		p_node_tag_create_path(out, "test/test1", VN_TAG_BOOLEAN, rand() > 0x80000000);
	else if(mode == 1)
		p_node_tag_create_path(out, "test/test1", VN_TAG_UINT32, rand());
	else if(mode == 2)
		p_node_tag_create_path(out, "test/test1", VN_TAG_REAL64, ((real64) rand()) / 0xffffffff);
	else if(mode == 3)
		p_node_tag_create_path(out, "test/test1", VN_TAG_STRING, "foo");
	else if(mode == 4)
		p_node_tag_create_path(out, "test/test1", VN_TAG_REAL64_VEC3, 1.0, 0.5, -1.0);
	else if(mode == 5)
		p_node_tag_create_path(out, "test/test1", VN_TAG_LINK, rand());
	else if(mode == 6)
		p_node_tag_create_path(out, "test/test1", VN_TAG_ANIMATION, rand(), rand(), rand());
	else if(mode == 7)
	{
		int	i;

		for(i = 0; i < sizeof blob; i++)
			blob[i] = rand();
		p_node_tag_create_path(out, "test/test1", VN_TAG_BLOB, sizeof blob, blob);
	}

/*	{
		PIter		iter;
		PNTagGroup	*tg;

		printf("**iter test:\n");
		for(p_node_tag_group_iter(out, &iter); (tg = p_iter_data(&iter)) != NULL; p_iter_next(&iter))
		{
			PIter	titer;
			PNTag	*tag;

			printf(" %u: '%s'\n", p_iter_index(&iter), p_node_tag_group_get_name(tg));
			printf(" tags:\n");
			for(p_node_tag_group_tag_iter(tg, &titer); (tag = p_iter_data(&titer)) != NULL; p_iter_next(&titer))
				printf("  %u: '%s'\n", p_iter_index(&titer), p_node_tag_get_name(tag));
		}
		printf("--done\n");
	}
*/	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("tagtest");
	p_init_input(0, P_VALUE_UINT32, "mode", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
