/**
 * unmakelev.c -- converts Wings level files to bmp images.
 * 
 * Copyright (C) 2001  Pauli Virtanen <pauli.virtanen@saunalahti.fi>
 *
 * This program converts the level data contained in a level file of
 * an elder cavern flying game Wings to a BMP image.
 *
 * This program is public domain: you can do anything with it.
 * I also disclaim all responsibility of anything this program
 * does or does not do. You use it at your own risk.
 *
 * Wings itself is made by Miika Virpioja. You can (and should :)
 * get Wings from the address <http://www.mbnet.fi/~mvirpioj>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>


/***************************************************************************
 * Prototypes and structures
 */
void* xrealloc(void* ptr, size_t size);
void* xmalloc(size_t size);
void  xfree(void* ptr);

unsigned char* load_data(FILE* in, size_t* length);
unsigned char *load_file(char* file_name, size_t* length);

/** A simple image structure **/
typedef struct image_t
{
	size_t w, h;
	unsigned char palette_r[256];
	unsigned char palette_g[256];
	unsigned char palette_b[256];
	unsigned char* pixels;
} image_t;

void write_image(image_t* image, char* filename);

/***************************************************************************
 * Conversion from the level file format to an image.
 */
#define LSB_UNSIGNED_SHORT(c) (	((unsigned short)*(c + 0) << 0) +	\
				((unsigned short)*(c + 1) << 8)	)
#define LSB_UNSIGNED_LONG(c) (	((unsigned long)*(c + 0) <<  0) +	\
				((unsigned long)*(c + 1) <<  8) +	\
				((unsigned long)*(c + 2) << 16) +	\
				((unsigned long)*(c + 3) << 24)	)

int analyze(unsigned char* data, size_t length, image_t* image)
{
	unsigned char* level_pcx_data;
	size_t level_pcx_data_length;
	unsigned char* level_palette;
	unsigned short level_w, level_h;

	/* Check that there is enough data for the header */
	if (length <= 0x308) {
		fprintf(stderr, "Not enough data.\n");
		return 1;
	}

	/* Palette is in RGB format, three bytes per color,
	 * RGB values are in range 0-63
	 * @0x0
	 */
	level_palette = data + 0x0;

	/* Width and height @0x300 and @0x302 LSB */
	level_w = LSB_UNSIGNED_SHORT(data + 0x300);
	level_h = LSB_UNSIGNED_SHORT(data + 0x302);

	/* Level data length is LSB unsigned long @0x304 */
	level_pcx_data_length = LSB_UNSIGNED_LONG(data + 0x304);
	
	/* Level bitmap in PCX format @0x308 */
	level_pcx_data = data + 0x308;

	/* And after this probably is the parallax picture. In PCX
	 * format, I think. */
	 
	/* At the end of the file there are probably some flags. */

	fprintf(stderr, "Level: %u x %u, %u data bytes\n",
			level_w, level_h, level_pcx_data_length);

	/** Check that all data needed is contained **/
	if (length < 0x308 + level_pcx_data_length) {
		fprintf(stderr, "Not enough data.\n");
		return 1;
	}

	/******************************************************************
	 * Fill the image structure
	 */
	image->w = level_w;
	image->h = level_h;
	
	/* Palette */
	{
		int i;
		for (i = 0; i < 256; ++i) {
			unsigned char r = level_palette[i * 3 + 0] * 4;
			unsigned char g = level_palette[i * 3 + 1] * 4;
			unsigned char b = level_palette[i * 3 + 2] * 4;
			
			image->palette_r[i] = r;
			image->palette_g[i] = g;
			image->palette_b[i] = b;
		}
	}

	/* Pixels (assume PCX RLE) */
	image->pixels = (unsigned char*)xmalloc(image->w * image->h);
	{
		unsigned char* p;
		unsigned char* o;
		unsigned char* start;
		unsigned char* end;
		
		start = level_pcx_data;
		end = start + level_pcx_data_length;

		o = image->pixels;
		
		for (p = start; p < end; ++p) {
			if (*p > 192) {
				int count = *p - 192;
				++p;
				do {
					*(o++) = *p;
					--count;
				} while (count > 0);
			} else {
				*(o++) = *p;
			}
		}
	}

	/* Now we are done */
	return 0;
}

