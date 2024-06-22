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

/** Maximum number of palette entries. */
#define CGIFH_PALETTE_MAX 256

/** Number of colour channels in the image palette. */
#define CGIFH_CHANNEL_COUNT 3

/** Glyph height in pixels. */
#define CGIFH_GLYPH_HEIGHT 8

/**
 * CGIF Helper image structure.
 */
typedef struct cgifh {
	/** RGB palette */
	uint8_t palette[CGIFH_CHANNEL_COUNT * CGIFH_PALETTE_MAX];
	/** Number of entries in the palette. */
	uint16_t palette_count;

	int width;    /**< Image width in pixels. */
	int height;   /**< Image height in pixels. */
	size_t size;  /**< Image data size in bytes. */
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
bool cgifh_palette_add(cgifh_t *img,
		uint8_t r,
		uint8_t g,
		uint8_t b,
		uint8_t *idx_out);

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
bool cgifh_palette_add_blend(cgifh_t *img,
		uint8_t idx0,
		uint8_t idx1,
		uint8_t pos,
		uint8_t *idx_out);

/**
 * Create an image.
 *
 * \param[in] width  Image width in pixels.
 * \param[in] height Image height in pixels.
 * \return Pointer to the new image, or NULL on failure.
 */
cgifh_t *cgifh_create(size_t width, size_t height);

/**
 * Destroy an image.
 *
 * \param[in] img The image to destroy.
 */
void cgifh_destroy(cgifh_t *img);

/**
 * Draw a vertical line.
 *
 * \param[in] img    The image to draw the line in.
 * \param[in] colour The palette index of the colour to draw the line in.
 * \param[in] y0     The top y coordinate of the line.
 * \param[in] y1     The bottom y coordinate of the line.
 * \param[in] x      The x coordinate of the line.
 */
void cgifh_v_line(
		cgifh_t *img,
		uint8_t colour,
		int y0,
		int y1,
		int x);

/**
 * Draw a horizontal line.
 *
 * \param[in] img    The image to draw the line in.
 * \param[in] colour The palette index of the colour to draw the line in.
 * \param[in] x0     The left x coordinate of the line.
 * \param[in] x1     The right x coordinate of the line.
 * \param[in] y      The y coordinate of the line.
 */
void cgifh_h_line(
		cgifh_t *img,
		uint8_t colour,
		int x0,
		int x1,
		int y);

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
void cgifh_line(
		cgifh_t *img,
		uint8_t colour,
		int x0, int y0,
		int x1, int y1);

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
void cgifh_rect_fill(
		cgifh_t *img,
		uint8_t colour,
		int x, int y,
		int w, int h);

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
int cgifh_char(
		cgifh_t *img,
		uint8_t colour,
		char character,
		int scale,
		int x,
		int y);

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
int cgifh_char_scaled(
		cgifh_t *img,
		uint8_t colour,
		char character,
		int scale_x,
		int scale_y,
		int x,
		int y);

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
int cgifh_text(
		cgifh_t *img,
		uint8_t colour,
		const char *text,
		int scale,
		int x,
		int y);

/**
 * Get the width of given text.
 *
 * \param[in] text  Text to get width of.
 * \param[in] scale Scale factor.
 * \return The width of the text in pixels.
 */
int cgifh_text_width(const char *text, int scale);

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
