/*
 *
*/

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "purple.h"

static const char *font[] = {
	"size 8x8",
	"dot *",
	"\n",
	" :"
	"........"
	"........"
	"........"
	"........"
	"........"
	"........"
 	"........"
	"........",
	"A:"
	"..***..."
	".**.**.."
	"**...**."
	"**...**."
	"*******."
	"**...**."
 	"**...**."
 	"........",
	"B:"
	"******.."
	"**...**."
	"**...**."
	"******.."
	"**...**."
	"**...**."
	"******.."
	"........",
	"C:"
	".*****.."
	"**...**."
	"**......"
	"**......"
	"**......"
	"**...**."
 	".*****.."
	"........",
	"D:"
	"******.."
	"**...**."
	"**...**."
	"**...**."
	"**...**."
	"**...**."
 	"******.."
	"........",
	"E:"
	"*******."
	"**......"
	"**......"
	"****...."
	"**......"
	"**......"
 	"*******."
	"........",
	"F:"
	"*******."
	"**......"
	"**......"
	"****...."
	"**......"
	"**......"
 	"**......"
	"........",
	"G:"
	".*****.."
	"**...**."
	"**......"
	"**.****."
	"**...**."
	"**...**."
 	".*****.."
	"........",
	"H:"
	"**...**."
	"**...**."
	"**...**."
	"*******."
	"**...**."
	"**...**."
 	"**...**."
	"........",
	"I:"
	"..****.."
	"...**..."
	"...**..."
	"...**..."
	"...**..."
	"...**..."
 	"..****.."
	"........",
	"J:"
	"....***."
	".....**."
	".....**."
	".....**."
	".....**."
	"**...**."
 	".*****.."
	"........",
	"K:"
	"**...**."
	"**..**.."
	"**.**..."
	"****...."
	"**.**..."
	"**..**.."
 	"**...**."
	"........",
	"L:"
	"**......"
	"**......"
	"**......"
	"**......"
	"**......"
	"**......"
 	"*******."
	"........",
	"M:"
	"**...**."
	"***.***."
	"**.*.**."
	"**.*.**."
	"**...**."
	"**...**."
 	"**...**."
	"........",
	"N:"
	"**...**."
	"***..**."
	"****.**."
	"**.****."
	"**..***."
	"**...**."
 	"**...**."
	"........",
	"O:"
	".*****.."
	"**...**."
	"**...**."
	"**...**."
	"**...**."
	"**...**."
 	".*****.."
	"........",
	"P:"
	"******.."
	"**...**."
	"**...**."
	"******.."
	"**......"
	"**......"
 	"**......"
	"........",
	"Q:"
	".*****.."
	"**...**."
	"**...**."
	"**...**."
	"**...**."
	"**.***.."
 	".***.**."
	"........",
	"R:"
	"******.."
	"**...**."
	"**...**."
	"******.."
	"**.**..."
	"**..**.."
 	"**...**."
	"........",
	"S:"
	".*****.."
	"**...**."
	"**......"
	".*****.."
	".....**."
	"**...**."
 	".*****.."
	"........",
	"T:"
	"******.."
	"..**...."
	"..**...."
	"..**...."
	"..**...."
	"..**...."
 	"..**...."
	"........",
	"U:"
	"**...**."
	"**...**."
	"**...**."
	"**...**."
	"**...**."
	"**...**."
 	".*****.."
	"........",
	"V:"
	"**...**."
	"**...**."
	"**...**."
	"**...**."
	"**...**."
	".**.**.."
 	"..***..."
	"........",
	"W:"
	"**...**."
	"**...**."
	"**...**."
	"**.*.**."
	"**.*.**."
	"***.***."
 	"**...**."
	"........",
	"X:"
	"**...**."
	"**...**."
	".**.**.."
	"..***..."
	".**.**.."
	"**...**."
 	"**...**."
	"........",
	"Y:"
	"**...**."
	"**...**."
	".**.**.."
	"..***..."
	"...*...."
	"...*...."
 	"...*...."
	"........",
	"Z:"
	"*******."
	".....**."
	"....**.."
	"..***..."
	".**....."
	"**......"
 	"*******."
	"........",
	".:"
	"........"
	"........"
	"........"
	"........"
	"........"
	".**....."
	".**....."
	"........",
	"0:"
	".*****.."
	"**...**."
	"**..***."
	"**.*.**."
	"***..**."
	"**...**."
	".*****.."
	"........",
	"1:"
	"...**..."
	"..***..."
	"...**..."
	"...**..."
	"...**..."
	"...**..."
	"..****.."
	"........",
	"2:"
	".*****.."
	"**...**."
	".....**."
	"..****.."
	".**....."
	"**......"
	"*******."
	"........",
	"3:"
	".*****.."
	"**...**."
	".....**."
	"...***.."
	".....**."
	"**...**."
	".*****.."
	"........",
	"4:"
	"**...**."
	"**...**."
	"**...**."
	"*******."
	".....**."
	".....**."
	".....**."
	"........",
	"5:"
	"*******."
	"**......"
	"**......"
	"******.."
	".....**."
	"**...**."
	".*****.."
	"........",
	"6:"
	"..**...."
	".**....."
	"**......"
	"******.."
	"**...**."
	"**...**."
	".*****.."
	"........",
	"7:"
	"*******."
	".....**."
	"....**.."
	"....**.."
	"...**..."
	"...**..."
	"...**..."
	"........",
	"8:"
	".*****.."
	"**...**."
	"**...**."
	".*****.."
	"**...**."
	"**...**."
	".*****.."
	"........",
	"9:"
	".*****.."
	"**...**."
	"**...**."
	".*****.."
	".....**."
	".....**."
	".*****.."
	"........",
	NULL
};

