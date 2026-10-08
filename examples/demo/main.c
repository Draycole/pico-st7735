// demo code: shows the driver's main features on a 128x160 ST7735 display.
// wiring info is listed in the top-level README.

#include "pico/stdlib.h"
#include "st7735.h"

int main(void) {
    st7735_init();
    st7735_fill_screen(COLOR_BLACK);
    sleep_ms(1000);
    st7735_fill_screen(COLOR_RED);
    sleep_ms(1000);

    draw_char(5, 5, 'E', COLOR_WHITE, COLOR_RED); sleep_ms(500);
    draw_char_scaled(15, 5, 'E', COLOR_BLUE, COLOR_RED, 2); sleep_ms(500);
    draw_char_scaled(30, 5, 'E', COLOR_BLACK, COLOR_RED, 3); sleep_ms(500);
    draw_char_scaled(55, 5, 'E', COLOR_GREEN, COLOR_RED, 4); sleep_ms(500);
    draw_char_scaled(85, 5, 'E', COLOR_WHITE, COLOR_RED, 5); sleep_ms(500);
    draw_pixel(60, 30, COLOR_GREEN); sleep_ms(500);
    draw_pixel(60, 40, COLOR_BLUE); sleep_ms(500);
    draw_rect(2, 50, 100, 50, COLOR_BLUE); sleep_ms(500);
    draw_filled_circle(117, 75, 9, COLOR_GREEN); sleep_ms(500);
    draw_string(5, 120, "Munachimso Henry", COLOR_WHITE, COLOR_RED, 1); sleep_ms(500);
    draw_string(5, 140, "-- (c) 2026", COLOR_WHITE, COLOR_RED, 1);

    while (1) {
        tight_loop_contents();
    }
}