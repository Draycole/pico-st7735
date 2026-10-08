// font5x7.h
// 5x7 bitmap font covering printable ASCII (32 ' ' to 126 '~').
// The glyph data itself lives in font5x7.c.

#ifndef FONT5X7_H
#define FONT5X7_H

#include <stdint.h>

#define FONT_FIRST_CHAR  32                                   // ' '
#define FONT_LAST_CHAR   126                                  // '~'
#define FONT_CHAR_COUNT  (FONT_LAST_CHAR - FONT_FIRST_CHAR + 1)  // 95 glyphs total
#define FONT_WIDTH       5                                    // glyph width in pixels
#define FONT_HEIGHT      7                                    // glyph height in pixels
#define FONT_ADVANCE     6                                    // glyph width + 1 pixel gap

// font5x7[c - FONT_FIRST_CHAR][row]: one byte per row, top to bottom.
// Only the low 5 bits are used: bit 4 is the leftmost pixel, bit 0 the rightmost.
extern const uint8_t font5x7[FONT_CHAR_COUNT][FONT_HEIGHT];

#endif