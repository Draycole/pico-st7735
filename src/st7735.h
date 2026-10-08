#ifndef ST7735_H
#define ST7735_H

#include <stdint.h>

#define PIN_DC   17
#define PIN_RES  16
#define PIN_CS   15
#define SPI_PORT spi0

// st7735 command bytes dictated by the datasheet
#define ST7735_SWRESET  0x01   // software reset
#define ST7735_SLPOUT   0x11   // sleep out
#define ST7735_COLMOD   0x3A   // color format
#define ST7735_MADCTL   0x36   // memory data access control
#define ST7735_CASET    0x2A   // column address set
#define ST7735_RASET    0x2B   // row address set
#define ST7735_RAMWR    0x2C   // memory write
#define ST7735_DISPON   0x29   // display on
#define ST7735_NORON    0x13   // normal display mode on

// (16-bit RGB565 format)
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F

// functions
void st7735_init(void);
void st7735_fill_screen(uint16_t color);
void draw_pixel(uint8_t x, uint8_t y, uint16_t color);
void draw_rect(uint8_t x0, uint8_t y0, uint8_t len, uint8_t wid, uint16_t color);
void draw_char(uint8_t x, uint8_t y, char c, uint16_t fg, uint16_t bg);
void draw_char_scaled(uint8_t x, uint8_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale);
void draw_string(uint8_t x, uint8_t y, const char *str, uint16_t fg, uint16_t bg, uint8_t scale);
void draw_filled_circle(uint8_t cx, uint8_t cy, uint8_t r, uint16_t color);
uint32_t st7735_set_baudrate(uint32_t hz);

#endif