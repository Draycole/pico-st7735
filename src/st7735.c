#include "st7735.h"
#include "pico/stdlib.h"
#include "font5x7.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#define MAX_SCALE 6

static inline void cs_select(void) {
    gpio_put(PIN_CS, 0);
}

static inline void cs_deselect(void) {
    gpio_put(PIN_CS, 1);
}

static inline void dc_command(void) {
    gpio_put(PIN_DC, 0);
}

static inline void dc_data(void) {
    gpio_put(PIN_DC, 1);
}

static void write_command(uint8_t cmd) {
    cs_select();
    dc_command();
    spi_write_blocking(SPI_PORT, &cmd, 1);
    cs_deselect();
}

// single data byte
static void write_data(uint8_t data) {
    cs_select();
    dc_data();
    spi_write_blocking(SPI_PORT, &data, 1);
    cs_deselect();
}

static void st7735_reset(void) {
    gpio_put(PIN_RES, 1);
    sleep_ms(10);
    gpio_put(PIN_RES, 0);  // pull reset LOW
    sleep_ms(10);
    gpio_put(PIN_RES, 1);  // release
    sleep_ms(150);          
}

// new inuput for d second grade optimization
static void write_data_buf(const uint8_t *data, size_t len) {
    cs_select();
    dc_data();
    spi_write_blocking(SPI_PORT, data, len);
    cs_deselect();
}


static void st7735_set_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    uint8_t col[4] = {0x00, x0, 0x00, x1};
    uint8_t row[4] = {0x00, y0, 0x00, y1};

    write_command(ST7735_CASET);
    write_data_buf(col, 4);
    write_command(ST7735_RASET);
    write_data_buf(row, 4);
    write_command(ST7735_RAMWR);
}

uint32_t st7735_set_baudrate(uint32_t hz) {
    return spi_set_baudrate(SPI_PORT, hz);   // returns the rate actually achieved
}

// start streaming 16-bit pixels (call after set_window)
static inline void pixels_begin(void) {
    spi_set_format(SPI_PORT, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    cs_select();
    dc_data();
}

// finish streaming and return to 8-bit frames for commands
static inline void pixels_end(void) {
    cs_deselect();
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

void draw_pixel(uint8_t x, uint8_t y, uint16_t color){
    st7735_set_window(x, y, x, y);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;    

    cs_select();
    dc_data();

    spi_write_blocking(SPI_PORT, &hi, 1);
    spi_write_blocking(SPI_PORT, &lo, 1);

    cs_deselect();
}

void draw_filled_circle(uint8_t cx, uint8_t cy, uint8_t r, uint16_t color) {
    // calculate the bounding box boundaries
    int start_x = (cx - r < 0) ? 0 : cx - r;
    int end_x   = (cx + r >= 128) ? 127 : cx + r;
    int start_y = (cy - r < 0) ? 0 : cy - r;
    int end_y   = (cy + r >= 160) ? 159 : cy + r;

    for (int x = start_x; x <= end_x; x++) {
        for (int y = start_y; y <= end_y; y++) {
            int dx = x - cx;
            int dy = y - cy;
            if (dx*dx + dy*dy <= r*r) {
                draw_pixel(x, y, color);
            }
        }
    }
}

void draw_rect(uint8_t x0, uint8_t y0, uint8_t len, uint8_t wid, uint16_t color) {
    if (len == 0 || wid == 0 || x0 >= 128 || y0 >= 160) return;
    if (x0 + len > 128) len = 128 - x0;      // clip to the screen
    if (y0 + wid > 160) wid = 160 - y0;

    st7735_set_window(x0, y0, x0 + len - 1, y0 + wid - 1);

    uint16_t row[128];
    for (int i = 0; i < len; i++) row[i] = color;

    pixels_begin();
    for (int y = 0; y < wid; y++) {
        spi_write16_blocking(SPI_PORT, row, len);
    }
    pixels_end();
}

void st7735_fill_screen(uint16_t color) {
    draw_rect(0, 0, 128, 160, color);        // one code path for both
}

void draw_char_scaled(uint8_t x, uint8_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale) {
    if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR) return;       
    if (scale == 0 || scale > MAX_SCALE) return;

    const int w = 5 * scale;
    const int h = 7 * scale;
    if (x + w > 128 || y + h > 160) return;   // don't draw if it won't fit

    static uint16_t buf[(5 * MAX_SCALE) * (7 * MAX_SCALE)];
    const uint8_t *glyph = font5x7[c - FONT_FIRST_CHAR];

    // build the glyph's pixels in RAM, each font pixel becoming scale x scale pixels
    for (int row = 0; row < 7; row++) {
        for (int sy = 0; sy < scale; sy++) {
            for (int col = 0; col < 5; col++) {
                uint16_t color = ((glyph[row] >> (4 - col)) & 1) ? fg : bg;
                for (int sx = 0; sx < scale; sx++) {
                    buf[(row * scale + sy) * w + col * scale + sx] = color;
                }
            }
        }
    }

    st7735_set_window(x, y, x + w - 1, y + h - 1);
    pixels_begin();
    spi_write16_blocking(SPI_PORT, buf, w * h);
    pixels_end();
}

void draw_char(uint8_t x, uint8_t y, char c, uint16_t fg, uint16_t bg) {
    draw_char_scaled(x, y, c, fg, bg, 1);
}

// draws one line of text (no wrapping): n characters starting at s.
// the caller guarantees the line fits within the screen width.
static void draw_text_line(uint8_t x, uint8_t y, const char *s, int n,
                           uint16_t fg, uint16_t bg, uint8_t scale) {
    const int w = n * FONT_ADVANCE * scale;
    const int h = FONT_HEIGHT * scale;
    uint16_t row_buf[128];

    st7735_set_window(x, y, x + w - 1, y + h - 1);   // one window for the whole line
    pixels_begin();

    for (int row = 0; row < FONT_HEIGHT; row++) {
        // build one pixel row across all the characters
        int px = 0;
        for (int i = 0; i < n; i++) {
            char c = s[i];
            if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR) c = '?';
            uint8_t bits = font5x7[c - FONT_FIRST_CHAR][row];

            for (int col = 0; col < FONT_ADVANCE; col++) {
                // columns 0-4 are the glyph, column 5 is the gap
                uint16_t color = (col < FONT_WIDTH && ((bits >> (4 - col)) & 1)) ? fg : bg;
                for (int sx = 0; sx < scale; sx++) row_buf[px++] = color;
            }
        }
        
        for (int sy = 0; sy < scale; sy++) {
            spi_write16_blocking(SPI_PORT, row_buf, w);
        }
    }
    pixels_end();
}

