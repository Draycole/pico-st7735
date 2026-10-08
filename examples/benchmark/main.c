#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "st7735.h"

#define N 20

// runs a statement n times and gives the average time in microseconds
#define TIME_AVG_US(n, ...) ({                       \
    uint64_t _t0 = time_us_64();                     \
    for (int _i = 0; _i < (n); _i++) { __VA_ARGS__; }\
    (time_us_64() - _t0) / (n);                      \
})

// ideal = wire-only time: (11 window bytes + 2 bytes per pixel) x 8 bits / baud
static void report(const char *name, uint64_t measured_us, uint32_t pixels) {
    double ideal_us = ((11.0 + 2.0 * pixels) * 8.0) / spi_get_baudrate(SPI_PORT) * 1e6;
    printf("%-22s %6llu us | ideal %6.0f us | %5.1f%% | %.2f us/px\n",
           name, (unsigned long long)measured_us, ideal_us,
           100.0 * ideal_us / measured_us, (double)measured_us / pixels);
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    st7735_init();
    st7735_fill_screen(COLOR_BLACK);

    const char *msg = "ST7735 driver on Pico";   // 21 characters

    uint64_t fill_us   = TIME_AVG_US(5, st7735_fill_screen(COLOR_BLUE));
    uint64_t char_us   = TIME_AVG_US(N, draw_char(30, 20, '8', COLOR_WHITE, COLOR_BLACK));
    uint64_t scaled_us = TIME_AVG_US(N, draw_char_scaled(60, 20, '8', COLOR_WHITE, COLOR_BLACK, 3));
    uint64_t rect_us   = TIME_AVG_US(N, draw_rect(10, 80, 30, 30, COLOR_RED));
    uint64_t str_us    = TIME_AVG_US(N, draw_string(1, 20, msg, COLOR_WHITE, COLOR_BLACK, 1));

    while (1) {
        printf("-- pico-st7735 benchmark at %u Hz --\n", spi_get_baudrate(SPI_PORT));
        report("fill_screen",       fill_us,   128 * 160);
        report("draw_char",         char_us,   35);
        report("draw_char_scaled3", scaled_us, 315);
        report("draw_rect 30x30",   rect_us,   900);
        report("draw_string (21)",  str_us,    882);
        sleep_ms(3000);
    }
}