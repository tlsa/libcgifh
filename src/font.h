/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2024 Michael Drake <tlsa@netsurf-browser.org>
 */

#ifndef CGIFH_FONT_H
#define CGIFH_FONT_H

/**
 * \file Font API.
*/

#include <cgifh.h>

/** Glyph width in pixels. */
#define CGIFH_GLYPH_WIDTH 8

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
extern const cgifh_glyph_t font_h8['~'];

#endif /* CGIFH_FONT_H */
