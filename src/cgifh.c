/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2024 Michael Drake <tlsa@netsurf-browser.org>
 */

/**
 * \file Simple bitmap font.
 */

#include <cgifh.h>

#include "bits.h"
#include "font.h"

/**
 * Get the number of elements in an array.
 *
 * \param[in] _a The array to get the number of elements in.
 * \return The number of elements in the array.
 */
#define CGIFH_ARRAY_LEN(_a) (sizeof(_a) / sizeof((_a)[0]))

/* Exported function, documented in cgifh.h */
bool cgifh_palette_add(cgifh_t *img,
		uint8_t r, uint8_t g, uint8_t b, uint8_t *idx_out)
{
	enum { R, G, B };

	if (img->palette_count >= CGIFH_PALETTE_MAX) {
		return false;
	}

	img->palette[CGIFH_CHANNEL_COUNT * img->palette_count + R] = r;
	img->palette[CGIFH_CHANNEL_COUNT * img->palette_count + G] = g;
	img->palette[CGIFH_CHANNEL_COUNT * img->palette_count + B] = b;

	if (idx_out != NULL) {
		*idx_out = (uint8_t) img->palette_count;
	}

	img->palette_count++;

	return true;
}

/* Exported function, documented in cgifh.h */
bool cgifh_palette_add_blend(cgifh_t *img,
		uint8_t idx0, uint8_t idx1, uint8_t pos,
		uint8_t *idx_out)
{
	enum { R, G, B };
	uint8_t *p0 = img->palette + CGIFH_CHANNEL_COUNT * idx0;
	uint8_t *p1 = img->palette + CGIFH_CHANNEL_COUNT * idx1;
	uint8_t r = (uint8_t)((p0[R] <= p1[R]) ?
			p0[R] + (p1[R] - p0[R]) * pos / 255 :
			p0[R] - (p0[R] - p1[R]) * pos / 255);
	uint8_t g = (uint8_t)((p0[G] <= p1[G]) ?
			p0[G] + (p1[G] - p0[G]) * pos / 255 :
			p0[G] - (p0[G] - p1[G]) * pos / 255);
	uint8_t b = (uint8_t)((p0[B] <= p1[B]) ?
			p0[B] + (p1[B] - p0[B]) * pos / 255 :
			p0[B] - (p0[B] - p1[B]) * pos / 255);

	return cgifh_palette_add(img, r, g, b, idx_out);
}

/* Exported function, documented in cgifh.h */
cgifh_t *cgifh_create(size_t width, size_t height)
{
	cgifh_t *img;

	if (width == 0 || height == 0 || width > INT_MAX || height > INT_MAX) {
		return NULL;
	}

	img = malloc(sizeof(cgifh_t) + width * height);
	if (img == NULL) {
		return NULL;
	}

	img->width = (int) width;
	img->height = (int) height;
	img->size = width * height;
	img->palette_count = 0;

	return img;
}

/* Exported function, documented in cgifh.h */
void cgifh_destroy(cgifh_t *img)
{
	free(img);
}

/**
 * Prototype for a function to set a pixel in an image.
 *
 * \param[in] img    The image to set the pixel in.
 * \param[in] colour The palette index of the colour to set the pixel to.
 * \param[in] x      The x coordinate of the pixel.
 * \param[in] y      The y coordinate of the pixel.
 */
typedef void (*cgifh_pixel_fn)(
		cgifh_t *img,
		uint8_t colour,
		int x,
		int y);

/**
 * Set a pixel in an image.
 *
 * This function does not clip the pixel coordinates, so it is up to the caller
 * to ensure that x and y are in range.
 *
 * \param[in] img    The image to set the pixel in.
 * \param[in] colour The palette index of the colour to set the pixel to.
 * \param[in] x      The x coordinate of the pixel.
 * \param[in] y      The y coordinate of the pixel.
 */
static inline void cgifh_pixel(
		cgifh_t *img,
		uint8_t colour,
		int x,
		int y)
{
	img->data[y * img->width + x] = colour;
}

/**
 * Set a pixel in an image, if the pixel is in bounds.
 *
 * \param[in] img    The image to set the pixel in.
 * \param[in] colour The palette index of the colour to set the pixel to.
 * \param[in] x      The x coordinate of the pixel.
 * \param[in] y      The y coordinate of the pixel.
 */
static inline void cgifh_pixel_clipped(
		cgifh_t *img,
		uint8_t colour,
		int x,
		int y)
{
	if (x >= 0 && x < img->width && y >= 0 && y < img->height) {
		cgifh_pixel(img, colour, x, y);
	}
}

/**
 * Get a pixel setting function for a given rectangle.
 *
 * If the rectangle is entirely within the image bounds, the fast pixel setting
 * function is returned. If the rectangle is entirely outside the image bounds,
 * NULL is returned. If the rectangle is partially within the image bounds, the
 * clipped pixel setting function is returned.
 *
 * \param[in] img     The image to get the pixel setting function for.
 * \param[in] test_x0 The left x coordinate of the pixel.
 * \param[in] test_y0 The top y coordinate of the pixel.
 * \param[in] test_x1 The right x coordinate of the pixel.
 * \param[in] test_y1 The bottom y coordinate of the pixel.
 * \return The pixel setting function to use or NULL.
 */