typedef struct {
	uint16	width, height;
	uint8	*glyph[256];
} Font;

typedef struct {
	Font	*font;
} State;

static Font * font_new(const char *def[])
{
	Font		*f;
	int		i, dl, hl, gw, gh, dot = ' ';
	size_t		gs, rs;
	unsigned char	ch;

	for(i = 0; def[i] != NULL; i++)
		;
	dl = i;
	for(i = 0; i < dl; i++)
		if(def[i][0] == '\n')
			break;
	hl = i;

	if(hl < 1)
		return NULL;
	for(i = 0; i < hl; i++)
	{
		if(sscanf(def[i], "size %dx%d", &gw, &gh) == 2)
			;
		else if(sscanf(def[i], "dot %c", &dot) == 1)
			;
		else
			printf("Unknown font header line '%s'", def[i]);
	}
/*	printf("font: def len=%u head len=%u, size=%ux%u dot='%c'\n", dl, hl, gw, gh, dot);*/
	f = malloc(sizeof *f);
	f->width  = gw;
	f->height = gh;
	for(i = 0; i < sizeof f->glyph / sizeof *f->glyph; i++)
		f->glyph[i] = NULL;

	rs = (f->width + 7) / 8;
	gs = rs * f->height;

	for(i = hl + 1; i < dl; i++)
	{
		if((ch = def[i][0]) != '\0' && def[i][1] == ':')
		{
			int	j = strlen(def[i] + 2), k, x, y;
			int	here;

			/* FIXME: Should pre-check # of glyphs and allocate a single chunk. */
			f->glyph[ch] = malloc(gs);
			if(f->glyph[ch] == NULL)
			{
				continue;
			}
/*			printf("initializing glyph %u at %p in font at %p\n", ch, f->glyph[ch], f);*/
			memset(f->glyph[ch], 0, gs);
			for(y = 0, k = 2; y < f->height; y++)
			{
				for(x = 0; x < f->width && def[i][k] != '\0'; x++, k++)
				{
					if(def[i][k] == dot)
						f->glyph[ch][y * rs + x / 8] |= 128 >> (7 - (x & 7));
				}
			}
/*			for(y = 0; y < f->height; y++)
			{
				for(x = 0; x < f->width; x++)
					printf("%c", f->glyph[ch][y * rs + x / 8] & (128 >> (7 - x)) ? '*' : ' ');
				printf("\n");
			}
*/		}
	}
	return f;
}

