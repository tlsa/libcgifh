/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2024 Michael Drake <tlsa@netsurf-browser.org>
 */

#ifndef CGIFH_H
#define CGIFH_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/** Get the number of elements in an array. */
#define CGIFH_ARRAY_LEN(_a) (sizeof(_a) / sizeof((_a)[0]))

/** Maximum number of palette entries. */
#define CGIFH_PALETTE_MAX 256

/** Number of colour channels in the image palette. */
#define CGIFH_CHANNEL_COUNT 3

/** Glyph height in pixels. */
#define CGIFH_GLYPH_HEIGHT 8

/** Maximum glyph width in pixels. */
#define CGIFH_GLYPH_WIDTH 8

/**
 * CGIF Helper image structure.
 */
typedef struct cgifh {
	/** RGB palette */
	uint8_t palette[CGIFH_CHANNEL_COUNT * CGIFH_PALETTE_MAX];
	/** Number of entries in the palette. */
	uint16_t palette_count;

	int width;  /**< Image width in pixels. */
	int height; /**< Image height in pixels. */
	int size;   /**< Image data size in bytes. */
	uint8_t data[]; /**< Image data. */
} cgifh_t;

/**
 * Add a colour to the image palette.
 *
 * \param[in] img The image to add the colour to.
 * \param[in] r   Red component of the colour.
 * \param[in] g   Green component of the colour.
 * \param[in] b   Blue component of the colour.
 * \param[out] idx_out Pointer to variable to receive the palette index.
 * \return true if the colour was added, false if the palette is full.
 */
static inline bool cgifh_palette_add(cgifh_t *img,
		uint8_t r, uint8_t g, uint8_t b, uint8_t *idx_out)
{
	enum { R, G, B };

	if (img->palette_count >= CGIFH_PALETTE_MAX) {
		return false;
	}

	img->palette[CGIFH_CHANNEL_COUNT * img->palette_count + R] = r;
	img->palette[CGIFH_CHANNEL_COUNT * img->palette_count + G] = g;
	img->palette[CGIFH_CHANNEL_COUNT * img->palette_count + B] = b;

	img->palette_count++;

	if (idx_out != NULL) {
		*idx_out = img->palette_count;
	}

	return true;
}

/**
 * Add a blend of two existing palette colours to the palette.
 *
 * \param[in] img  The image to add the colour to.
 * \param[in] idx0 The palette index of the first colour.
 * \param[in] idx1 The palette index of the second colour.
 * \param[in] pos  The position of the blend, 0 is idx0, 255 is idx1.
 * \param[out] idx_out Pointer to variable to receive the palette index.
 * \return true if the colour was added, false if the palette is full.
 */
static inline bool cgifh_palette_add_blend(cgifh_t *img,
		uint8_t idx0, uint8_t idx1, uint8_t pos,
		uint8_t *idx_out)
{
	enum { R, G, B };
	uint8_t *p0 = img->palette + CGIFH_CHANNEL_COUNT * idx0;
	uint8_t *p1 = img->palette + CGIFH_CHANNEL_COUNT * idx1;
	uint8_t r = (p0[R] <= p1[R]) ?
			p0[R] + (p1[R] - p0[R]) * pos / 255 :
			p0[R] - (p0[R] - p1[R]) * pos / 255;
	uint8_t g = (p0[G] <= p1[G]) ?
			p0[G] + (p1[G] - p0[G]) * pos / 255 :
			p0[G] - (p0[G] - p1[G]) * pos / 255;
	uint8_t b = (p0[B] <= p1[B]) ?
			p0[B] + (p1[B] - p0[B]) * pos / 255 :
			p0[B] - (p0[B] - p1[B]) * pos / 255;

	return cgifh_palette_add(img, r, g, b, idx_out);
}

/**
 * Clear the image to a single colour.
 *
 * \param[in] img The image to clear.
 * \param[in] c   Palette index of colour to clear the image to.
 */
static inline void cgifh_clear(cgifh_t *img, uint8_t colour)
{
	for (int i = 0; i < img->size; i++) {
		img->data[i] = colour;
	}
}

/**
 * Create an image.
 *
 * \param[in] width  Image width in pixels.
 * \param[in] height Image height in pixels.
 * \return Pointer to the new image, or NULL on failure.
 */
