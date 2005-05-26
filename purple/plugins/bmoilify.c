/*
 * A bitmap Purple plug-in that applies an "oilify" filter to incoming image data. 
*/

#include "purple.h"

/* Simple (pixel,count) data structure used in histogram computations. */
typedef struct
{
	uint32	pixel;
	uint32	count;
} HEntry; 

/* Clamp an integer value into the [mi,ma) range. */
#define	CLAMP(v,mi,ma)		if(v < mi) v = mi; else if(v >= ma) v = ma - 1;

/* Pixel access macros, to save some typing. Assumes fb is 24-bit packed RGB pixels. */
#define	GET1(fb,w,x,y,c)	fb[y * 3 * w + 3 * x + c]
#define	GET3(fb,w,x,y)		(GET1(fb,w,x,y,0) | (GET1(fb,w,x,y,1) << 8) | (GET1(fb,w,x,y,2) << 16))

#define	SET1(fb,w,x,y,c,v)	fb[y * 3 * w + 3 * x + c] = v
#define	SET3(fb,w,x,y,v)	do { SET1(fb,w,x,y,0,v); SET1(fb,w,x,y,1,(v >> 8)); SET1(fb,w,x,y,2,(v >> 16)); } while(0)
	       
/* Compute "oil" filter over the given pixels. Algorithm is simply: replace each pixel
 * with the one that is most common in the size*size area whose upper-left pixel is (x,y).
*/
static void do_oilify(void *pixels, uint16 width, uint16 height, uint32 size, HEntry *table)
{
	uint8	*get = pixels;
	int	x, y, x1, y1, x2, y2, ax, ay;
	uint32	here, tsize, i;
	HEntry	*max;

	size /= 2;

	printf(" applying filter size %u\n", size);

	for(y = 0; y < height; y++)
	{
		for(x = 0; x < width; x++)
		{
			x1 = x - size;
			y1 = y - size;
			x2 = x + size + 1;
			y2 = y + size + 1;
			CLAMP(x1, 0, width);
			CLAMP(y1, 0, height);
			CLAMP(x2, 0, width);
			CLAMP(y2, 0, height);
			memset(table, 0, size * size * sizeof *table);
			tsize = 0;
			for(ay = y1; ay < y2; ay++)
			{
				for(ax = x1; ax < x2; ax++)
				{
					here = GET3(get, width, ax, ay);
					for(i = 0; i < size * size; i++)
					{
						if(table[i].pixel == here)
						{
							table[i].count++;
							break;
						}
					}
					if(i == size * size)
						table[tsize++].pixel = here;
				}
			}
			for(i = 1, max = table; i < tsize; i++)
			{
				if(table[i].count > max->count)
					max = table + i;
			}
			SET3(get,width,x,y,max->pixel);
		}
	}
}

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode	*node;
	PONode	*out;
	int	i;
	uint16	w, h, z;
	uint32	size;
	HEntry	*table;

	size = p_input_uint32(input[1]);
	printf("oilify size is %u, allocating %u bytes of table space\n", size, size * size * sizeof *table);
	table = malloc(size * size * sizeof *table);
	if(table == NULL)
		return P_COMPUTE_AGAIN;

	for(i = 0; (node = p_input_node_nth(input[0], i)) != NULL; i++)
	{
		void	*pixels;

		if(p_node_get_type(node) != V_NT_BITMAP)
			continue;
		out = p_output_node_copy(output, node, 0);
		if((pixels = p_node_b_layer_access_multi_begin(out, VN_B_LAYER_UINT8, "col_r", "col_g", "col_b", NULL)) != NULL)
		{
			p_node_b_get_dimensions(out, &w, &h, NULL);
			printf("oilify: image is %ux%u\n", w, h);
			do_oilify(pixels, w, h, size, table);
			p_node_b_layer_access_multi_end(out, pixels);
		}
		break;
	}
	free(table);
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("bmoilify");
	p_init_input(0, P_VALUE_MODULE, "bitmap", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_UINT32, "size", P_INPUT_REQUIRED, P_INPUT_MIN(1), P_INPUT_MAX(32), P_INPUT_DEFAULT(8), P_INPUT_DONE);
	p_init_compute(compute);
}