static void font_render_char(const Font *f, unsigned int glyph, unsigned char *fb, size_t modulo, unsigned char pen)
{
	unsigned int		x, y;
	const unsigned char	*g;

	glyph = toupper(glyph);	/* We "know" the font doesn't include lower case chars. */
	if(f == NULL || glyph >= 256 || f->glyph[glyph] == NULL)
	{
		printf("bmtext ignoring bad glyph %u, font at %p glyph at %p\n", glyph, f, glyph < 256 ? f->glyph[glyph] : NULL);
		return;
	}
/*	printf("rendering '%c'\n", glyph);*/
	g = f->glyph[glyph];
	for(y = 0; y < f->height; y++, fb += modulo, g += (f->width + 7) / 8)
	{
		for(x = 0; x < f->width; x++)
		{
			if(g[x / 8] & (128 >> (7 - (x & 7))))
				fb[x] = pen;
			else
				fb[x] = 0;
/*			printf("%02X", fb[x]);*/
		}
/*		printf("\n");*/
	}
}

/* Render a line. Terminates by '\0' or '\n'. */
static void font_render_line(const Font *f, const char *string, unsigned char *fb, size_t modulo, unsigned char pen)
{
	for(; *string != '\0' && *string != '\n'; string++, fb += f->width)
		font_render_char(f, *string, fb, modulo, pen);
}

static void font_destroy(Font *f)
{
	int	i;

	for(i = 0; i < sizeof f->glyph / sizeof *f->glyph; i++)
	{
		if(f->glyph[i] != NULL)
			free(f->glyph[i]);
	}
	free(f);
}

/* ------------------------------------------------------------------------------------------------------------------------- */

static void ctor(void *state)
{
	State	*s = state;

	s->font = font_new(font);
}

static void dtor(void *state)
{
	State	*s = state;

	font_destroy(s->font);
}

/* Split text into lines, terminated by line feed characters. */
static int text_split(const char *string, const char **line, size_t max_line, size_t *longest)
{
	int	i, len = 0, mlen = 0;

	for(i = 0; i < max_line && *string; i++)
	{
		line[i] = string;
		for(len = 0; *string && *string != '\n'; len++, string++)
			;
		if(len > mlen)
			mlen = len;
		if(*string == '\n')
			string++;
	}
	if(longest != NULL)
		*longest = mlen;
	return i;
}

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	State		*s = state;
	const char	*text = p_input_string(input[0]);
	const char	*lname[] = { "col_r", "col_g", "col_b" };
	size_t		len, i, j, longest, lines;
	PONode		*node;
	PNBLayer	*layer;
	const char	*line[32];
	unsigned char	*ptr;

	if(text == NULL)
		return P_COMPUTE_DONE;
	lines = text_split(text, line, sizeof line / sizeof *line, &longest);

	node = p_output_node_create(output, V_NT_BITMAP, 0);
	p_node_set_name(node, "text");
	p_node_b_set_dimensions(node, longest * s->font->width, lines * s->font->height, 1);

	for(i = 0; i < sizeof lname / sizeof *lname; i++)
	{
		layer = p_node_b_layer_create(node, lname[i], VN_B_LAYER_UINT8);
		if((ptr = p_node_b_layer_access_begin(node, layer)) != NULL)
		{
			memset(ptr, 0, longest * s->font->width * lines * s->font->height);
			for(j = 0; j < lines; j++, ptr += s->font->height * (longest * s->font->width))
				font_render_line(s->font, line[j], ptr, longest * s->font->width, 0xff);
			p_node_b_layer_access_end(node, layer, ptr);
		}
	}
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("bmtext");
	p_init_input(0, P_VALUE_STRING, "text", P_INPUT_REQUIRED, P_INPUT_DEFAULT_STR("FOO"),
		     P_INPUT_DESC("The text to render is input here. You can embed newlines by using the "
				  "C standard notation of \\n. Text will be flush to the left of each line."),
		     P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("copyright", "2005 PDC, KTH");
	p_init_meta("desc/purpose", "Generates a bitmap representation of the input string. The text is rendered with "
		    "a built-in 8x8 pixel fixed-width font, in white on a black background.");
	p_init_state(sizeof (State), ctor, dtor);
	p_init_compute(compute);
}