static inline cgifh_t *cgifh_create(size_t width, size_t height)
{
	cgifh_t *img;

	if (width == 0 || height == 0 || width > INT_MAX || height > INT_MAX) {
		return NULL;
	}

	img = malloc(sizeof(cgifh_t) + width * height);
	if (img == NULL) {
		return NULL;
	}

	img->width = width;
	img->height = height;
	img->size = width * height;
	img->palette_count = 0;

	return img;
}

/**
 * Destroy an image.
 *
 * \param[in] img The image to destroy.
 */
static inline void cgifh_destroy(cgifh_t *img)
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

/**
 * Draw a vertical line.
 *
 * \param[in] img    The image to draw the line in.
 * \param[in] colour The palette index of the colour to draw the line in.
 * \param[in] y0     The top y coordinate of the line.
 * \param[in] y1     The bottom y coordinate of the line.
 * \param[in] x      The x coordinate of the line.
 */
static inline void cgifh_v_line(
		cgifh_t *img,
		uint8_t colour,
		size_t y0,
		size_t y1,
		size_t x)
{
	cgifh_pixel_fn px = cgifh_get_px_fn(img, x, y0, x, y1);

	if (px == NULL) {
		return;
	}

	if (y0 <= y1) {
		for (size_t row = y0; row <= y1; row++) {
			px(img, colour, x, row);
		}
	} else {
		for (size_t row = y1; row <= y0; row++) {
			px(img, colour, x, row);
		}
	}
}

/**
 * Draw a horizontal line.
 *
 * \param[in] img    The image to draw the line in.
 * \param[in] colour The palette index of the colour to draw the line in.
 * \param[in] x0     The left x coordinate of the line.
 * \param[in] x1     The right x coordinate of the line.
 * \param[in] y      The y coordinate of the line.
 */
static inline void cgifh_h_line(
		cgifh_t *img,
		uint8_t colour,
		size_t x0,
		size_t x1,
		size_t y)
{
	cgifh_pixel_fn px = cgifh_get_px_fn(img, x0, y, x1, y);

	if (px == NULL) {
		return;
	}

	if (x0 <= x1) {
		for (size_t col = x0; col <= x1; col++) {
			px(img, colour, col, y);
		}
	} else {
		for (size_t col = x1; col <= x0; col++) {
			px(img, colour, col, y);
		}
	}
}

/**
 * Draw a line.
 *
 * \param[in] img    The image to draw the line in.
 * \param[in] colour The palette index of the colour to draw the line in.
 * \param[in] x0     The x coordinate of the start of the line.
 * \param[in] y0     The y coordinate of the start of the line.
 * \param[in] x1     The x coordinate of the end of the line.
 * \param[in] y1     The y coordinate of the end of the line.
 */