void draw_string(uint8_t x, uint8_t y, const char *str,
                 uint16_t fg, uint16_t bg, uint8_t scale) {
    if (scale == 0 || scale > MAX_SCALE || x >= 128) return;

    const int adv = FONT_ADVANCE * scale;
    const int max_chars = (128 - x) / adv;       // characters that fit on one line
    if (max_chars == 0) return;
    const int line_h = (FONT_HEIGHT + 1) * scale;  // 7 rows of glyph + 1 row of spacing

    int cy = y;
    while (*str) {
        if (cy + FONT_HEIGHT * scale > 160) return;   // no room for another line

        int n = 0;                                    // characters on this line
        while (str[n] && str[n] != '\n' && n < max_chars) n++;

        if (n > 0) draw_text_line(x, cy, str, n, fg, bg, scale);

        str += n;
        if (*str == '\n') str++;                      // skip the newline itself
        cy += line_h;
    }
}

// initialisation 
void st7735_init(void) {

    // spi peripheral at 8MHz, mode 0
    spi_init(SPI_PORT, 8000000);
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    
    gpio_set_function(18, GPIO_FUNC_SPI);  // sck
    gpio_set_function(19, GPIO_FUNC_SPI);  // mosi

    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_init(PIN_RES);
    gpio_set_dir(PIN_RES, GPIO_OUT);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);

    cs_deselect();  // start with CS high (display not selected)

    // hardware reset
    st7735_reset();

    // command sequence — wake the display up
    write_command(ST7735_SWRESET);  // software reset
    sleep_ms(150);

    write_command(ST7735_SLPOUT);   // exit sleep mode
    sleep_ms(500);                  // datasheet requires 500ms after SLPOUT

    write_command(ST7735_COLMOD);   // set color format
    write_data(0x05);               // 0x05 = 16-bit RGB565 color

    write_command(ST7735_MADCTL);   // memory access control (orientation)
    write_data(0xC0);               // default orientation for now | corrected to accoutn for orientation mixup

    write_command(ST7735_NORON);    // normal display mode
    sleep_ms(10);

    write_command(ST7735_DISPON);   // turn display on
    sleep_ms(100);
}