static inline cgifh_pixel_fn cgifh_get_px_fn(
		const cgifh_t *img,
		int test_x0,
		int test_y0,
		int test_x1,
		int test_y1)
{
	int clip_x0 = 0;
	int clip_y0 = 0;
	int clip_x1 = img->width;
	int clip_y1 = img->height;

	if (test_x0 > test_x1) {
		int tmp = test_x0;
		test_x0 = test_x1;
		test_x1 = tmp;
	}

	if (test_y0 > test_y1) {
		int tmp = test_y0;
		test_y0 = test_y1;
		test_y1 = tmp;
	}

	if (test_x0 >= clip_x0 &&
	    test_x1 <  clip_x1 &&
	    test_y0 >= clip_y0 &&
	    test_y1 <  clip_y1) {
		return cgifh_pixel;
	}

	if (test_x1 <  clip_x0 ||
	    test_x0 >= clip_x1 ||
	    test_y1 <  clip_y0 ||
	    test_y0 >= clip_y1) {
		return NULL;
	}

	return cgifh_pixel_clipped;
}

/* Exported function, documented in cgifh.h */
void cgifh_v_line(
		cgifh_t *img,
		uint8_t colour,
		int y0,
		int y1,
		int x)
{
	cgifh_pixel_fn px = cgifh_get_px_fn(img, x, y0, x, y1);

	if (px == NULL) {
		return;
	}

	if (y0 <= y1) {
		for (int row = y0; row <= y1; row++) {
			px(img, colour, x, row);
		}
	} else {
		for (int row = y1; row <= y0; row++) {
			px(img, colour, x, row);
		}
	}
}

/* Exported function, documented in cgifh.h */
void cgifh_h_line(
		cgifh_t *img,
		uint8_t colour,
		int x0,
		int x1,
		int y)
{
	cgifh_pixel_fn px = cgifh_get_px_fn(img, x0, y, x1, y);

	if (px == NULL) {
		return;
	}

	if (x0 <= x1) {
		for (int col = x0; col <= x1; col++) {
			px(img, colour, col, y);
		}
	} else {
		for (int col = x1; col <= x0; col++) {
			px(img, colour, col, y);
		}
	}
}

/* Exported function, documented in cgifh.h */
void cgifh_line(
		cgifh_t *img,
		uint8_t colour,
		int x0, int y0,
		int x1, int y1)
{
	cgifh_pixel_fn px = cgifh_get_px_fn(img, x0, y0, x1, y1);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int dx =  abs(x1 - x0);
	int dy = -abs(y1 - y0);
	int error = dx + dy;

	if (px == NULL) {
		return;
	}

	while (true) {
		int error2;

		px(img, colour, x0, y0);

		if (x0 == x1 && y0 == y1) {
			break;
		}

		error2 = 2 * error;
		if (error2 >= dy) {
			error += dy;
			x0 += sx;
		}
		if (error2 <= dx) {
			error += dx;
			y0 += sy;
		}
	}
}

/* Exported function, documented in cgifh.h */
void cgifh_rect_fill(
		cgifh_t *img,
		uint8_t colour,
		int x, int y,
		int w, int h)
{
	cgifh_pixel_fn px = cgifh_get_px_fn(img, x, y, x + w, y + h);

	if (px == NULL) {
		return;
	}

	for (int row = y; row < y + h; row++) {
		for (int col = x; col < x + w; col++) {
			px(img, colour, col, row);
		}
	}
}

/**
 * Get the glyph for a character.
 *
 * \param[in] character  Character to get glyph for.
 * \return Pointer to glyph structure or NULL if character is not supported.
 */
static const cgifh_glyph_t *cgifh_get_glyph(char character)
{
	if ((unsigned char)character >= CGIFH_ARRAY_LEN(font_h8)) {
		return NULL;
	}

	return &font_h8[(unsigned char)character];
}

/* Exported function, documented in cgifh.h */
int cgifh_char_scaled(
		cgifh_t *img,
		uint8_t colour,
		char character,
		int scale_x,
		int scale_y,
		int x,
		int y)
{
	const cgifh_glyph_t *glyph = cgifh_get_glyph(character);
	cgifh_pixel_fn px;

	px = cgifh_get_px_fn(img, x, y,
			x + glyph->advance * scale_x,
			y + 8 * scale_y);

	if (glyph == NULL || glyph->advance == 0) {
		return 0;

	} else if (px == NULL) {
		return glyph->advance * scale_x;
	}

	for (int row = 0; row < CGIFH_GLYPH_HEIGHT; row++) {
		if (glyph->data[row] != 0) {
			int xx = x;
			for (int col = 0; col < CGIFH_GLYPH_WIDTH; col++) {
				if (glyph->data[row] & (0x80 >> col)) {
					for (int i = 0; i < scale_y; i++) {
						for (int j = 0; j < scale_x; j++) {
							px(img, colour, xx + j, y + i);
						}
					}
				}
				xx += scale_x;
			}
		}
		y += scale_y;
	}

	return glyph->advance * scale_x;
}

/* Exported function, documented in cgifh.h */
int cgifh_char(
		cgifh_t *img,
		uint8_t colour,
		char character,
		int scale,
		int x,
		int y)
{
	return cgifh_char_scaled(img, colour, character, scale, scale, x, y);
}

/* Exported function, documented in cgifh.h */
int cgifh_text(
		cgifh_t *img,
		uint8_t colour,
		const char *text,
		int scale,
		int x,
		int y)
{
	int advance = 0;

	while (*text != '\0') {
		advance += cgifh_char(img, colour, *text,
				scale, x + advance, y);
		text++;
	}

	return advance;
}

/* Exported function, documented in cgifh.h */
int cgifh_text_width(const char *text, int scale)
{
	int advance = 0;

	while (*text != '\0') {
		const cgifh_glyph_t *glyph = cgifh_get_glyph(*text);

		if (glyph != NULL) {
			advance += font_h8[(unsigned char)*text].advance;
		}
		text++;
	}

	return advance * scale;
}