/****************************************************************************
 * Main
 */
int main(int argc, char** argv)
{
	size_t length;
	unsigned char *data;
	image_t image = {0};
	char* filename;

	if (argc <= 1) {
		fprintf(stderr,
			"unmakelev -- a program for converting levels from\n"
			"             the cavern flying game Wings.\n"
			"\n"
			"Usage: unmakelev <levelfile>\n");
		return 1;
	}
	
	/* Load the file given */
	data = load_file(argv[1], &length);

	if (data == NULL) {
		fprintf(stderr, "Failed to load data.\n");
		return 1;
	}
	
	/* Convert it to an image */
	if (analyze(data, length, &image)) {
		fprintf(stderr, "Analyzing data failed.\n");
		xfree(data);
		return 1;
	}
	xfree(data);

	/* Write the image */
	filename = xmalloc(strlen(argv[1]) + 4 + 1);
	strcpy(filename, argv[1]);
	strcat(filename, ".bmp");
	write_image(&image, filename);

	/* Free image pixels */
	xfree(image.pixels);

	/* All done */
	return 0;
}

/***************************************************************************
 * Write the image
 */
void write_image(image_t* image, char* filename)
{
	SDL_Surface* surface;
	SDL_Color color;
	unsigned long i;
	
	SDL_Init(SDL_INIT_VIDEO);
	/*SDL_SetVideoMode(320, 200, 8, SDL_ANYFORMAT | SDL_SWSURFACE);*/

	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, image->w, image->h, 8,
	                               0xFF, 0xFF, 0xFF, 0);
	for (i = 0; i < 256; ++i) {
		color.r = image->palette_r[i];
		color.g = image->palette_g[i];
		color.b = image->palette_b[i];
		SDL_SetColors(surface, &color, i, 1);
	}
	SDL_LockSurface(surface);
	for (i = 0; i < image->w * image->h; ++i) {
		((unsigned char*)surface->pixels)[i] = image->pixels[i];
	}
	SDL_UnlockSurface(surface);

	SDL_SaveBMP(surface, filename);

	SDL_Quit();
}

/****************************************************************************
 * Load data 
 */
unsigned char *load_file(char* file_name, size_t* length)
{
	unsigned char* data;
	
	FILE* in = fopen(file_name, "r");

	if (in == NULL) {
		fprintf(stderr, "Unable to open file \"%s\" for reading.\n",
				file_name);
		return NULL;
	}
	
	data = load_data(in, length);
	fclose(in);

	return data;
}

#define BLOCK_SIZE 1024
unsigned char* load_data(FILE* in, size_t* length)
{
	unsigned char* data;
	size_t readen;
	size_t size;

	data = NULL;
	readen = 0;
	size = 0;
	*length = 0;

	do {
		*length += readen;
	
		if (*length + BLOCK_SIZE >= size) {
			size = (size + BLOCK_SIZE) * 2;
			data = xrealloc(data, size);
		}
	} while ((readen = fread(data + *length, 1, BLOCK_SIZE, in)) > 0);

	if (readen < 0) {
		/* An error occurred */
		free(data);
		return NULL;
	}

	return xrealloc(data, *length); /* Do not waste memory */
}

/*****************************************************************************
 * Common memory management functions
 */
void* xrealloc(void* ptr, size_t size)
{
	void* data;
	if (ptr != NULL) {
		data = realloc(ptr, size);
	} else {
		data = malloc(size);
	}
	if (data == NULL) {
		fprintf(stderr, "Could not allocate memory.\n");
		exit(-1);
	}
	return data;
}

void* xmalloc(size_t size)
{
	return xrealloc(NULL, size);
}

void xfree(void* ptr)
{
	if (ptr != NULL) {
		free(ptr);
	}
}