static inline void cgifh_line(
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

/**
 * Draw a filled rectangle.
 *
 * \param[in] img    The image to draw a rectangle in.
 * \param[in] colour The palette index of the colour to fill the rectangle with.
 * \param[in] x      The x coordinate of the top left corner of the rectangle.
 * \param[in] y      The y coordinate of the top left corner of the rectangle.
 * \param[in] w      The width of the rectangle.
 * \param[in] h      The height of the rectangle.
 */
static inline void cgifh_rect_fill(
		cgifh_t *img,
		uint8_t colour,
		size_t x, size_t y,
		size_t w, size_t h)
{
	cgifh_pixel_fn px = cgifh_get_px_fn(img, x, y, x + w, y + h);

	if (px == NULL) {
		return;
	}

	for (size_t row = y; row < y + h; row++) {
		for (size_t col = x; col < x + w; col++) {
			px(img, colour, col, row);
		}
	}
}

#define ________ 0x00
#define _______S 0x01
#define ______S_ 0x02
#define ______SS 0x03
#define _____S__ 0x04
#define _____S_S 0x05
#define _____SS_ 0x06
#define _____SSS 0x07
#define ____S___ 0x08
#define ____S__S 0x09
#define ____S_S_ 0x0A
#define ____S_SS 0x0B
#define ____SS__ 0x0C
#define ____SS_S 0x0D
#define ____SSS_ 0x0E
#define ____SSSS 0x0F
#define ___S____ 0x10
#define ___S___S 0x11
#define ___S__S_ 0x12
#define ___S__SS 0x13
#define ___S_S__ 0x14
#define ___S_S_S 0x15
#define ___S_SS_ 0x16
#define ___S_SSS 0x17
#define ___SS___ 0x18
#define ___SS__S 0x19
#define ___SS_S_ 0x1A
#define ___SS_SS 0x1B
#define ___SSS__ 0x1C
#define ___SSS_S 0x1D
#define ___SSSS_ 0x1E
#define ___SSSSS 0x1F
#define __S_____ 0x20
#define __S____S 0x21
#define __S___S_ 0x22
#define __S___SS 0x23
#define __S__S__ 0x24
#define __S__S_S 0x25
#define __S__SS_ 0x26
#define __S__SSS 0x27
#define __S_S___ 0x28
#define __S_S__S 0x29
#define __S_S_S_ 0x2A
#define __S_S_SS 0x2B
#define __S_SS__ 0x2C
#define __S_SS_S 0x2D
#define __S_SSS_ 0x2E
#define __S_SSSS 0x2F
#define __SS____ 0x30
#define __SS___S 0x31
#define __SS__S_ 0x32
#define __SS__SS 0x33
#define __SS_S__ 0x34
#define __SS_S_S 0x35
#define __SS_SS_ 0x36
#define __SS_SSS 0x37
#define __SSS___ 0x38
#define __SSS__S 0x39
#define __SSS_S_ 0x3A
#define __SSS_SS 0x3B
#define __SSSS__ 0x3C
#define __SSSS_S 0x3D
#define __SSSSS_ 0x3E
#define __SSSSSS 0x3F
#define _S______ 0x40
#define _S_____S 0x41
#define _S____S_ 0x42
#define _S____SS 0x43
#define _S___S__ 0x44
#define _S___S_S 0x45
#define _S___SS_ 0x46
#define _S___SSS 0x47
#define _S__S___ 0x48
#define _S__S__S 0x49
#define _S__S_S_ 0x4A
#define _S__S_SS 0x4B
#define _S__SS__ 0x4C
#define _S__SS_S 0x4D
#define _S__SSS_ 0x4E
#define _S__SSSS 0x4F
#define _S_S____ 0x50
#define _S_S___S 0x51
#define _S_S__S_ 0x52
#define _S_S__SS 0x53
#define _S_S_S__ 0x54
#define _S_S_S_S 0x55
#define _S_S_SS_ 0x56
#define _S_S_SSS 0x57
#define _S_SS___ 0x58
#define _S_SS__S 0x59
#define _S_SS_S_ 0x5A
#define _S_SS_SS 0x5B
#define _S_SSS__ 0x5C
#define _S_SSS_S 0x5D
#define _S_SSSS_ 0x5E
#define _S_SSSSS 0x5F
#define _SS_____ 0x60
#define _SS____S 0x61
#define _SS___S_ 0x62
#define _SS___SS 0x63
#define _SS__S__ 0x64
#define _SS__S_S 0x65
#define _SS__SS_ 0x66
#define _SS__SSS 0x67
#define _SS_S___ 0x68
#define _SS_S__S 0x69
#define _SS_S_S_ 0x6A
#define _SS_S_SS 0x6B
#define _SS_SS__ 0x6C
#define _SS_SS_S 0x6D
#define _SS_SSS_ 0x6E
#define _SS_SSSS 0x6F
#define _SSS____ 0x70
#define _SSS___S 0x71
#define _SSS__S_ 0x72
#define _SSS__SS 0x73
#define _SSS_S__ 0x74
#define _SSS_S_S 0x75
#define _SSS_SS_ 0x76
#define _SSS_SSS 0x77
#define _SSSS___ 0x78
#define _SSSS__S 0x79
#define _SSSS_S_ 0x7A
#define _SSSS_SS 0x7B
#define _SSSSS__ 0x7C
#define _SSSSS_S 0x7D
#define _SSSSSS_ 0x7E
#define _SSSSSSS 0x7F
#define S_______ 0x80
#define S______S 0x81
#define S_____S_ 0x82
#define S_____SS 0x83
#define S____S__ 0x84
#define S____S_S 0x85
#define S____SS_ 0x86
#define S____SSS 0x87
#define S___S___ 0x88
#define S___S__S 0x89
#define S___S_S_ 0x8A
#define S___S_SS 0x8B
#define S___SS__ 0x8C
#define S___SS_S 0x8D
#define S___SSS_ 0x8E
#define S___SSSS 0x8F
#define S__S____ 0x90
#define S__S___S 0x91
#define S__S__S_ 0x92
#define S__S__SS 0x93
#define S__S_S__ 0x94
#define S__S_S_S 0x95
#define S__S_SS_ 0x96
#define S__S_SSS 0x97
#define S__SS___ 0x98
#define S__SS__S 0x99
#define S__SS_S_ 0x9A
#define S__SS_SS 0x9B
#define S__SSS__ 0x9C
#define S__SSS_S 0x9D
#define S__SSSS_ 0x9E
#define S__SSSSS 0x9F
#define S_S_____ 0xA0
#define S_S____S 0xA1
#define S_S___S_ 0xA2
#define S_S___SS 0xA3
#define S_S__S__ 0xA4
#define S_S__S_S 0xA5
#define S_S__SS_ 0xA6
#define S_S__SSS 0xA7
#define S_S_S___ 0xA8
#define S_S_S__S 0xA9
#define S_S_S_S_ 0xAA
#define S_S_S_SS 0xAB
#define S_S_SS__ 0xAC
#define S_S_SS_S 0xAD
#define S_S_SSS_ 0xAE
#define S_S_SSSS 0xAF
#define S_SS____ 0xB0
#define S_SS___S 0xB1
#define S_SS__S_ 0xB2
#define S_SS__SS 0xB3
#define S_SS_S__ 0xB4
#define S_SS_S_S 0xB5
#define S_SS_SS_ 0xB6
#define S_SS_SSS 0xB7
#define S_SSS___ 0xB8
#define S_SSS__S 0xB9
#define S_SSS_S_ 0xBA
#define S_SSS_SS 0xBB
#define S_SSSS__ 0xBC
#define S_SSSS_S 0xBD
#define S_SSSSS_ 0xBE
#define S_SSSSSS 0xBF
#define SS______ 0xC0
#define SS_____S 0xC1
#define SS____S_ 0xC2
#define SS____SS 0xC3
#define SS___S__ 0xC4
#define SS___S_S 0xC5
#define SS___SS_ 0xC6
#define SS___SSS 0xC7
#define SS__S___ 0xC8
#define SS__S__S 0xC9
#define SS__S_S_ 0xCA
#define SS__S_SS 0xCB
#define SS__SS__ 0xCC
#define SS__SS_S 0xCD
#define SS__SSS_ 0xCE
#define SS__SSSS 0xCF
#define SS_S____ 0xD0
#define SS_S___S 0xD1
#define SS_S__S_ 0xD2
#define SS_S__SS 0xD3
#define SS_S_S__ 0xD4
#define SS_S_S_S 0xD5
#define SS_S_SS_ 0xD6
#define SS_S_SSS 0xD7
#define SS_SS___ 0xD8
#define SS_SS__S 0xD9
#define SS_SS_S_ 0xDA
#define SS_SS_SS 0xDB
#define SS_SSS__ 0xDC
#define SS_SSS_S 0xDD
#define SS_SSSS_ 0xDE
#define SS_SSSSS 0xDF
#define SSS_____ 0xE0
#define SSS____S 0xE1
#define SSS___S_ 0xE2
#define SSS___SS 0xE3
#define SSS__S__ 0xE4
#define SSS__S_S 0xE5
#define SSS__SS_ 0xE6
#define SSS__SSS 0xE7
#define SSS_S___ 0xE8
#define SSS_S__S 0xE9
#define SSS_S_S_ 0xEA
#define SSS_S_SS 0xEB
#define SSS_SS__ 0xEC
#define SSS_SS_S 0xED
#define SSS_SSS_ 0xEE
#define SSS_SSSS 0xEF
#define SSSS____ 0xF0
#define SSSS___S 0xF1
#define SSSS__S_ 0xF2
#define SSSS__SS 0xF3
#define SSSS_S__ 0xF4
#define SSSS_S_S 0xF5
#define SSSS_SS_ 0xF6
#define SSSS_SSS 0xF7
#define SSSSS___ 0xF8
#define SSSSS__S 0xF9
#define SSSSS_S_ 0xFA
#define SSSSS_SS 0xFB
#define SSSSSS__ 0xFC
#define SSSSSS_S 0xFD
#define SSSSSSS_ 0xFE
#define SSSSSSSS 0xFF

/**
 * Bitmap font glyph structure.
 */
typedef struct cgifh_glyph {
	int advance;
	uint8_t data[CGIFH_GLYPH_HEIGHT];
} cgifh_glyph_t;

/**
 * Bitmap font data.
 */
static const cgifh_glyph_t font_h8[] = {
	['a'] = {
		.advance = 5,
		.data = {
			________,
			SSS_____,
			___S____,
			_SSS____,
			S__S____,
			_SSS____,
			________,
			________,
		},
	},
	['b'] = {
		.advance = 6,
		.data = {
			S_______,
			SSSS____,
			S___S___,
			S___S___,
			S___S___,
			SSSS____,
			________,
			________,
		},
	},
	['c'] = {
		.advance = 6,
		.data = {
			________,
			_SSS____,
			S___S___,
			S_______,
			S___S___,
			_SSS____,
			________,
			________,
		},
	},
	['d'] = {
		.advance = 6,
		.data = {
			____S___,
			_SSSS___,
			S___S___,
			S___S___,
			S___S___,
			_SSSS___,
			________,
			________,
		},
	},
	['e'] = {
		.advance = 6,
		.data = {
			________,
			_SSS____,
			S___S___,
			SSSS____,
			S_______,
			_SSSS___,
			________,
			________,
		},
	},
	['f'] = {
		.advance = 4,
		.data = {
			_SS_____,
			S_______,
			SSS_____,
			S_______,
			S_______,
			S_______,
			________,
			________,
		},
	},
	['g'] = {
		.advance = 5,
		.data = {
			________,
			_SS_____,
			S__S____,
			S__S____,
			S__S____,
			_SSS____,
			___S____,
			SSS_____,
		},
	},
	['h'] = {
		.advance = 5,
		.data = {
			S_______,
			SSS_____,
			S__S____,
			S__S____,
			S__S____,
			S__S____,
			________,
			________,
		},
	},
	['i'] = {
		.advance = 2,
		.data = {
			S_______,
			________,
			S_______,
			S_______,
			S_______,
			S_______,
			________,
			________,
		},
	},
	['j'] = {
		.advance = 3,
		.data = {
			_S______,
			________,
			_S______,
			_S______,
			_S______,
			_S______,
			_S______,
			S_______,
		},
	},
	['k'] = {
		.advance = 5,
		.data = {
			S_______,
			S__S____,
			S_S_____,
			SS______,
			S_S_____,
			S__S____,
			________,
			________,
		},
	},
	['l'] = {
		.advance = 2,
		.data = {
			S_______,
			S_______,
			S_______,
			S_______,
			S_______,
			S_______,
			________,
			________,
		},
	},
	['m'] = {
		.advance = 6,
		.data = {
			________,
			SSSS____,
			S_S_S___,
			S_S_S___,
			S___S___,
			S___S___,
			________,
			________,
		},
	},
	['n'] = {
		.advance = 5,
		.data = {
			________,
			SSS_____,
			S__S____,
			S__S____,
			S__S____,
			S__S____,
			________,
			________,
		},
	},
	['o'] = {
		.advance = 5,
		.data = {
			________,
			_SS_____,
			S__S____,
			S__S____,
			S__S____,
			_SS_____,
			________,
			________,
		},
	},
	['p'] = {
		.advance = 5,
		.data = {
			________,
			SSS_____,
			S__S____,
			S__S____,
			S__S____,
			SSS_____,
			S_______,
			S_______,
		},
	},
	['q'] = {
		.advance = 5,
		.data = {
			________,
			_SS_____,
			S__S____,
			S__S____,
			S__S____,
			_SSS____,
			___S____,
			___S____,
		},
	},
	['r'] = {
		.advance = 5,
		.data = {
			________,
			SSS_____,
			S__S____,
			S_______,
			S_______,
			S_______,
			________,
			________,
		},
	},
	['s'] = {
		.advance = 6,
		.data = {
			________,
			_SSSS___,
			S_______,
			_SSS____,
			____S___,
			SSSS____,
			________,
			________,
		},
	},
	['t'] = {
		.advance = 4,
		.data = {
			S_______,
			SSS_____,
			S_______,
			S_______,
			S_______,
			_SS_____,
			________,
			________,
		},
	},
	['u'] = {
		.advance = 5,
		.data = {
			________,
			S__S____,
			S__S____,
			S__S____,
			S__S____,
			_SSS____,
			________,
			________,
		},
	},
	['v'] = {
		.advance = 6,
		.data = {
			________,
			S___S___,
			S___S___,
			S___S___,
			_S_S____,
			__S_____,
			________,
			________,
		},
	},
	['w'] = {
		.advance = 6,
		.data = {
			________,
			S___S___,
			S_S_S___,
			S_S_S___,
			S_S_S___,
			_SSS____,
			________,
			________,
		},
	},
	['x'] = {
		.advance = 6,
		.data = {
			________,
			S___S___,
			_S_S____,
			__S_____,
			_S_S____,
			S___S___,
			________,
			________,
		},
	},
	['y'] = {
		.advance = 5,
		.data = {
			________,
			S__S____,
			S__S____,
			S__S____,
			S__S____,
			_SSS____,
			___S____,
			SSS_____,
		},
	},
	['z'] = {
		.advance = 5,
		.data = {
			________,
			SSSS____,
			___S____,
			_SS_____,
			S_______,
			SSSS____,
			________,
			________,
		},
	},
	['A'] = {
		.advance = 6,
		.data = {
			__S_____,
			_S_S____,
			S___S___,
			SSSSS___,
			S___S___,
			S___S___,
			________,
			________,
		},
	},
	['B'] = {
		.advance = 6,
		.data = {
			SSSS____,
			S___S___,
			SSSS____,
			S___S___,
			S___S___,
			SSSS____,
			________,
		},
	},
	['C'] = {
		.advance = 6,
		.data = {
			_SSS____,
			S___S___,
			S_______,
			S_______,
			S___S___,
			_SSS____,
			________,
			________,
		},
	},
	['D'] = {
		.advance = 6,
		.data = {
			SSSS____,
			S___S___,
			S___S___,
			S___S___,
			S___S___,
			SSSS____,
			________,
			________,
		},
	},
	['E'] = {
		.advance = 6,
		.data = {
			SSSSS___,
			S_______,
			SSSS____,
			S_______,
			S_______,
			SSSSS___,
			________,
			________,
		},
	},
	['F'] = {
		.advance = 6,
		.data = {
			SSSSS___,
			S_______,
			SSSS____,
			S_______,
			S_______,
			S_______,
			________,
			________,
		},
	},
	['G'] = {
		.advance = 6,
		.data = {
			_SSS____,
			S___S___,
			S_______,
			S__SS___,
			S___S___,
			_SSS____,
			________,
			________,
		},
	},
	['H'] = {
		.advance = 6,
		.data = {
			S___S___,
			S___S___,
			SSSSS___,
			S___S___,
			S___S___,
			S___S___,
			________,
			________,
		},
	},
	['I'] = {
		.advance = 4,
		.data = {
			SSS_____,
			_S______,
			_S______,
			_S______,
			_S______,
			SSS_____,
			________,
			________,
		},
	},
	['J'] = {
		.advance = 5,
		.data = {
			_SSS____,
			___S____,
			___S____,
			___S____,
			S__S____,
			_SS_____,
			________,
			________,
		},
	},
	['K'] = {
		.advance = 6,
		.data = {
			S__S____,
			S_S_____,
			SS______,
			S_S_____,
			S__S____,
			S___S___,
			________,
			________,
		},
	},
	['L'] = {
		.advance = 5,
		.data = {
			S_______,
			S_______,
			S_______,
			S_______,
			S_______,
			SSSS____,
			________,
			________,
		},
	},
	['M'] = {
		.advance = 8,
		.data = {
			S_____S_,
			SS___SS_,
			S_S_S_S_,
			S__S__S_,
			S__S__S_,
			S_____S_,
			________,
			________,
		},
	},
	['N'] = {
		.advance = 6,
		.data = {
			S___S___,
			SS__S___,
			S_S_S___,
			S__SS___,
			S___S___,
			S___S___,
			________,
			________,
		},
	},
	['O'] = {
		.advance = 6,
		.data = {
			_SSS____,
			S___S___,
			S___S___,
			S___S___,
			S___S___,
			_SSS____,
			________,
			________,
		},
	},
	['P'] = {
		.advance = 6,
		.data = {
			SSSS____,
			S___S___,
			SSSS____,
			S_______,
			S_______,
			S_______,
			________,
			________,
		},
	},
	['Q'] = {
		.advance = 6,
		.data = {
			_SSS____,
			S___S___,
			S___S___,
			S___S___,
			_S_S____,
			__SSS___,
			________,
			________,
		},
	},
	['R'] = {
		.advance = 6,
		.data = {
			SSS_____,
			S__S____,
			SSS_____,
			S__S____,
			S___S___,
			S___S___,
			________,
			________,
		},
	},
	['S'] = {
		.advance = 7,
		.data = {
			_SSSS___,
			S____S__,
			_SS_____,
			___SS___,
			S____S__,
			_SSSS___,
			________,
			________,
		},
	},
	['T'] = {
		.advance = 6,
		.data = {
			SSSSS___,
			__S_____,
			__S_____,
			__S_____,
			__S_____,
			__S_____,
			________,
			________,
		},
	},
	['U'] = {
		.advance = 6,
		.data = {
			S___S___,
			S___S___,
			S___S___,
			S___S___,
			S___S___,
			_SSS____,
			________,
			________,
		},
	},
	['V'] = {
		.advance = 6,
		.data = {
			S___S___,
			S___S___,
			S___S___,
			_S_S____,
			_S_S____,
			__S_____,
			________,
			________,
		},
	},
	['W'] = {
		.advance = 8,
		.data = {
			S_____S_,
			S_____S_,
			S__S__S_,
			S_S_S_S_,
			SS___SS_,
			S_____S_,
			________,
			________,
		},
	},
	['X'] = {
		.advance = 6,
		.data = {
			S___S___,
			_S_S____,
			__S_____,
			__S_____,
			_S_S____,
			S___S___,
			________,
			________,
		},
	},
	['Y'] = {
		.advance = 6,
		.data = {
			S___S___,
			S___S___,
			_S_S____,
			__S_____,
			__S_____,
			__S_____,
			________,
			________,
		},
	},
	['Z'] = {
		.advance = 5,
		.data = {
			SSSS____,
			___S____,
			__S_____,
			_S______,
			S_______,
			SSSS____,
			________,
			________,
		},
	},
	['0'] = {
		.advance = 6,
		.data = {
			_SSS____,
			S___S___,
			S__SS___,
			SS__S___,
			S___S___,
			_SSS____,
			________,
			________,
		},
	},
	['1'] = {
		.advance = 6,
		.data = {
			__S_____,
			_SS_____,
			__S_____,
			__S_____,
			__S_____,
			_SSS____,
			________,
			________,
		},
	},
	['2'] = {
		.advance = 6,
		.data = {
			_SSS____,
			S___S___,
			___S____,
			_SS_____,
			S_______,
			SSSSS___,
			________,
			________,
		},
	},
	['3'] = {
		.advance = 6,
		.data = {
			_SSS____,
			S___S___,
			__SS____,
			____S___,
			S___S___,
			_SSS____,
			________,
			________,
		},
	},
	['4'] = {
		.advance = 6,
		.data = {
			___S____,
			__SS____,
			_S_S____,
			S__S____,
			SSSSS___,
			___S____,
			________,
			________,
		},
	},
	['5'] = {
		.advance = 6,
		.data = {
			SSSSS___,
			S_______,
			SSSS____,
			____S___,
			S___S___,
			_SSS____,
			________,
			________,
		},
	},
	['6'] = {
		.advance = 6,
		.data = {
			_SSS____,
			S_______,
			SSSS____,
			S___S___,
			S___S___,
			_SSS____,
			________,
			________,
		},
	},
	['7'] = {
		.advance = 6,
		.data = {
			SSSSS___,
			___S____,
			__S_____,
			_S______,
			_S______,
			_S______,
			________,
			________,
		},
	},
	['8'] = {
		.advance = 6,
		.data = {
			_SSS____,
			S___S___,
			_SSS____,
			S___S___,
			S___S___,
			_SSS____,
			________,
			________,
		},
	},
	['9'] = {
		.advance = 6,
		.data = {
			_SSS____,
			S___S___,
			S___S___,
			_SSSS___,
			____S___,
			_SSS____,
			________,
			________,
		},
	},
	[' '] = {
		.advance = 3,
		.data = {
			________,
			________,
			________,
			________,
			________,
			________,
			________,
			________,
		},
	},
	['!'] = {
		.advance = 2,
		.data = {
			S_______,
			S_______,
			S_______,
			S_______,
			________,
			S_______,
			________,
			________,
		},
	},
	['"'] = {
		.advance = 4,
		.data = {
			S_S_____,
			S_S_____,
			________,
			________,
			________,
			________,
			________,
			________,
		},
	},
	['('] = {
		.advance = 3,
		.data = {
			_S______,
			S_______,
			S_______,
			S_______,
			S_______,
			S_______,
			_S______,
			________,
		},
	},
	[')'] = {
		.advance = 3,
		.data = {
			S_______,
			_S______,
			_S______,
			_S______,
			_S______,
			_S______,
			S_______,
			________,
		},
	},
	[','] = {
		.advance = 3,
		.data = {
			________,
			________,
			________,
			________,
			_S______,
			S_______,
			________,
			________,
		},
	},
	['-'] = {
		.advance = 4,
		.data = {
			________,
			________,
			________,
			SSS_____,
			________,
			________,
			________,
			________,
		},
	},
	['_'] = {
		.advance = 6,
		.data = {
			________,
			________,
			________,
			________,
			________,
			________,
			________,
			SSSSS___,
		},
	},
	['.'] = {
		.advance = 2,
		.data = {
			________,
			________,
			________,
			________,
			________,
			S_______,
			________,
			________,
		},
	},
	[':'] = {
		.advance = 2,
		.data = {
			________,
			________,
			S_______,
			________,
			________,
			S_______,
			________,
			________,
		},
	},
	[';'] = {
		.advance = 3,
		.data = {
			________,
			________,
			_S______,
			________,
			________,
			_S______,
			S_______,
			________,
		},
	},
	['?'] = {
		.advance = 6,
		.data = {
			_SSS____,
			S___S___,
			___S____,
			__S_____,
			________,
			__S_____,
			________,
			________,
		},
	},
	['['] = {
		.advance = 3,
		.data = {
			SS______,
			S_______,
			S_______,
			S_______,
			S_______,
			S_______,
			SS______,
			________,
		},
	},
	[']'] = {
		.advance = 3,
		.data = {
			SS______,
			_S______,
			_S______,
			_S______,
			_S______,
			_S______,
			SS______,
			________,
		},
	},
	['{'] = {
		.advance = 4,
		.data = {
			__S_____,
			_S______,
			_S______,
			SS______,
			_S______,
			_S______,
			__S_____,
			________,
		},
	},
	['}'] = {
		.advance = 4,
		.data = {
			S_______,
			_S______,
			_S______,
			_SS_____,
			_S______,
			_S______,
			S_______,
			________,
		},
	},
};

/**
 * Get the glyph for a character.
 *
 * \param[in] character  Character to get glyph for.
 * \return Pointer to glyph structure or NULL if character is not supported.
 */
static inline const cgifh_glyph_t *cgifh_get_glyph(char character)
{
	if ((unsigned char)character >= CGIFH_ARRAY_LEN(font_h8)) {
		return NULL;
	}

	return &font_h8[(unsigned char)character];
}

/**
 * Draw a scaled character at a given position.
 *
 * \param[in] img       Image to draw on.
 * \param[in] colour    Colour to draw character in.
 * \param[in] character Character to draw.
 * \param[in] scale_x   Horizontal scale factor.
 * \param[in] scale_y   Vertical scale factor.
 * \param[in] x         X coordinate to draw character at.
 * \param[in] y         Y coordinate to draw character at.
 * \return The x-advance for the drawn glyph in pixels.
*/
static inline int cgifh_char_scaled(
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

/**
 * Draw a character at a given position.
 *
 * \param[in] img       Image to draw on.
 * \param[in] colour    Colour to draw character in.
 * \param[in] character Character to draw.
 * \param[in] scale     Scale factor.
 * \param[in] x         X coordinate to draw character at.
 * \param[in] y         Y coordinate to draw character at.
 * \return The x-advance for the drawn glyph in pixels.
 */
static inline int cgifh_char(
		cgifh_t *img,
		uint8_t colour,
		char character,
		int scale,
		int x,
		int y)
{
	return cgifh_char_scaled(img, colour, character, scale, scale, x, y);
}

/**
 * Draw text at a given position.
 *
 * \param[in] img    Image to draw on.
 * \param[in] colour Colour to draw text in.
 * \param[in] text   Text to draw.
 * \param[in] scale  Scale factor.
 * \param[in] x      X coordinate to draw text at.
 * \param[in] y      Y coordinate to draw text at.
 * \return The x-advance for the drawn text in pixels.
 */
static inline int cgifh_text(
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

/**
 * Get the width of given text.
 *
 * \param[in] text  Text to get width of.
 * \param[in] scale Scale factor.
 * \return The width of the text in pixels.
 */
static inline int cgifh_text_width(const char *text, int scale)
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

/**
 * Get the height of given text.
 *
 * \param[in] scale Scale factor.
 * \return The height of the text in pixels.
 */
static inline int cgifh_text_height(int scale)
{
	return CGIFH_GLYPH_HEIGHT * scale;
}

#endif /* CGIFH_H */
